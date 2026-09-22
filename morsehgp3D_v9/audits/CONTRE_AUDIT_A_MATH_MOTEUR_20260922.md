# Contre-audit A — mathématiques et moteur de l'ouverture v9

22 septembre 2026. Ouverture jugée : `3595725ae3352ab67dced716e33e8f7705372a2c` ; code v8 publié à `a74e90f22167105cdba90b6850f0597f1a01a329` (identique à `12294241` pour `morsehgp3D_v8/src/`). Lecture seule du code, calculs entiers indépendants en Python ; **aucun CTest, sanitizer, TSan, benchmark ni GCP exécuté par cet audit**. Les modifications de reprise u18, alors non commises, ont depuis été publiées à `3f0d188f` ; leurs reçus R2 restent en échec. Ce rapport répond au Lot 1 de la [question du développeur](QUESTION_CLAUDE_CONTRE_AUDIT_OUVERTURE_20260922.md) et se limite aux points dont la correction peut être jugée.

## 1. P0 — le raccord FULL ne peut pas garder ses gardes u16

Le [plan v9](../docs/PLAN_V9.md) a raison de viser la sémantique FULL, mais son port doit rendre exact **le catalogue d'entrée de la tour**, pas seulement substituer des candidats v8 au tableau de boules v7. `full_ball_tower.hpp:463–508` (`dc57ffd5`) exige une boule unique par clé, sa coquille complète, son intérieur, le niveau exact, puis calcule `q_min` sur la coquille et exige `q == ball.arity`. Une première présentation q4 choisie pour une clé ne prouve donc ni `q_min = 4` ni toutes les incidences de support nécessaires au quotient local. Le catalogue peut stocker **une boule** plutôt que toutes les présentations émises si une procédure exacte reconstitue ou certifie les supports pertinents depuis sa coquille ; cette procédure et son coût sont une obligation distincte. Une clé avec plusieurs supports doit être testée avant le fold, avec dédoublonnage par clé et non par `(clé, support)`.

Le port direct de la garde `full_ball_tower.hpp:477–480` rejetterait des clés u18 valides : `A < 2^68`, `|B_i| < 2^87`, `|C| < 2^105` sont les limites u16. Les majorants déjà prouvés pour le constructeur q3 v8 à `exact_ball.cpp:98–100` donnent à M=262143 : `A≤12M^4<2^76`, `|B_i|≤60M^5<2^96`, `|C|≤144M^6<2^116`, et la puissance globale reste `<2^117`. Ils tiennent en i128, mais **la garde v7 doit changer et être rejugée avant tout appel de `BallKey::power`**. `kBallShellMax=12` dans `morsehgp3D_v7/src/pipeline/expand.hpp:46–55` et le refus dans `full_ball_tower.hpp:465–467` ne sont pas des théorèmes de la grille 1 mm. Par exemple la sphère entière de rayon 5 centrée en `(5,5,5)` possède déjà plus de 12 sites de coquille distincts dans `[0,10]^3` (six points axiaux et les permutations de `(±3,±4,0)` translatées). Un refus explicite est un statut honnête d'étape ; il ne qualifie pas une tour entière pour toutes les entrées du contrat.

Le comparateur de niveaux v7 n'est **pas** immédiatement condamné par u18 : `morsehgp3D_v7/src/lanes/level.hpp:36–54` stocke numérateur U192, dénominateur i128 et compare en U320. Recalcul conservateur depuis `q3.hpp:75–84` : `D,E,X≤3M²`, `0<G≤9M⁴`, donc numérateur q3 `≤27M⁶<2^113`, dénominateur `≤36M⁴<2^78`, produit croisé q3/q3 `<972M^10<2^190` (U192 tient, avec moins de marge que sous u16). Pour le **code v7** `q4.hpp:50–67,148–155`, chaque entrée de matrice vaut au plus `2M`, chaque cofacteur au plus `8M²`, donc `|det|≤48M³<2^60`, `|N'_i|≤72M⁴<2^79`, `|N'|²≤15552M⁸<2^158`, `det²≤2304M⁶<2^120`, produit croisé q4/q4 `<15552·2304M^14<2^278` (U320 tient). Ce sont des **bornes sur les formules v7**, non une qualification du port, des conversions ou du GPU ; les portes de comparateur à 18 bits et les préconditions des multiplications larges restent obligatoires. Elles lèvent toutefois l'incertitude sur la largeur nominale du niveau avant optimisation.

