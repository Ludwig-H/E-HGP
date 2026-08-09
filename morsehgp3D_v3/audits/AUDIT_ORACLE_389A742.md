# Audit hostile de l'oracle et de ses CTests — `389a742`

Date : 9 août 2026 UTC.

Phase auditée : M1/M2.2, juge différentiel CPU exact sur entrée `quantized_u16_input`. Backend : CPU. Profil : Release, avec probes de mutation dédiés. Mode : audit seulement ; aucun code de production modifié, aucun Git, aucun GCP, builds et probes sous `/tmp`.

## 0. Ancrage, périmètre et verdict

Le snapshot audité est le commit complet :

`389a7428c88d9dede7a9c767634774b9ea842ca0`

| Fichier | SHA-256 |
|---|---|
| `morsehgp3D_v3/oracle/oracle_main.cpp` | `ed0fe1c1b86a5d0b4dd1a96a6ab00ccd094f0dbd1f3e5abcff83b27029989dbc` |
| `morsehgp3D_v3/oracle/bigint.hpp` | `ce6227b962d39fdc680adb123c3d44a81acf5ee2f8862ba396634a9e4fa00a05` |
| `morsehgp3D_v3/oracle/rational.hpp` | `51e30daeb0f00db2b5ee98c3b1bd212246287c27bbf32244134572f619b0f71e` |
| `morsehgp3D_v3/oracle/bigint_selftest.cpp` | `4ede41cd234c47eb9f8da02ff94763086d4c8d0f7083318ae04874140f3f1727` |
| `morsehgp3D_v3/prototype/edge_shallow.hpp` | `43992a786bbed0c6ff1877f39b828ae8442cf77cf7bb9a1df5306c0f861f91b1` |
| `morsehgp3D_v3/CMakeLists.txt` | `384b940d52b883a98f06657389bc7da8ec5474dcdef4519d7c80e3aa733e0874` |
| `docs/SPECIFICATION_MORSEHGP3D.md` | `dc1ab68087178318eacd736c4d11236fdb0ad5350ac23e413ef5a5abe3f1cae2` |

**Verdict : l'oracle ne peut pas qualifier un chemin à ce commit.** Son noyau rationnel est crédible et plusieurs gardes hostiles sont utiles, mais cinq défauts P0 suffisent à retirer l'autorité : le domaine `RelevantGP` est décidé avec un quantificateur trop fort, le reçu précède encore trois verdicts bloquants, une campagne négative peut être publiée `qualified:true`, le comparateur laisse passer des corruptions de champs publics, et la fixture Q1 nommée `convex_layer_refutation` ne teste pas le résultat mathématique annoncé.

| Priorité | Finding | Reproduction |
|---|---|---|
| P0 | `RelevantGP` sur-rejeté avant le filtre utile | faux rouge `edge_shallow`, puis faux vert `anchored` avec rejet d'un nuage normativement admissible |
| P0 | reçu écrit avant `min-positive`, property-test et verdict final d'injection | processus 1, JSON `exit_code:0`; processus 1, JSON `qualified:true` |
| P0 | une campagne d'injection est qualifiée positivement | injection attrapée, processus 0, JSON `qualified:true` |
| P0 | comparaison publique incomplète | quatre mutations hostiles distinctes passent avec `failures=0` |
| P0 | fixture Q1 non bien centrée et test ne construisant aucun peeling | CTest vert sans le support décisif |
| P1 | domaine CLI et budgets non fermés | `n=4, k=5` qualifié ; `n=64, k=31` accepté malgré une énumération combinatoire irréalisable |
| P1 | compteurs faux ou non finis | histogramme exactement doublé ; `measure-only` retourne 0 avec `-nan` |
| P1 | fixture constante utile mais non assertée | branche observée aujourd'hui, aucune porte ne garantit qu'elle le restera |
| P2 | parser dupliqué et options inapplicables silencieusement acceptées | trois branches mortes ; politique « dernière occurrence gagne » |

