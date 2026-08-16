# Audit consolidé q2/q3/q4 — scheduler pair-major après `79e73b6`

Date : 16 août 2026 UTC.  
Dossier : `morsehgp3D_v3/`.  
Dernier commit fonctionnel audité : `5a225f3d770c2effa68804cf84bfcff766077070`.  
Rétractations relues : `b62d1f0292e673fdeb403495850cea8cf9d5e769`.  
Dernier audit contre-audité : `79e73b6e5d0745e03cba6b342f971c1c79f77510`.

Complète notamment :

- [`AUDIT_REPONSE_5A225_PAIR_MAJOR_FRONTIER_20260816.md`](AUDIT_REPONSE_5A225_PAIR_MAJOR_FRONTIER_20260816.md) ;
- [`AUDIT_CONSTRUCTIF_FC634_F614_JONCTION_WSPD_LBVH_20260816.md`](AUDIT_CONSTRUCTIF_FC634_F614_JONCTION_WSPD_LBVH_20260816.md) ;
- [`NOTE_AUDITEUR_LBVH_SPARSE_Q3_Q4_APRES_53815F_20260816.md`](NOTE_AUDITEUR_LBVH_SPARSE_Q3_Q4_APRES_53815F_20260816.md) ;
- [`PROPOSITION.md`](../PROPOSITION.md).

Cadre :

```text
phase=exploration_v3_hors_registre
backend=math_reference_and_gpu_architecture
profile=quantized_u16_input_only
mode=consolidated_pair_major_q2_q3_q4_audit
public_status=not_claimed
```

> [!IMPORTANT]
> **Verdict général.** Le pivot des derniers commits est positif et substantiel.
> La combinaison
>
> ```text
> partition exacte des paires
> + ledger universel de profondeur
> + relation carrier propre à chaque lane
> + réduction axiale q4
> ```
>
> est la première architecture du dossier qui puisse raisonnablement devenir un
> générateur exact et physiquement sparse, plutôt qu’un énumérateur dense muni
> d’un préfiltre élaboré.
>
> La prochaine priorité n’est plus d’inventer un nouveau certificat géométrique.
> Il faut implémenter correctement le **quotient pair-major**, conserver
> l’autonomie de q2/q3/q4, puis reconnecter q4 à `Q4SeedAxisTopR4-LBVH`.

---

## 1. État exact du delta audité

Au pin `79e73b6`, la séquence pertinente est :

```text
5a225f3  dernier commit fonctionnel du gateway conjoint
b62d1f0  deux rétractations mathématiques et leurs fixtures
79e73b6  audit proposant PairState + frontière interne
```

Il n’existe donc pas encore de code pair-major postérieur à auditer. Le présent
rapport contre-audite l’architecture proposée et fixe un ordre d’implémentation
pour Claude.

Le contrat théorique reste celui des premières parties du manuscrit : produire
les interactions positives et suffisamment peu profondes qui engendrent les
`K`-polyèdres, afin de reconstruire la hiérarchie des composantes de forte
densité de l’estimateur `K`-NN, sans construire de Delaunay d’ordre supérieur.
La parcimonie n’est pas un objectif décoratif : elle doit préserver exactement
les supports, leur niveau, le shell, la provenance et le fold hiérarchique.

---

## 2. Réception positive des derniers commits

### 2.1 Raccord WSPD du gateway

Le raccord du gateway depuis les rectangles WSPD est conceptuellement correct.
Il apporte trois résultats réels :

1. le ledger `W4` devient actif sur des facteurs endpoint déjà séparés ;
2. les preuves sont héritées lorsque seul le facteur témoin `C` change ;
3. `two_lines` est éliminé sans matérialiser de `PairId`.

La gate :

```text
two_lines:
  pairid_expanded=0
  carriers=0
  carriers_symboliques=0
  active_edge=0
  seed3_emitted=0
```

est reçue comme un progrès important.

Le chemin actuel doit rester compilable sous une option explicite, par exemple :

```text
--scheduler=triple-task
```

Il servira de référence sémantique et de baseline de coût pendant la transition.

### 2.2 Rétractation de la borne déterministe `O(h)`

La rétractation de `b62d1f0` est entièrement reçue.

Une ancre `W4`-vivante peut avoir `Theta(n)` carriers. Dans la contre-fixture
annulaire, les régions sont séparées par le signe de `H` :

