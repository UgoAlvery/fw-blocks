# Format de trame `fwb_framing` v1

Ce document est **normatif**. Il décrit le format indépendamment de toute
implémentation et de tout langage. Une implémentation conforme doit produire et
accepter exactement les octets décrits ici, et les vecteurs de
[`../tests/vectors/`](../tests/vectors/) en font foi.

Version du format : **1**. Toute évolution incompatible incrémente le champ
`VER` et donne lieu à un nouveau document.

## 1. Vue d'ensemble

Une trame traverse deux étages indépendants :

```
   application
        │  type, numéro de séquence, payload
        ▼
┌───────────────────────────────────┐
│  codec de trame (§2 à §5)         │   produit la « trame décodée »
└───────────────────────────────────┘
        │  suite d'octets quelconques, 9 à 265 octets
        ▼
┌───────────────────────────────────┐
│  délimitation COBS (§6)           │   produit la « trame sur le fil »
└───────────────────────────────────┘
        │  aucun octet nul, puis un délimiteur 0x00
        ▼
      liaison
```

Deux termes sont utilisés dans tout le dépôt et ne doivent pas être confondus :

- **trame décodée** : la suite d'octets de §2, avant encodage COBS ;
- **trame sur le fil** : la trame décodée après encodage COBS, suivie du
  délimiteur `0x00`.

## 2. Structure de la trame décodée

Tous les champs multi-octets sont en **little-endian** (ADR-0003).

| Offset | Champ | Taille | Description |
|---|---|---|---|
| 0 | `VER` | 1 | Version du format. Vaut `0x01` en v1. |
| 1 | `FLAGS` | 1 | Drapeaux. Voir §3. |
| 2 | `TYPE` | 1 | Type de message. Sémantique laissée à l'application. |
| 3 | `SEQ` | 2 | Numéro de séquence de l'émetteur. Voir §4. |
| 5 | `LEN` | 2 | Longueur du `PAYLOAD` **seul**, en octets. |
| 7 | `PAYLOAD` | `LEN` | Données applicatives. Contenu quelconque, octets nuls inclus. |
| 7 + `LEN` | `CRC` | 2 | Contrôle d'intégrité. Voir §5. |

- Taille de l'en-tête : **7 octets**, constante.
- Taille du contrôle : **2 octets**, constante.
- Surcoût total du codec : **9 octets**.
- `LEN` ∈ [0, `FWB_FRAMING_MAX_PAYLOAD`], avec `FWB_FRAMING_MAX_PAYLOAD` = 256
  par défaut, configurable à la compilation.
- Taille de la trame décodée : **9 à 265 octets** en configuration par défaut.

**`LEN` ne couvre que le payload.** Ni l'en-tête, ni le CRC. C'est la définition
la plus simple à vérifier et celle qui évite les erreurs de calcul les plus
fréquentes ; elle est rappelée dans chaque exigence qui la mentionne.

### Redondance assumée de `LEN`

La délimitation COBS donne déjà la longueur exacte de la trame décodée. `LEN`
est donc redondant. Il est conservé délibérément : il sert de contrôle de
cohérence croisé contre un décodage COBS erroné, et il est nécessaire à une
future stratégie de délimitation par longueur (ADR-0006).

## 3. Champ `FLAGS`

| Bit | Nom | Signification en v1 |
|---|---|---|
| 0 | `MAC_PRESENT` | Réservé. Indiquera la présence d'un MAC entre `PAYLOAD` et `CRC`. **Doit valoir 0.** |
| 1 à 7 | — | Réservés. **Doivent valoir 0.** |

En v1, le champ `FLAGS` entier doit valoir `0x00`. Une trame dont un bit
quelconque est positionné est rejetée avec `FWB_ERR_BAD_FLAGS`.

Rejeter plutôt qu'ignorer les bits inconnus est un choix de sûreté : accepter
une trame dont on ne comprend pas tous les champs revient à agir sur une
intention qu'on n'a pas interprétée.

### Emplacement réservé au MAC

Lorsque la sécurité entrera dans le périmètre, un champ `MAC` de 8 ou 16 octets
sera inséré **entre `PAYLOAD` et `CRC`**, et le bit 0 de `FLAGS` sera positionné.
Le MAC couvrira `VER` jusqu'à la fin de `PAYLOAD`. Le CRC restera en dernière
position et couvrira également le MAC.

Rien de tout cela n'est implémenté en v1. Seuls l'emplacement et le bit de
drapeau sont réservés.

## 4. Champ `SEQ`

- Entier non signé de 16 bits, little-endian.
- L'émetteur l'incrémente de 1 à chaque trame émise, y compris en cas de
  retransmission d'un contenu identique.
- Le passage de `0xFFFF` à `0x0000` est normal et ne constitue pas une erreur.
- La valeur initiale après un démarrage n'est pas spécifiée par ce document.
  Les conséquences sont traitées dans `../../antireplay/docs/requirements.md`.