## 1. P0 — `RelevantGP` n'est pas la définition de la spécification

### 1.1 Le quantificateur exact

La spécification, lignes 971--979, ne rejette pas toute cosphéricité. Pour un support propre `U`, une coquille extérieure supplémentaire ne viole `RelevantGP` que si tous les points strictement intérieurs peuvent être inclus dans un ensemble utile de cardinal au plus `s_max`.

Si `i(U)` est le nombre global de points strictement intérieurs à la miniball de `U`, la condition utile pour une coquille supplémentaire est `|U| + i(U) <= s_max`. Il ne faut pas employer le rang fermé, car les points supplémentaires de coquille peuvent justement pousser celui-ci au-delà de `s_max` tout en laissant une violation pertinente.

Or `reference_catalogue`, lignes 204--238, incrémente `degenerate_shells` dès qu'un support bien centré possède un point supplémentaire sur la coquille, lignes 215--225. Le rejet arrive avant `members.size() > s_max`, ligne 227, et ne compte jamais séparément les points strictement intérieurs. Le prédicat implémenté est donc strictement plus fort que `RelevantGP`.

Le sujet `anchored` commet le même sur-rejet : `too_many` est calculé lignes 207--218 de `anchored_catalogue.hpp`, mais `degenerate` est incrémenté pour toute coquille bien centrée ligne 230, indépendamment de `too_many`. À l'inverse, les chemins de rang de `edge_shallow` ne font le census de coquille qu'après avoir vérifié que le nombre support plus intérieurs stricts tient dans la fenêtre utile. C'est pourquoi le garde dit « symétrique » aux lignes 1455--1469 peut produire soit un désaccord, soit un accord symétriquement faux.

### 1.2 Faux rouge exact

Commande :

```sh
mhgp3v_oracle --subject edge_shallow --clouds 5 --seed 314159 --min-points 4 --max-points 7 --max-order 3 --coord-max 10 --min-decided 1 --min-nodes 1
```

Résultat : code 1, avec `DOMAINE desaccord : sujet=dans reference=hors trial=4` puis identité non fermée.

Le nuage du trial 4 est :

```text
(4,4,2), (1,5,0), (8,8,4), (5,5,2), (1,10,7), (2,3,6), (7,4,4)
```

L'ordre tiré vaut 2, donc `s_max=3`. La boule diamétrale du support `{4,6}` a pour centre `(4,7,11/2)` et rayon carré `81/4`. Les points `2` et `3` ont respectivement les distances carrées `77/4` et `69/4`, donc sont strictement intérieurs ; le point `5` a la distance carrée `81/4`, donc est sur la coquille. Toute partie sans intrus extérieur strict doit déjà contenir le support et ces deux intérieurs, soit quatre points. Comme `4 > s_max`, cette coquille est hors du quantificateur utile : le nuage reste dans `RelevantGP` pour ce témoin.

### 1.3 Faux vert plus grave : le même nuage est rejeté des deux côtés

Commande :

```sh
mhgp3v_oracle --subject anchored --regime exhaustive --clouds 5 --seed 314159 --min-points 4 --max-points 7 --max-order 3 --coord-max 10 --min-decided 1 --min-nodes 1 --receipt receipt.json
```

Résultat observé :

```text
attempted=5 decided=1 rejected_domain=4
OK : campagne fermee, structure complete comparee sur la grille declaree
```

Le processus rend 0 et le reçu contient `status:"qualified"`, `exit_code:0`, `qualified:true`. Pourtant les deux implémentations ont rejeté le trial 4 admissible ci-dessus. La symétrie n'est donc pas une preuve que le prédicat de domaine est le bon ; elle peut seulement prouver que deux sur-approximations identiques sont d'accord.

Correction conceptuelle attendue : terminer le shell global, compter séparément les intérieurs stricts, puis rejeter une coquille supplémentaire exactement lorsque `support_size + strict_inside <= s_max`. Figer le trial 4 comme fixture positive `RelevantGP=true`, et ajouter la frontière complémentaire avec exactement `s_max - support_size` intérieurs plus un shell extérieur, qui doit être rejetée même si le rang fermé dépasse `s_max`.

