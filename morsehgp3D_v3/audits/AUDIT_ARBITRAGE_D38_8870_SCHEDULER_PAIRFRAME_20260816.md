# Arbitrage des contre-audits `d38cc11` et `8870e6f` avant `PairFrame`

Date : 16 août 2026 UTC.  
Dossier : `morsehgp3D_v3/`.  
Dernier commit fonctionnel commun relu : `a6171d1827245f530eede9bbc4e9b1b3407121ed`.

Documents arbitrés :

- [`CONTRE_AUDIT_A617_PROFONDEUR_FRONTIERE_PAIRFRAME_20260816.md`](CONTRE_AUDIT_A617_PROFONDEUR_FRONTIERE_PAIRFRAME_20260816.md), commit `d38cc11` ;
- [`AUDIT_RECEPTION_A617_TELEMETRIE_NONE_WQ_PAIRFRAME_20260816.md`](AUDIT_RECEPTION_A617_TELEMETRIE_NONE_WQ_PAIRFRAME_20260816.md), commit `8870e6f`.

Cadre :

```text
phase=exploration_v3_hors_registre
backend=math_reference_and_gpu_architecture
profile=quantized_u16_input_only
mode=arbitrage_two_auditors_before_pairframe
public_status=not_claimed
```

> [!IMPORTANT]
> **Verdict commun.** Les deux contre-audits convergent et sont compatibles :
>
> - `a6171d1` corrige justement les claims de complexité ;
> - `frontier_peak`, `refine_depth_max`, `refine_steps` et
>   `continuation_mass` doivent être renommés et séparés par unité ;
> - `PairFrame` doit commencer maintenant, après un petit nettoyage de types ;
> - le scheduler ternaire reste un oracle, pas une architecture à polir.
>
> Le contre-audit `d38cc11` apporte une correction importante : pour **ce radix
> LBVH précis**, la profondeur structurelle n'est pas linéaire en `n`. Elle est
> bornée par les bits de Morton et le tie-break d'indice. Je reçois cette
> correction.
>
> Deux compléments sont toutefois nécessaires :
>
> 1. les buckets ne suppriment pas seuls le coût quadratique, car le code rescane
>    aussi toute la frontière pour recalculer sa masse à chaque itération ;
> 2. la bucketisation est une politique de scheduling, pas un champ sémantique
>    du `CoreDepthLedger` ni de la continuation.
>
> La solution complète est : **masse exacte maintenue incrémentalement + choix
> bucketisé ou segmenté + raffinement par batch + continuation indépendante de
> la politique**.

---

## 1. Réception du contre-audit `d38cc11`

### 1.1 La distinction profondeur / nombre de splits est juste

`refine_depth_max` compte des tours de boucle sur des branches éventuellement
incomparables. Le renommer en :

```text
state_refine_iterations_max
```

est nécessaire.

Les compteurs utiles restent :

```text
witness_splits_total
witness_splits_max_per_state
witness_node_depth_max
state_refine_iterations_max
```

### 1.2 Borne correcte du radix LBVH courant

Dans `wspd_wavefront.hpp` :

```text
MortonKey : 48 bits utiles
wf_delta distinct keys : longueur de préfixe parmi ces 48 bits
wf_delta equal keys    : tie-break par préfixe des indices triés
```

Le long d'un chemin interne d'un Patricia/radix tree, la longueur de préfixe
augmente strictement. Pour `n>1`, une formulation générale est :

```text
internal_depth <= 48 + ceil(log2 n)
leaf_depth     <= 49 + ceil(log2 n)
```

à une unité de convention près.

Pour la cible :

```text
n <= 50000 < 2^16,
```

on obtient :

```text
internal_depth <= 64
leaf_depth     <= 65.
```

La correction de ma phrase générique sur un arbre binaire non équilibré est donc
reçue : elle ne décrivait pas le backend radix fixé du dossier.

Cette borne doit cependant être typée :

```text
RadixLBVHDepthBound-48bitMorton-indexTie
```

et non promue comme propriété de toute future partition ou de tout backend.
Le code devrait imposer ou publier `n`, la largeur de Morton et le codec du
tie-break dont la borne dépend.

