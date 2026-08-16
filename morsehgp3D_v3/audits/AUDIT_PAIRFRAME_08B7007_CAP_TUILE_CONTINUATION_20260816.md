# Audit de `PairFrame` après `08b7007` — cap de tuile, batch et continuation

Date : 16 août 2026 UTC.  
Dossier : `morsehgp3D_v3/`.  
Dernier commit fonctionnel relu : `08b7007ac42ed2d0dc4d10805f3e7a18cba622d0`.  
Parent fonctionnel principal : `566a05e39fc8354431664ec013764bc224bb876d`.  
Audit d’arbitrage contre-audité : `9739e3c1b266bd9fa1fea618b4729399e2599f00`.

Composants relus :

- [`prototype/pair_frame.hpp`](../prototype/pair_frame.hpp) ;
- [`prototype/pair_frame_probe.cpp`](../prototype/pair_frame_probe.cpp) ;
- [`prototype/acute_owner_gateway.hpp`](../prototype/acute_owner_gateway.hpp) ;
- [`prototype/acute_owner_gateway_probe.cpp`](../prototype/acute_owner_gateway_probe.cpp) ;
- [`NOTE_CLAUDE_PAIRFRAME_ORDONNANCEUR_20260816.md`](NOTE_CLAUDE_PAIRFRAME_ORDONNANCEUR_20260816.md) ;
- [`AUDIT_ARBITRAGE_D38_8870_SCHEDULER_PAIRFRAME_20260816.md`](AUDIT_ARBITRAGE_D38_8870_SCHEDULER_PAIRFRAME_20260816.md).

Cadre :

```text
phase=exploration_v3_hors_registre
backend=cpu_reference
profile=abi_modele_abstrait
mode=pairframe_resource_and_serialization_audit
public_status=not_claimed
```

> [!IMPORTANT]
> **Verdict général.** `PairFrame + CoreDepthLedger` est une bonne extraction de
> l’ordonnanceur et doit être conservé. La correction de `08b7007` est
> mathématiquement juste : `lower` peut être saturé, le majorant ne peut pas être
> maintenu sous forme saturée ; il faut transporter la masse candidate exacte et
> dériver `upper_open_sat`.
>
> Je reçois aussi la séparation des lanes, l’invariance de résultat sous
> politique, le principe du batch et le scheduler à buckets.
>
> Je ne reçois toutefois pas encore le **contrat de ressources** annoncé. Le cap
> de tuile borne actuellement
>
> ```text
> pair_mass * frontier_width,
> ```
>
> tandis que l’exactification du probe parcourt tous les points des spans mixtes.
> Son coût ponctuel est au moins
>
> ```text
> pair_mass * frontier_candidate_mass_exact.
> ```
>
> Un unique gros span mixte a une largeur physique égale à un, mais peut contenir
> 256, 50 000 ou davantage de points. Le cap courant peut donc sous-estimer le
> travail exact d’un facteur égal à la population du span. C’est le P0 du delta.
>
> Deux P1 suivent : un batch peut dépasser le budget de scissions restant, et la
> « continuation sérialisée » n’est encore reçue que pour un aller-retour de
> tampons valides, pas comme codec robuste, versionné et fail-closed.

---

## 1. Réception positive du squelette `PairFrame`

### 1.1 Tronc commun et autonomie des lanes

La structure :

```text
PairFrame immuable
  + Lane2State
  + Lane3State
  + Lane4State
```

est correcte. Elle grave enfin la distinction entre :

- partage d’index et de géométrie pure ;
- partage interdit des fates, continuations, preuves et sorties de lane.

En particulier :

- q3 conserve une relation carrier énumérative ;
- q4 sépare existence d’un carrier et racine d’énumération complète ;
- une décision q4 ne peut pas fermer q3.

Cette partie est reçue comme **ABI conceptuelle**. Le probe abstrait ne reçoit
pas encore les prédicats géométriques, le masque endpoint relationnel ni le
census ; Claude le dit explicitement et ne survend pas cette portée.

### 1.2 Invariant du ledger

Pour un bloc de paires `P` et une antichaîne de témoins, noter :

