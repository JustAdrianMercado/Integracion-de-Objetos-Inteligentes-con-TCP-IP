#include "ColaEnvio.h"
#include "ProtocoloAplicacion.h"

bool ColaEnvio::encolar(const char* contenido) {
    if (contenido == nullptr || contenido[0] == '\0') {
        return false;
    }
    std::size_t longitud = 0;
    while (contenido[longitud] != '\0') {
        const auto byte = static_cast<unsigned char>(contenido[longitud]);
        if (longitud == ProtocoloAplicacion::LongitudMaximaContenido || byte < 32 || byte > 126) {
            return false;
        }
        ++longitud;
    }
    if (longitud + 1 > CapacidadMaxima - pendientes_) {
        return false;
    }
    mensajes_.emplace_back(contenido, longitud);
    mensajes_.back().push_back('\n');
    pendientes_ += longitud + 1;
    return true;
}

const char* ColaEnvio::datos() const {
    return vacia() ? nullptr : mensajes_.front().data() + desplazamiento_;
}

std::size_t ColaEnvio::disponibles() const {
    return vacia() ? 0 : mensajes_.front().size() - desplazamiento_;
}

bool ColaEnvio::avanzar(std::size_t enviados) {
    if (enviados > disponibles()) {
        return false;
    }
    desplazamiento_ += enviados;
    pendientes_ -= enviados;
    if (!vacia() && desplazamiento_ == mensajes_.front().size()) {
        mensajes_.pop_front();
        desplazamiento_ = 0;
    }
    return true;
}

bool ColaEnvio::vacia() const {
    return mensajes_.empty();
}
