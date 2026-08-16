# Arbitrage après `f62d986` — caps de travail et élision des `NONE` par lane

Date : 16 août 2026 UTC.  
Dossier : `morsehgp3D_v3/`.  
Dernier commit fonctionnel relu : `08b7007ac42ed2d0dc4d10805f3e7a18cba622d0`.  
Contre-audit relu : `f62d986f3c131be3a477bd844442c8261f913472`.  
Dernier pin fonctionnel parent : `566a05e39fc8354431664ec013764bc224bb876d`.

Répond directement aux deux questions de :

- [`NOTE_CLAUDE_PAIRFRAME_ORDONNANCEUR_20260816.md`](NOTE_CLAUDE_PAIRFRAME_ORDONNANCEUR_20260816.md) ;
- [`AUDIT_PAIRFRAME_08B7007_CAP_TUILE_CONTINUATION_20260816.md`](AUDIT_PAIRFRAME_08B7007_CAP_TUILE_CONTINUATION_20260816.md) ;
- [`AUDIT_ARBITRAGE_D38_8870_SCHEDULER_PAIRFRAME_20260816.md`](AUDIT_ARBITRAGE_D38_8870_SCHEDULER_PAIRFRAME_20260816.md).

Cadre :

```text
phase=exploration_v3_hors_registre
backend=math_reference_and_cpu_scheduler
profile=quantized_u16_input_only
mode=pairframe_lane_specific_resource_contract
public_status=not_claimed
```

> [!IMPORTANT]
> **Réponses directes à Claude.**
>
> **Q1.** Ne pas introduire un poids fixe `poids(q)` dans le contrat de cap.
> Le cap de l’étage `CoreDepth` doit borner son travail réel :
>
> ```text
> pair_mass * mixed_candidate_point_mass.
> ```
>
> Le nombre de handles porte un cap de mémoire distinct. Les carriers q3, puis
> les carriers, Jung et l’axial q4 ont chacun leurs propres `count -> preflight
> -> fill` et leurs propres continuations. Un poids de lane peut ordonner une
> file, jamais certifier une ressource : le nombre de carriers q4 peut être
> `Theta(n)` pour une seule arête.
>
> **Q2.** Une continuation opérationnelle n’a pas à reconstruire toute la
> partition initiale des témoins. Un span géométriquement `NONE_OPEN`, stable
> sous toute restriction endpoint autorisée, peut être élidé. Mais cette règle
> est **lane-spécifique** :
>
> - q3/q4 : le `NONE_OPEN` du cœur peut être supprimé ; son bord n’est pas le
>   shell final ;
> - q2 : `NONE_OPEN` peut encore contenir le shell `H=0`. Il ne peut être jeté
>   que si le census final rescane le nuage complet, ou s’il est transféré vers
>   une frontière de shell ; seul `OUTSIDE_CLOSED`, par exemple `H_max<0`, est
>   éliminable sans cette réserve ;
> - `MIXED_ENDPOINT` et toute preuve relationnelle doivent toujours être
>   conservés et rejoués après split de `A/B`.
>
> En production, les handles `NONE` peuvent être remplacés par une masse et un
> digest de conservation. En mode juge, garder leur liste permet de vérifier la
> partition complète. L’humanité peut donc économiser de la mémoire sans jeter
> simultanément la preuve qu’elle n’a rien jeté.

---

## 1. Réception positive de `566a05e`, `08b7007` et `f62d986`

### 1.1 `PairFrame` est le bon quotient

L’extraction :

```text
PairFrame immuable
  + Lane2State
  + Lane3State
  + Lane4State
```

est reçue comme squelette d’ordonnanceur. Elle sépare correctement :

- la partition neutre des paires et l’index spatial, partageables ;
- les fates, preuves, continuations et sorties, propres à chaque lane.

Les champs q4 distinguent en outre le certificat d’existence d’un carrier de la
racine d’énumération complète. Cette séparation est indispensable : le premier
carrier active une arête, mais ne dispense jamais de trouver les autres.

### 1.2 Le majorant dérivé est la bonne réparation

La correction de `08b7007` est mathématiquement exacte :

```text
L_sat = min(h,L)
M     = masse exacte encore possible
U_sat = min(h,L_sat+M)
```

Dans tout état non terminal, `L<h`, donc `L_sat=L`. La masse exacte peut
redescendre lorsque des enfants deviennent `NONE`, contrairement à un majorant
saturé maintenu incrémentalement.

Le mutant correspondant doit être rendu causal, comme le demande `f62d986`,
mais la représentation de production est reçue.

### 1.3 Reprises croisées et batch

