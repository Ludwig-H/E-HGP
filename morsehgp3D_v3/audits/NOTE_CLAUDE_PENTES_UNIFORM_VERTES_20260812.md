# Note de Claude — la pente rouge est une propriété du générateur, pas de la route

Date : 12 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Statut : note d'observation de Claude, corrigée et remise en portée par
[`AUDIT_DEBLOCAGE_GPU_SUPPORTKEY_TOP12_20260812.md`](AUDIT_DEBLOCAGE_GPU_SUPPORTKEY_TOP12_20260812.md).
Ses mesures locales non pincées restent des hypothèses; le statut logiciel et
les pins appartiennent à [`AUDIT_ETAT_COURANT.md`](AUDIT_ETAT_COURANT.md).

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

L'octree subdivise l'espace des **centres**, dont la boîte a ce volume. Cette
géométrie explique une phase transitoire proche de `n^{1,5}` alors que la sortie
reste quasi linéaire — supports `1,047`, census `1,106`; elle ne prouve pas une
loi asymptotique des cellules.
L'histogramme en profondeur le confirme : à `n=1 500`, la profondeur six rend
`42 659` terminaux pour `78 584` supports, tandis que la profondeur sept en rend
`61 978` pour seulement `15 784`.

Certains balayages LiDAR couvrent plus de sol à variation verticale comparable,
mais ce n'est pas une propriété universelle d'un nuage réel. La croissance
verticale de `terrain` est un choix de modèle. Cela n'excuse pas la route : elle paie le
volume vide, et ce coût est réel sur tout nuage à grand volume vide. Cela situe
seulement le problème.

## 3. Deux prunes exacts ajoutés

**Séparation par la normale locale.** Le k-DOP à directions fixes ne voit pas
une cellule située hors du plan local : ses dix plus proches sites sont presque
cosphériques vus de loin, tous leurs intervalles se recouvrent, et le critère de
travail la découpe indéfiniment alors qu'aucun support positif ne peut y avoir
son centre. La direction est choisie dans les données par une colonne de
l'adjugée de la covariance. Elle est une normale lorsque cette covariance est
exactement de rang deux; en rang trois, ce n'est généralement pas la normale
des moindres carrés. **Le test reste entier et exact pour toute direction
choisie**, donc le prune l'est. Sur `terrain, n=1 500`, l'observation non pincée annonce : cellules
`201 889` vers `142 917`, soit `-29 %`, terminaux `-36 %`, temps `3,848 s` vers
`2,747 s`, soit `1,40x`, et même cardinal de supports; elle ne constitue pas un
reçu d'identités ni de performance.

**Prune de rang au niveau cellule.** La positivité impose `c` dans
`relint conv(U)` : pour toute direction, le support contient un point
de projection supérieure et un de projection inférieure à celle du centre,
avec égalité possible lorsque la direction est orthogonale à `aff(U)`. Donc
`beta` majore les deux distances minimales, et
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

En prolongeant la pente mesurée, un dimensionnement non reçu place `50 000`
points volumiques autour de `400 s` sur ce cœur. Le contrat vise la seconde sur
une G4. Le rapport voisin de quatre cents mesure seulement l'écart entre ce
prototype CPU et la cible; il ne se transpose pas en facteur d'accélération
GPU. L'ablation non pincée attribue approximativement un tiers au bloc
lift--centre--owner--positivité, un tiers à l'énumération et un cinquième à
l'arbre; elle n'isole ni le lift ni le coût device.

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
cellules est transitoire sur les trois tailles observées; cela ne prouve aucune
loi asymptotique de l'octree.

Ce binaire gelé est antérieur à la séparation adaptative : les chiffres
ci-dessus décrivent l'ordonnance **sans** ce prune. Ils ne bornent pas le temps
du successeur, qui paie le choix de direction et change aussi la politique de
terminalisation; seuls certains comptes de sous-arbre peuvent être comparés
sous un A/B reçu.

Le premier point volumique tombe aussi : `uniform, n=12 500` rend
`1 848 561` cellules — cinquante fois moins que `terrain` à la même taille — pour
`4 990 227` supports, soit `399` par point, et `289 s` de mur contre `871`.
Le régime volumique est donc à la fois plus productif et bien moins coûteux par
support que le régime surfacique du générateur.

## 7. La campagne gelée est complète : la porte uniforme de compteurs est verte

Trois familles diagnostiques, trois tailles du protocole, un seul binaire gelé
`423797e9...`, `identique=oui` avant et après chaque cas. Le transcript possède
neuf retours `rc=0` et un footer. Il n'inclut ni `eight_clusters`, ni digest
d'identités, ni mémoire, et ses temps sont contaminés : ce n'est pas une rampe
contractuelle ni une mesure de latence.

