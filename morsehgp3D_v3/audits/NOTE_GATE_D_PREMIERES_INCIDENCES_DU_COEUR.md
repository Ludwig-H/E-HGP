# Preuve statique — premières incidences du cœur

Date : 9 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Cette preuve est conservée parce que `first_incidence_dichotomy.cpp` la cite.
Elle ne décrit aucun état d'implémentation.

Pour une facette cœur `F`, soit `B_F` sa miniboule, `b_F` son niveau et
`E_F=(B_F intersect X) minus F` son census fermé hors facette.

## Dichotomie exacte

- Si `E_F` est non vide, le premier niveau vaut `b_F` et tous les
  co-minimiseurs sont exactement `F union {x}` pour `x` dans `E_F`.
- Si `E_F` est vide et si la source directe ouverte est complète et
  terminale, le premier niveau est le minimum des cofaces directes incidentes
  à `F`, avec conservation de tous les ex æquo.

La première branche découle de l'unicité de la miniboule. Dans la seconde,
un minimiseur contenant un point extérieur strict dans sa miniboule
admettrait une coface de niveau plus petit, contradiction.

Une implémentation streame les suppressions des cofaces directes, groupe par
clé de facette et niveau exact, puis exécute un census fermé complet. Un
plafond atteint rend toute la facette `unresolved`; un préfixe n'est jamais
un minimum. Cette factorisation évite l'étoile globale mais ne prouve pas
une source directe terminale ni sa parcimonie.

GCP non utilisé.
