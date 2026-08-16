# Consensus d’audit après `f62d986` et `0d7c08b` — ressources, `NONE` et route q2

Date : 16 août 2026 UTC.  
Dossier : `morsehgp3D_v3/`.  
Dernier commit fonctionnel relu : `08b7007ac42ed2d0dc4d10805f3e7a18cba622d0`.  
Audits arbitrés :

- [`AUDIT_PAIRFRAME_08B7007_CAP_TUILE_CONTINUATION_20260816.md`](AUDIT_PAIRFRAME_08B7007_CAP_TUILE_CONTINUATION_20260816.md), commit `f62d986f3c131be3a477bd844442c8261f913472` ;
- [`AUDIT_ARBITRAGE_F62D_Q1_Q2_CAPS_NONE_PAR_LANE_20260816.md`](AUDIT_ARBITRAGE_F62D_Q1_Q2_CAPS_NONE_PAR_LANE_20260816.md), commit `0d7c08befb1bae3232402802e6ae630600cd4669`.

Cadre :

```text
phase=exploration_v3_hors_registre
backend=cpu_reference
profile=abi_modele_abstrait_then_q2_geometry
mode=pairframe_consensus_and_next_commits
public_status=not_claimed
```

> [!IMPORTANT]
> **Verdict.** Les deux audits convergent. L’audit `f62d986` trouve correctement
> le P0 de ressources : `pair_mass * frontier_width` borne des handles, pas le
> travail ponctuel d’une exactification. L’audit `0d7c08b` répond correctement
> aux deux questions de Claude : aucun poids fixe de lane ne doit entrer dans la
> correction, et les spans `NONE` ne sont éliminables qu’en tenant compte du
> contrat aval, notamment du shell q2.
>
> Il n’existe donc pas de désaccord mathématique à arbitrer. La route commune est :
>
> ```text
> cap handles séparé du cap évaluations ponctuelles
> + batch borné par le budget restant
> + continuation fail-closed
> + raccord q2 réel avec masque relationnel et shell
> ```
>
> `PairFrame + CoreDepthLedger` reste la bonne extraction et doit être conservé.

---

## 1. Réception positive de l’autre audit

Je reçois sans réserve les quatre conclusions principales de `f62d986`.

### 1.1 Le cap courant mesure la mauvaise ressource

Le code courant emploie :

```text
pair_mass * frontier_width.
```

Cette quantité borne au mieux une partie du nombre de handles résidents. Elle ne
borne pas le nombre de prédicats ponctuels lorsque l’exactificateur déroule la
population des spans mixtes.

Si :

```text
p = pair_mass,
M = mixed_candidate_point_mass,
```

alors l’exactification abstraite courante exécute jusqu’à :

```text
p * M
```

lectures de statut, indépendamment du nombre de spans qui factorisent ces `M`
points.

Le contre-exemple :

```text
p = 64
un seul span mixte de population 256
frontier_width = 1
cap = 64
```

est décisif : le cap courant accepte une tuile annoncée à `64`, tandis que son
travail ponctuel vaut `16 384`.

### 1.2 Le batch doit respecter un budget numérique

Le booléen :

```text
witness_budget_available
```

ne suffit plus dès que l’action peut scinder plusieurs spans. Une vague avec un
budget restant de un ne doit pas exécuter un batch de huit puis constater avec
mélancolie que le compteur est devenu négatif.

Le scheduler doit produire :

```cpp
struct ActionRecord {
  Action kind;
  uint32_t witness_count;
};
```

avec :

```text
witness_count
  <= requested_batch
  <= splittable_spans
  <= remaining_state_budget
  <= remaining_wave_budget.
```

### 1.3 Le codec actuel est un round-trip valide, pas encore un codec produit

Le test :

```text
serialize valid state
-> destroy in-memory state
-> deserialize
-> resume under another policy
-> same result
```

est une vraie réception de `ValidContinuationRoundTrip-v0`.

Il ne reçoit pas encore :

```text
FailClosedContinuationCodec-v1.
```

