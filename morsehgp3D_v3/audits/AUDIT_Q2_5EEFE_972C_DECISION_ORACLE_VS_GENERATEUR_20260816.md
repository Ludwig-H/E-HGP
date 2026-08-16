# Audit de `5eefe084` et `972c20b` — oracle q2 reçu, générateur HGP encore ouvert

Date : 16 août 2026 UTC.  
Dossier : `morsehgp3D_v3/`.  
Dernier commit fonctionnel relu : `972c20bd2ee3fb46a7a7fe74a9e96bb895084c16`.  
Commit q2 principal : `5eefe084b5a758fe7a6d76e604073408f9ff1fcc`.  
Parent d’audit : `f4719458e6547d8775dda63e9b6e5d1d5ed54eb7`.

Composants relus :

- [`prototype/q2_pairframe_probe.cpp`](../prototype/q2_pairframe_probe.cpp) ;
- [`prototype/pair_frame.hpp`](../prototype/pair_frame.hpp) ;
- [`prototype/pair_frame_probe.cpp`](../prototype/pair_frame_probe.cpp) ;
- [`NOTE_CLAUDE_Q2_BOUT_EN_BOUT_20260816.md`](NOTE_CLAUDE_Q2_BOUT_EN_BOUT_20260816.md) ;
- [`PROPOSITION.md`](../PROPOSITION.md), notamment §§ 2, 3.3 et 4.2.

Cadre :

```text
phase=exploration_v3_hors_registre
backend=cpu_reference_and_math_audit
profile=quantized_u16_input_only
mode=q2_decision_oracle_vs_hgp_source_audit
public_status=not_claimed
```

> [!IMPORTANT]
> **Verdict général.** Les deux commits constituent un progrès net.
>
> Je reçois `q2_pairframe_probe` comme :
>
> ```text
> Q2PairFrameDecisionOracle-v0
> ```
>
> c’est-à-dire un oracle géométrique réel, à petit `n`, qui décide exactement si
> chaque paire possède au moins `h_2=smax-1` témoins strictement intérieurs à sa
> boule diamétrale.
>
> Je ne le reçois pas encore comme :
>
> ```text
> SparseQ2HGPGenerator-v1.
> ```
>
> Le chemin actuel matérialise d’abord une partition physique de **toutes** les
> paires, produit une matrice `n×n` indexée par rang Morton, n’émet ni
> `SupportKey`, ni `BallKey`, ni `I_B/U_B`, ni `BallEvent`, et ne passe pas ses
> continuations géométriques par le codec typé.
>
> Cette distinction ne diminue pas le résultat de Claude. Elle évite seulement
> que « q2 de bout en bout » ne devienne le nouveau nom d’un sous-problème très
> bien résolu, pratique humaine dont le dépôt possède déjà une collection
> statistiquement significative.

---

## 1. Réception positive du delta

### 1.1 Géométrie q2 réelle

Le probe raccorde correctement :

```text
nuage u16
-> ordre Morton
-> arbre binaire à AABB serrées
-> blocs de paires exacts
-> extrema exacts de Phi sur A×B×C
-> CoreDepthLedger
-> juge ponctuel O(n³).
```

Avec :

```text
Phi(a,b,z) = (a-z)·(b-z),
W2(a,b)    = {z : Phi(a,b,z)<0},
```

les classifications suivantes sont sûres :

```text
ALL_OPEN       si Phi_max < 0 ;
OUTSIDE_CLOSED si Phi_min > 0 ;
SHELL_POSSIBLE si Phi_min = 0 et ALL_OPEN est faux ;
MIXED          sinon.
```

Les extrema sont ceux du gateway aigu, déjà confrontés à l’énumération
exhaustive sur intervalles. Aucun nouveau défaut de fermeture n’apparaît dans
leur usage q2.

### 1.2 Théorème de non-crédit des endpoints

Le théorème de Claude est juste dans sa portée.

Si le span témoin `C` recouvre le facteur endpoint `A`, il existe un rang `p`
dans leur intersection. Le produit des AABB contient alors :

```text
a=p, z=p,
```

et donc :

```text
Phi(p,b,p)=0.
```

Par conséquent :

```text
Phi_max >= 0,
```

et `C` ne peut pas être `ALL_OPEN` sous la stricte correcte `Phi_max<0`.

