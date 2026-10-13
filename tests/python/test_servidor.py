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
        self.hilo = threading.Thread(target=self.servidor.serve_forever, kwargs={"poll_interval": 0.02})
        self.hilo.start()
        self.clientes = []

    def tearDown(self):
        for cliente in self.clientes:
            cliente.close()
        self.servidor.shutdown()
        self.hilo.join(timeout=3)
        self.servidor.server_close()
        self.assertFalse(self.hilo.is_alive())
        self.assertEqual(self.servidor.sesiones, {})

    def conectar(self):
        cliente = socket.create_connection(self.servidor.server_address, timeout=3.5)
        cliente.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        self.clientes.append(cliente)
        return cliente

    def linea(self, cliente):
        contenido = bytearray()
        while len(contenido) <= 127:
            byte = cliente.recv(1)
            self.assertTrue(byte, "Conexion cerrada antes de LF")
            if byte == b"\n":
                return contenido.decode("ascii")
            contenido.extend(byte)
        self.fail("Linea del servidor demasiado larga")

    def cerrado(self, cliente):
        try:
            self.assertEqual(cliente.recv(1), b"")
        except ConnectionResetError:
            pass

    def sin_datos(self, cliente):
        cliente.settimeout(0.1)
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

    def plazo(self, inicio):
        transcurrido = time.monotonic() - inicio
        self.assertGreaterEqual(transcurrido, 1.8)
        self.assertLessEqual(transcurrido, 3.2)

    def test_actuador_primero_rangos_y_comandos_identicos(self):
        actuador = self.registrar("actuator")
        sensor = self.registrar("sensor")
        for distancia, esperado in [(45, ROJO), (49.99, ROJO), (50, AMARILLO),
                                    (99.99, AMARILLO), (100, VERDE), (120, VERDE),
                                    (120, VERDE), ("invalid", APAGADO)]:
            self.post(sensor, actuador, distancia, esperado)

    def test_sensor_primero_ultima_lectura_reciente(self):
        sensor = self.registrar("sensor")
        sensor.sendall(b"POST distance=45.00\nPOST distance=120.00\n")
        time.sleep(0.1)
        actuador = self.registrar("actuator", VERDE)
        self.sin_datos(actuador)

    def test_lectura_invalid_reciente(self):
        sensor = self.registrar("sensor")
        sensor.sendall(b"POST distance=invalid\n")
        time.sleep(0.1)
        self.registrar("actuator")

    def test_lectura_caducada(self):
        sensor = self.registrar("sensor")
        sensor.sendall(b"POST distance=120.00\n")
        self.cerrado(sensor)
        self.registrar("actuator")

    def test_roles_duplicados_preservan_clientes_originales(self):
        actuador = self.registrar("actuator")
        sensor = self.registrar("sensor")
        self.post(sensor, actuador, 45, ROJO)
        for rol in ("sensor", "actuator"):
            duplicado = self.conectar()
            duplicado.sendall(f"REGISTER type={rol}\n".encode())
            self.error(duplicado, "role_busy")
            self.sin_datos(actuador)
            self.post(sensor, actuador, 100, VERDE)

    def test_error_se_entrega_aunque_el_cliente_siga_enviando(self):
        sensor = self.registrar("sensor")
        # Datos posteriores al mensaje rechazado, incluido un fin de envio del cliente.
        sensor.sendall(APAGADO.encode() + b"\n" + b"POST distance=45.00\n" * 50)
        sensor.shutdown(socket.SHUT_WR)
        self.error(sensor, "invalid_message")

    def test_mensaje_operativo_antes_del_registro(self):
        for mensaje in ("POST distance=45.00", APAGADO):
            cliente = self.conectar()
            cliente.sendall((mensaje + "\n").encode())
            self.error(cliente, "not_registered")

    def test_mensajes_malformados(self):
        for mensaje in ("UNKNOWN", "REGISTER type=other", "POST distance=0", "OK"):
            cliente = self.conectar()
            cliente.sendall((mensaje + "\n").encode())
            self.error(cliente, "invalid_message")

    def test_segundo_registro_libera_sensor_y_apaga(self):
        actuador = self.registrar("actuator")
        sensor = self.registrar("sensor")
        self.post(sensor, actuador, 45, ROJO)
        sensor.sendall(b"REGISTER type=sensor\n")
        self.error(sensor, "invalid_message")
        self.assertEqual(self.linea(actuador), APAGADO)
        nuevo = self.registrar("sensor")
        self.post(nuevo, actuador, 100, VERDE)

    def test_actuador_no_publica_lecturas(self):
        actuador = self.registrar("actuator")
        actuador.sendall(b"POST distance=45.00\n")
        self.error(actuador, "invalid_message")
        self.registrar("actuator")

    def test_post_malformado_apaga(self):
        actuador = self.registrar("actuator")
        sensor = self.registrar("sensor")
        self.post(sensor, actuador, 120, VERDE)
        sensor.sendall(b"POST distance=50,00\n")
        self.error(sensor, "invalid_message")
        self.assertEqual(self.linea(actuador), APAGADO)

    def test_fragmentos_consecutivos_y_otro_cliente_incompleto(self):
        pendiente = self.conectar()
        pendiente.sendall(b"REGISTER type=")
        actuador = self.registrar("actuator")
        sensor = self.conectar()
        sensor.sendall(b"REGI")
        self.sin_datos(sensor)
        sensor.sendall(b"STER type=sensor\nPOST distance=4")
        self.assertEqual(self.linea(sensor), "OK")
        self.sin_datos(actuador)
        sensor.sendall(b"5.00\nPOST distance=50.00\nPOST distance=100")
        self.assertEqual(self.linea(actuador), ROJO)
        self.assertEqual(self.linea(actuador), AMARILLO)
        self.sin_datos(actuador)
        sensor.sendall(b".00\n")
        self.assertEqual(self.linea(actuador), VERDE)

    def test_longitud_cr_y_nul(self):
        actuador = self.registrar("actuator")
        sensor = self.registrar("sensor")
        sensor.sendall(b"POST distance=" + b"0" * 112 + b"1\n")
        self.assertEqual(self.linea(actuador), ROJO)
        sensor.close()
        self.assertEqual(self.linea(actuador), APAGADO)
        for bloque in (b"x" * 128, b"REGISTER type=sensor\r\n", b"REGISTER\0type=sensor\n"):
            cliente = self.conectar()
            cliente.sendall(bloque)
            self.error(cliente, "invalid_message")
            self.sin_datos(actuador)

    def test_desconexion_sensor_apaga_sin_esperar_timeout(self):
        actuador = self.registrar("actuator")
        sensor = self.registrar("sensor")
        self.post(sensor, actuador, 120, VERDE)
        inicio = time.monotonic()
        sensor.close()
        self.assertEqual(self.linea(actuador), APAGADO)
        self.assertLess(time.monotonic() - inicio, 1)
        nuevo = self.registrar("sensor")
        self.post(nuevo, actuador, 45, ROJO)

    def test_reconexion_actuador_conserva_lectura(self):
        sensor = self.registrar("sensor")
        actuador = self.registrar("actuator")
        self.post(sensor, actuador, 120, VERDE)
        actuador.close()
        time.sleep(0.05)
        self.registrar("actuator", VERDE)

    def test_plazo_registro_sin_datos(self):
        cliente = self.conectar()
        inicio = time.monotonic()
        self.error(cliente, "not_registered")
        self.plazo(inicio)

    def test_fragmento_registro_no_renueva_plazo(self):
        cliente = self.conectar()
        inicio = time.monotonic()
        cliente.sendall(b"REGI")
        time.sleep(1.2)
        cliente.sendall(b"STER type=sensor")
        self.error(cliente, "not_registered")
        self.plazo(inicio)

    def test_sensor_sin_primer_post(self):
        actuador = self.registrar("actuator")
        sensor = self.registrar("sensor")
        inicio = time.monotonic()
        self.assertEqual(self.linea(actuador), APAGADO)
        self.cerrado(sensor)
        self.plazo(inicio)

    def test_post_incompleto_no_renueva_plazo(self):
        actuador = self.registrar("actuator")
        sensor = self.registrar("sensor")
        self.post(sensor, actuador, 45, ROJO)
        inicio = time.monotonic()
        sensor.sendall(b"POST distance=120")
        self.assertEqual(self.linea(actuador), APAGADO)
        self.cerrado(sensor)
        self.plazo(inicio)
        nuevo = self.registrar("sensor")
        self.post(nuevo, actuador, 100, VERDE)

    def test_invalid_renueva_plazo_mas_de_dos_segundos(self):
        actuador = self.registrar("actuator")
        sensor = self.registrar("sensor")
        for _ in range(12):
            self.post(sensor, actuador, "invalid", APAGADO)
            time.sleep(0.2)
        self.post(sensor, actuador, 45, ROJO)

    def test_limite_conexiones_y_recuperacion(self):
        actuador = self.registrar("actuator")
        sensor = self.registrar("sensor")
        pendientes = [self.conectar() for _ in range(6)]
        self.cerrado(self.conectar())
        pendientes.pop().close()
        time.sleep(0.05)
        reemplazo = self.conectar()
        reemplazo.sendall(b"REGISTER type=sensor\n")
        self.error(reemplazo, "role_busy")
        self.post(sensor, actuador, 45, ROJO)

    def test_registros_simultaneos_un_solo_ganador(self):
        clientes = [self.conectar(), self.conectar()]
        barrera = threading.Barrier(3)
        def registrar(cliente):
            barrera.wait(timeout=3)
            cliente.sendall(b"REGISTER type=sensor\n")
        hilos = [threading.Thread(target=registrar, args=(c,)) for c in clientes]
        for hilo in hilos:
            hilo.start()
        barrera.wait(timeout=3)
        for hilo in hilos:
            hilo.join(timeout=3)
        respuestas = [self.linea(c) for c in clientes]
        self.assertCountEqual(respuestas, ["OK", "ERROR reason=role_busy"])
        perdedor = respuestas.index("ERROR reason=role_busy")
        self.cerrado(clientes[perdedor])

    def test_cierre_servidor_cierra_clientes_e_hilos(self):
        sensor = self.registrar("sensor")
        actuador = self.registrar("actuator")
        self.servidor.shutdown()
        self.hilo.join(timeout=3)
        self.servidor.server_close()
        self.cerrado(sensor)
        self.cerrado(actuador)

    def test_puerto_ocupado_preserva_servidor(self):
        with self.assertRaises(OSError):
            ServidorTcp(self.servidor.server_address)
        self.registrar("sensor")
