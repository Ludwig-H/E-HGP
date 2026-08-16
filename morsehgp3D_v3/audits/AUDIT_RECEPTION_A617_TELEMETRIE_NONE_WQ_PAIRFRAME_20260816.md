# Réception de `a6171d1` et corrections de télémétrie avant `PairFrame`

Date : 16 août 2026 UTC.  
Dossier : `morsehgp3D_v3/`.  
Dernier commit fonctionnel audité : `a6171d1827245f530eede9bbc4e9b1b3407121ed`.  
Audit auquel Claude répond :
[`AUDIT_POSITIF_DESCENTE_CIBLEE_PAIRFRAME_288032_20260816.md`](AUDIT_POSITIF_DESCENTE_CIBLEE_PAIRFRAME_288032_20260816.md).

Cadre :

```text
phase=exploration_v3_hors_registre
backend=math_reference_and_gpu_architecture
profile=quantized_u16_input_only
mode=reaudit_a617_telemetry_and_pairframe
public_status=not_claimed
```

> [!IMPORTANT]
> **Verdict.**
>
> Le commit `a6171d1` est reçu positivement sur ses deux corrections de fond :
>
> 1. la descente est **pilotée** par le seuil, mais ni sa profondeur ni son
>    travail ne sont bornés par le seul seuil ;
> 2. `upper<h_q` donne `CORE_CLEAR`, jamais `LIVE_EXACT` pour q3/q4.
>
> Les réponses Q2 et Q3 sont également reçues : le ledger de cœur q3/q4 ne
> transporte que `lower_open/upper_open`, et la vague pair-major doit classifier
> la frontière courante, réduire par état puis choisir une seule action.
>
> Claude peut commencer `PairFrame`.
>
> **Réserve avant de figer l'ABI :** les cinq nouveaux compteurs ne mesurent pas
> encore les objets indiqués par leurs noms. `frontier_peak` est une masse
> logique, `refine_depth_max` n'est pas une profondeur, `refine_steps` compte des
> tours sans split, et un des deux chemins de cap n'alimente ni `cap_hits` ni
> `continuation_mass`. Ce n'est pas une faute géométrique, mais ce serait une
> mauvaise base pour dimensionner le GPU ou conclure sur la représentation
> sparse.
>
> Enfin, le grand résiduel observé ne doit pas encore être interprété comme une
> limite intrinsèque du branch-and-bound : le majorant ne possède actuellement
> qu'un certificat `NONE_W2`. Un certificat sûr `NONE_W3/NONE_W4`, exact aux
> singletons et très bon marché, est donné au § 6.

---

## 1. Ce que `a6171d1` ferme correctement

### 1.1 Rétractation du claim de profondeur

La rétractation est correcte et utile. Les chiffres :

```text
uniform, n=120, r4=8
frontier_peak=112
refine_depth_max=100
```

suffisent déjà à montrer que le mécanisme de seuil ne crée aucune petite borne
universelle ressemblant à `O(r4)`.

L'énoncé durable est bien :

```text
raffinement piloté par le seuil ;
arrêt anticipé dès qu'un verdict est prouvé ;
aucune borne déterministe indépendante de n au pire cas.
```

Cette correction doit rester dans `PROPOSITION.md` lors de la prochaine
consolidation. Elle protège notamment contre deux dimensionnements faux :

- capacité de frontière choisie comme `C*h_q` ;
- budget de travail choisi comme `C*h_q` par état.

### 1.2 `CORE_CLEAR` contre `LIVE_EXACT`

La correction est également exacte.

Pour q3/q4 :

```text
upper_open < h_q
```

prouve seulement qu'aucune paire du bloc n'atteint le seuil dans le **cœur
universel** `W_q`. Il reste à construire un support, imposer owner et positivité,
former sa miniboule particulière, puis faire le census exact.

Les fates corrects sont donc :

```text
PRUNED_BY_UNIVERSAL_DEPTH
CORE_CLEAR
MIXED_CORE
PENDING_RESOURCE
```

et non :

```text
DEAD
LIVE_EXACT
```

sauf spécialisation q2 soigneusement typée.

### 1.3 Q2 : séparation du core et du census

La séparation suivante est reçue :

