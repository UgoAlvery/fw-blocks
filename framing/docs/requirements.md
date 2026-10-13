# Exigences — `fwb_framing`

Chaque exigence porte un identifiant stable. Cet identifiant apparaît en
commentaire du ou des tests qui la vérifient, et dans
[`../../docs/traceability.md`](../../docs/traceability.md).

Vocabulaire : **doit** désigne une obligation, **ne doit pas** une interdiction.
Aucune exigence n'utilise « devrait ».

Méthodes de vérification employées :

| Code | Méthode |
|---|---|
| `TU` | Test unitaire |
| `VEC` | Vecteur de référence de `../tests/vectors/` |
| `FI` | Test d'injection de fautes |
| `FUZZ` | Campagne de fuzzing |
| `INSP` | Inspection de code ou de configuration |

---

## 1. Exigences générales

| ID | Exigence | Vérif. |
|---|---|---|
| FRM-REQ-001 | La brique ne doit appeler aucune fonction d'allocation dynamique. | INSP |
| FRM-REQ-002 | La brique ne doit dépendre d'aucune horloge, d'aucun système d'exploitation et d'aucun périphérique. | INSP |
| FRM-REQ-003 | Toute fonction publique recevant un pointeur nul pour un paramètre obligatoire doit renvoyer `FWB_ERR_INVALID_ARG` sans déréférencer ce pointeur. | TU |
| FRM-REQ-004 | La taille maximale d'une trame sur le fil doit être exposée par une macro évaluable à la compilation. | TU, INSP |
| FRM-REQ-005 | `FWB_FRAMING_MAX_PAYLOAD` doit être configurable à la compilation, avec 256 pour valeur par défaut. | TU |
| FRM-REQ-006 | Le codec de trame (encodage et décodage d'une trame isolée) ne doit conserver aucun état entre deux appels. | INSP |

## 2. Encodage

| ID | Exigence | Vérif. |
|---|---|---|
| FRM-REQ-010 | L'encodeur doit produire une trame conforme à `wire-format.md` §2 pour tout payload de 0 à `FWB_FRAMING_MAX_PAYLOAD` octets. | TU, VEC |
| FRM-REQ-011 | L'encodeur doit écrire `VER = 0x01` et `FLAGS = 0x00`. | TU, VEC |
| FRM-REQ-012 | L'encodeur doit écrire `SEQ` et `LEN` en little-endian. | TU, VEC |
| FRM-REQ-013 | `LEN` doit contenir la longueur du payload seul, hors en-tête et hors CRC. | TU, VEC |
| FRM-REQ-014 | L'encodeur doit calculer le CRC sur `link_id` en little-endian suivi des 7 octets d'en-tête et du payload, conformément à `wire-format.md` §5. | TU, VEC |
| FRM-REQ-015 | L'encodeur doit rejeter avec `FWB_ERR_PAYLOAD_TOO_LARGE` tout payload de plus de `FWB_FRAMING_MAX_PAYLOAD` octets. | TU |
| FRM-REQ-016 | L'encodeur doit rejeter avec `FWB_ERR_BUFFER_TOO_SMALL` tout tampon de sortie plus petit que la trame à produire, sans y écrire aucun octet. | TU |
| FRM-REQ-017 | L'encodeur doit accepter un pointeur de payload nul si et seulement si la longueur du payload est nulle. | TU |
| FRM-REQ-018 | L'encodeur ne doit pas modifier le tampon de payload fourni en entrée. | TU |
| FRM-REQ-019 | L'encodeur ne doit pas incrémenter ni modifier `SEQ` : la valeur transmise est exactement celle fournie par l'appelant. | TU |
| FRM-REQ-020 | L'encodeur doit renvoyer la taille exacte écrite sur le fil, délimiteur compris. | TU, VEC |
| FRM-REQ-021 | La sortie de l'encodeur ne doit contenir aucun octet `0x00` avant le délimiteur final. | TU, VEC, FUZZ |

## 3. Décodage d'une trame isolée

| ID | Exigence | Vérif. |
|---|---|---|
| FRM-REQ-030 | Le décodeur doit appliquer les contrôles dans l'ordre fixé par `wire-format.md` §7. | TU, VEC |
| FRM-REQ-031 | Le décodeur doit rejeter avec `FWB_ERR_FRAME_TOO_SHORT` toute trame décodée de moins de 9 octets. | TU, VEC |
| FRM-REQ-032 | Le décodeur doit rejeter avec `FWB_ERR_PAYLOAD_TOO_LARGE` toute trame dont `LEN` dépasse `FWB_FRAMING_MAX_PAYLOAD`, avant tout autre contrôle que celui de longueur minimale. | TU, VEC |
| FRM-REQ-033 | Le décodeur doit rejeter avec `FWB_ERR_LENGTH_MISMATCH` toute trame dont la longueur réelle diffère de `7 + LEN + 2`. | TU, VEC |
| FRM-REQ-034 | Le décodeur doit rejeter avec `FWB_ERR_BAD_CRC` toute trame dont le CRC recalculé diffère du CRC reçu. | TU, VEC, FI |
| FRM-REQ-035 | Le décodeur doit rejeter avec `FWB_ERR_BAD_VERSION` toute trame dont `VER` diffère de `0x01`, **après** validation du CRC. | TU, VEC |
| FRM-REQ-036 | Le décodeur doit rejeter avec `FWB_ERR_BAD_FLAGS` toute trame dont `FLAGS` diffère de `0x00`, **après** validation du CRC. | TU, VEC |
| FRM-REQ-037 | Le décodeur doit rejeter une trame dont le CRC a été calculé avec un `link_id` différent du sien, avec `FWB_ERR_BAD_CRC`. | TU, VEC |
| FRM-REQ-038 | Le décodeur ne doit lire aucun octet au-delà de la longueur de tampon qui lui est annoncée, quelle que soit l'entrée. | FUZZ, INSP |
| FRM-REQ-039 | Le décodeur ne doit écrire aucune donnée dans la structure de sortie lorsqu'il rejette une trame. | TU |
| FRM-REQ-040 | Sur acceptation, le décodeur doit restituer `TYPE`, `SEQ`, la longueur et le contenu du payload à l'identique de ce qui a été encodé. | TU, VEC, FUZZ |
| FRM-REQ-041 | Le décodeur doit accepter une trame dont `LEN` vaut 0 et signaler un payload de longueur nulle. | TU, VEC |
| FRM-REQ-042 | Le décodeur ne doit pas modifier le tampon d'entrée. | TU |

## 4. Délimitation COBS

| ID | Exigence | Vérif. |
|---|---|---|
| FRM-REQ-060 | L'encodeur COBS doit produire une sortie ne contenant aucun octet `0x00`. | TU, FUZZ |
| FRM-REQ-061 | Le décodage COBS d'une sortie d'encodage COBS doit restituer l'entrée d'origine, pour toute entrée de 0 à `FWB_FRAMING_MAX_FRAME_SIZE` octets. | TU, FUZZ |
| FRM-REQ-062 | L'encodeur COBS doit traiter correctement les blocs de 254 octets non nuls consécutifs, en émettant un code `0xFF`. | TU, VEC |
| FRM-REQ-063 | L'encodeur COBS doit traiter correctement une entrée se terminant par un octet nul. | TU |
| FRM-REQ-064 | Le décodeur COBS doit rejeter avec `FWB_ERR_COBS_MALFORMED` toute zone encodée contenant un octet de code nul. | TU, VEC |
| FRM-REQ-065 | Le décodeur COBS doit rejeter avec `FWB_ERR_COBS_MALFORMED` tout octet de code désignant un bloc qui dépasserait la fin de la zone encodée. | TU, VEC |
| FRM-REQ-066 | Le surcoût de l'encodage COBS ne doit jamais dépasser `1 + ⌊n / 254⌋` octets pour une entrée de `n` octets. | TU, FUZZ |

## 5. Récepteur en flux

Voir [`rx-state-machine.md`](rx-state-machine.md) pour les états et transitions.

| ID | Exigence | Vérif. |
|---|---|---|
| FRM-REQ-080 | Le récepteur doit accepter les octets un par un et signaler une trame complète dès réception du délimiteur. | TU |
| FRM-REQ-081 | Le récepteur doit produire le même résultat quel que soit le découpage du flux en appels, y compris octet par octet. | TU, FUZZ |
| FRM-REQ-082 | Le récepteur doit ignorer silencieusement, sans code d'erreur, une trame vide délimitée par deux `0x00` consécutifs ou par un `0x00` en tête de flux. | TU, VEC |
| FRM-REQ-083 | Le récepteur doit abandonner la trame en cours et renvoyer `FWB_ERR_OVERFLOW` dès que les octets accumulés dépasseraient `FWB_FRAMING_MAX_WIRE_SIZE`, sans jamais écrire hors de son tampon. | TU, FI, FUZZ |
| FRM-REQ-084 | Après un dépassement, le récepteur doit ignorer tous les octets jusqu'au prochain `0x00` inclus, puis reprendre normalement. | TU, FI |
| FRM-REQ-085 | Après le rejet d'une trame, quelle qu'en soit la cause, le récepteur doit être capable d'accepter la trame valide suivante. | TU, FI, FUZZ |
| FRM-REQ-086 | Le récepteur ne doit dépendre d'aucune horloge : l'écoulement du temps doit lui être signalé par un appel explicite de l'appelant. | INSP |
| FRM-REQ-087 | Le récepteur doit abandonner la trame en cours avec `FWB_ERR_TIMEOUT` lorsque le nombre configuré d'appels de temporisation s'écoule sans réception d'octet. | TU |
| FRM-REQ-088 | Une temporisation survenant alors qu'aucune trame n'est en cours ne doit produire aucune erreur. | TU |
| FRM-REQ-089 | La réinitialisation du contexte de réception doit le ramener à un état équivalent à celui de l'initialisation. | TU |
| FRM-REQ-090 | Le récepteur doit renvoyer un code d'erreur distinct par cause de rejet, conformément à `../../docs/conventions.md`. | TU, VEC |

## 6. Robustesse

| ID | Exigence | Vérif. |
|---|---|---|
| FRM-REQ-100 | Le récepteur ne doit jamais provoquer de lecture ou d'écriture hors limites, quelle que soit la suite d'octets injectée, y compris aléatoire ou adverse. | FUZZ |
| FRM-REQ-101 | Le récepteur ne doit jamais boucler indéfiniment ni déborder la pile, quelle que soit l'entrée. | FUZZ |
| FRM-REQ-102 | Toute trame acceptée par le décodeur doit se ré-encoder en une suite d'octets identique à celle reçue. | FUZZ |
| FRM-REQ-103 | Une inversion de bit unique en n'importe quelle position d'une trame valide doit conduire à un rejet. | FI |
| FRM-REQ-104 | Une troncature en n'importe quelle position d'une trame valide doit conduire à un rejet, ou à l'absence de trame signalée. | FI |
| FRM-REQ-105 | L'insertion d'un octet `0x00` en n'importe quelle position d'une trame valide sur le fil doit conduire à un rejet, ou à l'absence de trame signalée. | FI |
| FRM-REQ-106 | Aucune exécution du décodeur ne doit déclencher de comportement indéfini détectable par UndefinedBehaviorSanitizer. | FUZZ |

## 7. Exigences explicitement hors périmètre

Ces points sont consignés pour qu'on ne les croie pas oubliés.

| Point | Statut |
|---|---|
| Authentification des trames (MAC) | Hors périmètre v1. Emplacement réservé dans le format, voir `wire-format.md` §3. |
| Chiffrement du payload | Hors périmètre. |
| Retransmission, acquittement, contrôle de flux | Hors périmètre. Relèvent d'une couche supérieure. |
| Détection de rejeu | Hors périmètre de `framing`, assurée par la brique `antireplay`. |
| Détection de retard | Hors périmètre. Nécessite une horloge, donc relève de l'application. |
| Fragmentation d'un message long | Hors périmètre. |
| Interprétation du payload | Hors périmètre par conception, voir `docs/architecture.md`. |
