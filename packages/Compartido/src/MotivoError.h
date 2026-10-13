#pragma once

#include <cstdint>

enum class MotivoError : std::uint8_t {
    MensajeInvalido,
    NoRegistrado,
    RolOcupado,
};