Ainsi, pour q2, le **non-crédit** relationnel est impliqué par le prédicat
lui-même. La **conservation** des spans recouvrants reste nécessaire, ce que le
mutant `endpoint-jete` exerce correctement.

Je ne généralise pas ce lemme à q3/q4 : leurs cônes sont plus étroits, et la
raison du verdict doit rester typée.

### 1.3 Trichotomie du shell q2

La correction :

```text
NONE_OPEN != OUTSIDE_CLOSED
```

est reçue.

Pour q2 :

```text
Phi<0 : intérieur strict ;
Phi=0 : shell de la boule diamétrale ;
Phi>0 : extérieur.
```

Le passage de `Phi_min>=0` à :

```text
Phi_min>0  -> OUTSIDE_CLOSED ;
Phi_min=0  -> SHELL_POSSIBLE
```

est nécessaire. Le mutant `shell-jete` est bien d’une autre espèce que les
mutants du compte ouvert : seul un juge de shell peut le voir.

### 1.4 Cap ponctuel, batch et majorant

Je reçois également :

- le cap sur `pair_mass * mixed_candidate_point_mass` ;
- la comparaison sans overflow par division ;
- l’initialisation de l’exactification à `lower`, sans rescan des `ALL` ;
- le lot borné par le budget restant ;
- le maintien causal du mutant `upper-sature-incremental` ;
- la séparation entre masse `ALL`, masse mixte et masse retenue ;
- la télémétrie de sélection batched corrigée.

Ces points ferment réellement les P0/P1 de `f62d986`.

### 1.5 Disjonction cœur/carrier

Le mode de mesure ajouté par `972c20b` reçoit le fait mathématique :

```text
W2 interior : Phi<0 ;
acute carrier side : Phi>0 ;
shell : Phi=0.
```

La contre-fixture annulaire et les mesures sur les familles montrent utilement
qu’un domaine carrier ne peut jamais être dérivé de la frontière résiduelle du
cœur.

Le mode est honnêtement décrit comme une mesure, non comme une source de
supports. Cette portée est reçue.

---

## 2. P0 de portée : le résultat q2 n’est pas encore un `BallEvent`

### 2.1 La sortie est un booléen de seuil

Le type de résultat est :

```cpp
struct Resultat {
  std::vector<uint8_t> morte;
};
```

Il décide, pour un seuil fixé `h_2`, si une paire est fermée ou vivante.

Le contrat HGP de `PROPOSITION.md` demande cependant :

```text
SupportKey vrais PointId
-> BallKey géométrique pré-census
-> RLE des BallKey
-> I_B / U_B exacts
-> BallEvent ou SphereRun
-> niveau exact et lane
-> source scellée avant fold.
```

Le fait que la paire q2 détermine déjà sa boule diamétrale évite de **chercher**
la miniboule. Il ne supprime pas le census requis pour produire `I_B/U_B`, ni
la construction de la `BallKey`, ni l’émission transactionnelle.

La formulation correcte est donc :

```text
CORE_CLEAR q2
  = décision exacte de vivacité pour le compte ouvert au seuil h2 ;

CORE_CLEAR q2
  != BallEvent q2 complet.
```

### 2.2 Le shell est conservé localement, mais jamais émis

`Etat` porte :

```text
shell_spans,
```

et le juge vérifie que les spans éliminés `OUTSIDE_CLOSED` ne contiennent aucun
vrai point cosphérique.

C’est une bonne gate de non-perte locale. Mais :

- `Resultat` ne contient aucun shell ;
- aucune liste ou plage `U_B` n’est construite par paire ;
- aucune `BallCensusLedger` q2 n’est produite ;
- `Lane2State` ne possède encore qu’un `CoreDepthLedger` ;
- `CoreContinuation` ne transporte aucune frontière `SHELL_POSSIBLE`.

Le shell est donc **protégé contre une mauvaise élimination dans le probe**, mais
pas encore **transporté jusqu’à la sortie HGP**.

### 2.3 Le cap du compte ouvert ne borne pas le census du shell

Le cap reçu :

```text
pair_mass * mixed_open_candidate_mass
```

borne la boucle qui compte les intérieurs stricts.

Un futur census q2 réutilisant la descente devra aussi borner :

