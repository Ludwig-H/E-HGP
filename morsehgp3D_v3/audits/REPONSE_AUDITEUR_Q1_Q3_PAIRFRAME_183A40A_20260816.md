# Réponse de l’auditeur aux trois questions `PairFrame`

Date : 16 août 2026 UTC.  
Pin fonctionnel relu : `183a40a4d839f2a867e8f303298bd2e2972cfa17`.  
Dossier : `morsehgp3D_v3/`.

Répond à la section 12 de :

- [`NOTE_CLAUDE_GATEWAY_TERNAIRE_20260816.md`](NOTE_CLAUDE_GATEWAY_TERNAIRE_20260816.md) ;
- [`AUDIT_CONSOLIDE_Q2_Q3_Q4_PAIR_MAJOR_APRES_79E73B6_20260816.md`](AUDIT_CONSOLIDE_Q2_Q3_Q4_PAIR_MAJOR_APRES_79E73B6_20260816.md) ;
- [`AUDIT_REPONSE_5A225_PAIR_MAJOR_FRONTIER_20260816.md`](AUDIT_REPONSE_5A225_PAIR_MAJOR_FRONTIER_20260816.md).

Cadre :

```text
phase=exploration_v3_hors_registre
backend=math_reference_and_gpu_architecture
profile=quantized_u16_input_only
mode=pairframe_abi_answers
public_status=not_claimed
```

> [!IMPORTANT]
> **Réponses directes.**
>
> - **Q1 : choisir (a), mais adaptativement.** Le majorant grossier saturé est
>   déjà une borne correcte. Ne jamais imposer `upper-lower<h_q` comme condition
>   globale : cette inégalité ne décide pas le problème. On raffine uniquement
>   les états vérifiant `lower<h_q<=upper`.
> - **Q2 : Claude a raison de douter.** Le shell du fuseau `W_q`, `q>=3`, n’est
>   pas le shell de la miniboule finale et ne fait pas partie du contrat HGP.
>   `upper_closed` doit sortir du ledger de cœur q3/q4. Le shell final est décidé
>   plus tard par la `BallForm` propre au support. q2 est le seul cas où le cœur
>   `W_2` coïncide avec la boule canonique du support.
> - **Q3 : ni (a), ni le (b) spéculatif.** Classifier une fois la frontière
>   courante, opération de toute façon obligatoire, réduire par état, puis
>   choisir une seule action à partir de ce résumé et de proxys géométriques
>   bon marché. Ne jamais classifier les enfants des trois splits candidats
>   pour en jeter ensuite deux.

---

## 1. Réception positive du « Commit 1 »

Les commits `d5b39ca` et `183a40a` ferment correctement le premier jalon de
l’audit :

1. les spans recouvrant les endpoints sont conservés dans le majorant et rejoués
   après restriction ;
2. le juge compare les identités `(EdgeKey,PointId)` ;
3. la surcouverture d’ancres mortes est séparée des fautes ;
4. le mutant `juge-compense` prouve que le nouveau juge est strictement plus
   fort que le cardinal ;
5. la partition des paires est jugée indépendamment du gateway ;
6. les positions dupliquées sont effectivement exercées avec des `PointId`
   distincts.

Le tableau `occ` quadratique du juge de partition est acceptable parce qu’il
reste explicitement un oracle à petit `n`. Il ne doit naturellement jamais
migrer dans le chemin produit, proposition dont personne ne semblait rêver,
mais il vaut mieux l’écrire avant qu’un tableau `n*n` ne soit baptisé
« optimisation GPU ».

Le Commit 1 est donc **reçu**.

Une télémétrie facultative peut compléter `doubles` par :

```text
duplicate_excess = sum_key max(0, occurrences(key)-1)
```

mais ce n’est pas bloquant : `absentes=0`, `doubles=0` et
`masse=C(n,2)` suffisent déjà à l’exact-once.

---

## 2. Q1 — quelle borne `upper_open` sur une antichaîne grossière ?

