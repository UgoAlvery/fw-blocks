# ADR-0001 — Cœur en C11, wrapper C++ optionnel et différé

- **Statut** : accepté
- **Date** : 2026-10-13

## Contexte

Le dépôt vise à la fois des cibles embarquées contraintes et une utilisation
confortable depuis du code applicatif moderne. Les deux langages candidats, C et
C++, ont des publics et des contraintes différents. Mélanger les deux sans règle
produit des bibliothèques où l'on ne sait plus quelle partie est utilisable
depuis quel contexte.

## Décision

Le cœur de chaque brique est écrit en **C11**. Un wrapper **C++17** optionnel
peut venir par-dessus, dans une cible CMake distincte qui dépend de la cible C.

Le wrapper ne contient **aucune logique métier** : il enveloppe l'API C avec des
types forts, du RAII sur les contextes et éventuellement de la configuration par
templates. Il est sans exceptions, sans RTTI et sans allocation dynamique.

Le wrapper n'est pas écrit tant que l'API C n'est pas stabilisée par au moins
une brique complète, testée et fuzzée.

## Justification

- **Portabilité des toolchains.** Toutes les chaînes de compilation embarquées
  offrent un C conforme ; ce n'est pas le cas pour un C++ récent et complet.
- **Référentiel normatif.** MISRA C est le référentiel le plus courant dans
  l'embarqué critique, et il s'applique au C.
- **Stabilité de l'ABI.** Une API C est stable et appelable depuis à peu près
  tout, y compris Python, Rust ou un banc de test.
- **Auditabilité.** Le C limite le nombre de constructions implicites à vérifier
  lors d'une relecture de sûreté.
- **Ordre de travail.** Un wrapper écrit trop tôt fige des choix d'API que
  l'implémentation du cœur remettra probablement en cause.

## Conséquences

Positives :

- Le cœur est utilisable depuis n'importe quel contexte.
- Les deux usages, C pur et C++, sont démontrables sur le même code.

Négatives, assumées :

- Deux référentiels de règles à maintenir, MISRA C pour le cœur et un jeu de
  règles C++ pour le wrapper, configurés séparément dans `clang-tidy`.
- Les en-têtes publics doivent rester du C valide, ce qu'un jeu de tests écrit
  en C++ ne vérifie pas. D'où la cible `fwb_header_c_compile`, qui compile
  chaque en-tête public dans un `.c` minimal.
- Les gardes `extern "C"` sont obligatoires dans tous les en-têtes publics.

## Alternatives écartées

- **Tout en C++.** Exclut les cibles dont la toolchain C++ est incomplète, et
  rend l'audit plus coûteux.
- **Tout en C sans wrapper.** Fonctionne, mais prive le dépôt d'une démonstration
  utile et d'un confort réel côté applicatif.
- **Du C compilé en C++.** Masque les différences réelles entre les deux
  langages, notamment sur les conversions implicites depuis `void *`. Écarté.
