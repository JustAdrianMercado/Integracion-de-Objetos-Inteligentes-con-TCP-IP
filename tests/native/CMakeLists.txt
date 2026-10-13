set(FIRMWARE "${PROJECT_SOURCE_DIR}/apps/firmware")

# Biblioteca compartida por ambos ESP32: tipos, protocolo y receptor de lineas.
add_library(Compartido STATIC
    "${FIRMWARE}/lib/Compartido/ProtocoloAplicacion.cpp"
    "${FIRMWARE}/lib/Compartido/ReceptorLineas.cpp"
)
target_include_directories(Compartido PUBLIC "${FIRMWARE}/lib/Compartido")

# Codigo del firmware compilado contra los fakes de Arduino, WiFi y sockets.
add_library(firmware_simulado STATIC
    fakes/RedSimulada.cpp
    "${FIRMWARE}/src/ClienteTcp.cpp"
    "${FIRMWARE}/src/ControladorLeds.cpp"
    "${FIRMWARE}/src/SensorUltrasonico.cpp"
)
target_include_directories(firmware_simulado PUBLIC fakes "${FIRMWARE}/include")
target_link_libraries(firmware_simulado PUBLIC Compartido)

# Cada principal define setup() y loop(), por eso se compila en su propio ejecutable.
function(prueba_principal nombre principal)
    add_executable(pruebas_${nombre} ${nombre}.cpp "${FIRMWARE}/src/${principal}")
    target_include_directories(pruebas_${nombre} BEFORE PRIVATE fakes/configuracion)
    target_link_libraries(pruebas_${nombre} PRIVATE firmware_simulado)
    add_test(NAME ${nombre} COMMAND pruebas_${nombre})
endfunction()

foreach(prueba protocolo_aplicacion receptor_lineas cliente_tcp controlador_leds)
    add_executable(pruebas_${prueba} ${prueba}.cpp)
    target_link_libraries(pruebas_${prueba} PRIVATE firmware_simulado)
    add_test(NAME ${prueba} COMMAND pruebas_${prueba})
endforeach()

prueba_principal(principal_sensor PrincipalSensor.cpp)
prueba_principal(principal_actuador PrincipalActuador.cpp)

find_package(Python3 3.13 QUIET COMPONENTS Interpreter)
if(Python3_FOUND)
    add_test(NAME servidor_python
        COMMAND "${Python3_EXECUTABLE}" -m unittest discover -s tests/python -v
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    )
    set_tests_properties(servidor_python PROPERTIES TIMEOUT 90)
else()
    message(STATUS "Python 3.13 no encontrado; ejecutar unittest con Python 3.13 por separado.")
endif()
