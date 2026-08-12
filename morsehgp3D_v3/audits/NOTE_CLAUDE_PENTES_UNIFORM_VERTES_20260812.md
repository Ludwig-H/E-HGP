# Note de Claude — la pente rouge est une propriété du générateur, pas de la route

Date : 12 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## 1. Le fait

Sur `uniform`, le régime volumique que la section 14.5 du plan de tests rend
**bloquant**, tous les compteurs sont verts sur **trois doublements
consécutifs** :

| `n` | cellules | lifts | quadruplets | supports |
| ---: | ---: | ---: | ---: | ---: |
| `1 000` | `117 177` | `11 912 492` | `6 577 531` | `310 692` |
| `2 000` | `251 545` | `26 383 500` | `14 740 057` | `669 978` |
| `4 000` | `510 785` | `57 775 527` | `32 321 956` | `1 452 688` |
| `8 000` | `1 167 281` | `120 919 105` | `65 753 676` | `3 093 102` |

| pente | cellules | lifts | quadruplets | census | supports |
| --- | ---: | ---: | ---: | ---: | ---: |
| `1 000 -> 2 000` | `1,102` | `1,147` | `1,164` | `1,145` | `1,109` |
| `2 000 -> 4 000` | `1,022` | `1,131` | `1,133` | `1,176` | `1,117` |
| `4 000 -> 8 000` | `1,192` | `1,066` | `1,025` | `1,059` | `1,090` |

Aucune n'atteint `1,35`. Le critère « deux pentes successives » de la gate de
travail est donc satisfait trois fois, sur les cellules comme sur le reste.

Ces quatre points proviennent du même binaire local, non gelé, et n'ont pas de
transcript pincé : ils appellent une reprise en campagne gelée aux tailles
contractuelles. Ils ne remplacent pas la rampe `12 500/25 000/50 000`.

## 2. Pourquoi `terrain` est rouge et pourquoi ce n'est pas la même question

`terrain` donne des pentes de cellules `1,617` puis `1,888`. La cause est
mesurable et tient au générateur, pas à la route.

L'emprise vaut `coord=sqrt(25 n)`, donc le pas horizontal
`coord/sqrt(n)` reste constant à cinq unités. Mais l'amplitude des bosses et la
levée de canopée valent toutes deux `coord/8` : **l'extension verticale croît
comme `coord`, donc comme `sqrt(n)`**. Le volume de la boîte croît alors comme
`coord^3`, soit `n^{1,5}`, tandis que les sites croissent comme `n`. Le nuage
devient de plus en plus creux en trois dimensions.

L'octree subdivise l'espace des **centres**, qui est ce volume. Il paie donc
`n^{1,5}` là où la sortie reste linéaire — supports `1,047`, census `1,106`.
L'histogramme en profondeur le confirme : à `n=1 500`, la profondeur six rend
`42 659` terminaux pour `78 584` supports, tandis que la profondeur sept en rend
`61 978` pour seulement `15 784`.

Un nuage LiDAR réel ne se comporte pas ainsi : un balayage plus grand couvre
plus de sol à variation verticale comparable. La croissance verticale de
`terrain` est un choix de modèle. Cela n'excuse pas la route : elle paie le
volume vide, et ce coût est réel sur tout nuage à grand volume vide. Cela situe
seulement le problème.

## 3. Deux prunes exacts ajoutés

**Séparation par la normale locale.** Le k-DOP à directions fixes ne voit pas
une cellule située hors du plan local : ses dix plus proches sites sont presque
cosphériques vus de loin, tous leurs intervalles se recouvrent, et le critère de
travail la découpe indéfiniment alors qu'aucun support positif ne peut y avoir
son centre. La direction est choisie dans les données — normale du plan des
moindres carrés, obtenue par l'adjugée de la covariance — mais **le test reste
entier et exact**, donc le prune l'est. Sur `terrain, n=1 500` : cellules
`201 889` vers `142 917`, soit `-29 %`, terminaux `-36 %`, temps `3,848 s` vers
`2,747 s`, soit `1,40x`, et sortie identique au support près.

