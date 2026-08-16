# Réponse de l’auditeur à `5a225f3` — ne figer ni ne rescinder par `C`

Date : 16 août 2026 UTC.  
Pin fonctionnel audité : `5a225f3d770c2effa68804cf84bfcff766077070`.  
Dossier : `morsehgp3D_v3/`.

Complète :

- [`NOTE_CLAUDE_GATEWAY_TERNAIRE_20260816.md`](NOTE_CLAUDE_GATEWAY_TERNAIRE_20260816.md), section 8 ;
- [`AUDIT_CONSTRUCTIF_FC634_F614_JONCTION_WSPD_LBVH_20260816.md`](AUDIT_CONSTRUCTIF_FC634_F614_JONCTION_WSPD_LBVH_20260816.md) ;
- [`NOTE_AUDITEUR_LBVH_SPARSE_Q3_Q4_APRES_53815F_20260816.md`](NOTE_AUDITEUR_LBVH_SPARSE_Q3_Q4_APRES_53815F_20260816.md).

Cadre : `phase=exploration_v3_hors_registre`,
`backend=math_reference_and_gpu_architecture`,
`profile=quantized_u16_input_only`,
`mode=pair_major_joint_frontier_design`,
`public_status=not_claimed`.

> [!IMPORTANT]
> **Réponse à la question de Claude.** Il ne faut ni figer définitivement
> `(A,B)`, ni continuer à le raffiner indépendamment dans chaque tâche
> `(A,B,C)`.
>
> La bonne unité est un **état de paire possédé par un rectangle WSPD**, qui
> transporte toute sa frontière de témoins :
>
> ```text
> PairState(RectId,A,B,
>           W4ThresholdState,
>           AcuteExistenceState,
>           JointWitnessFrontier).
> ```
>
> `C` n’est plus un facteur de tâche. C’est une antichaîne interne au
> `PairState`. Une scission de `C` modifie cette frontière. Une scission de `A`
> ou `B` est décidée **une seule fois pour l’état entier**, puis ses enfants
> héritent de la frontière et des preuves. Le code actuel décide la même
> scission endpoint séparément pour plusieurs branches `C`, ce qui construit un
> produit de sous-arbres et explique les exposants rouges.
>
> La seconde correction est tout aussi importante : pour q4, le front ne doit
> pas énumérer tous les carriers. Il cherche d’abord un certificat
> **existentiel** : un seul nœud `C` classé `ALL_CARRIER` active toutes les paires
> du bloc déjà certifiées vivantes. Les autres branches `C` sont alors annulées.
> L’ancien test « `(A,B)` figé n’élague que des feuilles » mesurait
> l’énumération de toute la relation carrier, pas cette recherche existentielle.
> Il ne tranche donc pas la nouvelle architecture.

---

## 1. Réception positive de `5a225f3`

Le raccord depuis les rectangles WSPD est le bon raccord. Il rend enfin le
ledger `W4` actif, élimine `two_lines` sans `PairId`, et corrige les deux défauts
internes annoncés :

- troncature réévaluée après restriction de `(A,B)` ;
- ledger non recalculé lorsque seul `C` change.

La gate :

```text
two_lines:
  active_edge=0,
  seed3_emitted=0,
  pairid_expanded=0
```

est reçue comme un progrès réel.

Le surcomptage de `11` à `18 %` est acceptable pour un proposer fail-open. Il ne
constitue cependant pas encore un juge de complétude. La section suivante est
bloquante avant toute interprétation des nouvelles pentes.

---

## 2. P0 de correction : le masque des endpoints est relationnel

### 2.1 Le problème

Le ledger courant supprime un nœud témoin qui intersecte `A` ou `B` :

```text
si C intersecte A ou B :
    descendre C ;
    à la feuille, jeter le PointId.
```

Cette opération est sûre pour le **minorant universel** `L4`, mais pas pour la
borne supérieure `U4`, ni pour l’héritage après scission endpoint.

