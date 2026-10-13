# Modèle de menaces — `fwb_framing`

Ce document distingue deux analyses qu'il ne faut jamais confondre :

- la **sûreté de communication**, qui traite les défaillances *accidentelles* —
  bruit, câblage, erreurs logicielles ;
- la **sécurité**, qui traite un *adversaire intentionnel*, capable d'observer,
  de modifier et de forger des trames.

Un mécanisme de sûreté n'offre aucune protection de sécurité. Le CRC en est
l'illustration : il détecte une corruption aléatoire, mais un attaquant le
recalcule en quelques instructions.

## 1. Hypothèses

| # | Hypothèse |
|---|---|
| H1 | La liaison est point à point, avec un émetteur et un récepteur par sens. |
| H2 | Le canal peut corrompre, perdre, dupliquer, retarder et réordonner des octets. |
| H3 | Les deux extrémités sont correctement programmées ; on ne traite pas la compromission d'un nœud. |
| H4 | Le canal n'est **pas** protégé contre un accès physique ou logique adverse. |
| H5 | Les deux extrémités partagent la même configuration : `FWB_FRAMING_MAX_PAYLOAD`, `link_id`, version du format. |

H4 est l'hypothèse qui rend la section 3 nécessaire. Elle est posée
explicitement pour qu'on ne déduise pas des mécanismes de la section 2 une
protection qu'ils n'offrent pas.

## 2. Sûreté de communication

La liste de menaces est celle des normes de sûreté de communication
(IEC 61508-2 et IEC 61784-3), reprise dans les profils E2E d'AUTOSAR.

| Menace | Description | Mécanisme | Couverture |
|---|---|---|---|
| **Corruption** | Un ou plusieurs bits altérés en transit. | CRC-16/CCITT-FALSE sur l'en-tête et le payload. | Couverte. Toute erreur de 1 ou 2 bits, toute erreur en nombre impair de bits, toute rafale d'au plus 16 bits. |
| **Répétition** | Une trame ancienne est reçue une seconde fois. | Champ `SEQ`, exploité par la brique `antireplay`. | Couverte au niveau système. `framing` transporte `SEQ` sans l'interpréter. |
| **Perte** | Une trame n'arrive jamais. | Trou détecté dans la suite des `SEQ`. | Détectée, pas corrigée. Le traitement appartient à l'application. |
| **Insertion** | Une trame venant d'un autre flux est reçue. | `link_id` injecté dans le CRC (ADR-0007). | Couverte pour les cas accidentels, si les flux ont des `link_id` distincts. |
| **Resequencement** | Les trames arrivent dans le désordre. | `SEQ` et fenêtre glissante d'`antireplay`. | Couverte au niveau système. |
| **Masquerading** | Une trame d'un autre émetteur est prise pour légitime. | `link_id` injecté dans le CRC. | Couverte pour les cas accidentels uniquement. Voir §3. |
| **Retard** | Une trame arrive trop tard pour être utile. | Aucun. | **Non couverte.** Nécessite une horloge, donc relève de l'application, qui doit horodater à la réception. |

### Détails et limites par mécanisme

**CRC.** La probabilité de non-détection d'une corruption aléatoire est de
l'ordre de 2⁻¹⁶ dans le cas général, soit environ 1,5 × 10⁻⁵. Cette valeur est
insuffisante pour un niveau d'intégrité élevé sur une liaison très sollicitée :
un CRC de 32 bits serait alors nécessaire. La brique `crc` le permet sans
modification, et seul le format de trame devrait évoluer (`VER` incrémenté).

**Troncature.** Elle n'apparaît pas comme une menace distincte dans la liste,
mais elle est traitée doublement : par le contrôle de cohérence entre `LEN` et
la longueur réelle, et par la délimitation COBS, pour laquelle un `0x00`
prématuré termine la trame et provoque son rejet.

**Identifiant de lien.** Il ne distingue que des configurations différentes. Si
deux liaisons sont configurées avec le même `link_id`, aucune protection contre
l'insertion n'est obtenue. L'affectation des identifiants est une responsabilité
d'intégration, à documenter au niveau du système.

**Perte.** Le mécanisme ne fait que signaler un trou dans la numérotation. Il
n'y a ni retransmission, ni acquittement : une application qui a besoin de
garanties de livraison doit les bâtir au-dessus.

## 3. Sécurité

Cette section décrit ce que la v1 **ne protège pas**. Elle existe pour empêcher
que le dépôt soit utilisé en croyant à une sécurité qu'il n'offre pas.

