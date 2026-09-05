# Largeurs entières des cellules et de leurs coordonnées

5 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

**Les opérations de décision de la grille et de ses appels de seed tiennent dans leurs types.** Cette preuve ferme la clause entière laissée dans [CELLULES_COURANT](CELLULES_COURANT.md). La surcouverture géométrique et la marge du localisateur y sont déjà démontrées ; leur domaine binaire64 reste attaché aux options et à l'environnement de compilation.

## Domaine et base effectivement atteinte

M=65535, positions u16 distinctes, `D2=|b-a|²`, G de grille égal à 8 ou 16, `rho2_den` égal à 12 ou 8. Les formes q3 proviennent du même seed aigu ; les covers ont des identités uniques et au plus n positions, avec n inférieur à 2^30. Les helpers ne sont pas des API recevant des formes, bases ou tailles arbitraires.

La [preuve des secteurs](ARITHMETIQUE_SECTEUR_CORDE_COURANTE.md) établit que la base réussit à A=B=1 : les axes choisis omettent la plus grande composante de la direction, la norme carrée de leur croix vaut `d_k²*D2`, et les deux normes carrées de somme/différence sont au plus `2*D2`. Pour un dénominateur au moins 6, le premier test suffit. Les composantes de u et v sont donc bornées par M. La borne de secours `129*M` du commentaire produit est sûre mais inutile sur ces appels.

## Grille, chemin i64 et compteurs de témoins

Toutes les bornes ci-dessous couvrent aussi les sommes partielles avant simplification. Les produits de Gram sont promus en i128 avant multiplication.

| Expression écrite | Majorant du module |
| --- | --- |
| `w=2*z-a-b`, `n2w`, `n2w-D2` | 2M par axe, 12M², 12M² |
| `uu_i`, `vv_i`, `uv_i` | 3M² |
| Les deux produits du déterminant de Gram | 9M⁴ < 2^68 chacun ; leur différence non négative ne déborde pas |
| `du`, `dv` | 6M² chacun |
| `rhs=G*(n2w-D2)` | 192M² pour G=16 |
| `mag=4*G*(|du|+|dv|)` | 768M² < 2^46 |
| `av=4*(ii-G)*du` | Au plus `mag` |
| `bj=rhs-4*(jj-G)*dv` | Au plus `|rhs|+mag`, sous 2^47 |

Le chemin i64 est donc toujours accessible par la garde écrite. Même pour les entrées synthétiques de l'oracle hors géométrie, `mag < 2^62` et `|rhs| < 2^62` bornent chaque produit en module par 2^62 et leur différence strictement par 2^63 : aucun cast tardif ne masque un overflow. La voie i128 reste un repli ; elle n'est pas nécessaire sur les bases u16 prouvées.

Les boucles gardent `lo` entre 0 et NV, `hi` entre −1 et NV−1 et les indices des différences entre 0 et NC. Les indices `bound[cj+1]` restent inférieurs à NV. Chaque site contribue à une seule famille de différences ; préfixes et suffixes comptent des sous-ensembles disjoints. Ainsi les compteurs partiels, leurs sommes et `cnt` sont au plus la taille du cover, inférieure à 2^31 par la garde locale, et inférieure à 2^30 dans le pipeline. G=16 donne au plus 1024 cellules : `needed_cells` et `dead_cells` tiennent en u32.

La politique `acute_seeds*ratio`, avec ratio 2 ou 8 et `acute_seeds<=n`, tient dans le `size_t` 64 bits du domaine CPU. `near_m` utilise des produits promus i128. Ces politiques ne sont pas des certificats ; leur refus conserve l'ancre. Les compteurs diagnostiques cumulés de consultations ne participent pas aux décisions et ne sont pas une borne de travail global.

## Coordonnées exactes des centres et cordes

Les [bornes q3](ARITHMETIQUE_LANES_COURANTE.md) donnent `G3<=9*M^4`, `|W_i|<=36*M^5`. Donc `|N_i|=|W_i-G3*d_i|<=45*M^5<2^86`. Les produits avec u/v et leurs trois sommes ont un module au plus `135*M^6<2^104`. Le dénominateur `2*G3` est positif et inférieur à 2^69.

Dans `seed_chord_coords`, les termes `3*G3` et `2*l_ax*l_bx` sont bornés par 27M⁴ et 18M⁴. Leur différence, puis son produit par `D2<=3*M²`, tiennent en i128 ; lorsque J est non négatif, `J<=81*M^6<2^103`. Le refus J<0 précède la racine. Avec la racine entière corrigée, `mu_hat=floor(sqrt(floor(J/2)))+1` est strictement extérieur, y compris sur un carré exact, et inférieur à 2^51.

Une composante de n est bornée par 2M², donc `|n·u|` et `|n·v|` sont au plus 6M³. Les quatre coordonnées d'extrémité ont alors un module au plus `135*M^6 + mu_hat*6*M^3 < 2^105`. Les produits sont promus avant calcul, les sommes partielles également bornées. Cela ferme les entrées i128 exactes fournies au localisateur, sans supposer une annulation après débordement.

Après localisation, `range_in_domain` exige des intervalles ordonnés contenus dans `[-4G,4G]`. Les NaN et infinis échouent à ces comparaisons dans le domaine sans hypothèse de finitude imposée au compilateur. Les conversions de `floor` vers int et les bornes inclusives des boucles restent donc dans `[-64,64]` ; aucun accès de tableau n'a lieu avant le contrôle des indices de cellule.

## Certificat reproductible et portée

```bash
PYTHONDONTWRITEBYTECODE=1 python3 morsehgp3D_v7/audits/cell_width_certificate.py
PYTHONDONTWRITEBYTECODE=1 python3 -O morsehgp3D_v7/audits/cell_width_certificate.py
```

Le [certificat entier](cell_width_certificate.py) vérifie les majorants, huit frontières du chemin i64, 17 frontières de racine et trois variantes fautives de largeur/majoration. Ses [reçus normal](receipts_front_20260905/cell_width.json) et [optimisé](receipts_front_20260905/cell_width_optimized.json) épinglent les sources. Il ne compile ni n'exécute le moteur. Les portes CTest des cellules ont leur autorité d'exécution dans la [suite D](receipts_20260905/release/summary.json) ; aucune nouvelle suite complète E n'est déduite de cette preuve.

GCP non utilisé. Aucun fichier produit modifié.