Dans un bloc `A×B`, un point `z in A` n’est endpoint que pour les paires dont le
premier endpoint vaut `z`. Pour toute autre paire `(a,b)` avec `a != z`, il peut
être un témoin parfaitement légitime. Lorsque `A` est ensuite restreint à un
fils ne contenant plus `z`, ce point doit même redevenir un témoin ordinaire.
Le jeter du parent le rend irrécupérable.

Formellement, le minorant universel porte sur :

```text
L(A,B)
 = {z : pour tout a in A, b in B,
          z != a,b et z est W4-interieur de (a,b)}.
```

Un point de `A union B` ne peut appartenir à `L(A,B)`, puisque l’un des choix
d’endpoint l’annule. En revanche, la borne supérieure doit porter sur :

```text
U(A,B)
 = {z : il existe a in A, b in B,
          z != a,b et z peut être W4-interieur de (a,b)}.
```

Des points de `A union B` peuvent appartenir à `U(A,B)`.

### 2.2 Correctif minimal sûr

Tant que `A` ou `B` n’est pas singleton, un nœud témoin qui intersecte un facteur
endpoint doit rester :

```text
MIXED_ENDPOINT
```

et contribuer à `upper_closed_sat`, jamais à `lower_open_sat`.

Il ne peut être supprimé que lorsqu’un masque relationnel prouve que chaque
`PointId` du nœud est endpoint pour toutes les paires restantes. En pratique,
cela n’arrive pour un vrai ID qu’après singletonisation de l’endpoint concerné.

La version produit peut représenter :

```text
WitnessSpan {
  NodeKey C;
  RelationMask mask;  // overlap-A, overlap-B, disjoint
  W4Status w4;
  AcuteStatus acute;
}
```

La population utilisée dans `U4` est celle des vrais IDs après masque exact,
saturée au seuil huit.

### 2.3 Conséquence pour l’héritage

Lors d’une restriction `A' subset A`, `B' subset B` :

- `ALL_W4`, `NONE_W4`, `ALL_CARRIER`, `NONE_CARRIER` restent vrais ;
- les spans `MIXED_ENDPOINT` sont rejoués, car leur masque peut devenir plus
  précis ;
- aucun ID retiré pour cause d’endpoint au parent n’est perdu.

### 2.4 Gate minimale

Prendre :

```text
A={a0,a1}, B={b}, z=a1.
```

Choisir la géométrie de sorte que `z` soit un témoin W4 pour `(a0,b)`. Exiger :

```text
parent A×B : z non crédité dans L4, mais présent dans U4 ;
child {a0}×B : z redevient témoin ordinaire ;
child {a1}×B : z est masqué comme endpoint ;
```

et tuer :

```text
endpoint-overlap-dropped-from-upper,
endpoint-mask-inherited-without-replay.
```

---

## 3. P0 du juge : `sparse >= brute` en cardinalité ne prouve pas la couverture

Le chemin courant accepte :

```text
total_sparse >= total_brute.
```

Mais `total_sparse` contient aussi des carriers provenant d’ancres mortes. Une
vraie incidence vivante manquante peut être compensée numériquement par une
incidence supplémentaire issue d’une ancre morte. Le total reste vert, le
support est perdu.

Le juge petit `n` doit comparer les identités :

```text
Target = {(EdgeKey(a,b), PointId x) :
          (a,b) W4-vivante exacte et x carrier canonique}.
```

Chaque bloc symbolique et chaque feuille produite marquent les clés qu’ils
couvrent. Exiger :

```text
missing_target = 0,
duplicate_target = 0,
false_symbolic_triple = 0.
```

L’overcoverage provenant d’ancres mortes est comptée séparément :

```text
overcoverage_dead_anchor.
```

Pour chaque bloc `DEAD_W4`, rejouer en plus toutes ses `PairId` à petit `n` et
exiger qu’elles possèdent chacune au moins huit témoins W4. Pour chaque
`ACTIVE_ALL` présenté comme « toutes les paires vivantes », rejouer également
la vivacité de chaque paire. Un total global ne remplace pas ces reçus locaux.

