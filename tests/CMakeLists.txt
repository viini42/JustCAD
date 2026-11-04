include(FetchContent)
FetchContent_Declare(Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.7.1
    GIT_SHALLOW TRUE
    FIND_PACKAGE_ARGS 3)
FetchContent_MakeAvailable(Catch2)
include(Catch)

# Um executável por camada, para que cada teste só enxergue a sua camada.
function(justcad_add_test name)
    cmake_parse_arguments(ARG "" "" "SOURCES;LIBS" ${ARGN})
    add_executable(${name} ${ARG_SOURCES})
    target_link_libraries(${name} PRIVATE ${ARG_LIBS} justcad::warnings Catch2::Catch2WithMain)
    catch_discover_tests(${name})
endfunction()

justcad_add_test(test_geom
    SOURCES geom/test_vec_point.cpp geom/test_bbox.cpp geom/test_double_kernel.cpp
    LIBS justcad::geom)

justcad_add_test(test_topo
    SOURCES topo/test_ids.cpp
    LIBS justcad::topo)