```text
CoreDepthLedger q3/q4
  lower_open
  upper_open

BallCensusLedger d'une BallForm fixée
  I_B
  U_B
```

Une précision de vocabulaire est toutefois nécessaire : **le shell HGP n'est
pas une notion réservée à q2**. Toute BallForm finale q2/q3/q4 possède son shell
`U_B`. Ce qui est propre à q2 est l'identification :

```text
frontière de W2(a,b)
  = shell de la boule diamétrale du support {a,b}.
```

Pour q3/q4 :

```text
frontière de Wq(a,b)
  != shell de la circumboule finale en général.
```

Cette formulation évite qu'une simplification locale du ledger soit relue plus
tard comme « q3/q4 n'ont pas de shell », ce qui serait évidemment incompatible
avec le payload HGP.

### 1.4 Q3 : vague pair-major

La réponse est reçue :

```text
classify current witness jobs
-> segmented reduce by PairStateId
-> apply terminal fates
-> choose exactly one action
-> count
-> exclusive scan
-> fill next wave
```

Il ne faut pas classifier intégralement les enfants hypothétiques de `split A`,
`split B` et `split C` pour en jeter deux. Les preuves `ALL/NONE` produites sur
la frontière courante sont héritables sous restriction endpoint ; la passe
courante n'est donc pas perdue.

---

## 2. Contre-audit des nouveaux compteurs

Les compteurs sont une bonne initiative. Leurs noms actuels mélangent cependant
masse logique, taille physique et profondeur d'arbre.

### 2.1 `frontier_peak` est une masse logique d'IDs

Le code imprime :

```cpp
frontier_peak = g.front_masse_max
```

avec :

```cpp
masse_front = sum_h population(h).
```

Ce n'est pas le nombre de spans stockés. Un unique span racine peut porter une
masse `n`. Inversement, cent spans singleton portent aussi une masse cent.

Renommer :

```text
frontier_witness_mass_peak
```

et publier séparément le compteur déjà disponible :

```text
frontier_span_peak = g.frontiere_max
```

Interprétation correcte :

- `frontier_witness_mass_peak ~= n` dit que le **majorant logique** reste très
  incertain ;
- `frontier_span_peak ~= n` dirait que la **représentation physique** dégénère en
  CSR ponctuel ;
- seules les deux mesures ensemble permettent de conclure.

Le commit écrit actuellement « la frontière atteint tout le nuage ». Cela est
juste pour la masse logique, mais ne prouve pas que la frontière physique
contient `n` records.

### 2.2 `refine_depth_max` n'est pas une profondeur

Le code fait :

```cpp
long long pas = 0;
for (;;) {
  ++g.refine_steps;
  ++pas;
  if (terminal) break;
  ...
  split one selected span;
}
refine_depth_max = max(refine_depth_max,pas);
```

`pas` compte les itérations de la boucle pour un état. Il inclut l'itération
terminale, et les spans successivement scindés peuvent appartenir à des branches
différentes. Il ne mesure donc ni :

- la profondeur LBVH maximale ;
- la longueur d'une chaîne ancêtre-descendant ;
- exactement le nombre de splits.

Le chiffre `100` signifie approximativement « cet état a demandé 99
raffinements avant son dernier test », pas « une branche a atteint profondeur
100 ».

Remplacement recommandé :

```text
witness_split_count_total
witness_split_count_max_per_state
witness_tree_depth_max
```

avec :

```cpp
++witness_split_count_total;
++state_splits;
```

seulement après avoir effectivement choisi et remplacé un parent non-feuille.

La profondeur réelle se lit depuis la profondeur du `NodeHandle` scindé, ou se
transporte dans le nœud LBVH. Elle est au plus la hauteur de l'arbre ; le nombre
de splits d'un état peut, lui, être linéaire même sur un arbre équilibré.

### 2.3 `refine_steps` compte aussi les tests terminaux

Comme l'incrément précède les conditions d'arrêt, `refine_steps` n'est pas le
nombre de raffinements. Garder éventuellement :

```text
refine_loop_iterations
```

mais ajouter le vrai compteur :

```text
witness_splits
```

Pour la projection GPU, le second est plus proche du nombre de records enfants
à allouer ; le premier mesure le nombre de réductions/branches de contrôle.

### 2.4 Les deux caps ne partagent pas le même ledger

