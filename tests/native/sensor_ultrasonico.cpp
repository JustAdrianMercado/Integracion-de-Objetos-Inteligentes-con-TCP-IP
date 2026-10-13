#include "SensorUltrasonico.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {
std::vector<std::string> eventos;
unsigned long ecoSimulado = 0;
std::size_t fallos = 0;
std::size_t total = 0;

void comprobar(bool correcto, const char* nombre) {
    ++total;
    if (!correcto) {
        ++fallos;
        std::cerr << "FALLO: " << nombre << '\n';
    }
}
}

void pinMode(uint8_t pin, uint8_t modo) {
    eventos.push_back("modo:" + std::to_string(pin) + ":" + std::to_string(modo));
}
void digitalWrite(uint8_t pin, uint8_t valor) {
    eventos.push_back("gpio:" + std::to_string(pin) + ":" + std::to_string(valor));
}
void delayMicroseconds(unsigned int duracion) {
    eventos.push_back("espera:" + std::to_string(duracion));
}
unsigned long pulseIn(uint8_t pin, uint8_t valor, unsigned long timeout) {
    eventos.push_back("eco:" + std::to_string(pin) + ":" + std::to_string(valor) + ":" + std::to_string(timeout));
    return ecoSimulado;
}

int main() {
    SensorUltrasonico sensor(5, 18);
    sensor.iniciar();
    comprobar(eventos == std::vector<std::string>{"modo:5:1", "modo:18:0", "gpio:5:0"},
              "Configurar TRIG como salida y ECHO como entrada");
    eventos.clear();
    comprobar(std::isnan(sensor.medirDistanciaCm()), "Ausencia de ECHO produce lectura invalida");
    comprobar(eventos == std::vector<std::string>{"gpio:5:0", "espera:2", "gpio:5:1", "espera:10",
                                                 "gpio:5:0", "eco:18:1:30000"},
              "Pulso TRIG de 10 us y timeout ECHO de 30000 us");
    ecoSimulado = 1000;
    comprobar(std::fabs(sensor.medirDistanciaCm() - 17.15F) < 0.0001F, "Convertir duracion de ida y vuelta a cm");
    ecoSimulado = 7000;
    comprobar(std::fabs(sensor.medirDistanciaCm() - 120.05F) < 0.001F, "Medicion en el rango lejano");
    ecoSimulado = 0;
    comprobar(std::isnan(sensor.medirDistanciaCm()), "Una lectura invalida no conserva el eco anterior");
    std::cout << total - fallos << " de " << total << " comprobaciones con GPIO simulados aprobadas.\n";
    return fallos == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
