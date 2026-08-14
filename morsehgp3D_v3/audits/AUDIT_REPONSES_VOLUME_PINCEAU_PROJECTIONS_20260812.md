# Réponses auditées — volume, pinceau et source support-first

Date : 12 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

La note de question et de provenance, désormais absorbée, a été supprimée ; le
présent audit conserve les mesures, réponses et contre-exemples durables.
Le `HEAD` observé est
`8c00ab07695ef353e673ab73a778a6f260c87509`. Le snapshot logiciel audité est :

| objet | SHA-256 |
| --- | --- |
| `CMakeLists.txt` | `3135e86422b9e7fc6cff11e4ee661c2f9c9af49bdaec20b0217d17a0f8d5a7e4` |
| `prototype/ball_front.hpp` | `221356332743af11481a5387d65f6d27e0ec2b0ce0e10e2118f3796bb763d490` |
| `prototype/ball_front_probe.cpp` | `c815ffa828cd59e6a046df185d5cbcfaf4ea2d6e566420e0c7eb2e92efb04c04` |
| binaire Release `mhgp3v_ball_front` | `4bce8d09716ef795c9a8ff60a397686559950fb2c1fb58e2f2b1f64edd34b3e0` |
| `prototype/order_k_flats.hpp` | `a70f990adfff9bec9b810059c32ba9ec62aef95a3b06e679a3fb6f06b5af8bc8` |
| `prototype/flats_scale_probe.cpp` | `b3ecf5db981bab9741a97e828a6a00db996dab1f2be2678ddb5f50375e793a2d` |

Toute modification de l'un de ces objets rend seulement historiques les constats
logiciels associés. Le verdict live et les résultats CTest sont tenus dans
[`AUDIT_ETAT_COURANT.md`](AUDIT_ETAT_COURANT.md).

## Verdict

1. La densité mesurée sur une famille et quatre tailles ne prouve pas
   `Theta(n)`. Les sommets du plein arrangement sont des états de navigation
   propres à une ordonnance, pas la sortie normative et pas un minorant de toute
   source exacte.
2. Une famille u16 explicite sépare même quadratiquement ces deux volumes : à
   `n=50 000`, elle possède `34 364 000 715` sommets relevés à shell quatre de
   niveau au plus neuf, tous transits non positifs, mais seulement `499 945`
   supports q2--q4 de Source S. Le front intégral est
   donc refusé comme architecture 50 k, indépendamment de ses constantes.
3. Le successeur de pinceau courant choisit algébriquement le premier croisement
   dans les deux sens sans produit croisé large. Il reste un DFS heuristiquement
   ordonné, de pire cas linéaire par requête, suivi d'un second parcours de
   shell. Il n'est ni best-first ni logarithmique.
4. Le théorème de propriétaire reçoit la complétude d'une récolte q2/q3 depuis
   **tous** les sommets shallow en dimension affine trois. Il ne reçoit ni le
   coût de cette récolte, ni la conservation du niveau, ni l'émission courante.
5. La route industrielle doit chercher les supports avant les transits, séparer
   les plafonds `q2/q3/q4=9/8/7`, faire un census en deux passes, agréger par
   `BallKey` et représenter les plateaux par générateurs saturés. Aucune de ces
   décisions ne réduit `K_max=10` ni le payload contractuel.

Le seuil secondaire `p95 warm_e2e<1 s` et le seuil principal
`p95 warm_e2e<100 ms` restent ouverts. Aucun nombre de ce document n'est une
mesure de `BenchmarkOutputContract-v1`.

## Corrections de l'audit de volume concurrent

Les corrections suivantes sont nécessaires avant toute extrapolation.

- `214 847 238 / 207 216 = 1 036,83` est un nombre de touches ponctuelles
  **cumulées** par sommet à `n=200`, soit `5,18 n`, pas « de l'ordre de `n/2` ».
  Plusieurs requêtes et plusieurs boîtes peuvent retoucher le même `PointId`.
- Les ratios successifs de densité ne sont pas les exposants du volume total.
  Les quatre volumes publiés donnent des exposants locaux environ
  `1,078/1,044/1,025`; ils restent descriptifs d'une famille et d'une graine.
- `ball_node_visits` et `pivot_node_visits` comptent des nœuds LBVH.
  `grid_points_touched` compte des points visités avant déduplication par epoch.
  Les comparer comme une même unité est invalide.