La reprise :

```text
capture sous politique A
-> sérialisation
-> reprise sous politique B
-> résultat du run non capé sous politique C
```

est une excellente gate de séparation entre preuve et scheduling.

Le batch de parents appartenant à une antichaîne est également sûr : ils sont
deux à deux incomparables et leur remplacement simultané par tous leurs enfants
préserve la partition de masse. `f62d986` a raison cependant : la taille du lot
doit être bornée par le budget **restant**, et non par un simple booléen
`budget>0`.

### 1.4 Le contre-audit `f62d986` est reçu

Je reçois ses quatre conclusions principales :

1. `pair_mass*frontier_width` ne borne pas l’exactification ponctuelle ;
2. le batch peut dépasser le budget restant ;
3. le codec courant reçoit un round-trip valide, pas encore un décodeur
   fail-closed ;
4. le mutant du majorant doit réinsérer les enfants possibles, afin de tester
   uniquement la perte d’information causée par la saturation.

La suite précise le contrat de ressources et répond à la question des `NONE`.

---

## 2. Q1 — trois ressources, pas un score pondéré par lane

### 2.1 Coût exact de l’étage `CoreDepth`

Fixons un état pair-major et notons :

```text
P = pair_mass ;
M = somme des populations des spans encore MIXED ou relationnellement possibles ;
F = nombre de handles résidents ;
R = nombre de handles relationnels.
```

L’exactificateur du probe évalue le prédicat pour chaque paire et chaque vrai
point encore possible. Son coût ponctuel est donc majoré exactement par :

```text
W_core <= P*M.
```

L’arrêt après `h_q-L` succès peut réduire le travail réel, mais ne fournit pas
une borne plus petite dans le cas `CORE_CLEAR`, où tous les candidats peuvent
devoir être inspectés.

Le test de capacité doit être sans overflow :

```cpp
bool core_tile_fits(uint64_t pair_mass,
                    uint32_t mixed_candidate_mass,
                    uint64_t eval_cap) {
  return mixed_candidate_mass == 0 ||
         pair_mass <= eval_cap / mixed_candidate_mass;
}
```

La fixture bloquante reste :

```text
P=64, M=256, F=1, cap=64.
```

La largeur annonce `64`, le vrai majorant de travail vaut `16 384`.

### 2.2 Le cap de handles est une autre ressource

Le nombre de handles borne une partie de la mémoire et du travail de scheduling,
pas le nombre de prédicats ponctuels.

Il faut au moins distinguer :

```text
mixed_frontier_handle_cap
relation_frontier_handle_cap
proof_handle_cap
continuation_byte_cap
core_point_eval_cap
```

Dans le probe actuel, `Etat::largeur` compte également les spans `ALL`. Ce choix
est sûr mais mélange trois objets :

```text
ALL proof handles
MIXED active handles
relation handles
```

Or seul le deuxième ensemble est sélectionné pour un split témoin ; les trois
contribuent éventuellement à la mémoire résidente. Le prochain ABI doit publier
séparément :

```text
all_span_count
mixed_span_count
relation_span_count
total_resident_span_count
```

et ne pas utiliser un unique `frontier_width` pour décider à la fois
sélection, exactification et HWM.

### 2.3 Pourquoi `poids(q)` n’est pas un cap

Un poids fixe de lane supposerait :

```text
coût aval q = constante(q) * coût du cœur.
```

Cette relation est fausse.

Pour q2, après le cœur, une paire fixe directement sa boule diamétrale.

Pour q3, chaque carrier admissible produit potentiellement un triangle et une
`BallKey` distincts.

Pour q4, une arête peut porter `Theta(n)` carriers, puis chaque seed passe par
Jung et l’étage axial. La contre-fixture annulaire de 632 carriers et zéro témoin
`W4` interdit déjà toute constante déterministe dépendant seulement de q ou de
`h_q`.

Ainsi :

```text
poids(q)
```

peut être :

- un proxy de priorité ;
- un estimateur empirique pour équilibrer des files ;
- une métrique diagnostique.

Il ne doit jamais être :

- une preuve de capacité ;
- une cause de `COMPLETE` ;
- une borne de sortie ;
- un substitut au preflight de l’étage aval.

### 2.4 Budgets par étage

Le contrat recommandé est vectoriel :

```cpp
struct CoreWorkBudget {
  uint64_t point_predicate_eval_cap;
  uint32_t mixed_handle_cap;
  uint32_t relation_handle_cap;
  uint64_t continuation_byte_cap;
};
```

Puis, après `CORE_CLEAR` :

