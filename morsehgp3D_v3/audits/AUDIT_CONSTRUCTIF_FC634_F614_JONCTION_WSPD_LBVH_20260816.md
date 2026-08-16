# Audit constructif — après `fc63408` et `f614b74`

Date : 16 août 2026 UTC.  
Pins fonctionnels relus :

- `fc6340893a457d0f2957568aa4033e2e64ebc0bf` — carriers conditionnés par la vivacité exacte de l’ancre ;
- `f614b741577fa18506b9478552193ab6f2b81e37` — classifieur conjoint `W4 × carrier`, actuellement lancé depuis la racine.

Complète :

- [`NOTE_AUDITEUR_LBVH_SPARSE_Q3_Q4_APRES_53815F_20260816.md`](NOTE_AUDITEUR_LBVH_SPARSE_Q3_Q4_APRES_53815F_20260816.md) ;
- [`NOTE_AUDITEUR_POISSON_CHARGE_CARRIERS_AIGUS_20260815.md`](NOTE_AUDITEUR_POISSON_CHARGE_CARRIERS_AIGUS_20260815.md).

Cadre : `phase=exploration_v3_hors_registre`,
`backend=math_reference_and_gpu_architecture`,
`profile=quantized_u16_input_only`,
`mode=joint_walive_carrier_sparse_source_review`,
`public_status=not_claimed`.

> [!IMPORTANT]
> **Verdict court.** Les deux commits avancent dans la bonne direction :
>
> 1. mesurer les carriers seulement sur les ancres exactement vivantes corrige le
>    cubique artificiel du gateway non filtré ;
> 2. constater que le ledger `W4` est inerte depuis `(root,root,root)` localise
>    correctement le raccord : la conjonction doit être évaluée dans les
>    rectangles déjà séparés du `CKPairTape/WSPD`.
>
> Deux formulations doivent néanmoins être retirées avant de guider le code
> produit :
>
> - `#carriers = O(h)` n’est vrai **qu’en espérance sous un modèle homogène**, ou
>   sous une hypothèse déterministe de régularité locale ; une ancre `W4`-vivante
>   peut avoir `Theta(n)` carriers ;
> - le cœur de Jung `B(c0,|ab|/4)` sert à trouver des **intérieurs permanents** et
>   tuer un seed ; il ne contient généralement pas le quatrième sommet.
>
> La prochaine route reçue doit être :
>
> ```text
> WSPD terminal + ledger W4 hérité
>   -> jointure bloc W4-vivacité × acute-owner
>   -> microtuiles d’arêtes résiduelles
>   -> requête LBVH fusionnée par arête
>        compte W4 exact, arrêt à 8
>        + carriers exacts
>   -> cover LBVH partagé par arête
>   -> top-k axial LBVH par seed
>   -> owner6 / primary / positivité / BallKey / fold.
> ```
>
> Le bloc peut tuer une masse quadratique sans PairId. Le LBVH traite le résiduel
> sans scan global. Aucun catalogue résident de paires, de faces ou de
> `carrier×apex` n’est introduit.

---

## 1. Ce qui est reçu positivement

### 1.1 Conditionner par `V4` était indispensable

Le gateway de `53815f` comptait tous les triangles aigus possédés, y compris ceux
issus d’ancres manifestement mortes. Il mesurait donc un sur-objet cubique. Le
commit `fc63408` sépare maintenant :

```text
C4_sur_V4,
C4_sur_morts.
```

Cette séparation est correcte et informative. Les moyennes mesurées sur
`terrain`, `uniform` et `eight_clusters` montrent que le régime conditionnel est
beaucoup plus petit que le catalogue de tous les triangles aigus. Publier aussi
le maximum est la bonne décision : le dimensionnement GPU dépend de la queue,
jamais de la moyenne seule.

Les pentes entre `n=200,400,800` restent des observations de petite rampe. La
formulation reçue est :

> Sur les trois familles mesurées, le nombre total de carriers des ancres
> exactement `W4`-vivantes est sous-quadratique sur la rampe testée.

Ce n’est pas encore un théorème asymptotique.

