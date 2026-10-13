# Plan de implementación revisado

## Arquitectura y responsables

El sistema mantiene dos ESP32 clientes y una PC servidor. Los ESP32 usan C++ con Arduino-ESP32 2.0.17 y PlatformIO Core 6.2.0. La PC usa Python 3.13 y biblioteca estándar. El monorepo y el contrato ASCII/TCP se conservan.

El sensor mide y envía lecturas; el servidor clasifica y genera `SET`; el actuador aplica los estados de los LEDs. La lógica de clasificación en ejecución pertenece al servidor Python. El protocolo une ambas implementaciones; las pruebas comparan sus representaciones sin compartir un binario entre lenguajes.

La estructura actual ya separa `apps/firmware`, `apps/servidor`, `packages/Compartido` y las pruebas. La reorganización adapta responsabilidades y herramientas dentro de esa estructura.

## Revisión de las etapas implementadas

| Etapa | Decisión | Trabajo aprovechado |
|---|---|---|
| 1. Estados y distancia | Conservar los tipos C++; ejecutar las reglas en Python. | `EstadoLed`, `ComandoLeds`, casos de límites y `ControlDistancia` C++ como referencia de prueba. |
| 2. Monorepo | Conservar. | Firmware, servidor, paquetes, pruebas y configuraciones de raíz. |
| 3. Protocolo | Conservar y añadir la implementación Python. | `REGISTER`, `POST`, `SET`, `OK`, `ERROR`; LF, ASCII y máximo de 128 bytes en el cable. |
| 4. Servidor | Sustituir Winsock por Python. | Políticas de registro, roles ocupados, lectura reciente, apagado, caducidad, fragmentos y envío parcial. |

El protocolo C++ no necesita un refactor para hablar con Python. Las nuevas pruebas de contrato comprueban las mismas entradas en ambos lenguajes, incluida la representación float32 y los límites de 50 y 100 cm.

`SensorUltrasonico` se conserva y se verifica con GPIO simulados. El `ControladorLeds` actual pertenece al programa local de la Práctica 1: su clasificación se reemplazará por estados recibidos al desarrollar el actuador, en la etapa 6.

## Fix de arquitectura — fusionado

Rama: `fix/reorganize-architecture`. Commit: `fix: reorganize architecture`, fusionado en `main` antes de iniciar la etapa 5.

Esta entrega incluyó:

1. Servidor Python modular con `socketserver`, hilos de conexión, roles protegidos y logging.
2. Retirada del servidor C++ y de su dependencia Winsock en la construcción.
3. Traslado de los escenarios TCP y de envío parcial a `unittest`.
4. Conservación de las pruebas C++, comparación con Python y simulación de GPIO del sensor.
5. Versiones fijadas de la plataforma y el framework ESP32, con filtro explícito para el programa de referencia.
6. README y diseño actualizados; este plan define la siguiente entrega.

El firmware local continúa compilando en `esp32doit-devkit-v1`. Este fix dejó la base preparada para implementar los dos clientes en las etapas 5 y 6.

## Etapa 5 — Cliente ESP32 sensor

**Rama:** `ft/sensor-client`. **Estado:** implementada para revisión, sin commit ni push. Commit propuesto: `ft: implement ESP32 sensor client`.

**Objetivo:** el ESP32 de 30 pines con HC-SR04 se conecta por WiFi al servidor Python, completa su registro y envía una lectura cada 250 ms. Un actuador simulado en la PC permite verificar los comandos antes de implementar el segundo ESP32.

### Archivos y responsabilidades