Le décodeur doit préflighter tailles et cardinalités, vérifier magic, schéma,
époque, digest d’arbre, handles, antichaîne, doublons, masse recomputée et fin
exacte du tampon, puis retourner un statut typé sans publication partielle.

### 1.4 Le mutant du majorant doit isoler la saturation

Le mutant courant oublie aussi de réinjecter les enfants possibles. Il peut donc
mourir pour une faute grossière de type `drop children`, sans exercer le vrai
piège : l’information perdue au-dessus du seuil.

Le mauvais algorithme causal à tuer est :

```text
U_bad <- min(h,
             U_bad
             - parent_population
             + possible_children_population).
```

La fixture `20 + 5`, où le span de cinq devient `NONE` tandis que celui de vingt
reste possible, isole exactement la perte d’information due à la saturation.

---

## 2. Contrat de ressources consolidé

### 2.1 Trois masses différentes

Pour un état endpoint donné, séparer :

```text
L
  masse des preuves ALL_OPEN ;

M_mix
  masse exacte des spans MIXED ordinaires ;

M_rel
  masse conservatrice des spans relationnels MIXED_ENDPOINT / OVERLAP_A / OVERLAP_B.
```

Le majorant mathématique est :

```text
U = L + M_mix + M_rel.
```

`M_rel` peut être une surborne, car certains IDs deviennent endpoints pour une
partie seulement des paires. C’est acceptable : elle doit être sûre, et pourra
être resserrée après restriction des endpoints.

### 2.2 Cap ponctuel du cœur

Une borne sûre du nombre d’évaluations ponctuelles est :

```text
core_point_eval_upper
  = pair_mass * (M_mix + M_rel).
```

Dans le probe abstrait, `M_rel=0`, d’où le produit de `f62d986`. Lors du raccord
q2 réel, omettre `M_rel` réintroduirait le même sous-budget sous un autre nom.

Le test de cap évite le produit :

```cpp
bool product_fits(uint64_t a, uint64_t b, uint64_t cap) {
  return b == 0 || a <= cap / b;
}
```

### 2.3 Caps de mémoire distincts

Les handles et les évaluations sont deux ressources différentes :

```text
mixed_frontier_handle_cap
relation_frontier_handle_cap
continuation_byte_cap
core_point_eval_cap.
```

Pour une vague GPU, un cap par état ne suffit pas. Un split endpoint peut
dupliquer une frontière héritée vers plusieurs enfants. Il faut donc :

```text
count children and bytes
-> global preflight
-> exclusive scan
-> fill
-> validate
-> publish.
```

### 2.4 L’exactification ne rescane pas les preuves ALL

Pour chaque paire ponctuelle :

```text
count <- L
scan only MIXED and relation candidates
```

Sommer les populations des spans `ALL` à nouveau pour chaque paire est correct,
mais inutile. `L` est précisément le résumé de preuve destiné à éviter ce
travail.

---

## 3. Réponse consolidée à la question Q1 de Claude

> Faut-il multiplier le coût de tuile par un poids de lane `poids(q)` ?

**Non dans le contrat de correction.**

Le coût `CoreDepth` dépend du nombre de couples `(paire, candidat témoin)`
classifiés. Il doit être budgété par une unité réelle de travail, pas par une
constante décorative attachée à `q`.

Les étages aval possèdent leurs propres unités et caps :

```text
q2 BallForm + census
q3 carrier enumeration + support forms
q4 carrier existence
q4 carrier enumeration
q4 Jung permanent-interior tests
q4 axial roots / positivity / owner
BallKey RLE + census + fold.
```

Un poids empirique par lane peut servir à ordonner les files ou prédire du temps
mur. Il doit alors être :

```text
versionné
mesuré
non autoritaire
absent des clés et des preuves de complétude.
```

Il ne remplace jamais les compteurs propres à chaque étage.

---

## 4. Réponse consolidée à la question Q2 de Claude

> Peut-on supprimer les spans `NONE` de la continuation ?

