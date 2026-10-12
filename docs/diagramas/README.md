# Diagramas del proyecto

## Práctica 2 — Diseño del sistema

El documento [`Diseno_del_sistema.md`](Diseno_del_sistema.md) contiene nuestro diseño: el diagrama de arquitectura, el diagrama de clases, las secuencias de mensajes y la especificación del protocolo de aplicación. Define dos clientes ESP32 y un servidor TCP en C++ para Windows.

Los circuitos de los objetos inteligentes están disponibles por separado:

- [`circuito-sensor.svg`](circuito-sensor.svg): ESP32, HC-SR04 y divisor de voltaje de ECHO.
- [`circuito-actuador.svg`](circuito-actuador.svg): ESP32 y tres LEDs con sus resistencias.

Los diagramas Mermaid se mantienen dentro del documento Markdown. Los esquemas SVG pueden abrirse en un navegador y utilizarse como imágenes en el informe. El [enunciado original del docente](../consigna/Practica_2.md) se conserva en `docs/consigna`; este documento describe el diseño previo a la implementación.

## Práctica 1 — Referencia

Los diagramas originales de arquitectura, circuito, estructura y comportamiento se conservan en la sección **Análisis y Diseño** de [`INFORME.md`](../../INFORME.md).

El esquema del montaje original, que reúne el sensor y los LEDs en un solo ESP32, permanece en [`circuito.svg`](circuito.svg).
