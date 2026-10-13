# Firmware ESP32 — sensor y actuador

| Entorno | Placa | Programa |
|---|---|---|
| `sensor` (predeterminado) | ESP32 DEVKIT V1 de 30 pines (`esp32doit-devkit-v1`) | Mide con el HC-SR04 y publica `POST` al servidor Python. |
| `actuador` | ESP32 genérico de 38 pines (`esp32dev`) | Recibe `SET` del servidor y aplica los estados de los tres LEDs. |
| `esp32doit-devkit-v1` | ESP32 DEVKIT V1 | Programa local de referencia de la Práctica 1. |

El servidor decide los estados de los LEDs: el sensor no clasifica distancias y el actuador solo ejecuta los comandos recibidos.

## Configurar la red

Desde la raíz, copiar el ejemplo **si todavía no existe la configuración local**:

```powershell
Copy-Item apps/firmware/include/ConfiguracionRed.example.h apps/firmware/include/ConfiguracionRed.h
```

Editar el archivo copiado; ambas placas usan el mismo:

| Constante | Valor local |
|---|---|
| `SSID` | Nombre de la red WiFi de 2.4 GHz. |
| `CLAVE` | Contraseña de esa red; vacía para una red abierta. |
| `IP_SERVIDOR` | IPv4 de la PC en esa red. El servidor la muestra al iniciar con `--ip 0.0.0.0`; también se obtiene con `ipconfig`. |
| `PUERTO` | `5000`, igual al del servidor. |

`0.0.0.0` es la dirección de escucha del servidor, mientras que el ESP32 necesita la IPv4 concreta de la PC. Si la PC muestra varias (por ejemplo, un adaptador virtual `vEthernet`), usar la del adaptador WiFi. La IP se incorpora al compilar: si cambia, hay que recompilar y cargar ambas placas.

`ConfiguracionRed.h` está excluido de Git. Sin ese archivo se utiliza el ejemplo vacío: el proyecto compila, pero la consola pide completar la configuración y no intenta conexiones.

## Red para la demo

- **Firewall de Windows:** permitir conexiones entrantes TCP 5000. Desde una PowerShell de administrador:

  ```powershell
  New-NetFirewallRule -DisplayName "IoT P2 TCP 5000" -Direction Inbound -Protocol TCP -LocalPort 5000 -Action Allow -Profile Private
  ```

  La red WiFi debe estar como perfil **Privado** (Configuración → Red e Internet → propiedades de la red). Alternativamente, aceptar el aviso del firewall para `python.exe` en redes privadas la primera vez que se inicia el servidor.
- **Aislamiento de clientes:** algunas redes institucionales y hotspots impiden que los dispositivos se comuniquen entre sí. Si el ESP32 obtiene IP pero no conecta por TCP, usar un hotspot o un router propio que no aísle clientes.
- **Banda de 2.4 GHz:** el ESP32 no se asocia a redes de 5 GHz.
- Antes de la demo, comprobar la IP de la PC y probar la conexión con los simuladores desde otra PC o con la placa.

## Compilar y cargar

```powershell
pio run -e sensor
pio run -e actuador
```

