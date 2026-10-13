#include "RedSimulada.h"
#include "Arduino.h"

#include <cstdlib>
#include <iostream>

void setup();
void loop();

namespace {
size_t total = 0;
size_t fallos = 0;
void comprobar(bool correcto, const char* nombre) {
    ++total;
    if (!correcto) { ++fallos; std::cerr << "FALLO: " << nombre << '\n'; }
}
}

int main() {
    red.wifiConectado = true;
    setup(); loop();
    comprobar(Serial.velocidad == 115200, "Consola serie a 115200 baudios");
    comprobar(red.salida == "REGISTER type=sensor\n" && red.mediciones == 0, "No medir ni publicar antes de OK");
    red.salida.clear(); red.reloj = 249; loop();
    comprobar(red.salida.empty() && red.mediciones == 0, "El intervalo no habilita lecturas sin registrar");
    red.entrada = "O"; loop();
    comprobar(red.mediciones == 0, "Un fragmento de OK no habilita el sensor");
    red.reloj = 250; red.entrada = "K\n"; red.eco = 2624; loop();
    comprobar(red.salida == "POST distance=45.00\n" && red.mediciones == 1, "Medicion nueva inmediatamente despues de OK");
    red.salida.clear(); red.reloj = 499; loop();
    comprobar(red.mediciones == 1 && red.salida.empty(), "Esperar 250 ms entre mediciones");
    red.reloj = 500; red.eco = 4373; loop();
    comprobar(red.salida == "POST distance=75.00\n" && red.mediciones == 2, "Publicar el siguiente eco con dos decimales");
    red.salida.clear(); red.reloj = 750; red.eco = 6997; loop();
    comprobar(red.salida == "POST distance=120.00\n" && red.mediciones == 3, "El sensor publica datos sin clasificar rangos");
    red.salida.clear(); red.reloj = 1000; red.eco = 0; red.duracionMedicion = 30; loop();
    comprobar(red.salida == "POST distance=invalid\n" && red.mediciones == 4, "Timeout de eco publica invalid");
    red.duracionMedicion = 0;
    red.salida.clear(); red.reloj = 1200; red.wifiConectado = false; loop();
    red.reloj = 3199; loop();
    comprobar(!red.tcpAbierto && red.mediciones == 4 && red.salida.empty(), "No acumular mediciones durante la desconexion");
    red.reloj = 3200; loop();
    comprobar(red.intentosWifi == 1, "El principal sigue atendiendo los reintentos WiFi");
    red.reloj = 3201; red.wifiConectado = true; loop();
    comprobar(red.salida == "REGISTER type=sensor\n" && red.mediciones == 4, "Recuperacion comienza registrando otra conexion");
    red.salida.clear(); red.reloj = 3202; red.eco = 5831; red.entrada = "OK\n"; loop();
    comprobar(red.salida == "POST distance=100.00\n" && red.mediciones == 5, "Enviar un eco nuevo tras reconectar");
    red.salida.clear(); red.reloj = 3452; red.perderWifiDuranteEco = true; loop();
    comprobar(red.salida.empty() && !red.tcpAbierto && red.mediciones == 6, "No publicar si WiFi se pierde durante la medicion");
    red.perderWifiDuranteEco = false;
    red.reloj = 5452; loop(); red.wifiConectado = true; loop();
    red.entrada = "OK\n"; red.eco = 2624; loop(); red.salida.clear();
    red.reloj = UINT32_MAX - 100; loop(); red.salida.clear();
    const int mediciones = red.mediciones;
    red.reloj += 249; loop();
    comprobar(red.mediciones == mediciones, "Intervalo de muestreo soporta el desbordamiento de millis");
    ++red.reloj; loop();
    comprobar(red.mediciones == mediciones + 1 && red.salida == "POST distance=45.00\n", "Publicar al cumplirse 250 ms despues del desbordamiento");
    red.salida.clear(); red.maximoEnvio = 0; red.reloj += 250; loop();
    const int alAtascar = red.mediciones;
    red.reloj += 250; loop();
    comprobar(red.mediciones == alAtascar && red.salida.empty(), "Cola ocupada evita medir y acumular lecturas");
    red.reloj += 1750; loop();
    comprobar(!red.tcpAbierto && red.mediciones == alAtascar, "Envio atascado cierra sin publicar datos acumulados");
    red.maximoEnvio = 128; red.reloj += 2000; loop();
    red.entrada = "OK\n"; red.eco = 4373; loop();
    comprobar(red.salida == "REGISTER type=sensor\nPOST distance=75.00\n", "Reconexion tras atasco descarta el POST anterior");
    std::cout << total - fallos << " de " << total << " comprobaciones del principal sensor aprobadas.\n";
    return fallos == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
