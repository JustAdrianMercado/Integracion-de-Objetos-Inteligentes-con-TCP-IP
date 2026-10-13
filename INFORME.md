# Informe técnico — Práctica 2

**Actividad:** Integración de Objetos Inteligentes con TCP/IP

**Asignatura:** SIS-234 — Internet de las Cosas

**Grupo:** 8

**Integrantes:** Santiago Javier Torres Vacaflores y Ariel Adrian Mercado Alegre

**Plataformas previstas:** dos ESP32 / ESP-WROOM-32 y una PC Windows

**Estado del documento:** primera iteración; estructura documental

## Introducción

La [consigna de la Práctica 2](docs/consigna/Práctica_2.md) exige integrar un objeto inteligente con sensor ultrasónico, otro con actuadores y un servidor TCP central mediante WiFi (IEEE 802.11). El [diseño propuesto](docs/diagramas/Diseno_del_sistema.md) contempla un HC-SR04, tres LEDs y una PC que clasifica las distancias y envía comandos de actuación.

Este informe prepara la organización, la trazabilidad y los registros necesarios para documentar el trabajo por iteraciones. Las descripciones de arquitectura, clases, protocolo y pruebas corresponden a un diseño previo a la implementación. Los resultados de la Práctica 1 se conservan en [INFORME_P1.md](docs/historico/INFORME_P1.md) como referencia de organización y profundidad; no acreditan el funcionamiento del sistema distribuido de la Práctica 2.

Los marcadores indican información por incorporar: `[PENDIENTE DE IMPLEMENTACIÓN]` para desarrollo y revisión del código, `[PENDIENTE DE MEDICIÓN]` para datos experimentales y `[PENDIENTE DE EVIDENCIA]` para registros verificables. Un resultado esperado es un criterio de prueba, no un resultado obtenido.

## Objetivo general

Diseñar, implementar y validar un sistema distribuido que mida distancias con un sensor ultrasónico, las procese en un servidor TCP y controle tres LEDs en un objeto inteligente independiente, mediante sockets TCP sobre WiFi y un protocolo de aplicación definido por el grupo.

## 1. Requerimientos Funcionales y No Funcionales

### 1.1 Requerimientos funcionales y criterios de aceptación

Los identificadores RF1–RF4 corresponden a la consigna. Los criterios concretos se basan en el diseño propuesto.

| ID | Requerimiento | Criterio de aceptación previsto | Estado |
|---|---|---|---|
| RF1 | Medir la distancia entre el sensor ultrasónico y un objeto. | Obtener lecturas en centímetros y contrastarlas con una referencia física; comunicar lecturas inválidas explícitamente. | [PENDIENTE DE IMPLEMENTACIÓN] |
| RF2 | Clasificar la distancia en al menos tres rangos contiguos y sin solapamiento. | Clasificar en la PC toda lectura válida según los límites de 50 y 100 cm de §1.2. | [PENDIENTE DE IMPLEMENTACIÓN] |
| RF3 | Activar los actuadores con un comportamiento distinto por rango. | Aplicar los modos de §1.2, apagar los LEDs no seleccionados y conservar la fase ante comandos idénticos. | [PENDIENTE DE IMPLEMENTACIÓN] |
| RF4 | Intercambiar lecturas y comandos mediante sockets TCP sobre WiFi con un protocolo definido. | Registrar ambos clientes y comprobar el flujo sensor → `POST` → servidor → `SET` → actuador, conforme al protocolo propuesto. | [PENDIENTE DE IMPLEMENTACIÓN] |

### 1.2 Rangos y lógica de control propuestos

| Distancia recibida | LED rojo | LED amarillo | LED verde |
|---|---|---|---|
| Lectura inválida | Apagado | Apagado | Apagado |
| 0 < distancia < 50 cm | Parpadeo a 4 Hz | Apagado | Apagado |
| 50 ≤ distancia < 100 cm | Apagado | Parpadeo a 2 Hz | Apagado |
| distancia ≥ 100 cm | Apagado | Apagado | Encendido fijo |

