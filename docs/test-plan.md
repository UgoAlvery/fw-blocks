# Plan de test

Écrit avant le code, comme le reste. Il décrit **comment** chaque famille
d'exigences sera vérifiée ; le détail exigence par exigence est dans
[`traceability.md`](traceability.md).

## 1. Niveaux de test

| Niveau | Objet | Outil |
|---|---|---|
| Unitaire | Une fonction, un comportement nommé dans une exigence | Unity |
| Vecteurs | Conformité au format normatif | Unity, données de `framing/tests/vectors/` |
| Injection de fautes | Comportement sur entrée dégradée, générée mécaniquement | Unity |
| Fuzzing | Robustesse mémoire et propriétés, sur l'espace des entrées | libFuzzer + ASan + UBSan |
| Différentiel | Deux chemins censés être équivalents | libFuzzer |

Il n'y a pas de niveau « intégration » séparé : les briques n'ont pas de
dépendance externe, et le chaînage `framing` → `antireplay` est fait par
l'application, donc testé dans l'exemple plutôt que dans la bibliothèque.

## 2. Tests unitaires

Un fichier de test par thème, pas par fonction. Chaque cas de test porte en
commentaire le ou les identifiants d'exigence qu'il vérifie :

```c
/* FRM-REQ-033 : LEN incohérent avec la longueur réelle */
void test_decode_rejette_len_incoherent(void) { … }
```

Cette convention permet de vérifier automatiquement que la matrice de
traçabilité est à jour, par simple recherche textuelle des identifiants dans
l'arborescence de tests. Un identifiant présent dans une spécification mais
absent des tests est une régression de couverture, détectée en CI.

## 3. Vecteurs de référence

Les vecteurs de `framing/tests/vectors/vectors.txt` sont chargés tels quels. Le
test vérifie, pour chacun :

- que l'encodage des champs donnés produit exactement les octets attendus ;
- que le décodage de ces octets produit exactement les champs attendus ;
- que le code de retour est exactement celui annoncé, et pas seulement
  « une erreur ».

Ce dernier point est essentiel : un décodeur qui rejetterait tout passerait
n'importe quel jeu de tests négatifs qui ne vérifie pas le code précis.

## 4. Injection de fautes

Générée mécaniquement à partir des vecteurs valides, pas écrite à la main.

| Famille | Construction | Attendu |
|---|---|---|
| Inversion de bit | Les 8 × `n` inversions d'un bit unique | Rejet systématique |
| Troncature | Chaque préfixe strict | Rejet, ou aucune trame livrée |
| Insertion de `0x00` | Un `0x00` à chaque position | Rejet, ou aucune trame livrée |
| Insertion d'un octet quelconque | Un octet à chaque position, valeurs échantillonnées | Rejet, ou aucune trame livrée |
| Permutation | Échange de deux octets adjacents | Rejet, ou aucune trame livrée |

« Aucune trame livrée » est une issue acceptable au même titre que le rejet : une
troncature peut simplement laisser le récepteur en attente du délimiteur, ce qui
est correct. Ce qui est interdit, c'est la livraison d'une trame corrompue.

L'inversion d'un bit **unique** est toujours détectée par le polynôme `0x1021`,
ce qui rend l'attente déterministe. Les inversions multiples ne sont pas testées
avec une attente de rejet, puisque la détection n'est alors que probabiliste.

## 5. Fuzzing

### Harness prévus

| Harness | Entrée | Propriétés vérifiées |
|---|---|---|
| `fuzz_crc` | Octets quelconques | Pas d'accès hors limites ; découpage en fragments équivalent à un appel unique (CRC-REQ-031) |
| `fuzz_crc_impl` | Octets quelconques | Les deux implémentations donnent le même résultat (CRC-REQ-040) |
| `fuzz_cobs` | Octets quelconques | Aller-retour exact ; aucun `0x00` en sortie ; surcoût borné |
| `fuzz_rx` | Flux d'octets quelconque | Robustesse mémoire ; toute trame livrée se ré-encode à l'identique |
| `fuzz_rx_chunking` | Flux + découpage | Appels unitaires et par lot donnent le même résultat (FRM-REQ-081) |
| `fuzz_antireplay` | Suite de valeurs 16 bits | Comparaison avec un modèle de référence naïf (AR-REQ-041, AR-REQ-042) |

