# Matrice de traçabilité

Ce document relie chaque exigence au module qui l'implémentera et au test qui la
vérifiera. Il est **volontairement rempli avant l'écriture du code** : une
exigence sans test prévu est une exigence qu'on ne saura pas vérifier, et c'est
maintenant qu'il faut s'en apercevoir, pas à la fin.

## État

| | Nombre |
|---|---|
| Exigences spécifiées | 113 |
| Exigences implémentées | 0 |
| Exigences vérifiées par un test qui passe | 0 |

Le dépôt est au stade `v0.0-spec`. Les colonnes « Module prévu » et « Test
prévu » sont des engagements, pas des constats : aucun de ces fichiers n'existe
encore.

## Règles de tenue

1. Toute exigence a au moins une ligne.
2. L'identifiant de l'exigence apparaît en commentaire au-dessus du test qui la
   vérifie, sous la forme `/* AR-REQ-025 */`. C'est ce qui rend la matrice
   vérifiable automatiquement plutôt que déclarative.
3. Une exigence vérifiée par inspection porte la mention « revue de code » : il
   s'agit d'une propriété structurelle qu'aucun test d'exécution ne peut
   établir, par exemple l'absence d'allocation dynamique.
4. Une exigence sans test n'est pas close, quel que soit l'état du code.
5. Ajouter une exigence implique d'ajouter sa ligne dans le même commit.

## Répartition par méthode

| Méthode | Rôle |
|---|---|
| `TU` | Test unitaire sur un comportement précis |
| `VEC` | Confrontation aux vecteurs de référence |
| `FI` | Injection de fautes : inversion de bit, troncature, insertion |
| `FUZZ` | Campagne de fuzzing, propriétés et robustesse mémoire |
| `INSP` | Revue de code ou de configuration |

Une exigence peut relever de plusieurs méthodes. Les exigences de robustesse
mémoire relèvent systématiquement du fuzzing : aucun jeu de tests écrit à la
main ne couvre l'espace des entrées malformées.

## Matrice

### `fwb_crc`

