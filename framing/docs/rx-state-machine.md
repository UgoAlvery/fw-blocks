# Machine à états du récepteur — `fwb_framing`

Ce document spécifie le comportement du récepteur en flux : la couche qui
transforme une suite d'octets sans frontières en trames isolées, puis les
soumet au codec.

## 1. Principe

Le récepteur accumule les octets jusqu'au délimiteur `0x00`, décode le COBS,
puis appelle le codec de trame. Il ne connaît ni le contenu de la trame, ni sa
sémantique.

L'avantage de la délimitation COBS (ADR-0006) est que la machine n'a jamais à
deviner : le délimiteur ne peut pas apparaître dans une trame encodée, donc tout
`0x00` est sans ambiguïté une fin de trame.

## 2. États

| État | Signification |
|---|---|
| `IDLE` | Aucune trame en cours. Le récepteur attend le premier octet utile. |
| `RECEIVING` | Des octets sont accumulés dans le tampon. |
| `DISCARDING` | La trame en cours est irrécupérable. Tout est jeté jusqu'au prochain délimiteur. |

Trois états suffisent. L'état d'erreur est explicite plutôt qu'implicite, parce
que c'est lui qui garantit la resynchronisation (FRM-REQ-085) : sans lui, un
récepteur qui déborde continuerait à accumuler les octets d'une trame déjà
perdue et rejetterait aussi la suivante.

## 3. Transitions

```
                 ┌────────────────────────────────────┐
                 │                                    │
                 │           octet 0x00               │
                 │        (trame vide, ignorée)       │
                 ▼                                    │
         ┌───────────────┐                            │
   ──────▶     IDLE      │────────────────────────────┘
         └───────┬───────┘
                 │ octet ≠ 0x00
                 │ → accumuler
                 ▼
         ┌───────────────┐   octet ≠ 0x00, tampon non plein
         │   RECEIVING   │◀──┐ → accumuler
         └───┬───────┬───┘───┘
             │       │
             │       │ tampon plein et octet ≠ 0x00
             │       │ → FWB_ERR_OVERFLOW
             │       ▼
             │   ┌───────────────┐   octet ≠ 0x00
             │   │  DISCARDING   │◀──┐ → jeter
             │   └───────┬───────┘───┘
             │           │ octet 0x00
             │           │ → jeter, pas d'erreur
             │           ▼
             │       (retour IDLE)
             │
             │ octet 0x00
             │ → décoder COBS, valider, livrer ou rejeter
             ▼
         (retour IDLE)
```

### Table des transitions

| État | Événement | Action | État suivant |
|---|---|---|---|
| `IDLE` | octet `0x00` | Aucune. Trame vide ignorée, **aucune erreur** (FRM-REQ-082). | `IDLE` |
| `IDLE` | octet ≠ `0x00` | Réinitialiser le tampon, accumuler l'octet, armer la temporisation. | `RECEIVING` |
| `RECEIVING` | octet ≠ `0x00`, place disponible | Accumuler, réarmer la temporisation. | `RECEIVING` |
| `RECEIVING` | octet ≠ `0x00`, tampon plein | Renvoyer `FWB_ERR_OVERFLOW`. | `DISCARDING` |
| `RECEIVING` | octet `0x00` | Décoder COBS, puis valider. Voir §4. | `IDLE` |
| `RECEIVING` | temporisation échue | Renvoyer `FWB_ERR_TIMEOUT`, vider le tampon. | `IDLE` |
| `DISCARDING` | octet ≠ `0x00` | Jeter. | `DISCARDING` |
| `DISCARDING` | octet `0x00` | Jeter, **aucune erreur** : elle a déjà été signalée. | `IDLE` |
| `DISCARDING` | temporisation échue | Aucune erreur, elle a déjà été signalée. | `IDLE` |
| tout | réinitialisation explicite | Vider le tampon et désarmer la temporisation. | `IDLE` |

Après un dépassement, le passage par `DISCARDING` plutôt qu'un retour direct à
`IDLE` est essentiel : revenir à `IDLE` ferait interpréter le reste de la trame
en cours comme le début d'une nouvelle trame, qui serait à son tour rejetée.
Une seule erreur en produirait deux.

## 4. Traitement d'un délimiteur en état `RECEIVING`

