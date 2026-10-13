# Architecture

## Principe

Le dépôt est une collection de briques indépendantes, pas une bibliothèque
monolithique. Une brique se prend seule, avec ses dépendances déclarées, et
rien d'autre. Les dépendances vont dans un seul sens et le graphe est acyclique.

## Graphe de dépendances

```
                 application
                      │
        ┌─────────────┼─────────────┐
        │             │             │
   ┌────▼────┐   ┌────▼─────┐  ┌────▼──────┐
   │ framing │   │antireplay│  │   crc     │
   └────┬────┘   └────┬─────┘  └────┬──────┘
        │             │             │
        └──────► crc  │             │
                      │             │
        ┌─────────────┴─────────────┘
        │
   ┌────▼────┐
   │ common  │   codes d'erreur, types, helpers d'endianness
   └─────────┘
```

- `common` ne dépend de rien et ne contient aucune logique.
- `crc` dépend de `common` seul.
- `framing` dépend de `common` et `crc`.
- `antireplay` dépend de `common` seul. Il ne dépend **pas** de `framing` : il
  travaille sur un numéro de séquence nu, d'où qu'il vienne.
- C'est l'application qui câble `framing` et `antireplay` ensemble.

## Découpage en couches de la chaîne de réception

La séparation la plus importante du dépôt est celle entre ce qui a un état et ce
qui n'en a pas.

| Couche | Brique | État | Rôle |
|---|---|---|---|
| 1. Délimitation | `framing` | oui | Retrouver les bornes d'une trame dans un flux d'octets |
| 2. Codec de trame | `framing` | non | Construire et valider une trame déjà isolée |
| 3. Fraîcheur | `antireplay` | oui | Décider si un numéro de séquence est acceptable |
| 4. Sémantique | application | — | Interpréter le payload |

La couche 2 est composée de fonctions pures : mêmes entrées, mêmes sorties,
aucun effet de bord. C'est celle qui se teste et se fuzze le plus facilement, et
c'est pour cette raison qu'elle est isolée des couches 1 et 3.

## Ce que `common` contient, et ce qu'il ne contient pas

`common` doit rester minuscule. Dès qu'il grossit, les briques cessent d'être
indépendantes et le dépôt redevient monolithique.

Y est autorisé :

- l'énumération des codes d'erreur `fwb_status_t` ;
- les helpers de lecture et d'écriture d'entiers en little-endian et big-endian ;
- les macros de compilation conditionnelle et les attributs portables.

Y est interdit :

- toute logique de protocole ;
- tout buffer ou contexte partagé ;
- toute dépendance à une horloge, à un système d'exploitation ou à du matériel.

## Règles structurelles

1. **API publique et privée séparées.** Seul `<brique>/include/` est exposé. Les
   en-têtes de `<brique>/src/` sont internes et ne sont jamais installés.
2. **Chaque brique compile seule**, sans les autres briques sauf ses dépendances
   déclarées, et sans matériel.
3. **Aucune allocation dynamique** nulle part (voir ADR-0002).
4. **Aucune dépendance à une horloge** : le temps entre par des appels explicites
   de l'application (voir `framing/docs/rx-state-machine.md`).
5. **Une cible CMake par brique** : `fwb_common`, `fwb_crc`, `fwb_framing`,
   `fwb_antireplay`. Un éventuel wrapper C++ est une cible distincte qui dépend de
   la cible C correspondante (voir ADR-0001).

## Arborescence type d'une brique

```
<brique>/
├── README.md              garanties, non-garanties, exemple d'usage
├── CMakeLists.txt
├── docs/
│   ├── requirements.md    exigences numérotées
│   └── …                  specs propres à la brique
├── include/fwb/<brique>/  API publique, C valide, gardes extern "C"
├── src/                   implémentation et en-têtes privés
├── tests/
│   ├── unit/
│   └── vectors/           vecteurs de référence
└── fuzz/                  harness et corpus initial
```

## Portabilité visée

- Compilateurs : GCC et Clang sur hôte, `arm-none-eabi-gcc` en cible.
- Pas de dépendance à la libc au-delà de `<stdint.h>`, `<stdbool.h>`,
  `<stddef.h>` et `<string.h>`.
- Aucune hypothèse sur l'endianness de la machine : la sérialisation est
  explicite (voir ADR-0003).
- Hypothèse assumée : `CHAR_BIT == 8`. Elle est vérifiée par une assertion
  statique dans `common`.
