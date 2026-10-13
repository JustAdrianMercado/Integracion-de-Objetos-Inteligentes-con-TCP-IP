#include "ProtocoloAplicacion.h"

#include <cmath>
#include <cstdio>
#include <cstring>

constexpr std::size_t ProtocoloAplicacion::LongitudMaximaContenido;
constexpr std::size_t ProtocoloAplicacion::CapacidadBuffer;

namespace {
bool copiar(const char* texto, char* salida, std::size_t capacidad) {
    const std::size_t longitud = std::strlen(texto);
    if (salida == nullptr || longitud + 1 > capacidad) {
        return false;
    }
    std::memcpy(salida, texto, longitud + 1);
    return true;
}

// Lee un estado seguido del separador esperado (' ' o el final de la linea).
bool leerEstado(const char*& cursor, char separador, EstadoLed& estado) {
    struct Opcion { const char* texto; EstadoLed estado; };
    const Opcion opciones[] = {
        {"off", EstadoLed::Apagado},
        {"on", EstadoLed::Encendido},
        {"blink_2", EstadoLed::Parpadeo2Hz},
        {"blink_4", EstadoLed::Parpadeo4Hz},
    };
    for (const Opcion& opcion : opciones) {
        const std::size_t longitud = std::strlen(opcion.texto);
        if (std::strncmp(cursor, opcion.texto, longitud) == 0 && cursor[longitud] == separador) {
            estado = opcion.estado;
            cursor += longitud + (separador == '\0' ? 0 : 1);
            return true;
        }
    }
    return false;
}

bool consumir(const char*& cursor, const char* prefijo) {
    const std::size_t longitud = std::strlen(prefijo);
    if (std::strncmp(cursor, prefijo, longitud) != 0) {
        return false;
    }
    cursor += longitud;
    return true;
}
}

bool ProtocoloAplicacion::crearRegistro(TipoCliente tipo, char* salida, std::size_t capacidad) {
    return copiar(tipo == TipoCliente::Sensor ? "REGISTER type=sensor" : "REGISTER type=actuator",
                  salida, capacidad);
}

bool ProtocoloAplicacion::crearLectura(float distanciaCm, char* salida, std::size_t capacidad) {
    // Por debajo de 0.01 cm el texto seria "0.00", que el servidor no acepta como distancia.
    if (!std::isfinite(distanciaCm) || distanciaCm < 0.01F) {
        return copiar("POST distance=invalid", salida, capacidad);
    }
    char linea[CapacidadBuffer];
    const int longitud = std::snprintf(linea, sizeof(linea), "POST distance=%.2f",
                                       static_cast<double>(distanciaCm));
    return longitud > 0 && static_cast<std::size_t>(longitud) < sizeof(linea) &&
           copiar(linea, salida, capacidad);
}

bool ProtocoloAplicacion::leerComando(const char* linea, ComandoLeds& comando) {
    if (linea == nullptr) {
        return false;
    }
    const char* cursor = linea;
    ComandoLeds resultado;
    // Los tres estados se validan juntos: un SET con un error no aplica nada.
    if (!consumir(cursor, "SET redLed=") || !leerEstado(cursor, ' ', resultado.rojo) ||
        !consumir(cursor, "yellowLed=") || !leerEstado(cursor, ' ', resultado.amarillo) ||
        !consumir(cursor, "greenLed=") || !leerEstado(cursor, '\0', resultado.verde)) {
        return false;
    }
    comando = resultado;
    return true;
}
