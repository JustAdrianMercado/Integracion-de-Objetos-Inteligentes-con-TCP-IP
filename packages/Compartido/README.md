# Compartido

Biblioteca C++11 reutilizable por el servidor Windows y los clientes ESP32. Contiene los tipos del dominio, `ControlDistancia`, `ProtocoloAplicacion` y `ReceptorLineas`, sin dependencia de Arduino o Winsock.

## Crear e interpretar mensajes

`ProtocoloAplicacion` trabaja con el **contenido sin LF**, terminado localmente en `\0`. Cada función `crear...` recibe el buffer de salida y su capacidad real, incluido el espacio para `\0`. Devuelve `false` si el buffer es insuficiente o los datos no son válidos, y conserva el buffer anterior.

| Contenido | Crear | Interpretar |
|---|---|---|
| `REGISTER type=sensor` o `REGISTER type=actuator` | `crearRegistro` | `leerRegistro` |
| `POST distance=45.00` o `POST distance=invalid` | `crearLectura` | `leerLectura` |
| `SET redLed=off yellowLed=off greenLed=on` | `crearComando` | `leerComando` |
| `OK` | `crearOk` | `leerOk` |
| `ERROR reason=invalid_message`, `not_registered` o `role_busy` | `crearError` | `leerError` |

Los roles se representan con `TipoCliente`, los estados con `EstadoLed` y los motivos de error con `MotivoError`.

```cpp
#include "ProtocoloAplicacion.h"

char contenido[ProtocoloAplicacion::CapacidadBuffer];
if (ProtocoloAplicacion::crearLectura(45.0F, contenido, sizeof(contenido))) {
    // contenido contiene "POST distance=45.00", sin salto de linea.
    // El adaptador de red enviara el contenido y un byte LF, sin transmitir '\0'.
}
```

Las distancias se emiten con dos decimales y punto, independientemente del idioma del sistema. Cero, negativos, NaN e infinitos se normalizan a `invalid`; también se normaliza una lectura positiva que redondea a `0.00`.

Al leer se acepta una parte entera con una fracción opcional, como `45`, `45.00` o `0.01`. El resultado debe ser representable como `float` finito mayor que cero. Se rechazan signos, coma decimal, notación científica, fracciones sin parte entera y campos adicionales.

`leerLectura` entrega NaN para el token `invalid`, que `ControlDistancia` convierte en todos apagados. Un fallo de lectura conserva los datos de salida anteriores. `leerComando` solo entrega un `ComandoLeds` cuando **los tres estados** y toda la sintaxis son válidos.

## Recibir el flujo TCP

Mantener **un `ReceptorLineas` por conexión**. Pasar a `agregar` cada byte de los bloques recibidos, en orden. El receptor utiliza un buffer fijo y conserva el fragmento entre llamadas.

| Resultado de `agregar` | Acción del adaptador |
|---|---|
| `Incompleto` | Continuar atendiendo la red y los temporizadores; conservar el receptor. |
| `LineaCompleta` | Leer `linea()` e interpretar el contenido antes de agregar el siguiente byte. |
| `MensajeInvalido` | Aplicar la política de error y cerrar esa conexión. |

`linea()` devuelve `nullptr` cuando no hay una línea completa. Su puntero pertenece al receptor y solo conserva el contenido hasta agregar el siguiente byte o reiniciarlo.

El límite es **127 bytes de contenido más LF: 128 bytes en el cable**. Una línea vacía, CR, NUL, otros controles, bytes fuera de ASCII imprimible o un byte 128 de contenido provocan rechazo inmediato. Después de un rechazo, el receptor sigue rechazando bytes hasta `reiniciar()`.

Al abrir una conexión nueva, llamar a `reiniciar()` para descartar fragmentos y errores anteriores. Una línea completa todavía debe pasar la validación de `ProtocoloAplicacion`: delimitarla no implica aceptar su comando.

El [servidor TCP de Windows](../../apps/servidor/README.md) implementa las políticas de registro, roles ocupados, ausencia de lecturas y escrituras parciales. Los adaptadores de los ESP32 se implementarán en las siguientes etapas. La especificación completa está en el [diseño del sistema](../../docs/diagramas/Diseno_del_sistema.md).

## Pruebas

Desde la raíz del repositorio, en una terminal de desarrollo de Visual Studio:

```powershell
cmake -S . -B build/windows -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build/windows
ctest --test-dir build/windows --output-on-failure
```

Las pruebas verifican las reglas de distancia, todos los mensajes, las 64 combinaciones de estados de los LEDs, el rechazo sin cambios parciales, las capacidades de salida, los fragmentos, los mensajes consecutivos y los límites de recepción. También prueban el punto decimal bajo un idioma que utiliza coma cuando ese locale está instalado.
