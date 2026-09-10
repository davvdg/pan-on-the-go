# Architecture

## Le problème

29 notes à détecter, sur une plaque de carton, avec un ESP32 qui a une quinzaine de GPIO
utilisables. Et une baguette qui ne reste en contact avec le pad que **2 à 5 ms** — c'est le
temps de rebond d'une pointe dure sur une surface rigide.

Ces deux contraintes dictent tout ce qui suit.

## L'inversion

L'approche naïve : 29 pads = 29 entrées, avec des pull-ups de 1 à 10 MΩ (il faut être très
au-dessus de la résistance du corps humain). Ça donne 29 fils longs et haute impédance,
c'est-à-dire 29 antennes à 50 Hz. Et il n'y a de toute façon pas 29 GPIO.

Mais le bracelet est le **seul** point de mesure. Donc on inverse le sens :

```
ESP32                    4× 74HC595                    carton
                                                       
GPIO23 ──MOSI──────────► SER ─┐                        ┌─ pad 0
GPIO18 ──SCK───────────► SRCLK├─ 32 sorties ──────────►├─ pad 1
GPIO5  ──LATCH─────────► RCLK ┘                        ├─ ...
                                                       └─ pad 28
                                                            │
                                                     (pointe laiton)
                                                            │
                                                    (tige métal → paume)
                                                            │
                                                      (bracelet alu)
                                                            │
GPIO34 ◄────────────────────────────────────────────────────┘
   │
  1 MΩ
   │
  GND
```

On allume un ou plusieurs pads, on regarde si le courant arrive au poignet.

Ce que ça change :

- **4 GPIO au total** au lieu de 30+
- les 28 pads non testés sont tirés **activement à la masse** : ils font blindage, la
  diaphonie s'effondre
- **un seul** nœud haute impédance à fiabiliser, pas 29
- les pads sont *pilotés* (impédance de sortie ~50 Ω), donc insensibles au ronflement
- **deuxième main = +1 fil + GPIO35**, aucun changement du câblage des pads

### Sécurité

3,3 V continu, quelques microampères à travers le corps. C'est trois ordres de grandeur sous
le seuil de perception. La résistance corporelle en jeu (paume → poignet, ~100-500 kΩ dominée
par le contact de peau) limite de toute façon le courant à ~30 µA au pire.

## Détection en deux temps

L'identification ne peut pas être sur le chemin de l'horodatage : elle est trop lente.
On découple donc.

**1. Veille rapide.** Les 29 sorties à HIGH *simultanément*, lecture du poignet. ~200 µs.
Tant que c'est LOW, rien n'est touché. Cette boucle tourne à ~5 kHz.

**2. Horodatage.** Front montant → `esp_timer_get_time()` immédiatement, précision ~0,1 ms.
C'est cet instant-là que portera la note MIDI, pas celui de l'identification.

**3. Identification.** *Ensuite* on cherche quel pad. La note émise garde le timestamp
de l'étape 2.

Résultat : le timing enregistré reste sub-milliseconde, quel que soit le temps que met
l'identification.

## Pourquoi la dichotomie et pas un balayage

Un balayage pad par pad coûte `29 × SETTLE_US ≈ 3,4 ms`. C'est **le même ordre de grandeur
que la durée du contact** (2-5 ms) : une frappe sèche serait détectée puis perdue avant
d'avoir été identifiée.

La dichotomie coupe l'ensemble des candidats en deux à chaque étape : `log₂(29) ≈ 5` étapes,
**~600 µs**, mesuré à **1000 µs** avec les lectures de référence. Marge confortable.

Deux pads simultanés (accord, ou les deux baguettes) : la dichotomie converge vers l'un des
deux, on l'exclut et on recommence tant que le reste répond encore.

### Le piège de la dichotomie

> ⚠️ La dichotomie converge **toujours** vers un bit unique — y compris quand le contact a
> disparu en cours de route. Chaque test répond alors « non », on élimine des candidats, et
> la boucle sort sur `popcount == 1` en croyant avoir trouvé. **Ce pad-là n'a jamais été
> touché.**

Sans confirmation du candidat, une frappe très courte produit une note **aléatoire** sur un
pad voisin. Sur le carton, ça se manifesterait par « de temps en temps une note bizarre », et
on accuserait la diaphonie ou le bracelet pendant des heures.

D'où le `senseMask(search, handBit)` final avant de retenir le candidat, dans
`PadScanner::identifyHand`. Le test `Contact fugace 0,4 ms` du banc verrouille ce
comportement.

## Détection synchrone

Chaque décision de veille compare **deux** lectures : pads à HIGH, puis pads à LOW. Un contact
réel *suit* le pilotage ; le ronflement 50 Hz et le couplage capacitif d'une baguette qui
survole, non.

C'est ce qui rend un nœud à 1 MΩ exploitable sans Schmitt trigger. L'identification est
encadrée par deux lectures de référence (tous pads à LOW → le poignet doit lire LOW) : si un
parasite apparaît avant ou pendant, tout le scan est jeté et recompté dans `noiseAborts`.

## Debounce et roulements

