#include "ProtocoloAplicacion.h"

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

namespace {
std::size_t total = 0;
std::size_t fallos = 0;
void comprobar(bool correcto, const std::string& nombre) {
    ++total;
    if (!correcto) { ++fallos; std::cerr << "FALLO: " << nombre << '\n'; }
}
std::string lectura(float distancia) {
    char salida[ProtocoloAplicacion::CapacidadBuffer] = {};
    return ProtocoloAplicacion::crearLectura(distancia, salida, sizeof(salida)) ? salida : "<error>";
}
}

int main() {
    char salida[ProtocoloAplicacion::CapacidadBuffer] = {};
    comprobar(ProtocoloAplicacion::crearRegistro(TipoCliente::Sensor, salida, sizeof(salida)) &&
              std::string(salida) == "REGISTER type=sensor", "Registro del sensor");
    comprobar(ProtocoloAplicacion::crearRegistro(TipoCliente::Actuador, salida, sizeof(salida)) &&
              std::string(salida) == "REGISTER type=actuator", "Registro del actuador");
    char pequeno[5] = "xxxx";
    comprobar(!ProtocoloAplicacion::crearRegistro(TipoCliente::Sensor, pequeno, sizeof(pequeno)) &&
              std::string(pequeno) == "xxxx", "Un buffer pequeno falla sin modificarse");

    comprobar(lectura(45.0F) == "POST distance=45.00", "Dos decimales con punto");
    comprobar(lectura(49.996F) == "POST distance=50.00", "Redondeo a dos decimales");
    comprobar(lectura(0.01F) == "POST distance=0.01", "Distancia pequena valida");
    comprobar(lectura(150.25F) == "POST distance=150.25", "Distancia del rango verde");
    for (float invalida : {0.0F, 0.004F, -1.0F, NAN, INFINITY}) {
        comprobar(lectura(invalida) == "POST distance=invalid", "Lectura invalida: " + std::to_string(invalida));
    }

    struct Caso { const char* linea; EstadoLed rojo; EstadoLed amarillo; EstadoLed verde; };
    const Caso validos[] = {
        {"SET redLed=off yellowLed=off greenLed=off", EstadoLed::Apagado, EstadoLed::Apagado, EstadoLed::Apagado},
        {"SET redLed=blink_4 yellowLed=off greenLed=off", EstadoLed::Parpadeo4Hz, EstadoLed::Apagado, EstadoLed::Apagado},
        {"SET redLed=off yellowLed=blink_2 greenLed=off", EstadoLed::Apagado, EstadoLed::Parpadeo2Hz, EstadoLed::Apagado},
        {"SET redLed=on yellowLed=blink_2 greenLed=on", EstadoLed::Encendido, EstadoLed::Parpadeo2Hz, EstadoLed::Encendido},
    };
    for (const Caso& caso : validos) {
        ComandoLeds comando;
        comprobar(ProtocoloAplicacion::leerComando(caso.linea, comando) && comando.rojo == caso.rojo &&
                  comando.amarillo == caso.amarillo && comando.verde == caso.verde,
                  std::string("SET valido: ") + caso.linea);
    }

    const char* invalidos[] = {
        "", "SET", "SET redLed=on", "SET redLed=on yellowLed=off",
        "SET redLed=on yellowLed=off greenLed=", "SET redLed=on yellowLed=off greenLed=onn",
        "SET redLed=on yellowLed=off greenLed=on ", "SET redLed=on  yellowLed=off greenLed=on",
        "SET greenLed=on yellowLed=off redLed=on", "SET redLed=ON yellowLed=off greenLed=on",
        "SET redLed=blink_3 yellowLed=off greenLed=on", "set redLed=on yellowLed=off greenLed=on",
    };
    for (const char* linea : invalidos) {
        ComandoLeds comando;
        comando.rojo = EstadoLed::Encendido;
        comprobar(!ProtocoloAplicacion::leerComando(linea, comando) && comando.rojo == EstadoLed::Encendido &&
                  comando.verde == EstadoLed::Apagado,
                  std::string("SET invalido no modifica el comando: ") + linea);
    }
    ComandoLeds comando;
    comprobar(!ProtocoloAplicacion::leerComando(nullptr, comando), "Linea nula");

    std::cout << total - fallos << " de " << total << " comprobaciones del protocolo aprobadas.\n";
    return fallos == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
