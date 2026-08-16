# Note de l’auditeur — source sparse q3/q4 par LBVH et top-k axial

Date : 16 août 2026 UTC.  
Pin fonctionnel relu : `53815f53207176cb421f30c61b967721d6cc4478`.  
Dossier : `morsehgp3D_v3/`.

Composants directement concernés :

- [`prototype/morton_lbvh.hpp`](../prototype/morton_lbvh.hpp) ;
- [`prototype/acute_owner_gateway_probe.cpp`](../prototype/acute_owner_gateway_probe.cpp) ;
- [`prototype/q4seed_axis_topr4.hpp`](../prototype/q4seed_axis_topr4.hpp) ;
- [`prototype/axis_device_job.hpp`](../prototype/axis_device_job.hpp) ;
- [`prototype/axis_device_kernel.cu`](../prototype/axis_device_kernel.cu) ;
- [`prototype/edge_shallow.hpp`](../prototype/edge_shallow.hpp) ;
- [`PROPOSITION.md`](../PROPOSITION.md).

Cadre : `phase=exploration_v3_hors_registre`,
`backend=math_reference_and_gpu_architecture`,
`profile=quantized_u16_input_only`,
`mode=sparse_q3_q4_lbvh_design`,
`public_status=not_claimed`.

> [!IMPORTANT]
> **Verdict.** Oui : la structure spatiale pertinente existe déjà. Ce n’est pas
> un octree 8-aire à pointeurs, mais un **LBVH radix binaire sur clés Morton**,
> avec AABB exactes et plages contiguës de `PointId`. C’est préférable sur GPU.
>
> Elle doit être utilisée pour trouver le quatrième sommet q4, mais pas comme
> une requête de voisinage euclidien répétée pour chaque seed. Le quatrième
> sommet utile est une **racine extrémale de puissance affine**. La bonne
> primitive est donc un top-k axial par branch-and-bound sur les nœuds du LBVH,
> partagé entre les seeds d’une même arête owner.
>
> Avant cela, le commit `53815f` impose de fusionner au niveau bloc :
>
> ```text
> ancre possiblement W4-vivante
> ET
> existence d’un carrier aigu owner.
> ```
>
> Le gateway aigu seul est exact mais énumère un objet cubique. Le LBVH ne doit
> pas accélérer des seeds qui n’auraient jamais dû être produits.

> [!CAUTION]
> Cette version corrige une phrase de la première publication du document : les
> huit coins d’une AABB suffisent à certifier un **maximum** négatif d’une
> puissance convexe, donc un nœud permanent. Ils ne suffisent pas à certifier un
> **minimum** positif. Le prune `NO_ROOT_ALL_POSITIVE` est retiré du v0 tant que
> son minimum algébrique exact n’est pas implémenté.

---

## 1. Réception du commit `53815f`

Le nouveau `AcuteOwnerGateway(A,B,C)` est mathématiquement bien orienté :

```text
Phi      = (a-x)·(b-x),
Delta_E  = ||a-b||²-||a-x||²,
Delta_X  = ||a-b||²-||b-x||².
```

Les extrema sur trois AABB sont calculés exactement par séparabilité axe par
axe. Le juge au niveau nuage a utilement trouvé les erreurs que l’oracle de
boîtes ne pouvait pas voir : boîte Morton non serrée, auto-jointure non
canonique et mélange entre ordre Morton et `PointId`.

La porte `two_lines` est excellente :

```text
pairid_expanded=0,
carriers=0.
```

La mesure négative est tout aussi utile : sans filtre de vivacité de l’ancre,
la masse des carriers aigus croît comme `n³`. À `uniform,n=800`, elle représente
environ 46 % de tous les triples. Le gateway n’est pas faux ; il énumère
exactement le mauvais sur-objet.

Conclusion reçue :

> **Ne jamais lancer la source axiale q4 depuis tous les triangles aigus.**

---

## 2. La structure spatiale déjà présente

`MortonLbvh` fournit :

```text
MortonKey 48 bits,
ordre canonique (MortonKey,PointId),
AABB u16 exacte par nœud,
plage contiguë [begin,end),
enfants left/right.
```

C’est une hiérarchie octree-like. Pour le device, le LBVH linéaire est
préférable à un octree classique à pointeurs :

- tableaux SoA contigus ;
- construction par radix/Karras ;
- parcours stackless ou à petite pile ;
- cohérence Morton ;
- subdivision binaire régulière.