Si PowerShell no encuentra `pio`, usar el ejecutable instalado por la extensión de PlatformIO:

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run -e actuador
```

Con cada placa conectada por un cable USB de datos, localizar su puerto y sustituir `COMx`:

```powershell
pio device list
pio run -e sensor -t upload --upload-port COMx
pio run -e actuador -t upload --upload-port COMy
pio device monitor -e actuador --port COMy
```

El monitor trabaja a 115200 baudios.

- **Sensor:** GPIO 5 para TRIG y GPIO 18 para ECHO, pulso de 10 µs y espera máxima de 30000 µs. ECHO se conecta mediante el divisor a 3.3 V del [circuito del sensor](../../docs/diagramas/Diseno_del_sistema.md#31-objeto-inteligente-con-sensor).
- **Actuador:** verde en GPIO 25, amarillo en GPIO 26 y rojo en GPIO 27, cada uno con su resistencia de 220 Ω a GND.

La guía completa de cableado y las comprobaciones con ambas placas pertenecen a la etapa 7.

## Servidor y simuladores de consola

Abrir terminales en la raíz del repositorio. Servidor:

```powershell
py -3.13 -m apps.servidor --ip 0.0.0.0 --puerto 5000
```

Para probar la placa sensor sin actuador, un actuador que imprime los estados:

```powershell
py -3.13 -m tools.simular_actuador --ip 127.0.0.1 --puerto 5000
```

Para probar la placa actuadora sin sensor, un sensor que publica distancias cada 250 ms, recorriendo la lista en ciclo durante `--duracion` segundos cada una:

```powershell
py -3.13 -m tools.simular_sensor --ip 127.0.0.1 --puerto 5000 --distancias 45,75,120,invalid --duracion 3
```

Desde otra PC, los simuladores reciben la IPv4 del servidor en `--ip`. Debe existir solamente un cliente por rol; cerrar el simulador de un rol antes de conectar la placa real de ese rol (si no, el servidor responde `role_busy`). `Ctrl+C` termina ambos simuladores.

Con lecturas, el actuador muestra rojo `blink_4` por debajo de 50 cm, amarillo `blink_2` entre 50 y menos de 100 cm y verde `on` desde 100 cm. Una lectura inválida o la pérdida del sensor apaga todos.

## Funcionamiento del cliente

- `ClienteTcp` atiende WiFi, apertura TCP, registro y transporte desde `actualizar()`. Solo `OK` completo habilita la publicación o la recepción de `SET`.
- Desactiva el modem-sleep de WiFi (`WiFi.setSleep(false)`) para no retrasar la recepción de comandos.
- El primer intento WiFi es inmediato; tras una pérdida se reintenta a los 2000 ms y, mientras no se complete, `WiFi.begin` se repite cada 10000 ms para no reiniciar una asociación en curso.
- `connect` espera como máximo 1000 ms por intento. Registro y envío pendiente tienen un plazo de 2000 ms. Tras un fallo, los buffers se descartan y se reintenta TCP después de 2000 ms.
- Cada mensaje lleva LF y ningún NUL en el cable. Los fragmentos de recepción se conservan hasta completar la línea.
- El transporte usa `send` y `recv` con `MSG_DONTWAIT` sobre el descriptor propiedad de `WiFiClient`. Evita los reintentos internos de varios segundos de [`WiFiClient::write` en Arduino-ESP32 2.0.17](https://github.com/espressif/arduino-esp32/blob/2.0.17/libraries/WiFi/src/WiFiClient.cpp#L359-L415).

### Sensor

- `PrincipalSensor` toma una medición nueva al registrarse y luego cada 250 ms.
- `ProtocoloAplicacion` produce dos decimales con punto; un timeout de ECHO produce `POST distance=invalid`.
- Existe un solo envío pendiente. Las escrituras parciales conservan su posición; mientras esté ocupado, el principal evita acumular mediciones.

### Actuador

- `ClienteTcp` valida cada `SET` completo y conserva hasta cuatro en orden; `PrincipalActuador` los consume en cada vuelta y los entrega a `ControladorLeds`.
- Los tres estados se aplican juntos. `blink_2` alterna cada 250 ms (2 ciclos/s) y `blink_4` cada 125 ms (4 ciclos/s); un parpadeo nuevo empieza encendido y un `SET` idéntico conserva la fase.
- Desconexión, `ERROR` o `SET` inválido cierran la conexión y apagan los LEDs al instante. Con 2000 ms sin `SET` completo se apagan manteniendo la conexión; el siguiente `SET` los reactiva.

En la consola de cada ESP32 se verán intentos WiFi/TCP, `Registro aceptado (OK)`, las lecturas publicadas o los apagados del actuador. `ERROR`, mensajes inesperados, desconexiones y plazos vencidos cierran el socket y comienzan un nuevo registro al reconectar.

## Validación

Las [pruebas nativas](../../tests/native) compilan `ClienteTcp.cpp`, `PrincipalSensor.cpp`, `PrincipalActuador.cpp` y `ControladorLeds.cpp` con WiFi, GPIO y reloj simulados. Verifican registro, cadencia, estados y frecuencias de los LEDs, fragmentos, escrituras parciales, fallos, plazos y desbordamiento de `millis()`.

CTest también ejecuta los flujos entre C++ y Python ([sensor](../../tests/python/test_sensor.py) y [actuador](../../tests/python/test_actuador.py)): un puente entrega los bytes del mismo firmware al servidor mediante sockets TCP reales. Se comprueban los tres rangos, lectura inválida, desconexiones, reinicio del servidor, pérdida de WiFi simulada y el plazo sin comandos del actuador. Los puentes existen únicamente en la construcción de pruebas.

Ejecutar la [suite del monorepo](../../README.md#pruebas) desde una terminal de desarrollo. En `unittest` por separado, los flujos del firmware se omiten si no se proporcionan `IOT_SENSOR_EXE` e `IOT_ACTUATOR_EXE`; CTest los configura automáticamente.

La compilación y las simulaciones están verificadas. La asociación WiFi, la consola serie, las medidas y el parpadeo sobre las placas reales quedan por comprobar con la configuración y el montaje locales.
