# Déblocage Claude — préfixe WSPD mesurable et vraie source par carriers

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Cet audit est une proposition d'implémentation adressée à Claude. Il ne modifie
aucun code, ne promeut aucun claim et n'emploie pas GCP.

## 1. Décision : arrêter de régler le budget du probe actuel

Le `HEAD` observé est `e17a67e35890a813b1030a075b7c22a922c4ad78`.
Le worktree contient seulement les réponses logicielles de Claude dans
`rect_front.hpp` et `rect_front_probe.cpp`; l'auditeur ne les touche pas.

Le balayage transmis dans
[`NOTE_CLAUDE_BUDGET_PROFONDEUR_20260813.md`](NOTE_CLAUDE_BUDGET_PROFONDEUR_20260813.md)
montre qu'un quantum `48` ferme davantage de masse q2 que `24`. Il ne montre
pas que le verrou source est résolu : les runs reconstruisent une file depuis
`C=root` pour chaque rectangle, traitent les lanes séparément, n'activent pas
l'arrêt WSPD et ne consomment aucun résidu jusqu'au fold.

La prochaine étape ne doit donc être ni un nouveau balayage de budget, ni une
nouvelle session G4 du même probe. Elle doit être un seul préfixe obligatoire,
nommé `WspdFrontLowerBound-v1`, qui mesure l'ordonnance réellement candidate.

## 2. Réponse définitive à la question `Theta(log n)`

Un quantum dérivé de la profondeur est légitime comme **ordonnance** : il peut
fixer le nombre de tâches qu'un fast path exécute avant de transférer son état
à une source exacte. Il n'est jamais une preuve de complétude et ne doit pas
être un paramètre utilisateur du produit.

La règle est simple :

- la formule du quantum est versionnée dans le binaire, liée au digest de
  l'arbre et publiée dans le reçu ;
- modifier le quantum peut changer les compteurs `FAST/DELEGATED`, jamais le
  résultat scientifique après consommation complète ;
- à épuisement, le rectangle devient `DELEGATED` avec un état rejouable ; il
  ne devient ni `CLOSED`, ni `POSITIVE`, ni `SOURCE_EMPTY` ;
- le fallback et son coût restent dans le même `warm_e2e` ;
- un vrai manque de mémoire rend `resource_exhausted` atomiquement, sans
  préfixe scientifique.

La continuation persistante n'est donc pas la seule possibilité. Un handoff
terminal vers une autre source **complète** est également exact. Tant que cette
source n'existe pas, le quantum ne qualifie rien.

`evals <= budget * rect_visits` n'est pas une porte suffisante : les appels
self qui ne classifient aucun nœud donnent du slack. Il faut publier
`rect_eval_hwm`, `over_quantum_rects=0` et la consommation intégrale de chaque
record délégué.

## 3. Le plus petit jalon qui débloque réellement la seconde

`WspdFrontLowerBound-v1` couvre, dans une seule exécution synchronisée :

1. validation et transfert du nuage u16 frais ;
2. arbre canonique et WSPD entière à `s=2` ;
3. classification **des seuls terminaux WSPD**, avec un masque q2/q3/q4 ;
4. fast path du cœur central ;
5. fast path du corridor d'ordre sur les rectangles admissibles ;
6. `count -> scan -> fill` des arènes `CLOSED`, `Q2_SOURCE_FRONT`,
   `CARRIER_FRONT_3`, `CARRIER_FRONT_4` et `DELEGATED` ;
7. synchronisation et retour d'un reçu compact sur hôte.

Ce préfixe ne développe aucun `PairId`, ne forme aucun atlas de cellules et ne
construit aucune coface globale. Il laisse les arènes sur device et ne retourne
que compteurs, digests et HWM.

Si son p95 strict à 50 000 points vaut déjà au moins une seconde, cette
ordonnance ne peut pas satisfaire le contrat secondaire, puisque tout son
travail est obligatoire avant l'aval. Une cible d'ingénierie de `250--400 ms`
est raisonnable pour décider si l'aval mérite d'être écrit ; elle n'est pas un
SLO public. Un résultat inférieur à une seconde ne qualifie encore rien.

## 4. Construire d'abord la WSPD, classifier ensuite

Le probe courant appelle le classifieur sur les rectangles internes puis sur
les terminaux. Cela paie presque deux fois la traversée et mêle construction du
front et couverture géométrique.

La route candidate est à deux passes :

1. construire la partition WSPD sans appeler `H` ;
2. classifier exactement une fois chaque `RectId` terminal.

