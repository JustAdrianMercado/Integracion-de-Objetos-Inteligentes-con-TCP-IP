"""Ejecutar desde la raiz: py -3.13 -m apps.servidor."""

import argparse
import ipaddress
import logging
from pathlib import Path
import socket

from .servidor import ServidorTcp


def puerto_valido(texto):
    if not texto.isascii() or not texto.isdecimal() or not 0 <= int(texto) <= 65535:
        raise argparse.ArgumentTypeError("El puerto debe estar entre 0 y 65535")
    return int(texto)


def ipv4_valida(texto):
    try:
        return str(ipaddress.IPv4Address(texto))
    except ipaddress.AddressValueError as error:
        raise argparse.ArgumentTypeError("Se requiere una IPv4 local") from error


def ipv4_locales():
    """Direcciones que los ESP32 pueden usar como IP_SERVIDOR; 0.0.0.0 solo indica escucha."""
    try:
        datos = socket.getaddrinfo(socket.gethostname(), None, socket.AF_INET)
    except OSError:
        return []
    direcciones = {dato[4][0] for dato in datos}
    return sorted(d for d in direcciones if not ipaddress.IPv4Address(d).is_loopback)


def main():
    argumentos = argparse.ArgumentParser(description="Servidor TCP de sensor y actuador ESP32")
    argumentos.add_argument("--ip", type=ipv4_valida, default="0.0.0.0")
    argumentos.add_argument("--puerto", type=puerto_valido, default=5000)
    argumentos.add_argument("--registro", type=Path,
                            help="Archivo donde guardar tambien el registro (evidencia de las pruebas)")
    opciones = argumentos.parse_args()
    destinos = [logging.StreamHandler()]
    if opciones.registro is not None:
        opciones.registro.parent.mkdir(parents=True, exist_ok=True)
        destinos.append(logging.FileHandler(opciones.registro, encoding="utf-8"))
    logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s %(message)s",
                        handlers=destinos)
    try:
        with ServidorTcp((opciones.ip, opciones.puerto)) as servidor:
            logging.info("Servidor TCP escuchando en %s:%s", *servidor.server_address)
            if opciones.ip == "0.0.0.0":
                locales = ipv4_locales()
                logging.info("IPv4 de esta PC para IP_SERVIDOR: %s",
                             ", ".join(locales) if locales else "consultar con ipconfig")
            try:
                servidor.serve_forever()
            except KeyboardInterrupt:
                logging.info("Deteniendo servidor")
    except OSError as error:
        logging.error("No se pudo iniciar el servidor: %s", error)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