### 1.3 Le coût du `vector` est correctement signalé

Le contre-audit a raison : rechercher le maximum puis faire :

```cpp
frontiere.erase(frontiere.begin()+best)
```

peut coûter :

```text
sum_t Theta(F_t),
```

jusqu'à `Theta(n²)` pour un état si `F_t=Theta(n)` durant `Theta(n)` splits.

Le scheduler à buckets est une bonne politique CPU de référence.

---

## 2. Complément bloquant : un second rescan quadratique subsiste

Même après remplacement de la recherche du maximum et de `erase`, la boucle
courante exécute à chaque tour :

```cpp
long long mf = 0;
for (int h : frontiere)
    mf += population(h);
```

afin de tester :

```text
L + mf < h_q.
```

Un scheduler à buckets qui conserverait ce calcul reste en :

```text
sum_t Theta(F_t).
```

Il ne suffit donc pas de bucketiser la sélection. Il faut maintenir la masse de
frontière **incrémentalement**.

---

## 3. Lemme de mise à jour incrémentale du ledger

### 3.1 État

Pour un état endpoint et une lane, conserver :

```text
L = masse ALL_OPEN déjà créditée
M = somme exacte des populations des spans encore possibles
U = L + M.
```

L'invariant est :

```text
pour toute paire p de l'état : L <= N_q(p) <= U.
```

### 3.2 Split d'un span

Soit un parent `C` de population `m(C)`. Lorsqu'il est remplacé par ses enfants :

1. retirer `m(C)` de `M` ;
2. pour chaque enfant `D` :
   - `ALL_OPEN` : ajouter `m(D)` à `L` ;
   - `NONE_OPEN` : ne rien ajouter ;
   - `MIXED/MIXED_ENDPOINT` : ajouter `m(D)` à `M`.

Comme les enfants partitionnent le parent :

```text
sum_D m(D) = m(C).
```

On conserve :

```text
L non décroissant,
U non croissant,
L <= N_q(p) <= U.
```

Les tests terminaux deviennent `O(1)`.

### 3.3 Attention à la saturation

`L` peut être saturé à `h_q`, puisqu'alors l'état est terminal. En revanche, il
est dangereux de ne conserver qu'un `U` saturé : après un split qui prouve
beaucoup de `NONE`, il faut pouvoir redescendre de `h_q` vers une valeur
strictement inférieure.

Le futur état doit donc porter soit :

```text
frontier_candidate_mass_exact
```

soit une structure permettant de le réduire exactement depuis les spans.

ABI recommandée sous la cible 50k :

```cpp
struct CoreDepthLedger {
  uint8_t threshold;
  uint8_t lower_open_sat;
  uint32_t frontier_candidate_mass_exact;

  FrontierHandle frontier;
  RelationFrontierHandle relation_frontier;
  CreditProofHandle credits;
  ContinuationHandle continuation;
};
```

Puis :

```text
upper_open_sat = min(h_q,
                     lower_open_sat + frontier_candidate_mass_exact).
```

Dans une vague GPU, `M` peut être recalculé par réduction segmentée des jobs ;
dans la référence CPU persistante, la mise à jour incrémentale évite les
rescans.

---

## 4. Buckets : bonne politique, mauvais candidat pour l'ABI

### 4.1 Politique CPU reçue

Sous `n<=65535` :

```text
bucket(C) = floor(log2 population(C)) in {0,...,15}
```

avec un masque de buckets non vides fournit une sélection `O(1)` à un facteur
deux du plus gros span. La correction ne dépendant pas de l'ordre, c'est sûr.

### 4.2 Ne pas sérialiser les buckets comme preuve

Le `CoreDepthLedger` doit représenter :

- les spans ;
- leurs statuts ;
- les crédits ;
- les masques relationnels ;
- les bornes ;
- la continuation.

Il ne doit pas dépendre de :

```text
largest-exact
log2-buckets
FIFO
deepest-first
```

La bucketisation appartient au scheduler. Un même continuation record doit
pouvoir être repris sous une autre politique et produire les mêmes identités.

