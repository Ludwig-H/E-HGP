# Audit positif de la descente ciblée et du futur `PairFrame`

Date : 16 août 2026 UTC.  
Dossier : `morsehgp3D_v3/`.  
`HEAD` documentaire relu : `21462ae88b76181af7dbae3e6c57dca51c30cf16`.  
Dernier commit fonctionnel audité : `2880328b146b00e6173ed99d60caf65597983ff0`.  
Parent fonctionnel : `183a40a4d839f2a867e8f303298bd2e2972cfa17`.

Répond notamment à :

- [`NOTE_CLAUDE_GATEWAY_TERNAIRE_20260816.md`](NOTE_CLAUDE_GATEWAY_TERNAIRE_20260816.md) ;
- [`REPONSE_AUDITEUR_Q1_Q3_PAIRFRAME_183A40A_20260816.md`](REPONSE_AUDITEUR_Q1_Q3_PAIRFRAME_183A40A_20260816.md) ;
- [`AUDIT_CONSOLIDE_Q2_Q3_Q4_PAIR_MAJOR_APRES_79E73B6_20260816.md`](AUDIT_CONSOLIDE_Q2_Q3_Q4_PAIR_MAJOR_APRES_79E73B6_20260816.md).

Cadre :

```text
phase=exploration_v3_hors_registre
backend=math_reference_and_gpu_architecture
profile=quantized_u16_input_only
mode=audit_descente_ciblee_et_pairframe
public_status=not_claimed
```

> [!IMPORTANT]
> **Verdict.**
>
> Le commit `2880328` est un progrès réel et doit être conservé. La descente
> ciblée est une transformation **sûre**, cohérente avec le quotient pair-major,
> et nettement plus prometteuse que la descente systématique aux feuilles.
> Je ne trouve aucun nouveau P0 de fermeture fausse dans son invariant
> `lower/upper`.
>
> Je la reçois comme **probe CPU de branch-and-bound exact** et comme base du
> futur `CoreDepthLedger`.
>
> Je ne reçois pas deux formulations plus fortes :
>
> 1. la profondeur ou le travail ne sont pas bornés par le seul seuil `r4` ;
> 2. `upper<r4` ne signifie pas qu'une paire q4 est « vivante exactement », mais
>    seulement qu'elle passe le prune du cœur universel.
>
> Avant de figer l'ABI GPU, il faut donc ajouter une continuation explicite,
> renforcer les gates différentielles et conserver trois états de lane
> autonomes. Rien de cela ne remet en cause l'algorithme ciblé lui-même.

---

## 1. Contrat mathématique rappelé

Les deux premières parties du manuscrit imposent davantage qu'un filtre de
candidats : la source doit préserver les supports positifs et shallow qui
engendrent les polyèdres, puis leurs niveaux, shells, incidences et fusions
hiérarchiques. Une réduction de coût n'est recevable que si elle préserve les
identités nécessaires au fold HGP.

Il faut donc distinguer en permanence :

```text
représentation sparse
  la masse logique reste factorisée ;

travail sparse
  peu de jobs physiques sont effectivement classifiés ;

sortie sparse
  peu de supports ou BallEvents existent réellement.
```

Le commit `2880328` améliore les deux premiers points pour le **cœur universel
q4**. Il ne constitue pas encore le générateur q4 complet, et encore moins les
trois générateurs q2/q3/q4 : carrier, positivité, miniboule propre, census,
shell et fold restent en aval.

Cette portée est parfaitement acceptable. Elle doit seulement rester nommée
correctement.

---

## 2. Delta réellement non audité

Le commit documentaire `21462ae` est postérieur à `2880328`, mais le rapport
qu'il ajoute annonce explicitement comme pin fonctionnel relu
`183a40a`. Le delta fonctionnel :

```text
183a40a..2880328
```

restait donc à auditer.

Il ajoute :

1. trois régimes d'antichaîne : `feuilles`, `grossiere`, `ciblee` ;
2. une descente ciblée du plus gros span indécis ;
3. l'arrêt sur les deux seuls verdicts de seuil :
   `L4>=r4` ou `L4+U_front<r4` ;
