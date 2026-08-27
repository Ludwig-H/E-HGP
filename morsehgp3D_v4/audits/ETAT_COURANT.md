# État courant — audit complet de `morsehgp3D_v4`

Date : 22 août 2026.
Auteur : auditeur indépendant.
Pin fonctionnel audité : `f2533b4e2c79d381381f9e6c0e7d9bb16310548b`.
Branche : `main`.
Cadre déclaré : `phase=exploration_v4_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `public_status=not_claimed`.
GCP : non utilisé ; aucune ressource distante n'a été créée, démarrée ou modifiée.

## Verdict exécutif

`morsehgp3D_v4` est aujourd'hui une **référence CPU exacte et crédible sur un domaine borné**, avec une qualité de tests inhabituellement bonne pour un chantier de recherche. Ce n'est toutefois **ni une source HGP complète au sens du dépôt, ni une bibliothèque intégrable, ni un backend produit qualifié**. Le statut `public_status=not_claimed` est exact et doit être conservé.

La conclusion n'est pas « jeter v4 ». Ses prédicats q2/q3/q4, ses comparateurs de niveaux, sa gestion des macro-lots, son oracle de forêt et plusieurs de ses filtres fail-open constituent un excellent socle d'oracle et de fixtures pour la source active. La conclusion est plus précise : **promouvoir v4 comme backend dense de production serait contraire aux objectifs actuels du dépôt ; la promouvoir comme oracle CPU borné serait cohérent et utile.**

Deux blocages sont immédiats :

1. le commit `f2533b4e` annonce un plafond du « pic de résidence projeté », mais la formule oublie que les résultats des folds terminés restent résidents ; la garde peut donc accepter un calcul dont le seul état final connu dépasse déjà le pic annoncé ;
2. le contrat local « forêt complète K=1..10 en moins de 100 ms, dizaines de millions de points » reste formulé comme une trace exhaustive résidente, alors qu'une borne q2 déjà reçue impose à 30 millions de points au moins 158,4 Go de seuls `PointId` pour les facettes nées, avant q3, q4, niveaux, arbres, verticales et surcoûts de conteneurs.

### Décision de promotion

| Objet évalué | Verdict | Motif principal |
|---|---|---|
| prédicats entiers q2/q3/q4 | **reçus sur le profil u16 borné** | oracles indépendants, mutants causaux et bords stricts bien couverts |
| génération horizontale régulière | **reçue conditionnellement** | complète sur les petits juges et les hypothèses déclarées ; preuve de coût WSPD encore ouverte |
| plateaux sphériques | **référence bornée seulement** | sémantique convaincante, mais explosion combinatoire et UB pour une coquille de taille 32 |
| dix forêts horizontales | **reçues comme référence CPU** | macro-lots, naissances, croissances et multifusions vérifiés |
| tour HGP complète | **non reçue** | aucune application verticale entre ordres, aucune campagne de naturalité |
| rendu du chapitre 9 | **partiel, non publié** | agrégation testée, mais absente du chemin normal et payload final incomplet |
| API producteur pour `morsehgp3d` | **absente** | aucun adaptateur vers `CertifiedTowerInput`, aucun reçu canonique de source |
| plafond mémoire | **rejeté comme garantie** | résultats terminés non comptés dans la résidence cumulée |
| GPU | **non implémenté** | aucun target CUDA ; témoin `.cu` jamais compilé |
| 50 000 points / 100 ms | **non qualifié, très loin** | des dizaines de secondes sont encore mesurées à 8 000 points |
| 10 M à 30 M transactionnels | **non implémentés** | catalogues et résultats globaux résidents, indices locaux sans tuilage |
| statut public exact | **ne pas revendiquer** | la prudence actuelle est correcte |

## 1. Référentiel et méthode

L'audit couvre l'intégralité de `morsehgp3D_v4` : documentation, CMake, environ 16 100 lignes C++/CUDA, tests, probes, reçus et historique Git. Il compare ce dossier à trois autorités, dans cet ordre :

1. les parties I et II du manuscrit de thèse, pages PDF 35 à 134 ;
2. le contrat courant du dépôt à la racine, notamment `README.md`, `docs/SPECIFICATION_MORSEHGP3D.md`, l'état de Phase 15 et l'API publique `morsehgp3d` ;
3. les contrats locaux de v4 dans `README.md`, `docs/MATHEMATIQUES.md` et `PASSATION.md`.

Les notes et anciens audits ont servi à retrouver les obligations et les contre-fixtures, jamais à remplacer une relecture du code. Les commits fonctionnels postérieurs au dernier audit reçu sont principalement :

| commit | objet | réception de cet audit |
|---|---|---|
| `d32e5944` | préfiltre q4 par puissance équatoriale | reçu comme condition nécessaire, sous oracle |
| `c1af984e` | réemploi de `q3_power`, bord strict et deux étages i64 | reçu mathématiquement et par tests |
| `f2533b4e` | plafond sur le pic projeté | intention reçue, garantie de résidence rejetée |

### Vérifications exécutées

- configuration et build Release avec GCC 13.3, `-Wall -Wextra -Wpedantic -Werror` : succès ;
- CTest Release : **147/147 tests réussis** en 63,46 s ;
- build Debug GCC avec ASan+UBSan : succès ;
- CTest ASan+UBSan avec `detect_leaks=0` : **147/147 tests réussis** en 1 192,65 s ;
- `tools/check_docs.py`, `check_passation.py`, `check_implementation_status.py`, `check_scope.py` et `check_references.py` : succès ;
- reproduction UBSan indépendante sur une coquille cosphérique u16 de 32 points : échec déterministe à `src/forest/sphere_plateau.hpp:119` ;
- inspection de la CI GitHub : le workflow racine ne configure ni ne teste `morsehgp3D_v4`.

Deux portes optionnelles fondées sur Boost/OBig n'ont pas été enregistrées, l'en-tête Boost requis étant absent de l'environnement local. La suite compte donc 147 tests ici et 149 lorsque cette dépendance est disponible. Cette variabilité contredit les textes qui présentent 147 comme un total universel.

LeakSanitizer n'est pas exploitable dans ce conteneur à cause de l'interdiction de `ptrace` sur `/proc`. La campagne a donc été relancée avec la seule détection de fuites désactivée ; ASan et UBSan restent actifs. Clang et CUDA n'étaient pas disponibles localement. Ces limites sont des limites de preuve, pas des échecs attribués au code.

## 2. Confrontation aux objectifs réels du dépôt

Le dépôt racine ne cherche pas seulement « dix forêts ». Il cherche une source géométrique sparse exacte, sans mosaïque de Delaunay d'ordre supérieur, capable d'alimenter une réduction aval avec :

- les forêts horizontales de tous les ordres ;
- les arêtes verticales entre ordres adjacents ;
- les simplexes projectables et les niveaux de leurs cofaces incidentes ;
- des reçus liant le payload à une source complète, exacte et non surrogate ;
- un chemin transactionnel, budgeté puis streamé aux grandes tailles ;
- une qualification `warm_e2e` à 50 000 points, p95 inférieur à 100 ms, avec une porte secondaire sous 1 s ;
- ensuite 10 000 001 points et davantage, sans catalogue global incompatible avec la taille de sortie.

L'API publique matérialise cette frontière dans `morsehgp3d/include/morsehgp3d/api/point_hierarchy.hpp`: `CertifiedTowerInput` contient nœuds, arêtes et simplexes projectables ; `TowerSourceReceipts` exige quatre identifiants canoniques et les drapeaux `all_orders_complete`, `vertical_maps_complete`, `projectable_incidences_complete`, `exact_source_certified`, avec `surrogate_source_used=false`.

V4 ne satisfait pas encore cette frontière :

| Objectif du dépôt | Réalité v4 au pin audité |
|---|---|
| éviter Delaunay d'ordre supérieur | oui ; la source WSPD et les boules exactes respectent cet interdit |
| source sparse exacte | génération sparse en amont, puis événements, facettes et deltas globaux résidents en aval |
| tour multi-ordre | dix folds horizontaux indépendants ; aucune verticale |
| payload projectable | `RenderResult` de test, sans export vers l'API racine ni table publique complète des naissances |
| source certifiée | aucun schéma de reçu compatible avec la racine |
| API intégrable | seulement des exécutables CMake ; pas de `add_library`, `install`, en-tête public stable ou sérialiseur |
| GPU exact | macros de préparation et un `.cu` orphelin ; aucune compilation device |
| 50 k / 100 ms | aucune exécution complète à 50 k ; à 8 k, `t_gen` reste autour de 35,3 s et un fold budgeté autour de 21 s |
| 10 M+ streamés | préflight après matérialisation de `cands` et `balls`, puis sorties résidentes |

Le `README.md` local promet encore « forêt HGP complète K=1..10 en moins de 100 ms sur une G4 » et « dizaines de millions ». Le `README.md` racine décrit désormais plus honnêtement une cible `warm_e2e` à 50 k, suivie d'un chemin transactionnel streamé. Ces deux formulations doivent être unifiées. Tant que « complet » signifie trace symbolique exhaustive, la latence doit être explicitement output-sensitive ; si 100 ms vise une requête chaude ou des labels depuis un index préconstruit, le contrat doit le dire et séparer `cold_build`, `full_symbolic_stream` et `warm_query`.

## 3. Audit mathématique

### 3.1 Objet HGP et réduction événements-boules

La lecture du manuscrit confirme le cœur utilisé par v4 : un événement Gabriel d'ordre `K` relie par une hyperarête les facettes de son simplexe au niveau exact de sa miniboule ; les composantes, traitées par niveaux fermés, donnent les polyèdres et leur hiérarchie. Une représentation finie correcte doit préserver les égalités de niveaux, les multifusions et la naturalité entre ordres.

Sous sites distincts et position générale, la réduction q2/q3/q4 est correcte. Pour un simplexe Gabriel `sigma`, sa miniboule possède un support de deux à quatre sites, et tous les autres sommets de `sigma` sont strictement intérieurs. Réciproquement, support plus intérieur complet reconstruit l'événement. Le seuil `h_q=s_max-q+1` donne bien `10/9/8` lorsque `K_max=10` et `n>=11`.

Les implémentations respectent cette taxonomie :

- q2 utilise la boule diamétrale et une profondeur stricte ;
- q3 utilise l'acuité stricte, l'owner canonique, les formes de Gram/Cramer et des niveaux exacts comparés en U192 ;
- q4 utilise le déterminant non nul, la positivité du centre, le census exact et des comparaisons croisées U320 ;
- les égalités de niveaux sont sémantiques, et non des égalités de représentation binaire.

Les limites de largeur sont documentées et les oracles extrêmes u16 exercent les carries, signes et parités. Je n'ai trouvé ni overflow signé sur le chemin normal du profil u16 ni comparaison de niveau en flottant.

Réserve d'intégration : `Q4Level` est une représentation exacte interne, mais pas la fraction canonique `ExactLevel` de l'API racine. L'exactitude de comparaison est reçue ; la canonicalisation et la sérialisation inter-modules ne le sont pas.

### 3.2 Filtres q4 des commits du 19 août

Les deux filtres i64 exécutés avant `q3_power` sont des conditions nécessaires sûres pour un tétraèdre q4 admissible, avec `D2=|ab|²` :

- le test de sommet impose `2 max(l_ay,l_by,l_xy) > D2` ;
- le test de paire impose `max(l_ax+l_ay,l_bx+l_by) > D2`.

L'égalité est correctement rejetée sur la frontière non stricte. Les survivants passent ensuite au calcul exact de puissance de la face par la primitive q3 existante, puis à Cramer. Les oracles indépendants tuent les mutants de signe, de sommet, de paire, de frontière et de carry. Je n'ai trouvé aucune fausse mort dans ces étages.

La mesure « 80,7 % des rejets de centre capturés avant Cramer » est reçue comme résultat d'ingénierie sur la fixture mesurée, pas comme constante universelle. Elle améliore le coût unitaire mais ne change ni la taille de sortie ni le statut produit.

### 3.3 WSPD et complétude de génération

Le ledger conserve exactement la masse des paires et les décisions de mort sont fail-open : une boîte ne meurt que par un certificat inférieur exact, et un doute descend. Les petits juges exhaustifs vérifient les événements par identité et les mutants `drop-rect`, `cap-terminal`, non-stricts et pertes de tranches sont tués. La complétude fonctionnelle sur le domaine testé est donc crédible.

La preuve de complexité annoncée ne ferme toutefois pas encore l'implémentation exacte. `docs/ARCHITECTURE.md` invoque une dégradation constante de Callahan-Kosaraju pour l'arbre de préfixes, tandis que le code choisit aussi les boîtes serrées et une règle de scission concrète. Le ledger prouve une partition sans perte ; il ne prouve pas à lui seul une borne `O(s^3 n)` sur le nombre de rectangles produits par cette variante. Il faut soit raccorder formellement le code à une décomposition dont la borne est établie, soit publier la complexité comme conditionnelle aux compteurs mesurés.

Cette réserve ne remet pas en cause l'exactitude des événements déjà jugés. Elle interdit de transformer une bonne campagne à `n=8000` en garantie asymptotique pour 30 M.

### 3.4 Macro-lots, forêt et rendu

La partie forêt est la plus aboutie du dossier :

- tri stable par niveau exact et regroupement par `same_exact_level` ;
- racines pré-lot gelées avant les unions ;
- une transition `ComponentDelta` pour naissance, croissance ou multifusion ;
- identifiants canoniques par plus petite `FacetKey` ;
- toutes les facettes conservées pour le rendu, y compris celles nées dans le lot ;
- multiplicités d'incidence non écrasées ;
- naissance d'une facette calculée par miniboule exacte, pas par première incidence.

Le juge de forêt réénumère indépendamment les miniboules en grands entiers, construit les cliques du manuscrit puis compare les partitions, deltas, niveaux et rendus. Les fixtures de carré cosphérique, multifusion, croissance et naissance tuent les simplifications incorrectes. Sur le domaine borné, ce résultat est reçu.

Deux distinctions restent impératives :

1. `build_render` est appelé dans le chemin `--judge`, pas dans la production normale du probe ; le résultat imprimé n'est donc pas un payload du chapitre 9 ;
2. dix forêts horizontales ne sont pas la tour HGP : aucune application verticale `K+1 -> K` n'est construite ou vérifiée.

Le `RenderResult` actuel contient les facettes, leurs multiplicités par lot et les niveaux de lot. Il ne publie pas, dans un objet final unique, les niveaux de naissance de facette, les objets `S_tau`, `T_x`, `m_tau`, les votes, les cartes verticales et les reçus exigés par le consommateur racine. `facet_birth_level` n'est qu'une primitive de test appelée ponctuellement.

### 3.5 Plateaux et dégénérescences

La règle `expand_plateau` est mathématiquement raisonnable pour l'oracle borné : elle énumère les sous-ensembles de coquille `T`, conserve ceux dont le centre appartient à l'enveloppe convexe fermée, puis marque comme actifs les retraits qui changent la miniboule. Cela récupère les événements perdus par une hypothèse de position générale trop stricte.

Ce n'est pas une représentation produit. Le coût est exponentiel en la taille de coquille et le code contient un défaut défini :

```cpp
const u32 nu = (u32)shell_all.size();
for (u32 tm = 1; tm < (1u << nu); ++tm) {
```

À `nu=32`, `1u << nu` est un décalage hors largeur. Le CLI accepte une valeur arbitraire de `--shell-cap`; une coquille valide de 32 sites distincts sur une sphère u16 atteint donc cette ligne si l'appelant relève le plafond. UBSan produit :

```text
src/forest/sphere_plateau.hpp:119:29: runtime error:
shift exponent 32 is too large for 32-bit type 'unsigned int'
```

Le défaut est masqué par le plafond par défaut 12, mais l'API ne l'encode pas comme précondition. Même `nu=31`, techniquement défini, demanderait plus de deux milliards de masques. Correction minimale : refuser explicitement `nu>=32` avant tout décalage et borner séparément le régime d'énumération supporté. Correction produit : définir une représentation comprimée des plateaux ou retourner `unsupported_degeneracy` sans commencer l'expansion.

### 3.6 Sites distincts et identités

Le chemin principal de `forest_probe` refuse bien les positions dupliquées avant la géométrie exacte, conformément au manuscrit. Les `PointId` externes sont ensuite conservés à travers Morton, owners, supports et facettes ; les portes de relabeling sont solides.

La frontière n'est cependant pas encapsulée. `build_cloud_index` bucketise les doublons et renvoie un index utilisable ; `point_id(u)` choisit la première identité du bucket. Seul l'exécutable ajoute le refus `unique_count()==input.size()`. Une future réutilisation directe des en-têtes peut donc contourner le profil exact sans statut d'erreur. La bonne frontière est une façade qui valide identités, coordonnées et unicité avant d'exposer le moindre index géométrique ; le builder bas niveau peut rester interne.

V4 utilise des `PointId` u32, ce qui convient à sa cible locale de dizaines de millions, tandis que l'API racine utilise des IDs u64. Ce n'est pas un bug arithmétique, mais un adaptateur v4 ne pourrait pas accepter tout le domaine public sans vérification et refus explicite. Les indices de facettes et d'union-find restent également u32/i32 et demandent un tuilage avant `2^32`, déjà reconnu dans la passation mais non implémenté.

## 4. Blocage B0 — le plafond mémoire ne borne pas la résidence

### 4.1 Formule annoncée

`project_output_budget` calcule :

`bytes_peak = bytes_events + min(max(fold_budget, max_K m_K), sum_K m_K)`

où `m_K=fold_bytes_upper_from_counts(E_K,W_K)`. La preuve donnée est correcte pour la variable **d'ordonnancement** `reserved`: à tout instant, la somme des budgets des tâches actives reste sous cette borne, sauf une tâche hors budget admise seule.

Le passage invalide est d'identifier `reserved` à la mémoire résidente du fold. Dans `run_folds_budgeted`, `reserved -= bytes[idx]` dès que la fonction de tâche retourne. Or la tâche a déplacé son résultat dans `per_k_result[K]` : `nodes`, `deltas`, `batch_levels`, `batch_of_event`, `facet_keys` et `final_canon_fid` restent vivants jusqu'à la fin des dix folds. Les résultats terminés s'accumulent alors que leur `m_K` est retiré de `reserved`.

La formule mélange donc trois grandeurs :

- les événements, tous résidents ;
- les temporaires des folds actifs, réellement limités par l'ordonnanceur ;
- les sorties persistantes des folds terminés, absentes du calcul du pic.

### 4.2 Contre-preuve avec le reçu n=8000

Le reçu `campagne_locale_n8000_v2_20260818/v2_uniform_n8000_smax11.txt` donne :

| grandeur finale | compte |
|---|---:|
| événements | 3 126 158 |
| facettes denses | 19 466 907 |
| `ComponentDelta` | 2 791 148 |
| facettes dans `born` | 16 177 847 |
| nœuds | 1 974 086 |

Sur l'ABI GCC mesurée au pin, `sizeof(ForestEvent)=144`, `sizeof(FacetKey)=44`, `sizeof(ComponentDelta)=160` et `sizeof(ForestNode)=16`. Un minorant de l'état final, sans capacité excédentaire et sans aucun parent de delta, vaut :

| stockage nécessaire | octets |
|---|---:|
| événements | 450 166 752 |
| `facet_keys` + `final_canon_fid` | 934 411 536 |
| en-têtes de `deltas` | 446 583 680 |
| clés des seuls vecteurs `born` | 711 825 268 |
| nœuds | 31 585 376 |
| `batch_of_event` | 25 009 264 |
| **minorant final** | **2 599 581 876** |

Le commit annonce `bytes_peak=2 597 650 400`. Le seul état final obligatoire le dépasse déjà de **1 931 476 octets**. Ce minorant exclut pourtant :

- tous les `parents` des deltas ;
- `batch_levels`, capacités de vecteurs et fragmentation d'allocateur ;
- temporaires du fold encore actif ;
- candidats, boules, arbre, ordre Morton et piles, explicitement hors portée de la garde.

La mesure appariée du scheduler rapporte d'ailleurs un pic processus de 4 952 616 Kio pour une réserve maximale de 2 140 153 484 octets. Ce chiffre inclut l'amont et n'est donc pas la preuve principale ; il est cohérent avec le défaut de modèle.

### 4.3 Action exigée

Jusqu'à séparation de `m_K` en sortie persistante et temporaires, le stopgap sûr est de décider sur `bytes_events + sum_K m_K`. Il peut refuser trop tôt, mais ne doit pas être nommé « pic résident précis ». Le modèle final doit suivre :

`événements + sorties terminées + sortie en construction + temporaires des folds actifs + amont explicitement inclus ou publié séparément`.

La porte actuelle est auto-référentielle : elle compare la décision de la formule à la même formule gravée. Il faut une porte indépendante fondée au minimum sur les tailles réelles de `ForestResult` et une fixture où plusieurs folds terminés restent résidents. Un contrôle RSS peut compléter cette preuve, pas la remplacer.

En l'état, `--max-output-bytes` peut protéger contre certaines allocations massives, mais **ne doit pas promettre une borne du pic de résidence**.

## 5. Blocage B0 — la trace exhaustive et le SLO sont incompatibles

La borne Poisson q2 déjà reçue tranche le contrat de sortie, indépendamment de toute optimisation q3/q4. Dans un processus homogène sans bord, chaque profondeur `j=0..9` apporte asymptotiquement quatre événements q2 par point. À `K_max=10` :

- 40 événements q2 par point ;
- pour `j=1..9`, les retraits de points intérieurs injectent `4 sum j = 180` facettes nées distinctes par point ;
- ces facettes portent `4 sum j(j+1) = 1320` identités `PointId` par point.

À 30 millions de points, l'espérance impose donc déjà :

| objet q2 seul | cardinalité / taille |
|---|---:|
| événements | 1,2 milliard |
| facettes nées | 5,4 milliards |
| incidences `PointId` u32 | 39,6 milliards |
| seuls octets des IDs | **158,4 Go** |
| événements avec l'ABI v4 à 144 octets | **172,8 Go** |

Il s'agit d'un minorant théorique de la trace exhaustive attendue, avant les événements q3/q4, les facettes actives, les niveaux, les deltas, les verticales et les index. La campagne uniforme à 8 000 observe environ 391 événements tous ordres par point ; une extrapolation linéaire donnerait 11,7 milliards d'événements et environ 1,69 To avec l'enregistrement courant. Cette seconde valeur est une projection d'ingénierie, pas un théorème ; le minorant q2 suffit déjà.

Conséquence : le temps de production d'une trace exhaustive doit dépendre de la sortie, et le stockage doit être streamé ou externe. Le SLO de 100 ms ne peut être honnêtement attaché au même objet résident à 30 M. Le dépôt doit versionner au moins deux produits :

1. `full_symbolic_stream`, exact, transactionnel, output-sensitive, possiblement construit à froid ;
2. `warm_query` ou `labels`, depuis une représentation préconstruite et certifiée, seul candidat naturel au p95 de 100 ms.

La hiérarchie de connectivité implicite ne peut pas non plus prétendre être une trace de facettes si elle ne conserve qu'un quotient. Un quotient peut être un produit distinct, mais il doit déclarer exactement quelles requêtes, verticales et preuves il préserve.

## 6. Architecture et implémentation

### 6.1 Points forts réutilisables

- une seule géométrie d'index, déterministe sous permutation ;
- décisions critiques en entiers exacts, avec flottants seulement dans des filtres gardés et fail-open ;
- owners et `SupportKey` fondés sur les vrais `PointId` ;
- RLE par `BallKey` avant census, évitant des census répétés ;
- préflight avant `ev_k`, même s'il arrive encore après `cands` et `balls` ;
- parallélisme par tranches qui conserve l'ordre déterministe ;
- fold sort/reduce plus approprié que les anciennes maps imbriquées ;
- mutations causales, planchers anti-vacuité et petits oracles réellement indépendants.

Ces choix doivent être préservés dans un futur producteur sparse.

### 6.2 Ce qui empêche l'intégration

Le `CMakeLists.txt` ne définit que des exécutables de test et de benchmark. Il n'existe ni cible bibliothèque, ni installation, ni namespace public stable, ni objet de résultat transactionnel. Le principal pipeline vit dans `bench/forest_probe.cpp`, fichier qui cumule CLI, scheduling, préflight, bancs, portes, production et rapport.

Cette concentration est acceptable pour un oracle de recherche, mais pas pour une source du dépôt : elle empêche de tester la frontière publique indépendamment du CLI, de substituer un backend CUDA et de lier le payload aux reçus racine.

La forme cible devrait séparer :

- une bibliothèque de prédicats et niveaux exacts ;
- un producteur borné de fixtures de référence ;
- une interface de source streamée, avec statuts et continuation ;
- des exécutables de probe qui ne contiennent aucune logique scientifique exclusive.

### 6.3 Mémoire de l'index

`docs/ARCHITECTURE.md` annonce environ 96 octets par position unique et 3,2 Go à 30 M. L'ABI CPU actuelle est plus lourde : un `RadixNode` mesure 120 octets et les tableaux persistants de `CloudIndex` représentent environ 168 octets par site unique avant capacités et allocateur : clé 8, position 24, deux index CSR 8, préfixe de poids 8 et nœud moyen 120. Cela donne environ 5,04 Go à 30 M, auxquels s'ajoutent environ 0,96 Go d'`InputPoint`, 1,2 Go d'ordre/CSR selon le chemin, les temporaires de construction et toute la sortie.

Ce n'est pas un blocage isolé : 5 Go peuvent être acceptables dans certains environnements. C'est en revanche une preuve que la documentation SoA compacte décrit une cible, pas le layout effectivement construit par le fallback CPU. Les budgets doivent utiliser `sizeof` du backend réel et publier séparément persistent, temporaire et sortie.

### 6.4 GPU et performances

Le projet CMake déclare seulement le langage `CXX`. `src/gpu/device_compile_witness.cu` n'est rattaché à aucune cible ; les annotations `MHGP4_HD` deviennent vides dans le build courant. Aucune primitive n'est donc compilée par un compilateur CUDA, et aucune divergence host/device ne peut être détectée. La campagne G4 documentée ne contient aucun reçu réussi de v4.

Les mesures CPU montrent des progrès réels : paralléliser la descente WSPD réduit `t_gen` d'environ 72,8 s à 35,3 s sur la cellule uniforme à 8 000 points ; le scheduler budgeté réduit aussi le fold. Cela reste plusieurs ordres de grandeur au-dessus de 100 ms avant passage à 50 k, et aucune extrapolation sérieuse ne permet de revendiquer le SLO.

Le bon ordre est : compiler d'abord un témoin device minimal, faire passer les oracles différentiels host/device, porter les filtres exacts sans changer l'objet, puis lancer une campagne G4 épinglée. Porter le pipeline dense actuel avant de trancher le produit de sortie déplacerait le goulet sans résoudre la taille fondamentale.

## 7. Documentation, CI et gouvernance de preuve

### 7.1 CI

Le workflow `.github/workflows/ci.yml` construit la bibliothèque racine avec GCC, Clang et sanitizers, mais ne configure jamais `morsehgp3D_v4`. Les seuls contrôles qui touchent v4 sont documentaires. Une régression de ses 147 ou 149 portes peut donc être fusionnée sur `main` sans signal CI.

Porte minimale à ajouter avant toute promotion de v4 :

- GCC Release ;
- Clang Release ;
- ASan+UBSan ;
- Boost/OBig obligatoire pour les deux juges optionnels ;
- compilation CUDA du témoin lorsque le job dispose de l'outil, sans exiger encore un GPU d'exécution.

### 7.2 Documentation sémantiquement périmée

`PASSATION.md` est la meilleure carte de l'état réel, mais son titre et son pin restent datés du 18 août et sa section mémoire reprend la garantie erronée du dernier commit. Les autres documents ont davantage dérivé :

- `README.md` présente encore `src/events`, `src/forest` et `oracle` comme « à venir » ;
- `docs/ARCHITECTURE.md` décrit une descente trois lanes partagée et un layout SoA compact qui ne correspondent pas entièrement au chemin courant ;
- `docs/PLAN_DE_TESTS.md` ne reflète pas la suite de 147/149 portes ;
- `docs/MATHEMATIQUES.md` laisse Q1–Q4 ouvertes alors que plusieurs sont tranchées, et une proposition de rendu actif seulement contredit la règle « toutes les facettes » reçue ailleurs ;
- les commentaires de source emploient « temporaires » pour `fold_bytes_upper`, alors que la formule inclut une « sortie dense » persistante.

Les checkers actuels valident liens, fraîcheur et champs de statut, pas la cohérence sémantique entre documents. Après correction des deux blocages, une mise à jour documentaire unique doit retirer les états contradictoires plutôt qu'ajouter un nouvel addendum.

## 8. Résultats de test et limites de preuve

| campagne | résultat | interprétation |
|---|---|---|
| GCC Release, build strict | succès | aucune alerte compilateur sur le profil exercé |
| CTest Release | **147/147** | toutes les portes enregistrées sont vertes |
| GCC ASan+UBSan, `detect_leaks=0` | **147/147 en 1 192,65 s** | aucune erreur mémoire ou arithmétique sur les portes enregistrées |
| UBSan plateau `nu=32` | **échec reproduit, code 1** | défaut réel hors profil par défaut mais accessible par option |
| checkers du dépôt | tous verts | cohérence structurale, pas preuve sémantique |
| Boost/OBig optionnel | non exécuté | dépendance absente localement |
| Clang | non exécuté | compilateur absent localement |
| CUDA | non exécuté | aucune cible v4 et outil absent |
| G4 | non utilisé | aucune mesure distante produite par cet audit |

Les 147 tests couvrent bien les erreurs que les auteurs ont anticipées. Ils ne couvrent pas le plafond résident par une autorité indépendante, `shell_cap>=32`, les verticales, l'adaptateur public, un run 50 k complet, le tuilage u32 ou un backend device. C'est précisément la frontière entre « référence CPU bien testée » et « produit qualifié ».

## 9. Feuille de route recommandée

### Priorité 0 — rendre les refus vrais

1. retirer l'affirmation de pic résident de `--max-output-bytes` ou utiliser provisoirement `bytes_events + sum_K m_K` ;
2. séparer dans la comptabilité sortie persistante, temporaires actifs et amont déjà résident ;
3. ajouter une porte indépendante où plusieurs `ForestResult` terminés restent vivants ;
4. valider `shell_cap` avant le décalage et refuser le régime combinatoire non supporté ;
5. mettre v4 dans la CI avec Boost obligatoire.

### Priorité 1 — décider l'objet produit

Versionner explicitement les contrats `full_symbolic_stream`, `connectivity_index` et `warm_query_or_labels`. Pour chacun, fixer : contenu, complétude, verticales préservées, chronomètre, budget mémoire, reprise, sérialisation et statut public. Ne plus attacher le même « moins de 100 ms » à une trace dont la taille varie de plusieurs ordres de grandeur.

### Priorité 2 — positionner v4 comme oracle

Extraire le noyau exact dans une petite bibliothèque interne et faire de v4 le producteur de fixtures canoniques pour petits `n`. Ajouter une façade à statuts qui refuse doublons, IDs hors domaine, coquilles hors profil et capacités u32. Exporter les niveaux sous la forme canonique de l'API racine.

### Priorité 3 — fermer la tour

Sur des tailles bornées d'abord :

- construire les applications verticales entre ordres adjacents ;
- vérifier les carrés de naturalité à chaque niveau critique ;
- publier les simplexes projectables avec les niveaux de cofaces ;
- produire les quatre IDs de reçu et un digest canonique du payload ;
- adapter le résultat à `CertifiedTowerInput` et faire passer la réduction publique sans drapeau surrogate.

### Priorité 4 — rendre la source réellement streamée

Déplacer le préflight avant les vecteurs globaux de candidats et de boules, partitionner par tuiles de clés, sceller chaque chunk, effectuer les tris/merges externes exacts puis libérer chaque tranche après consommation. Les IDs globaux restent u64 ; les indices locaux peuvent rester u32 avec une base de tuile et une garde vérifiée.

### Priorité 5 — qualifier le GPU puis le SLO

Compiler le témoin CUDA, porter les prédicats et filtres, comparer bit à bit au backend CPU sur les fixtures, puis exécuter une campagne G4 fraîche et épinglée. La porte 50 k doit couvrir le produit décidé à la priorité 1 ; une mesure de front, de composant isolé ou de préfiltre ne vaut pas `warm_e2e`.

## 10. Portes de sortie proposées

V4 ne devrait changer de statut que lorsque toutes les portes suivantes sont vertes :

1. plafond mémoire validé contre une comptabilité indépendante et un RSS mesuré, sans résultat terminé oublié ;
2. coquilles de tailles 12, 31 et 32 : succès borné ou refus explicite, jamais UB ni boucle impraticable ;
3. suite v4 en CI GCC, Clang, ASan+UBSan et Boost/OBig ;
4. petit oracle de tour complète, verticales et carrés de naturalité inclus ;
5. round-trip canonique v4 vers `CertifiedTowerInput` avec reçus non nuls et source non surrogate ;
6. run transactionnel interrompu/repris sur plusieurs tuiles, digest identique au run monolithique borné ;
7. campagne G4 12 500 / 25 000 / 50 000 sur toutes les familles contractuelles, avec p95 et pic mémoire du produit complet choisi ;
8. campagne 10 000 001 sans catalogue global, avec reprise et plafonds respectés.

## Conclusion d'auditeur

L'effort v4 a corrigé plusieurs erreurs profondes des versions précédentes : vraie identité des points, niveaux exacts, séparation des lanes q3/q4, traitement des égalités, plateau non supprimé, transitions de composantes complètes et rendu non réduit à un arbre couvrant. Les commits q4 les plus récents poursuivent cette trajectoire et sont reçus.

La limite actuelle n'est plus principalement une formule locale. C'est l'écart entre un **oracle horizontal dense** et le **producteur sparse, vertical, transactionnel et certifié** que le dépôt demande. Le dernier commit mémoire illustre ce risque : une borne correcte pour l'ordonnanceur a été nommée borne de résidence sans compter la persistance des résultats.

Statut honnête au pin audité :

- `mathematical_predicates_u16 = received_bounded` ;
- `horizontal_cpu_reference = received_conditional` ;
- `complete_hgp_tower = not_implemented` ;
- `product_backend = not_implemented` ;
- `gpu_backend = not_compiled` ;
- `performance_contract = not_qualified` ;
- `public_status = not_claimed`, **correct**.

La recommandation est de corriger d'abord les deux garanties fausses, puis de conserver v4 comme oracle de haute qualité pendant que la source produit se construit autour du contrat racine. C'est la voie la plus honnête vis-à-vis des objectifs du dépôt et celle qui réutilise le mieux le travail déjà solide.
