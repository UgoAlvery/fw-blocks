# Conventions de codage

Ce document est normatif. Tout écart doit être justifié en commentaire dans le
code, avec une référence à la règle concernée.

## Langages et standards

| Élément | Choix |
|---|---|
| Cœur | C11 (`-std=c11`), sans extension GNU |
| Wrapper optionnel | C++17, sans exceptions, sans RTTI, sans allocation dynamique |
| Tests du cœur | Unity, écrits en C |
| Harness de fuzzing | libFuzzer, écrits en C++, appelant l'API C |

Le wrapper C++ n'est pas écrit tant que l'API C n'est pas stabilisée
(voir ADR-0001).

## Nommage

| Élément | Convention | Exemple |
|---|---|---|
| Fonction publique | `fwb_<brique>_<action>` | `fwb_framing_encode` |
| Type public | `fwb_<brique>_<nom>_t` | `fwb_framing_rx_t` |
| Énumérateur public | `FWB_<BRIQUE>_<NOM>` | `FWB_FRAMING_STATE_IDLE` |
| Macro de configuration | `FWB_<BRIQUE>_<NOM>` | `FWB_FRAMING_MAX_PAYLOAD` |
| Symbole interne | `static`, préfixe `fwb_` facultatif | `static crc16_step` |
| Fichier | minuscules, tiret bas | `rx_state_machine.c` |

Aucun symbole public sans le préfixe `fwb_` ou `FWB_`. C'est la seule protection
contre les collisions dans un firmware qui agrège plusieurs bibliothèques.

## En-têtes publics

Un en-tête de `include/` doit :

1. être du **C valide**, pas du C++ déguisé : pas de surcharge, pas de
   `namespace`, pas de mot-clé C++ comme identifiant, `bool` seulement après
   `#include <stdbool.h>` ;
2. porter les gardes `extern "C"` sous `#ifdef __cplusplus`, sans quoi l'édition
   de liens échoue depuis du C++ à cause de la décoration des noms ;
3. porter une garde d'inclusion `FWB_<BRIQUE>_<FICHIER>_H` ;
4. n'inclure que ce dont il a besoin, et rien de `src/`.

**Garde-fou automatique.** La cible CMake `fwb_header_c_compile` compile chaque
en-tête public dans un fichier `.c` minimal. Sans elle, un en-tête devenu
invalide en C passe inaperçu dès lors que les tests sont compilés en C++.

## Gestion des erreurs

Toute fonction susceptible d'échouer renvoie un `fwb_status_t`. Les valeurs de
sortie passent par des pointeurs de sortie, jamais par la valeur de retour.

```
FWB_OK                        =  0
FWB_ERR_INVALID_ARG           = -1   pointeur nul, paramètre incohérent
FWB_ERR_BUFFER_TOO_SMALL      = -2   tampon de sortie insuffisant
FWB_ERR_NOT_SUPPORTED         = -3   fonctionnalité absente de cette build
FWB_ERR_FRAME_TOO_SHORT       = -10  trame plus courte que le minimum structurel
FWB_ERR_PAYLOAD_TOO_LARGE     = -11  LEN dépasse FWB_FRAMING_MAX_PAYLOAD
FWB_ERR_LENGTH_MISMATCH       = -12  LEN incohérent avec la taille réelle
FWB_ERR_BAD_CRC               = -13  CRC calculé différent du CRC reçu
FWB_ERR_BAD_VERSION           = -14  champ VER inconnu
FWB_ERR_BAD_FLAGS             = -15  bit de FLAGS non supporté positionné
FWB_ERR_COBS_MALFORMED        = -16  encodage de délimitation invalide
FWB_ERR_OVERFLOW              = -17  trame plus longue que le tampon de réception
FWB_ERR_TIMEOUT               = -18  trame abandonnée faute d'octets
FWB_ERR_REPLAY                = -20  numéro de séquence déjà vu
FWB_ERR_TOO_OLD               = -21  numéro de séquence hors de la fenêtre
FWB_ERR_SEQ_GAP               = -22  trame acceptée, mais trou dans la séquence
```

Règles :

- `FWB_OK` vaut toujours 0, les erreurs sont strictement négatives. Un test
  `if (status != FWB_OK)` est donc toujours correct.
- `FWB_ERR_SEQ_GAP` est une **information**, pas un rejet : la trame est
  acceptée. C'est le seul code non nul qui n'annule pas l'opération, et ce
  statut est documenté comme tel dans `antireplay/docs/requirements.md`.