La classe CPU actuelle recalcule encore les AABB en rescannant les plages. Le
port produit devra employer :

```text
radix sort (MortonKey,PointId)
 -> topologie Karras
 -> réduction bottom-up des AABB.
```

Il n’est pas nécessaire de créer un deuxième index. Un overlay `BVH8` regroupant
trois niveaux binaires peut être testé ensuite pour réduire la profondeur et
utiliser les ballots de warp.

---

## 3. P0 : jointure corrélée W4-vivacité × carrier aigu

### 3.1 Pourquoi les deux clauses ne se séparent pas

Pour un bloc `A×B`, les deux assertions suivantes ne suffisent pas :

```text
il existe une paire W4-vivante,
il existe une paire possédant un carrier aigu.
```

Elles peuvent être réalisées par des paires différentes. La source doit
raffiner jusqu’à prouver ou rejouer la conjonction pour les mêmes paires.

### 3.2 Ledger de vivacité

Chaque tâche `A×B` porte pour q4 :

```text
L4_open   : nombre d’IDs universellement W4-intérieurs,
U4_closed : cardinalité de l’union fixe encore possiblement W4,
credit_spans disjoints,
none_spans,
frontier MIXED,
continuation.
```

Avec le seuil de rejet `r4=8` :

```text
L4_open >= 8   -> toutes les paires sont mortes ;
U4_closed <= 7 -> toutes les paires sont W4-vivantes ;
sinon          -> vivacité MIXED.
```

Les égalités restent dans `U4_closed` et ne créditent jamais l’intérieur.

### 3.3 Produit avec le gateway aigu

Le classifieur conjoint rend :

```text
DEAD_W4
  si L4_open >= 8 ;

DEAD_NO_CARRIER
  si AcuteOwnerGateway = DEAD ;

ACTIVE_ALL
  si U4_closed <= 7
  et AcuteOwnerGateway = ALL_STRICT
  et C contient un vrai PointId relationnellement admissible ;

MIXED
  sinon.
```

`ACTIVE_ALL(A,B,C)` ne matérialise ni `PairId` ni face. Il produit un
`ActiveOwnerEdgeBlock(A,B)` avec handle de preuve carrier. Une tâche `MIXED`
peut scinder `A`, `B`, `C` ou la frontier witness W4. Fixer toujours `C`
reproduirait l’ablation déjà réfutée.

### 3.4 Split GPU-compatible

Tester virtuellement un niveau de split pour les facteurs admissibles, puis
choisir celui qui maximise :

```text
masse immédiatement classée DEAD ou ACTIVE
------------------------------------------------
nombre de tâches enfants + coût fixe des extrema.
```

Le tie-break est déterministe et versionné. Cette décision ne manipule que
quelques entiers et peut être prise par warp.

### 3.5 Gate bloquante

Sur `two_lines` :

```text
V4 logique cross = Theta(n²),
carrier exact cross = 0,
PairId_cross_expanded = 0,
Seed3_cross_emitted = 0,
ActiveEdge_cross = 0,
pending = 0.
```

Sur les autres familles, publier séparément :

```text
pair blocks physiques,
masse logique W4,
masse logique carrier,
masse logique de la conjonction,
exact edges micro-développées,
bytes et HWM.
```

---

## 4. Cover LBVH exact d’une arête owner

Fixer une arête exacte `e={a,b}` et poser :

```text
D = ||a-b||²,
m = (a+b)/2.
```

Pour tout q4 admissible d’owner `e`, Jung donne :

```text
R² <= 3D/8,
||c-m||² <= D/8.
```

Tout sommet, intérieur ou point de shell vérifie :

```text
||z-m||
 <= R+||c-m||
 <= (sqrt(3)+1)/sqrt(8) * sqrt(D)
 < sqrt(D).
```

Donc la requête entière :

```text
||2z-a-b||² <= 4D
```

contient tous les sites pouvant affecter le q4 : apex, intérieurs et shell.

Pour être un sommet sous owner `e`, le point doit en plus être dans la lentille :

```text
||z-a||² <= D,
||z-b||² <= D,
```

ce qui implique :

```text
||2z-a-b||² <= 3D.
```

Chaque nœud du cover reçoit un masque :

