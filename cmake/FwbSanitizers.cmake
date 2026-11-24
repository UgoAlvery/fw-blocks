# Sanitizers, portes par une cible INTERFACE.
#
# Ils doivent etre actifs a la compilation ET a l'edition de liens, d'ou
# l'usage de target_link_options en plus de target_compile_options.

add_library(fwb_sanitizers INTERFACE)
add_library(fwb::sanitizers ALIAS fwb_sanitizers)

# Visibilite IDE : ajoute les sources au projet des qu'elles existent.
foreach(_src
        ../crc/include/fwb/crc/crc.h
        ../crc/src/crc_internal.h
        ../common/include/fwb/fwb_common.h
        ../crc/tests/unit/test_crc_example.c
        ../crc/tests/unit/test_crc_impl_equiv.c
        ../crc/tests/unit/test_crc_api.c)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_src}")
        target_sources(fwb_sanitizers INTERFACE "${_src}")
    endif()
endforeach()
unset(_src)

if(FWB_SANITIZERS)
    list(JOIN FWB_SANITIZERS "," _fwb_san_list)
    set(_fwb_san_flags -fsanitize=${_fwb_san_list} -fno-omit-frame-pointer)

    if("undefined" IN_LIST FWB_SANITIZERS)
        # Sans cette option, UBSan signale sur stderr et poursuit : un
        # comportement indefini ne ferait pas echouer le test.
        list(APPEND _fwb_san_flags -fno-sanitize-recover=all)
    endif()

    target_compile_options(fwb_sanitizers INTERFACE ${_fwb_san_flags})
    target_link_options(fwb_sanitizers INTERFACE ${_fwb_san_flags})

    message(STATUS "Sanitizers actifs : ${_fwb_san_list}")
    unset(_fwb_san_list)
    unset(_fwb_san_flags)
endif()
