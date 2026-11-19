"""Estados y reglas de distancia ejecutados por la PC."""

from dataclasses import dataclass
from enum import StrEnum
import math


class EstadoLed(StrEnum):
    """Modo de un LED, con el mismo texto que viaja en el protocolo."""

    APAGADO = "off"
    ENCENDIDO = "on"
    PARPADEO_2_HZ = "blink_2"
    PARPADEO_4_HZ = "blink_4"


@dataclass(frozen=True)
class ComandoLeds:
    """Estados de los tres LEDs. Por defecto todos apagados; es inmutable."""

    rojo: EstadoLed = EstadoLed.APAGADO
    amarillo: EstadoLed = EstadoLed.APAGADO
    verde: EstadoLed = EstadoLed.APAGADO


class ControlDistancia:
    """Algoritmo de control: la unica parte del sistema que clasifica distancias."""

    @staticmethod
    def calcular_comando(distancia_cm: float) -> ComandoLeds:
        """Convierte una distancia en cm en el comando para los tres LEDs.

        Rangos: (0, 5] rojo a 4 Hz; (5, 15] amarillo a 2 Hz; (15, 25] verde a 2 Hz;
        mas de 25 cm, los tres a 2 Hz (Lejos). Cada limite pertenece al rango que
        termina en el. NaN (lectura invalida), cero o negativo: todos apagados.
        """
        if not math.isfinite(distancia_cm) or distancia_cm <= 0:
            return ComandoLeds()
        if distancia_cm <= 5:
            return ComandoLeds(rojo=EstadoLed.PARPADEO_4_HZ)
        if distancia_cm <= 15:
            return ComandoLeds(amarillo=EstadoLed.PARPADEO_2_HZ)
        if distancia_cm <= 25:
            return ComandoLeds(verde=EstadoLed.PARPADEO_2_HZ)
        return ComandoLeds(EstadoLed.PARPADEO_2_HZ, EstadoLed.PARPADEO_2_HZ,
                           EstadoLed.PARPADEO_2_HZ)
