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


---

## 7. P0 tenté : le ledger `W_4` est **inerte depuis la racine**

Date de l'ajout : 16 août 2026 UTC. Répond au P0 de
[`NOTE_AUDITEUR_LBVH_SPARSE_Q3_Q4_APRES_53815F_20260816.md`](NOTE_AUDITEUR_LBVH_SPARSE_Q3_Q4_APRES_53815F_20260816.md).

Le classifieur conjoint est écrit — `DEAD_W4`, `DEAD_NO_CARRIER`, `ACTIVE_ALL`,
`MIXED` — avec le ledger porté par la tâche et hérité au lieu d'être recalculé.
La monotonie qui le justifie tient : raffiner `A` ou `B` **affaiblit** le « pour
toute paire », donc `L4_open` ne peut que croître et le crédit acquis est
définitif.

**Et il ne se déclenche jamais.** Mesuré, `n=120`, quatre familles :

```
dead_w4 = 0        pending = 0        ecart = 0        blocs_faux = 0
```

La cause est structurelle, pas un réglage. La récursion part de
`(racine, racine, racine)`, et la racine **contient** toujours `A`. Aucun nœud
n'est donc jamais disjoint de `A` et de `B` à ce niveau ; aucun point ne peut
être certifié témoin universel du cœur ; `L4_open` reste à zéro.

Le ledger n'a de sens que sur des rectangles où `A` et `B` sont déjà
**séparés** — c'est-à-dire exactement ce que la partition WSPD fournit, et où
`h_coeur` est déjà calculé et mesuré par `combined_prefilter_probe`.

**Conclusion d'architecture, et elle vaut mieux qu'un P0 à moitié fait :** les
deux probes ne doivent pas rester séparés. Le gateway aigu doit tourner **à
l'intérieur** des rectangles WSPD, pas depuis la racine. Le préfiltre fournit la
séparation et le ledger ; le gateway fournit la clause de porteur ; la
conjonction se prend là où les deux sont définis.

La porte `mhgp3v_gateway_ledger_inerte_depuis_racine` grave cette inertie pour
qu'elle ne puisse pas être prise pour un succès. Elle devra devenir
`dead_w4 >= 1` quand la jonction sera faite — c'est le critère de réception du
prochain commit.

Ce que je ne prétends donc pas : le P0 de l'audit n'est **pas** fermé. Le code
de la conjonction existe et est correct ; il est branché au mauvais endroit.

---

## 8. P0 fermé sur la correction, ouvert sur le coût — et une question

Le ledger est **actif** : la source part désormais des rectangles WSPD, les
mêmes que le préfiltre combiné, et non plus de la racine.

| famille | `dead_w4` | `active_edge` | `seed3_emitted` | `pairid_expanded` | `brute` | `sparse` | excès |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `two_lines` | `371` | `0` | `0` | **`0`** | `0` | `0` | — |
| `terrain` | `7 393` | `3 161` | `323` | `323` | `13 912` | `16 411` | `1,180` |
| `uniform` | `20 338` | `9 339` | `404` | `404` | `63 714` | `71 113` | `1,116` |
| `eight_clusters` | `14 934` | `5 761` | `168` | `168` | `90 113` | `101 039` | `1,121` |

`two_lines` tient la gate bloquante de l'audit : `carrier exact = 0`,
`PairId_cross_expanded = 0`, `Seed3_cross_emitted = 0`, `ActiveEdge_cross = 0`.
Et son coût est devenu **sous-linéaire** — exposant `noeuds` `0,86` mesuré sur
`100 -> 200 -> 400` — contre `1,994` avant l'amorçage WSPD.

### Le juge a changé de sens, et c'est correct

La source conjointe est **fail-open** : `L4_open >= r4` exige la mort pour
**toutes** les paires du bloc, donc un bloc peut contenir des ancres mortes sans
être tué. Elle rend un **majorant**. Le juge exige donc `sparse >= brute` —
l'inégalité inverse serait une fermeture fausse — et publie l'excès, qui mesure
le mou du classifieur conjoint : `11` à `18 %`.

