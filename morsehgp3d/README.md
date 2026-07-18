# MorseHGP3D — cœur certifié

Ce répertoire porte la nouvelle implémentation décrite par la roadmap. `HGP-old/` reste une référence historique séparée et n'est pas importée par ce cœur.

État actuel : Phase 5 en cours, backend `reference_cpu`, profil prioritaire `hgp_reduced` et mode `certified`. Les Phases 2A, 2B, 3 et 4 sont fermées. L'oracle spatial exact et son accélérateur Morton-LBVH certifié servent de vérité terrain; ils ferment la précondition spatiale de la Phase 5, sans fermer la porte globale G2 du catalogue et sans produire de statut public `exact`.

La tranche actuellement intégrée fournit :

- l'interprétation exacte des coordonnées binary64 comme dyadiques;
- le nuage de points canonique, avec normalisation réelle du zéro signé, identifiants globaux déterministes, provenance source et rejet exact des doublons;
- les exclusions triées et bornées par le $m_{\star}$ du run, y compris le cas $m_{\star}=0$;
- le 1-NN et le top-$k$ brute-force exacts, séparant les distances strictement inférieures du shell d'égalité complet et d'un choix canonique dérivé;
- la partition globale exacte d'une boule fermée en intérieur, shell et extérieur, sans confondre son rang fermé avec un rang filtré par exclusions;
- un index Morton-LBVH déterministe à 63 bits, avec collisions résolues par `PointId` et AABB formées d'extrema dyadiques exacts;
- le 1-NN, le top-$k$ et la boule fermée accélérés par élagages AABB stricts, avec shells complets, marges rationnelles positives et comptabilité séparée des distances réellement évaluées;
- l'ancre $k=1$ `reference_cpu` par graphe euclidien complet exact, avec niveaux EMST divisés par quatre, minima singleton, lots de longueurs égales figés, multifusions canoniques, poids exacts et coupes strictes ou fermées;
- la forêt $k=1$ compacte construite depuis un EMST géométriquement certifié, avec feuilles implicites, niveaux d'événements factorisés, enfants CSR, aucune couverture persistante et cinq arènes bornées par $6(n-1)$ enregistrements;
- `ExactRational3` homogène et canonique;
- `ExactLevel` non négatif, canonique et compatible avec le contrat v2;
- le fallback `boost::multiprecision::cpp_int` qui sert d'oracle aux filtres CPU;
- la comparaison de distances carrées et l'orientation 3D exactes;
- la forme affine exacte `H_RQ` pour deux labels canoniques de même cardinalité;
- `ExactPlane3`, ses coefficients homogènes primitifs orientés, sa clé géométrique non orientée et son record fermé `2.0.0`;
- la construction d'un plan depuis trois points dyadiques ou rationnels exactement indépendants;
- la classification sûre de `H_RQ` en plan propre, constante négative, constante positive ou forme identiquement nulle;
- l'orientation 2D exacte dans un plan support, avec validation exacte de l'incidence;
- l'intersection de trois plans par rang exact, avec classes `unique`, `empty` et `affine_family`;
- le signe déterminantal et le témoin rationnel d'incidence d'un quatrième plan;
- les centres circonscrits exacts de supports d'un à quatre points;
- la classification exacte des supports affinement dépendants, sans centre arbitraire;
- le couplage atomique du centre homogène `ExactRational3` et de son rayon carré `ExactLevel`;
- les coordonnées et signes barycentriques exacts dans tout support affinement indépendant de taille un à quatre;
- la classification exacte `relative_interior`, `relative_boundary` ou `exterior` du centre circonscrit;
- la réduction certifiée d'un support de frontière vers ses coefficients strictement positifs;
- la classification exacte d'un point comme strictement intérieur, sur la frontière ou extérieur à une sphère;
- la comparaison instrumentée de deux `ExactLevel` par produit croisé entier, avec témoin et égalité exacte;
- la clé canonique des supports de taille un à quatre, ordonnée par cardinalité puis par identifiants de points;
- le regroupement déterministe des supports déjà vérifiés en lots de niveaux exactement égaux, avec provenance et multiplicité d'émission séparées;
- des entrées `decision-only` séparées des témoins rationnels de replay;
- des décisions séparant signe scientifique, étage de certification et compteurs;
- des filtres d'intervalles FP64 conservateurs pour la comparaison de distances, l'orientation 3D et `H_RQ` à provenance binary64;
- des expansions flottantes exactes de signe, capables de certifier les annulations et insérées entre les intervalles FP64 et le fallback multiprécision;
- la provenance binary64 positionnelle des supports analysés et la provenance réduite des niveaux émis, toutes deux exclues de l'égalité scientifique;
- la provenance binary64 canonique des formes affines et plans issus de coefficients explicites, de trois points ou d'un bisecteur de puissance, exclue de leur identité scientifique et de leur record fermé;
- des polynômes homogènes Gram/Cramer sans division flottante pour les signes barycentriques, le côté d'une sphère circonscrite et l'ordre de deux niveaux issus de supports dyadiques;
- des polynômes homogènes sans division flottante pour l'orientation 2D, le côté d'un plan, les rangs d'un système de trois plans et l'incidence d'un quatrième plan;
- une politique par appel distinguant la cascade adaptative, l'ancien chemin FP64 puis multiprécision et le différentiel multiprécision seul;
- un replay diagnostique versionné à partir des mots binary64 et des plans exacts d'entrée.

