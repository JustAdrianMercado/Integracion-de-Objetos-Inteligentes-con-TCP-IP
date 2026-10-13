#include <Arduino.h>

#include "ClienteTcp.h"
#include "ControladorLeds.h"
#include "ProtocoloAplicacion.h"

#if __has_include("ConfiguracionRed.h")
#include "ConfiguracionRed.h"
#else
#include "ConfiguracionRed.example.h"
#endif

namespace {
constexpr uint8_t PIN_LED_VERDE = 25;
constexpr uint8_t PIN_LED_AMARILLO = 26;
constexpr uint8_t PIN_LED_ROJO = 27;
// El servidor envia un SET por cada POST (250 ms nominales); 2000 ms sin SET apagan.
constexpr uint32_t PLAZO_COMANDOS_MS = 2000;

ControladorLeds leds(PIN_LED_VERDE, PIN_LED_AMARILLO, PIN_LED_ROJO);
ClienteTcp cliente(TipoCliente::Actuador, ConfiguracionRed::SSID, ConfiguracionRed::CLAVE,
                   ConfiguracionRed::IP_SERVIDOR, ConfiguracionRed::PUERTO);
uint32_t ultimoComando = 0;
bool estabaRegistrado = false;
bool comandosVigentes = false;

void apagarPorAusencia(const char* motivo) {
    leds.apagarTodos();
    comandosVigentes = false;
    Serial.print("[Actuador] ");
    Serial.println(motivo);
}
}

void setup() {
    Serial.begin(115200);
    leds.iniciar();
    Serial.println("[Actuador] LEDs: verde=25, amarillo=26, rojo=27; apagados.");
    cliente.iniciar();
}

void loop() {
    cliente.actualizar();
    const uint32_t ahora = static_cast<uint32_t>(millis());
    if (!cliente.estaRegistrado()) {
        // Desconexion, ERROR o SET invalido: ClienteTcp cierra y el actuador apaga al instante.
        if (estabaRegistrado) {
            estabaRegistrado = false;
            apagarPorAusencia("Conexion perdida; LEDs apagados.");
        }
        leds.actualizar(ahora);
        return;
    }
    if (!estabaRegistrado) {
        estabaRegistrado = true;
        comandosVigentes = true;
        ultimoComando = ahora;
    }

    char linea[ProtocoloAplicacion::CapacidadBuffer];
    ComandoLeds comando;
    // ClienteTcp ya valido los SET; leerComando los convierte sin aplicar estados parciales.
    while (cliente.recibir(linea, sizeof(linea))) {
        if (ProtocoloAplicacion::leerComando(linea, comando)) {
            if (!comandosVigentes) {
                Serial.println("[Actuador] Comandos reanudados.");
            }
            leds.establecerEstados(comando);
            ultimoComando = ahora;
            comandosVigentes = true;
        }
    }
    if (comandosVigentes && ahora - ultimoComando >= PLAZO_COMANDOS_MS) {
        apagarPorAusencia("2000 ms sin SET; LEDs apagados y conexion conservada.");
    }
    leds.actualizar(ahora);
}
