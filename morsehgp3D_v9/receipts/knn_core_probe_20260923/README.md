# Mesure : pouvoir de preuve des voisins proches pour le certificat de voie morte

23 septembre 2026. **GCP non utilisé.** Cadre : `exploration_v9_hors_registre`,
`backend=reference_cpu`, `profile=quantized_u18_input_only`,
`public_status=not_claimed`. Mesure de conception seulement : aucun code produit
modifié. Patch expérimental : `knn_core_experiment.patch`, contre `f685461a`.

## Question

Pour chaque arête qui atteint le cœur diamétral, le patch lance un second
prouveur (mêmes cellules, mêmes seuils) sur un sous-ensemble : les 17 sites du
cœur les plus proches de a et les 17 plus proches de b, soit au plus 34 sites.
Tout sous-ensemble est sûr, puisqu'il ne peut que retirer des crédits. La
mesure compte les arêtes fermées par ce sous-ensemble et par le cœur entier.

## Résultats (sous-nuage emboîté 16 000 de 08/000000 sans sol, W8, s = 8)

| K | cœurs | fermées par le cœur | fermées par les voisins | sites (voisins / cœur) | tests uniformes (voisins) |
| ---: | ---: | ---: | ---: | --- | ---: |
| 5 | 969 742 | 584 580 | 561 015 (96,0 %) | 19,9 M / 257,6 M | 74,0 M |
| 10 | 2 148 586 | 1 323 622 | 1 137 716 (86,0 %) | 54,0 M / 597,5 M | 301,9 M |

Toute arête fermée par les voisins l'est aussi par le cœur, ce qu'on attend
d'un sous-ensemble.

## Lecture

- Les 34 voisins des extrémités portent presque toute la force de preuve du
  cœur à K5, et l'essentiel à K10, pour 10 à 13 fois moins de sites.
- Un levier produit devrait calculer les k plus proches voisins de chaque site
  une fois par trame, sans construire le cœur. Il prouverait d'abord avec eux,
  puis retomberait sur le cœur, et le cover ensuite.
- Gain estimé, non mesuré : de 4 à 8 % du CPU q3/q4 à K5. Le cœur ne vaut
  qu'environ 12 % de q3/q4, et les arêtes restées ouvertes repayent la
  tentative.
- Priorité inférieure aux leviers d'ordonnancement déjà portés ; à mesurer en
  ablation appariée si porté.
