#pragma once
#include <stdint.h>

#include "config.h"
#include "transport/Transport.h"

// ---------------------------------------------------------------------------
// MidiOut — transforme une frappe en couple note-on / note-off.
//
// Un son de pan est un one-shot résonant : la durée pendant laquelle la
// baguette touche le pad (5-20 ms) n'a aucun sens musical. On n'essaie donc pas
// de suivre le relâchement — on programme un note-off à durée fixe.
// ---------------------------------------------------------------------------

class MidiOut {
 public:
  void begin(Transport* t) { t_ = t; }

  void noteOn(uint8_t note, uint8_t vel, int64_t tUs);

  // À appeler à chaque tour de loop() : purge les note-off arrivés à échéance.
  void tick();

  // Coupe tout (déconnexion, changement de config).
  void allNotesOff();

 private:
  static constexpr uint8_t MAX_PENDING = 32;

  struct Pending {
    int64_t dueUs;
    uint8_t note;
    bool used;
  };

  void sendOff(uint8_t note, int64_t tUs);
  int find(uint8_t note) const;
  int freeSlot() const;

  Transport* t_ = nullptr;
  Pending pending_[MAX_PENDING] = {};
};
