# `fwb_crc`

CRC paramétrable selon le modèle de Rocksoft : une seule implémentation couvre
toutes les variantes courantes, décrites par six paramètres.

**État : spécifié, non implémenté.**

## Ce que la brique garantit

- Toute variante descriptible par le modèle de Rocksoft, en 8, 16 ou 32 bits.
- Une API incrémentale, donc le calcul sur des fragments non contigus.
- Deux implémentations au choix de l'intégrateur : bit à bit sans table, ou par
  table de 256 entrées.
- Des valeurs de contrôle vérifiées contre les valeurs publiées.
- Aucune allocation dynamique, aucune dépendance.

## Ce que la brique ne garantit pas

- **Aucune propriété cryptographique.** Un CRC n'est ni une fonction de hachage
  sûre, ni un code d'authentification. Il est linéaire : un attaquant peut
  modifier un message et corriger le contrôle, y compris sans connaître la
  valeur initiale. Pour de l'authenticité, il faut un MAC.
- Aucune accélération matérielle. Une couche de délégation à un périphérique de
  CRC reste possible derrière la même API.

## Modèle de Rocksoft

Six paramètres décrivent entièrement une variante :

| Paramètre | Rôle |
|---|---|
| `width` | Largeur en bits : 8, 16 ou 32 |
| `poly` | Polynôme générateur, forme normale, bit de poids fort implicite |
| `init` | Valeur initiale du registre |
| `refin` | Réflexion de chaque octet d'entrée |
| `refout` | Réflexion du registre final |
| `xorout` | Masque appliqué en sortie |

## Variantes fournies

| Nom | Largeur | Poly | Init | RefIn | RefOut | XorOut | Contrôle |
|---|---|---|---|---|---|---|---|
| CRC-16/CCITT-FALSE | 16 | `0x1021` | `0xFFFF` | non | non | `0x0000` | `0x29B1` |
| CRC-32C | 32 | `0x1EDC6F41` | `0xFFFFFFFF` | oui | oui | `0xFFFFFFFF` | `0xE3069283` |

La colonne « Contrôle » donne la valeur attendue pour la chaîne ASCII
`"123456789"`, soit les 9 octets `31 32 33 34 35 36 37 38 39`. C'est le test
d'acceptation de référence : il détecte immédiatement une erreur de réflexion,
de valeur initiale ou de masque final.

Les deux valeurs ci-dessus ont été vérifiées contre les valeurs publiées.

## Choix d'implémentation

| Implémentation | Flash (données) | Coût par octet | Usage visé |
|---|---|---|---|
| Bit à bit | 0 | environ 8 itérations | Cible très contrainte, débit faible |
| Table 256 entrées | 512 o (16 bits), 1 Kio (32 bits) | 1 itération | Cas général |

Le choix se fait à la compilation. Les tables sont générées à la compilation
par expression constante, pas par un script externe : un script introduirait une
étape de build à maintenir et un fichier généré à versionner.

Hors périmètre v1 : tables à 16 entrées (demi-octet) et slicing-by-N.

## Pourquoi `init = 0xFFFF` plutôt que `0x0000`

Avec une valeur initiale nulle, le registre reste à zéro tant que les octets
d'entrée sont nuls. Le CRC ne distingue donc pas une séquence de *n* octets nuls
d'une séquence de *m* octets nuls : une trame tronquée ou allongée par des zéros
en tête passe inaperçue.

Avec `init = 0xFFFF`, le registre quitte l'état nul dès le premier octet et
cette insensibilité disparaît. C'est la raison pour laquelle CRC-16/CCITT-FALSE
est préféré à la variante XMODEM, dont le polynôme est pourtant identique.

Détail dans l'[ADR-0005](../docs/adr/0005-crc-parametrable.md).

## API en prose

```
fwb_crc_init(ctx, params)            prépare un calcul
fwb_crc_update(ctx, data, len)       absorbe un fragment, appelable n fois
fwb_crc_finalize(ctx)                applique refout et xorout, renvoie la valeur
fwb_crc_oneshot(params, data, len)   raccourci pour un unique fragment
```

L'API incrémentale n'est pas un confort : elle est **nécessaire** au framing,
qui doit calculer un CRC sur un identifiant de lien non transmis suivi du
contenu de la trame, soit deux fragments non contigus.

## Documentation

| Fichier | Contenu |
|---|---|
| [`docs/requirements.md`](docs/requirements.md) | Exigences numérotées |
| [`../docs/adr/0005-crc-parametrable.md`](../docs/adr/0005-crc-parametrable.md) | Justification du modèle paramétrable |

## Dépendances

`fwb_common`.
