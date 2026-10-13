# Vecteurs de référence — `fwb_framing`

Ces vecteurs sont la **référence normative** du format décrit dans
[`../../docs/wire-format.md`](../../docs/wire-format.md). Une implémentation qui
ne les reproduit pas exactement n'est pas conforme, quelle que soit la lecture
qu'elle fait du texte.

Ils ont été produits avant l'implémentation, par un oracle indépendant écrit
dans un autre langage, et non par le code du projet. Ce point est essentiel :
des vecteurs générés par le code qu'ils sont censés valider ne valident rien.

Les valeurs de contrôle du CRC ont été vérifiées contre les valeurs publiées :
CRC-16/CCITT-FALSE donne `0x29B1` et CRC-32C donne `0xE3069283` sur la chaîne
`"123456789"`.

## Fichiers

| Fichier | Contenu |
|---|---|
| `vectors.txt` | Forme exploitable par machine. Source de vérité pour les tests. |
| `README.md` | Ce document : description et justification de chaque vecteur. |

Dans `vectors.txt`, `decoded` désigne la trame après décodage COBS et `wire` les
octets réellement transmis, délimiteur `0x00` inclus.

## Convention de nommage

- `FRM-V-xxx` : vecteur **valide**, attendu `FWB_OK`.
- `FRM-I-xxx` : vecteur **invalide**, attendu un code d'erreur précis.

Le code d'erreur attendu fait partie du vecteur. Vérifier seulement qu'une trame
est rejetée ne suffit pas : un décodeur qui rejetterait tout passerait tous les
tests négatifs. C'est la raison d'être de l'ADR-0004.

## Table récapitulative

Tous les vecteurs utilisent `link_id = 0x0000`, sauf `FRM-V-006`.

| ID | Description | Octets sur le fil | Attendu |
|---|---|---|---|
| `FRM-V-001` | Payload vide | `02 01 02 10 01 01 01 03 49 A4 00` | `FWB_OK` |
| `FRM-V-002` | Payload ASCII `"ABC"` | `02 01 03 20 01 02 03 06 41 42 43 F3 FF 00` | `FWB_OK` |
| `FRM-V-003` | Payload de trois octets nuls | `02 01 03 20 02 02 03 01 01 01 03 E5 1E 00` | `FWB_OK` |
| `FRM-V-004` | `SEQ` à sa valeur maximale | `02 01 05 30 FF FF 04 01 01 05 80 3F FC EF 00` | `FWB_OK` |
| `FRM-V-005` | Payload maximal de 256 octets | voir `vectors.txt` | `FWB_OK` |
| `FRM-V-006` | Comme `FRM-V-002`, `link_id = 0xBEEF` | `02 01 03 20 01 02 03 06 41 42 43 C6 D0 00` | `FWB_OK` |
| `FRM-I-001` | CRC erroné | `02 01 03 20 01 02 03 06 41 42 43 F2 FF 00` | `FWB_ERR_BAD_CRC` |
| `FRM-I-002` | Payload corrompu | `02 01 03 20 01 02 03 06 41 C2 43 F3 FF 00` | `FWB_ERR_BAD_CRC` |
| `FRM-I-003` | `LEN` incohérent | `02 01 03 20 01 02 04 06 41 42 43 27 98 00` | `FWB_ERR_LENGTH_MISMATCH` |
| `FRM-I-004` | Version inconnue | `02 02 03 20 01 02 03 06 41 42 43 3C 4E 00` | `FWB_ERR_BAD_VERSION` |
| `FRM-I-005` | Drapeau réservé positionné | `05 01 01 20 01 02 03 06 41 42 43 D0 14 00` | `FWB_ERR_BAD_FLAGS` |
| `FRM-I-006` | `LEN` supérieur au maximum | `02 01 03 20 01 08 01 01 41 42 43 C4 CD 00` | `FWB_ERR_PAYLOAD_TOO_LARGE` |
| `FRM-I-007` | Trame tronquée | `02 01 03 20 01 02 03 00` | `FWB_ERR_FRAME_TOO_SHORT` |
| `FRM-I-008` | Code COBS hors limites | `05 01 02 00` | `FWB_ERR_COBS_MALFORMED` |
| `FRM-I-009` | Trame décodée trop courte | `04 01 02 03 00` | `FWB_ERR_FRAME_TOO_SHORT` |
| `FRM-I-010` | Délimiteur isolé | `00` | `FWB_OK`, sans livraison |
| `FRM-I-011` | Délimiteur prématuré | `02 01 00 00` | `FWB_ERR_FRAME_TOO_SHORT` |