### 1.2 L’inertie depuis la racine est un résultat utile

Le diagnostic de `f614b74` est correct. Un ledger de témoins universels porte
sur un produit de facteurs déjà séparés. Depuis `(root,root,root)`, le facteur
témoin contient les endpoints possibles ; aucun site ne peut être universel pour
toutes les paires de `root×root`. Il est donc normal que :

```text
dead_w4 = 0.
```

La conclusion architecturale est la bonne : ne pas créer un troisième pipeline
concurrent, mais brancher le gateway aigu **dans** les terminaux WSPD, là où :

- `A` et `B` sont disjoints et séparés ;
- `h_coeur/lower_open` existe déjà ;
- les preuves `ALL/NONE/MIXED` peuvent être héritées ;
- le rectangle possède un `RectId` exact-once.

Cette réception n’autorise pas encore la phrase « le classifieur conjoint est
correct » au sens du protocole. La branche `DEAD_W4` est actuellement inerte et
n’a donc pas d’autorité causale au niveau nuage. Elle est plausible et bien
orientée ; elle doit maintenant être exercée sur une fixture WSPD dédiée.

---

## 2. Correction P0 : `W4`-vivant n’implique aucune borne déterministe `O(h)` sur les carriers

Le rapport de volumes `|L|/|W4|` contrôle une moyenne sous homogénéité. Il ne
fournit aucune domination de cardinalités pour un nuage arbitraire. Les deux
régions sont même géométriquement disjointes : `W4` est du côté `H>0`, tandis
qu’un carrier aigu vérifie `H<0`.

### 2.1 Contre-famille exacte

Fixer, dans `R^3` :

```text
a = (-R,0,0),
b = ( R,0,0),
D = ||a-b||² = 4R².
```

Pour tout point du plan médiateur

```text
x = (0,u,v),
s = u²+v²,
```

on a :

```text
E = ||x-a||² = R²+s,
X = ||b-x||² = R²+s,
H = (x-a)·(b-x) = R²-s.
```

Si

```text
R² < s < 3R²,
```

alors :

```text
E < D,
X < D,
H < 0.
```

Donc `ab` est l’arête maximale stricte et `abx` est un carrier aigu. En revanche,
`x` n’appartient même pas à la boule diamétrale `W2`, donc a fortiori pas à
`W3` ou `W4`.

Choisir tous les points entiers de l’anneau :

```text
X_R = {(0,u,v) in Z³ : R² < u²+v² < 3R²}.
```

Alors :

```text
#X_R = Theta(R²),
#(P ∩ W4(a,b)) = 0,
#carriers(a,b) = #X_R.
```

L’ancre est donc `W4`-vivante avec zéro témoin de fuseau, mais porte
`Theta(n)` carriers. La même famille réfute la borne déterministe pour q3.

### 2.2 Fixture u16 immédiatement disponible

Après translation, prendre :

```text
a=(1000,1000,1000),
b=(1020,1000,1000),
```

et tous les points :

```text
x=(1010,1000+u,1000+v),
100 < u²+v² < 300.
```

Il y en a exactement `632` dans la grille entière indiquée. Pour l’ancre `ab` :

```text
W4_count = 0,
canonical_carriers = 632.
```

Cette fixture doit devenir permanente sous un nom tel que :

```text
live_anchor_many_carriers_annulus.
```

Elle tue :

```text
carrier-cap-proportional-to-h,
carrier-average-used-as-cap,
live-anchor-implies-local-degree-bounded.
```

### 2.3 Énoncé mathématique correct

Sous Poisson homogène à densité fixe, les régions `W4(a,b)` et carrier sont
disjointes. Le calcul Campbell--Mecke déjà versé donne :

```text
E[#carriers | paire W4-vivante]
  = (c_d / v_4,d) * (h_4+1)/2.
```

En dimension trois et pour `h_4=8`, la valeur de référence est environ :

```text
29,335 carriers par paire vivante.
```

La bonne phrase devient donc :

> Le nombre de carriers est `O(h)` **en espérance sous Poisson homogène**, avec
> une constante explicite. Il peut être `Theta(n)` dans le pire cas.