Le tree doit être un fair-split tree canonique, ou un octree comprimé de
cellules d'aspect borné accompagné de sa preuve WSPD. Un LBVH binaire arbitraire
ne reçoit pas automatiquement la borne. L'ordre terminal est
`(Morton48,PointId)` et tout tie de split finit par `PointId`.

Pour une séparation entière en norme infinie, poser pour un nœud
`c2_i=lo_i+hi_i` et `r2=max_i(hi_i-lo_i)`. Pour deux nœuds, poser
`d2=max_i(abs(c2A_i-c2B_i))`. À `s=2`, l'arrêt exact est :

```text
d2 >= r2A + r2B
d2 - r2A - r2B >= 2 * max(r2A,r2B)
```

Toutes les quantités sont doublées, entières et tiennent en `i64` sous u16.
La WSPD ne décide aucune géométrie scientifique : un rectangle séparé mais non
fermé est simplement transmis.

Positions coïncidentes : refus explicite ou quotient reçu avant cette passe.
La somme des masses ne suffit pas comme preuve d'identité. À petit `n`, le juge
développe tous les records et exige une multiplicité exactement un pour chaque
`PairId`.

## 5. Premier fast path : cœur central entier partagé par les trois lanes

Pour un triple ponctuel, poser :

```text
D2 = ||b-a||^2
V2 = ||2z-a-b||^2
d  = b-a
v  = 2z-a-b
```

Les identités exactes sont `4H=D2-V2` et
`16*E2*X2=(D2+V2)^2-4*(d dot v)^2`. En supprimant le dernier terme négatif,
on obtient des certificats `ALL` suffisants et très bon marché.

Sur `A x B x C`, calculer :

- `Dlo`, minimum exact de `||b-a||^2` entre les AABB `A,B` ;
- `Vhi`, maximum exact de `||2z-a-b||^2`, par axe sur l'intervalle
  `[2*Clo-Ahi-Bhi, 2*Chi-Alo-Blo]`.

Verdicts sûrs :

```text
Dlo doit être strictement positif
q2 ALL : Vhi < Dlo
q3 ALL : 3 * Vhi < Dlo
q4 ALL : 209 * Vhi <= 56 * Dlo
```

Le dernier test est strictement intérieur à la vraie frontière parce que
`56/209 < 2-sqrt(3)`. Tous les bits sont imbriqués et calculés en une seule
classification. Un échec reste `UNKNOWN`; ces tests ne rendent jamais `NONE`,
`SOURCE_EMPTY` ou support positif.

Fixture q3 de frontière, qui tue `3*Vhi<=Dlo` :

```text
a=(0,4,0), b=(12,16,0), z=(8,8,4)
D2=288, V2=96, v dot d=0
```

Le point est exactement au contact q3 et doit rester ouvert.

La garde `Dlo>0` est nécessaire malgré la frontière rationnelle intérieure :
sans elle, `Dlo=Vhi=0` ferait passer q4 à cause de `0<=0`, alors que `H=0`.
La fixture dégénérée `A=B=C={(0,0,0)}` doit donc rendre non-ALL sur les trois
lanes, indépendamment du futur refus de positions coïncidentes.

### 5.1 Correction immédiate du juge du cœur au successeur `e63b7eb`

Claude a correctement reçu et jugé le cœur sphérique q2, puis a désactivé q3/q4
parce que `rect_classify(A,B,{z},q3/q4)` rendait souvent `MIXED`. Cette
conclusion est trop forte : pour q3/q4, `rect_classify` est un certificat
**suffisant**, pas un oracle exact. Il combine `Hmin` avec `E2max*X2max`, dont
les deux maxima peuvent être atteints sur des paires différentes. `ALL` est
sound, mais `MIXED` ne réfute pas le point du cœur.

Le cœur de rayon `D/4` autour du milieu d'une paire est bien inclus dans le
spindle q4, donc aussi q3 et q2. Preuve ponctuelle : écrire
`d=b-a`, `m=(a+b)/2`, `u=z-m`, `A=||d||^2`, `B=||u||^2`. Alors :

```text
H = A/4 - B
E2*X2 = (A/4 + B)^2 - (d dot u)^2
```

Si `B<A/16`, alors `H>0` et même en supprimant le terme favorable
`(d dot u)^2`, on a strictement :

```text
3 * (A/4 - B)^2 > (A/4 + B)^2
```

