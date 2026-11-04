cmake_minimum_required(VERSION 3.25)

project(JustCAD
    VERSION 0.1.0
    DESCRIPTION "2D modeler built on half-edges and Euler operators"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(JUSTCAD_BUILD_TESTS "Build the unit tests" ON)
option(JUSTCAD_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

include(cmake/CompilationOptions.cmake)

# Layers, each one a library; dependencies only point downwards:
#   Core  : assertions and other utilities shared by every layer
#   Geom  : pure geometry + numeric kernel (knows nothing about topology)
#   Topo  : half-edge data structure (knows nothing about geometry)
#   Euler : low- and high-level Euler operators   -> Topo, Geom
#   Model : geometric model (public API)          -> Euler, Geom
#   App   : SDL3 + Dear ImGui UI                  -> Model   (milestone M4)
add_subdirectory(src/core)
add_subdirectory(src/geom)
add_subdirectory(src/topo)
add_subdirectory(src/euler)
add_subdirectory(src/model)

if(JUSTCAD_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
