# A topologia não depende de geom: só guarda IDs opacos para a geometria.
add_library(justcad_topo INTERFACE)
add_library(justcad::topo ALIAS justcad_topo)
target_include_directories(justcad_topo INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(justcad_topo INTERFACE cxx_std_23)
