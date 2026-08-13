# Réponse à la fourche de Claude : source factorisée et `Central-VWave`

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Objet de la question pincé au début de l'audit :
`HEAD=dba89617ae1e494232b7b9af7698545b9769f517`. Son successeur observé est
`af08b0e191065228fef591e301fae3c69aa74ac6`, qui reçoit le compteur de masse
q2 et l'échantillonnage pondéré du résiduel. Le worktree live a ensuite ajouté
un premier `Central-VWave`; lors du relevé ses empreintes étaient
`rect_front.hpp=f4c3be616d79...` et
`wspd_wavefront_probe.cpp=6f6aa5a8942d...`. Ce delta logiciel n'est pas reçu
par le pin. L'auditeur n'a modifié aucun logiciel et n'a utilisé aucune
ressource GCP.

## Verdict exécutable

La source industrielle n'est **ni par paire, ni à coût fixe par record**. Elle
doit rester factorisée et sortie-sensible. La masse `PairId` est un ledger
sémantique obligatoire ; la développer est précisément la mosaïque globale que
la v3 interdit de reconstruire.

La fourche publiée au pin `dba8961` était en outre fondée sur une colonne mal
nommée : les valeurs `97,4/91,2/79,0/50,1/33,3 %` étaient exactement
`100 % - taux de records q2 fermés`, pas des masses de paires. Le delta live
calcule désormais une vraie masse q2 fermée. C'est une correction nécessaire,
mais elle ne transforme toujours pas cette masse en temps aval.

La décision immédiate remise à Claude est donc :

1. ne pas porter la descente best-first actuelle sur CUDA ;
2. ne pas choisir globalement `s=4` à partir de la masse ;
3. remplacer fenêtre et heap par une wavefront exacte du **score du certificat
   central**, `Central-VWave` ;
4. mesurer `s=1`, `3/2` et `2` sur le chemin factorisé complet ; garder `s=2`
   comme contrôle reçu et `s=1` comme candidat de coût, sans promotion avant
   les lanes q3/q4 et l'aval ;
5. mesurer ensuite `ProjectiveWindowCounter-v0` : les suffixes de crédits
   projectifs définissent une fenêtre complète de co-sommets par ancre ; ne
   construire le shallow local que si cette fenêtre passe ses pentes ;
6. garder le join `QueryTree × PointTree` et les relations carriers comme
   ablations/tombstones, jamais comme développement des PairIds.

## 1. Ce que coûte réellement une source v3

Écrire `R` pour les records résiduels et `M` pour leur masse en PairIds ne
suffit pas à modéliser le coût. Le contrat à recevoir a la forme physique

$$W_{\mathrm{source}}\leq c_R R+c_J J+c_B B+c_P P+c_H H_{\mathrm{out}},$$

où `J` compte les couples de nœuds réellement classifiés, `B` les blocs
factorisés émis, `P` les hits ponctuels inévitables et `H_out` les incidences ou
supports recertifiés. Aucun coefficient ne multiplie `M`.

`M` reste néanmoins bloquant dans trois rôles :

- identité `closed_pair_mass + residual_pair_mass = C(n,2)` ;
- pondération scientifique des familles et détection d'une perte de domaine ;
- risque à surveiller tant qu'aucune borne transitive sur `J/B/P/H_out` n'est
  reçue.

Un algorithme qui visite chaque paire du résiduel coûte au moins `M` et est
hors architecture, même si le front qui le décrit est linéaire. À l'inverse,
un record n'a pas un coût constant : le join d'un bloc difficile avec l'arbre
témoin peut produire beaucoup de tâches ou une sortie réellement dense. Le
choix de `s` minimise donc le vecteur mesuré

```text
(p95_total, F, R_q2, R_q3, R_q4, J, B, P, H_out,
 bytes_read, bytes_written, HWM, rounds, kernels, syncs)
```

et non une colonne isolée de masse ou de pourcentage fermé.

Une expansion tardive reste autorisée après une nouvelle réduction certifiée :
si les crédits projectifs produisent une fenêtre dirigée `N_q(a)` dont
`sum_a |N_q(a)|` passe les pentes et l'enveloppe mémoire, cette quantité devient
le `PlaneTape` physique du shallow local. Chaque incidence y est payée une fois.
Ce n'est pas le développement aveugle de la masse WSPD ; une fenêtre dense
refuse la route avant son allocation.

