#pragma once

#include <cstdint>

// Modos que el servidor puede ordenar al objeto con actuadores.
enum class EstadoLed : std::uint8_t {
    Apagado,
    Encendido,
    Parpadeo2Hz,
    Parpadeo4Hz,
};
