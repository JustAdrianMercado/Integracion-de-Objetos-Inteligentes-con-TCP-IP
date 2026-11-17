# Evidencias de la Práctica 2

Esta carpeta reúne las evidencias físicas y los registros con los que validamos los requerimientos funcionales y no funcionales de la Práctica 2: **Integración de Objetos Inteligentes con TCP/IP**, SIS-234, Grupo 8.

## Índice de evidencias

| Archivo | Contenido | Requisitos o pruebas que respalda | Enlace |
|---|---|---|---|
| `Registro de Pruebas RF y RFN.pdf` | Seis páginas con fotografías, mediciones, capturas TCP, reconexión y resultados no funcionales. | RF1–RF4; PN-01–PN-04 (RNF1–RNF4). | [Abrir PDF](<Registro de Pruebas RF y RFN.pdf>) |
| `leds.mp4` | Video del funcionamiento de los LEDs y del parpadeo. | RF3; PV-05. | [Ver video](leds.mp4) |
| `sensor.jpeg` | Fotografía del montaje del ESP32 sensor, el HC-SR04 y el divisor de tensión de ECHO. | Montaje del objeto sensor; RF1. | [Ver fotografía](sensor.jpeg) |
| `actuador.jpeg` | Fotografía del montaje del ESP32 actuador y los tres LEDs. | Montaje del objeto actuador; RF3. | [Ver fotografía](actuador.jpeg) |
| `pruebas/python.txt` | Salida completa de las pruebas Python (`unittest`), con fecha y commit. | RF2, RF4, PV-01–PV-16; RNF5. | [Ver salida](pruebas/python.txt) |
| `pruebas/ctest.txt` | Salida completa de CTest en GitHub Actions: seis suites C++ y la suite Python, con fecha y commit. | RF1–RF4, PV-01–PV-16; RNF5. | [Ver salida](pruebas/ctest.txt) |

## Descripción

El PDF documenta las referencias de distancia con regla, las lecturas del monitor serie, el recorrido de comunicación TCP y la reconexión con un rechazo temporal `role_busy` seguido de registro aceptado. También contiene los resultados de estabilidad, exactitud, tiempo de respuesta y frecuencia de lecturas.

El video muestra los LEDs según los rangos de distancia; el rojo parpadea visiblemente más rápido que los demás. Las frecuencias configuradas son 4 Hz para el rojo y 2 Hz para amarillo, verde y el estado Lejos, y se verifican con las pruebas de reloj simulado.

Las fotografías muestran los dos objetos del sistema. Las salidas de `pruebas/` muestran la ejecución de las pruebas automatizadas sobre el código final.

## Relación con el informe

Los procedimientos, resultados y conclusiones están en [INFORME.md](../../INFORME.md), secciones 4, 5 y 8. Los códigos PF identifican los bloques del registro físico; PV y PN, los casos de verificación y los ensayos no funcionales.
