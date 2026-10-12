# Monitor de distancia con ESP32 — Grupo 8

Proyecto de la Práctica 1 de SIS-234. Un ESP32 mide distancias con un sensor HC-SR04 y comunica cuatro rangos mediante el parpadeo no bloqueante de tres LEDs.

## Hardware

- ESP32 / ESP-WROOM-32.
- Sensor ultrasónico HC-SR04.
- LEDs verde, amarillo y rojo con una resistencia de 220 Ω por LED.
- Divisor de voltaje para ECHO con tres resistencias de 2.2 kΩ.

## Tecnologías

- C++ con el entorno de trabajo Arduino.
- PlatformIO.
- Placa configurada como `esp32doit-devkit-v1`.

## Estructura del repositorio

```text
include/             Declaraciones de las clases del proyecto
src/                 Implementación y programa principal
docs/consigna/       Enunciado de la práctica
docs/diagramas/      Recursos de los diagramas
docs/evidencias/     Evidencias de pruebas y validaciones
INFORME.md           Informe técnico
platformio.ini       Configuración de PlatformIO
```

## Compilación

Con PlatformIO instalado, ejecutar desde la raíz del proyecto:

```bash
pio run -e esp32doit-devkit-v1
```

La configuración del monitor serie utiliza 115200 baudios.

## Documentación

El diseño, la implementación y el plan de validación se encuentran en el [informe técnico](INFORME.md).
