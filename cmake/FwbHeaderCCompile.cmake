# Garde-fou : chaque en-tete public doit etre du C valide et auto-suffisant.
#
# Deux defauts que rien d'autre ne detecte :
#   1. un en-tete qui a cesse d'etre du C valide, invisible si tous les tests
#      qui l'incluent sont compiles en C++ ;
#   2. un en-tete qui ne compile pas seul, parce qu'il depend d'un autre
#      en-tete inclus avant lui par hasard chez l'appelant.
#
# La cible genere un .c par en-tete, qui l'inclut deux fois : la seconde
# inclusion verifie en prime que la garde d'inclusion fonctionne.
#
# Voir ADR-0001 et docs/conventions.md.

include_guard(GLOBAL)

function(fwb_add_header_c_compile_target)
    file(GLOB_RECURSE _headers CONFIGURE_DEPENDS
         "${CMAKE_SOURCE_DIR}/*/include/fwb/*.h")

    if(NOT _headers)
        # Stade v0.0-spec : aucun en-tete public n'existe encore. On declare
        # quand meme une cible, pour que la CI puisse l'invoquer sans condition.
        add_custom_target(fwb_header_c_compile
            COMMAND ${CMAKE_COMMAND} -E echo
                    "Aucun en-tete public : rien a verifier.")
        return()
    endif()

    set(_gen_dir "${CMAKE_BINARY_DIR}/header_c_compile")
    file(MAKE_DIRECTORY "${_gen_dir}")
    set(_stubs "")

    foreach(_hdr IN LISTS _headers)
        file(RELATIVE_PATH _rel "${CMAKE_SOURCE_DIR}" "${_hdr}")
        string(REGEX REPLACE "[/\\.]" "_" _slug "${_rel}")
        set(_stub "${_gen_dir}/${_slug}.c")

        # On inclut par chemin absolu : la cible ne doit dependre d'aucun
        # repertoire d'inclusion, puisqu'elle verifie justement que l'en-tete
        # se suffit a lui-meme.
        file(WRITE "${_stub}"
             "/* Genere par FwbHeaderCCompile.cmake — ne pas editer. */\n"
             "#include \"${_hdr}\"\n"
             "#include \"${_hdr}\"\n")
        list(APPEND _stubs "${_stub}")
    endforeach()

    add_library(fwb_header_c_compile OBJECT ${_stubs})
    target_link_libraries(fwb_header_c_compile PRIVATE fwb::warnings)
    set_target_properties(fwb_header_c_compile PROPERTIES
        C_STANDARD 11
        C_EXTENSIONS OFF)

    list(LENGTH _headers _n)
    message(STATUS "Verification C des en-tetes publics : ${_n} fichier(s)")
endfunction()
