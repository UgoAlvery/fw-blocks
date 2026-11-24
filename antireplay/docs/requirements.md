# Exigences — `fwb_antireplay`

Mêmes conventions que [`../../framing/docs/requirements.md`](../../framing/docs/requirements.md).

Notation : `W` désigne `FWB_ANTIREPLAY_WINDOW`, `delta` désigne
`(int16_t)(seq − last_seq)`.

---

## 0. Besoins

Un besoin dit ce qu'on veut obtenir, jamais comment.

| ID | Besoin |
|---|---|
| B1 | Détecter le rejeu d'une trame déjà traitée et signaler les pertes dans la séquence, sans bloquer le flux légitime. |
| B2 | Tolérer un désordre limité d'arrivée sans rejeter des trames légitimes. |
| B3 | Fonctionner correctement malgré le rebouclage du compteur de séquence sur 16 bits. |
| B4 | Exposer explicitement, plutôt que masquer, l'absence de protection au redémarrage. |
| B5 | Rester sûr face à n'importe quelle suite de valeurs, sans comportement indéfini ni état incohérent. |
| B6 | Fonctionner avec un état de taille fixe connue à la compilation, sans dépendance externe. |

## 1. Exigences générales

| ID | Exigence | Besoin | Vérif. |
|---|---|---|---|
| AR-REQ-001 | La brique ne doit appeler aucune fonction d'allocation dynamique. | B6 | INSP |
| AR-REQ-002 | La brique ne doit dépendre d'aucune horloge ni d'aucun matériel. | B6 | INSP |
| AR-REQ-003 | La brique ne doit dépendre d'aucune autre brique que `fwb_common`, et en particulier pas de `fwb_framing`. | B6 | INSP |
| AR-REQ-004 | Toute fonction publique recevant un pointeur nul doit renvoyer `FWB_ERR_INVALID_ARG` sans le déréférencer. | B5 | TU |
| AR-REQ-005 | `FWB_ANTIREPLAY_WINDOW` doit être configurable à la compilation parmi 8, 16, 32 et 64, avec 64 par défaut. | B6 | TU |
| AR-REQ-006 | L'état d'une instance ne doit pas dépasser 16 octets pour `W = 64`, bourrage inclus. | B6 | INSP |

## 2. Initialisation et amorçage

| ID | Exigence | Besoin | Vérif. |
|---|---|---|---|
| AR-REQ-010 | Après initialisation, le contexte doit être dans l'état « non amorcé », distinct de tout état amorcé. | B4 | TU |
| AR-REQ-011 | Dans l'état non amorcé, la première valeur soumise doit être acceptée sans contrôle de fraîcheur, quelle qu'elle soit, y compris 0. | B4 | TU |
| AR-REQ-012 | L'acceptation de cette première valeur doit faire passer le contexte à l'état amorcé et fixer `last_seq` à cette valeur. | B4 | TU |
| AR-REQ-013 | L'état d'amorçage doit être interrogeable par l'application avant toute soumission. | B4 | TU |
| AR-REQ-014 | La réinitialisation explicite doit ramener le contexte à l'état non amorcé. | B4 | TU |

AR-REQ-011 et AR-REQ-013 formalisent la limite décrite dans le `README` : la
brique ne masque pas cette faiblesse, elle l'expose pour que l'application
décide.

## 3. Décision de fraîcheur