En effet, avec `x=B/A<=1/16`, la marge normalisée vaut
`1/8 - 2*x + 2*x^2`, décroît sur cet intervalle et vaut encore `1/128` à
`x=1/16`. Le prédicat q4 `3H^2>E2*X2` est donc strict.

Pour deux nœuds contenus dans des boules `(c_A,r_A)` et `(c_B,r_B)`, poser
`d=||c_B-c_A||`, `S=r_A+r_B` et `m0=(c_A+c_B)/2`. Pour toute paire :

```text
||b-a|| >= d-S
||m-m0|| <= S/2
```

Donc tout `z` tel que `||z-m0||<(d-3S)/4` satisfait
`||z-m||<=(d-3S)/4+S/2=(d-S)/4<=||b-a||/4`. Ce cœur bloc est universel q4,
q3 et q2. Cette preuve n'emploie aucune précondition d'arête maximale owner.

Le juge indépendant à graver pour q3/q4 doit parcourir, sur de petits nœuds,
les **points réels** `a` de `A` et `b` de `B`, puis évaluer directement en
largeur suffisante `H>0`, `4H^2>E2*X2` et `3H^2>E2*X2`. Il ne doit appeler ni
`rect_classify`, ni ses extrema AABB indépendants. Le mutant du rayon trop
grand est tué dès qu'un seul triplet ponctuel direct échoue.

Un échantillon de quatre triples par nœud est un bon falsificateur de rampe,
pas une réception. La porte bornée restreint les tailles des nœuds puis
développe exhaustivement tous les triples réels de chaque verdict `ALL`.

Pour économiser réellement les produits, `Dlo/Vhi` doit être évalué **avant**
`rect_h_interval`, avec `Dlo` mis en cache par `RectId`. Il rend directement un
`closed_mask` imbriqué pour les trois lanes. Appeler d'abord `Lambda`, ou
réexécuter ce helper séparément pour q2/q3/q4, conserve l'essentiel du coût que
le fast path devait supprimer.

Le fast path sphérique de `e63b7eb` peut donc être réactivé sur les trois lanes
après ce nouveau juge. Le fast path `Dlo/Vhi` ci-dessus est sa version un peu
plus large et sans racine carrée ; les deux peuvent être comparés par ablation.

Rejeu diagnostique du premier worktree qui applique cette correction,
`rect_front.hpp=0c0f8dd9...` et `rect_front_probe.cpp=1121ea3b...`, Release CPU,
`terrain`, `n=2 000`, `budget-depth=4` :

| lane | masse fermée | triples ponctuels directs | désaccords |
| --- | ---: | ---: | ---: |
| q3 | `62,02 %` | `111 200` | `0` |
| q4 | `56,67 %` | `103 384` | `0` |

Ce résultat falsifie l'interprétation de l'ancien juge, pas le besoin d'une
porte exhaustive. Il paie encore environ deux millions de classifications par
lane à seulement 2 000 points, parce que WSPD, masque commun et classification
terminale ne sont pas encore installés.

## 6. Deuxième fast path : corridor d'ordre, sans DFS par rectangle

Dans la chambre canonique `x>=y>=z>=0`, utiliser la transformée unimodulaire
`T3(x,y,z)=(x-y,y-z,z)`. Son inverse est
`(p,q,r)->(p+q+r,q+r,r)` et ses rayons sont
`(1,0,0),(1,1,0),(1,1,1)`.

Deux vecteurs non nuls de cette chambre ont un cosinus carré au moins `1/3`.
Ils satisfont donc strictement q2 et q3. Pour un rectangle `A x B`, poser :

```text
L_i = max over A of T3_i(a)
U_i = min over B of T3_i(b)
Q   = {z : L_i <= T3_i(z) <= U_i for i=1,2,3}
```

Si `L<=U`, tout `z` de `Q` hors endpoints est un témoin universel de toutes les
paires du rectangle. Le même corridor certifie q4 si `L_2<U_2`; cette garde
exclut les deux seuls rayons extrêmes qui atteignent l'égalité q4.

Fixture obligatoire :

```text
a=(0,0,0), z=(1,0,0), b=(2,1,1)
H=1, E2*X2=3
```

q3 est strict mais q4 est à l'égalité. Sans `L_2<U_2`, q4 reste inconnue. Un
fallback q4 plus couvrant emploie les deux cônes unimodulaires
`T40=(x-y-z,y-z,z)` et `T41=(x-y,y-z,y+z-x)`.

