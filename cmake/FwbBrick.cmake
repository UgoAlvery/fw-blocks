# Helpers de declaration d'une brique, de ses tests et de ses fuzzers.
#
# Particularite du stade v0.0-spec : une brique dont la liste de sources est
# vide est declaree en bibliotheque INTERFACE plutot qu'en bibliotheque
# statique. CMake refuse une bibliotheque statique sans source, et nous voulons
# que la CI soit verte avant la premiere ligne de code metier. La bascule vers
# une bibliotheque statique est automatique des qu'une source est ajoutee :
# rien a modifier dans le CMakeLists de la brique.

include_guard(GLOBAL)

# fwb_add_brick(NAME <nom> [SOURCES ...] [DEPENDS ...])
function(fwb_add_brick)
    cmake_parse_arguments(ARG "" "NAME" "SOURCES;DEPENDS" ${ARGN})
    if(NOT ARG_NAME)
        message(FATAL_ERROR "fwb_add_brick : NAME est obligatoire")
    endif()

    set(_target "fwb_${ARG_NAME}")
    set(_inc "${CMAKE_CURRENT_SOURCE_DIR}/include")

    if(ARG_SOURCES)
        add_library(${_target} STATIC ${ARG_SOURCES})
        target_include_directories(${_target} PUBLIC ${_inc})
        target_link_libraries(${_target}
            PUBLIC  ${ARG_DEPENDS}
            PRIVATE fwb::warnings fwb::sanitizers)
        set_property(GLOBAL APPEND PROPERTY FWB_IMPLEMENTED_BRICKS ${ARG_NAME})
    else()
        add_library(${_target} INTERFACE)
        target_include_directories(${_target} INTERFACE ${_inc})
        if(ARG_DEPENDS)
            target_link_libraries(${_target} INTERFACE ${ARG_DEPENDS})
        endif()
        message(STATUS "Brique ${ARG_NAME} : specifiee, pas encore implementee")
    endif()

    add_library(fwb::${ARG_NAME} ALIAS ${_target})
    set_property(GLOBAL APPEND PROPERTY FWB_ALL_BRICKS ${ARG_NAME})
endfunction()

# fwb_add_brick_tests(BRICK <nom> SOURCES <fichiers de test>)
#
# Un executable par fichier de test, pour qu'un plantage dans un test
# n'emporte pas les autres : sur du code qui manipule de la memoire, un test
# qui segfault est un resultat courant pendant le developpement.
function(fwb_add_brick_tests)
    cmake_parse_arguments(ARG "" "BRICK" "SOURCES" ${ARGN})
    if(NOT FWB_BUILD_TESTS OR NOT ARG_SOURCES)
        return()
    endif()

    foreach(_src IN LISTS ARG_SOURCES)
        get_filename_component(_name "${_src}" NAME_WE)
        set(_exe "${ARG_BRICK}_${_name}")

        add_executable(${_exe} "${_src}")
        target_link_libraries(${_exe}
            PRIVATE fwb::${ARG_BRICK} unity::unity fwb::warnings fwb::sanitizers)

        add_test(NAME ${_exe} COMMAND ${_exe})
        set_tests_properties(${_exe} PROPERTIES
            LABELS "unit;${ARG_BRICK}"
            TIMEOUT 60)
    endforeach()
endfunction()

# fwb_add_fuzzer(BRICK <nom> SOURCE <fichier> [CORPUS <repertoire>])
#
# libFuzzer n'existe que sous Clang. Sous un autre compilateur, la cible est
# silencieusement ignoree plutot que de faire echouer la configuration.
function(fwb_add_fuzzer)
    cmake_parse_arguments(ARG "" "BRICK;SOURCE;CORPUS" "" ${ARGN})
    if(NOT FWB_BUILD_FUZZERS OR NOT ARG_SOURCE)
        return()
    endif()
    if(NOT CMAKE_C_COMPILER_ID MATCHES "Clang")
        message(STATUS "Fuzzing ignore : libFuzzer requiert Clang")
        return()
    endif()

    get_filename_component(_name "${ARG_SOURCE}" NAME_WE)
    add_executable(${_name} "${ARG_SOURCE}")
    target_link_libraries(${_name} PRIVATE fwb::${ARG_BRICK} fwb::warnings)
    target_compile_options(${_name} PRIVATE -fsanitize=fuzzer,address,undefined
                                            -fno-sanitize-recover=all)
    target_link_options(${_name}    PRIVATE -fsanitize=fuzzer,address,undefined)

    # En CI, le fuzzer est execute sur le corpus versionne uniquement :
    # -runs=0 rejoue le corpus sans exploration. Une regression sur le corpus
    # est bloquante, une nouvelle decouverte ne l'est pas (docs/test-plan.md).
    if(ARG_CORPUS AND EXISTS "${ARG_CORPUS}")
        add_test(NAME ${_name}_corpus
                 COMMAND ${_name} -runs=0 "${ARG_CORPUS}")
        set_tests_properties(${_name}_corpus PROPERTIES
            LABELS "fuzz-corpus;${ARG_BRICK}"
            TIMEOUT 300)
    endif()
endfunction()
