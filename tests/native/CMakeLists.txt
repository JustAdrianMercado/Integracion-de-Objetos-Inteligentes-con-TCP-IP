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
    "${PROJECT_SOURCE_DIR}/apps/firmware/src/SensorUltrasonico.cpp"
)
target_include_directories(pruebas_sensor_ultrasonico PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/fakes"
    "${PROJECT_SOURCE_DIR}/apps/firmware/include"
)
add_test(NAME sensor_ultrasonico COMMAND pruebas_sensor_ultrasonico)

find_package(Python3 3.13 QUIET COMPONENTS Interpreter)
if(Python3_FOUND)
    add_test(NAME servidor_python
        COMMAND "${Python3_EXECUTABLE}" -m unittest discover -s tests/python -v
    )
    set_tests_properties(servidor_python PROPERTIES
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        ENVIRONMENT "IOT_CONTRACT_EXE=$<TARGET_FILE:verificar_contrato>"
        TIMEOUT 90
    )
else()
    message(STATUS "Python 3.13 no encontrado; ejecutar unittest con Python 3.13 por separado.")
endif()
