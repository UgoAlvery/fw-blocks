# fwb_crc — CRC parametrable, modele de Rocksoft. Voir docs/requirements.md.

fwb_add_brick(NAME crc
        DEPENDS fwb::common
        SOURCES
        src/crc.c
        src/crc_bitwise.c
        src/crc_params.c
        src/crc_table.c
)

# Les noms de fichiers correspondent a la colonne « Test prevu » de
# docs/traceability.md. Decommenter au fur et a mesure de l'implementation.
fwb_add_brick_tests(BRICK crc
    SOURCES
    tests/unit/test_crc_example.c
    tests/unit/test_crc_checkvalues.c
    tests/unit/test_crc_model.c
    tests/unit/test_crc_incremental.c
    tests/unit/test_crc_errors.c
    tests/unit/test_crc_impl_equiv.c
    tests/unit/test_crc_api.c
)

fwb_add_fuzzer(BRICK crc
    SOURCE fuzz/fuzz_crc.c
    CORPUS "${CMAKE_CURRENT_SOURCE_DIR}/fuzz/corpus")
