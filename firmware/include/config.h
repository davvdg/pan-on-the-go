#pragma once
#include <stdint.h>

// ---------------------------------------------------------------------------
// pan-on-the-go — configuration matérielle et réglages de détection
//
// Tout ce qui se calibre au banc est ici. Voir docs/architecture.md pour le
// pourquoi de chaque valeur.
// ---------------------------------------------------------------------------

// --- Brochage ---------------------------------------------------------------
// Les 74HC595 sont pilotés en SPI matériel (VSPI).
static constexpr int PIN_SR_DATA  = 23;  // MOSI  -> SER   (broche 14 du 1er 595)
static constexpr int PIN_SR_CLK   = 18;  // SCK   -> SRCLK (broche 11)
static constexpr int PIN_SR_LATCH = 5;   // GPIO  -> RCLK  (broche 12, tous en //)
// OE (13) à la masse, SRCLR (10) au +3V3, sur tous les 595.

// Entrée bracelet. DOIT être 34..39 (entrées seules, pas de pull interne
// parasite) : on veut un nœud propre avec notre pull-down externe de 1 MΩ.
static constexpr int PIN_WRIST_A  = 34;

// Second canal de détection, sur GPIO35 avec son propre pull-down de 1 MΩ.
//
// ⚠️ Un SEUL bracelet suffit pour les DEUX baguettes : le corps est un
// conducteur unique, la baguette gauche arrive au bracelet droit par le torse
// (~1 kΩ, négligeable). Et pour la même raison, un second bracelet ne permet
// PAS de distinguer les mains — il verrait exactement la même chose.
//
// Ce canal ne sert donc que si l'électrode B est une BAGUETTE FILAIRE (fil
// dans le manche, sans passer par le corps). Câbler les deux baguettes et
// abandonner le bracelet donne alors l'attribution main gauche / main droite.
static constexpr bool USE_WRIST_B = false;
static constexpr int  PIN_WRIST_B = 35;

static constexpr int PIN_LED = 2;  // LED embarquée, clignote à chaque frappe

// Horloge SPI. 4 MHz et non 40 : la nappe vers les 595 est longue et non
// adaptée, on privilégie la propreté des fronts à la vitesse. Le scan reste
// dominé par SETTLE_US de toute façon.
static constexpr uint32_t SR_SPI_HZ = 4000000;

// --- Détection --------------------------------------------------------------

// Temps de stabilisation après un changement d'état des pads, en µs.
// C'est τ = R_corps × C_nœud (typiquement 30 µs), on prend ~3τ.
// À CALIBRER AU BANC : baisse-le tant que la détection reste fiable, chaque µs
// gagné est multiplié par PAD_COUNT sur la durée du scan.
static constexpr uint32_t SETTLE_US = 100;

// Temps mort par pad après une frappe. Un roulement de pan monte à ~15
// frappes/s sur une même note, soit 66 ms de période : 12 ms laisse largement
// passer les roulements tout en tuant les rebonds mécaniques (1-3 ms).
// NE PAS monter à 50 ms, ça écrêterait les roulements.
static constexpr uint32_t LOCKOUT_US = 12000;

// Nombre de scans consécutifs sans contact exigés avant de pouvoir redéclencher
// un pad. Évite qu'un contact maintenu ne se transforme en mitraillette.
static constexpr uint8_t RELEASE_SCANS = 2;

// --- MIDI -------------------------------------------------------------------

// Détection binaire : pas de vélocité mesurée. Voir docs/architecture.md
// § upgrades pour l'ajout d'un piézo.
static constexpr uint8_t FIXED_VELOCITY = 100;

// Un son de pan est un one-shot résonant : la durée de gate n'a aucun sens
// musical, et le contact ne dure que 5-20 ms. On envoie donc un note-off
// automatique à durée fixe plutôt que de suivre le relâchement.
static constexpr uint32_t NOTE_LEN_MS = 100;

static constexpr uint8_t MIDI_CHANNEL = 0;  // 0-15 (canal 1 en affichage)

// --- Transport --------------------------------------------------------------
// Sélectionné par les build_flags de platformio.ini, pas ici.
#if !defined(TRANSPORT_SERIAL) && !defined(TRANSPORT_WS) && !defined(TRANSPORT_BLE)
#error "Aucun transport sélectionné. Utilise -e esp32-serial, esp32-ws ou esp32-ble."
#endif

#ifdef TRANSPORT_WS
static constexpr uint16_t WS_PORT = 81;
#endif

#ifdef TRANSPORT_BLE
static constexpr const char* BLE_DEVICE_NAME = "PanOnTheGo";
#endif
