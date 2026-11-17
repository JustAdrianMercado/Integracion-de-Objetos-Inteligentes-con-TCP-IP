# Diagramas del proyecto

El documento [`Diseno_del_sistema.md`](Diseno_del_sistema.md) contiene nuestro diseño: el diagrama de arquitectura, el diagrama de clases, las secuencias de mensajes y la especificación del protocolo de aplicación. Define dos clientes ESP32 con firmware C++ y un servidor TCP Python.

Los circuitos de los objetos inteligentes están disponibles por separado:

- [`circuito-sensor.svg`](circuito-sensor.svg): ESP32, HC-SR04 y divisor de voltaje de ECHO.
- [`circuito-actuador.svg`](circuito-actuador.svg): ESP32 y tres LEDs con sus resistencias.

Los diagramas Mermaid se mantienen dentro del documento Markdown. Los esquemas SVG pueden abrirse en un navegador y se usan como imágenes en el informe. El [enunciado del docente](../consigna/Práctica_2.md) está en `docs/consigna`.