Los valores 50 y 100 cm pertenecen al intervalo que comienza en ese valor. El intervalo de trabajo previsto para la validación física es de 3 a 150 cm; su cobertura queda `[PENDIENTE DE MEDICIÓN]`.

### 1.3 Requerimientos no funcionales

| ID | Atributo | Criterio declarado según el diseño | Verificación prevista | Estado |
|---|---|---|---|---|
| RNF1 | Estabilidad | ≥ 10 minutos continuos sin reinicios ni bloqueos. | Observación y registro de eventos de los tres equipos. | [PENDIENTE DE MEDICIÓN] |
| RNF2 | Exactitud | Error absoluto máximo ≤ 3 cm dentro de 3 a 150 cm. | Comparación con regla o cinta métrica que cubra los tres rangos. | [PENDIENTE DE MEDICIÓN] |
| RNF3 | Tiempo de respuesta | Cambio visible del actuador en ≤ 1 s desde el cambio de distancia. | Medición extremo a extremo con grabación o instrumento de tiempo común. | [PENDIENTE DE MEDICIÓN] |
| RNF4 | Frecuencia de muestreo | ≥ 2 lecturas/s; intervalo nominal previsto de 250 ms. | Conteo de lecturas y de `POST` recibidos durante una ventana de tiempo. | [PENDIENTE DE MEDICIÓN] |
| RNF5 | Calidad del código | C++ legible, modular, orientado a objetos, documentado y conforme a sus convenciones. | Revisión de responsabilidades, nombres, comentarios y dependencias. | [PENDIENTE DE IMPLEMENTACIÓN] |

## 2. Análisis y Diseño

Los diagramas enlazados en esta sección representan la propuesta de la Práctica 2. Su correspondencia con el montaje y el software finales deberá revisarse tras la implementación.

### 2.1 Arquitectura distribuida del sistema

