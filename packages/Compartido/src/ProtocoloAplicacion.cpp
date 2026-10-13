#include "ProtocoloAplicacion.h"

#include <clocale>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>

constexpr std::size_t ProtocoloAplicacion::LongitudMaximaContenido;
constexpr std::size_t ProtocoloAplicacion::CapacidadBuffer;

namespace {
bool validarContenido(const char* linea) {
    if (linea == nullptr || linea[0] == '\0') {
        return false;
    }
    for (std::size_t i = 0; i <= ProtocoloAplicacion::LongitudMaximaContenido; ++i) {
        if (linea[i] == '\0') {
            return true;
        }
        const unsigned char byte = static_cast<unsigned char>(linea[i]);
        if (i == ProtocoloAplicacion::LongitudMaximaContenido ||
            byte < 32 || byte > 126) {
            return false;
        }
    }
    return false;
}

bool copiarSalida(const char* texto, char* salida, std::size_t capacidad) {
    const std::size_t longitud = std::strlen(texto);
    if (salida == nullptr || longitud + 1 > capacidad) {
        return false;
    }
    std::memcpy(salida, texto, longitud + 1);
    return true;
}

bool consumirPrefijo(const char*& cursor, const char* prefijo) {
    const std::size_t longitud = std::strlen(prefijo);
    if (std::strncmp(cursor, prefijo, longitud) != 0) {
        return false;
    }
    cursor += longitud;
    return true;
}

const char* textoEstado(EstadoLed estado) {
    switch (estado) {
    case EstadoLed::Apagado: return "off";
    case EstadoLed::Encendido: return "on";
    case EstadoLed::Parpadeo2Hz: return "blink_2";
    case EstadoLed::Parpadeo4Hz: return "blink_4";
    }
    return nullptr;
}

bool leerEstado(const char*& cursor, char separador, EstadoLed& estado) {
    const EstadoLed estados[] = {EstadoLed::Apagado, EstadoLed::Encendido,
                                EstadoLed::Parpadeo2Hz, EstadoLed::Parpadeo4Hz};
    for (EstadoLed candidato : estados) {
        const char* texto = textoEstado(candidato);
        const std::size_t longitud = std::strlen(texto);
        if (std::strncmp(cursor, texto, longitud) == 0 &&
            cursor[longitud] == separador) {
            estado = candidato;
            cursor += longitud + (separador == '\0' ? 0 : 1);
            return true;
        }
    }
    return false;
}

const char* textoError(MotivoError motivo) {
    switch (motivo) {
    case MotivoError::MensajeInvalido: return "ERROR reason=invalid_message";
    case MotivoError::NoRegistrado: return "ERROR reason=not_registered";
    case MotivoError::RolOcupado: return "ERROR reason=role_busy";
    }
    return nullptr;
}

bool esDigito(char caracter) {
    return caracter >= '0' && caracter <= '9';
}

bool leerDecimal(const char* texto, float& distanciaCm) {
    // No usa strtof: el punto del protocolo es independiente del idioma de la PC.
    if (!esDigito(*texto)) {
        return false;
    }
    double valor = 0.0;
    while (esDigito(*texto)) {
        valor = valor * 10.0 + (*texto++ - '0');
    }
    if (*texto == '.') {
        ++texto;
        if (!esDigito(*texto)) {
            return false;
        }
        double factor = 0.1;
        while (esDigito(*texto)) {
            valor += (*texto++ - '0') * factor;
            factor *= 0.1;
        }
    }
    if (*texto != '\0' || !std::isfinite(valor) || valor <= 0.0 ||
        valor > static_cast<double>(std::numeric_limits<float>::max())) {
        return false;
    }
    const float resultado = static_cast<float>(valor);
    if (!std::isfinite(resultado) || resultado <= 0.0F) {
        return false;
    }
    distanciaCm = resultado;
    return true;
}
}

bool ProtocoloAplicacion::crearRegistro(TipoCliente tipo, char* salida,
                                       std::size_t capacidad) {
    switch (tipo) {
    case TipoCliente::Sensor:
        return copiarSalida("REGISTER type=sensor", salida, capacidad);
    case TipoCliente::Actuador:
        return copiarSalida("REGISTER type=actuator", salida, capacidad);
    }
    return false;
}