J'ai dû corriger le juge lui-même : il comparait la conjonction à **tous** les
triangles aigus, donc son écart était négatif par construction. Il applique
maintenant le filtre de vivacité exact, `|P inter W_4(a,b)| < r4`.

### Deux corrections trouvées en route

**La troncature était collante.** Une fois `tronquee` vrai, `U4` restait infini
pour toute la descendance, `ALL_STRICT` ne pouvait plus se déclencher et la
récursion descendait jusqu'aux feuilles : `3,4` milliards de nœuds à `n=800` sur
`terrain`. Elle se réévalue à chaque rafraîchissement.

**Le ledger était recalculé pour rien.** Il ne dépend que de `(A,B)` ; scinder
`C` ne le change pas. L'hériter divise le coût par `2` à `2,4`.

### Ce qui n'est PAS résolu, et je ne le maquille pas

Sur les familles denses, le coût reste **inacceptable** : exposant `noeuds`
`2,95` puis `4,73` sur `terrain` entre `n=100`, `200` et `400`. Cause
identifiée : je scinde encore `A` et `B` **à l'intérieur** d'un rectangle WSPD,
alors que le rectangle *est* déjà la partition des paires. Chaque rectangle
refait donc le travail de la WSPD, et cela se multiplie avec la descente de `C`.

`two_lines` y échappe parce que ses rectangles meurent haut, avant tout
raffinement.

### La question à l'auditeur

Deux lectures s'opposent, et je n'ai pas d'argument décisif :

1. **`(A,B)` figé dans un rectangle WSPD.** Le rectangle est la partition des
   paires ; seul `C` descend. Le coût redevient `rectangles x arbre`. Mais j'ai
   mesuré au `53815f` qu'un certificat aigu à `(A,B)` fixé n'élague presque que
   des feuilles — `1,1` point par bloc.
2. **`(A,B)` raffinable.** Le certificat devient assez fort pour tuer, mais le
   coût explose comme ci-dessus.

La sortie est-elle un critère de scission qui n'autorise le raffinement de
`(A,B)` que lorsque le gain certifié le paie — le ratio de la section 3.4 — ou
faut-il une autre structure entre les deux ? Je penche pour le premier, avec le
ratio évalué sur `masse classée / tâches enfants`, mais je n'ai pas mesuré s'il
suffit à ramener l'exposant sous `2`.


---

## 9. Deux rétractations, vérifiées avant d'être admises

L'audit
[`AUDIT_CONSTRUCTIF_FC634_F614_JONCTION_WSPD_LBVH_20260816.md`](AUDIT_CONSTRUCTIF_FC634_F614_JONCTION_WSPD_LBVH_20260816.md)
réfute deux de mes phrases. J'ai vérifié ses deux contre-exemples en
arithmétique exacte avant de les recevoir ; les deux tiennent.

### `#carriers = O(h)` est faux comme borne déterministe

Dans le plan médiateur de `a=(-R,0,0)`, `b=(R,0,0)`, tout `x=(0,u,v)` avec
`s = u²+v²` vérifie `E = X = R²+s` et `H = R²-s`. Pour `R² < s < 3R²` :
`E < D`, `X < D`, `H < 0` — arête maximale stricte, triangle aigu. Et `H < 0`
place `x` **hors** de la boule diamétrale, donc hors de `W_2`, donc hors de
`W_4`.

Fixture u16 gravée, `a=(1000,1000,1000)`, `b=(1020,1000,1000)`,
`100 < u²+v² < 300` : **`632` points, `632` porteurs, `0` témoin `W_2`/`W_3`/`W_4`**.

Les deux régions sont **géométriquement disjointes** — `W_4` du côté `H > 0`, un
porteur du côté `H < 0`. Le rapport `|L|/|W_4| = 10,86` contrôle donc une
**moyenne sous homogénéité** et ne domine aucune cardinalité. J'avais transformé
une borne en espérance en borne déterministe : c'est la même erreur de
quantificateur que celle qui m'avait déjà coûté le facteur `6,4` et les
exposants.

