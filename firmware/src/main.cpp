#include <Arduino.h>

#include "MidiOut.h"
#include "PadScanner.h"
#include "config.h"
#include "esp_timer.h"
#include "padmap.h"

#ifdef TRANSPORT_SERIAL
#include "transport/SerialTransport.h"
static SerialTransport gTransport;
#elif defined(TRANSPORT_WS)
#include "transport/WsTransport.h"
static WsTransport gTransport;
#elif defined(TRANSPORT_BLE)
#include "transport/BleMidiTransport.h"
static BleMidiTransport gTransport;
#endif

static PadScanner gScanner;
static MidiOut gMidi;

static int64_t gLedOffUs = 0;
static int64_t gNextStatsUs = 0;
static uint32_t gHits = 0;

// Appelé depuis PadScanner, avec l'instant de la DÉTECTION (pas celui de
// l'identification qui l'a suivi de ~3 ms).
static void onHit(uint8_t pad, uint8_t hand, int64_t tUs) {
  const uint8_t note = PAD_MIDI_NOTE[pad];
  gHits++;
  gMidi.noteOn(note, FIXED_VELOCITY, tUs);

  digitalWrite(PIN_LED, HIGH);
  gLedOffUs = esp_timer_get_time() + 8000;

  // Ligne de diagnostic, distincte des lignes {"m":[...]} du transport Serial :
  // c'est celle que comptent les scripts de vérification (diaphonie, roulements).
  Serial.printf("{\"t\":%lld,\"pad\":%u,\"hand\":%u,\"note\":%u}\n", (long long)tUs,
                (unsigned)pad, (unsigned)hand, (unsigned)note);
}

void setup() {
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);

  gTransport.begin();
  gMidi.begin(&gTransport);
  gScanner.begin(onHit);

  Serial.println();
  Serial.println("=== pan-on-the-go ===");
  Serial.printf("transport   : %s\n", gTransport.name());
  Serial.printf("pads        : %u  (%u x 74HC595)\n", (unsigned)PAD_COUNT, (unsigned)SR_COUNT);
  Serial.printf("canaux      : %u\n", USE_WRIST_B ? 2u : 1u);
  Serial.printf("settle      : %u us\n", (unsigned)SETTLE_US);
  Serial.printf("lockout     : %u us\n", (unsigned)LOCKOUT_US);
  Serial.printf("note len    : %u ms\n", (unsigned)NOTE_LEN_MS);
  Serial.print("mapping     :");
  for (uint8_t i = 0; i < PAD_COUNT; i++) Serial.printf(" %u->%u", i, PAD_MIDI_NOTE[i]);
  Serial.println();
  Serial.println("=====================");
}

void loop() {
  gScanner.poll();
  gMidi.tick();
  gTransport.loop();

  const int64_t now = esp_timer_get_time();

  if (gLedOffUs && now >= gLedOffUs) {
    digitalWrite(PIN_LED, LOW);
    gLedOffUs = 0;
  }

  // Bilan toutes les 5 s. C'est l'outil de calibration de l'étape 1 :
  //   ghost   > 0  -> des frappes détectées mais non identifiées (contact plus
  //                   court que le scan, ou pad mal câblé)
  //   noise   > 0  -> parasites : bracelet mal en contact, ou pull-down absent
  //   scan_us      -> durée réelle du scan, sert à régler SETTLE_US
  if (now >= gNextStatsUs) {
    gNextStatsUs = now + 5000000;
    Serial.printf("[stats] hits=%u scans=%u ghost=%u noise=%u scan_us=%u link=%s\n", gHits,
                  gScanner.scanCount(), gScanner.ghostScans(), gScanner.noiseAborts(),
                  gScanner.lastScanUs(), gTransport.ready() ? "up" : "down");
  }

  yield();
}
