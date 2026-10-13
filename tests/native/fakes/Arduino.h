#pragma once

#include <cstdint>
#include <cstddef>

using std::uint8_t;
using std::uint16_t;
using std::uint32_t;
using std::int32_t;
using std::size_t;
constexpr uint8_t INPUT = 0;
constexpr uint8_t OUTPUT = 1;
constexpr uint8_t LOW = 0;
constexpr uint8_t HIGH = 1;

void pinMode(uint8_t pin, uint8_t modo);
void digitalWrite(uint8_t pin, uint8_t valor);
void delayMicroseconds(unsigned int duracion);
unsigned long pulseIn(uint8_t pin, uint8_t valor, unsigned long timeout);
unsigned long millis();

class SerialSimulado {
public:
    unsigned long velocidad = 0;
    void begin(unsigned long valor) { velocidad = valor; }
    template <typename T> void print(const T&) {}
    template <typename T> void println(const T&) {}
};
extern SerialSimulado Serial;
