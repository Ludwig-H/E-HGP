# Contre-audit de `0d7c08b` — les `NONE` du cœur ne sont pas les `NONE` de la lane

Date : 16 août 2026 UTC.  
Dossier : `morsehgp3D_v3/`.  
Dernier commit fonctionnel relu : `08b7007ac42ed2d0dc4d10805f3e7a18cba622d0`.  
Audit relu : `0d7c08befb1bae3232402802e6ae630600cd4669`.  
Contre-audit parent : `f62d986f3c131be3a477bd844442c8261f913472`.

Répond notamment à :

- [`AUDIT_ARBITRAGE_F62D_Q1_Q2_CAPS_NONE_PAR_LANE_20260816.md`](AUDIT_ARBITRAGE_F62D_Q1_Q2_CAPS_NONE_PAR_LANE_20260816.md) ;
- [`AUDIT_PAIRFRAME_08B7007_CAP_TUILE_CONTINUATION_20260816.md`](AUDIT_PAIRFRAME_08B7007_CAP_TUILE_CONTINUATION_20260816.md) ;
- [`NOTE_CLAUDE_PAIRFRAME_ORDONNANCEUR_20260816.md`](NOTE_CLAUDE_PAIRFRAME_ORDONNANCEUR_20260816.md) ;
- [`prototype/pair_frame.hpp`](../prototype/pair_frame.hpp).

Cadre :

```text
phase=exploration_v3_hors_registre
backend=math_reference_and_cpu_scheduler
profile=quantized_u16_input_only
mode=core_elision_vs_downstream_generation_audit
public_status=not_claimed
```

> [!IMPORTANT]
> **Verdict.** L’audit `0d7c08b` est bien orienté et reçoit correctement les
> quatre P0/P1 de `f62d986` : cap ponctuel par masse candidate, budget de batch
> quantitatif, codec fail-closed encore ouvert et mutant de majorant à rendre
> causal.
>
> Sa réponse sur les `NONE` exige toutefois une qualification bloquante :
>
> ```text
> un NONE_OPEN peut être élidé du sous-état CoreDepth ;
> il ne peut pas être élidé du domaine futur de la lane.
> ```
>
> Pour q3/q4, les carriers vivent précisément du côté opposé au cœur universel.
> Une implémentation qui réutiliserait la frontière résiduelle du cœur pour
> générer les carriers perdrait donc non pas quelques cas limites, mais
> potentiellement **tous** les supports.
>
> La solution est déjà esquissée dans `PairFrame` : conserver un
> `carrier_enumeration_root` indépendant et immuable, ou transférer explicitement
> certains `NONE` vers une frontière carrier typée. Un digest d’élision ne peut
> pas remplacer les IDs nécessaires à l’énumération.

---

## 1. Réception positive de `0d7c08b`

Je reçois les points suivants.

### 1.1 Le cap du cœur porte sur les évaluations ponctuelles

Pour un état pair-major :

```text
P = pair_mass
M = masse exacte des vrais PointId encore à classifier par paire
```

l’exactification ponctuelle paie au pire :

```text
P * M
```

appels du prédicat de lane. Le nombre de handles est une ressource distincte.
Le test doit être écrit sans multiplication susceptible d’overflow :

```cpp
fits = (M == 0) || (P <= eval_cap / M);
```

La fixture :

```text
P=64, mixed_handle_count=1, M=256, eval_cap=64
```

reste bloquante : `EXACTIFY_TILE` est interdit.

### 1.2 Les budgets doivent être vectoriels et par étage

Aucun `poids(q)` fixe ne peut certifier le coût aval. Il peut seulement ordonner
les files. Les étages q2, q3 et q4 ont des sorties de nature différente et des
charges non comparables par une constante déterministe de lane.

Le schéma :

```text
count / majorant sûr
-> preflight
-> fill
-> validate
-> publish
```

avec continuations propres à chaque étage est reçu.

### 1.3 Le budget de batch doit être quantitatif

Le simple booléen :

```text
witness_budget_available = budget > 0
```

est insuffisant. Une action doit porter un nombre :

```text
batch_count <= remaining_witness_budget.
```

Le lot est décidé avant le `count -> scan -> fill`, et le budget ne devient
jamais négatif.

