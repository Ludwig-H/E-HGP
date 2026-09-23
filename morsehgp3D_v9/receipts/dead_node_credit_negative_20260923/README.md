# Reçu négatif : crédit par nœuds d'index du certificat de voie morte

23 septembre 2026. **GCP non utilisé.** Cadre : `exploration_v9_hors_registre`,
`backend=reference_cpu`, `profile=quantized_u18_input_only`,
`public_status=not_claimed`. Étape 2 de la revue de conception multi-agents du
jour (« compter sans énumérer ») : jugée sûre, gain espéré de 4 à 10 % de
q34. Critère d'arrêt enregistré **avant** la mesure : baisse du coût cœur +
preuve inférieure à 15 % à K5 et à K10.

## Ce qui a été construit (hors produit, `node_credit.patch`)

Le prouveur de voie morte lit l'antichaîne des nœuds admis par le cœur ou le
cover au lieu d'une forme par site :
- un nœud crédite toute sa population quand le maximum exact de la puissance
  sur boîte × cellule est négatif ;
- il est retiré quand le minimum certifié est positif ou nul ;
- un singleton garde sa forme affine exacte ;
- un nœud incertain n'est scindé que tant qu'une décision de seuil de la
  cellule reste ouverte (T de la cellule, T4 quand les deux voies sont
  demandées, profondeur au coin plafonnée à T3).

La borne est celle de `Q4LocalGeometry::node_bounds_unchecked`, avec la même
base et l'échelle 2^20. Levier `q34_dead_node_credit` de la sonde, désactivé
par défaut. Une variante temporaire de mesure appliquait le crédit au seul
cœur (variable `MHGP9_NC_CORE_ONLY`).

## Mesures

Entrée : sous-nuage emboîté 16 000 de 08/000000 sans sol (SHA-256
`adcc3041…`, identique au reçu `lidar_scaling_local_20260923`), W8, s = 8,
binaire construit depuis `06f71037` avec le patch. L'hôte était partagé
(charge moyenne 15 à 17, harnais d'un autre auditeur) : les temps de mur sont
bruités, le CPU de chaîne est comparé en ABA.

| cas | condensé | q34 (ms) | CPU (s) | formes cœur | formes cover |
| --- | --- | ---: | ---: | ---: | ---: |
| K5, crédit OFF | `e23b6413` | 11 956 | 75,6 | 255,7 M | 115,8 M |
| K5, crédit ON (cœur + cover) | `e23b6413` | 15 424 | 96,4 | 54,4 M | 115,2 M |
| K10, crédit OFF | `9d8694fc` | 35 908 | 255,5 | 593,2 M | 418,4 M |
| K10, crédit ON (cœur + cover) | `9d8694fc` | 52 495 | 337,1 | 205,2 M | 414,2 M |
| K5 ABA : OFF, cœur seul, OFF, cœur seul | `e23b6413` | 10 914 / 11 911 / 11 925 / 12 055 | 76,1 / 81,3 / 75,8 / 81,4 | 255,7 M → 54,4 M | inchangé |

## Lecture

- **Exactitude confirmée** : les compteurs de cellules, les issues
  (`cells`, `outside_cells`, `deep_cells`, `failed_cells`) et les voies
  prouvées ou ouvertes du cœur et du cover sont égaux bit à bit, à K5 comme à
  K10 ; les condensés de tour sont identiques.
- **Coût en hausse** : CPU +27 % à K5 et +32 % à K10 avec le crédit sur le
  cœur et le cover ; +7 % avec le cœur seul.
  - Sur le cover complet, les décisions exactes de seuil forcent la
    résolution presque jusqu'aux sites (formes 115,8 → 115,2 M), avec en
    plus 130 M bornes de nœud et 112 M scissions à K5.
  - Sur le cœur, les formes baissent de ×4,7, mais 55 M bornes de nœud (quatre
    coins × trois axes, environ dix tests de site chacune) et 34 M scissions
    coûtent davantage.
- Critère d'arrêt atteint : piste **fermée**, code retiré du produit. Le
  certificat par site reste le chemin v9. Une réouverture exigerait une borne
  de nœud nettement moins chère ou une résolution non exacte des seuils
  (mêmes voies prouvées mais autres cellules), à mesurer de même.

## Contenu

`node_credit.patch` (diff du moteur, de la chaîne, de la sonde et de la porte
de disposition contre `06f71037`), `out/` (huit sorties JSON de sonde),
`SHA256SUMS`.
