# pan-on-the-go

Un steelpan tenor en carton qui envoie du MIDI à [panisto](https://github.com/davvdg/panisto).

Une plaque de carton avec les 29 notes d'un tenor imprimées à l'échelle 1:1, des pastilles
en ruban alu, deux baguettes à pointe métallique, un bracelet conducteur, un ESP32.
On frappe, ça devient des notes MIDI — pour jouer en synthèse, et surtout pour enregistrer
les notes d'un morceau.

## Où en est le projet

| | |
|---|---|
| Firmware ESP32 | ✅ écrit, 13/13 tests de logique passent |
| Générateur de gabarit | ✅ valide sur le tenor de panisto (29 notes, C4→E6) |
| Page de test web | ✅ écrite (BLE + WebSocket) |
| Compilation PlatformIO | ⬜ jamais lancée (toolchain pas installée) |
| Montage carton | ⬜ |
| Intégration panisto | ⬜ |
| Phase 2 — pliage | 🔬 contraintes calculées, cinématique à trancher |

## Comment ça marche

Le bracelet est le **seul** point de mesure, donc on inverse le sens habituel : les 29 pads
sont des **sorties** (4× 74HC595, 3 broches), le poignet est l'unique **entrée**. On allume
des pads, on regarde si le courant arrive au poignet en traversant la baguette et le corps.

Ça donne 4 GPIO au total au lieu de 30, et les pads inactifs — tirés activement à la masse —
font blindage. Détail complet dans [docs/architecture.md](docs/architecture.md).

## Démarrage

```bash
# 1. Logique de détection — quelques secondes, aucune toolchain requise
./firmware/test/host/run.sh

# 2. Gabarit + mapping, générés depuis le SVG de panisto
gh repo clone davvdg/panisto ../panisto
node tools/gen-template.mjs --scale 0.847     # 0.847 = taille d'un vrai tenor

# 3. Firmware (étape 1 de la méthode : banc 4 pads, port série)
pio run -e esp32-serial -t upload && pio device monitor
```

## L'ordre compte

Chaque étape est validable seule. **On ne découpe pas 29 pastilles avant d'avoir prouvé la
chaîne sur 4.** Détail dans [docs/build-v0.md](docs/build-v0.md).

0. *(optionnel, en parallèle)* Test A/B tuile rigide vs ruban alu — cf. [docs/folding.md](docs/folding.md)
1. Banc 4 pads, transport `Serial` → le corps ferme-t-il bien le circuit ? calibrer `SETTLE_US`
2. Mesurer la latence audio et tester un roulement → **décision go/no-go sur le jeu live**
3. Générer et imprimer le gabarit → vérifier l'échelle au réglet
4. Monter les 29 pads
5. `web/index.html` → valider tout le câblage sans toucher à panisto
6. Intégrer dans panisto

## Arborescence

```
firmware/     PlatformIO, ESP32. 3 environnements = 3 transports (serial / ws / ble)
  test/host/  banc de test de la logique de détection, compile en g++
tools/        gen-template.mjs   : SVG panisto -> gabarit + padmap.h + padmap.json
              analyse-folding.mjs : où peuvent passer les lignes de pli
web/          page de test standalone (BLE + WebSocket + grille de pads)
docs/         architecture, montage, nomenclature, pliage (phase 2)
```

## Ce qu'il faut savoir avant de s'y mettre

- L'ESP32 WROOM-32 **n'a pas d'USB natif** (le port USB passe par un pont CP2102/CH340).
  Pas d'USB-MIDI possible → le transport de production est **BLE-MIDI**.
- La détection est **binaire** : pas de vélocité. Un piézo l'ajoute plus tard sans recâbler.
- Le vrai risque du projet n'est pas le carton, c'est la **latence de sortie Web Audio sur
  Chrome Android** (40-150 ms). Sans effet sur l'enregistrement, potentiellement rédhibitoire
  pour le jeu live. À mesurer à l'étape 2, avant de construire.
