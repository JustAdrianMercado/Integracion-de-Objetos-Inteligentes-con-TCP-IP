# Resumen para el informe — etapas 5 y 6

Estado al **13 de octubre de 2026**. Este documento resume lo implementado en las etapas 5 (cliente sensor) y 6 (cliente actuador), para actualizar `INFORME.md` mientras se realizan el montaje y las pruebas físicas (etapa 7). No reemplaza al informe: indica qué cambió, dónde está el código y qué evidencias existen.

## 0. Antes de editar

- La rama `desarrollo-informe` parte de `de12c3b` y **no incluye** las etapas 5 y 6. Para enlazar archivos que existan en el repositorio, conviene traer `main` a la rama del informe cuando `ft/actuator-client` esté fusionada (`git merge origin/main`). Solo hay conflictos si alguien más editó `INFORME.md`.
- El informe actual dice que los clientes ESP32 están `[PENDIENTE DE IMPLEMENTACIÓN]`. **Ya están implementados ambos**; lo pendiente es la **validación física** (`[PENDIENTE DE MEDICIÓN]` / `[PENDIENTE DE EVIDENCIA]`).
- Las pruebas descritas abajo son automatizadas, con hardware simulado. No sustituyen las mediciones de PN-01 a PN-04, que se obtienen con las placas en la etapa 7.

## 1. Estado por requerimiento

| Req. | Implementación | Estado para el informe |
|---|---|---|
| RF1 | [`SensorUltrasonico`](../apps/firmware/src/cliente/SensorUltrasonico.cpp) + [`PrincipalSensor`](../apps/firmware/src/cliente/PrincipalSensor.cpp): una medición cada 250 ms tras el registro. | Implementado. Exactitud física pendiente (PN-02). |
| RF2 | [`dominio.py`](../apps/servidor/dominio.py) en la PC: `< 50`, `50 ≤ d < 100`, `≥ 100` cm. | Implementado y probado en límites (49.99/50/99.99/100). |
| RF3 | [`ControladorLeds`](../apps/firmware/src/cliente/ControladorLeds.cpp) + [`PrincipalActuador`](../apps/firmware/src/cliente/PrincipalActuador.cpp): rojo 4 Hz, amarillo 2 Hz, verde fijo; demás apagados. | Implementado y probado con GPIO simulados. Comportamiento visible pendiente (fotos/video). |
| RF4 | [`ClienteTcp`](../apps/firmware/src/cliente/ClienteTcp.cpp) en ambos ESP32 + [`servidor.py`](../apps/servidor/servidor.py): `REGISTER`/`OK`, `POST` → clasificación → `SET`. | Implementado; flujo verificado por TCP real con firmware simulado. WiFi real pendiente. |
| RNF1–4 | Parámetros: 250 ms de muestreo, plazos de 2000 ms, `connect` de 1000 ms. | Pendiente de medición (PN-01 a PN-04). |
| RNF5 | Clases separadas por responsabilidad en C++ y Python (ver §3). | Puede documentarse con los enlaces de §3. |

## 2. Cambios de diseño respecto del informe actual

1. **Controlador de LEDs.** `ControladorLeds` ahora recibe un comando completo (`establecerEstados(ComandoLeds)`), como en el diagrama de clases. El controlador original de la Práctica 1, que clasificaba distancias en el ESP32, se renombró a [`ControladorLedsLocal`](../apps/firmware/src/ControladorLedsLocal.cpp) y solo lo usa el programa local de referencia (`main.cpp`). El actuador **no clasifica**.
2. **Interfaz real de `ControladorLeds`:** `iniciar()`, `establecerEstados(const ComandoLeds&)`, `actualizar(unsigned long)`, `apagarTodos()`, `comandoActual()`. Pines: verde 25, amarillo 26, rojo 27.
3. **Tiempos de `ClienteTcp`** (actualizar la tabla de temporización si el informe la repite):

   | Parámetro | Valor |
   |---|---|
   | Reintento TCP | 2000 ms |
   | Reintento WiFi | 2000 ms tras la pérdida; luego cada 10000 ms mientras no se asocie |
   | Espera máxima de `connect` | 1000 ms |
   | Plazo de registro (`OK`) y de envío pendiente | 2000 ms |
   | Ausencia de `SET` en el actuador | 2000 ms → apaga y mantiene la conexión |