```text
L = masse des spans ALL_OPEN ;
M = masse exacte des spans encore possibles ;
U = L + M.
```

Alors, pour chaque paire ponctuelle `p` du bloc :

```text
L <= N_q(p) <= U.
```

Le split d’un span retire sa population de `M`, puis :

- enfant `ALL` : ajoute sa population à `L` ;
- enfant `NONE` : ne rajoute rien ;
- enfant `MIXED` : rajoute sa population à `M`.

L’invariant et la monotonie sont correctement maintenus dans le probe.

### 1.3 Correction de `08b7007`

La nouvelle représentation :

```cpp
uint8_t  lower_open_sat;
uint32_t frontier_candidate_mass_exact;

upper_open_sat()
  = min(h_q, lower_open_sat + frontier_candidate_mass_exact);
```

est correcte sous le contrat suivant :

- si `lower>=h_q`, l’état est terminal et ne descend plus ;
- dans tout état non terminal, `lower<h_q`, donc `lower_open_sat=lower` exactement ;
- la masse candidate reste non saturée et peut diminuer après élimination de
  spans `NONE`.

Ainsi le majorant dérivé peut redescendre correctement sous le seuil. Cette
réparation est reçue.

### 1.4 Buckets et politiques croisées

La bucketisation par :

```text
floor(log2(population))
```

est une bonne politique de référence. Elle ne participe pas à la correction et
ne doit pas entrer dans la continuation. Les reprises croisées entre politiques
sont donc une excellente gate.

Le lemme de batch est également juste : des parents appartenant à une
antichaîne sont incomparables ; les remplacer simultanément par tous leurs
enfants préserve l’antichaîne et la partition de masse.

---

## 2. P0 — le cap de tuile mesure la mauvaise quantité

### 2.1 Ce que calcule l’ABI

Dans `pair_frame.hpp`, le coût courant est :

```cpp
cout_tuile = pair_mass * frontier_width;
```

La largeur est un nombre de handles de spans.

Cette quantité est pertinente pour une partie de la HWM de la frontière, mais
elle ne borne pas le travail ponctuel d’une exactification.

### 2.2 Ce que fait réellement l’exactificateur

Dans `pair_frame_probe.cpp`, pour chaque paire, l’exactification :

1. ajoute la masse connue des spans `ALL` ;
2. parcourt chaque point des spans mixtes feuilles ;
3. parcourt également chaque point de chaque span mixte interne.

Si `M` désigne la somme des populations des spans mixtes :

```text
M = frontier_candidate_mass_exact,
```

le nombre de lectures du prédicat `statut(p,w)` est exactement :

```text
pair_mass * M
```

à un terme de gestion près.

La largeur physique :

```text
F = nombre de spans ALL ou MIXED
```

peut être très inférieure à `M`.

### 2.3 Contre-exemple minimal

Prendre :

```text
pair_mass = 64
un unique span mixte de population 256
frontier_width = 1
frontier_candidate_mass_exact = 256
exact_tile_cap = 64
```

Le scheduler courant calcule :

```text
cout_tuile = 64 * 1 = 64
```

et choisit `EXACTIFY_TILE`.

Le code exécute pourtant :

```text
64 * 256 = 16 384
```

lectures ponctuelles du statut.

Le cap est dépassé d’un facteur `256`. Sur la cible 50k, le même défaut peut
atteindre un facteur proche de `50 000`.

Ce n’est pas une objection asymptotique abstraite : c’est une divergence directe
entre la fonction de coût et la boucle qu’elle prétend caper.

### 2.4 Deux ressources distinctes

Il faut séparer :

```text
frontier_handle_cap
  borne le nombre de handles/spans résidents ;

exact_point_eval_cap
  borne le nombre d’évaluations ponctuelles de la tuile.
```

La seconde décision doit employer :

```text
pair_mass * frontier_candidate_mass_exact
```

et non `pair_mass * frontier_width`.

Les spans `ALL` n’ont pas à être rescannés par paire : l’exactificateur peut
initialiser directement :

```cpp
count = lower;
```

puis ne parcourir que les candidats mixtes. Le coût ponctuel est alors exactement
le produit ci-dessus.

### 2.5 Comparaison sans overflow

