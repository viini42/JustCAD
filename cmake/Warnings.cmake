add_library(justcad_warnings INTERFACE)
add_library(justcad::warnings ALIAS justcad_warnings)

if(MSVC)
    target_compile_options(justcad_warnings INTERFACE /W4 /permissive-)
    if(JUSTCAD_WARNINGS_AS_ERRORS)
        target_compile_options(justcad_warnings INTERFACE /WX)
    endif()
else()
    target_compile_options(justcad_warnings INTERFACE
        -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion
        -Wnon-virtual-dtor -Wold-style-cast -Woverloaded-virtual)
    if(JUSTCAD_WARNINGS_AS_ERRORS)
        target_compile_options(justcad_warnings INTERFACE -Werror)
    endif()
endif()
