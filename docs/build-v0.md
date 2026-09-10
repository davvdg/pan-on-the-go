# Montage v0 — plaque carton, tenor 29 notes

Chaque étape est validable seule. **Ne découpe pas 29 pastilles avant d'avoir prouvé la
chaîne sur 4.** L'étape 1 prend une soirée et répond à la seule vraie question ouverte :
est-ce que le corps ferme correctement le circuit ?

---

## Étape 0 — la logique, sans matériel (5 min)

```bash
./firmware/test/host/run.sh
```

13 tests doivent passer. Ça rejoue le vrai `PadScanner.cpp` avec une horloge virtuelle :
rebonds, roulements à 25 frappes/s, contacts de 2 ms, simultanéité, précision d'horodatage.
Aucune toolchain ESP32 requise.

---

## Étape 1 — banc 4 pads

### Câblage

Un seul 74HC595 suffit.

| 74HC595 | vers |
|---|---|
| 14 `SER` | GPIO23 (via 220 Ω) |
| 11 `SRCLK` | GPIO18 (via 220 Ω) |
| 12 `RCLK` | GPIO5 |
| 13 `OE` | **GND** |
| 10 `SRCLR` | **+3V3** |
| 16 `VCC` | +3V3, avec un 100 nF vers GND au plus près |
| 8 `GND` | GND |
| 15 `QA` … 1 `QD` | pads 0 à 3 |

Et le bracelet :

```
bracelet ──┬────────── GPIO34
           │
          1 MΩ
           │
          GND
```

> Le pull-down **externe** de 1 MΩ n'est pas optionnel : le pull interne de l'ESP32 (~45 kΩ)
> serait bien trop bas face à la résistance du corps, et GPIO34 n'en a de toute façon pas.

### Pads de test

4 carrés de ruban alu adhésif de ~8 cm de côté sur un bout de carton, bien séparés. Une
attache parisienne en laiton traverse chaque pastille et le carton ; au dos, on écarte les
pattes et on y sertit le fil.

### Bracelet

Sangle velcro + ruban alu côté peau, **grande surface** (~25 cm², soit 5×5 cm). C'est ce qui
fait descendre la résistance de contact de plusieurs MΩ à ~100 kΩ. Un fil serti sur l'alu,
vers GPIO34.

### Baguette

Tige métal nue tenue à pleine main, pointe laiton. **Pas de manchon isolant là où tu tiens
la baguette** — c'est ce contact qui ferme le circuit.

### Flash et observation

```bash
pio run -e esp32-serial -t upload && pio device monitor
```

Frappe chaque pad. Tu dois voir :

```
{"t":12345678,"pad":2,"hand":0,"note":64}
[stats] hits=12 scans=41 ghost=0 noise=0 scan_us=300 link=up
```

### Ce qu'on cherche à savoir

| Symptôme | Cause probable | Action |
|---|---|---|
| Rien ne se passe | résistance corps trop haute | agrandir le bracelet ; humidifier la peau pour tester ; sinon `74HC14` + pull-down 10 MΩ |
| `ghost` monte | contact plus court que l'identification | **baisser `SETTLE_US`** dans `config.h` (essaie 50, puis 30) |
| `noise` monte | parasites | vérifier le pull-down 1 MΩ, raccourcir le fil du bracelet |
| Notes sur le mauvais pad | câblage QA..QD inversé | l'ordre est QA = pad 0 |
| Notes en rafale | `RELEASE_SCANS` trop bas | ne devrait pas arriver, les tests le couvrent |

> ✅ **Critère de passage :** 10 frappes sur chaque pad → 10 notes, sur le bon pad,
> `ghost = 0`, `noise = 0`.

---

## Étape 2 — latence et roulements ⚠️ décision go/no-go

Toujours sur 4 pads. Bascule en BLE :

```bash
pio run -e esp32-ble -t upload
```

Ouvre `web/index.html` **en https://** sur le téléphone. Le plus simple : active GitHub Pages
sur ce dépôt (Settings → Pages → branche `main`, dossier `/web`), ce qui donne une URL
`https://davvdg.github.io/pan-on-the-go/`.

Clique **activer le son**, puis **Connecter en BLE**.

Trois nombres à relever :

- **latence audio** — c'est `AudioContext.outputLatency`. **C'est le chiffre qui décide.**
- **jitter liaison** — dispersion de l'écart entre l'horloge ESP32 et celle du navigateur.
  Sans importance pour l'enregistrement (l'horodatage à la source l'annule), significatif
  pour le jeu.