## Ce que chaque vecteur vérifie

### Vecteurs valides

**`FRM-V-001` — payload vide.** `LEN = 0` est licite. Vérifie que le cas limite
le plus court fonctionne et qu'un pointeur de payload nul est accepté quand la
longueur l'est aussi. Couvre FRM-REQ-041 et FRM-REQ-017.

**`FRM-V-002` — trame de référence.** C'est le vecteur détaillé pas à pas dans
`wire-format.md` §9. Il sert de point d'entrée pour comprendre le format, et de
base à six vecteurs invalides construits par mutation.

**`FRM-V-003` — octets nuls dans le payload.** Vérifie que COBS traite
correctement des zéros à l'intérieur des données. C'est le cas que l'ancienne
délimitation par octet de début gérait mal. Sur le fil, les trois zéros
deviennent trois codes `01`, et aucun `0x00` n'apparaît avant le délimiteur.
Couvre FRM-REQ-060.

**`FRM-V-004` — `SEQ = 0xFFFF`.** Vérifie que la valeur maximale est transmise
telle quelle et que le repli vers 0 n'est pas traité comme une erreur par le
codec. Le payload est le motif binaire de `1.0f` en little-endian, rappel que le
format transporte des octets et non des flottants (ADR-0003).

**`FRM-V-005` — payload maximal.** Trame la plus longue possible : 265 octets
décodés, 268 sur le fil. Vérifie le dimensionnement des tampons et le
découpage COBS en plus d'un bloc, le payload contenant plus de 254 octets non
nuls consécutifs. L'en-tête est `01 00 40 03 00 00 01` et le CRC vaut `0x7232`.
Couvre FRM-REQ-062 et FRM-REQ-066.

**`FRM-V-006` — identifiant de lien.** Champs strictement identiques à
`FRM-V-002`, mais `link_id = 0xBEEF`. Seul le CRC change, qui passe de `0xFFF3`
à `0xD0C6`. Ce vecteur démontre directement le mécanisme de l'ADR-0007 : un
récepteur configuré avec `link_id = 0x0000` doit rejeter cette trame avec
`FWB_ERR_BAD_CRC`, et réciproquement. Couvre FRM-REQ-037.

### Vecteurs invalides

**`FRM-I-001` et `FRM-I-002` — corruption.** Deux inversions d'un seul bit à
partir de `FRM-V-002`, l'une dans le CRC, l'autre dans le payload. Elles doivent
toutes deux donner `FWB_ERR_BAD_CRC`. Couvrent FRM-REQ-034 et FRM-REQ-103.

**`FRM-I-003` — `LEN` incohérent.** `LEN = 4` est annoncé alors que la trame
n'en contient que 3. Le CRC est calculé sur l'en-tête falsifié, donc valide :
seul le contrôle de cohérence de longueur peut rejeter cette trame. Vérifie que
le contrôle 3 s'exécute bien **avant** le contrôle 4. Couvre FRM-REQ-033.

**`FRM-I-004` et `FRM-I-005` — contrôles sémantiques.** `VER = 0x02` et
`FLAGS = 0x01`, chacun avec un **CRC valide** recalculé sur la trame modifiée.
Cette précaution est le cœur des deux vecteurs : si le CRC était faux, on ne
saurait pas distinguer un décodeur qui applique bien le contrôle sémantique d'un
décodeur qui s'arrête au CRC. Vérifient que les contrôles 5 et 6 s'exécutent et
qu'ils viennent après le CRC. Couvrent FRM-REQ-035 et FRM-REQ-036.

