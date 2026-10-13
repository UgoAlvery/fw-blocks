cmake_minimum_required(VERSION 3.20)

project(fw-blocks
        VERSION 0.0.0
        DESCRIPTION "Briques logicielles reutilisables pour firmware embarque"
        LANGUAGES C)

# Le C++ n'est active que si un wrapper ou un harness de fuzzing en a besoin.
# Voir ADR-0001 : le wrapper n'est pas ecrit tant que l'API C n'est pas stable.

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)          # -std=c11, pas -std=gnu11
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

# ---------------------------------------------------------------- options

option(FWB_BUILD_TESTS    "Construire les tests unitaires"            ON)
option(FWB_BUILD_FUZZERS  "Construire les harness de fuzzing"         OFF)
option(FWB_BUILD_EXAMPLES "Construire les exemples"                   ON)
option(FWB_WERROR         "Traiter les avertissements comme erreurs"  ON)
option(FWB_SPEC_CHECKS    "Enregistrer les controles de specification" ON)

set(FWB_SANITIZERS "" CACHE STRING
    "Sanitizers a activer, separes par des points-virgules : address;undefined")

include(FwbWarnings)
include(FwbSanitizers)
include(FwbBrick)
include(FwbHeaderCCompile)

if(FWB_BUILD_TESTS OR FWB_SPEC_CHECKS)
    enable_testing()
endif()

if(FWB_BUILD_TESTS)
    include(FwbUnity)
endif()

# ---------------------------------------------------------------- briques
#
# L'ordre respecte le graphe de dependances de docs/architecture.md :
#   common <- crc <- framing
#   common <- antireplay

add_subdirectory(common)
add_subdirectory(crc)
add_subdirectory(framing)
add_subdirectory(antireplay)

# ---------------------------------------------------- controles de specification
#
# Ils tournent meme sans code metier : c'est ce qui rend la CI utile au stade
# v0.0-spec. Voir docs/test-plan.md.

if(FWB_SPEC_CHECKS)
    add_subdirectory(tools)
endif()

# ------------------------------------------- garde-fou : en-tetes valides en C
#
# Compile chaque en-tete public dans un .c minimal. Sans cela, un en-tete
# devenu invalide en C passerait inapercu des lors que du C++ l'inclut.
# Voir ADR-0001 et docs/conventions.md.

fwb_add_header_c_compile_target()

# ---------------------------------------------------------------- recapitulatif

message(STATUS "")
message(STATUS "fw-blocks ${PROJECT_VERSION} — recapitulatif")
message(STATUS "  type de build    : ${CMAKE_BUILD_TYPE}")
message(STATUS "  compilateur C    : ${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}")
message(STATUS "  sanitizers       : ${FWB_SANITIZERS}")
message(STATUS "  tests            : ${FWB_BUILD_TESTS}")
message(STATUS "  fuzzers          : ${FWB_BUILD_FUZZERS}")
message(STATUS "  controles de spec: ${FWB_SPEC_CHECKS}")
message(STATUS "  avertissements   : erreurs = ${FWB_WERROR}")
message(STATUS "")
