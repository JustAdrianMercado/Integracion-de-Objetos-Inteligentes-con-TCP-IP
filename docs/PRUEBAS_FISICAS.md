# Pruebas físicas — etapa 7

Procedimiento para validar el sistema montado y registrar las evidencias que pide la consigna (§3.2 y §5.2). Primero se monta y pone en marcha todo según la [guía de montaje](GUIA_MONTAJE.md). Las tablas están vacías para completarlas durante las pruebas; los valores van luego al informe (§4 y §5).

## Preparación

- Guardar todas las evidencias en `docs/evidencias/p2/`, sin reemplazar las de la Práctica 1.
- Iniciar el servidor con un registro por sesión, por ejemplo:

  ```powershell
  py -3.13 -m apps.servidor --ip 0.0.0.0 --puerto 5000 --registro docs/evidencias/p2/pn01-estabilidad.log
  ```

- Resumir un registro (lecturas por segundo, mayor intervalo, cierres y errores):

  ```powershell
  py -3.13 -m tools.analizar_registro docs/evidencias/p2/pn01-estabilidad.log
  ```

- Superficie de medición: plana, rígida y perpendicular al sensor (caja o libro). Cinta métrica desde el frente del HC-SR04.
- Videos con el celular a 30 o 60 fps, mostrando en el mismo cuadro el objeto, la cinta y los LEDs.
- Anotar: fecha, red WiFi usada, commit del código (`git log -1 --oneline`) y temperatura aproximada del ambiente.

| Dato de la sesión | Valor |
|---|---|
| Fecha y hora | |
| Commit | |
| Red WiFi (tipo: hotspot / router) | |
| Temperatura aproximada | |
| Integrantes presentes | |

## PF-01 — Rangos y límites (RF2, RF3; casos PV-01 a PV-03)

Colocar el objeto a cada distancia y anotar la lectura de la consola del sensor y el LED observado. Cerca de 50 y 100 cm la lectura física varía unos milímetros: el límite exacto ya se verifica en `test_servidor`; aquí se comprueba el comportamiento a cada lado.

| Referencia (cm) | Lectura del sensor (cm) | Esperado | LED observado | ¿Cumple? |
|---:|---:|---|---|---|
| 20 | | Rojo 4 Hz | | |
| 45 | | Rojo 4 Hz | | |
| 48 | | Rojo 4 Hz | | |
| 52 | | Amarillo 2 Hz | | |
| 75 | | Amarillo 2 Hz | | |
| 98 | | Amarillo 2 Hz | | |
| 102 | | Verde fijo | | |
| 130 | | Verde fijo | | |

Evidencia: fotos o un video corto por rango (`pf01-rojo.jpg`, `pf01-amarillo.jpg`, `pf01-verde.jpg`).

## PF-02 — Lectura inválida (PV-04)

1. Apuntar el sensor al vacío (más de 4 m) o tapar el emisor.
2. Esperado: la consola del sensor muestra `POST distance=invalid` y los tres LEDs se apagan.

| Situación | Consola del sensor | LEDs | ¿Cumple? |
|---|---|---|---|
| Sin objeto / sensor tapado | | | |

## PF-03 — Frecuencia del parpadeo (PV-05)

Grabar 10 s de cada modo y contar los encendidos.

| Modo | Encendidos contados en 10 s | Frecuencia medida (Hz) | Esperado | ¿Cumple? |
|---|---:|---:|---|---|
| Rojo `blink_4` | | | 4 Hz (40 encendidos) | |
| Amarillo `blink_2` | | | 2 Hz (20 encendidos) | |

## PF-04 — Desconexión y recuperación (PV-13 a PV-15)

| Prueba | Esperado | Observado | ¿Cumple? |
|---|---|---|---|
| Desconectar el USB del sensor con un LED activo | El actuador apaga en ≤ 2 s. | | |
| Volver a conectar el sensor | Se registra solo y los LEDs vuelven a responder. | | |
| Detener el servidor (`Ctrl+C`) y volver a iniciarlo | Ambas placas reintentan cada 2 s y se registran de nuevo. | | |
| Apagar el WiFi (hotspot) unos segundos y encenderlo | Los LEDs se apagan; las placas se reconectan y el sistema continúa. | | |
| Reiniciar el actuador (botón EN) | Arranca apagado y recibe el estado actual al registrarse. | | |

## PN-01 — Estabilidad (RNF1: ≥ 10 min sin reinicios ni bloqueos)

1. Iniciar el servidor con `--registro docs/evidencias/p2/pn01-estabilidad.log` y conservar también las consolas serie de ambos ESP32. Identificar cada equipo y la versión utilizada.
2. Registrar inicio y fin de una ventana externa común. Dejar el sistema funcionando al menos 12 minutos, moviendo el objeto entre rangos de vez en cuando.
3. Observar continuidad de adquisición, comunicación y actuación; anotar reinicios, bloqueos, desconexiones y recuperaciones del sensor, del actuador y del servidor con sus instantes.
4. Detener y ejecutar `tools.analizar_registro` sobre el archivo de la PC. Contrastar ese resumen con las consolas y observaciones de los tres equipos; no utilizar el log del servidor como prueba única de continuidad.

Criterio de aceptación: operación durante ≥ 600 s con cero reinicios y cero bloqueos en ambos ESP32 y el servidor, manteniendo el recorrido y la actuación. Registros repetidos, cierres, `ERROR` e intervalos entre `POST` son indicadores complementarios de incidencias; no sustituyen el registro de eventos ni garantizan por sí solos estabilidad de los tres equipos.