Sous positions distinctes, `Q` contient au plus un endpoint de `A` et un de
`B`. Un range-report capé à `h+2`, puis filtré par les deux `NodeKey`, suffit
donc pour recevoir `h=10/9/8` IDs distincts.

Implémentation GPU proposée : un LBVH unique en xyz et une wavefront de requêtes
`(RectId,TransformId,L,U,lane_mask)`. Pour chaque AABB visitée, les intervalles
des trois formes linéaires se calculent par choix de coins ; aucune copie de 48
arbres n'est nécessaire. Le report s'arrête au seuil. Cap ou sous-seuil donne
`CARRIER_FRONT`, jamais un verdict négatif.

La v0 peut essayer un seul transformé admissible choisi canoniquement et
déléguer en cas d'échec : elle reste exacte, seulement moins couvrante. Une v1
qui prétend épuiser le certificat doit unir tous les transformés et dédupliquer
par `(RectId,PointId,lane)` ; `TransformId` est alors seulement l'owner du
doublon.

## 7. Les seules issues sémantiques autorisées

| issue | preuve |
| --- | --- |
| `CLOSED_RANK_WINDOW(q)` | au moins `10/9/8` témoins universels stricts et distincts |
| `Q2_FACTORIZED_RELEVANT` | borne supérieure exhaustive q2 strictement sous dix |
| `CARRIER_FRONT_3/4` | toute non-fermeture, ambiguïté, sous-seuil ou cap |
| `SOURCE_EMPTY` | sur-approximation complète des carriers prouvée vide sur tout `A x B x X` |
| `DELEGATED` | état exact consommable, sans décision scientifique |

Le jalon v1 n'a pas besoin de produire `SOURCE_EMPTY`. Il est plus sûr de tout
transmettre en `CARRIER_FRONT` jusqu'à ce qu'une requête carrier exhaustive
soit reçue.

`cred+pending<h` ne prouve jamais `SOURCE_EMPTY`. Il majore les témoins du
spindle universel, pas l'existence d'un carrier propre. Exemple q3 :

```text
a=(0,0,0), b=(4,0,0), x=(2,3,0)
D2=16, E2=X2=13, H=-5
```

`abx` est un triangle aigu porté par l'arête maximale alors qu'aucun crédit
universel supplémentaire n'est nécessaire pour le faire exister.

## 8. Raccord exact à la vraie source q3

Pour une paire owner `ab`, poser `D2=||b-a||^2`. Un point `x` est un carrier q3
propre avec `ab` arête maximale exactement lorsque :

```text
H=(x-a) dot (b-x) < 0
||x-a||^2 <= D2
||b-x||^2 <= D2
```

Forme équivalente sans produit scalaire :

```text
||x-a||^2 <= D2
||b-x||^2 <= D2
D2 < ||x-a||^2 + ||b-x||^2
```

Les inégalités de longueur sont faibles pour conserver les ties d'arête
maximale ; l'acuité est stricte. Le plus petit `PairId` parmi les arêtes
maximales devient owner. Ce prédicat trouve des carriers ; il ne décide encore
ni census, ni shell, ni pertinence finale.

Un classifieur ternaire sur `A x B x C` peut employer les marges
`D2-E2`, `D2-X2` et `-H`. Un nœud `C` est `NONE-carrier` si l'un de leurs
majorants rend le système impossible ; il est `ALL-carrier` si les trois
minorants satisfont les frontières ci-dessus. `SOURCE_EMPTY` n'est autorisé
qu'après partition exhaustive de `X` en nœuds `NONE-carrier`.

## 9. Raccord exact à q4 : ne pas exiger deux carriers aigus

Un q4 positif dont `AB` est arête maximale possède au moins une face aiguë
`ABx`, mais pas nécessairement deux. La source doit former une paire `x,y`
telle que :

```text
x et y appartiennent à la lentille fermée de AB
acute(x) || acute(y)
||x-y||^2 <= D2
rang affine = 3
positivité du tétraèdre stricte
owner d'arête maximale canonique
```

Contre-fixture à la faute « deux carriers aigus » :

```text
a=(0,3,2), b=(6,3,2), x=(1,0,1), y=(3,5,0)
H_x=-5, H_y=+1
```

`AB` est l'unique arête maximale et le tétraèdre est positif. Filtrer `y` par
la sortie q3 le perdrait.

Le moteur shallow existant doit donc conserver toute la lentille avec un bit
`acute`, former les cas `P-P/N-N/P-N`, tester `||x-y||^2<=D2`, rang,
positivité et owner, puis effectuer le census avec **tous** les points. Les
points non aigus ou hors lentille peuvent encore être témoins intérieurs ; la
lentille filtre les porteurs, jamais le flux de census.

