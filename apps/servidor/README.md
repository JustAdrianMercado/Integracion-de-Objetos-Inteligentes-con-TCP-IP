# Servidor TCP para Windows

Aplicación C++17 con Winsock que recibe lecturas de un ESP32 sensor y envía estados de los tres LEDs a un ESP32 actuador. Reutiliza `Compartido` para interpretar el protocolo y calcular los rangos de distancia.

## Compilar

Desde la raíz del repositorio, en **Developer PowerShell for Visual Studio** o **x64 Native Tools Command Prompt for Visual Studio**:

```powershell
cmake -S . -B build/windows -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build/windows
```

El ejecutable queda en `build/windows/apps/servidor/servidor_tcp.exe`. CMake enlaza `ws2_32` del SDK de Windows. La compilación del servidor se habilita en Windows; la biblioteca compartida conserva su independencia del sistema operativo.

## Ejecutar

Para usar todas las interfaces IPv4 de la PC y el puerto previsto para los ESP32:

```powershell
.\build\windows\apps\servidor\servidor_tcp.exe
```

Para pruebas solamente en la PC:

```powershell
.\build\windows\apps\servidor\servidor_tcp.exe --ip 127.0.0.1 --puerto 5000
```

| Opción | Valor predeterminado | Uso |
|---|---|---|
| `--ip` | `0.0.0.0` | IPv4 local donde escuchar; `127.0.0.1` limita las pruebas a esta PC. |
| `--puerto` | `5000` | Puerto de `0` a `65535`; `0` pide uno libre al sistema para pruebas. |
| `--help` o `--ayuda` | — | Mostrar la ayuda y salir. |

La consola muestra la dirección y el puerto efectivo, los registros, las lecturas, los comandos y los cierres por error o ausencia de datos. **Ctrl+C** detiene el ciclo y cierra los sockets.

`0.0.0.0` es la dirección de escucha. Los clientes ESP32 configurarán como destino la IPv4 real de la PC en la red WiFi y el mismo puerto. Al indicar una IP concreta, debe pertenecer a una interfaz local de la PC. Una IP inválida o un puerto ocupado produce un error de inicio y termina el programa.

## Comportamiento

- Acepta un sensor y un actuador, en cualquier orden. Cada conexión debe completar `REGISTER` en 2000 ms y recibe `OK` al reservar su rol.
- Al registrar el actuador, envía el comando de la última lectura recibida hace menos de 2000 ms, o todos apagados si no hay una lectura reciente válida.
- Cada `POST` completo del sensor produce un `SET`, incluso si coincide con el anterior. Menos de 50 cm activa rojo a 4 Hz; desde 50 y antes de 100 cm, amarillo a 2 Hz; desde 100 cm, verde fijo. El actuador implementará los parpadeos al desarrollar su firmware.
- `POST distance=invalid` es válido, produce todos apagados y renueva el plazo de lecturas.
- Un sensor desconectado, rechazado o sin un `POST` completo durante 2000 ms invalida la lectura, libera su rol y produce un comando con todos apagados.
- La desconexión del actuador libera su rol y conserva la lectura del sensor. Un actuador nuevo recibe el estado reciente al registrarse.
- Un rol ocupado se rechaza con `ERROR reason=role_busy`, conservando las conexiones existentes. Un mensaje operativo válido antes de registrarse recibe `not_registered`; una sintaxis inválida, un segundo registro o un mensaje incompatible con el rol recibe `invalid_message`.
- Las conexiones rechazadas envían `ERROR` antes de cerrar. Si la respuesta queda bloqueada, el cierre se fuerza tras un máximo de 2000 ms.

Cada conexión tiene su propio receptor y cola de salida. Se conservan fragmentos, se procesan líneas completas en orden y se retiene el progreso de cada escritura parcial. Los sockets usan [modo no bloqueante](https://learn.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-ioctlsocket) y el ciclo utiliza [`select`](https://learn.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-select) con una espera máxima de 20 ms para atender también los temporizadores.

El servidor admite hasta 8 conexiones, contando las registradas y las pendientes. Las conexiones que exceden ese límite se cierran. Cada cola tiene un máximo de 4096 bytes pendientes; si se satura, se cierra esa conexión y se libera su rol. La atención de recepciones, escrituras y conexiones nuevas tiene un presupuesto por ciclo.

`ServidorTcp` debe utilizarse desde un solo hilo. `iniciar()` abre los recursos, `actualizar()` atiende sockets y plazos, y `detener()` los libera. `actualizar()` devuelve `false` ante un fallo del servicio; los errores de un cliente individual se resuelven cerrando ese cliente.

## Pruebas sin placas

```powershell
ctest --test-dir build/windows --output-on-failure
```

Las pruebas usan sockets TCP reales en `127.0.0.1` y puertos elegidos por Windows, sin depender del puerto 5000. Ejecutan ambos órdenes de registro, clasificación y límites, roles duplicados, errores, fragmentos, mensajes consecutivos, caducidad, desconexión y reconexión. La cola de salida se prueba con escrituras parciales y pausas simuladas.

La suite del servidor dura aproximadamente 15 segundos porque verifica los plazos reales de 2000 ms. Estas pruebas se ejecutan en la PC; la integración con las placas se realizará en las siguientes etapas.