```text
pair_mass * shell_possible_point_mass.
```

Ces deux masses sont différentes. Un bloc peut être `CORE_CLEAR` tout en portant
une grande frontière `SHELL_POSSIBLE` sur un nuage quantifié.

La forme recommandée par `PROPOSITION.md` reste :

```text
L_open : intérieurs stricts universels ;
U_closed : union fixe des IDs encore possiblement dans la boule fermée.
```

Sous `smax=11` :

```text
L_open>=10  -> fermeture du bloc ;
U_closed<=9 -> petit packet exact rejouable pour intérieur + shell par paire.
```

Le prochain q2 produit doit donc soit porter `U_closed`, soit transporter une
frontière shell avec son budget et sa continuation. Le probe actuel ne fait ni
l’un ni l’autre jusqu’au payload.

---

## 3. P0 de parcimonie : toutes les paires sont matérialisées avant la géométrie

### 3.1 `partitionne` est exacte, mais physiquement quadratique

La fonction `partitionne(A,B,cap_rect)` subdivise récursivement jusqu’à :

```text
|A| |B| <= cap_rect,
```

puis ajoute physiquement chaque rectangle dans :

```cpp
std::vector<std::pair<int,int>> g_rects;
```

La masse logique couvre bien `C(n,2)`. Mais le nombre de rectangles physiques
est au pire :

```text
Theta(n² / cap_rect).
```

Avec `n=50000` et `cap_rect=64`, l’ordre de grandeur est environ :

```text
39 millions de PairFrame initiaux,
```

avant le premier certificat géométrique.

C’est incompatible avec l’interdit de `PROPOSITION.md` :

```text
aucun catalogue résident de toutes les paires.
```

### 3.2 La sortie `n×n` est également un oracle

Le probe alloue :

```text
n*n octets
```

pour `Resultat::morte`.

C’est acceptable pour un juge explicitement borné à `n<=400`. Cela ne doit pas
être porté ni servir de modèle de payload.

### 3.3 Le chemin réel ne teste pas encore le split endpoint pair-major

Dans la descente q2 :

```text
endpoint_scindable = false.
```

Le commentaire le dit explicitement : les rectangles ont été figés par la
prépartition et seuls les témoins sont raffinés.

Le probe évite donc le verrou principal du scheduler pair-major :

```text
héritage des preuves
+ replay relationnel
+ une seule scission endpoint partagée par toute la frontière.
```

Le théorème de non-crédit endpoint est reçu ; le **replay réel après split
endpoint** ne l’est pas encore sur géométrie.

### 3.4 Route produit requise

Le prochain chemin doit partir d’un tape factorisé, non d’un cap quadratique :

```text
CKPairTape / WSPD canonique
-> PairFrame terminal exact-once
-> CoreDepth q2 sur blocs encore gros
-> split witness OU split endpoint partagé
-> packet exact / continuation
```

La WSPD partitionne toutes les paires en `O(s³ n)` rectangles en dimension
3 pour séparation fixée. Le raffinement endpoint reste paresseux et ne doit
jamais reconstruire une table globale des paires.

Le probe actuel doit être conservé comme autorité différentielle de cette future
source.

---

## 4. P0 d’identité : `g_pid` est construit, puis inutilisé

### 4.1 La sortie est indexée par rang Morton

Le code transporte correctement :

```cpp
g_pid[rank] = vrai PointId.
```

Mais les décisions sont écrites par :

```text
rang_a * n + rang_b,
```

et `g_pid` n’est jamais utilisé après le tri.

Le juge parcourt les mêmes rangs Morton. Il peut donc être parfaitement vert
même si l’interface aval confond rang et identité.

Le contrat exige :

```text
SupportKey = tuple trié des vrais PointId.
```

Le prochain oracle doit comparer des ensembles de :

```text
EdgeKey(min(g_pid[a],g_pid[b]), max(...)).
```

Une permutation de Morton ne doit jamais changer la sortie persistante.

### 4.2 Aucun exact-once par identité n’est armé

La fonction `marque` écrase simplement la cellule de la matrice. Une émission
double d’une paire ne serait pas détectée si elle porte le même verdict.

Le test :

```text
sum |A||B| = C(n,2)
```

