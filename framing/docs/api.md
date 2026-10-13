# Spécification de l'API — `fwb_framing`

Ce document décrit l'interface **en prose**, avant toute écriture de code. Il
fixe les signatures, la propriété de la mémoire et le comportement attendu, de
façon à ce que l'implémentation n'ait plus de décision à prendre.

Les signatures sont données en pseudo-C pour fixer la forme ; les noms exacts
et les qualificateurs seront figés à l'écriture des en-têtes.

## 1. Constantes de configuration

| Constante | Défaut | Rôle |
|---|---|---|
| `FWB_FRAMING_MAX_PAYLOAD` | 256 | Payload maximal, configurable à la compilation. |
| `FWB_FRAMING_HEADER_SIZE` | 7 | Taille de l'en-tête. Non configurable. |
| `FWB_FRAMING_CRC_SIZE` | 2 | Taille du CRC. Non configurable. |
| `FWB_FRAMING_MIN_FRAME_SIZE` | 9 | Trame décodée minimale. |
| `FWB_FRAMING_MAX_FRAME_SIZE` | 265 | Trame décodée maximale. |
| `FWB_FRAMING_MAX_WIRE_SIZE` | 268 | Trame sur le fil maximale, délimiteur compris. |
| `FWB_FRAMING_VERSION` | 1 | Valeur du champ `VER`. |

Toutes doivent être utilisables dans une expression constante, donc dans la
déclaration d'un tableau et dans une assertion statique (FRM-REQ-004).

## 2. Types

### `fwb_framing_cfg_t`

Configuration partagée par l'encodeur et le décodeur, en lecture seule après
initialisation.

| Champ | Type | Rôle |
|---|---|---|
| `link_id` | `uint16_t` | Identifiant de lien implicite (ADR-0007). `0` = pas de distinction. |

Un unique champ aujourd'hui. La structure existe quand même, pour que l'ajout
d'un paramètre ne casse pas toutes les signatures.

### `fwb_framing_frame_t`

Résultat du décodage d'une trame.

| Champ | Type | Rôle |
|---|---|---|
| `type` | `uint8_t` | Champ `TYPE` reçu. |
| `seq` | `uint16_t` | Champ `SEQ` reçu. |
| `payload_len` | `uint16_t` | Longueur utile du payload. |
| `payload` | `uint8_t[FWB_FRAMING_MAX_PAYLOAD]` | Copie du payload. |

Le payload est **copié** et non référencé. Ce choix est délibéré : une vue sur
le tampon de réception serait invalidée dès l'octet suivant, ce qui est un piège
d'API dans un contexte où le flux continue d'arriver sous interruption. Le coût
est une copie de 256 octets au maximum et une structure volumineuse, qu'il faut
déclarer en statique plutôt que sur la pile.

### `fwb_framing_rx_t`

Contexte du récepteur en flux. Opaque par convention, mais de taille connue
(ADR-0002). L'appelant le déclare, ne lit ni n'écrit ses champs directement.

Contient le tampon d'accumulation de `FWB_FRAMING_MAX_WIRE_SIZE - 1` octets,
l'état courant, le nombre d'octets accumulés, le compteur d'inactivité, le seuil
de temporisation et une copie de la configuration.

## 3. Fonctions d'encodage

### `fwb_framing_encoded_size`

```
size_t fwb_framing_encoded_size(uint16_t payload_len);
```

Renvoie la taille maximale sur le fil, délimiteur compris, pour un payload de
cette longueur. Sert à dimensionner le tampon de sortie. Renvoie 0 si la
longueur dépasse le maximum.

Fonction pure, sans effet de bord, utilisable dans un contexte sans contexte.

### `fwb_framing_encode`

```
fwb_status_t fwb_framing_encode(const fwb_framing_cfg_t *cfg,
                                uint8_t type, uint16_t seq,
                                const uint8_t *payload, uint16_t payload_len,
                                uint8_t *dst, size_t dst_cap,
                                size_t *out_len);
```