---

## 4. Le diagnostic exact du coût actuel

Le stack courant stocke une tâche par triplet de nœuds :

```text
Tache(A,B,C,L4,frontiere,...).
```

Lorsqu’un même bloc endpoint `(A,B)` possède plusieurs branches `C1,...,Cm`, la
politique de taille peut décider dans chaque branche :

```text
split A -> A0/A1.
```

On construit alors `m` copies indépendantes du même raffinement endpoint. Aux
niveaux suivants, les partitions de `(A,B)` peuvent même diverger selon `C`.
Le coût physique devient celui d’un arbre produit, pas celui d’une partition de
paires suivie d’une relation témoin.

Cela explique pourquoi « le rectangle WSPD est déjà la partition » et
« raffiner A/B renforce les certificats » paraissent contradictoires. Les deux
sont vrais : le raffinement est utile, mais il doit être **unique et partagé**.

Le champ `noeuds` mélange toujours :

- les jobs `(A,B,C)` ;
- les évaluations de coins effectuées par `bloc_tout_w4`.

Avant une nouvelle pente, séparer au minimum :

```text
pair_states,
pair_witness_jobs,
w4_corner_triplet_tests,
acute_extrema_tests,
endpoint_splits,
witness_splits.
```

L’explosion observée est réelle, mais son exposant actuel additionne plusieurs
unités.

---

## 5. La troisième voie : `PairState` avec frontière interne

### 5.1 ABI conceptuelle

```text
PairState {
  RectId rect;
  NodeKey A, B;
  uint64 pair_mass;

  uint8 w4_lower_sat;   // min(8, population ALL_W4)
  uint8 w4_upper_sat;   // min(8, lower + population W4-possible)

  NodeHandle carrier_certificate;  // un C non vide ALL_CARRIER, ou NONE
  bool carrier_possible;

  SpanRange decided_proofs;
  SpanRange mixed_frontier;
  Continuation continuation;
  Fate fate;
}
```

La frontière est une antichaîne de nœuds témoins. `C` n’est plus dans la clé de
l’état endpoint.

### 5.2 Classification jointe d’un nœud témoin

Pour chaque `(PairState,C)`, calculer deux verdicts sûrs :

```text
W4Status:
  ALL_W4
  NONE_W4_OPEN
  OUTSIDE_W4_CLOSED
  MIXED_W4

AcuteStatus:
  ALL_CARRIER
  NONE_CARRIER
  MIXED_CARRIER
```

Les deux prédicats utilisent les mêmes AABB et une partie des mêmes produits.
Ils doivent être calculés dans une primitive partagée, pas dans deux traversals.

Les régions `ALL_W4` et `ALL_CARRIER` sont disjointes par le signe de `H`, mais
le statut `MIXED` peut contenir les deux types d’IDs.

### 5.3 Réduction par état

Après classification de toute la frontière :

```text
w4_lower_sat
  = min(8, somme des populations ALL_W4)

w4_upper_sat
  = min(8, w4_lower_sat
           + somme des populations non certifiées OUTSIDE_W4_CLOSED)
```

Les populations sont celles d’IDs disjoints avec masque relationnel.

Pour la clause carrier, q4 a une sémantique existentielle :

```text
carrier_certificate existe
  dès qu’un nœud non vide est ALL_CARRIER.
```

À partir de là, les autres branches carrier ne sont plus visitées pour cet état.
Le handle du certificat est hérité par tous les descendants endpoint.

Si aucun `ALL_CARRIER` n’existe et que tous les nœuds sont `NONE_CARRIER`, le
bloc est `DEAD_NO_CARRIER`.

### 5.4 Fates terminales

```text
w4_lower_sat == 8
  -> DEAD_W4

carrier impossible
  -> DEAD_NO_CARRIER

w4_upper_sat < 8 et carrier_certificate valide
  -> ACTIVE_EDGE_BLOCK

pair_mass <= pair_tile_cap
  -> EXACT_PAIR_TILE

sinon
  -> REFINE
```

