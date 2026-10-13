#include "ServidorTcp.h"

#include <ws2tcpip.h>

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

using namespace std::chrono_literals;

namespace {
const std::string apagado = "SET redLed=off yellowLed=off greenLed=off";
const std::string rojo = "SET redLed=blink_4 yellowLed=off greenLed=off";
const std::string amarillo = "SET redLed=off yellowLed=blink_2 greenLed=off";
const std::string verde = "SET redLed=off yellowLed=off greenLed=on";

void exigir(bool resultado, const std::string& mensaje) {
    if (!resultado) throw std::runtime_error(mensaje);
}

class WinsockPruebas {
public:
    WinsockPruebas() {
        WSADATA datos{};
        exigir(WSAStartup(MAKEWORD(2, 2), &datos) == 0, "WSAStartup en pruebas");
    }
    ~WinsockPruebas() { WSACleanup(); }
};

class Entorno {
public:
    Entorno() {
        exigir(servidor_.iniciar(0, "127.0.0.1"), servidor_.ultimoError());
        hilo_ = std::thread([this] {
            while (continuar_.load()) {
                if (!servidor_.actualizar()) break;
            }
        });
    }
    ~Entorno() { detener(); }
    std::uint16_t puerto() const { return servidor_.puerto(); }
    void detener() {
        continuar_.store(false);
        if (hilo_.joinable()) hilo_.join();
        servidor_.detener();
    }
private:
    ServidorTcp servidor_;
    std::atomic<bool> continuar_{true};
    std::thread hilo_;
};

class Cliente {
public:
    explicit Cliente(std::uint16_t puerto) {
        socket_ = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        exigir(socket_ != INVALID_SOCKET, "Crear socket de prueba");
        const DWORD plazo = 3500;
        const BOOL sinNagle = TRUE;
        const bool opciones = setsockopt(socket_, SOL_SOCKET, SO_RCVTIMEO,
            reinterpret_cast<const char*>(&plazo), sizeof(plazo)) == 0 &&
            setsockopt(socket_, SOL_SOCKET, SO_SNDTIMEO,
            reinterpret_cast<const char*>(&plazo), sizeof(plazo)) == 0 &&
            setsockopt(socket_, IPPROTO_TCP, TCP_NODELAY,
            reinterpret_cast<const char*>(&sinNagle), sizeof(sinNagle)) == 0;
        sockaddr_in destino{};
        destino.sin_family = AF_INET;
        destino.sin_port = htons(puerto);
        InetPtonA(AF_INET, "127.0.0.1", &destino.sin_addr);
        if (!opciones || ::connect(socket_, reinterpret_cast<const sockaddr*>(&destino), sizeof(destino)) != 0) {
            cerrar();
            throw std::runtime_error("Conectar cliente a localhost");
        }
    }
    ~Cliente() { cerrar(); }
    Cliente(const Cliente&) = delete;
    Cliente& operator=(const Cliente&) = delete;

    void enviar(const std::string& datos) {
        std::size_t escritos = 0;
        while (escritos < datos.size()) {
            const int cantidad = ::send(socket_, datos.data() + escritos,
                                        static_cast<int>(datos.size() - escritos), 0);
            exigir(cantidad > 0, "Enviar datos al servidor");
            escritos += static_cast<std::size_t>(cantidad);
        }
    }

    std::string leerLinea() {
        std::string linea;
        const auto limite = std::chrono::steady_clock::now() + 3500ms;
        while (std::chrono::steady_clock::now() < limite) {
            const auto restante = std::chrono::duration_cast<std::chrono::milliseconds>(
                limite - std::chrono::steady_clock::now());
            exigir(esperar(restante), "Tiempo agotado esperando una linea TCP");
            char byte = '\0';
            const int recibidos = ::recv(socket_, &byte, 1, 0);
            exigir(recibidos == 1, "Conexion cerrada antes de recibir una linea completa");
            if (byte == '\n') return linea;
            linea.push_back(byte);
            exigir(linea.size() <= 127, "El servidor excedio el limite de linea");
        }
        throw std::runtime_error("Tiempo agotado esperando LF");
    }

