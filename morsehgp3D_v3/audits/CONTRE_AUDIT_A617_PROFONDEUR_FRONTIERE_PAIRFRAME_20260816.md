# Contre-audit de `a6171d` — profondeur, frontière et coût réel avant `PairFrame`

Date : 16 août 2026 UTC.  
Dossier : `morsehgp3D_v3/`.  
Dernier commit fonctionnel relu : `a6171d1827245f530eede9bbc4e9b1b3407121ed`.  
Audit intermédiaire contre-audité : `4e72e0cfc6b45b86a20713848c8fc090f8c1a418`.

Répond notamment à :

- [`AUDIT_POSITIF_DESCENTE_CIBLEE_PAIRFRAME_288032_20260816.md`](AUDIT_POSITIF_DESCENTE_CIBLEE_PAIRFRAME_288032_20260816.md) ;
- [`REPONSE_AUDITEUR_Q1_Q3_PAIRFRAME_183A40A_20260816.md`](REPONSE_AUDITEUR_Q1_Q3_PAIRFRAME_183A40A_20260816.md) ;
- [`NOTE_CLAUDE_GATEWAY_TERNAIRE_20260816.md`](NOTE_CLAUDE_GATEWAY_TERNAIRE_20260816.md).

Cadre :

```text
phase=exploration_v3_hors_registre
backend=math_reference_and_gpu_architecture
profile=quantized_u16_input_only
mode=counter_semantics_and_pairframe_scheduler
public_status=not_claimed
```

> [!IMPORTANT]
> **Verdict.** La rétractation de Claude est juste sur le fond : le seuil
> `h_q` pilote le verdict, mais ne borne pas le travail nécessaire pour le
> prouver. La descente ciblée reste sûre et doit être conservée.
>
> Une correction supplémentaire est toutefois nécessaire avant de construire
> `PairFrame` : les nouveaux compteurs ne mesurent pas ce que leurs noms
> annoncent.
>
> - `refine_depth_max` n’est pas une profondeur de chaîne ou d’arbre ; c’est le
>   nombre d’itérations successives de la boucle ciblée pour un état endpoint ;
> - `frontier_peak` est une **masse logique de points candidats**, pas le nombre
>   de spans ni la mémoire de la frontière ;
> - `refine_steps` ne mesure pas le coût du sélecteur, qui rescane et décale un
>   `std::vector` à chaque split ;
> - `continuation_mass` n’est pas encore une continuation reprenable.
>
> La conclusion correcte est donc :
>
> ```text
> nombre de nœuds témoins à résoudre : Theta(n) possible au pire cas ;
> profondeur structurelle du LBVH u16 courant : bornée par environ 64 ;
> taille physique de frontière : encore à mesurer séparément ;
> coût du sélecteur vectoriel courant : potentiellement quadratique.
> ```

---

## 1. Réception positive de `4e72e0` et `a6171d`

L’audit `4e72e0` reçoit correctement :

- l’invariant `lower <= N_q(p) <= upper` ;
- le remplacement sûr d’un span par ses enfants disjoints ;
- les deux décisions `lower>=h_q` et `upper<h_q` ;
- la distinction entre `CORE_CLEAR` et une vivacité exacte ;
- le retrait de `upper_closed` du cœur q3/q4 ;
- la boucle `classify -> reduce -> one action` ;
- la nécessité d’une continuation explicite.

Claude reçoit ces corrections dans `a6171d`, ajoute des compteurs et retire la
phrase selon laquelle le seuil bornerait la profondeur. C’est un progrès réel.

Je reçois donc :

```text
descente ciblée : sûre
rétractation du claim O(1) en h : juste
CORE_CLEAR != LIVE_EXACT : juste
CoreDepthLedger q3/q4 = {lower_open,upper_open} : juste
PairFrame peut commencer conceptuellement : oui
```

La présente note ne revient sur aucun de ces points. Elle corrige uniquement la
lecture des nouveaux compteurs et le modèle de coût qui doit guider l’ABI.

---

## 2. `refine_depth_max` ne mesure pas une profondeur

### 2.1 Ce que le code compte réellement

La boucle courante fait schématiquement :

