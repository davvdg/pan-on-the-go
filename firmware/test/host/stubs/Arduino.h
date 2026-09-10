// Stub Arduino minimal pour compiler PadScanner.cpp sur l'hôte (g++), sans
// toolchain ESP32. Le temps est VIRTUEL : delayMicroseconds() fait avancer une
// horloge simulée, ce qui rend les tests de roulement déterministes et
// instantanés.
#pragma once
#include <stdint.h>

#include <cstddef>
#include <cstdio>

#define INPUT 0
#define OUTPUT 1
#define LOW 0
#define HIGH 1
#define MSBFIRST 1
#define SPI_MODE0 0

namespace sim {
extern int64_t nowUs;          // horloge virtuelle
extern uint32_t drivenMask;    // masque de pads actuellement à HIGH
uint32_t wristBits();          // modèle de contact, défini par le test
}  // namespace sim

inline void pinMode(int, int) {}
inline void digitalWrite(int, int) {}
inline int digitalRead(int) { return 0; }
inline void delay(unsigned long ms) { sim::nowUs += (int64_t)ms * 1000; }
inline void delayMicroseconds(unsigned int us) { sim::nowUs += (int64_t)us; }

struct SerialStub {
  template <typename... A>
  void printf(A...) {}
  template <typename... A>
  void println(A...) {}
  template <typename... A>
  void print(A...) {}
  void begin(unsigned long) {}
};
extern SerialStub Serial;
