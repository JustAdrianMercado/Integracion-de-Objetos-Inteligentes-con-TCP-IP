"""Servidor TCP: un hilo por conexion, un sensor y un actuador, reglas de distancia en la PC."""

import logging
import socket
import socketserver
import threading
import time

from .dominio import ComandoLeds, ControlDistancia
from .protocolo import (
    MensajeInvalido, MotivoError, ReceptorLineas, TipoCliente,
    codificar, crear_comando, crear_error, leer_mensaje,
)

LOG = logging.getLogger(__name__)
# Plazo de registro, de ausencia de POST, de antiguedad de la ultima lectura y de envio.
PLAZO = 2.0


class Cliente:
    """Una conexion aceptada. El lock evita mezclar lineas enviadas desde dos hilos."""

    def __init__(self, conexion: socket.socket):
        self.conexion = conexion
        self.rol = None
        self._lock = threading.Lock()

    def enviar(self, linea: str):
        with self._lock:
            self.conexion.sendall(codificar(linea))

    def cortar(self):
        try:
            self.conexion.shutdown(socket.SHUT_RDWR)
        except OSError:
            pass


class EstadoSistema:
    """Roles registrados y ultima lectura, compartidos por todos los hilos."""

    def __init__(self):
        self.lock = threading.Lock()
        self.roles = {}
        self.ultimo_comando = ComandoLeds()
        self.ultima_lectura = None

    def registrar(self, cliente: Cliente, rol: TipoCliente) -> bool:
        with self.lock:
            if rol in self.roles:
                return False
            self.roles[rol] = cliente
            cliente.rol = rol
            cliente.enviar("OK")
            if rol == TipoCliente.ACTUADOR:
                # El actuador recibe el estado actual si la lectura es reciente; si no, apagado.
                reciente = (self.ultima_lectura is not None
                            and time.monotonic() - self.ultima_lectura < PLAZO)
                self._actuar(self.ultimo_comando if reciente else ComandoLeds())
            return True

    def nueva_lectura(self, distancia: float):
        with self.lock:
            self.ultima_lectura = time.monotonic()
            self.ultimo_comando = ControlDistancia.calcular_comando(distancia)
            self._actuar(self.ultimo_comando)

    def liberar(self, cliente: Cliente):
        with self.lock:
            if cliente.rol is None or self.roles.get(cliente.rol) is not cliente:
                return
            del self.roles[cliente.rol]
            if cliente.rol == TipoCliente.SENSOR:
                # Sin sensor no hay datos validos: se apagan los LEDs.
                self.ultima_lectura = None
                self.ultimo_comando = ComandoLeds()
                self._actuar(self.ultimo_comando)
            cliente.rol = None

    def _actuar(self, comando: ComandoLeds):
        """Envia un SET al actuador registrado. Se llama con el lock tomado."""
        actuador = self.roles.get(TipoCliente.ACTUADOR)
        if actuador is None:
            return
        linea = crear_comando(comando)
        try:
            actuador.enviar(linea)
            LOG.info("%s", linea)
        except OSError:
            # Actuador desconectado o sin leer durante PLAZO: se libera su rol.
            LOG.warning("No se pudo enviar al actuador; cerrando su conexion")
            del self.roles[TipoCliente.ACTUADOR]
            actuador.rol = None
            actuador.cortar()


class ManejadorConexion(socketserver.BaseRequestHandler):
    def handle(self):
        self.cliente = Cliente(self.request)
        self.request.settimeout(PLAZO)
        self.server.agregar(self.cliente)
        receptor = ReceptorLineas()
        try:
            while not self.server.cerrando:
                try:
                    bloque = self.request.recv(128)
                except TimeoutError:
                    if self.cliente.rol == TipoCliente.ACTUADOR:
                        continue  # El actuador solo recibe; su silencio es normal.
                    if self.cliente.rol is None:
                        self.cerrar_con_error(MotivoError.NO_REGISTRADO)
                    else:
                        LOG.info("2000 ms sin POST; cerrando sensor")
                    return
                if not bloque:
                    return
                for linea in receptor.agregar(bloque):
                    motivo = self.procesar(linea)
                    if motivo is not None:
                        self.cerrar_con_error(motivo)
                        return
        except MensajeInvalido:
            self.cerrar_con_error(MotivoError.MENSAJE_INVALIDO)
        except OSError:
            LOG.debug("Conexion cerrada", exc_info=True)
        finally:
            self.server.estado.liberar(self.cliente)
            self.server.quitar(self.cliente)

    def procesar(self, linea: str):
        """Atiende una linea completa. Devuelve el motivo de error o None si es aceptada."""
        mensaje = leer_mensaje(linea)
        estado = self.server.estado
        if self.cliente.rol is None:
            if mensaje.tipo == "REGISTER":
                if not estado.registrar(self.cliente, mensaje.valor):
                    return MotivoError.ROL_OCUPADO
                LOG.info("REGISTER %s aceptado", mensaje.valor)
                return None
            if mensaje.tipo in ("POST", "SET"):
                return MotivoError.NO_REGISTRADO
            return MotivoError.MENSAJE_INVALIDO
        if self.cliente.rol == TipoCliente.SENSOR and mensaje.tipo == "POST":
            LOG.info("%s", linea)
            estado.nueva_lectura(mensaje.valor)
            return None
        # Segundo REGISTER, SET desde un cliente u otro mensaje que no corresponde al rol.
        return MotivoError.MENSAJE_INVALIDO

    def cerrar_con_error(self, motivo: MotivoError):
        # Liberar primero: un sensor rechazado apaga los LEDs aunque el envio falle.
        self.server.estado.liberar(self.cliente)
        try:
            self.cliente.enviar(crear_error(motivo))
            LOG.info("ERROR reason=%s", motivo)
            self.request.shutdown(socket.SHUT_WR)
            # Leer lo que el cliente siga enviando: cerrar con datos sin leer envia un RST
            # y el cliente podria perder el ERROR.
            limite = time.monotonic() + PLAZO
            while time.monotonic() < limite and self.request.recv(512):
                pass
        except OSError:
            pass


class ServidorTcp(socketserver.ThreadingTCPServer):
    allow_reuse_address = False

    def __init__(self, direccion=("0.0.0.0", 5000)):
        self.estado = EstadoSistema()
        self.cerrando = False
        self._clientes = set()
        self._lock_clientes = threading.Lock()
        super().__init__(direccion, ManejadorConexion)

    def server_bind(self):
        # En Windows evita que otro proceso escuche en el mismo puerto a la vez.
        if hasattr(socket, "SO_EXCLUSIVEADDRUSE"):
            self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_EXCLUSIVEADDRUSE, 1)
        super().server_bind()

    def agregar(self, cliente: Cliente):
        with self._lock_clientes:
            self._clientes.add(cliente)

    def quitar(self, cliente: Cliente):
        with self._lock_clientes:
            self._clientes.discard(cliente)

    def clientes_abiertos(self) -> int:
        with self._lock_clientes:
            return len(self._clientes)

    def server_close(self):
        # Cortar los sockets despierta a los hilos bloqueados en recv; luego se esperan.
        self.cerrando = True
        with self._lock_clientes:
            clientes = list(self._clientes)
        for cliente in clientes:
            cliente.cortar()
        super().server_close()
