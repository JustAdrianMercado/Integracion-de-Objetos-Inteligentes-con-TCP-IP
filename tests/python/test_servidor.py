import socket
import threading
import time
import unittest

from apps.servidor.servidor import ServidorTcp

APAGADO = "SET redLed=off yellowLed=off greenLed=off"
ROJO = "SET redLed=blink_4 yellowLed=off greenLed=off"
AMARILLO = "SET redLed=off yellowLed=blink_2 greenLed=off"
VERDE = "SET redLed=off yellowLed=off greenLed=on"


class PruebasServidor(unittest.TestCase):
    def setUp(self):
        self.servidor = ServidorTcp(("127.0.0.1", 0))
        self.hilo = threading.Thread(target=self.servidor.serve_forever, kwargs={"poll_interval": 0.05})
        self.hilo.start()
        self.clientes = []

    def tearDown(self):
        for cliente in self.clientes:
            cliente.close()
        self.servidor.shutdown()
        self.hilo.join(timeout=3)
        self.servidor.server_close()
        self.assertFalse(self.hilo.is_alive())

    def conectar(self):
        cliente = socket.create_connection(self.servidor.server_address, timeout=3.5)
        self.clientes.append(cliente)
        return cliente

    def linea(self, cliente):
        contenido = bytearray()
        while True:
            byte = cliente.recv(1)
            self.assertTrue(byte, "Conexion cerrada antes de LF")
            if byte == b"\n":
                return contenido.decode("ascii")
            contenido.extend(byte)

    def cerrado(self, cliente):
        try:
            self.assertEqual(cliente.recv(1), b"")
        except ConnectionResetError:
            pass

    def sin_datos(self, cliente):
        cliente.settimeout(0.2)
        try:
            with self.assertRaises(socket.timeout):
                cliente.recv(1)
        finally:
            cliente.settimeout(3.5)

    def registrar(self, rol, inicial=APAGADO):
        cliente = self.conectar()
        cliente.sendall(f"REGISTER type={rol}\n".encode())
        self.assertEqual(self.linea(cliente), "OK")
        if rol == "actuator":
            self.assertEqual(self.linea(cliente), inicial)
        return cliente

    def error(self, cliente, motivo):
        self.assertEqual(self.linea(cliente), f"ERROR reason={motivo}")
        self.cerrado(cliente)

    def post(self, sensor, actuador, distancia, esperado):
        sensor.sendall(f"POST distance={distancia}\n".encode())
        self.assertEqual(self.linea(actuador), esperado)

    def test_rangos_limites_y_comandos_identicos(self):
        actuador = self.registrar("actuator")
        sensor = self.registrar("sensor")
        for distancia, esperado in [(45, ROJO), (49.99, ROJO), (50, AMARILLO), (99.99, AMARILLO),
                                    (100, VERDE), (120, VERDE), (120, VERDE), ("invalid", APAGADO)]:
            self.post(sensor, actuador, distancia, esperado)

    def test_actuador_recibe_la_ultima_lectura_reciente(self):
        sensor = self.registrar("sensor")
        sensor.sendall(b"POST distance=45.00\nPOST distance=120.00\n")
        time.sleep(0.1)
        actuador = self.registrar("actuator", VERDE)
        self.sin_datos(actuador)

    def test_reconexion_del_actuador_conserva_la_lectura(self):
        sensor = self.registrar("sensor")
        actuador = self.registrar("actuator")
        self.post(sensor, actuador, 75, AMARILLO)
        actuador.close()
        time.sleep(0.1)
        self.registrar("actuator", AMARILLO)

    def test_desconexion_del_sensor_apaga(self):
        actuador = self.registrar("actuator")
        sensor = self.registrar("sensor")
        self.post(sensor, actuador, 45, ROJO)
        sensor.close()
        self.assertEqual(self.linea(actuador), APAGADO)
        self.registrar("sensor")  # El rol quedo libre.

    def test_sensor_sin_post_durante_2_s_se_cierra_y_apaga(self):
        actuador = self.registrar("actuator")
        sensor = self.registrar("sensor")
        self.post(sensor, actuador, 120, VERDE)
        inicio = time.monotonic()
        self.assertEqual(self.linea(actuador), APAGADO)
        self.assertGreaterEqual(time.monotonic() - inicio, 1.8)
        self.cerrado(sensor)

    def test_registro_vencido(self):
        cliente = self.conectar()
        inicio = time.monotonic()
        self.error(cliente, "not_registered")
        self.assertGreaterEqual(time.monotonic() - inicio, 1.8)

    def test_rol_ocupado_no_afecta_al_original(self):
        actuador = self.registrar("actuator")
        sensor = self.registrar("sensor")
        for rol in ("sensor", "actuator"):
            duplicado = self.conectar()
            duplicado.sendall(f"REGISTER type={rol}\n".encode())
            self.error(duplicado, "role_busy")
            self.post(sensor, actuador, 100, VERDE)

    def test_mensaje_antes_del_registro(self):
        for mensaje, motivo in [("POST distance=45.00", "not_registered"), (APAGADO, "not_registered"),
                                ("OK", "invalid_message"), ("HOLA", "invalid_message")]:
            with self.subTest(mensaje=mensaje):
                cliente = self.conectar()
                cliente.sendall(f"{mensaje}\n".encode())
                self.error(cliente, motivo)

    def test_mensajes_que_no_corresponden_al_rol(self):
        actuador = self.registrar("actuator")
        actuador.sendall(b"POST distance=45.00\n")
        self.error(actuador, "invalid_message")
        actuador = self.registrar("actuator")
        for mensaje in ("REGISTER type=sensor", APAGADO, "POST distance=4,5"):
            with self.subTest(mensaje=mensaje):
                sensor = self.registrar("sensor")
                sensor.sendall(f"{mensaje}\n".encode())
                self.error(sensor, "invalid_message")
                self.assertEqual(self.linea(actuador), APAGADO, "Un sensor rechazado apaga")

    def test_fragmentos_y_varios_mensajes_por_bloque(self):
        actuador = self.registrar("actuator")
        sensor = self.registrar("sensor")
        for fragmento in (b"POST dis", b"tance=4", b"5.00\nPOST distance=120.00\n"):
            sensor.sendall(fragmento)
            time.sleep(0.05)
        self.assertEqual(self.linea(actuador), ROJO)
        self.assertEqual(self.linea(actuador), VERDE)

    def test_linea_larga_cr_y_nul(self):
        for bloque in (b"x" * 128 + b"\n", b"OK\r\n", b"OK\0\n"):
            with self.subTest(bloque=bloque):
                cliente = self.conectar()
                cliente.sendall(bloque)
                self.error(cliente, "invalid_message")

    def test_error_se_entrega_aunque_el_cliente_siga_enviando(self):
        sensor = self.registrar("sensor")
        sensor.sendall(APAGADO.encode() + b"\n" + b"POST distance=45.00\n" * 50)
        sensor.shutdown(socket.SHUT_WR)
        self.error(sensor, "invalid_message")

    def test_cierre_del_servidor_cierra_clientes(self):
        actuador = self.registrar("actuator")
        self.servidor.shutdown()
        self.hilo.join(timeout=3)
        self.servidor.server_close()
        self.cerrado(actuador)
        self.assertEqual(self.servidor.clientes_abiertos(), 0)

    def test_puerto_ocupado(self):
        with self.assertRaises(OSError):
            ServidorTcp(self.servidor.server_address)
