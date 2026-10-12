# Evidencias de pruebas y validaciones

Esta carpeta contiene las evidencias reales utilizadas para respaldar las pruebas documentadas en [`INFORME.md`](../../INFORME.md). Incluye el registro de las mediciones y fotografías del montaje y de los actuadores en funcionamiento.

## Evidencias disponibles

| Archivo | Tipo de evidencia | Qué demuestra | Sección relacionada del informe |
|---|---|---|---|
| [`Pruebas de rangos.pdf`](<Pruebas de rangos.pdf>) | PDF de pruebas | Registra las pruebas de clasificación de rangos, las mediciones de exactitud y la respuesta ante una lectura inválida; además, contiene los timestamps utilizados como evidencia de operación continua. | 4.2 Validación funcional, 4.3 Validación de exactitud y 4.4 Validación de estabilidad |
| [`prototipo.jpeg`](prototipo.jpeg) | Fotografía general | Muestra el montaje físico del prototipo con el ESP32, el HC-SR04 y los actuadores. | 2.3 Diseño del circuito y 4.2 Validación funcional |
| [`led-rojo.jpeg`](led-rojo.jpeg) | Fotografía del LED rojo | Respalda visualmente la activación del LED rojo para su rango correspondiente. | 4.2 Validación funcional |
| [`led-amarillo.jpeg`](led-amarillo.jpeg) | Fotografía del LED amarillo | Respalda visualmente la activación del LED amarillo para su rango correspondiente. | 4.2 Validación funcional |
| [`led-verde.jpeg`](led-verde.jpeg) | Fotografía del LED verde | Respalda visualmente la activación del LED verde para su rango correspondiente. | 4.2 Validación funcional |
| [`tres-colores-leds.jpeg`](tres-colores-leds.jpeg) | Fotografía de los tres LEDs | Respalda visualmente la activación simultánea de los tres LEDs para el rango `Lejos`. | 4.2 Validación funcional |

## Relación con las pruebas

- Las mediciones numéricas incluidas en `Pruebas de rangos.pdf` respaldan la prueba de exactitud y la clasificación de distancias en los rangos definidos.
- Las fotografías respaldan visualmente el montaje físico y la activación de los actuadores en los rangos rojo, amarillo, verde y lejos. El comportamiento de parpadeo se valida mediante la observación realizada durante la prueba funcional.
- Los timestamps registrados durante las pruebas respaldan el intervalo de operación continua empleado en la validación de estabilidad.
- El tiempo de respuesta y la frecuencia de muestreo fueron validados lógicamente a partir del diseño del firmware. Por ello no requieren video ni evidencia física adicional en este informe.

## Conservación de evidencias

- Los archivos corresponden a pruebas reales realizadas por el Grupo 8.
- Los valores observados en las evidencias no deben modificarse.
- Cualquier prueba futura debe agregar nuevas evidencias sin reemplazar los archivos existentes.
