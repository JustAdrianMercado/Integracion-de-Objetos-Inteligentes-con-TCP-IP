#include "ControlDistancia.h"
#include "ProtocoloAplicacion.h"

#include <array>
#include <clocale>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <string>

namespace {
struct Pruebas {
    std::size_t total = 0;
    std::size_t fallos = 0;

    void comprobar(bool resultado, const std::string& nombre) {
        ++total;
        if (!resultado) {
            ++fallos;
            std::cerr << "FALLO: " << nombre << '\n';
        }
    }
};

bool iguales(const ComandoLeds& a, const ComandoLeds& b) {
    return a.rojo == b.rojo && a.amarillo == b.amarillo && a.verde == b.verde;
}

template <typename Constructor>
void probarBuffer(Pruebas& pruebas, const char* esperado, Constructor crear) {
    std::array<char, ProtocoloAplicacion::CapacidadBuffer> salida;
    salida.fill('#');
    const auto original = salida;
    const std::size_t longitud = std::strlen(esperado);
    pruebas.comprobar(!crear(salida.data(), longitud) && salida == original,
                     std::string("Buffer insuficiente sin escritura parcial: ") + esperado);
    pruebas.comprobar(crear(salida.data(), longitud + 1) &&
                     std::strcmp(salida.data(), esperado) == 0 &&
                     salida[longitud + 1] == '#',
                     std::string("Buffer exacto y terminador C: ") + esperado);
    pruebas.comprobar(!crear(nullptr, longitud + 1),
                     std::string("Salida nula: ") + esperado);
    pruebas.comprobar(!crear(salida.data(), 0),
                     std::string("Capacidad cero: ") + esperado);
}
}