Une version déterministe `O(h)` exige une hypothèse de régularité locale, par
exemple des bornes supérieure et inférieure de type Ahlfors sur les populations
de boules comparables. Aucune telle hypothèse n’appartient actuellement au
contrat produit.

### 2.4 Corrections documentaires demandées

Dans le titre, les commentaires CMake et les notes de `fc63408`, remplacer :

```text
Porteurs par ancre vivante : O(h) et non O(n)
```

par :

```text
Porteurs par ancre vivante : petite moyenne sur les familles testées ;
O(h) en espérance homogène, non borné déterministement.
```

Le test numérique actuel reste utile. C’est son interprétation qui doit être
corrigée, pas sa mesure.

---

## 3. Correction P0 : le cœur de Jung ne génère pas le quatrième sommet

Le pipeline proposé dans `fc63408` contient :

```text
y <- requête sur le cœur de Jung B(c0,|ab|/4).
```

Cette ligne est fausse si `y` désigne le quatrième sommet. Le cœur de Jung est
contenu dans toutes les boules q4 admissibles du seed. Tout point de ce cœur est
un **intérieur permanent**, précisément l’objet qui fait mourir le seed à huit
IDs. Un sommet de support est sur le shell d’une sphère particulière ; il n’a
aucune raison d’appartenir à l’intersection commune des intérieurs.

### Contre-fixture déjà présente : tétraèdre régulier

Prendre les quatre sommets alternés de `{0,2}³` :

```text
a=(0,0,0),
b=(0,2,2),
x=(2,0,2),
y=(2,2,0).
```

Pour le seed `(a,b,x)`, la face est équilatérale. Son circumcentre plan vaut :

```text
c0=(2/3,2/3,4/3).
```

Le rayon du cœur est :

```text
|ab|/4 = sqrt(8)/4.
```

Mais :

```text
||y-c0|| = 4/sqrt(3) > sqrt(8)/4.
```

Le véritable apex est donc très loin du cœur.

La route correcte reste :

```text
SeedCoreQuarter / SeedJungPermanent16
  -> mort éventuelle par intérieurs permanents
  -> sinon Q4SeedAxisTopR4-LBVH
       premières/dernières racines axiales
  -> y parmi ces groupes de racines.
```

Ajouter un mutant permanent :

```text
q4-apex-source-limited-to-jung-core.
```

Le tétraèdre régulier doit le tuer.

---

## 4. `W4`-vivant ne borne pas non plus la longueur de l’ancre

La phrase :

```text
b <- candidats locaux, la vivacité de l’ancre bornant |ab|
```

n’est pas vraie sans hypothèse de densité inférieure. `two_lines` en est déjà un
contre-exemple : des paires très longues sont `W4`-vivantes parce que le fuseau
traverse un vide. Deux amas séparés produisent le même phénomène.

La complétude des ancres doit donc rester portée par :

```text
CKPairTape/WSPD exact-once,
```

et non par un voisinage k-NN local non certifié. Un chemin local peut être un
fast path :

```text
local_radius_certified -> CSR local,
```

mais les rectangles inter-amas restent dans le tape factorisé et passent au
gateway collectif.

---

## 5. Le prochain raccord doit consommer le ledger WSPD existant, pas le recopier

Le bon résultat de `f614b74` n’est pas « fusionner deux gros probes ». Le bon
résultat logiciel est d’extraire une autorité partagée.

### 5.1 ABI minimale

```text
W4WitnessState {
  uint8  lower_open_sat;      // min(8, crédits universels distincts)
  uint8  upper_closed_sat;    // min(8, lower + population possible)
  SpanRange all_spans;        // antichaîne déjà créditée
  SpanRange none_spans;       // définitivement hors W4 fermé
  TaskRange mixed_frontier;   // NodeHandle disjoints
  Continuation continuation;
}

JointPairCarrierJob {
  RectId rect;
  NodeKey A, B, C;
  W4WitnessState w4;
  uint64 logical_pair_mass;
  uint64 logical_triple_mass;
  LaneMask lanes;
  Fate fate;
}
```