La [synthèse](../docs/AUDIT_V8_SYNTHESE.md) §7 et l'[héritage](../docs/HERITAGE_V7_V8.md) §1 doivent annoncer l'autorité avec sa portée exacte : le registre `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md:120–122,131–134` donne un **théorème conditionnel horizontal** sous prémisses régulières et une fenêtre d'inertie qui ne prouve ni toutes les verticales ni l'objet public. Une grille 1 mm peut avoir plateaux et extra-shells de rang pertinent. Le statut `unsupported_rank_relevant_extra_shell_degeneracy` et un quotient de plateau prouvé sont donc une porte de domaine, pas une simple anomalie statistique. Le taux de doublons q4 ne mesure pas à lui seul cette porte : trois sites en triangle rectangle sur la grille ont une extra-shell de la boule diamétrale sans aucun record q4 possible.

**Action V9-1.** Graver ensemble (i) une même clé émise par plusieurs présentations et `q_min` recalculé de sa coquille ; (ii) une coquille entière de plus de 12 points ; (iii) un triangle rectangle et le carré cosphérique à rang pertinent ; (iv) une clé q3 et deux niveaux q3 près des majorants u18, comparés à un oracle entier plus large. Déclarer séparément `exact_full_regular`, `exact_full_quotient_certified` et le refus de domaine ; ne pas convertir une absence de doublons q4 en certificat de régularité.

### Complément au pin v8 `3f0d188f` : borne q4 fermée et `q_min` sans table exponentielle

La réserve « borne q4 à établir » du plan v9 peut être fermée **pour la
garde de `BallKey::power`**. Le constructeur q4
`exact_ball.cpp:145–152` prouve `A≤6M³`, `|B_i|≤30M⁴`,
`|C|≤72M⁵` ; le constructeur q2 est plus petit. Ils satisfont donc tous
les trois majorants q3 `12M⁴`, `60M⁵`, `144M⁶` ci-dessus. Même pour une
clé publique forgée mais admise par les gardes proposées
`A<2^76`, `|B_i|<2^96`, `|C|<2^116`, et un point u18, le calcul naïf de
puissance est borné par
`3·2^76·M² + 3·2^96·M + 2^116 < 2^117` : i128 suffit.
Cela ne qualifie ni le quotient de plateau, ni toutes les autres formules
FULL, mais retire une incertitude numérique du port des clés q2/q3/q4.

La [proposition B pour les grandes coquilles](PLATEAUX_GRANDES_COQUILLES_B_20260922.md)
évite la table `2^u` de `ShellTable`, mais ses plans et son `q_min=3`
demandent des largeurs exactes. Pour une clé primitive positive issue du
constructeur u18, poser `w_i=2A p_i+B=2A(p_i−c)`. Alors
`|w_{i,r}|≤84M⁵<2^97`. Le normal d'un plan passant par le centre et deux
sites se calcule sous la forme **factorisée**
`N_ij=w_i×(p_j−p_i)` ; `w_i×w_j=2A N_ij` et
`|N_{ij,r}|≤168M⁶<2^116`. Il tient en i128, contrairement au produit
direct `w_i×w_j` dont les produits intermédiaires n'y tiennent pas
nécessairement. Après détection des antipodes (`q_min=2`), chaque paire
définit un normal non nul. Grouper ses normales primitives à signe
canonique regroupe exactement les sites des mêmes plans par le centre,
en `O(u² log u)` temps et `O(u²)` enregistrements pour `u=|U|`.
Dans chaque groupe de trois sites ou plus, projeter sur les deux axes
complémentaires d'une composante non nulle du normal, puis trier les
directions circulairement. L'origine appartient à l'enveloppe convexe du
groupe si et seulement si **chaque écart consécutif est strictement
inférieur à π** ; sans antipodes, les signes stricts des déterminants
projetés, dans une orientation circulaire cohérente, suffisent. Leur
signe se calcule avec la composante correspondante de `N_ij`, toujours
en i128. Une telle composante donne `q_min=3` ; en son absence, la
présentation positive déjà certifiée et Carathéodory donnent `q_min=4`.
La somme des tailles des groupes est `O(u²)` car chaque groupe de `r`
sites correspond à `r(r−1)/2` paires distinctes.

