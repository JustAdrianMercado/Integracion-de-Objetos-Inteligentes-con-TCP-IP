#pragma once

#include "Arduino.h"

constexpr int WIFI_STA = 1;
constexpr int WL_CONNECTED = 3;
constexpr int WL_DISCONNECTED = 6;

class IPAddress {
public:
    IPAddress(uint8_t a = 0, uint8_t b = 0, uint8_t c = 0, uint8_t d = 0)
        : bytes_{a, b, c, d} {}
    bool fromString(const char* texto);
    bool operator==(const IPAddress& otra) const;
private:
    uint8_t bytes_[4];
};

class WiFiSimulado {
public:
    bool mode(int modo);
    bool setAutoReconnect(bool activo);
    bool setSleep(bool activo);
    int status();
    int begin(const char* ssid, const char* clave);
    IPAddress localIP();
};
extern WiFiSimulado WiFi;
