#pragma once

#include <cstdint>

// Rol con el que cada ESP32 se registra en el servidor.
enum class TipoCliente : std::uint8_t {
    Sensor,
    Actuador,
};

// Modos que el servidor puede ordenar a cada LED.
enum class EstadoLed : std::uint8_t {
    Apagado,
    Encendido,
    Parpadeo2Hz,
    Parpadeo4Hz,
};

// Una orden siempre especifica los tres LEDs y comienza con todos apagados.
struct ComandoLeds {
    EstadoLed rojo = EstadoLed::Apagado;
    EstadoLed amarillo = EstadoLed::Apagado;
    EstadoLed verde = EstadoLed::Apagado;
};