Il existe deux sorties de capacité :

1. construction initiale :

```cpp
while (!pile.empty() && frontiere.size() <= kCapFrontiere)
```

puis :

```cpp
if (!pile.empty()) {
  ++pending;
  ...
  tronquee=true;
}
```

2. descente ciblée :

```cpp
if (frontiere.size() > 4*kCapFrontiere) {
  ++cap_hits;
  continuation_mass += ...;
  break;
}
```

Le premier chemin n'incrémente ni `cap_hits` ni `continuation_mass`. Ainsi :

```text
cap_hits=0
continuation_mass=0
```

ne prouve pas qu'aucune capacité n'a interrompu l'état. Il faut un seul
mécanisme :

```text
ContinuationReason::INITIAL_FRONTIER_CAP
ContinuationReason::TARGET_REFINEMENT_CAP
ContinuationReason::ENDPOINT_SPLIT_CAP
ContinuationReason::OUTPUT_CAP
```

et des compteurs communs :

```text
continuation_states
continuation_spans
continuation_witness_mass
continuation_pair_mass
continuation_bytes
```

`pending` peut rester un total, mais il doit être l'agrégat des raisons typées,
pas un second canal qui échappe aux métriques nouvelles.

### 2.5 `continuation_mass` doit nommer son unité

La valeur actuelle somme les populations de spans témoins. Elle n'est ni :

- le nombre de paires en attente ;
- le nombre de supports en attente ;
- la mémoire du continuation record ;
- une masse unique, car plusieurs états peuvent porter des témoins identiques.

Le nom correct est :

```text
continuation_witness_mass_sum
```

Pour le contrat transactionnel, publier en plus :

```text
continuation_pair_mass
continuation_record_count
continuation_bytes
```

Un GPU se dimensionne en records et octets, pas en somme sémantique d'IDs
possibles, aussi poétique que cette dernière puisse paraître.

---

## 3. Un résidu Q2 dans le code : `U4_closed`

Le commit affirme correctement que `upper_closed` est supersédé pour le cœur
q3/q4. Pourtant le header conserve :

```cpp
kActiveAll  // U4_closed < r4 ET ALL_STRICT

classifie_conjoint(..., long long U4_closed, ...)
```

Cette variable est en réalité un majorant du nombre d'intérieurs **ouverts**
possibles dans `W4`, pas un census fermé et pas une provenance de shell.

À renommer avant l'extraction de `PairFrame` :

```text
U4_open
upper_open
```

et, idéalement :

```text
kAllCarrierCoreClear
```

ou :

```text
kActiveForCarrierEnumeration
```

Le nom `kActiveAll` est ambigu : il ne signifie ni que tous les supports q4
existent, ni qu'ils sont shallow dans leur circumboule finale. Il signifie que
tout triplet du bloc est carrier et que toutes les paires passent le prune du
cœur universel.

Cette correction est petite aujourd'hui. Après gel de l'ABI, elle deviendrait le
genre de dette sémantique qui finit par faire compter un bord de fuseau comme un
shell de sphère, précisément l'erreur que Q2 vient de fermer.

---

## 4. La gate actuelle est une régression, pas un théorème asymptotique

La gate :

```text
uniform,n=120:
frontier_peak=112
refine_steps=213187
cap_hits=0
continuation_mass=0
refine_depth_max=100
```

est utile pour graver le comportement courant. Elle ne prouve pas à elle seule
une croissance asymptotique, et ses nombres exacts sont sensibles à :

- l'ordre des enfants ;
- le tie-break du « plus gros span » ;
- la structure du LBVH ;
- l'instrumentation des itérations terminales.

Je recommande de la renommer conceptuellement :

```text
telemetry_uniform_120_regression
```

et d'ajouter une gate analytique distincte avec des inégalités stables :

```text
witness_split_count_max_per_state >= c*n
frontier_witness_mass_peak >= n-C
```

sur plusieurs tailles, ou sur une famille explicite donnée au § 7.

L'absence de workflow/status GitHub attaché à `a6171d1` signifie par ailleurs
que les « 30 portes » restent ici un reçu développeur. Le code et les définitions
de gates ont été relus ; les exécutables n'ont pas été rejoués indépendamment
dans cette session.

---

