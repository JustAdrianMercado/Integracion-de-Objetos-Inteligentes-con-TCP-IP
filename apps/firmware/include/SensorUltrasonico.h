#pragma once

#include <Arduino.h>

class SensorUltrasonico {
public:
    SensorUltrasonico(uint8_t pinDisparo, uint8_t pinEco);

    void iniciar();
    float medirDistanciaCm();

private:
    uint8_t pinDisparo_;
    uint8_t pinEco_;
};
