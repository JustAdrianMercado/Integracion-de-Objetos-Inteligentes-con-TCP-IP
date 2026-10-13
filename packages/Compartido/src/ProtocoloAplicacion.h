#pragma once

#include "ComandoLeds.h"
#include "MotivoError.h"
#include "TipoCliente.h"

#include <cstddef>

// Traduce el contenido de una linea, sin LF, entre texto y datos C++.
// El adaptador TCP agrega LF al enviar; ReceptorLineas lo retira al recibir.
class ProtocoloAplicacion {
public:
    static constexpr std::size_t LongitudMaximaContenido = 127;
    static constexpr std::size_t CapacidadBuffer = LongitudMaximaContenido + 1;

    // Incluyen el terminador C '\0', pero no LF. Si fallan, no modifican salida.
    static bool crearRegistro(TipoCliente tipo, char* salida, std::size_t capacidad);
    static bool crearLectura(float distanciaCm, char* salida, std::size_t capacidad);
    static bool crearComando(const ComandoLeds& comando, char* salida,
                             std::size_t capacidad);
    static bool crearOk(char* salida, std::size_t capacidad);
    static bool crearError(MotivoError motivo, char* salida, std::size_t capacidad);

    // Requieren texto terminado en '\0'. Un fallo deja los datos sin modificar.
    // distance=invalid es valido y entrega NaN; el resto debe ser finito y > 0.
    static bool leerRegistro(const char* linea, TipoCliente& tipo);
    static bool leerLectura(const char* linea, float& distanciaCm);
    static bool leerComando(const char* linea, ComandoLeds& comando);
    static bool leerOk(const char* linea);
    static bool leerError(const char* linea, MotivoError& motivo);
};
