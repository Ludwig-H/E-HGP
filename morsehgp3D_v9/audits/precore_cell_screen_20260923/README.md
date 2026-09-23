# Avant le cœur q3/q4 : plafond d'une cellule clippée et borne corrélée

23 septembre 2026. Audit CPU du **LiDAR brut avec sol** 08/000000, grille
commune 1 mm/u18, K5/s8/W8/static8. Les trois densités 1/4 ⊂ 1/2 ⊂
entière viennent de la même sélection globale d'IDs ; les quatre quarts
spatiaux utilisent les plans physiques `x=0`, `y=0` passant par le capteur.
Source S2 `c265a5dae`, mêmes traces par arête et contrôles moteur/lot que
le [reçu apparié](../edge_matched_core_20260923/README.md). Il s'agit
d'un **criblage géométrique nécessaire**, sans nouveau calcul HGP ni
chronométrage G4, et non d'un certificat ajouté au moteur.

## Ce que la boîte réelle peut au plus sauver

Le [lemme de redondance de B](../CERTIFICAT_B_REDONDANCE_CELLULE_UNIQUE_20260923.md)
montre que, si le disque nominal des centres q3 ou q4 d'une arête
survivante S2 est **entièrement contenu** dans la boîte réelle du
sous-nuage, un jeu de gardes universels sur une unique cellule clippée
aurait déjà été compté par le filtre singleton S2 pour cette voie. Pour
éviter **tout le chargement du cœur** de l'arête par ce schéma, les
disques de **toutes ses voies encore ouvertes** doivent donc déborder
de la boîte. Cette condition reste très loin d'être suffisante : elle
ne trouve aucun garde et ne ferme aucune arête.

Le lecteur teste cette condition avec des entiers. Pour une arête
`a,b`, `d=b−a`, `D=|d|²` et la boîte `[lo,hi]³`, il pose
`qᵢ=min(aᵢ+bᵢ−2loᵢ, 2hiᵢ−aᵢ−bᵢ)`. Le disque q3 est contenu si
`3qᵢ²≥D−dᵢ²` sur les trois axes ; q4 l'est si
`2qᵢ²≥D−dᵢ²`. Une égalité tangentielle est **contenue** ; seul l'échec
strict d'une de ces inégalités signifie que le disque déborde.
`F` est la somme des tailles des cœurs effectivement chargés par le
port, extrémités incluses. La colonne « F possible » est seulement
la masse déjà payée sur les arêtes passant ce crible.

| Densité, scène entière | Sites | Charges | F | Charges possibles | F possible / F |
| --- | ---: | ---: | ---: | ---: | ---: |
| 1/4 | 30 847 | 774 494 | 37 009 904 | 14 717 | 898 975 / 37 009 904 = **2,429 %** |
| 1/2 | 61 694 | 1 684 675 | 128 852 821 | 25 747 | 2 641 182 / 128 852 821 = **2,050 %** |
| Entière | 123 389 | 3 986 433 | 559 661 741 | 46 340 | 7 528 704 / 559 661 741 = **1,345 %** |

Au plein, ces 46 340 charges ne sont que **1,162 %** des arêtes
survivantes S2 ; 46 074 d'entre elles débordent sur l'axe `z`.
La boîte entière encodée est `[0,0,0]` à
`[158607,158284,30595]`. La part possible **baisse** avec la densité
dans cette seule trame et cette seule graine. Cela ne démontre aucune
loi asymptotique.

Une [contre-mesure indépendante de B](../bbox_clipping_s2_20260923/README.md)
sur une **autre trace** retrouve exactement nos deux comptes par voie
au plein : q3, 27 868 arêtes / 2 636 115 formes ; q4, 54 431 /
8 042 106. Elle donne 57 677 arêtes / 8 363 262 formes si **au moins
une** voie déborde. Notre condition, plus stricte car elle demande le
débordement de **toutes** les voies ouvertes pour éviter tout le cœur,
retire 11 337 arêtes / 834 558 formes de cette union. Les deux
plafonds répondent donc à des questions différentes.

La coupe spatiale resserre la boîte, surtout en hauteur. Chaque quart
reconstruit sa propre tour ; les masses des quarts ne sont donc pas une
décomposition de celle du plein.