| Archivo | Responsabilidad |
|---|---|
| `apps/firmware/src/cliente/PrincipalSensor.cpp` | `setup` y `loop` del sensor; medición periódica y publicación. |
| `apps/firmware/src/cliente/ClienteTcp.cpp` | Adaptador común: WiFi, TCP, registro, colas, recepción y reintentos. |
| `apps/firmware/include/ClienteTcp.h` | Interfaz común para el cliente sensor y el futuro actuador. |
| `apps/firmware/src/cliente/SensorUltrasonico.cpp` | Implementación actual del HC-SR04, movida a la carpeta del cliente. |
| `apps/firmware/include/ConfiguracionRed.example.h` | Ejemplo de SSID, contraseña, IP de la PC y puerto. |
| `apps/firmware/include/ConfiguracionRed.h` | Configuración local, excluida de Git. |
| `tools/simular_actuador.py` | Cliente Python de prueba que se registra y muestra `SET`. |

Los encabezados de hardware y los tipos de `packages/Compartido` se reutilizan. `WiFi.h` y `WiFiClient.h` los proporciona el framework Arduino-ESP32 fijado en PlatformIO.

### Trabajo implementado

1. Añadir la configuración de red de ejemplo y la exclusión del archivo local.
2. Implementar `ClienteTcp` con estados de conexión y registro. WiFi y TCP se atienden desde `actualizar`; las conexiones tienen un tiempo de espera acotado.
3. Crear `PrincipalSensor`, reutilizando `SensorUltrasonico` y el codificador de lecturas C++.
4. Mover la implementación del sensor y ajustar también el filtro del programa local para que siga compilando.
5. Habilitar el entorno `sensor` y establecerlo como entorno predeterminado para esta entrega.
6. Añadir el actuador simulado y las pruebas del flujo sensor → servidor Python → actuador simulado.

### Reglas del cliente

- Antes de publicar, completar `REGISTER type=sensor` y esperar `OK`, con un plazo de 2000 ms.
- Cada 250 ms enviar un `POST` nuevo mientras esté registrado. El mensaje usa punto y dos decimales; ausencia de ECHO, cero, negativos o datos no finitos producen `distance=invalid`.
- Mantener TRIG de 10 µs, timeout de ECHO de 30000 µs y, como base, GPIO 5 para TRIG y 18 para ECHO.
- Conservar el progreso de las escrituras parciales y los fragmentos de entrada. Transmitir un LF por mensaje y ningún terminador C.
- Ante `ERROR`, respuesta no permitida, registro vencido o desconexión, cerrar la conexión y reiniciar sus buffers y estado.
- Reintentar WiFi/TCP cada 2000 ms. Usar `millis()` y resta sin signo para los temporizadores, con un ciclo principal sin esperas repetitivas que detengan la atención del cliente.
- Tras reconectar, repetir el registro y publicar lecturas nuevas. El sensor no acumula un historial de mensajes durante la desconexión.
- La clasificación de rangos se ejecuta en Python; el programa sensor se dedica a medición y transporte.

El cliente conserva un solo envío pendiente y el principal omite nuevas mediciones mientras esté ocupado. Un envío atascado durante 2000 ms cierra la conexión. La apertura TCP espera como máximo 250 ms por intento; el plazo de registro comienza al terminar esa apertura. `send` y `recv` usan `MSG_DONTWAIT` sobre el descriptor de `WiFiClient` para atender escrituras y recepción sin esperas internas.

La configuración vacía permite compilar sin credenciales, pero deshabilita los intentos y muestra por consola cómo completar el archivo local. La [guía del firmware](../apps/firmware/README.md) describe configuración, carga y uso del simulador.

### Filtros implementados

Las opciones comunes de plataforma, framework y velocidad de monitor se heredan de `[env]`. `sensor` es el entorno predeterminado y tiene este filtro:

```ini
[env:sensor]
board = esp32doit-devkit-v1
build_src_filter =
    -<*>
    +<cliente/PrincipalSensor.cpp>
    +<cliente/ClienteTcp.cpp>
    +<cliente/SensorUltrasonico.cpp>
```

Los filtros se interpretan respecto de `apps/firmware/src`. Al excluir primero todo se evita incorporar el `main.cpp` de la Práctica 1 o el programa del actuador en este firmware.

### Criterios de aceptación

