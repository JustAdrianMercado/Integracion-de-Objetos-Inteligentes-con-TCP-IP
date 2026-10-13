"""Flujo del firmware C++ (hardware simulado) por TCP real al servidor Python."""

from contextlib import contextmanager
from dataclasses import dataclass
import os
import queue
import socket
import subprocess
import threading
import time
import unittest

from apps.servidor.dominio import ComandoLeds, ControlDistancia
from apps.servidor.protocolo import MensajeInvalido, codificar, crear_comando
from apps.servidor.servidor import ServidorTcp
from tools.simular_actuador import ActuadorSimulado

PUENTE = os.environ.get("IOT_SENSOR_EXE")
APAGADO = ComandoLeds()


def recibir_exactamente(conexion, cantidad):
    contenido = bytearray()
    while len(contenido) < cantidad:
        bloque = conexion.recv(cantidad - len(contenido))
        if not bloque:
            raise ConnectionError("Conexion cerrada antes de completar los bytes esperados")
        contenido.extend(bloque)
    return bytes(contenido)


class ServidorDePrueba(unittest.TestCase):
    def setUp(self):
        self.servidor = self.iniciar_servidor()

    def iniciar_servidor(self):
        servidor = ServidorTcp(("127.0.0.1", 0))
        hilo = threading.Thread(target=servidor.serve_forever, kwargs={"poll_interval": 0.02})
        hilo.start()
        self.addCleanup(self.detener_servidor, servidor, hilo)
        servidor.hilo_prueba = hilo
        return servidor

    def detener_servidor(self, servidor, hilo):
        if hilo.is_alive():
            servidor.shutdown()
            hilo.join(timeout=3)
            servidor.server_close()
        self.assertFalse(hilo.is_alive())
        self.assertEqual(servidor.sesiones, {})

    def actuador(self):
        cliente = ActuadorSimulado(*self.servidor.server_address)
        self.addCleanup(cliente.cerrar)
        self.assertEqual(cliente.recibir_comando(2), APAGADO)
        return cliente

    def sensor_python(self):
        cliente = socket.create_connection(self.servidor.server_address, timeout=2)
        self.addCleanup(cliente.close)
        cliente.sendall(codificar("REGISTER type=sensor"))
        self.assertEqual(recibir_exactamente(cliente, 3), b"OK\n")
        return cliente


class PruebasActuadorSimulado(ServidorDePrueba):
    def test_rangos_invalid_y_apagado_al_desconectar_sensor(self):
        actuador = self.actuador()
        sensor = self.sensor_python()
        for texto, valor in [("45.00", 45), ("75.00", 75), ("120.00", 120), ("invalid", float("nan"))]:
            sensor.sendall(codificar(f"POST distance={texto}"))
            self.assertEqual(actuador.recibir_comando(2), ControlDistancia.calcular_comando(valor))
        sensor.sendall(codificar("POST distance=120.00"))
        self.assertEqual(actuador.recibir_comando(2), ControlDistancia.calcular_comando(120))
        sensor.close()
        self.assertEqual(actuador.recibir_comando(2), APAGADO)

    def test_rol_ocupado_no_interrumpe_el_actuador_original(self):
        original = self.actuador()
        with self.assertRaisesRegex(RuntimeError, "role_busy"):
            ActuadorSimulado(*self.servidor.server_address)
        sensor = self.sensor_python()
        sensor.sendall(codificar("POST distance=45.00"))
        self.assertEqual(original.recibir_comando(2), ControlDistancia.calcular_comando(45))

    def test_fragmentos_y_varios_set_en_un_bloque(self):
        rojo = crear_comando(ControlDistancia.calcular_comando(45))
        bloques = [b"O", b"K\nSET redLed=off yellowLed=off greenLed=", b"off\n" + codificar(rojo)]
        with servidor_programado(bloques) as direccion:
            with ActuadorSimulado(*direccion) as cliente:
                self.assertEqual(cliente.recibir_comando(2), APAGADO)
                self.assertEqual(cliente.recibir_comando(2), ControlDistancia.calcular_comando(45))
                with self.assertRaises(ConnectionError):
                    cliente.recibir_comando(2)

    def test_rechazar_respuesta_incompatible_o_set_invalido(self):
        for contenido in [b"OK\n", b"SET redLed=on yellowLed=off greenLed=invalido\n", b"SET redLed=off\r\n"]:
            with self.subTest(contenido=contenido), servidor_programado([b"OK\n", contenido]) as direccion:
                with ActuadorSimulado(*direccion) as cliente:
                    with self.assertRaises(MensajeInvalido):
                        cliente.recibir_comando(2)


