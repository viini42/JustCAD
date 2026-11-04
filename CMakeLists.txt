cmake_minimum_required(VERSION 3.25)

project(JustCAD
    VERSION 0.1.0
    DESCRIPTION "Modelador 2D baseado em half-edges e operadores de Euler"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(JUSTCAD_BUILD_TESTS "Compila os testes" ON)
option(JUSTCAD_WARNINGS_AS_ERRORS "Trata warnings como erros" OFF)

include(cmake/Warnings.cmake)

# Camadas (cada uma é uma biblioteca; dependências só para baixo):
#   geom   : geometria pura + kernel numérico (não conhece topologia)
#   topo   : estrutura half-edge (não conhece geometria)
#   euler  : operadores de Euler de baixo e alto nível  -> topo, geom
#   model  : modelo geométrico (API pública)            -> euler, geom
#   app    : UI SDL3 + Dear ImGui                       -> model   (marco M4)
add_subdirectory(src/geom)
add_subdirectory(src/topo)
add_subdirectory(src/euler)
add_subdirectory(src/model)

if(JUSTCAD_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
