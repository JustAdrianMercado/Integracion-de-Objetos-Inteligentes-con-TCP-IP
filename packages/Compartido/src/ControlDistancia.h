#pragma once

#include "ComandoLeds.h"

// Reglas de control de la PC, independientes de GPIO y conexiones de red.
class ControlDistancia {
public:
    ComandoLeds calcularComando(float distanciaCm) const;
};
