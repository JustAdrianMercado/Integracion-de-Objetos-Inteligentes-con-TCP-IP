add_executable(pruebas_control_distancia control_distancia.cpp)
target_link_libraries(pruebas_control_distancia PRIVATE Compartido)

add_test(NAME control_distancia COMMAND pruebas_control_distancia)

add_executable(pruebas_protocolo_aplicacion protocolo_aplicacion.cpp)
target_link_libraries(pruebas_protocolo_aplicacion PRIVATE Compartido)
add_test(NAME protocolo_aplicacion COMMAND pruebas_protocolo_aplicacion)

add_executable(pruebas_receptor_lineas receptor_lineas.cpp)
target_link_libraries(pruebas_receptor_lineas PRIVATE Compartido)
add_test(NAME receptor_lineas COMMAND pruebas_receptor_lineas)

add_executable(verificar_contrato verificar_contrato.cpp)
target_link_libraries(verificar_contrato PRIVATE Compartido)

add_executable(pruebas_sensor_ultrasonico
    sensor_ultrasonico.cpp
    "${PROJECT_SOURCE_DIR}/apps/firmware/src/cliente/SensorUltrasonico.cpp"
)
target_include_directories(pruebas_sensor_ultrasonico PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/fakes"
    "${PROJECT_SOURCE_DIR}/apps/firmware/include"
)
add_test(NAME sensor_ultrasonico COMMAND pruebas_sensor_ultrasonico)

add_library(red_simulada STATIC fakes/RedSimulada.cpp)
target_include_directories(red_simulada PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/fakes")

add_library(cliente_tcp_simulado STATIC
    "${PROJECT_SOURCE_DIR}/apps/firmware/src/cliente/ClienteTcp.cpp"
)
target_include_directories(cliente_tcp_simulado PUBLIC "${PROJECT_SOURCE_DIR}/apps/firmware/include")
target_link_libraries(cliente_tcp_simulado PUBLIC Compartido red_simulada)

add_executable(pruebas_cliente_tcp cliente_tcp.cpp)
target_link_libraries(pruebas_cliente_tcp PRIVATE cliente_tcp_simulado)
add_test(NAME cliente_tcp COMMAND pruebas_cliente_tcp)

add_library(firmware_sensor_simulado STATIC
    "${PROJECT_SOURCE_DIR}/apps/firmware/src/cliente/PrincipalSensor.cpp"
    "${PROJECT_SOURCE_DIR}/apps/firmware/src/cliente/SensorUltrasonico.cpp"
)
target_include_directories(firmware_sensor_simulado BEFORE PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/fakes/configuracion")
target_link_libraries(firmware_sensor_simulado PUBLIC cliente_tcp_simulado)

add_executable(pruebas_principal_sensor principal_sensor.cpp)
target_link_libraries(pruebas_principal_sensor PRIVATE firmware_sensor_simulado)
add_test(NAME principal_sensor COMMAND pruebas_principal_sensor)

add_executable(puente_sensor puente_sensor.cpp)
target_link_libraries(puente_sensor PRIVATE firmware_sensor_simulado)

add_library(controlador_leds_simulado STATIC
    "${PROJECT_SOURCE_DIR}/apps/firmware/src/cliente/ControladorLeds.cpp"
)
target_include_directories(controlador_leds_simulado PUBLIC "${PROJECT_SOURCE_DIR}/apps/firmware/include")
target_link_libraries(controlador_leds_simulado PUBLIC Compartido red_simulada)

add_executable(pruebas_controlador_leds controlador_leds.cpp)
target_link_libraries(pruebas_controlador_leds PRIVATE controlador_leds_simulado)
add_test(NAME controlador_leds COMMAND pruebas_controlador_leds)

add_library(firmware_actuador_simulado STATIC
    "${PROJECT_SOURCE_DIR}/apps/firmware/src/cliente/PrincipalActuador.cpp"
)
target_include_directories(firmware_actuador_simulado BEFORE PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/fakes/configuracion")
target_link_libraries(firmware_actuador_simulado PUBLIC cliente_tcp_simulado controlador_leds_simulado)

add_executable(pruebas_principal_actuador principal_actuador.cpp)
target_link_libraries(pruebas_principal_actuador PRIVATE firmware_actuador_simulado)
add_test(NAME principal_actuador COMMAND pruebas_principal_actuador)

add_executable(puente_actuador puente_actuador.cpp)
target_link_libraries(puente_actuador PRIVATE firmware_actuador_simulado)

find_package(Python3 3.13 QUIET COMPONENTS Interpreter)
if(Python3_FOUND)
    add_test(NAME servidor_python
        COMMAND "${Python3_EXECUTABLE}" -m unittest discover -s tests/python -v
    )
    set_tests_properties(servidor_python PROPERTIES
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        ENVIRONMENT "IOT_CONTRACT_EXE=$<TARGET_FILE:verificar_contrato>;IOT_SENSOR_EXE=$<TARGET_FILE:puente_sensor>;IOT_ACTUATOR_EXE=$<TARGET_FILE:puente_actuador>"
        TIMEOUT 90
    )
else()
    message(STATUS "Python 3.13 no encontrado; ejecutar unittest con Python 3.13 por separado.")
endif()