Les compteurs sont saturés à huit pour la décision, mais les masses logiques et
les populations de spans restent en 64 bits.

### 5.2 Ne pas descendre la frontière jusqu’aux feuilles

Le commit affirme que la frontière transporte des handles et non des points.
Dans le microprobe actuel, un nœud `MIXED` est pourtant subdivisé jusqu’à une
feuille avant d’entrer dans `frontiere`; `U4=L4+frontiere.size()` fonctionne donc
parce que chaque handle représente un singleton. C’est correct pour le probe,
mais cela recrée un CSR de points sous un autre nom.

Le chemin produit doit conserver une **antichaîne grossière** :

```text
upper_closed_sat
  = min(8, lower_open_sat + sum population(node) des MIXED).
```

- si `lower_open_sat==8`, le bloc est mort ;
- si `upper_closed_sat<8`, toutes ses paires sont vivantes ;
- sinon on conserve les `NodeHandle MIXED` et on ne scinde que celui choisi par
  la politique de raffinement.

Il n’existe aucune raison de connaître exactement `U4` lorsqu’il est déjà au
moins huit. Cette saturation est à la fois mathématiquement suffisante et
beaucoup plus GPU-friendly.

### 5.3 Héritage

Pour un enfant obtenu par restriction de `A`, `B` ou `C` :

- un span `ALL_W4` du parent reste `ALL_W4` ;
- un span `NONE_W4` du parent reste `NONE_W4` ;
- seul `MIXED` est rejoué ;
- les spans crédités restent masqués dans tous les parcours descendants.

La monotonie annoncée par Claude est correcte. La porte doit néanmoins vérifier
l’égalité de l’union d’IDs, pas seulement les compteurs saturés.

### 5.4 Une seule autorité géométrique

Extraire les primitives de :

```text
combined_prefilter_probe.cpp,
acute_owner_gateway_probe.cpp
```

vers des en-têtes partagés, par exemple :

```text
w4_block_relation.hpp,
joint_walive_acute.hpp.
```

Les probes deviennent des clients et des juges. Copier `bloc_tout_w4`, les
strictes ou les extrema dans un troisième fichier créerait trois versions du
même prédicat, ce qui est une méthode assez fiable pour obtenir trois réponses.

---

## 6. Réception mathématique de `bloc_tout_w4`, sous réserve d’un oracle causal

L’idée de tester les coins peut être justifiée par convexité séparée :

1. à `b,z` fixés, la condition q4 sur `e=z-a` définit l’intérieur d’un cône
   circulaire convexe d’angle inférieur à `pi/2` ;
2. symétriquement à `a,z` fixés ;
3. à `a,b` fixés, `W4(a,b)` est l’intersection des boules q4 admissibles, donc
   un ensemble convexe en `z`.

Si les huit coins de chaque facteur donnent des triplets strictement dans `W4`,
on remplit successivement `A`, puis `B`, puis `Z` par convexité. La stratégie de
coins est donc mathématiquement plausible et devrait être reçue.

Mais la branche vaut actuellement zéro sur les familles du probe. Elle doit être
jugée indépendamment avant son raccord WSPD.

### Oracle demandé

Sur de petites AABB entières :

```text
bloc_tout_w4(A,B,Z) == true
  implique
pair_lane(a,b,z)>=4 pour tout triplet entier.
```

Ajouter des boîtes biaisées qui rendent réellement la branche vraie, puis les
mutants :

```text
w4-coin-oublie,
w4-egalite-acceptee,
w4-utilise-seuil-q3,
w4-compte-endpoint,
w4-carre-avant-promotion.
```

La frontière `3H²=ET` déjà gravée doit rester `MIXED`, jamais `ALL_W4`.

---

## 7. Deux défauts d’instrumentation à corriger avant les pentes

### 7.1 `noeuds` compte aussi des évaluations de coins

Le microprobe passe actuellement :

```text
bloc_tout_w4(..., &g.noeuds).
```

La fonction incrémente ce pointeur à chaque évaluation de coin. Le champ
`noeuds`, documenté comme nombre de jobs `(A,B,C)` classifiés, mélange donc deux
unités.

