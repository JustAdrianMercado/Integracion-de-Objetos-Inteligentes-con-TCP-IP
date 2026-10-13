"""Un hilo por conexion; roles y lectura protegidos, I/O fuera del bloqueo global."""

from collections import deque
import logging
import select
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
PLAZO_REGISTRO = 2.0
PLAZO_LECTURAS = 2.0
PLAZO_CIERRE = 2.0
MAX_CONEXIONES = 8
MAX_SALIDA = 4096
INTERVALO_ATENCION = 0.02


class ColaEnvio:
    def __init__(self):
        self._mensajes = deque()
        self._desplazamiento = 0
        self._pendientes = 0
        self._lock = threading.Lock()

    def encolar(self, linea: str) -> bool:
        mensaje = codificar(linea)
        with self._lock:
            if self._pendientes + len(mensaje) > MAX_SALIDA:
                return False
            self._mensajes.append(mensaje)
            self._pendientes += len(mensaje)
            return True

    def vacia(self) -> bool:
        with self._lock:
            return not self._mensajes

    def enviar(self, destino) -> None:
        with self._lock:
            presupuesto = MAX_SALIDA
            while self._mensajes and presupuesto:
                fragmento = self._mensajes[0][self._desplazamiento:]
                try:
                    cantidad = destino.send(fragmento[:presupuesto])
                except (BlockingIOError, InterruptedError):
                    return
                if cantidad == 0:
                    raise ConnectionError("Envio sin progreso")
                self._desplazamiento += cantidad
                self._pendientes -= cantidad
                presupuesto -= cantidad
                if self._desplazamiento == len(self._mensajes[0]):
                    self._mensajes.popleft()
                    self._desplazamiento = 0


class Sesion:
    def __init__(self, conexion: socket.socket):
        self.conexion = conexion
        self.aceptada = time.monotonic()
        self.ultima_actividad = self.aceptada
        self.rol = None
        self.cerrando = False
        self.inicio_cierre = 0.0
        self.detener = threading.Event()
        self.salida = ColaEnvio()
        self.receptor = ReceptorLineas()


class EstadoSistema:
    def __init__(self):
        self.lock = threading.RLock()
        self.roles = {}
        self.ultimo_comando = ComandoLeds()
        self.ultima_lectura = None

    def _encolar(self, sesion: Sesion, contenido: str):
        if not sesion.salida.encolar(contenido):
            LOG.warning("Salida saturada; cerrando conexion")
            sesion.detener.set()
            self.liberar(sesion)

    def _actuar(self, comando: ComandoLeds):
        actuador = self.roles.get(TipoCliente.ACTUADOR)
        if actuador is not None:
            linea = crear_comando(comando)
            self._encolar(actuador, linea)
            LOG.info("%s", linea)

    def procesar(self, sesion: Sesion, linea: str):
        mensaje = leer_mensaje(linea)
        with self.lock:
            ahora = time.monotonic()
            if sesion.rol is None:
                if mensaje.tipo != "REGISTER":
                    motivo = (MotivoError.NO_REGISTRADO if mensaje.tipo in ("POST", "SET")
                              else MotivoError.MENSAJE_INVALIDO)
                    self.rechazar(sesion, motivo)
                elif mensaje.valor in self.roles:
                    self.rechazar(sesion, MotivoError.ROL_OCUPADO)
                else:
                    sesion.rol = mensaje.valor
                    sesion.ultima_actividad = ahora
                    self.roles[mensaje.valor] = sesion
                    self._encolar(sesion, "OK")
                    LOG.info("REGISTER %s aceptado", mensaje.valor)
                    if mensaje.valor == TipoCliente.ACTUADOR:
                        reciente = self.ultima_lectura is not None and ahora - self.ultima_lectura < PLAZO_LECTURAS
                        self._actuar(self.ultimo_comando if reciente else ComandoLeds())
            elif sesion.rol != TipoCliente.SENSOR or mensaje.tipo != "POST":
                self.rechazar(sesion, MotivoError.MENSAJE_INVALIDO)
            else:
                sesion.ultima_actividad = ahora
                self.ultima_lectura = ahora
                self.ultimo_comando = ControlDistancia.calcular_comando(mensaje.valor)
                LOG.info("%s", linea)
                self._actuar(self.ultimo_comando)

    def rechazar(self, sesion: Sesion, motivo: MotivoError):
        with self.lock:
            self.liberar(sesion)
            sesion.cerrando = True
            sesion.inicio_cierre = time.monotonic()
            self._encolar(sesion, crear_error(motivo))
            LOG.info("ERROR reason=%s", motivo)

    def liberar(self, sesion: Sesion):
        with self.lock:
            if self.roles.get(TipoCliente.SENSOR) is sesion:
                del self.roles[TipoCliente.SENSOR]
                self.ultima_lectura = None
                self.ultimo_comando = ComandoLeds()
                self._actuar(self.ultimo_comando)
            if self.roles.get(TipoCliente.ACTUADOR) is sesion:
                del self.roles[TipoCliente.ACTUADOR]
            sesion.rol = None

    def revisar_plazos(self, sesion: Sesion):
        with self.lock:
            ahora = time.monotonic()
            if sesion.cerrando:
                if sesion.salida.vacia() or ahora - sesion.inicio_cierre >= PLAZO_CIERRE:
                    sesion.detener.set()
            elif sesion.rol is None and ahora - sesion.aceptada >= PLAZO_REGISTRO:
                self.rechazar(sesion, MotivoError.NO_REGISTRADO)
            elif sesion.rol == TipoCliente.SENSOR and ahora - sesion.ultima_actividad >= PLAZO_LECTURAS:
                LOG.info("2000 ms sin POST; cerrando sensor")
                self.liberar(sesion)
                sesion.detener.set()