Les mesures déjà disponibles éliminent `s=4` comme baseline : sur la vague
courante à `n=8000`, `s=2` donne environ `408429` terminaux et des résiduels
q2/q3/q4 de `40,32/50,80/50,91` records par point ; `s=4` donne environ
`46,72/119,62/127,31` résiduels par point, avec `2,75` fois plus de front.
`s=2` domine donc `s=4` dans les trois lanes et dans les compteurs physiques
observés. Le tableau q2 du pin rend `s=1` prometteur, mais il ne mesure ni q3,
ni q4, ni `J`, ni l'aval : `s=1` est une ablation prioritaire, pas une décision
reçue.

## 2. Contre-audit du correctif de masse live

Le delta non commité ajoute `mass_closed_q2` et, pour chaque terminal fermé,
additionne `|A| |B|`. Sous la porte déjà existante qui prouve que les
terminaux partitionnent chaque PairId non ordonné avec multiplicité un, cette
somme est la vraie masse q2 fermée. Le type `long long` couvre les tailles CLI
actuelles. Il manque encore :

- masses fermées q3 et q4, avec sémantique `PRUNED_OWNER_SHARD` ;
- masse résiduelle sérialisée par lane et digest de partition ;
- invariants `closed_mass + residual_mass = C(n,2)` par lane ;
- compteurs du consommateur de ces records.

Le nouvel échantillonnage choisit un rectangle proportionnellement à
`|A||B|`, puis une paire uniforme dans `A×B`. Il estime donc correctement, à
la précision Monte-Carlo près, la fraction des **PairIds q2 résiduels** dont la
boule diamétrale contient déjà au moins dix points. Il ne mesure toujours pas
des témoins universels du rectangle et ne reçoit aucune conclusion sur le coût
de la source. Le libellé exact est par exemple
`pairwise_q2_dead_fraction_in_residual_mass`, pas « rappel de la banque » ni
« supports produits ».

Pour localiser une perte de rappel, quatre étages distincts sont requis sur un
petit oracle :

1. vérité pairwise pour chaque PairId développé ;
2. points universels pour les populations réelles de `A×B` ;
3. points éligibles par le certificat AABB employé ;
4. points effectivement proposés et recertifiés.

Les pertes `1→2`, `2→3` et `3→4` sont respectivement la coalescence du
rectangle, la relaxation AABB et la proposition. Le pin comparait l'étage 1
d'une seule paire à l'étage 4, ce qui ne pouvait identifier la cause.

## 3. La loi d'inflation ne décide ni `s`, ni le rappel

Le rapport de volumes entre une boule individuelle et un cœur commun ne donne
aucune relation déterministe entre leurs populations. Sans hypothèse de
densité, tous les points peuvent être dans le croissant extérieur, ou tous
dans le cœur. Il n'existe donc pas de loi exacte `K lambda(s)`, ni de loi
`s(K)`, pour les nuages u16 arbitraires.

Même comme inversion du modèle volumique, les deux valeurs publiées étaient
incorrectes. Avec `lambda=1+j/K`, `r=lambda^(1/3)`,
`u=(r-1)/(2r+1)` et `s=2/u-2`, la marge `j=2` donne environ `79,70` pour
`K=8` et `97,76` pour `K=10`, non `54/65`. Cela reste une espérance de modèle,
jamais un certificat discret.

Le delta live rend l'échantillon PairId mieux défini ; il ne réhabilite ni la
loi volumique, ni le claim « la discontinuité Morton est le seul goulet ».

## 4. Remplacement exact : `Central-VWave`

### 4.1 Score du certificat

Pour un terminal `R=A×B`, poser `D=Dlo(A,B)`. Pour un PointId `z`, définir

$$S_R(z)=\sum_{i=1}^{3}\max\left(\left|2z_i-A_i^{lo}-B_i^{lo}\right|,\left|2z_i-A_i^{hi}-B_i^{hi}\right|\right)^2.$$

`S_R(z)` est exactement `Vhi(A,B,{z})`. Le masque central reçu est alors :

```text
q2 : S < D
q3 : 3 S < D
q4 : D > 0 et 209 S <= 56 D
```

Ces tests sont suffisants pour les prédicats géométriques q2/q3/q4 sur tout
`A×B`, mais la sélection de tous les PointIds qui satisfont **ce certificat**
peut être complète.

