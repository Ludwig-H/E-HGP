# MorseHGP3D — cœur certifié

Ce répertoire porte la nouvelle implémentation décrite par la roadmap. `HGP-old/` reste une référence historique séparée et n'est pas importée par ce cœur.

État actuel : Phase 2A, backend `reference_cpu`, couche numérique commune aux profils, mode `certified`.

La tranche actuellement intégrée fournit :

- l'interprétation exacte des coordonnées binary64 comme dyadiques;
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
- des filtres d'intervalles FP64 conservateurs pour la comparaison de distances et l'orientation 3D;
- une politique par appel permettant de désactiver ces filtres pour le différentiel multiprécision;
- un replay diagnostique versionné à partir des mots binary64 et des plans exacts d'entrée.

Cette tranche clôt les sous-lots affines 2A.4, centres 2A.5, minimalité locale 2A.6 et ordre exact 2A.7, mais ne ferme ni la Phase 2A ni G1. Les comparaisons de distances et orientations 3D bien conditionnées peuvent être certifiées par `fp64_filtered`; toute borne contenant zéro, tout overflow ou environnement flottant non conforme revient à `cpu_multiprecision`. Les prédicats affines, les constructions de centres, les barycentriques, les côtés de sphère et l'ordre des niveaux restent entièrement multiprécision. Les expansions et la cascade adaptative complète seront ajoutées par les lots testables suivants.

Le filtre exige IEEE binary64, l'arrondi au plus proche dans le FENV et MXCSR sur x86, les sous-normaux actifs et les options strictes exportées par la cible CMake. Chaque opération d'intervalle est élargie vers les deux infinis; les exceptions flottantes du processus appelant sont restaurées. `PredicateFilterPolicy::multiprecision_only` garde un chemin de décision indépendant. Dans les API riches et le replay, un témoin rationnel reste matérialisé même si le signe a été certifié en FP64 : `certification_stage` désigne l'autorité du signe, pas le coût du diagnostic.

La convention d'orientation est exactement `det([b-a, c-a, d-a])` : le quadruplet `(0, e1, e2, e3)` est positif. Cette définition explicite prévaut sur les conventions de signe parfois opposées d'autres bibliothèques.

Un plan orienté représente `a*x + b*y + c*z + d = 0`. Ses quatre coefficients entiers sont divisés par leur PGCD positif, sans imposer le signe du premier coefficient : multiplier une entrée par un facteur positif conserve le plan orienté, tandis qu'une négation inverse son orientation. `unoriented_key()` choisit séparément un signe canonique pour identifier le plan géométrique. Une normale nulle est toujours rejetée.

`ExactAffineForm3` conserve les coefficients rationnels et leur échelle exacte. Sa clé orientée est seulement le représentant homogène primitif à facteur positif, utilisé pour classifier le lieu zéro; elle ne remplace pas la valeur exacte de la forme. L'orientation 2D utilise le signe de `n dot ((b-a) cross (c-a))` pour la normale primitive orientée `n` du support. Cette valeur n'est pas appelée aire, car `n` n'est pas normalisée.

Une intersection unique expose un `ExactRational3`, les rangs normal et augmenté égaux à 3 et une dimension affine égale à 0. Une famille cohérente expose la dimension `3 - normal_rank`; un système incohérent n'expose ni point ni dimension. Le replay conserve les trois plans liants dans l'entrée normalisée couverte par son identifiant, de sorte que les coordonnées seules ne sont jamais présentées comme certificat autonome.

`ExactCenter3` est un alias sémantique de `ExactRational3` : le centre conserve ainsi un dénominateur commun strictement positif et une réduction canonique déjà couverte par le contrat `2.0.0`. `CircumcenterResult` associe atomiquement ce centre et son `ExactLevel`. Une paire distincte utilise son milieu; un triangle indépendant intersecte son plan affine et deux plans médiateurs; un tétraèdre indépendant intersecte trois plans médiateurs. Chaque résultat est ensuite revérifié par égalité exacte des distances aux points du support.

Un support dépendant expose sa dimension affine exacte et des témoins `null`; il n'est jamais complété par un centre arbitraire. `analyze_circumcenter_support` calcule les barycentriques exacts du centre d'un support indépendant. Un centre intérieur rend le support localement minimal; un centre sur la frontière conserve seulement les coefficients strictement positifs, puis le centre, le niveau et l'intérieur relatif réduits sont revérifiés exactement. Un centre extérieur reste explicitement `exterior_circumcenter` et ne déclenche aucune recherche implicite parmi les sous-supports.

Le support singleton a pour centre le point lui-même, dimension affine zéro et `ExactLevel` nul. `classify_sphere_point` compare exactement la distance carrée d'un point au niveau d'une sphère et expose simultanément le signe, la classe et le décalage rationnel signé. Ces décisions sont locales : elles n'énumèrent pas les candidats miniball, ne complètent pas `RelevantGP` et ne suffisent jamais à produire un statut public `exact`.

`compare_exact_levels` matérialise le témoin entier `N_left * D_right - N_right * D_left`; son signe est l'ordre scientifique et zéro est la seule égalité admise. Les comparateurs purs employés par le tri ne modifient aucun compteur, car leur nombre d'appels dépend de l'implémentation de la bibliothèque standard. Chaque comparaison scientifique explicite enregistre en revanche exactement une décision `cpu_multiprecision`.

`canonical_level_batches` reçoit seulement des items de support déjà vérifiés. Il trie les lots par niveau exact puis les supports par cardinalité minimale et identifiants canoniques, agrège séparément les supports sources et compte les émissions identiques. Une même clé de support minimal associée à deux niveaux est une contradiction fermée. Cette canonisation ne recherche aucun sous-support, ne vérifie l'enveloppe d'aucun ensemble ambiant et ne remplace ni un `CriticalEvent` public ni un `EqualLevelBatch` contractuel complet.

