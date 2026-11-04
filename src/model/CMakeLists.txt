# Modelo geométrico (API pública usada pela UI). Implementação a partir do marco M3.
add_library(justcad_model INTERFACE)
add_library(justcad::model ALIAS justcad_model)
target_link_libraries(justcad_model INTERFACE justcad::euler justcad::geom)