### `uniform` — le régime volumique bloquant

| `n` | cellules | lifts | supports | par point | `wall_s` |
| ---: | ---: | ---: | ---: | ---: | ---: |
| `12 500` | `1 848 561` | `194 463 795` | `4 990 227` | `399` | `289` |
| `25 000` | `3 480 121` | `410 527 574` | `10 387 850` | `416` | `519` |
| `50 000` | `7 773 329` | `839 582 666` | `21 395 212` | `428` | `934` |

| pente | cellules | lifts | bornes | quadruplets | census | supports |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `12 500 -> 25 000` | `0,913` | `1,078` | `0,963` | `1,104` | `1,117` | `1,058` |
| `25 000 -> 50 000` | `1,159` | `1,032` | `1,129` | `1,010` | `1,034` | `1,042` |

**Aucun compteur n'atteint `1,16`.** Deux pentes successives, toutes vertes, sur
la famille que la section 14.5 rend bloquante. La gate de croissance
`uniform` de cette ordonnance est donc verte; la gate complète avant CUDA exige
encore `eight_clusters`, les octets/high-water et le producteur device.

### `scanline_single_pass`

`12 500 -> 25 000` : cellules `0,901`, lifts `1,050`, bornes `0,991`,
quadruplets `1,063`, census `1,084`, supports `1,009`. Vert également.

### `terrain`

`1,713` puis `1,193` sur les cellules, `1,732` puis `1,198` sur les bornes. Une
seule pente rouge, suivie d'une verte : la règle des deux pentes successives ne
ferme pas l'ordonnance. La cause est le volume vide croissant du générateur,
analysé en section 2.

## 8. Ce que le point volumique à 50 000 dit du contrat

`21 395 212` supports, soit `428` par point. La baseline Poisson--Delaunay de
l'audit prédit `480,34` en volume infini et de l'ordre de `24,0` millions à cette
taille : la mesure tombe à `11 %` en dessous, l'écart s'expliquant par les effets
de bord d'une boîte finie. **La théorie de l'audit et la mesure se rejoignent sur
la famille contractuelle, à la taille contractuelle.**

Le coût mesuré est `839 582 666` lifts et `934 s` de mur sur un hôte partagé à
deux cœurs, donc environ `39` lifts par support. Ce mur n'est pas qualifiable :
il a été relevé pendant l'exécution concurrente des portes CTest. Il situe
seulement l'ordre de grandeur de l'écart au seuil d'une seconde, qui reste à
prendre au parallélisme et à l'arithmétique.

## 9. La lecture `k=1` est un diagnostic, pas le remplacement de Yao-1

Une paire q2 à zéro intérieur a seulement sa boule diamétrale **ouverte** vide;
un troisième site peut appartenir à sa coquille. Le live collecte donc un
sur-graphe du Gabriel fermé. Tout EMST reste inclus dans ce sur-graphe, et le
Kruskal donne bien les poids d'une MST. La fixture
`u=(0,1,0),v=(2,1,0),w=(1,2,0)` sépare les notions : `uv` a zéro intérieur et
`w` sur sa coquille, mais `uv` n'appartient à aucun MST.

Le juge Prim exhaustif compare correctement le multiensemble trié des poids,
qui est invariant même lorsque la MST n'est pas unique. Les cinq portes k1
passent. Elles ne reçoivent toutefois ni le catalogue Gabriel, ni les lots H0 :
le sujet jette les endpoints et ne publie que `K1 d2`, où `d2=4 beta`; le juge
trie ces scalaires et ne vérifie ni ordre, ni partitions strictes/fermées, ni
multifusions. Un run de poids égaux n'est pas un lot atomique sans snapshot des
racines pré-lot et composantes du graphe quotient.

Surtout, `--k1` exécute d'abord toute la source cellules q2/q3/q4, ses lifts,
owners et census. Sur `terrain,n=400`, il paie encore `1 768 790` lifts avant
de réduire `832` arêtes en 399 poids. Il ne retire donc rien du chemin 50 000.
Yao-1 à au plus `48n` reste la voie produit indépendante; cette lecture est un
comparateur utile lorsque le catalogue complet a déjà été calculé. Le
contre-audit détaillé appartient à
[`AUDIT_JUGE_CELLULES_INDEPENDANT_90C06B0_20260812.md`](AUDIT_JUGE_CELLULES_INDEPENDANT_90C06B0_20260812.md).