class ManejadorConexion(socketserver.BaseRequestHandler):
    def handle(self):
        servidor = self.server
        with servidor.estado.lock:
            sesion = servidor.sesiones[self.request]
        entrada_abierta = True
        try:
            while not sesion.detener.is_set():
                servidor.estado.revisar_plazos(sesion)
                if sesion.detener.is_set():
                    break
                lectura = [self.request] if entrada_abierta else []
                escritura = [] if sesion.salida.vacia() else [self.request]
                if not lectura and not escritura:
                    break
                listos, escribibles, _ = select.select(lectura, escritura, [], INTERVALO_ATENCION)
                servidor.estado.revisar_plazos(sesion)
                if sesion.detener.is_set():
                    break
                if escribibles:
                    sesion.salida.enviar(self.request)
                if listos:
                    try:
                        bloque = self.request.recv(512)
                    except (BlockingIOError, InterruptedError):
                        continue
                    if not bloque:
                        if not sesion.cerrando:
                            break
                        # El cliente termino de enviar; todavia puede recibir el ERROR pendiente.
                        entrada_abierta = False
                        continue
                    if sesion.cerrando:
                        # Descartar la entrada pendiente: cerrar con datos sin leer envia RST
                        # y el cliente podria perder el ERROR todavia en transito.
                        continue
                    try:
                        for linea in sesion.receptor.agregar(bloque):
                            servidor.estado.procesar(sesion, linea)
                            if sesion.cerrando or sesion.detener.is_set():
                                break
                    except MensajeInvalido:
                        servidor.estado.rechazar(sesion, MotivoError.MENSAJE_INVALIDO)
        except (OSError, ValueError):
            LOG.debug("Socket cerrado", exc_info=True)
        finally:
            servidor.estado.liberar(sesion)
            with servidor.estado.lock:
                servidor.sesiones.pop(self.request, None)


class ServidorTcp(socketserver.ThreadingTCPServer):
    daemon_threads = False
    block_on_close = True
    request_queue_size = MAX_CONEXIONES
    allow_reuse_address = False

    def __init__(self, direccion=("0.0.0.0", 5000)):
        self.estado = EstadoSistema()
        self.sesiones = {}
        self._cerrado = False
        super().__init__(direccion, ManejadorConexion)

    def server_bind(self):
        if hasattr(socket, "SO_EXCLUSIVEADDRUSE"):
            self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_EXCLUSIVEADDRUSE, 1)
        super().server_bind()

    def process_request(self, request, client_address):
        request.setblocking(False)
        request.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        with self.estado.lock:
            if self._cerrado or len(self.sesiones) >= MAX_CONEXIONES:
                self.shutdown_request(request)
                return
            self.sesiones[request] = Sesion(request)
        try:
            super().process_request(request, client_address)
        except Exception:
            with self.estado.lock:
                self.sesiones.pop(request, None)
            raise

    def server_close(self):
        with self.estado.lock:
            self._cerrado = True
            sesiones = list(self.sesiones.values())
            for sesion in sesiones:
                sesion.detener.set()
        for sesion in sesiones:
            try:
                sesion.conexion.shutdown(socket.SHUT_RDWR)
            except OSError:
                pass
        super().server_close()
