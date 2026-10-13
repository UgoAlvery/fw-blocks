# ADR-0002 — Aucune allocation dynamique, mémoire fournie par l'appelant

- **Statut** : accepté
- **Date** : 2026-10-13

## Contexte

Une bibliothèque de communication doit stocker des tampons de réception et des
contextes d'état. La question est de savoir qui les possède et d'où vient la
mémoire.

## Décision

Aucune brique n'appelle `malloc`, `calloc`, `realloc` ni `free`. Toute mémoire
est **fournie par l'appelant** :

- les contextes à état (`fwb_framing_rx_t`, `fwb_antireplay_t`) sont des
  structures publiques que l'appelant déclare où il veut, en statique, sur la
  pile ou dans un pool à lui ;
- les tampons de sortie sont passés en paramètre avec leur taille, et la
  fonction renvoie `FWB_ERR_BUFFER_TOO_SMALL` plutôt que d'écrire au-delà ;
- une fonction ou une macro donne la **taille maximale nécessaire** pour que
  l'appelant dimensionne ses tampons à la compilation.

La taille maximale du payload est fixée à la compilation par
`FWB_FRAMING_MAX_PAYLOAD` (256 par défaut). Elle détermine la taille des
structures publiques.

## Justification

- **Déterminisme.** L'empreinte mémoire est connue au moment de l'édition de
  liens, pas à l'exécution.
- **Absence de fragmentation**, qui est un mode de défaillance insidieux sur un
  système qui tourne des mois sans redémarrer.
- **Pas d'échec d'allocation à gérer**, donc un chemin d'erreur en moins.
- Les normes de sûreté restreignent ou interdisent l'allocation dynamique après
  l'initialisation. S'en passer entièrement évite d'avoir à justifier un usage.
- **Testabilité.** Pas de fuite possible, donc pas de dépendance à un détecteur
  de fuites pour valider un test.

## Conséquences

Positives :

- Empreinte mémoire calculable, vérifiable dans la carte d'édition de liens.
- Les briques fonctionnent sans système d'exploitation et avant l'initialisation
  d'un tas.

Négatives, assumées :

- Les structures de contexte sont publiques, donc leur disposition fait partie
  de l'API. Un changement de champ casse la compatibilité binaire. Accepté : le
  projet distribue des sources, pas une bibliothèque binaire.
- La taille du payload est figée à la compilation. Une application qui a besoin
  de deux tailles doit compiler deux instances de la brique, ou dimensionner au
  maximum.
- Les structures sont plus volumineuses qu'un pointeur opaque, ce qui pèse sur
  la pile si l'appelant les y place. Documenté dans le `README` de chaque brique.

## Alternatives écartées

- **Contextes opaques avec allocateur injecté.** Plus souple, mais réintroduit
  un chemin d'échec et de la complexité pour un bénéfice nul dans le périmètre
  visé.
- **Tampons statiques internes à la bibliothèque.** Simple, mais interdit
  plusieurs instances et rend le code non réentrant. Écarté.