| Quart physique, densité entière | Sites | F | F possible / F |
| --- | ---: | ---: | ---: |
| `x<0,y<0` | 30 265 | 46 146 150 | 3 492 200 / 46 146 150 = **7,568 %** |
| `x<0,y≥0` | 30 780 | 47 832 906 | 3 217 099 / 47 832 906 = **6,726 %** |
| `x≥0,y<0` | 31 391 | 52 302 311 | 2 116 230 / 52 302 311 = **4,046 %** |
| `x≥0,y≥0` | 30 953 | 26 739 169 | 952 580 / 26 739 169 = **3,563 %** |

Pour le quart `x≥0,y<0`, aux densités 1/4 puis 1/2 puis entière,
le plafond en formes vaut **4,266 % / 4,682 % / 4,046 %**.
La somme des quatre quarts entiers donne 9 778 109 / 173 020 536
formes, soit **5,651 %** de leurs seuls calculs séparés. Même dans
ces boîtes plus serrées, une cellule unique ne peut attaquer que cette
petite fraction par le mécanisme testé. Une restriction locale des
centres plus forte ou plusieurs sous-cellules ne sont pas exclues.

Sur le plein, les masques **après** preuve du cœur laissent 2 179 536
arêtes avec au moins une voie ouverte. Parmi elles, 12 107 arêtes,
pondérées par 1 120 809 / 32 988 797 formes de cœur déjà calculées,
passent le même crible (3,398 % des formes de cette classe). Ce poids
après cœur caractérise les survivants ; il n'est pas un coût de cœur
encore évitable.

## Une borne exacte à essayer sur des sous-cellules

Pour un segment `E` de **vraies arêtes survivantes** S2 et une cellule
convexe `C` de centres couverts, définissons à chaque sommet `v`

`F_E(v) = min_(a,b)∈E (|a−v|²+|b−v|²)/2`.

Ce minorant corrélé domine les deux minorants par boîtes `L₁,L₂`
de la [proposition de B](../CONTRE_AUDIT_B_CROISSANCE_Q34_AVAL_S2_20260923.md),
car il ne sépare pas artificiellement les deux extrémités d'une
arête. Si un garde réel `g`, distinct des extrémités et hors des deux
plages originales, satisfait `|g−v|²<F_E(v)` à **chaque** sommet de
`C`, alors, pour toute arête `(a,b)∈E`, la fonction
`2|g−c|²−|a−c|²−|b−c|²` est affine en `c` et négative dans `C`.
Le garde est strictement intérieur à toute boule admissible de
cette sous-cellule. `K−1` gardes distincts ferment q3 et q4 ;
`K−2` ne ferment que q4. Il faut couvrir **toutes** les sous-cellules
avant de fermer une voie entière, en gardant le repli exact ailleurs.

La réduction est associative et parallélisable : avec `sₑ=a+b` et
`hₑ=|a|²+|b|²`, `2F_E(v)=2|v|²+minₑ(hₑ−2sₑ·v)`.
Elle paie cependant `|E|×nombre de sommets distincts` évaluations,
plus la recherche des gardes. Elle ne doit être tentée qu'en shadow
sur les segments lourds, avec ce travail, les formes cœur **et cover**
réellement évitées, les replis, les clés émises et la tour mesurés.
Une réduction parallèle ne transforme pas à elle seule une masse
quadratique en algorithme sous-quadratique.

**Première passe avant ce shadow.** Les traces par arête actuelles ne
portent pas l'identité du rectangle S2. Dans la validation structurelle
des survivants du port `c265a5dae` (`wspd_q34.cpp`, juste avant la
libération de `rectangles`), le curseur parcourt déjà chaque rectangle
ouvert et ses survivants contigus. Enregistrer pour chaque segment non
vide `(rect_id, begin, end, |A|·|B|, masque_rectangle)` ajoute un passage
`O(R+S)` sans reconstruire `A×B`. Le worker connaît l'ordinal `j` de
chaque survivant : une case préallouée `core_sites_by_j[j]` peut recevoir
la différence du compteur avant/après `surviving_edge`, et de même pour
`cover_sites`, sans verrou ni ordre de worker supposé. Exiger
`Δcore_builds=1` par arête avec `dead_core=true`, une case écrite par
ordinal, puis les sommes exactes des ledgers et l'égalité du jumeau
moteur/lot. Ce premier reçu classe `F=ΣΔcore_sites` (extrémités
incluses) et les formes `Σ(Δcore_sites−2)` selon `end−begin` et
`|A|·|B|` ; il ne calcule encore aucun garde.