```text
DEPTH_ONLY       : peut affecter le census, jamais être apex owner ;
APEX_POSSIBLE    : intersecte la lentille ;
APEX_ALL         : entièrement dans la lentille, hors ties non résolus ;
OUTSIDE          : rejeté.
```

Le cover est une antichaîne de nœuds LBVH, jamais une liste de tous les IDs. Il
est construit une fois par arête et partagé par ses seeds.

---

## 5. Le quatrième point est une racine extrémale

Pour un seed `T=(a,b,x)`, `Q4SeedAxisTopR4` paramètre les centres par :

```text
c(tau)=a+(W+tau*n)/(2G),
2 tau² <= T2.
```

Pour un site `z` :

```text
P_z(tau)=A_z-tau B_z,
A_z = G||z-a||²-W·(z-a),
B_z = n·(z-a).
```

Lorsque `B_z!=0`, sa racine est :

```text
rho_z=A_z/B_z.
```

Le théorème top-r4 demande seulement :

```text
les k premières racines B>0,
les k dernières racines B<0,
k=8-p,
```

avec tous les IDs à égalité.

Une requête de plus proches voisins de `x` serait donc sans rapport avec l’ordre
utile. Le LBVH doit faire du branch-and-bound sur `rho`, la métrique spatiale ne
servant que de premier prune.

---

## 6. Bornes exactes par nœud LBVH

### 6.1 `B_z`

`B_z=n·(z-a)` est affine. Sur une AABB, `B_lo,B_hi` sont exacts en choisissant
l’extrémité dictée par le signe de `n_i`.

```text
B_lo>0       -> entrant homogène ;
B_hi<0       -> sortant homogène ;
B_lo=B_hi=0  -> constant ;
sinon        -> split.
```

### 6.2 `A_z`

`A_z` est séparable :

```text
A_z=sum_i [G s_i²-W_i s_i],
s_i=z_i-a_i.
```

`G>0`. Par axe :

- le maximum est à une extrémité ;
- le minimum entier est à l’un des deux entiers voisins de `W_i/(2G)`, clippés à
  l’intervalle.

On obtient `A_lo,A_hi` exacts sur le réseau u16 de l’AABB. Les comparaisons de
racines conservent `BigInt<4>`.

### 6.3 Permanents

Appliquer d’abord `SeedCoreQuarter` :

```text
v(z)=2G(z-a)-W,
4 max_Z ||v(z)||² <= D G².
```

Il certifie tout le nœud permanent. Les trois IDs du seed sont masqués et les
spans crédités forment une antichaîne. Dès huit IDs distincts :

```text
DEAD_PERMANENT,
root comparisons=0.
```

Le certificateur plus fort `SeedJungPermanent16` vérifie les huit coins aux
deux bouts de Jung. Cette utilisation est correcte : pour un bout fixé,
`P_z(tau)` est convexe en `z`; son **maximum** sur une boîte est atteint à un
coin. Si les seize valeurs sont strictement négatives, tout le nœud est
permanent.

### 6.4 Correction importante : pas de prune positif par les coins

Pour prouver qu’un nœud est strictement extérieur à toutes les sphères de
`J_f`, il faudrait montrer :

```text
min_z P_z(-tau_max)>0
et
min_z P_z(+tau_max)>0.
```

La puissance étant convexe, ce minimum peut être intérieur. Les huit coins ne
suffisent donc pas.

Le v0 doit choisir l’une des deux solutions sûres :

1. **ne pas utiliser ce prune** et descendre ;
2. implémenter le minimum séparable exact de
   `G s_i²-(W_i±tau_max n_i)s_i` avec comparaison algébrique exacte pour les
   entiers voisins du minimiseur.

La première est recommandée pour fermer rapidement l’oracle. La seconde est une
optimisation ultérieure.

### 6.5 Prune exact contre le seuil courant

Supposons un nœud entrant `B_z>0` et le seuil :

```text
rho_*=A_*/B_*, B_*>0.
```

Alors :

```text
A_z/B_z >= A_*/B_*
<=>
F_*(z)=B_* A_z-A_* B_z >= 0.
```

`F_*` est une somme de quadratiques convexes séparables. Son minimum entier exact
sur l’AABB se calcule comme pour `A_z`.

```text
min F_*>0 -> nœud strictement pire, prune ;
min F_*=0 -> groupe d’égalité possible, descendre ;
min F_*<0 -> amélioration possible, descendre.
```

