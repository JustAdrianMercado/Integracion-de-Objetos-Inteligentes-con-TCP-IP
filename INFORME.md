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

Esta sección establece las condiciones que deberá satisfacer el sistema distribuido. Los requerimientos funcionales RF1–RF4 mantienen la numeración y el alcance de la [consigna oficial](docs/consigna/Práctica_2.md); los criterios de aceptación concretan las decisiones del [diseño del equipo](docs/diagramas/Diseno_del_sistema.md). Los requerimientos no funcionales RNF1–RNF5 declaran los atributos de calidad y sus métodos de verificación.

La adquisición y el envío de distancia corresponden al ESP32 sensor; la recepción, clasificación y generación de comandos corresponden al servidor TCP en la PC; la recepción y ejecución de esos comandos corresponden al ESP32 actuador. Esta distribución sitúa el algoritmo de control en el servidor, conforme a la consigna. Las clases citadas representan responsabilidades previstas y deberán contrastarse con el código final.

### 1.1 Requerimientos funcionales y criterios de aceptación

| ID | Funcionalidad requerida | Criterio de aceptación | Componente responsable según el diseño | Estado de implementación |
|---|---|---|---|---|
| RF1 | Medir la distancia entre el sensor ultrasónico HC-SR04 y un objeto. | Obtener lecturas finitas positivas expresadas en centímetros y contrastarlas con una referencia física mediante PN-02. Ante timeout de ECHO o valor no válido, identificar la lectura como inválida, según PV-04; la tolerancia de medición se evalúa en RNF2. | ESP32 sensor: `SensorUltrasonico` y programa principal de adquisición. | [PENDIENTE DE IMPLEMENTACIÓN] |
| RF2 | Interpretar cada distancia válida y clasificarla en tres rangos contiguos y sin solapamiento. | Asignar cada distancia finita positiva a exactamente un rango de §1.2. Las entradas de PV-01 a PV-03, incluidos 49.99, 50, 99.99 y 100 cm, deben producir la clasificación prevista y el comando correspondiente. | Servidor TCP en PC: `ControlDistancia`, coordinado por `ServidorTcp`. | [PENDIENTE DE IMPLEMENTACIÓN] |
| RF3 | Activar los tres LEDs del objeto actuador en función del rango, con un comportamiento distinto por rango. | Aplicar el comando completo según §1.2: rojo a 4 Hz, amarillo a 2 Hz o verde fijo, con los demás LEDs apagados; una lectura inválida debe producir todos apagados. Verificar estos estados mediante PV-01 a PV-04 y la continuidad de fase ante comandos idénticos mediante PV-05. | Servidor TCP: generación de `ComandoLeds`; ESP32 actuador: recepción y ejecución mediante `ControladorLeds`. | [PENDIENTE DE IMPLEMENTACIÓN] |
| RF4 | Enviar lecturas desde el objeto sensor al servidor y comandos desde el servidor al objeto actuador mediante sockets TCP sobre WiFi (IEEE 802.11), utilizando un protocolo de aplicación definido por el grupo. | Establecer una conexión de cada ESP32 hacia la PC, completar `REGISTER` / `OK` y comprobar con registros el recorrido integrado `POST` → clasificación en servidor → `SET` → actuación física. Los mensajes deben respetar la especificación de §2.6; PV-06 a PV-16 verificarán registro, delimitación, rechazo de errores y recuperación previstos. | ESP32 sensor y actuador: `ClienteTcp` y sus programas principales; PC: `ServidorTcp`; codificación e interpretación: `ProtocoloAplicacion`. | [PENDIENTE DE IMPLEMENTACIÓN] |

Los criterios se relacionan con el diseño y las validaciones mediante la matriz de §2.7. Las pruebas con entradas controladas al servidor permitirán verificar los límites exactos de RF2 sin confundirlos con el error físico del sensor; las pruebas del recorrido integrado comprobarán conjuntamente adquisición, comunicación y actuación. La revisión del código final y las evidencias de §4 serán necesarias para declarar el cumplimiento de cada requisito.

### 1.2 Rangos y lógica de control propuestos

Para la clasificación, se considera válida una distancia finita mayor que cero, expresada en centímetros. El servidor deberá aplicar la siguiente lógica y generar un comando con los estados de los tres LEDs:

