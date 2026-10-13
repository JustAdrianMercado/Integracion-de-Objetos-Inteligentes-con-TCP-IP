#include "ServidorTcp.h"

#include <ws2tcpip.h>

#include <algorithm>
#include <ostream>

namespace {
bool hacerNoBloqueante(SOCKET socket) {
    u_long modo = 1;
    return ioctlsocket(socket, FIONBIO, &modo) == 0;
}
}

ServidorTcp::ServidorTcp(std::ostream* registro) : registro_(registro) {}

ServidorTcp::~ServidorTcp() {
    detener();
}

bool ServidorTcp::iniciar(std::uint16_t puerto, const char* direccion) {
    if (winsockIniciado_) {
        error_ = "El servidor ya esta iniciado.";
        return false;
    }
    error_.clear();
    if (direccion == nullptr) {
        error_ = "La direccion IPv4 es nula.";
        return false;
    }
    WSADATA datos{};
    const int inicio = WSAStartup(MAKEWORD(2, 2), &datos);
    if (inicio != 0) {
        return falloInicio("WSAStartup", inicio);
    }
    winsockIniciado_ = true;
    if (LOBYTE(datos.wVersion) != 2 || HIBYTE(datos.wVersion) != 2) {
        return falloInicio("Version de Winsock incompatible", WSAVERNOTSUPPORTED);
    }
    sockaddr_in local{};
    local.sin_family = AF_INET;
    local.sin_port = htons(puerto);
    if (InetPtonA(AF_INET, direccion, &local.sin_addr) != 1) {
        error_ = "Direccion IPv4 invalida: " + std::string(direccion);
        detener();
        return false;
    }
    escucha_ = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (escucha_ == INVALID_SOCKET) {
        return falloInicio("socket", WSAGetLastError());
    }
    const BOOL exclusivo = TRUE;
    if (setsockopt(escucha_, SOL_SOCKET, SO_EXCLUSIVEADDRUSE,
                   reinterpret_cast<const char*>(&exclusivo), sizeof(exclusivo)) == SOCKET_ERROR) {
        return falloInicio("SO_EXCLUSIVEADDRUSE", WSAGetLastError());
    }
    if (::bind(escucha_, reinterpret_cast<const sockaddr*>(&local), sizeof(local)) == SOCKET_ERROR) {
        return falloInicio("bind (IP no local o puerto ocupado)", WSAGetLastError());
    }
    if (::listen(escucha_, SOMAXCONN) == SOCKET_ERROR || !hacerNoBloqueante(escucha_)) {
        return falloInicio("listen/FIONBIO", WSAGetLastError());
    }
    int longitud = sizeof(local);
    if (getsockname(escucha_, reinterpret_cast<sockaddr*>(&local), &longitud) == SOCKET_ERROR) {
        return falloInicio("getsockname", WSAGetLastError());
    }
    puerto_ = ntohs(local.sin_port);
    registrar("Servidor TCP escuchando en " + std::string(direccion) + ":" + std::to_string(puerto_));
    return true;
}

bool ServidorTcp::actualizar() {
    if (escucha_ == INVALID_SOCKET) {
        error_ = "El servidor no esta iniciado.";
        return false;
    }
    static_assert(MaximoConexiones + 1 <= FD_SETSIZE, "Demasiadas conexiones para select");
    revisarPlazos(Reloj::now());
    eliminarCerradas();
    fd_set lectura;
    fd_set escritura;
    FD_ZERO(&lectura);
    FD_ZERO(&escritura);
    FD_SET(escucha_, &lectura);
    for (const Conexion& conexion : conexiones_) {
        if (!conexion.cerrando) {
            FD_SET(conexion.socket, &lectura);
        }
        if (!conexion.salida.vacia()) {
            FD_SET(conexion.socket, &escritura);
        }
    }
    const timeval espera{0, 20000};
    const int listos = ::select(0, &lectura, &escritura, nullptr, &espera);
    if (listos == SOCKET_ERROR) {
        const int codigo = WSAGetLastError();
        if (codigo == WSAEINTR) {
            return true;
        }
        error_ = "select fallo (Winsock " + std::to_string(codigo) + ").";
        return false;
    }
    // Una linea incompleta no posterga la caducidad del registro o de las lecturas.
    revisarPlazos(Reloj::now());
    for (Conexion& conexion : conexiones_) {
        if (conexion.socket == INVALID_SOCKET) continue;
        if (FD_ISSET(conexion.socket, &escritura)) {
            enviarPendiente(conexion);
        }
        if (conexion.socket != INVALID_SOCKET && !conexion.cerrando &&
            FD_ISSET(conexion.socket, &lectura)) {
            recibir(conexion);
        }
    }
    // Liberar tambien el cupo de los sockets cerrados durante este ciclo.
    eliminarCerradas();
    if (FD_ISSET(escucha_, &lectura) && !aceptarConexiones()) {
        return false;
    }
    revisarPlazos(Reloj::now());
    eliminarCerradas();
    return true;
}

