add_executable(pruebas_control_distancia control_distancia.cpp)
target_link_libraries(pruebas_control_distancia PRIVATE Compartido)

add_test(NAME control_distancia COMMAND pruebas_control_distancia)

add_executable(pruebas_protocolo_aplicacion protocolo_aplicacion.cpp)
target_link_libraries(pruebas_protocolo_aplicacion PRIVATE Compartido)
add_test(NAME protocolo_aplicacion COMMAND pruebas_protocolo_aplicacion)

add_executable(pruebas_receptor_lineas receptor_lineas.cpp)
target_link_libraries(pruebas_receptor_lineas PRIVATE Compartido)
add_test(NAME receptor_lineas COMMAND pruebas_receptor_lineas)

if(WIN32)
    add_executable(pruebas_cola_envio cola_envio.cpp)
    target_link_libraries(pruebas_cola_envio PRIVATE ServidorTcp)
    add_test(NAME cola_envio COMMAND pruebas_cola_envio)

    add_executable(pruebas_servidor_tcp servidor_tcp.cpp)
    target_link_libraries(pruebas_servidor_tcp PRIVATE ServidorTcp)
    add_test(NAME servidor_tcp COMMAND pruebas_servidor_tcp)
    set_tests_properties(servidor_tcp PROPERTIES TIMEOUT 90)
endif()
