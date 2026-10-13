# Bibliotecas compartidas

Cada biblioteca reutilizable se guarda en su propia carpeta dentro de `packages/`.

`Compartido/src/` contiene los estados, los roles y el protocolo de aplicación C++ de los clientes ESP32. La versión C++ de las reglas de distancia se conserva como referencia de prueba. El servidor Python ejecuta las reglas de `apps/servidor/dominio.py`; las pruebas de contrato verifican la compatibilidad entre lenguajes.

La [guía de Compartido](Compartido/README.md) explica el uso del protocolo y la recepción de líneas.

El CMake de la raíz incorpora `Compartido` para compilarlo y probarlo en la PC. PlatformIO utiliza este directorio mediante `lib_dir = packages` y detecta las bibliotecas que el firmware incluye.