### 2.1 Définition correcte

Fixons une lane `q`, un état endpoint `A×B` et une antichaîne de spans témoins
`F`. Pour une paire ponctuelle `p=(a,b)`, noter :

```text
N_q(p) = nombre de vrais PointId, distincts des endpoints,
         strictement dans W_q(a,b).
```

Pour chaque span `C` de l’antichaîne, le classifieur rend au moins :

```text
ALL_OPEN
  tout ID relationnellement admissible de C est dans W_q
  pour toute paire de A×B ;

NONE_OPEN
  aucun ID admissible de C n’est dans W_q
  pour aucune paire de A×B ;

MIXED
  aucune conclusion uniforme ;

MIXED_ENDPOINT
  la géométrie et le rôle d’endpoint varient avec la paire.
```

Définir :

```text
lower = somme des populations admissibles des spans ALL_OPEN

upper = lower
        + somme des populations admissibles des spans MIXED
        + somme des populations possibles des spans MIXED_ENDPOINT
```

Les spans `NONE_OPEN` contribuent zéro. Les populations sont celles des vrais
`PointId`, avec masque relationnel, et les sommes sont saturées à `h_q`.

Alors, pour toute paire `p in A×B` :

```text
lower <= N_q(p) <= upper.
```

C’est la seule propriété requise.

### 2.2 Les deux seules décisions de seuil

```text
lower >= h_q
  -> PRUNED_BY_UNIVERSAL_DEPTH
     toutes les paires ont au moins h_q témoins universels ;

upper < h_q
  -> CORE_CLEAR
     aucune paire ne peut avoir h_q témoins dans le cœur universel ;

lower < h_q <= upper
  -> MIXED_CORE.
```

Le majorant peut être extrêmement lâche et rester parfaitement correct. S’il
sature à `h_q`, il dit seulement « je ne sais pas ».

### 2.3 Pourquoi `upper-lower<h_q` est la mauvaise condition

Cette condition n’est pas suffisante. Avec :

```text
h_q=8, lower=7, upper=8,
```

on a :

```text
upper-lower=1<8,
```

mais le bloc reste indécis : certaines paires peuvent atteindre huit et
d’autres non.

Elle est par ailleurs redondante lorsque le vrai certificat tient :

```text
upper<h_q  =>  upper-lower<h_q
```

puisque `lower>=0`.

La condition utile est donc directement `upper<h_q`, jamais la largeur de
l’intervalle prise isolément.

### 2.4 Politique de raffinement

Il faut accepter le majorant grossier, puis raffiner **uniquement** lorsque :

```text
lower < h_q <= upper
```

et qu’aucune autre fate ne termine l’état.

Deux potentiels permettent d’ordonner le travail :

```text
need_to_kill  = h_q-lower
need_to_clear = upper-(h_q-1)
```

- `need_to_kill` petit : traiter d’abord les spans susceptibles d’être `ALL` ;
- `need_to_clear` petit : traiter d’abord les spans susceptibles d’être `NONE`.

Ce sont des heuristiques d’ordonnancement, pas des invariants.

### 2.5 Pas de fausse borne de profondeur

Il n’existe pas de borne déterministe sublinéaire générale : une configuration
adversariale peut placer beaucoup de points arbitrairement près de la frontière
exacte du prédicat. Sous le profil u16, la descente termine parce que l’index est
fini ; elle peut néanmoins atteindre les feuilles.

Le contrat correct est donc :

```text
branch-and-bound exact
+ cap explicite
+ continuation sérialisable
+ exactification sous une microtuile
```

et non une promesse de profondeur uniforme.

### 2.6 ABI recommandée

Pour le cœur universel q3/q4 :

```cpp
struct CoreDepthLedger {
  uint8_t reject_threshold;
  uint8_t lower_open_sat;
  uint8_t upper_open_sat;

  SpanRange all_spans;
  SpanRange mixed_spans;
  SpanRange relation_spans;
  ContinuationHandle continuation;
};
```

