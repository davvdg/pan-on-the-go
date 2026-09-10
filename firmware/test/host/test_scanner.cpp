// ---------------------------------------------------------------------------
// Banc de test hôte du PadScanner. Compile en g++, sans toolchain ESP32 :
//
//   ./firmware/test/host/run.sh
//
// Il rejoue le VRAI PadScanner.cpp contre des stubs Arduino/SPI/GPIO dans
// lesquels le temps est virtuel : delayMicroseconds() fait avancer une horloge
// simulée. Les tests de roulement à 25 frappes/s s'exécutent donc
// instantanément et de façon parfaitement déterministe.
//
// Ce qu'on vérifie, c'est exactement ce qui est difficile à déboguer une fois
// le carton monté : rebonds, roulements, temps mort, contacts courts,
// simultanéité, précision de l'horodatage.
// ---------------------------------------------------------------------------

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "Arduino.h"
#include "PadScanner.h"
#include "SPI.h"
#include "soc/gpio_struct.h"

// --- Modèle physique simulé -------------------------------------------------

namespace sim {
int64_t nowUs = 0;
uint32_t drivenMask = 0;

struct Contact {
  uint8_t pad;
  uint8_t hand;  // 0 = bracelet A
  int64_t t0, t1;
};
std::vector<Contact> contacts;

// Le poignet lit HIGH si un pad actuellement PILOTÉ est en contact MAINTENANT.
// C'est exactement le comportement physique : le courant part du 595, traverse
// la pointe, la baguette et le corps, et revient sur l'entrée.
uint32_t wristBits() {
  uint32_t bits = 0;
  for (const auto& c : contacts) {
    if (nowUs < c.t0 || nowUs >= c.t1) continue;
    if (!(drivenMask & (1u << c.pad))) continue;
    const int pin = (c.hand == 0) ? PIN_WRIST_A : PIN_WRIST_B;
    bits |= 1u << (pin - 32);
  }
  return bits;
}
}  // namespace sim

SimGpio GPIO;
SPIStub SPI;
SerialStub Serial;

// --- Harnais ----------------------------------------------------------------

struct Hit {
  uint8_t pad, hand;
  int64_t t;
};
static std::vector<Hit> hits;
static PadScanner scanner;
static int failures = 0;
static uint32_t identifyUs = 0;  // durée d'identification d'une VRAIE frappe
static int64_t base = 0;         // origine des temps du test courant

static void onHit(uint8_t pad, uint8_t hand, int64_t t) {
  hits.push_back({pad, hand, t});
  // lastScanUs_ est renseigné juste avant l'appel du callback : c'est donc la
  // durée d'identification de CETTE frappe, et pas celle d'un scan à vide.
  identifyUs = scanner.lastScanUs();
}

static void reset() {
  // On ne démarre pas à 0 : lastHitUs_ vaut 0 après begin(), et un temps de
  // départ nul rendrait le tout premier test du temps mort ambigu.
  sim::nowUs = 1000000;
  base = sim::nowUs;
  sim::contacts.clear();
  hits.clear();
  scanner.begin(onHit);
}

// Programme un contact : `at` et `dur` sont en µs RELATIFS au début du test.
static void contact(uint8_t pad, int64_t at, int64_t dur, uint8_t hand = 0) {
  sim::contacts.push_back({pad, hand, base + at, base + at + dur});
}

static void run(int64_t durationUs) {
  const int64_t end = sim::nowUs + durationUs;
  while (sim::nowUs < end) scanner.poll();
}

static void check(bool ok, const std::string& what) {
  printf("  %s %s\n", ok ? "\033[32mOK  \033[0m" : "\033[31mFAIL\033[0m", what.c_str());
  if (!ok) failures++;
}

static int hitsOnPad(uint8_t pad) {
  int n = 0;
  for (const auto& h : hits)
    if (h.pad == pad) n++;
  return n;
}

#define TEST(name) printf("\n\033[1m%s\033[0m\n", name), reset()

// --- Tests ------------------------------------------------------------------

