#pragma once
#ifdef TRANSPORT_SERIAL

#include <Arduino.h>

#include "Transport.h"

// ---------------------------------------------------------------------------
// Transport de bring-up : une ligne JSON par message MIDI sur le port série.
// Zéro réseau, zéro appairage — c'est celui de l'étape 1 de la méthode, et
// celui que lisent les scripts de vérification (diaphonie, roulements).
//
//   {"t":1234567,"m":[144,60,100]}
//
//   t : µs depuis le boot de l'ESP32, instant de la FRAPPE
//   m : octets MIDI bruts
// ---------------------------------------------------------------------------

class SerialTransport : public Transport {
 public:
  void begin() override {
    Serial.begin(115200);
    delay(50);
  }

  bool ready() const override { return true; }

  void sendMidi(const uint8_t* msg, size_t len, int64_t tUs) override {
    Serial.printf("{\"t\":%lld,\"m\":[", (long long)tUs);
    for (size_t i = 0; i < len; i++) {
      Serial.printf(i ? ",%u" : "%u", msg[i]);
    }
    Serial.println("]}");
  }

  const char* name() const override { return "serial"; }
};

#endif  // TRANSPORT_SERIAL
