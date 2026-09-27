# Capture synthétique : toutes les scènes et tous les réglages

34 scènes complètes, 17 scénarios × deux nouvelles graines, K5 ; 612 scores nouveaux. Contre-audit séparé lié par le hash du reçu de capture ; ce formateur ne le réexécute pas.

[Scores bruts](scores.csv), [306 agrégats](aggregates.csv), [136 comparaisons appariées](comparisons.csv), [diagnostic des tailles](size_diagnostics.csv), [provenance et hashes](receipt.json). Aucun label ponctuel ni gros objet natif n’est recopié.

## Profil principal préannoncé : K5 / m20 / z1

Routage moins HDBSCAN commun : ΔARI moyen +0.137610 sur les 34 scènes ; 21 gains, 9 pertes, 4 égalités exactes des scores stockés. Ces comptes descriptifs ne sont ni un test de significativité ni une domination générale. Aucun meilleur seuil ou exposant n’est choisi par scène.

ARI tous points ci-dessous. Couverture, bruit et F1 apparié figurent pour chaque scène dans les CSV ; le signe des deltas est toujours routage moins HDBSCAN commun (un bruit plus élevé n’est pas un gain).

| Scène (n / G / δ / bruit / graine dans l’ID) | Routage | Vote massique | Première couverture | HDB commun | HDB standard |
|---|---:|---:|---:|---:|---:|
| quality_spherical_n400_g8_d4_noise0_s1 | 0.1672 | 0.1672 | 0.1671 | 0.4947 | 0.4995 |
| quality_spherical_n800_g8_d4_noise0_s1 | 0.1719 | 0.1717 | 0.1719 | 0.1642 | 0.1639 |
| quality_spherical_n1600_g8_d4_noise0_s1 | 0.1727 | 0.1732 | 0.1729 | 0.1671 | 0.1664 |
| quality_spherical_n800_g2_d4_noise0_s1 | 0.5061 | 0.5078 | 0.4907 | 0.3017 | 0.3001 |
| quality_spherical_n800_g4_d4_noise0_s1 | 0.3033 | 0.3033 | 0.3045 | 0.4098 | 0.4105 |
| quality_spherical_n800_g16_d4_noise0_s1 | 0.7183 | 0.3464 | 0.7047 | 0.2824 | 0.2875 |
| quality_spherical_n800_g32_d4_noise0_s1 | 0.0305 | 0.0299 | 0.0304 | 0.0293 | 0.0293 |
| quality_spherical_n800_g8_d2_noise0_s1 | 0.0397 | 0.0357 | 0.0283 | 0.0185 | 0.0182 |
| quality_spherical_n800_g8_d8_noise0_s1 | 1.0000 | 1.0000 | 1.0000 | 1.0000 | 1.0000 |
| quality_anisotropic_n800_g8_d4_noise0_s1 | 0.5738 | 0.6243 | 0.5682 | 0.4775 | 0.4786 |
| quality_unbalanced_n800_g8_d4_noise0_s1 | 0.2842 | 0.2840 | 0.2876 | 0.3038 | 0.3035 |
| quality_heteroscedastic_n800_g8_d4_noise0_s1 | 0.7498 | 0.7465 | 0.7352 | 0.5869 | 0.5874 |
| quality_rings_n800_g8_d1p5_noise0_s1 | 0.3130 | 0.3459 | 0.3078 | 0.4508 | 0.4620 |
| quality_rings_n800_g8_d3_noise0_s1 | 1.0000 | 1.0000 | 1.0000 | 1.0000 | 1.0000 |
| quality_rings_n800_g8_d6_noise0_s1 | 1.0000 | 1.0000 | 1.0000 | 1.0000 | 1.0000 |
| quality_spherical_n800_g8_d4_noise0p1_s1 | 0.2095 | 0.2095 | 0.2096 | 0.1915 | 0.1929 |
| quality_rings_n800_g8_d3_noise0p1_s1 | 0.9069 | 0.8949 | 0.9026 | 0.9195 | 0.9195 |
| quality_spherical_n400_g8_d4_noise0_s2 | 0.8492 | 0.1754 | 0.8296 | 0.1644 | 0.1617 |
| quality_spherical_n800_g8_d4_noise0_s2 | 0.1767 | 0.1767 | 0.1769 | 0.1697 | 0.1696 |
| quality_spherical_n1600_g8_d4_noise0_s2 | 0.8326 | 0.1770 | 0.8060 | 0.1629 | 0.1628 |
| quality_spherical_n800_g2_d4_noise0_s2 | 0.5414 | 0.5469 | 0.5141 | 0.3547 | 0.3561 |
| quality_spherical_n800_g4_d4_noise0_s2 | 0.7609 | 0.7681 | 0.7499 | 0.3290 | 0.3284 |
| quality_spherical_n800_g16_d4_noise0_s2 | 0.7451 | 0.0169 | 0.7288 | 0.1114 | 0.1116 |
| quality_spherical_n800_g32_d4_noise0_s2 | 0.6932 | 0.0229 | 0.6641 | 0.0293 | 0.0297 |
| quality_spherical_n800_g8_d2_noise0_s2 | 0.0217 | 0.0190 | 0.0214 | 0.0146 | 0.0149 |
| quality_spherical_n800_g8_d8_noise0_s2 | 0.9971 | 0.9971 | 1.0000 | 0.9986 | 0.9986 |
| quality_anisotropic_n800_g8_d4_noise0_s2 | 0.0748 | 0.0748 | 0.0749 | 0.0765 | 0.0765 |
| quality_unbalanced_n800_g8_d4_noise0_s2 | 0.8062 | 0.2375 | 0.7926 | 0.3214 | 0.3219 |
| quality_heteroscedastic_n800_g8_d4_noise0_s2 | 0.7731 | 0.7620 | 0.7677 | 0.5571 | 0.5615 |
| quality_rings_n800_g8_d1p5_noise0_s2 | 0.4972 | 0.4467 | 0.5145 | 0.0763 | 0.0763 |
| quality_rings_n800_g8_d3_noise0_s2 | 0.9486 | 1.0000 | 0.9488 | 1.0000 | 1.0000 |
| quality_rings_n800_g8_d6_noise0_s2 | 1.0000 | 1.0000 | 1.0000 | 1.0000 | 1.0000 |
| quality_spherical_n800_g8_d4_noise0p1_s2 | 0.2023 | 0.2023 | 0.2023 | 0.1928 | 0.1928 |
| quality_rings_n800_g8_d3_noise0p1_s2 | 0.8832 | 0.8723 | 0.8833 | 0.9155 | 0.9116 |

