# ADR-0005 — CRC paramétrable selon le modèle de Rocksoft

- **Statut** : accepté
- **Date** : 2026-10-13

## Contexte

Il existe des dizaines de variantes de CRC, qui diffèrent par leur largeur, leur
polynôme, leur valeur initiale, la réflexion des bits en entrée et en sortie, et
le masque final. Une implémentation qui fige ces paramètres n'est réutilisable
que pour un protocole.

## Décision

La brique `crc` implémente le **modèle de Rocksoft**, qui décrit toute variante
courante par six paramètres : `width`, `poly`, `init`, `refin`, `refout`,
`xorout`.

Deux variantes prédéfinies sont fournies :

| Nom | Largeur | Poly | Init | RefIn | RefOut | XorOut | Check (`"123456789"`) |
|---|---|---|---|---|---|---|---|
| CRC-16/CCITT-FALSE | 16 | `0x1021` | `0xFFFF` | non | non | `0x0000` | `0x29B1` |
| CRC-32C (Castagnoli) | 32 | `0x1EDC6F41` | `0xFFFFFFFF` | oui | oui | `0xFFFFFFFF` | `0xE3069283` |

L'API est **incrémentale** (`init`, `update`, `finalize`), ce qui permet de
calculer un CRC sur des fragments non contigus — indispensable pour couvrir un
identifiant de lien non transmis suivi d'une trame (voir ADR-0007).

Deux implémentations sont disponibles, choisies à la compilation :

- **bit à bit**, sans table, environ 8 opérations par bit, zéro octet de données ;
- **par table de 256 entrées**, un octet par itération, 512 octets de flash pour
  une variante 16 bits et 1 Kio pour une variante 32 bits.

Les variantes à table réduite (16 entrées) et le slicing-by-N sont hors
périmètre en v1.

## Justification

- **Réutilisabilité.** Le même code sert pour CRC-16/CCITT, CRC-32C, CRC-8/SAE-J1850
  et le reste, en changeant six constantes.
- **Vérifiabilité.** Chaque variante a une valeur de contrôle publiée sur la
  chaîne `"123456789"`. C'est un test d'acceptation sans ambiguïté, qui détecte
  immédiatement une erreur de réflexion ou de masque final.
- **Compromis flash / vitesse explicite.** Le choix de l'implémentation est une
  décision d'intégration, pas une contrainte de la bibliothèque.

### Pourquoi `init = 0xFFFF` et non `0x0000`

Avec une valeur initiale nulle, le registre reste à zéro tant que les données
d'entrée sont nulles : le CRC ne distingue donc pas une séquence de *n* octets
nuls d'une séquence de *m* octets nuls. Une trame tronquée ou allongée par des
zéros en tête passe inaperçue.

Avec `init = 0xFFFF`, le registre quitte l'état nul dès le premier octet et
cette insensibilité disparaît. C'est la raison pour laquelle CRC-16/CCITT-FALSE
est préféré à la variante XMODEM (`init = 0x0000`), à polynôme pourtant
identique.

### Pourquoi le polynôme `0x1021` pour le framing

`0x1021` (CCITT) détecte toute erreur portant sur un ou deux bits, toute erreur
en nombre impair de bits, et toute rafale d'au plus 16 bits consécutifs. Sur des
trames courtes (jusqu'à 265 octets ici), il offre une distance de Hamming
supérieure à celle du polynôme `0x8005` (CRC-16/IBM). Le choix est justifié par
la longueur maximale de trame visée, pas par la popularité du polynôme.

## Conséquences

Positives :

- Une seule brique couvre tous les besoins de CRC du dépôt et au-delà.
- Les vecteurs de contrôle rendent toute erreur d'implémentation visible au
  premier test.

Négatives, assumées :

- L'API paramétrable est légèrement plus verbeuse qu'une fonction figée. Des
  constantes prédéfinies limitent la gêne.
- Le passage des paramètres par structure coûte quelques cycles par appel face à
  une version entièrement figée à la compilation. Négligeable devant le coût du
  calcul lui-même, et récupérable par `inline` si le besoin apparaît.

## Alternatives écartées

- **CRC figé.** Non réutilisable, et oblige à dupliquer le code pour chaque
  variante.
- **Délégation à un périphérique matériel de CRC.** Introduit une dépendance
  matérielle, et les périphériques exposent rarement tous les paramètres du
  modèle. Une couche d'accélération reste possible plus tard derrière la même
  API.
