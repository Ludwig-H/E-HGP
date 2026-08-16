# Note de l’auditeur — utiliser le LBVH pour la source sparse q3/q4

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
> avec AABB exactes et plages contiguës de `PointId`. C’est même préférable sur
> GPU.
>
> Elle doit être utilisée pour trouver le quatrième sommet q4, mais pas comme
> une simple requête sphérique répétée pour chaque seed. La bonne primitive est
> une **sélection axiale top-k par branch-and-bound sur les nœuds du LBVH**,
> partagée entre les seeds d’une même arête owner.
>
> Avant cela, le dernier commit de Claude impose une correction architecturale :
> le gateway aigu seul énumère un objet cubique. Il faut fusionner au même niveau
> de blocs :
>
> ```text
> ancre possiblement W4-vivante
> ET
> existence d’un carrier aigu owner.
> ```
>
> Sans cette fusion, le LBVH accélérerait la recherche du quatrième point pour
> une masse de seeds qui n’aurait jamais dû être produite.

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

La contre-mesure est néanmoins tout aussi importante : sans filtre de vivacité
de l’ancre, la masse des carriers aigus croît comme `n³`. À `uniform,n=800`,
elle représente déjà environ 46 % de tous les triples. Le gateway n’est pas
faux ; il énumère exactement le mauvais sur-objet.

Conclusion reçue :

> **Ne jamais lancer la source axiale q4 depuis tous les triangles aigus.**

---

## 2. L’« octree » déjà présent

`MortonLbvh` fournit :

```text
MortonKey 48 bits,
ordre canonique (MortonKey,PointId),
AABB u16 exacte par nœud,
plage contiguë [begin,end),
enfants left/right.
```

C’est une hiérarchie spatiale octree-like. Pour le device, le LBVH linéaire est
préférable à un octree classique à pointeurs :

- tableaux SoA contigus ;
- construction par radix/Karras possible ;
- parcours stackless ou à petite pile ;
- cohérence Morton ;
- subdivision binaire sans listes d’enfants irrégulières.

La classe CPU actuelle calcule encore les AABB en rescannant les plages durant
la récursion. Ce n’est pas la construction produit. Le port GPU devra employer :

```text
radix sort (MortonKey,PointId)
 -> topologie Karras
 -> réduction bottom-up des AABB.
```

Il n’est pas nécessaire de créer un deuxième index spatial.

Un overlay `BVH8` peut ensuite regrouper trois niveaux binaires pour réduire la
profondeur et tester huit enfants par ballot de warp. C’est une optimisation,
jamais une nouvelle autorité géométrique.

---

## 3. P0 : fusionner W4-vivacité et carrier aigu au niveau bloc

### 3.1 Pourquoi la conjonction doit rester corrélée

Pour un bloc de paires `A×B`, deux faits séparés ne suffisent pas :

```text
il existe une paire W4-vivante dans A×B,
il existe une paire avec carrier aigu dans A×B.
```

Ils peuvent être réalisés par deux paires différentes. La source doit raffiner
jusqu’à prouver ou rejouer la conjonction pour les mêmes paires.

### 3.2 État du ledger pair-level

Chaque tâche `A×B` doit porter pour la lane q4 :

```text
L4_open  = nombre d’IDs universellement W4-intérieurs,
U4_closed = cardinalité de l’union fixe des IDs encore possiblement W4,
credit_spans disjoints,
none_spans,
frontier MIXED,
continuation.
```

Avec le seuil de mort `r4=8` :

```text
L4_open >= 8  -> toutes les paires du bloc sont mortes ;
U4_closed <= 7 -> toutes les paires du bloc sont W4-vivantes ;
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
  et le facteur C contient au moins un vrai PointId relationnellement admissible ;

MIXED
  sinon.
```

Un `ACTIVE_ALL(A,B,C)` ne matérialise ni les `PairId`, ni les faces. Il produit
un `ActiveOwnerEdgeBlock(A,B)` avec un handle de preuve carrier. Les tâches
`MIXED` peuvent scinder `A`, `B`, `C` ou la frontier witness W4. Fixer toujours
`C` reproduirait l’ablation déjà réfutée.

### 3.4 Politique de split GPU-compatible

Éviter une optimisation continue compliquée. Tester virtuellement un niveau de
split pour les facteurs admissibles, puis choisir celui qui maximise :

```text
masse immédiatement classée DEAD ou ACTIVE
------------------------------------------------
nombre de tâches enfants + coût fixe des extrema
```

Cette décision ne manipule que quelques entiers et peut être prise par warp.
Le tie-break est déterministe : `W4-frontier`, puis `A`, `B`, `C`, ou tout ordre
fixé et versionné.

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

Sur `uniform/terrain/eight_clusters`, publier séparément :