## Moyennes par scénario : deux graines, même profil principal

| Axe / famille / n / G / δ / bruit | Routage | Vote massique | Première couverture | HDB commun | HDB standard |
|---|---:|---:|---:|---:|---:|
| shape_density / anisotropic / 800 / 8 / 4 / 0 | 0.3243 | 0.3496 | 0.3215 | 0.2770 | 0.2776 |
| shape_density / heteroscedastic / 800 / 8 / 4 / 0 | 0.7614 | 0.7543 | 0.7515 | 0.5720 | 0.5744 |
| nonconvex / rings / 800 / 8 / 1.5 / 0 | 0.4051 | 0.3963 | 0.4112 | 0.2635 | 0.2692 |
| nonconvex / rings / 800 / 8 / 3 / 0 | 0.9743 | 1.0000 | 0.9744 | 1.0000 | 1.0000 |
| noise / rings / 800 / 8 / 3 / 0.1 | 0.8951 | 0.8836 | 0.8930 | 0.9175 | 0.9156 |
| nonconvex / rings / 800 / 8 / 6 / 0 | 1.0000 | 1.0000 | 1.0000 | 1.0000 | 1.0000 |
| size / spherical / 1600 / 8 / 4 / 0 | 0.5026 | 0.1751 | 0.4895 | 0.1650 | 0.1646 |
| size / spherical / 400 / 8 / 4 / 0 | 0.5082 | 0.1713 | 0.4983 | 0.3296 | 0.3306 |
| groups / spherical / 800 / 16 / 4 / 0 | 0.7317 | 0.1816 | 0.7168 | 0.1969 | 0.1996 |
| groups / spherical / 800 / 2 / 4 / 0 | 0.5237 | 0.5273 | 0.5024 | 0.3282 | 0.3281 |
| groups / spherical / 800 / 32 / 4 / 0 | 0.3618 | 0.0264 | 0.3473 | 0.0293 | 0.0295 |
| groups / spherical / 800 / 4 / 4 / 0 | 0.5321 | 0.5357 | 0.5272 | 0.3694 | 0.3694 |
| difficulty / spherical / 800 / 8 / 2 / 0 | 0.0307 | 0.0274 | 0.0248 | 0.0165 | 0.0165 |
| size / spherical / 800 / 8 / 4 / 0 | 0.1743 | 0.1742 | 0.1744 | 0.1669 | 0.1668 |
| noise / spherical / 800 / 8 / 4 / 0.1 | 0.2059 | 0.2059 | 0.2059 | 0.1922 | 0.1929 |
| difficulty / spherical / 800 / 8 / 8 / 0 | 0.9986 | 0.9986 | 1.0000 | 0.9993 | 0.9993 |
| shape_density / unbalanced / 800 / 8 / 4 / 0 | 0.5452 | 0.2607 | 0.5401 | 0.3126 | 0.3127 |