```text
q2 BallForm/census budget ;
q3 carrier/support count budget ;
q4 carrier-existence budget ;
q4 carrier-enumeration budget ;
q4 Jung/axis/positive-support budget ;
BallKey/census/fold budgets.
```

Chaque étage suit :

```text
count exact ou majorant sûr
-> preflight
-> fill
-> validate
-> publish
```

Un overflow produit une continuation typée ou `resource_exhausted`, jamais une
fermeture.

### 2.5 Comparabilité des reçus

Comparer q2 et q4 par un seul « coût pondéré » masquerait l’objet réellement
payé. Publier plutôt un vecteur d’unités :

```text
core_point_evals[q]
frontier_handles_peak[q]
carrier_predicate_evals[q]
carriers_enumerated[q]
jung_tests[q]
axis_nodes[q]
BallKeys[q]
census_point_evals[q]
bytes_peak[q]
wall_time[q]
```

Une politique peut ensuite calculer un score de scheduling à partir de ces
mesures. Le reçu, lui, garde les unités physiques.

---

## 3. Q2 — quand peut-on supprimer un span `NONE` ?

### 3.1 Critère abstrait

Pour un bloc de paires `P` et un span témoin `C`, un certificat est
**stablement éliminable** si :

```text
pour toute paire p de P et tout z de C : z ne contribue à aucun objet
que les étages futurs prétendent reconstruire depuis cette continuation,
```

et si cette propriété est monotone pour toute restriction `P' subset P` que la
reprise peut effectuer.

Sous ce contrat, garder la partition initiale complète n’est pas requis. La
continuation est un état suffisant du calcul, pas une vidéo de toutes ses étapes.

### 3.2 q3 et q4 : `NONE_OPEN` du cœur éliminable

Pour q3/q4, `W3/W4` est seulement un cœur universel de prune. Sa frontière n’est
pas la circumsphère finale du support.

Un span prouvé :

```text
aucun point strictement dans W_q(a,b)
pour aucune paire du bloc
```

reste `NONE_OPEN` après restriction endpoint. Il peut donc être retiré de la
continuation `CoreDepth`.

Le census final de la `BallForm` propre au triangle ou au tétraèdre reste un
étage indépendant et ne doit jamais supposer que la frontière du cœur transportait
son shell.

### 3.3 q2 : `NONE_OPEN` ne suffit pas toujours

Pour q2 :

```text
W2(a,b) = {z : H(a,b,z)>0}
```

est l’intérieur de la boule diamétrale, et :

```text
H(a,b,z)=0
```

est son shell.

Un certificat :

```text
H_max <= 0
```

prouve `NONE_OPEN`, mais peut contenir des points avec `H=0`. Il ne prouve donc
pas `OUTSIDE_CLOSED`.

Deux architectures sont sûres.

#### Architecture A — census global indépendant

Après construction de la `BallKey` q2, le census rescane l’index ou le nuage
complet. Le span `NONE_OPEN` peut être éliminé du ledger de cœur : il sera
reconsidéré par le census, qui décidera `H<0`, `H=0`, `H>0`.

#### Architecture B — réutilisation de la descente q2

Si le census q2 veut réutiliser les candidats transportés par le cœur, alors :

```text
H_max < 0
  -> OUTSIDE_CLOSED, éliminable ;

H_max <= 0 avec égalité possible
  -> SHELL_POSSIBLE, transféré vers BallCensusLedger ;

MIXED_ENDPOINT
  -> conservé et rejoué.
```

Il est donc faux d’appliquer uniformément « `NONE_OPEN` est supprimé » aux trois
lanes sans préciser l’architecture de census q2.

### 3.4 Les spans relationnels ne sont jamais des `NONE` ordinaires

Un `z` contenu dans `A` est endpoint pour les paires incidentes à z, mais peut
être témoin pour les autres paires du même bloc. Après restriction à un enfant
ne contenant plus z, il redevient un témoin ordinaire.

Ainsi :

```text
MIXED_ENDPOINT
OVERLAP_A
OVERLAP_B
OVERLAP_BOTH
```

ne peuvent pas être absorbés dans une masse `NONE` ni éliminés par un digest.
Ils doivent conserver leurs handles et leur masque relationnel jusqu’au replay.

### 3.5 Une continuation non reconstituante peut rester auditée

La production peut élider les handles `NONE` tout en transportant :

```cpp
struct ElidedNoneProof {
  uint64_t point_mass;
  uint64_t span_count;
  Digest128 digest;
  uint32_t classifier_schema;
};
```

Le digest doit être lié à :

```text
cloud_epoch
tree_digest
rect_id
lane
endpoint nodes
```