```text
pair blocks physiques,
masse logique W4,
masse logique carrier,
masse logique de la conjonction,
exact edges micro-développées,
bytes et HWM.
```

---

## 4. Localiser tout ce qui peut compter pour une arête owner

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

Tout sommet, intérieur ou point de shell de sa sphère vérifie alors :

```text
||z-m||
 <= R+||c-m||
 <= (sqrt(3)+1)/sqrt(8) * sqrt(D)
 < sqrt(D).
```

Donc une seule requête LBVH exacte :

```text
||2z-a-b||² <= 4D
```

contient **tous** les sites pouvant affecter le q4 : apex, intérieurs et shell.

Pour être un sommet sous owner `e`, le point doit en plus appartenir à la
lentille :

```text
||z-a||² <= D,
||z-b||² <= D,
```

ce qui implique la borne plus forte :

```text
||2z-a-b||² <= 3D.
```

Chaque nœud du cover reçoit donc un masque :

```text
DEPTH_ONLY       : peut affecter le census, jamais être apex owner ;
APEX_POSSIBLE    : intersecte la lentille ;
APEX_ALL         : entièrement dans la lentille, hors ties non résolus ;
OUTSIDE          : rejeté.
```

Le cover doit être une antichaîne de nœuds LBVH, pas une liste de tous les IDs.
Il est partagé par tous les seeds de la même arête.

---

## 5. Le quatrième point n’est pas un voisin métrique : c’est une racine extrémale

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

La racine est :

```text
rho_z = A_z/B_z,
```

lorsque `B_z!=0`. Le théorème top-r4 demande uniquement :

```text
les k premières racines B>0,
les k dernières racines B<0,
k=8-p,
```

avec tous les IDs à égalité.

Ainsi, une requête « voisins les plus proches de x » serait sans rapport avec
l’ordre utile. Le LBVH doit faire du **branch-and-bound sur la racine**, avec la
métrique spatiale seulement comme premier prune.

---

## 6. Bornes exactes par nœud LBVH

### 6.1 Bornes de `B_z`

`B_z=n·(z-a)` est affine. Sur une AABB :

```text
B_lo, B_hi
```

sont exacts en choisissant, axe par axe, l’extrémité dictée par le signe de
`n_i`.

Fates :

```text
B_lo>0  -> nœud entrant homogène ;
B_hi<0  -> nœud sortant homogène ;
B_lo=B_hi=0 -> constant ;
sinon -> split nécessaire.
```

### 6.2 Bornes de `A_z`

`A_z` est séparable :

```text
A_z = sum_i [G s_i²-W_i s_i],
s_i=z_i-a_i.
```

`G>0`. Par axe :

- le maximum est à une extrémité ;
- le minimum entier est à l’un des deux entiers voisins de `W_i/(2G)`, clippés à
  l’intervalle.

On obtient des bornes `A_lo,A_hi` exactes sur le réseau u16 de l’AABB. Les
produits sont formés après promotion ; `i128` suffit pour `A`, tandis que les
comparaisons de racines conservent `BigInt<4>`.

### 6.3 Cœur permanent rapide

Avant toute recherche de racines :

1. appliquer `SeedCoreQuarter` ;
2. si nécessaire, `SeedJungPermanent16`.

Pour le premier :

```text
v(z)=2G(z-a)-W,
4 max_Z ||v(z)||² <= D G²
```

certifie tout le nœud permanent. Les trois IDs du seed sont masqués et les
spans crédités forment une antichaîne. Dès huit IDs distincts :

```text
DEAD_PERMANENT,
root comparisons = 0.
```

### 6.4 Prune exact contre le seuil courant

Supposons un nœud entrant `B_z>0` et une racine seuil :

```text
rho_* = A_*/B_*,  B_*>0.
```

Pour prouver que toutes les racines du nœud sont pires ou égales :

```text
A_z/B_z >= A_*/B_*
<=>
F_*(z)=B_* A_z-A_* B_z >= 0.
```

`F_*` est encore une somme de quadratiques convexes séparables. Son minimum
entier exact sur l’AABB se calcule comme pour `A_z`.

Donc :

```text
min F_*>0  -> nœud strictement pire, prune ;
min F_*=0  -> peut contenir le groupe d’égalité, descendre ;
min F_*<0  -> peut améliorer le top-k, descendre.
```

Pour `B_z<0`, réfléchir le paramètre :

```text
eta=-rho=A_z/(-B_z).
```

La recherche des dernières racines en `rho` devient la même recherche des
premières racines en `eta`.

Cette comparaison au seuil est nettement plus forte qu’un simple intervalle
`[A_lo,A_hi]/[B_lo,B_hi]`, car elle conserve la corrélation de `A_z` et `B_z`.
L’intervalle indépendant reste utile comme clé grossière de priorité.

