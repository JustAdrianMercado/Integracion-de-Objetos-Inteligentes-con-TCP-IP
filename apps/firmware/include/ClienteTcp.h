#pragma once

#include <WiFi.h>
#include <WiFiClient.h>

#include "ReceptorLineas.h"
#include "Tipos.h"

// Conexion WiFi/TCP comun a ambos ESP32. Se atiende sin bloqueo llamando actualizar()
// en cada vuelta de loop(). Los textos de configuracion deben vivir tanto como el cliente.
class ClienteTcp {
public:
    ClienteTcp(TipoCliente tipo, const char* ssid, const char* clave,
               const char* ipServidor, uint16_t puerto);
    void iniciar();
    void actualizar();
    bool estaRegistrado() const;
    bool puedeEnviar() const;
    // Sensor: encola un POST sin LF. Solo existe un envio pendiente a la vez.
    bool enviar(const char* contenido);
    // Actuador: entrega el ultimo SET completo recibido, sin LF.
    bool recibir(char* linea, size_t capacidad);

private:
    enum class Estado : uint8_t { Deshabilitado, Desconectado, EsperandoOk, Registrado };
    static constexpr uint32_t INTERVALO_REINTENTO_MS = 2000;
    // Asociacion WPA2 y DHCP pueden tardar varios segundos: no reiniciar un intento en curso.
    static constexpr uint32_t PLAZO_ASOCIACION_WIFI_MS = 10000;
    static constexpr uint32_t PLAZO_REGISTRO_MS = 2000;
    static constexpr uint32_t PLAZO_ENVIO_MS = 2000;
    static constexpr int32_t TIMEOUT_CONEXION_MS = 1000;

    TipoCliente tipo_;
    const char* ssid_;
    const char* clave_;
    const char* ipServidor_;
    uint16_t puerto_;
    IPAddress direccion_;
    WiFiClient socket_;
    Estado estado_ = Estado::Deshabilitado;
    bool wifiConectado_ = false;
    uint32_t ultimoIntentoWifi_ = 0;
    uint32_t esperaWifi_ = 0;
    uint32_t ultimoIntentoTcp_ = 0;
    uint32_t aperturaTcp_ = 0;
    uint32_t inicioEnvio_ = 0;
    ReceptorLineas receptor_;
    char envio_[ProtocoloAplicacion::CapacidadBuffer] = {};
    size_t longitudEnvio_ = 0;
    size_t enviados_ = 0;
    char comando_[ProtocoloAplicacion::CapacidadBuffer] = {};
    bool hayComando_ = false;

    void limpiarBuffers();
    void cerrar(uint32_t ahora, const char* motivo);
    bool prepararEnvio(const char* contenido, uint32_t ahora);
    void avanzarEnvio(uint32_t ahora);
    void atenderRecepcion(uint32_t ahora);
    bool procesarLinea(const char* linea, uint32_t ahora);
};
