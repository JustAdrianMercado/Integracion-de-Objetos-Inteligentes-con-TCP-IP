# Integración de objetos inteligentes con TCP/IP — Grupo 8

Proyecto de la Práctica 2 de SIS-234, desarrollado por etapas en un monorepo. El sistema previsto conecta un ESP32 con un sensor HC-SR04, un servidor C++ en Windows y un ESP32 que controla tres LEDs.

La base de firmware conserva el funcionamiento local de la Práctica 1: un ESP32 mide distancias y controla los LEDs. La biblioteca `Compartido` contiene los estados, las reglas de distancia y el protocolo de aplicación. El servidor TCP para Windows ya atiende ambos roles y se prueba con clientes simulados en la PC; los clientes ESP32 se integrarán en las siguientes etapas.

## Estructura del repositorio

```text
apps/
  firmware/
    include/              Declaraciones del firmware ESP32
    src/                  Implementación y programa principal
  servidor/
    include/              Servidor TCP y cola de envíos
    src/                  Winsock y programa de consola para Windows
    README.md             Compilación, ejecución y pruebas del servidor
packages/
  Compartido/
    src/                  Estados, roles, control de distancia y protocolo en C++
    README.md             Uso de la biblioteca y recepción de mensajes
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

Las configuraciones de CMake y PlatformIO se mantienen en la raíz, desde donde se ejecutan los comandos.

`packages/Compartido` reúne el código reutilizable por la PC y los ESP32. La lógica actual no depende de Arduino ni de Winsock. Las pruebas nativas y las de firmware tienen directorios distintos para que cada herramienta compile las que le corresponden.

La [guía de la biblioteca compartida](packages/Compartido/README.md) describe cómo crear e interpretar los mensajes y cómo conservar los fragmentos recibidos mediante `ReceptorLineas`.

## Compilar y ejecutar el servidor

Desde una terminal de desarrollo de Visual Studio, en la raíz del repositorio:

```powershell
cmake -S . -B build/windows -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build/windows
.\build\windows\apps\servidor\servidor_tcp.exe --ip 127.0.0.1 --puerto 5000
```

Para la futura conexión con los ESP32, el servidor puede ejecutarse con `--ip 0.0.0.0`; los clientes usarán la IP real de la PC. La [guía del servidor](apps/servidor/README.md) detalla sus opciones y su funcionamiento.

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

CTest ejecuta `control_distancia` (26 casos), `protocolo_aplicacion` y `receptor_lineas`; en Windows también ejecuta `cola_envio` y `servidor_tcp`. Estas pruebas cubren los mensajes, la validación de datos y buffers, los envíos parciales y los escenarios TCP con clientes simulados en la propia PC. La carpeta `build/` contiene archivos generados y está excluida de Git.

## Etapas de implementación

1. **Completada:** biblioteca compartida de estados y control de distancia, con pruebas nativas.
2. **Completada:** organización como monorepo y actualización de las rutas de compilación.
3. **Completada:** codificación, lectura y validación del protocolo de aplicación, con recepción de líneas fragmentadas.
4. **Implementada para revisión:** servidor TCP para Windows con Winsock, registro, recuperación y pruebas locales.
5. **Pendiente:** cliente ESP32 del sensor.
6. **Pendiente:** cliente ESP32 del actuador.
7. **Pendiente:** integración, guía de cableado y pruebas con la PC y ambos ESP32.

A partir de la etapa 3, cada etapa se implementa en una rama propia. Los cambios quedan sin commit ni push para su revisión; se confirman y se suben manualmente antes de iniciar la siguiente etapa. La rama de la etapa 4 es `ft/tcp-server`.

## Documentación

El [diseño del sistema y protocolo](docs/diagramas/Diseno_del_sistema.md) guía la implementación. El [informe académico](INFORME.md) se trabaja por separado; sus referencias a rutas se actualizarán al elaborar el informe final.