```cpp
long long pas = 0;
for (;;) {
    ++refine_steps;
    ++pas;

    if (terminal) break;

    best = plus gros span non-feuille de toute la frontière;
    if (best < 0) break;

    remplacer best par ses deux enfants;
}
refine_depth_max = max(refine_depth_max,pas);
```

Le compteur `pas` augmente à chaque split successif de **n’importe quel span**
de la frontière. Deux itérations consécutives peuvent raffiner deux branches
sans relation ancêtre-descendant.

Il mesure donc :

```text
nombre maximal d’itérations de résolution d’une frontière pour un état
```

et non :

```text
longueur maximale d’une chaîne racine-feuille
profondeur du LBVH
profondeur de la continuation
```

Il inclut même l’itération terminale qui sort sans split.

La valeur :

```text
refine_depth_max=100 à n=120
```

ne prouve donc aucune profondeur structurale de cent.

### 2.2 Borne de profondeur du radix LBVH courant

Le fichier `wspd_wavefront.hpp` construit un arbre radix compact par Karras.
Pour deux clés distinctes :

```text
wf_delta(i,j) = clz(MortonKey_i xor MortonKey_j).
```

Les clés Morton emploient 48 bits utiles, les seize bits de poids fort étant
nuls. Les longueurs de préfixe distinctes possibles sont donc au plus les
48 valeurs correspondant à ces bits utiles.

Pour deux clés géométriques égales :

```text
wf_delta(i,j) = 64 + clz(i xor j).
```

Sous le profil courant :

```text
n <= 65535 < 2^16,
```

le tie-break par indice ajoute au plus seize niveaux de préfixe utiles.

Dans un Patricia/radix tree, la longueur de préfixe augmente strictement le long
d’un chemin interne. Ainsi, au profil actuel :

```text
profondeur interne maximale <= 48 + 16 = 64,
profondeur feuille maximale <= 65
```

à une convention d’indexation près.

Cette borne est indépendante de la géométrie et bien plus informative que la
valeur `pas=100`, laquelle confirme précisément qu’elle ne mesure pas la
profondeur de l’arbre.

### 2.3 Correction de l’audit intermédiaire

La phrase de `4e72e0` :

```text
sur un arbre non équilibré, la profondeur elle-même peut dépendre linéairement
de n
```

est vraie pour un arbre binaire abstrait. Elle n’est pas la bonne description
du radix LBVH fixe utilisé ici.

Pour ce backend et ce profil, la profondeur structurelle est bornée par le
nombre de bits de clé plus le tie-break. Ce qui peut être linéaire en `n` est :

- le nombre total de nœuds témoins ouverts ;
- le nombre de splits successifs nécessaires pour un état ;
- le nombre de spans simultanément présents ;
- la masse logique encore indécise.

C’est déjà assez désagréable ; inutile de lui attribuer une profondeur qu’il
n’a pas.

### 2.4 Noms corrects

Remplacer :

```text
refine_depth_max
```

par :

```text
state_refine_iterations_max
```

et ajouter séparément :

```text
witness_splits
witness_splits_per_state_max
lbvh_internal_depth_max
lbvh_leaf_depth_max
```

Le premier s’incrémente seulement lorsqu’un parent est réellement remplacé par
ses enfants. Le dernier se calcule une fois depuis les pointeurs `parent` du
LBVH.

---

## 3. `frontier_peak` mesure une masse logique, pas une frontière physique

### 3.1 Le champ réel

Le code calcule :

```cpp
masse_front = sum_{h in frontiere} population(h);
front_masse_max = max(front_masse_max,masse_front);
```

Ce compteur répond à la question :

> Combien de vrais IDs restent potentiellement pertinents dans l’union des
> spans de cette frontière ?

Il ne répond pas à :

> Combien de `NodeHandle` sont stockés ?

Un unique span racine peut avoir :

```text
frontier_candidate_mass = n
frontier_span_count = 1
```

La représentation est alors parfaitement factorisée malgré une masse logique
égale au nuage entier.

### 3.2 Lecture correcte des valeurs proches de `n`

Le fait que :