Aucun champ `gap_small` n’est requis.

---

## 3. Q2 — le shell de `W_q`, q3/q4, appartient-il au contrat ?

### 3.1 Correction de mon audit précédent

**Non.** J’avais trop généralisé la distinction `upper_open/upper_closed` au
ledger de cœur q3/q4.

Pour q2 :

```text
W_2(a,b) = boule diamétrale ouverte de la paire.
```

C’est précisément la miniboule canonique du support q2. Sa frontière est donc
le vrai shell de cette `BallKey`.

Pour q3/q4, `W_q(a,b)` est un **cœur universel** obtenu en intersectant les
intérieurs des boules admissibles sous les contraintes de lane et d’owner. Sa
frontière est l’enveloppe de cette famille. Elle n’est pas la sphère d’une
miniboule canonique particulière.

L’égalité :

```text
4H^2=EX  pour q3
3H^2=EX  pour q4
```

signale la frontière angulaire du cœur universel. Elle ne signifie pas :

```text
Power_B(z)=0
```

pour la circumboule finale du triangle ou du tétraèdre produit.

### 3.2 Conséquence d’ABI

Pour le **prune universel** q3/q4, conserver seulement :

```text
lower_open
upper_open
```

avec les statuts :

```text
ALL_OPEN
NONE_OPEN
MIXED
MIXED_ENDPOINT
```

Un classifieur plus fort peut distinguer « extérieur même à la fermeture »,
mais ce bit reste une optimisation interne de `NONE_OPEN`. Il ne doit pas être
promu en shell HGP ni transporté dans l’ABI persistante.

### 3.3 Traitement des égalités

- une égalité ne crédite jamais `lower_open` ;
- si un bloc est prouvé sans aucun point **strictement** intérieur, y compris
  lorsque tous ses points sont sur la frontière du fuseau, il contribue zéro à
  `upper_open` ;
- sinon il reste `MIXED`.

Autrement dit, pour exclure l’intérieur ouvert, une borne non stricte du bon
côté suffit. Il n’est pas nécessaire de compter la fermeture de `W_q`.

### 3.4 Où le vrai shell q3/q4 apparaît

Après génération d’un support positif, sa `BallForm=(A,B,C)` fixe une boule
canonique. Le census final décide alors :

```text
P(z)<0   intérieur strict I_B
P(z)=0   shell U_B
P(z)>0   extérieur
```

C’est `BallFormRange-u16`, ou le reçu axial q4 équivalent, qui porte ce contrat.

Il faut donc séparer les types :

```text
CoreDepthLedger        // q3/q4 : prune universel, intérieur seulement
BallCensusLedger       // support fixé : intérieur + shell exacts
```

q2 peut réutiliser le second très tôt parce que sa paire détermine déjà sa
boule. q3/q4 ne le peuvent pas.

### 3.5 Réponse finale à Q2

```text
upper_closed dans le cœur q3/q4 : à retirer
upper_closed dans le census d’une BallForm fixée : obligatoire
q2 : cas particulier où les deux étages coïncident géométriquement
```

La question de Claude a donc identifié une vraie sur-généralisation de mon
précédent audit.

---

## 4. Q3 — quand décider une scission endpoint ?

### 4.1 La bonne réponse est une troisième option

Il faut distinguer :

1. la classification de la frontière **courante** ;
2. une classification spéculative des frontières **enfants**.

La première est obligatoire, quel que soit le choix futur. La seconde peut être
gaspillée.

La vague pair-major doit donc faire :

```text
1. classifier une fois tous les jobs courants (StateId,CNode)
2. réduire par StateId
3. appliquer les fates terminales
4. choisir UNE action avec le résumé courant et des proxys bon marché
5. count -> scan -> fill de la prochaine vague
```

La décision vient donc **après** une passe sur la frontière courante, mais sans
classifier les trois familles d’enfants candidates.