L'ensemble intégré clôt les sous-lots affines 2A.4, centres 2A.5, minimalité locale 2A.6 et ordre exact 2A.7, les trois sous-lots adaptatifs 2A.8a à 2A.8c, la campagne 2A.9 et la revue 2A.10. La Phase 2A et G1 sont donc fermées. Les comparaisons de distances, orientations 3D et évaluations de `H_RQ` dont le témoin et les labels conservent leurs entrées binary64 suivent `fp64_filtered` puis `expansion`, avant `cpu_multiprecision`. La même cascade couvre les signes barycentriques de requêtes et de centres circonscrits, le côté d'une sphère définie par un support dyadique, l'ordre de niveaux dont les supports dyadiques sont conservés, puis l'orientation 2D, le côté d'un plan, la classification de trois plans et l'incidence d'un quatrième plan lorsque leurs recettes binary64 sont disponibles. Toutes les entrées arbitrairement rationnelles sans provenance restent multiprécision; aucun `BigInt` n'est converti approximativement en `double` pour inventer un filtre.

La Phase 2B ferme les propositions GPU des trois prédicats seulement parce que chaque `unknown` tombe vers une décision CPU exacte et que les campagnes matérielles sont qualifiées. La Phase 4 a fermé `morsehgp3d::spatial` sur CPU de référence et qualifié le front LBVH parallèle de proposition sur G4. Son cutoff top-$k$ est un `ExactLevel`; la sortie conserve tous les co-minimiseurs, même lorsque leur cardinal dépasse $k$, $s_{\max}$ ou quatre. `canonical_choice_ids()` n'est qu'un représentant déterministe. `ClosedBallPartition` reste volontairement globale et sans exclusions afin que `closed_rank()` garde le sens du rang fermé d'un événement. Le LBVH ne propose qu'un ordre de visite : chaque prune est décidé ou recertifié par une borne rationnelle exacte strictement séparée du cutoff ou du rayon, et une égalité descend toujours jusqu'au shell. La Phase 5 possède maintenant son ancre EMST exacte `reference_cpu`, son catalogue exhaustif des boules diamétrales et sa réduction rang-deux certifiée contre cinq chemins de rejeu; le différentiel indépendant passe sur 50 cas jusqu'à $n=14$. La forêt compacte préserve exactement les événements, coupes et multifusions de l'ancre sans stocker les couvertures; seule la voie GPU scalable reste ouverte.

Les deux étages flottants exigent IEEE binary64, l'arrondi au plus proche dans le FENV et MXCSR sur x86, les sous-normaux actifs et les options strictes exportées par la cible CMake. Chaque opération d'intervalle est élargie vers les deux infinis. Les expansions utilisent `TwoSum` et un résidu FMA, rejettent avant calcul tout bit de produit exact situé sous `2^-1074`, puis échouent fermées sur toute exception d'underflow, overflow ou opération invalide. Elles bornent aussi explicitement le nombre de composantes et de produits intermédiaires; atteindre une borne invalide seulement la tentative et déclenche le repli multiprécision, sans tronquer un témoin. L'environnement et les drapeaux flottants de l'appelant sont restaurés. `PredicateFilterPolicy::allow_adaptive` active la cascade complète; `allow_fp64` conserve le comportement historique FP64 puis multiprécision; `multiprecision_only` garde un chemin indépendant. Les API décisionnelles ne matérialisent leur déterminant multiprécision qu'en fallback, après les validations exactes de domaine ou de précondition. Les API riches et le replay reconstruisent en plus le témoin rationnel diagnostique même si le signe a été certifié rapidement : `certification_stage` désigne l'autorité du signe, pas le coût total du diagnostic.

