#include "RedSimulada.h"

#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

void setup();
void loop();

// Python enlaza este firmware con sockets TCP reales. Solo se simulan WiFi, reloj y GPIO.
// Usa el mismo formato que puente_sensor; la segunda columna informa los LEDs rojo, amarillo y verde.
int main() {
    setup();
    uint32_t tiempo;
    unsigned long eco;
    int wifi, cerrado;
    size_t limiteEnvio;
    std::string entrada;
    while (std::cin >> tiempo >> eco >> wifi >> cerrado >> limiteEnvio >> entrada) {
        red.reloj = tiempo;
        red.wifiConectado = wifi != 0;
        red.remotoCerrado = cerrado != 0;
        red.maximoEnvio = limiteEnvio;
        if (entrada != "-") {
            for (size_t i = 0; i < entrada.size(); i += 2) {
                red.entrada.push_back(static_cast<char>(std::stoul(entrada.substr(i, 2), nullptr, 16)));
            }
        }
        loop();
        std::ostringstream salida;
        salida << std::hex << std::setfill('0');
        for (unsigned char byte : red.salida) { salida << std::setw(2) << static_cast<unsigned int>(byte); }
        std::cout << red.intentosTcp << ' '
                  << static_cast<int>(red.niveles[27]) << static_cast<int>(red.niveles[26])
                  << static_cast<int>(red.niveles[25]) << ' ' << red.tcpAbierto << ' '
                  << (red.salida.empty() ? "-" : salida.str()) << std::endl;
        red.salida.clear();
    }
    return EXIT_SUCCESS;
}