À la réception du `0x00`, dans cet ordre :

1. Si le tampon est vide, la trame est vide : ignorer, **sans erreur**, retour à
   `IDLE` (cas d'un flux qui commence par plusieurs délimiteurs).
2. Décoder le COBS. En cas d'échec, renvoyer `FWB_ERR_COBS_MALFORMED` et revenir
   à `IDLE`.
3. Soumettre la trame décodée au codec, qui applique les contrôles de
   `wire-format.md` §7 et renvoie son propre code d'erreur.
4. Sur succès, livrer la trame à l'appelant avec `FWB_OK`.
5. Dans tous les cas, revenir à `IDLE` avec un tampon vide.

Le retour à `IDLE` est inconditionnel. C'est ce qui garantit FRM-REQ-085 : quelle
que soit la cause du rejet, la trame suivante est reçue normalement.

## 5. Dimensionnement du tampon

Le tampon accumule des octets **encodés**, avant décodage COBS. Sa taille est
donc `FWB_FRAMING_MAX_WIRE_SIZE` moins le délimiteur, soit **267 octets** en
configuration par défaut.

| Constante | Valeur par défaut | Signification |
|---|---|---|
| `FWB_FRAMING_MAX_PAYLOAD` | 256 | Payload maximal |
| `FWB_FRAMING_MAX_FRAME_SIZE` | 265 | Trame décodée maximale, soit 7 + 256 + 2 |
| `FWB_FRAMING_MAX_WIRE_SIZE` | 268 | Trame sur le fil maximale, délimiteur compris |

Une trame encodée plus longue que le tampon ne peut être qu'invalide, puisque
l'émetteur conforme ne peut pas en produire. Le dépassement est donc un rejet
légitime et non une limitation.

Conséquence à connaître : une trame dont `LEN` dépasserait largement le maximum
est rejetée par le **récepteur** avec `FWB_ERR_OVERFLOW`, et non par le codec
avec `FWB_ERR_PAYLOAD_TOO_LARGE`, car elle n'atteint jamais le codec. Les deux
chemins sont testés séparément : le codec est testé directement avec le vecteur
`FRM-I-006`, le récepteur avec un flux surdimensionné.

## 6. Temporisation

Le récepteur **ne lit aucune horloge** (FRM-REQ-086). L'application signale
l'écoulement du temps par des appels explicites à une fonction de temporisation,
à la cadence de son choix.

- Un compteur d'inactivité est remis à zéro à chaque octet reçu.
- Chaque appel de temporisation l'incrémente.
- Au-delà du seuil configuré, en état `RECEIVING`, la trame est abandonnée avec
  `FWB_ERR_TIMEOUT`.
- Un seuil nul ou absent désactive entièrement la temporisation.
- Une temporisation en état `IDLE` ne produit aucune erreur (FRM-REQ-088).

Ce découplage sert trois objectifs : la brique reste testable de façon
déterministe, sans horloge simulée ; elle fonctionne avant l'initialisation
d'un quelconque service de temps ; et l'appelant reste libre de la résolution
temporelle.

### Rôle réel de la temporisation

Elle ne protège pas contre la perte de trames, puisqu'un délimiteur ultérieur
suffirait à resynchroniser. Elle sert à **libérer le tampon** après une
interruption de transmission, pour qu'un reliquat de trame ancienne ne se
concatène pas avec le début d'une trame future. Sans elle, cette concaténation
serait rejetée par le CRC, mais au prix d'une trame valide perdue.

## 7. Invariants

Ces propriétés doivent être vraies à tout instant et sont vérifiées par fuzzing.

| # | Invariant |
|---|---|
| I1 | Le nombre d'octets accumulés ne dépasse jamais la capacité du tampon. |
| I2 | En état `IDLE`, le tampon est vide. |
| I3 | Un octet `0x00` ramène toujours la machine en `IDLE`, quel que soit l'état de départ. |
| I4 | Une trame n'est livrée à l'appelant qu'avec `FWB_OK`. |
| I5 | Chaque octet d'entrée provoque au plus une livraison et au plus un code d'erreur. |
| I6 | Aucune séquence d'octets ne peut faire sortir la machine des trois états définis. |

L'invariant I3 est la formulation exacte de la propriété de resynchronisation,
et c'est le bénéfice principal du choix de COBS.