Un test direct de coplanarité `N_ij·(p_k−p_i)` a seulement le majorant
`504M⁷<2^135` : l'exécuter en i192, ou l'éviter par le regroupement,
mais ne pas le glisser en i128 sur la seule base du code u18 actuel.
Cette preuve **ne borne pas l'ordonnancement des intersections de grands
cercles** du quotient B. Gates : paire antipodale, triangle sur la sphère
de rayon 5 translatée en `(5,5,5)` avec directions
`(5,0,0),(-3,4,0),(-3,-4,0)` et un site hors plan, puis tétraèdre
`(±1,±1,±1)` de signes pairs translaté ; comparer `q_min` au
`ShellTable` v7 pour `u≤12`, permutations et normales opposées comprises.

Un [oracle indépendant](check_qmin_planes_u18_20260922.py) contrôle cette
factorisation contre l'énumération exacte des triangles à coefficients
`Fraction`. `python3 -B morsehgp3D_v9/audits/check_qmin_planes_u18_20260922.py`
et la même commande avec `-O` rendent `PASS` : quatre fixtures, dont un
centre demi-entier et un tétraèdre `q_min=4`, puis 755 sous-coquilles sans
antipodes d'une sphère entière à 30 sites. Il teste aussi les largeurs u18
ci-dessus. C'est un gate local reproductible, pas un test du quotient FULL
ni une qualification des grandes coquilles du moteur.

## 2. P0 positif — le rejet q3 par l'atlas est correct, avec une prémisse à expliciter

Le [certificat publié](../../morsehgp3D_v8/docs/Q3_CERTIFICAT_ATLAS_20260921.md) (`0948d2d0`) et `q4_local_partition.cpp:303–361` établissent sur **chaque cellule fermée** C un compte de sites certifiés strictement intérieurs à toutes les boules de centre C : borne supérieure de la forme `<0`, nœuds spatiaux disjoints, compte transmis aux enfants ; les égalités restent dans la frontière. Les budgets n'effacent aucun bloc ambigu. `q4_local.cpp:228–249` renvoie ce minorant pour une cellule contenant le centre, et **aucun certificat** pour `Outside` ou hors racine. `wspd_q34.cpp:535–559` ne rejette la graine q3 qu'à `inside_count≥K−1`, seuil exact de cette voie. L'ancien arrêt profond q4 à `K−2` ne suffit pas à rejeter q3 ; le code publié ne fait pas cette substitution. La reprise [désormais publiée](../../morsehgp3D_v8/docs/REPRISE_U18_ET_ATLAS_SATURANT_20260922.md) choisit un certificat terminal distinct au seuil `K−1`, option désactivée par défaut. Ne jamais publier son préfixe comme fragment complet ni additionner `saturation_work.prefixes` à `work.partition.prefixes` : c'est un sous-ensemble.

Il manque dans la note une ligne de preuve pour `root_lane_skips` (`wspd_q34.cpp:513–516`) : pourquoi le compte de la racine s'applique-t-il **à toutes** les graines q3 propriétaires ? Soit `l=|b−a|`, `m=(a+b)/2`, `h=max_i|b_i−a_i|`. Pour une graine aiguë dont `ab` est l'arête maximale, son circumrayon vérifie `R²≤l²/3`, donc `|c−m|²=R²−l²/4≤l²/12`. Les vecteurs de base du code (`q4_local_partition.cpp:52–65`) ont une matrice de Gram de plus petite valeur propre `h²`, avec `h²≥l²/3`. Dans la convention du code `c=m+(u_1A+u_2B)/2`, il suit `|u|²≤4|c−m|²/h²≤1`. La racine `[-2,2]^2` contient donc tous ces centres, même aux frontières. Le même calcul donne pour tout z strictement intérieur `|z−m|<R+|c−m|≤√3l/2<l` : la couverture fermée `edge_cover.cpp:38–45` contient tous les intrus à compter. Ces deux arguments ferment le relais q3 sans recours à Voronoï ou Delaunay d'ordre supérieur.

