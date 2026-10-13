#include "ClienteTcp.h"

#include <errno.h>
#include <lwip/sockets.h>
#include <string.h>

#include "ComandoLeds.h"
#include "MotivoError.h"
#include "ProtocoloAplicacion.h"

ClienteTcp::ClienteTcp(TipoCliente tipo, const char* ssid, const char* clave,
                       const char* ipServidor, uint16_t puerto)
    : tipo_(tipo), ssid_(ssid), clave_(clave), ipServidor_(ipServidor), puerto_(puerto) {
}

void ClienteTcp::limpiarBuffers() {
    receptor_.reiniciar();
    longitudEnvio_ = 0;
    enviados_ = 0;
    primeraRecibida_ = 0;
    cantidadRecibidas_ = 0;
}

void ClienteTcp::iniciar() {
    socket_.stop();
    limpiarBuffers();
    estado_ = Estado::Deshabilitado;
    wifiConectado_ = false;
    char registro[ProtocoloAplicacion::CapacidadBuffer];
    if (ssid_ == nullptr || ssid_[0] == '\0' || strlen(ssid_) > 32 ||
        clave_ == nullptr || strlen(clave_) > 64 || ipServidor_ == nullptr ||
        !direccion_.fromString(ipServidor_) || direccion_ == IPAddress(0, 0, 0, 0) ||
        puerto_ == 0 || !ProtocoloAplicacion::crearRegistro(tipo_, registro, sizeof(registro))) {
        Serial.println("[Red] Completar apps/firmware/include/ConfiguracionRed.h y recompilar.");
        return;
    }

    WiFi.mode(WIFI_STA);
    // El ciclo principal controla todos los reintentos con su propio intervalo.
    WiFi.setAutoReconnect(false);
    const uint32_t ahora = static_cast<uint32_t>(millis());
    ultimoIntentoWifi_ = ahora - INTERVALO_REINTENTO_MS;
    ultimoIntentoTcp_ = ahora - INTERVALO_REINTENTO_MS;
    estado_ = Estado::Desconectado;
}

void ClienteTcp::cerrar(uint32_t ahora, const char* motivo) {
    Serial.print("[TCP] ");
    Serial.println(motivo);
    socket_.stop();
    limpiarBuffers();
    estado_ = Estado::Desconectado;
    ultimoIntentoTcp_ = ahora;
}

bool ClienteTcp::estaRegistrado() const {
    return estado_ == Estado::Registrado;
}

bool ClienteTcp::puedeEnviar() const {
    return estaRegistrado() && tipo_ == TipoCliente::Sensor && longitudEnvio_ == 0;
}

bool ClienteTcp::prepararEnvio(const char* contenido, uint32_t ahora) {
    if (contenido == nullptr || longitudEnvio_ != 0) {
        return false;
    }
    // Examinar como maximo el limite mas un byte; no copiar una linea demasiado larga.
    size_t longitud = 0;
    while (longitud < sizeof(envio_) && contenido[longitud] != '\0') {
        const unsigned char byte = static_cast<unsigned char>(contenido[longitud]);
        if (byte < 32 || byte > 126) {
            return false;
        }
        ++longitud;
    }
    if (longitud == 0 || longitud > ProtocoloAplicacion::LongitudMaximaContenido) {
        return false;
    }
    memcpy(envio_, contenido, longitud);
    envio_[longitud] = '\n';
    longitudEnvio_ = longitud + 1;
    enviados_ = 0;
    inicioEnvio_ = ahora;
    return true;
}

bool ClienteTcp::enviar(const char* contenido) {
    float distancia = 0;
    return puedeEnviar() && ProtocoloAplicacion::leerLectura(contenido, distancia) &&
           prepararEnvio(contenido, static_cast<uint32_t>(millis()));
}

bool ClienteTcp::recibir(char* linea, size_t capacidad) {
    if (!estaRegistrado() || cantidadRecibidas_ == 0 || linea == nullptr) {
        return false;
    }
    const size_t longitud = strlen(recibidas_[primeraRecibida_]);
    if (capacidad <= longitud) {
        return false;
    }
    memcpy(linea, recibidas_[primeraRecibida_], longitud + 1);
    primeraRecibida_ = (primeraRecibida_ + 1) % CAPACIDAD_RECEPCIONES;
    --cantidadRecibidas_;
    return true;
}

void ClienteTcp::avanzarEnvio(uint32_t ahora) {
    if (longitudEnvio_ == 0) {
        return;
    }
    if (ahora - inicioEnvio_ >= PLAZO_ENVIO_MS) {
        cerrar(ahora, "Envio vencido; reintento en 2000 ms.");
        return;
    }
    // WiFiClient::write de Arduino-ESP32 2.0.17 puede esperar varios segundos.
    // Su fd sigue perteneciendo a WiFiClient; un send sin espera evita esos reintentos internos.
    const int resultado = send(socket_.fd(), envio_ + enviados_,
                               longitudEnvio_ - enviados_, MSG_DONTWAIT);
    if (resultado > 0) {
        enviados_ += static_cast<size_t>(resultado);
        if (enviados_ == longitudEnvio_) {
            longitudEnvio_ = 0;
            enviados_ = 0;
        }
    } else if (resultado == 0 || (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)) {
        cerrar(ahora, "Fallo de envio; reintento en 2000 ms.");
    }
}

