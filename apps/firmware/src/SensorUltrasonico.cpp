#include "SensorUltrasonico.h"

#include <math.h>

namespace {
// Limita la espera de ECHO a 30 ms para detectar la ausencia de un eco válido.
// 30 ms de ida y vuelta equivalen a unos 5 m, más que el alcance útil del HC-SR04.
constexpr unsigned long TIEMPO_MAXIMO_ECO_US = 30000UL;
// 343 m/s a unos 20 °C, expresado en cm/µs.
constexpr float VELOCIDAD_SONIDO_CM_POR_US = 0.0343F;
// El HC-SR04 necesita al menos 10 µs en alto en TRIG para emitir la ráfaga.
constexpr unsigned int DURACION_PULSO_DISPARO_US = 10U;
// TRIG en bajo un instante antes del pulso garantiza un flanco de subida limpio.
constexpr unsigned int PAUSA_PREVIA_DISPARO_US = 2U;
}

SensorUltrasonico::SensorUltrasonico(uint8_t pinDisparo, uint8_t pinEco)
    : pinDisparo_(pinDisparo), pinEco_(pinEco) {
}

void SensorUltrasonico::iniciar() {
    pinMode(pinDisparo_, OUTPUT);
    pinMode(pinEco_, INPUT);
    digitalWrite(pinDisparo_, LOW);
}

float SensorUltrasonico::medirDistanciaCm() {
    digitalWrite(pinDisparo_, LOW);
    delayMicroseconds(PAUSA_PREVIA_DISPARO_US);
    digitalWrite(pinDisparo_, HIGH);
    delayMicroseconds(DURACION_PULSO_DISPARO_US);
    digitalWrite(pinDisparo_, LOW);

    // pulseIn mide cuánto tiempo permanece ECHO en alto; devuelve 0 si vence el plazo.
    const unsigned long duracionEcoUs = pulseIn(
        pinEco_, HIGH, TIEMPO_MAXIMO_ECO_US);

    // Sin eco no hay distancia: NAN se publica como "distance=invalid".
    if (duracionEcoUs == 0UL) {
        return NAN;
    }

    // La duración representa ida y vuelta del sonido; por eso se divide entre dos.
    return duracionEcoUs * VELOCIDAD_SONIDO_CM_POR_US / 2.0F;
}