### 4.2 Bornes exactes sur un nœud témoin

Pour un axe, poser `u=Alo+Blo`, `v=Ahi+Bhi` et
`f(z)=max(|2z-u|,|2z-v|)^2`. Sur l'intervalle entier d'un nœud `C` :

- le maximum de `f` est atteint à une extrémité de `C` ;
- le minimum est atteint à l'un des entiers voisins de `(u+v)/4`, écrêtés à
  `C`.

Les axes étant indépendants, sommer ces trois minima et maxima donne
`Smin(R,C)` et `Smax(R,C)` exacts sur l'AABB entière de `C`. Les points réels
du nœud étant un sous-ensemble de cette boîte, le classifieur suivant est sûr.
Dans l'ABI, `NONE` doit s'appeler `CENTRAL_DEAD` : il élimine seulement le bit
de ce certificat et ne prouve jamais l'absence d'un témoin géométrique :

```text
lane q2 : ALL si Smax < D;        CENTRAL_DEAD si Smin >= D
lane q3 : ALL si 3*Smax < D;      CENTRAL_DEAD si 3*Smin >= D
lane q4 : ALL si D>0 et 209*Smax <= 56*D;
          CENTRAL_DEAD si D==0 ou 209*Smin > 56*D
```

Les produits tiennent sous `2^44` au profil u16 ; cette tranche exige
`wide_products=0`.

### 4.3 Wavefront et invariants

Le travail initial est une suite de tâches
`(RectOrdinal,CNodeOrdinal,open_mask)`. À chaque ronde :

- `ALL` crédite la population du nœud, saturée à `10/9/8`, et retire le bit ;
- `CENTRAL_DEAD` retire seulement ce bit du certificat central ;
- `MIXED` remplace le nœud par ses enfants ;
- un cap sérialise exactement la tâche comme `DELEGATED_RESIDUAL` ;
- aucune tâche ne repart silencieusement de `C=root`.

Les nœuds crédités forment une antichaîne disjointe. Un nœud `ALL` ne peut pas
contenir un endpoint de `A` ou `B`, car choisir `z=a` ou `z=b` force l'échec du
bit q2, donc des bits plus étroits. L'ABI conserve néanmoins jusqu'à dix
`proof_ids` canoniques et le replay vérifie identité, disjonction, seuil et
masque.

Le premier worktree ne respecte pas encore cet invariant : il stocke seulement
un `CNode` dans sa pile et conserve un masque `open` global. Si q2 est `ALL`
sur un parent sans saturer dix crédits, tandis que q3 est `MIXED`, les enfants
sont poussés puis leur population est créditée une seconde fois en q2. Cela
peut fermer à tort. La tâche doit être `(CNode,lane_mask)` et **seuls** les bits
`MIXED` du parent sont transmis aux enfants. Les bits `ALL/NONE` sont consommés
exactement une fois au parent. De même, une pile pleine ou un quantum épuisé
doit sérialiser une continuation ; le test `sn+2<=capacity` ne peut jamais
abandonner les enfants.

Cette vague supprime la fenêtre Morton, le top-`L`, le heap par rectangle et
leur rappel indémontré. Elle est exacte pour l'ensemble des crédits du masque
central. Elle ne prouve pas encore un coût linéaire : si `J` désigne les tâches
`Rect×CNode` consommées, le pire cas reste `Theta(Fn)`. `J`, les octets et HWM
sont donc des portes bloquantes, pas des détails de profilage.

### 4.3.1 Réception partielle au pin `7b58fc3`

Le pin `7b58fc3530bfa0e6907ddc562135f94889dbbccb` porte désormais le masque dans
chaque tâche et tue le mutant `masque-global` avec le code contractuel `4`.
Cette correction rend la comptabilité par population sûre : pour une lane
donnée, les nœuds `ALL` consommés forment une antichaîne disjointe. Elle ne
reçoit pas encore le ledger d'identité.

Le juge live balaie tous les PointIds, mais réemploie le même masque central
singleton. Il vérifie qu'une fermeture possède assez de candidats centraux ;
il ne vérifie ni quels IDs ont été crédités, ni leur multiplicité, ni un
prédicat `H/E/X` indépendant. Son plancher est global et peut être rempli par
q2 lorsque q3/q4 sont vacants. La porte suivante doit donc publier et rejouer,
par lane, les unions d'IDs des nœuds crédités.

