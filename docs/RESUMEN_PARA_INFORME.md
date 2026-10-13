# Resumen para el informe — etapas 5 y 6 y simplificación

Estado al **13 de octubre de 2026**. Este documento resume lo implementado en las etapas 5 (cliente sensor) y 6 (cliente actuador), y la simplificación posterior del código, para actualizar `INFORME.md` mientras se realizan el montaje y las pruebas físicas (etapa 7). No reemplaza al informe: indica qué cambió, dónde está el código y qué evidencias existen.

## 0. Antes de editar

- La rama `desarrollo-informe` parte de `de12c3b` y **no incluye** las etapas 5 y 6 ni la simplificación. Para enlazar archivos que existan en el repositorio, conviene traer `main` a la rama del informe cuando estén fusionadas (`git merge origin/main`). Solo hay conflictos si alguien más editó `INFORME.md`.
- El informe actual dice que los clientes ESP32 están `[PENDIENTE DE IMPLEMENTACIÓN]`. **Ya están implementados ambos**; lo pendiente es la **validación física** (`[PENDIENTE DE MEDICIÓN]` / `[PENDIENTE DE EVIDENCIA]`).
- **Rutas que cambiaron con la simplificación.** Los enlaces del informe actual a `packages/Compartido/...`, `apps/firmware/src/main.cpp`, `ControladorLeds` de la P1 o `ControlDistancia.cpp` ya no existen; usar las rutas de §3.
- Las pruebas descritas abajo son automatizadas, con hardware simulado. No sustituyen las mediciones de PN-01 a PN-04, que se obtienen con las placas en la etapa 7.

## 1. Estado por requerimiento

| Req. | Implementación | Estado para el informe |
|---|---|---|
| RF1 | [`SensorUltrasonico`](../apps/firmware/src/SensorUltrasonico.cpp) + [`PrincipalSensor`](../apps/firmware/src/PrincipalSensor.cpp): una medición cada 250 ms tras el registro. | Implementado. Exactitud física pendiente (PN-02). |
| RF2 | [`dominio.py`](../apps/servidor/dominio.py) en la PC: `< 50`, `50 ≤ d < 100`, `≥ 100` cm. | Implementado y probado en límites (49.99/50/99.99/100). |
| RF3 | [`ControladorLeds`](../apps/firmware/src/ControladorLeds.cpp) + [`PrincipalActuador`](../apps/firmware/src/PrincipalActuador.cpp): rojo 4 Hz, amarillo 2 Hz, verde fijo; demás apagados. | Implementado y probado con GPIO simulados. Comportamiento visible pendiente (fotos/video). |
| RF4 | [`ClienteTcp`](../apps/firmware/src/ClienteTcp.cpp) en ambos ESP32 + [`servidor.py`](../apps/servidor/servidor.py): `REGISTER`/`OK`, `POST` → clasificación → `SET`. | Implementado; sensor simulado → servidor → actuador simulado verificado por TCP real. WiFi real pendiente. |
| RNF1–4 | Parámetros: 250 ms de muestreo, plazos de 2000 ms, `connect` de 1000 ms. | Pendiente de medición (PN-01 a PN-04). |
| RNF5 | Clases separadas por responsabilidad en C++ y Python (ver §3). | Puede documentarse con los enlaces de §3. |

## 2. Cambios de diseño respecto del informe actual

1. **Controlador de LEDs.** `ControladorLeds` recibe un comando completo (`establecerEstados(ComandoLeds)`), como en el diagrama de clases. El actuador **no clasifica**; el controlador de la Práctica 1 se eliminó del proyecto (queda en el historial de Git).
2. **Interfaz real de `ControladorLeds`:** `iniciar()`, `establecerEstados(const ComandoLeds&)`, `actualizar(unsigned long)`, `apagarTodos()`, `comandoActual()`. Pines: verde 25, amarillo 26, rojo 27.
3. **`ProtocoloAplicacion` C++** tiene solo lo que usan los ESP32: `crearRegistro`, `crearLectura` y `leerComando`. `ControlDistancia` existe solo en Python. Actualizar el diagrama de clases si el informe muestra otras funciones.
4. **Tiempos de `ClienteTcp`** (actualizar la tabla de temporización si el informe la repite):

   | Parámetro | Valor |
   |---|---|
   | Reintento TCP | 2000 ms |
   | Reintento WiFi | 2000 ms tras la pérdida; luego cada 10000 ms mientras no se asocie |
   | Espera máxima de `connect` | 1000 ms |
   | Plazo de registro (`OK`) y de envío pendiente | 2000 ms |
   | Ausencia de `SET` en el actuador | 2000 ms → apaga y mantiene la conexión |

