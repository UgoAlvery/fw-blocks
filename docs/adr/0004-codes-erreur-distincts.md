# ADR-0004 — Codes d'erreur distincts plutôt que booléens

- **Statut** : accepté
- **Date** : 2026-10-13

## Contexte

Un décodeur de trames peut rejeter une entrée pour une dizaine de raisons
différentes : trame trop courte, longueur incohérente, CRC faux, version
inconnue, délimitation malformée. Un retour booléen les confond toutes.

## Décision

Toute fonction susceptible d'échouer renvoie un `fwb_status_t`, énumération
définie dans `common`, avec `FWB_OK == 0` et une valeur strictement négative par
cause distincte. Les valeurs de sortie passent par des pointeurs de sortie.

Une cause de rejet = un code. On ne fusionne pas deux causes au motif qu'elles
mènent au même traitement côté appelant.

## Justification

- **Diagnostic en production.** Sur une cible embarquée, le code de retour est
  souvent la seule information disponible. Distinguer « CRC faux » de « longueur
  incohérente » oriente immédiatement : le premier suggère du bruit sur la
  liaison, le second une erreur d'implémentation de l'émetteur ou une attaque.
- **Métriques.** Un compteur par code d'erreur donne un indicateur de qualité de
  la liaison exploitable en exploitation.
- **Testabilité.** Un test peut vérifier *pourquoi* une trame a été rejetée, pas
  seulement qu'elle l'a été. Sans cela, un décodeur qui rejette tout passerait
  tous les tests négatifs.
- **Traçabilité.** Chaque exigence de rejet se relie à un code précis, donc à une
  assertion précise.

Ce dernier point est le plus important : c'est lui qui rend la matrice de
traçabilité utile plutôt que décorative.

## Conséquences

Positives :

- Les tests négatifs sont spécifiques et la matrice de traçabilité a du sens.
- L'ordre des contrôles dans le décodeur devient une décision explicite, à
  spécifier, puisqu'il détermine quel code est renvoyé quand plusieurs
  conditions sont violées. Cet ordre est figé dans
  `framing/docs/wire-format.md`.

Négatives, assumées :

- L'appelant doit convertir le statut en texte pour le journaliser. Une fonction
  `fwb_status_str` est fournie, désactivable par `FWB_NO_STATUS_STRINGS` pour
  économiser la flash.
- Un code non nul n'implique pas toujours un échec : `FWB_ERR_SEQ_GAP` est
  informatif et la trame reste acceptée. Cette exception unique est signalée
  dans `conventions.md` et dans l'exigence correspondante.

## Alternatives écartées

- **Retour booléen.** Perd l'information de cause, rend les tests négatifs non
  spécifiques.
- **Retour de la taille écrite, 0 en cas d'erreur.** Ambigu dès qu'une sortie de
  taille nulle est légitime, ce qui est le cas pour un payload vide.
- **`errno` global.** Non réentrant, incompatible avec un usage multi-instance.
