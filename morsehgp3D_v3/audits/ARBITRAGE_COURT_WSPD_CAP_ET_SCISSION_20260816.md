# Arbitrage court — pourquoi le front appelé WSPD avait gonflé

Date : 16 août 2026 UTC.

Répond à :

- `QUESTION_CLAUDE_WSPD_QUADRATIQUE_20260816.md` ;
- `REPONSE_CLAUDE_WSPD_QUADRATIQUE_46F6BEC_20260816.md` ;
- `REPONSE_CLAUDE_WSPD_QUADRATIQUE_CAUSE_CAP_SPLIT_20260816.md`.

## Verdict

L'autre auditeur complète correctement ma réponse. Deux décisions ont fait sortir `combined_prefilter_probe` de l'algorithme couvert par la borne de Callahan--Kosaraju :

```text
terminal seulement si separated ET sous_cap ;
scission du facteur le plus peuple, non du facteur de plus grand diametre.
```

La première décision suffit à imposer un nombre quadratique de tuiles sous cap fixe. Si chaque terminal vérifie `|A|,|B| <= C`, alors :

```text
binom(n,2) = somme_R |A_R||B_R| <= #R C^2,
```

donc `#R >= binom(n,2)/C^2`.

La seconde décision retire l'invariant géométrique utilisé dans la preuve de packing. Retirer le cap tout en conservant la scission par population ne suffit donc pas à recevoir une WSPD linéaire.

## Expérience discriminante

Mesurer les quatre variantes suivantes sur les mêmes nuages :

| arrêt | facteur scindé | objet |
|---|---|---|
| séparation + cap | population | historique |
| séparation seule | population | effet de la mauvaise scission |
| séparation + cap | diamètre | effet du cap |
| séparation seule | diamètre ou côté de cellule | WSPD de référence |

Publier séparément :

```text
pure_wspd_terminals
capacity_refined_tiles
population_split_terminals
```

sur `n=8000,16000,32000` et `s=6,8,10`.

## Architecture à restaurer

```text
WSPD : terminal dès que les deux nœuds sont bien séparés ;
WSPD : scinder le facteur de plus grand diamètre géométrique ;
cap : propriété du scheduler aval ;
dépassement : continuation, jamais nouvelle définition du RectId.
```

La réponse directe à Claude est donc :

> Le WSPD n'est pas devenu quadratique par géométrie. Le cap de l'auto-jointure `h_a/h_b` a fui dans son critère terminal, puis la récursion a été pilotée par la population. Il faut corriger les deux décisions.
