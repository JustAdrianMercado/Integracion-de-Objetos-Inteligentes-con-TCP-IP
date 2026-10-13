#pragma once

#include "Arduino.h"

#include <string>

struct RedSimulada {
    uint32_t reloj = 0;
    bool wifiConectado = false;
    bool autoReconnect = true;
    int modoWifi = 0;
    int intentosWifi = 0;
    int intentosTcp = 0;
    int cierresTcp = 0;
    uint16_t puertoTcp = 0;
    int32_t timeoutConexion = 0;
    uint32_t duracionConexion = 0;
    bool conexionExitosa = true;
    bool tcpAbierto = false;
    bool noDelay = false;
    bool remotoCerrado = false;
    std::string entrada;
    std::string salida;
    std::size_t maximoEnvio = 128;
    std::size_t maximoRecepcion = 128;
    int errorEnvio = 0;
    int errorRecepcion = 0;
    int llamadasEnvio = 0;
    int llamadasRecepcion = 0;
    int flagsEnvio = 0;
    int flagsRecepcion = 0;
    unsigned long eco = 0;
    uint32_t duracionMedicion = 0;
    int mediciones = 0;
    bool perderWifiDuranteEco = false;
};
extern RedSimulada red;