```text
W4 intérieur : H>0
carrier aigu : H<0
```

La vivacité de l’ancre ne borne donc pas déterministement le nombre de carriers.
Le rapport de volumes ne contrôle qu’une moyenne sous hypothèse d’homogénéité.

Conséquences bloquantes :

- aucun cap de carriers proportionnel à `h4` ;
- aucune capacité GPU dimensionnée sur la moyenne ;
- tout overflow doit produire une continuation ou `resource_exhausted` ;
- le chemin q4 doit posséder un backend collectif ou spillable pour les arêtes
  à très grand nombre de carriers.

### 2.3 Rétractation du cœur de Jung comme source d’apex

La seconde rétractation est également reçue.

Le cœur de Jung sert à trouver des **intérieurs permanents** communs à toutes les
boules admissibles, donc à tuer un seed lorsqu’il atteint le seuil de profondeur.
Il ne contient pas en général le quatrième sommet. Celui-ci vit sur le shell
d’une sphère particulière et doit être trouvé par la géométrie axiale.

La route correcte est :

```text
cœur de Jung
  -> rejet éventuel par 8 intérieurs permanents
sinon
  -> Q4SeedAxisTopR4-LBVH
  -> premières/dernières racines axiales
```

Le tétraèdre régulier doit rester une fixture permanente de cette séparation.

---

## 3. Deux P0 à corriger avant le scheduler pair-major

### 3.1 P0 : le masque endpoint est relationnel

Le code courant descend un span témoin qui intersecte `A` ou `B`, puis jette les
feuilles correspondantes comme endpoints. Cette opération est sûre pour un
minorant universel, mais pas pour une borne supérieure ni pour l’héritage après
restriction endpoint.

Un ID `z in A` est endpoint seulement pour les paires dont le premier endpoint
vaut `z`. Pour une autre paire `(a,b)` avec `a != z`, il peut être un témoin
légitime. Après restriction à un enfant `A'` ne contenant plus `z`, il doit
redevenir un témoin ordinaire.

La règle correcte est :

```text
overlap endpoint
  -> jamais crédité dans lower_open
  -> conservé dans upper_open / upper_closed
  -> rejoué après toute restriction de A ou B
```

ABI minimale recommandée :

```cpp
enum class RelationMask : uint8_t {
  DISJOINT,
  OVERLAP_A,
  OVERLAP_B,
  OVERLAP_BOTH
};

struct WitnessSpan {
  NodeKey node;
  RelationMask relation;
  WitnessStatus status;
};
```

Un span relationnel ne peut être retiré que lorsqu’un masque exact prouve que
tous ses vrais `PointId` sont endpoints pour toutes les paires restantes.

Gate minimale :

```text
A={a0,a1}, B={b}, z=a1
```

avec une géométrie où `z` est témoin pour `(a0,b)` :

```text
parent A×B       : z absent de lower, présent dans upper
child {a0}×B     : z redevient témoin ordinaire
child {a1}×B     : z est masqué comme endpoint
```

Mutants à tuer :

```text
endpoint-overlap-dropped-from-upper
endpoint-mask-inherited-without-replay
```

### 3.2 P0 : le juge par cardinalité ne prouve pas la couverture

Le test global :

```text
sparse_count >= brute_count
```

est insuffisant. Une incidence vraie manquante peut être compensée par une
incidence surnuméraire provenant d’une ancre morte.

Le juge petit `n` doit comparer les identités :

```text
Target_q4 = {
  (EdgeKey(a,b), PointId(x)) :
  (a,b) passe exactement le prune universel q4
  et x est carrier canonique de (a,b)
}
```

Exigences :

```text
missing_target=0
duplicate_target=0
false_symbolic_triple=0
```

La surcouverture due aux ancres non éliminées doit être publiée séparément :

```text
overcoverage_dead_anchor
```

Les blocs symboliques doivent marquer les clés qu’ils couvrent. Un total global
ne remplace pas une couverture par identités.

### 3.3 Nuance sur `U4=L4+frontiere.size()`

Dans le probe actuel non tronqué, la frontière est descendue jusqu’aux feuilles.
Par conséquent, `frontiere.size()` compte bien des IDs et n’est pas, dans cette
version précise, une sous-estimation de population.

