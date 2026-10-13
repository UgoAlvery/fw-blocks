# ADR-0006 — Délimitation par COBS plutôt que par octet de début

- **Statut** : accepté
- **Date** : 2026-10-13

## Contexte

Sur une liaison orientée flux (UART, par exemple), les octets arrivent sans
frontière de message. Le récepteur doit retrouver seul le début et la fin de
chaque trame, y compris après une perte d'octets ou un démarrage en cours de
transmission.

L'approche la plus répandue utilise un octet de début arbitraire, par exemple
`0xAA`, suivi d'un champ de longueur. Elle a un défaut connu : **l'octet de
début peut apparaître à l'intérieur du payload**. Un récepteur qui se
resynchronise après une erreur peut alors se caler sur un faux début, et
l'ambiguïté n'est levée que par le CRC, c'est-à-dire tardivement et de manière
probabiliste.

## Décision

La délimitation v1 utilise **COBS** (*Consistent Overhead Byte Stuffing*), avec
`0x00` comme délimiteur de fin de trame.

COBS transforme un bloc d'octets quelconque en un bloc qui ne contient **aucun
octet nul**. L'octet `0x00` devient donc un délimiteur sans ambiguïté possible.

La couche de délimitation est séparée du codec de trame derrière une interface
dédiée, afin de pouvoir ajouter plus tard une stratégie « octet de début +
longueur », utile sur les transports qui préservent déjà les frontières de
message (CAN, UDP).

## Justification

- **Resynchronisation triviale et déterministe.** Après une erreur, le récepteur
  jette tout jusqu'au prochain `0x00`. Il est alors garanti d'être au début
  d'une trame, sans dépendre du CRC pour lever une ambiguïté.
- **Surcoût borné et faible.** COBS ajoute au plus `1 + ⌊n/254⌋` octets pour
  *n* octets d'entrée, auxquels s'ajoute le délimiteur. Pour la trame maximale
  du projet (265 octets), le surcoût total est de 3 octets, soit 1,1 %.
- **Surcoût constant.** Contrairement à l'échappement de type SLIP, dont la
  taille de sortie dépend du contenu et peut doubler dans le pire cas, le
  surcoût de COBS ne dépend que de la longueur. Le dimensionnement des tampons
  est donc exact, ce qui compte sans allocation dynamique (ADR-0002).
- **Détection de troncature gratuite.** Un `0x00` prématuré termine la trame
  courante, qui est alors rejetée pour longueur insuffisante ou CRC faux. Avec
  un octet de début, une troncature n'est détectée qu'au message suivant.

## Conséquences

Positives :

- Pas de faux positif de début de trame, donc pas de scénario de
  resynchronisation pathologique à analyser.
- Taille de tampon de réception calculable exactement : 268 octets pour la
  configuration par défaut.

Négatives, assumées :

- Un étage d'encodage et de décodage supplémentaire à écrire, à tester et à
  fuzzer. Il est petit mais il a ses propres cas limites, notamment les blocs de
  254 octets et le zéro final.
- Le champ `LEN` de la trame devient **redondant** avec la délimitation, qui
  donne déjà la longueur. Il est conservé malgré tout, pour deux raisons : il
  sert de contrôle de cohérence croisé contre un décodage COBS erroné, et il est
  nécessaire à la future stratégie de délimitation par longueur. Ce choix est
  assumé : 2 octets contre un contrôle supplémentaire.
- COBS est moins connu qu'un octet de début, donc demande une explication dans
  la documentation du format.

## Alternatives écartées

- **Octet de début + longueur seuls.** Ambiguïté de resynchronisation, et
  longueur non protégée tant que le CRC n'est pas vérifié.
- **SLIP** (échappement de `0xC0` et `0xDB`). Simple et répandu, mais le
  surcoût dépend du contenu et atteint 100 % dans le pire cas, ce qui oblige à
  dimensionner les tampons pour le double de la trame maximale.
- **Délimitation par silence sur la ligne.** Dépend du débit et d'une horloge,
  et impose une contrainte temps réel au récepteur. Contraire à la règle
  d'indépendance vis-à-vis de l'horloge.
