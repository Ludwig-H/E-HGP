# Diagnostic gaussien : dendrogramme de points après FULL

13 scènes entières connues de 1 200 points, K5 ; 52 nouvelles sélections, 182 résultats hérités inchangés. Aucun nouveau fit HDBSCAN, calcul géométrique ou GCP. Ce n’est pas une évaluation tenue à l’écart du développement.

Le profil principal préannoncé est m20/z1. Tous les seuils et exposants figurent ci-dessous ; aucun meilleur réglage choisi par scène. Les valeurs sont des ARI moyens tous points, bruit inclus. Trois graines par régime sphérique, deux par stress. CSV : détails par scène, ARI bruit en singletons, couverture, groupes, F1 macro apparié, minima et maxima.

## expZ=1, seuil=20

| Régime | Routage exclusif | Vote pondéré | Première couverture | HDBSCAN commun | HDBSCAN standard |
|---|---:|---:|---:|---:|---:|
| spherical, G2, δ8 | 1.0000 | 1.0000 | 1.0000 | 1.0000 | 1.0000 |
| spherical, G8, δ4 | 0.5061 | 0.1729 | 0.4948 | 0.2239 | 0.2243 |
| spherical, G16, δ2 | 0.0200 | 0.0226 | 0.0201 | 0.0197 | 0.0196 |
| anisotropic, G8, δ4 | 0.1977 | 0.1970 | 0.1970 | 0.1856 | 0.0660 |
| unbalanced, G8, δ4 | 0.5406 | 0.3115 | 0.5359 | 0.2686 | 0.2685 |

## expZ=1, seuil=50

| Régime | Routage exclusif | Vote pondéré | Première couverture | HDBSCAN commun | HDBSCAN standard |
|---|---:|---:|---:|---:|---:|
| spherical, G2, δ8 | 1.0000 | 1.0000 | 1.0000 | 1.0000 | 1.0000 |
| spherical, G8, δ4 | 0.5061 | 0.1729 | 0.4948 | 0.2250 | 0.2253 |
| spherical, G16, δ2 | 0.0245 | 0.0182 | 0.0242 | 0.0070 | 0.0071 |
| anisotropic, G8, δ4 | 0.1977 | 0.1970 | 0.1970 | 0.0667 | 0.0660 |
| unbalanced, G8, δ4 | 0.3107 | 0.2386 | 0.3121 | 0.2367 | 0.2366 |

## expZ=2, seuil=20

| Régime | Routage exclusif | Vote pondéré | Première couverture | HDBSCAN commun |
|---|---:|---:|---:|---:|
| spherical, G2, δ8 | 1.0000 | 1.0000 | 1.0000 | 1.0000 |
| spherical, G8, δ4 | 0.5064 | 0.5134 | 0.4948 | 0.2239 |
| spherical, G16, δ2 | 0.0407 | 0.0224 | 0.0357 | 0.0197 |
| anisotropic, G8, δ4 | 0.4558 | 0.1970 | 0.4755 | 0.2523 |
| unbalanced, G8, δ4 | 0.5412 | 0.3110 | 0.5359 | 0.3473 |

## expZ=2, seuil=50

| Régime | Routage exclusif | Vote pondéré | Première couverture | HDBSCAN commun |
|---|---:|---:|---:|---:|
| spherical, G2, δ8 | 1.0000 | 1.0000 | 1.0000 | 1.0000 |
| spherical, G8, δ4 | 0.5064 | 0.1724 | 0.4948 | 0.2239 |
| spherical, G16, δ2 | 0.0245 | 0.0183 | 0.0242 | 0.0070 |
| anisotropic, G8, δ4 | 0.4755 | 0.1970 | 0.4670 | 0.0667 |
| unbalanced, G8, δ4 | 0.3102 | 0.2386 | 0.3121 | 0.2367 |

## Interprétation et limites

Le routage exclusif fixe une branche par point avant condensation et compte des points unitaires. Le vote pondéré condense des masses de facettes puis vote entre groupes sélectionnés. Le seuil a donc un sens différent ; les deux variantes ne sont pas des reproductions identiques. La première couverture reste une autre baseline, conservée sans modification.

Les dates géométriques de l’arbre sont rationnelles. Les scores Sτ repris sont des arrondis binary64, relevés exactement en rationnels dyadiques pour le routage ; cela vaut aussi en z2. EOM reste binary64. Les racines sont exclues pour les deux méthodes comparées par EOM commun ; aucun remplissage du bruit. Changer z modifie le routage et λ de la nouvelle variante, mais seulement λ du HDBSCAN commun.

Le nouveau min_cluster_size garantit la cardinalité des groupes ponctuels sélectionnés. Les pertes de qualité restent des pertes : la correction structurelle n’est pas une preuve de domination statistique. Le catalogue contributif Gabriel utilisé ici ne reproduit pas implicitement celui de HGP-old/HGP-Clusterer3D. K10, SIPU, généralisation statistique, croissance LiDAR et contrats GPU restent hors de ce pilote.
