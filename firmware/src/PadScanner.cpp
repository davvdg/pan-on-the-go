#include "PadScanner.h"

#include <Arduino.h>
#include <SPI.h>
#include <string.h>

#include "esp_timer.h"
#include "soc/gpio_struct.h"

// GPIO 0-31 et 32-39 vivent dans deux registres différents. `pin` est toujours
// une constexpr ici, donc le compilateur replie la branche.
static inline bool fastRead(int pin) {
  if (pin < 32) return (GPIO.in >> pin) & 1;
  return (GPIO.in1.val >> (pin - 32)) & 1;
}

// Moitié « basse » d'un ensemble de candidats : les popcount/2 premiers bits
// à 1. Le découpage exact importe peu, seul compte qu'il coupe en deux.
static inline uint32_t lowerHalf(uint32_t m) {
  int n = __builtin_popcount(m) / 2;
  uint32_t h = 0;
  while (n-- > 0) {
    const uint32_t lsb = m & (~m + 1);
    h |= lsb;
    m &= ~lsb;
  }
  return h;
}

static_assert(PIN_SR_LATCH < 32, "PIN_SR_LATCH doit etre < 32 (registre out_w1ts)");
static_assert(PIN_WRIST_A >= 32,
              "PIN_WRIST_A doit etre 34..39 : entree seule, pas de pull interne");

void PadScanner::begin(HitCallback cb) {
  cb_ = cb;

  pinMode(PIN_SR_LATCH, OUTPUT);
  digitalWrite(PIN_SR_LATCH, LOW);

  // Pull-down externe de 1 MΩ vers la masse : le pull interne de l'ESP32
  // (~45 kΩ) serait bien trop bas face à la résistance du corps.
  pinMode(PIN_WRIST_A, INPUT);
  if (USE_WRIST_B) pinMode(PIN_WRIST_B, INPUT);

  SPI.begin(PIN_SR_CLK, -1 /* pas de MISO */, PIN_SR_DATA, -1 /* CS géré à la main */);
  // Transaction ouverte en permanence : rien d'autre n'utilise VSPI ici, et ça
  // épargne un begin/end à chaque étape (on en fait ~10 000 par seconde).
  SPI.beginTransaction(SPISettings(SR_SPI_HZ, MSBFIRST, SPI_MODE0));

  writePads(0);
  memset(lastHitUs_, 0, sizeof(lastHitUs_));
  memset(releaseCount_, RELEASE_SCANS, sizeof(releaseCount_));
}

// Câblage de la chaîne : le PREMIER octet envoyé finit dans le DERNIER 595.
// On émet donc du 595 le plus lointain vers le plus proche du MOSI.
// En MSBFIRST, le bit b d'un octet atterrit sur la sortie Q[b] (QA = bit 0).
// D'où : pad i  ->  595 numéro (i / 8), sortie Q[i % 8].
void PadScanner::writePads(uint32_t mask) {
  uint8_t buf[4];
  for (uint8_t i = 0; i < SR_COUNT; i++) {
    buf[i] = (uint8_t)((mask >> (8 * (SR_COUNT - 1 - i))) & 0xFF);
  }
  GPIO.out_w1tc = (1UL << PIN_SR_LATCH);
  SPI.writeBytes(buf, SR_COUNT);
  GPIO.out_w1ts = (1UL << PIN_SR_LATCH);  // front montant sur RCLK -> sorties
}

uint8_t PadScanner::readWrists() const {
  uint8_t r = fastRead(PIN_WRIST_A) ? 0b01 : 0;
  if (USE_WRIST_B) r |= fastRead(PIN_WRIST_B) ? 0b10 : 0;
  return r;
}

// Une étape élémentaire : allumer `mask`, laisser stabiliser, lire un poignet.
bool PadScanner::senseMask(uint32_t mask, uint8_t handBit) {
  writePads(mask);
  delayMicroseconds(SETTLE_US);
  return (readWrists() & handBit) != 0;
}

void PadScanner::markAllReleased() {
  // Rien n'est touché nulle part : par construction, TOUS les pads sont
  // relâchés. C'est l'astuce qui rend le suivi de relâchement gratuit.
  memset(releaseCount_, RELEASE_SCANS, sizeof(releaseCount_));
}