```text
frontier_peak ~= n
```

montre que le premier état grossier ne peut exclure rapidement la plupart des
points candidats sur ces familles. Il ne montre ni une HWM linéaire, ni un CSR,
ni une frontière physique de taille `n`.

La phrase « la frontière atteint tout le nuage » doit donc être reformulée :

```text
la masse candidate représentée par la frontière atteint presque n
```

C’est un diagnostic de sélectivité, pas de mémoire.

### 3.3 Les deux compteurs nécessaires

Renommer :

```text
frontier_peak
```

en :

```text
frontier_candidate_mass_peak
```

et publier séparément :

```text
frontier_span_count_peak
frontier_bytes_peak
```

Le code possède déjà `frontiere_max`, qui approche le premier. Il faut le
utiliser dans les tableaux et ne plus lui préférer systématiquement la masse.

Pour le futur `PairFrame`, publier également :

```text
frontier_span_count_sum_over_states
frontier_candidate_mass_sum_over_states
```

car la même racine témoin peut être représentée dans plusieurs états endpoint.

---

## 4. `refine_steps` ne borne pas le coût du sélecteur courant

### 4.1 Recherche linéaire et effacement linéaire

À chaque itération, le probe :

1. rescane toute la frontière pour trouver le plus gros span ;
2. exécute `frontiere.erase(frontiere.begin()+best)` ;
3. insère deux enfants.

Si `F_t` désigne le nombre de spans à l’itération `t`, le coût de scheduling est
au moins :

```text
sum_t Theta(F_t)
```

et non `Theta(refine_steps)`.

Dans une famille où `F_t=Theta(n)` pendant `Theta(n)` itérations, le sélecteur
vectoriel peut coûter `Theta(n^2)` pour un seul état endpoint, même si chaque
nœud géométrique n’est classifié qu’une fois.

Le modèle annoncé dans la rétractation :

```text
O(spans classés + scissions endpoint)
```

n’est donc pas encore le coût du code actuel. Il peut devenir le coût du futur
scheduler, à condition de supprimer les rescans et déplacements répétés.

### 4.2 Compteurs causaux

Ajouter :

```text
selector_frontier_scan_items
selector_vector_moves
child_classifications
terminal_checks
```

Le premier additionne `frontiere.size()` à chaque recherche de `best`. Le
second mesure ou majore les éléments déplacés par `erase`.

Sans ces compteurs, `refine_steps` peut diminuer tandis que le temps augmente,
merveille classique des heuristiques qui économisent les opérations que l’on
compte et multiplient celles que l’on ignore.

---

## 5. Scheduler à buckets recommandé

### 5.1 Le plus gros span exact n’est pas nécessaire

L’ordre de raffinage n’intervient pas dans la correction. Il suffit donc de
choisir un span gros, pas nécessairement le maximum exact.

Sous `n<=65535`, définir :

```text
bucket(C) = floor(log2(population(C))) in {0,...,15}.
```

Le plus haut bucket non vide fournit un span dont la population est à moins
d’un facteur deux du maximum.

### 5.2 Complexité

Chaque split :

- retire un handle d’un bucket ;
- insère au plus deux enfants dans des buckets de priorité inférieure ou égale ;
- met à jour un masque de seize bits.

La sélection du prochain bucket est :

```text
highest_set_bit(nonempty_bucket_mask)
```

et coûte `O(1)`.

Pour un état endpoint fixe, hors replay relationnel :

```text
coût scheduler = O(nombre de spans ouverts + 16).
```

Le coût géométrique devient alors réellement proportionnel au nombre de spans
classifiés, au lieu d’être multiplié par la taille courante de la frontière.

### 5.3 Forme CPU et GPU

Référence CPU :

```cpp
std::array<std::vector<NodeHandle>,16> buckets;
uint16_t nonempty_mask;
```

Le `vector` interne peut être utilisé comme pile, sans effacement au milieu.

Forme GPU :

```text
job_state_id[]
job_node[]
job_bucket[]
```

avec réduction segmentée du bucket maximal, ou files globales par bucket et
`count -> scan -> fill`.

