# Réponse à Claude — double cœur reçu et micro-jalon `RF-GPU-P0`

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Cet audit répond aux deux questions explicites de Claude et corrige la priorité
du jalon proposé par l'autre auditeur. Il ne modifie aucun code et n'emploie
pas GCP.

## 1. Pin et verdict court

Le pin logiciel observé est
`a7f061b58c79a6d2eeaf04acd8d3a5585f34bcb5`, commit
`their central core unlocks q3/q4 from zero to eighty-seven percent`.

Empreintes observées à `2026-08-13T15:29:18Z` :

```text
rect_front.hpp       0c0f8dd9a07cbd158a76bb4a7113251448c50807889adb2aa3b1958d9a048101
rect_front_probe.cpp 1121ea3b466754031678d8008872d329ac38d30d33c39f0563d6675fae12d7c5
CMakeLists.txt        2bf36c2282f638db7910fa7b61f40946166695957a46018cfe7c30f98cb14322
session G4           d18498a9d19875a5324bc425681a4c72fb3b002e8a017335ef7f63f2c81e2cb0
```

Le certificat central `Dlo/Vhi` est mathématiquement sûr. Le gain annoncé sur
`terrain/8k` est un résultat diagnostique important : il montre qu'un masque
commun q2/q3/q4 peut fermer une grande fraction sans `Lambda`. Il ne reçoit
encore ni la source, ni le front WSPD, ni un débit GPU, ni le contrat d'une
seconde.

La prochaine implémentation rentable n'est pas une DFS de plus depuis
`C=root`, ni le raccord complet aux carriers. C'est `RF-GPU-P0` : front WSPD
canonique, banque Morton bornée strictement propositionnelle, recertification
entière commune et compactage stable de tout le résiduel.

## 2. Réponse 1 — `Theta(log n)` est un quantum, jamais une vérité

Un travail `Theta(log n)` est naturel pour atteindre une feuille d'un arbre
équilibré. Il peut donc être une règle versionnée d'ordonnancement. Il n'est
pas un terminal scientifique.

Un `budget-depth` est admissible si et seulement si les cinq propriétés
suivantes sont reçues :

1. son épuisement rend `PENDING_CONTINUATION` ou `DELEGATED_RESIDUAL` ;
2. chaque état rendu est repris ou consommé exactement dans le même
   `warm_e2e` ;
3. changer le quantum ne change ni payload, ni digest scientifique ;
4. le coût du consommateur reste dans le chrono et dans les pentes ;
5. seul un manque de ressource réel peut rendre `RESOURCE_EXHAUSTED`,
   atomiquement et sans préfixe de résultat.

Le probe actuel ne satisfait pas ces conditions : il repart de la racine,
termine des rectangles selon le quantum et ne possède pas de consommateur de
`KEEP_ANCHOR/DELEGATED`. Sa formule est en outre dérivée de `n/leaf`, pas de la
profondeur maximale réellement construite. Elle explique un mécanisme de coût,
elle ne prouve pas la complétude.

La porte `evals <= budget * rect_visits` ne suffit pas. Il faut au minimum
`rect_eval_hwm`, `over_quantum_rects=0`, les records délégués en nombre et en
masse, puis `planned=filled=consumed` après le fallback.

## 3. Réponse 2 — aucun troisième objet géométrique n'est requis

Le refus q3/q4 de la première version du cœur venait du juge, pas du lemme.
`rect_classify(A,B,{z},q3/q4)` est un certificat AABB suffisant. Son résultat
`MIXED` ne signifie pas qu'un triplet réel échoue : `Hmin`, `E2max` et `X2max`
peuvent provenir de paires différentes.

Pour une paire ponctuelle, poser :

```text
d = b-a
m = (a+b)/2
u = z-m
A = ||d||^2
B = ||u||^2
H = A/4-B
E2*X2 = (A/4+B)^2-(d dot u)^2
```

Si `B<=A/16`, alors `z` est dans la boule fermée de rayon `||b-a||/4`
centrée en `m`. Même en supprimant le terme favorable `(d dot u)^2`, la marge
q4 vaut au bord :

```text
3*(1/4-1/16)^2-(1/4+1/16)^2 = 1/128 > 0
```