## Construction locale

```bash
cmake -S morsehgp3d -B build/morsehgp3d -DMORSEHGP3D_BUILD_TESTS=ON
cmake --build build/morsehgp3d --parallel
ctest --test-dir build/morsehgp3d --output-on-failure
```

Le target exporté est `morsehgp3d::exact`. Après `cmake --install`, un consommateur peut utiliser `find_package(MorseHGP3D CONFIG REQUIRED)` sans dépendre de chemins d'en-têtes propres à la machine de construction.

## Replay d'un prédicat

Le wrapper Python valide un artefact d'entrée séparé du schéma scientifique public, appelle le binaire C++ et produit un résultat JSON canonique muni d'un identifiant SHA-256 stable de l'entrée normalisée :

```bash
python morsehgp3d/tools/replay_predicate.py \
  morsehgp3d/tests/fixtures/predicates/distance_one_ulp.json \
  --executable build/morsehgp3d/morsehgp3d_replay_predicate
```

Cet identifiant ne couvre ni le résultat ni le binaire et n'est donc pas un certificat public. Le replay conserve les bits de zéro signé à des fins diagnostiques, tout en canonisant leur valeur géométrique. Une coordonnée non finie, un prédicat inconnu, une sortie native non canonique ou incohérente, tout diagnostic natif inattendu sur `stderr`, et un binaire explicitement demandé mais introuvable échouent fermés.

Le schéma diagnostique v1 reste actif pour les prédicats historiques à nombre fixe de points. Le schéma v2 ajoute `power_bisector_side` avec table de points, labels de 1 à 10 identifiants triés et uniques, et témoin `ExactRational3`; les deux labels doivent avoir le même cardinal afin que le terme quadratique s'annule exactement. Un signe négatif signifie que le coût de `R` est inférieur à celui de `Q`.

Le schéma v3, dans le domaine SHA-256 distinct `MorseHGP3D/predicate-replay-v3/`, ajoute `plane_through_points`, `power_bisector_affine_form`, `orientation_2d_in_plane`, `intersect_three_planes` et `fourth_plane_incidence`. Un plan imbriqué est un objet fermé `ExactPlane3` `2.0.0` dont les chaînes entières doivent déjà être primitives et canoniques. Le wrapper recalcule les plans, coefficients exacts de `H_RQ`, orientations et intersections avec `Fraction`; son élimination de Gauss est indépendante des mineurs et de la règle de Cramer du C++.

Le schéma v4 utilise le domaine séparé `MorseHGP3D/predicate-replay-v4/` et ajoute `circumcenter_support` pour deux à quatre points. Sa sortie fixe publie la dimension et la classe du support, puis soit un centre homogène et un `ExactLevel`, soit deux témoins `null`. Le wrapper résout indépendamment le système de Gram par RREF sur `Fraction`, tandis que le C++ utilise les plans médiateurs et la règle de Cramer du noyau affine.

Le schéma v5 utilise `MorseHGP3D/predicate-replay-v5/`. `circumcenter_support_analysis` accepte un à quatre points et ajoute coordonnées, signes, classe d'enveloppe convexe, statut local et indices du support réduit. `sphere_side` reçoit un centre rationnel canonique, un `ExactLevel` et un point binary64. Le wrapper recalcule les barycentriques et le côté de sphère avec `Fraction`; ni l'un ni l'autre ne représente une preuve de complétude `RelevantGP`.

Le schéma v6 utilise `MorseHGP3D/predicate-replay-v6/`. `compare_exact_levels` reçoit deux records `ExactLevel` fermés et rejoue leur produit croisé exact. `canonical_level_batches` accepte des niveaux, supports minimaux et supports sources dans un ordre arbitraire, puis publie les lots, provenances et multiplicités canoniques. Le wrapper recalcule indépendamment l'ordre avec `Fraction` et refuse notamment les fractions irréduites, les identifiants hors du domaine JSON exact, les supports incohérents et un même support minimal annoncé à deux niveaux.

Le wrapper accepte aussi `--executable-prefix-argument=ARG` de façon répétable. L'argument est transmis avant le nom du prédicat natif, sans shell; le différentiel l'utilise pour activer `--multiprecision-only` et pour lancer ses faux natifs Python de manière portable.

Le wrapper et le binaire de replay sont des outils diagnostiques locaux, pas des services pour entrées hostiles. La taille des entiers exacts et des lignes batch reste volontairement non bornée afin de préserver le contrat multiprécision; une intégration réseau doit imposer ses propres quotas avant de les invoquer.

Le binaire accepte aussi un flux batch, une commande par ligne, afin que les différentiels n'ouvrent pas un processus par décision :

```bash
python morsehgp3d/tests/differential/check_predicate_random.py \
  build/morsehgp3d/morsehgp3d_replay_predicate
```

La suite courte traite 2 048 cas déterministes, publie le hash du corpus, vérifie chaque sortie contre `Fraction`, puis rejoue le même flux avec `--multiprecision-only`. Les paramètres `--distance-cases`, `--orientation-cases` et `--power-cases` permettent d'augmenter ce volume; cette infrastructure riche ne constitue pas encore la campagne de fermeture à dix millions de signes.

Le mode indépendant est également disponible directement :

```bash
build/morsehgp3d/morsehgp3d_replay_predicate \
  --multiprecision-only --batch < predicate-lines.txt
```

Le mode batch est un flux : une erreur tardive peut suivre des lignes déjà écrites. Un consommateur ne valide donc jamais ces lignes avant d'avoir observé la fin du processus avec un code nul. Toute erreur de lecture ou d'écriture du flux produit un échec fermé.