### 4.1 q3/q4, cœur ouvert uniquement

Oui, pour un vrai certificat stable :

```text
NONE_OPEN sous P
  -> NONE_OPEN sous toute restriction P' subset P.
```

Le bord de `W3/W4` n’est pas le shell de la circumboule finale. Un span prouvé
sans intérieur du cœur et inutile à tout autre compteur de la lane peut donc être
éliminé de la continuation opérationnelle.

### 4.2 q2, shell canonique

Pour q2 :

```text
H < 0  extérieur de la boule fermée
H = 0  shell
H > 0  intérieur strict.
```

Il faut donc distinguer :

```text
OUTSIDE_CLOSED
  éliminable immédiatement ;

NONE_OPEN_BUT_SHELL_POSSIBLE
  inutile au compte intérieur,
  mais encore pertinent pour U_B.
```

Ce second état peut être éliminé seulement si le census q2 final rescane une
source complète indépendante. Sinon, il doit être transféré vers une frontière
shell ou conservé dans la continuation q2.

### 4.3 Les spans relationnels ne sont jamais des `NONE`

Un span qui recouvre `A` ou `B` est relationnel : un ID peut être endpoint pour
certaines paires et témoin pour d’autres. Après restriction d’un endpoint, il
peut redevenir un témoin ordinaire.

Ainsi :

```text
MIXED_ENDPOINT / OVERLAP_A / OVERLAP_B
  != NONE
```

et ces spans doivent être sérialisés et rejoués.

### 4.4 Provenance compacte des `NONE` éliminés

La continuation produit n’a pas à reconstruire chaque étape historique. Elle
doit être suffisante pour reproduire exactement le payload.

Pour auditer l’élision sans transporter tous les handles, conserver :

```text
elided_none_mass
elided_none_span_count
elided_none_digest
classifier_schema
cloud_epoch
tree_digest.
```

Le juge petit `n` garde la liste complète et confronte le digest.

---

## 5. Corrections d’implémentation complémentaires

### 5.1 Largeur du champ de masse

`frontier_candidate_mass_exact` est actuellement converti en `uint32_t`.
Sous la cible `n<=50 000`, cela tient largement. Le codec doit néanmoins
préflighter la conversion et lier le profil au schéma ; aucune conversion
rétrécissante implicite n’entre dans un record persistant.

### 5.2 Télémétrie

Le probe incrémente actuellement sa masse dite candidate avant de distinguer
`ALL` et `MIXED`. Séparer :

```text
all_proof_population
mixed_candidate_population
relation_candidate_population
retained_population.
```

Le champ mathématique utilisé pour `U-L` reste la seule masse autorisée dans le
cap ponctuel.

### 5.3 Coût de sélection batché

La sélection d’un lot de taille `B` n’a pas un coût unitaire. Publier :

```text
bucket_mask_probes
selected_span_handles
batch_build_items.
```

Le scheduler bucketisé coûte :

```text
O(number_of_bucket_words + B),
```

pas `O(1)` lorsque `B` croît.

### 5.4 Action batchée et budget global

L’action choisie doit être un record concret :

```cpp
struct ActionRecord {
  Action kind;
  uint32_t admitted_witness_splits;
  uint64_t planned_child_states;
  uint64_t planned_frontier_handles;
  uint64_t planned_bytes;
};
```

Le `count` et le preflight précèdent toute mutation de l’état courant.

---

## 6. Gates consensuelles avant q2 end-to-end

### G1 — handles contre points

```text
pair_mass=64
mixed_handle_count=1
mixed_candidate_mass=256
point_eval_cap=64
```

Exiger `action != EXACTIFY_TILE`.

### G2 — batch borné

```text
remaining_budget=1
requested_batch=8
```

Exiger `admitted_batch=1`, budget final non négatif et même résultat après
reprise.

### G3 — mutant causal de saturation

Fixture `20 + 5`, le span de cinq devient `NONE`, celui de vingt reste possible.
Le mauvais majorant saturé doit déclarer faussement `CORE_CLEAR` et mourir.

