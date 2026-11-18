#include <Arduino.h>

#include "ClienteTcp.h"
#include "ProtocoloAplicacion.h"
#include "SensorUltrasonico.h"

#if __has_include("ConfiguracionRed.h")
#include "ConfiguracionRed.h"
#else
#include "ConfiguracionRed.example.h"
#endif

// Programa del ESP32 sensor: mide con el HC-SR04 y publica cada lectura al servidor
// como POST. No clasifica distancias; esa decision la toma el servidor.

namespace {
constexpr uint8_t PIN_DISPARO = 23;  // TRIG (D23)
constexpr uint8_t PIN_ECO = 18;      // ECHO (D18), a traves del divisor de tension
// 250 ms entre mediciones: 4 lecturas/s, por encima del minimo de 2 lecturas/s.
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
    Serial.println("[Sensor] HC-SR04: TRIG=23, ECHO=18; intervalo=250 ms.");
    cliente.iniciar();
}

void loop() {
    // Atender WiFi/TCP primero: reintentos, registro y envio pendiente.
    cliente.actualizar();
    // Sin registro no se mide: no tiene sentido acumular lecturas para despues.
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
    // La resta sin signo sigue funcionando cuando millis() se desborda (~49 dias).
    // Si el POST anterior aun no salio, se espera en vez de medir y descartar.
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