### 6.5 Nœuds sans racine dans Jung

Un nœud peut aussi être pruné si toutes ses puissances sont strictement
positives aux deux bouts de `J_f` : l’affinité en `tau` les rend alors positives
partout. Un nœud entièrement négatif aux deux bouts est permanent.

Pour un endpoint fixé, la puissance en `z` est une quadratique convexe
séparable avec coefficient algébrique. La version robuste déjà proposée est :

```text
8 coins de l’AABB × 2 bouts de Jung,
```

avec `sgn_A_moins_Ytau`. Elle coûte seize signes exacts ; elle doit rester après
les prunes spatiaux et le cœur rationnel, pas au sommet de chaque traversal.

---

## 7. Algorithme `Q4SeedAxisTopR4-LBVH`

### 7.1 Première passe : seuils

Pour chaque seed :

```text
p = permanents bulk,
k = 8-p.
```

Si `k<=0`, mourir. Sinon lancer deux recherches best-first sur le cover LBVH de
l’arête :

```text
entrant : B_lo>0, minimum de rho ;
sortant : B_hi<0, minimum de eta=-rho.
```

Chaque recherche maintient :

```text
frontier de nœuds,
top-k fixe de racines,
seuil courant,
compteur de nœuds et continuation.
```

Les feuilles évaluent `SitePower` exactement. Une fois `k` racines trouvées,
le prune `F_*` élimine les nœuds strictement pires.

### 7.2 Deuxième passe : groupes d’égalité

Rejouer le cover avec le seuil reçu :

```text
racine meilleure ou égale au seuil -> conserver,
racine strictement pire -> prune,
```

et émettre tous les vrais `PointId` du groupe frontière. Une égalité peut avoir
plus de `k` IDs ; la capacité de sortie est préflightée ou devient une
continuation, jamais une troncature.

### 7.3 Replay final

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

Le LBVH ne décide jamais la positivité q4 à la place de l’autorité Gram-Cramer.

---

## 8. Ordonnance GPU

### 8.1 Ne pas garder « un thread par seed »

Le kernel actuel affecte un thread à chaque seed. Le noyau appelle ensuite
`select_axis_topr4` qui balaie plusieurs fois le CSR de tous ses sites. Le code
lui-même identifie ce balayage comme le mur de temps.

Pour le LBVH, l’unité correcte est :

```text
un CTA par arête owner ou microtuile d’arêtes,
plusieurs warps pour les seeds de cette arête.
```

Le cover spatial de l’arête est construit une fois et réutilisé.

### 8.2 Disposition suggérée

```text
warp 0 : traversal LBVH spatial et production des NodeHandle du cover ;
warps 1..w : seeds x de l’arête, top-k axial ;
shared memory : petite frontier, cover courant, top-k et seuils ;
global queue : spills/continuations seulement.
```

Deux variantes doivent être mesurées :

1. **warp par seed** : faible divergence dans les comparaisons BigInt ;
2. **warp par nœud, lanes=seeds** : un nœud chargé une fois, plusieurs axes
   évalués en parallèle.

La seconde est souvent meilleure lorsque une arête possède de nombreux
carriers ; la première lorsque les covers divergent vite.

### 8.3 Capacités fixes et continuations

Utiliser :

```text
top-k de taille <=8 par côté,
frontier locale 32 ou 64 nœuds,
égalité locale bornée,
spill explicite vers une queue globale.
```

Un overflow local rend `PENDING_RESOURCE` avec continuation. Il ne change aucun
verdict mathématique.

### 8.4 SoA et coalescence

Stocker séparément :

```text
node_lo_x/y/z,
node_hi_x/y/z,
node_begin/end,
node_left/right,
point_x/y/z,
PointId.
```

Trier les jobs par :

```text
niveau Morton de l’arête,
classe de diamètre,
RectId,
```

pour que des CTA voisins interrogent des régions voisines.

### 8.5 `count -> scan -> fill`

Première vague :

```text
compter seeds, continuations, groupes racines et BallRuns.
```

Puis preflight, prefix-scan et fill atomique. Aucun `cudaMalloc` par lot et
aucun tableau CSR de tous les sites par seed.

---

## 9. Partage par arête et backend hybride

L’axe par seed reste excellent lorsque le nombre local de carriers `c_e` et le
cover `m_e` sont petits. Il répète néanmoins une partie du travail pour tous les
seeds de la même arête.

Employer deux backends sous un contrat unique :

```text
si c_e * m_e <= budget_axis :
    Q4SeedAxisTopR4-LBVH ;

sinon :
    EdgeCenterShallowCut sur l’arrangement 2D de l’arête.
```

