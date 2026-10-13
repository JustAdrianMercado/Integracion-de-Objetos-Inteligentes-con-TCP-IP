# Integración de objetos inteligentes con TCP/IP — Grupo 8

Práctica 2 de SIS-234: dos ESP32 (sensor y actuador) con firmware C++ y un servidor TCP en Python 3.13 que clasifica las distancias y ordena los estados de los LEDs.

```text
ESP32 sensor --POST distance=45.00--> PC servidor --SET redLed=blink_4 ...--> ESP32 actuador
(HC-SR04)                             (reglas de rango)                      (3 LEDs)
```

El sensor publica una medición cada 250 ms. El servidor la clasifica (rojo 4 Hz por debajo de 50 cm, amarillo 2 Hz hasta 100 cm, verde fijo desde 100 cm) y envía el comando al actuador, que lo aplica a los tres LEDs. Queda la integración física con ambas placas (etapa 7).

## Estructura

```text
apps/
  firmware/                 Programas de los dos ESP32 (PlatformIO)
    src/                    PrincipalSensor, PrincipalActuador, ClienteTcp,
                            SensorUltrasonico, ControladorLeds
    include/                Encabezados y ejemplo de configuración de red
    lib/Compartido/         Tipos, protocolo y receptor de líneas comunes a ambos ESP32
  servidor/                 Servidor Python: __main__, servidor, protocolo, dominio
tools/                      Simuladores sin placas y analizar_registro.py (evidencias)
tests/
  native/                   Pruebas C++ del firmware con WiFi, reloj y GPIO simulados
  python/                   Pruebas del protocolo, el servidor y los simuladores
docs/                       Diseño, diagramas, plan, consigna y evidencias
platformio.ini              Entornos sensor y actuador
CMakeLists.txt              Pruebas en la PC (CTest)
INFORME.md                  Informe académico, trabajado por separado
```

## Ejecutar

Servidor, desde la raíz (`Ctrl+C` lo detiene):

```powershell
py -3.13 -m apps.servidor --ip 0.0.0.0 --puerto 5000
```

Al escuchar en `0.0.0.0` muestra las IPv4 de la PC que los ESP32 deben usar como `IP_SERVIDOR`. La [guía del servidor](apps/servidor/README.md) describe roles, plazos y mensajes de error.

Firmware: copiar `apps/firmware/include/ConfiguracionRed.example.h` a `ConfiguracionRed.h`, completar SSID, contraseña e IPv4 de la PC, y compilar cada placa:

```powershell
pio run -e sensor -t upload --upload-port COMx
pio run -e actuador -t upload --upload-port COMy
```

La [guía del firmware](apps/firmware/README.md) explica configuración, carga y pines. Para el montaje completo seguir la [guía de montaje](docs/GUIA_MONTAJE.md) y registrar los resultados con las [pruebas físicas](docs/PRUEBAS_FISICAS.md).

Para probar una placa sin la otra:

```powershell
py -3.13 -m tools.simular_actuador --ip 127.0.0.1 --puerto 5000
py -3.13 -m tools.simular_sensor --ip 127.0.0.1 --puerto 5000 --distancias 45,75,120,invalid
```

## Pruebas

```powershell
py -3.13 -m unittest discover -s tests/python -v
```

Pruebas C++ y Python juntas, desde una terminal de desarrollo de Visual Studio (o con g++ cambiando el generador):

```powershell
cmake -S . -B build/windows -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build/windows
ctest --test-dir build/windows --output-on-failure
```

## Entorno

| Componente | Versión |
|---|---|
| Placas | ESP32 DEVKIT V1 de 30 pines (sensor) y ESP32 genérico de 38 pines (actuador). |
| Firmware | C++ con Arduino-ESP32 2.0.17, PlatformIO Core 6.2.0, `espressif32@7.0.1`. |
| Servidor | Python 3.13, solo biblioteca estándar. |
| Pruebas | `unittest`; CMake + CTest con MSVC o g++. |

## Etapas

1. Estados y reglas de distancia.
2. Organización del repositorio.
3. Protocolo de aplicación.
4. Servidor TCP (Python).
5. Cliente ESP32 sensor.
6. Cliente ESP32 actuador, más la auditoría y la simplificación del código.
7. **En curso:** montaje, integración física y pruebas con ambas placas ([guía](docs/GUIA_MONTAJE.md) y [pruebas](docs/PRUEBAS_FISICAS.md)).

El [plan de implementación](docs/PLAN_IMPLEMENTACION.md) detalla cada entrega y el [diseño del sistema](docs/diagramas/Diseno_del_sistema.md) contiene el protocolo y los diagramas. `INFORME.md` y la documentación histórica quedan a cargo del compañero que redacta el informe.