Le certificat est **un filtre local exact**, pas une borne de croissance : nombre de cellules, somme des frontières actives, copies d'IDs et census résiduels restent à mesurer par arête et par trame. L'opération proposée dans V9-2 « utiliser le census q3 dans les fragments exacts » doit conserver un fragment complet et son propriétaire immuable ; un certificat profond seul ne donne ni les IDs intérieurs exacts ni la coquille.

**Porte manquante au pin `a74e90f2`.** Aucun test ciblé ne juge directement `q3_center` et `certified_inside_count` avec centre sur frontière fermée, site de puissance zéro et cellule `Outside` ; les tests de flux bornés et le mutant de seuil sont utiles mais ne remplacent pas ce juge. La reprise `3f0d188f` documente un premier juge dont x/y étaient inversés, puis une correction : ne l'hériter qu'une fois son reçu qualifié et épinglé.

## 3. Bornes u18 : code arithmétique mieux établi que sa note, mais deux commentaires restent faux

La [note d'élargissement](../../morsehgp3D_v8/docs/ELARGISSEMENT_18_BITS_20260922.md) est contradictoire avec son annexe B et avec le code. Un contrôle Python entier de **42 énoncés simples** `c·M^d < 2^b` dans les commentaires de `morsehgp3D_v8/src/` à `a74e90f2`, M=262143, en trouve 42 vrais et zéro faux. En particulier `360M^6<2^117`, `512M^6<2^117`, `96M²·2^40<2^83` tiennent. La division longue `scaled_floor` (`q4_local.cpp:72–105`) normalise d'abord le reste dans `[0,den)`, puis effectue 20 doublements ; `den<2^117` assure `2r<2^118`, et la garde du quotient avant boucle assure `<2^121` après 20 pas. La comparaison d'une cellule fermée par `(floor, reste nul)` respecte également la frontière ; aucun `Q·numérateur` 137 bits n'est formé. Les demandes de fabrique publique hors `[0,M]` restaient toutefois sans garde au pin ; la reprise `3f0d188f` les protège, sans étendre le domaine certifié.

Huit assertions atomiques fausses de la note publiée sont directement vérifiables : stockage annoncé `uint32_t` au § « Décisions », alors que `types.hpp:20` est `int32_t` ; `144M^6<2^115` (la valeur occupe 116 bits) ; changement annoncé de Local28 à `Q=2^18` et `max_depth=18`, alors que `q4_local_partition.hpp:21–23` garde `2^20` et 20 ; bornes `|p|<2^73`, `|q|<2^73`, `|det|<2^113` du centre q3 ; et `96M²·(2^18)²<2^76` (la valeur occupe 79 bits). Pour les trois bornes du centre, la graine équilatérale entière `a=(0,0,0)`, `b=(0,M,M)`, `x=(M,0,M)` est aiguë, avec arête `ab` propriétaire par départage d'IDs 0,1,2 ; les formules publiées donnent des numérateurs de **112 et 113 bits** et un dénominateur de **114 bits**. Le code utilise les majorants corrigés `32M^4<2^77`, `512M^6<2^117`, `480M^6<2^117` (`q4_local_partition.cpp:118–138`). La borne `Q·|x|<2^131` du projet initial n'est pas déduite de ses propres majorants ; le code évite justement le produit.

Deux commentaires **du code** restent périmés : `q4_local_partition.cpp:64` dit `Gram<2^33`, alors que `2M²` occupe 37 bits (`:127` donne la bonne borne `<2^37`) ; `q3_ball_census.hpp:54` dit puissance `<2^105`, alors que la voie u18 prouve `<2^117` dans `exact_ball.cpp:100` et `q3_ball_census.cpp:54`. Cette dernière affirmation est fausse même **après réduction primitive** : la boule q3 de `a=(0,0,0)`, `b=(91261,M,0)`, `x=(168099,116308,M)` a un pgcd global égal à 1, et sa puissance en `z=(M,M,M)` occupe 108 bits. Ils ne changent pas l'exécution, mais le port v9 doit prendre les preuves corrigées et générer les `static_assert` à partir de `coordinate_bits` comme le prévoit V9-0.

## 4. Autres commits du Lot 1 : conservation vérifiée statiquement, qualification non héritée

- `748ec082` abaisse l'échelle des cellules à `2^20` ; à M=262143, les bornes de forme et de blocs réécrites dans `q4_local_partition.cpp:107–145,240–270` restent sous i64 (`<2^62`) et le seul carré intermédiaire est promu en i128. `02987f18` passe les fragments à `make_shared` et réserve la frontière : bon gain d'allocations possible, **aucun gain de temps démontré** sur la seule mesure annoncée. Au pin `a74e90f2`, `Q4LocalFragment::Key` a un constructeur public (`q4_local_partition.hpp:158`) et ne verrouille donc pas réellement le constructeur par passkey ; la reprise `3f0d188f` le rend privé. Porter cette correction, pas l'ancien commentaire de sécurité.
- `5224ff4e` : la boucle `rectangle()` (`wspd_q34.cpp:395–446`) partitionne les rangs A en plages disjointes ; chaque plage est soit publiée, soit développée sur place si la file refuse. La file sous mutex (`:738–856`) n'annonce un résultat réussi que si tous les jobs et toutes les plages publiées sont consommés. L'annulation ne renvoie pas de registre de succès partiel. C'est une justification statique de **conservation des plages**, pas une preuve de débit ni une qualification TSan. L'arête reste entière dans une plage : ce mécanisme ne découpe pas encore une arête coûteuse en tâches de centres/graines indépendantes.
- `5fdda963` sépare correctement temps mur, CPU de thread et attente sur condition variable ; ces durées ne sont pas des compteurs logiques à comparer en égalité. `clock_gettime` échoue cependant vers zéro silencieux (`wspd_q34.cpp:727–735`) : avant d'en faire une autorité de profil G4, publier un bit `cpu_clock_valid` ou échouer explicitement. Aucun TSan ni essai GPU de ces commits n'a été rejoué ici.

Le choix d'objet est donc constructif : **k-Gabriel et miniball locales**, avec certificats et propriétaires par arête/cellule, puis catalogue canonique et quotient FULL. Il n'oblige pas à construire une mosaïque de Delaunay/Voronoï d'ordre K. Le coût total reste la somme du front, des cellules, des frontières actives, des supports émis, de la canonisation et du fold ; une preuve sous-quadratique pour le régime LiDAR devra porter sur ces objets et leurs sorties, pas sur un seul filtre.

## Reproduction et provenance

Rejeu hermétique des hashes source, des 42 commentaires simples, des 16 majorants explicites et de la graine équilatérale, sans dépendance Python externe ni lecture du code de travail mutable :

```sh
python3 -B morsehgp3D_v9/audits/check_u18_bounds_20260922.py
python3 -B -O morsehgp3D_v9/audits/check_u18_bounds_20260922.py
```

Les deux exécutions rendent le même JSON : `{"explicit_u18_bounds":16,"q3_center_fixture_bits":[112,113,114],"simple_comment_bounds":42,"source_hashes":12,"status":"PASS","v7_pin":"dc57ffd5","v8_pin":"a74e90f2"}`. Le script sort avec code non nul dès qu'un hash, une borne ou un décompte change ; le mode `-O` ne désactive aucun contrôle.

Empreintes SHA-256 de `git show a74e90f2:<chemin>` : `src/lanes/q4_local.cpp` `ebe0087c79d8`, `src/lanes/q4_local_partition.cpp` `210caaa63bd2`, `src/lanes/q4_local_partition.hpp` `6835f7712363`, `src/pipeline/wspd_q34.cpp` `79ae04fe5056`, `src/core/types.hpp` `dbe746853b3a`. Le constructeur FULL lu à `dc57ffd5` (`src/forest/full_ball_tower.hpp`) a pour SHA-256 `83f1c78e0656`. Le texte u18 publié à `a74e90f2` a pour SHA-256 `4b783c2bf839` ; il peut être corrigé sans changer ces observations sur le code gelé.