Deux fixtures axiales isolent la faute. Pour `A={0}`, `B={10}` et un `CNode`
peuplé par `{1,...,6}`, le parent fournit six crédits q2 mais reste `MIXED` q3 ;
un masque global recrédite ses enfants et ferme faussement q2. Pour `A={0}`,
`B={30}`, `CNode={7,...,11}`, le même mécanisme double cinq crédits q3 pendant
que q4 descend. La frontière endpoint `z=a` ou `z=b`, où `S=D`, tue en plus le
mutant faible `<=` en q2/q3.

Le cap reste seulement un compteur. À `uniform,n=800,s=2,tight`, `window=2`
tronque les `24745` terminaux et ne ferme rien ; `window=64` en tronque `8440`
et ferme `1867/8/0` records ; `window=1024` ne tronque rien et ferme
`4297/72/38`. La sûreté fail-open est conservée, mais il n'existe aucun payload
de reprise. L'ABI minimale suivante est bloquante :

```text
CentralTask      = RectId, CNodeKey, lane_mask
CentralCredit    = RectId, CNodeKey, closed_mask, population, proof_span
CentralPending   = RectId, CNodeKey, lane_mask
CentralRectResult= RectId, closed_mask, residual_mask, credit_span, pending_span
```

Les tableaux sont SoA, préalloués et produits par `count--scan--fill`. La gate
exige `tasks_created=tasks_consumed+tasks_pending`, multiplicité d'ID `0/1` par
lane, `credit_count=|union(proof_ids)|`, planchers par lane et digest invariant
après consommation complète de `CentralPending`. Tant que ce contrat n'existe
pas, optimiser la proportion de `MIXED` dans la DFS scalaire ne rapproche pas
le chemin warm GPU.

### 4.3.2 Contre-audit de `locate-and-climb` au pin `75f16db`

Préconstruire `leaf_parent` en `O(n)` est correct et supprime la recherche
linéaire qui existait dans le premier delta concurrent. Mais les frères d'une
feuille à la racine partitionnent seulement `X\{z_0}`. Le pin empile tous ces
frères sans jamais empiler `z_0`. À `uniform,n=8000,s=2`, quantum non saturé,
les classifications passent de `30422095` à `27645737`, mais les fermetures
q2/q3/q4 passent simultanément de `104237/2455/1288` à
`97822/1989/1010`. Les `9 %` ne sont donc pas une économie à résultat constant.

L'ordre est également renversé : les frères sont ajoutés du voisinage de la
feuille vers la racine, puis dépilés en LIFO, donc le plus grossier passe le
premier. L'ordonnance exacte proche-d'abord est : empiler les frères dans
l'ordre racine-vers-feuille, puis empiler la feuille `z_0`. Le premier pop est
alors `z_0`, suivi du frère immédiat et des frères de plus en plus grossiers.

Trois gates sont nécessaires avant de comparer le nombre de tâches :

1. pour chaque feuille petit `n`, développer les racines initiales et exiger
   tous les PointIds exactement une fois ;
2. sans cap, exiger identité bit à bit de l'union créditée et du `closed_mask`
   entre `C=root` et `climb` ;
3. placer la feuille localisée comme dixième, neuvième et huitième crédit des
   lanes q2/q3/q4, afin que son omission tue le test.

Le pin ne contient aucun CTest `--climb`. Son juge de sûreté peut passer malgré
une perte de complétude, puisqu'il demande seulement que toute fermeture
annoncée possède assez de candidats. Il est donc prématuré d'inférer de cette
ablation que le facteur manquant ne peut venir que de `QueryTree×PointTree`.
Enfin, le champ `parent` ajouté après quatre AABB aligne `WfNode` à `128`
octets, contre `120` auparavant. Le placer dans le trou après `level` conserve
la taille ; `leaf_parent` ajoute encore environ `4n` octets. Le coût reste
faible à `50k`, mais représente environ `120 MB` supplémentaires à dix
millions de nœuds et appartient au ledger HWM.

### 4.4 Factoriser aussi les requêtes — ablation après la fenêtre projective

Si les `F` graines `Rect×Croot` restent trop coûteuses, un join global
`QueryTree(RectId) × PointTree(PointId)` est une ablation possible :