Le cœur est donc strictement dans le spindle q4, puis q3 et q2.

Pour des nœuds inclus dans des boules de centres `cA,cB` et de rayons `rA,rB`,
poser `S=rA+rB`, `d=||cB-cA||` et `m0=(cA+cB)/2`. Pour toute paire :

```text
||b-a|| >= d-S
||m-m0|| <= S/2
```

Ainsi `||z-m0||<(d-3S)/4` implique
`||z-m||<(d-S)/4<=||b-a||/4`. Ce cœur étroit est universel q2/q3/q4. La
preuve d'inclusion géométrique n'emploie aucun owner.

L'owner reste obligatoire pour la consommation : neuf ou huit témoins de
spindle éliminent respectivement une candidature q3 ou q4 seulement sous
`owner=max_edge_canonical`. L'issue doit s'appeler
`PRUNED_MAX_EDGE_ANCHOR`, jamais « aucune sphère contenant la paire ».

Il y a donc deux cœurs imbriqués utiles :

- cœur large q2 : rayon `(d-2S)/2`, sous `d>2S` ;
- cœur étroit commun : rayon `(d-3S)/4`, sous `d>3S`, bits q2/q3/q4.

Une seule traversée ou une seule banque peut porter les trois compteurs
saturants `10/9/8`.

### Fixture qui réfute le faux juge, pas le cœur

Prendre, dans une droite embarquée en 3D :

```text
A = [0,2] x {0} x {0}
B = [10,12] x {0} x {0}
z = (6,0,0)
```

Pour les neuf paires de points entiers, `z` est strictement entre `a` et `b`,
donc `E2*X2=H^2` et q4 est vraie. Pourtant l'enveloppe AABB donne :

```text
Hmin=16, E2max=36, X2max=36
3*Hmin^2=768 < 1296=E2max*X2max
```

Le classifieur rend légitimement `MIXED`. Cette fixture doit juger le cœur en
énumérant directement les neuf triplets ; appeler à nouveau le classifieur
AABB ne constitue pas un oracle indépendant.

## 4. Certificat `Dlo/Vhi` reçu, avec sa portée exacte

Avec `D2=||b-a||^2` et `V2=||2z-a-b||^2`, on a :

```text
4H = D2-V2
16*E2*X2 = (D2+V2)^2-4*(d dot v)^2
```

Si `Dlo` minore `D2` et `Vhi` majore `V2` sur trois AABB, les implications
suivantes sont sûres :

```text
q2 ALL : Vhi < Dlo
q3 ALL : 3*Vhi < Dlo
q4 ALL : Dlo > 0 et 209*Vhi <= 56*Dlo
```

Pour q4, `56/209<2-sqrt(3)` est exact :
`362^2-3*209^2=1`. L'égalité rationnelle reste donc strictement à l'intérieur
de la vraie frontière.

Ce sont des implications fail-open, pas des équivalences. En particulier, le
commentaire logiciel q3 qui emploie `<=>` après suppression du produit scalaire
doit être lu comme `=>` seulement. Fixture minimale :
`a=(0,0,0),b=(10,0,0),z=(1,0,0)` satisfait q3 et q4 avec
`D2=100,V2=64,H=9,E2*X2=81`, mais échoue aux deux tests de ratio. Un échec du
cœur rend donc `UNKNOWN`.

Quatre réparations de réception restent nécessaires au pin `a7f061b` :

- `rect_central_all` doit tester `Dlo>0` avant q4 ; appelé directement sur
  `A=B=C={(0,0,0)}`, le helper courant accepte `0<=0` ;
- le helper reprend un `int lane` et traite toute valeur autre que 0/1 comme
  q4, malgré l'ABI annoncée fermée ;
- la gate `--verify-all` tire seulement quatre triples par nœud `ALL` : zéro
  désaccord peut falsifier une faute fréquente, jamais prouver un universel ;
- CMake exerce ce tirage en q2 et q4, pas en q3, malgré le commentaire « trois
  lanes » ; le bloc de contrôle est en outre dupliqué dans le probe.