Le défaut industriel est ailleurs : descendre systématiquement aux feuilles
reconstitue un CSR de points et ne passe pas à l’échelle.

La version produit doit conserver une antichaîne grossière et calculer :

```text
upper = lower + sum(population(span possible))
```

avec saturation au seuil. Il faut donc changer la structure de données, pas
attribuer au compteur actuel une faute qu’il n’a pas.

---

## 4. Contre-audit de l’architecture `PairState`

Le diagnostic principal de `79e73b6` est juste :

```text
Tache(A,B,C,...)
```

est le mauvais état matériel. Quand plusieurs branches `C` demandent la même
scission endpoint, le code construit plusieurs copies du même raffinement. Le
coût devient celui d’un produit d’arbres.

La bonne organisation est pair-major : `C` devient une frontière interne et une
scission endpoint est décidée une seule fois pour tout l’état.

Quatre corrections sont cependant nécessaires avant d’en faire le contrat.

### 4.1 Ne pas partager un état mutable entre q2, q3 et q4

Le dossier impose l’autonomie des producteurs.

Partage autorisé :

- `PointStore` ;
- ordre Morton et LBVH ;
- partition neutre des paires ;
- AABB et primitives géométriques pures ;
- caches immuables.

Partage interdit :

- fates ;
- seuils et caps ;
- continuations ;
- preuves de complétude ;
- listes de supports retenus ;
- rangs ou census d’une autre lane.

La bonne abstraction est :

```text
PairFrame commun et immuable
  + Lane2State autonome
  + Lane3State autonome
  + Lane4State autonome
```

ABI recommandée :

```cpp
struct PairFrame {
  RectId rect_id;
  NodeKey a_node;
  NodeKey b_node;
  uint64_t pair_mass;
  GeometryCacheHandle geometry;
};

struct PairLaneState {
  uint8_t q;
  uint8_t reject_threshold;

  uint8_t lower_strict_sat;
  uint8_t upper_open_sat;
  uint8_t upper_closed_sat;

  SpanRange decided_spans;
  SpanRange mixed_spans;
  SpanRange relation_spans;

  ContinuationHandle continuation;
  LaneFate fate;
};
```

Un même kernel peut calculer plusieurs prédicats et écrire plusieurs masques.
Une fermeture q4 ne doit jamais annuler le travail q3.

### 4.2 Conserver `upper_open` et `upper_closed`

Il faut distinguer :

```text
upper_open
  nombre maximal possible de témoins strictement intérieurs

upper_closed
  nombre maximal possible de témoins intérieurs ou shell
```

Le premier sert à la profondeur stricte. Le second sert au shell, aux plateaux
et aux reçus finaux.

Le classifieur doit donc distinguer :

```text
ALL_INTERIOR
NONE_INTERIOR_OPEN
OUTSIDE_CLOSED
MIXED
```

et les réductions doivent conserver les deux majorants. Employer uniquement
`upper_closed` est sûr, mais abandonne une information déjà payée.

Sous `smax=11` :

```text
h2=10
h3=9
h4=8
```

Ces seuils doivent être paramétrés par lane et non codés autour du seul huit q4.

### 4.3 Le court-circuit q4 ne concerne que l’activation

Le premier `ALL_CARRIER` peut prouver :

```text
pour toute paire du bloc, il existe au moins un carrier
```

Il peut donc arrêter la recherche **existentielle** d’activation q4.

Il ne peut pas arrêter l’énumération complète des carriers, car d’autres
carriers peuvent produire d’autres seeds et d’autres supports q4.

Il faut séparer :

```text
CarrierExistenceFrontier
CarrierEnumerationRoot
```

ABI recommandée :

```cpp
struct Lane4CarrierGate {
  PointId uniform_carrier_id;
  NodeHandle carrier_search_root;
  CarrierCoverageHandle coverage;
  SpanRange mixed_activation_spans;
};
```

Le certificat doit transporter un vrai `PointId` disjoint de `A union B`, pas
seulement un nœud géométriquement non vide.

Après activation, l’arête doit conserver un handle vers le domaine complet des
carriers pour l’étage q4 réel.

### 4.4 Ne pas appeler « vivante exacte » une paire seulement libérée du cœur universel

Pour q2, la boule diamétrale est la boule canonique du support.

