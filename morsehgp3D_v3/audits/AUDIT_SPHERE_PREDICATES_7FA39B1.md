# Audit prédicat par prédicat de `sphere.hpp`

## Verdict

Le noyau mathématique de `sphere.hpp` est **correct sur le profil strict `quantized_u16_input`**, sous trois préconditions actuellement externes : coordonnées dans $[0,65535]^3$, sites distincts après application explicite de la politique de collisions, et `Sphere` issue avec succès de l'un des constructeurs. Les constructions d'arité trois et quatre, les tests de côté et de bon centrage, le comparateur de niveaux et les deux comparaisons de diamètre réutilisées par la v3 n'ont montré aucun désaccord avec l'oracle multiprécision indépendant.

Ce résultat ne qualifie toutefois **pas** le composant tel quel pour le produit : `sphere2` accepte un support coïncident, le type public `Sphere` admet la sentinelle `den == 0` que les prédicats interprètent silencieusement, et aucune primitive de cette voie ne représente `exact_dyadic_input`.

| profil / usage | décision |
| --- | --- |
| formules internes, entrée u16 déjà certifiée et support contrôlé | **GO mathématique conditionnel** |
| reprise directe de l'API actuelle dans une frontière produit | **NO-GO** avant durcissement des invariants |
| `exact_dyadic_input` | **NO-GO absolu** pour ce backend fixe |
| aptitude au SLO et arbitrage PEL-4 | **non mesurée** |

Phase annoncée : audit de la brique arithmétique candidate ; backend `reference_cpu` avec code candidat hôte/device ; profile `quantized_u16_input` puis analyse séparée de `exact_dyadic_input` ; mode `audit_only`. La porte documentaire est ouverte par la section 8 de `PROPOSITION.md`. Aucune phase d'implémentation ou de promotion n'est ouverte par ce rapport.

## Snapshot

Audit commencé au commit `389a7428c88d9dede7a9c767634774b9ea842ca0`, stabilisé au commit `7fa39b1d8c9d3b566bcd098bb4bdd2dbc107d7af`. Les primitives visées n'ont pas changé entre ces deux commits.

| objet | SHA-256 audité |
| --- | --- |
| `morsehgp3D_v2/include/mhgp/exact.hpp` | `72b93c0c11ad80326265d43f7692e40ed0cfbfaf61d52e3f3d344c721bb74796` |
| `morsehgp3D_v2/include/mhgp/sphere.hpp` | `cdc6e98735833286a460a8d04c738a9059621fa9c6ca373493424e322164584a` |
| `morsehgp3D_v2/include/mhgp/miniball.hpp` | `1a590c8992c26ada7c738ca82191fa3e03e2b432e2314d29e929185392710a20` |
| `morsehgp3D_v3/prototype/anchored_catalogue.hpp` | `28b18507fc702cabedf0194dd0db26da23c385d9ffa70338b4bb28875cbd52cf` |
| `morsehgp3D_v3/prototype/edge_shallow.hpp` | `43992a786bbed0c6ff1877f39b828ae8442cf77cf7bb9a1df5306c0f861f91b1` |
| `morsehgp3D_v3/PROPOSITION.md` | `615935ad798ce5afb3eb3280a54a3bfd8306eed9d7570ff474866c7a3255d912` |

Le fichier `prototype/order_k_bfs.hpp` était modifié en concurrence. Son prédicat relevé `in_sphere_side` est indépendant et n'est donc pas couvert par le vert de `sphere.hpp` ; son audit possède son propre rapport.

## Findings bloquants pour une reprise directe

### SPH-01 — un doublon devient un faux support minimal d'arité deux