4. **Ahorro de energía WiFi desactivado** (`WiFi.setSleep(false)`) en ambos ESP32, para no retrasar la recepción de comandos (relevante para RNF3).
5. **Servidor:** al escuchar en `0.0.0.0` muestra las IPv4 de la PC. Mientras entrega un `ERROR`, lee y descarta la entrada adicional del cliente para que el cierre no provoque un RST que haga perder ese `ERROR`.
6. **Nuevo simulador** [`tools/simular_sensor.py`](../tools/simular_sensor.py), complemento de [`tools/simular_actuador.py`](../tools/simular_actuador.py): permite probar cada placa sin la otra.

## 3. Estructura del código (para §3.2 y §3.3)

| Módulo | Clases / funciones | Responsabilidad |
|---|---|---|
| [`apps/servidor/servidor.py`](../apps/servidor/servidor.py) | `ServidorTcp`, `ManejadorConexion`, `EstadoSistema`, `Sesion`, `ColaEnvio` | Un hilo por conexión, roles, plazos, colas de envío parcial. |
| [`apps/servidor/protocolo.py`](../apps/servidor/protocolo.py) | `leer_mensaje`, `crear_comando`, `ReceptorLineas` | Contrato ASCII/LF estricto en Python. |
| [`apps/servidor/dominio.py`](../apps/servidor/dominio.py) | `EstadoLed`, `ComandoLeds`, `ControlDistancia` | Reglas de rango en la PC. |
| [`packages/Compartido/src`](../packages/Compartido/src) | `ProtocoloAplicacion`, `ReceptorLineas`, `ComandoLeds`, `EstadoLed`, `TipoCliente` | Contrato C++ compartido por ambos ESP32. |
| [`ClienteTcp`](../apps/firmware/src/cliente/ClienteTcp.cpp) | `iniciar`, `actualizar`, `enviar`, `recibir` | WiFi, TCP, registro y transporte sin bloqueo, común a ambos roles. |
| [`PrincipalSensor`](../apps/firmware/src/cliente/PrincipalSensor.cpp) | `setup`, `loop` | Mide cada 250 ms y publica `POST`. |
| [`PrincipalActuador`](../apps/firmware/src/cliente/PrincipalActuador.cpp) | `setup`, `loop` | Consume `SET`, aplica estados y vigila los 2000 ms sin comandos. |
| [`ControladorLeds`](../apps/firmware/src/cliente/ControladorLeds.cpp) | `establecerEstados`, `actualizar`, `apagarTodos` | Parpadeo no bloqueante y conservación de fase. |

### Fragmentos clave y decisiones que conviene explicar

- **Parpadeo con `millis()` sin `delay()`** ([`ControladorLeds::actualizarLed`](../apps/firmware/src/cliente/ControladorLeds.cpp)): resta sin signo para soportar el desbordamiento de `millis()`. El temporizador avanza un intervalo fijo para no acumular deriva y se resincroniza tras una espera larga.
- **Conservación de fase:** el servidor envía un `SET` por cada `POST` (cada 250 ms). Un estado repetido no reinicia el parpadeo (`cambiarEstado` ignora estados iguales); si no, el LED a 2 Hz nunca completaría su ciclo.
- **Aplicación conjunta y sin estados parciales:** `ClienteTcp` solo entrega `SET` completos y validados; los tres estados se aplican en el mismo `actualizar`.
- **Vigilancia del actuador** ([`PrincipalActuador.cpp`](../apps/firmware/src/cliente/PrincipalActuador.cpp)): con 2000 ms sin `SET` apaga los LEDs y conserva la conexión; cubre una caída del servidor que todavía no se haya detectado.
- **Transporte sin bloqueo** ([`ClienteTcp::avanzarEnvio` / `atenderRecepcion`](../apps/firmware/src/cliente/ClienteTcp.cpp)): `send`/`recv` con `MSG_DONTWAIT`, porque `WiFiClient::write` de Arduino-ESP32 2.0.17 puede esperar varios segundos.
- **Reintento WiFi de 10 s:** la asociación WPA2 más DHCP tarda varios segundos; volver a llamar a `WiFi.begin()` cada 2 s podía reiniciar una conexión en curso.
- **Float32 en el servidor:** Python convierte la distancia a float32 antes de clasificar, para coincidir con el C++ cerca de 50 y 100 cm.

## 4. Pruebas automatizadas (para §4, separadas de las físicas)

