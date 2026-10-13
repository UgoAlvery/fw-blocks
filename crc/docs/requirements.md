# Exigences — `fwb_crc`

Mêmes conventions que [`../../framing/docs/requirements.md`](../../framing/docs/requirements.md).

Méthodes de vérification : `TU` test unitaire, `VEC` vecteur de référence,
`FUZZ` fuzzing, `INSP` inspection.

---

## 1. Exigences générales

| ID | Exigence | Vérif. |
|---|---|---|
| CRC-REQ-001 | La brique ne doit appeler aucune fonction d'allocation dynamique. | INSP |
| CRC-REQ-002 | La brique ne doit dépendre d'aucun matériel, d'aucun système d'exploitation et d'aucune horloge. | INSP |
| CRC-REQ-003 | Toute fonction publique recevant un pointeur nul pour un paramètre obligatoire doit renvoyer `FWB_ERR_INVALID_ARG` sans le déréférencer. | TU |
| CRC-REQ-004 | Le résultat ne doit dépendre ni de l'endianness, ni de l'alignement, ni du jeu d'instructions de la machine hôte. | TU, INSP |

## 2. Modèle paramétrable

| ID | Exigence | Vérif. |
|---|---|---|
| CRC-REQ-010 | La brique doit accepter les six paramètres du modèle de Rocksoft : `width`, `poly`, `init`, `refin`, `refout`, `xorout`. | TU |
| CRC-REQ-011 | Les largeurs 8, 16 et 32 bits doivent être prises en charge. | TU |
| CRC-REQ-012 | La réflexion de l'entrée et celle de la sortie doivent être activables indépendamment l'une de l'autre. | TU |
| CRC-REQ-013 | Le polynôme doit être fourni sous forme normale, bit de poids fort implicite, pour toutes les largeurs. | TU, INSP |
| CRC-REQ-014 | La variante CRC-16/CCITT-FALSE doit être fournie en constante prédéfinie. | TU |
| CRC-REQ-015 | La variante CRC-32C doit être fournie en constante prédéfinie. | TU |

## 3. Conformité aux valeurs de contrôle

| ID | Exigence | Vérif. |
|---|---|---|
| CRC-REQ-020 | CRC-16/CCITT-FALSE appliqué à `"123456789"` doit donner `0x29B1`. | VEC |
| CRC-REQ-021 | CRC-32C appliqué à `"123456789"` doit donner `0xE3069283`. | VEC |
| CRC-REQ-022 | Toute variante ajoutée ultérieurement doit être accompagnée de sa valeur de contrôle publiée, vérifiée par un test. | INSP |

Ces trois exigences sont le filet de sécurité principal de la brique : une
erreur de réflexion, de valeur initiale ou de masque final produit un résultat
plausible mais faux, qu'aucune relecture ne détecte de façon fiable.

## 4. API incrémentale

| ID | Exigence | Vérif. |
|---|---|---|
| CRC-REQ-030 | Le calcul doit pouvoir être réalisé en plusieurs appels successifs sur des fragments distincts. | TU |
| CRC-REQ-031 | Pour tout découpage d'une même séquence d'octets en fragments, le résultat doit être identique à celui obtenu en un seul appel. | TU, FUZZ |
| CRC-REQ-032 | Un fragment de longueur nulle ne doit pas modifier l'état du calcul, et doit être accepté avec un pointeur nul. | TU |
| CRC-REQ-033 | Le calcul sur une séquence vide doit donner la valeur obtenue en appliquant `refout` puis `xorout` à `init`. | TU |
| CRC-REQ-034 | L'appel de finalisation ne doit pas modifier l'état, de sorte que deux finalisations successives donnent le même résultat. | TU |
| CRC-REQ-035 | Le raccourci en un appel doit donner exactement le même résultat que la séquence initialisation, absorption, finalisation. | TU, FUZZ |
| CRC-REQ-036 | Le contexte de calcul ne doit contenir aucun pointeur vers les données absorbées. | INSP |

CRC-REQ-031 est l'exigence qui conditionne l'usage par `framing`, lequel calcule
un CRC sur deux fragments non contigus.

## 5. Implémentations

| ID | Exigence | Vérif. |
|---|---|---|
| CRC-REQ-040 | Les implémentations bit à bit et par table doivent produire des résultats identiques pour toute variante et toute entrée. | TU, FUZZ |
| CRC-REQ-041 | Le choix de l'implémentation doit se faire à la compilation, sans modification du code appelant. | TU, INSP |
| CRC-REQ-042 | L'implémentation par table ne doit pas consommer plus de 512 octets de données constantes pour une variante 16 bits, ni plus de 1024 octets pour une variante 32 bits. | INSP |
| CRC-REQ-043 | Les tables doivent être produites par expression constante à la compilation, sans script externe ni fichier généré versionné. | INSP |
| CRC-REQ-044 | L'implémentation bit à bit ne doit consommer aucune donnée constante. | INSP |

CRC-REQ-040 est vérifiée par fuzzing différentiel : les deux implémentations
sont alimentées avec la même entrée et leurs sorties comparées. C'est le test le
plus efficace de la brique, puisqu'il ne nécessite aucun oracle externe.

## 6. Robustesse

| ID | Exigence | Vérif. |
|---|---|---|
| CRC-REQ-050 | Aucune exécution ne doit provoquer de lecture hors limites, quelle que soit la longueur d'entrée. | FUZZ |
| CRC-REQ-051 | Aucune exécution ne doit déclencher de comportement indéfini détectable par UndefinedBehaviorSanitizer, notamment par décalage ou dépassement sur type signé. | FUZZ |
| CRC-REQ-052 | Les entrées non alignées doivent être traitées correctement. | TU |
| CRC-REQ-053 | Le temps de calcul doit être proportionnel à la longueur de l'entrée et indépendant de son contenu. | INSP |

CRC-REQ-053 n'est pas une exigence de sécurité : un CRC ne protège aucun
secret, donc une fuite temporelle n'a pas d'enjeu ici. Elle sert à garantir le
déterminisme temporel, propriété utile en contexte temps réel.

## 7. Hors périmètre

| Point | Statut |
|---|---|
| Tables à 16 entrées, slicing-by-N | Hors périmètre v1. |
| Délégation à un périphérique matériel | Hors périmètre v1, possible derrière la même API. |
| CRC de plus de 32 bits | Hors périmètre. |
| Usage cryptographique | Impossible par nature. Voir le `README` de la brique. |