Un roulement de pan monte à ~15 frappes/s **sur une même note**, soit 66 ms de période. Le
rebond mécanique, lui, dure 1 à 3 ms. La fenêtre est large, mais il ne faut pas se tromper de
côté :

- `LOCKOUT_US = 12000` — temps mort par pad. **Ne pas monter à 50 ms**, ça écrêterait les
  roulements.
- `RELEASE_SCANS = 2` — nombre de scans sans contact exigés avant de pouvoir redéclencher.
  Empêche un contact maintenu de se transformer en mitraillette.

Quand la veille dit « rien touché nulle part », **tous** les pads sont relâchés par
construction : le suivi de relâchement est donc gratuit (un `memset`).

## Durée de note

Le contact dure 5-20 ms. Un son de pan est un one-shot résonant : la durée de gate n'a aucun
sens musical. On n'essaie donc pas de suivre le relâchement — `MidiOut` programme un note-off
à **+100 ms** fixe.

Sur un roulement, la note est retriggée alors que la précédente résonne encore : `MidiOut`
envoie alors le note-off **avant** le nouveau note-on, sinon le synthé se retrouve avec une
voix orpheline que le note-off suivant couperait au mauvais moment.

## Transports

Les trois émettent **exactement les mêmes octets MIDI**, derrière `Transport.h`. Seul
l'emballage change — c'est ce qui permet de déboguer en série au banc puis de basculer en BLE
sur le téléphone sans toucher une ligne du scanner.

| env | usage | format |
|---|---|---|
| `esp32-serial` | bring-up, banc, scripts de vérification | JSON par ligne |
| `esp32-ws` | dev sur le LAN depuis le laptop | binaire : `[µs uint32 LE][MIDI]` |
| `esp32-ble` | production, téléphone | BLE-MIDI 1.0 |

> ⚠️ Sur ESP32 classique, **WiFi et BLE partagent la radio**. Un seul environnement à la fois.

> ⚠️ Une page en `https://` ne peut pas ouvrir un `ws://` (contenu mixte). Donc : WebSocket
> depuis une page `http://` (laptop), BLE depuis une page `https://` (téléphone).

### Pourquoi BLE-MIDI et pas du JSON sur WebSocket

La trame BLE-MIDI porte **nativement** un horodatage 13 bits en millisecondes :

```
[0] header    = 0x80 | (ms >> 7) & 0x3F
[1] timestamp = 0x80 | (ms      & 0x7F)
[2..] message MIDI brut
```

On horodate à la source (étape 2 ci-dessus) et le récepteur reconstruit le timing exact : le
jitter de 10-20 ms du BLE **disparaît complètement à l'enregistrement**. C'est ce qui sauve
l'usage « enregistrer un morceau ».

Le compteur reboucle toutes les 8192 ms ; le récepteur doit dérouler ces bouclages
(cf. `unwrap()` dans `web/index.html`).

## Budget de latence

| Étage | Coût |
|---|---|
| Veille | ~0,2 ms |
| Identification | ~1 ms *(mesuré au banc)* |
| BLE-MIDI, ESP32 classique | ~10-20 ms |
| **Sortie Web Audio, Chrome Android** | **40-150 ms** |

Le goulot n'est pas là où on l'attend. Conséquence :

- **Enregistrement** : problème résolu par l'horodatage à la source. Aucun impact.
- **Jeu live** : à mesurer avant de construire (étape 2 de la méthode). Si c'est mauvais :
  `new AudioContext({latencyHint:'interactive'})`, sinon jouer depuis un laptop, sinon
  accepter que l'enregistrement soit l'usage principal.

## Calibration

`SETTLE_US` est le seul paramètre vraiment physique : c'est `3τ` avec `τ = R_corps × C_nœud`
(~30 µs typique). Valeur de départ 100 µs, à réduire tant que la détection reste fiable —
chaque µs gagné est multiplié par le nombre d'étapes.

Les compteurs imprimés toutes les 5 s le pilotent :

| Compteur | Signification | Action |
|---|---|---|
| `ghost` | détecté en veille, aucun pad identifié | **baisse `SETTLE_US`** |
| `noise` | parasite vu sur une lecture de référence | bracelet mal en contact, ou pull-down absent |
| `scan_us` | durée réelle de l'identification | doit rester bien sous 2000 |

## Upgrades laissées ouvertes

- **Vélocité** — 1 piézo collé sous la plaque sur GPIO36 (ADC1_CH0). Le contact dit *quelle*
  note, le piézo dit *quelle force*. +1 €, aucun recâblage.
- **Deux mains** — +1 bracelet sur GPIO35, `USE_WRIST_B = true`. Le scanner gère déjà les
  deux canaux ; permet de distinguer main gauche et main droite.
- **USB-MIDI plug-and-play** — remplacer le WROOM-32 par un **ESP32-S3** (~8 €). Le scan 595
  est identique, seul `transport/` gagne un `UsbMidiTransport.cpp` (TinyUSB, mode
  `USB OTG`). Chrome Android expose alors le pan comme périphérique MIDI class-compliant via
  Web MIDI, sans appairage.
