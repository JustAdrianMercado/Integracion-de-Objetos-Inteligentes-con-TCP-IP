#pragma once

#include <cstddef>

constexpr int MSG_DONTWAIT = 0x40;
int send(int socket, const void* datos, std::size_t longitud, int flags);
int recv(int socket, void* datos, std::size_t longitud, int flags);