    void esperarLinea(const std::string& esperada) {
        const std::string obtenida = leerLinea();
        exigir(obtenida == esperada, "Esperado: " + esperada + "; recibido: " + obtenida);
    }

    bool sinDatos(std::chrono::milliseconds plazo = 100ms) { return !esperar(plazo); }

    bool cerrado() {
        if (!esperar(3500ms)) return false;
        char byte;
        const int cantidad = ::recv(socket_, &byte, 1, 0);
        if (cantidad == 0) return true;
        const int codigo = WSAGetLastError();
        return cantidad == SOCKET_ERROR && (codigo == WSAECONNRESET || codigo == WSAECONNABORTED);
    }

    void cerrar() {
        if (socket_ != INVALID_SOCKET) {
            closesocket(socket_);
            socket_ = INVALID_SOCKET;
        }
    }

private:
    bool esperar(std::chrono::milliseconds plazo) {
        fd_set lectura;
        FD_ZERO(&lectura);
        FD_SET(socket_, &lectura);
        if (plazo.count() < 0) plazo = 0ms;
        const timeval espera{static_cast<long>(plazo.count() / 1000),
                              static_cast<long>((plazo.count() % 1000) * 1000)};
        const int resultado = ::select(0, &lectura, nullptr, nullptr, &espera);
        exigir(resultado != SOCKET_ERROR, "select en cliente de prueba");
        return resultado > 0;
    }
    SOCKET socket_ = INVALID_SOCKET;
};

void registrarSensor(Cliente& cliente) {
    cliente.enviar("REGISTER type=sensor\n");
    cliente.esperarLinea("OK");
}

void registrarActuador(Cliente& cliente, const std::string& inicial = apagado) {
    cliente.enviar("REGISTER type=actuator\n");
    cliente.esperarLinea("OK");
    cliente.esperarLinea(inicial);
}

void esperarErrorYCierre(Cliente& cliente, const std::string& motivo) {
    cliente.esperarLinea("ERROR reason=" + motivo);
    exigir(cliente.cerrado(), "Cerrar conexion despues de enviar ERROR");
}

void comprobarPlazo(std::chrono::steady_clock::time_point inicio) {
    const auto transcurrido = std::chrono::steady_clock::now() - inicio;
    exigir(transcurrido >= 1800ms && transcurrido <= 3200ms,
           "El plazo nominal de 2000 ms no se aplico dentro del margen de la prueba");
}

struct Pruebas {
    std::size_t total = 0;
    std::size_t fallos = 0;
    void caso(const char* nombre, const std::function<void()>& ejecutar) {
        ++total;
        try {
            ejecutar();
        } catch (const std::exception& error) {
            ++fallos;
            std::cerr << "FALLO: " << nombre << ": " << error.what() << '\n';
        }
    }
};
}

