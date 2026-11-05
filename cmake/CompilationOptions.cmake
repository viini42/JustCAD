add_library(JustCadCompilationOptions INTERFACE)
add_library(JustCad::CompilationOptions ALIAS JustCadCompilationOptions)

if(MSVC)
    target_compile_options(JustCadCompilationOptions INTERFACE /W4 /permissive- /utf-8)
    if(JCAD_WARNINGS_AS_ERRORS)
        target_compile_options(JustCadCompilationOptions INTERFACE /WX)
    endif()
else()
    target_compile_options(JustCadCompilationOptions INTERFACE
        -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion
        -Wnon-virtual-dtor -Wold-style-cast -Woverloaded-virtual)
    if(JCAD_WARNINGS_AS_ERRORS)
        target_compile_options(JustCadCompilationOptions INTERFACE -Werror)
    endif()
endif()
