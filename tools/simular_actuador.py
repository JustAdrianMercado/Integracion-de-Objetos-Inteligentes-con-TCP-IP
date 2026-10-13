"""Actuador de consola: py -3.13 -m tools.simular_actuador --ip 127.0.0.1."""

import argparse
from collections import deque
import socket
import sys
import time

from apps.servidor.dominio import ComandoLeds
from apps.servidor.protocolo import MensajeInvalido, ReceptorLineas, codificar, leer_mensaje


class ActuadorSimulado:
    def __init__(self, ip: str, puerto: int, timeout: float = 2.0):
        self._receptor = ReceptorLineas()
        self._lineas = deque()
        self._socket = socket.create_connection((ip, puerto), timeout=timeout)
        try:
            self._socket.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
            self._socket.sendall(codificar("REGISTER type=actuator"))
            respuesta = leer_mensaje(self._recibir_linea(timeout))
            if respuesta.tipo == "ERROR":
                raise RuntimeError(f"Registro rechazado: {respuesta.valor}")
            if respuesta.tipo != "OK":
                raise MensajeInvalido("Se esperaba OK para completar el registro")
        except Exception:
            self.cerrar()
            raise

    def _recibir_linea(self, timeout: float | None) -> str:
        limite = None if timeout is None else time.monotonic() + timeout
        while not self._lineas:
            restante = None if limite is None else limite - time.monotonic()
            if restante is not None and restante <= 0:
                raise TimeoutError("Plazo de recepcion vencido")
            self._socket.settimeout(restante)
            bloque = self._socket.recv(128)
            if not bloque:
                raise ConnectionError("El servidor cerro la conexion")
            self._lineas.extend(self._receptor.agregar(bloque))
        return self._lineas.popleft()

    def recibir_comando(self, timeout: float | None = None) -> ComandoLeds:
        respuesta = leer_mensaje(self._recibir_linea(timeout))
        if respuesta.tipo == "ERROR":
            raise RuntimeError(f"Servidor rechazo la conexion: {respuesta.valor}")
        if respuesta.tipo != "SET":
            raise MensajeInvalido("Se esperaba un SET completo")
        return respuesta.valor

    def cerrar(self):
        self._socket.close()

    def __enter__(self):
        return self

    def __exit__(self, *_):
        self.cerrar()


def puerto_valido(texto: str) -> int:
    valor = int(texto)
    if not 1 <= valor <= 65535:
        raise argparse.ArgumentTypeError("El puerto debe estar entre 1 y 65535")
    return valor


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description="Muestra los SET del servidor sin una placa actuadora.")
    parser.add_argument("--ip", default="127.0.0.1", help="IPv4 o nombre de la PC servidor")
    parser.add_argument("--puerto", type=puerto_valido, default=5000)
    args = parser.parse_args(argv)
    try:
        with ActuadorSimulado(args.ip, args.puerto) as actuador:
            print("Actuador registrado. Ctrl+C para terminar.", flush=True)
            while True:
                try:
                    # Un recv sin plazo no atiende Ctrl+C en Windows hasta recibir datos.
                    comando = actuador.recibir_comando(0.5)
                except TimeoutError:
                    continue
                print(f"{time.strftime('%H:%M:%S')} | rojo={comando.rojo} "
                      f"amarillo={comando.amarillo} verde={comando.verde}", flush=True)
    except KeyboardInterrupt:
        return 0
    except (OSError, MensajeInvalido, RuntimeError) as error:
        print(f"Actuador simulado: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
