# Sanitizers, portes par une cible INTERFACE.
#
# Ils doivent etre actifs a la compilation ET a l'edition de liens, d'ou
# l'usage de target_link_options en plus de target_compile_options.

add_library(fwb_sanitizers INTERFACE)
add_library(fwb::sanitizers ALIAS fwb_sanitizers)

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