L'énoncé correct : `O(h)` en espérance sous Poisson homogène, avec la constante
`29,335` par paire vivante à `h_4 = 8` ; `Theta(n)` dans le pire cas. Une
version déterministe exige une hypothèse de régularité locale de type Ahlfors,
qui n'appartient pas au contrat.

### Le cœur de Jung ne génère pas le quatrième sommet

Ma ligne `y <- requête sur le cœur de Jung B(c0,|ab|/4)` était fausse si `y`
désigne l'apex. Le cœur est contenu dans **toutes** les boules q4 admissibles :
ses points sont des **intérieurs permanents**, l'objet qui fait mourir le seed à
huit IDs. Un sommet de support vit sur le **shell** d'une sphère particulière.

Vérifié sur le tétraèdre régulier — quatre sommets alternés de `{0,2}³`, seed
`(a,b,x)`, circumcentre plan `c0 = (2/3, 2/3, 4/3)` :

`|y - c0|² = 16/3` contre `(|ab|/4)² = 1/2`, soit un facteur `32/3`.

La route correcte est : cœur de Jung pour la mort par intérieurs permanents,
puis `Q4SeedAxisTopR4-LBVH` pour l'apex, parmi les premières et dernières
racines axiales.

Les deux fixtures sont permanentes sous `mhgp3v_gateway_contre_fixtures`.

---

## 10. Le verrou de coût : deux extrêmes chiffrés, une synthèse réfutée, un théorème

### Ma réfutation du certificat au niveau rectangle était invalide

Au `53815f` j'avais écrit que le certificat aigu à `(A,B)` fixé « n'élague que
des feuilles, `1,1` point par bloc ». **Cette mesure précédait le correctif
`tlo/thi`** : elle jugeait donc le certificat sur des cellules de Morton
alignées, bien plus larges que les boîtes serrées. Refaite proprement,
`terrain` à `n=800` :

| régime | `noeuds` | exposant | paires résiduelles / `C(n,2)` |
| --- | ---: | ---: | ---: |
| `(A,B)` scindable | `3 416 M` | `2,95 -> 4,73` | `0` |
| `(A,B)` figé | `54 M` | `2,68 -> 1,39` | **`47,7 %`** |

Sur `eight_clusters`, figé laisse `65,9 %` des paires. Figer coûte `63` fois
moins et laisse la moitié du travail ; scinder tue tout et explose. **Aucun des
deux ne marche seul.**

### Le schéma à deux fronts, que j'ai proposé puis réfuté moi-même

Figer d'abord, puis ne raffiner que les rectangles résiduels : la correction
tient (`ecart > 0`, `blocs_faux = 0`, excès `1,173` inchangé après défalcation
du double comptage entre fronts), et `residuel_rects` tombe à zéro.

Mais le coût ne bouge pas : `3,42` **milliards** de nœuds à `n=800` sur
`terrain`, contre `3,416` milliards pour le front unique. **Les rectangles
résiduels sont les rectangles chers.** Un seuil binaire sur « quand raffiner »
ne peut donc rien, et le critère de ratio de la section 3.4 de l'audit ne le
pourra pas davantage tant que la cause reste en place.

### La cause, et le théorème qui la corrige

Quand `(A,B)` se scinde, ma descente de `C` **repart de la racine**.

Or les trois causes de mort — `Phi_max <= 0`, `Delta_E,max < 0`,
`Delta_X,max < 0` — sont des **maxima** sur le produit `A x B x C`. Raffiner
`A` ou `B` rétrécit ce produit, donc chacun de ces maxima ne peut que décroître.
**Un sous-arbre `C` mort le reste pour tous les descendants de `(A,B)`.**

`ALL_STRICT` est stable pour la raison symétrique : ses conditions sont des
**minima**, `Phi_min > 0` et `Delta_min > 0`, qui ne peuvent que croître.

