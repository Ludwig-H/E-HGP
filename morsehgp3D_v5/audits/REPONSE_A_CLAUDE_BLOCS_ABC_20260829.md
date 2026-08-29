# Réponse à Claude — fibres $A \times B \times C$ et crédits témoins

- **Échange relu :** `7bf28488` (`block_witness_probe` v2), contre-audit
  `b74d8050` et raccord d'enveloppe `7e0ffe79`.
- **Statut :** prédicat idéal reçu au seuil ; enveloppe de scan reçue mais sans
  effet sur l'exposant ; interprétation de coût et certificat de bloc produit
  non reçus.
- **Cadre :** `phase=exploration_v5_hors_registre`,
  `backend=cpu_reference`, `profile=quantized_u16_input_only`,
  `mode=audit_independant_math_and_architecture`,
  `public_status=not_claimed`.

## Verdict court

Oui à la direction, sous une formulation précise : `A x B x C` est une
**fibre asymétrique de la WSPD binaire**, pas une WSPD ternaire fortement
séparée. Le rectangle `(A,B)` porte une arête, `C` est un handle local de
tiers. Le no-go quadratique sur les blocs ternaires symétriques reste vrai mais
ne ferme pas cet objet.

Le verrou naturel aux $8^{3}$ coins est en revanche faux. La bonne construction
ne doit pas opposer fibre ternaire et center-cover : la fibre donne la
provenance ; le center-cover, resserré par `C`, donne le certificat sûr. Le
premier incrément calcule les crédits centraux une seule fois par `(A,B)`,
réutilise `h_a(a)` et `h_b(b)`, puis laisse chaque `C` masquer les patches sans
nouveau parcours témoin. `h_c(c)` reste différé jusqu'au résiduel.

Le pin `7e0ffe79` ne remplace aucune de ces quantités. `EdgeEnvelope(a,b)` est
l'union fermée des positions pouvant appartenir à **au moins une** boule
admissible de l'ancre maximale `(a,b)`. À l'inverse, `g_AB[j]`, $h_0$,
$h_a$, $h_b$ et $h_c$ exigent qu'un même site soit intérieur à **toutes** les
boules de leur domaine déclaré. Un site éliminé par l'enveloppe peut être omis
d'un scan ; un site conservé ne rapporte aucun crédit. Réutiliser le compte des
sites conservés comme témoin universel, l'ajouter à `core` ou appliquer cette
enveloppe à la paire d'une `Lca3Forest` serait une fausse mort. La porte de
`7e0ffe79` vérifie l'implication de puissance q3/q4 et l'inclusion de la
lentille, pas un théorème de center-cover.

## Suivi du probe v2 : vérité géométrique reçue, coût rétracté

Le booléen calculé par la v2 est juste au seuil : sur les blocs non capés, il
teste bien si **tout** triplet valide a au moins `h3` intérieurs stricts. Le
cover coefficient 3 contient tout carrier possédé et tout intérieur q3 associé
à l'arête maximale ; WSPD, antichaîne de handles, diagonales retirées et
`EdgeKey` ferment la provenance. Le nom `min_exact_ball_depth` promet cependant
une valeur qui n'est pas calculée après la première boule peu profonde ; le
prédicat doit s'appeler par exemple `all_valid_supports_depth_ge_h3`.

La baseline v2 parcourt aussi toutes les ancres actives et mesure correctement
la condition « le bloc entier est déjà mort par $W_3$ ». Ajouter l'invariant
exécutable `pair_w3_dead => all_valid_supports_depth_ge_h3` : sa violation
signalerait une erreur de cover, de support ou de stricte puissance. Il n'existe
en revanche aucune dominance de coût entre ce minimum capé et l'ancien compte
commun ; leurs sorties anticipées portent sur des axes différents.

La pondération annoncée comme « travail » n'est pas reçue. Le produit
`valid_forms * rectangle_candidates` est seulement un
`full_scan_upper_pairings` statique sur la cohorte jugée :

- la production construit un cover exact **par ancre**, éventuellement compacté
  par l'enveloppe, puis s'arrête au neuvième intérieur ; histogramme, $W_3$,
  secteurs et grille ont déjà retiré des seeds ou des sites ;