## 2. P0 — le reçu n'est toujours pas le verdict final

Le commentaire des lignes 1561--1562 dit que le verdict précède toute sérialisation. En réalité :

- `baseline_passed`, `probe_passed`, `diagnostic_only`, `qualified` et `exit_code` sont figés lignes 1563--1583 ;
- le reçu est écrit lignes 1585--1678 ;
- le seuil `--require-incomplete-anchors` n'est décidé qu'aux lignes 1683--1708 ;
- le seuil `--min-positive-depth` n'est décidé qu'aux lignes 1717--1730 ;
- les validations finales du dictionnaire et de l'injection arrivent lignes 1733--1757.

### 2.1 Faux reçu qualifié avant le plancher de profondeur

Commande minimale reproduite :

```sh
mhgp3v_oracle --subject edge_shallow --clouds 1 --seed 4242 --min-points 4 --max-points 4 --min-order 1 --max-order 1 --min-decided 1 --min-nodes 1 --min-positive-depth 1 --receipt receipt.json
```

Le processus rend 1, car il observe zéro émission de profondeur positive. Le fichier déjà publié contient néanmoins :

```json
"status": "qualified",
"exit_code": 0,
"qualified": true,
"emitted_positive_depth": 0
```

Le reçu ne sérialise même pas `minimum_positive_depth`, donc un lecteur ne peut pas reconstruire la porte violée.

### 2.2 Property-test rouge, reçu à code zéro

Commande :

```sh
mhgp3v_oracle --subject anchored --regime assumed_window --seed-neighbours 16 --clouds 1 --seed 4242 --min-points 22 --max-points 22 --max-order 1 --min-decided 1 --min-nodes 1 --require-incomplete-anchors 23 --receipt receipt.json
```

Le compteur réel vaut 22, donc le processus rend 1. Le reçu contient pourtant `status:"diagnostic_only"`, `exit_code:0`, `probe_passed:true` et `failures:0`. Il publie bien la valeur 22 et le seuil 23, mais ment explicitement sur le code final.

Le cas `--require-incomplete-anchors` avec un sujet autre que `anchored` est encore validé après le reçu : une erreur d'usage qui doit rendre 2 peut donc laisser auparavant un reçu `exit_code:0`.

### 2.3 Une campagne négative devient une qualification positive

`diagnostic_only`, ligne 1581, inclut un property-test ou une fixture, mais pas `!injection.empty()`. Une campagne d'injection réussie stocke ses fautes dans une comptabilité séparée, donc `campaign.failures` reste nul et `probe_passed=true`.

Commande reproduite :

```sh
mhgp3v_oracle --inject member_unsorted --clouds 1 --seed 4242 --min-points 8 --max-points 8 --max-order 1 --min-decided 1 --min-nodes 1 --receipt receipt.json
```

Le processus rend légitimement 0 parce que la faute a été attrapée exactement une fois. Illégitimement, le reçu contient `status:"qualified"` et `qualified:true`. Un test négatif qualifie ici le sujet propre qu'il a préalablement copié, alors qu'il doit porter un statut séparé tel que `negative_test_passed`, jamais une qualification positive.

Correction conceptuelle attendue : calculer un objet `FinalVerdict` unique après toutes les portes sémantiques et avant toute écriture ; inclure injection, fixture et property-test dans les statuts non qualifiants ; sérialiser tous les seuils effectifs ; écrire exactement le code qui sera retourné. Une écriture atomique temporaire puis renommage éviterait aussi de laisser un artefact partiel après crash.

## 3. P0 — le comparateur ne couvre pas le payload public annoncé

Le README affirme comparer le catalogue complet, les deux représentations d'adjacence et tous les compteurs publics. Les comparateurs des lignes 528--887 sont nettement meilleurs que l'ancien oracle, mais l'affirmation reste trop forte.

### 3.1 Probe hostile autonome