void PadScanner::poll() {
  // --- 1. Veille rapide -----------------------------------------------------
  // Tous les pads allumés d'un coup : ~2 × SETTLE_US suffisent à savoir si
  // QUELQUE CHOSE est touché, sans savoir quoi. C'est la boucle chaude, elle
  // tourne à ~5 kHz.
  writePads(ALL_PADS_MASK);
  delayMicroseconds(SETTLE_US);
  const uint8_t hi = readWrists();

  writePads(0);
  delayMicroseconds(SETTLE_US);
  const uint8_t lo = readWrists();

  // Détection synchrone : un contact réel SUIT le pilotage des pads. Le
  // ronflement 50 Hz et le couplage capacitif d'une baguette qui survole, non.
  // C'est ce qui rend un nœud à 1 MΩ exploitable sans Schmitt trigger.
  const uint8_t active = (uint8_t)(hi & ~lo & HAND_MASK);
  if (!active) {
    markAllReleased();
    return;
  }

  // --- 2. Horodatage AVANT identification -----------------------------------
  // Toute la subtilité est là : la note portera CET instant, précis à ~0,1 ms,
  // quel que soit le temps que met l'identification à aboutir.
  const int64_t tHitUs = esp_timer_get_time();

  identify(active, tHitUs);
}

// Dichotomie : trouve les pads touchés par UNE main, dans l'ensemble `pool`.
//
// Invariant d'entrée : on sait déjà que quelque chose est touché (la veille l'a
// dit). On coupe l'ensemble des candidats en deux, on teste une moitié, on garde
// celle qui répond. 29 candidats -> 5 étapes.
//
// Deux pads simultanés (accord, ou deux baguettes) : la dichotomie converge vers
// l'un des deux. On l'exclut et on recommence tant que le reste répond encore.
uint32_t PadScanner::identifyHand(uint8_t handBit, uint32_t pool) {
  uint32_t found = 0;

  for (uint8_t round = 0; round < MAX_SIMULTANEOUS; round++) {
    uint32_t search = pool & ~found;
    if (!search) break;
    if (!senseMask(search, handBit)) break;  // plus rien dans le reste

    while (__builtin_popcount(search) > 1) {
      const uint32_t half = lowerHalf(search);
      if (senseMask(half, handBit)) {
        search = half;
      } else {
        search &= ~half;
      }
    }

    if (!search) break;

    // ⚠️ Confirmation obligatoire du candidat.
    //
    // La dichotomie converge TOUJOURS vers un bit unique, y compris quand le
    // contact s'est évanoui en cours de route : chaque test répond alors « non »,
    // on élimine des candidats, et la boucle sort sur popcount == 1 en croyant
    // avoir trouvé. Ce pad-là n'a jamais été touché.
    //
    // Sans cette ligne, une frappe très courte produit une note ALÉATOIRE sur un
    // pad voisin. C'est indétectable à la lecture du code et infernal à déboguer
    // une fois le carton monté (« de temps en temps une note bizarre »).
    if (!senseMask(search, handBit)) break;

    found |= search;
  }

  return found;
}

void PadScanner::identify(uint8_t activeHands, int64_t tHitUs) {
  const int64_t t0 = esp_timer_get_time();
  uint32_t touched[2] = {0, 0};

  // Référence d'entrée : tous les pads à LOW, le poignet DOIT lire LOW.
  // Sinon c'est un parasite et pas un contact : on jette tout.
  writePads(0);
  delayMicroseconds(SETTLE_US);
  if (readWrists() & activeHands) {
    noiseAborts_++;
    return;
  }

  if (activeHands & 0b01) touched[0] = identifyHand(0b01, ALL_PADS_MASK);
  if (USE_WRIST_B && (activeHands & 0b10)) touched[1] = identifyHand(0b10, ALL_PADS_MASK);

  // Référence de sortie : l'environnement n'a pas bougé pendant l'identification ?
  writePads(0);
  delayMicroseconds(SETTLE_US);
  if (readWrists() & activeHands) {
    noiseAborts_++;
    return;
  }

  scanCount_++;
  lastScanUs_ = (uint32_t)(esp_timer_get_time() - t0);

  bool any = false;
  for (uint8_t h = 0; h < HANDS; h++) {
    for (uint8_t i = 0; i < PAD_COUNT; i++) {
      if (touched[h] & (1UL << i)) {
        any = true;
        const bool released = releaseCount_[h][i] >= RELEASE_SCANS;
        const bool armed = (tHitUs - lastHitUs_[h][i]) >= (int64_t)LOCKOUT_US;
        if (released && armed) {
          lastHitUs_[h][i] = tHitUs;
          if (cb_) cb_(i, h, tHitUs);
        }
        releaseCount_[h][i] = 0;
      } else if (releaseCount_[h][i] < 255) {
        releaseCount_[h][i]++;
      }
    }
  }

  // Détecté en veille mais aucun pad identifié : contact plus court que
  // l'identification, ou pad mal câblé. C'est LE compteur à surveiller au banc —
  // s'il monte, baisse SETTLE_US.
  if (!any) ghostScans_++;
}
