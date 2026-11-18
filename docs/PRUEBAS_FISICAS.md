# Pruebas físicas

Procedimiento para validar el sistema montado y registrar las evidencias que pide la consigna (§3.2 y §5.2). Primero se monta y pone en marcha todo según la [guía de montaje](GUIA_MONTAJE.md). Las tablas sirven como plantilla para cada sesión de pruebas; los resultados que obtuvimos están en el [informe](../INFORME.md) (§4 y §5).

## Preparación

- Guardar las evidencias en `docs/evidencias/`.
- Iniciar el servidor con un registro por sesión, por ejemplo:

  ```powershell
  py -3.13 -m apps.servidor --ip 0.0.0.0 --puerto 5000 --registro docs/evidencias/estabilidad.log
  ```

- Resumir un registro (lecturas por segundo, mayor intervalo, cierres y errores):

  ```powershell
  py -3.13 -m tools.analizar_registro docs/evidencias/estabilidad.log
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

## Rangos y límites (RF2, RF3; casos PV-01 a PV-03)

Colocar el objeto a cada distancia y anotar la lectura de la consola del sensor y el LED observado. Cerca de 5, 15 y 25 cm la lectura física varía unos milímetros: el límite exacto ya se verifica en `test_servidor`; aquí se comprueba el comportamiento a cada lado.

| Referencia (cm) | Lectura del sensor (cm) | Esperado | LED observado | ¿Cumple? |
|---:|---:|---|---|---|
| 3 | | Rojo 4 Hz | | |
| 5 | | Rojo 4 Hz | | |
| 5.01 | | Amarillo 2 Hz | | |
| 15 | | Amarillo 2 Hz | | |
| 15.01 | | Verde 2 Hz | | |
| 25 | | Verde 2 Hz | | |
| 25.01 | | Lejos: tres LEDs simultáneos a 2 Hz | | |
| 50 | | Lejos: tres LEDs simultáneos a 2 Hz | | |
| 100 | | Lejos: tres LEDs simultáneos a 2 Hz | | |

Evidencia: fotos o un video corto por rango. Los valores centesimales (5.01, 15.01, 25.01) se verifican con entradas controladas en las pruebas del servidor, porque el HC-SR04 no tiene esa resolución.

## Lectura inválida (PV-04)

1. Apuntar el sensor al vacío (más de 4 m) o tapar el emisor.
2. Esperado: la consola del sensor muestra `POST distance=invalid` y los tres LEDs se apagan.

| Situación | Consola del sensor | LEDs | ¿Cumple? |
|---|---|---|---|
| Sin objeto / sensor tapado | | | |

## Frecuencia del parpadeo (PV-05)

Grabar 10 s de cada modo y contar los encendidos.

| Modo | Encendidos contados en 10 s | Frecuencia medida (Hz) | Esperado | ¿Cumple? |
|---|---:|---:|---|---|
| Rojo `blink_4` | | | 4 Hz (40 encendidos) | |
| Amarillo `blink_2` | | | 2 Hz (20 encendidos) | |
| Verde `blink_2` | | | 2 Hz (20 encendidos) | |
| Lejos: tres `blink_2` | | | 2 Hz por LED (20 encendidos), en fase | |

Verificar la entrada en Lejos desde rojo, amarillo y verde; mantener comandos repetidos y comprobar que los tres LEDs alternan juntos. Luego provocar una lectura inválida y comprobar el apagado.

## Desconexión y recuperación (PV-13 a PV-15)

| Prueba | Esperado | Observado | ¿Cumple? |
|---|---|---|---|
| Desconectar el USB del sensor desde Lejos | El actuador apaga en ≤ 2 s. | | |
| Volver a conectar el sensor | Se registra solo y los LEDs vuelven a responder. | | |
| Detener el servidor (`Ctrl+C`) y volver a iniciarlo | Ambas placas reintentan cada 2 s y se registran de nuevo. | | |
| Apagar el WiFi (hotspot) unos segundos y encenderlo | Los LEDs se apagan; las placas se reconectan y el sistema continúa. | | |
| Reiniciar el actuador (botón EN) | Arranca apagado y recibe el estado actual al registrarse. | | |

## PN-01 — Estabilidad (RNF1: ≥ 10 min sin reinicios ni bloqueos)

1. Iniciar el servidor con `--registro docs/evidencias/estabilidad.log` y abrir también las consolas serie de ambos ESP32.
2. Anotar la hora de inicio. Dejar el sistema funcionando al menos 12 minutos, moviendo el objeto entre rangos de vez en cuando.
3. Observar la continuidad de adquisición, comunicación y actuación; anotar con su hora cualquier reinicio, bloqueo, desconexión o recuperación del sensor, del actuador o del servidor.
4. Detener y ejecutar `tools.analizar_registro` sobre el archivo de la PC, y contrastar ese resumen con las consolas de las placas.

Criterio de aceptación: operación durante ≥ 600 s con cero reinicios y cero bloqueos en ambos ESP32 y el servidor, manteniendo el recorrido y la actuación. Los registros repetidos, cierres, `ERROR` y el mayor intervalo entre `POST` del resumen ayudan a detectar incidencias.

| Duración (s) | POST | Mayor intervalo (s) | Registros sensor / actuador | Reinicios sensor / actuador / PC | Bloqueos y continuidad de los tres equipos | Cierres / ERROR | ¿Cumple? |
|---:|---:|---:|---|---|---|---|---|
| | | | | | | | |

## PN-02 — Exactitud (RNF2: error absoluto máximo ≤ 3 cm entre 3 y 150 cm)

Tomar al menos tres lecturas individuales por posición de referencia, de la consola del sensor. Para cada lectura válida, calcular `error absoluto = |lectura − referencia|`. Evaluar el máximo individual por posición y el máximo global, no solo los promedios. Las lecturas inválidas se anotan aparte y no cuentan como error cero.

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

Error absoluto máximo individual de todas las posiciones: ______ cm. Criterio: ≤ 3 cm en el intervalo de 3 a 150 cm. Si el error crece proporcionalmente con la distancia, una causa probable es la temperatura: `0.0343 cm/µs` supone unos 20 °C.

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

Tiempo máximo observado: ______ s. Criterio: ≤ 1 s.

## PN-04 — Frecuencia de muestreo (RNF4: adquisición ≥ 2 lecturas/s)

1. Definir una ventana de duración conocida y anotar su inicio y fin.
2. Contar las lecturas que muestra la consola serie del sensor en esa ventana, separando las inválidas. Contar también los `POST` recibidos por la PC en la misma ventana.
3. Calcular `frecuencia de adquisición = lecturas / duración` y, por separado, `frecuencia de recepción = POST recibidos / duración`. Si difieren, anotar pérdidas o reconexiones.
4. `tools.analizar_registro` calcula la frecuencia de recepción a partir del registro de la PC.

| Duración (s) | Adquisiciones sensor | Inválidas sensor | POST recibidos | POST invalid | Frecuencia adquisición (lecturas/s) | Frecuencia recepción (POST/s) | ¿Adquisición ≥ 2/s? |
|---:|---:|---:|---:|---:|---:|---:|---|
| | | | | | | | |

El valor configurado es de 4 lecturas/s (una cada 250 ms) mientras el sensor está registrado y puede enviar.

## Evidencias

Guardar en `docs/evidencias/` las fotos de los rangos, el video del parpadeo, los registros del servidor (`--registro`) y las capturas de las consolas. Los videos pesados pueden subirse a un enlace externo y citarse en el informe; GitHub rechaza archivos de más de 100 MB.