- `full_grid_sweeps` compte les amorces dont la boîte a fini par couvrir la
  grille; `exhaustive_scans` compte les vrais balayages des `n` points. Appeler
  les premiers « balayages complets » est faux sans le second compteur.
- `reverse_live_high_water=364` compte des identifiants shell/intérieur sur un
  chemin de reverse search. Il ne compte ni octets, ni capacités de vecteurs, ni
  scratch d'epochs, ni index, ni sorties. Le chemin `flat_catalogue` matérialise
  par ailleurs `owned_vertices`; le high-water de `reverse_search_stream` ne
  reçoit pas sa mémoire.
- Les `207 216` sommets à `k_nav=9` sont les états visités par cette ordonnance.
  Les `117 534` classés dans une fenêtre fermée et les autres transits ne sont ni
  des `BallActivation`, ni le payload, ni des états dont toute route a besoin.

La mini-rampe locale `12/20/30` et l'extrapolation conditionnelle à cinquante
millions ne possèdent pas de reçu brut pincé. Elles sont retirées du verdict : le
théorème exact ci-dessous tranche plus fortement la question d'architecture.

## Théorème de séparation : arrangement quadratique, Source S linéaire

Pour `m>=11`, prendre les `n=2m` points u16

$$A_i=(1+i,0,0),\quad B_j=(0,1+j,1),\quad 0\leq i,j<m.$$

La construction reste dans u16 pour `m<=65 535`; elle couvre donc `n=50 000`
avec `m=25 000`.

### Volume exact de l'arrangement shallow

Choisir deux abscisses `alpha<beta` sur la droite des `A` et deux ordonnées
`gamma<delta` sur la droite des `B`. Les quatre points sont affinement
indépendants. L'équation de leur sphère, restreinte à chacune des deux droites,
est un polynôme unitaire dont les racines sont ses deux endpoints :

$$\mathrm{pow}(A_x)=(x-\alpha)(x-\beta),\qquad \mathrm{pow}(B_y)=(y-\gamma)(y-\delta).$$

Le shell contient donc exactement les quatre endpoints et le niveau strict vaut
`r+s-2`, où `r=beta-alpha` et `s=delta-gamma`. Deux couples d'intervalles
différents ne peuvent porter la même boule relevée, car un polynôme quadratique
non nul n'a pas trois racines distinctes sur une droite. Les comptes ci-dessous
portent sur les sommets de l'arrangement **relevé R4** `(centre,beta)`; deux
boules distinctes ne sont pas confondues même si leur projection R3 partageait
un centre.

À `k_nav=9`, il faut `r+s<=11`. Le nombre exact de sommets est alors

$$V_{\leq 9}(m)=\sum_{r=1}^{10}\sum_{s=1}^{11-r}(m-r)(m-s)=55m^2-440m+715.$$

Le seul niveau zéro contient déjà `(m-1)^2` sommets. À `m=25 000`, cela donne
`624 950 001` sommets de niveau zéro et `34 364 000 715` sommets jusqu'au niveau
neuf.

### Volume exact de Source S

Une paire de la même droite séparée par `r` pas a exactement `r-1` intérieurs et
aucun label de l'autre droite dans ou sur sa miniboule. Les paires admissibles
ont donc `1<=r<=10`, au nombre total `20m-110`.

Pour la paire croisée `A_iB_j`, le prédicat diamétral restreint aux deux droites
vaut respectivement `x(x-(1+i))` et `y(y-(1+j))`. Elle a exactement `i+j`
intérieurs, aucun extra-shell, et `i+j<=9` donne `55` paires. Ainsi

$$|\mathrm{SourceS}_{q2}|=20m-55.$$

Il n'existe aucun support q3 propre positif. Tout triangle contient deux points
d'une même droite; à l'endpoint le plus proche de l'origine, le produit scalaire
des deux arêtes est strictement négatif, donc l'angle est obtus.

Il n'existe aucun support q4 propre positif. Pour un tétraèdre formé de deux
points de chaque droite, noter `z` la somme des poids barycentriques des deux
points `B`. Son circumcentre a `x=(alpha+beta)/2` et
`y=(gamma+delta)/2`. S'il était dans l'intérieur relatif du tétraèdre, sa
coordonnée `x` imposerait `1-z>(alpha+beta)/(2beta)>1/2`, tandis que sa
coordonnée `y` imposerait `z>(gamma+delta)/(2delta)>1/2`, contradiction. Les
autres quadruplets sont affinement dépendants.