Pour q3/q4, `W3/W4` sont des cœurs universels communs aux boules admissibles. Ils
fournissent une preuve suffisante de mort, mais leur faible profondeur ne prouve
pas qu’une circumboule particulière sera shallow.

Les fates devraient donc être nommés :

```text
PRUNED_BY_UNIVERSAL_DEPTH
CORE_CLEAR
MIXED_CORE
```

plutôt que `DEAD/LIVE_EXACT`, sauf pour q2.

Le census de la miniboule propre au support reste obligatoire après génération.

---

## 5. Architecture recommandée des trois générateurs

```text
NeutralPairPartition
  |
  `-- PairFrame(RectId,A,B)
        |
        +-- Lane2State
        +-- Lane3State
        `-- Lane4State
```

Le `RectId` WSPD reste le propriétaire immuable des paires. Les raffinements
ultérieurs subdivisent son produit ; ils ne reconstruisent pas une seconde WSPD.

### 5.1 Générateur q2

```text
PairFrame
  -> distinct PointId relation
  -> Midball/W2 universal depth, reject_at_10
  -> PRUNED_Q2 ou Q2_PAIR_BLOCK
  -> exactification sous cap
  -> BallForm diamètre
  -> census exact I_B/U_B
  -> BallKey/RLE
  -> fold
```

Il n’existe aucun facteur carrier.

Un bloc peut rester symbolique tant que l’aval accepte cette factorisation. Dès
que les `BallKey` individuelles sont nécessaires, les paires sont développées
par tuiles sous un contrat output-sensitive.

### 5.2 Générateur q3

```text
PairFrame
  -> W3 universal-core ledger, reject_at_9
  -> CarrierBlock(A,B,C)
       owner canonique
       triangle strictement aigu
       trois PointId distincts
  -> miniboule q3 par Gram/Cramer
  -> census exact
  -> BallKey/RLE
  -> fold
```

La relation carrier q3 est énumérative : chaque carrier peut produire un support
q3 distinct.

Conséquences :

- aucun court-circuit sur le premier `ALL_CARRIER` ;
- un `ALL_CARRIER` représente un bloc logique de supports q3 ;
- ce bloc peut rester symbolique temporairement, mais tous ses supports doivent
  finir par être produits, spillés ou refusés transactionnellement.

Des familles à sortie quadratique existent. Le générateur q3 doit donc être
output-sensitive, sans promesse universelle sous-quadratique.

### 5.3 Générateur q4

```text
PairFrame
  -> W4 universal-core ledger, reject_at_8
  -> existence d’au moins un carrier
  -> ACTIVE_Q4_EDGE_BLOCK
  -> microtuiles d’arêtes exactes
  -> énumération complète des carriers par arête
  -> pour chaque seed aigu :
       cœur de Jung = rejet par intérieurs permanents
       sinon Q4SeedAxisTopR4-LBVH
  -> owner parmi les six arêtes
  -> primary carrier
  -> positivité tétraédrique
  -> BallKey
  -> census exact
  -> fold
```

Les deux quotients utiles sont complémentaires :

```text
PairFrame
  supprime le produit endpoint × témoin

Q4SeedAxisTopR4
  supprime le produit carrier × apex
```

Quand une arête possède peu de carriers :

```text
seed-major axial
```

Quand elle en possède beaucoup :

```text
EdgeCenterShallowCut collectif
ou continuation/spill reçu
```

Le backend collectif n’est pas optionnel à cause de la contre-fixture annulaire.

---

## 6. Invariants mathématiques à graver

### 6.1 Exact-once de la partition des paires

Chaque paire de feuilles possède un LCA unique dans le radix tree et appartient
à un unique produit entre sous-arbres frères. La récursion WSPD remplace ensuite
chaque produit par des produits enfants disjoints.

Par induction :

```text
chaque PairId non ordonné appartient à un unique PairFrame terminal
```

Gate indépendante :

```text
for every unordered PairId:
  occurrences == 1
```

Elle doit inclure les positions dupliquées : mêmes coordonnées, vrais
`PointId` distincts, aucune déduplication silencieuse.

### 6.2 Antichaîne de témoins

Pour chaque lane, les spans actifs forment une antichaîne du LBVH :

- aucun couple ancêtre-descendant simultané ;
- populations additives ;
- remplacement atomique d’un parent par tous ses enfants ;
- conservation exacte de la masse logique.

Cette propriété est nécessaire aux crédits et aux majorants.