## Classes sous le seuil : conservées, jamais exclues

Les effectifs viennent de `true_counts` du manifeste. Le seuil du vote massique porte sur les facettes, pas sur la cardinalité des groupes après vote. Les autres méthodes ont un seuil ponctuel. La condition de taille est nécessaire à la restitution exacte d’une classe, jamais suffisante.

| Scène | Effectifs vrais (−1 : bruit injecté) | Classes <20 | Classes <50 |
|---|---|---:|---:|
| quality_spherical_n400_g8_d4_noise0_s1 | {"1":50,"2":50,"3":50,"4":50,"5":50,"6":50,"7":50,"8":50} | 0 | 0 |
| quality_spherical_n800_g8_d4_noise0_s1 | {"1":100,"2":100,"3":100,"4":100,"5":100,"6":100,"7":100,"8":100} | 0 | 0 |
| quality_spherical_n1600_g8_d4_noise0_s1 | {"1":200,"2":200,"3":200,"4":200,"5":200,"6":200,"7":200,"8":200} | 0 | 0 |
| quality_spherical_n800_g2_d4_noise0_s1 | {"1":400,"2":400} | 0 | 0 |
| quality_spherical_n800_g4_d4_noise0_s1 | {"1":200,"2":200,"3":200,"4":200} | 0 | 0 |
| quality_spherical_n800_g16_d4_noise0_s1 | {"1":50,"10":50,"11":50,"12":50,"13":50,"14":50,"15":50,"16":50,"2":50,"3":50,"4":50,"5":50,"6":50,"7":50,"8":50,"9":50} | 0 | 0 |
| quality_spherical_n800_g32_d4_noise0_s1 | {"1":25,"10":25,"11":25,"12":25,"13":25,"14":25,"15":25,"16":25,"17":25,"18":25,"19":25,"2":25,"20":25,"21":25,"22":25,"23":25,"24":25,"25":25,"26":25,"27":25,"28":25,"29":25,"3":25,"30":25,"31":25,"32":25,"4":25,"5":25,"6":25,"7":25,"8":25,"9":25} | 0 | 32 |
| quality_spherical_n800_g8_d2_noise0_s1 | {"1":100,"2":100,"3":100,"4":100,"5":100,"6":100,"7":100,"8":100} | 0 | 0 |
| quality_spherical_n800_g8_d8_noise0_s1 | {"1":100,"2":100,"3":100,"4":100,"5":100,"6":100,"7":100,"8":100} | 0 | 0 |
| quality_anisotropic_n800_g8_d4_noise0_s1 | {"1":100,"2":100,"3":100,"4":100,"5":100,"6":100,"7":100,"8":100} | 0 | 0 |
| quality_unbalanced_n800_g8_d4_noise0_s1 | {"1":160,"2":40,"3":160,"4":40,"5":160,"6":40,"7":160,"8":40} | 0 | 4 |
| quality_heteroscedastic_n800_g8_d4_noise0_s1 | {"1":100,"2":100,"3":100,"4":100,"5":100,"6":100,"7":100,"8":100} | 0 | 0 |
| quality_rings_n800_g8_d1p5_noise0_s1 | {"1":100,"2":100,"3":100,"4":100,"5":100,"6":100,"7":100,"8":100} | 0 | 0 |
| quality_rings_n800_g8_d3_noise0_s1 | {"1":100,"2":100,"3":100,"4":100,"5":100,"6":100,"7":100,"8":100} | 0 | 0 |
| quality_rings_n800_g8_d6_noise0_s1 | {"1":100,"2":100,"3":100,"4":100,"5":100,"6":100,"7":100,"8":100} | 0 | 0 |
| quality_spherical_n800_g8_d4_noise0p1_s1 | {"-1":80,"1":90,"2":90,"3":90,"4":90,"5":90,"6":90,"7":90,"8":90} | 0 | 0 |
| quality_rings_n800_g8_d3_noise0p1_s1 | {"-1":80,"1":90,"2":90,"3":90,"4":90,"5":90,"6":90,"7":90,"8":90} | 0 | 0 |
| quality_spherical_n400_g8_d4_noise0_s2 | {"1":50,"2":50,"3":50,"4":50,"5":50,"6":50,"7":50,"8":50} | 0 | 0 |
| quality_spherical_n800_g8_d4_noise0_s2 | {"1":100,"2":100,"3":100,"4":100,"5":100,"6":100,"7":100,"8":100} | 0 | 0 |
| quality_spherical_n1600_g8_d4_noise0_s2 | {"1":200,"2":200,"3":200,"4":200,"5":200,"6":200,"7":200,"8":200} | 0 | 0 |
| quality_spherical_n800_g2_d4_noise0_s2 | {"1":400,"2":400} | 0 | 0 |
| quality_spherical_n800_g4_d4_noise0_s2 | {"1":200,"2":200,"3":200,"4":200} | 0 | 0 |
| quality_spherical_n800_g16_d4_noise0_s2 | {"1":50,"10":50,"11":50,"12":50,"13":50,"14":50,"15":50,"16":50,"2":50,"3":50,"4":50,"5":50,"6":50,"7":50,"8":50,"9":50} | 0 | 0 |
| quality_spherical_n800_g32_d4_noise0_s2 | {"1":25,"10":25,"11":25,"12":25,"13":25,"14":25,"15":25,"16":25,"17":25,"18":25,"19":25,"2":25,"20":25,"21":25,"22":25,"23":25,"24":25,"25":25,"26":25,"27":25,"28":25,"29":25,"3":25,"30":25,"31":25,"32":25,"4":25,"5":25,"6":25,"7":25,"8":25,"9":25} | 0 | 32 |
| quality_spherical_n800_g8_d2_noise0_s2 | {"1":100,"2":100,"3":100,"4":100,"5":100,"6":100,"7":100,"8":100} | 0 | 0 |
| quality_spherical_n800_g8_d8_noise0_s2 | {"1":100,"2":100,"3":100,"4":100,"5":100,"6":100,"7":100,"8":100} | 0 | 0 |
| quality_anisotropic_n800_g8_d4_noise0_s2 | {"1":100,"2":100,"3":100,"4":100,"5":100,"6":100,"7":100,"8":100} | 0 | 0 |
| quality_unbalanced_n800_g8_d4_noise0_s2 | {"1":160,"2":40,"3":160,"4":40,"5":160,"6":40,"7":160,"8":40} | 0 | 4 |
| quality_heteroscedastic_n800_g8_d4_noise0_s2 | {"1":100,"2":100,"3":100,"4":100,"5":100,"6":100,"7":100,"8":100} | 0 | 0 |
| quality_rings_n800_g8_d1p5_noise0_s2 | {"1":100,"2":100,"3":100,"4":100,"5":100,"6":100,"7":100,"8":100} | 0 | 0 |
| quality_rings_n800_g8_d3_noise0_s2 | {"1":100,"2":100,"3":100,"4":100,"5":100,"6":100,"7":100,"8":100} | 0 | 0 |
| quality_rings_n800_g8_d6_noise0_s2 | {"1":100,"2":100,"3":100,"4":100,"5":100,"6":100,"7":100,"8":100} | 0 | 0 |
| quality_spherical_n800_g8_d4_noise0p1_s2 | {"-1":80,"1":90,"2":90,"3":90,"4":90,"5":90,"6":90,"7":90,"8":90} | 0 | 0 |
| quality_rings_n800_g8_d3_noise0p1_s2 | {"-1":80,"1":90,"2":90,"3":90,"4":90,"5":90,"6":90,"7":90,"8":90} | 0 | 0 |