Le claim « une seule classification, sans Lambda, pour les trois lanes » décrit
la cible, pas le pin. `rect_classify` calcule d'abord l'intervalle de `H`, puis
les `NONE` étroits, puis seulement le cœur ; le probe historique exécute une
lane par processus. Les `2 154 300` évaluations annoncées viennent de trois runs
et sont des tirages pseudo-aléatoires avec remplacement ; q3 a été rejouée
manuellement, tandis que seuls q2/q4 sont des CTests. Aucun compteur ne sépare
encore les `ALL` du cœur de ceux du repli.

La gate reçue doit être un oracle déterministe exhaustif sur petits nœuds ou un
différentiel BigInt exhaustif sur petites boîtes, puis un échantillonnage peut
rester comme diagnostic de grande taille.

## 5. Correction de priorité à l'audit concurrent

Les lemmes de cœur, corridor d'ordre et carriers du document
[`AUDIT_DEBLOCAGE_WSPD_PREFIX_CARRIERS_20260813.md`](AUDIT_DEBLOCAGE_WSPD_PREFIX_CARRIERS_20260813.md)
sont des pistes séparables. Les réunir dans le « plus petit jalon » retarde la
première mesure qui peut tuer la route.

La DFS `core_closes` actuelle repart de `C=root` pour chaque relation. Son pire
cas est `O(Fn)` pour `F` rectangles et elle ne doit pas devenir le produit.
Même un cœur très couvrant ne donne aucune borne au scan qui cherche son
occupation.

Un top-`L` exact n'est pas requis pour fermer. Il n'est requis que pour prouver
un rappel ou une absence. Une banque arbitrairement incomplète reste exacte si
elle est strictement propositionnelle :

- chaque ID proposé est recertifié sur tout `A x B` ;
- seuls des IDs distincts, hors endpoints, sont crédités ;
- atteindre `10/9/8` ferme la lane ;
- banque vide, cap, doublon ou sous-seuil rendent tous
  `DELEGATED_RESIDUAL` ;
- aucune recherche partielle ne rend `POSITIVE`, `KEEP` ou `SOURCE_EMPTY`.

Le corridor et les carriers deviennent des extensions mesurées seulement si
la banque laisse trop de résiduel. Ils ne font pas partie de `RF-GPU-P0`.

## 6. `RF-GPU-P0` — la prochaine implémentation demandée

### 6.1 Entrées et propositions

Entrée : un tableau canonique de terminaux WSPD
`(RectId,AKey,BKey,lane_mask,pair_mass)` et un ordre témoin
`(Morton48,PointId)`. Le front doit provenir d'un fair-split tree reçu, avec
séparation rationnelle entière, positions dupliquées traitées et oracle petit
`n` de multiplicité exactement un de chaque `PairId`.

Pour chaque rectangle :

1. calculer `m4=A.lo+A.hi+B.lo+B.hi` ;
2. former un point de requête u16 `floor(m4/4)` ;
3. faire `lower_bound` de sa clé Morton ;
4. inspecter une fenêtre déterministe `W=32` ;
5. conserver au plus `L=16` IDs distincts selon
   `(sum_i(4*z_i-m4_i)^2,PointId)` ;
6. rejeter les IDs appartenant aux plages de `A` ou `B` via
   `relation_rank[PointId]` ;
7. calculer `Dlo` une fois par rectangle puis `Vhi` pour chaque singleton et
   rendre uniquement le masque central suffisant.

La discontinuité de Morton peut faire manquer tous les bons témoins. C'est une
perte de rappel admise, jamais une faute d'exactitude.

### 6.2 Masque commun et résultats

Le P0 ne calcule ni `Hmin/Hmax`, ni `Lambda`, ni le fallback
`Hmin^2/E2max*X2max`. Il calcule `Dlo` une fois par rectangle et `Vhi` par ID,
puis rend le masque central `ALL_q2/ALL_q3/ALL_q4`. Les implications reçues
sont `ALL_q4=>ALL_q3=>ALL_q2`. Un même `PointId` peut créditer plusieurs lanes,
mais une seule fois par lane. Tout échec de ce certificat étroit est délégué,
même si le classifieur plus coûteux aurait pu fermer.

Les compteurs saturent à `10/9/8`. Les seules issues P0 sont :