- un radix fixe ordonne les RectIds par `(A-path,B-path,RectOrdinal)` ;
- une seule graine `(Qroot,Croot)` est lancée ;
- `NONE` élague, `ALL` émet un bloc factorisé `(QSpan,CNode)` et `MIXED` scinde
  `Q` ou `C` avec tie-break canonique ;
- une feuille `Q` traite un petit warp de RectIds contre le même `CNode` ;
- le coût honnête est `O(R+n+J+B+P)`, sans cacher que `J` peut rester
  quadratique.

Ce join n'est pas encore la source recommandée : avant de l'implémenter, la
porte `ProjectiveWindowCounter-v0` décrite dans
[`AUDIT_REPONSE_FOURCHE_SOURCE_AF08B0E_20260813.md`](AUDIT_REPONSE_FOURCHE_SOURCE_AF08B0E_20260813.md)
doit mesurer la fenêtre certifiée `N_q(a)`. Les agrégats d'un `QNode` ne
décident que lorsqu'ils donnent une borne universelle sûre ; une boîte trop
lâche rend `MIXED`. L'oracle petit `n` développe tous les blocs et exige chaque
couple `(PairId,PointId)` exactement une fois.

## 5. Relation carrier : classer les marges, pas trois distances séparées

Pour `D=||b-a||^2`, `E=||x-a||^2` et `X=||b-x||^2`, poser
`M0=E+X-D=-2H`, `M1=D-E` et `M2=D-X`. Un carrier vérifie exactement
`M0>0`, `M1>=0`, `M2>=0`.

Pour un axe et un entier `u`, écrire `near(u,I)` pour la distance carrée à
l'intervalle entier `I`, et `far(u,I)` pour la plus grande distance carrée à
ses deux extrémités. Les extrema exacts sont :

```text
M1min_axis = min sur a in {Alo,Ahi} de near(a,B)-far(a,C)
M1max_axis = max sur a in {Alo,Ahi} de far(a,B)-near(a,C)

M2min_axis = min sur b in {Blo,Bhi} de near(b,A)-far(b,C)
M2max_axis = max sur b in {Blo,Bhi} de far(b,A)-near(b,C)
```

La preuve est l'affinité de `M1` en `a` et de `M2` en `b`, puis la séparation
des variables restantes. Les extrema de `M0` sont `-2*Hmax` et `-2*Hmin`.
Après somme des trois axes :

```text
ALL exact : M0min>0 et M1min>=0 et M2min>=0
NONE sûr  : M0max<=0 ou M1max<0 ou M2max<0
```

`ALL` est complet sur le produit AABB. `NONE` reste incomplet, car deux
contraintes peuvent échouer sur des points différents. Fixture permanente :
`a=(0,0)`, `b=(0,1)`, `C={(1,0),(1,1)}` ; les deux points donnent
respectivement `(M0,M1,M2)=(2,0,-1)` et `(2,-1,0)`. Aucun n'est carrier, mais
aucune marge n'est impossible partout. Les valeurs tiennent en `i64`.

Le compteur historique `has` fusionne `ALL` et une feuille `MIXED`; il doit
s'appeler `POSSIBLE_OR_PRESENT`. La wave carrier est lancée si q3 **ou** q4
reste ouvert. Elle produit `P=AcuteLens` pour q3/q4 et `L=Lens` pour q4.

## 6. q4 sans produit `P×L`

La source q4 ne matérialise ni `C(n_lens,2)`, ni les produits cartésiens de
blocs. Pour une arête owner, injecter les formes affines associées aux points de
`L` dans le moteur mono-ancre shallow, avec un bit indiquant l'appartenance à
`P`. Seuls les sommets de profondeur au plus `7-credit4` incidents à au moins
une forme aiguë sont émis. Chaque sortie est ensuite recertifiée :

```text
distance xy, rang affine, positivité stricte,
owner max-edge canonique, BallKey, census et shell
```

La complexité reste sortie-sensible et charge explicitement concurrences,
bundles et dégénérescences à `H_out`. Un block-join `P×L` n'est qu'un oracle ou
fallback borné ; il n'est jamais le chemin produit.

## 7. Conditions manquantes à la preuve du WSPD par vagues

Les affirmations actuelles du header ne suffisent pas à invoquer la borne :

