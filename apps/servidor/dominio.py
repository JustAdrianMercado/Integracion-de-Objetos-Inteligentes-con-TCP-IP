"""Estados y reglas de distancia ejecutados por la PC."""

from dataclasses import dataclass
from enum import StrEnum
import math


class EstadoLed(StrEnum):
    APAGADO = "off"
    ENCENDIDO = "on"
    PARPADEO_2_HZ = "blink_2"
    PARPADEO_4_HZ = "blink_4"


@dataclass(frozen=True)
class ComandoLeds:
    rojo: EstadoLed = EstadoLed.APAGADO
    amarillo: EstadoLed = EstadoLed.APAGADO
    verde: EstadoLed = EstadoLed.APAGADO


class ControlDistancia:
    @staticmethod
    def calcular_comando(distancia_cm: float) -> ComandoLeds:
        if not math.isfinite(distancia_cm) or distancia_cm <= 0:
            return ComandoLeds()
        if distancia_cm < 50:
            return ComandoLeds(rojo=EstadoLed.PARPADEO_4_HZ)
        if distancia_cm < 100:
            return ComandoLeds(amarillo=EstadoLed.PARPADEO_2_HZ)
        return ComandoLeds(verde=EstadoLed.ENCENDIDO)