@contextmanager
def servidor_programado(bloques):
    errores = []
    with socket.socket() as escucha:
        escucha.bind(("127.0.0.1", 0))
        escucha.listen(1)
        escucha.settimeout(3)

        def atender():
            try:
                with escucha.accept()[0] as conexion:
                    conexion.settimeout(3)
                    registro = recibir_exactamente(conexion, len(b"REGISTER type=actuator\n"))
                    if registro != b"REGISTER type=actuator\n":
                        raise AssertionError(registro)
                    for bloque in bloques:
                        conexion.sendall(bloque)
                        time.sleep(0.01)
            except Exception as error:
                errores.append(error)

        hilo = threading.Thread(target=atender)
        hilo.start()
        try:
            yield escucha.getsockname()
        finally:
            hilo.join(timeout=4)
            if hilo.is_alive():
                raise AssertionError("El servidor de respuestas no termino")
            if errores:
                raise errores[0]


@dataclass
class PasoFirmware:
    intentos: int
    dato: str
    conectado: bool
    enviados: bytes

    @property
    def mediciones(self) -> int:
        return int(self.dato)

    @property
    def leds(self) -> str:
        """Niveles rojo, amarillo y verde informados por el puente del actuador."""
        return self.dato


class EnlaceFirmware:
    """Adapta los bytes del firmware compilado a un socket real, sin recodificarlos."""

    def __init__(self, direccion, ejecutable=PUENTE):
        self.direccion = direccion
        self.conexion = None
        self.intentos = 0
        self.transmitido = bytearray()
        self.proceso = subprocess.Popen([ejecutable], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                        stderr=subprocess.PIPE, text=True, bufsize=1,
                                        creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
        self.respuestas = queue.Queue()

        def leer_salida():
            for linea in self.proceso.stdout:
                self.respuestas.put(linea)
            self.respuestas.put(None)

        self.hilo = threading.Thread(target=leer_salida, daemon=True)
        self.hilo.start()

    def paso(self, tiempo, eco=0, *, wifi=True, cerrado=False, limite=128, entrada=b""):
        self.proceso.stdin.write(f"{tiempo} {eco} {int(wifi)} {int(cerrado)} {limite} {entrada.hex() or '-'}\n")
        self.proceso.stdin.flush()
        try:
            linea = self.respuestas.get(timeout=3)
        except queue.Empty as error:
            raise AssertionError("El firmware simulado no atendio el ciclo principal") from error
        if linea is None:
            raise AssertionError("El puente del firmware termino antes de devolver los bytes")
        intentos, dato, conectado, hexadecimales = linea.split()
        resultado = PasoFirmware(int(intentos), dato, conectado == "1",
                                 b"" if hexadecimales == "-" else bytes.fromhex(hexadecimales))
        if not resultado.conectado or resultado.intentos != self.intentos:
            if self.conexion is not None:
                self.conexion.close()
                self.conexion = None
        if resultado.conectado and self.conexion is None:
            self.conexion = socket.create_connection(self.direccion, timeout=2)
            self.conexion.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        self.intentos = resultado.intentos
        if resultado.enviados:
            self.conexion.sendall(resultado.enviados)
            self.transmitido.extend(resultado.enviados)
        return resultado

    def confirmar_registro(self):
        return recibir_exactamente(self.conexion, 3)

    def cerrar(self):
        if self.conexion is not None:
            self.conexion.close()
        self.proceso.stdin.close()
        try:
            self.proceso.wait(timeout=3)
        except subprocess.TimeoutExpired:
            self.proceso.kill()
            self.proceso.wait(timeout=3)
        self.hilo.join(timeout=3)
        self.proceso.stdout.close()
        self.proceso.stderr.close()


@unittest.skipUnless(PUENTE, "Construir con CMake y ejecutar CTest para enlazar el firmware C++")
class PruebasFlujoFirmware(ServidorDePrueba):
    def enlace(self):
        enlace = EnlaceFirmware(self.servidor.server_address)
        self.addCleanup(enlace.cerrar)
        return enlace

    def test_firmware_real_fragmentos_cadencia_y_tres_rangos(self):
        actuador = self.actuador()
        enlace = self.enlace()
        for tiempo in range(8):
            paso = enlace.paso(tiempo, limite=3)
            self.assertEqual(paso.mediciones, 0)
        self.assertEqual(enlace.transmitido, b"REGISTER type=sensor\n")
        respuesta = enlace.confirmar_registro()
        self.assertEqual(respuesta, b"OK\n")
        self.assertEqual(enlace.paso(20, entrada=respuesta[:1], limite=3).mediciones, 0)
        self.assertEqual(enlace.paso(21, entrada=respuesta[1:2], limite=3).mediciones, 0)
        self.assertEqual(enlace.paso(22, eco=2624, entrada=respuesta[2:], limite=3).mediciones, 1)
        for tiempo in range(23, 30):
            enlace.paso(tiempo, eco=2624, limite=3)
        self.assertEqual(actuador.recibir_comando(2), ControlDistancia.calcular_comando(45))
        self.assertEqual(enlace.paso(271, eco=4373).mediciones, 1)
        for numero, tiempo, eco, texto, valor in [(2, 272, 4373, "75.00", 75),
                                                 (3, 522, 6997, "120.00", 120),
                                                 (4, 772, 0, "invalid", float("nan"))]:
            paso = enlace.paso(tiempo, eco=eco)
            self.assertEqual(paso.mediciones, numero)
            self.assertEqual(paso.enviados, codificar(f"POST distance={texto}"))
            self.assertEqual(actuador.recibir_comando(2), ControlDistancia.calcular_comando(valor))
        self.assertNotIn(0, enlace.transmitido)
        self.assertEqual(bytes(enlace.transmitido).splitlines(),
                         [b"REGISTER type=sensor", b"POST distance=45.00", b"POST distance=75.00",
                          b"POST distance=120.00", b"POST distance=invalid"])

    def test_reinicio_servidor_y_perdida_wifi_exigen_registro_y_eco_nuevos(self):
        actuador = self.actuador()
        enlace = self.enlace()
        enlace.paso(0)
        enlace.paso(1, eco=2624, entrada=enlace.confirmar_registro())
        self.assertEqual(actuador.recibir_comando(2), ControlDistancia.calcular_comando(45))
        self.detener_servidor(self.servidor, self.servidor.hilo_prueba)
        self.assertEqual(enlace.conexion.recv(1), b"")
        self.assertFalse(enlace.paso(10, cerrado=True).conectado)
        self.servidor = self.iniciar_servidor()
        enlace.direccion = self.servidor.server_address
        actuador = self.actuador()
        self.assertEqual(enlace.paso(2009, eco=6997).intentos, 1)
        self.assertEqual(enlace.paso(2010, eco=6997).enviados, b"REGISTER type=sensor\n")
        paso = enlace.paso(2011, eco=6997, entrada=enlace.confirmar_registro())
        self.assertEqual(paso.mediciones, 2)
        self.assertEqual(paso.enviados, b"POST distance=120.00\n")
        self.assertEqual(actuador.recibir_comando(2), ControlDistancia.calcular_comando(120))
        self.assertFalse(enlace.paso(2020, wifi=False).conectado)
        self.assertEqual(actuador.recibir_comando(2), APAGADO)
        self.assertEqual(enlace.paso(4019, wifi=False).mediciones, 2)
        enlace.paso(4020, wifi=False)
        self.assertEqual(enlace.paso(4021).enviados, b"REGISTER type=sensor\n")
        paso = enlace.paso(4022, eco=4373, entrada=enlace.confirmar_registro())
        self.assertEqual(paso.mediciones, 3)
        self.assertEqual(paso.enviados, b"POST distance=75.00\n")
        self.assertEqual(actuador.recibir_comando(2), ControlDistancia.calcular_comando(75))