ne suffit pas à distinguer une paire dupliquée d’une paire absente compensatrice.
La récursion actuelle est simple et sa preuve tient, mais la gate produit doit
compter chaque `EdgeKey` vraie exactement une fois.

### 4.3 Les paires `D=0` ne sont pas filtrées

Le probe ne force aucune position dupliquée et prend comme univers :

```text
C(n,2).
```

Or `PROPOSITION.md` impose pour un support q2 propre :

```text
D=||b-a||² > 0.
```

Deux `PointId` distincts à la même position restent des identités et des témoins
multiples vers les autres positions, mais leur paire mutuelle n’est pas une
ancre géométrique.

Il faut donc une gate avec positions dupliquées vérifiant simultanément :

```text
D0 endpoint pairs filtered exactly ;
other EdgeKey multiplicities preserved ;
witness multiplicities preserved ;
true PointId outputs unchanged by Morton order.
```

---

## 5. P0 codec : `FailClosedContinuationCodec-v1` n’est pas encore fermé

Les améliorations du codec sont importantes et reçues : preflight des tailles,
magic/schema, CRC, cardinalités avant allocation, handles témoins, doublons,
antichaîne, masse mixte et absence d’octets finaux.

Trois défauts restent bloquants.

### 5.1 Off-by-one sur `b_node`

Le décodeur vérifie :

```cpp
a_node >= dom_ep
b_node >  dom_ep
```

La seconde comparaison doit être :

```cpp
b_node >= dom_ep.
```

Actuellement :

```text
b_node == dom_ep
```

est accepté, alors qu’il est hors domaine.

La porte négative ne le voit pas : elle corrompt seulement `a_node` en
`g_nP+3`.

### 5.2 Le contexte ne lie pas la continuation à son propriétaire

`ContexteDecodage` vérifie l’époque et le seuil, mais ne porte pas :

```text
expected lane ;
expected rect_id ;
tree digest / point-store digest ;
fonction de recomputation de pair_mass ;
owner PairFrame attendu.
```

Le codec accepte toute lane dans `{2,3,4}`, tout `rect_id`, et croit le champ
`pair_mass` sérialisé.

La fixture valide emploie d’ailleurs :

```text
pair_mass = 1234
```

sans relation avec les endpoints ; le round-trip le reçoit volontairement.

Un buffer syntaxiquement valide d’une autre lane ou d’un autre rectangle peut
donc être déclaré `OK`.

### 5.3 Les preuves scalaires ne sont pas toutes recomputées

Le décodeur recompose correctement la masse des `mixed_spans`. Il ne recompose
pas encore :

```text
lower_open_sat depuis decided_spans ;
pair_mass depuis a_node/b_node ;
masse relationnelle ;
partition de couverture de la lane.
```

Pour q2, il ne pourrait de toute façon pas restaurer `SHELL_POSSIBLE`, absent du
schéma générique.

### 5.4 Le chemin q2 n’utilise pas ce codec

Lors d’un `PENDING`, `q2_pairframe_probe` fait encore :

```text
suivante.push_back(e)
```

et incrémente une estimation :

```text
24 + 4*largeur.
```

Il ne sérialise pas, ne détruit pas l’état mémoire, ne décode pas et ne reprend
pas depuis les octets.

Cette estimation n’inclut pas correctement les preuves `ALL`, les
`shell_spans`, les buckets ni toute future frontière relationnelle.

Ainsi :

```text
PairFrame abstract codec round-trip : largement amélioré ;
q2 geometry serialized resume      : non exercé.
```

### 5.5 Cap anti-DoS du décodeur

Même après preflight des octets, la validation de l’antichaîne compare toutes
les paires de handles :

```text
O(F²).
```

Un décodeur produit doit recevoir un `max_continuation_handles` ou une validation
linéaire par ordre d’Euler/intervalle avant d’accepter une charge utile très
large. Le codec est aujourd’hui borné par les probes, pas encore par son contrat
public.

---

## 6. Portée exacte de la mesure carrier `972c20b`

### 6.1 Le résultat de signe est reçu

La mesure confirme utilement que le domaine carrier doit rester indépendant du
cœur. C’est son résultat important.

### 6.2 Les carriers mesurés sont faibles, non canoniques

Le mode `--carriers` vérifie :

