# Evidencias de la Práctica 2

Esta carpeta reúne las evidencias físicas y los registros utilizados para validar los requerimientos funcionales y no funcionales de la Práctica 2: **Integración de Objetos Inteligentes con TCP/IP**, SIS-234, Grupo 8.

## Índice de evidencias

| Archivo | Contenido | Requisitos o pruebas que respalda | Enlace |
|---|---|---|---|
| `Registro de Pruebas RF y RFN.pdf` | Seis páginas con fotografías, mediciones, capturas TCP, reconexión y resultados no funcionales. | RF1–RF4; PN-01–PN-04 (RNF1–RNF4), con el alcance indicado en el informe. | [Abrir PDF](<Registro de Pruebas RF y RFN.pdf>) |
| `leds.mp4` | Video del funcionamiento de los LEDs y observación del parpadeo. | RF3; PF-04/PF-05 del expediente del informe y observación complementaria de PV-05. | [Ver video](leds.mp4) |
| `sensor.jpeg` | Fotografía del montaje físico del ESP32 sensor, HC-SR04 y divisor de tensión de ECHO. | Montaje del objeto sensor; RF1. | [Ver fotografía](sensor.jpeg) |
| `actuador.jpeg` | Fotografía del montaje físico del ESP32 actuador y los tres LEDs. | Montaje del objeto actuador; RF3. | [Ver fotografía](actuador.jpeg) |

## Descripción y alcance

El PDF documenta las referencias de distancia con regla, las lecturas del monitor serie, el recorrido de comunicación TCP y la reconexión con un rechazo temporal `role_busy` seguido de registro aceptado. También contiene los resultados de estabilidad, exactitud, tiempo de respuesta y frecuencia de lecturas.

El video complementa la observación de los LEDs según los rangos de distancia. El equipo señala que el rojo parpadea visualmente más rápido que los demás. Las frecuencias **nominales configuradas** son 4 Hz para el rojo y 2 Hz para amarillo, verde y el estado Lejos; la observación visual no constituye una medición exacta de esas frecuencias.

Las fotografías independientes muestran los dos objetos del sistema. Documentan el montaje, pero una imagen estática no demuestra la frecuencia de parpadeo ni el tiempo de respuesta.

## Relación con el informe

Los procedimientos, resultados, límites de validación y conclusiones están desarrollados en [INFORME.md](../../INFORME.md), especialmente en las secciones 4, 5 y 8. La numeración PF corresponde al expediente definido allí; PV y PN identifican los casos de verificación y los ensayos no funcionales.
