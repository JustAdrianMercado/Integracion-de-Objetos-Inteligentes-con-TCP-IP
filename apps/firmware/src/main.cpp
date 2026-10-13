#include <Arduino.h>

#include "ControladorLeds.h"
#include "SensorUltrasonico.h"

namespace {
constexpr uint8_t PIN_DISPARO = 5;
constexpr uint8_t PIN_ECO = 18;
constexpr uint8_t PIN_LED_VERDE = 25;
constexpr uint8_t PIN_LED_AMARILLO = 26;
constexpr uint8_t PIN_LED_ROJO = 27;
constexpr unsigned long INTERVALO_MEDICION_MS = 250UL;
constexpr unsigned long VELOCIDAD_SERIAL = 115200UL;

SensorUltrasonico sensor(PIN_DISPARO, PIN_ECO);
ControladorLeds controladorLeds(
    PIN_LED_VERDE, PIN_LED_AMARILLO, PIN_LED_ROJO);

unsigned long ultimaMedicionMs = 0UL;

const char* obtenerNombreRango(RangoDistancia rango) {
    switch (rango) {
        case RangoDistancia::Rojo:
            return "Rojo";
        case RangoDistancia::Amarillo:
            return "Amarillo";
        case RangoDistancia::Verde:
            return "Verde";
        case RangoDistancia::Lejos:
            return "Lejos";
        case RangoDistancia::Invalido:
        default:
            return "Invalido";
    }
}

void mostrarLectura(unsigned long tiempoMs, float distanciaCm) {
    const RangoDistancia rango = controladorLeds.obtenerRangoActual();

    Serial.print("Tiempo: ");
    Serial.print(tiempoMs);
    Serial.print(" ms | ");

    if (rango == RangoDistancia::Invalido) {
        Serial.println("Lectura invalida | Rango: Invalido");
        return;
    }

    Serial.print("Distancia: ");
    Serial.print(distanciaCm, 2);
    Serial.print(" cm | Rango: ");
    Serial.println(obtenerNombreRango(rango));
}
}

void setup() {
    Serial.begin(VELOCIDAD_SERIAL);
    sensor.iniciar();
    controladorLeds.iniciar();
}

void loop() {
    const unsigned long tiempoActual = millis();
    // El controlador se actualiza en cada ciclo para conservar el parpadeo no bloqueante.
    controladorLeds.actualizar(tiempoActual);

    if (tiempoActual - ultimaMedicionMs < INTERVALO_MEDICION_MS) {
        return;
    }

    ultimaMedicionMs = tiempoActual;
    const float distanciaCm = sensor.medirDistanciaCm();
    controladorLeds.establecerDistancia(distanciaCm);

    const unsigned long tiempoLecturaMs = millis();
    controladorLeds.actualizar(tiempoLecturaMs);
    mostrarLectura(tiempoLecturaMs, distanciaCm);
}
