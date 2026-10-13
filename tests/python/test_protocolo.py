import itertools
import json
import math
import os
from pathlib import Path
import subprocess
import unittest

from apps.servidor.dominio import ComandoLeds, ControlDistancia, EstadoLed
from apps.servidor.protocolo import (
    MensajeInvalido, ReceptorLineas, codificar, crear_comando, leer_mensaje,
)
from apps.servidor.servidor import ColaEnvio

RAIZ = Path(__file__).resolve().parents[2]


class PruebasProtocolo(unittest.TestCase):
    def test_limites_y_lecturas_invalidas(self):
        for distancia, led in [(45, "rojo"), (49.99, "rojo"), (50, "amarillo"),
                               (99.99, "amarillo"), (100, "verde"), (120, "verde")]:
            with self.subTest(distancia=distancia):
                comando = ControlDistancia.calcular_comando(distancia)
                self.assertNotEqual(getattr(comando, led), EstadoLed.APAGADO)
                self.assertEqual(sum(e != EstadoLed.APAGADO for e in
                                     (comando.rojo, comando.amarillo, comando.verde)), 1)
        for distancia in [0, -0.0, -1, math.nan, math.inf, -math.inf]:
            self.assertEqual(ControlDistancia.calcular_comando(distancia), ComandoLeds())
        self.assertTrue(math.isnan(leer_mensaje("POST distance=invalid").valor))

    def test_todas_las_combinaciones_de_leds(self):
        for estados in itertools.product(EstadoLed, repeat=3):
            with self.subTest(estados=estados):
                comando = ComandoLeds(*estados)
                self.assertEqual(leer_mensaje(crear_comando(comando)).valor, comando)
        with self.assertRaises(MensajeInvalido):
            crear_comando(ComandoLeds(rojo="otro"))

    def test_fragmentos_y_lineas_consecutivas(self):
        receptor = ReceptorLineas()
        self.assertEqual(list(receptor.agregar(b"REGI")), [])
        self.assertEqual(list(receptor.agregar(b"STER type=sensor\nPOST distance=4")),
                         ["REGISTER type=sensor"])
        self.assertEqual(list(receptor.agregar(b"5.00\nOK\nPO")), ["POST distance=45.00", "OK"])
        self.assertEqual(list(receptor.agregar(b"ST distance=invalid\n")), ["POST distance=invalid"])

    def test_limite_y_rechazo_permanente(self):
        contenido = b"POST distance=" + b"0" * 112 + b"1"
        self.assertEqual(len(contenido), 127)
        self.assertEqual(list(ReceptorLineas().agregar(contenido + b"\n")), [contenido.decode()])
        for bloque in [b"x" * 128, b"\n", b"OK\r\n", b"OK\0extra\n", b"OK\t\n", b"\xff"]:
            with self.subTest(bloque=bloque):
                receptor = ReceptorLineas()
                with self.assertRaises(MensajeInvalido):
                    list(receptor.agregar(bloque))
                with self.assertRaises(MensajeInvalido):
                    list(receptor.agregar(b"OK\n"))

    def test_linea_valida_previa_a_error_no_se_pierde(self):
        receptor = ReceptorLineas()
        lineas = receptor.agregar(b"OK\n\0")
        self.assertEqual(next(lineas), "OK")
        with self.assertRaises(MensajeInvalido):
            next(lineas)

    def test_contrato_documentado(self):
        casos = json.loads((RAIZ / "tests/contratos/protocolo.json").read_text(encoding="utf-8"))
        for caso in casos:
            with self.subTest(contenido=caso["contenido"]):
                self.assertEqual(canonizar(caso["contenido"]), caso["esperado"])

    def test_contrato_entre_python_y_cpp(self):
        candidatos = [os.environ.get("IOT_CONTRACT_EXE"),
                      RAIZ / "build/windows/tests/native/verificar_contrato.exe",
                      RAIZ / "build/native/tests/native/verificar_contrato",
                      RAIZ / "build/native/verificar_contrato"]
        ejecutable = next((Path(p) for p in candidatos if p and Path(p).is_file()), None)
        if ejecutable is None:
            self.skipTest("Compilar verificar_contrato con CMake para comprobar ambos lenguajes")
        casos = json.loads((RAIZ / "tests/contratos/protocolo.json").read_text(encoding="utf-8"))
        entrada = "\n".join(c["contenido"].encode("utf-8").hex() for c in casos) + "\n"
        resultado = subprocess.run([str(ejecutable)], input=entrada.encode("ascii"),
                                   capture_output=True, timeout=5, check=True)
        cpp = resultado.stdout.decode("ascii").splitlines()
        self.assertEqual(len(cpp), len(casos))
        for caso, nativo in zip(casos, cpp):
            with self.subTest(contenido=caso["contenido"]):
                self.assertEqual(nativo, caso["esperado"])
                self.assertEqual(canonizar(caso["contenido"]), nativo)


def canonizar(contenido):
    try:
        mensaje = leer_mensaje(contenido)
    except MensajeInvalido:
        return "INVALID"
    if mensaje.tipo == "POST":
        return crear_comando(ControlDistancia.calcular_comando(mensaje.valor))
    if mensaje.tipo == "SET":
        return crear_comando(mensaje.valor)
    return contenido


class DestinoSimulado:
    def __init__(self, escrituras):
        self.escrituras = iter(escrituras)
        self.recibido = bytearray()

    def send(self, datos):
        cantidad = next(self.escrituras, len(datos))
        if cantidad is None:
            raise BlockingIOError()
        cantidad = min(cantidad, len(datos))
        self.recibido.extend(datos[:cantidad])
        return cantidad


class PruebasColaEnvio(unittest.TestCase):
    def test_escrituras_parciales_y_socket_ocupado(self):
        cola = ColaEnvio()
        mensajes = ["OK", crear_comando(ComandoLeds()), "ERROR reason=role_busy"]
        for mensaje in mensajes:
            self.assertTrue(cola.encolar(mensaje))
        destino = DestinoSimulado([1, None, 1, None, 2, 7, None, 3, 4, None, 9])
        while not cola.vacia():
            cola.enviar(destino)
        self.assertEqual(destino.recibido, b"".join(codificar(m) for m in mensajes))

    def test_saturacion_no_descarta_ni_duplica_mensajes(self):
        cola = ColaEnvio()
        mensaje = "POST distance=" + "0" * 112 + "1"
        for _ in range(32):
            self.assertTrue(cola.encolar(mensaje))
        self.assertFalse(cola.encolar("OK"))
        destino = DestinoSimulado([3, None])
        cola.enviar(destino)
        self.assertTrue(cola.encolar("OK"))
        cola.enviar(destino)
        self.assertTrue(cola.vacia())
        self.assertEqual(destino.recibido, codificar(mensaje) * 32 + b"OK\n")

    def test_envio_cero_es_desconexion(self):
        cola = ColaEnvio()
        cola.encolar("OK")
        with self.assertRaises(ConnectionError):
            cola.enviar(DestinoSimulado([0]))
        self.assertFalse(cola.vacia())
