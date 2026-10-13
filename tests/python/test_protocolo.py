import itertools
import math
import unittest

from apps.servidor.dominio import ComandoLeds, ControlDistancia, EstadoLed
from apps.servidor.protocolo import (
    MensajeInvalido, ReceptorLineas, codificar, crear_comando, crear_error, leer_mensaje,
)


class PruebasDominio(unittest.TestCase):
    def test_limites_de_los_rangos(self):
        for distancia, led in [(45, "rojo"), (49.99, "rojo"), (50, "amarillo"),
                               (99.99, "amarillo"), (100, "verde"), (120, "verde")]:
            with self.subTest(distancia=distancia):
                comando = ControlDistancia.calcular_comando(distancia)
                self.assertNotEqual(getattr(comando, led), EstadoLed.APAGADO)
                self.assertEqual(sum(e != EstadoLed.APAGADO for e in
                                     (comando.rojo, comando.amarillo, comando.verde)), 1)

    def test_lecturas_invalidas_apagan(self):
        for distancia in [0, -1, math.nan, math.inf]:
            self.assertEqual(ControlDistancia.calcular_comando(distancia), ComandoLeds())


class PruebasMensajes(unittest.TestCase):
    def test_mensajes_validos(self):
        self.assertEqual(leer_mensaje("REGISTER type=sensor").valor, "sensor")
        self.assertEqual(leer_mensaje("REGISTER type=actuator").valor, "actuator")
        self.assertEqual(leer_mensaje("POST distance=45.00").valor, 45.0)
        self.assertTrue(math.isnan(leer_mensaje("POST distance=invalid").valor))
        self.assertEqual(leer_mensaje("OK").tipo, "OK")
        self.assertEqual(leer_mensaje(crear_error("role_busy")).tipo, "ERROR")

    def test_todas_las_combinaciones_de_leds(self):
        for estados in itertools.product(EstadoLed, repeat=3):
            with self.subTest(estados=estados):
                comando = ComandoLeds(*estados)
                self.assertEqual(leer_mensaje(crear_comando(comando)).valor, comando)

    def test_mensajes_invalidos(self):
        for linea in ["", "ok", "REGISTER type=server", "POST distance=0", "POST distance=-1",
                      "POST distance=4,5", "POST distance=1e3", "POST distance=nan",
                      "POST  distance=45.00", "SET redLed=on yellowLed=off",
                      "SET redLed=on yellowLed=off greenLed=blink_3", "ERROR reason=otro",
                      "x" * 128, "OK\r"]:
            with self.subTest(linea=linea), self.assertRaises(MensajeInvalido):
                leer_mensaje(linea)

    def test_codificar_agrega_lf(self):
        self.assertEqual(codificar("OK"), b"OK\n")
        with self.assertRaises(MensajeInvalido):
            codificar("OK\n")


class PruebasReceptor(unittest.TestCase):
    def test_fragmentos_y_lineas_consecutivas(self):
        receptor = ReceptorLineas()
        self.assertEqual(list(receptor.agregar(b"REGI")), [])
        self.assertEqual(list(receptor.agregar(b"STER type=sensor\nPOST distance=4")),
                         ["REGISTER type=sensor"])
        self.assertEqual(list(receptor.agregar(b"5.00\nOK\n")), ["POST distance=45.00", "OK"])

    def test_limite_y_bytes_no_permitidos(self):
        contenido = b"POST distance=" + b"0" * 112 + b"1"
        self.assertEqual(list(ReceptorLineas().agregar(contenido + b"\n")), [contenido.decode()])
        for bloque in [b"x" * 128, b"\n", b"OK\r\n", b"OK\0\n", b"\xff"]:
            with self.subTest(bloque=bloque), self.assertRaises(MensajeInvalido):
                list(ReceptorLineas().agregar(bloque))