À `m=25 000`, Source S q2--q4 contient donc exactement `499 945` supports; avec
les `50 000` singletons éventuels, `549 945` records minimaux suffisent. Cette
famille est régulière pour q2 : elle n'utilise aucune extra-shell.

### Conséquence algorithmique

Sur toute la plage légale u16, le plein arrangement suit ici un polynôme
quadratique en `n`, alors que la vraie Source S suit une expression linéaire.
Les notations asymptotiques `Theta(n^2)` et `Theta(n)` valent pour l'extension
entière non bornée de la même construction. Un chemin exact peut reconnaître et vérifier la réunion
des deux droites, trier leurs paramètres en `O(n log n)`, émettre les seules
paires à écart borné en `O(nK)`, puis tomber fail-open sur la voie générale si la
structure n'est pas reconnue. Il est exact sur tout nuage et paie `o(V)` sur
cette famille. `V` n'est donc ni une taille de sortie ni un minorant par instance.

Cette famille doit devenir une fixture générative de volume avec trois portes :
formule de niveau, compte fermé de Source S et refus de matérialiser le front.

Même les caps propres ne la rendent pas sparse. Les formules exactes valent
`V_8=45m^2-330m+495` et `V_7=36m^2-240m+330`, soit respectivement
`28 116 750 495` et `22 494 000 330` sommets à `m=25 000`, alors que q3/q4 ne
produisent aucun support. Les caps sont des plafonds de complétude, jamais une
obligation d'énumération.

## Audit du successeur de pinceau courant

Pour un flat triangulaire orienté et un point `y`, la puissance de `y` varie
affinement le long du pinceau. Depuis la sphère courante `v`, le signe de son
temps de croisement est déterminé par `-power_v(y)/lambda(y)`, où `lambda` est
l'orientation affine par rapport au flat. Une fois un incumbent `b` connu, `y`
le précède dans le sens `dir` exactement lorsque
`dir*sign(power_b(y))*sign(lambda(y))>0`.

Le comparateur du snapshot pincé implémente cette identité. Son prune de nœud est
également conservatif : si les puissances de tous les points de la boîte ont le
même signe strict aux deux sphères terminales, aucune fonction affine ne peut
s'annuler entre elles; toute égalité reste ambiguë et descend. Cette formulation
évite le produit croisé du quotient, potentiellement plus large que 128 bits.
Sous l'hypothèse u16, une borne conservatrice de la puissance q4 tient dans
`__int128`; le probe accepte pourtant `--coord` jusqu'à `100000000`, donc son
préflight ne garantit pas l'hypothèse arithmétique annoncée.

Ce résultat ne rend pas la requête logarithmique :

- la structure est une pile DFS; la distance au barycentre entier ne fait
  qu'ordonner les deux enfants pour obtenir tôt un incumbent;
- le pire cas conserve `Theta(n)` nœuds ou feuilles ambigus;
- chaque successeur relance ensuite `collect_shell`, un second parcours LBVH;
- `tie_mass` additionne le shell fermé complet, pas seulement le lot entrant;
- le ledger omet les tests de feuilles et puissances de `collect_shell`, et
  `point_tests` n'inclut pas le nouveau chemin;
- la voie de repli `transition=pivot` conserve le risque `K+1`, sans garde
  terminale dans `record` ni invariant `sum(level[0..K])==cells`.

Les accords globaux sur l'ensemble final des cellules ne reçoivent pas l'ordre
local : un ancien comparateur inversé atteignait le même ensemble et passait ces
portes. La gate nécessaire compare, pour chaque `(cellule,flat ferme,sens)`, le
**premier** croisement et son lot à un oracle exhaustif rationnel. Elle tue un
mutant `flip-pencil-order` et couvre `lambda=0`, ex æquo, transport d'intérieur,
`K+1`, fallback pivot et élagage trop agressif. Le mutant `drop-quotient` est un
mutant de coût qui peut conserver les identités; il doit être tué par un ledger
de flats fermés uniques et de transitions, pas par le seul juge de cellules.

Cette primitive reste utile dans un oracle borné ou comme requête ponctuelle
d'owner q3. Elle ne justifie ni un sweep complet ni un port G4 du front.

## Théorème de propriétaire et plafonds par arité

