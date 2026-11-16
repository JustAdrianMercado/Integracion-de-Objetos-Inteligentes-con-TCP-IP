#include "RedSimulada.h"
#include "Arduino.h"

#include <cstdlib>
#include <iostream>
#include <string>

void setup();
void loop();

namespace {
constexpr uint8_t VERDE = 25;
constexpr uint8_t AMARILLO = 26;
constexpr uint8_t ROJO = 27;
const std::string APAGADO = "SET redLed=off yellowLed=off greenLed=off\n";
const std::string ROJO_4HZ = "SET redLed=blink_4 yellowLed=off greenLed=off\n";
const std::string AMARILLO_2HZ = "SET redLed=off yellowLed=blink_2 greenLed=off\n";
const std::string VERDE_FIJO = "SET redLed=off yellowLed=off greenLed=on\n";
const std::string LEJOS = "SET redLed=blink_2 yellowLed=blink_2 greenLed=blink_2\n";

size_t total = 0;
size_t fallos = 0;
void comprobar(bool correcto, const char* nombre) {
    ++total;
    if (!correcto) { ++fallos; std::cerr << "FALLO: " << nombre << '\n'; }
}
bool niveles(int rojo, int amarillo, int verde) {
    return red.niveles[ROJO] == rojo && red.niveles[AMARILLO] == amarillo && red.niveles[VERDE] == verde;
}
void paso(uint32_t tiempo, const std::string& entrada = "") {
    red.reloj = tiempo;
    red.entrada += entrada;
    loop();
}
}

int main() {
    red.wifiConectado = true;
    red.niveles[ROJO] = red.niveles[AMARILLO] = red.niveles[VERDE] = HIGH;
    setup();
    comprobar(Serial.velocidad == 115200, "Consola serie a 115200 baudios");
    comprobar(red.modosPin[ROJO] == OUTPUT && red.modosPin[AMARILLO] == OUTPUT && red.modosPin[VERDE] == OUTPUT &&
              niveles(LOW, LOW, LOW), "GPIO 25/26/27 como salidas y apagados al iniciar");
    comprobar(red.wifiSinAhorro, "El actuador desactiva el modem-sleep");
    loop();
    comprobar(red.salida == "REGISTER type=actuator\n", "Registrar el rol actuador");
    red.salida.clear();
    paso(5, "OK\n");
    comprobar(niveles(LOW, LOW, LOW), "OK sin SET mantiene los LEDs apagados");
    paso(8, ROJO_4HZ.substr(0, 20));
    comprobar(niveles(LOW, LOW, LOW), "Un SET fragmentado no actua hasta su LF");
    paso(10, ROJO_4HZ.substr(20));
    comprobar(niveles(HIGH, LOW, LOW), "SET completo: rojo blink_4 empieza encendido");
    paso(134);
    comprobar(niveles(HIGH, LOW, LOW), "Mantener 125 ms encendido");
    paso(135);
    comprobar(niveles(LOW, LOW, LOW), "Alternar el rojo a los 125 ms");
    paso(200, VERDE_FIJO);
    comprobar(niveles(LOW, LOW, HIGH), "Aplicar conjuntamente los tres estados");
    paso(2199);
    comprobar(niveles(LOW, LOW, HIGH), "Conservar el ultimo SET durante 2000 ms");
    paso(2200);
    comprobar(niveles(LOW, LOW, LOW) && red.tcpAbierto, "2000 ms sin SET apagan y conservan la conexion");
    paso(2300, VERDE_FIJO);
    comprobar(niveles(LOW, LOW, HIGH), "Un SET posterior reactiva las salidas");
    paso(2400, AMARILLO_2HZ);
    paso(2649);
    comprobar(niveles(LOW, HIGH, LOW), "blink_2 mantiene 250 ms encendido");
    paso(2650, AMARILLO_2HZ);
    comprobar(niveles(LOW, LOW, LOW), "Un SET identico no reinicia la fase");
    paso(2700, "SET redLed=on yellowLed=off greenLed=invalido\n");
    comprobar(niveles(LOW, LOW, LOW) && !red.tcpAbierto, "Un SET invalido cierra sin aplicar estados parciales");

    paso(4700);
    comprobar(red.salida == "REGISTER type=actuator\n", "Reconectar repitiendo el registro");
    red.salida.clear();
    paso(4701, "OK\n" + AMARILLO_2HZ);
    comprobar(niveles(LOW, HIGH, LOW), "Recuperar la actuacion tras reconectar");
    red.remotoCerrado = true;
    paso(4702);
    comprobar(niveles(LOW, LOW, LOW) && !red.tcpAbierto, "La desconexion del servidor apaga inmediatamente");

    paso(UINT32_MAX - 100);
    red.salida.clear();
    paso(UINT32_MAX - 99, "OK\n" + VERDE_FIJO);
    comprobar(niveles(LOW, LOW, HIGH), "Registro y SET cerca del desbordamiento de millis");
    paso(UINT32_MAX - 99 + 1999);
    comprobar(niveles(LOW, LOW, HIGH), "El plazo de comandos atraviesa el desbordamiento");
    paso(UINT32_MAX - 99 + 2000);
    comprobar(niveles(LOW, LOW, LOW) && red.tcpAbierto, "Vencer el plazo de comandos despues del desbordamiento");
    paso(UINT32_MAX - 99 + 2001, APAGADO);
    comprobar(niveles(LOW, LOW, LOW) && red.tcpAbierto, "Un SET de apagado se acepta sin cerrar");

    paso(10000, LEJOS);
    comprobar(niveles(HIGH, HIGH, HIGH), "Lejos enciende los tres al aplicar el SET");
    paso(10250, LEJOS);
    comprobar(niveles(LOW, LOW, LOW), "Lejos alterna sincronizado con SET repetido");
    paso(10500, LEJOS);
    comprobar(niveles(HIGH, HIGH, HIGH), "Lejos completa un ciclo a 2 Hz");
    for (uint32_t tiempo = 10750; tiempo <= 12250; tiempo += 250) { paso(tiempo); }
    paso(12499);
    comprobar(niveles(LOW, LOW, LOW) && red.tcpAbierto, "Lejos sigue alternando antes del plazo sin SET");
    paso(12500);
    comprobar(niveles(LOW, LOW, LOW) && red.tcpAbierto, "El timeout existente tambien apaga Lejos");
    paso(12600, LEJOS);
    paso(12601, APAGADO);
    comprobar(niveles(LOW, LOW, LOW) && red.tcpAbierto, "La lectura invalida apaga Lejos mediante SET");
    paso(12700, LEJOS);
    red.remotoCerrado = true;
    paso(12701);
    comprobar(niveles(LOW, LOW, LOW) && !red.tcpAbierto, "Perder la comunicacion tambien apaga Lejos");

    std::cout << total - fallos << " de " << total << " comprobaciones del principal actuador aprobadas.\n";
    return fallos == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
