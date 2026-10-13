cmake_minimum_required(VERSION 3.20)

# Pruebas en la PC: firmware C++ con WiFi, reloj y GPIO simulados, y la suite Python.
project(Practica2TcpIp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests/native)
endif()