int main() {
    Pruebas pruebas;
    char salida[ProtocoloAplicacion::CapacidadBuffer] = {};

    const TipoCliente roles[] = {TipoCliente::Sensor, TipoCliente::Actuador};
    const char* registros[] = {"REGISTER type=sensor", "REGISTER type=actuator"};
    for (std::size_t i = 0; i < 2; ++i) {
        TipoCliente tipo = roles[1 - i];
        pruebas.comprobar(ProtocoloAplicacion::crearRegistro(roles[i], salida, sizeof(salida)) &&
                         std::strcmp(salida, registros[i]) == 0 &&
                         ProtocoloAplicacion::leerRegistro(salida, tipo) && tipo == roles[i],
                         registros[i]);
        probarBuffer(pruebas, registros[i], [i, &roles](char* texto, std::size_t capacidad) {
            return ProtocoloAplicacion::crearRegistro(roles[i], texto, capacidad);
        });
    }

    const char* registrosInvalidos[] = {
        "", "REGISTER", "REGISTER type=", "REGISTER type=Sensor", "register type=sensor",
        "REGISTER type=unknown", "REGISTER type=sensor ", " REGISTER type=sensor",
        "REGISTER  type=sensor", "REGISTER\ttype=sensor", "REGISTER type=sensor\r",
        "REGISTER type=sensor\n", "REGISTER type=sensor type=actuator", "OK",
    };
    for (const char* texto : registrosInvalidos) {
        TipoCliente tipo = TipoCliente::Actuador;
        pruebas.comprobar(!ProtocoloAplicacion::leerRegistro(texto, tipo) &&
                         tipo == TipoCliente::Actuador,
                         std::string("Registro invalido conserva el rol: ") + texto);
    }

    struct Lectura { const char* texto; float distancia; };
    const Lectura lecturas[] = {
        {"POST distance=45", 45.0F}, {"POST distance=45.00", 45.0F},
        {"POST distance=00045.50", 45.5F}, {"POST distance=0.01", 0.01F},
        {"POST distance=49.99", 49.99F}, {"POST distance=50", 50.0F},
        {"POST distance=99.99", 99.99F}, {"POST distance=100.00", 100.0F},
        {"POST distance=120.75", 120.75F},
    };
    for (const Lectura& lectura : lecturas) {
        float distancia = -1.0F;
        pruebas.comprobar(ProtocoloAplicacion::leerLectura(lectura.texto, distancia) &&
                         distancia == lectura.distancia, lectura.texto);
    }

    float distancia = 45.0F;
    const bool invalidaAceptada = ProtocoloAplicacion::leerLectura("POST distance=invalid", distancia);
    const ComandoLeds apagados;
    pruebas.comprobar(invalidaAceptada && std::isnan(distancia) &&
                     iguales(ControlDistancia().calcularComando(distancia), apagados),
                     "La lectura invalid es valida y produce todos apagados");

    const char* lecturasInvalidas[] = {
        "POST", "POST distance=", "POST distance=0", "POST distance=0.00",
        "POST distance=-1", "POST distance=+1", "POST distance=.5", "POST distance=1.",
        "POST distance=1,25", "POST distance=1e2", "POST distance=NaN",
        "POST distance=nan", "POST distance=Infinity", "POST distance=inf",
        "POST distance=INVALID", "POST distance=invalidx", "POST distance=1.2.3",
        "POST distance=1 2", "POST distance= 1", "POST distance=1 ",
        "POST  distance=1", "POST distance=1\r", "POST distance=1\n",
        "post distance=1", "POST Distance=1", "POST distance=1 extra=2",
        "POST distance=99999999999999999999999999999999999999999999999999",
        "POST distance=0.000000000000000000000000000000000000000000000000000000000000001",
    };
    for (const char* texto : lecturasInvalidas) {
        distancia = 37.5F;
        pruebas.comprobar(!ProtocoloAplicacion::leerLectura(texto, distancia) && distancia == 37.5F,
                         std::string("Lectura invalida conserva el destino: ") + texto);
    }

    const std::string prefijo = "POST distance=";
    const std::string linea127 = prefijo +
        std::string(127 - prefijo.size() - 1, '0') + "1";
    pruebas.comprobar(ProtocoloAplicacion::leerLectura(linea127.c_str(), distancia) && distancia == 1.0F,
                     "127 bytes de contenido con sintaxis valida");
    pruebas.comprobar(!ProtocoloAplicacion::leerLectura((linea127 + "0").c_str(), distancia),
                     "128 bytes de contenido rechazados por el codec");

    struct Emision { float distancia; const char* texto; };
    const Emision emisiones[] = {
        {45.0F, "POST distance=45.00"}, {45.126F, "POST distance=45.13"},
        {49.99F, "POST distance=49.99"}, {50.0F, "POST distance=50.00"},
        {100.0F, "POST distance=100.00"}, {0.01F, "POST distance=0.01"},
        {0.001F, "POST distance=invalid"}, {0.0F, "POST distance=invalid"},
        {-0.0F, "POST distance=invalid"}, {-1.0F, "POST distance=invalid"},
        {std::numeric_limits<float>::infinity(), "POST distance=invalid"},
        {-std::numeric_limits<float>::infinity(), "POST distance=invalid"},
        {std::numeric_limits<float>::quiet_NaN(), "POST distance=invalid"},
        {std::numeric_limits<float>::denorm_min(), "POST distance=invalid"},
        {std::numeric_limits<float>::max(),
         "POST distance=340282346638528859811704183484516925440.00"},
    };
    for (const Emision& emision : emisiones) {
        pruebas.comprobar(ProtocoloAplicacion::crearLectura(emision.distancia, salida, sizeof(salida)) &&
                         std::strcmp(salida, emision.texto) == 0 &&
                         ProtocoloAplicacion::leerLectura(salida, distancia),
                         std::string("Emitir dos decimales y volver a leer: ") + emision.texto);
    }
    probarBuffer(pruebas, "POST distance=45.00", [](char* texto, std::size_t capacidad) {
        return ProtocoloAplicacion::crearLectura(45.0F, texto, capacidad);
    });
    probarBuffer(pruebas, "POST distance=invalid", [](char* texto, std::size_t capacidad) {
        return ProtocoloAplicacion::crearLectura(0.0F, texto, capacidad);
    });

    const EstadoLed estados[] = {EstadoLed::Apagado, EstadoLed::Encendido,
                                 EstadoLed::Parpadeo2Hz, EstadoLed::Parpadeo4Hz};
    const char* tokens[] = {"off", "on", "blink_2", "blink_4"};
    for (std::size_t r = 0; r < 4; ++r) {
        for (std::size_t a = 0; a < 4; ++a) {
            for (std::size_t v = 0; v < 4; ++v) {
                ComandoLeds esperado;
                esperado.rojo = estados[r];
                esperado.amarillo = estados[a];
                esperado.verde = estados[v];
                const std::string texto = std::string("SET redLed=") + tokens[r] +
                    " yellowLed=" + tokens[a] + " greenLed=" + tokens[v];
                ComandoLeds leido;
                pruebas.comprobar(ProtocoloAplicacion::crearComando(esperado, salida, sizeof(salida)) &&
                                 texto == salida && ProtocoloAplicacion::leerComando(salida, leido) &&
                                 iguales(leido, esperado), texto);
            }
        }
    }
    probarBuffer(pruebas, "SET redLed=off yellowLed=off greenLed=off",
                 [&apagados](char* texto, std::size_t capacidad) {
        return ProtocoloAplicacion::crearComando(apagados, texto, capacidad);
    });

    const char* comandosInvalidos[] = {
        "SET", "SET redLed=off", "SET redLed=off yellowLed=off",
        "SET redLed=off yellowLed=off greenLed=", "SET redLed=on yellowLed=blink_2 greenLed=bad",
        "SET redLed=bad yellowLed=off greenLed=off", "SET redLed=off yellowLed=bad greenLed=off",
        "SET yellowLed=off redLed=off greenLed=off", "SET redLed=on redLed=off greenLed=off",
        "SET redLed=blink_3 yellowLed=off greenLed=off", "SET redLed=OFF yellowLed=off greenLed=off",
        "SET redLed=off  yellowLed=off greenLed=off", "SET redLed=off yellowLed=off greenLed=off ",
        " SET redLed=off yellowLed=off greenLed=off", "SET redLed=off yellowLed=off greenLed=off\n",
        "SET redLed=off yellowLed=off greenLed=off\r", "SET redLed=off yellowLed=off greenLed=off extra=1",
        "SET redLed=offx yellowLed=off greenLed=off", "SET redLed=off yellowLed=off greenLed=offx",
    };
    ComandoLeds anterior;
    anterior.rojo = EstadoLed::Encendido;
    anterior.amarillo = EstadoLed::Parpadeo4Hz;
    anterior.verde = EstadoLed::Parpadeo2Hz;
    for (const char* texto : comandosInvalidos) {
        ComandoLeds comando = anterior;
        pruebas.comprobar(!ProtocoloAplicacion::leerComando(texto, comando) && iguales(comando, anterior),
                         std::string("SET invalido sin cambios parciales: ") + texto);
    }

    pruebas.comprobar(ProtocoloAplicacion::crearOk(salida, sizeof(salida)) &&
                     std::strcmp(salida, "OK") == 0 && ProtocoloAplicacion::leerOk(salida), "OK");
    probarBuffer(pruebas, "OK", [](char* texto, std::size_t capacidad) {
        return ProtocoloAplicacion::crearOk(texto, capacidad);
    });
    for (const char* texto : {"ok", "OK ", " OK", "OK\n", "OK\r", "OK extra=1", ""}) {
        pruebas.comprobar(!ProtocoloAplicacion::leerOk(texto), std::string("OK invalido: ") + texto);
    }

    const MotivoError motivos[] = {MotivoError::MensajeInvalido, MotivoError::NoRegistrado,
                                   MotivoError::RolOcupado};
    const char* errores[] = {"ERROR reason=invalid_message", "ERROR reason=not_registered",
                             "ERROR reason=role_busy"};
    for (std::size_t i = 0; i < 3; ++i) {
        MotivoError motivo = MotivoError::RolOcupado;
        pruebas.comprobar(ProtocoloAplicacion::crearError(motivos[i], salida, sizeof(salida)) &&
                         std::strcmp(salida, errores[i]) == 0 &&
                         ProtocoloAplicacion::leerError(salida, motivo) && motivo == motivos[i], errores[i]);
        probarBuffer(pruebas, errores[i], [i, &motivos](char* texto, std::size_t capacidad) {
            return ProtocoloAplicacion::crearError(motivos[i], texto, capacidad);
        });
    }
    for (const char* texto : {"ERROR", "ERROR reason=", "ERROR reason=other", "ERROR reason=role_busy ",
                              "ERROR reason=role_busy\n", "ERROR  reason=role_busy", "error reason=role_busy"}) {
        MotivoError motivo = MotivoError::NoRegistrado;
        pruebas.comprobar(!ProtocoloAplicacion::leerError(texto, motivo) && motivo == MotivoError::NoRegistrado,
                         std::string("ERROR invalido conserva el motivo: ") + texto);
    }

    const char textoInicial[] = "sin cambios";
    std::memcpy(salida, textoInicial, sizeof(textoInicial));
    pruebas.comprobar(!ProtocoloAplicacion::crearRegistro(static_cast<TipoCliente>(255), salida, sizeof(salida)) &&
                     std::strcmp(salida, "sin cambios") == 0, "No emitir rol desconocido");
    pruebas.comprobar(!ProtocoloAplicacion::crearError(static_cast<MotivoError>(255), salida, sizeof(salida)) &&
                     std::strcmp(salida, "sin cambios") == 0, "No emitir motivo desconocido");
    for (int led = 0; led < 3; ++led) {
        ComandoLeds comando;
        if (led == 0) comando.rojo = static_cast<EstadoLed>(255);
        if (led == 1) comando.amarillo = static_cast<EstadoLed>(255);
        if (led == 2) comando.verde = static_cast<EstadoLed>(255);
        pruebas.comprobar(!ProtocoloAplicacion::crearComando(comando, salida, sizeof(salida)) &&
                         std::strcmp(salida, "sin cambios") == 0, "No emitir un estado LED desconocido");
    }

    TipoCliente tipo = TipoCliente::Sensor;
    MotivoError motivo = MotivoError::RolOcupado;
    ComandoLeds comando = anterior;
    distancia = 37.5F;
    pruebas.comprobar(!ProtocoloAplicacion::leerRegistro(nullptr, tipo) && tipo == TipoCliente::Sensor,
                     "Registro nulo");
    pruebas.comprobar(!ProtocoloAplicacion::leerLectura(nullptr, distancia) && distancia == 37.5F,
                     "Lectura nula");
    pruebas.comprobar(!ProtocoloAplicacion::leerComando(nullptr, comando) && iguales(comando, anterior),
                     "Comando nulo");
    pruebas.comprobar(!ProtocoloAplicacion::leerOk(nullptr), "OK nulo");
    pruebas.comprobar(!ProtocoloAplicacion::leerError(nullptr, motivo) && motivo == MotivoError::RolOcupado,
                     "ERROR nulo");

    const std::string localeAnterior = std::setlocale(LC_NUMERIC, nullptr);
    bool localeConComa = false;
    for (const char* locale : {"Spanish_Spain.1252", "es_ES.UTF-8", "de_DE.UTF-8"}) {
        if (std::setlocale(LC_NUMERIC, locale) != nullptr &&
            std::strcmp(std::localeconv()->decimal_point, ".") != 0) {
            localeConComa = true;
            pruebas.comprobar(ProtocoloAplicacion::crearLectura(45.5F, salida, sizeof(salida)) &&
                             std::strcmp(salida, "POST distance=45.50") == 0 &&
                             ProtocoloAplicacion::leerLectura(salida, distancia) && distancia == 45.5F,
                             "El idioma del sistema no cambia el punto del protocolo");
            pruebas.comprobar(!ProtocoloAplicacion::leerLectura("POST distance=45,50", distancia),
                             "Rechazar coma incluso con locale que utiliza coma");
            break;
        }
    }
    std::setlocale(LC_NUMERIC, localeAnterior.c_str());
    if (!localeConComa) {
        std::cout << "Prueba opcional de locale con coma omitida: locale no instalado.\n";
    }

    std::cout << pruebas.total - pruebas.fallos << " de " << pruebas.total
              << " comprobaciones del protocolo aprobadas.\n";
    return pruebas.fallos == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
