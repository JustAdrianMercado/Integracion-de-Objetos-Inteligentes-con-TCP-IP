# Integración de objetos inteligentes con TCP/IP — Grupo 8

Monorepo de la Práctica 2 de SIS-234: dos clientes ESP32 con firmware C++ y un servidor TCP en Python 3.13 que clasifica las distancias y ordena los estados de los LEDs.

El servidor Python y los dos clientes ESP32 están implementados. El sensor se registra por WiFi/TCP y publica mediciones cada 250 ms; el actuador recibe los `SET` y aplica los estados y parpadeos de los tres LEDs. Dos simuladores de consola permiten probar cada placa sin la otra. Queda la integración física con ambas placas (etapa 7). El programa local de la Práctica 1 se conserva como referencia.

## Estructura

```text
apps/
  firmware/
    include/              Hardware y adaptador TCP; configuración de red local
    src/cliente/          PrincipalSensor, PrincipalActuador, ClienteTcp,
                          SensorUltrasonico y ControladorLeds
    src/main.cpp          Programa local de referencia de la Práctica 1
                          (con ControladorLedsLocal)
    README.md             Configuración, carga y pruebas de ambos ESP32
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
  python/                 unittest: dominio, protocolo, concurrencia y flujo C++ → Python
  native/                 C++: dominio, protocolo y firmware con WiFi/GPIO simulados
  contratos/              Vectores para comprobar C++ y Python juntos
  firmware/               Pruebas de PlatformIO
docs/
  PLAN_IMPLEMENTACION.md  Estado y alcance de las entregas
  diagramas/              Arquitectura, circuitos y contrato de aplicación
  consigna/               Enunciado del docente
CMakeLists.txt            Construcción y pruebas nativas C++
platformio.ini            Construcción del firmware ESP32
tools/simular_actuador.py  Cliente de consola que muestra los SET
tools/simular_sensor.py    Cliente de consola que publica distancias
.python-version          Rama de Python utilizada: 3.13
INFORME.md                Informe académico, trabajado por separado
```

El [plan de implementación](docs/PLAN_IMPLEMENTACION.md) contiene el alcance de cada entrega. Cada etapa se desarrolla en su propia rama y se revisa antes de publicarla y fusionarla.

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

## Compilar los ESP32

```powershell
pio run -e sensor
pio run -e actuador
```

`sensor` (ESP32 DEVKIT V1 de 30 pines) es el entorno predeterminado; `actuador` usa el perfil `esp32dev` de la placa genérica de 38 pines. Copiar `apps/firmware/include/ConfiguracionRed.example.h` a `ConfiguracionRed.h` en la misma carpeta y completar SSID, contraseña e IPv4 de la PC. El archivo local está excluido de Git y ambas placas lo comparten. Sin configuración, compilan con el ejemplo vacío y solicitan completarlo por consola.

La [guía del firmware](apps/firmware/README.md) explica la configuración, la carga y la red para la demo. El programa local continúa compilando con `pio run -e esp32doit-devkit-v1`.

Para probar una placa sin la otra, desde otra terminal en la misma PC:

```powershell
py -3.13 -m tools.simular_actuador --ip 127.0.0.1 --puerto 5000
py -3.13 -m tools.simular_sensor --ip 127.0.0.1 --puerto 5000 --distancias 45,75,120,invalid
```

El actuador simulado muestra los `SET` completos; el sensor simulado publica las distancias indicadas cada 250 ms, recorriéndolas en ciclo. Ambos terminan con `Ctrl+C` y no requieren paquetes externos. Cerrar el simulador de un rol antes de conectar la placa real de ese rol.

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

CMake incorpora `unittest` cuando encuentra Python 3.13. La prueba de contrato ejecuta el verificador C++ con los mismos 127 vectores que Python. Dos puentes enlazan el firmware C++ de cada ESP32 con el servidor Python mediante TCP real, simulando WiFi, reloj y GPIO. CTest proporciona los tres ejecutables automáticamente. En `unittest` por separado se omiten la comparación y los flujos del firmware si no están configurados `IOT_CONTRACT_EXE`, `IOT_SENSOR_EXE` e `IOT_ACTUATOR_EXE`. Las pruebas TCP usan `127.0.0.1`, puertos libres y plazos reales de 2 segundos.

## Etapas

1. **Conservada:** estados y reglas C++ de referencia, con pruebas nativas.
2. **Conservada:** organización como monorepo.
3. **Conservada y ampliada:** protocolo C++ y contraparte Python, con pruebas de compatibilidad.
4. **Implementada y fusionada:** servidor Python que sustituye la implementación Winsock.
5. **Implementada y fusionada:** cliente ESP32 sensor, adaptador WiFi/TCP, entorno `sensor` y actuador simulado.
6. **Implementada para revisión:** cliente ESP32 actuador, estados y parpadeos, entorno `actuador`, sensor simulado y correcciones de la auditoría.
7. **Pendiente:** integración física, cableado, configuración de la PC y pruebas con ambas placas.

La entrega actual se revisa en `ft/actuator-client`. La [etapa 6 del plan](docs/PLAN_IMPLEMENTACION.md#etapa-6--cliente-esp32-actuador) distingue las comprobaciones automatizadas de la validación pendiente en las placas reales.

El [diseño del sistema](docs/diagramas/Diseno_del_sistema.md) conserva el contrato de mensajes. `INFORME.md` y la documentación histórica quedan a cargo del compañero que redacta el informe.