bool ClienteTcp::procesarLinea(const char* linea, uint32_t ahora) {
    MotivoError motivo;
    if (ProtocoloAplicacion::leerError(linea, motivo)) {
        Serial.print("[TCP] ");
        Serial.println(linea);
        cerrar(ahora, "Registro o mensaje rechazado; reintento en 2000 ms.");
        return false;
    }
    if (estado_ == Estado::EsperandoOk && longitudEnvio_ == 0 &&
        ProtocoloAplicacion::leerOk(linea)) {
        estado_ = Estado::Registrado;
        Serial.println("[TCP] Registro aceptado (OK).");
        return true;
    }
    ComandoLeds comando;
    if (estado_ == Estado::Registrado && tipo_ == TipoCliente::Actuador &&
        ProtocoloAplicacion::leerComando(linea, comando) &&
        cantidadRecibidas_ < CAPACIDAD_RECEPCIONES) {
        const size_t indice = (primeraRecibida_ + cantidadRecibidas_) % CAPACIDAD_RECEPCIONES;
        memcpy(recibidas_[indice], linea, strlen(linea) + 1);
        ++cantidadRecibidas_;
        return true;
    }
    cerrar(ahora, "Respuesta inesperada o cola llena; reintento en 2000 ms.");
    return false;
}

void ClienteTcp::atenderRecepcion(uint32_t ahora) {
    char bloque[ProtocoloAplicacion::CapacidadBuffer];
    // Una lectura acotada por vuelta. recv=0 detecta tambien el cierre normal del servidor.
    const int recibidos = recv(socket_.fd(), bloque, sizeof(bloque), MSG_DONTWAIT);
    if (recibidos == 0) {
        cerrar(ahora, "Servidor desconectado; reintento en 2000 ms.");
    } else if (recibidos < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
            cerrar(ahora, "Fallo de recepcion; reintento en 2000 ms.");
        }
    } else {
        for (int i = 0; i < recibidos; ++i) {
            const ResultadoRecepcion resultado = receptor_.agregar(bloque[i]);
            if (resultado == ResultadoRecepcion::MensajeInvalido) {
                cerrar(ahora, "Framing invalido; reintento en 2000 ms.");
                return;
            }
            if (resultado == ResultadoRecepcion::LineaCompleta &&
                !procesarLinea(receptor_.linea(), ahora)) {
                return;
            }
        }
    }
}

void ClienteTcp::actualizar() {
    if (estado_ == Estado::Deshabilitado) {
        return;
    }
    uint32_t ahora = static_cast<uint32_t>(millis());
    if (WiFi.status() != WL_CONNECTED) {
        if (wifiConectado_) {
            wifiConectado_ = false;
            ultimoIntentoWifi_ = ahora;
            cerrar(ahora, "WiFi desconectado; reintento en 2000 ms.");
        }
        if (ahora - ultimoIntentoWifi_ >= INTERVALO_REINTENTO_MS) {
            Serial.println("[WiFi] Intentando conectar.");
            WiFi.begin(ssid_, clave_);
            ultimoIntentoWifi_ = static_cast<uint32_t>(millis());
        }
        return;
    }
    if (!wifiConectado_) {
        wifiConectado_ = true;
        Serial.print("[WiFi] Conectado; IP local: ");
        Serial.println(WiFi.localIP());
    }
    if (estado_ == Estado::Desconectado) {
        if (ahora - ultimoIntentoTcp_ < INTERVALO_REINTENTO_MS) {
            return;
        }
        Serial.print("[TCP] Intentando conectar a ");
        Serial.print(ipServidor_);
        Serial.print(":");
        Serial.println(puerto_);
        const bool conectado = socket_.connect(direccion_, puerto_, TIMEOUT_CONEXION_MS) == 1;
        ahora = static_cast<uint32_t>(millis());
        ultimoIntentoTcp_ = ahora;
        if (!conectado) {
            socket_.stop();
            Serial.println("[TCP] Conexion fallida; reintento en 2000 ms.");
            return;
        }
        socket_.setNoDelay(true);
        limpiarBuffers();
        aperturaTcp_ = ahora;
        estado_ = Estado::EsperandoOk;
        char registro[ProtocoloAplicacion::CapacidadBuffer];
        ProtocoloAplicacion::crearRegistro(tipo_, registro, sizeof(registro));
        prepararEnvio(registro, ahora);
    }
    if (estado_ == Estado::EsperandoOk && ahora - aperturaTcp_ >= PLAZO_REGISTRO_MS) {
        cerrar(ahora, "Plazo de registro vencido; reintento en 2000 ms.");
        return;
    }
    atenderRecepcion(ahora);
    if (estado_ == Estado::EsperandoOk || estado_ == Estado::Registrado) {
        avanzarEnvio(ahora);
    }
}