int main() {
    try {
        WinsockPruebas winsock;
        Pruebas pruebas;
        pruebas.caso("Inicio, configuracion invalida y reinicio", [] {
            ServidorTcp servidor;
            exigir(!servidor.actualizar(), "No actualizar sin iniciar");
            exigir(!servidor.iniciar(0, "no-es-una-ip") && !servidor.ultimoError().empty(), "Rechazar IP invalida");
            exigir(!servidor.iniciar(0, nullptr), "Rechazar IP nula");
            exigir(servidor.iniciar(0, "127.0.0.1") && servidor.puerto() != 0, "Abrir puerto dinamico");
            exigir(!servidor.iniciar(), "No volver a iniciar un servidor activo");
            servidor.detener();
            servidor.detener();
            exigir(servidor.iniciar(0, "127.0.0.1"), "Reiniciar tras cerrar los recursos");
        });

        pruebas.caso("Puerto ocupado conserva el servidor original", [] {
            Entorno entorno;
            ServidorTcp segundo;
            exigir(!segundo.iniciar(entorno.puerto(), "127.0.0.1") && !segundo.ultimoError().empty(),
                   "No compartir un puerto ocupado");
            Cliente sensor(entorno.puerto());
            registrarSensor(sensor);
        });

        pruebas.caso("Actuador primero, limites y comandos identicos", [] {
            Entorno entorno;
            Cliente actuador(entorno.puerto());
            registrarActuador(actuador);
            Cliente sensor(entorno.puerto());
            registrarSensor(sensor);
            const std::pair<const char*, std::string> casos[] = {
                {"45.00", rojo}, {"49.99", rojo}, {"50.00", amarillo},
                {"99.99", amarillo}, {"100.00", verde}, {"120.00", verde},
                {"120.00", verde}, {"invalid", apagado},
            };
            for (const auto& caso : casos) {
                sensor.enviar(std::string("POST distance=") + caso.first + "\n");
                actuador.esperarLinea(caso.second);
            }
        });

        pruebas.caso("Sensor primero conserva solamente la ultima lectura reciente", [] {
            Entorno entorno;
            Cliente sensor(entorno.puerto());
            registrarSensor(sensor);
            sensor.enviar("POST distance=45.00\nPOST distance=120.00\n");
            std::this_thread::sleep_for(100ms);
            Cliente actuador(entorno.puerto());
            registrarActuador(actuador, verde);
            exigir(actuador.sinDatos(), "No reproducir lecturas anteriores al registro del actuador");
        });

        pruebas.caso("Lectura invalid reciente al registrar el actuador", [] {
            Entorno entorno;
            Cliente sensor(entorno.puerto());
            registrarSensor(sensor);
            sensor.enviar("POST distance=invalid\n");
            std::this_thread::sleep_for(100ms);
            Cliente actuador(entorno.puerto());
            registrarActuador(actuador);
        });

        pruebas.caso("Lectura caducada no se restaura al registrar el actuador", [] {
            Entorno entorno;
            Cliente sensor(entorno.puerto());
            registrarSensor(sensor);
            sensor.enviar("POST distance=120.00\n");
            exigir(sensor.cerrado(), "Cerrar sensor al caducar la lectura");
            Cliente actuador(entorno.puerto());
            registrarActuador(actuador);
        });

        pruebas.caso("Roles ocupados preservan ambos clientes originales", [] {
            Entorno entorno;
            Cliente actuador(entorno.puerto());
            registrarActuador(actuador);
            Cliente sensor(entorno.puerto());
            registrarSensor(sensor);
            sensor.enviar("POST distance=45.00\n");
            actuador.esperarLinea(rojo);
            for (const char* rol : {"sensor", "actuator"}) {
                Cliente duplicado(entorno.puerto());
                duplicado.enviar(std::string("REGISTER type=") + rol + "\n");
                esperarErrorYCierre(duplicado, "role_busy");
                exigir(actuador.sinDatos(), "El rechazo de otro cliente no debe apagar el actuador original");
                sensor.enviar("POST distance=100.00\n");
                actuador.esperarLinea(verde);
            }
        });

        pruebas.caso("Mensajes operativos antes de REGISTER", [] {
            Entorno entorno;
            for (const std::string& texto : {std::string("POST distance=45.00\n"), apagado + "\n"}) {
                Cliente cliente(entorno.puerto());
                cliente.enviar(texto);
                esperarErrorYCierre(cliente, "not_registered");
            }
        });

        pruebas.caso("Comandos malformados o incompatibles con la direccion", [] {
            Entorno entorno;
            for (const char* texto : {"UNKNOWN\n", "REGISTER type=other\n", "POST distance=0\n", "OK\n"}) {
                Cliente cliente(entorno.puerto());
                cliente.enviar(texto);
                esperarErrorYCierre(cliente, "invalid_message");
            }
        });

        pruebas.caso("Segundo REGISTER cierra sensor, apaga y libera el rol", [] {
            Entorno entorno;
            Cliente actuador(entorno.puerto());
            registrarActuador(actuador);
            Cliente sensor(entorno.puerto());
            registrarSensor(sensor);
            sensor.enviar("POST distance=45.00\n");
            actuador.esperarLinea(rojo);
            sensor.enviar("REGISTER type=sensor\n");
            esperarErrorYCierre(sensor, "invalid_message");
            actuador.esperarLinea(apagado);
            Cliente nuevo(entorno.puerto());
            registrarSensor(nuevo);
            nuevo.enviar("POST distance=100.00\n");
            actuador.esperarLinea(verde);
        });

        pruebas.caso("Actuador no puede publicar lecturas", [] {
            Entorno entorno;
            Cliente actuador(entorno.puerto());
            registrarActuador(actuador);
            actuador.enviar("POST distance=45.00\n");
            esperarErrorYCierre(actuador, "invalid_message");
            Cliente nuevo(entorno.puerto());
            registrarActuador(nuevo);
        });

        pruebas.caso("POST invalido apaga sin aplicar parcialmente una lectura", [] {
            Entorno entorno;
            Cliente actuador(entorno.puerto());
            registrarActuador(actuador);
            Cliente sensor(entorno.puerto());
            registrarSensor(sensor);
            sensor.enviar("POST distance=120.00\n");
            actuador.esperarLinea(verde);
            sensor.enviar("POST distance=50,00\n");
            esperarErrorYCierre(sensor, "invalid_message");
            actuador.esperarLinea(apagado);
        });

        pruebas.caso("Fragmentos, varias lineas y otro cliente incompleto", [] {
            Entorno entorno;
            Cliente pendiente(entorno.puerto());
            pendiente.enviar("REGISTER type=");
            Cliente actuador(entorno.puerto());
            registrarActuador(actuador);
            Cliente sensor(entorno.puerto());
            sensor.enviar("REGI");
            exigir(sensor.sinDatos(), "No responder a REGISTER incompleto");
            sensor.enviar("STER type=sensor\nPOST distance=4");
            sensor.esperarLinea("OK");
            exigir(actuador.sinDatos(), "No actuar con un POST incompleto");
            sensor.enviar("5.00\nPOST distance=50.00\nPOST distance=100");
            actuador.esperarLinea(rojo);
            actuador.esperarLinea(amarillo);
            exigir(actuador.sinDatos(), "Conservar fragmento final sin generar SET");
            sensor.enviar(".00\n");
            actuador.esperarLinea(verde);
        });

        pruebas.caso("Limites de longitud, CR y NUL en sockets reales", [] {
            Entorno entorno;
            Cliente actuador(entorno.puerto());
            registrarActuador(actuador);
            {
                Cliente sensor(entorno.puerto());
                registrarSensor(sensor);
                const std::string prefijo = "POST distance=";
                sensor.enviar(prefijo + std::string(127 - prefijo.size() - 1, '0') + "1\n");
                actuador.esperarLinea(rojo);
                sensor.cerrar();
                actuador.esperarLinea(apagado);
            }
            for (const std::string& bloque : {std::string(128, 'x'), std::string("REGISTER type=sensor\r\n"),
                                               std::string("REGISTER\0type=sensor\n", 21)}) {
                Cliente cliente(entorno.puerto());
                cliente.enviar(bloque);
                esperarErrorYCierre(cliente, "invalid_message");
                exigir(actuador.sinDatos(), "Un cliente sin rol no invalida la lectura de otro");
            }
        });

        pruebas.caso("Desconexion del sensor apaga inmediatamente", [] {
            Entorno entorno;
            Cliente actuador(entorno.puerto());
            registrarActuador(actuador);
            Cliente sensor(entorno.puerto());
            registrarSensor(sensor);
            sensor.enviar("POST distance=120.00\n");
            actuador.esperarLinea(verde);
            const auto inicio = std::chrono::steady_clock::now();
            sensor.cerrar();
            actuador.esperarLinea(apagado);
            exigir(std::chrono::steady_clock::now() - inicio < 1000ms, "Apagado sin esperar 2000 ms");
            Cliente nuevo(entorno.puerto());
            registrarSensor(nuevo);
            nuevo.enviar("POST distance=45.00\n");
            actuador.esperarLinea(rojo);
        });

        pruebas.caso("Reconectar actuador conserva la lectura reciente", [] {
            Entorno entorno;
            Cliente sensor(entorno.puerto());
            registrarSensor(sensor);
            Cliente actuador(entorno.puerto());
            registrarActuador(actuador);
            sensor.enviar("POST distance=120.00\n");
            actuador.esperarLinea(verde);
            actuador.cerrar();
            std::this_thread::sleep_for(50ms);
            Cliente nuevo(entorno.puerto());
            registrarActuador(nuevo, verde);
        });

        pruebas.caso("Registro sin datos vence a los 2000 ms", [] {
            Entorno entorno;
            Cliente cliente(entorno.puerto());
            const auto inicio = std::chrono::steady_clock::now();
            esperarErrorYCierre(cliente, "not_registered");
            comprobarPlazo(inicio);
        });

        pruebas.caso("Fragmentos de registro no renuevan el plazo", [] {
            Entorno entorno;
            Cliente cliente(entorno.puerto());
            const auto inicio = std::chrono::steady_clock::now();
            cliente.enviar("REGI");
            std::this_thread::sleep_for(1200ms);
            cliente.enviar("STER type=sensor");
            esperarErrorYCierre(cliente, "not_registered");
            comprobarPlazo(inicio);
        });

        pruebas.caso("Sensor registrado sin primer POST vence", [] {
            Entorno entorno;
            Cliente actuador(entorno.puerto());
            registrarActuador(actuador);
            Cliente sensor(entorno.puerto());
            registrarSensor(sensor);
            const auto inicio = std::chrono::steady_clock::now();
            actuador.esperarLinea(apagado);
            exigir(sensor.cerrado(), "Cerrar sensor sin lecturas");
            comprobarPlazo(inicio);
        });

        pruebas.caso("POST incompleto no renueva plazo; reconexion descarta fragmentos", [] {
            Entorno entorno;
            Cliente actuador(entorno.puerto());
            registrarActuador(actuador);
            Cliente sensor(entorno.puerto());
            registrarSensor(sensor);
            sensor.enviar("POST distance=45.00\n");
            actuador.esperarLinea(rojo);
            const auto inicio = std::chrono::steady_clock::now();
            sensor.enviar("POST distance=120");
            actuador.esperarLinea(apagado);
            exigir(sensor.cerrado(), "Cerrar sensor que no completa POST");
            comprobarPlazo(inicio);
            Cliente nuevo(entorno.puerto());
            registrarSensor(nuevo);
            nuevo.enviar("POST distance=100.00\n");
            actuador.esperarLinea(verde);
        });

        pruebas.caso("POST invalid mantiene la conexion activa por mas de 2000 ms", [] {
            Entorno entorno;
            Cliente actuador(entorno.puerto());
            registrarActuador(actuador);
            Cliente sensor(entorno.puerto());
            registrarSensor(sensor);
            for (int i = 0; i < 12; ++i) {
                sensor.enviar("POST distance=invalid\n");
                actuador.esperarLinea(apagado);
                std::this_thread::sleep_for(200ms);
            }
            sensor.enviar("POST distance=45.00\n");
            actuador.esperarLinea(rojo);
        });

        pruebas.caso("Limite de conexiones no afecta a los roles originales", [] {
            Entorno entorno;
            Cliente actuador(entorno.puerto());
            registrarActuador(actuador);
            Cliente sensor(entorno.puerto());
            registrarSensor(sensor);
            std::vector<std::unique_ptr<Cliente>> pendientes;
            for (int i = 0; i < 6; ++i) pendientes.emplace_back(std::make_unique<Cliente>(entorno.puerto()));
            Cliente exceso(entorno.puerto());
            exigir(exceso.cerrado(), "Cerrar novena conexion sin reservar otro rol");
            pendientes.back()->cerrar();
            Cliente reemplazo(entorno.puerto());
            reemplazo.enviar("REGISTER type=sensor\n");
            esperarErrorYCierre(reemplazo, "role_busy");
            sensor.enviar("POST distance=45.00\n");
            actuador.esperarLinea(rojo);
        });

        pruebas.caso("Detener servidor cierra ambos clientes", [] {
            Entorno entorno;
            Cliente sensor(entorno.puerto());
            registrarSensor(sensor);
            Cliente actuador(entorno.puerto());
            registrarActuador(actuador);
            entorno.detener();
            exigir(sensor.cerrado() && actuador.cerrado(), "Cerrar sockets y liberar recursos");
        });

        std::cout << pruebas.total - pruebas.fallos << " de " << pruebas.total
                  << " escenarios TCP locales aprobados.\n";
        return pruebas.fallos == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
    } catch (const std::exception& error) {
        std::cerr << "No se pudieron ejecutar las pruebas TCP: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
