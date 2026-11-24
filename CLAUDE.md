# CLAUDE.md — fw-blocks

## Commandes

```sh
# Configuration + build + tests
cmake --preset debug && cmake --build --preset debug && ctest --preset debug

# Contrôles de spécification (sans build)
python3 tools/gen_vectors.py --check
python3 tools/check_docs.py

# Vérification des en-têtes publics en C pur
cmake --build --preset debug --target fwb_header_c_compile

# Build avec sanitizers avant tout merge
cmake --preset asan-ubsan && cmake --build --preset asan-ubsan && ctest --preset asan-ubsan
```

Sorties des presets : `build/debug/`, `build/release/`, `build/asan-ubsan/`.

## Règles de codage prioritaires

- **Pas de malloc/calloc/realloc/free, pas de récursion, pas de VLA.**
- Toute fonction publique valide ses pointeurs et renvoie `FWB_ERR_INVALID_ARG`
  sur un pointeur nul. Pas d'`assert` : le firmware tourne en `NDEBUG`.
- Toutes les conversions entières sont explicites (`-Wconversion` est actif).
- Pas de `memcpy` d'une structure vers un tampon réseau : sérialisation
  champ par champ sur `uint8_t *` (ADR-0003).
- Retours anticipés de validation autorisés (écart MISRA 15.5, documenté).

## Style de test Unity

Un fichier par thème. L'identifiant d'exigence figure en commentaire
au-dessus de la fonction de test :

```c
/* CRC-REQ-003 */
void test_null_ptr_renvoie_invalid_arg(void) { … }
```

## Traçabilité

Mettre à jour `docs/traceability.md` dans le **même commit** que le code
qui couvre une exigence. `check_docs.py` vérifie que chaque exigence
listée dans `docs/traceability.md` est définie dans son fichier
`requirements.md` et que le compte annoncé est exact.