### 4.2 Pourquoi cette passe n’est pas un surcoût perdu

Le théorème introduit au commit `d80c4a2` est exactement celui qu’il fallait :

- `DEAD_*` est stable sous restriction de `A/B`, car les maxima ne peuvent que
  décroître ;
- `ALL_STRICT` est stable, car les minima ne peuvent que croître ;
- les preuves `ALL/NONE` du ledger universel sont également héritables ;
- seuls `MIXED` et `MIXED_ENDPOINT` sont rejoués.

La classification de la frontière courante construit donc les preuves que les
états enfants réutiliseront. Elle n’est pas jetée après le split endpoint.

### 4.3 Correction du critère de l’audit précédent

La formule :

```text
masse immédiatement classée / nombre de tâches enfants
```

était une bonne description conceptuelle d’un look-ahead, mais elle ne doit pas
entrer dans le premier ABI ni dans une gate de réception.

Tester intégralement :

```text
split A
split B
split C
```

puis jeter deux résultats multiplierait précisément le travail que le
scheduler pair-major cherche à supprimer.

Je retire donc le look-ahead exhaustif comme exigence initiale.

### 4.4 Politique déterministe v0 recommandée

Après classification et réduction de l’état courant :

```text
if fate terminale:
    terminer
else if pair_mass <= pair_tile_cap:
    EXACTIFY
else if existe un C MIXED non-feuille
        et front_count < frontier_cap:
    SPLIT_WITNESS(le plus gros C MIXED pondéré)
else:
    SPLIT_ENDPOINT(A ou B, une seule fois)
```

Le poids d’un span peut être :

```text
weight(C) = min(eligible_population(C), h_q)
```

ou, plus finement :

```text
weight(C) = min(eligible_population(C),
                max(need_to_kill,need_to_clear)).
```

Pour choisir `A` contre `B`, employer un proxy géométrique calculable depuis les
AABB des enfants, sans toucher à `C` :

```text
shrink(X) = diag2(X)-max_child diag2(child(X))
```

Choisir le plus grand `shrink`, puis départager par population et `NodeKey`.
Cette règle est heuristique, déterministe et versionnable. Elle ne participe
pas à la preuve.

### 4.5 Pourquoi cette politique est raisonnable

- un split witness ne modifie qu’un span local ;
- un split endpoint duplique tous les spans encore `MIXED` vers les enfants ;
- il faut donc épuiser les gros spans witness tant que la frontière reste
  bornée ;
- lorsque la frontière menace d’exploser, ou lorsque ses spans sont déjà des
  feuilles mais restent mixtes à cause de `A/B`, le split endpoint devient la
  seule action informative ;
- les spans déjà décidés sont hérités et ne sont jamais redescendus depuis la
  racine.

Cette v0 suffit à tester l’hypothèse structurelle du pair-major. Elle évite de
mélanger cette expérience avec une politique de split sophistiquée.

### 4.6 Look-ahead futur, si les mesures le demandent

Après réception de la v0, un look-ahead borné peut être ajouté, mais selon deux
règles :

1. il ne teste qu’un candidat présélectionné par proxy, pas les trois ;
2. tout travail calculé devient la prochaine vague et n’est pas jeté.

Une politique statistique ou adaptative pourra ensuite apprendre des compteurs
par famille. Elle restera une optimisation remplaçable, jamais une partie de
l’ABI de preuve.

### 4.7 Réponse finale à Q3

```text
classification de la frontière courante : OUI, une fois, obligatoire et réutilisée
classification spéculative de tous les enfants : NON
choix d’action : résumé courant + proxy AABB déterministe
```

---

## 5. Deux corrections de réception supplémentaires

### 5.1 Ne pas recevoir l’exposant générique `noeuds<2`

Le compteur courant `noeuds` additionne encore des unités différentes, notamment
les tâches ternaires et certaines évaluations internes du ledger. Il ne doit pas
servir seul de critère de réception du prochain commit.

Publier au minimum :

