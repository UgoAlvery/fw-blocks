# Avertissements du projet.
#
# Portes par une cible INTERFACE plutot que par des variables globales : les
# dependances externes (Unity, par exemple) ne doivent pas etre compilees avec
# -Werror, sinon une mise a jour de leur part casse notre build sans que nous
# ayons rien change.

add_library(fwb_warnings INTERFACE)
add_library(fwb::warnings ALIAS fwb_warnings)

set(_fwb_c_warnings
    -Wall
    -Wextra
    -Wconversion          # conversion implicite entre types entiers
    -Wsign-conversion
    -Wshadow
    -Wpointer-arith
    -Wcast-qual           # retrait silencieux d'un const
    -Wcast-align
    -Wstrict-prototypes
    -Wmissing-prototypes
    -Wredundant-decls
    -Wundef               # #if sur une macro non definie
    -Wswitch-enum         # cas d'enumeration oublie dans un switch
    -Wvla                 # tableau a longueur variable, interdit par conventions.md
    -Wdouble-promotion
    -Wformat=2)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    list(APPEND _fwb_c_warnings
         -Wjump-misses-init
         -Wlogical-op
         -Wduplicated-cond
         -Wduplicated-branches)
elseif(CMAKE_C_COMPILER_ID MATCHES "Clang")
    list(APPEND _fwb_c_warnings
         -Wno-unknown-warning-option)
endif()

target_compile_options(fwb_warnings INTERFACE
    $<$<COMPILE_LANGUAGE:C>:${_fwb_c_warnings}>)

if(FWB_WERROR)
    target_compile_options(fwb_warnings INTERFACE -Werror)
endif()

unset(_fwb_c_warnings)
