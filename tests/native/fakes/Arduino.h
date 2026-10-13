#pragma once

#include <cstdint>

using std::uint8_t;
constexpr uint8_t INPUT = 0;
constexpr uint8_t OUTPUT = 1;
constexpr uint8_t LOW = 0;
constexpr uint8_t HIGH = 1;

void pinMode(uint8_t pin, uint8_t modo);
void digitalWrite(uint8_t pin, uint8_t valor);
void delayMicroseconds(unsigned int duracion);
unsigned long pulseIn(uint8_t pin, uint8_t valor, unsigned long timeout);
