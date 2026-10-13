add_library(ServidorTcp STATIC
    src/ColaEnvio.cpp
    src/ServidorTcp.cpp
)
target_include_directories(ServidorTcp PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_definitions(ServidorTcp PUBLIC WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(ServidorTcp PUBLIC Compartido ws2_32)

add_executable(servidor_tcp src/main.cpp)
target_link_libraries(servidor_tcp PRIVATE ServidorTcp)
