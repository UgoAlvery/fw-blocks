# `fwb_common`

Le strict minimum partagé par les autres briques. **Aucune logique.**

**État : spécifié, non implémenté.**

## Contenu autorisé

| Élément | Rôle |
|---|---|
| `fwb_status_t` | Énumération des codes d'erreur, `FWB_OK == 0`, erreurs strictement négatives |
| `fwb_status_str` | Libellé textuel d'un code, supprimable par `FWB_NO_STATUS_STRINGS` |
| Helpers d'endianness | Lecture et écriture d'entiers 16 et 32 bits, little-endian et big-endian, sans hypothèse d'alignement |
| Assertions statiques | Vérification de `CHAR_BIT == 8` et des largeurs de types |

La liste complète des codes d'erreur est dans
[`../docs/conventions.md`](../docs/conventions.md).

## Contenu interdit

- Toute logique de protocole.
- Tout tampon ou contexte partagé entre briques.
- Toute dépendance à une horloge, à un système d'exploitation ou à du matériel.
- Toute fonction utilitaire ajoutée « parce qu'elle pourrait servir ».

Cette dernière interdiction est la plus importante. Un module commun grossit par
petites additions individuellement raisonnables, et chaque ajout rapproche un
peu plus les briques les unes des autres. Si un besoin n'est partagé que par une
brique, il reste dans cette brique.

## Helpers d'endianness

Ils travaillent sur `uint8_t *` et traitent les octets un par un, sans
déréférencer de pointeur vers un type plus large. C'est ce qui garantit
l'absence d'accès non aligné et l'indépendance vis-à-vis de l'endianness de la
machine, conformément à l'[ADR-0003](../docs/adr/0003-serialisation-champ-par-champ.md).

```
fwb_rd_u16_le(p)        lit 2 octets en little-endian
fwb_wr_u16_le(p, v)     écrit 2 octets en little-endian
fwb_rd_u32_le(p)        …
```

Les variantes big-endian existent pour les protocoles tiers, même si le format
du dépôt est entièrement little-endian.

## Dépendances

Aucune, hors `<stdint.h>`, `<stdbool.h>`, `<stddef.h>` et `<limits.h>`.
