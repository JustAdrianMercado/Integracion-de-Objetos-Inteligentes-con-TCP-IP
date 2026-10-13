# Integración de objetos inteligentes con TCP/IP — Grupo 8

Proyecto de la Práctica 2 de SIS-234, desarrollado por etapas en un monorepo. El sistema previsto conecta un ESP32 con un sensor HC-SR04, un servidor C++ en Windows y un ESP32 que controla tres LEDs.

La base de firmware conserva el funcionamiento local de la Práctica 1: un ESP32 mide distancias y controla los LEDs. La biblioteca `Compartido` ya contiene los estados y las reglas de distancia para la Práctica 2, verificados con 26 casos en la PC. Su integración con los clientes y el servidor TCP corresponde a las siguientes etapas.

## Estructura del repositorio

```text
apps/
  firmware/
    include/              Declaraciones del firmware ESP32
    src/                  Implementación y programa principal
packages/
  Compartido/
    src/                  Estados, roles y control de distancia en C++
    CMakeLists.txt        Compilación de la biblioteca para PC
tests/
  native/                 Pruebas en PC mediante CMake y CTest
  firmware/               Directorio reservado para pruebas de PlatformIO
docs/
  consigna/               Enunciado de la práctica
  diagramas/              Diseño y especificación del protocolo
  evidencias/             Evidencias de pruebas y validaciones
CMakeLists.txt            Configuración de C++ para PC
platformio.ini            Configuración de los ESP32
INFORME.md                Informe académico
```

El servidor se incorporará en `apps/servidor` durante su etapa de implementación. Las configuraciones de CMake y PlatformIO se mantienen en la raíz, desde donde se ejecutan los comandos.

`packages/Compartido` reúne el código reutilizable por la PC y los ESP32. La lógica actual no depende de Arduino ni de Winsock. Las pruebas nativas y las de firmware tienen directorios distintos para que cada herramienta compile las que le corresponden.

## Herramientas necesarias

- PlatformIO Core, o VS Code con la extensión PlatformIO IDE, para compilar y cargar el firmware Arduino.
- Visual Studio con la carga de trabajo **Desarrollo para el escritorio con C++**, incluyendo MSVC, CMake, Ninja y el SDK de Windows, para compilar y probar el código de PC.

## Compilar el firmware

Desde la raíz del repositorio, en una terminal de PlatformIO:

```powershell
pio run -e esp32doit-devkit-v1
```

El entorno sigue configurado para `esp32doit-devkit-v1` y el monitor serie utiliza 115200 baudios. Los entornos separados para sensor y actuador se incorporarán al implementar cada cliente.

## Ejecutar las pruebas de PC

Abrir **Developer PowerShell for Visual Studio** o **x64 Native Tools Command Prompt for Visual Studio**, situarse en la raíz del repositorio y ejecutar:

```powershell
cmake -S . -B build/windows -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build/windows
ctest --test-dir build/windows --output-on-failure
```

CTest ejecuta `control_distancia`, cuyo programa verifica los 26 casos. La carpeta `build/` contiene archivos generados y está excluida de Git.

## Etapas de implementación

1. **Completada:** biblioteca compartida de estados y control de distancia, con pruebas nativas.
2. **Implementada para revisión:** organización como monorepo y actualización de las rutas de compilación.
3. **Pendiente:** codificación, lectura y validación del protocolo de aplicación.
4. **Pendiente:** servidor TCP para Windows con Winsock.
5. **Pendiente:** cliente ESP32 del sensor.
6. **Pendiente:** cliente ESP32 del actuador.
7. **Pendiente:** integración, guía de cableado y pruebas con la PC y ambos ESP32.

Cada etapa se revisa y se sube manualmente al repositorio antes de iniciar la siguiente.

## Documentación

El [diseño del sistema y protocolo](docs/diagramas/Diseno_del_sistema.md) guía la implementación. El [informe académico](INFORME.md) se trabaja por separado; sus referencias a rutas se actualizarán al elaborar el informe final.