### 1.4 Le codec courant reste seulement un round-trip positif

La distinction reste :

```text
ValidContinuationRoundTrip-v0 : reçu
FailClosedContinuationCodec-v1 : non reçu
```

Le futur décodeur doit préflighter toutes les longueurs, refuser les octets
finaux, valider les handles, l’antichaîne, l’époque, le digest d’arbre et les
masses recomputées.

---

## 2. P0 de qualification : `NONE_CORE` n’est pas `NONE_LANE`

### 2.1 Les deux régions sont séparées par le signe de `H`

Pour une arête owner `e=(a,b)` et un site `x`, posons :

```text
H_e(x) = (x-a) dot (b-x).
```

Le cœur diamétral vérifie :

```text
W2(a,b) = {x : H_e(x) > 0}.
```

Les cœurs q3 et q4 sont plus petits :

```text
W3(a,b) subset W2(a,b)
W4(a,b) subset W2(a,b).
```

En revanche, pour que `x` soit un carrier aigu d’une arête maximale `ab`, le
prédicat reçu dans le dossier impose :

```text
H_e(x) < 0
```

avec les deux contraintes de longueur owner.

Ainsi, point par point :

```text
carrier_q3/q4(x)  =>  x notin W2(a,b)
                    => x notin W3(a,b), x notin W4(a,b).
```

Un carrier est donc un `NONE_OPEN` du cœur. La contre-fixture annulaire déjà
gravée rend ce fait spectaculaire :

```text
632 carriers
0 témoin W2
0 témoin W3
0 témoin W4.
```

### 2.2 Conséquence d’architecture

L’énoncé de `0d7c08b` est correct uniquement sous la portée suivante :

```text
NONE_OPEN q3/q4
  -> éliminable du CoreDepthLedger et de sa continuation
  -> jamais éliminable du PointStore, du PairFrame neutre,
     ni du domaine d’énumération carrier.
```

Il faut donc distinguer explicitement :

```text
CoreDepthContinuation
CarrierEnumerationState
BallCensusState
```

La disparition d’un handle du premier objet ne vaut aucune disparition des deux
autres.

### 2.3 Le champ existe déjà, mais sa complétude n’est pas reçue

`Lane3State` et `Lane4State` portent déjà un :

```text
carrier_enumeration_root
```

séparé du ledger de cœur. C’est la bonne ABI conceptuelle.

Il manque encore une gate prouvant que cette racine désigne le domaine neutre
complet, ou un cover carrier complet, indépendamment des élisions réalisées par
`CoreDepth`.

Une simple valeur de handle non nulle ne suffit pas : elle pourrait pointer vers
la frontière résiduelle après suppression des `NONE`, auquel cas elle serait
précisément vide sur la famille annulaire.

### 2.4 Un digest ne peut pas réénumérer

Le :

```text
ElidedNoneProof { mass, span_count, digest }
```

proposé par `0d7c08b` est utile pour l’intégrité et l’audit de conservation. Il
ne contient pas les `PointId` et ne permet pas de reconstruire les carriers.

Il ne peut donc être utilisé que si :

```text
- aucun étage futur de cette continuation n’a besoin des IDs élidés ; ou
- cet étage possède par ailleurs une racine/partition complète indépendante.
```

Le digest ne doit jamais servir de substitut à `carrier_enumeration_root`, ni
faire autorité sur une fate géométrique. Il constate qu’un ensemble a été
omis ; il ne ressuscite pas l’ensemble, fonction que même un hash très motivé ne
sait toujours pas assurer.

---

## 3. Deux implémentations sûres pour q3/q4

### 3.1 Route v0 recommandée : redémarrage depuis la racine carrier

La route la plus simple à recevoir est :

```text
CoreDepth q3/q4
  -> élide ses NONE stables
  -> PRUNED ou CORE_CLEAR

si CORE_CLEAR :
  CarrierEnumerationState
  -> repart de carrier_enumeration_root complet
  -> owner + acuité + exact-once
```

Avantages :

- preuve simple ;
- aucune dépendance à la raison précise du `NONE` ;
- continuation du cœur compacte ;
- indépendance nette des étages.

Coût : certaines AABB sont revisitées. Ce coût est acceptable pour une v0 de
réception ; il sera mesuré avant toute fusion plus ambitieuse.

