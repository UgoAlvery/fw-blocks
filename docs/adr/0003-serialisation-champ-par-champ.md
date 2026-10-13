# ADR-0003 — Sérialisation champ par champ, endianness explicite

- **Statut** : accepté
- **Date** : 2026-10-13

## Contexte

Il est tentant de définir une `struct` correspondant à l'en-tête de trame et de
la recopier dans le tampon d'émission avec un `memcpy`. C'est court, rapide, et
c'est une source classique de défauts d'interopérabilité.

## Décision

Aucune structure n'est jamais recopiée vers ou depuis un tampon destiné à la
transmission. Chaque champ est lu et écrit **individuellement**, octet par
octet, avec un ordre d'octets explicite.

L'ordre retenu pour tous les champs multi-octets du dépôt est le
**little-endian**.

Les conversions passent par des helpers de `common` (`fwb_rd_u16_le`,
`fwb_wr_u16_le`, et leurs équivalents 32 bits), qui travaillent sur des
`uint8_t *` sans hypothèse d'alignement.

Les nombres à virgule flottante ne sont **pas** des types transportés par le
format. Une application qui veut transmettre un flottant le convertit elle-même
en motif binaire de 32 bits et documente la représentation retenue.

## Justification

- **Padding.** Le compilateur peut insérer des octets de bourrage entre les
  champs d'une structure. Leur présence et leur valeur ne sont pas spécifiées,
  donc la taille et la disposition changent d'une plateforme à l'autre.
- **Endianness.** Recopier une structure transmet l'ordre d'octets de la machine
  émettrice, qui n'est pas forcément celui du récepteur.
- **Alignement.** Lire un `uint16_t` à une adresse impaire est un comportement
  indéfini en C, et provoque une exception matérielle sur plusieurs cœurs, dont
  certains Cortex-M.
- **Les attributs `packed` ne suffisent pas.** Ils règlent le padding mais pas
  l'endianness, ils ne sont pas standard, et ils génèrent des accès non alignés
  silencieux.
- **Le flottant est pire encore.** La représentation, bien que presque toujours
  IEEE 754, n'est pas garantie par le standard C, et son ordre d'octets peut
  différer de celui des entiers sur certaines plateformes historiques.

Le choix du little-endian plutôt que de l'ordre réseau est pragmatique : les
cibles visées (ARM Cortex-M, RISC-V, x86) sont little-endian, ce qui évite une
conversion sur le chemin le plus courant. Le format étant explicite, un
récepteur big-endian reste interopérable sans modification du format.

## Conséquences

Positives :

- Le format sur le fil est identique sur toutes les plateformes, et vérifiable
  contre les vecteurs d'or indépendamment de la machine de test.
- Les helpers sont triviaux à vérifier et à fuzzer.

Négatives, assumées :

- Le code d'encodage et de décodage est plus verbeux qu'un `memcpy`.
- Un coût d'exécution marginal par champ, négligeable au regard du calcul du CRC.

## Alternatives écartées

- **`struct` avec `__attribute__((packed))`.** Non standard, ne règle pas
  l'endianness, génère des accès non alignés.
- **Ordre réseau (big-endian).** Défendable, mais impose une conversion
  systématique sur toutes les cibles visées pour un bénéfice symbolique.
- **Transmettre des flottants comme type du format.** Reporterait sur le format
  une hypothèse de représentation qui relève de l'application.