bool ProtocoloAplicacion::crearLectura(float distanciaCm, char* salida,
                                      std::size_t capacidad) {
    if (!std::isfinite(distanciaCm) || distanciaCm <= 0.0F) {
        return copiarSalida("POST distance=invalid", salida, capacidad);
    }

    char numero[CapacidadBuffer];
    const int escritos = std::snprintf(numero, sizeof(numero), "%.2f",
                                       static_cast<double>(distanciaCm));
    if (escritos <= 0 || static_cast<std::size_t>(escritos) >= sizeof(numero)) {
        return false;
    }

    // printf puede usar coma u otro separador local. El mensaje siempre usa punto.
    const char* separador = std::localeconv()->decimal_point;
    if (separador == nullptr || separador[0] == '\0') {
        return false;
    }
    const char* punto = std::strstr(numero, separador);
    if (punto == nullptr) {
        return false;
    }
    const char* fraccion = punto + std::strlen(separador);
    const int longitudEntera = static_cast<int>(punto - numero);
    if (longitudEntera <= 0 || std::strlen(fraccion) != 2 ||
        !esDigito(fraccion[0]) || !esDigito(fraccion[1])) {
        return false;
    }
    if (longitudEntera == 1 && numero[0] == '0' && std::strcmp(fraccion, "00") == 0) {
        // Una lectura que redondea a cero no puede emitirse como decimal valido.
        return copiarSalida("POST distance=invalid", salida, capacidad);
    }

    char linea[CapacidadBuffer];
    const int longitud = std::snprintf(linea, sizeof(linea), "POST distance=%.*s.%s",
                                      longitudEntera, numero, fraccion);
    return longitud > 0 && static_cast<std::size_t>(longitud) < sizeof(linea) &&
           copiarSalida(linea, salida, capacidad);
}

bool ProtocoloAplicacion::crearComando(const ComandoLeds& comando, char* salida,
                                      std::size_t capacidad) {
    const char* rojo = textoEstado(comando.rojo);
    const char* amarillo = textoEstado(comando.amarillo);
    const char* verde = textoEstado(comando.verde);
    if (rojo == nullptr || amarillo == nullptr || verde == nullptr) {
        return false;
    }
    char linea[CapacidadBuffer];
    const int longitud = std::snprintf(linea, sizeof(linea),
        "SET redLed=%s yellowLed=%s greenLed=%s", rojo, amarillo, verde);
    return longitud > 0 && static_cast<std::size_t>(longitud) < sizeof(linea) &&
           copiarSalida(linea, salida, capacidad);
}

bool ProtocoloAplicacion::crearOk(char* salida, std::size_t capacidad) {
    return copiarSalida("OK", salida, capacidad);
}

bool ProtocoloAplicacion::crearError(MotivoError motivo, char* salida,
                                    std::size_t capacidad) {
    const char* texto = textoError(motivo);
    return texto != nullptr && copiarSalida(texto, salida, capacidad);
}

bool ProtocoloAplicacion::leerRegistro(const char* linea, TipoCliente& tipo) {
    if (!validarContenido(linea)) {
        return false;
    }
    if (std::strcmp(linea, "REGISTER type=sensor") == 0) {
        tipo = TipoCliente::Sensor;
        return true;
    }
    if (std::strcmp(linea, "REGISTER type=actuator") == 0) {
        tipo = TipoCliente::Actuador;
        return true;
    }
    return false;
}

bool ProtocoloAplicacion::leerLectura(const char* linea, float& distanciaCm) {
    if (!validarContenido(linea)) {
        return false;
    }
    const char* cursor = linea;
    if (!consumirPrefijo(cursor, "POST distance=")) {
        return false;
    }
    if (std::strcmp(cursor, "invalid") == 0) {
        distanciaCm = std::numeric_limits<float>::quiet_NaN();
        return true;
    }
    return leerDecimal(cursor, distanciaCm);
}

bool ProtocoloAplicacion::leerComando(const char* linea, ComandoLeds& comando) {
    if (!validarContenido(linea)) {
        return false;
    }
    const char* cursor = linea;
    ComandoLeds resultado;
    if (!consumirPrefijo(cursor, "SET redLed=") ||
        !leerEstado(cursor, ' ', resultado.rojo) ||
        !consumirPrefijo(cursor, "yellowLed=") ||
        !leerEstado(cursor, ' ', resultado.amarillo) ||
        !consumirPrefijo(cursor, "greenLed=") ||
        !leerEstado(cursor, '\0', resultado.verde)) {
        return false;
    }
    comando = resultado;
    return true;
}

bool ProtocoloAplicacion::leerOk(const char* linea) {
    return validarContenido(linea) && std::strcmp(linea, "OK") == 0;
}

bool ProtocoloAplicacion::leerError(const char* linea, MotivoError& motivo) {
    if (!validarContenido(linea)) {
        return false;
    }
    const MotivoError motivos[] = {MotivoError::MensajeInvalido,
                                   MotivoError::NoRegistrado, MotivoError::RolOcupado};
    for (MotivoError candidato : motivos) {
        if (std::strcmp(linea, textoError(candidato)) == 0) {
            motivo = candidato;
            return true;
        }
    }
    return false;
}