Cette mesure est un préalable au choix des segments à traiter. Les
42 020 rectangles ouverts de produit ≥64 à K5 portent 62,46 % des
**paires développables** du [shadow des rectangles](../q34_raw_rectangle_mass_20260923/README.md),
mais leur part des 559,662 M incidences `F` du cœur est inconnue. Si
elles sont surtout dans des segments singleton, le minimum `F_E` n'apporte
aucun partage entre arêtes. Pour huit sous-cellules partageant les 27
sommets d'une grille `3³`, une tentative sur tous les survivants paie
déjà jusqu'à `27S` termes corrélés, soit 107,634 M à K5 ou 210,919 M à
K10 sur le plein brut 08/000000, **avant** gardes, repli exact, transport
et cover. Séparer `E_q3` et `E_q4` d'après le masque de chaque arête ;
une voie n'est fermée que si **toutes** ses sous-cellules sont certifiées.
Ne créditer une forme cœur comme évitée que lorsque toutes les voies
ouvertes de son arête sont fermées **avant** `load`. Garder S3 désactivé
pour ce premier appariement, car il déplace le cœur hors des workers S2.

La [fixture entière](verify_correlated_fixture.py), exécutable aussi
sous `python3 -O`, comporte 12 sites u18 après une translation commune.
Ses quatre arêtes `A×B` ont **zéro** témoin singleton S2 q3/q4 ; une
vraie présentation q4 positive possède l'une de ces arêtes comme unique
plus longue. Dans la sous-cellule `[-1/2,1/2]³` avant translation,
quatre gardes sont certifiés par `F_E` aux huit sommets, avec marge
minimale exacte 56, tandis que `L₁=L₂=250000` au centre et que le
garde le plus proche y a distance carrée 251001. Les deux bornes par
boîtes échouent aussi aux huit sommets. Cette fixture prouve
la **force locale** du test, pas la fermeture de tout le domaine ni
un gain LiDAR.

## Reçu et relecture

[`screen.py`](screen.py) lit les traces binaires du [reçu par
arête](../edge_matched_core_20260923/README.md) sans relancer HGP.
Il contrôle les SHA des entrées, binaires, stdout, parties de trace,
source injectée et manifestes, puis les joint aux `RESULTS.json` et
`POST_CORE_FULL.json` **versionnés** de ce reçu. Les deux petits JSON
de ce dossier contiennent les compteurs par voie, axe et masque :
[`FULL_DENSITIES.json`](FULL_DENSITIES.json) et
[`SPATIAL_SECTORS.json`](SPATIAL_SECTORS.json).
[`SHA256SUMS`](SHA256SUMS) scelle les pièces versionnées.

Relecture LIVE avec les traces historiques encore présentes :

```sh
repo=/workspaces/E-HGP
audit="$repo/morsehgp3D_v9/audits/precore_cell_screen_20260923"
python3 -B "$audit/screen.py" --repo "$repo" --densities full half quarter --sectors full --out /tmp/screen-full-recheck.json
cmp "$audit/FULL_DENSITIES.json" /tmp/screen-full-recheck.json
python3 -B "$audit/screen.py" --repo "$repo" --case full:quarter_x_nonneg_y_neg --case half:quarter_x_nonneg_y_neg --case quarter:quarter_x_nonneg_y_neg --case full:quarter_x_neg_y_neg --case full:quarter_x_neg_y_nonneg --case full:quarter_x_nonneg_y_nonneg --out /tmp/screen-spatial-recheck.json
cmp "$audit/SPATIAL_SECTORS.json" /tmp/screen-spatial-recheck.json
python3 -B "$audit/verify_correlated_fixture.py"
python3 -B -O "$audit/verify_correlated_fixture.py"
```

Le lecteur accepte `--inputs`, `--before-root` et `--after-root` pour
des traces reconstruites à d'autres chemins. Les traces lourdes ne sont
pas versionnées ; si `/tmp` disparaît, le script
[`replay.sh`](../edge_matched_core_20260923/replay.sh) du reçu source
doit les reproduire avant cette relecture. Les JSON compacts gardent
le constat et sa jointure SHA, sans devenir une preuve de complétude
des clés jamais émises (`complete_relative`), de gain GPU/G4 ou de
respect du contrat de tour. La trace par arête ne conserve ni ID de
rectangle ni ordre des segments S2 : elle ne permet **pas** d'inférer
la proportion de segments que certifierait un test partagé.
