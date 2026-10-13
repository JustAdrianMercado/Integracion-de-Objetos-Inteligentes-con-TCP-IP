#pragma once

#include <Arduino.h>

#include "Tipos.h"

// Aplica en los GPIO los estados ordenados por el servidor. No clasifica distancias.
class ControladorLeds {
public:
    ControladorLeds(uint8_t pinVerde, uint8_t pinAmarillo, uint8_t pinRojo);

    // Configura los tres GPIO como salidas y los deja apagados.
    void iniciar();
    // Recibe los tres estados juntos. Un LED que conserva su estado mantiene su fase;
    // los cambios se reflejan en el siguiente actualizar(). Un parpadeo nuevo empieza encendido.
    void establecerEstados(const ComandoLeds& comando);
    // Llamar en cada vuelta del ciclo principal: alterna los parpadeos sin bloquear.
    void actualizar(unsigned long tiempoActual);
    // Apaga inmediatamente; se usa al iniciar, al perder la conexion y sin comandos recientes.
    void apagarTodos();
    const ComandoLeds& comandoActual() const;

private:
    // Un ciclo de 2 Hz dura 500 ms y uno de 4 Hz, 250 ms: se alterna cada medio ciclo.
    static constexpr uint32_t ALTERNANCIA_2HZ_MS = 250;
    static constexpr uint32_t ALTERNANCIA_4HZ_MS = 125;

    struct Led {
        uint8_t pin;
        EstadoLed estado;
        bool encendido;
        bool pendiente;
        uint32_t ultimoCambioMs;
    };

    Led rojo_;
    Led amarillo_;
    Led verde_;
    ComandoLeds comandoActual_;

    static uint32_t intervaloAlternancia(EstadoLed estado);
    static void cambiarEstado(Led& led, EstadoLed estado);
    static void actualizarLed(Led& led, uint32_t ahora);
    static void escribir(Led& led, bool encendido);
};