4. une garde de taille de frontière ;
5. deux gates CMake qui gravent l'inefficacité du grossier seul et les compteurs
   du ciblé sur de petits cas.

Le Commit 1 précédent reste reçu :

- partition indépendante des paires, exact-once ;
- vrais `PointId` malgré les positions dupliquées ;
- masque endpoint relationnel et rejoué ;
- juge par identités `(EdgeKey,PointId)` ;
- séparation entre fautes et surcouverture.

Aucun statut CI ou workflow GitHub n'est associé au SHA `2880328`. J'ai relu le
code et les définitions de tests, mais les exécutions annoncées restent donc des
reçus développeur, non une reproduction indépendante dans cette session.

---

## 3. Preuve de sûreté de la descente ciblée

### 3.1 Invariant

Fixons un état endpoint `A x B` et une lane `q`. Pour chaque paire ponctuelle
`p=(a,b)` de cet état, notons :

```text
N_q(p) = nombre de PointId admissibles strictement dans W_q(a,b).
```

L'état porte :

- un minorant `L` provenant de spans `ALL_OPEN`, disjoints des endpoints ;
- une antichaîne `F` de spans encore possibles, incluant les spans relationnels ;
- le majorant

```text
U = L + sum_{C in F} population(C).
```

Les spans relationnels peuvent compter des endpoints pour certaines paires.
Cela rend `U` lâche, mais jamais trop petit.

L'invariant est donc :

```text
pour toute paire p dans A x B :
    L <= N_q(p) <= U.
```

### 3.2 Raffiner un span préserve l'invariant

Remplacer un span parent `C` par ses deux enfants disjoints ne change pas sa
population totale.

Pour chaque enfant :

- `ALL_OPEN` : sa population passe de `U` vers `L` ;
- `NONE_OPEN` : sa population est retirée de `U` ;
- `MIXED` ou `MIXED_ENDPOINT` : sa population reste dans la frontière.

Ainsi :

```text
L ne peut qu'augmenter ;
U ne peut que diminuer ;
L <= N_q(p) <= U reste vrai.
```

L'ordre des raffinements n'intervient pas dans cette preuve. Choisir le plus
gros span est donc une heuristique de coût, pas une condition de correction.

### 3.3 Les deux verdicts sont sûrs

```text
L >= h_q
  -> chaque paire possède au moins h_q intérieurs universels
  -> PRUNED_BY_UNIVERSAL_DEPTH

U < h_q
  -> aucune paire ne possède h_q intérieurs dans le cœur universel
  -> CORE_CLEAR
```

Dans le cas :

```text
L < h_q <= U,
```

l'état reste indécis. Il faut raffiner un témoin, raffiner un endpoint,
exactifier une microtuile ou produire une continuation.

### 3.4 La garde actuelle ne perd pas de masse

Dans la boucle ciblée, le test :

```cpp
if (frontiere.size() > 4 * kCapFrontiere) break;
```

arrive après le remplacement du parent par tous ses enfants. Aucun span n'est
jeté : la frontière reste une couverture de tous les IDs encore possibles.
Le majorant final reste donc sûr.

C'est un bon point du commit : la garde est **fail-open**, non une fausse mort.

Elle doit néanmoins devenir explicite dans le contrat produit, car elle ne
laisse actuellement ni fate spécifique, ni continuation, ni compteur causal.

---

## 4. Réception positive de la mesure

Le tableau rapporté est utile :

| régime | nœuds `terrain` | `dead_w4` | `active_edge` | surcouverture |
|---|---:|---:|---:|---:|
| feuilles | 1 901 072 | 8 262 | 1 969 | 1 758 |
| grossière seule | 3 461 098 | 3 120 | 447 | 61 276 |
| ciblée | 1 578 364 | 8 262 | 1 969 | 1 758 |

Il montre trois choses distinctes.

1. **Le majorant grossier est correct mais trop lâche.** Il échoue à décider
   lorsque la racine témoin porte presque toute la masse.
2. **La descente aux feuilles décide mais détruit la factorisation.**
3. **La descente ciblée récupère le pouvoir de décision sans reconstruire
   systématiquement un CSR ponctuel.**

