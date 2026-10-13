# Servidor TCP Python

Python 3.13 con biblioteca estándar. Acepta un sensor y un actuador, clasifica cada distancia recibida y envía el comando de LEDs al actuador.

## Ejecutar

Desde la raíz del repositorio:

```powershell
py -3.13 -m apps.servidor --ip 0.0.0.0 --puerto 5000
```

| Opción | Predeterminado | Uso |
|---|---|---|
| `--ip` | `0.0.0.0` | IPv4 local donde escuchar; `127.0.0.1` para pruebas en la misma PC. |
| `--puerto` | `5000` | Puerto TCP; `0` elige uno libre. |
| `--registro` | — | Archivo donde guardar también el registro, como evidencia; `tools.analizar_registro` lo resume. |

Con `0.0.0.0` la consola muestra las IPv4 de la PC para configurar `IP_SERVIDOR` en los ESP32. También registra registros, lecturas, comandos, errores y cierres. `Ctrl+C` cierra el servidor y sus conexiones.

## Módulos

| Módulo | Contenido |
|---|---|
| `__main__.py` | Argumentos, logging, inicio y cierre. |
| `servidor.py` | `ServidorTcp` (un hilo por conexión), `ManejadorConexion`, `Cliente` y `EstadoSistema` (roles y última lectura). |
| `protocolo.py` | `leer_mensaje`, `crear_comando`, `crear_error`, `codificar` y `ReceptorLineas`. |
| `dominio.py` | `EstadoLed`, `ComandoLeds` y `ControlDistancia`: las reglas de rango. |

## Funcionamiento

- Cada conexión tiene su hilo, que lee del socket con un plazo de 2 s y separa líneas terminadas en LF.
- `EstadoSistema` guarda qué cliente tiene cada rol y la última lectura, protegido por un `threading.Lock`. Cada `Cliente` tiene además su propio lock de envío para que dos hilos no mezclen líneas.
- `REGISTER` reserva el rol y responde `OK`. Al registrar el actuador se le envía el estado actual si la última lectura tiene menos de 2 s; si no, todos apagados.
- Cada `POST` del sensor genera un `SET` para el actuador, aunque sea igual al anterior. `distance=invalid` apaga los LEDs.
- **Plazos:** sin datos durante 2 s antes de registrarse → `ERROR reason=not_registered`; un sensor sin datos durante 2 s se cierra y se apagan los LEDs. El actuador puede estar en silencio indefinidamente.
- La desconexión o el rechazo del sensor apaga los LEDs. La desconexión del actuador conserva la lectura para su siguiente registro.
- Si el envío al actuador falla o queda bloqueado más de 2 s, se cierra su conexión y se libera el rol.

## Errores

| Situación | Respuesta |
|---|---|
| Rol ya ocupado | `ERROR reason=role_busy`; el cliente existente no se modifica. |
| `POST` o `SET` antes de registrarse, o registro no completado en 2 s | `ERROR reason=not_registered` |
| Sintaxis inválida, línea de más de 127 bytes, CR/NUL, segundo registro o mensaje que no corresponde al rol | `ERROR reason=invalid_message` |

Tras un `ERROR` la conexión se cierra. Antes de cerrar, el servidor lee lo que el cliente siga enviando: cerrar con datos sin leer enviaría un RST y el cliente podría perder el `ERROR`.

## Pruebas

```powershell
py -3.13 -m unittest discover -s tests/python -v
```

`test_servidor.py` usa sockets reales en `127.0.0.1` con los plazos reales de 2 s: rangos y límites, orden de registro, lecturas recientes, roles ocupados, mensajes inválidos, fragmentos, plazos, desconexiones y cierre del servidor.
