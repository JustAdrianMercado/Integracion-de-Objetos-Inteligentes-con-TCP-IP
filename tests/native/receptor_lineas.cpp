#include "ReceptorLineas.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {
struct Pruebas {
    std::size_t total = 0;
    std::size_t fallos = 0;

    void comprobar(bool resultado, const char* nombre) {
        ++total;
        if (!resultado) {
            ++fallos;
            std::cerr << "FALLO: " << nombre << '\n';
        }
    }
};

// Simula cada bloque entregado por un socket. Copia las lineas antes del byte siguiente.
bool entregar(ReceptorLineas& receptor, const std::string& bloque,
              std::vector<std::string>& lineas) {
    for (char byte : bloque) {
        const ResultadoRecepcion resultado = receptor.agregar(byte);
        if (resultado == ResultadoRecepcion::MensajeInvalido) {
            return false;
        }
        if (resultado == ResultadoRecepcion::LineaCompleta) {
            lineas.emplace_back(receptor.linea());
        }
    }
    return true;
}
}

int main() {
    Pruebas pruebas;
    ReceptorLineas receptor;
    std::vector<std::string> lineas;
    pruebas.comprobar(receptor.linea() == nullptr, "Sin linea al iniciar");
    pruebas.comprobar(entregar(receptor, "REGI", lineas) && lineas.empty() && receptor.linea() == nullptr,
                     "Conservar primer fragmento sin procesarlo");
    pruebas.comprobar(entregar(receptor, "STER type=sensor\nPOST distance=45", lineas) &&
                     lineas == std::vector<std::string>{"REGISTER type=sensor"} && receptor.linea() == nullptr,
                     "Procesar la primera linea y conservar la segunda incompleta");
    pruebas.comprobar(entregar(receptor, ".00\n", lineas) &&
                     lineas == std::vector<std::string>{"REGISTER type=sensor", "POST distance=45.00"},
                     "Completar POST una sola vez");

    receptor.reiniciar();
    lineas.clear();
    pruebas.comprobar(entregar(receptor, "OK\nSET redLed=blink_4 yellowLed=off greenLed=off\nPOST dist", lineas) &&
                     lineas == std::vector<std::string>{"OK", "SET redLed=blink_4 yellowLed=off greenLed=off"} &&
                     receptor.linea() == nullptr, "Varias lineas completas y fragmento final en un bloque");
    pruebas.comprobar(entregar(receptor, "ance=invalid\n", lineas) && lineas.size() == 3 &&
                     lineas.back() == "POST distance=invalid", "Retener el fragmento del siguiente mensaje");

    receptor.reiniciar();
    lineas.clear();
    bool soloAlFinal = true;
    const std::string byteAByte = "REGISTER type=actuator\n";
    for (std::size_t i = 0; i < byteAByte.size(); ++i) {
        const auto resultado = receptor.agregar(byteAByte[i]);
        soloAlFinal = soloAlFinal && resultado == (i + 1 == byteAByte.size()
            ? ResultadoRecepcion::LineaCompleta : ResultadoRecepcion::Incompleto);
    }
    pruebas.comprobar(soloAlFinal && std::string(receptor.linea()) == "REGISTER type=actuator",
                     "Registro recibido byte a byte, disponible solo con LF");

    receptor.reiniciar();
    const std::string prefijo = "POST distance=";
    const std::string contenido127 = prefijo + std::string(127 - prefijo.size() - 1, '0') + "1";
    pruebas.comprobar(entregar(receptor, contenido127, lineas) && lineas.empty() && receptor.linea() == nullptr,
                     "127 bytes de contenido esperan LF");
    pruebas.comprobar(entregar(receptor, "\n", lineas) && lineas.size() == 1 && lineas[0] == contenido127,
                     "128 bytes en el cable: 127 de contenido mas LF");

    receptor.reiniciar();
    lineas.clear();
    pruebas.comprobar(entregar(receptor, std::string(127, 'x'), lineas) &&
                     receptor.agregar('x') == ResultadoRecepcion::MensajeInvalido && receptor.linea() == nullptr,
                     "Rechazo inmediato del byte 128 antes de LF");
    pruebas.comprobar(receptor.agregar('\n') == ResultadoRecepcion::MensajeInvalido &&
                     !entregar(receptor, "OK\n", lineas) && lineas.empty(),
                     "Una conexion rechazada no se resincroniza con mensajes posteriores");
    receptor.reiniciar();
    pruebas.comprobar(entregar(receptor, "OK\n", lineas) && lineas == std::vector<std::string>{"OK"},
                     "Reiniciar permite recibir en una conexion nueva");

    receptor.reiniciar();
    lineas.clear();
    entregar(receptor, "POST distance=4", lineas);
    receptor.reiniciar();
    pruebas.comprobar(entregar(receptor, "OK\n", lineas) && lineas == std::vector<std::string>{"OK"},
                     "La reconexion descarta el fragmento anterior");

    const std::vector<std::string> bloquesInvalidos = {
        "\n", "OK\r\n", "OK\t\n", std::string("OK\0extra\n", 9),
        std::string(1, static_cast<char>(127)), std::string(1, static_cast<char>(128)),
        std::string(1, static_cast<char>(255)), std::string(10000, 'x'),
    };
    for (const std::string& bloque : bloquesInvalidos) {
        receptor.reiniciar();
        lineas.clear();
        pruebas.comprobar(!entregar(receptor, bloque, lineas) && lineas.empty() && receptor.linea() == nullptr,
                         "Rechazar linea vacia, control, NUL, no ASCII o longitud excesiva");
    }

    receptor.reiniciar();
    lineas.clear();
    pruebas.comprobar(!entregar(receptor, "OK\n\n", lineas) && lineas == std::vector<std::string>{"OK"},
                     "Un segundo LF no entrega una linea vacia ni duplica OK");

    ReceptorLineas sensor;
    ReceptorLineas actuador;
    std::vector<std::string> lineasSensor;
    std::vector<std::string> lineasActuador;
    entregar(sensor, "POST distance=", lineasSensor);
    entregar(actuador, "REGISTER type=", lineasActuador);
    pruebas.comprobar(entregar(sensor, "45.00\n", lineasSensor) &&
                     entregar(actuador, "actuator\n", lineasActuador) &&
                     lineasSensor == std::vector<std::string>{"POST distance=45.00"} &&
                     lineasActuador == std::vector<std::string>{"REGISTER type=actuator"},
                     "Buffers independientes para conexiones intercaladas");

    std::cout << pruebas.total - pruebas.fallos << " de " << pruebas.total
              << " comprobaciones del receptor aprobadas.\n";
    return pruebas.fallos == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
