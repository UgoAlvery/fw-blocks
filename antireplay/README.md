# `fwb_antireplay`

Contrôle de fraîcheur par fenêtre glissante sur un numéro de séquence de 16
bits. Distingue une trame nouvelle, un doublon, une trame trop ancienne et un
trou dans la séquence.

**État : implémenté et testé.** `check_docs.py --strict-tests --brick antireplay` passe à 100 %.

## Ce que la brique garantit

- Rejet de tout numéro de séquence déjà accepté, dans la limite de la fenêtre.
- Acceptation des trames légèrement désordonnées, dans la limite de la fenêtre.
- Rejet des trames trop anciennes pour être vérifiables.
- Signalement des trous dans la numérotation, sans rejeter la trame.
- Gestion correcte du repli de `0xFFFF` à `0x0000`.
- Un état d'au plus 16 octets par instance, aucune allocation, aucune horloge.

## Ce que la brique ne garantit pas

- **Aucune protection contre un attaquant.** Voir la section « Limite après
  redémarrage » ci-dessous, qui est la faille structurelle de la v1.
- Aucune persistance : l'état est perdu à chaque redémarrage.
- Aucune détection de retard : sans horloge, une trame arrivée très en retard
  mais dans l'ordre est indiscernable d'une trame normale.
- Aucun lien avec `framing` : la brique travaille sur un entier nu. C'est
  l'application qui lui transmet le `SEQ` d'une trame validée.

## Principe

L'état tient en deux valeurs :

- `last_seq`, le plus grand numéro accepté au sens de la comparaison modulaire ;
- un masque de bits qui mémorise, pour les `W − 1` numéros qui le précèdent,
  lesquels ont déjà été vus.

À l'arrivée d'un numéro `seq`, on calcule `delta = (int16_t)(seq − last_seq)`,
c'est-à-dire une comparaison **modulaire** sur 16 bits, comme pour les numéros
de séquence TCP. Elle traite le repli de `0xFFFF` à `0x0000` sans cas
particulier.

| Cas | Interprétation | Action |
|---|---|---|
| `delta > 0` | Trame plus récente | Décaler le masque de `delta`, marquer, mettre `last_seq` à jour. Accepté. |
| `delta == 0` | Doublon du plus récent | Rejet, `FWB_ERR_REPLAY`. |
| `−W < delta < 0` | Trame désordonnée dans la fenêtre | Si le bit est déjà posé, `FWB_ERR_REPLAY`. Sinon accepté et marqué. |
| `delta ≤ −W` | Trop ancienne pour être vérifiable | Rejet, `FWB_ERR_TOO_OLD`. |

Avec `delta > 1`, des numéros ont été sautés : la trame est **acceptée** et
`FWB_ERR_SEQ_GAP` est renvoyé à titre informatif. C'est le seul code non nul de
tout le dépôt qui n'annule pas l'opération.

## Taille de la fenêtre

`FWB_ANTIREPLAY_WINDOW`, 64 par défaut, au choix parmi 8, 16, 32 et 64.

Le compromis : une fenêtre large tolère plus de désordre, mais prolonge la durée
pendant laquelle un rejeu peut être tenté sur un numéro jamais utilisé. Une
fenêtre de 64 convient à une liaison point à point qui réordonne peu. Sur une
cible 8 ou 16 bits, une fenêtre de 32 évite le recours à l'arithmétique 64 bits.

## Limite après redémarrage

**C'est la limite la plus importante de la brique, et elle est structurelle.**

L'état n'est pas persistant. Après un redémarrage du récepteur, la première
trame reçue est acceptée sans contrôle de fraîcheur, puisqu'il n'existe aucune
référence à laquelle la comparer.

Conséquence : un attaquant qui a enregistré une trame légitime et qui provoque
ou attend un redémarrage du récepteur peut la rejouer avec succès. La fenêtre
ne se referme qu'à partir de cette première trame.

La brique signale explicitement ce premier état via `fwb_antireplay_is_seeded()`,
pour que l'application puisse décider : refuser toute trame jusqu'à un
amorçage explicite, ou accepter le risque.

Trois réponses possibles, toutes **hors du périmètre v1** :

1. un compteur monotone persisté en mémoire non volatile, restauré au
   démarrage ;
2. un échange d'aléa en début de session, qui rend tout enregistrement ancien
   inutilisable ;
3. un horodatage signé, qui suppose une base de temps commune.

Aucune de ces réponses n'a de sens sans authentification cryptographique des
trames, ce qui renvoie à la section sécurité de
[`../framing/docs/threat-model.md`](../framing/docs/threat-model.md).

## Limite du repli de compteur

La comparaison modulaire sur 16 bits suppose que l'écart réel entre deux trames
reste inférieur à 32768. Au-delà, le signe de `delta` s'inverse et une trame
récente est prise pour une trame ancienne.

À 10 trames par seconde, cela correspond à une interruption de liaison d'environ
55 minutes. Une application dont les interruptions peuvent dépasser ce seuil
doit réinitialiser explicitement le contexte à la reprise, ou utiliser un
numéro de séquence plus large, ce qui impliquerait une évolution du format de
trame.

## Usage

```c
static fwb_antireplay_ctx_t ar;
fwb_antireplay_init(&ar);

/* après qu'une trame a été validée par fwb_framing */
fwb_status_t st = fwb_antireplay_check(&ar, frame.seq);

switch (st) {
case FWB_OK:              traiter(&frame); break;
case FWB_ERR_SEQ_GAP:     metrics_lost(); traiter(&frame); break;  /* acceptée */
case FWB_ERR_REPLAY:
case FWB_ERR_TOO_OLD:     metrics_rejected(st); break;
default:                  break;
}
```

Le piège à éviter : traiter `FWB_ERR_SEQ_GAP` comme un rejet. La trame est
valide et doit être traitée ; seul le compteur de pertes est concerné.

## Documentation

| Fichier | Contenu |
|---|---|
| [`docs/requirements.md`](docs/requirements.md) | Exigences numérotées et cas limites |

## Dépendances

`fwb_common`. **Pas** `fwb_framing` : la brique ne connaît que des entiers.