**Prune de rang au niveau cellule.** La positivité impose `c` dans
`relint conv(U)` : pour toute direction, le support contient un point
strictement de chaque côté. Donc `beta` majore les deux distances minimales, et
tout site vérifiant `u_C(x) < beta` est strictement intérieur. Si plus de
`smax-2` sites le vérifient, la cellule ne peut posséder aucun support positif
d'arité au moins deux. Le prune est exact et se propage à la descendance.
Mesuré : il ne coupe que `94` cellules sur `142 917` à `n=1 500`. Il est
conservé parce qu'il est exact et qu'il vise précisément le régime à grand
volume vide, mais il n'est pas un levier sur les familles testées.

Une première version prenait le **minimum** des deux distances au lieu du
maximum et ne coupait rien ; c'est corrigé.

## 4. Coût et distance au contrat

`uniform, n=4 000` : `24,0 s` `user` sur un cœur de cette machine partagée,
pour `57 775 527` lifts et `1 452 688` supports, soit environ `40` lifts par
support — contre `115` sur `terrain`.

En prolongeant la pente mesurée, `50 000` points volumiques demanderaient de
l'ordre de `400 s` sur ce cœur. Le contrat vise la seconde sur une G4. L'écart
est donc d'un facteur voisin de quatre cents, à obtenir du parallélisme et de
l'arithmétique : l'ablation attribue un tiers du coût au lift, où `i128` pénalise
le plus un device, un tiers à l'énumération, purement bitset, et un cinquième à
l'arbre, qui est un `count/scan/fill`.

Ce n'est pas une prédiction de latence. C'est la distance mesurée, sur le régime
qui bloque le contrat, avec une croissance dont les trois pentes sont vertes.

## 5. Confirmation croisée de la baseline de l'audit

Les supports par point valent `311`, `335`, `363` puis `387` aux quatre tailles.
Ils montent vers la constante de Poisson--Delaunay de l'audit, `480,34` en
volume infini, l'écart restant s'expliquant par les effets de bord qui
diminuent avec `n`. Prolongée à `50 000`, cette suite donne de l'ordre de vingt
à vingt-deux millions de supports, contre les `24,0` millions prédits. La
théorie de l'audit et la mesure se rejoignent.

GCP non utilisé.

## 6. La rampe gelée ferme `terrain` : la porte n'est pas fermée

Le binaire gelé `423797e9...` a clos les trois tailles contractuelles sur
`terrain`, `identique=oui` avant et après chaque cas :

| `n` | cellules | lifts | supports | `wall_s` |
| ---: | ---: | ---: | ---: | ---: |
| `12 500` | `14 262 497` | `92 531 928` | `906 078` | `871` |
| `25 000` | `46 745 417` | `220 298 378` | `1 872 528` | `1 851` |
| `50 000` | `106 894 617` | `486 206 523` | `3 807 762` | `3 223` |

| pente | cellules | lifts | supports |
| --- | ---: | ---: | ---: |
| `12 500 -> 25 000` | `1,713` | `1,251` | `1,047` |
| `25 000 -> 50 000` | **`1,193`** | `1,142` | `1,024` |

La règle de la gate est que **deux pentes successives** au-dessus de `1,35`
ferment l'ordonnance. Ici une seule pente est rouge, sur un seul compteur, et la
suivante est verte : **la porte n'est donc pas fermée**. La superlinéarité des
cellules était transitoire — elle correspond au moment où l'arbre atteint la
résolution du nuage — et non asymptotique.

Ce binaire gelé est antérieur à la séparation par la normale locale : les
chiffres ci-dessus sont ceux de l'ordonnance **sans** ce prune, donc une borne
supérieure du coût de l'ordonnance courante.

Le premier point volumique tombe aussi : `uniform, n=12 500` rend
`1 848 561` cellules — cinquante fois moins que `terrain` à la même taille — pour
`4 990 227` supports, soit `399` par point, et `289 s` de mur contre `871`.
Le régime volumique est donc à la fois plus productif et bien moins coûteux par
support que le régime surfacique du générateur.