Ne pas former aveuglément le produit. Tester :

```cpp
bool tile_fits(uint64_t pair_mass,
               uint32_t candidate_mass,
               uint64_t cap) {
  return candidate_mass == 0 ||
         pair_mass <= cap / candidate_mass;
}
```

Sous la cible courante le produit tient dans `uint64_t`, mais une ABI de cap n’a
aucune raison d’introduire un overflow là où une division suffit.

### 2.6 Gate bloquante

Ajouter une fixture directe :

```text
pair_mass=64
frontier_width=1
frontier_candidate_mass_exact=256
cap=64
```

Exiger :

```text
action != EXACTIFY_TILE
```

et tuer :

```text
tile-cost-uses-span-width
```

Ajouter aussi un compteur réel :

```text
exact_point_predicate_evaluations
```

avec la porte :

```text
si une tuile est acceptée :
  exact_point_predicate_evaluations <= exact_point_eval_cap
```

Le test `adversaire, paires=64, témoins=256` est une fixture naturelle.

---

## 3. P1 — le batch peut dépasser le budget restant

La boucle actuelle transforme le budget en booléen :

```text
witness_budget_available = (budget > 0).
```

Puis `scinde_lot` peut sélectionner `B=4`, `B=8` ou `B=0` signifiant « tous »,
et la boucle fait seulement ensuite :

```text
budget -= faits.
```

Ainsi :

```text
budget restant = 1
witness_batch = 8
```

peut produire huit scissions et rendre le budget négatif.

Le batch est mathématiquement sûr, mais le contrat de ressources ne l’est pas.

### Correctif recommandé

L’action doit transporter une taille de lot :

```cpp
struct ActionRecord {
  Action kind;
  uint32_t witness_count;
};
```

avec :

```text
B_effectif = min(B_demandé, budget_restant, spans_scindables)
```

et, pour `B=0` :

```text
B_effectif = min(spans_scindables, budget_restant).
```

Une simple propriété booléenne ne suffit plus dès que l’action est batchée.

### Gate

```text
budget_temoins=1
witness_batch=8
```

Exiger, dans la vague :

```text
witness_splits <= 1
budget_remaining >= 0
```

puis reprise ou autre action exacte, sans divergence de résultat.

---

## 4. P1 — la continuation n’est reçue que sur des octets valides

### 4.1 Résultat positif reçu

Le test actuel prouve utilement :

```text
serialize(valid state)
-> abandon memory state
-> deserialize
-> resume under another policy
-> same decisions as uncapped brute force.
```

C’est une vraie avancée. La politique n’est pas entrée dans la sémantique du
record.

### 4.2 Ce qui manque à un codec produit

Le parseur courant :

- lit les octets sans vérifier la longueur restante ;
- accepte des cardinalités `na/nm` sans preflight ;
- ne vérifie ni magic, ni longueur totale, ni fin exacte du tampon ;
- lit `schema_version` et `cloud_epoch`, mais ne les confronte pas à l’exécution ;
- ne valide pas les `NodeHandle` ;
- ne vérifie pas l’antichaîne, les doublons ou les relations ancêtre-descendant ;
- ne transporte pas encore la frontière relationnelle dans ce modèle abstrait ;
- n’authentifie pas la topologie LBVH ou l’identité du nuage.

Un tampon tronqué ou un champ de taille corrompu peut donc provoquer une lecture
hors limites plutôt qu’un `invalid_input` typé.

### 4.3 Statut correct

Je reçois :

```text
ValidContinuationRoundTrip-v0
```

Je ne reçois pas encore :

```text
FailClosedContinuationCodec-v1.
```

### 4.4 Forme recommandée

```cpp
struct ContinuationHeader {
  uint32_t magic;
  uint16_t schema;
  uint16_t header_bytes;
  uint64_t payload_bytes;
  uint64_t cloud_epoch;
  uint64_t tree_digest;
  uint32_t crc32_or_digest;
};
```

Le décodeur doit retourner un type :

```text
expected<CoreContinuation, DecodeError>
```

et préflighter avant toute lecture ou allocation.

Après décodage, vérifier :