Produit une trame complète, prête à émettre, délimiteur compris.

| Aspect | Comportement |
|---|---|
| Propriété de `dst` | À l'appelant. La fonction n'écrit jamais au-delà de `dst_cap`. |
| Succès | `FWB_OK`, `*out_len` reçoit le nombre d'octets écrits. |
| `cfg`, `dst` ou `out_len` nul | `FWB_ERR_INVALID_ARG`. |
| `payload` nul avec `payload_len > 0` | `FWB_ERR_INVALID_ARG` (FRM-REQ-017). |
| `payload` nul avec `payload_len == 0` | Accepté. |
| `payload_len > FWB_FRAMING_MAX_PAYLOAD` | `FWB_ERR_PAYLOAD_TOO_LARGE`. |
| `dst_cap` insuffisant | `FWB_ERR_BUFFER_TOO_SMALL`, **aucun octet écrit** (FRM-REQ-016). |
| Modification de `payload` | Jamais (FRM-REQ-018). |
| `seq` | Transmis tel quel, jamais incrémenté (FRM-REQ-019). |

Fonction pure : aucun état conservé entre deux appels.

## 4. Fonctions de décodage d'une trame isolée

Ces fonctions travaillent sur une trame **déjà délimitée**. Elles servent au
récepteur en flux, mais sont publiques pour permettre un usage sur un transport
qui préserve déjà les frontières de message (CAN, UDP), et pour être testées et
fuzzées directement.

### `fwb_framing_decode`

```
fwb_status_t fwb_framing_decode(const fwb_framing_cfg_t *cfg,
                                const uint8_t *frame, size_t frame_len,
                                fwb_framing_frame_t *out);
```

Valide une trame **décodée** (après COBS) et remplit `out`.

| Aspect | Comportement |
|---|---|
| Ordre des contrôles | Celui de `wire-format.md` §7, strictement. |
| Lecture | Jamais au-delà de `frame_len` (FRM-REQ-038). |
| Modification de `frame` | Jamais (FRM-REQ-042). |
| En cas de rejet | `out` n'est pas modifié (FRM-REQ-039). |
| Codes d'erreur possibles | `FRAME_TOO_SHORT`, `PAYLOAD_TOO_LARGE`, `LENGTH_MISMATCH`, `BAD_CRC`, `BAD_VERSION`, `BAD_FLAGS`, `INVALID_ARG`. |

### `fwb_framing_decode_wire`

```
fwb_status_t fwb_framing_decode_wire(const fwb_framing_cfg_t *cfg,
                                     const uint8_t *wire, size_t wire_len,
                                     fwb_framing_frame_t *out);
```

Identique, mais prend une trame **sur le fil**, délimiteur exclu : décode le
COBS dans un tampon interne, puis appelle `fwb_framing_decode`.

Code d'erreur supplémentaire : `FWB_ERR_COBS_MALFORMED`.

## 5. Récepteur en flux

### `fwb_framing_rx_init`

```
fwb_status_t fwb_framing_rx_init(fwb_framing_rx_t *rx,
                                 const fwb_framing_cfg_t *cfg,
                                 uint16_t timeout_ticks);
```

Initialise le contexte. `timeout_ticks` à 0 désactive la temporisation.

Doit être appelée avant tout autre usage du contexte. Aucune allocation.

### `fwb_framing_rx_reset`

```
void fwb_framing_rx_reset(fwb_framing_rx_t *rx);
```

Ramène le contexte à l'état `IDLE` avec un tampon vide, sans toucher à la
configuration (FRM-REQ-089).

### `fwb_framing_rx_push`

```
fwb_status_t fwb_framing_rx_push(fwb_framing_rx_t *rx, uint8_t byte,
                                 fwb_framing_frame_t *out);
```

Soumet **un octet**. C'est le cœur du récepteur.

| Valeur renvoyée | Signification |
|---|---|
| `FWB_OK` **et** trame livrée | Une trame valide est disponible dans `out`. |
| `FWB_OK` **sans** livraison | Octet accumulé, rien à signaler. C'est le cas courant. |
| Code d'erreur | Une trame a été rejetée pour cette raison. Le récepteur est déjà resynchronisé. |