[`sphere2`](../../morsehgp3D_v2/include/mhgp/sphere.hpp#L27) ne peut pas échouer et [`well_centered2`](../../morsehgp3D_v2/include/mhgp/sphere.hpp#L87) renvoie toujours vrai. Le raccord v3 [`build_sphere`](../prototype/anchored_catalogue.hpp#L93) reprend ces décisions sans vérifier `a != b`.

Fixture exacte : pour `a = b = (17,23,42)`, le résultat est `num = (0,0,0)`, `den = 2`, `support = 2` et `well_centered2() == true`. Il représente géométriquement la sphère singleton de niveau zéro, mais l'étiquette comme support de deux points affinement indépendant. C'est contraire à `RelevantGP` et au commentaire contractuel de `Sphere`.

Ce cas est exclu seulement si la frontière amont a déjà refusé ou agrégé toute collision de coordonnées. Or la quantification peut créer une collision entre deux binary64 distincts : l'absence de doublons dans la source ne suffit pas. `miniball_of` masque souvent le défaut en trouvant d'abord un singleton, mais les générateurs ancrés appellent directement `build_sphere` ; ce comportement n'est donc pas une protection d'API.

**Exigence avant reprise :** constructeur contrôlé d'arité deux qui rejette les coordonnées coïncidentes, canonisation globale des doublons avant toute géométrie, politique `reject` ou `aggregate` et multiplicité authentifiées dans le reçu.

### SPH-02 — la sentinelle `den == 0` compare égale à tous les niveaux

`Sphere` est un agrégat public et sa valeur par défaut possède `den == 0`. Ni [`sphere_side`](../../morsehgp3D_v2/include/mhgp/sphere.hpp#L77), ni [`sphere_beta`](../../morsehgp3D_v2/include/mhgp/sphere.hpp#L137), ni [`sphere_cmp_beta`](../../morsehgp3D_v2/include/mhgp/sphere.hpp#L142) ne valident l'invariant `den > 0`.

Reproduction minimale : `Sphere bad{}; Sphere good = sphere2({0,0,0},{2,0,0});`. Le résultat observé est `sphere_cmp_beta(bad, good) == 0`, `sphere_side(bad,{1,0,0}) == 0` et `sphere_beta(bad)` vaut NaN. Le comparateur conclut à l'égalité parce que les deux produits croisés sont nuls. Une sentinelle qui atteint le tri peut donc fusionner atomiquement des niveaux sans rapport.

Les chemins audités emploient actuellement un booléen `built` ou `largest_valid` avant de consommer leur valeur par défaut, et l'oracle multiprécision avorte sur un dénominateur nul. Cela réduit la joignabilité actuelle, mais ne rend pas la frontière sûre.

**Exigence avant reprise :** rendre la sphère valide non constructible sans fabrique, ou imposer un contrôle fail-closed à chaque frontière de tri, sérialisation et décision. Une valeur invalide ne doit posséder ni côté, ni niveau, ni ordre total géométrique.

### SPH-03 — `exact_dyadic_input` n'est pas un profil de cette arithmétique

[`P3`](../../morsehgp3D_v2/include/mhgp/exact.hpp#L210) contient trois `i64` et toutes les bornes reposent sur `kCoordBits == 16`. Hors de ce domaine, `p3_sub` peut déborder en signé et `p3_cross` multiplie dans `i64` avant de stocker le résultat. Ce n'est pas un repli exact général.

Un alignement entier commun des binary64 finis peut demander environ 2 099 bits par coordonnée avant même les déterminants. Le faire entrer dans `P3` par quantification change les distances, égalités, rangs et lots ; ce serait le profil u16, pas une implémentation du profil dyadique.

**Décision :** conserver deux backends explicitement disjoints. Pour `exact_dyadic_input`, employer des dyadiques multiprécision ou un DAG de signes avec filtre dirigé et repli multiprécision ; ne jamais élargir silencieusement le domaine annoncé de `sphere.hpp`.

### SPH-04 — les entiers fixes sont exacts par preuve de domaine, pas par garde dynamique

`BigInt<N>` effectue des opérations modulaires sans drapeau de débordement. De plus, le commentaire de [`big_mul_i128`](../../morsehgp3D_v2/include/mhgp/exact.hpp#L162) annonce `N >= M + 2`, alors que [`sphere_cmp_beta`](../../morsehgp3D_v2/include/mhgp/sphere.hpp#L142) appelle volontairement `big_mul_i128<6,6>`. Ces appels sont sûrs sur u16 parce que leur produit tient très largement dans 384 bits ; le contrat générique écrit n'est néanmoins pas celui réellement employé.

**Exigence avant reprise :** spécialiser et documenter les largeurs par prédicat, attacher le profil au type ou à la frontière d'appel, et ajouter en qualification un témoin de dépassement ou une vérification multiprécision. Aucun appel futur ne doit hériter par analogie d'une preuve qui ne porte que sur u16.

## Correction mathématique sur `quantized_u16_input`

Posons $M=65535$, $B_i=x_i-x_0$, $\ell_i=\left\Vert B_i\right\Vert^2$, $X=B_1\times B_2$ et $T=\ell_1B_2-\ell_2B_1$. Les coordonnées d'un point de la grille donnent $\ell_i<3M^2$. Pour le triangle, le code emploie `den = 2 |X|^2` et `num = T × X`. Les identités suivantes valident à la fois la formule et ses largeurs :

$$\left\Vert X\right\Vert^2\leq\ell_1\ell_2,\qquad\left\Vert T\right\Vert^2=\ell_1\ell_2\ell_3,\qquad\left\Vert\mathrm{num}\right\Vert^2=\left\Vert X\right\Vert^2\ell_1\ell_2\ell_3.$$

Il en découle `den < 18 M^4 < 2^68.17` et `num2 < 243 M^10 < 2^167.93`. Le majorant publié `num2 < 2^169.93` est donc conservateur. Le produit croisé de deux niveaux est inférieur au majorant publié $2^{306.28}$ et tient dans la plage positive signée de `BigInt<6>`, qui va jusqu'à $2^{383}$. `BigInt<4>` possède de même plus de 80 bits de marge pour `num2`.

Pour `sphere_side`, l'expression est exactement la différence entre la distance carrée au centre et le rayon carré après multiplication par le dénominateur positif ; le terme constant $\left\Vert\mathrm{num}\right\Vert^2$ s'annule parce que `base` appartient au support. Les bornes ci-dessus donnent une magnitude inférieure à $108M^6<2^{102.76}$ dans le cas triangle, bien sous `i128`. Les bornes du tétraèdre sont plus petites. Les intermédiaires de `well_centered4` restent eux aussi sous 128 bits ; les signes de Cramer sont recalibrés avec l'orientation recomputée, ce qui rend le résultat invariant par permutation.

| primitive | résultat u16 | réserve |
| --- | --- | --- |
| `p3_sub`, `p3_dot`, `p3_cross`, `det3`, `orient3d` | correct et sans débordement dans le domaine | aucune garde hors domaine |
| `mul128`, `big_cmp`, multiplications utilisées par les niveaux | accord multiprécision | exactitude conditionnée à la largeur prouvée |
| `sphere1`, `sphere2` | formule correcte | `sphere2` ne rejette pas `a == b` |
| `sphere3`, `sphere4` | centres exacts, shell exact, dégénérescences affines rejetées | sortie inexploitable si le booléen vaut faux |
| `sphere_side` | convention `-1` intérieur, `0` shell, `+1` extérieur confirmée | sphère valide requise |
| `well_centered3`, `well_centered4` | intérieur relatif strict confirmé, frontières rejetées | support indépendant requis |
| `sphere_num2`, `sphere_cmp_beta` | ordre rationnel exact, y compris représentations non réduites | `den > 0` requis |
| `within_diameter`, `diameter_squared_at_most` | inégalités fermées exactes confirmées | distance carrée non négative u16 requise |
| `sphere_beta` | projection cohérente pour affichage | interdite pour toute décision ou mise en lot |

## Différentiel indépendant

Le probe est resté sous `/tmp` ; il n'a partagé avec le sujet ni entier large, ni solveur rationnel, ni construction de centre. La référence emploie `boost::multiprecision::cpp_int`, vérifie les équations de bissection, recalcule les signes de distance en entier arbitraire, résout les barycentriques par Cramer et compare les niveaux par produits croisés indépendants.

```bash
g++ -std=c++20 -O2 -I morsehgp3D_v2/include -I morsehgp3D_v3 /tmp/mhgp_sphere_predicate_audit.cpp -o /tmp/mhgp_sphere_predicate_audit
/tmp/mhgp_sphere_predicate_audit 200000
```

```text
OK trials=200000 built3=200064 deg3=0 built4=200048 deg4=16 side=3000880 cmp=200000 diameter=400000 permutations=4704 grid3=41288 grid3deg=376 grid4=570800 grid4deg=64576 duplicate_pair_den=2 duplicate_pair_num2=0 duplicate_pair_well=1
```

La partie exhaustive parcourt les 41 664 triplets et 635 376 quadruplets de la grille $4^3$, dont 376 triplets alignés et 64 576 quadruplets coplanaires. Pour chaque support non dégénéré, elle balaie les 64 requêtes de côté. Les cas aléatoires couvrent toute la grille u16, des motifs de coins extrêmes, les extrema signés de `i128`, 200 000 comparaisons de niveaux, 400 000 décisions de diamètre et 4 704 permutations de tétraèdres.

Le même probe sous `-fsanitize=undefined,signed-integer-overflow -fno-sanitize-recover=all`, avec 20 000 tirages et tout l'exhaustif $4^3$, termine avec la même ligne `OK` et aucun diagnostic.

| artefact sous `/tmp` | SHA-256 |
| --- | --- |
| source du probe | `d3dbbde794097638c17a17561b7da0ca013cc3ad1c36c29d82344c3af2403fe4` |
| binaire Release | `c427970907bb33169c6ee9997cbd950dc8dc828706d92c45d62a821e4959d0e4` |
| binaire UBSan | `683ccf026ada83aea20be3c8450033f67e31387716e4c5f1cf8c4949e0fd167a` |

Limites : la campagne ne compile pas le chemin CUDA, ne mesure ni latence ni débit, ne couvre pas les dyadiques binary64 et ne prouve pas les préconditions d'entrée d'un pipeline complet. Le volume différentiel falsifie efficacement une erreur de formule ou de largeur observée ; la preuve de largeur ci-dessus reste l'autorité pour le domaine entier borné.

## Dépendances partagées et non-propagation

- `anchored_catalogue` et `edge_shallow` réutilisent effectivement `build_sphere`, `sphere_side`, le bon centrage, le comparateur de niveaux et les tests de diamètre : le vert mathématique u16 leur est transférable seulement après SPH-01 et SPH-02.
- `order_k_bfs` possède son propre déterminant relevé `in_sphere_side`. Une correction de `sphere_side` ne peut ni valider son signe, ni sa navigation, ni le transport de niveau. Un différentiel où force brute et parcours partagent ce prédicat serait circulaire.
- Le juge v3 transforme les champs d'une `Sphere` en rationnels multiprécision, mais cela vérifie une sortie ; cela ne rend pas la production dyadique et ne protège pas une décision prise avant le juge.
- `miniball_of` partage tous les prédicats de `sphere.hpp`. Son énumération par arité n'est correcte comme support minimal que sous le contrat de points distincts et après rejet exact des supports affinement dépendants.

## Porte de reprise proposée

Une reprise honnête demande simultanément :

1. un profil runtime ou statique authentifié `quantized_u16_input`, avec contrôle de plage, transformation, digest et politique de collisions ;
2. des constructeurs contrôlés pour les arités un à quatre et un type de sphère valide qui rend `den == 0` inobservable par les prédicats ;
3. des fixtures permanentes pour doublon, support affine dégénéré, permutation impaire, bon centrage sur frontière, sentinelle, égalité de niveaux non réduits et distances extrêmes ;
4. un différentiel multiprécision permanent sur CPU et sur le vrai chemin device, sans prédicat partagé avec le sujet ;
5. une table de largeur versionnée par profil, reliée à chaque intermédiaire et vérifiée au changement de formule ;
6. une mesure séparée PEL-4 du coût complet de `sphere_side`, `well_centered4` et `sphere_cmp_beta`, avec compilateur, architecture, taux de repli et ledger de bout en bout ;
7. un backend distinct pour `exact_dyadic_input`, avec ses propres bornes, reçus et SLO.

Après ces gardes, reprendre les formules u16 est justifié. Les reprendre comme preuve que « `sphere.hpp` est sain » sans nommer le profil et les préconditions ne l'est pas.

GCP non utilisé.