| Duración (s) | POST | Mayor intervalo (s) | Registros sensor / actuador | Reinicios sensor / actuador / PC | Bloqueos y continuidad de los tres equipos | Cierres / ERROR | ¿Cumple? |
|---:|---:|---:|---|---|---|---|---|
| | | | | | | | |

## PN-02 — Exactitud (RNF2: error absoluto máximo ≤ 3 cm entre 3 y 150 cm)

Tomar al menos tres lecturas individuales por posición de referencia, de la consola del sensor. Para cada lectura válida, calcular `error absoluto = |lectura − referencia|`. Evaluar el máximo individual por posición y el máximo global, no únicamente promedios. Registrar las lecturas inválidas por separado, sin asignarles error numérico cero ni ocultarlas. Anotar las condiciones y la cobertura realmente ensayada.

| Referencia (cm) | Lectura 1 | Lectura 2 | Lectura 3 | Error máximo individual (cm) | ¿Cumple? |
|---:|---:|---:|---:|---:|---|
| 3 | | | | | |
| 5 | | | | | |
| 20 | | | | | |
| 45 | | | | | |
| 75 | | | | | |
| 100 | | | | | |
| 125 | | | | | |
| 150 | | | | | |

Error absoluto máximo individual de todas las posiciones: ______ cm. Criterio: ≤ 3 cm en el intervalo ensayado de 3 a 150 cm. Si el error crece proporcionalmente con la distancia, considerar la velocidad del sonido (`0.0343 cm/µs` supone unos 20 °C) en el análisis, sin dar por demostrada esa causa.

## PN-03 — Tiempo de respuesta (RNF3: ≤ 1 s de extremo a extremo)

1. Grabar en un mismo cuadro el objeto, la referencia de distancia y los LEDs, con una única referencia temporal (60 fps si es posible); anotar la tasa real de cuadros y su resolución.
2. Mover el objeto entre dos rangos. Identificar en la grabación el instante del cambio físico de distancia/rango que inicia la respuesta, **sin esperar a que el movimiento termine**.
3. Identificar el cambio visible correspondiente del actuador. Contar los cuadros entre ambos eventos y calcular `tiempo = cuadros / fps`. No restar directamente timestamps de relojes distintos no sincronizados.
4. Repetir las transiciones y evaluar el máximo observado; conservar el video y los instantes seleccionados para cada ensayo.

| Ensayo | Transición | Cuadros desde el cambio físico hasta actuación | fps | Tiempo (s) | ¿≤ 1 s? |
|---:|---|---:|---:|---:|---|
| 1 | Verde → rojo | | | | |
| 2 | Rojo → verde | | | | |
| 3 | Verde → amarillo | | | | |
| 4 | Amarillo → rojo | | | | |
| 5 | Rojo → amarillo | | | | |

Tiempo máximo observado: ______ s. Criterio: ≤ 1 s. El intervalo nominal de adquisición y los tiempos configurados de red no sustituyen esta medición física.

## PN-04 — Frecuencia de muestreo (RNF4: adquisición ≥ 2 lecturas/s)

1. Definir una ventana externa de duración conocida, de al menos 60 s, e identificar el método de conteo verificable de cada adquisición del sensor. Registrar instrumento o fuente del conteo, inicio y fin.
2. Contar adquisiciones totales e inválidas del sensor por separado. Contabilizar también `POST` recibidos en la PC y cuántos comunican `invalid`; las llegadas TCP no prueban automáticamente todas las adquisiciones.
3. Calcular `frecuencia de adquisición = adquisiciones / duración` y, separadamente, `frecuencia de recepción = POST recibidos / duración`. Documentar pérdidas, envíos pendientes, reconexiones y diferencias entre los conteos.
4. Usar `tools.analizar_registro` de PN-01, o de otra sesión, como apoyo para recepción. Su estimación entre la primera y última llegada no equivale al conteo de adquisiciones en la ventana externa. Si solo se dispone del log de la PC, dejar pendiente la validación de RNF4.

| Duración (s) | Adquisiciones sensor | Inválidas sensor | POST recibidos | POST invalid | Frecuencia adquisición (lecturas/s) | Frecuencia recepción (POST/s) | ¿Adquisición ≥ 2/s? |
|---:|---:|---:|---:|---:|---:|---:|---|
| | | | | | | | |

Método y fuente del conteo de adquisiciones: [PENDIENTE DE EVIDENCIA]. Valor nominal: 4 lecturas/s con intervalo de 250 ms, condicionado al registro y capacidad de envío; no es un resultado medido.

## Lista de evidencias

| Archivo | Prueba |
|---|---|
| `pf01-rojo.jpg`, `pf01-amarillo.jpg`, `pf01-verde.jpg` | PF-01 |
| `pf03-parpadeo.mp4` | PF-03 |
| `pf04-recuperacion.mp4` | PF-04 |
| `pn01-estabilidad.log` | PN-01 y PN-04 |
| `pn03-respuesta.mp4` | PN-03 |
| `montaje-sensor.jpg`, `montaje-actuador.jpg` | Montaje |

Los videos pesados pueden subirse a un enlace externo y citarse en el informe; GitHub rechaza archivos de más de 100 MB.
