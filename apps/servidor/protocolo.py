"""Protocolo ASCII/LF del sistema; el firmware usa el mismo contrato (apps/firmware/lib/Compartido)."""

from dataclasses import dataclass
from enum import StrEnum
import math
import re

from .dominio import ComandoLeds, EstadoLed

# Bytes de contenido por linea, sin contar el LF: 128 bytes en total por mensaje.
MAX_CONTENIDO = 127


class MensajeInvalido(ValueError):
    """Linea que no cumple el protocolo; el servidor responde invalid_message y cierra."""


class TipoCliente(StrEnum):
    """Rol que declara cada cliente en REGISTER."""

    SENSOR = "sensor"
    ACTUADOR = "actuator"


class MotivoError(StrEnum):
    """Valores posibles de reason en ERROR."""

    MENSAJE_INVALIDO = "invalid_message"
    NO_REGISTRADO = "not_registered"
    ROL_OCUPADO = "role_busy"


@dataclass(frozen=True)
class Mensaje:
    """Linea ya interpretada: el comando y su valor (rol, distancia, comando o motivo)."""

    tipo: str
    valor: TipoCliente | float | ComandoLeds | MotivoError | None = None


def leer_mensaje(linea: str) -> Mensaje:
    """Valida una linea sin LF y la convierte en Mensaje.

    Lanza MensajeInvalido si la linea esta vacia, supera 127 bytes, tiene caracteres
    fuera del ASCII imprimible o no coincide con ningun comando. La distancia
    "invalid" se devuelve como NaN; cero, negativos, coma decimal o notacion
    cientifica se rechazan.
    """
    if not linea or len(linea) > MAX_CONTENIDO or any(not 32 <= ord(c) <= 126 for c in linea):
        raise MensajeInvalido("Contenido no valido")
    if linea == "OK":
        return Mensaje("OK")
    if linea.startswith("REGISTER type="):
        try:
            return Mensaje("REGISTER", TipoCliente(linea[14:]))
        except ValueError as error:
            raise MensajeInvalido("Rol no valido") from error
    if linea.startswith("POST distance="):
        texto = linea[14:]
        if texto == "invalid":
            return Mensaje("POST", math.nan)
        if re.fullmatch(r"[0-9]+(?:\.[0-9]+)?", texto) is None:
            raise MensajeInvalido("Decimal no valido")
        valor = float(texto)
        if not math.isfinite(valor) or valor <= 0:
            raise MensajeInvalido("La distancia debe ser positiva")
        return Mensaje("POST", valor)
    patron = r"SET redLed=(off|on|blink_2|blink_4) yellowLed=(off|on|blink_2|blink_4) greenLed=(off|on|blink_2|blink_4)"
    estados = re.fullmatch(patron, linea)
    if estados is not None:
        return Mensaje("SET", ComandoLeds(*(EstadoLed(e) for e in estados.groups())))
    if linea.startswith("ERROR reason="):
        try:
            return Mensaje("ERROR", MotivoError(linea[13:]))
        except ValueError as error:
            raise MensajeInvalido("Motivo no valido") from error
    raise MensajeInvalido("Comando o argumentos no validos")


def crear_comando(comando: ComandoLeds) -> str:
    """Arma la linea SET con los tres estados, sin LF."""
    if any(not isinstance(e, EstadoLed) for e in (comando.rojo, comando.amarillo, comando.verde)):
        raise MensajeInvalido("Estado LED no valido")
    return f"SET redLed={comando.rojo} yellowLed={comando.amarillo} greenLed={comando.verde}"


def crear_error(motivo: MotivoError) -> str:
    """Arma la linea ERROR reason=..., sin LF."""
    return f"ERROR reason={MotivoError(motivo)}"


def codificar(linea: str) -> bytes:
    """Valida la linea y la devuelve como bytes ASCII terminados en LF."""
    leer_mensaje(linea)
    return linea.encode("ascii") + b"\n"


class ReceptorLineas:
    """Reconstruye lineas completas a partir del flujo TCP de una conexion."""

    def __init__(self):
        self._contenido = bytearray()
        self._rechazado = False

    def agregar(self, bloque: bytes):
        """Agrega un bloque recibido y entrega, una por una, las lineas completas.

        Los bytes que siguen al ultimo LF quedan guardados para el proximo bloque.
        Un byte no imprimible o mas de 127 bytes sin LF lanzan MensajeInvalido, y
        desde entonces el receptor rechaza todo lo que llegue.
        """
        # El generador entrega cada linea antes de validar la siguiente del bloque.
        for byte in bloque:
            if self._rechazado:
                raise MensajeInvalido("Conexion ya rechazada")
            if byte == 10 and self._contenido:
                linea = self._contenido.decode("ascii")
                self._contenido.clear()
                yield linea
            elif not 32 <= byte <= 126 or len(self._contenido) == MAX_CONTENIDO:
                self._rechazado = True
                raise MensajeInvalido("Framing no valido")
            else:
                self._contenido.append(byte)
