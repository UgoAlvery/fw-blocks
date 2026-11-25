# fwb_antireplay — fenetre glissante sur numero de sequence.
# Ne depend PAS de fwb_framing : voir AR-REQ-003.

fwb_add_brick(NAME antireplay
    DEPENDS fwb::common
    SOURCES
        src/antireplay.c
)

fwb_add_brick_tests(BRICK antireplay
    SOURCES
        tests/unit/test_ar_api.c
        tests/unit/test_ar_seed.c
        tests/unit/test_ar_window.c
        tests/unit/test_ar_modular.c
)

fwb_add_fuzzer(BRICK antireplay
    SOURCE fuzz/fuzz_antireplay.c
    CORPUS "${CMAKE_CURRENT_SOURCE_DIR}/fuzz/corpus")