Séparer :

```text
joint_jobs,
w4_corner_triplet_tests,
acute_extrema_tests,
witness_node_visits.
```

Une pente sur `noeuds` n’a pas de sens tant que ces coûts y sont additionnés.

### 7.2 Le probe n’est pas réellement `u16`

`acute_owner_gateway_probe.cpp` stocke les coordonnées dans :

```cpp
struct P3 { short x,y,z; };
```

et effectue des casts explicites vers `short`. Les valeurs `32768..65535` se
replient. De plus, `two_lines` produit actuellement une coordonnée `-1`, ensuite
encodée par un Morton u16. La boîte serrée répare certains certificats, mais ne
répare pas le contrat de domaine ni la cohérence de la cellule Morton.

Le correctif demandé est :

- stocker au moins en `int32_t` dans le probe ;
- vérifier explicitement `0<=coord<=65535` avant Morton ;
- translater les fixtures négatives dans le cube u16 ;
- ajouter une gate proche de `65535` et une gate de translation exacte.

Le tri doit utiliser les mêmes coordonnées que la géométrie. Une clé calculée
avant un cast rétrécissant et un prédicat calculé après ce cast décrivent deux
nuages différents.

---

## 8. Ordonnance produit recommandée

### 8.1 Front WSPD conjoint

```text
CKPairTerminal(A,B,RectId,W4WitnessState)
  -> DEAD_W4 si lower_open_sat==8
  -> AcuteOwnerGateway(A,B,C)
  -> DEAD_NO_CARRIER si gateway DEAD
  -> ACTIVE_BLOCK si W4 entièrement vivant et carrier ALL
  -> MIXED sinon
  -> split déterministe parmi A/B/C/frontière W4.
```

Un `ACTIVE_BLOCK` reste symbolique jusqu’à ce que :

```text
pair_mass <= pair_tile_cap
et
carrier_conflict <= carrier_tile_cap.
```

Ensuite seulement, une microtuile est développée en registres/shared memory.

### 8.2 Exactification fusionnée par arête résiduelle

Pour chaque arête exacte de la microtuile, faire **un seul parcours LBVH** qui
classe chaque nœud relativement aux régions :

```text
W4 witness,
acute carrier,
neither/MIXED.
```

Le parcours :

- s’arrête dès huit témoins W4 ;
- n’émet des carriers que si l’ancre reste vivante ;
- transporte un overflow explicite si la liste carrier dépasse le cap local ;
- partage les AABB et les tests `H/Delta`.

Cette fusion est plus propre que :

```text
scan W4 complet,
puis nouvelle requête pour les carriers.
```

Elle ne remplace pas le gateway de bloc : `two_lines` doit mourir avant toute
arête exacte.

### 8.3 Quatrième sommet

Pour chaque arête vivante et sa liste de carriers :

```text
cover = LBVH antichain dans ||2z-a-b||² <= 4D
```

construit une seule fois. Puis chaque seed utilise :

```text
Q4SeedAxisTopR4-LBVH
```

avec :

- `SeedCoreQuarter` et `SeedJungPermanent16` comme certificats de mort ;
- bornes exactes de `A_z`, `B_z` ;
- top-k des racines entrantes/sortantes ;
- descente obligatoire sur les égalités ;
- aucun scan de tous les points et aucun `carrier×apex`.

### 8.4 Backend dense

Si une arête possède beaucoup de carriers et un cover large :

```text
carrier_count * cover_mass > switch_budget
```

basculer vers l’arrangement collectif `EdgeCenterShallowCut`. Le switch est une
optimisation ; les deux backends doivent rendre les mêmes `SupportKey`,
`BallKey`, `I_B/U_B` et fates.

---

## 9. ABI GPU concrète

Utiliser des SoA plates et des compteurs saturés :

```text
A_key[], B_key[], C_key[],
rect_id[], lane_mask[],
lower_sat[], upper_sat[],
front_offset[], front_count[],
logical_pair_mass[], logical_triple_mass[],
fate[], continuation[].
```

Une vague :

```text
classify -> child_count -> exclusive_scan -> fill.
```