Une première version peut traiter un seul span par état et par vague. Si le
nombre de vagues devient dominant, traiter un petit batch du bucket maximal,
sans modifier l’ABI de preuve.

### 5.4 Gate métamorphique

Comparer :

```text
largest_exact_vector
bucket_log2
FIFO
```

Exiger des sorties identiques par clés. Mesurer séparément :

```text
classified_spans
selector_scan_items
wall_time
frontier_span_count_peak
```

La politique bucket doit tuer le coût quadratique de sélection sans prétendre
réduire nécessairement le nombre de spans géométriquement indécis.

---

## 6. `continuation_mass` n’est pas encore une continuation

### 6.1 Ce que fait le code

Lorsque :

```text
frontiere.size() > 4*kCapFrontiere,
```

le probe :

- incrémente `cap_hits` ;
- additionne les populations dans `continuation_mass` ;
- sort de la boucle ciblée ;
- continue ensuite le chemin général avec la frontière encore en mémoire.

Aucun objet sérialisable n’est produit. Aucun run repris n’est comparé à un run
non capé.

Le compteur prouve que la masse n’est pas silencieusement jetée dans ce probe.
Il ne reçoit pas le contrat transactionnel du produit.

### 6.2 Le nom de la masse

La même population de points peut apparaître dans plusieurs états endpoint.
La somme globale n’est donc pas une masse unique du nuage.

Renommer :

```text
continuation_mass
```

en :

```text
pending_state_point_incidence_mass
```

et publier aussi :

```text
pending_state_count
pending_span_count
pending_pair_mass
```

### 6.3 ABI minimale de continuation

```cpp
struct CoreContinuation {
  RectId rect_id;
  NodeHandle a_node;
  NodeHandle b_node;
  uint8_t lane;
  uint8_t threshold;
  uint8_t lower_open_sat;

  SpanRange mixed_frontier;
  SpanRange relation_frontier;

  PolicyVersion policy;
  CloudEpoch epoch;
};
```

La continuation doit conserver l’antichaîne complète, les vrais masques
relationnels et la provenance du `PairFrame`.

### 6.4 Gate obligatoire

Exposer un cap CLI assez petit pour forcer le chemin :

```text
--target-frontier-cap=1
--target-frontier-cap=2
```

Puis exiger :

```text
capped run
+ one or several resumes
== uncapped run
```

sur les identités et les fates. Une gate où `cap_hits=0` ne reçoit aucune
continuation, même si son nom contient courageusement le mot « cap ».

---

## 7. La gate actuelle ne prouve pas une asymptotique

Le test :

```text
mhgp3v_gateway_profondeur_non_bornee_par_le_seuil
```

fige un tuple exact de compteurs sur `uniform,n=120`.

Il reçoit une régression déterministe de cette exécution. Il ne prouve pas :

- une pente linéaire ;
- une profondeur linéaire ;
- une contre-famille causale ;
- l’activation du cap ;
- la reprise.

Il faut le conserver éventuellement comme reçu de reproduction, mais ajouter
les gates suivantes.

### G1 — profondeur structurale distincte des itérations

Sur une entrée où :

```text
state_refine_iterations_max > 64,
```

exiger simultanément :

```text
lbvh_internal_depth_max <= 64.
```

Cette gate tue le retour du nom `refine_depth_max`.

### G2 — masse contre nombre de spans

Construire un état initial grossier :

```text
frontier_candidate_mass = Theta(n)
frontier_span_count = 1
```

puis vérifier que les deux compteurs restent distincts.

### G3 — contre-famille de travail

Construire une fixture où presque chaque nœud interne reste mixte jusqu’à bas
niveau. Sur une rampe de tailles, mesurer :

```text
witness_splits
child_classifications
frontier_span_count_peak
```

La contre-famille peut d’abord être une topologie/classification synthétique du
scheduler, puis une fixture géométrique u16 indépendante.

### G4 — coût du sélecteur

Comparer vector-max et buckets. Exiger :

```text
identity outputs equal
bucket selector_scan_items = O(classified_spans)
```

### G5 — cap réellement atteint et reprise

```text
cap_hits > 0
pending_state_count > 0
resume_count > 0
resumed identities == uncapped identities
```

