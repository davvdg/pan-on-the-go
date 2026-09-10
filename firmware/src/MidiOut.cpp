#include "MidiOut.h"

#include "esp_timer.h"

int MidiOut::find(uint8_t note) const {
  for (int i = 0; i < MAX_PENDING; i++) {
    if (pending_[i].used && pending_[i].note == note) return i;
  }
  return -1;
}

int MidiOut::freeSlot() const {
  for (int i = 0; i < MAX_PENDING; i++) {
    if (!pending_[i].used) return i;
  }
  return -1;
}

void MidiOut::sendOff(uint8_t note, int64_t tUs) {
  if (!t_) return;
  const uint8_t msg[3] = {(uint8_t)(0x80 | MIDI_CHANNEL), note, 0};
  t_->sendMidi(msg, 3, tUs);
}

void MidiOut::noteOn(uint8_t note, uint8_t vel, int64_t tUs) {
  if (!t_) return;

  // Retrigger pendant un roulement : la note résonne encore et son note-off est
  // programmé. On la coupe MAINTENANT avant de la relancer, sinon le synthé se
  // retrouve avec une voix orpheline que le note-off suivant coupera au mauvais
  // moment (il couperait la nouvelle frappe, pas l'ancienne).
  const int prev = find(note);
  if (prev >= 0) {
    sendOff(note, tUs);
    pending_[prev].used = false;
  }

  const uint8_t msg[3] = {(uint8_t)(0x90 | MIDI_CHANNEL), note, vel};
  t_->sendMidi(msg, 3, tUs);

  const int slot = freeSlot();
  if (slot >= 0) {
    pending_[slot].note = note;
    pending_[slot].dueUs = tUs + (int64_t)NOTE_LEN_MS * 1000;
    pending_[slot].used = true;
  } else {
    // Ne devrait pas arriver : MAX_PENDING = 32 >= PAD_COUNT et le retrigger
    // recycle le slot. Par sécurité on coupe tout de suite plutôt que de
    // laisser une note pendue.
    sendOff(note, tUs);
  }
}

void MidiOut::tick() {
  const int64_t now = esp_timer_get_time();
  for (int i = 0; i < MAX_PENDING; i++) {
    if (!pending_[i].used || now < pending_[i].dueUs) continue;
    // Horodaté à l'échéance voulue, pas à l'instant réel du balayage : le
    // note-off tombe juste même si loop() a pris du retard.
    sendOff(pending_[i].note, pending_[i].dueUs);
    pending_[i].used = false;
  }
}

void MidiOut::allNotesOff() {
  const int64_t now = esp_timer_get_time();
  for (int i = 0; i < MAX_PENDING; i++) {
    if (!pending_[i].used) continue;
    sendOff(pending_[i].note, now);
    pending_[i].used = false;
  }
}