- une baseline booléenne de bloc crédite zéro à un bloc mixte, même si $W_3$
  évite tous les rescans de plusieurs de ses ancres ;
- les boules profondes surpondérées par ce proxy peuvent précisément être les
  moins chères grâce à l'arrêt anticipé ;
- les blocs capés sortent du dénominateur et leur compteur vaut `T` alors que
  la détection prouve au moins `T+1` ; ces blocs lourds peuvent dominer ;
- les blocs vides sont échantillonnés uniformément en blocs, souvent sur de
  petits handles ou des diagonales, et `travail_vide` n'est ni publié ni dans
  la bonne unité ;
- le pas de phase zéro est corrélé à l'ordre Morton/WSPD et ne sélectionne pas
  exactement la valeur demandée par `--blocs`.

Les pourcentages `99,7 %`, `99,5 %`, `78,9 %`, `76,2 %`, les facteurs de
résidu `70` et `48`, ainsi que l'explication « gros cover donc beaucoup de
$W_3$ » sont donc rétractés comme conclusions de coût ou de causalité. Le taux
d'environ 74 % de blocs jugés entièrement profonds reste un signal diagnostique
conditionnel aux blocs non capés, pas un gain produit receipté.

### Réponses V64--V66

- **V64 — pas de renversement de priorité.** Fibre et center-cover sont le
  même premier incrément : la fibre porte provenance, masse et fates ; le
  center-cover décide. La fréquence uniforme des blocs vides ne justifie pas
  de commencer par `EMPTY` avant d'avoir mesuré le coût qu'il évite.
- **V65 — certificats $O(1)$ sûrs mais incomplets.** Un bloc ne contient aucun
  tiers strictement aigu si le majorant de
  `||2*C-A-B||^2` ne dépasse pas le minorant de
  `D=||B-A||^2`. De même, `lower(||C-A||^2) > upper(D)` ou son symétrique
  prouve que `AB` ne peut être maximale. Ces rejets ne classent pas toute la
  vacuité. Décomposer au moins `ZERO_ROLE_MASS`, `NONE_ACUTE`,
  `NONE_MAX_EDGE` et `NONE_OWNER`, puis mesurer les appels réellement évités
  par cause.
- **V66 — rejouer le chemin causal.** Compter par étage les ancres, sites de
  cover filtrés, sites après enveloppe, appels de puissance réellement
  exécutés et sorties anticipées. Le coût du certificateur est un compteur
  séparé. Publier parallèlement le potentiel par blocs, la masse brute de rôles
  et la masse de supports valides ; ne jamais convertir l'un en temps évité.

Le probe suivant doit aussi porter une cible CMake, un pin et un digest
d'entrée, la seed et la méthode d'échantillonnage, mur/HWM, des cumuls vérifiés
en entier large et deux caps distincts pour rôles inspectés et supports
retenus. Pour un bloc capé, publier un intervalle de masse, jamais l'exclure
silencieusement. À `n<=14`, l'oracle vérifie le prédicat idéal, l'implication
$W_3$, le ledger et le compte exact des appels avec arrêt anticipé.

## Provenance exacte de la fibre

La WSPD partitionne les arêtes non ordonnées. Pour un rectangle `r=(A,B)`, les
handles de `rect_cover_handles` forment une antichaîne disjointe dans la
fenêtre proposée. Ils partitionnent des **rôles** `(arête, tiers)`, pas encore
les triangles acceptés. Identités distinctes, acuité puis vrai `EdgeKey`
restent des filtres obligatoires.

Comme $A\cap B=\varnothing$, la masse d'un bloc vaut :

$$m(A,B,C)=\lvert A\rvert\lvert B\rvert\lvert C\rvert-\lvert A\cap C\rvert\lvert B\rvert-\lvert B\cap C\rvert\lvert A\rvert.$$

La somme globale ferme $3\binom{n_u}{3}$ rôles, jamais
$6\binom{n_u}{3}$. Longueur maximale puis `EdgeKey` conservent exactement un
rôle par triangle. Le complément des handles reçoit explicitement le fate
`DEAD_OUTSIDE_WINDOW`. Sa sûreté q3 vient de l'identité suivante pour tout
tiers dont `ab` est l'arête maximale :

