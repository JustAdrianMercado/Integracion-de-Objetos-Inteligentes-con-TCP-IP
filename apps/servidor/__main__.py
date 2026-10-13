"""Ejecutar desde la raiz: py -3.13 -m apps.servidor."""

import argparse
import ipaddress
import logging

from .servidor import INTERVALO_ATENCION, ServidorTcp


def puerto_valido(texto):
    if not texto.isascii() or not texto.isdecimal() or not 0 <= int(texto) <= 65535:
        raise argparse.ArgumentTypeError("El puerto debe estar entre 0 y 65535")
    return int(texto)


def ipv4_valida(texto):
    try:
        return str(ipaddress.IPv4Address(texto))
    except ipaddress.AddressValueError as error:
        raise argparse.ArgumentTypeError("Se requiere una IPv4 local") from error


def main():
    argumentos = argparse.ArgumentParser(description="Servidor TCP de sensor y actuador ESP32")
    argumentos.add_argument("--ip", type=ipv4_valida, default="0.0.0.0")
    argumentos.add_argument("--puerto", type=puerto_valido, default=5000)
    opciones = argumentos.parse_args()
    logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s %(message)s")
    try:
        with ServidorTcp((opciones.ip, opciones.puerto)) as servidor:
            logging.info("Servidor TCP escuchando en %s:%s", *servidor.server_address)
            try:
                servidor.serve_forever(poll_interval=INTERVALO_ATENCION)
            except KeyboardInterrupt:
                logging.info("Deteniendo servidor")
    except OSError as error:
        logging.error("No se pudo iniciar el servidor: %s", error)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
