# Unity, framework de test unitaire en C.
#
# Recupere par FetchContent avec une version epinglee : une version flottante
# rendrait la CI non reproductible, et un echec de test ne serait plus
# attribuable au seul code du projet.
#
# Unity est compile SANS fwb::warnings : nos options sont volontairement
# severes et une dependance externe n'a pas a y satisfaire.

include_guard(GLOBAL)
include(FetchContent)

set(FWB_UNITY_TAG "v2.6.0" CACHE STRING "Version d'Unity a utiliser")

FetchContent_Declare(unity
    GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
    GIT_TAG        ${FWB_UNITY_TAG}
    GIT_SHALLOW    TRUE
    EXCLUDE_FROM_ALL)

FetchContent_MakeAvailable(unity)

if(NOT TARGET unity::unity)
    if(TARGET unity)
        add_library(unity::unity ALIAS unity)
    else()
        message(FATAL_ERROR "Unity recupere mais sa cible est introuvable")
    endif()
endif()

# UNITY_INCLUDE_DOUBLE : les comparaisons de flottants double precision ne
# sont pas compilees par defaut. Nous n'en avons pas besoin aujourd'hui, mais
# l'activer maintenant evite une surprise au premier test qui en aura besoin.
if(TARGET unity)
    target_compile_definitions(unity PUBLIC UNITY_INCLUDE_DOUBLE)
endif()