Soit `U` un support minimal positif d'arité `q`, `B_U` l'intérieur strict de sa
miniboule et `H_U` son flat d'égalité. En dimension affine trois, le polyèdre de
signes dans `H_U` qui conserve `B_U` dedans et les autres labels dehors contient
le centre de la miniboule et possède un sommet `o(U)`. Son shell contient `U` et
son niveau vérifie `ell(o(U))<=|B_U|`.

La conséquence utile n'est pas un plafond commun neuf, mais trois plafonds :

| arité | condition Source S | plafond owner suffisant |
| --- | --- | ---: |
| q2 | `p+2<=11` | `ell<=9` |
| q3 | `p+3<=11` | `ell<=8` |
| q4 | `p+4<=11` | `ell<=7` |

Employer `k_nav=9` pour une lane q4 séparée paie donc deux niveaux sans raison
de complétude. Cette spécialisation ne réduit pas `K_max`; elle retire du travail
de navigation propre à une arité.

Le niveau du support n'est pas celui de son owner. Une unique fixture u16 le
montre simultanément pour q2 et q3 :

```text
A=(15,10,20)  B=(7,14,20)  C=(7,6,20)  D=(10,10,21)
```

La sphère `ABCD` a centre `(10,10,8)`, rayon carré `169`, déterminant affine
non nul et niveau zéro. La paire `AB` a centre `(11,12,20)`, rayon carré `20` et
contient strictement `D` à distance carrée `6`; le triangle aigu `ABC` a centre
`(10,10,20)`, rayon carré `25` et contient strictement `D` à distance carrée
`1`. Le même owner de niveau zéro porte donc des supports q2 et q3 de niveau un.
Cette fixture propriétaire est proposée comme porte permanente.

Le `--fixtures` du probe courant emploie deux autres nuages et n'est raccordé à
aucun CTest. `--harvest` appelle désormais `lift_pair/lift_triangle`, mais :

- il rejette `inside.size()+shell.size()>smax`; ce filtre de rang fermé perd une
  Source S admissible dès qu'une extra-shell vérifie `p+q<=11` mais
  `p+|U_B|>11`;
- sa clé ne contient que les identifiants du support, sans
  `(BallKey,support,I_B,U_B,owner)` ni hypergraphe des supports;
- il ne compare pas sa récolte à une énumération exhaustive q2/q3, malgré le
  commentaire; le juge existant ne compare que les cellules q4;
- les propositions rejetées ne sont pas mémorisées et peuvent repayer leur
  census depuis plusieurs shells;
- la récolte naïve d'un shell de taille `m` tente jusqu'à
  `C(m,2)+C(m,3)+C(m,4)` sous-ensembles.

Le théorème reçoit donc la couverture des **candidats** par un parcours complet,
pas l'émission courante ni sa parcimonie. En dimension affine inférieure à trois,
une lane essentielle séparée reste obligatoire. Yao-1 ferme seulement `k=1`;
Yao48 pour q2 profond reste un candidat non reçu.

## Blueprint support-first, sparse et GPU-friendly

### 1. Séparer les lanes et ne jamais matérialiser les transits

- `k=1` : transcript Yao-1 exact, au plus `48n` arêtes dirigées, RLE puis
  Kruskal/Borůvka sparse avec lots de distances égales.
- q2 profond : cascade Yao48, banques affines et dual-tree résiduel, puis census
  terminal; aucun de ces étages n'est encore reçu comme source complète.
- q3 : candidats directs par range-report/certificats de centre; un pinceau
  owner ponctuel peut certifier exact-once, sans parcourir son arrangement.
- q4 : center-cover de blocs, Jung/Helly et profondeur terminale; seulement le
  résiduel devient microtile exact.

Le flux logique est `candidat support -> décision géométrique/owner ->
GeometricBallKey -> RLE -> strict-count/census -> U_B -> activation -> fold`, jamais
`tous les sommets -> filtrage`. La clé géométrique exacte est calculable depuis
le support avant census; le shell `U_B`, variable et potentiellement linéaire,
reste une identité sémantique aval.

### 2. RLE avant un census exact en deux passes

Le LBVH doit fournir pour chaque nœud les extrema exacts de puissance. Pour les
supports candidats :

1. calculer `GeometricBallKey`, trier/RLE et conserver tous les `SupportKey` de
   chaque run; poser `q_min` égal à leur plus petite arité;