5. **Ahorro de energía WiFi desactivado** (`WiFi.setSleep(false)`) en ambos ESP32, para no retrasar la recepción de comandos (relevante para RNF3).
6. **Servidor simplificado:** un hilo por conexión con sockets bloqueantes (plazo de 2 s) y `sendall`. El plazo de 2 s se renueva con cualquier dato recibido. Muestra al iniciar las IPv4 de la PC.
7. **Simuladores** [`tools/simular_sensor.py`](../tools/simular_sensor.py) y [`tools/simular_actuador.py`](../tools/simular_actuador.py): permiten probar cada placa sin la otra.

## 3. Estructura del código (para §3.2 y §3.3)

| Módulo | Clases / funciones | Responsabilidad |
|---|---|---|
| [`apps/servidor/servidor.py`](../apps/servidor/servidor.py) | `ServidorTcp`, `ManejadorConexion`, `Cliente`, `EstadoSistema` | Un hilo por conexión, roles, plazos y envío de `SET`. |
| [`apps/servidor/protocolo.py`](../apps/servidor/protocolo.py) | `leer_mensaje`, `crear_comando`, `ReceptorLineas` | Mensajes ASCII/LF en Python. |
| [`apps/servidor/dominio.py`](../apps/servidor/dominio.py) | `EstadoLed`, `ComandoLeds`, `ControlDistancia` | Reglas de rango en la PC. |
| [`apps/firmware/lib/Compartido`](../apps/firmware/lib/Compartido) | `Tipos.h`, `ProtocoloAplicacion`, `ReceptorLineas` | Código C++ común a ambos ESP32. |
| [`ClienteTcp`](../apps/firmware/src/ClienteTcp.cpp) | `iniciar`, `actualizar`, `enviar`, `recibir` | WiFi, TCP, registro y transporte sin bloqueo, común a ambos roles. |
| [`PrincipalSensor`](../apps/firmware/src/PrincipalSensor.cpp) | `setup`, `loop` | Mide cada 250 ms y publica `POST`. |
| [`PrincipalActuador`](../apps/firmware/src/PrincipalActuador.cpp) | `setup`, `loop` | Aplica cada `SET` y vigila los 2000 ms sin comandos. |
| [`ControladorLeds`](../apps/firmware/src/ControladorLeds.cpp) | `establecerEstados`, `actualizar`, `apagarTodos` | Parpadeo no bloqueante y conservación de fase. |
| [`SensorUltrasonico`](../apps/firmware/src/SensorUltrasonico.cpp) | `iniciar`, `medirDistanciaCm` | Pulso TRIG de 10 µs y eco con timeout de 30 ms. |

### Fragmentos clave y decisiones que conviene explicar

- **Parpadeo con `millis()` sin `delay()`** ([`ControladorLeds::actualizarLed`](../apps/firmware/src/ControladorLeds.cpp)): resta sin signo para soportar el desbordamiento de `millis()`; el temporizador avanza un intervalo fijo y se resincroniza tras una espera larga.
- **Conservación de fase:** el servidor envía un `SET` por cada `POST` (cada 250 ms). Un estado repetido no reinicia el parpadeo (`cambiarEstado` ignora estados iguales); si no, el LED a 2 Hz nunca completaría su ciclo.
- **Aplicación conjunta:** `leerComando` valida los tres estados antes de modificar el comando; un `SET` con un error no aplica nada.
- **Vigilancia del actuador** ([`PrincipalActuador.cpp`](../apps/firmware/src/PrincipalActuador.cpp)): con 2000 ms sin `SET` apaga los LEDs y conserva la conexión; cubre una caída del servidor aún no detectada.
- **Transporte sin bloqueo** ([`ClienteTcp`](../apps/firmware/src/ClienteTcp.cpp)): `send`/`recv` con `MSG_DONTWAIT`, porque `WiFiClient::write` de Arduino-ESP32 2.0.17 puede esperar varios segundos.
- **Reintento WiFi de 10 s:** la asociación WPA2 más DHCP tarda varios segundos; volver a llamar a `WiFi.begin()` cada 2 s podía reiniciar una conexión en curso.
- **Servidor con hilos y locks** ([`servidor.py`](../apps/servidor/servidor.py)): `EstadoSistema` protege los roles con un `Lock`; cada `Cliente` tiene un lock de envío porque el hilo del sensor escribe en el socket del actuador.

## 4. Pruebas automatizadas (para §4, separadas de las físicas)