$$\lVert 2c-a-b\rVert^{2}=2\lVert c-a\rVert^{2}+2\lVert c-b\rVert^{2}-\lVert b-a\rVert^{2}\leq3\lVert b-a\rVert^{2}.$$

Le ledger d'un rectangle est donc `sum(handle_mass) + outside_mass =
|A||B|(n_u-2)`. Une capacité atteinte conserve le rôle en `pending` ; elle ne
le perd pas et ne développe pas silencieusement le produit.

## Réfutation permanente des $8^{3}$ coins

Prendre les deux extrémités, le segment de tiers et le témoin suivants :

```text
a  = (10, 0, 0)       b  = (50, 0, 0)
x- = (20,24, 0)       x0 = (30,24, 0)       x+ = (40,24, 0)
z  = (30,25, 0)
```

Les trois triangles sont strictement aigus et `ab` est leur arête maximale
stricte. Pourtant :

```text
q3_power(a,b,x-;z) = -57 600 000
q3_power(a,b,x0;z) = +38 400 000
q3_power(a,b,x+;z) = -57 600 000
```

Les deux coins distincts de la boîte plate `C` disent « intérieur strict » et
son point entier intérieur dit « extérieur strict ». `q3_power` n'est pas
séparément convexe dans le carrier. La fixture est préparée dans
`mhgp5_q3_skinny_center` et passe localement ; elle doit être épinglée avec le
prochain delta fonctionnel.

## Certificat sûr : center-cover conditionné par $C$

Un fallback simple évalue la forme polynomiale exacte par intervalles entiers
dirigés sur `A,B,C,W`. `power_upper < 0` crédite un nœud témoin,
`power_lower >= 0` le rejette, et `MIXED` subdivise ou rend `pending`. Cette
route est sûre mais risque d'être lâche à cause des dépendances d'intervalles.

La forme à encadrer est exactement celle de `q3.hpp`. Avec `d=b-a`, `u=c-a`,
`y=z-a`, `D=d.d`, `E=u.u`, `F=d.u`, `G=D*E-F*F` et
`W=E*(D-F)*d+D*(E-F)*u`, poser :

$$\Pi(a,b,c;z)=G(y\mathbin{\cdot}y)-y\mathbin{\cdot}W.$$

Construire les intervalles par `add/sub/mul/square`, le carré prenant zéro
comme minimum s'il traverse zéro. L'identité de Gram autorise à intersecter la
borne de $G$ avec `[0,+inf)` sans perdre de valeur réelle. `Pi_upper < 0`
signifie `ALL_STRICT_INTERIOR`, `Pi_lower >= 0` signifie seulement que ce nœud
ne fournit aucun témoin, et tout autre résultat reste `MIXED`. Cette voie sert
aussi d'oracle indépendant du raccord par patches.

La route prioritaire réemploie les patches entiers déjà spécifiés, avec un seul
parcours témoin par rectangle :

1. construire une fois les 64 patches q3 `Q_j` du rectangle `(A,B)` et leur
   masque de médiatrice `AB` ;
2. parcourir les témoins une fois et construire, hors `A union B`, les crédits
   `g_AB[j]` tels que
   `max(L32(Q_j,A,W), L32(Q_j,B,W)) > 0` ;
3. pour chaque handle `C`, conserver le bit `j` seulement si les trois
   intervalles de médiatrice `AB`, `AC`, `BC` contiennent zéro ; un intervalle
   est impossible exactement si `L32 > 0` ou `U32 < 0`, tandis que toute
   égalité reste faisable ;
4. si le masque est vide, aucun support réel n'existe dans le bloc ; sinon
   exiger le seuil de profondeur sur chacun de ses bits ;
5. ne lancer un parcours dépendant de `C`, par exemple avec
   `L32(Q_j,C,W)>0`, que comme renforcement mesuré sur le résiduel.

Ces trois tests médiateurs séparés ne prouvent pas qu'un même triplet réalise
simultanément les égalités. Ils conservent donc un sur-ensemble, ce qui est le
bon sens fail-open. Pour q3 ils ignorent aussi la coplanarité du centre
distingué. Cette perte peut diminuer le prune, jamais créer une fausse mort.

L'implémentation de `L32` peut rester courte. Par axe, la fonction
`dist(t,P)^2 - max((t-x0)^2,(t-x1)^2)` est concave ; son minimum sur
l'intervalle du patch est donc atteint à l'une de ses deux extrémités. Les
bornes de `P` ajoutées dans la note antérieure sont inutiles mais inoffensives.
Le produit de boîtes est connexe et la différence de puissances continue : son
image est exactement l'intervalle `[L32,U32]`. Zéro dans cet intervalle prouve
seulement une égalité relaxée pour cette médiatrice, jamais les trois à la fois.

La réutilisation de `g_AB[j]` est sûre : tout vrai centre de `(a,b,c)` reste
dans au moins un bit du masque de son handle et, pour ce centre, le test témoin
positif relativement à `A` ou `B` prouve une puissance strictement intérieure,
indépendamment de `C`. Retirer d'autres patches ne change ni `Q_j`, ni son
antichaîne. Le compteur `witness_node_pops` doit donc être indépendant du
nombre de handles `C`. La réutilisation cesse si `(A,B)`, la grille, la lane ou
le pavage changent.

Les crédits de patches différents ne sont ni sommés ni unis. Ils peuvent en
revanche utiliser des témoins différents, ce qui est précisément le gain que
le compte commun du probe ne mesure pas.

## Contrat de $h_0,h_a,h_b,h_c$

Soit `F` l'ensemble non vide des triplets distinct-ID, aigus et possédés du
bloc, et `I_t` l'ensemble des sites de puissance strictement négative pour
`t`. Comme `C` peut recouvrir les extrémités, les domaines physiques disjoints
sont :

$$D_A=A,\qquad D_B=B,\qquad D_C=C\setminus(A\cup B),\qquad D_0=P\setminus(A\cup B\cup C).$$

Les crédits complets s'écrivent :

$$H_0=D_0\cap\bigcap_{t\in F}I_t,\qquad H_A(a)=D_A\cap\bigcap_{t\in F:\,t_A=a}I_t,\qquad H_B(b)=D_B\cap\bigcap_{t\in F:\,t_B=b}I_t,\qquad H_C(c)=D_C\cap\bigcap_{t\in F:\,t_C=c}I_t.$$

Une fibre ou une tranche fixant `a`, `b` ou `c` sans complétion valide reçoit
zéro, jamais une cardinalité vacante. Pour tout
`t=(a,b,c)` de `F`, les quatre ensembles sont disjoints et inclus dans `I_t` :

$$\mathrm{depth}(t)\geq h_0+h_a(a)+h_b(b)+h_c(c).$$

Le premier incrément doit néanmoins omettre $h_c$. Il réutilise les tableaux
`h_a(a),h_b(b)` de $W_3$, déjà calculés une fois par rectangle, et le crédit
`g_AB[j]` extérieur à `A union B`. Ce dernier n'est **pas** le vrai $h_0$ à
quatre strates : il peut contenir d'autres positions de `C`. Il reste
additionnable à $h_a,h_b$ tant que $h_c$ est absent. Le carrier effectivement
choisi ne peut pas être crédité dans le patch de son vrai centre, car sa
puissance y vaut zéro. Ces crédits restent sûrs même si `C` recouvre `A` ou
`B` : un tiers aigu vérifie `H<0`, tandis qu'un témoin $W_3$ exige `H>0`.

Une fixture interdit de réduire ces tableaux à deux scalaires. Avec
`a0=(4,2,0)`, `a1=(3,2,0)`, `b=(0,0,0)` et `c=(0,3,0)`, les deux triangles
sont aigus et `(ai,b)` est strictement maximal. `a1` est intérieur à la boule
de `a0,b,c`, alors que `a0` est extérieur à celle de `a1,b,c` :
`h_a(a0)=1` et `h_a(a1)=0`.

Le `tb` actuel n'est pas ce central additionnable : il exclut seulement les
sites apparaissant dans un triplet valide, pas toutes les plages `A` et `B`.
Un point inactif de `A` peut donc aussi vivre dans `h_a`. Le prochain probe
publie `central_outside_AB` ou conserve les IDs.

`AliveRect::core` et `g_AB[j]` peuvent reconnaître le même site. Sans
identités, leur seule composition sûre est `max(core_AB,g_AB[j])`. Avec au
plus huit crédits dans un rectangle q3 vivant,
`collect_universal_ids` permet de former explicitement l'union puis de chercher
seulement de nouveaux IDs. Aucun crédit n'est hérité après un split dont les
patches changent.

Une condition simple de mort du patch `j` est :

$$\max(h_{\mathrm{core}},g_{AB,j})+\min_{a\in A}h_a(a)+\min_{b\in B}h_b(b)\geq h_3.$$

Tous les patches faisables doivent la satisfaire pour tuer le bloc entier.
Sur le domaine complet, le critère exact reste le minimum couplé de
`h0+ha+hb+hc` sur `F`. Une convolution des histogrammes est exacte seulement
sur un produit cartésien ; acuité et owner couplent généralement les rôles.
Elle donne sinon un surcompte de travail, pas une partition des survivants.

Un futur `h_c(c)` prend ses témoins dans `C` privé de `A union B`, et le vrai
central devient alors extérieur à `A union B union C`. `g_AB[j]` et $h_c(c)$
peuvent partager un autre site de `C` : avant de les composer, conserver les
IDs et prendre leur union, ou reconstruire `h0_j(C)` hors `C`. La fixture
future minimale prend `a=(0,0,0)`, `b=(4,0,0)`, `c=(2,3,0)` et `z=(2,1,0)`,
avec `c,z` dans le même `C` : `z` appartient à $W_3(a,b)$ et est strictement
intérieur à la circumboule de `(a,b,c)`, donc peut vivre à la fois dans
`g_AB[j]` et $h_c(c)$. L'auto-jointure de $h_c$ est capée par les 32 positions
d'un handle mais peut encore payer 1024 couples par bloc. Elle vient seulement
après les prunes `EMPTY/NONE_OWNER`, médiatrices et central, sur le résiduel
mesuré.

La version autoritaire transporte pour chaque source un
`CappedWitnessSet<h3>` trié d'IDs. Deux méthodes appliquées au même domaine se
composent par union ; sans IDs, seulement par `max`. Après un split qui change
les patches, un enfant ne transporte que les IDs explicitement revalidés sous
son propre certificat ; sa recherche ignore ensuite ces IDs. L'addition
`parent_count + fresh_count` est interdite.

Si l'on veut récupérer dans $h_c$ les positions de $C$ qui recouvrent $A$ ou
$B$, il faut d'abord normaliser l'auto-jointure ordonnée. Pour un nœud
`N=(L,R)` :

$$\mathrm{Ord2}(N)=\mathrm{Ord2}(L)\mathbin{\dot\cup}(L\times R)\mathbin{\dot\cup}(R\times L)\mathbin{\dot\cup}\mathrm{Ord2}(R).$$

Les tiers extérieurs et les deux auto-jointures internes ferment alors la
masse exacte $\lvert A\rvert\lvert B\rvert(n-2)$ sans diagonale. Une somme de
cardinalités sur `C=root` sans cette normalisation est fausse.

Après saturation à `need=h3-h0`, un domaine restant réellement cartésien et
disjoint autorise les histogrammes `N_A[i],N_B[j],N_C[k]` et :

$$M_{surv}=\sum_{i+j+k<need}N_A[i]N_B[j]N_C[k].$$

La convolution coûte `O(|A|+|B|+|C|+need^2)` avec `need<=9`. Elle rend donc la
**combinaison** des crédits constante, pas leur calcul. Le verrou courant reste
`corner_histograms`, en `O(|A|^2+|B|^2)`. Sa relève parcourt les témoins par
nœuds, crédite un sous-arbre certifié, scinde `MIXED` et s'arrête après neuf
IDs. Son coût n'est quasi linéaire que si le nombre de nœuds `MIXED` le reste ;
c'est une porte de mesure, pas une borne reçue.

## Relève directement intégrable de `corner_histograms`

Le prochain incrément utile ne demande ni nouvelle WSPD, ni nouveau carrier.
Il remplace d'abord chaque ligne quadratique de l'histogramme par une requête
saturée sur l'arbre spatial déjà construit :

```cpp
struct FactorQueryStats {
  u64 endpoint_queries = 0;
  u64 node_visits = 0;
  u64 none_prunes = 0;
  u64 bulk_nodes = 0;
  u64 bulk_positions = 0;
  u64 leaf_tests = 0;
  u64 diagonal_splits = 0;
  u64 saturated_endpoints = 0;
};