Aucune `std::vector` par tâche, aucune allocation par seed. La frontière globale
est un pool de `NodeHandle`; chaque tâche n’en porte qu’un span. Les spills sont
des continuations transactionnelles, jamais des morts.

Politique de split simple et déterministe : évaluer un niveau virtuel pour les
facteurs admissibles et maximiser :

```text
masse classée DEAD/ACTIVE
-------------------------.
coût fixe + nombre d’enfants
```

Le tie-break doit être versionné. Les statistiques de tentative sont séparées
des fates terminaux.

---

## 10. Gates bloquantes du prochain commit

### G1 — contre-famille annulaire

```text
W4_count=0,
carrier_count=632,
anchor_alive=1,
no hard carrier cap,
overflow path exercised.
```

### G2 — fermeture WSPD universelle

Construire deux petits blocs endpoints très séparés et huit `PointId` dans un
petit cœur commun. Exiger :

```text
bloc_tout_w4 exercised,
dead_w4>=1,
PairId_expanded=0,
no double credit,
pending=0.
```

Cette fixture, et non `terrain`, reçoit causalement la branche. Le plancher
`dead_w4>=1` sur une famille aléatoire reste au mieux un contrôle de non-vacuité.

### G3 — bloc entièrement vivant avec carrier

Construire :

```text
upper_closed_sat<8,
AcuteOwnerGateway=ALL_STRICT,
ACTIVE_BLOCK>=1.
```

Puis comparer la masse logique et tous les triples ponctuels à l’oracle.

### G4 — `two_lines`

```text
V4_cross=Theta(n²),
PairId_cross_expanded=0,
Seed3_cross_emitted=0,
axis_jobs_cross=0,
pending=0.
```

### G5 — tétraèdre régulier

```text
Jung core does not contain apex,
axis top-k contains apex,
q4 event exact-once=1.
```

### G6 — parité complète petit `n`

Comparer :

```text
joint WSPD route
== exact pair/seed route
== BallFormRange brute
```

séparément sur :

```text
alive edge,
carrier,
SupportKey,
owner,
primary,
positivity,
BallKey,
I_B,
U_B,
rank,
lane.
```

### G7 — domaine u16

```text
translation invariant,
coordinates near 65535,
no narrowing,
Morton geometry agrees with predicate geometry.
```

---

## 11. Ordre de travail conseillé à Claude

1. **Corriger les affirmations**, sans retirer les mesures de `fc63408`.
2. Ajouter `live_anchor_many_carriers_annulus` et la gate apex-hors-cœur.
3. Recevoir `bloc_tout_w4` avec son oracle biaisé et ses strictes.
4. Extraire `W4WitnessState` et `JointPairCarrierJob` dans des composants
   partagés ; ne pas fusionner deux exécutables monolithiques.
5. Initialiser la jointure depuis les terminaux WSPD, avec le ledger existant.
6. Garder les frontières `MIXED` grossières et saturées ; ne pas les développer
   jusqu’aux points.
7. Obtenir les gates G2--G4 avant toute pente.
8. Implémenter ensuite `Q4SeedAxisTopR4-LBVH` sur les microtuiles résiduelles.
9. Porter sur CUDA seulement l’ABI reçue : SoA, wavefront, count--scan--fill.

La priorité immédiate n’est donc pas un nouveau certificat. C’est le raccord
correct de deux briques déjà utiles, avec les bonnes unités et sans transformer
une frontière de `NodeHandle` en liste de singletons par enthousiasme
administratif.

---

## 12. Statut

- `fc63408` : **mesure reçue**, interprétation `O(h)` déterministe rejetée ;
- `f614b74` : **diagnostic d’inertie reçu**, branche `DEAD_W4` non encore reçue ;
- insertion dans les terminaux WSPD : **prochaine architecture recommandée** ;
- top-k axial LBVH : **reste la bonne source du quatrième sommet** ;
- cœur de Jung : **certificat d’intérieurs permanents uniquement** ;
- claim de sparsité globale : **non reçu**, en attente des gates et pentes
  physiques après la jointure.
