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

1. Iniciar el servidor con `--registro docs/evidencias/p2/pn01-estabilidad.log`.
2. Dejar el sistema funcionando al menos 12 minutos, moviendo el objeto entre rangos de vez en cuando.
3. Detener y ejecutar `tools.analizar_registro` sobre el archivo.

Criterio: duración ≥ 600 s, un solo registro de cada placa, 0 cierres por 2 s sin `POST`, 0 `ERROR` y mayor intervalo entre `POST` menor que 2 s.

| Duración (s) | POST | Mayor intervalo (s) | Registros sensor / actuador | Cierres / ERROR | ¿Cumple? |
|---:|---:|---:|---|---|---|
| | | | | | |

## PN-02 — Exactitud (RNF2: error ≤ ±3 cm entre 3 y 150 cm)

Tres lecturas por posición, tomadas de la consola del sensor. Error = |lectura − referencia|.

| Referencia (cm) | Lectura 1 | Lectura 2 | Lectura 3 | Error máximo (cm) | ¿Cumple? |
|---:|---:|---:|---:|---:|---|
| 5 | | | | | |
| 20 | | | | | |
| 45 | | | | | |
| 75 | | | | | |
| 100 | | | | | |
| 125 | | | | | |
| 150 | | | | | |

Error máximo de todas las posiciones: ______ cm. Si el error crece con la distancia de forma proporcional, la causa probable es la velocidad del sonido (`0.0343 cm/µs` supone unos 20 °C); mencionarlo en el análisis.

## PN-03 — Tiempo de respuesta (RNF3: ≤ 1 s de extremo a extremo)

1. Grabar en un mismo cuadro el objeto y los LEDs (60 fps si es posible).
2. Mover el objeto de forma rápida entre dos rangos y dejarlo quieto.
3. En el video, contar los cuadros entre el instante en que el objeto queda en la nueva posición y el cambio del LED. Tiempo = cuadros / fps.

| Ensayo | Transición | Cuadros | fps | Tiempo (s) | ¿≤ 1 s? |
|---:|---|---:|---:|---:|---|
| 1 | Verde → rojo | | | | |
| 2 | Rojo → verde | | | | |
| 3 | Verde → amarillo | | | | |
| 4 | Amarillo → rojo | | | | |
| 5 | Rojo → amarillo | | | | |

Tiempo máximo: ______ s. Valor esperado: menos de 0.5 s (muestreo de 250 ms más la red).

## PN-04 — Frecuencia de muestreo (RNF4: ≥ 2 lecturas/s)

Usar el resultado de `tools.analizar_registro` de PN-01 (o de una sesión de al menos 60 s).

| Duración (s) | POST recibidos | Lecturas/s | Inválidas | ¿≥ 2/s? |
|---:|---:|---:|---:|---|
| | | | | |

Valor nominal: 4 lecturas/s (una cada 250 ms).

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