```text
handles in range
spans form an antichain
no duplicates
all/mixed/relation ranges disjoint
stored candidate mass == recomputed candidate mass
stored lower == proof mass, below threshold
rect/lane/epoch/schema match the active run
no unconsumed trailing bytes
```

### 4.5 Gates négatives

Pour chaque record valide :

- tronquer à chaque frontière de champ ;
- corrompre `na` et `nm` ;
- injecter un handle hors domaine ;
- dupliquer un span ;
- insérer un parent et son descendant ;
- changer l’époque, le schéma ou le digest ;
- ajouter des octets finaux inattendus.

Tous ces cas doivent rendre :

```text
invalid_input
```

sans lecture hors limites ni publication partielle.

---

## 5. P1 de réception — le mutant du majorant n’isole pas encore la faute subtile

La correction de production est juste. Le mutant `upper-sature-incremental`
n’est en revanche pas encore causalement précis.

Dans le probe :

```text
retire_du_bucket
  soustrait la population du parent au majorant mutant ;

Etat::ajoute
  ne rajoute jamais la population possible des enfants au majorant mutant.
```

Le mutant peut donc mourir simplement parce qu’il oublie tous les enfants,
même lorsque les deux enfants restent mixtes et que la masse candidate n’a pas
diminué. Cela teste « drop children », faute beaucoup plus grossière que le piège
de saturation.

### Mutant causal recommandé

Maintenir le mauvais majorant selon l’algorithme plausible :

```text
upper_sat_bad
  = min(h,
        upper_sat_bad
        - parent_population
        + possible_children_population).
```

La faute vient alors uniquement de l’information cachée au-dessus de `h`.

### Fixture exacte

Prendre `h=10` avec deux spans candidats :

```text
C0 : population possible 20, reste possible ;
C1 : population possible 5, devient entièrement NONE après split.
```

Avant split :

```text
raw upper = 25
stored bad upper = 10
```

Après élimination de `C1` :

```text
raw upper = 20 >= 10
stored bad upper = 5 < 10
```

Le mutant déclare faussement `CORE_CLEAR`. Cette gate tue exactement la faute
annoncée, et non l’oubli des enfants.

---

## 6. P2 — télémétrie et portes de coût à nettoyer

### 6.1 `frontier_candidate_mass_peak` inclut actuellement les spans `ALL`

Dans `Etat::ajoute`, la variable `masse_candidate` est incrémentée avant de
savoir si le span est `ALL` ou `MIXED`. Elle mesure donc la population retenue
par les spans non-`NONE`, pas la seule masse candidate indécise.

Or le vrai champ du ledger est :

```text
upper - lower.
```

Publier séparément :

```text
all_proof_population_peak
mixed_candidate_population_peak
retained_population_peak
all_span_count_peak
mixed_span_count_peak
```

Le champ `frontier_candidate_mass_exact` du ledger, construit depuis
`upper-lower`, reste correct ; c’est la télémétrie du probe qui est mal nommée.

### 6.2 La gate `mode_politiques` réutilise le mauvais `terminal_checks`

La porte sauvegarde :

```text
scan_buckets
splits_buckets
```

pour la configuration de référence, mais compare ensuite avec :

```text
g_c.terminal_checks
```

alors que `g_c` contient les compteurs de la **dernière** politique exécutée.

Il faut sauvegarder :

```text
checks_buckets
```

au même moment que les deux autres compteurs.

### 6.3 Le coût de sélection batché est sous-compté

Pour `witness_batch>1`, et surtout pour `witness_batch=0` signifiant « tous », le
sélecteur buckets parcourt plusieurs handles mais ajoute une seule unité à
`selector_scan_items`.

La bucketisation reste bonne, mais la télémétrie ne doit pas prétendre `O(1)`
quand elle énumère un lot de taille `B`.

Compteurs recommandés :

```text
bucket_mask_probes
selected_span_handles
batch_build_items
```

avec coût :

```text
O(number_of_nonempty_bucket_words + B).
```

---

## 7. Gates supplémentaires avant q2 de bout en bout

### G1 — cap ponctuel, pas largeur

La fixture `64 × 256` ci-dessus doit refuser l’exactification sous cap `64`.

### G2 — cap et batch

```text
budget=1, batch=8
```