Pour `B_z<0`, réfléchir le paramètre :

```text
eta=-rho=A_z/(-B_z).
```

Chercher les dernières racines en `rho` devient chercher les premières en
`eta`. Ce prune conserve la corrélation de `A_z` et `B_z`; il est plus fort et
plus sûr qu’une division d’intervalles indépendante.

---

## 7. Algorithme `Q4SeedAxisTopR4-LBVH`

### 7.1 Passe seuil

Pour chaque seed :

```text
p=permanents bulk,
k=8-p.
```

Si `k<=0`, mourir. Sinon lancer deux recherches best-first sur le cover de
l’arête :

```text
entrant : B_lo>0, minimum de rho ;
sortant : B_hi<0, minimum de eta=-rho.
```

Chaque recherche maintient :

```text
petite frontier de nœuds,
top-k fixe de racines,
seuil courant,
continuation en cas de spill.
```

Les feuilles évaluent `SitePower` exactement. Une fois `k` racines trouvées, le
prune `F_*` élimine les nœuds strictement pires.

### 7.2 Passe égalités

Rejouer le cover avec le seuil reçu :

```text
racine meilleure ou égale -> conserver,
racine strictement pire   -> prune.
```

Tous les vrais `PointId` du groupe frontière sont émis. Une égalité peut contenir
plus de `k` IDs : capacité préflightée ou continuation, jamais troncature.

### 7.3 Replay

Pour chaque apex retenu :

```text
distinct-ID4,
owner6 canonique,
carrier primaire,
indépendance affine,
barycentriques q4 strictement positives,
BallKey,
I_B/U_B depuis le reçu axial,
RLE/fold.
```

---

## 8. Ordonnance GPU

### 8.1 Changer l’unité de kernel

Le kernel actuel affecte un thread par seed et `select_axis_topr4` balaie
plusieurs fois son CSR de sites. Le code identifie déjà ce balayage comme le mur
de temps.

L’unité correcte devient :

```text
un CTA par arête owner ou microtuile d’arêtes,
plusieurs warps pour les seeds de cette arête.
```

Le cover spatial est construit une fois et réutilisé.

### 8.2 Disposition

```text
warp 0      : traversal spatial et NodeHandle du cover ;
warps 1..w  : seeds x, top-k axial ;
shared      : frontier, cover courant, top-k, seuils ;
global      : spills/continuations seulement.
```

Deux variantes doivent être mesurées :

1. warp par seed ;
2. warp par nœud, lanes représentant plusieurs seeds.

La seconde partage mieux les lectures lorsque une arête possède beaucoup de
carriers; la première diverge moins lorsque les axes se séparent vite.

### 8.3 Capacités et SoA

```text
top-k <=8 par côté,
frontier locale 32 ou 64 nœuds,
égalité locale bornée,
spill explicite.
```

Un overflow local rend `PENDING_RESOURCE`, jamais `DEAD`.

Stockage SoA :

```text
node_lo_x/y/z,
node_hi_x/y/z,
node_begin/end,
node_left/right,
point_x/y/z,
PointId.
```

Trier les jobs par niveau Morton, classe de diamètre et `RectId` pour améliorer
la cohérence.

### 8.4 Pas de CSR de sites par seed

Remplacer :

```text
site_offset par seed,
site_id par seed
```

par :

```text
edge_cover_offset,
edge_cover_node,
seed_to_edge.
```

Le point n’est développé qu’à la feuille terminale.

### 8.5 Transaction

```text
count -> preflight -> scan -> fill -> validate -> publish.
```

Aucun `cudaMalloc` par vague et aucun payload partiel.

---

## 9. Backend hybride

Le top-k axial LBVH est le premier choix lorsque `c_e`, nombre de carriers d’une
arête, et `m_e`, taille de son cover, sont modérés.

```text
si c_e*m_e <= budget_axis :
    Q4SeedAxisTopR4-LBVH ;

sinon :
    EdgeCenterShallowCut sur l’arrangement 2D de l’arête.
```

Le second backend construit collectivement les premiers niveaux de l’arrangement
de droites dans le plan médiateur. Le LBVH continue de fournir les blocs
`candidate_line` et `witness_only`.

Le switch se choisit par mesure :

```text
node visits,
BigInt comparisons,
conflict HWM,
root groups,
wall time.
```

