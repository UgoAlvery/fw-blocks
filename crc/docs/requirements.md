# Exigences — `fwb_crc`

Mêmes conventions que [`../../framing/docs/requirements.md`](../../framing/docs/requirements.md).

Méthodes de vérification : `TU` test unitaire, `VEC` vecteur de référence,
`FUZZ` fuzzing, `INSP` inspection.

---

## 0. Besoins

Un besoin dit ce qu'on veut obtenir, jamais comment. Chaque exigence cite le
besoin qu'elle sert dans la colonne « Besoin » des tables ci-dessous. Une
exigence qui ne sert aucun besoin est à supprimer ou à déplacer vers un ADR —
c'est ce qui a fait sortir REQ-042 et REQ-043 du périmètre des exigences
(voir §5).

| ID | Besoin                                                                                                                                           |
|----|--------------------------------------------------------------------------------------------------------------------------------------------------|
| B1 | Calculer un contrôle d'intégrité pour n'importe quelle variante de CRC courante, pas seulement une variante figée.                               |
| B2 | Fournir des variantes connues et garantir que leur résultat est conforme à des valeurs de référence publiées, indépendamment de la machine hôte. |
| B3 | Permettre de calculer un contrôle sur des données qui arrivent en plusieurs morceaux non contigus.                                               |
| B4 | Fonctionner sur une cible sans système d'exploitation, sans horloge et sans tas.                                                                 |
| B5 | Permettre à l'intégrateur de choisir le compromis entre taille de code et vitesse selon la cible.                                                |
| B6 | Rester sûr et correct face à n'importe quelle entrée, y compris non alignée, de longueur arbitraire ou nulle.                                    |

## 1. Exigences générales

| ID          | Exigence                                                                                                                                 | Besoin | Vérif.   |
|-------------|------------------------------------------------------------------------------------------------------------------------------------------|--------|----------|
| CRC-REQ-001 | La brique ne doit appeler aucune fonction d'allocation dynamique.                                                                        | B4     | INSP     |
| CRC-REQ-002 | La brique ne doit dépendre d'aucun matériel, d'aucun système d'exploitation et d'aucune horloge.                                         | B4     | INSP     |
| CRC-REQ-003 | Toute fonction publique recevant un pointeur nul pour un paramètre obligatoire doit renvoyer `FWB_ERR_INVALID_ARG` sans le déréférencer. | B6     | TU       |
| CRC-REQ-004 | Le résultat ne doit dépendre ni de l'endianness, ni de l'alignement, ni du jeu d'instructions de la machine hôte.                        | B2     | TU, INSP |

## 2. Modèle paramétrable

| ID          | Exigence                                                                                                                 | Besoin | Vérif.   |
|-------------|--------------------------------------------------------------------------------------------------------------------------|--------|----------|
| CRC-REQ-010 | La brique doit accepter les six paramètres du modèle de Rocksoft : `width`, `poly`, `init`, `refin`, `refout`, `xorout`. | B1     | TU       |
| CRC-REQ-011 | Les largeurs 8, 16 et 32 bits doivent être prises en charge.                                                             | B1     | TU       |
| CRC-REQ-012 | La réflexion de l'entrée et celle de la sortie doivent être activables indépendamment l'une de l'autre.                  | B1     | TU       |
| CRC-REQ-013 | Le polynôme doit être fourni sous forme normale, bit de poids fort implicite, pour toutes les largeurs.                  | B1     | TU, INSP |
| CRC-REQ-014 | La variante CRC-16/CCITT-FALSE doit être fournie en constante prédéfinie.                                                | B2     | TU       |
| CRC-REQ-015 | La variante CRC-32C doit être fournie en constante prédéfinie.                                                           | B2     | TU       |

## 3. Conformité aux valeurs de contrôle

| ID          | Exigence                                                                                                            | Besoin | Vérif. |
|-------------|---------------------------------------------------------------------------------------------------------------------|--------|--------|
| CRC-REQ-020 | CRC-16/CCITT-FALSE appliqué à `"123456789"` doit donner `0x29B1`.                                                   | B2     | VEC    |
| CRC-REQ-021 | CRC-32C appliqué à `"123456789"` doit donner `0xE3069283`.                                                          | B2     | VEC    |
| CRC-REQ-022 | Toute variante ajoutée ultérieurement doit être accompagnée de sa valeur de contrôle publiée, vérifiée par un test. | B2     | INSP   |