| Menace | Faisabilité pour un attaquant sur le canal | Statut v1 |
|---|---|---|
| **Écoute** | Triviale. Le payload est en clair. | Non couverte. Hors périmètre. |
| **Forge d'une trame** | Triviale. Le CRC est public et déterministe. | **Non couverte.** |
| **Modification d'une trame en transit** | Triviale. Il suffit de recalculer le CRC. | **Non couverte.** |
| **Rejeu intentionnel** | Faisable. `antireplay` bloque un rejeu de trame ancienne, mais pas un rejeu immédiatement après un redémarrage du récepteur. | Partielle, et par accident plus que par conception. |
| **Récupération du `link_id`** | Faisable. 16 bits, recouvrables par force brute depuis une seule trame observée. | Sans objet : le `link_id` n'est pas un secret. |
| **Déni de service** | Trivial. Un bruit continu empêche toute réception. | Non couverte. Aucun protocole de ce niveau ne peut la traiter. |

### Le `link_id` n'est pas une clé

Il faut être explicite, car la construction y ressemble : un identifiant partagé
mélangé dans un calcul de contrôle évoque un MAC à clé. Ce n'en est pas un.

- 16 bits, soit 65 536 valeurs possibles, énumérables instantanément ;
- le CRC est **linéaire**, donc même avec un identifiant plus long, il est
  possible de modifier un message et de corriger le contrôle sans connaître cet
  identifiant ;
- il n'y a aucune primitive cryptographique dans la chaîne.

Le `link_id` adresse une erreur de câblage ou de routage, rien d'autre.

### Ce qu'il faudrait pour une vraie sécurité

| Objectif | Mécanisme nécessaire |
|---|---|
| Authenticité et intégrité face à un adversaire | MAC (HMAC-SHA256 tronqué, ou CMAC-AES) sur `VER` jusqu'à la fin du payload, avec une clé par lien. |
| Fraîcheur face à un rejeu | Compteur monotone persistant en mémoire non volatile, ou échange d'un aléa au démarrage de session. |
| Confidentialité | Chiffrement authentifié (AEAD), qui remplacerait alors le MAC. |
| Comparaison du MAC | Comparaison en temps constant, pour éviter une attaque temporelle. |

Le format réserve l'emplacement et le bit de `FLAGS` nécessaires au MAC
(`wire-format.md` §3). Lorsque ce travail sera entrepris, **aucune primitive
cryptographique ne sera écrite pour ce dépôt** : une bibliothèque éprouvée sera
encapsulée derrière une interface de la brique.

## 4. Menaces liées à l'implémentation

Indépendamment du protocole, le code de réception est une surface d'attaque,
puisqu'il traite des données non fiables par définition.

| Menace | Mitigation | Vérification |
|---|---|---|
| Lecture hors limites sur une longueur falsifiée | Contrôles structurels **avant** tout accès dépendant de `LEN` (`wire-format.md` §7). | FRM-REQ-038, FRM-REQ-100, fuzzing sous ASan |
| Débordement du tampon de réception | Contrôle de capacité à chaque octet accumulé, puis abandon jusqu'au prochain délimiteur. | FRM-REQ-083, FRM-REQ-084 |
| Dépassement arithmétique sur le calcul de longueur | Tous les calculs sont faits en `size_t` sur des valeurs déjà bornées par le contrôle 2. | FRM-REQ-032, UBSan |
| Boucle infinie sur une entrée malformée | Toute boucle a une borne démontrable ; interdiction de la récursion. | FRM-REQ-101, `conventions.md` |
| Blocage permanent du récepteur après une erreur | Resynchronisation garantie sur le prochain délimiteur. | FRM-REQ-085 |
| Comportement indéfini | `-Wconversion`, UBSan, pas de recopie de structure. | FRM-REQ-106, ADR-0003 |

## 5. Synthèse

Ce que la v1 apporte :

- une détection robuste des corruptions accidentelles ;
- une resynchronisation déterministe après erreur ;
- une détection des erreurs de câblage ou de routage entre liaisons distinctes ;
- un décodeur dont la robustesse mémoire est vérifiée par fuzzing.

Ce que la v1 n'apporte pas, et ne doit pas être supposé :

- **aucune protection contre un adversaire**, à aucun niveau ;
- aucune garantie de livraison ;
- aucune détection de retard.

Une liaison exposée à un accès adverse ne doit pas utiliser ce format seul.