| Distancia recibida | LED rojo | LED amarillo | LED verde |
|---|---|---|---|
| Lectura inválida | Apagado | Apagado | Apagado |
| 0 < distancia < 50 cm | Parpadeo a 4 Hz | Apagado | Apagado |
| 50 ≤ distancia < 100 cm | Apagado | Parpadeo a 2 Hz | Apagado |
| distancia ≥ 100 cm | Apagado | Apagado | Encendido fijo |

Los intervalos válidos son `(0, 50)`, `[50, 100)` y `[100, +∞)`. El valor 50 cm pertenece únicamente al rango amarillo y 100 cm únicamente al verde: cada límite se excluye del intervalo anterior y se incluye en el siguiente. De esta forma, no existen huecos ni solapamientos para las distancias finitas positivas. La lectura inválida constituye un estado de tratamiento de error, no un cuarto rango de distancia.

En el protocolo, `off` representa apagado y `on` encendido fijo. `blink_4` representa cuatro ciclos completos por segundo: un período de 250 ms, con 125 ms encendido y 125 ms apagado. `blink_2` representa dos ciclos completos por segundo: un período de 500 ms, con 250 ms encendido y 250 ms apagado. Estas frecuencias describen ciclos completos, no el número de alternancias del GPIO. Al comenzar un modo de parpadeo, el LED deberá encenderse; si se recibe de nuevo el mismo modo, deberá conservar su fase y temporización para que el envío periódico de comandos no reinicie el patrón.

Un timeout de ECHO, una lectura no finita o una distancia menor o igual que cero deberán normalizarse en el objeto sensor como `POST distance=invalid`. El servidor deberá interpretar ese mensaje como una lectura inválida y enviar `SET` con los tres estados `off`. El token `invalid` es válido dentro del protocolo; un mensaje mal formado o una distancia numérica cero o negativa recibida directamente se rechazarán conforme a sus reglas. Esta distinción evita tratar un fallo de comunicación como una medición válida.

El diseño también prevé el apagado al iniciar el actuador, ante una desconexión detectada y tras 2000 ms de ausencia de lecturas en el servidor o de comandos en el actuador, según corresponda. El registro de roles, el rechazo de mensajes y la reconexión son comportamientos complementarios de RF3 y RF4, especificados en §2.6 y cubiertos por PV-06 a PV-16; no se incorporan como requerimientos funcionales adicionales.

El dominio de clasificación no impone un límite superior de 150 cm ni excluye valores positivos inferiores a 3 cm. En cambio, **3 a 150 cm es el intervalo previsto para validar físicamente la exactitud** de RNF2. La cobertura lógica de los rangos no demuestra que el sensor cumpla la tolerancia fuera de ese intervalo ni que ya la cumpla dentro de él. Su cobertura experimental y sus resultados permanecen `[PENDIENTE DE MEDICIÓN]` y `[PENDIENTE DE EVIDENCIA]`.

### 1.3 Requerimientos no funcionales

Los cuatro atributos cuantitativos adoptan los valores de referencia de la consigna como objetivos del equipo. RNF5 concreta la exigencia de código legible, modular, orientado a objetos y documentado mediante una revisión verificable. Su alcance incluye ambos firmwares y el servidor.