u8 factor_witness_count_sat(const CloudIndex& ix, Lane lane, i32 support,
                            NodeRef partner, NodeRef witness_factor, u8 cap,
                            FactorQueryScratch* scratch,
                            FactorQueryStats* stats);
```

Pour `h_a(a)`, `support=a`, `partner=B` et `witness_factor=A`; pour
`h_b(b)`, échanger les rôles. Construire une fois par endpoint la boîte
ponctuelle `S={support}`, `Box(partner)` et, pour q3/q4, leur `core_ball`. La
descente suit exactement ces règles :

1. si le nœud témoin contient `support`, le scinder avant tout crédit ; à la
   feuille diagonale, ne rien compter ;
2. si `hmax4_boxes(S,Box(partner),Box(Z)) <= 0`, rejeter `Z` pour toutes les
   lanes ;
3. en q2, `hmin_boxes(S,Box(partner),Box(Z)) > 0` crédite tout `Z`; en q3/q4,
   `box_vs_ball(Box(Z),core_ball) > 0` fait de même ;
4. tout autre nœud interne est `MIXED` et se scinde ; en particulier
   `box_vs_ball < 0` ne prouve pas que `Z` est hors du fuseau complet ;
5. une feuille non diagonale garde l'autorité actuelle
   `universal_over_corners(lane,S,Box(partner),z)` ;
6. chaque crédit est borné par `cap-count` et la requête s'arrête à `cap`.

Sous le profil sans positions dupliquées, un crédit de nœud ajoute sa
cardinalité de positions, jamais un poids de multiplicité. Les nœuds crédités
forment une antichaîne et aucun descendant n'est visité après leur crédit. Le
résultat autoritaire est `min(cap,compte_actuel)` ; le bit `saturated` signifie
seulement « au moins `cap` », pas que le compte complet est connu. Employer
`cap=need=h_q-h_core` suffit à toutes les décisions actuelles.

La complexité d'un rectangle devient `O(|A|+|B|+V_A+V_B)`, où `V_A,V_B`
comptent **toutes** les visites de nœuds des requêtes d'endpoints. Elle peut
encore être quadratique si presque tout reste `MIXED`; ce compteur est donc la
porte de réfutation de l'idée. Dans les régimes où les boules-cœurs créditent
des sous-arbres ou où neuf témoins sont trouvés tôt, elle évite réellement les
auto-produits complets.

Le second étage doit retirer aussi le produit `A x B` déjà mort. Comme les
comptes sont saturés à `need`, construire les bitsets cumulatifs
`B_lt[t]={b : h_b(b)<t}` pour `1<=t<=need`. Pour chaque `a`, les seuls
partenaires à émettre sont les bits de `B_lt[need-h_a(a)]`; un seuil nul émet
rien. Parcourir les mots puis leurs bits de poids faible à fort conserve
l'ordre canonique `ua`, puis `ub`. Les compteurs des ancres mortes sont mis à
jour en masse, sans construire ces ancres.

Le ledger exécutable est :

```text
total_pairs    = |A| |B|
killed_pairs   = #{(a,b) : h_a(a)+h_b(b) >= need}
survivor_pairs = #{(a,b) : h_a(a)+h_b(b) <  need}
total_pairs    = killed_pairs + survivor_pairs
```

Hors requêtes d'arbre, ce filtre coûte
`O(need*ceil(|B|/64)+|A|+survivor_pairs)`, avec `need<=9` en q3 et `need<=8`
en q4 au profil courant. Le pire cas reste proportionnel au nombre de
survivants, ce qui est nécessaire puisque le terminal les consomme ; le cas
« toutes les paires tuées » ne parcourt plus `A x B`.

L'API doit être partagée par le CPU, les lanes batched et la préparation
device, afin de ne pas remplacer une boucle quadratique par trois versions
divergentes. Le premier raccord reste CPU-reference et counter-only : comparer
les tableaux saturés et l'ordre des survivants à l'ancienne double boucle sur
les trois lanes, puis activer la nouvelle énumération derrière une option
explicite. Compteurs minimaux : `hist_endpoint_queries`, `hist_node_visits`,
`hist_leaf_tests`, `hist_bulk_positions`, `hist_saturated_endpoints`,
`hist_total_pairs` et `hist_survivor_pairs_iterated`.

Les fixtures permanentes couvrent un bulk non vide, tout `MIXED`, la diagonale,
la coquille stricte, les saturations à 1 et au seuil maximal, puis comparent
masse et ordre exacts des survivants. Une fixture doit avoir
`total_pairs>0`, `survivor_pairs=0` et aucune itération d'ancre. Les mutants
retirent respectivement le split diagonal, descendent après un bulk, utilisent
`cap-1` et ferment la coquille. C'est seulement après ces portes que la même
primitive alimente `g_AB[j]`; la convolution avec `C` reste interdite tant que
le sous-domaine n'est pas prouvé cartésien après acuité, owner et diagonales.

## Ordre d'implémentation transmis à Claude

```text
RectId(A,B), patches, core IDs, h_a, h_b, g_AB[j] (un seul scan témoin)
  -> handles C + masse de rôles + fate DEAD_OUTSIDE_WINDOW
  -> masque C par médiatrices AB/AC/BC
  -> masque vide, ou seuil max(core,g_AB[j]) + minima h_a/h_b
  -> bloc entièrement mort, split borné, ou pending
  -> ancres et terminal shallow seulement sur le résiduel