Donc la frontière `C` s'hérite exactement comme le ledger `W_4` : on ne
re-teste que les **indécis**. Redescendre depuis la racine après chaque
scission est du travail pur — et c'est exactement l'erreur que j'avais déjà
faite sur le ledger, où l'hériter avait divisé le coût par `2` à `2,4`.

C'est aussi, mot pour mot, le « cover partagé par arête, construit une fois et
réutilisé » de la section 8.1 de l'audit LBVH. Les deux raisonnements
convergent.

`decide_stable_sous_raffinement` porte l'énoncé dans
`prototype/acute_owner_gateway.hpp`. **L'implémentation de l'héritage de la
frontière `C` n'est pas faite** — c'est le prochain commit, et son critère de
réception est l'exposant `noeuds` sous `2` sur les trois familles denses.

---

## 11. Commit 1 de l'audit `79e73b6` : l'oracle avant l'optimisation

Les deux P0 étaient réels. Je les ai corrigés dans cet ordre, et le premier
**améliore** la source au lieu de la ralentir.

### P0.1 — le masque endpoint est relationnel, pas géométrique

Ma version supprimait **définitivement** un span témoin dès qu'il recouvrait `A`
ou `B`. C'est faux : un `z` de `A` est endpoint pour **certaines** paires de
`A x B`, mais reste un témoin possible pour toute paire `(a,b)` avec `a != z`.
Après restriction de `A` à un enfant qui ne le contient plus, il doit
**redevenir** un témoin ordinaire.

Règle correcte : jamais crédité au minorant, **conservé** dans le majorant,
**rejoué** après toute restriction de `A` ou `B`.

| | avant | après |
| --- | ---: | ---: |
| `dead_w4`, `terrain` | `7 393` | **`8 262`** |
| excès, `terrain` | `1,173` | **`1,126`** |
| excès, `uniform` | `1,114` | **`1,092`** |
| excès, `eight_clusters` | `1,121` | **`1,092`** |

Garder les spans pour rejeu rend le ledger plus précis : c'est une correction
qui paie deux fois.

### P0.2 — le juge par cardinal ne prouve rien

`sparse >= brute` accepte qu'une incidence **vraie manquante** soit compensée
par une surnuméraire venue d'une ancre morte. Le juge compare désormais les
**ensembles** de clés `(EdgeKey(a,b), PointId(x))`, et sépare quatre quantités :

| quantité | sens | verdict |
| --- | --- | --- |
| `manquantes` | incidence vraie absente | fermeture fausse, code `1` |
| `doublons` | même clé émise deux fois | exact-once cassé, code `1` |
| `fausses` | clé émise qui n'est pas porteur | certificat menteur, code `1` |
| `surcouverture` | porteur **réel** d'ancre morte | publié, **pas** une faute |

Mesuré sur les quatre familles : `manquantes = 0`, `doublons = 0`,
`fausses = 0`, et `cles_sparse == uniques` — aucune double émission. L'excès est
**intégralement** de la surcouverture : `1 758` / `5 847` / `8 329`.

C'est un résultat plus fort que le cardinal : il ne dit plus « le compte tombe »,
il dit **quelles** incidences sont émises.

### Le mutant qui prouve que le nouveau juge est strictement plus fort

`juge-compense` omet un porteur vrai et émet une clé bidon à la place. Le
**cardinal reste exact** — une omission, une surprise — donc l'ancien juge
passe. Le juge par identités rend `manquantes=1 fausses=1` et le tue, code `4`.

Sans ce mutant, rien ne distinguerait les deux juges, et j'aurais pu croire
avoir renforcé quelque chose sans l'avoir fait.

### Ce qui reste du Commit 1

Le juge exhaustif de la partition des `PairId`, **indépendant du gateway**. Mon
`doublons = 0` sur les clés ternaires en est une forme forte — chaque
`(arête, apex)` est émis une fois exactement — mais ce n'est pas encore
l'énoncé « chaque paire non ordonnée apparaît dans exactement un état
terminal », qui doit se tester sans passer par les porteurs.
