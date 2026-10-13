#pragma once

#include "Tipos.h"

#include <cstddef>

// Traduce el contenido de una linea (sin LF) entre texto y datos C++.
// ClienteTcp agrega el LF al enviar; ReceptorLineas lo retira al recibir.
class ProtocoloAplicacion {
public:
    static constexpr std::size_t LongitudMaximaContenido = 127;
    static constexpr std::size_t CapacidadBuffer = LongitudMaximaContenido + 1;

    // "REGISTER type=sensor" o "REGISTER type=actuator".
    static bool crearRegistro(TipoCliente tipo, char* salida, std::size_t capacidad);
    // "POST distance=45.00"; sin eco, cero o negativa produce "POST distance=invalid".
    static bool crearLectura(float distanciaCm, char* salida, std::size_t capacidad);
    // Interpreta "SET redLed=.. yellowLed=.. greenLed=..". Si falla, no modifica comando.
    static bool leerComando(const char* linea, ComandoLeds& comando);
};
