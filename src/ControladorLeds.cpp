#include "ControladorLeds.h"

#include <math.h>

namespace {
// Cada límite pertenece al rango inferior: (0,5], (5,15] y (15,25].
constexpr float LIMITE_ROJO_CM = 5.0F;
constexpr float LIMITE_AMARILLO_CM = 15.0F;
constexpr float LIMITE_VERDE_CM = 25.0F;
}

ControladorLeds::ControladorLeds(
    uint8_t pinVerde, uint8_t pinAmarillo, uint8_t pinRojo)
    : pinVerde_(pinVerde),
      pinAmarillo_(pinAmarillo),
      pinRojo_(pinRojo),
      rangoActual_(RangoDistancia::Invalido),
      ledsEncendidos_(false),
      cambioPendiente_(false),
      ultimoCambioMs_(0UL) {
}

void ControladorLeds::iniciar() {
    pinMode(pinVerde_, OUTPUT);
    pinMode(pinAmarillo_, OUTPUT);
    pinMode(pinRojo_, OUTPUT);
    apagarTodos();
}

void ControladorLeds::establecerDistancia(float distanciaCm) {
    const RangoDistancia nuevoRango = clasificarDistancia(distanciaCm);

    if (nuevoRango == rangoActual_) {
        return;
    }

    rangoActual_ = nuevoRango;
    ledsEncendidos_ = false;
    cambioPendiente_ = true;
    apagarTodos();
}

void ControladorLeds::actualizar(unsigned long tiempoActual) {
    if (rangoActual_ == RangoDistancia::Invalido) {
        // Una lectura inválida no debe mantener activo ningún actuador.
        apagarTodos();
        cambioPendiente_ = false;
        return;
    }

    if (cambioPendiente_) {
        ledsEncendidos_ = true;
        ultimoCambioMs_ = tiempoActual;
        cambioPendiente_ = false;
        aplicarEstadoLeds();
        return;
    }

    // millis() permite alternar los LEDs sin detener las mediciones con delay().
    if (tiempoActual - ultimoCambioMs_ >= INTERVALO_PARPADEO_MS) {
        ledsEncendidos_ = !ledsEncendidos_;
        ultimoCambioMs_ = tiempoActual;
        aplicarEstadoLeds();
    }
}

RangoDistancia ControladorLeds::obtenerRangoActual() const {
    return rangoActual_;
}

RangoDistancia ControladorLeds::clasificarDistancia(float distanciaCm) const {
    if (!isfinite(distanciaCm) || distanciaCm <= 0.0F) {
        return RangoDistancia::Invalido;
    }

    if (distanciaCm <= LIMITE_ROJO_CM) {
        return RangoDistancia::Rojo;
    }
    if (distanciaCm <= LIMITE_AMARILLO_CM) {
        return RangoDistancia::Amarillo;
    }
    if (distanciaCm <= LIMITE_VERDE_CM) {
        return RangoDistancia::Verde;
    }
    return RangoDistancia::Lejos;
}

void ControladorLeds::aplicarEstadoLeds() {
    apagarTodos();

    if (!ledsEncendidos_) {
        return;
    }

    switch (rangoActual_) {
        case RangoDistancia::Rojo:
            digitalWrite(pinRojo_, HIGH);
            break;
        case RangoDistancia::Amarillo:
            digitalWrite(pinAmarillo_, HIGH);
            break;
        case RangoDistancia::Verde:
            digitalWrite(pinVerde_, HIGH);
            break;
        case RangoDistancia::Lejos:
            digitalWrite(pinVerde_, HIGH);
            digitalWrite(pinAmarillo_, HIGH);
            digitalWrite(pinRojo_, HIGH);
            break;
        case RangoDistancia::Invalido:
            break;
    }
}

void ControladorLeds::apagarTodos() {
    digitalWrite(pinVerde_, LOW);
    digitalWrite(pinAmarillo_, LOW);
    digitalWrite(pinRojo_, LOW);
}
