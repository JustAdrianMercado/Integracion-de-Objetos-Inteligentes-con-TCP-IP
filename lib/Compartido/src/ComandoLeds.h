#pragma once

#include "EstadoLed.h"

// Una orden siempre especifica los tres LEDs y comienza con todos apagados.
struct ComandoLeds {
    EstadoLed rojo = EstadoLed::Apagado;
    EstadoLed amarillo = EstadoLed::Apagado;
    EstadoLed verde = EstadoLed::Apagado;
};
