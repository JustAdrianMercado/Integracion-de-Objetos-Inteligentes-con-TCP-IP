# Integración de objetos inteligentes con TCP/IP — Grupo 8

Monorepo de la Práctica 2 de SIS-234: dos clientes ESP32 con firmware C++ y un servidor TCP en Python 3.13 que clasifica las distancias y ordena los estados de los LEDs.

El servidor Python está implementado y se prueba con clientes simulados en la PC. El firmware actual conserva la base local de la Práctica 1; los programas independientes del sensor y del actuador se implementarán en las etapas 5 y 6.

## Estructura

```text
apps/
  firmware/
    include/              Declaraciones de hardware; configuración de red en etapa 5
    src/                  Base ESP32 actual; cliente/ se incorporará en etapa 5
  servidor/
    __main__.py           Entrada de consola con argparse y logging
    servidor.py           socketserver, sesiones, roles y colas de envío
    dominio.py            Estados y reglas de distancia ejecutados en la PC
    protocolo.py          Mensajes ASCII y receptor de líneas
    README.md             Uso y funcionamiento del servidor
packages/
  Compartido/             Tipos y protocolo C++ reutilizables por ambos ESP32
                          ControlDistancia C++ como referencia de las pruebas
tests/
  python/                 unittest: dominio, protocolo, concurrencia y TCP real
  native/                 C++: dominio, protocolo y sensor con GPIO simulados
  contratos/              Vectores para comprobar C++ y Python juntos
  firmware/               Pruebas de PlatformIO
docs/
  PLAN_IMPLEMENTACION.md  Revisión de etapas y especificación de la etapa 5
  diagramas/              Arquitectura, circuitos y contrato de aplicación
  consigna/               Enunciado del docente
CMakeLists.txt            Construcción y pruebas nativas C++
platformio.ini            Construcción del firmware ESP32
.python-version          Rama de Python utilizada: 3.13
INFORME.md                Informe académico, trabajado por separado
```

El [plan de implementación](docs/PLAN_IMPLEMENTACION.md) contiene el alcance de cada entrega. Cada etapa se desarrolla en su propia rama y queda sin commit ni push para su revisión y publicación manual.

## Entorno

| Componente | Selección |
|---|---|
| Placas | Dos ESP32 clásicos, 240 MHz, 320 KB de RAM y 4 MB de flash. |
| Firmware | C++ con Arduino-ESP32 2.0.17; `framework-arduinoespressif32@3.20017.0`. |
| Construcción ESP32 | PlatformIO Core 6.2.0; `espressif32@7.0.1`. |
| Servidor | Python 3.13, biblioteca estándar. |
| Pruebas Python | `unittest`, sockets TCP locales y puertos libres. |
| Pruebas C++ | CMake y CTest; g++ o MSVC, con GPIO simulados para el sensor. |

En esta PC las pruebas nativas se validan con MSVC, CMake y Ninja ya instalados. El servidor se ejecuta directamente con Python. Su uso no requiere compilar C++ ni instalar paquetes con pip.

## Ejecutar el servidor

Desde la raíz del repositorio, para pruebas en la misma PC:

```powershell
py -3.13 -m apps.servidor --ip 127.0.0.1 --puerto 5000
```

Para la red WiFi de los ESP32:

```powershell
py -3.13 -m apps.servidor --ip 0.0.0.0 --puerto 5000
```

Los clientes configurarán la IPv4 real de la PC como destino. `Ctrl+C` detiene el servidor. La [guía del servidor](apps/servidor/README.md) describe los roles, plazos, colas y opciones. En sistemas con Python 3.13 disponible como `python3`, se usa `python3 -m apps.servidor`.

## Compilar la base ESP32

```powershell
pio run -e esp32doit-devkit-v1
```

Este entorno mantiene el programa local de referencia a 115200 baudios. Su filtro incluye únicamente sus tres archivos actuales. Los entornos `sensor` y `actuador` se habilitarán cuando existan sus programas principales, en las etapas 5 y 6.

## Pruebas

Pruebas Python, desde la raíz:

```powershell
py -3.13 -m unittest discover -s tests/python -v
```

Pruebas C++ y Python juntas, desde una terminal de desarrollo de Visual Studio:

```powershell
cmake -S . -B build/windows -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build/windows
ctest --test-dir build/windows --output-on-failure
```

Con g++ y Ninja disponibles se puede usar otra carpeta de construcción:

```sh
cmake -S . -B build/native -G Ninja -DCMAKE_CXX_COMPILER=g++ -DCMAKE_BUILD_TYPE=Debug
cmake --build build/native
ctest --test-dir build/native --output-on-failure
```

CMake incorpora `unittest` cuando encuentra Python 3.13. La prueba de contrato ejecuta el verificador C++ con los mismos 127 vectores que Python. Si se ejecuta `unittest` sin haber construido ese verificador, solo se omite esa comparación entre lenguajes. Las pruebas TCP usan `127.0.0.1` y puertos libres; verifican los plazos reales de 2 segundos y duran aproximadamente 16 segundos.

## Etapas

1. **Conservada:** estados y reglas C++ de referencia, con pruebas nativas.
2. **Conservada:** organización como monorepo.
3. **Conservada y ampliada:** protocolo C++ y contraparte Python, con pruebas de compatibilidad.
4. **Migrada para revisión:** servidor Python que sustituye la implementación Winsock.
5. **Pendiente:** cliente ESP32 sensor, adaptador WiFi/TCP y entorno `sensor`.
6. **Pendiente:** cliente ESP32 actuador, estados y parpadeos, entorno `actuador`.
7. **Pendiente:** integración física, cableado, configuración de la PC y pruebas con ambas placas.

La entrega actual se revisa en `fix/reorganize-architecture`, con el commit propuesto `fix: reorganize architecture`. La etapa 5 está [especificada en el plan](docs/PLAN_IMPLEMENTACION.md#etapa-5--cliente-esp32-sensor) para implementarla después de revisar y subir esta entrega.

El [diseño del sistema](docs/diagramas/Diseno_del_sistema.md) conserva el contrato de mensajes. `INFORME.md` y la documentación histórica quedan a cargo del compañero que redacta el informe.