`ACTIVE_EDGE_BLOCK` prouve uniquement que chaque paire du bloc est vivante et
possède au moins un carrier. Il ne publie ni tous les carriers, ni un support q4.
La liste exacte de carriers n’est demandée qu’après microdéveloppement d’une
arête ou via le backend collectif d’espace des centres.

---

## 6. Pourquoi le court-circuit existentiel change le résultat du test `(A,B)` figé

Le test négatif de `53815f` répondait à :

> Combien de blocs `C` faut-il développer pour représenter **tous les carriers**
> de toutes les paires de `(A,B)` ?

La question produit q4 est d’abord :

> Existe-t-il, pour chaque paire de ce bloc vivant, au moins un carrier ?

Un seul `C` classé `ALL_CARRIER` répond pour tout le bloc. Il faut alors annuler
les autres branches `C`, non les parcourir jusqu’aux feuilles pour compter une
masse devenue inutile.

Il faut donc rejouer l’ablation « endpoints figés » avec :

```text
short_circuit_on_first_ALL_CARRIER=ON,
carrier_mass_counting=OFF.
```

L’ancien ratio `1,1 point par bloc` n’est pas une preuve contre cette nouvelle
requête. Il mesurait une sortie beaucoup plus riche.

---

## 7. Scheduler exact, pair-major

Une vague traite les jobs plats :

```text
PairWitnessJob {state_id, CNodeKey}.
```

### 7.1 Étapes d’une vague

```text
1. classify_joint(PairState,C)
2. segmented_reduce par state_id
3. terminal_fates
4. choisir UNE action par PairState
5. count -> exclusive_scan -> fill de la vague suivante
```

### 7.2 Actions possibles

```text
SPLIT_WITNESS(C)
  remplace un nœud de la frontière par ses enfants ;
  A,B et leur ledger ne changent pas.

SPLIT_A
  crée deux PairState enfants une seule fois ;
  toute la frontière MIXED est reclassée pour chacun.

SPLIT_B
  symétrique.

EXACTIFY
  microdéveloppe les paires sous un cap déclaré.
```

Il est interdit qu’un job individuel `(state_id,C)` décide de scinder `A` ou
`B`. Seul le réducteur du `PairState` peut prendre cette décision.

### 7.3 Politique de décision

La politique initiale peut être déterministe et simple :

1. si un certificat carrier existe, ne plus raffiner la frontière aiguë ;
2. si `w4_upper_sat<8`, ne travailler que la frontière carrier ;
3. si le carrier est certifié mais `W4` reste mixte, ne travailler que la
   frontière `W4` et les endpoints ;
4. si les deux restent mixtes, évaluer un look-ahead d’un niveau pour
   `split A`, `split B` et le plus gros nœud `C` ;
5. choisir le meilleur score :

```text
masse de paires rendue terminale
+ beta * population témoin classée
---------------------------------
nombre de nouveaux jobs + coût fixe
```

6. sous `pair_tile_cap`, exactifier au lieu de poursuivre la hiérarchie.

Le look-ahead porte sur toute la frontière de l’état. Il ne reconstruit pas une
partition endpoint différente pour chaque `C`.

### 7.4 Identité et mémoïsation

La clé d’un état est :

```text
(RectId, ANodeKey, BNodeKey),
```

pas seulement `(A,B)`. `RectId` conserve l’owner exact-once de la partition
WSPD. Une même clé ne doit être créée qu’une fois dans une vague. Publier :

```text
duplicate_pair_state_keys=0,
endpoint_split_replays=0.
```

---

## 8. Preuve de correction

### 8.1 Exact-once des paires

Chaque `PairId` appartient à un unique `RectId` WSPD. Une scission de `A` ou `B`
remplace le produit par des produits enfants disjoints. Par induction, chaque
paire appartient à un unique `PairState` terminal.

### 8.2 Héritage des preuves

Pour `A' subset A`, `B' subset B` :