## 5. Pourquoi la masse de frontière presque `n` n'est pas encore une limite intrinsèque

Le ledger courant possède deux certificats :

```text
ALL_W4     via bloc_tout_w4
NONE_W2    via Hmax<=0
```

Il ne possède pas de certificat :

```text
NONE_W4
```

Un point dans la boule diamétrale `W2`, mais hors du fuseau plus étroit `W4`,
reste donc dans `upper_open`, même lorsqu'il est singleton et que son statut q4
pourrait être décidé exactement.

Par conséquent, une grande partie de :

```text
frontier_witness_mass_peak ~= n
```

peut mesurer la faiblesse volontaire du majorant `NONE_W2`, pas la difficulté
intrinsèque de la frontière exacte de `W4`.

Cela ne réhabilite aucune borne en `O(h_q)` : une configuration arbitrairement
proche du bord exact peut toujours imposer beaucoup de travail. Mais avant de
conclure sur le comportement pratique du futur `CoreDepthLedger`, il faut au
moins donner au majorant une sortie `NONE_Wq` qui devienne exacte aux feuilles.

---

## 6. Nouveau certificat sûr `NONE_OPEN_Wq`

### 6.1 Définition

Pour trois boîtes `A`, `B`, `Z`, poser :

```text
H(a,b,z) = (z-a)·(b-z)
E(a,z)   = ||z-a||²
X(b,z)   = ||b-z||²
```

et :

```text
Hmax = max_{A×B×Z} H = -phi_min
Emin = min_{A×Z} E = dist²(A,Z)
Xmin = min_{B×Z} X = dist²(B,Z)
```

Les deux distances de boîtes sont exactes et séparables par axe :

```text
gap(I,J) = max(0, I.lo-J.hi, J.lo-I.hi)
dist²(A,Z) = sum_k gap(A_k,Z_k)².
```

Les cœurs s'écrivent :

```text
W2 : H>0
W3 : H>0 et 4H²>EX
W4 : H>0 et 3H²>EX.
```

Définir :

```text
c2 : pas de seconde inégalité
c3 = 4
c4 = 3.
```

### 6.2 Théorème

Pour q3 ou q4, si :

```text
Hmax <= 0
```

ou :

```text
c_q * Hmax² <= Emin * Xmin,
```

alors aucun triplet de `A×B×Z` n'appartient à `W_q`.

### 6.3 Preuve

Pour tout triplet :

```text
H <= Hmax
E >= Emin
X >= Xmin.
```

Lorsque `H>0` :

```text
c_q H²
  <= c_q Hmax²
  <= Emin Xmin
  <= EX.
```

L'inégalité stricte requise par `W_q` est donc impossible. Lorsque `H<=0`, le
triplet est déjà hors de `W2`.

Le certificat est fail-open : s'il ne tire pas, il ne conclut rien.

### 6.4 Propriété importante

Lorsque `A`, `B` et `Z` sont des singletons :

```text
Hmax=H,
Emin=E,
Xmin=X.
```

Le certificat décide donc **exactement** `NONE_W3/NONE_W4` à la feuille. Le
branch-and-bound ne termine plus avec des feuilles connues hors de `W4` encore
comptées dans `upper_open`.

### 6.5 Coût et largeur

Sous u16 :

```text
Emin, Xmin < 2^34
Hmax          < 2^35
c_q Hmax²     < 2^72
Emin Xmin     < 2^68
```

`i128` suffit largement. Le calcul ajoute deux distances boîte-boîte, un carré
et une multiplication, sans racine ni division.

### 6.6 ABI lane-générique

```cpp
enum class CoreSpanStatus : uint8_t {
  ALL_OPEN,
  NONE_OPEN,
  MIXED,
  MIXED_ENDPOINT
};

CoreSpanStatus classify_core_span(
    Lane q,
    Box A,
    Box B,
    Box Z,
    RelationMask relation);
```

Ordre conseillé :

```text
relation endpoint
-> ALL_OPEN fort
-> Hmax<=0
-> NONE_OPEN_Wq par Hmax/Emin/Xmin
-> MIXED
```

Le certificat peut être ajouté comme primitive pure sans retarder l'extraction
`PairFrame`.

---

## 7. Fixture analytique du fuseau annulaire

Prendre :