La convention d'orientation est exactement `det([b-a, c-a, d-a])` : le quadruplet `(0, e1, e2, e3)` est positif. Cette définition explicite prévaut sur les conventions de signe parfois opposées d'autres bibliothèques.

Un plan orienté représente `a*x + b*y + c*z + d = 0`. Ses quatre coefficients entiers sont divisés par leur PGCD positif, sans imposer le signe du premier coefficient : multiplier une entrée par un facteur positif conserve le plan orienté, tandis qu'une négation inverse son orientation. `unoriented_key()` choisit séparément un signe canonique pour identifier le plan géométrique. Une normale nulle est toujours rejetée.

`ExactAffineForm3` conserve les coefficients rationnels et leur échelle exacte. Sa clé orientée est seulement le représentant homogène primitif à facteur positif, utilisé pour classifier le lieu zéro; elle ne remplace pas la valeur exacte de la forme. L'orientation 2D utilise le signe de `n dot ((b-a) cross (c-a))` pour la normale primitive orientée `n` du support. Cette valeur n'est pas appelée aire, car `n` n'est pas normalisée.

`Binary64AffineProvenance` conserve une recette fermée plutôt qu'une approximation des coefficients entiers : quatre coefficients binary64 explicites, trois points binary64 orientés, ou deux multisets binary64 de même cardinalité définissant `H_RQ`. Les zéros signés sont canonicalisés, les points et multisets sont triés avec un multiplicateur d'orientation explicite, et les coefficients rationnels reconstruits depuis la recette sont vérifiés contre l'objet exact. Une négation produit la recette orientée opposée. La provenance n'entre ni dans `operator==`, ni dans la clé scientifique, ni dans le record `ExactPlane3` `2.0.0`; relire ce record donne donc volontairement un plan exact sans voie rapide.

Une intersection unique expose un `ExactRational3`, les rangs normal et augmenté égaux à 3 et une dimension affine égale à 0. Une famille cohérente expose la dimension `3 - normal_rank`; un système incohérent n'expose ni point ni dimension. `CertifiedThreePlaneIntersection` ajoute l'étage collectif et le signe du déterminant des normales dans l'ordre canonique des plans liants; ces diagnostics sont exclus de l'égalité scientifique. Le filtre FP64 ne certifie que le rang plein. L'expansion examine exactement déterminant et mineurs afin de certifier aussi les familles et systèmes incompatibles; toute provenance absente ou tentative invalide replie vers le rang rationnel. Le replay conserve les trois plans liants dans l'entrée normalisée couverte par son identifiant, de sorte que les coordonnées seules ne sont jamais présentées comme certificat autonome.

`ExactCenter3` est un alias sémantique de `ExactRational3` : le centre conserve ainsi un dénominateur commun strictement positif et une réduction canonique déjà couverte par le contrat `2.0.0`. `CircumcenterResult` associe atomiquement ce centre et son `ExactLevel`. Une paire distincte utilise son milieu; un triangle indépendant intersecte son plan affine et deux plans médiateurs; un tétraèdre indépendant intersecte trois plans médiateurs. Chaque résultat est ensuite revérifié par égalité exacte des distances aux points du support.

Un support dépendant expose sa dimension affine exacte et des témoins `null`; il n'est jamais complété par un centre arbitraire. `analyze_circumcenter_support` calcule les barycentriques exacts du centre d'un support indépendant. Un centre intérieur rend le support localement minimal; un centre sur la frontière conserve seulement les coefficients strictement positifs, puis le centre, le niveau et l'intérieur relatif réduits sont revérifiés exactement. Un centre extérieur reste explicitement `exterior_circumcenter` et ne déclenche aucune recherche implicite parmi les sous-supports.

