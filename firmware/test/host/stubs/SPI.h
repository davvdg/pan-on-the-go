#pragma once
#include <stdint.h>

#include <cstddef>

#include "Arduino.h"

struct SPISettings {
  SPISettings(uint32_t, uint8_t, uint8_t) {}
};

struct SPIStub {
  void begin(int, int, int, int) {}
  void beginTransaction(SPISettings) {}
  void endTransaction() {}

  // Reconstruit le masque écrit par PadScanner::writePads : le premier octet
  // envoyé va dans le 595 le plus lointain (poids fort).
  void writeBytes(const uint8_t* buf, size_t n) {
    uint32_t m = 0;
    for (size_t i = 0; i < n; i++) m |= (uint32_t)buf[i] << (8 * (n - 1 - i));
    sim::drivenMask = m;
  }
};

extern SPIStub SPI;