## 10. ABI compacte remise à Claude

```text
WspdRectRecord {
  RectId, ANodeKey, BNodeKey, pair_mass, lane_mask
}

FrontDecisionRecord {
  RectId, closed_mask, sort,
  witness_count[3], witness_ids[10],
  TransformId, continuation_span, receipt_digest
}

CarrierFrontRecord {
  RectId, ANodeKey, BNodeKey,
  lane_mask, carrier_span, lens_span,
  acute_mask_span, continuation_span
}
```

Les arènes sont SoA. Chaque passe fait `count -> scan -> fill` stable ; aucun
scratch maximal par ancre, aucune `priority_queue` par rectangle et aucun
atomic global par support. Les records délégués gardent l'antichaîne ou le
curseur nécessaire pour ne pas revenir à `C=root`.

## 11. Porte minimale avant tout nouveau développement q4

Oracle hôte, `n<=64` :

- multiplicité exactement un de chaque `PairId` dans la WSPD ;
- conservation par lane de toute la masse ;
- aucun faux `CLOSED` contre le spindle ponctuel ;
- égalité des `SupportCandidateKey` q3/q4 après consommation du carrier front ;
- échange `A/B`, permutations d'entrée, ties de Morton et PointId ;
- frontières `H=0`, q3, q4, corridor et max-edge ;
- quantum différent, nombre de threads différent et tuilage différent donnent
  les mêmes sorties finales ;
- `planned=filled=consumed` pour chaque sort et `repeated_task=0`.

Rampe physique `12 500/25 000/50 000`, d'abord `uniform` et
`eight_clusters` :

- `wspd_nodes`, `front_records`, `front_bytes`, HWM ;
- classifications terminales, jamais internes ;
- core hits par lane ;
- corridor queries, nœuds visités, IDs bruts, distincts et endpoints rejetés ;
- `CLOSED`, `CARRIER_FRONT`, `DELEGATED` en records **et** masse ;
- octets lus/écrits par passe et nombre de produits larges ;
- `rect_eval_hwm`, `over_quantum_rects`, pushes/pops ;
- deux pentes consécutives de chaque compteur de travail, seuil `1,35` ;
- temps build/WSPD/core/corridor/fill et temps préfixe synchronisé.

Une masse factorisée quadratique est permise ; un travail physique quadratique
ne l'est pas. Oublier `KEEP_ANCHOR`, `CARRIER_FRONT` ou `DELEGATED` du ledger
est un faux vert.

## 12. Ordre d'implémentation recommandé

1. Figer le probe actuel comme diagnostic ; ne plus optimiser sa file.
2. Écrire la WSPD entière/canonique et son oracle de multiplicité.
3. Produire uniquement ses terminaux en SoA et mesurer leurs octets.
4. Ajouter le masque partagé et le fast core `Dlo/Vhi`.
5. Ajouter une seule requête corridor canonique par terminal admissible.
6. Publier `WspdFrontLowerBound-v1` avec tout échec délégué.
7. Porter ce préfixe sur device et appliquer la rampe/p95.
8. Seulement si le préfixe tient le budget d'ingénierie, consommer
   `CARRIER_FRONT_3` par le prédicat exact q3.
9. Conserver toute la lentille et raccorder le moteur shallow q4.
10. Raccorder enfin `BallKey -> census -> fold -> BenchmarkOutputContract-v1`.

Cette séquence donne un résultat utile à chaque étape et peut tuer la route
avant d'investir dans le stade suivant. Elle n'introduit aucune mosaïque de
Delaunay d'ordre supérieur : arbre de points, front de rectangles, listes
locales de carriers et arrangements shallow temporaires seulement.

## 13. Non-claims

Le corridor et le cœur sont des certificats `ALL` opportunistes, pas une source
exhaustive. La WSPD borne les `RectId`, pas les range reports, les carriers, les
niveaux q4, le shell, les sorties ou le fold. Aucun théorème ici ne prouve une
source globale linéaire en pire cas.

Le reçu G4 transmis mesure l'ancienne ordonnance CPU, sans arrêt WSPD, avec
lanes séparées et reprise à `C=root`. Il est utile comme réfutation de cette
ordonnance, pas comme mesure de `WspdFrontLowerBound-v1`.

Le contrat `50 000` sous une seconde reste ouvert. GCP non utilisé par cet
audit.