### 3.2 Route v1 possible : transfert typé du cœur vers les carriers

Pour éviter un redémarrage complet, le classifieur conjoint peut transporter la
raison du verdict :

```text
NONE_CORE_BY_H_NONPOS
NONE_CORE_BY_CONE_WITH_H_POS
NONE_CORE_BY_OWNER
MIXED_CORE
MIXED_ENDPOINT
```

Alors :

```text
NONE_CORE_BY_H_NONPOS
  -> candidat potentiel carrier ; transfert vers CarrierFrontier

NONE_CORE_BY_CONE_WITH_H_POS
  -> ni intérieur du cœur q, ni carrier aigu de l’arête ; éliminable

MIXED
  -> conservé / reclassifié
```

Le transfert reste fail-open : owner, distances et stricte `H<0` sont
recertifiés dans l’étage carrier.

Cette optimisation est prometteuse parce que le même `Phi/H` apparaît dans les
deux classifieurs. Elle ne doit cependant pas retarder la réception de la route
v0.

---

## 4. q2 : le census global est sûr, mais mauvais choix produit par défaut

### 4.1 Réception de la distinction shell

Pour q2 :

```text
H>0 : intérieur
H=0 : shell
H<0 : extérieur fermé
```

Ainsi :

```text
H_max <= 0
```

prouve seulement `NONE_OPEN`, tandis que :

```text
H_max < 0
```

prouve `OUTSIDE_CLOSED`.

La distinction de `0d7c08b` est juste.

### 4.2 Les deux architectures sont sûres, pas également pertinentes

Le census global indépendant est une autorité simple et un bon fallback. Il peut
cependant revisiter une grande partie de l’index pour chaque paire q2 survivante.
Une source q2 peut elle-même avoir une sortie quadratique ; un parcours global
par sortie n’offre alors aucune route crédible vers 50 000 points.

Pour le chemin produit, je recommande donc la route réutilisante :

```text
PairFrame q2
  -> Midball/W2
  -> interior proof spans
  -> SHELL_POSSIBLE frontier
  -> BallForm diamétrale
  -> BallCensusLedger q2
  -> exactification des seuls spans encore possibles
```

avec :

```text
OUTSIDE_CLOSED  -> éliminé
SHELL_POSSIBLE  -> transféré au census
MIXED_ENDPOINT  -> rejoué
```

Le census global reste :

```text
- oracle petit n ;
- fallback borné ;
- autorité indépendante de comparaison ;
- continuation/resource_exhausted si son preflight échoue.
```

### 4.3 Pourquoi q2 est le bon prochain jalon

q2 possède déjà :

- une boule canonique déterminée par la paire ;
- un prédicat entier simple ;
- un shell qui coïncide avec la frontière `H=0` ;
- aucun carrier ;
- une comparaison exhaustive directe.

Il permet donc de recevoir réellement :

```text
PairFrame géométrique
+ masque relationnel
+ core
+ shell
+ BallKey
+ census
+ continuation
```

avant d’ajouter les sorties énumératives de q3 et la chaîne q4.

---

## 5. Caps : préciser l’unité sans réintroduire un poids de lane

Le produit :

```text
pair_mass * mixed_candidate_point_mass
```

borne le nombre d’appels du prédicat ponctuel du **seul étage CoreDepth**. Il ne
borne ni les instructions machine ni l’aval.

Le reçu doit donc nommer l’unité :

```text
core_point_predicate_calls
```

et conserver séparément :

```text
core_primitive_ops
carrier_predicate_calls
census_predicate_calls
axis_node_visits
```

Une lane peut employer un cap numérique différent et un score de priorité
empirique. Aucun de ces coefficients ne devient une preuve de capacité d’un
autre étage.

La masse `M` utilisée dans le cap doit être :

```text
mixed geometric mass
+ relation mass encore admissible pour au moins une paire
```

après exclusion exacte des IDs prouvés endpoints pour toutes les paires
restantes. Un simple nombre de handles, ou une masse incluant les spans `ALL`,
ne porte pas cette unité.

---

## 6. Gates bloquantes supplémentaires

### G11 — élision core, carriers conservés

Sur la contre-fixture annulaire :