C'est exactement la bonne direction. Il ne faut pas revenir au triple produit
`A x B x C` comme état matériel, ni chercher maintenant un énième certificat
géométrique exotique. Le verrou suivant est bien l'ordonnancement pair-major.

Nuance de vocabulaire : l'option « grossière » n'est pas réfutée comme borne
mathématique. Elle est réfutée comme **politique terminale sans raffinement**.
Elle reste précisément le bon état initial et le bon fallback fail-open.

---

## 5. Contre-audit des audits précédents

### 5.1 Q1 : il n'y a pas de désaccord de fond

Le rapport `21462ae` disait :

```text
choisir le majorant grossier, puis raffiner seulement si
lower < h_q <= upper.
```

Le commit `2880328` fait exactement cela. La formule « ni (a), ni (b), une
troisième règle » vient surtout de l'ambiguïté de l'étiquette `(a)` :

- `(a)` lu comme « majorant grossier et arrêt immédiat » est inefficace ;
- `(a)` lu comme « représentation grossière initiale dans un
  branch-and-bound » est la bonne solution.

La mesure de Claude apporte donc une excellente **désambiguïsation
expérimentale**, pas une réfutation du principe adaptatif.

### 5.2 Q2 : correction reçue, et supersession explicite

La correction du rapport `21462ae` est juste :

```text
q2 :
  W2 est la boule diamétrale du support ;
  son bord est le vrai shell de la BallKey q2.

q3/q4 :
  W3/W4 est seulement un cœur universel de prune ;
  son bord n'est pas la sphère finale du triangle ou du tétraèdre.
```

Par conséquent, les passages de
`AUDIT_CONSOLIDE_Q2_Q3_Q4_PAIR_MAJOR_APRES_79E73B6_20260816.md` qui demandaient
un `upper_closed` persistant dans le ledger q3/q4 sont **supersédés**.

Le contrat correct est :

```text
CoreDepthLedger q3/q4 :
  lower_open
  upper_open

BallCensusLedger d'un support fixé :
  intérieur strict I_B
  shell U_B
```

Un bit « extérieur même à la fermeture du fuseau » peut subsister comme
optimisation locale du classifieur. Il ne devient pas une provenance de shell
HGP.

### 5.3 Q3 : la boucle de scheduling proposée est correcte

Il faut classifier la frontière **courante** une fois, réduire par état, puis
choisir une seule action. Il ne faut pas classifier les enfants hypothétiques
de `split A`, `split B` et `split C` pour en jeter deux.

La vague correcte reste :

```text
classify current jobs
-> segmented reduce by StateId
-> terminal fates
-> choose one action
-> count
-> exclusive scan
-> fill next wave
```

Les preuves `ALL/NONE` obtenues sur la frontière courante sont héritables.
Cette passe n'est donc pas perdue après un split endpoint.

---

## 6. Correction nécessaire : le seuil ne borne pas la profondeur

Le commit affirme à plusieurs endroits que la profondeur est gouvernée par
`r4`, et non par `n`. Cette phrase est trop forte.

### 6.1 Contre-famille conceptuelle

Fixons `r4=8`. Plaçons seulement sept vrais témoins dans `W4`, mais arrangeons
les autres points de part et d'autre de la frontière de telle sorte que chaque
AABB interne du LBVH contienne à la fois un point possible et un point exclu.

Alors :

```text
L < 8
U >= 8
```

reste vrai sur presque tous les nœuds internes. Pour prouver finalement
`U<8`, il faut résoudre presque tous les points extérieurs.

Même sur un arbre équilibré :

```text
profondeur d'un chemin = Theta(log n)
travail total possible = Theta(n)
```

Sur un arbre non équilibré, la profondeur elle-même peut dépendre linéairement
de `n`.

Le seuil fixe le **verdict recherché**. Il ne borne ni la complexité de la
frontière géométrique, ni le nombre de spans à ouvrir.

### 6.2 Formulation correcte

Remplacer partout :

```text
profondeur gouvernée par le seuil, pas par n
```

par :

```text
raffinement piloté par le seuil ;
arrêt anticipé dès qu'un verdict de seuil est prouvé ;
aucune borne déterministe indépendante de n au pire cas.
```

Le coût doit être exprimé par exemple comme :