| Exigence | Résumé | Vérif. | Module prévu | Test prévu |
|---|---|---|---|---|
| `CRC-REQ-001` | La brique ne doit appeler aucune fonction d'allocation dynamique. | INSP | `crc/src/crc.c` | revue de code |
| `CRC-REQ-002` | La brique ne doit dépendre d'aucun matériel, d'aucun système d'exploitation et… | INSP | `crc/src/crc.c` | revue de code |
| `CRC-REQ-003` | Toute fonction publique recevant un pointeur nul pour un paramètre obligatoire doit… | TU | `crc/src/crc.c` | `crc/tests/unit/test_crc_api.c` |
| `CRC-REQ-004` | Le résultat ne doit dépendre ni de l'endianness, ni de l'alignement, ni du jeu… | TU, INSP | `crc/src/crc.c` | `crc/tests/unit/test_crc_api.c` |
| `CRC-REQ-010` | La brique doit accepter les six paramètres du modèle de Rocksoft : `width`, `poly`,… | TU | `crc/src/crc.c` | `crc/tests/unit/test_crc_model.c` |
| `CRC-REQ-011` | Les largeurs 8, 16 et 32 bits doivent être prises en charge. | TU | `crc/src/crc.c` | `crc/tests/unit/test_crc_model.c` |
| `CRC-REQ-012` | La réflexion de l'entrée et celle de la sortie doivent être activables… | TU | `crc/src/crc.c` | `crc/tests/unit/test_crc_model.c` |
| `CRC-REQ-013` | Le polynôme doit être fourni sous forme normale, bit de poids fort implicite, pour… | TU, INSP | `crc/src/crc.c` | `crc/tests/unit/test_crc_model.c` |
| `CRC-REQ-014` | La variante CRC-16/CCITT-FALSE doit être fournie en constante prédéfinie. | TU | `crc/src/crc.c` | `crc/tests/unit/test_crc_model.c` |
| `CRC-REQ-015` | La variante CRC-32C doit être fournie en constante prédéfinie. | TU | `crc/src/crc.c` | `crc/tests/unit/test_crc_model.c` |
| `CRC-REQ-020` | CRC-16/CCITT-FALSE appliqué à `"123456789"` doit donner `0x29B1`. | VEC | `crc/src/crc_params.c` | `crc/tests/unit/test_crc_checkvalues.c` |
| `CRC-REQ-021` | CRC-32C appliqué à `"123456789"` doit donner `0xE3069283`. | VEC | `crc/src/crc_params.c` | `crc/tests/unit/test_crc_checkvalues.c` |
| `CRC-REQ-022` | Toute variante ajoutée ultérieurement doit être accompagnée de sa valeur de contrôle… | INSP | `crc/src/crc_params.c` | revue de code |
| `CRC-REQ-030` | Le calcul doit pouvoir être réalisé en plusieurs appels successifs sur des fragments… | TU | `crc/src/crc.c` | `crc/tests/unit/test_crc_incremental.c` |
| `CRC-REQ-031` | Pour tout découpage d'une même séquence d'octets en fragments, le résultat doit être… | TU, FUZZ | `crc/src/crc.c` | `crc/tests/unit/test_crc_incremental.c` |
| `CRC-REQ-032` | Un fragment de longueur nulle ne doit pas modifier l'état du calcul, et doit être… | TU | `crc/src/crc.c` | `crc/tests/unit/test_crc_incremental.c` |
| `CRC-REQ-033` | Le calcul sur une séquence vide doit donner la valeur obtenue en appliquant `refout`… | TU | `crc/src/crc.c` | `crc/tests/unit/test_crc_incremental.c` |
| `CRC-REQ-034` | L'appel de finalisation ne doit pas modifier l'état, de sorte que deux finalisations… | TU | `crc/src/crc.c` | `crc/tests/unit/test_crc_incremental.c` |
| `CRC-REQ-035` | Le raccourci en un appel doit donner exactement le même résultat que la séquence… | TU, FUZZ | `crc/src/crc.c` | `crc/tests/unit/test_crc_incremental.c` |
| `CRC-REQ-036` | Le contexte de calcul ne doit contenir aucun pointeur vers les données absorbées. | INSP | `crc/src/crc.c` | revue de code |
| `CRC-REQ-040` | Les implémentations bit à bit et par table doivent produire des résultats identiques… | TU, FUZZ | `crc/src/crc_table.c` | `crc/tests/unit/test_crc_impl_equiv.c` |
| `CRC-REQ-041` | Le choix de l'implémentation doit se faire à la compilation, sans modification du… | TU, INSP | `crc/src/crc_table.c` | `crc/tests/unit/test_crc_impl_equiv.c` |
| `CRC-REQ-042` | L'implémentation par table ne doit pas consommer plus de 512 octets de données… | INSP | `crc/src/crc_table.c` | revue de code |
| `CRC-REQ-043` | Les tables doivent être produites par expression constante à la compilation, sans… | INSP | `crc/src/crc_table.c` | revue de code |
| `CRC-REQ-044` | L'implémentation bit à bit ne doit consommer aucune donnée constante. | INSP | `crc/src/crc_table.c` | revue de code |
| `CRC-REQ-050` | Aucune exécution ne doit provoquer de lecture hors limites, quelle que soit la… | FUZZ | `crc/src/crc.c` | `crc/fuzz/fuzz_crc.c` |
| `CRC-REQ-051` | Aucune exécution ne doit déclencher de comportement indéfini détectable par… | FUZZ | `crc/src/crc.c` | `crc/fuzz/fuzz_crc.c` |
| `CRC-REQ-052` | Les entrées non alignées doivent être traitées correctement. | TU | `crc/src/crc.c` | `crc/fuzz/fuzz_crc.c` |
| `CRC-REQ-053` | Le temps de calcul doit être proportionnel à la longueur de l'entrée et indépendant… | INSP | `crc/src/crc.c` | revue de code |

### `fwb_framing`

