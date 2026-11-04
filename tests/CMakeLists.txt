include(FetchContent)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_Declare(GoogleTest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.2
    GIT_SHALLOW TRUE
    SYSTEM
    FIND_PACKAGE_ARGS NAMES GTest)
FetchContent_MakeAvailable(GoogleTest)
include(GoogleTest)

# One executable per layer, so each test only sees its own layer.
function(JustCadAddTests name)
    cmake_parse_arguments(ARG "" "" "SOURCES;LIBS" ${ARGN})
    add_executable(${name} ${ARG_SOURCES})
    target_link_libraries(${name} PRIVATE ${ARG_LIBS} JustCad::CompilationOptions GTest::gtest_main)
    gtest_discover_tests(${name})
endfunction()

JustCadAddTests(CoreTests
    SOURCES test_assert.cpp
    LIBS JustCad::Core)

JustCadAddTests(GeomTests
    SOURCES test_vec2.cpp test_point2.cpp test_bbox.cpp test_double_kernel.cpp test_scoped_tolerance.cpp
    LIBS JustCad::Geom)

JustCadAddTests(TopoTests
    SOURCES test_id.cpp
    LIBS JustCad::Topo)