Un harness sous `/tmp` a inclus le juge dans une traduction séparée, construit un résultat v2 propre sur six points, vérifié `clean failures=0`, puis appliqué chaque mutation indépendamment. Sortie exacte :

```text
reference_degenerate=0 subject_out=0 spheres=24 forests=2
clean failures=0
nan_beta failures=0
garbage_pool failures=0
reversed_catalogue_order failures=0
root_link_changed=1
root_next_sibling_cycle failures=0
```

Les mutations étaient :

1. remplacer tous les `CriticalSphere::beta` et tous les `ForestNode::beta` par `NaN` ;
2. préfixer et suffixer `Catalogue::members` par des identifiants hors domaine, en décalant les offsets afin de préserver seulement les tranches référencées ;
3. inverser `Catalogue::spheres`, puis reconstruire les forêts cohérentes avec ce nouvel ordre ;
4. poser, sur une racine, `next_sibling` égal à son propre index.

Ces quatre payloads passent avec zéro échec.

### 3.2 Champs effectivement non fermés

| Objet public | Comparaison actuelle | Trou restant |
|---|---|---|
| `CriticalSphere::beta` | aucune | `NaN`, infini ou projection fausse acceptés |
| queue de `support[4]` | seules les `n_support` premières cases sont lues | sentinelles non canoniques et mémoire indéterminée non détectées |
| représentation de `Sphere` | centre et niveau rationnels seulement | mise à l'échelle non canonique du quadruplet numérateur--dénominateur acceptée |
| ordre de `Catalogue::spheres` | comparaison comme map par support | ordre public et indices de sources non canoniques acceptés |
| pool `Catalogue::members` | tranches référencées seulement | trous, préfixe/suffixe, chevauchements et taille globale non canoniques acceptés |
| diagnostics du catalogue | aucune comparaison | `neighbourhood_size`, `growth_rounds`, `certified`, compteurs candidats et diagnostics de shell non fermés |
| `ForestNode::beta` | aucune | projection publique arbitraire acceptée |
| liens de nœuds non atteints depuis `first_child` | bornes seulement | un cycle `next_sibling` sur une racine passe |
| ordre exact des chaînes d'enfants | topologie canonisée par fermeture des minima | ordre de sérialisation non comparé |
| champs de `Result` | lecture sélective dans `main` | `n`, `k_max`, `uncertified_points`, `censored_orders` et provenance ne sont pas confrontés |

Une comparaison sémantique par ensembles peut volontairement ignorer certains détails de représentation. Dans ce cas, le README doit dire « équivalence scientifique partielle ». Si le résultat doit devenir un payload canonique, digestible et publiable, les champs ci-dessus doivent être normalisés puis comparés un à un. `beta` reste une projection d'affichage, mais une projection publique fausse ou `NaN` ne doit pas être blanchie par le fait que les décisions exactes utilisent `source`.

Autre garde interne manquant : `reference_forest` abandonne silencieusement une face ou coface si `exact_miniball` rend faux, lignes 285--295 et 309--317. Une miniball existe pour toute famille finie non vide ; un tel retour signale donc un défaut de l'oracle et doit fermer la campagne, avec un compteur attendu de faces et cofaces, pas réduire silencieusement la référence.

## 4. Fixtures Q1 et constante intérieure

### 4.1 `convex_layer_refutation` ne contient toujours pas le contre-exemple Morse annoncé

La fixture des lignes 1341--1369 conserve le support `(p,q,x1,x3)` de l'ancien exemple. Son centre est `(19/2,19/2,15)`, son rayon carré `51/2`, mais ses barycentriques dans cet ordre valent `(1/10,-1/10,1/2,1/2)`. La coordonnée de `q` est négative : le support n'est pas bien centré et n'appartient pas au catalogue critique de référence.

Les commentaires numériques sont également incohérents avec les points commis :

