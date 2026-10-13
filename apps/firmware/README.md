# Firmware ESP32 — etapa 5

El entorno `sensor` ejecuta el cliente de la placa ESP32 DEVKIT V1 de 30 pines. Mide con el HC-SR04 y publica `POST` al servidor Python; el servidor decide los estados de los LEDs. El cliente del ESP32 actuador se implementará en la etapa 6.

## Configurar la red

Desde la raíz, copiar el ejemplo **si todavía no existe la configuración local**:

```powershell
Copy-Item apps/firmware/include/ConfiguracionRed.example.h apps/firmware/include/ConfiguracionRed.h
```

Editar el archivo copiado:

| Constante | Valor local |
|---|---|
| `SSID` | Nombre de la red WiFi de 2.4 GHz. |
| `CLAVE` | Contraseña de esa red; vacía para una red abierta. |
| `IP_SERVIDOR` | IPv4 de la PC en esa red, obtenida con `ipconfig`. |
| `PUERTO` | `5000`, igual al del servidor. |

La PC y el ESP32 deben poder comunicarse en la misma red. `0.0.0.0` es la dirección de escucha del servidor, mientras que el ESP32 necesita la IPv4 concreta de la PC. `127.0.0.1` sirve para el simulador en esa PC.

`ConfiguracionRed.h` está excluido de Git. Las credenciales se editan localmente y se incorporan al firmware al compilar. Sin ese archivo se utiliza el ejemplo vacío: el proyecto compila, pero la consola pide completar la configuración y no intenta conexiones.

## Compilar y cargar

```powershell
pio run -e sensor
```

`sensor` es también el entorno predeterminado de `pio run`. El programa local de la Práctica 1 sigue disponible con `pio run -e esp32doit-devkit-v1`.

Si PowerShell no encuentra `pio`, usar el ejecutable instalado por la extensión de PlatformIO:

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run -e sensor
```

Cuando el montaje esté preparado, localizar el puerto de la placa y sustituir `COMx` por ese puerto:

```powershell
pio device list
pio run -e sensor -t upload --upload-port COMx
pio device monitor -e sensor --port COMx
```

El monitor trabaja a 115200 baudios. El sensor conserva GPIO 5 para TRIG, GPIO 18 para ECHO, pulso de 10 µs y espera máxima de 30000 µs. ECHO se conecta mediante el divisor a 3.3 V del [circuito del sensor](../../docs/diagramas/Diseno_del_sistema.md#31-objeto-inteligente-con-sensor). La guía completa de cableado y las comprobaciones con ambas placas pertenecen a la etapa 7.

## Servidor y actuador de consola

Abrir dos terminales en la raíz del repositorio. En la primera:

```powershell
py -3.13 -m apps.servidor --ip 0.0.0.0 --puerto 5000
```

En la segunda, un actuador que imprime los estados sin una placa ni dependencias de pip:

```powershell
py -3.13 -m tools.simular_actuador --ip 127.0.0.1 --puerto 5000
```

Desde otra PC, el simulador recibe la IPv4 del servidor en `--ip`. Debe existir solamente un cliente por rol; cerrar otro actuador si el servidor responde `role_busy`. `Ctrl+C` termina el simulador.

El simulador muestra inicialmente todos apagados. Con el sensor enviando lecturas, muestra rojo `blink_4` por debajo de 50 cm, amarillo `blink_2` entre 50 y menos de 100 cm y verde `on` desde 100 cm. Una lectura inválida o la pérdida del sensor produce todos apagados.

## Funcionamiento del cliente

- `ClienteTcp` atiende WiFi, apertura TCP, registro y transporte desde `actualizar()`. Solo `OK` completo habilita la publicación.
- `PrincipalSensor` toma una medición nueva al registrarse y luego cada 250 ms. No clasifica distancias ni controla LEDs.
- `ProtocoloAplicacion` produce dos decimales con punto; un timeout de ECHO produce `POST distance=invalid`.
- Cada mensaje lleva LF y ningún NUL en el cable. Los fragmentos de recepción se conservan hasta completar la línea.
- Existe un solo envío pendiente. Las escrituras parciales conservan su posición; mientras esté ocupado, el principal evita acumular mediciones.
- `connect` espera como máximo 250 ms por intento. Registro y envío pendiente tienen un plazo de 2000 ms. Tras un fallo, los buffers se descartan y se reintenta después de 2000 ms.
- El transporte usa `send` y `recv` con `MSG_DONTWAIT` sobre el descriptor propiedad de `WiFiClient`. Evita los reintentos internos de varios segundos de [`WiFiClient::write` en Arduino-ESP32 2.0.17](https://github.com/espressif/arduino-esp32/blob/2.0.17/libraries/WiFi/src/WiFiClient.cpp#L359-L415).
- El adaptador común acepta `SET` completos para el futuro actuador y conserva hasta cuatro en orden. Si la aplicación no los consume y llena la cola, cierra la conexión. La actuación física se añade en la etapa 6.

En la consola del ESP32 se verán intentos WiFi/TCP, `Registro aceptado (OK)` y las lecturas publicadas. `ERROR`, mensajes inesperados, desconexiones y plazos vencidos cierran el socket y comienzan un nuevo registro al reconectar.

## Validación de esta entrega

Las [pruebas nativas](../../tests/native) compilan `ClienteTcp.cpp` y `PrincipalSensor.cpp` con WiFi, GPIO y reloj simulados. Verifican registro, cadencia, fragmentos, escrituras parciales, fallos, ausencia de datos acumulados y desbordamiento de `millis()`.

CTest también ejecuta [el flujo entre C++ y Python](../../tests/python/test_sensor.py): un puente entrega los bytes del mismo firmware al servidor mediante sockets TCP reales y el actuador simulado valida los resultados. Se comprueban los tres rangos, lectura inválida, reinicio del servidor y recuperación tras una pérdida de WiFi simulada. El puente existe únicamente en la construcción de pruebas.

Ejecutar la [suite del monorepo](../../README.md#pruebas) desde una terminal de desarrollo. En `unittest` por separado, los dos flujos del firmware se omiten si no se proporciona el ejecutable nativo mediante `IOT_SENSOR_EXE`; CTest lo configura automáticamente.

La compilación y las simulaciones están verificadas. La asociación WiFi, la consola serie y las medidas sobre la placa real quedan por comprobar con la configuración y el montaje locales.