1. `pio run -e sensor` compila el firmware con los archivos del rol sensor y la biblioteca compartida.
2. El servidor Python acepta el registro y recibe nominalmente cuatro lecturas por segundo.
3. El actuador simulado recibe los tres rangos y el apagado ante una lectura inválida.
4. Reiniciar el servidor permite al sensor reconectar y registrarse de nuevo.
5. Cortar y recuperar WiFi produce reintentos acotados y una recuperación observable por consola serie.
6. Los mensajes fragmentados y las escrituras parciales se manejan completos; no se publica antes de `OK`.
7. Las suites Python y C++ continúan aprobadas. Las verificaciones que requieren placa se distinguen de las simuladas.

### Validación de la entrega

- Compilación del entorno `sensor` y del programa local `esp32doit-devkit-v1` aprobadas con Arduino-ESP32 2.0.17.
- Pruebas nativas del adaptador y del mismo principal sensor con WiFi, reloj y GPIO simulados: registro antes de publicar, intervalo de 250 ms, fragmentos, envíos parciales, errores, plazos, reintentos, descarte de datos anteriores y desbordamiento de `millis()`.
- Flujo automatizado del firmware C++ → servidor Python por TCP real → actuador simulado, incluyendo los tres rangos, lectura inválida, reinicio del servidor y pérdida/recuperación de WiFi simulada.
- La asociación a una red WiFi real, medidas físicas y recuperación observable por consola serie quedan pendientes de probar en la placa con su configuración local. La integración y el montaje completo se verifican en la etapa 7.

**Fin de etapa:** entregar los cambios sin commit ni push y esperar la revisión y publicación manual antes de iniciar el actuador.

## Etapa 6 — Cliente ESP32 actuador

Rama prevista: `ft/actuator-client`.

Crear `cliente/PrincipalActuador.cpp` y adaptar `ControladorLeds` para recibir `ComandoLeds`, reutilizando `ClienteTcp`. El programa clasificador local de la Práctica 1 sigue como referencia en su propio entorno.

El entorno `actuador` usará el perfil `esp32dev` para la placa genérica de 38 pines y un filtro que incluya solamente su principal, `ClienteTcp.cpp` y el controlador de LEDs. Ambos entornos compartirán la plataforma y el framework fijados.

El actuador aplicará conjuntamente los tres estados recibidos. `blink_2` alternará cada 250 ms y `blink_4` cada 125 ms. Los comandos idénticos conservarán la fase. Una desconexión o un `SET` inválido apagará inmediatamente; 2000 ms sin `SET` completo también apagarán las salidas, manteniendo la conexión si sigue abierta.

Ampliar las pruebas C++ con GPIO y reloj simulados para estados, frecuencias, ausencia de actuación parcial, watchdog y desbordamiento de `millis()`. Validar con el servidor Python y un sensor simulado antes de pasar a ambas placas.

## Etapa 7 — Integración física y pruebas

Rama prevista: `ft/system-integration`.

Entregar la guía de conexión para las placas reales: pines, HC-SR04, divisor de ECHO, resistencias de LEDs, alimentación, GND, cables USB de datos, puertos COM, carga de cada entorno, WiFi e IP de la PC.

Probar el sistema completo y registrar evidencia de rangos, límites, lectura inválida, recuperación, frecuencia de muestreo, tiempo de respuesta, exactitud y estabilidad durante al menos 10 minutos. Las conexiones físicas y los resultados se verifican en esta etapa; la redacción del informe académico continúa a cargo del compañero.

## Flujo de trabajo por entrega

1. Partir de `main` actualizado y abrir una rama para el trabajo acordado.
2. Implementar una sola etapa o fix, actualizar la guía técnica y ejecutar sus pruebas.
3. Dejar todos los cambios sin commit ni push para revisión.
4. El usuario revisa, confirma, sube y fusiona manualmente.
5. Iniciar la siguiente rama cuando la entrega anterior esté revisada y publicada.
