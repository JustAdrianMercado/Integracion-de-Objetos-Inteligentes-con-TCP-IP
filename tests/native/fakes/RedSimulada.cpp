#include "RedSimulada.h"
#include "WiFi.h"
#include "WiFiClient.h"
#include "lwip/sockets.h"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <sstream>

RedSimulada red;
WiFiSimulado WiFi;
SerialSimulado Serial;

bool IPAddress::fromString(const char* texto) {
    std::istringstream entrada(texto);
    for (size_t i = 0; i < 4; ++i) {
        unsigned int valor;
        if (!(entrada >> valor) || valor > 255) { return false; }
        bytes_[i] = static_cast<uint8_t>(valor);
        if (i < 3 && entrada.get() != '.') { return false; }
    }
    return entrada.peek() == std::char_traits<char>::eof();
}
bool IPAddress::operator==(const IPAddress& otra) const {
    return std::memcmp(bytes_, otra.bytes_, sizeof(bytes_)) == 0;
}
bool WiFiSimulado::mode(int modo) { red.modoWifi = modo; return true; }
bool WiFiSimulado::setAutoReconnect(bool activo) { red.autoReconnect = activo; return true; }
bool WiFiSimulado::setSleep(bool activo) { red.wifiSinAhorro = !activo; return true; }
int WiFiSimulado::status() { return red.wifiConectado ? WL_CONNECTED : WL_DISCONNECTED; }
int WiFiSimulado::begin(const char*, const char*) { ++red.intentosWifi; return status(); }
IPAddress WiFiSimulado::localIP() { return IPAddress(192, 168, 1, 20); }

int WiFiClient::connect(IPAddress, uint16_t puerto, int32_t timeout) {
    ++red.intentosTcp;
    red.puertoTcp = puerto;
    red.timeoutConexion = timeout;
    red.reloj += red.duracionConexion;
    red.tcpAbierto = red.conexionExitosa;
    return red.tcpAbierto ? 1 : 0;
}
int WiFiClient::setNoDelay(bool activo) { red.noDelay = activo; return 0; }
int WiFiClient::fd() const { return red.tcpAbierto ? 42 : -1; }
void WiFiClient::stop() {
    if (red.tcpAbierto) { ++red.cierresTcp; }
    red.tcpAbierto = false;
    red.entrada.clear();
    red.remotoCerrado = false;
}

int send(int socket, const void* datos, std::size_t longitud, int flags) {
    ++red.llamadasEnvio;
    red.flagsEnvio = flags;
    if (socket != 42 || !red.tcpAbierto) { errno = EBADF; return -1; }
    if (red.errorEnvio != 0) { errno = red.errorEnvio; return -1; }
    if (red.maximoEnvio == 0) { errno = EAGAIN; return -1; }
    const std::size_t enviados = std::min(longitud, red.maximoEnvio);
    red.salida.append(static_cast<const char*>(datos), enviados);
    return static_cast<int>(enviados);
}
int recv(int socket, void* datos, std::size_t longitud, int flags) {
    ++red.llamadasRecepcion;
    red.flagsRecepcion = flags;
    if (socket != 42 || !red.tcpAbierto) { errno = EBADF; return -1; }
    if (red.errorRecepcion != 0) { errno = red.errorRecepcion; return -1; }
    if (red.entrada.empty()) {
        if (red.remotoCerrado) { return 0; }
        errno = EAGAIN;
        return -1;
    }
    const std::size_t recibidos = std::min({longitud, red.entrada.size(), red.maximoRecepcion});
    std::memcpy(datos, red.entrada.data(), recibidos);
    red.entrada.erase(0, recibidos);
    return static_cast<int>(recibidos);
}
unsigned long millis() { return red.reloj; }
void pinMode(uint8_t pin, uint8_t modo) { if (pin < 40) { red.modosPin[pin] = modo; } }
void digitalWrite(uint8_t pin, uint8_t valor) {
    if (pin < 40) { red.niveles[pin] = valor; ++red.escriturasPin; }
}
void delayMicroseconds(unsigned int) {}
unsigned long pulseIn(uint8_t, uint8_t, unsigned long) {
    ++red.mediciones;
    red.reloj += red.duracionMedicion;
    if (red.perderWifiDuranteEco) { red.wifiConectado = false; }
    return red.eco;
}
