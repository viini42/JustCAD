cmake_minimum_required(VERSION 3.25)

project(JustCAD
    VERSION 0.1.0
    DESCRIPTION "2D modeler built on half-edges and Euler operators"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(JCAD_BUILD_TESTS "Build the unit tests" ON)
option(JCAD_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

include(cmake/CompilationOptions.cmake)

add_subdirectory(src)

if(JCAD_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