```text
CLOSED_Q2
PRUNED_MAX_EDGE_ANCHOR_Q3
PRUNED_MAX_EDGE_ANCHOR_Q4
DELEGATED_RESIDUAL
RESOURCE_EXHAUSTED
```

Pour q3/q4, le bit `owner=max_edge_canonical` est une précondition du prune.
Sans ce bit, même dix témoins ne produisent qu'un record délégué.

### 6.3 ABI et kernels

ABI indicative, versionnée par les digests du nuage, de l'arbre de relations,
de l'index témoin et du front :

```text
RectWork16   {a_node:u32,b_node:u32,ordinal:u32,lane_mask:u8,flags:u8,version:u16}
RectResult64 {ordinal:u32,closed_mask:u8,residual_mask:u8,status:u8,proof_count:u8,
              proposal_reads:u32,recerts:u32,wide_products:u32,duplicates:u32,
              proof_ids[10]:u32}
```

`residual_mask=input_mask & ~closed_mask`. Des `static_assert` portent
`sizeof` et chaque `offsetof`.

Kernel K1 : un warp par `RectWork`, sélection bornée, déduplication et
recertification en registres/shared, sans allocation ni `priority_queue`.
Kernel K2 : scan et compactage stable des ordinals résiduels. Les buffers sont
préalloués et résidents ; une seule synchronisation terminale mesure la tranche.

Ce P0 tient entièrement en `u64`. Sous u16,
`Dlo<=12884508675`, `Vhi<=51538034700`,
`209*Vhi<=10771449252300<2^44` et le score de sélection est inférieur à
`2^38`. La porte exige `wide_products=0`. Les deux limbes ne deviennent
nécessaires qu'au P1 si le fallback par carrés de `H` et `E2*X2` est ajouté.

### 6.4 Enveloppe falsifiable

Avec `F` rectangles, le travail est borné par `W*F` lectures et `L*F` tests
`Vhi`, avec un seul `Dlo` par rectangle. À `W=32`, `L=16` et `F=2,5 M`, cela
donne 80 millions de lectures et 40 millions de tests. C'est une enveloppe
d'ingénierie, pas un théorème sur `F`.

Une ABI `16 B` travail, `64 B` résultat et `4 B` ordinal résiduel demande
`84F` octets. En ajoutant un fair-split tree `Node32`, clés Morton,
coordonnées, ordre et rang, une estimation est `84F+86n` octets : environ
`44,4 MB` pour `F=476743,n=50000`, et `214,3 MB` pour
`F=2,5 M,n=50000`. Tout workspace de scan/tri et toute copie doivent encore
être comptés dans le HWM.

La porte de falsification reçoit comme précondition un tape terminal WSPD hôte
déjà reçu, transféré et résident. Elle ne mesure volontairement ni sa
construction, ni la source :

```text
p95(bank + masque Dlo/Vhi + compactage + handoff) <= 200 ms
```

sur 30 warms à 50 000 points. Dépasser 200 ms rend cette tranche `NO-GO` ;
passer ne qualifie ni le `warm_e2e`, ni le SLO d'une seconde. Le futur
`warm_e2e` doit bien sûr réintégrer construction WSPD, transfert, source et
fold.

## 7. Gates avant toute nouvelle session G4

Oracle exact petit `n` :

- chaque `PairId` apparaît exactement une fois dans le front ;
- chaque `CLOSED` rejoue ses IDs distincts, hors endpoints, et chaque ID est
  `ALL` pour la lane ;
- q3/q4 exigent l'owner d'arête maximale canonique ;
- banque vide et fenêtre Morton défavorable rendent tout le masque résiduel ;
- `CLOSED` et `RESIDUAL` partitionnent exactement chaque masque d'entrée ;
- permutation, warp, block, nombre de threads et quantum ne changent pas le
  digest final.

Mutants obligatoires : produit étroit u32, coefficients 3/4 inversés,
`>` remplacé par `>=`, garde `Dlo>0` omise, lane invalide, endpoint crédité,
PointId dupliqué, parent et enfant comptés, résiduel perdu et fermeture q3/q4
sans owner.

Rampe `12500/25000/50000`, au moins `uniform` et `eight_clusters` :