bool ServidorTcp::aceptarConexiones() {
    // Presupuesto por ciclo: un flujo de conexiones nuevas no monopoliza el servidor.
    for (std::size_t i = 0; i < MaximoConexiones; ++i) {
        const SOCKET socket = ::accept(escucha_, nullptr, nullptr);
        if (socket == INVALID_SOCKET) {
            const int codigo = WSAGetLastError();
            if (codigo == WSAEWOULDBLOCK || codigo == WSAECONNRESET) return true;
            error_ = "accept fallo (Winsock " + std::to_string(codigo) + ").";
            return false;
        }
        if (conexiones_.size() >= MaximoConexiones || !hacerNoBloqueante(socket)) {
            closesocket(socket);
            continue;
        }
        const BOOL sinNagle = TRUE;
        if (setsockopt(socket, IPPROTO_TCP, TCP_NODELAY,
                       reinterpret_cast<const char*>(&sinNagle), sizeof(sinNagle)) == SOCKET_ERROR) {
            closesocket(socket);
            continue;
        }
        conexiones_.emplace_back();
        Conexion& conexion = conexiones_.back();
        conexion.socket = socket;
        conexion.aceptada = Reloj::now();
        conexion.ultimaActividad = conexion.aceptada;
        registrar("Nueva conexion; esperando REGISTER.");
    }
    return true;
}

void ServidorTcp::recibir(Conexion& conexion) {
    char bloque[512];
    const int recibidos = ::recv(conexion.socket, bloque, sizeof(bloque), 0);
    if (recibidos == SOCKET_ERROR) {
        const int codigo = WSAGetLastError();
        if (codigo != WSAEWOULDBLOCK && codigo != WSAEINTR) cerrar(conexion);
        return;
    }
    if (recibidos == 0) {
        cerrar(conexion);
        return;
    }
    for (int i = 0; i < recibidos && !conexion.cerrando && conexion.socket != INVALID_SOCKET; ++i) {
        const ResultadoRecepcion resultado = conexion.receptor.agregar(bloque[i]);
        if (resultado == ResultadoRecepcion::MensajeInvalido) {
            rechazar(conexion, MotivoError::MensajeInvalido);
        } else if (resultado == ResultadoRecepcion::LineaCompleta) {
            procesarLinea(conexion, conexion.receptor.linea());
        }
    }
}

void ServidorTcp::procesarLinea(Conexion& conexion, const char* linea) {
    const Instante ahora = Reloj::now();
    if (!conexion.registrada) {
        TipoCliente tipo;
        if (!ProtocoloAplicacion::leerRegistro(linea, tipo)) {
            float distancia;
            ComandoLeds comando;
            const bool operativo = ProtocoloAplicacion::leerLectura(linea, distancia) ||
                                    ProtocoloAplicacion::leerComando(linea, comando);
            rechazar(conexion, operativo ? MotivoError::NoRegistrado : MotivoError::MensajeInvalido);
            return;
        }
        Conexion*& rol = tipo == TipoCliente::Sensor ? sensor_ : actuador_;
        if (rol != nullptr) {
            rechazar(conexion, MotivoError::RolOcupado);
            return;
        }
        rol = &conexion;
        conexion.registrada = true;
        conexion.ultimaActividad = ahora;
        char contenido[ProtocoloAplicacion::CapacidadBuffer];
        ProtocoloAplicacion::crearOk(contenido, sizeof(contenido));
        if (!encolar(conexion, contenido)) return;
        registrar(tipo == TipoCliente::Sensor ? "REGISTER sensor aceptado." : "REGISTER actuator aceptado.");
        if (tipo == TipoCliente::Actuador) {
            const bool reciente = hayLectura_ && ahora - ultimaLectura_ < PlazoLecturas;
            enviarComando(reciente ? ultimoComando_ : ComandoLeds{});
        }
        return;
    }
    float distancia;
    if (&conexion != sensor_ || !ProtocoloAplicacion::leerLectura(linea, distancia)) {
        rechazar(conexion, MotivoError::MensajeInvalido);
        return;
    }
    conexion.ultimaActividad = ahora;
    ultimaLectura_ = ahora;
    hayLectura_ = true;
    ultimoComando_ = control_.calcularComando(distancia);
    registrar(linea);
    enviarComando(ultimoComando_);
}