```text
a=(-R,0,0)
b=( R,0,0)
z=(0,u,v)
s=u²+v².
```

Alors :

```text
E=X=R²+s
H=R²-s.
```

On obtient exactement :

```text
z in W2  <=>  s < R²
z in W3  <=>  s < R²/3
z in W4  <=>  s < (2-sqrt(3)) R².
```

Ainsi, tout point de l'anneau :

```text
(2-sqrt(3))R² <= s < R²
```

est dans `W2` mais hors de `W4`. Le certificat courant `NONE_W2` ne peut jamais
le retirer. Le nouveau certificat `NONE_W4` le décide exactement lorsque les
boîtes sont ponctuelles.

Fixture u16 simple, après translation :

```text
a=(900,1000,1000)
b=(1100,1000,1000)
R=100
z=(1000,1060,1000)
s=3600
```

Comme :

```text
(2-sqrt(3))*10000 ~= 2679,49 < 3600 < 10000,
```

`z` est :

```text
W2 : oui
W3 : non
W4 : non.
```

Construire ensuite :

- sept vrais `PointId` strictement dans `W4` ;
- beaucoup de vrais `PointId` dans l'anneau `W2\W4` ;
- endpoints disjoints et positions dupliquées optionnelles.

À `h4=8`, l'ancien majorant reste grand ; le nouveau doit retirer l'anneau sans
changer aucune identité de sortie.

Gates et mutants :

```text
old_none_w2_frontier_mass > new_none_w4_frontier_mass
new_none_w4_missing=0
new_none_w4_false_dead=0
```

Mutants à tuer :

```text
strict < au lieu de <= pour exclure l'ouvert
Hmax = -phi_max au lieu de -phi_min
Emax/Xmax utilisés dans le mauvais sens
coefficient q3/q4 échangé
```

---

## 8. Télémétrie minimale avant et après `PairFrame`

### 8.1 Avant quotient pair-major

Publier :

```text
ledger_refreshes_total
unique_pair_state_keys
repeated_pair_state_refreshes

frontier_span_peak
frontier_witness_mass_peak
witness_splits_total
witness_splits_max_per_state
witness_node_depth_max

initial_cap_hits
target_cap_hits
continuation_states
continuation_spans
continuation_pair_mass
continuation_bytes
```

Le rapport :

```text
ledger_refreshes_total / unique_pair_state_keys
```

mesure directement la duplication que `PairFrame` doit supprimer.

### 8.2 Après quotient pair-major

La porte structurante est :

```text
one mutable LaneState per (RectId,A_node,B_node,q)
```

avec :

```text
endpoint_split_replays = 0
pair_state_duplicate_keys = 0
```

Le scheduler historique peut rendre les mêmes identités avec plus de travail ;
le scheduler pair-major doit réduire les refreshes sans changer supports,
BallKeys, census ou shells.

---

## 9. Ordre de travail recommandé à Claude

### Commit 1 — correction des noms et compteurs

Avant toute structure persistante :

1. `U4_closed -> U4_open` ;
2. qualifier la phrase « shell propre à q2 » ;
3. `frontier_peak -> frontier_witness_mass_peak` ;
4. publier `frontier_span_peak` ;
5. remplacer `refine_depth_max` par les trois compteurs exacts ;
6. unifier `pending` et `cap_hits` sous des continuations typées.

### Commit 2 — squelette `PairFrame` CPU

Introduire sans changer les prédicats :

```text
PairFrame
Lane2State
Lane3State
Lane4State
CoreDepthLedger
WitnessJob
ContinuationRecord
```

Le scheduler triple-task reste oracle de transition.

### Commit 3 — q2 de bout en bout

q2 est le banc d'essai naturel :

```text
PairFrame
-> W2 depth
-> BallForm diamètre
-> BallKey
-> exact I_B/U_B
-> entrée du fold
```

C'est la seule lane où le core et la boule finale coïncident, donc celle où une
confusion de types se voit immédiatement.

### Commit 4 — certificat `NONE_Wq`

L'ajouter comme primitive lane-générique et mesurer :

```text
frontier_witness_mass_peak
frontier_span_peak
witness_splits
```

avant/après, sans changer aucune identité.

