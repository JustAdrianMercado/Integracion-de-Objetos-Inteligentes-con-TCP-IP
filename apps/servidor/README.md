# Servidor TCP Python

Servidor Python 3.13 con biblioteca estándar: `socketserver`, `threading`, `logging`, `argparse` y módulos auxiliares. Mantiene el contrato de aplicación validado en C++, con un sensor y un actuador registrados simultáneamente.

## Ejecutar

Desde la raíz del monorepo:

```powershell
py -3.13 -m apps.servidor --ip 127.0.0.1 --puerto 5000
```

Para escuchar en las interfaces IPv4 de la PC:

```powershell
py -3.13 -m apps.servidor --ip 0.0.0.0 --puerto 5000
```

| Opción | Predeterminado | Uso |
|---|---|---|
| `--ip` | `0.0.0.0` | Dirección IPv4 local donde escuchar. |
| `--puerto` | `5000` | Puerto entre 0 y 65535; 0 selecciona uno libre para pruebas. |
| `--help` | — | Ayuda de argparse. |

Los ESP32 usarán la IPv4 real de la PC y el puerto efectivo. La consola registra conexiones aceptadas por rol, lecturas, comandos y cierres por ausencia de datos. `Ctrl+C` cierra el servicio, los sockets y los hilos de conexión.

## Responsabilidades

| Módulo | Responsabilidad |
|---|---|
| `__main__.py` | Argumentos, logging, inicio y cierre de consola. |
| `servidor.py` | Servidor, sesiones, roles, caducidad, recepción y envíos parciales. |
| `dominio.py` | `EstadoLed`, `ComandoLeds` y `ControlDistancia` de Python. |
| `protocolo.py` | Interpretación estricta de mensajes, codificación y delimitación LF. |

`ServidorTcp` deriva de [`socketserver.ThreadingTCPServer`](https://docs.python.org/3.13/library/socketserver.html). Cada conexión tiene un hilo, un receptor de líneas y una cola de salida. `EstadoSistema` protege los roles y la última lectura mediante un `threading.RLock`. Los envíos se hacen con sockets no bloqueantes y una cola con progreso parcial; no se hace I/O de red manteniendo el bloqueo de estado.

La cola tiene su propio bloqueo para que un `SET` generado en el hilo del sensor no se mezcle con los bytes enviados por el hilo del actuador. La reserva de un rol y la secuencia `OK` → `SET` se realizan bajo el bloqueo de estado. Dos registros simultáneos del mismo rol producen un solo ganador.

## Políticas conservadas

- `REGISTER` debe completarse dentro de 2000 ms; se responde `OK` al reservar el rol.
- Ambos órdenes de registro funcionan. El actuador recibe la última lectura si tiene menos de 2000 ms, o todos apagados.
- Cada `POST` del sensor genera `SET`, incluso si coincide con el anterior. `invalid` es una lectura válida del protocolo que apaga los LEDs y renueva el plazo.
- La desconexión o el rechazo del sensor invalida su lectura y envía todos apagados. Un sensor sin un `POST` completo durante 2000 ms se cierra y libera su rol.
- La desconexión del actuador conserva la lectura reciente del sensor para el siguiente registro.
- Un rol ocupado genera `role_busy`; un mensaje operativo válido antes del registro, `not_registered`; una sintaxis inválida, un segundo registro o un comando incompatible con el rol, `invalid_message`.
- Un `ERROR` se conserva en la cola antes de cerrar la conexión; el cierre se fuerza si no termina de enviarse dentro de 2000 ms.
- El límite es 127 bytes de contenido más LF. Se rechazan CR, NUL, controles y bytes fuera del ASCII imprimible.
- Se admiten hasta 8 conexiones y 4096 bytes pendientes por conexión. Las conexiones que superan esos límites se cierran.

La regla en ejecución pertenece a `dominio.py`: rojo a 4 Hz para menos de 50 cm, amarillo a 2 Hz desde 50 y antes de 100 cm, verde fijo desde 100 cm. El firmware del actuador implementará los parpadeos.

Las distancias recibidas se convierten a float32 antes de clasificarlas, para conservar el comportamiento de los clientes C++ en valores muy próximos a los límites. Las pruebas de contrato comparan ambas implementaciones, sin enlazar C++ con Python.

## Pruebas

```powershell
py -3.13 -m unittest discover -s tests/python -v
```

Ejecutan sockets reales en la propia PC, recepción fragmentada, roles ocupados y simultáneos, recuperación, datos inválidos, plazos y cierre de los hilos. También simulan escrituras parciales y sockets temporalmente ocupados. La comparación con C++ se ejecuta cuando está construido `verificar_contrato`; CTest proporciona su ruta automáticamente.

El servidor utiliza solamente la biblioteca estándar. CMake se usa para las pruebas nativas del firmware y del contrato, y el informe académico se trabaja por separado.