void ServidorTcp::enviarPendiente(Conexion& conexion) {
    std::size_t presupuesto = ColaEnvio::CapacidadMaxima;
    while (!conexion.salida.vacia() && presupuesto != 0) {
        const auto longitud = static_cast<int>(std::min(presupuesto, conexion.salida.disponibles()));
        const int enviados = ::send(conexion.socket, conexion.salida.datos(), longitud, 0);
        if (enviados == SOCKET_ERROR) {
            const int codigo = WSAGetLastError();
            if (codigo != WSAEWOULDBLOCK && codigo != WSAEINTR) cerrar(conexion);
            return;
        }
        if (enviados == 0) {
            cerrar(conexion);
            return;
        }
        conexion.salida.avanzar(static_cast<std::size_t>(enviados));
        presupuesto -= static_cast<std::size_t>(enviados);
    }
}

bool ServidorTcp::encolar(Conexion& conexion, const char* contenido) {
    if (!conexion.salida.encolar(contenido)) {
        registrar("Salida saturada; cerrando conexion.");
        cerrar(conexion);
        return false;
    }
    return true;
}

void ServidorTcp::enviarComando(const ComandoLeds& comando) {
    if (actuador_ == nullptr) return;
    char contenido[ProtocoloAplicacion::CapacidadBuffer];
    if (ProtocoloAplicacion::crearComando(comando, contenido, sizeof(contenido)) &&
        encolar(*actuador_, contenido)) {
        registrar(contenido);
    }
}

void ServidorTcp::rechazar(Conexion& conexion, MotivoError motivo) {
    liberarRol(conexion);
    conexion.cerrando = true;
    conexion.inicioCierre = Reloj::now();
    char contenido[ProtocoloAplicacion::CapacidadBuffer];
    ProtocoloAplicacion::crearError(motivo, contenido, sizeof(contenido));
    if (encolar(conexion, contenido)) registrar(contenido);
}

void ServidorTcp::liberarRol(Conexion& conexion) {
    if (sensor_ == &conexion) {
        sensor_ = nullptr;
        hayLectura_ = false;
        ultimoComando_ = ComandoLeds{};
        registrar("Sensor desconectado o rechazado; invalidando lectura.");
        enviarComando(ultimoComando_);
    }
    if (actuador_ == &conexion) {
        actuador_ = nullptr;
        registrar("Actuador desconectado o rechazado.");
    }
    conexion.registrada = false;
}

void ServidorTcp::cerrar(Conexion& conexion) {
    liberarRol(conexion);
    if (conexion.socket != INVALID_SOCKET) {
        ::shutdown(conexion.socket, SD_SEND);
        closesocket(conexion.socket);
        conexion.socket = INVALID_SOCKET;
    }
}

void ServidorTcp::revisarPlazos(Instante ahora) {
    for (Conexion& conexion : conexiones_) {
        if (conexion.socket == INVALID_SOCKET) continue;
        if (conexion.cerrando) {
            if (conexion.salida.vacia() || ahora - conexion.inicioCierre >= PlazoCierre) cerrar(conexion);
        } else if (!conexion.registrada && ahora - conexion.aceptada >= PlazoRegistro) {
            rechazar(conexion, MotivoError::NoRegistrado);
        } else if (&conexion == sensor_ && ahora - conexion.ultimaActividad >= PlazoLecturas) {
            registrar("2000 ms sin POST; cerrando sensor.");
            cerrar(conexion);
        }
    }
}

void ServidorTcp::eliminarCerradas() {
    conexiones_.remove_if([](const Conexion& conexion) { return conexion.socket == INVALID_SOCKET; });
}

bool ServidorTcp::falloInicio(const char* operacion, int codigo) {
    error_ = std::string(operacion) + " fallo (Winsock " + std::to_string(codigo) + ").";
    detener();
    return false;
}

void ServidorTcp::detener() {
    // La desconexion del servidor se refleja en los sockets de ambos clientes.
    sensor_ = nullptr;
    actuador_ = nullptr;
    for (Conexion& conexion : conexiones_) cerrar(conexion);
    conexiones_.clear();
    hayLectura_ = false;
    ultimoComando_ = ComandoLeds{};
    if (escucha_ != INVALID_SOCKET) {
        closesocket(escucha_);
        escucha_ = INVALID_SOCKET;
    }
    if (winsockIniciado_) {
        WSACleanup();
        winsockIniciado_ = false;
    }
    puerto_ = 0;
}

std::uint16_t ServidorTcp::puerto() const {
    return puerto_;
}

const std::string& ServidorTcp::ultimoError() const {
    return error_;
}

void ServidorTcp::registrar(const std::string& mensaje) {
    if (registro_ != nullptr) *registro_ << mensaje << '\n';
}