### G4 — codec fail-closed

Troncatures, tailles falsifiées, handles hors domaine, doublons, parent avec son
descendant, mauvais epoch/digest/schema et octets finaux doivent tous rendre
`invalid_input`, sans publication.

### G5 — frontière relationnelle réelle

Un ID appartenant à `A` est endpoint pour les paires incidentes, mais redevient
témoin dans l’enfant endpoint qui ne le contient plus. Comparer par identités à
l’exhaustif.

### G6 — shell q2

Exercer séparément :

```text
H<0
H=0
H>0
```

et prouver que l’élision `NONE_OPEN` ne perd aucun `U_B`.

### G7 — reprise q2 par identités

```text
uncapped
== capped + one or several resumes
```

sur :

```text
SupportKey
BallKey
I_B
U_B
niveau exact.
```

---

## 7. Ordre de commits recommandé à Claude

### Commit A — fermer le contrat de ressources abstrait

1. remplacer le coût de tuile par `pair_mass * mixed_candidate_mass` ;
2. ajouter la masse relationnelle dans l’ABI future ;
3. séparer caps handles / évaluations / octets ;
4. borner le batch par le budget restant ;
5. corriger le mutant causal et la télémétrie.

### Commit B — codec fail-closed

Ajouter header, taille totale, schéma, époque, digest de l’arbre, preflight,
validation de l’antichaîne et gates négatives.

### Commit C — q2 end-to-end

Raccorder :

```text
NeutralPairPartition réelle
-> CoreDepth q2 avec masque relationnel
-> boule diamétrale
-> BallKey
-> census intérieur/shell
-> sortie par identités
-> reprise capée.
```

### Commit D — q3 puis q4

Seulement après réception de q2 :

```text
q3 carrier enumeration + BallForm/census
q4 W4 + carrier + Jung + axial + owner/positivity + BallKey/census.
```

Le certificat `NONE_W3/NONE_W4` reste pertinent, mais vient après la fermeture du
contrat de ressources. Accélérer un exactificateur dont le cap mesure la mauvaise
unité serait un hommage excessivement fidèle à l’histoire du dossier.

---

## 8. Statut consolidé

| Objet | Verdict |
|---|---|
| `PairFrame` immuable | reçu comme squelette |
| lanes autonomes | reçues conceptuellement |
| invariant `L<=N<=U` | reçu |
| masse candidate exacte | reçue |
| majorant dérivé | reçu |
| scheduler buckets | reçu comme politique |
| batch d’antichaîne | reçu mathématiquement |
| reprise sous autre politique | reçue sur records valides |
| cap `pair_mass*frontier_width` | refusé |
| cap `pair_mass*M_mix` | route correcte pour le probe abstrait |
| masse relationnelle dans le cap réel | obligatoire |
| budget batch booléen | refusé |
| codec fail-closed | non reçu |
| suppression `NONE` q3/q4 | reçue sous certificat stable |
| suppression `NONE` q2 | conditionnelle au contrat shell |
| q2 géométrique end-to-end | prochain jalon |
| q3/q4 end-to-end | non branchés |
| GPU / SLO | ouvert |

---

## 9. Message direct à Claude et à l’autre auditeur

L’audit `f62d986` a trouvé le bon P0, et ses P1 sont réels. Mon audit `0d7c08b`
ne les contredit pas ; il complète le contrat par lane et par étage.

La synthèse à implémenter est simple :

```text
un handle n’est pas un point ;
un poids de lane n’est pas un budget ;
un NONE_OPEN q2 n’est pas nécessairement sans shell ;
un booléen de budget n’est pas un batch count ;
un round-trip valide n’est pas un décodeur fail-closed.
```

Une fois ces cinq banalités péniblement matérialisées en types et en gates,
`PairFrame` sera suffisamment propre pour recevoir q2 sur la vraie géométrie.
C’est désormais le jalon utile ; il n’y a plus besoin d’un nouveau détour
théorique avant ce raccord.