Ces trois exigences sont le filet de sécurité principal de la brique : une
erreur de réflexion, de valeur initiale ou de masque final produit un résultat
plausible mais faux, qu'aucune relecture ne détecte de façon fiable.

## 4. API incrémentale

| ID          | Exigence                                                                                                                        | Besoin | Vérif.   |
|-------------|---------------------------------------------------------------------------------------------------------------------------------|--------|----------|
| CRC-REQ-030 | Le calcul doit pouvoir être réalisé en plusieurs appels successifs sur des fragments distincts.                                 | B3     | TU       |
| CRC-REQ-031 | Pour tout découpage d'une même séquence d'octets en fragments, le résultat doit être identique à celui obtenu en un seul appel. | B3     | TU, FUZZ |
| CRC-REQ-032 | Un fragment de longueur nulle ne doit pas modifier l'état du calcul, et doit être accepté avec un pointeur nul.                 | B3     | TU       |
| CRC-REQ-033 | Le calcul sur une séquence vide doit donner la valeur obtenue en appliquant `refout` puis `xorout` à `init`.                    | B3     | TU       |
| CRC-REQ-034 | L'appel de finalisation ne doit pas modifier l'état, de sorte que deux finalisations successives donnent le même résultat.      | B3     | TU       |
| CRC-REQ-035 | Le raccourci en un appel doit donner exactement le même résultat que la séquence initialisation, absorption, finalisation.      | B3     | TU, FUZZ |
| CRC-REQ-036 | Le contexte de calcul ne doit contenir aucun pointeur vers les données absorbées.                                               | B6     | INSP     |

CRC-REQ-031 est l'exigence qui conditionne l'usage par `framing`, lequel calcule
un CRC sur deux fragments non contigus.

## 5. Implémentations

| ID          | Exigence                                                                                                                  | Besoin | Vérif.   |
|-------------|---------------------------------------------------------------------------------------------------------------------------|--------|----------|
| CRC-REQ-040 | Les implémentations bit à bit et par table doivent produire des résultats identiques pour toute variante et toute entrée. | B5     | TU, FUZZ |
| CRC-REQ-041 | Le choix de l'implémentation doit se faire à la compilation, sans modification du code appelant.                          | B5     | TU, INSP |
| CRC-REQ-044 | L'implémentation bit à bit ne doit consommer aucune donnée constante.                                                     | B5     | INSP     |

CRC-REQ-040 est vérifiée par fuzzing différentiel : les deux implémentations
sont alimentées avec la même entrée et leurs sorties comparées. C'est le test le
plus efficace de la brique, puisqu'il ne nécessite aucun oracle externe.

> Les contraintes sur la taille et la génération des tables (anciennement
> REQ-042 et REQ-043) décrivaient une solution, pas un besoin : elles sont
> déplacées dans [l'ADR-0005](../../docs/adr/0005-crc-parametrable.md), qui les
> justifiait déjà.

## 6. Robustesse

| ID          | Exigence                                                                                                                                                      | Besoin | Vérif. |
|-------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------|--------|--------|
| CRC-REQ-050 | Aucune exécution ne doit provoquer de lecture hors limites, quelle que soit la longueur d'entrée.                                                             | B6     | FUZZ   |
| CRC-REQ-051 | Aucune exécution ne doit déclencher de comportement indéfini détectable par UndefinedBehaviorSanitizer, notamment par décalage ou dépassement sur type signé. | B6     | FUZZ   |
| CRC-REQ-052 | Les entrées non alignées doivent être traitées correctement.                                                                                                  | B6     | TU     |
| CRC-REQ-053 | Le temps de calcul doit être proportionnel à la longueur de l'entrée et indépendant de son contenu.                                                           | B6     | INSP   |

CRC-REQ-053 n'est pas une exigence de sécurité : un CRC ne protège aucun
secret, donc une fuite temporelle n'a pas d'enjeu ici. Elle sert à garantir le
déterminisme temporel, propriété utile en contexte temps réel.

## 7. Hors périmètre

| Point                                 | Statut                                                |
|---------------------------------------|-------------------------------------------------------|
| Tables à 16 entrées, slicing-by-N     | Hors périmètre v1.                                    |
| Délégation à un périphérique matériel | Hors périmètre v1, possible derrière la même API.     |
| CRC de plus de 32 bits                | Hors périmètre.                                       |
| Usage cryptographique                 | Impossible par nature. Voir le `README` de la brique. |