`PolicyVersion` peut être enregistré pour la reproductibilité des performances,
mais il ne doit pas être nécessaire à la correction ni changer la sémantique du
record.

Gate forte :

```text
cap under policy A
resume under policy B
== uncapped under policy C
```

par identités.

### 4.3 Forme GPU préférée

Plutôt qu'un tableau de seize vecteurs par état :

```text
WitnessJobSoA:
  state_id
  node
  relation
  population
  priority_bucket
```

Une réduction segmentée par `state_id` rend :

```text
lower increment
frontier mass
max bucket
selected job or selected batch
terminal fate
```

Le stockage reste plat et compact ; la politique de priorité peut changer sans
migration de l'ABI.

---

## 5. Lemme de raffinement par batch

La règle « une action par état » n'impose pas « un seul span par action ».

Soit `S` un sous-ensemble de spans mixtes non-feuilles d'une antichaîne `F`.
Remplacer simultanément chaque parent de `S` par tous ses enfants :

- préserve l'antichaîne, car les parents sont deux à deux incomparables ;
- préserve la partition de masse ;
- conserve l'invariant `L<=N<=U` par application indépendante du lemme du § 3 ;
- ne dépend d'aucun ordre entre les splits.

Une action GPU peut donc être :

```text
SPLIT_WITNESS_BATCH(state, selected_spans[0:B])
```

avec par exemple les `B` spans du plus haut bucket.

Cette version évite une vague globale par split. La première référence peut
commencer avec `B=1`, mais l'ABI doit autoriser `B>1`, faute de quoi une famille
à `Theta(n)` splits imposerait `Theta(n)` synchronisations globales. Apparemment,
les GPU n'apprécient pas davantage que les humains les réunions où une seule
chose est décidée à chaque tour.

---

## 6. Continuation : compléter le schéma de `d38cc11`

Le schéma proposé par l'autre auditeur est une bonne base, mais un simple :

```text
lower_open_sat
mixed_frontier
relation_frontier
```

ne donne pas encore une preuve audit-able des crédits déjà consommés.

Deux options sûres :

```text
A. transporter les spans ALL_OPEN crédités ;
B. transporter un CreditProofHandle scellé avec digest, masse et provenance.
```

Une continuation minimale devient :

```cpp
struct CoreContinuation {
  PairFrameHandle pair_frame;
  uint8_t lane;
  uint8_t threshold;
  uint8_t lower_open_sat;
  uint32_t frontier_candidate_mass_exact;

  CreditProofHandle credits;
  SpanRange mixed_frontier;
  SpanRange relation_frontier;

  CloudEpoch epoch;
  SchemaVersion schema;
};
```

Les statistiques de politique restent dans le reçu, pas dans la vérité
mathématique du record.

Tous les chemins de capacité doivent converger vers ce type :

```text
initial frontier cap
target refinement cap
endpoint split cap
output cap
```

La coexistence actuelle de `pending` et `cap_hits` hors d'un ledger commun reste
à supprimer.

---

## 7. Le certificat `NONE_Wq` reste complémentaire et prioritaire après le squelette

Le contre-audit `d38cc11` corrige le scheduler, mais pas la sélectivité du
majorant. Le code ne sait exclure que :

```text
NONE_W2 : Hmax<=0.
```

Il conserve donc dans `upper_open` les points de `W2\W3` ou `W2\W4`, y compris
aux feuilles.

Le certificat de `8870e6f` :

```text
Hmax = -phi_min
Emin = dist²(A,Z)
Xmin = dist²(B,Z)

q3 NONE si 4 Hmax² <= Emin Xmin
q4 NONE si 3 Hmax² <= Emin Xmin
```

est sûr et devient exact sur trois singletons. Il doit être ajouté comme
primitive pure après ou pendant l'extraction du squelette, sans retarder
`PairFrame`.

La séquence rationnelle est donc :

```text
nettoyer les types et compteurs
-> extraire PairFrame/CoreDepthLedger
-> brancher masse incrémentale + scheduler amorti
-> ajouter NONE_Wq
-> mesurer séparément sélectivité et coût
```

