#include "ControlDistancia.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace {
struct CasoPrueba {
    const char* nombre;
    float distanciaCm;
    EstadoLed rojo;
    EstadoLed amarillo;
    EstadoLed verde;
};

bool comprobarEstados(const char* nombre, const ComandoLeds& comando,
                      EstadoLed rojo, EstadoLed amarillo, EstadoLed verde) {
    if (comando.rojo == rojo && comando.amarillo == amarillo &&
        comando.verde == verde) {
        return true;
    }

    std::cerr << "FALLO: " << nombre
              << "\n  Esperado (rojo, amarillo, verde): "
              << static_cast<int>(rojo) << ", " << static_cast<int>(amarillo)
              << ", " << static_cast<int>(verde)
              << "\n  Obtenido (rojo, amarillo, verde): "
              << static_cast<int>(comando.rojo) << ", "
              << static_cast<int>(comando.amarillo) << ", "
              << static_cast<int>(comando.verde) << '\n';
    return false;
}
}

int main() {
    const EstadoLed apagado = EstadoLed::Apagado;
    const EstadoLed encendido = EstadoLed::Encendido;
    const EstadoLed parpadeo2Hz = EstadoLed::Parpadeo2Hz;
    const EstadoLed parpadeo4Hz = EstadoLed::Parpadeo4Hz;
    const float infinito = std::numeric_limits<float>::infinity();

    const CasoPrueba casos[] = {
        {"Cero", 0.0F, apagado, apagado, apagado},
        {"Cero negativo", -0.0F, apagado, apagado, apagado},
        {"Distancia negativa", -1.0F, apagado, apagado, apagado},
        {"Valor finito negativo extremo", std::numeric_limits<float>::lowest(),
         apagado, apagado, apagado},
        {"NaN", std::numeric_limits<float>::quiet_NaN(),
         apagado, apagado, apagado},
        {"Infinito positivo", infinito, apagado, apagado, apagado},
        {"Infinito negativo", -infinito, apagado, apagado, apagado},
        {"Positivo mas cercano a cero", std::nextafter(0.0F, 1.0F),
         parpadeo4Hz, apagado, apagado},
        {"Distancia cercana: 3 cm", 3.0F, parpadeo4Hz, apagado, apagado},
        {"Distancia cercana: 45 cm", 45.0F, parpadeo4Hz, apagado, apagado},
        {"Distancia cercana: 49.99 cm", 49.99F,
         parpadeo4Hz, apagado, apagado},
        {"Valor representable anterior a 50 cm", std::nextafter(50.0F, 0.0F),
         parpadeo4Hz, apagado, apagado},
        {"Limite exacto de 50 cm", 50.0F, apagado, parpadeo2Hz, apagado},
        {"Valor representable posterior a 50 cm", std::nextafter(50.0F, infinito),
         apagado, parpadeo2Hz, apagado},
        {"Distancia intermedia: 75 cm", 75.0F,
         apagado, parpadeo2Hz, apagado},
        {"Distancia intermedia: 99.99 cm", 99.99F,
         apagado, parpadeo2Hz, apagado},
        {"Valor representable anterior a 100 cm", std::nextafter(100.0F, 0.0F),
         apagado, parpadeo2Hz, apagado},
        {"Limite exacto de 100 cm", 100.0F, apagado, apagado, encendido},
        {"Valor representable posterior a 100 cm", std::nextafter(100.0F, infinito),
         apagado, apagado, encendido},
        {"Distancia mayor: 120 cm", 120.0F, apagado, apagado, encendido},
        {"Extremo del intervalo de pruebas: 150 cm", 150.0F,
         apagado, apagado, encendido},
        {"Mayor distancia finita", std::numeric_limits<float>::max(),
         apagado, apagado, encendido},
        {"Lectura invalida despues del rango verde", 0.0F,
         apagado, apagado, apagado},
        {"Volver al rango rojo", 45.0F, parpadeo4Hz, apagado, apagado},
        {"Lectura invalida despues del rango rojo", -1.0F,
         apagado, apagado, apagado},
    };

    std::size_t fallos = 0;
    const ComandoLeds comandoInicial;
    if (!comprobarEstados("Orden inicial con todos apagados", comandoInicial,
                         apagado, apagado, apagado)) {
        ++fallos;
    }

    const ControlDistancia control;
    for (const CasoPrueba& caso : casos) {
        const ComandoLeds comando = control.calcularComando(caso.distanciaCm);
        if (!comprobarEstados(caso.nombre, comando,
                             caso.rojo, caso.amarillo, caso.verde)) {
            ++fallos;
        }
    }

    const std::size_t totalCasos = sizeof(casos) / sizeof(casos[0]) + 1;
    if (fallos != 0) {
        std::cerr << fallos << " de " << totalCasos << " casos fallaron.\n";
        return EXIT_FAILURE;
    }

    std::cout << totalCasos << " casos aprobados.\n";
    return EXIT_SUCCESS;
}