| ID | Exigence | Besoin | Vérif. |
|---|---|---|---|
| AR-REQ-020 | Une valeur avec `delta > 0` doit être acceptée, le masque décalé de `delta`, la valeur marquée et `last_seq` mis à jour. | B1 | TU |
| AR-REQ-021 | Une valeur avec `delta == 0` doit être rejetée avec `FWB_ERR_REPLAY`. | B1 | TU |
| AR-REQ-022 | Une valeur avec `−W < delta < 0` dont le bit n'est pas posé doit être acceptée et marquée. | B2 | TU |
| AR-REQ-023 | Une valeur avec `−W < delta < 0` dont le bit est déjà posé doit être rejetée avec `FWB_ERR_REPLAY`. | B1 | TU |
| AR-REQ-024 | Une valeur avec `delta ≤ −W` doit être rejetée avec `FWB_ERR_TOO_OLD`. | B1 | TU |
| AR-REQ-025 | Une valeur acceptée avec `delta > 1` doit renvoyer `FWB_ERR_SEQ_GAP` **tout en étant acceptée**. | B1 | TU |
| AR-REQ-026 | Une valeur acceptée avec `delta == 1` doit renvoyer `FWB_OK`. | B1 | TU |
| AR-REQ-027 | Une valeur rejetée ne doit modifier aucun champ de l'état. | B5 | TU |
| AR-REQ-028 | Avec `delta ≥ W`, le masque doit être entièrement remis à zéro avant marquage, sans décalage de largeur indéfinie. | B5 | TU, FUZZ |

AR-REQ-028 vise un piège classique : en C, décaler un entier d'un nombre de bits
supérieur ou égal à sa largeur est un comportement **indéfini**. Le cas
`delta ≥ W` doit donc être traité par une branche explicite, jamais par le
décalage générique.

AR-REQ-025 est l'unique entorse du dépôt à la règle « code non nul = échec ».
Elle est signalée ici, dans `../../docs/conventions.md` et dans l'ADR-0004.

## 4. Arithmétique modulaire

| ID | Exigence | Besoin | Vérif. |
|---|---|---|---|
| AR-REQ-030 | Le passage de `0xFFFF` à `0x0000` doit être traité comme une progression normale, sans cas particulier et sans rejet. | B3 | TU |
| AR-REQ-031 | La comparaison doit rester correcte pour tout écart réel strictement inférieur à 32768. | B3 | TU, FUZZ |
| AR-REQ-032 | Le calcul de `delta` ne doit provoquer aucun dépassement sur type signé détectable par UndefinedBehaviorSanitizer. | B3 | FUZZ |
| AR-REQ-033 | Une séquence de 70000 valeurs consécutives, couvrant plus d'un cycle complet du compteur, doit être intégralement acceptée. | B3 | TU |

AR-REQ-032 mérite une précision d'implémentation : la soustraction doit être
effectuée en arithmétique **non signée** sur 16 bits, et seul le résultat doit
être réinterprété en signé. Soustraire directement deux valeurs promues en `int`
produirait un résultat hors de l'intervalle d'un `int16_t`, donc une conversion
définie par l'implémentation et non par le standard.

## 5. Robustesse

| ID | Exigence | Besoin | Vérif. |
|---|---|---|---|
| AR-REQ-040 | Aucune suite de valeurs, quelle qu'elle soit, ne doit provoquer un comportement indéfini. | B5 | FUZZ |
| AR-REQ-041 | Aucune valeur déjà acceptée ne doit pouvoir être acceptée une seconde fois tant qu'elle reste dans la fenêtre. | B1 | FUZZ |
| AR-REQ-042 | Pour toute suite de valeurs, le nombre d'acceptations distinctes ne doit jamais dépasser le nombre de valeurs distinctes soumises. | B1 | FUZZ |
| AR-REQ-043 | Le temps de décision doit être borné et indépendant de l'historique. | B6 | INSP |

AR-REQ-041 et AR-REQ-042 sont des propriétés vérifiées par un modèle de
référence en parallèle du code pendant le fuzzing : un ensemble naïf des valeurs
récemment acceptées, dont les décisions sont comparées à celles de la brique.

## 6. Hors périmètre

| Point | Statut |
|---|---|
| Persistance de l'état en mémoire non volatile | Hors périmètre v1. Voir le `README`. |
| Amorçage par échange d'aléa en début de session | Hors périmètre v1. Suppose une authentification. |
| Détection de retard | Hors périmètre. Nécessite une horloge. |
| Numéros de séquence de plus de 16 bits | Hors périmètre. Impliquerait une évolution du format de trame. |
| Plusieurs émetteurs sur un même contexte | Hors périmètre. Un contexte par émetteur. |