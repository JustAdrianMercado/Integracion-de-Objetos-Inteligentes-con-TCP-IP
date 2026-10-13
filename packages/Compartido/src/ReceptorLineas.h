#pragma once

#include "ProtocoloAplicacion.h"

#include <cstdint>

enum class ResultadoRecepcion : std::uint8_t {
    Incompleto,
    LineaCompleta,
    MensajeInvalido,
};

// Una instancia por conexion. No reserva memoria dinamica ni depende de sockets.
class ReceptorLineas {
public:
    ResultadoRecepcion agregar(char byte);

    // Solo devuelve texto despues de LineaCompleta. Leerlo antes del siguiente byte.
    const char* linea() const;

    // Usar al abrir una conexion nueva: descarta fragmentos y errores anteriores.
    void reiniciar();

private:
    char buffer_[ProtocoloAplicacion::CapacidadBuffer] = {};
    std::size_t longitud_ = 0;
    bool completa_ = false;
    bool rechazada_ = false;
};