Il peut aussi être ajouté avant Commit 3 s'il est purement local ; il ne doit
pas repousser l'extraction pair-major.

### Commit 5 — q3, puis q4 complet

q3 : carrier énumératif, owner, Gram/Cramer, census.

q4 :

```text
W4 core
-> carrier existence
-> carrier enumeration root
-> Jung permanent-interior kill
-> Q4SeedAxisTopR4
-> owner/primary/positivity
-> BallKey/census
```

Le premier carrier active l'arête, mais ne clôt jamais l'énumération complète.

---

## 10. Gates bloquantes révisées

### G1 — sémantique des compteurs

```text
frontier_span_peak <= frontier_witness_mass_peak
witness_splits_total <= refine_loop_iterations
witness_splits_max_per_state + terminal_checks
  = old_refine_depth_metric
```

sur une fixture simple où les valeurs sont calculables à la main.

### G2 — tous les caps deviennent continuations

Forcer séparément les deux anciennes limites :

```text
initial frontier cap
refinement frontier cap
```

et exiger :

```text
continuation_states>0
capped+resume == uncapped
```

par identités.

### G3 — `U_open`, aucun shell de core q3/q4

Le code et les sérialisations ne doivent contenir aucun champ persistant :

```text
upper_closed_q3
upper_closed_q4
core_shell_q3
core_shell_q4
```

Le shell final reste exigé dans `BallCensusLedger` pour les trois lanes.

### G4 — `NONE_Wq` annulaire

La fixture du § 7 doit réduire le majorant et les splits, avec :

```text
missing=0
duplicate=0
false_dead=0
```

### G5 — politique de split indépendante de la sémantique

Comparer :

```text
largest_span
FIFO
deepest_span
```

Les compteurs de travail changent ; les identités non.

### G6 — quotient pair-major

Plusieurs branches témoins réclament le même split endpoint :

```text
endpoint_splits=1
pair_state_duplicate_keys=0
```

### G7 — parité finale q2/q3/q4

```text
pair-major source
== triple-task source
== exhaustive oracle
```

sur supports, owner, BallKey, `I_B/U_B` et niveau.

---

## 11. Statut synthétique

| Objet | Verdict |
|---|---|
| rétractation « profondeur gouvernée par le seuil » | reçue |
| correction `CORE_CLEAR` | reçue |
| réponse Q2 core/census | reçue, vocabulaire à qualifier |
| réponse Q3 classify/reduce/one-action | reçue |
| `frontier_peak` actuel | masse logique, mal nommé |
| `refine_depth_max` actuel | pas une profondeur |
| `refine_steps` actuel | itérations, pas splits |
| couverture des caps par les métriques | incomplète |
| `U4_closed` résiduel | à renommer avant ABI |
| grande frontière comme limite intrinsèque | non démontrée |
| certificat `NONE_OPEN_Wq` | théorème sûr proposé |
| démarrage de `PairFrame` | recommandé immédiatement |
| qualification GPU | ouverte |

---

## 12. Message direct à Claude

Ta réponse à l'audit est bonne : tu as rétracté exactement les deux claims qui
dépassaient les invariants, ajouté des observables, et tu n'as pas essayé de
sauver une phrase de complexité devenue fausse. C'est la bonne méthode.

Le prochain commit doit être moins spectaculaire et plus utile : nettoyer les
unités de télémétrie et le mot `closed`, puis extraire `PairFrame`.

Ne lis pas encore `frontier_peak ~= n` comme « le branch-and-bound exact exige
un CSR ». Ce compteur est une masse logique, et ton upper ne sait même pas
reconnaître un singleton situé dans `W2\W4` comme `NONE_W4`. Le certificat :

```text
c_q Hmax² <= Emin Xmin
```

ferme ce trou proprement, devient exact aux feuilles et coûte presque rien.
Mesure-le, mais ne laisse pas cette optimisation retarder le quotient
pair-major.

La direction reste donc positive et désormais assez claire :

```text
PairFrame d'abord,
q2 reçu de bout en bout,
NONE_Wq comme resserrement pur,
puis q3 et la chaîne q4 complète.
```

Le prototype vient de cesser de mentir sur son asymptotique. Il lui reste à
cesser de mentir sur les unités de ses compteurs, rite de passage moins noble
mais tout aussi nécessaire.
