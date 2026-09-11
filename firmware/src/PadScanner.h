#pragma once
#include <stdint.h>
#include "config.h"
#include "padmap.h"

// ---------------------------------------------------------------------------
// PadScanner — détection de frappe par contact, pads pilotés / poignet lu.
//
// L'idée centrale : le bracelet est le SEUL point de mesure, donc on inverse le
// sens habituel. Les 29 pads sont des SORTIES (74HC595), le poignet est
// l'unique ENTRÉE. On allume un ou plusieurs pads, on regarde si ça arrive au
// poignet.
//
// Conséquences : 4 GPIO au lieu de 30, les pads inactifs (tirés activement à la
// masse) font blindage, et un seul nœud haute impédance à fiabiliser.
//
// ── La contrainte qui dicte tout le reste ────────────────────────────────────
// Une pointe de baguette rebondit sur une surface dure en 2 à 5 ms de contact.
// C'est notre budget TOTAL pour détecter ET identifier la note, sinon la frappe
// est vue puis perdue.
//
// D'où deux décisions :
//
//  1. Veille en un seul coup (tous les pads allumés, ~200 µs) découplée de
//     l'identification. On HORODATE dès la veille, pas après l'identification.
//
//  2. Identification par DICHOTOMIE et non par balayage. 29 pads balayés un par
//     un coûteraient 3,4 ms — le même ordre de grandeur que le contact
//     lui-même, donc des frappes perdues. La dichotomie descend à 5 étapes,
//     ~0,6 ms, avec une marge confortable.
// ---------------------------------------------------------------------------

class PadScanner {
 public:
  // hand : 0 = canal A (bracelet), 1 = canal B (baguette filaire, si USE_WRIST_B).
  // tHitUs : instant de la DÉTECTION, pas de l'identification.
  using HitCallback = void (*)(uint8_t pad, uint8_t hand, int64_t tHitUs);

  void begin(HitCallback cb);
  void poll();

  // Diagnostics, imprimés toutes les 5 s pour la calibration au banc.
  uint32_t noiseAborts() const { return noiseAborts_; }
  uint32_t ghostScans() const { return ghostScans_; }
  uint32_t scanCount() const { return scanCount_; }
  uint32_t lastScanUs() const { return lastScanUs_; }

 private:
  static constexpr uint8_t HANDS = USE_WRIST_B ? 2 : 1;
  static constexpr uint8_t HAND_MASK = USE_WRIST_B ? 0b11 : 0b01;

  // Garde-fou : nombre de pads simultanés qu'on cherche par main avant
  // d'abandonner. Deux baguettes dans une même main n'existent pas ; 3 couvre
  // le cas tordu d'un contact parasite pendant une frappe.
  static constexpr uint8_t MAX_SIMULTANEOUS = 3;

  void writePads(uint32_t mask);
  uint8_t readWrists() const;
  bool senseMask(uint32_t mask, uint8_t handBit);
  uint32_t identifyHand(uint8_t handBit, uint32_t pool);
  void markAllReleased();
  void identify(uint8_t activeHands, int64_t tHitUs);

  HitCallback cb_ = nullptr;
  int64_t lastHitUs_[2][32] = {};
  uint8_t releaseCount_[2][32] = {};

  uint32_t noiseAborts_ = 0;  // scans jetés : parasite vu sur la référence
  uint32_t ghostScans_ = 0;   // détecté en veille, aucun pad identifié
  uint32_t scanCount_ = 0;
  uint32_t lastScanUs_ = 0;
};
