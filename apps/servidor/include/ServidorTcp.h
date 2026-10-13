#pragma once

#include <winsock2.h>

#include "ColaEnvio.h"
#include "ControlDistancia.h"
#include "ReceptorLineas.h"

#include <chrono>
#include <cstdint>
#include <iosfwd>
#include <list>
#include <string>

// Solo el hilo del ciclo principal modifica sockets, roles y temporizadores.
class ServidorTcp {
public:
    explicit ServidorTcp(std::ostream* registro = nullptr);
    ~ServidorTcp();
    ServidorTcp(const ServidorTcp&) = delete;
    ServidorTcp& operator=(const ServidorTcp&) = delete;

    // Puerto 0 pide un puerto libre al sistema, util para las pruebas locales.
    bool iniciar(std::uint16_t puerto = 5000, const char* direccion = "0.0.0.0");
    bool actualizar();
    void detener();
    std::uint16_t puerto() const;
    const std::string& ultimoError() const;

private:
    using Reloj = std::chrono::steady_clock;
    using Instante = Reloj::time_point;
    static constexpr std::size_t MaximoConexiones = 8;
    static constexpr std::chrono::milliseconds PlazoRegistro{2000};
    static constexpr std::chrono::milliseconds PlazoLecturas{2000};
    static constexpr std::chrono::milliseconds PlazoCierre{2000};

    struct Conexion {
        SOCKET socket = INVALID_SOCKET;
        ReceptorLineas receptor;
        ColaEnvio salida;
        Instante aceptada;
        Instante ultimaActividad;
        Instante inicioCierre;
        bool registrada = false;
        bool cerrando = false;
    };

    bool aceptarConexiones();
    void recibir(Conexion& conexion);
    void enviarPendiente(Conexion& conexion);
    void procesarLinea(Conexion& conexion, const char* linea);
    bool encolar(Conexion& conexion, const char* contenido);
    void enviarComando(const ComandoLeds& comando);
    void rechazar(Conexion& conexion, MotivoError motivo);
    void liberarRol(Conexion& conexion);
    void cerrar(Conexion& conexion);
    void revisarPlazos(Instante ahora);
    void eliminarCerradas();
    bool falloInicio(const char* operacion, int codigo);
    void registrar(const std::string& mensaje);

    SOCKET escucha_ = INVALID_SOCKET;
    bool winsockIniciado_ = false;
    std::uint16_t puerto_ = 0;
    std::list<Conexion> conexiones_;
    Conexion* sensor_ = nullptr;
    Conexion* actuador_ = nullptr;
    ControlDistancia control_;
    bool hayLectura_ = false;
    ComandoLeds ultimoComando_;
    Instante ultimaLectura_;
    std::string error_;
    std::ostream* registro_;
};
