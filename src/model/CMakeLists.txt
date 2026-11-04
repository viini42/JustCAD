# Geometric model (public API used by the UI). Implemented from milestone M3 on.
add_library(JustCadModel INTERFACE)
add_library(JustCad::Model ALIAS JustCadModel)
target_link_libraries(JustCadModel INTERFACE JustCad::Euler JustCad::Geom)