- un Patricia Morton u16 est borné par les `48` bits utiles, pas par
  `2 log2(n)` ; une famille de `49` clés à bits unitaires atteint une chaîne de
  hauteur `48` ;
- avec `n-1` graines, `T` terminaux et `I` expansions binaires, l'identité est
  exactement `tests=I+T=2T-(n-1)` ; elle ne prouve pas `T=O(n)` ;
- tronquer un préfixe partiel à `floor(bits/3)` donne la même cellule à des
  frères. Il faut conserver les `0/1/2` bits partiels : le nœud devient un pavé
  aligné d'aspect au plus deux, à intérieurs disjoints ;
- rétrécir une boîte déplace son centre et peut faire échouer un test
  centre/rayon qui passait sur la cellule. Pour hériter du tape cellulaire,
  choisir le split sur la cellule et arrêter sur `sep_cell || sep_tight`, puis
  gater le coarsening et la lineage ;
- trois tailles uniformes et un degré maximal observé ne constituent pas une
  preuve de packing. `O(s^3 n)` est une majoration, pas la loi de rapport huit
  lorsque `s` double.

Le contre-exemple de monotonie à graver pour `s=2` est : cellule
`A=[0,127]^3`, cellule `B=[256,383]^3`, puis boîte serrée
`A'=[0,127]×[126,127]×[126,127]`. Le test cellule passe, le test serré échoue.

## 8. Ordre d'implémentation et portes

### P0 — corriger la base WSPD

- cellule de préfixe partiel, split cellulaire et arrêt hybride ;
- identité entière `tests=2T-(n-1)` ;
- profondeur/pair-split bornées par le domaine Morton ;
- `FrontDigest` canonique, oracle PairId exact-once, bytes et HWM ;
- aucune conclusion de packing issue d'une seule rampe.

### P1 — `Central-VWave` CPU counter-only, puis device

- sorties `CLOSED_PAIR_SHARD`, `PRUNED_OWNER_SHARD` et
  `DELEGATED_RESIDUAL` typées ;
- preuve IDs, antichaîne et compactage stable ;
- `J`, ALL/NONE/MIXED, splits, rounds, octets/HWM et masse par fate ;
- corpus `s=1,3/2,2`, trois lanes et cinq familles ;
- petit oracle exhaustif contre le masque singleton direct ;
- invariance au tuilage, au quantum et au nombre de threads.

### P2 — fenêtre projective par ancre

- vrais `PointId` dans des `CreditKey` transactionnels et disjoints ;
- suffixes exacts, cellules incomplètes laissées ouvertes ;
- `AnchorWindow N_q(a)` par spans, avec oracle de complétude sur le plus petit
  `PointId` de chaque vrai support ;
- `sum_a |N_q(a)|`, plans logiques, octets/HWM et deux pentes ;
- `NO-GO` avant shallow/CUDA si cette fenêtre reste quasi quadratique.

### P3 — shallow local et census par `BallKey`

- arrangement inversé shallow local seulement sur `N_q(a)` ;
- points hors fenêtre conservés dans le census global ;
- RLE des centres par équation primitive de sphère ;
- census une fois par `BallKey` avant expansion des `SupportKey` ;
- DVT et join global seulement comme tombstones/ablations factorisées.

### Ablation — join global et carriers

- `join_root_seeds=1`, `scalar_rect_root_launches=0` ;
- `tasks_created=tasks_consumed`, aucun restart racine ;
- classifieur par marges, blocs factorisés et oracle `(PairId,PointId)` ;
- niveaux shallow q3/q4 avec sorties et dégénérescences comptées.

### Porte G4

Aucun script actuel ne qualifie cette route : il n'existe encore ni kernel
`Central-VWave`, ni ABI de résultat, ni p95 résident. Une session ne devient
utile qu'après les portes natives, un manifeste pincé et trente warms sur
`12500/25000/50000`. Les deux pentes `<=1,35` portent sur `J`, blocs, hits,
octets et HWM ; la masse reste un ledger. Le p95 final inclut WSPD, vague,
source, shallow, census, fold et copie du payload.

## 9. Contre-audit de la fenêtre projective proposée par l'autre auditeur

