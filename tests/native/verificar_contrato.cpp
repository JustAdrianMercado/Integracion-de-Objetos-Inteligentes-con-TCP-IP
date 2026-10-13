#include "ControlDistancia.h"
#include "ProtocoloAplicacion.h"
#include "ReceptorLineas.h"

#include <iostream>
#include <string>

namespace {
int digitoHex(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

std::string canonizar(const char* linea) {
    TipoCliente tipo;
    MotivoError motivo;
    ComandoLeds comando;
    float distancia;
    char salida[ProtocoloAplicacion::CapacidadBuffer];
    if (ProtocoloAplicacion::leerRegistro(linea, tipo)) {
        ProtocoloAplicacion::crearRegistro(tipo, salida, sizeof(salida));
    } else if (ProtocoloAplicacion::leerLectura(linea, distancia)) {
        ProtocoloAplicacion::crearComando(ControlDistancia().calcularComando(distancia), salida, sizeof(salida));
    } else if (ProtocoloAplicacion::leerComando(linea, comando)) {
        ProtocoloAplicacion::crearComando(comando, salida, sizeof(salida));
    } else if (ProtocoloAplicacion::leerOk(linea)) {
        ProtocoloAplicacion::crearOk(salida, sizeof(salida));
    } else if (ProtocoloAplicacion::leerError(linea, motivo)) {
        ProtocoloAplicacion::crearError(motivo, salida, sizeof(salida));
    } else {
        return "INVALID";
    }
    return salida;
}

std::string comprobar(const std::string& hexadecimal) {
    if (hexadecimal.size() % 2 != 0) return "INVALID";
    std::string contenido;
    for (std::size_t i = 0; i < hexadecimal.size(); i += 2) {
        const int alto = digitoHex(hexadecimal[i]);
        const int bajo = digitoHex(hexadecimal[i + 1]);
        if (alto < 0 || bajo < 0) return "INVALID";
        contenido.push_back(static_cast<char>(alto * 16 + bajo));
    }
    contenido.push_back('\n');
    ReceptorLineas receptor;
    std::string resultado;
    for (char byte : contenido) {
        const auto estado = receptor.agregar(byte);
        if (estado == ResultadoRecepcion::MensajeInvalido) return "INVALID";
        if (estado == ResultadoRecepcion::LineaCompleta) {
            if (!resultado.empty()) return "INVALID";
            resultado = canonizar(receptor.linea());
        }
    }
    return resultado.empty() ? "INVALID" : resultado;
}
}

int main() {
    std::string entrada;
    while (std::getline(std::cin, entrada)) std::cout << comprobar(entrada) << '\n';
    return 0;
}