```text
CoreDepth q4 : 632 IDs peuvent être classés NONE_OPEN et élidés
CarrierEnumeration q4 : les 632 carriers sont néanmoins retrouvés
missing_carrier=0
duplicate_carrier=0
```

Cette gate tue l’implémentation qui initialise la recherche carrier depuis la
frontière résiduelle du cœur.

### G12 — q3 séparé de q4

Une collection de carriers q3 est placée hors de `W3`. Exiger :

```text
élision CoreDepth q3 ON/OFF
-> mêmes (EdgeKey,PointId) carriers q3
```

### G13 — digest sans racine interdite

Construire une continuation q4 contenant seulement le digest des `NONE`, sans
`carrier_enumeration_root` valide. Exiger :

```text
invalid_input ou incomplete_continuation
```

jamais une sortie vide déclarée complète.

### G14 — q2 shell sans rescan

Avec un vrai point `H=0` :

```text
census_global=OFF
NONE_OPEN core élidé
SHELL_POSSIBLE transféré
U_B contient le PointId shell
```

### G15 — q2 fallback global

Même fixture avec la frontière shell volontairement absente :

```text
fallback global ON  -> même U_B
fallback global OFF -> incomplete_continuation
```

### G16 — raisons d’élision

Un bloc `NONE_W3` par `H<=0` et un bloc `NONE_W3` par échec du cône avec `H>0`
ne doivent pas recevoir le même destin dans la route v1 :

```text
premier -> CarrierFrontier
second  -> éliminable
```

---

## 7. Ordre immédiat recommandé à Claude

1. Appliquer les corrections de ressources reçues par les deux audits : cap
   `P*M`, handles séparés, batch borné, codec fail-closed et mutant causal.
2. Graver que `NONE_OPEN` est une fate de **sous-état**, jamais une suppression
   du domaine neutre de la lane.
3. Conserver et valider `carrier_enumeration_root` pour q3/q4 ; ajouter G11--G13.
4. Brancher q2 de bout en bout avec `SHELL_POSSIBLE` ; garder le census global
   comme oracle/fallback borné.
5. Mesurer ensuite l’intérêt d’un transfert typé CoreDepth -> CarrierFrontier.
6. N’ajouter q3/q4 complets qu’après réception de q2 par identités et reprise.

---

## 8. Statut consolidé

| Élément | Verdict |
|---|---|
| audit `0d7c08b` | reçu positivement |
| cap `P*mixed_candidate_mass` | reçu comme unité CoreDepth |
| caps de handles séparés | requis |
| poids fixe de lane comme preuve | refusé |
| `NONE_OPEN` élidé de CoreDepth q3/q4 | autorisé |
| `NONE_OPEN` élidé du domaine carrier | **refusé** |
| `carrier_enumeration_root` séparé | bonne ABI, complétude à recevoir |
| digest d’élision comme contrôle d’intégrité | autorisé |
| digest comme source d’IDs | refusé |
| census global q2 | autorité/fallback sûr |
| census global q2 comme route produit par défaut | déconseillé |
| frontière `SHELL_POSSIBLE` q2 | recommandée |
| q2 end-to-end | prochain jalon |
| q3/q4 end-to-end | ouverts |

---

## 9. Message direct à Claude

L’autre audit et le mien convergent sur le contrat de ressources. La seule
précision bloquante est une question de portée : supprimer un `NONE` du ledger
de cœur ne doit jamais supprimer ce point de la lane entière.

Pour q3/q4, les carriers sont justement hors du cœur. La famille annulaire le
montre sans aucune subtilité numérique : le cœur en voit zéro, l’étage carrier
doit en retrouver 632. Le champ `carrier_enumeration_root` existe déjà ; il faut
maintenant prouver qu’il reste complet après toutes les élisions.

Pour q2, choisis dès maintenant la route produit : transporte les candidats de
shell et réutilise la descente. Le census global doit rester l’autorité
indépendante et le fallback, pas devenir un second parcours massif pour chaque
paire survivante.

Une fois ces deux raccords gravés, `PairFrame` cessera d’être seulement un bon
ordonnanceur abstrait et commencera enfin à porter les objets géométriques que
la thèse lui demande. Les logiciels, ces créatures délicates, finissent parfois
par devoir traiter les données qu’ils prétendent organiser.