Les deux backends doivent rendre exactement les mêmes `SupportKey`, `BallKey`,
owner, primary, positivité, `I_B/U_B` et fates.

---

## 10. Lane q3

La même infrastructure sert q3 sans faire de Lane3 une source pour Lane4.

Pour une arête q3 active :

1. le cover de lentille produit les carriers `x` par blocs ;
2. chaque `x` définit une unique circumsphère q3 ;
3. un parcours AABB-sphère exact compte les intérieurs avec arrêt au neuvième ;
4. le shell est reporté complètement si le support survit.

Pour petit `c_e`, cette requête par carrier est simple. Pour grand `c_e`, la vue
q3 de l’arrangement traite collectivement les pieds des droites dans l’ellipse
`||t||²<=D/12`.

La géométrie neutre peut être partagée : cover LBVH, base du plan médiateur,
coefficients de droite et bornes de nœuds. Les seuils et ledgers restent séparés :

```text
q3 : rejet à 9 intérieurs,
q4 : rejet à 8 intérieurs.
```

---

## 11. Gates

### 11.1 Parité axiale

```text
select_axis_topr4 scan complet
==
select_axis_topr4_lbvh
```

champ par champ : verdict, permanents, `k`, entrants, sortants, égalités,
profondeur min et census.

### 11.2 Mutants causaux

```text
axis-node-utilise-Alo/Bhi-sans-signe,
axis-node-oublie-correlation-F,
axis-node-prune-egalite,
axis-node-melange-B-positif-negatif,
axis-node-oublie-depth-only,
axis-node-core-parent-enfant,
axis-node-coins-certifient-minimum-positif.
```

### 11.3 `two_lines`

La famille meurt au front conjoint W4/carrier :

```text
axis_jobs=0,
LBVH_axis_node_visits=0,
PairId_cross=0,
pending=0.
```

### 11.4 Dense mais profond

Construire une arête avec beaucoup de racines dont tous les candidats ont
profondeur au moins huit. Le scan complet lit tous les sites ; le LBVH doit les
fermer par permanents et seuils avant les feuilles.

### 11.5 Dense réellement shallow

Construire une famille avec beaucoup de groupes réellement shallow. La source
préflight, streame ou rend `resource_exhausted`; elle ne prétend pas supprimer
une vraie sortie dense.

### 11.6 Plateaux

`cube8`, positions dupliquées et grands shells :

```text
égalité non tronquée,
PointId distincts conservés,
aucun support D=0,
owner stable sous permutation de stockage.
```

---

## 12. Ordre d’implémentation à Claude

### P0

Fusionner dans le gateway actuel :

```text
W4 ledger + AcuteOwnerGateway.
```

Mesurer la conjonction, pas les triangles aigus globaux.

### P1

Créer :

```text
prototype/q4axis_lbvh_bounds.hpp
prototype/q4axis_lbvh_bounds_probe.cpp
```

et recevoir :

```text
A_lo/A_hi,
B_lo/B_hi,
permanent core,
prune F_* contre seuil,
égalité.
```

Le prune positif aux bouts de Jung est absent du v0.

### P2

Créer `q4seed_axis_lbvh_probe.cpp`, puis comparer bit à bit au scan actuel.

### P3

Construire le cover partagé par arête et remplacer le CSR de sites par des
`NodeHandle`.

### P4

Porter le traversal et le top-k en CTA par arête, avec continuations. Garder le
kernel actuel comme autorité device différentielle.

### P5

Brancher `EdgeCenterShallowCut` uniquement si les mesures montrent que
`c_e*m_e` reste trop élevé.

---

## 13. Décision

La réponse à « utiliser l’octree pour trouver le quatrième point ? » est :

> **Oui, absolument, mais en recherchant les racines extrémales de la puissance
> affine, pas un quatrième voisin euclidien.**

La meilleure route actuelle est :

```text
jointure factorisée W4-vivant ∧ carrier aigu
 -> arêtes actives tardivement micro-développées
 -> cover Morton-LBVH partagé par arête
 -> top-k axial LBVH par seed
 -> fallback arrangement 2D seulement pour les arêtes localement denses.
```

Elle est exacte, GPU-friendly et compatible avec les preuves existantes. Elle
retire le scan de tous les sites que le code identifie déjà comme le terme
dominant, sans reformer le produit `carrier × apex` que `Q4SeedAxisTopR4` avait
précisément éliminé.