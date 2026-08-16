# Note de Claude — le gateway ternaire marche, et il révèle que j'énumérais le mauvais objet

Date : 16 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=diagnostic_counter_only`,
`public_status=not_claimed`. GCP non utilisé — `gcloud` est absent du conteneur.

Répond à `NOTE_AUDITEUR_ORDRE_EXECUTION_APRES_5CE2634` et à
`AUDIT_SUIVI_PORTEUR_AIGU_GATEWAY_JUNG_207B542`.

## 1. La séparabilité par axe, qui rend les extrema exacts en `O(1)`

Avec `D = |a-b|^2`, `E = |a-x|^2`, `X = |b-x|^2`, `Phi = (a-x).(b-x)` et
`Delta_E = D - E`, `Delta_X = D - X`, le porteur aigu est

`Delta_E >= 0`, `Delta_X >= 0`, `Phi > 0`.

Ces trois quantités sont des **sommes de termes par axe** :

`Phi = somme_k (a_k - x_k)(b_k - x_k)`,
`Delta_E = somme_k [ (a_k - b_k)^2 - (a_k - x_k)^2 ]`.

L'extremum sur un produit de trois AABB est donc la **somme** des extrema sur
trois produits d'intervalles. C'est exact — pas une relaxation — et surtout cela
**préserve la corrélation par `a_k`**, que des bornes indépendantes sur `D` et
`E` détruiraient. C'est précisément le risque que l'audit signalait.

Chaque extremum d'axe coûte `O(1)` :

| quantité | lieu de l'extremum | candidats |
| --- | --- | ---: |
| `Phi_max` | convexe en `x`, affine en `a` et `b` | `8` par axe, `24` |
| `Phi_min` | minimum en `x` à `(a+b)/2` écrêté | `4` par axe, `12` |
| `Delta_E,max` | `b` au coin le plus **loin**, `x = clamp(a)` | `2` |
| `Delta_E,min` | `b = clamp(a)`, `x` au coin le plus **loin** | `2` |

Les comptes `24` et `12` sont exactement ceux de l'audit — deux dérivations
indépendantes qui tombent juste, ce qui vaut mieux qu'une seule.

**Vérifiées contre l'énumération exhaustive de tous les entiers de six mille
triplets d'intervalles : zéro écart.** Ma première version de `Delta_E,min`
prenait `b` à un coin au lieu du clamp ; elle est morte là, et nulle part
ailleurs.

## 2. Ce que le juge au niveau nuage a trouvé — trois fautes

L'oracle sur boîtes ne juge que le **classifieur**. Il ne dit rien de la
**récursion**, où j'avais trois bugs qu'il n'aurait jamais vus :

**La cellule Morton au lieu de la boîte serrée.** Je lisais `nodes[h].lo/hi` —
la cellule alignée sur un préfixe de clé — au lieu de `tlo/thi`. Sur `two_lines`,
qui produit `z = -1` et sort donc du profil u16 que l'encodage suppose, la
cellule est fausse : deux blocs `ALL_STRICT` dont **aucun** des quatre triples
n'était porteur. Corrigé, le nombre de nœuds tombe de `2 960 431` à `50 136`.

**Le double comptage.** Partir de `(racine, racine, racine)` visite chaque paire
de blocs deux fois. Le juge a lu exactement `2 x brute` sur les trois familles
denses.

**Mon premier correctif était faux aussi.** Élaguer la moitié mal ordonnée sur
`premier(A) > premier(B)` fait passer de `+2 x` à `-13 %` : un nœud peut être
l'**ancêtre** de l'autre, et couper là supprime des descendants légitimes. La
règle correcte est l'auto-jointure en pas cadencé — `A == B` se scinde en
**trois**, la diagonale inverse étant omise.

**Et un quatrième, résiduel.** Un filtre `pid` laissé à la feuille, devenu
redondant avec l'invariant, coupait les paires dont l'ordre Morton et l'ordre
`pid` divergent : `-196`, `-157`, `-45` porteurs. On **ordonne** l'appel au lieu
de filtrer.

Après quoi, `n=120` :

| famille | `brute` | `sparse` | écart | `blocs_faux` | `pairid_expanded` |
| --- | ---: | ---: | ---: | ---: | ---: |
| `two_lines` | `0` | `0` | `0` | `0` | **`0`** |
| `terrain` | `80 374` | `80 374` | `0` | `0` | `656` |
| `uniform` | `126 530` | `126 530` | `0` | `0` | `549` |
| `eight_clusters` | `129 771` | `129 771` | `0` | `0` | `200` |

Plus de `99,5 %` des porteurs sont émis **symboliquement**, en blocs, sans
jamais former un triple.

## 3. `two_lines` : zéro paire matérialisée

C'est l'exigence de l'ordre d'exécution, et elle est tenue à `n=120` comme à
`n=800` : `pairid_expanded = 0`, `carriers = 0`. La famille meurt **sans qu'une
seule `PairId` existe en mémoire**, par les clauses `DEAD_OWNER_E` et
`DEAD_PHI` au niveau des blocs.

## 4. Ce que la mesure de croissance réfute — et c'est le point important

| famille | exposant `noeuds` | exposant masse porteurs | exposant `pairid_expanded` |
| --- | ---: | ---: | ---: |
| `two_lines` | `1,994` | — | — (`0`) |
| `terrain` | `2,55` | **`3,01`** | `2,00` |
| `uniform` | `2,72` | **`3,01`** | `2,32` |
| `eight_clusters` | `2,69` | **`3,01`** | `2,37` |

**La masse des porteurs croît en `n^3`.** À `n=800, uniform`, il y a `39 256 112`
porteurs pour `C(800,3) = 85 013 600` triples — soit `46 %` de **tous** les
triples du nuage.

Ce n'est pas un défaut du gateway : c'est que **l'objet que je lui fais énumérer
est cubique**. Un triangle aigu quelconque n'est pas une source q4 ; il faut
encore que son ancre `(a,b)` soit `W_4`-vivante, c'est-à-dire que le fuseau soit
presque vide. J'ai construit un énumérateur de porteurs **sans** le filtre de
vivacité d'ancre, et j'ai obtenu, logiquement, un objet cubique.

Je le dis parce que j'ai failli publier ce gateway comme un succès de sparsité.
Il est exact, il est sûr, il tient `two_lines` sans une paire — et il n'est pas
encore sparse sur un nuage ordinaire, parce qu'il compte autre chose que ce
qu'il faut compter.

## 4bis. Correction — le cubique venait de l'absence de filtre, pas de l'objet

> [!CAUTION]
> **La lecture de la section 4 était trop pessimiste, et l'utilisateur l'a vu
> avant moi.** Une ancre `W_4`-vivante a moins de `h_4 = 8` points dans son
> fuseau ; le rapport lentille/`W_4` valant `10,86`, elle ne peut porter que
> quelques dizaines de porteurs. Le `n^3,01` ne vient donc pas de l'objet, mais
> de ce que le gateway n'a **aucun** filtre d'ancre — ni exact, ni même le
> préfiltre.

Mesuré, en séparant les porteurs des ancres vivantes de ceux des ancres mortes :

| famille | `n=200` | `n=400` | `n=800` | max à `n=800` | exposant de `C4` sur `V4` |
| --- | ---: | ---: | ---: | ---: | ---: |
| `terrain` | `4,99` | `5,39` | `5,86` | `99` | `n^1,23` |
| `uniform` | `16,10` | `18,58` | `21,49` | `136` | `n^1,47` |
| `eight_clusters` | `23,21` | `35,67` | `48,52` | `430` | `n^1,70` |

**Sous-quadratique sur les trois familles.** Avec le seul préfiltre — sans même
le test exact — `C4` est déjà en `n^1,30`. Le `10,86` prédit bien l'ordre de
grandeur : `terrain` est très en dessous, `eight_clusters` au-dessus, comme pour
la lentille.

Ce qui n'est **pas** borné, et qu'un kernel GPU doit traiter explicitement : la
moyenne croît lentement — `23,2` à `48,5` sur `eight_clusters` — et le
**maximum croît à peu près linéairement**, `110`, `222`, `430`. Un buffer
dimensionné sur la moyenne déborderait. Il faut un cap déclaré par seed et un
chemin de débordement, jamais une moyenne.

`two_lines` reste hors de ce raisonnement : ses ancres croisées sont vivantes
avec un `|ab|` énorme. C'est là que le gateway par blocs reste nécessaire — les
deux mécanismes sont complémentaires, pas concurrents.

## 5. Ce qu'il reste à faire, et qui est maintenant clair

Le gateway doit porter **les deux** filtres au même niveau de bloc :

- la clause de porteur, `Phi` / `Delta_E` / `Delta_X` — faite ;
- la clause de **mort d'ancre**, `h_coeur + h_a + h_b >= h_q` — déjà écrite et
  mesurée dans le préfiltre combiné, mais pas encore fusionnée ici.

Un bloc `A x B x C` doit mourir si **l'une ou l'autre** tombe. C'est cette
fusion, et non un certificat géométrique de plus, qui rendra la source
sous-quadratique sur les familles denses.

Et le pipeline entièrement sparse se lit alors ainsi, chaque étage étant une
requête octree à sortie **bornée et mesurée** :

    pour chaque `a`  (un point par thread ou par warp)
      `b` <- candidats locaux, la vivacité de l'ancre bornant `|ab|`
      `x` <- requête sur `L(a,b)` privée de `B(m, |ab|/2)`   5 à 49 points
      `y` <- requête sur le cœur de Jung `B(c0, |ab|/4)`     au plus 8 avant mort

La région pour `x` est une intersection de deux boules privée d'une troisième :
trois tests sphère--boîte par nœud, entiers et `O(1)`, primitives déjà présentes
dans `spindle_core_ball.hpp`. L'octree rend un surensemble par boîtes, le
prédicat exact `Phi`/`Delta` filtre ensuite — c'est exactement la « génération
sparse avant vérification exacte ».

## 6. Les portes, et les deux espèces de mutants

| porte | ce qu'elle exige |
| --- | --- |
| `mhgp3v_gateway_oracle_boites` | `dead_faux=0`, `all_faux=0`, plancher `dead >= 400` |
| `mhgp3v_gateway_oracle_all_strict_atteint` | boîtes **fabriquées** pour atteindre `ALL_STRICT`, plancher `1 000` |
| `mhgp3v_gateway_juge_<fam>` | `ecart=0` et `blocs_faux=0`, quatre familles |
| `mhgp3v_gateway_two_lines_sans_pairid` | `pairid_expanded=0`, `carriers=0` |

Sans les boîtes biaisées, `all_strict` valait **zéro** : la branche n'était
jamais prise et le mutant `all-strict-lache` survivait par pure vacuité. C'est
le vert-par-vacuité que le protocole interdit, et je l'avais introduit sans le
voir.

**Deux espèces de mutants, à ne pas confondre.** `dead-large` et
`all-strict-lache` rendent le classifieur **faux** : ils meurent au juge, code
`4`. `phi-large` ne ment pas — il rend `MIXED` là où `DEAD` était justifié, donc
il reste **sûr** et seulement plus cher. Le déclarer « survivant » serait une
erreur de catégorie ; il se tue par **perte de couverture**, `351` blocs morts
contre `416`, sous un plancher à `400`.

`all-strict-lache` mérite une mention : il est invisible à l'oracle sur boîtes,
parce que sa faute — admettre l'égalité `E = D`, donc une arête maximale ex
aequo — n'apparaît que sous l'**owner canonique**, que des boîtes ne connaissent
pas. Seul le juge au niveau nuage le tue.