Consultar el [diagrama de despliegue y las capas de comunicación](docs/diagramas/Diseno_del_sistema.md#1-arquitectura-del-sistema). Ambos ESP32 iniciarían conexiones TCP persistentes hacia la PC; la clasificación se realizaría en el servidor. Las IP del diagrama son ilustrativas y el puerto propuesto es `5000`.

| Componente previsto | Responsabilidad | Intercambio principal | Configuración real |
|---|---|---|---|
| ESP32 sensor y HC-SR04 | Medir cada 250 ms y comunicar lecturas válidas o inválidas. | `REGISTER type=sensor`, `POST`. | [PENDIENTE DE IMPLEMENTACIÓN] |
| PC / servidor TCP | Registrar clientes, clasificar distancias y vigilar la ausencia de datos. | Recibir `POST`; enviar `OK`, `ERROR`, `SET`. | [PENDIENTE DE IMPLEMENTACIÓN] |
| ESP32 actuador y tres LEDs | Aplicar comandos y mantener el parpadeo. | `REGISTER type=actuator`; recibir `SET`. | [PENDIENTE DE IMPLEMENTACIÓN] |
| Router WiFi | Proporcionar conectividad IP entre los equipos. | Tráfico TCP sobre IEEE 802.11. | [PENDIENTE DE EVIDENCIA] |

### 2.2 Circuito del objeto sensor

![Circuito propuesto del objeto sensor](docs/diagramas/circuito-sensor.svg)

Consultar las [conexiones y el divisor de ECHO](docs/diagramas/Diseno_del_sistema.md#31-objeto-inteligente-con-sensor). El diseño utiliza TRIG en GPIO5 y ECHO mediante divisor resistivo en GPIO18.

**Montaje final, lista de componentes y correspondencia con el esquema:** [PENDIENTE DE EVIDENCIA]

### 2.3 Circuito del objeto actuador

![Circuito propuesto del objeto actuador](docs/diagramas/circuito-actuador.svg)

Consultar las [conexiones de los LEDs](docs/diagramas/Diseno_del_sistema.md#32-objeto-inteligente-con-actuadores). El diseño asigna GPIO27 al rojo, GPIO26 al amarillo y GPIO25 al verde, con una resistencia de 220 Ω por LED.

**Montaje final, lista de componentes y correspondencia con el esquema:** [PENDIENTE DE EVIDENCIA]

### 2.4 Diagrama de clases

Consultar el [diagrama de clases propuesto](docs/diagramas/Diseno_del_sistema.md#4-diagrama-de-clases-propuesto). Las interfaces y tipos allí definidos son propuestas; el registro de su implementación se prepara en §3.2.

**Justificación detallada y correspondencia con las interfaces finales:** [PENDIENTE DE IMPLEMENTACIÓN]

### 2.5 Diagramas de secuencia

- [Registro y funcionamiento normal](docs/diagramas/Diseno_del_sistema.md#51-registro-y-funcionamiento-normal).
- [Ausencia de datos y recuperación](docs/diagramas/Diseno_del_sistema.md#52-ausencia-de-datos-y-recuperación).

**Correspondencia de las secuencias con los registros reales de intercambio:** [PENDIENTE DE EVIDENCIA]

### 2.6 Protocolo de aplicación TCP

La [especificación propuesta](docs/diagramas/Diseno_del_sistema.md#6-especificación-del-protocolo-de-aplicación) define líneas ASCII delimitadas por LF (`\n`), con hasta 127 bytes de contenido y 128 bytes incluyendo LF. Prevé reconstruir mensajes fragmentados y separar mensajes agrupados en el flujo TCP.

| Mensaje previsto | Dirección | Finalidad |
|---|---|---|
| `REGISTER type=<sensor o actuator>` | Cliente → servidor | Solicitar el registro del rol. |
| `OK` | Servidor → cliente | Confirmar exclusivamente el registro. |
| `POST distance=<decimal positivo o invalid>` | Sensor → servidor | Comunicar una lectura en cm o su invalidez. |
| `SET redLed=<estado> yellowLed=<estado> greenLed=<estado>` | Servidor → actuador | Aplicar los tres modos: `off`, `on`, `blink_2` o `blink_4`. |
| `ERROR reason=<motivo>` | Servidor → cliente | Rechazar un mensaje o registro y cerrar la conexión. |

Los campos entre `< >` son marcadores descriptivos. El registro, la ausencia de datos y los reintentos contemplan plazos de 2000 ms según el diseño. La especificación completa conserva las reglas de sintaxis, orden, errores y recuperación.

**Correspondencia con el protocolo implementado y ejemplos reales de intercambio:** [PENDIENTE DE IMPLEMENTACIÓN] / [PENDIENTE DE EVIDENCIA]

### 2.7 Matriz de trazabilidad

| Requisito | Diseño relacionado | Implementación prevista | Validación prevista | Evidencia |
|---|---|---|---|---|
| RF1 | Circuito sensor y protocolo `POST`. | `SensorUltrasonico`, programa sensor: [PENDIENTE DE IMPLEMENTACIÓN]. | PV-04 y PN-02. | [PENDIENTE DE EVIDENCIA] |
| RF2 | Rangos de §1.2. | `ControlDistancia`: [PENDIENTE DE IMPLEMENTACIÓN]. | PV-01 a PV-03, incluidos límites. | [PENDIENTE DE EVIDENCIA] |
| RF3 | Circuito actuador y comandos `SET`. | `ControladorLeds`: [PENDIENTE DE IMPLEMENTACIÓN]. | PV-01 a PV-05. | [PENDIENTE DE EVIDENCIA] |
| RF4 | Arquitectura, secuencias y protocolo. | `ClienteTcp`, `ServidorTcp`, `ProtocoloAplicacion`: [PENDIENTE DE IMPLEMENTACIÓN]. | Flujo integrado y PV-06 a PV-16. | [PENDIENTE DE EVIDENCIA] |
| RNF1 | Operación distribuida y recuperación. | Sistema completo: [PENDIENTE DE IMPLEMENTACIÓN]. | PN-01. | [PENDIENTE DE EVIDENCIA] |
| RNF2 | Medición ultrasónica. | Módulo sensor: [PENDIENTE DE IMPLEMENTACIÓN]. | PN-02. | [PENDIENTE DE EVIDENCIA] |
| RNF3 | Flujo sensor → servidor → actuador. | Sistema completo: [PENDIENTE DE IMPLEMENTACIÓN]. | PN-03. | [PENDIENTE DE EVIDENCIA] |
| RNF4 | Medición y envío nominal cada 250 ms. | Programa sensor y recepción TCP: [PENDIENTE DE IMPLEMENTACIÓN]. | PN-04. | [PENDIENTE DE EVIDENCIA] |
| RNF5 | Clases y separación de responsabilidades. | Módulos y tipos de §3.2: [PENDIENTE DE IMPLEMENTACIÓN]. | Revisión de §3.4. | [PENDIENTE DE EVIDENCIA] |

## 3. Desarrollo e Implementación

Esta sección queda reservada para describir el desarrollo de la Práctica 2 con enlaces al código y a fragmentos clave, sin copiar archivos completos. Las clases del diseño no se presentan como implementadas.

### 3.1 Entorno de desarrollo y configuración

| Elemento | Base prevista | Versión o configuración utilizada |
|---|---|---|
| Firmware de ambos ESP32 | C++ / Arduino / PlatformIO. | [PENDIENTE DE IMPLEMENTACIÓN] |
| Servidor PC | C++ / Windows / Winsock. | [PENDIENTE DE IMPLEMENTACIÓN] |
| Red y servicio | WiFi local / IPv4 / puerto propuesto 5000. | [PENDIENTE DE IMPLEMENTACIÓN] |
| Compilación y ejecución | Documentar herramientas y pasos reproducibles. | [PENDIENTE DE IMPLEMENTACIÓN] |

### 3.2 Implementación de módulos y clases

| Módulo previsto | Clases o tipos propuestos | Responsabilidad | Archivo y enlace al código P2 | Estado |
|---|---|---|---|---|
| Sensor | `SensorUltrasonico`, programa principal sensor. | Medir y coordinar envío de lecturas. | [PENDIENTE DE IMPLEMENTACIÓN] | Reutilización prevista; por verificar. |
| Actuador | `ControladorLeds`, programa principal actuador. | Aplicar comandos y temporizar parpadeos. | [PENDIENTE DE IMPLEMENTACIÓN] | Adaptación prevista; por verificar. |
| Cliente de red | `ClienteTcp`. | Conectar, registrar, enviar, recibir y reintentar. | [PENDIENTE DE IMPLEMENTACIÓN] | Desarrollo propuesto. |
| Servidor de red | `ServidorTcp`. | Atender sockets, roles y ausencia de lecturas. | [PENDIENTE DE IMPLEMENTACIÓN] | Desarrollo propuesto. |
| Control en PC | `ControlDistancia`. | Convertir distancias en estados de LEDs. | [PENDIENTE DE IMPLEMENTACIÓN] | Desarrollo propuesto. |
| Dominio compartido | `EstadoLed`, `ComandoLeds`. | Representar la actuación. | [PENDIENTE DE IMPLEMENTACIÓN] | Tipos propuestos. |
| Protocolo compartido | `ProtocoloAplicacion`, `TipoCliente`. | Codificar y validar mensajes y roles. | [PENDIENTE DE IMPLEMENTACIÓN] | Desarrollo propuesto. |

### 3.3 Fragmentos clave y decisiones técnicas

| Aspecto por documentar | Explicación y decisión final | Enlace al fragmento |
|---|---|---|
| Medición, periodicidad y lectura inválida. | [PENDIENTE DE IMPLEMENTACIÓN] | [PENDIENTE DE IMPLEMENTACIÓN] |
| Registro, sockets y delimitación del flujo TCP. | [PENDIENTE DE IMPLEMENTACIÓN] | [PENDIENTE DE IMPLEMENTACIÓN] |
| Clasificación en servidor y actuación no bloqueante. | [PENDIENTE DE IMPLEMENTACIÓN] | [PENDIENTE DE IMPLEMENTACIÓN] |
| Errores, apagado y recuperación. | [PENDIENTE DE IMPLEMENTACIÓN] | [PENDIENTE DE IMPLEMENTACIÓN] |

### 3.4 Revisión de calidad del código

| Criterio RNF5 | Hallazgos y ajustes | Evidencia de revisión |
|---|---|---|
| POO, modularidad, nombres, convenciones, comentarios y separación de hardware/red/control. | [PENDIENTE DE IMPLEMENTACIÓN] | [PENDIENTE DE EVIDENCIA] |

## 4. Pruebas y Validaciones

El plan toma como referencia las [validaciones previstas del diseño](docs/diagramas/Diseno_del_sistema.md#7-trazabilidad-y-validaciones-previstas). No se registra ninguna prueba como ejecutada o aprobada en esta iteración.

### 4.1 Datos de la sesión y procedimiento

| Campo | Registro |
|---|---|
| Fecha, hora e integrantes responsables | [PENDIENTE DE EVIDENCIA] |
| Versiones o commits de ambos firmwares y del servidor | [PENDIENTE DE IMPLEMENTACIÓN] |
| Montaje, alimentación, red, IP y puerto reales | [PENDIENTE DE EVIDENCIA] |
| Instrumento de referencia y condiciones ambientales | [PENDIENTE DE EVIDENCIA] |
| Método, repeticiones y resolución temporal de las mediciones | [PENDIENTE DE MEDICIÓN] |

### 4.2 Casos de prueba funcionales

Para los límites exactos, distinguir pruebas con entradas controladas al servidor de pruebas físicas sujetas al error del sensor. Registrar el método empleado y verificar además el recorrido integrado sensor → servidor → actuador.

| ID | Requisitos | Entrada o situación prevista | Resultado esperado según el diseño | Resultado y evaluación | Evidencia |
|---|---|---|---|---|---|
| PV-01 | RF2, RF3 | 45 y 49.99 cm. | Solo rojo a 4 Hz. | [PENDIENTE DE MEDICIÓN] | [PENDIENTE DE EVIDENCIA] |
| PV-02 | RF2, RF3 | 50, 75 y 99.99 cm. | Solo amarillo a 2 Hz. | [PENDIENTE DE MEDICIÓN] | [PENDIENTE DE EVIDENCIA] |
| PV-03 | RF2, RF3 | 100 y 120 cm. | Solo verde fijo. | [PENDIENTE DE MEDICIÓN] | [PENDIENTE DE EVIDENCIA] |
| PV-04 | RF1, RF3, RF4 | Timeout de ECHO o lectura inválida. | `POST distance=invalid`; `SET` con todos apagados. | [PENDIENTE DE MEDICIÓN] | [PENDIENTE DE EVIDENCIA] |
| PV-05 | RF3, RF4 | Comandos idénticos cada 250 ms. | Conservar fase; ciclos de 500 o 250 ms. | [PENDIENTE DE MEDICIÓN] | [PENDIENTE DE EVIDENCIA] |
| PV-06 | RF4 | Registrar primero sensor y luego actuador; repetir en orden inverso. | Ambos órdenes funcionan; estado reciente o apagado inicial. | [PENDIENTE DE MEDICIÓN] | [PENDIENTE DE EVIDENCIA] |
| PV-07 | RF4 | Solicitar un rol ocupado desde un cliente nuevo. | `ERROR reason=role_busy`; conservar al cliente original. | [PENDIENTE DE MEDICIÓN] | [PENDIENTE DE EVIDENCIA] |
| PV-08 | RF4 | Fragmentar un `POST` o `SET` en TCP. | Procesarlo una sola vez tras recibir LF. | [PENDIENTE DE MEDICIÓN] | [PENDIENTE DE EVIDENCIA] |
| PV-09 | RF4 | Agrupar mensajes en una lectura TCP. | Procesarlos en orden y conservar el fragmento final. | [PENDIENTE DE MEDICIÓN] | [PENDIENTE DE EVIDENCIA] |
| PV-10 | RF4 | Líneas de 127 y 128 bytes de contenido antes de LF. | 127 cumple longitud, sujeto a sintaxis; 128 se rechaza. | [PENDIENTE DE MEDICIÓN] | [PENDIENTE DE EVIDENCIA] |
| PV-11 | RF3, RF4 | Comando desconocido, argumentos incompletos, coma decimal, cero o rol incompatible. | Rechazo sin actuación parcial; cierre y apagado según corresponda. | [PENDIENTE DE MEDICIÓN] | [PENDIENTE DE EVIDENCIA] |
| PV-12 | RF4 | Socket abierto sin completar registro en 2000 ms. | Cierre por plazo de registro y posterior reintento. | [PENDIENTE DE MEDICIÓN] | [PENDIENTE DE EVIDENCIA] |
| PV-13 | RF3, RF4 | Sensor deja de enviar; servidor activo. | Tras 2000 ms: invalidar datos, apagar LEDs y liberar sensor. | [PENDIENTE DE MEDICIÓN] | [PENDIENTE DE EVIDENCIA] |
| PV-14 | RF3, RF4 | Actuador no recibe comandos; desconexión aún no detectada. | Apagado local tras 2000 ms sin `SET`. | [PENDIENTE DE MEDICIÓN] | [PENDIENTE DE EVIDENCIA] |
| PV-15 | RF3, RF4 | Desconexión detectada y recuperación. | Apagado, reintentos cada 2000 ms, nuevo registro y restauración válida. | [PENDIENTE DE MEDICIÓN] | [PENDIENTE DE EVIDENCIA] |
| PV-16 | RF3, RF4 | Actuador se conecta con lectura reciente, inválida, antigua o inexistente. | Último estado reciente o todos apagados, según el protocolo. | [PENDIENTE DE MEDICIÓN] | [PENDIENTE DE EVIDENCIA] |

### 4.3 PN-01 — Estabilidad del sistema completo

**Criterio:** RNF1, ≥ 10 minutos sin reinicios ni bloqueos. Conservar eventos de los tres equipos y verificar continuidad de lecturas y actuación.

| Inicio | Fin | Duración | Reinicios por equipo | Bloqueos | Continuidad y fallos | Evaluación | Evidencia |
|---|---|---|---|---|---|---|---|
| [PENDIENTE DE MEDICIÓN] | [PENDIENTE DE MEDICIÓN] | [PENDIENTE DE MEDICIÓN] | [PENDIENTE DE MEDICIÓN] | [PENDIENTE DE MEDICIÓN] | [PENDIENTE DE MEDICIÓN] | [PENDIENTE DE MEDICIÓN] | [PENDIENTE DE EVIDENCIA] |

### 4.4 PN-02 — Exactitud de medición

**Criterio:** RNF2, error absoluto máximo ≤ 3 cm en el intervalo previsto de 3 a 150 cm. Agregar filas para referencias que cubran los tres rangos. Calcular `error absoluto = |lectura − referencia|` y conservar las lecturas individuales.

| Referencia física (cm) | Lecturas (cm) y cantidad | Mínimo / máximo / promedio (cm) | Error absoluto máximo (cm) | Evaluación | Evidencia |
|---|---|---|---|---|---|
| [PENDIENTE DE MEDICIÓN] | [PENDIENTE DE MEDICIÓN] | [PENDIENTE DE MEDICIÓN] | [PENDIENTE DE MEDICIÓN] | [PENDIENTE DE MEDICIÓN] | [PENDIENTE DE EVIDENCIA] |

**Cobertura real del intervalo y análisis de errores:** [PENDIENTE DE MEDICIÓN]

### 4.5 PN-03 — Tiempo de respuesta extremo a extremo

**Criterio:** RNF3, ≤ 1 s desde el cambio físico de distancia hasta el cambio visible del actuador. Registrar ambos eventos con una referencia temporal común; los relojes locales no sincronizados no permiten restar timestamps de equipos distintos directamente.

| Ensayo y transición de rango | Método y resolución temporal | Instante del cambio físico | Instante de actuación | Tiempo extremo a extremo | Evaluación | Evidencia |
|---|---|---|---|---|---|---|
| [PENDIENTE DE MEDICIÓN] | [PENDIENTE DE EVIDENCIA] | [PENDIENTE DE MEDICIÓN] | [PENDIENTE DE MEDICIÓN] | [PENDIENTE DE MEDICIÓN] | [PENDIENTE DE MEDICIÓN] | [PENDIENTE DE EVIDENCIA] |

**Repeticiones, máximo observado y análisis de demoras:** [PENDIENTE DE MEDICIÓN]

### 4.6 PN-04 — Frecuencia de muestreo

**Criterio:** RNF4, ≥ 2 lecturas/s. El intervalo propuesto de 250 ms equivale nominalmente a 4 lecturas/s, pero no constituye una medición. Contar por separado las lecturas obtenidas y los `POST` recibidos, indicando lecturas inválidas. Calcular `frecuencia = cantidad / duración en segundos`.

| Ventana de observación | Duración (s) | Lecturas obtenidas / inválidas | POST recibidos | Frecuencia sensor (lecturas/s) | Frecuencia recepción (POST/s) | Evaluación | Evidencia |
|---|---|---|---|---|---|---|---|
| [PENDIENTE DE MEDICIÓN] | [PENDIENTE DE MEDICIÓN] | [PENDIENTE DE MEDICIÓN] | [PENDIENTE DE MEDICIÓN] | [PENDIENTE DE MEDICIÓN] | [PENDIENTE DE MEDICIÓN] | [PENDIENTE DE MEDICIÓN] | [PENDIENTE DE EVIDENCIA] |

### 4.7 Incidencias y control de evidencias

| Caso o requisito | Incidencia observada | Causa y acción correctiva | Repetición y resultado | Evidencia |
|---|---|---|---|---|
| [PENDIENTE DE MEDICIÓN] | [PENDIENTE DE MEDICIÓN] | [PENDIENTE DE IMPLEMENTACIÓN] | [PENDIENTE DE MEDICIÓN] | [PENDIENTE DE EVIDENCIA] |

Relacionar cada registro con el índice de §8.2. Las fotografías estáticas documentarán el montaje y los estados visibles; la frecuencia de parpadeo y las mediciones temporales requieren registros adecuados.

## 5. Resultados

### 5.1 Cumplimiento de requisitos

| Requisito | Criterio de referencia | Resultado obtenido | Cumplimiento | Evidencia |
|---|---|---|---|---|
| RF1 | Medición ultrasónica y tratamiento de invalidez. | [PENDIENTE DE MEDICIÓN] | [PENDIENTE DE EVIDENCIA] | [PENDIENTE DE EVIDENCIA] |
| RF2 | Tres rangos y límites de 50 y 100 cm. | [PENDIENTE DE MEDICIÓN] | [PENDIENTE DE EVIDENCIA] | [PENDIENTE DE EVIDENCIA] |
| RF3 | Modos de LEDs y apagado según §1.2. | [PENDIENTE DE MEDICIÓN] | [PENDIENTE DE EVIDENCIA] | [PENDIENTE DE EVIDENCIA] |
| RF4 | Comunicación TCP/WiFi y protocolo. | [PENDIENTE DE MEDICIÓN] | [PENDIENTE DE EVIDENCIA] | [PENDIENTE DE EVIDENCIA] |
| RNF1 | ≥ 10 minutos sin reinicios ni bloqueos. | [PENDIENTE DE MEDICIÓN] | [PENDIENTE DE EVIDENCIA] | [PENDIENTE DE EVIDENCIA] |
| RNF2 | Error máximo ≤ 3 cm en 3 a 150 cm. | [PENDIENTE DE MEDICIÓN] | [PENDIENTE DE EVIDENCIA] | [PENDIENTE DE EVIDENCIA] |
| RNF3 | Respuesta extremo a extremo ≤ 1 s. | [PENDIENTE DE MEDICIÓN] | [PENDIENTE DE EVIDENCIA] | [PENDIENTE DE EVIDENCIA] |
| RNF4 | ≥ 2 lecturas/s. | [PENDIENTE DE MEDICIÓN] | [PENDIENTE DE EVIDENCIA] | [PENDIENTE DE EVIDENCIA] |
| RNF5 | Calidad del código según §3.4. | [PENDIENTE DE IMPLEMENTACIÓN] | [PENDIENTE DE EVIDENCIA] | [PENDIENTE DE EVIDENCIA] |

### 5.2 Análisis crítico y limitaciones

**Comparación de objetivos y resultados, errores, cobertura y restricciones del montaje distribuido:** [PENDIENTE DE MEDICIÓN] / [PENDIENTE DE EVIDENCIA]

## 6. Conclusiones

**Conclusiones sobre el objetivo general y el cumplimiento de requisitos, fundamentadas en §4 y §5:** [PENDIENTE DE MEDICIÓN] / [PENDIENTE DE EVIDENCIA]

## 7. Recomendaciones

| Hallazgo que sustenta la recomendación | Recomendación técnica | Prioridad y validación propuesta |
|---|---|---|
| [PENDIENTE DE EVIDENCIA] | [PENDIENTE DE MEDICIÓN] | [PENDIENTE DE MEDICIÓN] |

## 8. Anexos

### 8.1 Documentación y diagramas de referencia

| Anexo | Documento existente | Uso |
|---|---|---|
| A | [Consigna oficial P2](docs/consigna/Práctica_2.md). | Requisitos, entregables y rúbrica. |
| B | [Diseño del sistema P2](docs/diagramas/Diseno_del_sistema.md). | Arquitectura, clases, secuencias y protocolo propuestos. |
| C | [Circuito sensor](docs/diagramas/circuito-sensor.svg) y [circuito actuador](docs/diagramas/circuito-actuador.svg). | Esquemas del diseño previo. |
| D | [Informe P1 conservado](docs/historico/INFORME_P1.md). | Referencia histórica; sus evidencias corresponden a P1. |

### 8.2 Índice de evidencias de la Práctica 2

No se reutilizan las evidencias de la Práctica 1 como pruebas del sistema distribuido.

| ID reservado | Evidencia por incorporar | Prueba o sección relacionada | Archivo / enlace | Fecha, responsable y versión |
|---|---|---|---|---|
| EV-P2-01 | Fotografías de ambos objetos y del montaje distribuido. | §2.2, §2.3 y §4.1. | [PENDIENTE DE EVIDENCIA] | [PENDIENTE DE EVIDENCIA] |
| EV-P2-02 | Registros de mensajes, roles, errores y recuperación. | RF4; PV-06 a PV-16. | [PENDIENTE DE EVIDENCIA] | [PENDIENTE DE EVIDENCIA] |
| EV-P2-03 | Registros funcionales, límites y parpadeo. | PV-01 a PV-05. | [PENDIENTE DE EVIDENCIA] | [PENDIENTE DE EVIDENCIA] |
| EV-P2-04 | Registro continuo de estabilidad de los tres equipos. | PN-01. | [PENDIENTE DE EVIDENCIA] | [PENDIENTE DE EVIDENCIA] |
| EV-P2-05 | Lecturas originales y referencia física. | PN-02. | [PENDIENTE DE EVIDENCIA] | [PENDIENTE DE EVIDENCIA] |
| EV-P2-06 | Grabación o registro temporal extremo a extremo. | PN-03. | [PENDIENTE DE EVIDENCIA] | [PENDIENTE DE EVIDENCIA] |
| EV-P2-07 | Conteos y ventanas de muestreo y recepción. | PN-04. | [PENDIENTE DE EVIDENCIA] | [PENDIENTE DE EVIDENCIA] |
| EV-P2-08 | Registro de revisión de calidad del código. | RNF5 y §3.4. | [PENDIENTE DE EVIDENCIA] | [PENDIENTE DE EVIDENCIA] |

### 8.3 Código y reproducción de la Práctica 2

**Enlaces a versiones finales del código, instrucciones de compilación, configuración y ejecución de los tres equipos:** [PENDIENTE DE IMPLEMENTACIÓN]

### 8.4 Aportes del grupo

| Integrante | Aporte documentado | Commits o evidencia |
|---|---|---|
| Santiago Javier Torres Vacaflores | [PENDIENTE DE EVIDENCIA] | [PENDIENTE DE EVIDENCIA] |
| Ariel Adrian Mercado Alegre | [PENDIENTE DE EVIDENCIA] | [PENDIENTE DE EVIDENCIA] |