**`FRM-I-006` — `LEN` hors limites.** `LEN = 0x0101`, soit 257, dépasse le
maximum de 256. La trame fait 12 octets, donc la longueur est *aussi*
incohérente. Le code attendu est `FWB_ERR_PAYLOAD_TOO_LARGE` et non
`FWB_ERR_LENGTH_MISMATCH` : ce vecteur vérifie précisément que le contrôle 2
précède le contrôle 3. Couvre FRM-REQ-032 et FRM-REQ-030.

> À noter : en passant par le récepteur en flux, une trame dont `LEN` est
> grossièrement surdimensionné est rejetée plus tôt, avec `FWB_ERR_OVERFLOW`,
> parce qu'elle dépasse le tampon avant d'atteindre le codec. Ce vecteur
> s'applique donc à `fwb_framing_decode` appelée directement. La distinction est
> expliquée dans `rx-state-machine.md` §5.

**`FRM-I-007` — troncature.** Six octets décodés, en deçà du minimum structurel
de neuf. Vérifie que le contrôle 1 protège les accès ultérieurs. Couvre
FRM-REQ-031 et FRM-REQ-104.

**`FRM-I-008` — COBS malformé.** Le code `0x05` annonce un bloc de 4 octets
alors que la zone encodée n'en contient que 2. Un décodeur naïf lirait au-delà
du tampon. C'est le vecteur le plus important pour la sûreté mémoire. Couvre
FRM-REQ-065.

**`FRM-I-009` — trame décodée trop courte.** L'encodage COBS est valide et se
décode proprement, mais ne produit que 3 octets. Vérifie que la validité de la
délimitation n'exempte pas des contrôles structurels. Couvre FRM-REQ-031.

**`FRM-I-010` — délimiteur isolé.** Un `0x00` sans rien devant. C'est le cas
normal d'un récepteur qui démarre en cours de transmission, ou de deux
délimiteurs consécutifs. Il doit être **ignoré silencieusement** : aucune trame
livrée, et surtout **aucun code d'erreur**. Un récepteur qui signalerait une
erreur ici noierait les journaux au démarrage. Couvre FRM-REQ-082.

**`FRM-I-011` — délimiteur prématuré.** Le code `0x02` annonce un bloc d'un
octet, puis un `0x00` arrive. La trame se termine donc après 1 octet décodé.
Vérifie qu'un délimiteur ferme toujours la trame en cours, même au milieu d'un
bloc COBS, et que le récepteur se resynchronise ensuite. Couvre FRM-REQ-031 et
l'invariant I3 de `rx-state-machine.md`.

## Tests dérivés

Au-delà de ces vecteurs figés, les tests génèrent mécaniquement :

| Famille | Construction | Attendu |
|---|---|---|
| Inversion de bit | Les 8 × `n` inversions d'un seul bit sur chaque vecteur valide | Rejet systématique (FRM-REQ-103) |
| Troncature | Chaque préfixe strict de chaque vecteur valide | Rejet, ou aucune trame livrée (FRM-REQ-104) |
| Insertion de `0x00` | Un `0x00` inséré à chaque position de chaque vecteur valide | Rejet, ou aucune trame livrée (FRM-REQ-105) |
| Découpage du flux | Chaque vecteur poussé octet par octet, puis par lots de tailles variables | Résultat identique (FRM-REQ-081) |
| Aller-retour | Encodage puis décodage, pour toute longueur de payload de 0 à 256 | Restitution à l'identique (FRM-REQ-040) |

L'inversion de bit n'est pas garantie détectée dans l'absolu — c'est la limite
probabiliste d'un CRC de 16 bits — mais elle l'est pour une inversion **unique**,
que le polynôme `0x1021` détecte toujours. C'est pourquoi la famille se limite à
un bit à la fois.

## Régénération

Les vecteurs ne sont pas régénérés en routine. Ils sont figés avec le format.
Une modification du format implique d'incrémenter `VER`, de produire un nouveau
jeu de vecteurs et de conserver l'ancien pour tester la rétrocompatibilité du
rejet.
