add_library(JustCadGeom STATIC double_kernel.cpp scoped_tolerance.cpp)
add_library(JustCad::Geom ALIAS JustCadGeom)
target_include_directories(JustCadGeom PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(JustCadGeom
    PUBLIC JustCad::Core
    PRIVATE JustCad::CompilationOptions)
