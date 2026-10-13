#pragma once

#include <stdint.h>

// Datos ficticios usados solo por el firmware compilado para las pruebas nativas.
namespace ConfiguracionRed {
constexpr char SSID[] = "red-de-prueba";
constexpr char CLAVE[] = "clave-de-prueba";
constexpr char IP_SERVIDOR[] = "127.0.0.1";
constexpr uint16_t PUERTO = 5000;
}