| ID | Atributo de calidad | Criterio de aceptación | Método de verificación | Componente o alcance afectado | Estado de validación |
|---|---|---|---|---|---|
| RNF1 | Estabilidad | Operar durante al menos 10 minutos continuos, con cero reinicios y cero bloqueos, manteniendo la adquisición, el intercambio TCP y la respuesta de los LEDs. | PN-01: registrar inicio, fin y eventos de los tres equipos; observar continuidad de lecturas y actuación, y documentar cualquier interrupción. | Sistema distribuido completo: ESP32 sensor, servidor TCP y ESP32 actuador. | [PENDIENTE DE MEDICIÓN] / [PENDIENTE DE EVIDENCIA] |
| RNF2 | Exactitud de medición | Mantener un error absoluto máximo ≤ 3 cm respecto de una referencia física dentro del intervalo previsto de 3 a 150 cm. | PN-02: comparar lecturas individuales con una regla o cinta métrica en posiciones que cubran los tres rangos y los extremos del intervalo; calcular el valor absoluto de cada diferencia y su máximo. Registrar referencias, repeticiones y condiciones del ensayo. | HC-SR04, circuito y adquisición del ESP32 sensor; comprobar la conservación del valor comunicado al servidor. | [PENDIENTE DE MEDICIÓN] / [PENDIENTE DE EVIDENCIA] |
| RNF3 | Tiempo de respuesta extremo a extremo | Reflejar cada cambio de rango ensayado en el estado visible del actuador en un tiempo máximo de 1 s desde el cambio físico de distancia. | PN-03: registrar el cambio físico y la actuación con una grabación o instrumento de tiempo común; calcular la diferencia temporal por ensayo y el máximo observado, indicando resolución y transiciones evaluadas. | Recorrido completo sensor → servidor → actuador, incluidos adquisición, red, procesamiento y ejecución del comando. | [PENDIENTE DE MEDICIÓN] / [PENDIENTE DE EVIDENCIA] |
| RNF4 | Frecuencia de muestreo | Obtener al menos 2 lecturas/s durante la operación registrada del sensor. | PN-04: contar lecturas obtenidas durante una ventana de duración conocida y calcular cantidad/duración; contabilizar por separado lecturas inválidas y `POST` recibidos en el servidor para contrastar adquisición y recepción. | Planificación de adquisición en ESP32 sensor; recepción en servidor como comprobación del envío. | [PENDIENTE DE MEDICIÓN] / [PENDIENTE DE EVIDENCIA] |
| RNF5 | Calidad del código | Presentar módulos con responsabilidades delimitadas, clases que encapsulen estado y operaciones, interfaces documentadas, nombres y convenciones de C++ consistentes, y reglas de control separadas del hardware y los sockets. | Revisión de §3.4 sobre una versión identificada del código final: inspeccionar módulos, clases, interfaces y dependencias; registrar por criterio el cumplimiento, el archivo o fragmento que lo respalda y los ajustes pendientes. | Código de ambos ESP32, servidor y módulos compartidos de dominio y protocolo. | [PENDIENTE DE IMPLEMENTACIÓN] / [PENDIENTE DE EVIDENCIA] |

Para RNF2, la aceptación se basará en el **máximo de los errores absolutos individuales**, no únicamente en el promedio; este último podría ocultar lecturas que excedan la tolerancia. Las lecturas inválidas se registrarán por separado y no se usarán como errores numéricos. La conclusión experimental deberá indicar las posiciones realmente ensayadas y sus condiciones, sin generalizar a distancias no probadas.

Para RNF3, el origen temporal será el cambio físico de distancia y el final el cambio visible correspondiente del LED. Así se incluirá la espera hasta la siguiente adquisición y todo el recorrido de comunicación y control. No se restarán directamente timestamps de relojes de equipos distintos sin una referencia temporal común. El análisis de temporización del diseño podrá apoyar la explicación, pero no sustituirá la medición extremo a extremo exigida por la consigna.

La revisión de RNF5 deberá comprobar, en particular, que el sensor adquiere y comunica, el servidor clasifica y produce comandos, y el actuador ejecuta los estados recibidos. También verificará la encapsulación de pines y conexiones, la coherencia entre declaraciones y uso de interfaces, constantes con nombres descriptivos y comentarios que expliquen unidades, temporización y tratamiento de errores. El registro de revisión permitirá justificar cada criterio mediante enlaces al código y distinguir los hallazgos de los ajustes efectivamente realizados.

Los **valores objetivo** son los límites de aceptación de la tabla. Los **parámetros de configuración previstos** incluyen el intervalo de adquisición y envío de 250 ms, el timeout de ECHO de 30000 µs y los plazos de registro, ausencia de datos y reintento de 2000 ms. El intervalo nominal de 250 ms equivale teóricamente a 4 lecturas/s, pero no demuestra la frecuencia obtenida ni el tiempo de respuesta del sistema completo. Los **resultados medidos** se incorporarán en §4 y §5, con su versión de software y evidencias; en esta iteración permanecen pendientes. La revisión de implementación también permanece pendiente y los resultados de la Práctica 1 no acreditan estos requisitos de la Práctica 2.

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