```text
pair_states
pair_witness_jobs
w4_corner_tests
acute_extrema_tests
endpoint_splits
witness_splits
relation_replays
```

La disparition du produit doit se lire dans `pair_witness_jobs` et
`endpoint_split_replays`, pas dans un agrégat dont l’unité change au fil du code.

### 5.2 Autonomie des lanes

Les réponses ci-dessus portent d’abord sur Lane4, mais l’ABI doit conserver :

```text
PairFrame immuable partagé
Lane2State autonome
Lane3State autonome
Lane4State autonome
```

En particulier :

- Lane3 conserve tous ses `CarrierBlock` ;
- Lane4 peut court-circuiter seulement son test existentiel d’activation ;
- le retrait de `upper_closed` du cœur q3/q4 ne retire évidemment pas le shell
  de leur `BallCensusLedger` final.

---

## 6. Gates à ajouter avant `PairFrame`

### GQ1 — le gap n’est pas une décision

Fixture :

```text
h=8, lower=7, upper=8
```

Exiger :

```text
gap=1
fate=MIXED_CORE
```

et tuer un mutant `gap-small-means-clear`.

### GQ1b — majorant grossier mais concluant

Une frontière grossière dont la somme admissible vaut sept sous `h=8` :

```text
upper=7
fate=CORE_CLEAR
```

sans descente aux feuilles.

### GQ2 — frontière de fuseau ≠ shell de boule

Construire un point sur :

```text
3H^2=EX
```

pour une paire q4, puis une complétion positive dont la circumboule canonique
place ce point strictement à l’intérieur ou à l’extérieur.

Exiger qu’il ne soit jamais publié comme shell de la `BallKey` finale au seul
motif qu’il est sur `partial W_4`.

### GQ3 — aucune passe spéculative jetée

Avec plusieurs spans mixtes :

```text
current_jobs_classified = front_count
candidate_child_jobs_discarded = 0
one_action_per_state = 1
```

### GQ3b — héritage stable

Après split endpoint :

```text
decided_spans_retested=0
mixed_spans_retested>0
relation_spans_replayed>0
```

---

## 7. Ordre immédiat donné à Claude

1. Corriger l’ABI projetée : `CoreDepthLedger` q3/q4 ne contient que
   `lower_open/upper_open`.
2. Garder le shell dans `BallCensusLedger`, et seulement là, sauf la
   spécialisation q2.
3. Implémenter `PairFrame` Lane4 avec la politique déterministe v0, sans
   look-ahead exhaustif.
4. Hériter tous les spans géométriquement décidés ; rejouer seulement `MIXED` et
   `MIXED_ENDPOINT`.
5. Ajouter GQ1--GQ3b.
6. Publier les compteurs physiques séparés.
7. Mesurer avant toute sophistication de la politique de split.

---

## 8. Verdict

| Élément | Verdict |
|---|---|
| masque relationnel et replay | reçu |
| juge par identités | reçu |
| mutant compensateur | reçu |
| partition exact-once indépendante | reçue |
| doublons de positions avec IDs distincts | reçus |
| théorème de stabilité sous raffinement endpoint | reçu |
| `upper-lower<h_q` comme arrêt | refusé |
| majorant grossier saturé | reçu |
| `upper_closed` dans le cœur q3/q4 | retiré/corrigé |
| shell final par `BallForm` | obligatoire |
| classification courante avant décision | requise |
| look-ahead complet A/B/C | refusé pour la v0 |
| politique pair-major déterministe | recommandée |
| réception par le seul compteur `noeuds` | refusée |

Les trois questions étaient donc utiles. Q1 évite une condition d’arrêt fausse,
Q2 corrige une sur-généralisation de mon audit, et Q3 sépare enfin le travail
nécessaire du travail spéculatif. C’est précisément le genre de questions qu’il
faut poser avant de couler l’ABI dans le béton, ce matériau que les logiciels de
recherche adorent employer juste avant de découvrir une égalité de frontière.
