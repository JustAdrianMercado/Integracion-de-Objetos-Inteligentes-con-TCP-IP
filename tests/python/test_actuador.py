"""Flujo sensor Python -> servidor Python -> firmware C++ del actuador (GPIO simulados)."""

import argparse
import os
import unittest

from apps.servidor.dominio import ControlDistancia
from apps.servidor.protocolo import codificar
from test_sensor import EnlaceFirmware, ServidorDePrueba
from tools.simular_sensor import SensorSimulado, lectura_valida

PUENTE_ACTUADOR = os.environ.get("IOT_ACTUATOR_EXE")
APAGADOS = "000"
ROJO = "100"
AMARILLO = "010"
VERDE = "001"


def recibir_lineas(conexion, cantidad):
    """Lee del socket real exactamente `cantidad` lineas completas enviadas por el servidor."""
    contenido = bytearray()
    while contenido.count(b"\n") < cantidad:
        bloque = conexion.recv(1)
        if not bloque:
            raise ConnectionError("Conexion cerrada antes de recibir las lineas esperadas")
        contenido.extend(bloque)
    return bytes(contenido)


class PruebasSimuladorSensor(ServidorDePrueba):
    def test_normaliza_distancias_como_el_esp32(self):
        self.assertEqual(lectura_valida("45"), "45.00")
        self.assertEqual(lectura_valida("99.999"), "100.00")
        self.assertEqual(lectura_valida("invalid"), "invalid")
        for texto in ("-1", "0", "abc", "nan"):
            with self.subTest(texto=texto), self.assertRaises(argparse.ArgumentTypeError):
                lectura_valida(texto)

    def test_publica_lecturas_hacia_el_actuador(self):
        actuador = self.actuador()
        with SensorSimulado(*self.servidor.server_address) as sensor:
            for lectura, valor in [("45.00", 45), ("75.00", 75), ("120.00", 120)]:
                sensor.publicar(lectura)
                self.assertEqual(actuador.recibir_comando(2), ControlDistancia.calcular_comando(valor))
            sensor.esperar(0.05)


@unittest.skipUnless(PUENTE_ACTUADOR, "Construir con CMake y ejecutar CTest para enlazar el firmware C++")
class PruebasFlujoActuador(ServidorDePrueba):
    def enlace_registrado(self):
        enlace = EnlaceFirmware(self.servidor.server_address, PUENTE_ACTUADOR)
        self.addCleanup(enlace.cerrar)
        paso = enlace.paso(0)
        self.assertTrue(paso.conectado)
        self.assertEqual(paso.enviados, b"REGISTER type=actuator\n")
        respuesta = recibir_lineas(enlace.conexion, 2)
        self.assertEqual(respuesta, b"OK\n" + codificar("SET redLed=off yellowLed=off greenLed=off"))
        self.assertEqual(enlace.paso(1, entrada=respuesta).leds, APAGADOS)
        return enlace

    def publicar(self, sensor, enlace, tiempo, texto):
        sensor.sendall(codificar(f"POST distance={texto}"))
        return enlace.paso(tiempo, entrada=recibir_lineas(enlace.conexion, 1))

    def test_rangos_parpadeo_invalid_y_desconexion_del_sensor(self):
        enlace = self.enlace_registrado()
        sensor = self.sensor_python()
        self.assertEqual(self.publicar(sensor, enlace, 10, "45.00").leds, ROJO)
        self.assertEqual(enlace.paso(135).leds, APAGADOS, "blink_4 alterna a los 125 ms")
        self.assertEqual(enlace.paso(260).leds, ROJO)
        self.assertEqual(self.publicar(sensor, enlace, 300, "75.00").leds, AMARILLO)
        self.assertEqual(enlace.paso(549).leds, AMARILLO, "blink_2 conserva 250 ms encendido")
        self.assertEqual(enlace.paso(550).leds, APAGADOS)
        self.assertEqual(self.publicar(sensor, enlace, 600, "120.00").leds, VERDE)
        self.assertEqual(self.publicar(sensor, enlace, 850, "invalid").leds, APAGADOS)
        self.assertEqual(self.publicar(sensor, enlace, 1100, "120.00").leds, VERDE)
        sensor.close()
        paso = enlace.paso(1200, entrada=recibir_lineas(enlace.conexion, 1))
        self.assertEqual(paso.leds, APAGADOS)
        self.assertTrue(paso.conectado)
        self.assertEqual(enlace.transmitido, b"REGISTER type=actuator\n", "El actuador nunca publica lecturas")

    def test_sin_set_durante_2000_ms_apaga_y_conserva_la_conexion(self):
        enlace = self.enlace_registrado()
        sensor = self.sensor_python()
        self.assertEqual(self.publicar(sensor, enlace, 100, "120.00").leds, VERDE)
        self.assertEqual(enlace.paso(2099).leds, VERDE)
        paso = enlace.paso(2100)
        self.assertEqual(paso.leds, APAGADOS)
        self.assertTrue(paso.conectado)
        self.assertEqual(self.publicar(sensor, enlace, 2200, "120.00").leds, VERDE,
                         "Un SET posterior reactiva las salidas")