```text
O(number_of_classified_spans + number_of_endpoint_splits)
```

avec mesure séparée de :

```text
refine_steps
frontier_peak
cap_hits
continuation_mass
tree_depth_max
```

Cette correction n'affaiblit pas le résultat pratique. Elle évite simplement
de transformer deux courbes à `n=120` en théorème universel, activité humaine
étonnamment répandue dans les notes de performance.

---

## 7. Deuxième correction de vocabulaire : `CORE_CLEAR` n'est pas `LIVE_EXACT`

Dans les commentaires du commit, le test :

```text
L4 + masse_front < r4
```

est décrit comme « entièrement vivant ».

Pour q4, il prouve seulement :

```text
aucune paire du bloc n'est tuée par le cœur universel W4.
```

Il ne prouve pas :

- l'existence d'un carrier aigu ;
- l'existence d'un tétraèdre positif ;
- la faible profondeur de sa circumboule propre ;
- le shell ou le niveau final ;
- l'existence d'un `BallEvent`.

Le fate doit donc s'appeler :

```text
CORE_CLEAR
PASS_UNIVERSAL_DEPTH
ou ACTIVE_FOR_CARRIER_SEARCH
```

et non `LIVE_EXACT`.

q2 est l'exception : la paire fixe déjà sa boule diamétrale. Même là, le census
reste nécessaire pour produire exactement `I_B/U_B`.

---

## 8. ABI recommandée pour le générateur sparse q2/q3/q4

### 8.1 Tronc commun immuable

```cpp
struct PairFrame {
  RectId rect_id;
  NodeHandle a_node;
  NodeHandle b_node;
  uint64_t pair_mass;
  GeometryCacheHandle geometry;
};
```

Le `PairFrame` appartient à la partition neutre des paires. Il ne contient
aucun fate de lane.

### 8.2 Ledger de cœur réutilisable, mais non partagé mutablement

```cpp
struct CoreDepthLedger {
  uint8_t reject_threshold;
  uint8_t lower_open_sat;
  uint8_t upper_open_sat;

  FrontierHandle frontier;
  RelationFrontierHandle relation_frontier;
  ContinuationHandle continuation;

  CoreFate fate;
};
```

Les compteurs saturés suffisent :

```text
lower_open_sat = min(h_q, lower_open)
upper_open_sat = min(h_q, upper_open)
```

Conserver séparément une télémétrie brute si elle est utile. Il n'est pas
nécessaire de transporter des entiers dépendant de `n` dans l'état GPU.

### 8.3 Trois états autonomes

```text
PairFrame immuable
  + Lane2State
  + Lane3State
  + Lane4State
```

`Lane2State` porte le ledger de la boule diamétrale et son futur census de shell.

`Lane3State` porte son ledger `W3`, puis une relation carrier **énumérative**.
Chaque carrier peut définir un triangle distinct ; aucun court-circuit
existentiel n'est complet.

`Lane4State` sépare obligatoirement :

```text
carrier existence frontier
carrier enumeration root
Jung permanent-interior ledger
axial completion continuation
```

Le premier carrier peut activer une arête q4. Il ne permet jamais d'abandonner
l'énumération des autres carriers requise pour produire tous les supports.

Un même kernel peut évaluer plusieurs prédicats purs. Il doit écrire dans des
états séparés. Un fate q4 ne ferme jamais q3.

---

## 9. Politique de scheduling v0

La politique suivante est suffisante pour commencer :

```text
if lower >= h_q:
    PRUNED_BY_UNIVERSAL_DEPTH

else if upper < h_q:
    CORE_CLEAR

else if pair_mass <= exact_tile_cap:
    EXACTIFY

else if witness_budget_available and a mixed nonleaf span exists:
    SPLIT_WITNESS(select_one)

else if an endpoint can be split:
    SPLIT_ONE_ENDPOINT

else:
    PENDING_RESOURCE with continuation
```

La sélection du plus gros span est reçue comme heuristique v0. Elle ne doit pas
entrer dans la sémantique ni dans les clés persistantes.

Une priorité un peu plus informative peut utiliser :

