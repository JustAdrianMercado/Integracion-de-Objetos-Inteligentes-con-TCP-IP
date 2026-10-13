"""Sensor de consola: py -3.13 -m tools.simular_sensor --ip 127.0.0.1 --distancias 45,75,120,invalid."""

import argparse
import itertools
import select
import socket
import sys
import time

from apps.servidor.protocolo import MensajeInvalido, ReceptorLineas, codificar, leer_mensaje

INTERVALO_PUBLICACION = 0.25


def lectura_valida(texto: str) -> str:
    """Normaliza una distancia de la consola al contenido de POST que emite el ESP32."""
    texto = texto.strip()
    if texto != "invalid":
        try:
            texto = f"{float(texto):.2f}"
        except ValueError as error:
            raise argparse.ArgumentTypeError(f"Distancia no valida: {texto}") from error
    try:
        leer_mensaje(f"POST distance={texto}")
    except MensajeInvalido as error:
        raise argparse.ArgumentTypeError(f"Distancia no valida: {texto}") from error
    return texto


class SensorSimulado:
    def __init__(self, ip: str, puerto: int, timeout: float = 2.0):
        self._receptor = ReceptorLineas()
        self._socket = socket.create_connection((ip, puerto), timeout=timeout)
        try:
            self._socket.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
            self._socket.sendall(codificar("REGISTER type=sensor"))
            respuesta = self._recibir_linea()
            if respuesta.tipo == "ERROR":
                raise RuntimeError(f"Registro rechazado: {respuesta.valor}")
            if respuesta.tipo != "OK":
                raise MensajeInvalido("Se esperaba OK para completar el registro")
        except Exception:
            self.cerrar()
            raise

    def _recibir_linea(self):
        lineas = []
        while not lineas:
            bloque = self._socket.recv(128)
            if not bloque:
                raise ConnectionError("El servidor cerro la conexion")
            lineas.extend(self._receptor.agregar(bloque))
        return leer_mensaje(lineas[0])

    def publicar(self, lectura: str):
        self._socket.sendall(codificar(f"POST distance={lectura}"))

    def esperar(self, segundos: float):
        """Espera sin bloquear Ctrl+C; el servidor solo escribe al sensor para rechazarlo."""
        listos, _, _ = select.select([self._socket], [], [], segundos)
        if listos:
            respuesta = self._recibir_linea()
            raise RuntimeError(f"Servidor rechazo la conexion: {respuesta.valor}")

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
    parser = argparse.ArgumentParser(description="Publica distancias como el ESP32 sensor, sin placa.")
    parser.add_argument("--ip", default="127.0.0.1", help="IPv4 o nombre de la PC servidor")
    parser.add_argument("--puerto", type=puerto_valido, default=5000)
    parser.add_argument("--distancias", default="45,75,120,invalid",
                        help="Lista separada por comas, en cm o invalid; se recorre en ciclo")
    parser.add_argument("--duracion", type=float, default=3.0,
                        help="Segundos que se repite cada distancia, un POST cada 250 ms")
    args = parser.parse_args(argv)
    try:
        lecturas = [lectura_valida(texto) for texto in args.distancias.split(",")]
    except argparse.ArgumentTypeError as error:
        parser.error(str(error))
    repeticiones = max(1, round(args.duracion / INTERVALO_PUBLICACION))
    try:
        with SensorSimulado(args.ip, args.puerto) as sensor:
            print("Sensor registrado. Ctrl+C para terminar.", flush=True)
            for lectura in itertools.cycle(lecturas):
                print(f"{time.strftime('%H:%M:%S')} | POST distance={lectura}", flush=True)
                for _ in range(repeticiones):
                    sensor.publicar(lectura)
                    sensor.esperar(INTERVALO_PUBLICACION)
    except KeyboardInterrupt:
        return 0
    except (OSError, MensajeInvalido, RuntimeError) as error:
        print(f"Sensor simulado: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
