#pragma once

#include <Arduino.h>

// HC-SR04: un pulso en TRIG inicia la medicion y ECHO queda en alto mientras el
// sonido va y vuelve. ECHO llega al ESP32 a traves del divisor de 5 V a 3.3 V.
class SensorUltrasonico {
public:
    SensorUltrasonico(uint8_t pinDisparo, uint8_t pinEco);

    // Configura TRIG como salida en bajo y ECHO como entrada.
    void iniciar();
    // Devuelve la distancia en cm, o NAN si no hubo eco dentro del tiempo maximo.
    float medirDistanciaCm();

private:
    uint8_t pinDisparo_;
    uint8_t pinEco_;
};