- `ALL_W4(A,B,C)` implique `ALL_W4(A',B',C)` ;
- `OUTSIDE_W4_CLOSED(A,B,C)` implique le même verdict par restriction ;
- `ALL_CARRIER(A,B,C)` implique `ALL_CARRIER(A',B',C)` ;
- `NONE_CARRIER(A,B,C)` implique le même verdict.

Seuls les statuts `MIXED` et les masques relationnels doivent être rejoués.

### 8.3 Sûreté des morts

- `w4_lower_sat==8` fournit huit vrais IDs distincts intérieurs pour toute paire ;
- `carrier impossible` signifie que la partition complète du témoin est
  certifiée `NONE_CARRIER`.

Aucun `MIXED`, cap ou débordement ne devient `DEAD`.

### 8.4 Complétude des actifs

Si un q4 pertinent possède l’owner `(a,b)` et un carrier `x`, alors son
`PairId` reste dans l’unique descendant WSPD. Le `PointId x` appartient à un
unique nœud de la frontière. Tant que la relation n’est pas décidée, ce nœud
reste `MIXED` et ne peut être jeté. À une microtuile exacte, le prédicat ponctuel
le retrouve. Aucun support n’est donc perdu.

---

## 9. Pourquoi cette structure est GPU-friendly

### 9.1 SoA

```text
state_A[], state_B[], rect_id[], pair_mass[],
w4_lower[], w4_upper[], carrier_cert[],
front_offset[], front_count[], fate[], action[].

job_state_id[], job_C_node[].
```

### 9.2 Kernels

```text
classify_joint_kernel
segmented_reduce_kernel
decide_action_kernel
count_children_kernel
exclusive_scan
fill_next_wave_kernel
```

Un endpoint split duplique seulement les handles `MIXED` nécessaires dans le
fill suivant. Les preuves décidées restent des spans immuables ou des digests.
Aucune `std::vector` par tâche, aucune allocation par seed.

### 9.3 Annulation

Dès qu’un `ALL_CARRIER` est trouvé, le drapeau `carrier_certified` empêche
l’émission des jobs carrier suivants pour cet état. Les jobs déjà lancés dans la
vague peuvent finir ; ils ne produisent aucun descendant. C’est un OR parallèle,
pas une énumération.

### 9.4 Frontière bornée

Un dépassement rend :

```text
PENDING_RESOURCE
```

avec continuation. Il ne fixe pas `w4_upper` à une valeur artificiellement
petite et ne publie pas d’actif terminal.

---

## 10. Relation avec la suite q4

Après `ACTIVE_EDGE_BLOCK` :

```text
1. microtuile d’arêtes exactes ;
2. parcours LBVH fusionné par arête :
     W4 exact avec arrêt à 8,
     carriers exacts ou compte/overflow ;
3. si carriers peu nombreux :
     Q4SeedAxisTopR4-LBVH ;
4. si carriers nombreux :
     EdgeCenterShallowCut collectif ;
5. owner6, primary, positivité, BallKey, census reçu, fold.
```

Le certificat carrier du bloc ne remplace pas cette exactification. Il prouve
seulement qu’une arête ne doit pas être éliminée avant l’étage des centres.

La contre-famille annulaire de l’audit précédent reste obligatoire : une arête
vivante peut avoir `Theta(n)` carriers. Le backend collectif ou le chemin
d’overflow n’est donc pas optionnel.

---

## 11. Vue q3, séparée de q4

Le court-circuit existentiel est propre à l’activation q4. Pour q3, chaque
carrier peut donner un support distinct ; il faut donc conserver les
`CarrierBlock` et les envoyer à la vue de centres :

```text
Lane3View:
  pieds des droites,
  ellipse D/12,
  profondeur < 9.
```

La couche neutre peut partager :

- le `PairState` ;
- la classification acute-owner ;
- les AABB et le LBVH ;
- les blocs symboliques.

Mais `Lane4` ne lit aucun support retenu ni aucun rang de `Lane3`. Pour q4, la
projection existentielle arrête la recherche après certificat ; pour q3, la vue
énumérative continue vers les pieds shallow.