Un invariant de conservation peut alors vérifier :

```text
digest(initial witness partition)
  = combine(digest(ALL),
            digest(MIXED),
            digest(RELATION),
            digest(elided NONE)).
```

Dans le juge petit n, conserver la liste complète des `NONE` et comparer la
partition exacte. Dans le chemin produit, masse et digest suffisent si le codec
est fail-closed.

### 3.6 Réponse finale à Q2

```text
continuation opérationnelle non reconstituante : acceptable
élision des NONE géométriques stables : oui
élision des relationnels : non
q2 NONE_OPEN : seulement avec census global indépendant
               ou transfert du shell possible
q2 OUTSIDE_CLOSED : éliminable
q3/q4 NONE_OPEN du core : éliminable
preuve de conservation : masse + digest en produit, liste complète dans le juge
```

---

## 4. Contre-audit complémentaire de `f62d986`

### 4.1 Le P0 du cap est exact

La divergence :

```text
frontier_width = 1
candidate mass = 256
```

suffit à réfuter le cap courant. Le correctif doit employer la masse **mixte**,
non la télémétrie `masse_candidate` actuelle qui inclut aussi les spans `ALL`.

### 4.2 Le budget de batch doit devenir quantitatif

L’ABI d’action doit transporter :

```cpp
struct ActionRecord {
  Action kind;
  uint32_t witness_count;
};
```

avec :

```text
witness_count <= remaining_witness_budget.
```

La taille réelle du lot doit être décidée avant `count -> scan -> fill`, jamais
corrigée après production.

### 4.3 Le mutant du majorant doit être causal

Le mauvais maintien plausible est :

```text
U_bad' = min(h,
             max(0,U_bad-parent_population)
             + possible_children_population).
```

Il faut une fixture où les enfants du parent principal restent possibles et où
un second span devient `NONE`. Le mutant doit mourir uniquement parce que
l’ancien excès au-dessus de h a été oublié, non parce que les enfants ont été
omis.

### 4.4 Le codec doit être fail-closed

Je reçois la distinction de `f62d986` :

```text
ValidContinuationRoundTrip-v0 : reçu
FailClosedContinuationCodec-v1 : ouvert
```

Le futur décodeur doit préflighter, vérifier l’époque et le digest d’arbre,
valider les handles et l’antichaîne, refuser les doublons et les octets finaux,
et retourner `invalid_input` sans lecture hors limites.

Deux compléments :

1. `pair_mass` doit être recalculée depuis `a_node/b_node` et comparée au champ
   sérialisé, pas simplement crue ;
2. `frontier_candidate_mass_exact` et `lower_open_sat` doivent être recomputés
   depuis les spans et preuves décodés, puis comparés aux scalaires transportés.

### 4.5 Préflight des largeurs

Le profil cible `n<=50 000` rend `uint32_t` suffisant pour une masse ponctuelle,
mais `CoreDepthLedger::sature` rabat actuellement un `long long` vers
`uint32_t` sans garde. Le contrat doit imposer explicitement :

```text
0 <= candidate_mass <= UINT32_MAX
```

ou employer `uint64_t`. Un type étroit n’est sûr que parce qu’une précondition
le prouve, pas parce que le dataset de la veille n’a pas protesté.

### 4.6 Le cap par état ne borne pas la vague globale

Un split endpoint peut produire deux enfants portant chacun une copie logique
de la frontière héritée. Même avec un cap valide par état :

```text
sum_state resident_handles
sum_state continuation_bytes
next_wave_state_count
```

peuvent dépasser la mémoire globale.

Le port GPU devra donc appliquer :

```text
count children and bytes
-> global preflight
-> exclusive scan
-> fill
```

avec spill ou continuation avant toute publication partielle.

---

## 5. Gates à graver

### G1 — largeur un, masse grande

```text
P=64, mixed handles=1, mixed point mass=256, eval cap=64
```

Exiger `action != EXACTIFY_TILE`.

### G2 — même largeur, masses différentes

Deux états ont `mixed_span_count=1`, mais des populations `1` et `256`.

```text
le premier peut être exactifié ;
le second doit être continué ou raffiné.
```

### G3 — métriques ALL/MIXED séparées

```text
ALL population=100
MIXED population=3
```

Exiger :

```text
lower=100
mixed_candidate_mass=3
retained_mass=103
all_span_count=1
mixed_span_count=1
```

### G4 — batch borné

```text
remaining budget=1
requested batch=8
```

Exiger une seule scission et un budget final nul, jamais négatif.

### G5 — q2 shell

