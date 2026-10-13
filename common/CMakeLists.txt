# fwb_common — codes d'erreur, helpers d'endianness, assertions statiques.
# Aucune logique. Voir README.md et docs/architecture.md.

fwb_add_brick(NAME common
    SOURCES
        # src/status.c
)

fwb_add_brick_tests(BRICK common
    SOURCES
        # tests/unit/test_endian.c
        # tests/unit/test_status.c
)