Ejecución del 13/10/2026 en Windows con MSVC + CTest y Python 3.13: **7/7 suites aprobadas** (191 comprobaciones C++ y 26 pruebas Python). `sensor` y `actuador` compilan con PlatformIO 6.2.0 y Arduino-ESP32 2.0.17.

| Suite | Comprobaciones | Qué cubre |
|---|---:|---|
| `protocolo_aplicacion` | 29 | `REGISTER`, `POST` con dos decimales e `invalid`, `SET` válidos e inválidos. |
| `receptor_lineas` | 23 | Fragmentos, LF, longitud y bytes inválidos. |
| `cliente_tcp` | 76 | Registro, reintentos WiFi/TCP, envíos parciales, errores, plazos y desbordamiento. |
| `controlador_leds` | 21 | Estados, frecuencias, fase, aplicación conjunta y desbordamiento. |
| `principal_sensor` | 19 | Cadencia de 250 ms, eco → distancia, timeout → `invalid`, desconexión. |
| `principal_actuador` | 23 | Registro, `SET` fragmentado, plazo de 2 s, desconexión y `SET` inválido. |
| `servidor_python` | 26 pruebas | Dominio, protocolo, servidor con sockets reales, simuladores y analizador de registros. |

Correspondencia con los casos PV del [diseño](diagramas/Diseno_del_sistema.md#72-casos-para-validar-durante-la-implementación):

| Caso | Prueba automatizada |
|---|---|
| PV-01 a PV-03 | `test_servidor.test_rangos_limites_y_comandos_identicos`; `test_simuladores.test_flujo_completo_y_apagado_al_desconectar_el_sensor` |
| PV-04 | `principal_sensor` (timeout de eco → `invalid`); `test_servidor` (`invalid` apaga) |
| PV-05 | `controlador_leds` (`SET` idéntico cada 250 ms conserva la fase); `principal_actuador` |
| PV-06, PV-16 | `test_servidor.test_actuador_recibe_la_ultima_lectura_reciente`, `test_reconexion_del_actuador_conserva_la_lectura` |
| PV-07 | `test_servidor.test_rol_ocupado_no_afecta_al_original` |
| PV-08, PV-09 | `test_servidor.test_fragmentos_y_varios_mensajes_por_bloque`; `principal_actuador` (`SET` fragmentado) |
| PV-10, PV-11 | `test_servidor.test_linea_larga_cr_y_nul`, `test_mensajes_que_no_corresponden_al_rol`, `test_mensaje_antes_del_registro`; `protocolo_aplicacion` |
| PV-12 | `test_servidor.test_registro_vencido`; `cliente_tcp` |
| PV-13 | `test_servidor.test_sensor_sin_post_durante_2_s_se_cierra_y_apaga` |
| PV-14 | `principal_actuador` (2000 ms sin `SET` apaga y conserva la conexión) |
| PV-15 | `cliente_tcp` y `principal_sensor`/`principal_actuador` (pérdida y recuperación de WiFi/TCP) |

Cómo reproducirlas: ver [README § Pruebas](../README.md#pruebas). El registro de la ejecución queda en `build/windows/Testing/Temporary/LastTest.log` (no versionado); si se quiere como anexo, copiarlo a `docs/evidencias/`.

## 5. Lo que sigue pendiente (etapa 7, en curso)

Se completará con las placas y se pasará al informe a medida que haya evidencias. El procedimiento y las tablas a completar están en [PRUEBAS_FISICAS.md](PRUEBAS_FISICAS.md); la guía de conexión, en [GUIA_MONTAJE.md](GUIA_MONTAJE.md) (útil para §2.2, §2.3 y §3.1). Las evidencias se guardan en `docs/evidencias/p2/`.

- Montaje de ambos objetos según los circuitos del [diseño](diagramas/Diseno_del_sistema.md) (§3.1 y §3.2).
- PN-01 estabilidad ≥ 10 min, PN-02 exactitud ±3 cm (3–150 cm), PN-03 respuesta ≤ 1 s, PN-04 ≥ 2 lecturas/s.
- Fotos/video de los tres rangos, límites 50/100 cm, lectura inválida y recuperación tras desconexión.
- Riesgos a mencionar en *Análisis crítico* si aparecen: firewall de Windows o aislamiento de clientes en la red WiFi, asociación lenta y la velocidad del sonido (`0.0343 cm/µs` supone unos 20 °C).

## 6. Para la sección de aportes (§8.4)

- Etapa 5 (cliente sensor): commit `151e61d`, PR #5.
- Etapa 6 (cliente actuador) y auditoría: rama `ft/actuator-client`, PR #6.
- Simplificación del código: rama `refactor/simplify-code`.
- Montaje y pruebas físicas: en curso (etapa 7).
