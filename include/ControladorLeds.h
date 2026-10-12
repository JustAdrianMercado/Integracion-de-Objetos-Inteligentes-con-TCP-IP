#pragma once

#include <Arduino.h>

enum class RangoDistancia {
    Invalido,
    Rojo,
    Amarillo,
    Verde,
    Lejos
};

class ControladorLeds {
public:
    ControladorLeds(uint8_t pinVerde, uint8_t pinAmarillo, uint8_t pinRojo);

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