### 6.3 Monotonie sous restriction endpoint

Pour `A' subset A`, `B' subset B` :

```text
ALL                  reste ALL
OUTSIDE/NONE         reste OUTSIDE/NONE
MIXED                doit être rejoué
MIXED_ENDPOINT       doit être rejoué
```

Les preuves géométriques universelles sont monotones. Les masques relationnels
peuvent devenir plus précis et ne doivent pas être hérités sans replay.

### 6.4 Preuve contre politique de split

La politique :

```text
split A
split B
split C
exactify
```

n’intervient jamais dans la correction. Elle choisit seulement l’ordre d’une
partition exhaustive.

Tout cap ou overflow donne :

```text
PENDING_RESOURCE
```

avec continuation sérialisable. Jamais `DEAD`, jamais `ACTIVE` par défaut.

### 6.5 Provenance q4

Quand un tétraèdre possède plusieurs préfixes aigus admissibles, la provenance
primaire est choisie par une règle totale sur les vrais `PointId`, après
connaissance des quatre sommets.

Elle ne doit dépendre ni de l’ordre Morton, ni de l’ordre d’arrivée des jobs.

---

## 7. Ordre d’implémentation recommandé à Claude

### Commit 1 — rendre l’oracle actuel fiable

Sur `--scheduler=triple-task` :

1. remplacer la suppression endpoint par `MIXED_ENDPOINT` ;
2. rejouer le masque relationnel après split `A/B` ;
3. comparer les ensembles `(EdgeKey,PointId)` ;
4. publier l’overcoverage séparément ;
5. ajouter le juge exact de la partition des paires.

Aucune optimisation avant cette étape.

### Commit 2 — introduire `PairFrame`

Créer :

```text
PairFrame
PairWitnessJob
PairLaneState
```

sans modifier encore les décisions géométriques.

Conserver le scheduler historique compilable.

### Commit 3 — scheduler pair-major q4 CPU

Implémenter :

```text
classify
segmented reduce
one action per PairFrame/Lane4State
count
exclusive scan
fill
```

Contraintes :

```text
une seule décision endpoint par état
C uniquement dans la frontière
aucun look-ahead complexe au premier passage
```

### Commit 4 — dual upper et activation existentielle

Ajouter :

```text
upper_open
upper_closed
uniform_carrier_id
carrier_search_root
```

Le court-circuit arrête uniquement les jobs d’activation carrier.

### Commit 5 — Lane2 autonome

Brancher q2 sur le même `PairFrame`, avec état, fate et continuation propres.

### Commit 6 — Lane3 autonome

Conserver les `CarrierBlock` q3 et ajouter une gate prouvant que l’activation q4
ne change aucune sortie q3.

### Commit 7 — reconnecter la chaîne q4 complète

```text
active edge
  -> full carrier traversal
  -> Jung permanent-interior kill
  -> axial top-r
  -> owner6 / primary
  -> positive support
  -> BallKey / census
```

### Commit 8 — CUDA

Porter seulement après parité CPU et réception des compteurs physiques.

Kernels candidats :

```text
classify_joint_kernel
segmented_reduce_kernel
decide_action_kernel
count_children_kernel
exclusive_scan
fill_next_wave_kernel
```

Disposition SoA, aucune allocation dynamique par état.

---

## 8. Gates bloquantes

### G1 — partition exacte des paires

```text
pair_partition_exact_once:
  every unordered PairId occurs once
```

### G2 — positions dupliquées

```text
same coordinates, distinct PointId:
  no support quotient
  no witness multiplicity loss
```

### G3 — replay relationnel endpoint

```text
parent overlap remains in upper
nonincident child restores witness
incident child masks endpoint
```

### G4 — juge par identités

Construire une faute où une incidence vraie est omise et une fausse ajoutée :

```text
same total count
missing_target > 0
false_symbolic_triple > 0
```

Le juge doit échouer.

### G5 — `upper_open` contre `upper_closed`

Fixture riche en shell :

```text
upper_open < upper_closed
strict-depth fate correct
shell provenance conserved
```

### G6 — une seule scission endpoint

Plusieurs spans `C` mixtes demandent le même split de `A` :

```text
endpoint_splits=1
unique_pair_state_keys=2
endpoint_split_replays=0
```

### G7 — carriers différents selon les paires