- les droites de `x1` et `x3` se coupent en `(-1/5,1/5)`, pas en `(-61/20,61/20)` ;
- après le déplacement de `x4.z` à 17, les quatre formes ont les constantes `(4,4,4,16)`, elles n'ont plus de constante commune ;
- le déplacement de `x4` ne change aucune barycentrique du support cible.

Le CTest lignes 147--153 de `CMakeLists.txt` n'implémente par ailleurs aucun peeling convexe. Il exécute seulement `edge_shallow` contre le catalogue exhaustif et n'assert ni le support cible, ni sa profondeur, ni son appartenance à une diagonale absente des couches. Il passe actuellement avec six autres émissions d'arité quatre ; le support prétendument décisif peut être absent sans faire rougir le test.

La vraie fixture bien centrée déjà documentée dans `audits/REPONSE_README_PREFIXE_SHALLOW.md` doit remplacer celle-ci, avec assertions littérales sur support, membres, rang, centre, barycentriques, profondeur, diamètre, absence de shell supplémentaire et résultat explicite du peeling testé.

### 4.2 `constant_inside_witness` est mathématiquement utile, mais la porte est vacue

Pour le support formé des quatre premiers points de la fixture, le calcul indépendant donne centre `(105,298/3,805/8)`, rayon carré `14881/576` et barycentriques `(53/192,251/576,2/9,19/288)`, toutes strictement positives. Le point `(101,100,100)` a un résidu exact de distance carrée égal à `-9` : il est strictement intérieur. Pour l'ancre des deux premiers points, sa forme est bien algébriquement constante et intérieure. Le run courant observe `emitted_positive_constant=2` et `dictionary_refuted=0`.

C'est donc une bonne donnée de régression. Mais le CTest lignes 155--159 ne demande aucun minimum de constante intérieure et ne vérifie aucun record précis. Une régression où la branche `c_e` retombe à zéro peut rester verte si le même catalogue est récupéré par une autre ancre ou un autre chemin. Il faut une assertion dédiée : `c_e=1`, profondeur cible nulle, support `{0,1,2,3}`, rang 5, membres `{0,1,2,3,4}`, et au moins une émission positive-constante attribuée à cette branche.

Les deux fixtures écrasent en outre `n` et `order` après la validation CLI, mais ne possèdent pas les contraintes explicites des fixtures de sources de fusion. Avec les valeurs par défaut, le CTest Q1 exécute réellement `n=6, order=4` alors que son éventuel reçu annoncerait `points:[8,11]` et `maximum_order:3`. Les paramètres de fixture doivent être fixés avant validation et reflétés tels quels dans le reçu.

## 5. Parser, domaine sémantique et budgets

### 5.1 Parser dupliqué

Les branches `--fixture`, `--min-order` et `--min-positive-depth` apparaissent une première fois lignes 1087--1093 puis une seconde fois lignes 1099--1105. Les secondes sont mortes à cause de la chaîne `else if`. Les clés JSON dupliquées signalées dans un audit antérieur sont corrigées à ce commit et `python3 -m json.tool` accepte les reçus produits ; le doublon restant est bien dans le parser C++.

Le parser accepte aussi plusieurs occurrences d'une option et conserve silencieusement la dernière. Il accepte des options hors sujet : `--min-positive-depth` avec `v2`, `--regime` avec `v2` ou `edge_shallow`, `--points` hors `measure-only`, et `--seed-neighbours` lorsque le sujet ne l'utilise pas. Pour un outil de qualification, ces combinaisons doivent être rejetées plutôt que sérialisées comme si elles avaient participé au verdict.

`minimum_positive_depth` peut être négatif. `require_incomplete_anchors` accepte toute valeur inférieure à `-1` comme si l'option était absente, et la valeur zéro crée un property-test vacuement vert. Ces seuils doivent avoir des domaines explicites strictement positifs lorsqu'ils sont demandés.

### 5.2 `k` peut dépasser `n`

Il n'existe aucun garde `maximum_order < minimum_points` ni contrôle par trial. La commande suivante est acceptée :