Sans `NONE_Wq`, une masse candidate proche de `n` peut refléter le manque d'un
certificat évident. Sans scheduler amorti, un bon `NONE_Wq` peut encore être
masqué par les rescans du conteneur. Les deux verrous sont orthogonaux.

---

## 8. Résidus à corriger dans le code avant copie vers `PairFrame`

1. `U4_closed` doit devenir `U4_open` ou `upper_open` ;
2. `kActiveAll` doit être qualifié, par exemple
   `kAllCarrierCoreClear` ;
3. les deux commentaires encore présents sur une profondeur « gouvernée par le
   seuil » doivent disparaître ;
4. `frontier_peak` doit être séparé en masse, spans et octets ;
5. `refine_depth_max` doit disparaître ;
6. `pending` et `cap_hits` doivent alimenter le même type de continuation ;
7. le shell final reste obligatoire pour q2/q3/q4, seule l'identification
   `boundary(Wq)=shell` étant propre à q2.

---

## 9. Ordre de commits commun proposé à Claude

### A. Nettoyage sémantique très court

```text
U_open
frontier_candidate_mass_peak
frontier_span_count_peak
witness_splits
state_refine_iterations
continuation reasons
```

Aucune nouvelle optimisation.

### B. `PairFrame` CPU avec parité

```text
PairFrame immuable
LaneState autonome
CoreDepthLedger
flat WitnessJobs
one action per state
```

Comparer au triple-task par identités.

### C. Scheduler amorti

CPU : buckets ou heap sans effacement central.  
GPU : réduction segmentée et batch optionnel.  
Dans les deux cas : `frontier_candidate_mass_exact` maintenue sans rescan.

### D. q2 de bout en bout

```text
PairFrame -> W2 -> BallKey -> I_B/U_B -> fold input
```

### E. `NONE_W3/NONE_W4`

Gate annulaire, mutants de stricte et coefficients, parité par identités.

### F. q3 puis q4 complet

Aucun raccourci existentiel q4 ne modifie q3 ; l'activation carrier q4 conserve
la racine d'énumération complète.

---

## 10. Statut d'arbitrage

| Point | Verdict |
|---|---|
| correction de `refine_depth_max` | reçue des deux audits |
| borne radix `48+ceil(log2 n)` | reçue sous backend/profil explicites |
| masse logique vs spans physiques | reçue des deux audits |
| coût quadratique du vector-max | reçu |
| buckets comme politique CPU | reçus |
| buckets seuls comme solution linéaire | incomplets sans masse incrémentale |
| buckets dans l'ABI de preuve | déconseillés |
| batch de spans par action | sûr et recommandé pour GPU |
| continuation de `d38cc11` | bonne base, crédits/provenance à ajouter |
| certificat `NONE_Wq` de `8870e6f` | complément sûr, à mesurer |
| démarrage de `PairFrame` | immédiat après nettoyage court |

---

## 11. Message direct aux deux développeurs/auditeurs

Les deux audits indépendants ont trouvé le même défaut de mesure, ce qui est un
bon signe : il ne s'agit pas d'une querelle de vocabulaire, mais d'une vraie
séparation entre logique, stockage et travail.

Le contre-audit `d38cc11` a raison sur la profondeur du radix et sur le coût du
sélecteur. Il manque seulement le rescan de `mf`, qui maintiendrait le
quadratique même avec des buckets. Le contre-audit `8870e6f` ajoute le
resserrement géométrique `NONE_Wq`, qui ne remplace pas le scheduler mais évite
de lui faire résoudre des points déjà décidables.

La bonne synthèse tient en quatre lignes :

```text
M exact maintenu incrémentalement ;
priorité bucketisée/segmentée, hors ABI ;
raffinement par batch autorisé ;
NONE_Wq pour rendre les feuilles exactes.
```

Avec cela, `PairFrame` peut être construit sans importer les ambiguïtés du
prototype. Continuer à optimiser le vieux `std::vector` serait désormais une
forme très élaborée de procrastination architecturale.