Pour une entrée `CertifiedPoint3`, l'analyse conserve en plus le support binary64 dans son ordre positionnel. La cascade évalue les numérateurs barycentriques par déterminants de Gram et règle de Cramer, exige un déterminant strictement positif au même étage et remappe les signes de l'ordre canonique vers les positions d'origine. Tous les coefficients rationnels sont néanmoins reconstruits et revérifiés exactement. Une analyse issue de `ExactRational3` n'invente aucune provenance et reste exclusivement `cpu_multiprecision`.

Le support singleton a pour centre le point lui-même, dimension affine zéro et `ExactLevel` nul. `classify_sphere_point` compare exactement la distance carrée d'un point au niveau d'une sphère et expose simultanément le signe, la classe et le décalage rationnel signé. Ces décisions sont locales : elles n'énumèrent pas les candidats miniball, ne complètent pas `RelevantGP` et ne suffisent jamais à produire un statut public `exact`.

La surcharge de `classify_sphere_point` prenant directement un tableau binary64 travaille sur la sphère circonscrite du support affinement indépendant. Elle ne prouve pas que ce support est le miniball d'un ensemble ambiant. De même, la surcharge tableau de `compare_support_levels` compare les rayons carrés de deux sphères circonscrites. La chaîne normative pour un niveau de miniball local reste `analyze_circumcenter_support`, réduction exacte éventuelle, puis `support_level_emission_from_analysis`; seule cette émission conserve le support dyadique minimal utilisé par la cascade d'ordre.

`compare_exact_levels` matérialise le témoin entier `N_left * D_right - N_right * D_left`; son signe est l'ordre scientifique et zéro est la seule égalité admise. Les comparateurs purs employés par le tri ne modifient aucun compteur, car leur nombre d'appels dépend de l'implémentation de la bibliothèque standard. Chaque comparaison scientifique explicite enregistre en revanche exactement une décision `cpu_multiprecision`.

Les signes rapides de sphère et de niveau utilisent uniquement des polynômes homogènes : aucune coordonnée de centre, aucun rayon et aucun quotient flottant ne sont matérialisés. Une tentative rapide est toujours comparée au signe du témoin rationnel diagnostique; une contradiction interne lève une erreur au lieu d'être publiée. L'égalité sémantique de `BarycentricCoordinates`, `CircumcenterSupportAnalysis` et `SupportLevelEmission` ignore l'étage ou la provenance interne afin que la cascade activée et le chemin multiprécision décrivent le même objet scientifique.

`canonical_level_batches` reçoit seulement des items de support déjà vérifiés. Il trie les lots par niveau exact puis les supports par cardinalité minimale et identifiants canoniques, agrège séparément les supports sources et compte les émissions identiques. Une même clé de support minimal associée à deux niveaux est une contradiction fermée. Cette canonisation ne recherche aucun sous-support, ne vérifie l'enveloppe d'aucun ensemble ambiant et ne remplace ni un `CriticalEvent` public ni un `EqualLevelBatch` contractuel complet.

## Construction locale

```bash
cmake -S morsehgp3d -B build/morsehgp3d -DMORSEHGP3D_BUILD_TESTS=ON
cmake --build build/morsehgp3d --parallel
ctest --test-dir build/morsehgp3d --output-on-failure
```

Les targets exportés sont `morsehgp3d::exact`, `morsehgp3d::spatial` et `morsehgp3d::hierarchy`; chaque étage propage ses dépendances exactes. Après `cmake --install`, un consommateur peut utiliser `find_package(MorseHGP3D CONFIG REQUIRED)` sans dépendre de chemins d'en-têtes propres à la machine de construction.

### Profils reproductibles de Phase 3

La Phase 3 fournit quatre workflows CMake isolés :

```bash
(
  cd morsehgp3d
  cmake --workflow --preset cpu-release
  cmake --workflow --preset sanitizer
  cmake --workflow --preset cuda-release
  cmake --workflow --preset cuda-audit
)
```

Les deux premiers sont qualifiables sans CUDA. Les deux derniers exigent explicitement NVIDIA CUDA 12.9.x et produisent uniquement du code réel AOT pour `sm_120`; ils refusent PTX, fast math, architecture brute, fichier d'options ou injection par l'environnement. L'image associée est `containers/cuda12.9-sm120.Dockerfile`, épinglée par digest et snapshot Ubuntu.