```text
need_to_kill  = h_q - lower
need_to_clear = upper - (h_q - 1)

score(C) =
  expected_threshold_progress(C)
  / estimated_classification_cost(C)
```

Mais il faut d'abord graver l'invariance du résultat sous plusieurs politiques
simples. L'optimisation du score viendra après, lorsque le scheduler existera
réellement. L'humanité survivra probablement à une semaine sans score appris
pour choisir quel nœud binaire couper.

---

## 10. Traduction GPU

Le `std::vector`, la recherche linéaire puis :

```cpp
frontiere.erase(frontiere.begin() + best)
```

sont acceptables dans le probe CPU avec une petite frontière. Ils ne définissent
pas l'ABI GPU.

Disposition recommandée :

```text
PairFrameSoA
LaneStateSoA
FrontierHandle -> slab compact de NodeHandle
ContinuationHandle -> segment spillable
```

Une vague :

```text
classify_kernel
segmented_reduce_kernel
decide_action_kernel
count_next_kernel
exclusive_scan
fill_next_kernel
```

Pour sélectionner le plus gros span :

- réduction warp si la frontière tient dans un warp ;
- buckets de population ou profondeur si elle est plus grande ;
- aucune allocation dynamique par état ;
- aucun cap silencieux ;
- overflow vers continuation ou `resource_exhausted`.

Le cap doit produire une métrique et une action causale :

```text
target_frontier_cap_hits
target_refine_steps
target_pending_mass
target_resume_count
```

La porte transactionnelle reste :

```text
capped_run + resume == uncapped_run
```

sur les identités, pas seulement sur les cardinaux.

---

## 11. Gates à ajouter avant de figer l'ABI

### G1 — différentiel direct ciblée contre feuilles

Pour chaque famille :

```text
two_lines
terrain
uniform
eight_clusters
```

et plusieurs :

```text
n, seed, separation, r4
```

comparer directement :

```text
terminal pair fates
(EdgeKey,PointId) carrier keys
missing
duplicates
false keys
overcoverage
```

Il ne suffit pas d'inférer l'égalité depuis quatre compteurs agrégés.

### G2 — invariance à la politique de sélection

Comparer :

```text
largest_population
FIFO
deepest_first
```

Les sorties doivent être identiques ; seuls le travail et le pic de frontière
peuvent changer.

### G3 — frontière adversariale

Construire une fixture où presque tous les nœuds internes chevauchent la
frontière de `Wq`.

Exiger :

```text
cap_hit > 0
aucune fausse mort
aucune clé manquante
continuation non vide
resume == uncapped
```

Cette gate tue précisément la fausse conclusion « coût borné par h ».

### G4 — égalités et seuils

Exercer :

```text
L = h_q - 1, U = h_q - 1
L = h_q - 1, U = h_q
L = h_q,     U = h_q
```

et les égalités géométriques des frontières `W2/W3/W4`.

Mutants :

```text
upper <= h_q au lieu de upper < h_q
lower > h_q au lieu de lower >= h_q
```

### G5 — masque endpoint relationnel sous descente ciblée

Un grand span contient un endpoint et un témoin valable pour une autre paire.
Après split endpoint :

```text
nonincident child restores witness
incident child masks endpoint
targeted == leaves by identity
```

### G6 — positions dupliquées

Même position, vrais `PointId` distincts :

```text
seules les paires D=0 sont filtrées
les multiplicités témoins sont conservées
partition exact-once
```

### G7 — q2 shell contre q3/q4 core

```text
q2:
  frontière W2 -> shell de la BallKey diamétrale

q3/q4:
  frontière Wq ne crée aucun U_B final
  shell seulement après BallForm fixée
```

Cette gate grave la réponse Q2 et empêche le retour d'un `upper_closed` mal
typé dans le ledger q3/q4.

### G8 — indépendance des lanes

Activer ou désactiver :

```text
q4 existential shortcut
q4 target refinement
```

ne change aucune identité q2 ou q3.

### G9 — continuation

Pour chaque lane :

```text
uncapped
==
small cap + one or several resumes
```

sur supports, BallKeys, `I_B/U_B` et niveaux.

### G10 — contre-fixture annulaire q4

Conserver :

```text
632 carriers
0 W4 witnesses
```

