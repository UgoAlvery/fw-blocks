# `fwb_framing`

Mise en trame pour liaison série : délimitation sans ambiguïté, contrôle
d'intégrité par CRC, et récepteur en flux capable de se resynchroniser après
une erreur.

**État : implémenté et testé.** `check_docs.py --strict-tests --brick framing` passe à 100 %.

## Ce que la brique garantit

- Un format de trame spécifié octet par octet, identique sur toute plateforme
  (`docs/wire-format.md`).
- Une délimitation par COBS : l'octet `0x00` ne peut pas apparaître à
  l'intérieur d'une trame encodée, donc la resynchronisation après erreur est
  déterministe et immédiate.
- La détection de toute erreur de 1 ou 2 bits, de toute erreur portant sur un
  nombre impair de bits, et de toute rafale d'au plus 16 bits consécutifs.
- Un décodeur qui ne lit jamais hors de ses tampons, quelle que soit l'entrée.
- Un code d'erreur distinct par cause de rejet.
- Aucune allocation dynamique, aucune dépendance à une horloge, aucune
  dépendance matérielle.
- Une taille de tampon connue à la compilation : 268 octets par défaut.

## Ce que la brique ne garantit pas

- **Aucune protection contre un attaquant.** Le CRC se recalcule librement, et
  l'identifiant de lien n'est pas un secret. Voir `docs/threat-model.md`.
- **Aucune livraison fiable** : ni acquittement, ni retransmission, ni contrôle
  de flux.
- **Aucune détection de rejeu** : c'est le rôle de la brique `antireplay`, que
  l'application câble au-dessus.
- **Aucune détection de retard** : il faudrait une horloge.
- **Aucune interprétation du payload** : la brique transporte des octets.

## Format en un coup d'œil

```
trame décodée :  VER │ FLAGS │ TYPE │ SEQ │ LEN │ PAYLOAD │ CRC
                  1      1       1     2     2    0..256    2

sur le fil    :  COBS(trame décodée) │ 0x00
```

- Little-endian pour tous les champs multi-octets.
- `LEN` couvre le payload **seul**.
- Le CRC (CRC-16/CCITT-FALSE, `init = 0xFFFF`) couvre `VER` jusqu'à la fin du
  payload, précédé d'un identifiant de lien de 16 bits **non transmis**.
- Surcoût total : 9 octets de codec, plus 2 à 3 octets de délimitation.

Le détail normatif est dans [`docs/wire-format.md`](docs/wire-format.md).

## Usage

```c
fwb_framing_cfg_t cfg = { .link_id = 0x1234 };

/* Émission */
uint8_t wire[FWB_FRAMING_MAX_WIRE_SIZE];
size_t n;
fwb_framing_encode(&cfg, type, seq++, payload, len, wire, sizeof wire, &n);

/* Réception, un octet à la fois */
static fwb_framing_rx_t rx;
static fwb_framing_frame_t frame;
bool ready = false;

fwb_framing_rx_init(&rx, &cfg, 50);
fwb_status_t st = fwb_framing_rx_push(&rx, byte, &frame, &ready);
```

L'interface complète est spécifiée dans [`docs/api.md`](docs/api.md).

## Précautions d'intégration

**Les structures sont volumineuses.** `fwb_framing_rx_t` contient un tampon de
267 octets et `fwb_framing_frame_t` une copie du payload de 256 octets. Les
déclarer en statique, pas sur la pile, surtout dans une tâche à pile réduite.

**Un contexte de réception n'est pas réentrant.** S'il est alimenté depuis une
interruption et consulté depuis la boucle principale, c'est à l'appelant de le
protéger. La brique ne désactive jamais les interruptions pour vous.

**L'incrémentation de `SEQ` appartient à l'appelant.** L'encodeur transmet la
valeur fournie sans la modifier, ce qui permet de rester une fonction pure et de
retransmettre un contenu avec un numéro de séquence maîtrisé.

**Les deux extrémités doivent partager la même configuration** :
`FWB_FRAMING_MAX_PAYLOAD`, `link_id` et version du format.

### Symptôme à connaître

**100 % des trames rejetées avec `FWB_ERR_BAD_CRC`, dès la première.** La cause
la plus probable n'est pas une liaison bruitée, mais un `link_id` différent aux
deux extrémités. Une liaison réellement bruitée produit un taux d'erreur partiel
et irrégulier, pas un rejet total et immédiat. C'est la contrepartie assumée de
l'ADR-0007.

## Configuration

| Macro | Défaut | Effet |
|---|---|---|
| `FWB_FRAMING_MAX_PAYLOAD` | 256 | Payload maximal. Dimensionne les structures. |
| `FWB_NO_STATUS_STRINGS` | non défini | Supprime les chaînes de diagnostic pour économiser la flash. |

## Documentation

| Fichier | Contenu |
|---|---|
| [`docs/wire-format.md`](docs/wire-format.md) | Format normatif, octet par octet, et ordre des contrôles |
| [`docs/requirements.md`](docs/requirements.md) | Exigences numérotées et méthodes de vérification |
| [`docs/threat-model.md`](docs/threat-model.md) | Menaces couvertes, et surtout celles qui ne le sont pas |
| [`docs/rx-state-machine.md`](docs/rx-state-machine.md) | États, transitions, temporisation, invariants |
| [`docs/api.md`](docs/api.md) | Interface en prose, propriété de la mémoire |
| [`tests/vectors/README.md`](tests/vectors/README.md) | Vecteurs de référence et ce que chacun vérifie |

## Dépendances

`fwb_common`, `fwb_crc`.
