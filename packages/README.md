# Bibliotecas compartidas

Cada biblioteca reutilizable se guarda en su propia carpeta dentro de `packages/`.

`Compartido/src/` contiene los estados de los LEDs, los roles de los clientes, las reglas de control de distancia y el protocolo de aplicación. Su código no depende de Arduino ni de Winsock.

La [guía de Compartido](Compartido/README.md) explica el uso del protocolo y la recepción de líneas.

El CMake de la raíz incorpora `Compartido` para compilarlo y probarlo en la PC. PlatformIO utiliza este directorio mediante `lib_dir = packages` y detecta las bibliotecas que el firmware incluye.