Chaque paire du parent possède un carrier, mais aucun carrier n’est commun à
toutes les paires :

```text
parent remains MIXED
shared endpoint split occurs once
children obtain distinct certificates
```

### G8 — indépendance q3/q4

```text
q4 existential shortcut ON/OFF
q3 support identity unchanged
```

### G9 — contre-fixture annulaire

```text
632 carriers
0 W4 witnesses
no O(h) cap
continuation or collective backend succeeds
```

### G10 — tétraèdre régulier

```text
Jung core does not contain apex
axis route recovers apex
owner/primary exact
```

### G11 — reprise après cap

```text
capped run + resume
  == uncapped run
```

sur les ensembles q2/q3/q4, leurs shells et leurs `BallKey`.

### G12 — parité finale petit `n`

```text
PairFrame source
  == exhaustive support oracle
  == BallFormRange/census outputs
```

comparaison par identités, pas par cardinalité.

---

## 9. Télémétrie à publier

Le compteur générique `noeuds` mélange plusieurs unités et ne doit plus recevoir
un exposant.

Publier séparément :

```text
pair_frames[q]
pair_witness_jobs[q]
endpoint_splits[q]
witness_splits[q]
relation_replays[q]

wq_classifier_calls[q]
corner_tests[q]
symbolic_mass[q]

exact_pair_tiles[q]
carrier_blocks_q3
carrier_exists_jobs_q4
carriers_enumerated_q4
axis_roots_q4

events_out[q]
pending_mass[q]
bytes_peak
wall_time
```

Trois notions doivent rester distinctes :

1. **représentation sparse** : la masse logique reste factorisée ;
2. **travail sparse** : peu de jobs physiques sont visités ;
3. **sortie sparse** : peu de `BallEvent` sont produits.

La première paraît désormais atteignable. La seconde doit être mesurée après le
scheduler pair-major. La troisième est fausse sur certaines familles
adversariales et ne doit pas être revendiquée universellement.

---

## 10. Statut consolidé

| Objet | Verdict |
|---|---|
| raccord WSPD du gateway | reçu positivement |
| gate `two_lines` sans `PairId` | reçue positivement |
| rétractation `carriers=O(h)` déterministe | reçue |
| rétractation cœur de Jung comme apex | reçue |
| extrema exacts du gateway aigu | reçus dans leur portée |
| masque endpoint actuel | non reçu |
| juge `sparse>=brute` par total | non reçu |
| scheduler triple-task | oracle à conserver, architecture produit refusée |
| `PairState` pair-major | direction recommandée |
| état mutable partagé q2/q3/q4 | refusé |
| `upper_open` + `upper_closed` | requis |
| court-circuit carrier q4 | reçu comme gate d’activation uniquement |
| énumération complète q3 | obligatoire et output-sensitive |
| énumération complète des carriers q4 | obligatoire après activation |
| backend collectif pour arêtes lourdes | obligatoire |
| claim de complexité sparse | en attente des gates et mesures physiques |

---

## Conclusion à Claude

Tu as maintenant isolé les deux vrais produits qu’il faut quotienter :

```text
endpoint × témoin
carrier × apex
```

Le premier relève du scheduler pair-major ; le second de la géométrie axiale.
La WSPD donne la propriété exacte des paires, le LBVH donne l’index spatial, les
cœurs universels donnent des morts sûres, et les miniboules propres aux supports
donnent les événements finaux.

La route la plus solide est donc :

```text
1. réparer les identités et les masques relationnels ;
2. implémenter PairFrame + états autonomes de lane ;
3. recevoir q4 pair-major sur CPU ;
4. reconnecter l’axe et le census ;
5. porter la wavefront reçue sur CUDA.
```

Il ne faut pas chercher à prouver que toutes les sorties sont petites : c’est
faux. Il faut garantir que le travail est factorisé tant que possible, exact
quand il est développé, output-sensitive quand la sortie est lourde, et
transactionnel lorsqu’une ressource manque.

Mathématiquement, cette stratégie est cohérente avec l’objet HGP du manuscrit et
avec les contre-exemples désormais gravés. C’est aujourd’hui la meilleure route
du dossier vers un générateur q2/q3/q4 exact, GPU-compatible et réellement
parcimonieux sur les nuages d’intérêt, sans vendre au compilateur une borne que
la géométrie a déjà réfutée.
