# Guía de montaje e integración — etapa 7

Pasos para montar los dos objetos inteligentes, conectarlos a la PC, cargar el firmware y poner en marcha el sistema completo. Las pruebas y la forma de registrar evidencias están en [PRUEBAS_FISICAS.md](PRUEBAS_FISICAS.md).

## 1. Materiales

| Objeto | Componentes |
|---|---|
| Sensor | ESP32 DEVKIT V1 (30 pines), HC-SR04, 3 resistencias de 2.2 kΩ, protoboard, cables. |
| Actuador | ESP32 genérico (38 pines), LED rojo, amarillo y verde, 3 resistencias de 220 Ω, protoboard, cables. |
| Comunes | 2 cables USB **de datos** (no solo de carga), PC con Python 3.13 y PlatformIO, red WiFi de 2.4 GHz. |

## 2. Objeto sensor

Esquema: [circuito-sensor.svg](diagramas/circuito-sensor.svg) y [diseño §3.1](diagramas/Diseno_del_sistema.md#31-objeto-inteligente-con-sensor).

| HC-SR04 | ESP32 DEVKIT V1 |
|---|---|
| VCC | VIN (5 V del USB) |
| GND | GND |
| TRIG | GPIO5 (directo) |
| ECHO | R1 de 2.2 kΩ → nodo → GPIO18; del nodo a GND, R2 + R3 de 2.2 kΩ en serie |

1. Con la placa **desconectada**, montar el divisor de ECHO: ECHO → R1 → nodo; nodo → GPIO18; nodo → R2 → R3 → GND.
2. **No conectar ECHO directo a GPIO18:** el HC-SR04 entrega 5 V y el ESP32 tolera 3.3 V. Con el divisor, el nodo queda en unos 3.33 V (5 V × 4.4 kΩ / 6.6 kΩ). Si hay multímetro, comprobar que el nodo no supera 3.4 V con el sensor alimentado.
3. Conectar TRIG a GPIO5, VCC a VIN y GND a GND.
4. Fijar el sensor mirando al frente, sin obstáculos cercanos, para medir contra una superficie plana (una caja o libro).

## 3. Objeto actuador

Esquema: [circuito-actuador.svg](diagramas/circuito-actuador.svg) y [diseño §3.2](diagramas/Diseno_del_sistema.md#32-objeto-inteligente-con-actuadores).

| LED | Conexión |
|---|---|
| Verde | GPIO25 → ánodo (pata larga) → cátodo → 220 Ω → GND |
| Amarillo | GPIO26 → ánodo → cátodo → 220 Ω → GND |
| Rojo | GPIO27 → ánodo → cátodo → 220 Ω → GND |

1. Cada LED lleva su propia resistencia; todas terminan en un GND del ESP32.
2. Si un LED no enciende nunca, revisar primero la polaridad (pata larga hacia el GPIO).
3. Los GPIO 25/26/27 de la placa de 38 pines pueden estar en otra posición física que en la de 30 pines: guiarse por la serigrafía de la placa, no por la posición.

## 4. Conexión a la PC

1. Conectar cada placa con un cable USB de datos. En el Administrador de dispositivos → *Puertos (COM y LPT)* debe aparecer un puerto (CP210x o CH340). Si no aparece, instalar el controlador del chip USB de la placa o probar otro cable.
2. Identificar qué COM corresponde a cada placa conectándolas de una en una:

   ```powershell
   pio device list
   ```

3. Si la carga se queda en `Connecting....`, mantener presionado el botón **BOOT** de la placa hasta que empiece a escribir.

## 5. Red y PC

1. La PC y los dos ESP32 deben estar en la **misma red WiFi de 2.4 GHz**. Las redes institucionales y algunos hotspots aíslan a los clientes; si los ESP32 obtienen IP pero no conectan por TCP, usar un hotspot o router propio.
2. Iniciar el servidor guardando el registro para las evidencias:

   ```powershell
   py -3.13 -m apps.servidor --ip 0.0.0.0 --puerto 5000 --registro docs/evidencias/p2/servidor.log
   ```

   La consola muestra `IPv4 de esta PC para IP_SERVIDOR: ...`. Usar la del adaptador WiFi (no la de adaptadores virtuales como `vEthernet` 172.x).
3. **Firewall de Windows:** aceptar el aviso para `python.exe` en redes **privadas**, o crear la regla desde una PowerShell de administrador:

   ```powershell
   New-NetFirewallRule -DisplayName "IoT P2 TCP 5000" -Direction Inbound -Protocol TCP -LocalPort 5000 -Action Allow -Profile Private
   ```

   La red WiFi debe estar como perfil *Privado* (Configuración → Red e Internet → propiedades de la red).
4. Comprobar desde otra PC o un celular en la misma red que el puerto responde, o directamente con las placas (paso 7).

## 6. Configurar y cargar el firmware

1. Copiar la configuración de ejemplo y completarla (no se sube a Git):

   ```powershell
   Copy-Item apps/firmware/include/ConfiguracionRed.example.h apps/firmware/include/ConfiguracionRed.h
   ```

   `SSID`, `CLAVE`, `IP_SERVIDOR` (la IPv4 del paso 5.2) y `PUERTO = 5000`.
2. Cargar cada placa en su puerto:

   ```powershell
   pio run -e sensor -t upload --upload-port COMx
   pio run -e actuador -t upload --upload-port COMy
   ```

3. **Si cambia la IP de la PC** (otra red, DHCP), hay que editar `IP_SERVIDOR` y volver a cargar ambas placas.

## 7. Puesta en marcha y comprobación

Orden recomendado: servidor → actuador → sensor. Abrir el monitor serie de cada placa (115200 baudios) en terminales separadas:

```powershell
pio device monitor --port COMy -b 115200
```

| Consola | Mensajes esperados |
|---|---|
| Actuador | `[Actuador] LEDs: verde=25, amarillo=26, rojo=27; apagados.` → `[WiFi] Conectado; IP local: ...` → `[TCP] Registro aceptado (OK).` |
| Sensor | `[Sensor] HC-SR04: TRIG=5, ECHO=18; intervalo=250 ms.` → `[TCP] Registro aceptado (OK).` → `[Sensor] POST distance=...` cuatro veces por segundo |
| Servidor | `REGISTER actuator aceptado`, `REGISTER sensor aceptado`, y pares `POST` / `SET` |

Con el sistema funcionando: objeto a menos de 50 cm → rojo parpadea rápido (4 Hz); entre 50 y 100 cm → amarillo parpadea lento (2 Hz); 100 cm o más → verde fijo; sin objeto delante → todos apagados.

## 8. Problemas frecuentes

| Síntoma | Causa probable | Qué hacer |
|---|---|---|
| `[Red] Completar apps/firmware/include/ConfiguracionRed.h` | Se compiló sin configuración local. | Crear `ConfiguracionRed.h` (paso 6.1) y volver a cargar. |
| `[WiFi] Intentando conectar.` repetido cada 10 s | SSID o clave incorrectos, red de 5 GHz o fuera de alcance. | Revisar `ConfiguracionRed.h`; usar una red de 2.4 GHz. |
| WiFi conectado pero `[TCP] Conexion fallida` | IP de la PC incorrecta, servidor detenido, firewall o aislamiento de clientes. | Comparar con la IPv4 del servidor; revisar paso 5. |
| `ERROR reason=role_busy` en la consola de la placa | Hay un simulador u otra placa con ese rol conectada. | Cerrar el simulador del mismo rol. |
| El sensor publica siempre `invalid` | Sin eco: cableado de ECHO/divisor, objeto fuera de alcance o superficie que no refleja. | Revisar el divisor y apuntar a una superficie plana a menos de 4 m. |
| Lecturas que saltan mucho | Superficie inclinada o blanda, o cables flojos en la protoboard. | Usar una superficie plana y perpendicular; asegurar cables. |
| Los LEDs se apagan solos cada 2 s | El actuador no recibe `SET`: el sensor está desconectado o el servidor lo cerró. | Revisar la consola del sensor y el registro del servidor. |
| Un LED no enciende | Polaridad invertida, resistencia o GPIO equivocado. | Probar el LED con `simular_sensor` y revisar la conexión. |

Para aislar un problema, probar cada placa con el simulador del otro rol (ver [README](../README.md#ejecutar)).
