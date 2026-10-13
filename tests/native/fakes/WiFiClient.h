#pragma once

#include "WiFi.h"

class WiFiClient {
public:
    int connect(IPAddress ip, uint16_t puerto, int32_t timeout);
    int setNoDelay(bool activo);
    int fd() const;
    void stop();
};