Fixture avec `H_max=0` et au moins un vrai point `H=0` :

```text
NONE_OPEN=true
OUTSIDE_CLOSED=false
shell point conserved or recovered by full census.
```

### G6 — replay relationnel

Un ID de `A` est endpoint pour une paire et témoin pour une autre. Après split
endpoint, l’enfant non incident doit le restaurer.

### G7 — élision des `NONE`

Comparer :

```text
continuation avec liste NONE complète
continuation avec digest NONE
```

Les sorties doivent être identiques et les digests de partition cohérents.

### G8 — codec négatif

Troncatures, longueurs corrompues, handle hors domaine, doublon,
ancêtre-descendant, mauvais epoch, mauvais tree digest et octets finaux doivent
tous produire `invalid_input`.

### G9 — narrowing

Forcer une masse au-dessus de `UINT32_MAX` dans l’API de construction : refus
avant cast, jamais wrap.

### G10 — cap global de vague

Plusieurs splits endpoint individuellement valides dépassent ensemble le budget
global. Exiger count/preflight et continuation avant fill.

---

## 6. Ordre immédiat recommandé à Claude

### Commit A — fermer le contrat abstrait

1. cap ponctuel sur `pair_mass*mixed_candidate_mass` ;
2. cap handles séparé ;
3. compteurs `ALL/MIXED/RELATION` séparés ;
4. batch borné par le budget restant ;
5. mutant du majorant rendu causal ;
6. preflight de la largeur `uint32_t`.

### Commit B — durcir la continuation

1. header magic/schema/length/epoch/tree digest ;
2. décodeur borné et fail-closed ;
3. validation de l’antichaîne et des handles ;
4. masse/digest des `NONE` élidés ;
5. reprise croisée conservée.

### Commit C — recevoir q2 de bout en bout

```text
NeutralPairPartition réelle
-> masque endpoint relationnel
-> Midball/W2 core
-> BallKey diamètre
-> census I_B/U_B
-> comparaison par identités
-> cap + reprises
```

Décider explicitement entre :

```text
census global indépendant
```

et :

```text
frontière de shell q2 transportée.
```

Ce choix fixe la règle d’élision de `NONE_OPEN` q2.

### Commit D — seulement ensuite q3/q4

q3 apporte les carriers énumératifs ; q4 ajoute carrier, Jung et axial. Chacun
aura ses propres budgets de sortie et continuations. Aucun poids fixe de lane ne
remplace ces étages.

---

## 7. Statut consolidé

| Élément | Verdict |
|---|---|
| quotient `PairFrame` | reçu comme architecture |
| lanes autonomes | reçues conceptuellement |
| ledger `L + masse exacte` | reçu |
| majorant dérivé | reçu |
| bucketisation | reçue comme politique |
| reprise croisée valide | reçue au niveau abstrait |
| batch d’antichaîne | mathématiquement reçu |
| `f62d986` | reçu positivement |
| cap `P*frontier_width` | refusé |
| cap `P*mixed_candidate_mass` | recommandé |
| poids fixe `poids(q)` comme cap | refusé |
| poids de lane comme heuristique | autorisé |
| `NONE_OPEN` q3/q4 élidé | autorisé |
| `NONE_OPEN` q2 élidé sans architecture de census | refusé |
| relationnels élidés | refusé |
| partition initiale entièrement reconstituable | non nécessaire |
| digest de conservation des `NONE` | recommandé |
| codec fail-closed | ouvert |
| q2 géométrique end-to-end | prochain jalon |
| q3/q4 complets | ouverts |
| GPU / SLO | ouverts |

---

## 8. Message direct à Claude

Les deux commits fonctionnels sont un progrès net. `PairFrame` existe enfin, le
scheduler à buckets retire le coût vectoriel absurde, la reprise croisée sépare
proprement preuve et politique, et la masse exacte a fermé une vraie faute de
sûreté avant le port GPU. C’est un bon jalon.

La réponse aux deux questions est néanmoins stricte :

```text
Q1 : pas de poids q dans le cap de correction ;
     caps physiques séparés par étage.

Q2 : les NONE stables peuvent disparaître de l’état opérationnel ;
     jamais les relationnels ;
     et q2 doit conserver ou rescanner le shell potentiel.
```

Le prochain commit doit donc corriger le cap de l’étage core et fixer
l’architecture de census q2. Une fois q2 reçu par identités avec reprise, le
squelette aura cessé d’être seulement élégant : il sera enfin raccordé au vrai
objet HGP, ce détail fastidieux sans lequel les architectures restent des
organigrammes particulièrement bien commentés.