La distinction entre les deux cas de `FWB_OK` passe par un paramètre de sortie
booléen ou par un code `FWB_FRAME_READY` distinct. **Décision figée : un
paramètre de sortie booléen `frame_ready`**, pour que `FWB_OK` conserve
exactement une signification, conformément à l'ADR-0004.

La signature définitive est donc :

```
fwb_status_t fwb_framing_rx_push(fwb_framing_rx_t *rx, uint8_t byte,
                                 fwb_framing_frame_t *out, bool *frame_ready);
```

### `fwb_framing_rx_push_buf`

```
fwb_status_t fwb_framing_rx_push_buf(fwb_framing_rx_t *rx,
                                     const uint8_t *data, size_t len,
                                     size_t *consumed,
                                     fwb_framing_frame_t *out,
                                     bool *frame_ready);
```

Variante par lot, qui consomme les octets jusqu'à ce qu'une trame soit prête ou
qu'une erreur survienne, puis s'arrête en indiquant combien d'octets ont été
consommés. L'appelant rappelle la fonction avec le reste.

Cette forme évite un appel de fonction par octet sur les liaisons rapides. Elle
doit produire **exactement le même résultat** que des appels unitaires
équivalents (FRM-REQ-081), propriété vérifiée par fuzzing différentiel.

### `fwb_framing_rx_tick`

```
fwb_status_t fwb_framing_rx_tick(fwb_framing_rx_t *rx);
```

Signale l'écoulement d'une unité de temps. Renvoie `FWB_ERR_TIMEOUT` si cet
appel provoque l'abandon de la trame en cours, `FWB_OK` sinon.

La durée d'une unité est choisie par l'appelant : la brique ne connaît aucune
unité de temps (FRM-REQ-086).

## 6. Règles transverses

1. **Aucune fonction n'alloue.** Toute mémoire vient de l'appelant.
2. **Aucune fonction ne bloque** ni n'attend.
3. **Aucune fonction n'est réentrante vis-à-vis d'un même contexte.** Un contexte
   `rx` utilisé depuis une interruption et depuis la boucle principale doit être
   protégé par l'appelant. À documenter dans le `README` de la brique.
4. **Les fonctions sans contexte sont réentrantes et sûres en concurrence**,
   puisqu'elles ne touchent aucun état partagé.
5. **Tout pointeur obligatoire nul donne `FWB_ERR_INVALID_ARG`**, sans
   déréférencement (FRM-REQ-003).
6. **Aucune fonction n'écrit dans un paramètre de sortie en cas d'erreur**, hors
   mention explicite.

## 7. Usage type

```
/* Émission */
fwb_framing_cfg_t cfg = { .link_id = 0x1234 };
uint8_t wire[FWB_FRAMING_MAX_WIRE_SIZE];
size_t n;
static uint16_t seq;

if (fwb_framing_encode(&cfg, TYPE_CONSIGNE, seq++, data, sizeof data,
                       wire, sizeof wire, &n) == FWB_OK) {
    uart_write(wire, n);
}

/* Réception */
static fwb_framing_rx_t rx;          /* statique : la structure est volumineuse */
static fwb_framing_frame_t frame;
fwb_framing_rx_init(&rx, &cfg, 50);

for (;;) {
    uint8_t b;
    if (uart_read(&b)) {
        bool ready = false;
        fwb_status_t st = fwb_framing_rx_push(&rx, b, &frame, &ready);
        if (ready) {
            application_on_frame(&frame);
        } else if (st != FWB_OK) {
            metrics_count_error(st);   /* un compteur par code */
        }
    }
    if (tick_elapsed()) {
        (void)fwb_framing_rx_tick(&rx);
    }
}
```

Ce fragment est un **exemple d'usage**, pas du code du projet. Il sera repris
dans `../examples/` une fois l'implémentation écrite.
