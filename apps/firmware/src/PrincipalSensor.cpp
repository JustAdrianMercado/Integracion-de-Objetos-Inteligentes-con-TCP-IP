#include <Arduino.h>

#include "ClienteTcp.h"
#include "ProtocoloAplicacion.h"
#include "SensorUltrasonico.h"

#if __has_include("ConfiguracionRed.h")
#include "ConfiguracionRed.h"
#else
#include "ConfiguracionRed.example.h"
#endif

namespace {
constexpr uint8_t PIN_DISPARO = 5;
constexpr uint8_t PIN_ECO = 18;
constexpr uint32_t INTERVALO_MEDICION_MS = 250;

SensorUltrasonico sensor(PIN_DISPARO, PIN_ECO);
ClienteTcp cliente(TipoCliente::Sensor, ConfiguracionRed::SSID, ConfiguracionRed::CLAVE,
                   ConfiguracionRed::IP_SERVIDOR, ConfiguracionRed::PUERTO);
uint32_t ultimaMedicion = 0;
bool estabaRegistrado = false;
}

void setup() {
    Serial.begin(115200);
    sensor.iniciar();
    Serial.println("[Sensor] HC-SR04: TRIG=5, ECHO=18; intervalo=250 ms.");
    cliente.iniciar();
}

void loop() {
    cliente.actualizar();
    if (!cliente.estaRegistrado()) {
        estabaRegistrado = false;
        return;
    }
    const uint32_t ahora = static_cast<uint32_t>(millis());
    if (!estabaRegistrado) {
        estabaRegistrado = true;
        // Cada registro comienza con una medicion nueva, sin datos guardados de otra conexion.
        ultimaMedicion = ahora - INTERVALO_MEDICION_MS;
    }
    if (ahora - ultimaMedicion < INTERVALO_MEDICION_MS || !cliente.puedeEnviar()) {
        return;
    }
    ultimaMedicion = ahora;
    const float distanciaCm = sensor.medirDistanciaCm();
    char lectura[ProtocoloAplicacion::CapacidadBuffer];
    // El eco puede tardar hasta 30 ms: revisar la conexion antes de publicar.
    cliente.actualizar();
    if (ProtocoloAplicacion::crearLectura(distanciaCm, lectura, sizeof(lectura)) &&
        cliente.enviar(lectura)) {
        Serial.print("[Sensor] ");
        Serial.println(lectura);
        cliente.actualizar();
    }
}