int main() {
  printf("PAD_COUNT=%u  SETTLE_US=%u  LOCKOUT_US=%u  RELEASE_SCANS=%u\n", (unsigned)PAD_COUNT,
         (unsigned)SETTLE_US, (unsigned)LOCKOUT_US, (unsigned)RELEASE_SCANS);

  {
    TEST("Frappe simple : un contact de 8 ms produit exactement une note");
    contact(5, 10000, 8000);
    run(200000);
    check(hits.size() == 1, "une seule note emise (obtenu " + std::to_string(hits.size()) + ")");
    check(hits.size() == 1 && hits[0].pad == 5, "sur le bon pad");
  }

  {
    TEST("Diaphonie : aucun pad voisin ne se déclenche");
    contact(5, 10000, 8000);
    run(200000);
    bool clean = true;
    for (const auto& h : hits)
      if (h.pad != 5) clean = false;
    check(clean, "zero note parasite sur un autre pad");
  }

  {
    TEST("Horodatage : la note porte l'instant de la FRAPPE, pas de l'identification");
    contact(27 % PAD_COUNT, 50000, 8000);
    run(200000);
    const int64_t err = hits.empty() ? -1 : (hits[0].t - (base + 50000));
    check(!hits.empty() && err >= 0 && err < 1000,
          "erreur = " + std::to_string(err) + " us (attendu < 1000)");
    printf("       identification mesuree : %u us (budget : duree du contact, 2-5 ms)\n",
           identifyUs);
  }

  {
    TEST("Contact maintenu 1 s : une seule note, pas de mitraillette");
    contact(5, 10000, 1000000);
    run(1200000);
    check(hits.size() == 1, "une seule note (obtenu " + std::to_string(hits.size()) + ")");
  }

  {
    TEST("Temps mort : deux frappes espacees de 10 ms (< LOCKOUT 12 ms) -> une seule");
    contact(5, 10000, 5000);
    contact(5, 20000, 5000);
    run(200000);
    check(hits.size() == 1, "rebond filtre (obtenu " + std::to_string(hits.size()) + ")");
  }

  {
    TEST("Roulement 15 frappes/s pendant 3 s : aucune note perdue");
    const int n = 45;
    for (int k = 0; k < n; k++) {
      contact(5, 20000 + (int64_t)k * 66667, 8000);
    }
    run(3200000);
    check(hitsOnPad(5) == n,
          "45 notes attendues, " + std::to_string(hitsOnPad(5)) + " recues");
  }

  {
    TEST("Roulement rapide 25 frappes/s : encore aucune note perdue");
    const int n = 50;
    for (int k = 0; k < n; k++) {
      contact(5, 20000 + (int64_t)k * 40000, 6000);
    }
    run(2200000);
    check(hitsOnPad(5) == n,
          "50 notes attendues, " + std::to_string(hitsOnPad(5)) + " recues");
  }

  {
    TEST("Contact court 2 ms (rebond sec d'une pointe dure) : doit passer");
    // C'est la raison d'etre de la dichotomie. Un balayage lineaire des 29 pads
    // prendrait ~3,4 ms et raterait cette frappe.
    contact(PAD_COUNT - 1, 10000, 2000);
    run(200000);
    check(hits.size() == 1,
          "frappe seche capturee (obtenu " + std::to_string(hits.size()) + ")");
  }

  {
    TEST("Contact fugace 0,4 ms : jamais de note sur un pad NON touche");
    // Regression : la dichotomie converge toujours vers un bit unique, meme si
    // le contact disparait en cours de route. Sans confirmation du candidat,
    // elle inventait une note sur un pad arbitraire. Ici le contact est plus
    // court que l'identification : le seul resultat acceptable est 0 note, ou 1
    // note sur le bon pad -- jamais une note ailleurs.
    const uint8_t target = 7 % PAD_COUNT;
    for (int k = 0; k < 40; k++) contact(target, 10000 + (int64_t)k * 25000, 400);
    run(1200000);
    bool clean = true;
    for (const auto& h : hits)
      if (h.pad != target) clean = false;
    check(clean, "aucune note fantome (" + std::to_string(hits.size()) + " note(s), toutes sur le pad " +
                     std::to_string((int)target) + ")");
  }

  {
    TEST("Deux pads simultanes (accord) : deux notes distinctes");
    contact(3, 10000, 10000);
    contact(PAD_COUNT > 20 ? 20 : 1, 10000, 10000);
    run(200000);
    check(hits.size() == 2, "deux notes (obtenu " + std::to_string(hits.size()) + ")");
  }

  {
    TEST("Pad le plus haut et pad le plus bas : le mapping couvre toute la chaine");
    contact(0, 10000, 8000);
    contact(PAD_COUNT - 1, 40000, 8000);
    run(200000);
    check(hitsOnPad(0) == 1, "pad 0 (premier 595, QA)");
    check(hitsOnPad(PAD_COUNT - 1) == 1, "pad " + std::to_string(PAD_COUNT - 1) + " (dernier 595)");
  }

  printf("\n%s %d echec(s)\n", failures ? "\033[31mECHEC\033[0m" : "\033[32mTOUT PASSE\033[0m",
         failures);
  return failures ? 1 : 0;
}
