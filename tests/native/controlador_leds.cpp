#include "ControladorLeds.h"
#include "RedSimulada.h"

#include <cstdlib>
#include <iostream>

namespace {
constexpr uint8_t VERDE = 25;
constexpr uint8_t AMARILLO = 26;
constexpr uint8_t ROJO = 27;

size_t total = 0;
size_t fallos = 0;
void comprobar(bool correcto, const char* nombre) {
    ++total;
    if (!correcto) { ++fallos; std::cerr << "FALLO: " << nombre << '\n'; }
}
bool niveles(int rojo, int amarillo, int verde) {
    return red.niveles[ROJO] == rojo && red.niveles[AMARILLO] == amarillo && red.niveles[VERDE] == verde;
}
ComandoLeds comando(EstadoLed rojo, EstadoLed amarillo, EstadoLed verde) {
    ComandoLeds resultado;
    resultado.rojo = rojo;
    resultado.amarillo = amarillo;
    resultado.verde = verde;
    return resultado;
}
// Cuenta las alternancias del pin durante una ventana, avanzando el reloj de 1 en 1 ms.
int alternancias(ControladorLeds& leds, uint8_t pin, uint32_t& reloj, uint32_t duracion) {
    int cambios = 0;
    uint8_t anterior = red.niveles[pin];
    for (uint32_t i = 0; i < duracion; ++i) {
        leds.actualizar(++reloj);
        if (red.niveles[pin] != anterior) { ++cambios; anterior = red.niveles[pin]; }
    }
    return cambios;
}
}

int main() {
    const EstadoLed off = EstadoLed::Apagado;
    const EstadoLed on = EstadoLed::Encendido;
    const EstadoLed b2 = EstadoLed::Parpadeo2Hz;
    const EstadoLed b4 = EstadoLed::Parpadeo4Hz;
    {
        red = RedSimulada{};
        red.niveles[ROJO] = red.niveles[AMARILLO] = red.niveles[VERDE] = HIGH;
        ControladorLeds leds(VERDE, AMARILLO, ROJO);
        leds.iniciar();
        comprobar(red.modosPin[ROJO] == OUTPUT && red.modosPin[AMARILLO] == OUTPUT && red.modosPin[VERDE] == OUTPUT,
                  "Los tres GPIO se configuran como salidas");
        comprobar(niveles(LOW, LOW, LOW), "Iniciar con todos los LEDs apagados");

        leds.establecerEstados(comando(off, off, on));
        leds.actualizar(0);
        comprobar(niveles(LOW, LOW, HIGH), "on enciende fijo el verde");
        uint32_t reloj = 0;
        comprobar(alternancias(leds, VERDE, reloj, 2000) == 0, "on no parpadea");

        leds.establecerEstados(comando(b4, off, off));
        comprobar(niveles(LOW, LOW, HIGH), "El cambio se aplica en el siguiente actualizar");
        leds.actualizar(reloj);
        comprobar(niveles(HIGH, LOW, LOW), "Aplicar los tres estados juntos; blink_4 empieza encendido");
        leds.actualizar(reloj + 124);
        comprobar(niveles(HIGH, LOW, LOW), "blink_4 conserva 125 ms encendido");
        leds.actualizar(reloj + 125);
        comprobar(niveles(LOW, LOW, LOW), "blink_4 alterna a los 125 ms");
        reloj += 125;
        comprobar(alternancias(leds, ROJO, reloj, 1000) == 8, "blink_4: cuatro ciclos completos por segundo");

        leds.establecerEstados(comando(off, b2, off));
        leds.actualizar(reloj);
        comprobar(niveles(LOW, HIGH, LOW), "blink_2 empieza encendido y apaga el rojo");
        leds.actualizar(reloj + 249);
        comprobar(niveles(LOW, HIGH, LOW), "blink_2 conserva 250 ms encendido");
        comprobar(alternancias(leds, AMARILLO, reloj, 1000) == 4, "blink_2: dos ciclos completos por segundo");

        // El servidor repite el SET cada 250 ms: la fase no debe reiniciarse.
        int cambios = 0;
        uint8_t anterior = red.niveles[AMARILLO];
        for (uint32_t i = 0; i < 2000; ++i) {
            if (i % 250 == 0) { leds.establecerEstados(comando(off, b2, off)); }
            leds.actualizar(++reloj);
            if (red.niveles[AMARILLO] != anterior) { ++cambios; anterior = red.niveles[AMARILLO]; }
        }
        comprobar(cambios == 8, "Comandos identicos cada 250 ms conservan la fase de blink_2");

        leds.establecerEstados(comando(b4, b2, on));
        leds.actualizar(reloj);
        comprobar(red.niveles[ROJO] == HIGH && red.niveles[VERDE] == HIGH, "Solo los LEDs que cambian reinician su estado");
        leds.apagarTodos();
        comprobar(niveles(LOW, LOW, LOW), "apagarTodos apaga sin esperar a actualizar");
        comprobar(alternancias(leds, ROJO, reloj, 1000) == 0 && niveles(LOW, LOW, LOW),
                  "Despues de apagar no queda ningun parpadeo activo");
        comprobar(leds.comandoActual().rojo == off && leds.comandoActual().verde == off,
                  "El comando actual refleja el apagado");
        leds.establecerEstados(comando(b4, b2, on));
        leds.actualizar(reloj);
        comprobar(niveles(HIGH, HIGH, HIGH), "Un comando posterior reactiva las salidas");
    }
    {
        red = RedSimulada{};
        ControladorLeds leds(VERDE, AMARILLO, ROJO);
        leds.iniciar();
        uint32_t reloj = UINT32_MAX - 300;
        leds.establecerEstados(comando(b4, off, off));
        leds.actualizar(reloj);
        comprobar(alternancias(leds, ROJO, reloj, 1000) == 8, "blink_4 mantiene su frecuencia al desbordar millis");
    }
    {
        red = RedSimulada{};
        ControladorLeds leds(VERDE, AMARILLO, ROJO);
        leds.iniciar();
        leds.establecerEstados(comando(b2, off, off));
        leds.actualizar(0);
        leds.actualizar(1000);
        const int escrituras = red.escriturasPin;
        leds.actualizar(1001);
        comprobar(red.escriturasPin == escrituras, "Una espera larga no encadena alternancias atrasadas");
        leds.actualizar(1250);
        comprobar(red.escriturasPin == escrituras + 1, "Tras resincronizar se retoma el intervalo normal");
    }
    std::cout << total - fallos << " de " << total << " comprobaciones del controlador de LEDs aprobadas.\n";
    return fallos == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