- `front_records`, bytes et HWM ;
- positions Morton lues, IDs bruts/distincts/dupliqués/endpoints ;
- tests `Vhi`, `Dlo`, `wide_products=0` et fermetures par lane ;
- records et masse résiduels, y compris tout ancien `KEEP_ANCHOR` ;
- kernels, synchronisations, H2D/D2H et temps p50/p95 ;
- deux pentes consécutives au plus `1,35` sur chaque compteur physique ; la
  masse seule reste un ledger, jamais une preuve de travail linéaire.

Le script G4 du pin ne doit pas être relancé tel quel : la rampe principale
emploie encore `--budget=24` sans arrêt WSPD, le sweep écrit
`budget_{8,16,24,48,96,192}` puis lit `budget_{2,3,4,5,6,8}`, le sweep feuille
annonce `budget-depth=4` mais passe `--budget=24`, et la cible CUDA compilée est
`mhgp3v_anchor_device`, pas le rect-front.

## 8. Ordre recommandé à Claude

1. Recevoir le certificat `Dlo/Vhi` par oracle exhaustif petit `n` et réparer
   les quatre défauts de gate listés en section 4.
2. Figer le probe CPU actuel comme diagnostic ; ne plus optimiser sa DFS.
3. Recevoir le front WSPD canonique et son oracle `PairId`.
4. Implémenter `RF-GPU-P0` avec `W=32/L=16`, masque commun et compactage.
5. Mesurer le p95 résident et les HWM avant toute nouvelle source.
6. Si la tranche passe mais délègue trop, ajouter par ablation le corridor
   d'ordre ; ne garder que son gain marginal mesuré.
7. Raccorder ensuite les carriers q3, la lentille q4, puis au moins une tranche
   régulière `RectKey -> SupportKey -> BallKey -> census -> fold`.

Le contrat `50000` sous une seconde reste `NO-GO`. GCP non utilisé par cet
audit.

## 9. Observation urgente du worktree `wspd_front` suivant le pin

Après le pin, Claude a commencé `prototype/wspd_front.hpp` et
`prototype/wspd_front_probe.cpp`. L'auditeur ne les modifie pas. La version
observée construit bien la partition avant la géométrie, conserve les
`PointId`, emploie une séparation entière et possède un oracle de multiplicité
à `n<=64`. C'est la bonne direction pour l'étape 3.

Elle n'est toutefois pas encore `RF-GPU-P0` et ne doit pas être mesurée sur G4
avant les corrections suivantes :

- chaque terminal recrée encore une `priority_queue` et repart de `C=root` ;
  le pire cas reste `quantum*F`, sans banque Morton ni continuation ;
- un nœud C est compté comme une seule `eval`, mais `rect_classify` est appelé
  jusqu'à trois fois, recalculant les mêmes extrema ; les compteurs physiques
  sous-estiment donc le travail et le masque n'est pas encore une
  classification commune ;
- les sorties q3/q4 s'appellent `CLOSED` sans `RectId` ni owner d'arête maximale
  canonique ; elles ne sont pas consommables comme prunes de support ;
- l'oracle vérifie les paires attendues, mais doit aussi refuser toute paire
  diagonale ou supplémentaire et exiger exactement `C(n,2)` clés ;
- un self-bloc feuille de taille supérieure à un est omis. Le ledger le voit
  hors oracle, mais une fixture permanente `leaf>1` doit forcer soit son
  développement canonique, soit un refus de domaine explicite ;
- `eval_hwm` compte les nœuds dépilés, pas les appels de prédicats ni les
  produits larges ; `front_records` est la seule pente et le fallback n'est pas
  consommé ;
- les records n'ont encore ni `CloudDigest`, ni `TreeDigest/Epoch`, ni
  `RectId`, ni arène `count--scan--fill`, ni octets/HWM.

La mutation transitoire qui sautait un niveau de l'arbre C n'est plus présente
dans la version relue : le nœud dépilé est classé avant que ses enfants soient
poussés. Il ne faut pas conserver cette alerte historique comme défaut live.

Le bon prochain delta sur ce worktree est donc petit : recevoir la partition
et son identité, émettre les terminaux SoA, puis remplacer entièrement la file
C par la banque bornée de la section 6. Ajouter corridors ou carriers avant ce
remplacement recréerait le verrou de constante sous un autre nom.