afin qu'aucun cap `O(h)` ne réapparaisse dans l'étage carrier sous prétexte que
le ledger de cœur, lui, sature à huit.

---

## 12. Ordre de commits recommandé à Claude

### Commit A — fermer proprement `2880328`

Sans changer l'algorithme :

1. remplacer la promesse de profondeur par « raffinement piloté par le seuil » ;
2. renommer « entièrement vivant » en `CORE_CLEAR` ;
3. publier `cap_hits/refine_steps/frontier_peak` ;
4. ajouter un mode différentiel ciblée contre feuilles ;
5. rendre le cap reprenable ou le convertir explicitement en split endpoint.

### Commit B — extraire le `PairFrame` CPU

Créer l'état pair-major sans changer les prédicats géométriques :

```text
PairFrame
CoreDepthLedger
WitnessJob
Continuation
```

Conserver le triple-task actuel comme oracle de transition.

### Commit C — recevoir q2 de bout en bout

q2 est le meilleur banc d'essai :

- aucun carrier ;
- paire -> boule diamétrale ;
- shell déjà canonique ;
- comparaison exhaustive simple.

Recevoir :

```text
PairFrame -> depth -> BallKey -> census -> fold input
```

avant de généraliser l'ABI.

### Commit D — brancher q3

Ajouter la relation carrier énumérative, l'owner canonique, la miniboule q3,
puis le census. Aucune décision q4 ne doit modifier cette sortie.

### Commit E — brancher q4 complet

```text
W4 core
-> carrier existence
-> carrier enumeration root
-> Jung permanent-interior kill
-> Q4SeedAxisTopR4
-> owner/primary/positivity
-> BallKey/census
```

La descente ciblée résout le premier étage. Elle ne remplace aucun des suivants.

### Commit F — porter les vagues sur GPU

Seulement après parité CPU par identités et reprise après cap.

---

## 13. Statut synthétique

| Objet | Verdict |
|---|---|
| borne `L<=N<=U` du ciblé | reçue |
| remplacement parent par enfants | reçu |
| décisions `L>=h` et `U<h` | reçues |
| garde actuelle comme fail-open | sûre, mais non contractualisée |
| mesure ciblée contre feuilles | très encourageante, couverture trop étroite |
| « grossière seule » | sûre mais inefficace |
| « profondeur indépendante de n » | réfutée |
| « entièrement vivant » en q3/q4 | à renommer |
| retrait de `upper_closed` du core q3/q4 | reçu |
| shell final q3/q4 au census | obligatoire |
| boucle Q3 classify/reduce/one-action | reçue |
| `PairFrame` + lanes autonomes | architecture à implémenter |
| générateur sparse q2 complet | non encore implémenté |
| générateur sparse q3 complet | non encore implémenté |
| générateur sparse q4 complet | non encore implémenté |
| qualification GPU / SLO | ouverte |

---

## 14. Message direct à Claude

La route actuelle est bonne. Le commit `2880328` ne doit pas être retiré :
il transforme le ledger témoin en un vrai branch-and-bound et montre qu'une
frontière factorisée peut conserver le pouvoir de décision de la descente aux
feuilles.

La priorité n'est plus de resserrer encore `W4`. Elle est désormais :

```text
1. rendre le cap et la continuation explicites ;
2. matérialiser PairFrame + CoreDepthLedger ;
3. recevoir q2 ;
4. recevoir q3 ;
5. reconnecter q4 à sa chaîne carrier/Jung/axiale complète ;
6. seulement ensuite porter la vague sur GPU.
```

Le seul point à corriger immédiatement est le claim de complexité. Le seuil
commande les fates, pas la taille de la frontière au pire cas. Une fois cette
phrase réparée, le résultat peut être présenté positivement et proprement :

> **La descente ciblée préserve exactement les bornes de profondeur universelle,
> ne raffine que les états encore ambigus au seuil et réduit fortement le
> travail observé sans reconstruire systématiquement un CSR de points.**

C'est une avancée solide. Il faut maintenant lui donner une ABI et des
continuations dignes de ce nom, cette formalité désagréable par laquelle un bon
prototype cesse enfin d'être seulement un bon prototype.