Ejecución del 13/10/2026 en Windows con MSVC + CTest y Python 3.13: **9/9 suites aprobadas, 414 comprobaciones C++ y 44 pruebas Python, sin omisiones**. Los tres firmwares (`sensor`, `actuador`, `esp32doit-devkit-v1`) compilan con PlatformIO 6.2.0 y Arduino-ESP32 2.0.17.

| Suite | Comprobaciones | Qué cubre |
|---|---:|---|
| `control_distancia` | 26 | Reglas C++ de referencia y límites. |
| `protocolo_aplicacion` | 220 | Codificación e interpretación estricta. |
| `receptor_lineas` | 24 | Fragmentos, LF, longitud y bytes inválidos. |
| `sensor_ultrasonico` | 6 | Conversión del eco y timeout con GPIO simulados. |
| `cliente_tcp` | 75 | Registro, reintentos WiFi/TCP, envíos parciales, errores y desbordamiento. |
| `principal_sensor` | 19 | Cadencia de 250 ms y comportamiento ante desconexión. |
| `controlador_leds` | 21 | Estados, frecuencias, fase, aplicación conjunta y desbordamiento. |
| `principal_actuador` | 23 | Registro, `SET` fragmentado, plazo de 2 s, desconexión y `SET` inválido. |
| `servidor_python` | 44 pruebas | Servidor, contrato C++/Python (127 vectores) y flujos completos con el firmware compilado. |

Correspondencia con los casos PV del [diseño](diagramas/Diseno_del_sistema.md#72-casos-para-validar-durante-la-implementación):

| Caso | Prueba automatizada |
|---|---|
| PV-01 a PV-03 | `test_servidor.test_actuador_primero_rangos_y_comandos_identicos`; `test_actuador.test_rangos_parpadeo_invalid_y_desconexion_del_sensor` |
| PV-04 | `principal_sensor` (timeout de eco → `invalid`); `test_actuador` (`invalid` apaga) |
| PV-05 | `controlador_leds` (SET idéntico cada 250 ms conserva la fase); `principal_actuador` |
| PV-06, PV-16 | `test_servidor`: `sensor_primero_ultima_lectura_reciente`, `lectura_invalid_reciente`, `lectura_caducada` |
| PV-07 | `test_servidor.test_roles_duplicados_preservan_clientes_originales` |
| PV-08, PV-09 | `test_servidor.test_fragmentos_consecutivos_y_otro_cliente_incompleto`; `principal_actuador` (SET fragmentado) |
| PV-10, PV-11 | `test_servidor.test_longitud_cr_y_nul`, `test_mensajes_malformados`; `protocolo_aplicacion` |
| PV-12 | `test_servidor.test_plazo_registro_sin_datos`; `cliente_tcp` |
| PV-13 | `test_servidor.test_sensor_sin_primer_post`, `test_post_incompleto_no_renueva_plazo` |
| PV-14 | `principal_actuador`; `test_actuador.test_sin_set_durante_2000_ms_apaga_y_conserva_la_conexion` |
| PV-15 | `test_sensor.test_reinicio_servidor_y_perdida_wifi_exigen_registro_y_eco_nuevos`; `principal_actuador` (recuperación) |

Cómo reproducirlas: ver [README § Pruebas](../README.md#pruebas). El registro de la ejecución queda en `build/windows/Testing/Temporary/LastTest.log` (no versionado); si se quiere como anexo, copiarlo a `docs/evidencias/`.

## 5. Lo que sigue pendiente (etapa 7, en curso)

Se completará con las placas y se pasará al informe a medida que haya evidencias:

- Montaje de ambos objetos según los circuitos de [diseño](diagramas/Diseno_del_sistema.md) (§3.1 y §3.2).
- PN-01 estabilidad ≥ 10 min, PN-02 exactitud ±3 cm (3–150 cm), PN-03 respuesta ≤ 1 s, PN-04 ≥ 2 lecturas/s.
- Fotos/video de los tres rangos, límites 50/100 cm, lectura inválida y recuperación tras desconexión.
- Riesgos a mencionar en *Análisis crítico* si aparecen: firewall de Windows o aislamiento de clientes en la red WiFi, asociación lenta y la velocidad del sonido (`0.0343 cm/µs` supone unos 20 °C).

## 6. Para la sección de aportes (§8.4)

- Etapa 5 (cliente sensor): commit `151e61d`, PR #5.
- Etapa 6 (cliente actuador) y correcciones de la auditoría: rama `ft/actuator-client`.
- Montaje y pruebas físicas: en curso (etapa 7).
