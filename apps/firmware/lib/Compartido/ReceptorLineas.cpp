#include "ReceptorLineas.h"

ResultadoRecepcion ReceptorLineas::agregar(char byte) {
    if (rechazada_) {
        return ResultadoRecepcion::MensajeInvalido;
    }
    if (completa_) {
        longitud_ = 0;
        completa_ = false;
        buffer_[0] = '\0';
    }
    if (byte == '\n' && longitud_ != 0) {
        buffer_[longitud_] = '\0';
        completa_ = true;
        return ResultadoRecepcion::LineaCompleta;
    }

    const unsigned char dato = static_cast<unsigned char>(byte);
    if (dato < 32 || dato > 126 ||
        longitud_ == ProtocoloAplicacion::LongitudMaximaContenido) {
        rechazada_ = true;
        buffer_[0] = '\0';
        return ResultadoRecepcion::MensajeInvalido;
    }
    buffer_[longitud_++] = byte;
    buffer_[longitud_] = '\0';
    return ResultadoRecepcion::Incompleto;
}

const char* ReceptorLineas::linea() const {
    return completa_ && !rechazada_ ? buffer_ : nullptr;
}

void ReceptorLineas::reiniciar() {
    longitud_ = 0;
    completa_ = false;
    rechazada_ = false;
    buffer_[0] = '\0';
}