- Un code d'erreur par cause distincte. Fusionner deux causes fait perdre du
  diagnostic en production, là où on n'a souvent que le code de retour.
- Aucune fonction ne renvoie un booléen pour signaler un échec (voir ADR-0004).

## Règles de codage

Obligatoire :

- Tous les paramètres de pointeur en entrée sont `const` s'ils ne sont pas
  modifiés.
- Toute fonction publique valide ses pointeurs et renvoie `FWB_ERR_INVALID_ARG`
  sur un pointeur nul. Pas d'`assert` en lieu et place : le firmware tourne en
  `NDEBUG`.
- Pas de `goto`, sauf pour un épilogue unique de libération, inutile ici faute
  d'allocation.
- Pas de récursion, pour borner la pile.
- Pas de tableau à longueur variable (VLA), pour la même raison.
- Pas de `malloc`, `calloc`, `realloc`, `free` (voir ADR-0002).
- Pas de `memcpy` d'une structure vers un tampon réseau (voir ADR-0003).
- Toute boucle a une borne supérieure démontrable à la lecture.
- Les conversions entre types entiers sont explicites. `-Wconversion` est actif,
  un cast implicite est une erreur de compilation.
- Un seul `return` par fonction n'est **pas** exigé : les retours anticipés de
  validation sont plus lisibles et sont préférés.

## Sous-ensemble MISRA C

Le projet vise un sous-ensemble de **MISRA C:2012**, vérifié par outillage et
non par relecture manuelle.

- Le texte des règles MISRA est sous licence et **n'est pas redistribué dans ce
  dépôt**. L'addon MISRA de cppcheck nécessite que l'utilisateur fournisse son
  propre exemplaire du texte.
- Les écarts assumés sont consignés dans ce fichier, section ci-dessous, avec
  leur justification.
- Les règles concernant la gestion dynamique de la mémoire, la récursion, les
  VLA et les conversions implicites sont couvertes par les règles de codage
  ci-dessus et par les options du compilateur.

### Écarts assumés

| Règle | Écart | Justification |
|---|---|---|
| 15.5 (un seul point de sortie) | Non appliquée | Les retours anticipés de validation réduisent l'imbrication et le risque d'erreur. Écart courant et documenté dans l'industrie. |
| 21.x (fonctions de `<string.h>`) | `memcpy` et `memset` autorisés | Seules ces deux fonctions sont utilisées, toujours avec une taille calculée et bornée, jamais avec une chaîne terminée par un nul. |

## Options de compilation

```
-std=c11 -Wall -Wextra -Wconversion -Wshadow -Wpointer-arith
-Wcast-qual -Wstrict-prototypes -Wmissing-prototypes -Werror
```

Les tests et les harness de fuzzing sont compilés avec les mêmes options.

## Presets CMake

| Preset | Usage |
|---|---|
| `debug` | `-O0 -g`, assertions actives, tests unitaires |
| `release` | `-Os`, représentatif d'une build embarquée |
| `asan-ubsan` | AddressSanitizer et UndefinedBehaviorSanitizer, tests unitaires |
| `fuzz` | Clang, libFuzzer, ASan et UBSan |

## Intégration continue

La CI doit être verte avant tout merge. Elle exécute, dans l'ordre :

1. vérification du formatage (`clang-format --dry-run --Werror`) ;
2. build `debug` et tests unitaires ;
3. build `release` ;
4. build `asan-ubsan` et tests unitaires ;
5. `clang-tidy` et `cppcheck` ;
6. cible `fwb_header_c_compile` ;
7. rejeu du corpus de fuzzing versionné (`-runs=0`, sans exploration), bloquant
   en cas de régression sur une entrée déjà connue.

L'exploration de nouvelles entrées ne se fait pas en CI : sa durée utile est
sans rapport avec le temps acceptable sur une *pull request*, et son résultat
est non déterministe, donc impropre à bloquer une fusion. Elle se fait avant
chaque marquage de version, selon `test-plan.md` §5.

## Documentation

- Chaque brique a un `README.md` qui dit ce qu'elle garantit **et ce qu'elle ne
  garantit pas**.
- Chaque exigence porte un identifiant stable (`CRC-REQ-xxx`, `FRM-REQ-xxx`,
  `AR-REQ-xxx`), qui apparaît dans la spécification, en commentaire du test
  correspondant et dans `docs/traceability.md`.
- Une décision structurante donne lieu à un ADR dans `docs/adr/`, numéroté et
  jamais réécrit : un ADR obsolète est marqué `Remplacé par ADR-00xx`, pas
  supprimé.