## Taille 400 / 800 / 1600 : diagnostic distinct des scores

Même famille sphérique/G8/δ4, grille commune et flux par composante. Deux graines, pas deux répétitions chronométriques. Les deux exposants et tous les temps de phase disponibles sont conservés dans `size_diagnostics.csv`, avec rapports au doublement pour chaque graine. Les compteurs et temps natifs/communs sont répétés sur les lignes z1/z2 : ne pas les additionner.

| Graine / z / n | Cofaces | Facettes | Incidences | Nœuds source routage | Nœuds virtuels cumulés | Internes ponctuels | Routage ms |
|---|---:|---:|---:|---:|---:|---:|---:|
| 2026092801 / 1 / 400 | 6344 | 24920 | 124600 | 32872 | 171027 | 343 | 2334.097 |
| 2026092801 / 1 / 800 | 15166 | 58789 | 293945 | 77643 | 404213 | 696 | 5819.386 |
| 2026092801 / 1 / 1600 | 35594 | 136422 | 682110 | 180379 | 939341 | 1410 | 15283.029 |
| 2026092801 / 2 / 400 | 6344 | 24920 | 124600 | 32872 | 171027 | 344 | 2143.719 |
| 2026092801 / 2 / 800 | 15166 | 58789 | 293945 | 77643 | 404213 | 696 | 5964.786 |
| 2026092801 / 2 / 1600 | 35594 | 136422 | 682110 | 180379 | 939341 | 1408 | 15063.192 |
| 2026092802 / 1 / 400 | 6074 | 23857 | 119285 | 31433 | 163584 | 348 | 2194.843 |
| 2026092802 / 1 / 800 | 14981 | 57913 | 289565 | 76366 | 397577 | 697 | 5404.887 |
| 2026092802 / 1 / 1600 | 34600 | 132954 | 664770 | 175771 | 915272 | 1421 | 14054.599 |
| 2026092802 / 2 / 400 | 6074 | 23857 | 119285 | 31433 | 163584 | 348 | 2163.374 |
| 2026092802 / 2 / 800 | 14981 | 57913 | 289565 | 76366 | 397577 | 699 | 6140.527 |
| 2026092802 / 2 / 1600 | 34600 | 132954 | 664770 | 175771 | 915272 | 1423 | 14215.593 |

