#pragma once
#include <stddef.h>
#include <stdint.h>

// ---------------------------------------------------------------------------
// Interface commune aux trois transports.
//
// Les trois émettent EXACTEMENT le même flux d'octets MIDI ; seul l'emballage
// change. C'est ce qui permet de déboguer en Serial au banc puis de basculer en
// BLE sur le téléphone sans toucher une ligne du scanner ni du parseur côté web.
// ---------------------------------------------------------------------------

class Transport {
 public:
  virtual ~Transport() {}

  virtual void begin() = 0;
  virtual void loop() {}
  virtual bool ready() const = 0;

  // msg : message MIDI brut (3 octets pour note on/off).
  // tUs : instant de la FRAPPE en µs depuis le boot — pas l'instant d'envoi.
  //       Les transports qui savent transporter un horodatage (BLE-MIDI) s'en
  //       servent pour que le jitter de la liaison disparaisse à l'arrivée.
  virtual void sendMidi(const uint8_t* msg, size_t len, int64_t tUs) = 0;

  // Ligne d'état lisible, affichée au boot et sur demande.
  virtual const char* name() const = 0;
};