Le second backend construit collectivement les premiers niveaux de l’arrangement
de droites dans le plan médiateur. Le LBVH demeure utile : il fournit les
`candidate_line` et `witness_only` par nœuds et alimente les cellules de centres.

Le switch se choisit par mesure :

```text
node visits,
BigInt comparisons,
conflict HWM,
root groups,
wall time.
```

Il ne doit jamais changer le résultat. Les deux backends sont comparés sur
`SupportKey`, `BallKey`, owner, primary, positivité, `I_B/U_B` et fates.

---

## 10. Lane q3

La même infrastructure sert q3 sans faire de Lane3 une source pour Lane4.

Pour une arête q3 active :

1. le cover LBVH de la lentille produit les carriers `x` par blocs ;
2. chaque `x` définit une unique circumsphère q3 ;
3. un parcours AABB-sphère exact compte les intérieurs avec arrêt au neuvième ;
4. le shell est reporté complètement lorsque le support survit.

Pour petit `c_e`, cette requête par carrier est simple et GPU-friendly. Pour un
`c_e` élevé, la vue q3 de l’arrangement de centres traite collectivement les
pieds des droites dans l’ellipse `||t||²<=D/12`.

La géométrie neutre peut être partagée :

```text
LBVH cover,
base du plan médiateur,
coefficient de droite,
classification des nœuds.
```

Les ledgers et seuils restent séparés :

```text
q3 : mort à 9 intérieurs,
q4 : mort à 8 intérieurs.
```

---

## 11. Gates demandées

### 11.1 Parité axiale

Sur chaque fixture existante :

```text
select_axis_topr4 scan complet
==
select_axis_topr4_lbvh
```

champ par champ : verdict, permanents, `k`, entrants, sortants, groupes égaux,
profondeur min et census.

### 11.2 Prune de seuil causal

Mutants :

```text
axis-node-utilise-Alo/Bhi-sans-signe,
axis-node-oublie-correlation-F,
axis-node-prune-egalite,
axis-node-melange-B-positif-negatif,
axis-node-oublie-depth-only,
axis-node-core-parent-enfant.
```

### 11.3 `two_lines`

La famille doit mourir au front conjoint W4/carrier :

```text
axis_jobs=0,
LBVH_axis_node_visits=0,
PairId_cross=0,
pending=0.
```

### 11.4 Dense mais profond

Construire une arête avec beaucoup de racines dont tous les candidats q4 ont
profondeur au moins huit. Le scan complet lit tous les sites ; le LBVH doit les
fermer par permanents, seuils ou nœuds sans racine avant les feuilles.

### 11.5 Dense et réellement shallow

Construire une famille avec beaucoup de groupes réellement shallow. La source
doit préflight, streamer ou rendre `resource_exhausted`; elle ne prétend pas
faire disparaître une vraie sortie dense.

### 11.6 Plateaux

`cube8`, positions dupliquées et grands shells :

```text
égalité non tronquée,
PointId distincts conservés,
aucun support D=0,
owner canonique stable sous permutation de stockage.
```

---

## 12. Ordre d’implémentation recommandé à Claude

### P0 — immédiatement

Modifier le gateway actuel pour porter et raffiner simultanément :

```text
W4 ledger + AcuteOwnerGateway.
```

La campagne doit mesurer la masse de la **conjonction**, pas la masse des
triangles aigus globaux.

### P1 — bornes LBVH autonomes

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

### P2 — remplacement du scan complet

Créer :

```text
q4seed_axis_lbvh_probe.cpp
```

sur une arête/seed explicite, puis comparer bit à bit au noyau actuel.

### P3 — cover partagé par arête

Remplacer le CSR `site_id` par :

```text
edge_cover_offset,
edge_cover_node,
seed_to_edge.
```

Le point n’est développé qu’à la feuille terminale.

### P4 — kernel CTA par arête

Porter le traversal et le top-k avec continuations. Conserver le kernel actuel
comme autorité device différentielle.

### P5 — backend collectif seulement si nécessaire

Si les mesures montrent `c_e*m_e` trop élevé, brancher
`EdgeCenterShallowCut`. Ne pas commencer par cette brique plus complexe alors
que le LBVH possède déjà exactement les prunes spatiaux et axiaux nécessaires.

---

## 13. Décision

La réponse à la question « utiliser l’octree pour trouver le quatrième point ? »
est :

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

Elle est exacte, output-sensitive dans les régimes favorables, compatible avec
les preuves existantes, et bien plus GPU-friendly qu’un nouvel octree à
pointeurs ou qu’un arrangement 4D global. Elle retire le scan de tous les sites
que le code identifie déjà comme le terme dominant, sans reformer le produit
`carrier × apex` que `Q4SeedAxisTopR4` avait précisément éliminé.