Les workflows CUDA construisent une sonde de compilation, le runtime JSONL `morsehgp3d_phase3_runtime` et le module nanobind `morsehgp3d_phase3`. Le runtime utilise CCCL/CUB, DLPack et NVTX dans une arène `cudaMallocAsync` unique et bornée; il sépare les mesures `warm` et `resident`, interdit toute compilation pendant ces mesures, compare le résultat bit à bit puis à un oracle CPU indépendant par vecteur, somme et hash, et convertit les erreurs CUDA attendues en records structurés. Les manifests et records ont un schéma fermé avec horodatages et durées vérifiés. Les checkouts DLPack et nanobind doivent conserver à la fois leur commit épinglé et un arbre propre. Le module Python expose uniquement une qualification d'environnement et une sonde DLPack sans copie; chaque capsule porte un contexte de propriété privé vérifié avant toute lecture du tenseur. Aucun artefact ne porte `public_status`; ils fixent `scientific_result_claimed=false` et `scientific_public_status=null`.

La qualification réelle doit passer par `gcp-migration/run_phase3_qualification.sh`, qui délègue le calcul au worker invité puis certifie la cible exacte `TERMINATED`. Les preuves et limites de ce lot sont consignées dans `docs/validation/PHASE3_PROGRESS.md`; le test G4 réel reste requis avant la fermeture de la Phase 3.

## Replay d'un prédicat

Le wrapper Python valide un artefact d'entrée séparé du schéma scientifique public, appelle le binaire C++ et produit un résultat JSON canonique muni d'un identifiant SHA-256 stable de l'entrée normalisée :

```bash
python morsehgp3d/tools/replay_predicate.py \
  morsehgp3d/tests/fixtures/predicates/distance_one_ulp.json \
  --executable build/morsehgp3d/morsehgp3d_replay_predicate
```

Cet identifiant ne couvre ni le résultat ni le binaire et n'est donc pas un certificat public. Le replay conserve les bits de zéro signé à des fins diagnostiques, tout en canonisant leur valeur géométrique. Une coordonnée non finie, un prédicat inconnu, une sortie native non canonique ou incohérente, tout diagnostic natif inattendu sur `stderr`, et un binaire explicitement demandé mais introuvable échouent fermés.

Le schéma diagnostique v1 reste actif pour les prédicats historiques à nombre fixe de points. Le schéma v2 ajoute `power_bisector_side` avec table de points, labels de 1 à 10 identifiants triés et uniques, et témoin `ExactRational3`; les deux labels doivent avoir le même cardinal afin que le terme quadratique s'annule exactement. Lorsque les trois coordonnées rationnelles du témoin sont exactement représentables en binary64, le replay reconstruit cette provenance et exerce la cascade adaptative; sinon il conserve le chemin rationnel multiprécision. Un signe négatif signifie que le coût de `R` est inférieur à celui de `Q`.

Le schéma v3, dans le domaine SHA-256 distinct `MorseHGP3D/predicate-replay-v3/`, ajoute `plane_through_points`, `power_bisector_affine_form`, `orientation_2d_in_plane`, `intersect_three_planes` et `fourth_plane_incidence`. Un plan imbriqué est un objet fermé `ExactPlane3` `2.0.0` dont les chaînes entières doivent déjà être primitives et canoniques. Le wrapper recalcule les plans, coefficients exacts de `H_RQ`, orientations et intersections avec `Fraction`; son élimination de Gauss est indépendante des mineurs et de la règle de Cramer du C++.

Le schéma v4 utilise le domaine séparé `MorseHGP3D/predicate-replay-v4/` et ajoute `circumcenter_support` pour deux à quatre points. Sa sortie fixe publie la dimension et la classe du support, puis soit un centre homogène et un `ExactLevel`, soit deux témoins `null`. Le wrapper résout indépendamment le système de Gram par RREF sur `Fraction`, tandis que le C++ utilise les plans médiateurs et la règle de Cramer du noyau affine.

Le schéma v5 utilise `MorseHGP3D/predicate-replay-v5/`. `circumcenter_support_analysis` accepte un à quatre points et ajoute coordonnées, signes, classe d'enveloppe convexe, statut local et indices du support réduit. `sphere_side` reçoit un centre rationnel canonique, un `ExactLevel` et un point binary64. Le wrapper recalcule les barycentriques et le côté de sphère avec `Fraction`; ni l'un ni l'autre ne représente une preuve de complétude `RelevantGP`.

