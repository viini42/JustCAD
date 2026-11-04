add_library(justcad_geom INTERFACE)
add_library(justcad::geom ALIAS justcad_geom)
target_include_directories(justcad_geom INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(justcad_geom INTERFACE cxx_std_23)
