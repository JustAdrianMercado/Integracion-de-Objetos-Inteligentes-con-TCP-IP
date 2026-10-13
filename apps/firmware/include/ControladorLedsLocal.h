#pragma once

#include <Arduino.h>

enum class RangoDistancia {
    Invalido,
    Rojo,
    Amarillo,
    Verde,
    Lejos
};

// Controlador del programa local de la Practica 1: clasifica distancias en el ESP32.
// El actuador de la Practica 2 usa ControladorLeds y solo aplica comandos del servidor.
class ControladorLedsLocal {
public:
    ControladorLedsLocal(uint8_t pinVerde, uint8_t pinAmarillo, uint8_t pinRojo);

    void iniciar();
    void establecerDistancia(float distanciaCm);
    void actualizar(unsigned long tiempoActual);
    RangoDistancia obtenerRangoActual() const;

private:
    static constexpr unsigned long INTERVALO_PARPADEO_MS = 150UL;

    uint8_t pinVerde_;
    uint8_t pinAmarillo_;
    uint8_t pinRojo_;
    RangoDistancia rangoActual_;
    bool ledsEncendidos_;
    bool cambioPendiente_;
    unsigned long ultimoCambioMs_;

    RangoDistancia clasificarDistancia(float distanciaCm) const;
    void aplicarEstadoLeds();
    void apagarTodos();
};