Le schéma v6 utilise `MorseHGP3D/predicate-replay-v6/`. `compare_exact_levels` reçoit deux records `ExactLevel` fermés et rejoue leur produit croisé exact. `canonical_level_batches` accepte des niveaux, supports minimaux et supports sources dans un ordre arbitraire, puis publie les lots, provenances et multiplicités canoniques. Le wrapper recalcule indépendamment l'ordre avec `Fraction` et refuse notamment les fractions irréduites, les identifiants hors du domaine JSON exact, les supports incohérents et un même support minimal annoncé à deux niveaux.

Le schéma v7 utilise `MorseHGP3D/predicate-replay-v7/`. `binary64_barycentric_coordinates`, `binary64_circumcenter_support_analysis`, `support_sphere_side` et `compare_support_levels` conservent leurs supports binary64 et exposent l'étage collectif ou scalaire qui a certifié le signe. Le wrapper vérifie indépendamment les coordonnées affines, résout le centre circonscrit par un système bordé RREF sur `Fraction`, recalcule la puissance signée et compare les niveaux rationnels. Les commandes de sphère et de comparaison directe portent sur les sphères circonscrites de leurs supports; elles ne revendiquent pas à elles seules la minimalité d'un miniball ambiant.

Le schéma v8 utilise `MorseHGP3D/predicate-replay-v8/`. `adaptive_plane_side`, `adaptive_orientation_2d_in_plane`, `adaptive_intersect_three_planes` et `adaptive_fourth_plane_incidence` reçoivent des sources affines fermées distinguées par `source_kind` : coefficients binary64, trois points binary64, bisecteur de puissance binary64 ou plan exact sans provenance. Les trois premières origines exercent la cascade; la dernière vérifie le fallback rationnel. Le wrapper recalcule directement les recettes avec `Fraction`, évalue le côté d'un plan, classe les rangs par RREF et évalue le quatrième plan au point d'intersection, sans reprendre les polynômes homogènes, les mineurs ou la règle de Cramer du C++. Les trois plans liants doivent avoir une intersection unique pour la commande d'incidence.

Le wrapper accepte aussi `--executable-prefix-argument=ARG` de façon répétable. L'argument est transmis avant le nom du prédicat natif, sans shell; le différentiel l'utilise pour activer `--multiprecision-only` et pour lancer ses faux natifs Python de manière portable.

Le wrapper et le binaire de replay sont des outils diagnostiques locaux, pas des services pour entrées hostiles. La taille des entiers exacts et des lignes batch reste volontairement non bornée afin de préserver le contrat multiprécision; une intégration réseau doit imposer ses propres quotas avant de les invoquer.

Le binaire accepte aussi un flux batch, une commande par ligne, afin que les différentiels n'ouvrent pas un processus par décision :

```bash
python morsehgp3d/tests/differential/check_predicate_random.py \
  build/morsehgp3d/morsehgp3d_replay_predicate
```

Le sous-lot 2A.9(1) ajoute un chemin compact destiné aux grandes campagnes de signes :

```bash
build/morsehgp3d/morsehgp3d_replay_predicate \
  --decision-only --batch < predicate-lines.txt
```

Ce mode accepte uniquement `compare_squared_distances`, `orientation_3d` et `power_bisector_side`. Chaque ligne de sortie est un objet JSON canonique fermé réduit à `certification_stage`, `counters`, `predicate` et `sign`; aucun niveau, déterminant ou témoin rationnel multiprécision n'est sérialisé. Une invocation forme une transaction : toutes ses lignes sont certifiées avant publication, et une erreur tardive ne laisse aucun préfixe sur la sortie standard. La transaction conserve en mémoire la sortie compacte du chunk; l'appelant doit donc toujours imposer une taille de chunk bornée. Les commandes, schémas et sorties riches des replays v1 à v8 restent inchangés et demeurent la voie de diagnostic. Le différentiel CTest rejoue 14 cas ciblés contre l'oracle `Fraction`, puis avec `--multiprecision-only`; il couvre les trois signes de chaque famille, les trois étages, les zéros exacts, les replis fermés et l'identité des sentinelles riches. Cette qualification de l'interface et le checkpoint de niveaux 2A.9(2) ne constituent pas encore la campagne de fermeture à $10^7$ signes.