Le codec de trame se contente de transporter `SEQ`. **Il ne l'interprète
jamais.** Toute décision d'acceptation liée à la fraîcheur appartient à la
brique `antireplay`, qui est indépendante (voir `docs/architecture.md`).

## 5. Champ `CRC`

| Paramètre | Valeur |
|---|---|
| Variante | CRC-16/CCITT-FALSE |
| Largeur | 16 bits |
| Polynôme | `0x1021` |
| Valeur initiale | `0xFFFF` |
| Réflexion entrée | non |
| Réflexion sortie | non |
| Masque final | `0x0000` |
| Valeur de contrôle sur `"123456789"` | `0x29B1` |
| Ordre d'octets du champ | little-endian |

Le choix de `init = 0xFFFF` plutôt que `0x0000` est justifié dans l'ADR-0005.

### Zone couverte

Le CRC est calculé sur la concaténation suivante :

```
link_id (2 octets, little-endian)  ||  VER  FLAGS  TYPE  SEQ  LEN  PAYLOAD
```

soit `2 + 7 + LEN` octets en entrée du calcul.

Les 2 octets de `link_id` **ne sont pas transmis**. C'est l'identifiant de lien
implicite décrit dans l'ADR-0007. Il est configuré identiquement aux deux
extrémités. La valeur `0x0000` désigne « pas de distinction de lien » et rend le
mécanisme neutre.

Le CRC ne se couvre évidemment pas lui-même.

> **Limite à retenir.** Le CRC protège contre les erreurs de transmission, pas
> contre un attaquant, qui peut le recalculer librement. Le `link_id` ne change
> rien à cela : 16 bits recouvrables par force brute à partir d'une seule trame
> observée ne constituent pas un secret. Voir `threat-model.md`.

## 6. Délimitation COBS

La trame décodée est encodée avec **COBS**, puis suivie d'un délimiteur `0x00`.

### Rappel de l'algorithme

COBS supprime tout octet nul de la sortie. Les données sont découpées en blocs
terminés soit par un octet nul du message, soit par 254 octets non nuls. Chaque
bloc est précédé d'un octet de code valant `longueur_du_bloc + 1`, donc compris
entre `0x01` et `0xFF`. Un code valant `0xFF` signifie un bloc plein de 254
octets non suivi d'un zéro ; tout autre code signifie que le bloc était terminé
par un octet nul du message d'origine.

La sortie ne contient donc jamais `0x00`, ce qui fait de cet octet un
délimiteur sans ambiguïté.

### Surcoût

Pour *n* octets d'entrée, la sortie COBS fait au plus `n + 1 + ⌊n / 254⌋`
octets, auxquels s'ajoute le délimiteur.

| Trame décodée | Sur le fil (maximum) |
|---|---|
| 9 octets (minimum) | 11 octets |
| 265 octets (maximum par défaut) | 268 octets |

`FWB_FRAMING_MAX_WIRE_SIZE` vaut donc **268** en configuration par défaut. C'est
la taille à donner au tampon de réception.

### Cas limites normatifs

1. Deux délimiteurs consécutifs, ou un délimiteur en tête de flux, délimitent une
   trame **vide**. Elle est ignorée silencieusement, sans code d'erreur : c'est
   le cas normal d'un récepteur qui démarre en cours de transmission.
2. Un octet de code valant `0x00` dans la zone encodée est invalide et donne
   `FWB_ERR_COBS_MALFORMED`.
3. Un octet de code qui désignerait un bloc dépassant la fin de la zone encodée
   est invalide et donne `FWB_ERR_COBS_MALFORMED`.
4. Une zone encodée dont le décodage produit moins de 9 octets donne
   `FWB_ERR_FRAME_TOO_SHORT`.

## 7. Ordre des contrôles au décodage

**Cet ordre est normatif.** Il détermine quel code d'erreur est renvoyé lorsque
plusieurs conditions sont violées simultanément, donc les vecteurs de test et la
matrice de traçabilité en dépendent directement.

Sur la trame décodée, de longueur `n` :

| # | Contrôle | Code en cas d'échec |
|---|---|---|
| 1 | `n ≥ 9` | `FWB_ERR_FRAME_TOO_SHORT` |
| 2 | `LEN ≤ FWB_FRAMING_MAX_PAYLOAD` | `FWB_ERR_PAYLOAD_TOO_LARGE` |
| 3 | `n == 7 + LEN + 2` | `FWB_ERR_LENGTH_MISMATCH` |
| 4 | CRC calculé == CRC reçu | `FWB_ERR_BAD_CRC` |
| 5 | `VER == 0x01` | `FWB_ERR_BAD_VERSION` |
| 6 | `FLAGS == 0x00` | `FWB_ERR_BAD_FLAGS` |

### Justification de cet ordre

