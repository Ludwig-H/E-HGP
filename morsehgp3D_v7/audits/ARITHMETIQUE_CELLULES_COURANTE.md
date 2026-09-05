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

Le [certificat entier](cell_width_certificate.py) vérifie les majorants, huit frontières du chemin i64, 17 frontières de racine et trois variantes fautives de largeur/majoration. Ses [reçus normal](receipts_front_20260905/cell_width.json) et [optimisé](receipts_front_20260905/cell_width_optimized.json) épinglent les sources. Il ne compile ni n'exécute le moteur. Les portes CTest des cellules ont leur autorité d'exécution dans la [suite D](receipts_20260905/release/summary.json).

## Raccord aux helpers compilés : fermé

Le [pont C++](cell_compiled_bridge.cpp) appelle les vrais `CellGridT<8/16>`, `count_site`, `seed_center_coords` et `seed_chord_coords`. Le [juge Python](cell_compiled_oracle.py) compare les distances carrées aux quatre sommets de chaque cellule ; il ne reprend pas la formule produit `du/dv/rhs` pour les nuages géométriques. Le centre est obtenu par élimination de Gram en `Fraction`, indépendamment des formes q3. La longueur de corde est déduite du rayon rationnel et de la distance au plan, puis arrondie strictement vers l'extérieur en entier Python.

```bash
PYTHONDONTWRITEBYTECODE=1 python3 morsehgp3D_v7/audits/cell_compiled_oracle.py
```

Les [reçus](receipts_front_compiled_20260905/cell/summary.json) conservent les entrées, sorties, commandes, hashes des sources et des binaires. Deux builds C++20 passent : O2 et O1 UBSan, avec `-Wall -Wextra -Wpedantic -Werror`, sans diagnostic. Les options numériques sont celles par défaut de GCC 13.3, comme pour la porte d'arrondi D ; la localisation n'est appelée qu'au plus proche, puis la garde est contrôlée sous les trois autres modes.

| Contrôle par build | Résultat |
| --- | --- |
| Nuages u16, G=8/16, dénominateurs 8/12 et seuils 1/3 | 32 grilles ; 10 940 cellules nécessaires mortes et 1 476 vivantes |
| Compteurs géométriques et frontières synthétiques i64/i128 | 60 cas, dont 28 synthétiques explicitement hors profil ; 38 400 cellules comparées |
| Égalités strictes aux sommets | 20 180 contacts de coquille rencontrés dans le juge |
| Centres q3 et cordes | 32 de chaque, coordonnées exactes atteignant 98 bits ; toutes les boîtes contiennent les cellules fermées attendues |
| Gardes effectives | 96 refus d'environnement ; 192 refus de domaine, dont NaN, infinis et intervalles inversés |

Les trois vrais mutants produit sont détectés sur chacun des deux binaires : `cell-kill-nonstrict` par les comptes, `cell-kill-h-minus-one` par les seuils, `cell-locate-eps-zero` par le centre exactement sur un coin. Le [rejeu renforcé](replay_compiled_front.py), normal et sous `-O`, exige aussi huit vrais changements de décisions de mort pour le mutant de seuil, dont 61 vers 113 cellules mortes au cas 17 ; la détection ne repose donc pas seulement sur le seuil stocké. Il contrôle les 32 bases de chaque binaire par orthogonalité, Gram positif et distance rationnelle aux droites d'arêtes, en complément du [juge sectoriel](ARITHMETIQUE_SECTEUR_CORDE_COURANTE.md). Pour le mutant de localisation, contenir la cellule de droite ne suffit pas : la cellule de gauche est aussi requise au contact entier. Le pont rend 0 après transport ; le juge rend 0 seulement après avoir constaté chaque divergence causale. Ce ne sont pas des codes 4 de CTest.

Les entrées synthétiques sollicitent directement `count_site<i64/i128>` dans leur domaine de représentation, sans prétendre rendre la voie i128 atteignable sous u16. Les points du cover sont uniques ; prendre le petit nuage entier comme cover conserve le contrat local de comptage. Les quatre fichiers du delta E sont inchangés pendant ces exécutions ; les helpers du front sont communs à D et E. Cette fermeture locale ne se substitue pas à une suite intégrée E.

GCP non utilisé. Aucun fichier produit modifié.
