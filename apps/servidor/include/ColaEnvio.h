#pragma once

#include <cstddef>
#include <deque>
#include <string>

// Conserva cada linea y el progreso de send(), sin perder bytes al bloquearse.
class ColaEnvio {
public:
    static constexpr std::size_t CapacidadMaxima = 4096;

    bool encolar(const char* contenido);
    const char* datos() const;
    std::size_t disponibles() const;
    bool avanzar(std::size_t enviados);
    bool vacia() const;

private:
    std::deque<std::string> mensajes_;
    std::size_t desplazamiento_ = 0;
    std::size_t pendientes_ = 0;
};