Le sous-lot 2A.9(2) fournit le coordinateur local des runs triés de niveaux exacts :

```bash
python morsehgp3d/tools/level_run_checkpoint.py \
  --native build/morsehgp3d/morsehgp3d_replay_predicate \
  --input level-emissions.jsonl \
  --checkpoint-dir checkpoint-levels \
  --chunk-size 31
```

L'entrée JSONL contient une émission fermée et canonique par ligne : `kind=morsehgp3d_phase2a_level_emission`, `schema_version=1.0.0`, un `ExactLevel` v2, un support minimal et son support source. La taille de chunk est comprise entre 1 et 64; chaque chunk passe par la commande native `canonical_level_batches` avant publication atomique d'un run checksummé. Le merge ne commence qu'après gel de tous les runs. Il ferme ensuite un niveau seulement après consommation de toutes ses occurrences dans toutes les têtes de runs, agrège globalement provenances et doublons, puis publie ensemble l'artefact de niveau et les nouveaux curseurs. `--max-transactions 1` permet de s'arrêter proprement après exactement un run, un niveau fermé ou la finalisation; `--max-transactions 0` crée ou valide seulement le bootstrap.

Le manifeste `morsehgp3d_phase2a_level_checkpoint` v1 est un format opérationnel interne fermé. Il lie les hashes de l'entrée, de la configuration, de l'outil, du binaire et de chaque artefact, conserve le catalogue figé, les curseurs et le dernier niveau entièrement fermé, et porte `public_contract_claimed=false`. Il n'est ni le `$defs/CheckpointManifest` ni le `$defs/EqualLevelBatch` du contrat public v2. Le résultat durable est adressé par contenu sous `results/`; `result.json` n'est créé qu'après le manifeste `complete`. Une coupure après publication d'un run, d'un niveau ou du résultat laisse l'ancien manifeste autoritaire et rejoue la transaction entière.

Ce coordinateur impose un verrou mono-écrivain et exige POSIX pour `flock` et la synchronisation des répertoires. Sa validation exhaustive privilégie la preuve de reprise à la vitesse et garde en mémoire l'index global support–niveau ainsi que le niveau ouvert. Il qualifie la transaction locale de Phase 2A, pas encore le streaming budgété des phases 15 et 18, ni Apple, Windows ou ARM.

La campagne de fermeture des signes utilise un second coordinateur interne, indépendant du format des niveaux :

```bash
python morsehgp3d/tools/predicate_campaign.py \
  --config morsehgp3d/tests/campaigns/phase2a_predicates_v1.json \
  --repository "$(pwd)" \
  --precompute-roots /tmp/morsehgp3d-phase2a-roots.json
```

Ce premier passage ne lance aucun prédicat natif. Sur un commit propre, il dérive les racines ordonnées des commandes et des signes de l'oracle entier dyadique pour 10 800 000 cas de base. Ces racines doivent ensuite remplacer les deux valeurs `null` de la configuration et être commitées avant la campagne certifiante; le mode production refuse une racine absente ou divergente. Le générateur `splitmix64-counter-v2` exige qu'un cas `well_conditioned` ait un signe strict et régénère de façon déterministe et bornée toute égalité fortuite; la fixture `campaign_well_conditioned_zero_regression.json` conserve le contre-exemple minimal qui avait révélé `R=Q` dans la version précédente. Les 10 044 000 cas bien conditionnés pseudo-aléatoires dépassent ainsi à eux seuls le seuil de $10^7$; les sept strates adversariales ajoutent 756 000 cas construits. Les dérivés métamorphiques sont comptés séparément et ne contribuent jamais à ce seuil.

Après gel et commit des racines, une transaction native se lance ainsi :

```bash
python morsehgp3d/tools/predicate_campaign.py \
  --native build/morsehgp3d/morsehgp3d_replay_predicate \
  --config morsehgp3d/tests/campaigns/phase2a_predicates_v1.json \
  --checkpoint-dir /tmp/morsehgp3d-phase2a-campaign \
  --repository "$(pwd)" \
  --max-chunks 1
```

