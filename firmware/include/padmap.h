#pragma once
#include <stdint.h>
#include "config.h"

// ---------------------------------------------------------------------------
// Table pad → note MIDI.
//
// L'index de pad est l'index de sortie physique sur la chaîne de 74HC595 :
//
//   pad i  ->  595 numéro (i / 8)  ->  sortie Q[i % 8]   (QA = 0 ... QH = 7)
//
// avec le 595 numéro 0 = celui directement relié au MOSI de l'ESP32.
//
// PADMAP_TENOR est GÉNÉRÉ par tools/gen-template.ts depuis le SVG de panisto,
// exactement en même temps que le gabarit imprimé. Ne l'édite pas à la main :
// c'est ce qui garantit que le carton, le firmware et l'app parlent des mêmes
// notes.
// ---------------------------------------------------------------------------

#if defined(PADMAP_BENCH)

// Banc d'essai — étape 1 de la méthode. 4 carrés d'alu, un seul 74HC595.
// Notes choisies pour être audibles et bien distinctes à l'oreille.
static constexpr uint8_t PAD_MIDI_NOTE[] = {
    60,  // pad 0 -> QA -> C4
    62,  // pad 1 -> QB -> D4
    64,  // pad 2 -> QC -> E4
    67,  // pad 3 -> QD -> G4
};

#elif defined(PADMAP_CHROMATIC)

// Table de secours : chromatique D4..F#6, soit exactement 29 demi-tons — la
// tessiture d'un tenor. Permet de câbler et tester les 29 pads AVANT d'avoir
// généré le vrai mapping, et sert aux tests hôte (test/host/).
// Les notes ne tomberont pas aux bons endroits sur le carton : c'est normal,
// c'est un placeholder de câblage, pas un mapping musical.
static constexpr uint8_t PAD_MIDI_NOTE[] = {
    62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76,
    77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90,
};

#elif defined(PADMAP_TENOR)
#include "padmap_tenor.h"  // généré par tools/gen-template.ts, ne pas éditer
#else
#error "Définis PADMAP_BENCH, PADMAP_CHROMATIC ou PADMAP_TENOR dans platformio.ini"
#endif

static_assert(sizeof(PAD_MIDI_NOTE) / sizeof(PAD_MIDI_NOTE[0]) == PAD_COUNT,
              "PAD_COUNT ne correspond pas a la taille de PAD_MIDI_NOTE");
static_assert(PAD_COUNT > 0 && PAD_COUNT <= 32,
              "PAD_COUNT doit tenir dans le masque 32 bits (4 x 74HC595)");

// Nombre de 74HC595 à chaîner pour couvrir PAD_COUNT sorties.
static constexpr uint8_t SR_COUNT = (PAD_COUNT + 7) / 8;

// Masque « tous les pads allumés », utilisé par la boucle de veille rapide.
static constexpr uint32_t ALL_PADS_MASK =
    (PAD_COUNT >= 32) ? 0xFFFFFFFFu : ((1u << PAD_COUNT) - 1u);
