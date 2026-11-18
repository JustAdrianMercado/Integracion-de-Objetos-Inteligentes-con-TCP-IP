#include "ReceptorLineas.h"

// Reconstruye lineas del flujo TCP, que puede entregar un mensaje en varios
// fragmentos o varios mensajes juntos. El receptor recibe un byte por llamada y
// pasa por tres estados: acumulando (Incompleto), linea lista (LineaCompleta) y
// rechazado (MensajeInvalido) hasta que se llame a reiniciar().
ResultadoRecepcion ReceptorLineas::agregar(char byte) {
    // Tras un error la conexion ya no es confiable: se ignora todo lo que llegue.
    if (rechazada_) {
        return ResultadoRecepcion::MensajeInvalido;
    }
    // La linea anterior se entrego en la llamada previa; este byte inicia otra.
    // Por eso linea() debe leerse antes de agregar el siguiente byte.
    if (completa_) {
        longitud_ = 0;
        completa_ = false;
        buffer_[0] = '\0';
    }
    // LF cierra el mensaje. Un LF sin contenido no es un mensaje y se rechaza abajo.
    if (byte == '\n' && longitud_ != 0) {
        buffer_[longitud_] = '\0';
        completa_ = true;
        return ResultadoRecepcion::LineaCompleta;
    }

    // Solo ASCII imprimible (32..126): rechaza CR, NUL, tabuladores y bytes no ASCII.
    // El contenido admite 127 bytes; el byte 128 sin LF supera el limite del protocolo.
    const unsigned char dato = static_cast<unsigned char>(byte);
    if (dato < 32 || dato > 126 ||
        longitud_ == ProtocoloAplicacion::LongitudMaximaContenido) {
        rechazada_ = true;
        buffer_[0] = '\0';
        return ResultadoRecepcion::MensajeInvalido;
    }
    // Se mantiene el terminador NUL para que el buffer sea siempre un texto valido.
    buffer_[longitud_++] = byte;
    buffer_[longitud_] = '\0';
    return ResultadoRecepcion::Incompleto;
}

const char* ReceptorLineas::linea() const {
    // Un fragmento a medias nunca se expone como mensaje.
    return completa_ && !rechazada_ ? buffer_ : nullptr;
}

void ReceptorLineas::reiniciar() {
    // Al reconectar se descartan fragmentos y errores de la conexion anterior.
    longitud_ = 0;
    completa_ = false;
    rechazada_ = false;
    buffer_[0] = '\0';
}