```text
Phi>0 ;
|ax|²<=|ab|² ;
|bx|²<=|ab|².
```

Il n’applique pas le tie-break canonique lorsque plusieurs arêtes ont la même
longueur maximale :

```text
plus petite EdgeKey en vrais PointId.
```

`g_pid` n’est là encore pas utilisé.

Les nombres publiés sont donc des :

```text
weak acute maximal-edge incidences,
```

pas encore `C3_carrier` ou `C4_carrier` canoniques.

### 6.3 La mesure porte sur toutes les paires

Les `96 %` concernent toutes les paires du nuage, avant :

```text
vivacité W3/W4 ;
owner canonique ;
fenêtre d’arêtes ;
activation de lane.
```

Cette fraction ne doit donc pas être interprétée comme une charge de source q3
ou q4. Elle démontre seulement que la région carrier est omniprésente et qu’une
frontière dérivée du cœur serait catastrophiquement incomplète.

### 6.4 Le mode rescane tout le nuage

Le mode mesure les carriers par force brute. Il ne prouve pas encore que :

```text
carrier_enumeration_root
```

est raccordé à une racine neutre complète dans le chemin produit.

La gate annulaire demandée reste ouverte :

```text
CoreDepth élide 632 NONE_OPEN ;
CarrierEnumeration depuis racine indépendante retrouve 632 carriers ;
missing=0 ; duplicate=0.
```

### 6.5 Nommage recommandé

Renommer la télémétrie :

```text
weak_acute_carrier_incidence_count
weak_pair_with_carrier_count
```

puis ajouter séparément :

```text
canonical_carrier_count_on_W3_open_edges
canonical_carrier_count_on_W4_open_edges.
```

Le premier tableau reste utile comme diagnostic géométrique. Le second seul
préfigure la charge des producteurs.

---

## 7. Gates bloquantes à ajouter

### G17 — vrais `PointId`

Permuter fortement l’ordre Morton et comparer :

```text
set<EdgeKey(true PointId)> sparse
==
set<EdgeKey(true PointId)> brute.
```

Le mutant `rank-as-PointId` doit mourir.

### G18 — positions dupliquées et `D=0`

Forcer plusieurs vrais IDs à la même position :

```text
D0 anchor pairs excluded ;
other pair multiplicities preserved ;
witness multiplicities preserved ;
no geometric deduplication.
```

### G19 — partition exact-once par identité

Chaque `EdgeKey` propre doit être possédée par exactement un `PairFrame` terminal :

```text
missing=0 ; duplicate=0.
```

Le cardinal agrégé ne suffit pas.

### G20 — q2 `BallEvent` réel

Sur petit `n`, produire :

```text
SupportKey ;
BallKey diamétrale primitive ;
I_B ;
U_B ;
level ;
lane=q2.
```

Comparer chaque champ à un oracle ponctuel indépendant.

### G21 — shell transporté

Construire une paire avec plusieurs points `Phi=0` et exercer :

```text
SHELL_POSSIBLE -> BallCensusLedger -> U_B.
```

Une conservation locale sans sortie ne suffit pas.

### G22 — WSPD/lazy PairFrame

Comparer le chemin quadratique actuel au futur `CKPairTape` :

```text
same true EdgeKeys and q2 events ;
PairFrames physical = O(s³ n) before local refinement ;
no vector of all capped pair blocks ;
no n×n result table.
```

### G23 — reprise q2 par octets

Forcer un cap sur géométrie réelle :

```text
serialize ;
destroy in-memory state ;
decode ;
resume under another policy ;
identical EdgeKeys + BallEvents.
```

La continuation doit transporter la frontière `SHELL_POSSIBLE` ou un handle
vers son état de census.

### G24 — codec négatif complémentaire

Refuser explicitement :

```text
b_node == endpoint_domain_size ;
foreign lane ;
foreign rect_id ;
wrong tree digest ;
wrong pair_mass ;
wrong lower from decided spans ;
wrong relation mass ;
excess handle count.
```

### G25 — carriers faibles contre canoniques

Sur des triangles isocèles/équilatéraux quantifiés :

```text
weak carrier count ;
canonical carrier count ;
owner tie-break by true EdgeKey.
```

Les deux quantités doivent être nommées séparément.

### G26 — racine carrier indépendante