```

Le probe reste counter-only. Il se streame par rectangle ; il ne matérialise
pas une liste globale de millions de blocs et ne relance pas un census complet
pour chaque `C` au premier essai. Un handle mort comme **carrier** reste dans
la vue `census_handles` des autres carriers : seule sa vue `support_handles`
est filtrée.

La porte exhaustive à `n<=14` vérifie chaque bloc pruné, le ledger des rôles et
les diagonales. Elle conserve les tangences `L32==0` et `U32==0`, vérifie que
le patch de tout circumcentre rationnel survit, tue les mutants qui unissent
des patches ou somment `core+g_AB`, et rend visible tout rescan témoin par `C`.
Le reçu publie patches visités/faisables, tests de médiatrices,
`witness_node_pops`, blocs entièrement morts, masse de rôles morte, blocs
capés, seeds et rescans réellement évités, coût ajouté, mur et HWM. Commande,
`HEAD`, worktree et sorties brutes sont obligatoires avant tout nouvel
exposant.

## Extension q4 : même tape, une strate de plus

Pour q4, `A x B x C` n'est pas encore le support complet : il reste un
quatrième sommet. Les deux sommets opposés à `AB` forment toutefois un rôle
**non ordonné** `{c,d}`. Les nommer `C`, puis `D`, est un ordre de parcours,
pas une seconde provenance sémantique. Le ledger reste
`Omega4={(e,{c,d})}`, de masse $6\binom{n_u}{4}$, et non douze occurrences par
support.

Le premier étage reste identique et sans rescan : la grille q4 et ses crédits
`g4_AB[j]` sont calculés une fois par `(A,B)`. Un premier handle applique
`AB/AC/BC`, puis un second ajoute `AD/BD/CD`. Employer les six tests séparément
resserre le sur-ensemble ; cela ne prouve ni leur réalisation simultanée, ni
la non-coplanarité, ni le bien-centrage, qui restent fail-open jusqu'au terminal
exact. Surtout, ne pas exiger que le handle visité en premier soit déjà la face
aiguë canonique : le terminal doit choisir entre `c` et `d` avec la règle
exact-once actuelle, ou employer un prédicat symétrique.

Le seuil q4 est `h4=smax-3`, soit huit pour `smax=11`, et ses patches sont ceux
de q4, jamais ceux de q3. Le crédit `g4_AB[j]` est sûr pour toute sphère du
patch parce qu'il se compare déjà au rayon porté par `a` ou `b`; il ne dépend
ni de `C`, ni de `D`. Les tableaux `h_a,h_b` employés ici sont ceux de $W_4$,
pas les crédits q3. Une mort q3 ne tue toujours pas une complétion q4.

La décomposition complète ajoute nécessairement $h_d(d)$ :

$$D_A=A,\qquad D_B=B,\qquad D_C=C\setminus(A\cup B),\qquad D_D=D\setminus(A\cup B\cup C),\qquad D_0=P\setminus(A\cup B\cup C\cup D).$$

Les crédits $h_0,h_a,h_b,h_c,h_d$ sont définis par intersections universelles
sur leurs fibres non vides, exactement comme en q3. Le premier incrément q4
doit pourtant s'arrêter à `g4_AB + h_a + h_b`. Ajouter $h_c$, puis $h_d$, exige
des IDs ou une repartition explicite à chaque nouveau handle. Le flux doit donc
imbriquer les handles `C`, puis `D` seulement sur les masques survivants, avec
continuations et fates, sans matérialiser un catalogue global `C x D`.

### Ledger local des paires de handles

Les handles `H_i` d'un rectangle forment une antichaîne, donc leurs plages de
positions sont disjointes. Parcourir seulement `i<j` pour les blocs croisés et
`choose2(H_i)` pour les blocs diagonaux partitionne les paires non ordonnées.
Poser $n_X=\lvert X\rvert$, $\alpha_X=\lvert A\cap X\rvert$ et
$\beta_X=\lvert B\cap X\rvert$. Pour deux handles distincts `C,D`, la masse de
quadruplets à IDs distincts est :

$$m_4(A,B;C,D)=n_A n_B n_C n_D-n_D(n_B\alpha_C+n_A\beta_C)-n_C(n_B\alpha_D+n_A\beta_D)+\alpha_C\beta_D+\beta_C\alpha_D.$$

Pour le bloc diagonal d'un handle `H`, elle vaut :

$$m_4(A,B;H,H)=n_A n_B\binom{n_H}{2}-(n_H-1)(n_B\alpha_H+n_A\beta_H)+\alpha_H\beta_H.$$

Ces formules retirent les extrémités `a,b` sans ordonner `c,d`. Les évaluer en
`i128`, exiger un résultat non négatif, puis convertir en `u128`. La masse hors
fenêtre est calculée par différence exacte et reçoit son fate ; pour tous les
rectangles WSPD, la fermeture attendue est :

$$\sum_r(\text{covered mass}_r+\text{outside mass}_r)=\sum_r\lvert A_r\rvert\lvert B_r\rvert\binom{n_u-2}{2}=6\binom{n_u}{4}.$$

Une petite porte énumère directement les quadruplets pour les cas `C!=D` et
`C==D`, y compris chaque recouvrement possible avec `A` ou `B`, puis mute le
`choose2` en produit ordonné. Ce ledger est une preuve de provenance, pas une
autorisation de construire tous les couples de handles dans le hot path.

## Ablation structurelle différée

L'autre audit propose une partition `Lca3Forest` : pour chaque nœud interne
`u`, prendre ses enfants `L(u),R(u)` et, pour chaque ancêtre strict `v`, le fils
de `v` opposé au chemin vers `u`. Les blocs `L(u) x R(u) x C(u,v)` partitionnent
exactement les triplets en facteurs disjoints et leur nombre est au plus
`48(n-1)` sous les clés Morton48 distinctes.

Cette observation est mathématiquement utile comme comparateur de ledger, mais
elle ne remplace pas le premier incrément : la paire LCA n'est généralement
pas l'arête maximale et n'est pas WSPD-séparée. Ni le spindle ni le cover owner
actuels ne s'y appliquent. La tester comme oracle structurel est légitime ; la
présenter comme nouvelle route produit avant le probe fibré ne l'est pas.
