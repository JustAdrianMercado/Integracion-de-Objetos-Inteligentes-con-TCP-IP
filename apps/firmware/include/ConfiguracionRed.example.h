#pragma once

#include <stdint.h>

// Copiar a ConfiguracionRed.h y completar en la PC local. Ese archivo no se sube.
namespace ConfiguracionRed {
constexpr char SSID[] = "";                  // Nombre de la red WiFi de 2.4 GHz.
constexpr char CLAVE[] = "";                 // Vacia solamente si la red es abierta.
constexpr char IP_SERVIDOR[] = "0.0.0.0";    // IPv4 real de la PC; obtener con ipconfig.
constexpr uint16_t PUERTO = 5000;
}