```sh
mhgp3v_oracle --subject v2 --clouds 1 --seed 2 --min-points 4 --max-points 4 --min-order 5 --max-order 5 --min-decided 1 --min-nodes 1
```

Elle rend 0 et annonce `forets=5`, donc qualifie notamment un ordre 5 sur quatre points. Le domaine HGP utile exige au minimum `1 <= k < n`; la génération aléatoire doit choisir un ordre admissible pour chaque cardinal, ou refuser la plage entière.

### 5.3 Les plafonds numériques ne bornent pas le travail

Le jugement accepte jusqu'à `n=64` et `k=31`. `reference_forest` matérialise pourtant tous les `k`-sous-ensembles et tous les `(k+1)`-sous-ensembles. À ces bornes, les vecteurs auraient une taille combinatoire irréalisable, sans préflight de binomiale, budget mémoire, budget d'opérations ni timeout. Le second garde `maximum_points > 4096`, lignes 1143--1147, est mort puisque `maximum_points > 64` a déjà été rejeté lignes 1116--1121. Le domaine exhaustif normatif du dépôt est borné à `n<=14`, avec `k<=10` dans les jalons correspondants.

Le mode mesure accepte un million de points. `edge_shallow` commence par toutes les paires et peut examiner un nombre de sommets d'ordre `n^4`; `anchored` exhaustif peut effectuer un nombre de tests témoins d'ordre `n^5`. À `n=1 000 000`, ces quantités dépassent largement `LLONG_MAX`, alors que tous les compteurs sont des `long long` signés. L'interface accepte donc des exécutions qui sont à la fois irréalisables et susceptibles d'overflow signé si elles progressaient assez loin.

Correction conceptuelle attendue : préflight en arithmétique large des binomiales, incidences, tests témoins, sommets, octets et durées ; caps cohérents avec le domaine documenté ; refus avant allocation ou géométrie ; compteurs non signés larges ou saturation explicite avec retrait d'autorité.

## 6. Compteurs et couverture CTest

### 6.1 Histogramme exactement doublé

`EdgeShallowStatistics::absorb` additionne déjà `rank_histogram` aux lignes 83--105 de `edge_shallow.hpp`. Après `edge_shallow_total.absorb(shallow)`, `oracle_main.cpp` le somme une seconde fois lignes 1426--1428.

La fixture Q1 imprime 40 émissions non singleton (`15+19+6`) mais un histogramme de somme 80. La fixture constante imprime 15 émissions (`10+4+1`) mais un histogramme de somme 30. Le reçu porte donc une mesure fausse par construction.

Par ailleurs, les compteurs `emitted_*`, `emitted_positive_depth` et `emitted_positive_constant` sont incrémentés avant la déduplication globale des supports. Ils comptent des incidences ancre--support, pas nécessairement des sphères uniques du catalogue final. Le plancher de profondeur CTest doit nommer cette unité et, si l'objectif est la diversité de couverture, publier aussi les événements uniques par arité et par rang.

### 6.2 `measure-only` publie un nombre non fini avec succès

Commande :

```sh
mhgp3v_oracle --subject edge_shallow --measure-only --points 2 --clouds 1 --max-order 1
```

Résultat : code 0 avec `-nan par m_e^2`. Les divisions lignes 1239--1251 ne protègent pas `lines_per_edge == 0`, ni même `edges_examined == 0` en présence de doublons. Un reçu ou stdout de mesure ne doit publier que des nombres finis, ou une valeur absente avec cause explicite.

Le résumé `anchored` a aussi un dénominateur faux lorsqu'il existe des rejets de domaine : les moyennes de tous les trials sont accumulées lignes 1439--1450, puis divisées par `campaign.decided` lignes 1532--1541. La campagne `RelevantGP` ci-dessus imprime ainsi `fenetre moyenne=26.0 max=6`, impossibilité arithmétique qui révèle le mélange entre cinq trials accumulés et un seul trial décidé.

### 6.3 Ce que les CTests prouvent réellement

