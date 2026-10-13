add_library(Compartido STATIC
    src/ControlDistancia.cpp
    src/ProtocoloAplicacion.cpp
    src/ReceptorLineas.cpp
)

target_include_directories(Compartido PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_features(Compartido PUBLIC cxx_std_11)