La configuration ferme trois prédicats, huit strates, les cardinalités `H_RQ` de 1 à 10, les dénominateurs de témoin 1, 2, 3, 5, 7 et 8, et cinq transformations exactes : permutation signée propre, réflexion impropre, translation dyadique non nulle, homothétie par puissance de deux et symétrie propre au prédicat. Un chunk contient au plus 8 800 cas de base et 9 691 sorties natives; entrées et sorties transitent par fichiers temporaires, tandis que seuls les agrégats et hashes sont durables. Le binaire, le cache CMake, le fichier `flags.make` effectif de la cible, le compilateur, les options, l'outil, la configuration et le commit propre sont revérifiés avant et après chaque chunk. Toute divergence publie d'abord son entrée originale, puis tente une réduction automatique des coordonnées, bits, témoin et labels. Le certificat distingue `scope=smoke` de `scope=production`, garde `repository_phase_exit_claimed=false` et `gcp_used=false`, et ne décide jamais à lui seul la fermeture du registre.

`--max-chunks 1` ci-dessus n'exécute qu'une transaction reprenable. La campagne qualifiante a utilisé une borne de 2 000 pour terminer ses 1 228 chunks et publie ses octets canoniques dans `tests/campaigns/phase2a_predicates_v1.result.json`, de SHA-256 `57627bdb68662234ea1d1716d06e8c8abbb4cc828b2353fdf763cde64df54ad8`. Elle certifie 10 800 000 bases et 1 080 000 dérivés, avec `wrong_sign=0` et `remaining_unknown=0`. La [revue 2A.10](../docs/validation/PHASE2A_GATE_REVIEW.md) a évalué `G1=go` et porte la décision de fermeture du registre; le certificat seul conserve `repository_phase_exit_claimed=false`.

La suite courte traite 2 048 cas déterministes, publie le hash du corpus, force chacun des trois étages sur les trois familles adaptatives, vérifie chaque sortie contre `Fraction`, puis rejoue le même flux avec `--multiprecision-only`. Les paramètres `--distance-cases`, `--orientation-cases` et `--power-cases` permettent d'augmenter ce volume; cette infrastructure riche ne constitue pas encore la campagne de fermeture à dix millions de signes.

Le différentiel 2A.8b ajoute 4 050 commandes déterministes pour les quatre familles v7. Il couvre les tailles de support un à quatre, force les signes stricts aux intervalles, les trois signes aux expansions et les trois signes aux replis multiprécision, puis exige les mêmes témoins scientifiques avec la cascade désactivée. Trente fixtures permanentes, 288 relations métamorphiques et six sentinelles d'identité v1 à v6 complètent ce corpus; cette qualification locale ne remplace pas davantage la campagne de fermeture 2A.9.

Le différentiel 2A.8c ajoute 5 120 commandes déterministes pour les quatre familles v8. Son hash de commandes est `63d22e4daf8417de218b98101b49fcedf501c0557eaff7a2b251a17c07bed5eb` et celui des réponses de l'oracle `03dfcacbd777b2debaebd802a86f356fa7e34948ba04ef43e3d748bbdc2fabdc`. Chaque famille force les signes négatif et positif au filtre FP64, les signes négatif, nul et positif à l'expansion et les trois signes au fallback multiprécision, y compris avec toutes les sources munies de leur provenance rapide. Un système à provenance mixte conserve en plus un point d'intersection et une incidence signée de dénominateur trois sous autorité multiprécision. Le corpus couvre les quatre origines, des plans obliques, un bisecteur multi-point, tous les couples de rang observables pour trois plans propres, 48 groupes métamorphiques et 120 relations. Les projections scientifiques du mode adaptatif et de `--multiprecision-only` sont identiques. Trente-sept fixtures permanentes de manifeste `20649770c099e9c4fca6837af0ea86af0b8e7399d82c786e4ed532941727cf49`, sept sentinelles v1 à v7 et les rejets fermés de schéma, commandes natives et sorties falsifiées complètent ce sous-lot; la campagne de dix millions de signes reste réservée à 2A.9.

Le mode indépendant est également disponible directement :

```bash
build/morsehgp3d/morsehgp3d_replay_predicate \
  --multiprecision-only --batch < predicate-lines.txt
```

Le batch riche historique reste un flux : une erreur tardive peut suivre des lignes déjà écrites. Un consommateur ne valide donc jamais ces lignes avant d'avoir observé la fin du processus avec un code nul. Toute erreur de lecture ou d'écriture du flux produit un échec fermé.
