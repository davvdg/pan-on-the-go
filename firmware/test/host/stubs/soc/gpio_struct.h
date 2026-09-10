#pragma once
#include <stdint.h>

#include "../Arduino.h"

// Les registres d'entrée sont des proxies : leur lecture appelle le modèle de
// contact du test (sim::wristBits), qui dépend du masque de pads actuellement
// piloté et de l'horloge virtuelle.
struct SimInputReg {
  operator uint32_t() const { return sim::wristBits(); }
};

struct SimIn1 {
  SimInputReg val;
};

struct SimGpio {
  SimInputReg in;   // GPIO 0-31 (inutilisé : le poignet est en 34)
  SimIn1 in1;       // GPIO 32-39
  uint32_t out_w1ts = 0;
  uint32_t out_w1tc = 0;
};

extern SimGpio GPIO;