2. avec la convention reçue `power>0` intérieur et `power<0` extérieur, une
   passe de compte strict **par boule** classe `lower>0` comme tout intérieur
   et additionne alors la masse du nœud, tandis que `upper<0` classe tout
   extérieur; elle abandonne dès le `(12-q_min)`-ième intérieur, car aucun
   support du run ne peut alors vérifier `p+q<=11`;
3. pour chaque run survivant, une seconde passe matérialise le census fermé
   `I_B/U_B` complet. Un nœud
   n'est shell en bloc que si ses deux extrema valent exactement zéro; tout
   chevauchement reste ambigu et descend.

Cette séparation évite de remplir `I_B/U_B` pour une boule déjà hors fenêtre
et évite même de répéter le strict-count pour ses différents supports. Le `p`
est commun au run, mais sa pertinence reste évaluée séparément pour chaque
arité; rejeter un support q4 ne tombstone pas un support q3 de la même boule. Elle ne
promet aucune borne sublinéaire : les masses `bulk_inside`, les nœuds ambigus et
les feuilles restent mesurés.

### 3. Quotienter les plateaux sans énumérer leurs cofaces

Après RLE, une boule porte le générateur saturé
`Sat(B)=I_B union U_B`. À l'ordre `k`, il représente implicitement le graphe de
Johnson sur les `k`-sous-ensembles de `Sat(B)`. Les interfaces entre deux
générateurs existent lorsque leur intersection contient au moins `k` labels.

L'implémentation GPU candidate construit des postings inversés
`PointId -> generator_ids`, accumule les tailles d'intersection par tri/RLE de
paires de générateurs et ne matérialise jamais tous les sous-ensembles. Le coût
des postings, notamment la somme des paires induites par les grands degrés,
reste une gate; une facette canonique unique n'est pas un substitut exact.

### 4. Organisation device

- points, nœuds LBVH, candidats et records en structure-of-arrays;
- producteurs persistants par microtuiles, jamais un lancement par support ou
  pivot;
- `count--scan--fill` avec offsets 64 bits vérifiés, segments qui ne coupent ni
  `BallKey` ni lot exact;
- sort radix/RLE des clés, puis agrégation de **tous** les supports minimaux;
- arènes bornées physiquement avec statut `resource_exhausted`, jamais préfixe;
- prédicats exacts spécialisés u16, avec preuve de largeur pour chaque
  formulation et parité CPU/device sur les mêmes identités.

Le ledger sépare au minimum `candidate_supports`, `unique_BallKey`,
`strict_queries`, `closed_queries`, visites nœuds/feuilles, masses bulk,
ambiguïtés, tests exacts, `sum|I_B|`, `sum|U_B|`, postings, paires de générateurs,
activations, événements de forêt, octets et high-water par arène. Le nombre de
sommets de navigation, s'il existe dans un oracle, reste un compteur séparé du
payload.

### 5. Gates avant G4

1. différentiels exhaustifs bornés sur identités
   `(BallKey,support,I_B,U_B,owner)` et fixtures de plateau;
2. compte exact de toutes les tâches prévues/remplies/consommées;
3. rampes `12 500/25 000/50 000` sur les quatre familles, deux pentes par
   compteur dominant et par octets;
4. refus avant CUDA si le produit candidat, les ambiguïtés, les postings ou le
   census deviennent denses;
5. seulement ensuite parité device, Compute Sanitizer, puis mesure du payload
   officiel complet.

## Décision transmise à Claude

Le front de boules est utile comme oracle q4 borné et banc de prédicats; il ne
doit pas être optimisé ou porté tel quel vers G4. Les actions à plus fort levier
sont, dans cet ordre :

1. graver le premier successeur local, les ties, `lambda=0`, le cap et le mutant
   d'ordre;
2. fermer le transcript Yao-1 pour retirer q2 profond de `k=1`;
3. recevoir `GeometricBallKey` et son RLE sur les candidats directs, puis le
   census LBVH deux passes une fois par boule et `U_B` après census;
4. construire q3/q4 support-first avec plafonds `8/7`, en conservant le front
   seulement comme oracle de couverture;
5. recevoir le générateur saturé et son join d'intersections avant tout fold;
6. mesurer le ledger de sortie et d'octets, pas les seuls transits.

Cette décision interdit une qualification G4 du front ou du pipeline produit
tant que ces portes restent ouvertes. Elle ne retire pas le protocole G4
diagnostique distinct de P1a, gardé et explicitement mass-only, après son
différentiel hôte borné.

GCP non utilisé.
