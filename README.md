# Integración de objetos inteligentes con TCP/IP — Grupo 8

Práctica 2 de SIS-234: dos ESP32 (sensor y actuador) con firmware C++ y un servidor TCP en Python 3.13 que clasifica las distancias y ordena los estados de los LEDs.

```text
ESP32 sensor --POST distance=5.00--> PC servidor --SET redLed=blink_4 ...--> ESP32 actuador
(HC-SR04)                             (reglas de rango)                      (3 LEDs)
```

El sensor publica una medición cada 250 ms. El servidor la clasifica (rojo 4 Hz para 0 < d <= 5 cm, amarillo 2 Hz para 5 < d <= 15 cm, verde 2 Hz para 15 < d <= 25 cm y Lejos con los tres LEDs sincronizados a 2 Hz para d > 25 cm) y envía el comando al actuador, que lo aplica a los tres LEDs. Una lectura inválida o la ausencia del sensor apaga los tres LEDs.

Las mediciones, fotografías y el video del sistema funcionando están en [docs/evidencias](docs/evidencias/README.md), y el análisis completo en el [informe](INFORME.md).

Montaje: sensor con Micro-USB, TRIG en GPIO23 (D23) y ECHO en GPIO18 (D18) mediante tres resistencias de 2.2 kΩ; actuador con USB-C y LEDs verde/amarillo/rojo en GPIO25/26/27.

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
docs/                       Diseño, diagramas, consigna, guías y evidencias
platformio.ini              Entornos sensor y actuador
CMakeLists.txt              Pruebas en la PC (CTest)
INFORME.md                  Informe técnico de la práctica
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
py -3.13 -m tools.simular_sensor --ip 127.0.0.1 --puerto 5000 --distancias 5,15,25,50,invalid
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

## Documentación

- [Diseño del sistema](docs/diagramas/Diseno_del_sistema.md): arquitectura, clases, secuencias y protocolo.
- [Guía de montaje](docs/GUIA_MONTAJE.md) y [pruebas físicas](docs/PRUEBAS_FISICAS.md): cómo armar el sistema y repetir los ensayos.
- [Evidencias](docs/evidencias/README.md): registro de pruebas, fotografías, video y salidas de las pruebas automatizadas.
