#include "ControlDistancia.h"

#include <cmath>

namespace {
constexpr float LIMITE_ROJO_CM = 50.0F;
constexpr float LIMITE_AMARILLO_CM = 100.0F;
}

ComandoLeds ControlDistancia::calcularComando(float distanciaCm) const {
    ComandoLeds comando;

    if (!std::isfinite(distanciaCm) || distanciaCm <= 0.0F) {
        return comando;
    }

    // Los limites exactos pertenecen al rango que comienza en ese valor.
    if (distanciaCm < LIMITE_ROJO_CM) {
        comando.rojo = EstadoLed::Parpadeo4Hz;
    } else if (distanciaCm < LIMITE_AMARILLO_CM) {
        comando.amarillo = EstadoLed::Parpadeo2Hz;
    } else {
        comando.verde = EstadoLed::Encendido;
    }

    return comando;
}