- **débit crête** — fais un roulement soutenu : tu dois monter à 12-15/s sans note perdue.

| Latence audio mesurée | Verdict |
|---|---|
| < 30 ms | jeu live confortable |
| 30-60 ms | jouable, un peu mou |
| > 60 ms | **le live sera frustrant.** L'enregistrement reste parfait. Envisage de jouer depuis un laptop. |

> Si le verdict est mauvais, ça ne condamne pas le projet — ça recentre la v0 sur
> l'enregistrement, qui était de toute façon l'usage le plus solide. Mais mieux vaut le
> savoir **avant** d'avoir découpé 29 pastilles.

---

## Étape 3 — gabarit

```bash
gh repo clone davvdg/panisto ../panisto
node tools/gen-template.mjs --scale 0.847
```

`--scale 0.847` ramène le dessin de panisto (67,7 cm) à **57,4 cm**, la taille d'un vrai
tenor (fût de 55 gallons). C'est important : tout l'intérêt de ce carton est de transférer la
mémoire musculaire vers un vrai instrument.

Ouvre `out/template.html`, imprime en **« Taille réelle » / « 100 % »** — surtout pas
« Ajuster à la page ».

> ✅ **Mesure la règle de 100 mm au réglet avant de découper quoi que ce soit.** Une
> impression à 96 % passe totalement inaperçue et fausse tout le carton.

Les 12 feuilles A4 se recouvrent de 10 mm : superpose les traits pointillés avant de scotcher.

Copie ensuite le mapping généré :

```bash
cp out/padmap_tenor.h firmware/include/
```

et dans `platformio.ini`, remplace `-D PAD_COUNT=4 -D PADMAP_BENCH` par
`-D PAD_COUNT=29 -D PADMAP_TENOR`.

---

## Étape 4 — les 29 pads

1. Colle le gabarit assemblé sur le carton double cannelure (bâton de colle, bien à plat).
2. Pour chaque note : découpe un morceau de ruban alu un peu plus grand que la forme, colle-le
   **à l'intérieur** du trait. Le plus simple est de suivre le contour au cutter après collage.
3. Le grand chiffre au centre de chaque note est son **numéro de pad**, c'est-à-dire sa sortie
   sur la chaîne de 595. Note-le sur la pastille au marqueur avant de la recouvrir.
4. Perce au centre, passe une attache parisienne, écarte les pattes au dos.
5. Câble la nappe au dos en suivant les numéros. L'ordre `--order angle` fait que les pads
   voisins en numéro sont voisins sur le carton : la nappe suit les rayons du pan sans
   croisement.
6. **Isole les pattes de laiton** au chatterton — elles sont à quelques millimètres les unes
   des autres au dos et un court-circuit se traduirait par deux notes qui sonnent ensemble.

> Laisse 2-3 mm de carton nu entre pastilles voisines. Elles ne doivent jamais se toucher.

---

## Étape 5 — validation du câblage

```bash
pio run -e esp32-ble -t upload
```

Ouvre `web/index.html`, connecte, et frappe **chaque pad 10 fois**.

- Les compteurs doivent monter un par un, sur la bonne note.
- **Un compteur qui bouge sur un pad que tu n'as pas touché = diaphonie.** Vérifie l'isolation
  au dos et l'espacement des pastilles.
- Une note qui ne répond jamais = attache parisienne mal sertie, ou fil de nappe inversé.

Le bouton *remettre les compteurs à zéro* permet d'enchaîner les passes.

> ✅ **Critère de passage :** 29 × 10 frappes → 290 notes, chacune sur la bonne note,
> aucun compteur parasite.

---

## Étape 6 — intégration panisto

À faire une fois le carton validé. Le principe : `web/index.html` contient déjà tout ce qu'il
faut (parseur BLE-MIDI, déroulement de l'horodatage 13 bits, parseur MIDI). Il s'agit de le
porter en module TypeScript dans panisto et de le brancher sur deux consommateurs :

1. **Affichage** — `InstrumentCanvas` allume déjà les notes. Le brancher dessus donne un
   retour visuel immédiat, et sert de test de câblage définitif.
2. **Enregistrement** — accumuler les événements en `NotePunch[]`
   (`src/features/Utils/PlayerTypes.tsx`), puis exporter avec `writeMidi()` de `midi-file`,
   déjà en dépendance.

Le mapping est déjà bon : `out/padmap.json` sort du même SVG que celui qu'affiche panisto,
avec le même `tonalOffset`. Rien à ressaisir.
