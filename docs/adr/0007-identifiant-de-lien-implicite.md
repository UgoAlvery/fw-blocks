# ADR-0007 — Identifiant de lien implicite, injecté dans le CRC

- **Statut** : accepté
- **Date** : 2026-10-13

## Contexte

Un système embarqué comporte souvent plusieurs liaisons et plusieurs flux. Un
câble inversé, un bus partagé ou une erreur de routage logiciel peuvent amener
une trame parfaitement valide, mais **émise pour un autre destinataire**, à être
acceptée par le mauvais récepteur. C'est la menace de *masquerading* au sens de
l'IEC 61784-3.

Un CRC ne détecte pas cette situation : la trame n'est pas corrompue, elle est
simplement adressée ailleurs.

## Décision

Chaque extrémité d'une liaison est configurée avec un **identifiant de lien** de
16 bits, `link_id`, identique des deux côtés et **non transmis sur le fil**.

Le CRC est calculé sur la concaténation :

```
link_id (2 octets, little-endian)  ||  VER .. fin du PAYLOAD
```

Seuls les octets de la trame sont transmis. Le récepteur recalcule le CRC en
préfixant son propre `link_id`. Si les deux identifiants diffèrent, le CRC ne
correspond pas et la trame est rejetée comme corrompue.

La valeur `0x0000` est réservée au cas « pas de distinction de lien », ce qui
rend le mécanisme neutre par défaut.

C'est le principe du *Data ID* des profils E2E d'AUTOSAR.

## Justification

- **Coût nul sur le fil.** Aucun octet transmis en plus, alors qu'un champ
  explicite coûterait 2 octets par trame.
- **Réutilise un mécanisme déjà présent.** Pas de contrôle supplémentaire à
  exécuter ni à tester dans le chemin de réception.
- **Détecte aussi une erreur de configuration.** Deux extrémités mal appairées
  échouent immédiatement et totalement, plutôt que de fonctionner par
  intermittence.
- **Utilisable pour discriminer autre chose qu'un lien** : une direction de
  communication, une version d'application, un rôle. L'application choisit la
  sémantique.

## Conséquences

Positives :

- Protection contre l'insertion et le masquerading **accidentels** sans surcoût
  de transmission.
- Un câble inversé entre deux liaisons configurées différemment produit un rejet
  systématique et immédiat.

Négatives, assumées :

- **Aucune protection contre un attaquant.** Le `link_id` n'est pas un secret :
  il n'a que 16 bits, il est recouvrable par force brute à partir d'une seule
  trame observée, et il ne constitue en aucun cas une clé. Il n'adresse que les
  erreurs accidentelles. Cette limite est reprise dans le modèle de menaces.
- **Diagnostic dégradé.** Une erreur de configuration se manifeste par
  `FWB_ERR_BAD_CRC`, le même code qu'une liaison bruitée. Mitigation : un
  symptôme « 100 % des trames en CRC faux » est documenté dans le `README` de
  `framing` comme signature d'un `link_id` mal appairé.
- **Le CRC porte deux rôles.** Intégrité et appairage. C'est acceptable parce
  que la réponse est la même dans les deux cas — rejeter la trame — mais cela
  doit être explicite dans la spécification.

## Alternatives écartées

- **Champ explicite dans la trame.** Coûte 2 octets par trame et ajoute un
  contrôle, pour une protection équivalente contre les erreurs accidentelles.
- **Rien du tout.** Laisse la menace de masquerading entièrement non traitée,
  alors qu'elle figure explicitement dans la liste des menaces de sûreté de
  communication retenue.
- **MAC avec clé par lien.** C'est la vraie réponse contre un attaquant, mais
  elle relève de la sécurité et non de la sûreté. Hors périmètre v1, un
  emplacement est réservé dans le format.