`ctest -N` déclare 25 tests au total, dont 21 propres à la v3 et quatre hérités de la v2. Sur le snapshot figé, les huit tests d'injection passent en 9,72 secondes et les six tests ciblés domaine/fixtures/CLI/reçu passent en 0,14 seconde. Cela confirme que les gardes ciblés fonctionnent ; cela ne ferme pas les trous ci-dessus.

Manquent au minimum :

- une fixture `RelevantGP` positive au-delà du budget utile et une fixture négative exactement à la frontière utile ;
- un parseur de reçu qui compare le JSON au code final après chaque porte tardive ;
- le statut non qualifiant obligatoire de toute injection ;
- les mutations `beta=NaN`, pool non canonique, ordre de catalogue, queue de support et liens inutilisés ;
- une fixture Q1 qui exécute réellement le peeling réfuté et exige le support Morse correct ;
- une assertion exacte du terme constant intérieur ;
- `k=n-1` accepté et `k>=n` rejeté ;
- seuils négatifs, options dupliquées et options hors sujet ;
- mesures zéro sans `NaN` ;
- préflights de travail et tests de refus aux caps.

## 7. Arithmétique exacte : résultat plus favorable, mais portée à préciser

Je n'ai pas trouvé d'overflow dans `BigInt`, `Rational`, l'élimination de Gauss ou la lecture multiprécision des niveaux du sujet. Les conversions de l'entier signé minimal vers une magnitude non signée sont écrites sans négation signée, les dénominateurs nuls avortent, et les comparaisons rationnelles emploient des produits multiprécision.

Le selftest compare bien `BigInt` à `__int128` puis à GMP jusqu'à plusieurs milliers de bits lorsque GMP est disponible, et échoue fermé sans GMP hors `--dev`. En revanche, les identités de `Rational` sont auto-cohérentes mais ne sont pas différentielles contre `mpq_class`; `gauss_solve`, `sphere_through` et la forêt de référence n'ont pas de second oracle indépendant permanent. C'est une limitation de confiance, pas une réfutation observée de l'arithmétique.

Le champ `widest_exact_level_bits` mesure seulement les rationnels finaux lus et comparés, pas la largeur maximale de tous les intermédiaires de Gauss ou de comparaison. Son nom actuel est acceptable ; il ne doit pas être présenté comme un plafond mémoire ou arithmétique global.

## 8. Ordre de correction recommandé

1. Aligner exactement `RelevantGP` sur `support_size + strict_inside <= s_max`, puis figer les deux côtés de la frontière et retirer toute qualification produite avec l'ancien prédicat.
2. Construire une seule décision finale, exécuter toutes les portes avant le reçu, et interdire `qualified:true` pour fixtures, property-tests, injections et mesures.
3. Décider explicitement si l'oracle compare une équivalence scientifique ou un payload canonique. Dans le second cas, fermer tous les champs publics et ajouter les mutations hostiles du probe.
4. Remplacer la fixture Q1 et tester réellement le peeling ; renforcer la fixture constante par des assertions littérales.
5. Fermer le domaine CLI `1 <= k < n`, supprimer les branches dupliquées, rejeter les options inapplicables et ajouter des budgets combinatoires préflightés.
6. Corriger l'agrégation des statistiques, distinguer incidences et événements uniques, interdire les nombres non finis et ajouter une porte positive-constante.
7. Ajouter les digests des nuages, le commit et les paramètres effectifs au reçu ; la graine, le compilateur et l'heure de build ne suffisent pas à reconstruire une campagne portable.

## 9. Traçabilité opérationnelle

- Snapshot source audité : commit `389a7428c88d9dede7a9c767634774b9ea842ca0`.
- Binaire Release figé : `/tmp/mhgp3v-389a742.Kk3tmh/build/mhgp3v_oracle`.
- Harness comparateur et sorties : `/tmp/oracle_389_comparator_probe*` et `/tmp/oracle_389_*.out`.
- Dépôt : seul ce rapport Markdown a été créé.
- Git : aucune mutation, aucune branche, aucun commit.
- GCP : non utilisé.