| Exigence | Résumé | Vérif. | Module prévu | Test prévu |
|---|---|---|---|---|
| `FRM-REQ-001` | La brique ne doit appeler aucune fonction d'allocation dynamique. | INSP | `framing/src/framing.c` | revue de code |
| `FRM-REQ-002` | La brique ne doit dépendre d'aucune horloge, d'aucun système d'exploitation et… | INSP | `framing/src/framing.c` | revue de code |
| `FRM-REQ-003` | Toute fonction publique recevant un pointeur nul pour un paramètre obligatoire doit… | TU | `framing/src/framing.c` | `framing/tests/unit/test_framing_api.c` |
| `FRM-REQ-004` | La taille maximale d'une trame sur le fil doit être exposée par une macro évaluable… | TU, INSP | `framing/src/framing.c` | `framing/tests/unit/test_framing_api.c` |
| `FRM-REQ-005` | `FWB_FRAMING_MAX_PAYLOAD` doit être configurable à la compilation, avec 256 pour… | TU | `framing/src/framing.c` | `framing/tests/unit/test_framing_api.c` |
| `FRM-REQ-006` | Le codec de trame (encodage et décodage d'une trame isolée) ne doit conserver aucun… | INSP | `framing/src/framing.c` | revue de code |
| `FRM-REQ-010` | L'encodeur doit produire une trame conforme à `wire-format.md` §2 pour tout payload… | TU, VEC | `framing/src/encode.c` | `framing/tests/unit/test_encode.c` |
| `FRM-REQ-011` | L'encodeur doit écrire `VER = 0x01` et `FLAGS = 0x00`. | TU, VEC | `framing/src/encode.c` | `framing/tests/unit/test_encode.c` |
| `FRM-REQ-012` | L'encodeur doit écrire `SEQ` et `LEN` en little-endian. | TU, VEC | `framing/src/encode.c` | `framing/tests/unit/test_encode.c` |
| `FRM-REQ-013` | `LEN` doit contenir la longueur du payload seul, hors en-tête et hors CRC. | TU, VEC | `framing/src/encode.c` | `framing/tests/unit/test_encode.c` |
| `FRM-REQ-014` | L'encodeur doit calculer le CRC sur `link_id` en little-endian suivi des 7 octets… | TU, VEC | `framing/src/encode.c` | `framing/tests/unit/test_encode.c` |
| `FRM-REQ-015` | L'encodeur doit rejeter avec `FWB_ERR_PAYLOAD_TOO_LARGE` tout payload de plus de… | TU | `framing/src/encode.c` | `framing/tests/unit/test_encode.c` |
| `FRM-REQ-016` | L'encodeur doit rejeter avec `FWB_ERR_BUFFER_TOO_SMALL` tout tampon de sortie plus… | TU | `framing/src/encode.c` | `framing/tests/unit/test_encode.c` |
| `FRM-REQ-017` | L'encodeur doit accepter un pointeur de payload nul si et seulement si la longueur… | TU | `framing/src/encode.c` | `framing/tests/unit/test_encode.c` |
| `FRM-REQ-018` | L'encodeur ne doit pas modifier le tampon de payload fourni en entrée. | TU | `framing/src/encode.c` | `framing/tests/unit/test_encode.c` |
| `FRM-REQ-019` | L'encodeur ne doit pas incrémenter ni modifier `SEQ` : la valeur transmise est… | TU | `framing/src/encode.c` | `framing/tests/unit/test_encode.c` |
| `FRM-REQ-020` | L'encodeur doit renvoyer la taille exacte écrite sur le fil, délimiteur compris. | TU, VEC | `framing/src/encode.c` | `framing/tests/unit/test_encode.c` |
| `FRM-REQ-021` | La sortie de l'encodeur ne doit contenir aucun octet `0x00` avant le délimiteur final. | TU, VEC, FUZZ | `framing/src/encode.c` | `framing/tests/unit/test_encode.c` |
| `FRM-REQ-030` | Le décodeur doit appliquer les contrôles dans l'ordre fixé par `wire-format.md` §7. | TU, VEC | `framing/src/decode.c` | `framing/tests/unit/test_decode.c` |
| `FRM-REQ-031` | Le décodeur doit rejeter avec `FWB_ERR_FRAME_TOO_SHORT` toute trame décodée de moins… | TU, VEC | `framing/src/decode.c` | `framing/tests/unit/test_decode.c` |
| `FRM-REQ-032` | Le décodeur doit rejeter avec `FWB_ERR_PAYLOAD_TOO_LARGE` toute trame dont `LEN`… | TU, VEC | `framing/src/decode.c` | `framing/tests/unit/test_decode.c` |
| `FRM-REQ-033` | Le décodeur doit rejeter avec `FWB_ERR_LENGTH_MISMATCH` toute trame dont la longueur… | TU, VEC | `framing/src/decode.c` | `framing/tests/unit/test_decode.c` |
| `FRM-REQ-034` | Le décodeur doit rejeter avec `FWB_ERR_BAD_CRC` toute trame dont le CRC recalculé… | TU, VEC, FI | `framing/src/decode.c` | `framing/tests/unit/test_decode.c` |
| `FRM-REQ-035` | Le décodeur doit rejeter avec `FWB_ERR_BAD_VERSION` toute trame dont `VER` diffère… | TU, VEC | `framing/src/decode.c` | `framing/tests/unit/test_decode.c` |
| `FRM-REQ-036` | Le décodeur doit rejeter avec `FWB_ERR_BAD_FLAGS` toute trame dont `FLAGS` diffère… | TU, VEC | `framing/src/decode.c` | `framing/tests/unit/test_decode.c` |
| `FRM-REQ-037` | Le décodeur doit rejeter une trame dont le CRC a été calculé avec un `link_id`… | TU, VEC | `framing/src/decode.c` | `framing/tests/unit/test_decode.c` |
| `FRM-REQ-038` | Le décodeur ne doit lire aucun octet au-delà de la longueur de tampon qui lui est… | FUZZ, INSP | `framing/src/decode.c` | `framing/tests/unit/test_decode.c` |
| `FRM-REQ-039` | Le décodeur ne doit écrire aucune donnée dans la structure de sortie lorsqu'il… | TU | `framing/src/decode.c` | `framing/tests/unit/test_decode.c` |
| `FRM-REQ-040` | Sur acceptation, le décodeur doit restituer `TYPE`, `SEQ`, la longueur et le contenu… | TU, VEC, FUZZ | `framing/src/decode.c` | `framing/tests/unit/test_decode.c` |
| `FRM-REQ-041` | Le décodeur doit accepter une trame dont `LEN` vaut 0 et signaler un payload de… | TU, VEC | `framing/src/decode.c` | `framing/tests/unit/test_decode.c` |
| `FRM-REQ-042` | Le décodeur ne doit pas modifier le tampon d'entrée. | TU | `framing/src/decode.c` | `framing/tests/unit/test_decode.c` |
| `FRM-REQ-060` | L'encodeur COBS doit produire une sortie ne contenant aucun octet `0x00`. | TU, FUZZ | `framing/src/cobs.c` | `framing/tests/unit/test_cobs.c` |
| `FRM-REQ-061` | Le décodage COBS d'une sortie d'encodage COBS doit restituer l'entrée d'origine,… | TU, FUZZ | `framing/src/cobs.c` | `framing/tests/unit/test_cobs.c` |
| `FRM-REQ-062` | L'encodeur COBS doit traiter correctement les blocs de 254 octets non nuls… | TU, VEC | `framing/src/cobs.c` | `framing/tests/unit/test_cobs.c` |
| `FRM-REQ-063` | L'encodeur COBS doit traiter correctement une entrée se terminant par un octet nul. | TU | `framing/src/cobs.c` | `framing/tests/unit/test_cobs.c` |
| `FRM-REQ-064` | Le décodeur COBS doit rejeter avec `FWB_ERR_COBS_MALFORMED` toute zone encodée… | TU, VEC | `framing/src/cobs.c` | `framing/tests/unit/test_cobs.c` |
| `FRM-REQ-065` | Le décodeur COBS doit rejeter avec `FWB_ERR_COBS_MALFORMED` tout octet de code… | TU, VEC | `framing/src/cobs.c` | `framing/tests/unit/test_cobs.c` |
| `FRM-REQ-066` | Le surcoût de l'encodage COBS ne doit jamais dépasser `1 + ⌊n / 254⌋` octets pour… | TU, FUZZ | `framing/src/cobs.c` | `framing/tests/unit/test_cobs.c` |
| `FRM-REQ-080` | Le récepteur doit accepter les octets un par un et signaler une trame complète dès… | TU | `framing/src/rx.c` | `framing/tests/unit/test_rx.c` |
| `FRM-REQ-081` | Le récepteur doit produire le même résultat quel que soit le découpage du flux en… | TU, FUZZ | `framing/src/rx.c` | `framing/tests/unit/test_rx.c` |
| `FRM-REQ-082` | Le récepteur doit ignorer silencieusement, sans code d'erreur, une trame vide… | TU, VEC | `framing/src/rx.c` | `framing/tests/unit/test_rx.c` |
| `FRM-REQ-083` | Le récepteur doit abandonner la trame en cours et renvoyer `FWB_ERR_OVERFLOW` dès… | TU, FI, FUZZ | `framing/src/rx.c` | `framing/tests/unit/test_rx.c` |
| `FRM-REQ-084` | Après un dépassement, le récepteur doit ignorer tous les octets jusqu'au prochain… | TU, FI | `framing/src/rx.c` | `framing/tests/unit/test_rx.c` |
| `FRM-REQ-085` | Après le rejet d'une trame, quelle qu'en soit la cause, le récepteur doit être… | TU, FI, FUZZ | `framing/src/rx.c` | `framing/tests/unit/test_rx.c` |
| `FRM-REQ-086` | Le récepteur ne doit dépendre d'aucune horloge : l'écoulement du temps doit lui être… | INSP | `framing/src/rx.c` | revue de code |
| `FRM-REQ-087` | Le récepteur doit abandonner la trame en cours avec `FWB_ERR_TIMEOUT` lorsque le… | TU | `framing/src/rx.c` | `framing/tests/unit/test_rx.c` |
| `FRM-REQ-088` | Une temporisation survenant alors qu'aucune trame n'est en cours ne doit produire… | TU | `framing/src/rx.c` | `framing/tests/unit/test_rx.c` |
| `FRM-REQ-089` | La réinitialisation du contexte de réception doit le ramener à un état équivalent à… | TU | `framing/src/rx.c` | `framing/tests/unit/test_rx.c` |
| `FRM-REQ-090` | Le récepteur doit renvoyer un code d'erreur distinct par cause de rejet,… | TU, VEC | `framing/src/rx.c` | `framing/tests/unit/test_rx.c` |
| `FRM-REQ-100` | Le récepteur ne doit jamais provoquer de lecture ou d'écriture hors limites, quelle… | FUZZ | `framing/src/` | `framing/fuzz/fuzz_rx.c` |
| `FRM-REQ-101` | Le récepteur ne doit jamais boucler indéfiniment ni déborder la pile, quelle que… | FUZZ | `framing/src/` | `framing/fuzz/fuzz_rx.c` |
| `FRM-REQ-102` | Toute trame acceptée par le décodeur doit se ré-encoder en une suite d'octets… | FUZZ | `framing/src/` | `framing/fuzz/fuzz_rx.c` |
| `FRM-REQ-103` | Une inversion de bit unique en n'importe quelle position d'une trame valide doit… | FI | `framing/src/` | `framing/fuzz/fuzz_rx.c` |
| `FRM-REQ-104` | Une troncature en n'importe quelle position d'une trame valide doit conduire à un… | FI | `framing/src/` | `framing/fuzz/fuzz_rx.c` |
| `FRM-REQ-105` | L'insertion d'un octet `0x00` en n'importe quelle position d'une trame valide sur le… | FI | `framing/src/` | `framing/fuzz/fuzz_rx.c` |
| `FRM-REQ-106` | Aucune exécution du décodeur ne doit déclencher de comportement indéfini détectable… | FUZZ | `framing/src/` | `framing/fuzz/fuzz_rx.c` |

### `fwb_antireplay`

| Exigence | Résumé | Vérif. | Module prévu | Test prévu |
|---|---|---|---|---|
| `AR-REQ-001` | La brique ne doit appeler aucune fonction d'allocation dynamique. | INSP | `antireplay/src/antireplay.c` | revue de code |
| `AR-REQ-002` | La brique ne doit dépendre d'aucune horloge ni d'aucun matériel. | INSP | `antireplay/src/antireplay.c` | revue de code |
| `AR-REQ-003` | La brique ne doit dépendre d'aucune autre brique que `fwb_common`, et en particulier… | INSP | `antireplay/src/antireplay.c` | revue de code |
| `AR-REQ-004` | Toute fonction publique recevant un pointeur nul doit renvoyer `FWB_ERR_INVALID_ARG`… | TU | `antireplay/src/antireplay.c` | `antireplay/tests/unit/test_ar_api.c` |
| `AR-REQ-005` | `FWB_ANTIREPLAY_WINDOW` doit être configurable à la compilation parmi 8, 16, 32 et… | TU | `antireplay/src/antireplay.c` | `antireplay/tests/unit/test_ar_api.c` |
| `AR-REQ-006` | L'état d'une instance ne doit pas dépasser 10 octets pour `W = 64`. | INSP | `antireplay/src/antireplay.c` | revue de code |
| `AR-REQ-010` | Après initialisation, le contexte doit être dans l'état « non amorcé », distinct de… | TU | `antireplay/src/antireplay.c` | `antireplay/tests/unit/test_ar_seed.c` |
| `AR-REQ-011` | Dans l'état non amorcé, la première valeur soumise doit être acceptée sans contrôle… | TU | `antireplay/src/antireplay.c` | `antireplay/tests/unit/test_ar_seed.c` |
| `AR-REQ-012` | L'acceptation de cette première valeur doit faire passer le contexte à l'état amorcé… | TU | `antireplay/src/antireplay.c` | `antireplay/tests/unit/test_ar_seed.c` |
| `AR-REQ-013` | L'état d'amorçage doit être interrogeable par l'application avant toute soumission. | TU | `antireplay/src/antireplay.c` | `antireplay/tests/unit/test_ar_seed.c` |
| `AR-REQ-014` | La réinitialisation explicite doit ramener le contexte à l'état non amorcé. | TU | `antireplay/src/antireplay.c` | `antireplay/tests/unit/test_ar_seed.c` |
| `AR-REQ-020` | Une valeur avec `delta > 0` doit être acceptée, le masque décalé de `delta`, la… | TU | `antireplay/src/antireplay.c` | `antireplay/tests/unit/test_ar_window.c` |
| `AR-REQ-021` | Une valeur avec `delta == 0` doit être rejetée avec `FWB_ERR_REPLAY`. | TU | `antireplay/src/antireplay.c` | `antireplay/tests/unit/test_ar_window.c` |
| `AR-REQ-022` | Une valeur avec `−W < delta < 0` dont le bit n'est pas posé doit être acceptée et… | TU | `antireplay/src/antireplay.c` | `antireplay/tests/unit/test_ar_window.c` |
| `AR-REQ-023` | Une valeur avec `−W < delta < 0` dont le bit est déjà posé doit être rejetée avec… | TU | `antireplay/src/antireplay.c` | `antireplay/tests/unit/test_ar_window.c` |
| `AR-REQ-024` | Une valeur avec `delta ≤ −W` doit être rejetée avec `FWB_ERR_TOO_OLD`. | TU | `antireplay/src/antireplay.c` | `antireplay/tests/unit/test_ar_window.c` |
| `AR-REQ-025` | Une valeur acceptée avec `delta > 1` doit renvoyer `FWB_ERR_SEQ_GAP` **tout en étant… | TU | `antireplay/src/antireplay.c` | `antireplay/tests/unit/test_ar_window.c` |
| `AR-REQ-026` | Une valeur acceptée avec `delta == 1` doit renvoyer `FWB_OK`. | TU | `antireplay/src/antireplay.c` | `antireplay/tests/unit/test_ar_window.c` |
| `AR-REQ-027` | Une valeur rejetée ne doit modifier aucun champ de l'état. | TU | `antireplay/src/antireplay.c` | `antireplay/tests/unit/test_ar_window.c` |
| `AR-REQ-028` | Avec `delta ≥ W`, le masque doit être entièrement remis à zéro avant marquage, sans… | TU, FUZZ | `antireplay/src/antireplay.c` | `antireplay/tests/unit/test_ar_window.c` |
| `AR-REQ-030` | Le passage de `0xFFFF` à `0x0000` doit être traité comme une progression normale,… | TU | `antireplay/src/antireplay.c` | `antireplay/tests/unit/test_ar_modular.c` |
| `AR-REQ-031` | La comparaison doit rester correcte pour tout écart réel strictement inférieur à 32768. | TU, FUZZ | `antireplay/src/antireplay.c` | `antireplay/tests/unit/test_ar_modular.c` |
| `AR-REQ-032` | Le calcul de `delta` ne doit provoquer aucun dépassement sur type signé détectable… | FUZZ | `antireplay/src/antireplay.c` | `antireplay/tests/unit/test_ar_modular.c` |
| `AR-REQ-033` | Une séquence de 70000 valeurs consécutives, couvrant plus d'un cycle complet du… | TU | `antireplay/src/antireplay.c` | `antireplay/tests/unit/test_ar_modular.c` |
| `AR-REQ-040` | Aucune suite de valeurs, quelle qu'elle soit, ne doit provoquer un comportement… | FUZZ | `antireplay/src/antireplay.c` | `antireplay/fuzz/fuzz_antireplay.c` |
| `AR-REQ-041` | Aucune valeur déjà acceptée ne doit pouvoir être acceptée une seconde fois tant… | FUZZ | `antireplay/src/antireplay.c` | `antireplay/fuzz/fuzz_antireplay.c` |
| `AR-REQ-042` | Pour toute suite de valeurs, le nombre d'acceptations distinctes ne doit jamais… | FUZZ | `antireplay/src/antireplay.c` | `antireplay/fuzz/fuzz_antireplay.c` |
| `AR-REQ-043` | Le temps de décision doit être borné et indépendant de l'historique. | INSP | `antireplay/src/antireplay.c` | revue de code |


## Couverture des menaces

Complément à la matrice d'exigences : chaque menace de
[`../framing/docs/threat-model.md`](../framing/docs/threat-model.md) doit être
reliée à un mécanisme et à une exigence.

| Menace | Mécanisme | Exigences | Statut |
|---|---|---|---|
| Corruption | CRC-16/CCITT-FALSE | FRM-REQ-034, FRM-REQ-103, CRC-REQ-020 | Couverte |
| Répétition | `SEQ` + fenêtre glissante | AR-REQ-021, AR-REQ-023, AR-REQ-041 | Couverte |
| Perte | Trou détecté dans `SEQ` | AR-REQ-025 | Détectée, non corrigée |
| Insertion | Identifiant de lien dans le CRC | FRM-REQ-037 | Couverte, cas accidentels |
| Resequencement | Fenêtre glissante | AR-REQ-022, AR-REQ-024 | Couverte |
| Masquerading | Identifiant de lien dans le CRC | FRM-REQ-037 | Couverte, cas accidentels |
| Retard | — | — | **Non couverte**, hors périmètre |
| Troncature | `LEN` + délimitation COBS | FRM-REQ-033, FRM-REQ-104 | Couverte |
| Forge, modification, rejeu adverse | MAC | — | **Non couverte**, hors périmètre v1 |

Les deux lignes sans exigence sont les trous assumés du périmètre v1. Elles
figurent ici précisément pour qu'on ne les oublie pas et qu'on ne croie pas la
couverture complète.
