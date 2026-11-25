# fwb_framing — mise en trame, delimitation COBS, recepteur en flux.
# Voir docs/wire-format.md pour le format normatif.

fwb_add_brick(NAME framing
    DEPENDS fwb::common fwb::crc
    SOURCES
        src/cobs.c
        src/encode.c
        src/decode.c
        src/rx.c
)

fwb_add_brick_tests(BRICK framing
    SOURCES
        tests/unit/test_framing_api.c
        tests/unit/test_encode.c
        tests/unit/test_decode.c
        tests/unit/test_cobs.c
        tests/unit/test_rx.c
        tests/unit/test_vectors.c
        tests/fault_injection/test_bitflip.c
        tests/fault_injection/test_truncation.c
        tests/fault_injection/test_insertion.c
)

fwb_add_fuzzer(BRICK framing
    SOURCE fuzz/fuzz_rx.c
    CORPUS "${CMAKE_CURRENT_SOURCE_DIR}/fuzz/corpus")

fwb_add_fuzzer(BRICK framing
    SOURCE fuzz/fuzz_cobs.c
    CORPUS "${CMAKE_CURRENT_SOURCE_DIR}/fuzz/corpus")

fwb_add_fuzzer(BRICK framing
    SOURCE fuzz/fuzz_rx_chunking.c
    CORPUS "${CMAKE_CURRENT_SOURCE_DIR}/fuzz/corpus")

# Les tests de vecteurs lisent tests/vectors/vectors.txt a l'execution plutot
# que de l'embarquer dans le binaire : les vecteurs restent ainsi une donnee
# unique, lisible par un humain et par la machine, et non une copie figee dans
# du code. Le chemin est passe par definition de macro.
if(FWB_BUILD_TESTS AND TARGET framing_test_vectors)
    target_compile_definitions(framing_test_vectors PRIVATE
        FWB_VECTORS_PATH="${CMAKE_CURRENT_SOURCE_DIR}/tests/vectors/vectors.txt")
endif()

if(FWB_BUILD_EXAMPLES)
    # add_subdirectory(examples)
endif()
