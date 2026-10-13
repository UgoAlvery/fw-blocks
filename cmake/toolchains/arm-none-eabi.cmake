# Toolchain ARM bare-metal, fournie comme point de depart.
#
# NON VERIFIEE : aucune cible materielle n'est testee en CI, et ce fichier
# n'a pas ete exerce. Il documente l'intention, pas un resultat. Le dire
# vaut mieux que de laisser croire a un support qui n'existe pas.
#
# Usage :
#   cmake -B build/arm \
#         -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake \
#         -DFWB_BUILD_TESTS=OFF -DFWB_SPEC_CHECKS=OFF
#
# Les tests sont desactives : ils s'executent sur hote, pas sur cible. Les
# briques sont concues pour etre integralement testables sur PC
# (docs/test-plan.md §6), la build ARM ne sert qu'a verifier la compilation
# croisee et a mesurer l'empreinte.

set(CMAKE_SYSTEM_NAME      Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(FWB_ARM_CPU   "cortex-m4" CACHE STRING "Coeur cible")
set(FWB_ARM_FPU   "fpv4-sp-d16" CACHE STRING "Unite flottante, ou 'none'")
set(FWB_ARM_FLOAT "hard" CACHE STRING "ABI flottante : soft, softfp, hard")

find_program(ARM_GCC arm-none-eabi-gcc REQUIRED)
find_program(ARM_OBJCOPY arm-none-eabi-objcopy)
get_filename_component(_arm_prefix "${ARM_GCC}" DIRECTORY)

set(CMAKE_C_COMPILER   "${ARM_GCC}")
set(CMAKE_ASM_COMPILER "${ARM_GCC}")

set(_arm_flags "-mcpu=${FWB_ARM_CPU} -mthumb")
if(NOT FWB_ARM_FPU STREQUAL "none")
    string(APPEND _arm_flags " -mfpu=${FWB_ARM_FPU} -mfloat-abi=${FWB_ARM_FLOAT}")
endif()

# -ffunction-sections et -fdata-sections, avec --gc-sections a l'edition de
# liens : seules les fonctions reellement appelees sont embarquees. C'est ce
# qui rend acceptable une bibliotheque qui expose plus de variantes que
# l'application n'en utilise, notamment pour les tables de CRC.
string(APPEND _arm_flags " -ffunction-sections -fdata-sections")

set(CMAKE_C_FLAGS_INIT "${_arm_flags}")
set(CMAKE_EXE_LINKER_FLAGS_INIT
    "${_arm_flags} -Wl,--gc-sections -Wl,--print-memory-usage --specs=nano.specs")

# Sans cela, CMake tente de produire un executable complet pour tester le
# compilateur, ce qui echoue faute de script d'edition de liens.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_FIND_ROOT_PATH "${_arm_prefix}/..")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
