#include "ColaEnvio.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>

namespace {
std::size_t total = 0;
std::size_t fallos = 0;

void comprobar(bool resultado, const char* nombre) {
    ++total;
    if (!resultado) {
        ++fallos;
        std::cerr << "FALLO: " << nombre << '\n';
    }
}

std::string vaciar(ColaEnvio& cola) {
    std::string resultado;
    while (!cola.vacia()) {
        const std::size_t cantidad = cola.disponibles();
        resultado.append(cola.datos(), cantidad);
        cola.avanzar(cantidad);
    }
    return resultado;
}
}

int main() {
    ColaEnvio cola;
    comprobar(cola.vacia() && cola.datos() == nullptr && cola.disponibles() == 0, "Cola inicial vacia");
    comprobar(cola.encolar("OK") && cola.encolar("SET redLed=off yellowLed=off greenLed=on"),
              "Encolar mensajes consecutivos");
    const std::string esperado = "OK\nSET redLed=off yellowLed=off greenLed=on\n";
    std::string enviado;
    const std::size_t escrituras[] = {1, 0, 1, 0, 2, 7, 0, 3, 4, 0, 9, 1};
    std::size_t ciclo = 0;
    while (!cola.vacia()) {
        const auto cantidad = std::min(cola.disponibles(), escrituras[ciclo++ % 12]);
        // Cero simula WSAEWOULDBLOCK: no debe avanzar ni repetir lo ya enviado.
        enviado.append(cola.datos(), cantidad);
        comprobar(cola.avanzar(cantidad), "Aceptar el progreso real de una escritura parcial");
    }
    comprobar(enviado == esperado, "Escrituras parciales conservan orden, LF y todos los bytes sin duplicados");
    comprobar(cola.datos() == nullptr && cola.disponibles() == 0, "Sin datos despues de completar los envios");

    cola.encolar("OK");
    comprobar(!cola.avanzar(4) && cola.disponibles() == 3 && std::string(cola.datos(), 3) == "OK\n",
              "Rechazar avance mayor que los bytes pendientes sin alterar el mensaje");
    comprobar(vaciar(cola) == "OK\n", "Seguir enviando despues de un avance rechazado");

    const char* contenidosInvalidos[] = {nullptr, "", "OK\n", "OK\r", "OK\t"};
    for (const char* contenido : contenidosInvalidos) {
        comprobar(!cola.encolar(contenido) && cola.vacia(), "No encolar contenido nulo, vacio o con controles");
    }
    const std::string limite(127, 'x');
    comprobar(cola.encolar(limite.c_str()) && cola.disponibles() == 128 && vaciar(cola) == limite + "\n",
              "127 bytes de contenido y un LF, sin transmitir el terminador C");
    comprobar(!cola.encolar(std::string(128, 'x').c_str()) && cola.vacia(), "No exceder el limite por mensaje");
    comprobar(!cola.encolar(std::string(1, static_cast<char>(255)).c_str()), "No transmitir bytes no ASCII");

    bool llena = true;
    for (std::size_t i = 0; i < 32; ++i) llena = cola.encolar(limite.c_str()) && llena;
    comprobar(llena && !cola.encolar("OK"), "4096 bytes como limite de la cola por conexion");
    cola.avanzar(3);
    comprobar(cola.encolar("OK"), "Una escritura parcial libera capacidad para otro mensaje");
    const std::string restante = vaciar(cola);
    std::string esperadoRestante = limite.substr(3) + "\n";
    for (std::size_t i = 1; i < 32; ++i) esperadoRestante += limite + "\n";
    esperadoRestante += "OK\n";
    comprobar(restante == esperadoRestante, "La saturacion no elimina mensajes ni pierde un prefijo parcial");

    std::cout << total - fallos << " de " << total << " comprobaciones de envio aprobadas.\n";
    return fallos == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