Les contrôles se répartissent en trois groupes, dans cet ordre précis :

1. **Contrôles structurels (1 à 3)** : ils sont indispensables *avant* de pouvoir
   localiser le CRC en mémoire. Les exécuter en premier garantit qu'aucune
   lecture hors limites n'est possible, quelle que soit l'entrée. C'est la
   propriété la plus importante du décodeur.
2. **Contrôle d'intégrité (4)** : une fois la structure sûre, on vérifie que les
   octets sont ceux qui ont été émis.
3. **Contrôles sémantiques (5 et 6)** : ils ne s'appliquent qu'à des données dont
   l'intégrité est déjà établie.

Placer le CRC **avant** les contrôles de version et de drapeaux est délibéré :
une trame corrompue doit être signalée comme corrompue. Si `VER` était vérifié
en premier, un bit inversé dans l'octet 0 produirait `FWB_ERR_BAD_VERSION`,
diagnostic trompeur qui ferait chercher une incompatibilité de protocole là où
il n'y a que du bruit sur la ligne.

Inversement, le contrôle de longueur (2 et 3) **doit** précéder le CRC, puisque
la position du CRC dépend de `LEN`. L'ordre n'est donc pas libre : il est
contraint par la sûreté d'accès mémoire, puis choisi par qualité de diagnostic.

## 8. Encodage, procédure normative

Entrées : `type`, `seq`, `payload`, `payload_len`, `link_id`.

1. Rejeter si `payload_len > FWB_FRAMING_MAX_PAYLOAD`.
2. Rejeter si le tampon de sortie fait moins de `FWB_FRAMING_MAX_WIRE_SIZE`
   octets, ou moins que la taille exacte calculée pour cette trame.
3. Écrire `VER = 0x01`, `FLAGS = 0x00`, `TYPE = type`, puis `SEQ` et `LEN` en
   little-endian.
4. Copier le payload. Si `payload_len == 0`, aucune copie n'est faite et le
   pointeur de payload peut être nul.
5. Calculer le CRC sur `link_id` suivi des `7 + LEN` octets écrits, puis l'écrire
   en little-endian.
6. Encoder le tout en COBS dans le tampon de sortie, puis ajouter `0x00`.

L'incrémentation de `SEQ` n'est **pas** réalisée par l'encodeur : elle appartient
à l'appelant, pour que l'encodeur reste une fonction pure (voir
`docs/architecture.md`).

## 9. Exemple complet

Trame de référence `FRM-V-002` : `TYPE = 0x20`, `SEQ = 1`, payload `"ABC"`,
`link_id = 0x0000`.

```
Champs :
  VER     = 01
  FLAGS   = 00
  TYPE    = 20
  SEQ     = 01 00        (1, little-endian)
  LEN     = 03 00        (3, little-endian)
  PAYLOAD = 41 42 43     ("ABC")

Entrée du CRC (12 octets) :
  00 00                  link_id = 0x0000
  01 00 20 01 00 03 00   en-tête
  41 42 43               payload
  → CRC = 0xFFF3

Trame décodée (12 octets) :
  01 00 20 01 00 03 00 41 42 43 F3 FF

Trame sur le fil (14 octets) :
  02 01 03 20 01 02 03 06 41 42 43 F3 FF 00
```

Avec `link_id = 0xBEEF` et des champs par ailleurs identiques, le CRC devient
`0xD0C6` et la trame décodée se termine par `C6 D0`. C'est l'illustration
directe du mécanisme de l'ADR-0007 : les octets transmis sont les mêmes à
l'exception du CRC, et un récepteur configuré avec un autre `link_id` rejette la
trame.

L'ensemble des vecteurs, valides et invalides, se trouve dans
[`../tests/vectors/README.md`](../tests/vectors/README.md).

## 10. Questions explicitement tranchées

Pour éviter toute réinterprétation ultérieure, les décisions suivantes sont
figées :

| Question | Réponse |
|---|---|
| `LEN` couvre-t-il l'en-tête ? | Non, le payload seul. |
| `LEN` couvre-t-il le CRC ? | Non. |
| Le CRC couvre-t-il le `link_id` ? | Oui, en préfixe du calcul, mais le `link_id` n'est pas transmis. |
| Le CRC couvre-t-il les octets de COBS ? | Non. Le CRC est calculé avant l'encodage COBS. |
| Un payload vide est-il licite ? | Oui, `LEN = 0`. |
| `SEQ` est-il incrémenté par la bibliothèque ? | Non, par l'appelant. |
| Les bits réservés de `FLAGS` sont-ils ignorés ? | Non, ils provoquent un rejet. |
| Que vaut `VER` en v1 ? | `0x01`. Toute autre valeur est rejetée. |
| Le format transporte-t-il des flottants ? | Non. L'application sérialise elle-même. |
| Quel est le maximum sur le fil ? | 268 octets en configuration par défaut. |