ne doit jamais produire plus d’une scission dans la vague.

### G3 — continuation corrompue

Tous les tampons invalides doivent être refusés proprement.

### G4 — majorant saturé causal

Fixture `20 + 5` avec élimination du span de cinq.

### G5 — masse candidate contre preuve ALL

Construire une racine qui se sépare en :

```text
ALL de population 100
MIXED de population 3
```

Exiger :

```text
lower = 100
candidate_mass = 3
retained_mass = 103
```

et des compteurs distincts.

### G6 — masque endpoint géométrique

Le probe abstrait ne peut pas la porter. Elle reste bloquante lors du raccord q2
réel : un ID endpoint pour certaines paires doit redevenir témoin dans l’enfant
non incident.

### G7 — cap + reprise par identités q2

Après raccord géométrique :

```text
uncapped q2
== capped + resumes
```

sur :

```text
SupportKey
BallKey
I_B
U_B
niveau exact
```

---

## 8. Ordre immédiat recommandé à Claude

1. **Corriger `cout_tuile`** pour employer la masse candidate exacte et ajouter
   le compteur réel d’évaluations ponctuelles.
2. Initialiser l’exactification par `lower`, sans rescanner les spans `ALL` pour
   chaque paire.
3. Faire transporter au scheduler un `batch_count` borné par le budget restant.
4. Rendre le mutant du majorant causal avec la fixture `20+5`.
5. Corriger les trois télémétries du § 6.
6. Conserver le codec actuel comme round-trip v0, puis ajouter un décodeur
   fail-closed avant de l’utiliser avec la géométrie réelle.
7. Brancher ensuite q2 de bout en bout : partition réelle, masque relationnel,
   boule diamétrale, `BallKey`, census intérieur/shell et sortie par identités.
8. Ne brancher q3/q4 qu’après réception de ce banc d’essai q2.

Le certificat `NONE_W3/NONE_W4` proposé par l’autre audit reste utile pour la
sélectivité, mais il ne faut pas le placer avant cette correction : améliorer un
majorant tout en sous-estimant le coût de l’exactification produirait seulement
une machine plus sophistiquée pour dépasser un cap mal défini.

---

## 9. Statut consolidé

| Élément | Verdict |
|---|---|
| `PairFrame` immuable | reçu comme squelette |
| trois états de lane autonomes | reçus conceptuellement |
| invariant `L<=N<=U` | reçu |
| masse candidate exacte | reçue |
| majorant dérivé | reçu |
| majorant saturé incrémental | correctement interdit |
| bucketisation | reçue comme politique |
| reprise sous autre politique | reçue sur tampons valides |
| batch d’antichaîne | mathématiquement reçu |
| cap `pair_mass*frontier_width` | **refusé** |
| budget booléen avec batch | **refusé** |
| codec fail-closed | non reçu |
| mutant causal du majorant | à renforcer |
| télémétrie de masse candidate | à renommer/corriger |
| géométrie q2 réelle | non branchée |
| générateurs q3/q4 | non branchés |
| qualification GPU / SLO | ouverte |

---

## 10. Message direct à Claude

Le travail depuis `d38cc11` est bon. Tu as extrait le bon objet, reçu la
séparation politique/sémantique, et surtout corrigé une vraie faute de sûreté sur
le majorant avant de la propager sur GPU. C’est exactement le moment où un audit
sert à quelque chose.

Le prochain correctif est plus terre-à-terre mais bloquant : **un span n’est pas
un point**. La largeur de la frontière borne les handles ; elle ne borne pas le
nombre de prédicats exécutés lorsque l’exactification déroule les populations.
Le ledger transporte déjà la bonne quantité,
`frontier_candidate_mass_exact`. Il suffit donc de l’utiliser pour le cap, de
borner le batch par le vrai budget restant, puis de durcir le codec.

Après ces trois réparations, q2 devient un excellent banc d’essai end-to-end. Il
permettra de recevoir le scheduler sur la vraie géométrie avant d’y accrocher les
carrier blocks q3 et la chaîne axiale q4, ces deux endroits où la nature a déjà
montré qu’elle ne respectait pas nos moyennes ni nos noms de variables.
