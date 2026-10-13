#include "ControladorLeds.h"

#include <initializer_list>

ControladorLeds::ControladorLeds(uint8_t pinVerde, uint8_t pinAmarillo, uint8_t pinRojo)
    : rojo_{pinRojo, EstadoLed::Apagado, false, false, 0},
      amarillo_{pinAmarillo, EstadoLed::Apagado, false, false, 0},
      verde_{pinVerde, EstadoLed::Apagado, false, false, 0} {
}

void ControladorLeds::iniciar() {
    pinMode(rojo_.pin, OUTPUT);
    pinMode(amarillo_.pin, OUTPUT);
    pinMode(verde_.pin, OUTPUT);
    apagarTodos();
}

void ControladorLeds::establecerEstados(const ComandoLeds& comando) {
    cambiarEstado(rojo_, comando.rojo);
    cambiarEstado(amarillo_, comando.amarillo);
    cambiarEstado(verde_, comando.verde);
    comandoActual_ = comando;
}

void ControladorLeds::actualizar(unsigned long tiempoActual) {
    const uint32_t ahora = static_cast<uint32_t>(tiempoActual);
    actualizarLed(rojo_, ahora);
    actualizarLed(amarillo_, ahora);
    actualizarLed(verde_, ahora);
}

void ControladorLeds::apagarTodos() {
    for (Led* led : {&rojo_, &amarillo_, &verde_}) {
        led->estado = EstadoLed::Apagado;
        led->pendiente = false;
        escribir(*led, false);
    }
    comandoActual_ = ComandoLeds();
}

const ComandoLeds& ControladorLeds::comandoActual() const {
    return comandoActual_;
}

uint32_t ControladorLeds::intervaloAlternancia(EstadoLed estado) {
    switch (estado) {
    case EstadoLed::Parpadeo2Hz: return ALTERNANCIA_2HZ_MS;
    case EstadoLed::Parpadeo4Hz: return ALTERNANCIA_4HZ_MS;
    case EstadoLed::Apagado:
    case EstadoLed::Encendido:
        break;
    }
    return 0;
}

void ControladorLeds::cambiarEstado(Led& led, EstadoLed estado) {
    // El servidor repite el SET cada 250 ms: un estado igual no reinicia el parpadeo.
    if (estado == led.estado) {
        return;
    }
    led.estado = estado;
    led.pendiente = true;
}

void ControladorLeds::actualizarLed(Led& led, uint32_t ahora) {
    if (led.pendiente) {
        led.pendiente = false;
        led.ultimoCambioMs = ahora;
        escribir(led, led.estado != EstadoLed::Apagado);
        return;
    }
    const uint32_t intervalo = intervaloAlternancia(led.estado);
    if (intervalo == 0) {
        return;
    }
    // Resta sin signo: soporta el desbordamiento de millis() a los 49.7 dias.
    const uint32_t transcurrido = ahora - led.ultimoCambioMs;
    if (transcurrido < intervalo) {
        return;
    }
    // Avanzar un intervalo conserva la frecuencia; tras una espera larga se resincroniza
    // en lugar de encadenar alternancias atrasadas.
    led.ultimoCambioMs = transcurrido >= 2 * intervalo ? ahora : led.ultimoCambioMs + intervalo;
    escribir(led, !led.encendido);
}

void ControladorLeds::escribir(Led& led, bool encendido) {
    led.encendido = encendido;
    digitalWrite(led.pin, encendido ? HIGH : LOW);
}