L'audit concurrent
[`AUDIT_REPONSE_FOURCHE_SOURCE_AF08B0E_20260813.md`](AUDIT_REPONSE_FOURCHE_SOURCE_AF08B0E_20260813.md)
apporte un candidat de source plus structurel que le join carrier générique.
Son lemme de groupes projectifs est correct sous ses hypothèses.

Fixer une ancre `a`, une cible `b`, `d=b-a` et des vecteurs
`s_i=z_i-a`. Si des coefficients non négatifs vérifient
`d=sum_i lambda_i s_i` et `d dot s_i>||s_i||^2`, alors, pour le centre
`t=c-a` de toute sphère passant par `a,b`, on a
`2 t dot d=||d||^2` et

$$\sum_i\lambda_i\left(2t\mathbin{\cdot}s_i-\left\Vert s_i\right\Vert^2\right)=\left\Vert d\right\Vert^2-\sum_i\lambda_i\left\Vert s_i\right\Vert^2>0.$$

Au moins un membre du groupe est donc strictement intérieur à toute telle
sphère. `h_q=smax+1-q` groupes dont les ensembles de PointIds sont deux à deux
disjoints donnent `h_q` intérieurs distincts et ferment la paire pour les
supports pertinents d'arité `q`.

Cela justifie conditionnellement la route :

```text
ProjectiveCreditBank
  -> AnchorWindow N_q(a) en NodeSpans
  -> arrangement shallow local et éphémère
  -> BallKey/RLE
  -> census global une fois par BallKey
  -> SupportKey, positivité, owner et fold
```

Avec `a=min(PointId du support)`, tout autre sommet d'un support pertinent doit
rester dans `N_q(a)` ; sinon la sphère du support serait déjà fermée par les
groupes. Enlever les autres points du **générateur** local ne perd donc pas ce
support : sa profondeur restreinte ne peut qu'être plus faible. Ces points
restent impérativement dans le census global, qui rejette les faux positifs.

Trois conditions manquent avant de recevoir cette route :

1. le lemme ci-dessus est ponctuel en `b`. Un `CreditKey` qui ferme tout un
   `BNode` doit authentifier la représentation conique et les strictes pour
   **chaque** cible du span, ou rester ponctuel ; extrapoler un crédit de
   `b` à sa cellule serait un faux prune ;
2. la construction de `ProjectiveCreditBank` n'a encore ni coût factorisé, ni
   cap consommable, ni preuve que les unions d'IDs restent disjointes après
   réplication des cellules ;
3. l'arrangement local peut rester dense. Le compteur
   `sum_a |N_q(a)|`, le nombre de formes, les événements shallow, les BallKeys,
   octets et HWM doivent passer les rampes avant tout kernel source.

Le choix provisoire `s=1` et « un split local » de l'autre audit est une
ablation raisonnable, pas une preuve. L'inégalité `2F_1<F_2` n'a été observée
que sur `uniform`; chaque famille et chaque lane doit être rejouée, et le split
local porte un budget global de records/tâches. De même, la proximité du ratio
de degrés observé avec `(s+2)^3` n'établit aucune constante de packing.

L'ordonnance commune devient donc :

1. WSPD corrigée ;
2. `Central-VWave`, pour fermer exactement tout ce que le certificat central
   sait fermer et produire un résiduel typé ;
3. `ProjectiveWindowCounter-v0`, premier falsificateur de la vraie source ;
4. si sa fenêtre est sparse, shallow local puis BallKey/census ;
5. sinon, `QueryTree×PointTree` et carrier-wave restent le fallback factorisé
   mesuré, jamais une expansion PairId.

## Réponse courte à Claude

> Ta source ne doit jamais payer une fois par paire. Elle ne coûte pas non plus
> une constante par rectangle : elle coûte les tâches du join factorisé et ses
> vraies sorties. Le tableau qui semblait favoriser `s=4` ne mesurait pas la
> masse ; ton correctif live répare ce compteur, mais la masse ne devient pas un
> temps. N'implémente pas le heap actuel. Fais d'abord une `Central-VWave` sur le
> score `Vhi`, complète pour ton certificat, puis mesure `s=1,3/2,2` jusqu'aux
> fenêtres projectives et au shallow. Le lemme projectif de l'autre audit est
> bon, mais il doit être universalisé aux spans et recevoir son compteur avant
> de devenir la source. À ce jour, `s=4` est dominé et ne doit pas être figé.
