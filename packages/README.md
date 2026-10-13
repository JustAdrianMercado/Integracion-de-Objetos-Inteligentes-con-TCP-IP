# Bibliotecas compartidas

Cada biblioteca reutilizable se guarda en su propia carpeta dentro de `packages/`.

`Compartido/src/` contiene los estados de los LEDs, los roles de los clientes y las reglas de control de distancia. Su código no depende de Arduino ni de Winsock.

El CMake de la raíz incorpora `Compartido` para compilarlo y probarlo en la PC. PlatformIO utiliza este directorio mediante `lib_dir = packages` y detecta las bibliotecas que el firmware incluye.
