# fw-blocks

Briques logicielles réutilisables pour firmware embarqué, écrites avec le niveau
de rigueur attendu dans un contexte critique : spécification écrite avant le
code, exigences numérotées, traçabilité exigence → test, fuzzing des parseurs,
analyse statique en intégration continue.

Chaque brique est autonome, compilable et testable sur PC, sans dépendance
matérielle, sans allocation dynamique et sans dépendance à une horloge.

## État du dépôt

| Brique | Rôle | Spécification | Implémentation |
|---|---|---|---|
| `common` | Codes d'erreur et types partagés | ✅ | ✅ |
| `crc` | CRC paramétrable, modèle Rocksoft | ✅ | ✅ |
| `framing` | Trames délimitées et contrôlées par CRC | ✅ | ⬜ |
| `antireplay` | Fenêtre glissante sur numéro de séquence | ✅ | ⬜ |

`common` et `crc` sont implémentés et testés — 11/11 ctest sur debug, asan-ubsan
et release. `framing` et `antireplay` sont au stade `v0.0-spec` : spécification
figée, implémentation à venir. L'outillage est en place et la CI est verte.

## Ce que ce dépôt garantit

- Un format de trame spécifié octet par octet, indépendant du langage et de la
  plateforme, dans [`framing/docs/wire-format.md`](framing/docs/wire-format.md).
- Un modèle de menaces explicite, qui dit aussi ce qui **n'est pas** couvert, dans
  [`framing/docs/threat-model.md`](framing/docs/threat-model.md).
- Des décodeurs qui ne lisent jamais hors de leurs tampons, quelle que soit
  l'entrée, propriété vérifiée par fuzzing.
- Des vecteurs de test calculés à la main et vérifiés contre un oracle
  indépendant, dans [`framing/tests/vectors/`](framing/tests/vectors/).

## Ce que ce dépôt ne garantit pas

- **Aucune sécurité cryptographique en v1.** Le CRC protège contre les erreurs de
  transmission, pas contre un attaquant, qui peut le recalculer. Un emplacement
  est réservé dans le format pour un MAC, mais rien n'est implémenté.
- **Aucune certification.** Le sous-ensemble MISRA C et les pratiques de
  traçabilité s'inspirent des normes de sûreté sans prétendre à la conformité à
  l'IEC 61508 ou à l'ISO 26262.
- **Aucune primitive cryptographique maison.** Quand la sécurité entrera dans le
  périmètre, une bibliothèque éprouvée sera encapsulée, jamais réimplémentée.

## Documentation

- [`docs/architecture.md`](docs/architecture.md) — découpage en briques et graphe de dépendances
- [`docs/conventions.md`](docs/conventions.md) — règles de codage, nommage, MISRA, erreurs
- [`docs/traceability.md`](docs/traceability.md) — matrice exigence → code → test
- [`docs/adr/`](docs/adr/) — décisions d'architecture et leurs justifications

## Construction

```sh
cmake --preset debug && cmake --build --preset debug && ctest --preset debug
```

| Preset | Usage |
|---|---|
| `debug` | `-O0 -g3`, tests unitaires |
| `release` | `-Os -DNDEBUG`, représentatif d'une build embarquée |
| `asan-ubsan` | AddressSanitizer et UndefinedBehaviorSanitizer |
| `fuzz` | Clang et libFuzzer |

Prérequis : CMake 3.20, Ninja, GCC ou Clang, Python 3. Unity est récupéré
automatiquement, à une version épinglée.

Cibles et presets utiles :

```sh
cmake --build --preset headers      # les en-têtes publics sont-ils du C valide ?
ctest  --preset spec                # cohérence des specs et des vecteurs, sans build
ctest  --preset fuzz-corpus         # rejeu du corpus de fuzzing versionné
```

### La CI a du sens avant le premier code

Deux contrôles tournent sans aucun code métier, et c'est ce qui rend utile une
CI au stade `v0.0-spec` :

```sh
python3 tools/gen_vectors.py --check   # les vecteurs sont-ils ceux de l'oracle ?
python3 tools/check_docs.py            # liens, exigences, ADR, traçabilité
```

Le premier échoue si quelqu'un édite `vectors.txt` à la main. Le second
vérifie que chaque exigence citée existe, que chacune figure dans la matrice de
traçabilité, et publie le taux de couverture par brique.

Voir [`docs/conventions.md`](docs/conventions.md) et
[`docs/test-plan.md`](docs/test-plan.md).

## Licence

MIT. Voir [`LICENSE`](LICENSE).