### Propriétés plutôt qu'absence de crash

Un harness qui se contente de ne pas planter ne vérifie que la sûreté mémoire.
Chaque harness vérifie en plus une **propriété** :

- *aller-retour* : décoder puis ré-encoder redonne l'entrée, et réciproquement ;
- *équivalence* : deux chemins censés être identiques le sont réellement ;
- *modèle de référence* : une implémentation naïve et lente, écrite pour le test,
  produit les mêmes décisions que la brique.

Le modèle de référence est l'outil le plus efficace pour `antireplay`, dont les
erreurs probables sont logiques et non mémorielles : une erreur d'un bit dans un
décalage ne plante pas, elle accepte un rejeu.

### Corpus et critères d'arrêt

- Un corpus initial est versionné : les vecteurs de référence, plus quelques
  flux construits à la main (trames concaténées, délimiteurs multiples,
  troncatures).
- Toute entrée ayant révélé un défaut est ajoutée au corpus de régression, avec
  un test unitaire dédié. Un défaut trouvé par fuzzing devient un test nommé.
- En CI : rejeu du corpus versionné uniquement (`-runs=0`, sans exploration),
  via `ctest --preset fuzz-corpus`. Une régression sur une entrée connue est
  **bloquante**. L'exploration n'a pas sa place en CI : son résultat est non
  déterministe et sa durée utile sans rapport avec le temps acceptable sur une
  *pull request*.
- Avant un marquage de version : au minimum une heure par harness, sans
  découverte nouvelle.

Compilation sous ASan et UBSan systématiquement. Sans sanitizers, un accès hors
limites qui reste dans la page mémoire ne provoque aucun symptôme.

## 6. Tests exclus et pourquoi

| Test | Raison de l'exclusion |
|---|---|
| Couverture de code comme critère d'acceptation | Mesurée et publiée, mais jamais érigée en objectif : viser un pourcentage produit des tests écrits pour la métrique. Le critère reste la traçabilité des exigences. |
| Tests de performance chiffrés | Trop dépendants de la cible pour être signifiants sur l'hôte. Les coûts relatifs sont documentés dans les `README`. |
| Tests sur matériel réel | Hors périmètre : les briques sont conçues pour être testables intégralement sur hôte. |
| Tests de résistance temporelle (*timing attacks*) | Sans objet : aucun secret n'est manipulé en v1. À réintroduire avec le MAC. |

## 7. Critères de sortie du stade `v0.0-spec`

Le code d'une brique peut être écrit lorsque :

1. toutes ses exigences sont rédigées et numérotées ;
2. chaque exigence a une ligne dans la matrice de traçabilité, avec un test
   prévu ;
3. ses vecteurs de référence existent et ont été produits par un oracle
   indépendant du code à venir ;
4. la CI est verte sur le dépôt sans code métier.

Une brique est considérée comme terminée lorsque :

1. `python3 tools/check_docs.py --strict-tests --brick <nom>` passe, c'est-à-dire
   que chaque exigence non documentaire de la brique porte son identifiant dans
   au moins un test ;
2. tous ces tests passent ;
3. les harness de fuzzing tournent une heure sans découverte ;
4. aucun avertissement de compilation, de `clang-tidy` ni de `cppcheck` ;
5. son `README` décrit fidèlement ce qu'elle garantit **et ce qu'elle ne
   garantit pas**.

Le critère 1 est volontairement **hors CI** : la couverture y est publiée comme
indicateur, jamais comme condition de fusion. Une couverture bloquante dès le
premier test écrit rendrait la CI rouge pendant tout le développement d'une
brique, et la réaction naturelle serait de désactiver le contrôle.