Ce sont des compteurs de sorties et du prototype de projection, pas un inventaire complet du travail géométrique. Les nœuds virtuels sont cumulés par point, pas le nombre de nœuds d’un seul arbre matérialisé. Des rapports inférieurs à4 ne prouveraient aucune borne sous-quadratique. Aucun 8k/16k/32k n’a été exécuté ici.

## Sens des scores, coût et limites

Le F1 macro apparié utilise un appariement hongrois un-à-un maximisant le nombre de points correctement appariés, pas le F1 lui-même. Les classes de bruit vrai sont exclues de cet appariement mais restent une contamination des groupes prédits. ARI tous points conserve −1 comme label ; `ari_inliers_noise_singletons` exclut le bruit vrai et traite chaque abstention prédite comme singleton. Les cellules vides des CSV signifient métrique non applicable, jamais zéro.

Les labels vrais décrivent le mécanisme générateur, pas nécessairement des modes de densité identifiables. Deux graines nouvelles confirment ces scénarios préannoncés, pas toutes les distributions. m50 et z2 sont conservés intégralement dans les CSV, y compris les cas défavorables ; HDBSCAN standard n’a que z1. z modifie les poids et le routage HGP, ainsi que λ ; pour HDBSCAN commun il ne modifie que λ. Le catalogue Gabriel courant n’est pas une reproduction implicite du catalogue historique d’ordre-Voronoï.

Durée de capture : 582.900 s ; 2 workers de scènes. Les chronos concernent une chaîne CPU/Python/export de diagnostic partagée, pas une performance de production ni le temps du seul moteur FULL. `selection_ms` n’inclut ni toute la géométrie ni la préparation ; zéro pour HDBSCAN standard désigne une sélection déjà incluse dans son fit, pas un algorithme gratuit. Les sommes de durées concurrentes ne sont pas une durée murale.

Neuf entrées de croissance 8k/16k/32k sont préparées mais NON exécutées par cette capture. Aucune borne sous-quadratique, qualification GPU/GCP ou latence de production ne découle de ce rapport. Le formateur ne lance aucun fit, calcul géométrique, routage ou EOM.
