"""Los simuladores de consola contra el servidor real: sensor simulado -> servidor -> actuador simulado."""

import argparse
import threading
import unittest

from apps.servidor.dominio import ComandoLeds, ControlDistancia
from apps.servidor.servidor import ServidorTcp
from tools.analizar_registro import resumir
from tools.simular_actuador import ActuadorSimulado
from tools.simular_sensor import SensorSimulado, lectura_valida


class PruebasSimuladores(unittest.TestCase):
    def setUp(self):
        self.servidor = ServidorTcp(("127.0.0.1", 0))
        hilo = threading.Thread(target=self.servidor.serve_forever, kwargs={"poll_interval": 0.05})
        hilo.start()

        def detener():
            self.servidor.shutdown()
            hilo.join(timeout=3)
            self.servidor.server_close()
        self.addCleanup(detener)
        self.direccion = self.servidor.server_address

    def test_flujo_completo_y_apagado_al_desconectar_el_sensor(self):
        with ActuadorSimulado(*self.direccion) as actuador:
            self.assertEqual(actuador.recibir_comando(2), ComandoLeds())
            with SensorSimulado(*self.direccion) as sensor:
                for lectura, valor in [("45.00", 45), ("75.00", 75), ("120.00", 120),
                                       ("invalid", float("nan"))]:
                    sensor.publicar(lectura)
                    self.assertEqual(actuador.recibir_comando(2), ControlDistancia.calcular_comando(valor))
                sensor.publicar("120.00")
                self.assertEqual(actuador.recibir_comando(2), ControlDistancia.calcular_comando(120))
                sensor.esperar(0.05)
            self.assertEqual(actuador.recibir_comando(2), ComandoLeds())

    def test_rol_ocupado(self):
        with ActuadorSimulado(*self.direccion) as actuador:
            actuador.recibir_comando(2)
            with self.assertRaisesRegex(RuntimeError, "role_busy"):
                ActuadorSimulado(*self.direccion)
        with SensorSimulado(*self.direccion):
            with self.assertRaisesRegex(RuntimeError, "role_busy"):
                SensorSimulado(*self.direccion)

    def test_analizar_registro(self):
        registro = """2026-10-13 10:00:00,000 INFO REGISTER actuator aceptado
2026-10-13 10:00:01,000 INFO REGISTER sensor aceptado
2026-10-13 10:00:01,000 INFO POST distance=45.00
2026-10-13 10:00:01,001 INFO SET redLed=blink_4 yellowLed=off greenLed=off
2026-10-13 10:00:01,250 INFO POST distance=invalid
2026-10-13 10:00:01,500 INFO POST distance=120.00
2026-10-13 10:00:02,000 INFO POST distance=120.00
2026-10-13 10:00:04,000 INFO 2000 ms sin POST; cerrando sensor
linea ajena al formato""".splitlines()
        r = resumir(registro)
        self.assertEqual((r["posts"], r["invalidas"], r["set"]), (4, 1, 1))
        self.assertEqual((r["registros_sensor"], r["registros_actuador"], r["cierres_sin_post"]), (1, 1, 1))
        self.assertAlmostEqual(r["duracion_s"], 1.0)
        self.assertAlmostEqual(r["lecturas_por_s"], 3.0)
        self.assertAlmostEqual(r["mayor_intervalo_s"], 0.5)

    def test_sensor_simulado_normaliza_como_el_esp32(self):
        self.assertEqual(lectura_valida("45"), "45.00")
        self.assertEqual(lectura_valida("99.999"), "100.00")
        self.assertEqual(lectura_valida("invalid"), "invalid")
        for texto in ("-1", "0", "abc", "nan"):
            with self.subTest(texto=texto), self.assertRaises(argparse.ArgumentTypeError):
                lectura_valida(texto)