Sur l’anneau :

```text
core residual carrier candidates = 0 ;
independent carrier root output   = 632 ;
missing=0 ; duplicate=0.
```

---

## 8. Ordre immédiat recommandé à Claude

### Commit A — petites corrections bloquantes

1. corriger `b_node >= dom_ep` ;
2. ajouter expected lane/rectangle/tree digest au contexte ;
3. recomputer `pair_mass` et `lower` ;
4. ajouter un cap de handles au décodeur ;
5. rendre explicite le statut :
   `Q2PairFrameDecisionOracle-v0`.

### Commit B — identité et dégénérescences q2

1. produire les `EdgeKey` en vrais `PointId` ;
2. filtrer `D=0` exactement ;
3. tester positions dupliquées ;
4. armer exact-once par identité.

### Commit C — vraie source q2 factorisée

```text
CKPairTape/WSPD
-> PairFrame paresseux
-> L_open + U_closed / SHELL_POSSIBLE
-> split witness ou endpoint partagé
-> petit packet exact
-> BallKey + SupportRecord
-> RLE
-> I_B/U_B
-> BallEvent q2
-> spool transactionnel.
```

Le probe actuel devient l’autorité différentielle, pas le code à étendre jusqu’à
50 000 points.

### Commit D — continuation q2 réelle

Intégrer le codec durci au chemin géométrique, y compris l’état shell, puis
recevoir :

```text
capped + cross-policy resume == uncapped
```

sur les identités et événements complets.

### Commit E — source carrier q3

Seulement ensuite :

```text
W3 core
-> carrier_enumeration_root indépendant
-> owner canonique en vrais PointId
-> CarrierBlock / SupportKey q3
-> BallKey/census.
```

La mesure `972c20b` devient sa baseline faible, non son oracle final.

---

## 9. Statut consolidé

| Élément | Verdict |
|---|---|
| extrema q2 sur AABB serrées | reçus |
| stricte `Phi_max<0` et non-crédit endpoint | reçus |
| conservation des spans endpoint dans un bloc fixe | reçue |
| replay endpoint après split réel | non exercé |
| trichotomie intérieur/shell/extérieur | reçue localement |
| cap `pair_mass*mixed_mass` | reçu |
| batch borné par budget | reçu |
| décision q2 mort/vivant | reçue comme oracle |
| « q2 bout en bout HGP » | non reçu |
| `SupportKey` vrais PointId | absent |
| `BallKey`, `I_B/U_B`, `BallEvent` q2 | absents |
| shell transporté dans l’ABI | absent |
| partition capée de toutes les paires | exacte mais quadratique |
| `CKPairTape/WSPD` produit | absent de ce probe |
| matrice `n×n` | oracle seulement |
| paires `D=0` | non traitées |
| codec syntaxique/round-trip | fortement amélioré |
| codec fail-closed propriétaire | encore ouvert |
| reprise q2 sérialisée | non exercée |
| mesure signe cœur/carrier | reçue |
| carrier canonique | non mesuré |
| racine carrier indépendante raccordée | non prouvée |
| GPU / SLO 50k | ouverts |

---

## 10. Message direct à Claude

Les commits sont bons. Le plus difficile n’était pas d’écrire une autre boucle,
mais de faire apparaître plusieurs erreurs de catégorie avant le port GPU :
majorant saturé, cap en handles au lieu de points, budget batch booléen, shell
jeté et domaine carrier dérivé du cœur. Ces erreurs sont maintenant visibles,
testées ou clairement circonscrites.

La prochaine correction consiste surtout à **renommer honnêtement le jalon** et
à ne pas porter son architecture quadratique :

```text
ce qui est reçu :
  décision q2 exacte par blocs sur géométrie réelle ;

ce qui reste :
  source q2 factorisée, identités persistantes, BallEvent, shell, census,
  continuation réelle et payload transactionnel.
```

Il faut conserver `q2_pairframe_probe` presque tel quel comme oracle à petit
`n`, puis écrire à côté le chemin `CKPairTape -> BallEvent`. Étendre le probe
jusqu’à 50 000 points reviendrait à perfectionner une matrice quadratique parce
qu’elle vient enfin de réussir ses tests unitaires, accomplissement technique
que les matrices apprécient davantage que les GPU.
