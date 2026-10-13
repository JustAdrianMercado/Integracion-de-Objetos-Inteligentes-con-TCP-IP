#include "ClienteTcp.h"
#include "RedSimulada.h"
#include "lwip/sockets.h"

#include <cerrno>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {
size_t total = 0;
size_t fallos = 0;
void comprobar(bool correcto, const char* nombre) {
    ++total;
    if (!correcto) { ++fallos; std::cerr << "FALLO: " << nombre << '\n'; }
}
void reiniciar(uint32_t tiempo = 0) {
    red = RedSimulada{};
    red.reloj = tiempo;
    red.wifiConectado = true;
}
ClienteTcp crearCliente(TipoCliente tipo = TipoCliente::Sensor) {
    return ClienteTcp(tipo, "red-de-prueba", "clave-de-prueba", "192.168.1.10", 5000);
}
void registrar(ClienteTcp& cliente) {
    cliente.iniciar();
    cliente.actualizar();
    red.entrada = "OK\n";
    cliente.actualizar();
    red.salida.clear();
}
const char* APAGADO = "SET redLed=off yellowLed=off greenLed=off";
}

int main() {
    {
        reiniciar();
        red.wifiConectado = false;
        auto cliente = crearCliente();
        cliente.iniciar();
        comprobar(red.modoWifi == WIFI_STA && !red.autoReconnect, "WiFi en modo estacion con reintentos controlados");
        comprobar(red.wifiSinAhorro, "Desactivar el modem-sleep para no retrasar la recepcion");
        comprobar(!cliente.enviar("POST distance=45.00"), "No publicar antes de abrir TCP");
        cliente.actualizar();
        comprobar(red.intentosWifi == 1 && red.intentosTcp == 0, "Primer intento WiFi inmediato sin TCP");
        red.reloj = 9999;
        cliente.actualizar();
        comprobar(red.intentosWifi == 1, "No reiniciar una asociacion WiFi en curso antes de 10000 ms");
        red.reloj = 10000;
        cliente.actualizar();
        comprobar(red.intentosWifi == 2, "Reintentar WiFi al vencer el plazo de asociacion");
        red.wifiConectado = true;
        cliente.actualizar();
        comprobar(red.salida == "REGISTER type=sensor\n", "Registrar el rol sensor con LF sin NUL");
        comprobar(red.timeoutConexion == 1000 && red.puertoTcp == 5000 && red.noDelay,
                  "Acotar connect a 1000 ms y habilitar TCP_NODELAY");
        comprobar(red.flagsEnvio == MSG_DONTWAIT && red.flagsRecepcion == MSG_DONTWAIT,
                  "Enviar y recibir sin esperas internas");
        comprobar(!cliente.estaRegistrado() && !cliente.enviar("POST distance=45.00"), "Esperar OK antes del POST");
        red.entrada = "O"; cliente.actualizar();
        red.entrada = "K"; cliente.actualizar();
        comprobar(!cliente.estaRegistrado(), "Conservar fragmentos sin aceptar OK hasta LF");
        red.entrada = "\n"; cliente.actualizar();
        comprobar(cliente.estaRegistrado() && cliente.puedeEnviar(), "OK completo habilita las lecturas");
        comprobar(!cliente.enviar(nullptr) && !cliente.enviar("REGISTER type=sensor"),
                  "La aplicacion solo publica POST");
        red.salida.clear();
        red.maximoEnvio = 3;
        comprobar(cliente.enviar("POST distance=45.00"), "Encolar una lectura registrada");
        comprobar(!cliente.puedeEnviar() && !cliente.enviar("POST distance=120.00"), "No acumular un historial de lecturas");
        const int llamadas = red.llamadasEnvio;
        cliente.actualizar();
        comprobar(red.salida == "POS" && red.llamadasEnvio == llamadas + 1, "Una escritura parcial por vuelta");
        for (int i = 0; i < 8; ++i) { ++red.reloj; cliente.actualizar(); }
        comprobar(red.salida == "POST distance=45.00\n" && cliente.puedeEnviar(), "Completar el POST sin repetir prefijos");
        red.salida.clear();
        red.maximoEnvio = 128;
        const std::string maxima = "POST distance=" + std::string(112, '0') + "1";
        comprobar(cliente.enviar(maxima.c_str()), "Aceptar 127 caracteres de contenido");
        cliente.actualizar();
        comprobar(red.salida == maxima + '\n' && red.salida.size() == 128, "Transmitir exactamente 128 bytes maximos");
        comprobar(!cliente.enviar((maxima + "1").c_str()), "Rechazar una linea que excede el limite");
    }
    {
        reiniciar();
        auto cliente = crearCliente();
        red.maximoEnvio = 1;
        cliente.iniciar(); cliente.actualizar();
        comprobar(red.salida == "R", "Conservar un REGISTER parcialmente enviado");
        red.entrada = "OK\n"; cliente.actualizar();
        comprobar(!red.tcpAbierto && !cliente.estaRegistrado(), "Rechazar OK antes de terminar de transmitir REGISTER");
        comprobar(red.salida == "R", "No seguir enviando tras una respuesta inesperada");
    }
    {
        reiniciar();
        auto cliente = crearCliente();
        cliente.iniciar(); cliente.actualizar();
        red.entrada = "O"; cliente.actualizar();
        red.reloj = 1999; cliente.actualizar();
        comprobar(red.tcpAbierto && !cliente.estaRegistrado(), "Registro permanece pendiente hasta su plazo");
        red.reloj = 2000; red.entrada = "K\n"; cliente.actualizar();
        comprobar(!red.tcpAbierto, "Un OK recibido fuera de plazo no registra al cliente");
        red.reloj = 3999; cliente.actualizar();
        comprobar(red.intentosTcp == 1, "Esperar 2000 ms despues del cierre");
        red.reloj = 4000; cliente.actualizar();
        comprobar(red.intentosTcp == 2, "Abrir un socket nuevo al reintentar");
        red.entrada = "OK\n"; cliente.actualizar();
        comprobar(cliente.estaRegistrado(), "Reiniciar el receptor descarta fragmentos de la conexion anterior");
        comprobar(red.salida == "REGISTER type=sensor\nREGISTER type=sensor\n", "Repetir el registro completo tras reconectar");
    }
    {
        reiniciar();
        red.conexionExitosa = false;
        red.duracionConexion = 250;
        auto cliente = crearCliente();
        cliente.iniciar(); cliente.actualizar();
        comprobar(!red.tcpAbierto && red.salida.empty(), "Connect fallido no intenta registrar");
        red.reloj = 2249; cliente.actualizar();
        comprobar(red.intentosTcp == 1, "Contar el reintento desde el final del connect");
        red.reloj = 2250; red.conexionExitosa = true; cliente.actualizar();
        red.reloj = 4499; red.entrada = "OK\n"; cliente.actualizar();
        comprobar(cliente.estaRegistrado(), "Plazo de registro comienza despues del connect acotado");
    }
    {
        reiniciar();
        auto cliente = crearCliente(); registrar(cliente);
        red.maximoEnvio = 0;
        cliente.enviar("POST distance=45.00"); cliente.actualizar();
        red.reloj = 1999; cliente.actualizar();
        comprobar(red.tcpAbierto && !cliente.puedeEnviar() && red.salida.empty(), "EAGAIN conserva el envio parcial sin bloquear");
        red.reloj = 2000; cliente.actualizar();
        comprobar(!red.tcpAbierto && !cliente.estaRegistrado(), "Cerrar un envio atascado a los 2000 ms");
        red.maximoEnvio = 128; red.reloj = 4000; cliente.actualizar();
        red.entrada = "OK\n"; cliente.actualizar();
        cliente.enviar("POST distance=120.00"); cliente.actualizar();
        comprobar(red.salida == "REGISTER type=sensor\nPOST distance=120.00\n", "No retransmitir lecturas viejas tras reconectar");
    }
    {
        reiniciar();
        auto cliente = crearCliente(); registrar(cliente);
        red.errorEnvio = EINTR;
        red.errorRecepcion = EINTR;
        cliente.enviar("POST distance=invalid"); cliente.actualizar();
        comprobar(cliente.estaRegistrado() && red.salida.empty(), "EINTR permite continuar en otra vuelta");
        red.errorEnvio = 0; red.errorRecepcion = 0; cliente.actualizar();
        comprobar(red.salida == "POST distance=invalid\n", "Completar la lectura despues de EINTR");
        red.errorEnvio = ECONNRESET;
        cliente.enviar("POST distance=45.00"); cliente.actualizar();
        comprobar(!red.tcpAbierto, "Error fatal de envio cierra la conexion");
    }
    for (int modo = 0; modo < 2; ++modo) {
        reiniciar();
        auto cliente = crearCliente(); registrar(cliente);
        if (modo == 0) { red.remotoCerrado = true; }
        else { red.errorRecepcion = ECONNRESET; }
        cliente.actualizar();
        comprobar(!cliente.estaRegistrado() && !red.tcpAbierto, "Detectar tanto FIN como error fatal de recepcion");
    }
    {
        reiniciar();
        auto cliente = crearCliente(); registrar(cliente);
        red.reloj = 50; red.wifiConectado = false; cliente.actualizar();
        comprobar(!cliente.estaRegistrado() && !red.tcpAbierto && red.intentosWifi == 0, "Perdida de WiFi cierra TCP inmediatamente");
        red.reloj = 2049; cliente.actualizar();
        comprobar(red.intentosWifi == 0, "Esperar el intervalo tras perder WiFi");
        red.reloj = 2050; cliente.actualizar();
        comprobar(red.intentosWifi == 1, "Reintentar WiFi despues de una perdida");
        red.reloj = 12049; cliente.actualizar();
        comprobar(red.intentosWifi == 1, "Dejar 10000 ms a la asociacion iniciada tras la perdida");
        red.reloj = 12050; cliente.actualizar();
        comprobar(red.intentosWifi == 2, "Repetir la asociacion si no se completo");
        red.wifiConectado = true; cliente.actualizar();
        comprobar(red.salida == "REGISTER type=sensor\n", "Recuperar WiFi exige un registro nuevo");
    }
    const std::vector<std::string> respuestasInvalidas = {
        "OK\r\n", "\n", std::string("OK\0\n", 4), std::string(128, 'A'),
        "POST distance=45.00\n", std::string(APAGADO) + '\n', "OK\nOK\n",
        "ERROR reason=invalid_message\n", "ERROR reason=not_registered\n", "ERROR reason=role_busy\n"
    };
    for (const auto& respuesta : respuestasInvalidas) {
        reiniciar();
        auto cliente = crearCliente(); cliente.iniciar(); cliente.actualizar();
        red.entrada = respuesta; cliente.actualizar();
        comprobar(!red.tcpAbierto && !cliente.estaRegistrado(), "Cerrar respuestas invalidas, inesperadas o ERROR");
    }
    {
        reiniciar();
        auto cliente = crearCliente(); registrar(cliente);
        red.entrada = std::string(APAGADO) + '\n'; cliente.actualizar();
        comprobar(!cliente.estaRegistrado(), "El sensor registrado tampoco acepta SET");
    }
    {
        reiniciar(UINT32_MAX - 100);
        auto cliente = crearCliente(); cliente.iniciar(); cliente.actualizar();
        red.reloj += 1999; cliente.actualizar();
        comprobar(red.tcpAbierto, "Plazo de registro soporta el desbordamiento de millis");
        ++red.reloj; cliente.actualizar();
        comprobar(!red.tcpAbierto, "Vencer el plazo a traves del desbordamiento");
        red.reloj += 1999; cliente.actualizar();
        comprobar(red.intentosTcp == 1, "Intervalo de reconexion soporta el desbordamiento");
        ++red.reloj; cliente.actualizar();
        comprobar(red.intentosTcp == 2, "Reconectar al cumplirse el intervalo despues del desbordamiento");
    }
    {
        reiniciar(UINT32_MAX - 100);
        auto cliente = crearCliente(); registrar(cliente);
        red.maximoEnvio = 0; cliente.enviar("POST distance=45.00");
        red.reloj += 2000; cliente.actualizar();
        comprobar(!red.tcpAbierto, "Plazo de envio tambien soporta el desbordamiento");
    }
    {
        reiniciar();
        auto cliente = crearCliente(TipoCliente::Actuador);
        cliente.iniciar(); cliente.actualizar();
        comprobar(red.salida == "REGISTER type=actuator\n", "El adaptador comun registra ambos roles");
        red.entrada = "OK\n" + std::string(APAGADO) + '\n'; cliente.actualizar();
        comprobar(cliente.estaRegistrado() && !cliente.enviar("POST distance=45.00"), "El actuador no publica lecturas");
        char salida[128] = "sentinela";
        comprobar(!cliente.recibir(salida, 2) && std::string(salida) == "sentinela", "Buffer pequeno no consume ni modifica el SET");
        comprobar(cliente.recibir(salida, sizeof(salida)) && std::string(salida) == APAGADO,
                  "Procesar OK y SET juntos sin perder el comando");
        red.entrada = "SET redLed=on yellowLed=off "; cliente.actualizar();
        comprobar(!cliente.recibir(salida, sizeof(salida)), "SET fragmentado espera su LF");
        red.entrada = "greenLed=off\n"; cliente.actualizar();
        comprobar(cliente.recibir(salida, sizeof(salida)) && std::string(salida) == "SET redLed=on yellowLed=off greenLed=off",
                  "Entregar el SET al completar su LF");
        comprobar(!cliente.recibir(salida, sizeof(salida)), "Cada SET se entrega una sola vez");
        red.entrada = "SET redLed=on yellowLed=off greenLed=off\n" + std::string(APAGADO) + '\n'; cliente.actualizar();
        comprobar(cliente.recibir(salida, sizeof(salida)) && std::string(salida) == APAGADO &&
                  !cliente.recibir(salida, sizeof(salida)), "Con dos SET en un bloque gana el ultimo");
        red.entrada = "SET redLed=on yellowLed=off greenLed=invalido\n"; cliente.actualizar();
        comprobar(!red.tcpAbierto && !cliente.recibir(salida, sizeof(salida)), "No entregar comandos parcialmente validos");
    }
    {
        reiniciar();
        auto cliente = crearCliente(TipoCliente::Actuador); registrar(cliente);
        for (int i = 0; i < 5; ++i) { red.entrada = std::string(APAGADO) + '\n'; cliente.actualizar(); }
        char salida[128];
        comprobar(red.tcpAbierto && cliente.recibir(salida, sizeof(salida)) && !cliente.recibir(salida, sizeof(salida)),
                  "SET no consumidos se reemplazan sin cerrar la conexion");
    }
    for (int caso = 0; caso < 4; ++caso) {
        reiniciar();
        ClienteTcp cliente(TipoCliente::Sensor, caso == 0 ? "" : "red",
                           "clave-de-prueba", caso == 1 ? "servidor.local" : (caso == 2 ? "0.0.0.0" : "192.168.1.10"),
                           caso == 3 ? 0 : 5000);
        cliente.iniciar(); cliente.actualizar();
        comprobar(red.intentosWifi == 0 && red.intentosTcp == 0, "Configuracion incompleta no intenta conexiones");
    }
    std::cout << total - fallos << " de " << total << " comprobaciones del cliente TCP aprobadas.\n";
    return fallos == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