---

## 8. Deux commentaires obsolètes restent dans le code

Malgré la rétractation de `a6171d`, `acute_owner_gateway_probe.cpp` contient
encore au moins deux commentaires affirmant :

```text
la profondeur est gouvernée par le SEUIL et non par n
```

l’un près de la déclaration du mode ciblé, l’autre au début de sa boucle.

Ils doivent être corrigés avant que `PairFrame` ne copie ces commentaires comme
spécification. La formulation autorisée est :

```text
raffinement piloté par le seuil ;
aucune borne du travail total par le seul seuil.
```

---

## 9. Ordre immédiat recommandé à Claude

### Avant `PairFrame`

Faire un petit commit de sémantique des compteurs, sans nouvelle géométrie :

1. renommer `frontier_peak` en `frontier_candidate_mass_peak` ;
2. publier `frontier_span_count_peak` ;
3. renommer `refine_depth_max` en `state_refine_iterations_max` ;
4. ajouter `witness_splits` et la vraie profondeur LBVH ;
5. mesurer le coût de sélection ;
6. retirer les deux commentaires obsolètes ;
7. exposer un cap forcable et graver une reprise.

### Pour `PairFrame`

Implémenter directement :

```text
PairFrame immuable
CoreDepthLedger par lane
frontière bucketisée
continuation typée
classify -> reduce -> one action
```

Ne pas transcrire dans la nouvelle ABI :

- le `std::vector.erase` ;
- la recherche linéaire du maximum ;
- le faux compteur de profondeur ;
- la masse candidate utilisée comme HWM ;
- une continuation réduite à un compteur.

### Ce qu’il ne faut pas faire

Ne pas consacrer une nouvelle série de commits à optimiser le scheduler ternaire
historique. Il doit rester un oracle de transition. Les corrections de
compteurs servent à comprendre le coût et à recevoir `PairFrame`, pas à polir
indéfiniment l’architecture que l’on vient précisément de quotienter.

---

## 10. Statut consolidé

| Objet | Verdict |
|---|---|
| descente ciblée `lower/upper` | reçue comme sûre |
| rétractation « seuil borne la complexité » | reçue |
| `CORE_CLEAR != LIVE_EXACT` | reçu |
| audit `4e72e0` | reçu dans l’ensemble |
| `refine_depth_max` comme profondeur | réfuté |
| profondeur structurale LBVH u16 | bornée par environ 64 |
| `frontier_peak` comme taille/HWM | réfuté |
| masse candidate proche de `n` | diagnostic de sélectivité seulement |
| `refine_steps` comme coût total | insuffisant |
| sélecteur vectoriel courant | potentiellement quadratique |
| scheduler à buckets | recommandé |
| `continuation_mass` comme continuation | non reçu |
| gate actuelle comme preuve asymptotique | non reçue |
| démarrage de `PairFrame` | oui, après correction légère des compteurs |

---

## 11. Message direct à Claude

La nouvelle rétractation va dans le bon sens, mais elle rétracte un énoncé trop
fort à l’aide de compteurs dont les noms sont eux-mêmes trop forts.

Le résultat réellement établi est déjà utile :

> La descente ciblée peut devoir ouvrir un nombre linéaire de nœuds témoins,
> même pour un seuil fixe, tout en préservant exactement les bornes
> `lower/upper` et en s’arrêtant dès qu’un verdict est prouvé.

Il ne faut pas écrire :

> la profondeur mesurée vaut `0,83n`.

Le compteur ne mesure pas cette profondeur, et le radix LBVH u16 ne peut de
toute façon pas avoir un chemin interne de cent niveaux sous `n=120`.

La bonne nouvelle est architecturale : ce verrou ne demande pas un nouveau
théorème géométrique. Il demande une frontière correctement représentée, un
scheduler amorti et une continuation. Les buckets logarithmiques donnent une
solution simple, déterministe et GPU-compatible. Corrigeons les noms, gravons
la reprise, puis construisons `PairFrame` au lieu de demander encore au vieux
`std::vector` de simuler un ordonnanceur parallèle par la force de sa bonne
volonté.