---

## 12. Gates bloquantes

### G1 — masque endpoint relationnel

```text
parent: endpoint-overlap reste dans U4 ;
child non incident: il redevient témoin ;
child incident: il est masqué ;
missing_target=0.
```

### G2 — juge par identités

```text
missing_target=0,
duplicate_target=0,
false_symbolic_triple=0,
overcoverage_dead_anchor publié séparément.
```

### G3 — un seul arbre endpoint pour plusieurs `C`

Construire un état avec beaucoup de nœuds `C` mixtes et forcer un split de `A`.
Exiger :

```text
endpoint_splits=1,
unique_pair_state_keys=2,
endpoint_split_replays=0.
```

L’ancien scheduler produirait un split par branche `C`.

### G4 — court-circuit carrier

Une paire-bloc entièrement vivante, un premier nœud `C0` `ALL_CARRIER`, puis
beaucoup d’autres nœuds carriers. Exiger :

```text
carrier_certificates=1,
carrier_jobs_after_certificate=0,
active_pair_mass correct.
```

### G5 — carriers différents selon les paires

Construire un parent où chaque paire possède un carrier, mais aucun `C` n’est
universel pour toutes les paires. Le parent reste `MIXED`; après un unique split
endpoint partagé, les enfants trouvent leurs certificats respectifs. Cela tue un
algorithme qui conclurait `DEAD_NO_CARRIER` faute de carrier universel au parent.

### G6 — `two_lines`

```text
pairid_cross_expanded=0,
seed3_cross_emitted=0,
active_edge_cross=0,
endpoint_split_replays=0,
pending=0.
```

### G7 — parité finale petit `n`

Comparer les ensembles exacts de clés, pas les totaux :

```text
PairState source
== exact live-carrier oracle
== BallFormRange sur les q4 finalement produits.
```

### G8 — pentes physiques

Sur quatre tailles :

```text
pair_states,
pair_witness_jobs,
endpoint_splits,
witness_splits,
exact_pair_tiles,
bytes/HWM,
wall time.
```

Ne pas publier un exposant de `noeuds` tant que les coins W4 y sont comptés.

---

## 13. Ordre d’implémentation donné à Claude

1. Réparer le masque endpoint et le juge par identités avant toute optimisation.
2. Garder le code actuel sous `--scheduler=triple-task` comme oracle de coût.
3. Ajouter `--scheduler=pair-major` avec `PairState` et frontière interne.
4. Implémenter d’abord le court-circuit existentiel sans look-ahead compliqué :
   `C` descend, et `A/B` ne se scindent qu’à une feuille `C` encore `MIXED` ou
   lorsque la frontière dépasse un cap.
5. Ajouter G3--G5 ; mesurer face au scheduler actuel.
6. Ajouter ensuite le look-ahead `A/B/C` par état entier.
7. Lorsque les pentes physiques sont reçues, raccorder les microtuiles au LBVH
   axial.
8. Porter seulement alors la wavefront pair-major sur CUDA.

Cette ordonnance répond directement à la question : le rectangle WSPD reste le
propriétaire immuable des paires, mais ses endpoints peuvent être raffinés dans
un **unique arbre de paire partagé par toute la frontière témoin**. Ce n’est ni
`A,B` figé, ni une seconde WSPD répétée pour chaque `C`. C’est le quotient exact
du produit que le stack actuel déroule consciencieusement, parce que les piles
ne ressentent aucune honte à refaire cent fois le même travail.

---

## 14. Statut

- raccord WSPD et gate `two_lines` : **reçus positivement** ;
- `DEAD_W4` : **à contre-juger par bloc et par identités** ;
- masque endpoint actuel : **non reçu** ;
- juge cardinal `sparse>=brute` : **insuffisant** ;
- choix « figer ou raffiner librement » : **refusé comme fausse dichotomie** ;
- `PairState + JointWitnessFrontier` : **solution recommandée** ;
- claim de coût sparse : **en attente de G1--G8**.
