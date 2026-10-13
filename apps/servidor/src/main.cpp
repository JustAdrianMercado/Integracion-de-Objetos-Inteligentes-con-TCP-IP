#include "ServidorTcp.h"

#include <windows.h>

#include <atomic>
#include <charconv>
#include <cstdlib>
#include <iostream>
#include <string>

namespace {
std::atomic<bool> continuar{true};

BOOL WINAPI interrumpir(DWORD evento) {
    if (evento == CTRL_C_EVENT || evento == CTRL_BREAK_EVENT) {
        continuar.store(false);
        return TRUE;
    }
    return FALSE;
}

void mostrarAyuda() {
    std::cout << "Uso: servidor_tcp [--ip IPv4-local] [--puerto 0..65535]\n"
                 "Valores predeterminados: --ip 0.0.0.0 --puerto 5000\n"
                 "Puerto 0 selecciona un puerto libre para pruebas. Ctrl+C detiene el servidor.\n";
}
}

int main(int argc, char* argv[]) {
    std::string direccion = "0.0.0.0";
    std::uint16_t puerto = 5000;
    for (int i = 1; i < argc; ++i) {
        const std::string argumento = argv[i];
        if (argumento == "--help" || argumento == "--ayuda") {
            mostrarAyuda();
            return EXIT_SUCCESS;
        }
        if ((argumento != "--ip" && argumento != "--puerto") || i + 1 == argc) {
            mostrarAyuda();
            return EXIT_FAILURE;
        }
        const std::string valor = argv[++i];
        if (argumento == "--ip") {
            direccion = valor;
        } else {
            unsigned int numero = 0;
            const auto resultado = std::from_chars(valor.data(), valor.data() + valor.size(), numero);
            if (resultado.ec != std::errc{} || resultado.ptr != valor.data() + valor.size() || numero > 65535) {
                std::cerr << "Puerto invalido: " << valor << '\n';
                return EXIT_FAILURE;
            }
            puerto = static_cast<std::uint16_t>(numero);
        }
    }
    if (!SetConsoleCtrlHandler(interrumpir, TRUE)) {
        std::cerr << "No se pudo instalar el manejador de Ctrl+C.\n";
        return EXIT_FAILURE;
    }
    std::cout << std::unitbuf;
    ServidorTcp servidor(&std::cout);
    if (!servidor.iniciar(puerto, direccion.c_str())) {
        std::cerr << servidor.ultimoError() << '\n';
        return EXIT_FAILURE;
    }
    int resultado = EXIT_SUCCESS;
    while (continuar.load()) {
        if (!servidor.actualizar()) {
            std::cerr << servidor.ultimoError() << '\n';
            resultado = EXIT_FAILURE;
            break;
        }
    }
    servidor.detener();
    std::cout << "Servidor detenido.\n";
    SetConsoleCtrlHandler(interrumpir, FALSE);
    return resultado;
}
