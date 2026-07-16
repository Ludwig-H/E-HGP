# Revue de porte Phase 2A — clôture des prédicats exacts CPU

> [!IMPORTANT]
> Cette revue ferme la Phase 2A et décide `G1=go` pour le laboratoire de prédicats exacts CPU. Elle certifie la couche numérique commune aux profils, sur le backend `reference_cpu` et en mode `certified`. Elle ne ferme ni G2, ni G3, ni G4, ne prouve pas `full_pi0` et ne promeut aucun résultat MorseHGP3D public vers `public_status=exact`.

## Décision

- Phase : 2A — laboratoire de prédicats exacts CPU.
- Backend qualifié : `reference_cpu`.
- Profils : couche numérique commune, avec priorité d'intégration `hgp_reduced`; aucune preuve nouvelle n'est apportée à `full_pi0`.
- Mode : `certified`.
- Porte d'entrée : satisfaite par les Phases 0 et 1 fermées.
- Verdict de sortie : satisfait.
- Porte scientifique : **G1 — `go`**. Aucun filtre n'a certifié un mauvais signe sur le périmètre T1 effectivement livré par la Phase 2A; les indécisions sont transmises à l'étage suivant et `remaining_unknown=0` au terme de la campagne qualifiante.
- Effet opérationnel attendu dans le même commit de fermeture : Phase 2A `completed`, Phase 3 `in_progress`, sous-phase de recherche 17A `ready`; la Phase 2B reste `blocked` par la seule Phase 3 tant que l'environnement CUDA G4 n'est pas fermé.

La décision G1 ne repose ni sur un temps moyen, ni sur un taux favorable de filtrage, ni sur une sortie plausible. Elle repose sur les témoins exacts, les replays indépendants, les tests négatifs, les régressions permanentes et le certificat de campagne. Le certificat conserve honnêtement `repository_phase_exit_claimed=false` : le runner fournit une preuve de campagne, tandis que la présente revue porte la décision de dépôt.

## Matrice des familles T1 livrées

Pour cette revue, « tout T1 » suit le périmètre explicité dans le plan de test : toutes les décisions numériques signées et toutes les constructions exactes effectivement livrées par le laboratoire CPU. Les requêtes spatiales globales sont traitées séparément dans les limites ci-dessous.

| famille | API et témoin certifié | replay ou oracle indépendant | tests positifs, négatifs et dégénérés | verdict |
|---|---|---|---|---|
| coordonnées binary64, rationnels et niveaux exacts | conversion par bits, rationnels canoniques, `ExactRational3` et `ExactLevel` homogène à dénominateur positif | dump contractuel et différentiel de types exacts | zéros signés, sous-normaux, exposants extrêmes, grands entiers, sérialisation canonique et entrées invalides | couvert |
| comparaison de distances carrées | décision adaptative et résultat riche avec différence rationnelle exacte | replay v1, oracle `Fraction`, batch décisionnel et campagne massive | signes négatif, nul et positif, annulations, un ULP, overflow ou underflow fermé, politiques adaptative et multiprécision seule | couvert |
| signe de `H_RQ` | moments exacts de labels, forme affine orientée, témoin rationnel et provenance binary64 vérifiée | replay v2, oracle entier dyadique recoupé par `Fraction`, batch et campagne massive | cardinalités 1 à 10, dénominateurs 1, 2, 3, 5, 7 et 8, recouvrements de labels, formes propres, constantes et égalités exactes | couvert |
| orientations 3D et 2D dans un support | déterminant exact, cascade FP64–expansion–multiprécision et recette du plan vérifiée | replays v1 et v8, oracles `Fraction` et déterminantaux, batch 3D et campagne massive | trois signes, quasi-coplanarité, permutations, réflexions, sous-normaux, FTZ/DAZ et modes d'arrondi | couvert |
| côté d'un plan affine | forme exacte orientée, provenance fermée et témoin rationnel | replays v3 et v8, oracle `Fraction` direct | trois signes aux étages disponibles, plans par coefficients, trois points ou bisecteur de puissance, sources exactes sans provenance et faux témoins rejetés | couvert |
| rangs, intersection de trois plans et incidence d'un quatrième | rangs normal et augmenté, résultat `unique`, `empty` ou `affine_family`, centre rationnel et déterminant d'incidence | replays v3 et v8, élimination de Gauss et RREF indépendantes | tous les couples de rang possibles, intersections uniques, droites, plans et systèmes vides, permutations, inversions et sorties falsifiées rejetées | couvert |
| centres circonscrits de supports de taille un à quatre | centre rationnel exact, dimension affine, rayon carré homogène et statut de dépendance | replays v4 et v5, oracles Gram et RREF indépendants | singleton, paires, triangles et tétraèdres, supports dépendants, centres non dyadiques, permutations et frontières à un ULP | couvert |
| signes barycentriques, intérieur relatif et réduction de support | coefficients rationnels, classification locale et support minimal revérifié | replays v5 et v7, oracles RREF et Gram/Cramer indépendants | `minimal`, `boundary_reduced`, `exterior_circumcenter`, `affinely_dependent`, tailles un à quatre et rejets fermés | couvert |
| intérieur, frontière ou extérieur d'une sphère | puissance signée exacte, centre et niveau rationnels canoniques | replays v5 et v7, oracle `Fraction` indépendant | trois classes, rayon nul, niveau rationnel, quasi-cosphéricité à un ULP, permutations et trois étages disponibles | couvert |
| comparaison de rayons et égalité de niveaux | signe exact du produit croisé, témoin rationnel et provenance de support | replays v6 et v7, oracle `Fraction` indépendant | égalités issues de supports distincts, produits croisés `-1` et `1`, grands entiers, niveaux presque égaux et mode multiprécision seul | couvert |
| ordre, déduplication, lots de niveaux et reprise | tie-break canonique, provenances triées, lots exacts et dernier niveau entièrement fermé | replay v6 et coordinateur `level_run_checkpoint.py` | permutations, doublons, contradictions support–niveau, 1, 2, 3, 7 et 31 runs, cinq coupures injectées et résultat adressé par contenu | couvert |

Les replays fermés v1 à v8 conservent leurs sentinelles historiques. Les API riches matérialisent le témoin exact et refusent toute contradiction entre ce témoin et un signe rapide. Le batch `decision-only-batch-v1`, volontairement limité aux trois prédicats massifs, ne sérialise pas de témoin bigint, mais il est comparé aux mêmes API riches et à l'oracle indépendant sur son corpus de qualification.

## Campagne qualifiante 2A.9

La campagne versionnée `phase2a_predicates_v1` a exécuté 1 228 chunks reprenables sur un arbre propre lié au commit `4c64d0c2788e487bf7fd2e7ae00d3b481e3656d6`. Elle contient 10 800 000 cas de base et 1 080 000 dérivés métamorphiques, soit 11 880 000 décisions natives. Les dérivés ne contribuent pas au seuil pseudo-aléatoire : les 10 044 000 bases strictement non nulles de la seule strate `well_conditioned` dépassent à elles seules $10^7$; les 756 000 bases adversariales sont additionnelles.

### Verdict et répartition

| mesure | valeur certifiée |
|---|---:|
| cas de base | 10 800 000 |
| bases pseudo-aléatoires `well_conditioned` | 10 044 000 |
| bases adversariales | 756 000 |
| dérivés métamorphiques | 1 080 000 |
| décisions natives totales | 11 880 000 |
| mauvais signes | **0** |
| décisions inconnues restantes | **0** |

Chaque prédicat massif totalise 3 960 000 décisions : comparaison de distances carrées, orientation 3D et signe de `H_RQ`. Chacune des 24 cellules prédicat–strate est non vide. Pour chaque prédicat, la strate `well_conditioned` compte 3 456 000 décisions après dérivation et chacune des sept strates adversariales en compte 72 000.

| étage terminal | décisions |
|---|---:|
| `fp64_filtered` | 9 192 000 |
| `expansion` | 696 000 |
| `cpu_multiprecision` | 1 992 000 |

| signe | décisions |
|---|---:|
| négatif | 5 832 357 |
| positif | 5 831 643 |
| nul | 216 000 |

Les 216 000 zéros exacts proviennent de la strate dédiée aux égalités. Les autres strates couvrent annulation, sous-normaux, exposants extrêmes, grands décalages, quasi-coplanarité et quasi-cosphéricité. Tous les étages et les trois signes sont donc observés; leur fréquence reste diagnostique et n'entre pas dans la décision de correction.

### Métamorphismes

| transformation exacte | relation attendue | cas vérifiés |
|---|---|---:|
| permutation signée propre des axes | signe conservé | 391 500 |
| réflexion signée impropre | orientation opposée si non nulle, zéro préservé; autres prédicats conservés | 499 500 |
| translation dyadique non nulle exactement représentable | signe conservé | 67 500 |
| homothétie exacte par puissance de deux | signe conservé | 54 000 |
| symétrie des arguments propre au prédicat | signe opposé | 67 500 |

Les relations métamorphiques totalisent 882 000 signes conservés et 198 000 signes opposés. Les transformations vérifient l'exacte représentabilité avant d'interroger le natif; elles ne masquent donc pas un arrondi d'entrée.

## Racines, certificat et chaîne de commits

- `92df6fc` — infrastructure initiale : générateur indexé, oracle entier dyadique indépendant, chunks transactionnels, racines ordonnées, catalogue métamorphique et minimisation.
- `eebd859` — fermeture de la contradiction du générateur v1 : exclusion des zéros de la strate bien conditionnée, passage à `splitmix64-counter-v2`, fixture permanente et registre des preuves mis à jour.
- `4c64d0c` — gel de la configuration v2 et des deux racines attendues avant l'exécution native qualifiante.
- `6e0e70d` — publication des octets canoniques du certificat et des compteurs de la campagne longue.

Artefacts cryptographiques principaux :

| artefact | SHA-256 |
|---|---|
| certificat versionné `phase2a_predicates_v1.result.json` | `57627bdb68662234ea1d1716d06e8c8abbb4cc828b2353fdf763cde64df54ad8` |
| racine ordonnée du corpus | `76544b0c85c2f6602e560bd007762609e1feb080f5fe13a71ec4d0d3f31f294b` |
| racine ordonnée de l'oracle | `62a4f14bc4d25971d67d7c321bdea9091d45f7a17dc37d79a292fd34cd078210` |
| racine ordonnée des sorties natives | `729f06551fa4012e7ff3a97b871ba81faa9524c99c900c8616b7ed87c0de9079` |
| configuration gelée | `984bc1c54afa7e96e4a8f0bd486f73cb95ddde6304d2217893bb3207a67a6c5e` |
| outil de campagne | `f75c22cb6ecb376953a463ecaf7244b516e5b79e4836016e1d928cffa7139cc8` |
| exécutable natif | `fce527af88a6750ac719ffd082d9c2d28f3e78ae80bcdef21c534604cc433511` |
| métadonnées de build | `a5321f3e6697db96f5b8068b37df3151676bea86c54d93e50ef55413c29f9880` |

Le certificat enregistre GCC 13.3, le build Release, les options strictes `-fno-fast-math`, `-ffp-contract=off` et `-frounding-math`, le cache CMake, `flags.make`, la plateforme, Python 3.12.1, le commit initial et la propreté stable du dépôt. Il porte `gcp_used=false`.

## Reprise, échec fermé et minimisation

- Chaque chunk natif est borné, checksummé et publié atomiquement avant progression du manifeste. Le verrou POSIX empêche deux écrivains; les fichiers temporaires, hashes, curseurs, métadonnées de build et état Git sont revalidés avant et après les transactions.
- Les quatre coupures de campagne injectées couvrent publication de l'artefact de chunk, manifeste de chunk, certificat adressé par contenu et alias final. Une reprise conserve l'ancien manifeste autoritaire et ne publie jamais un préfixe comme résultat complet.
- Après la campagne, une reprise avec `--max-chunks 0` et une lecture indépendante des 1 228 artefacts ont reproduit le certificat sans nouvel appel natif.
- Le coordinateur de niveaux exerce séparément 95 reprises après runs, fermetures de niveaux et finalisation. Aucun niveau égal traversant une frontière de chunk n'est finalisé partiellement.
- Le smoke injecte une vraie divergence de signe dans un wrapper natif de test. Le cas original est publié durablement avant toute réduction, puis le minimiseur réduit automatiquement le repro tout en conservant la divergence. Aucune divergence native réelle n'est apparue dans la campagne longue; aucun nouveau contre-exemple numérique n'a donc été fabriqué.

### Régression permanente du générateur v1

Le premier pré-calcul a été arrêté avant publication de racines lorsqu'un contre-audit a trouvé, à l'indice 32, un cas `H_RQ` étiqueté `well_conditioned` avec `R=Q` et signe identiquement nul. Ce défaut invalidait le décompte expérimental des signes stricts, sans contredire l'implémentation du prédicat.

La fixture `morsehgp3d/tests/fixtures/predicates/campaign_well_conditioned_zero_regression.json` conserve ce contre-exemple minimal. Le générateur v2 impose des labels distincts et un resampling entier, déterministe et borné de toute nullité fortuite. Aucune racine v1 n'a été gelée; les racines publiées appartiennent exclusivement au corpus corrigé v2. Cette séquence satisfait la règle selon laquelle une contradiction devient une fixture et met à jour le registre des preuves avant toute reprise d'optimisation.

La fixture `orientation_daz_regression.json` reste également permanente : son déterminant exact positif avait été mal certifié sous DAZ par une ancienne sonde. Les tests actuels exigent l'indécision du filtre non conforme, le repli multiprécision, le signe exact et la restauration de l'environnement flottant.

## Qualification croisée et intégration

- GCC 13 Release strict : CTest **25/25**.
- Clang 18 Release strict : CTest **25/25**.
- GCC 13 Debug, ASan et UBSan : CTest **25/25**.
- Package CMake installé : binaire installé qualifié sur le batch décisionnel et les 95 reprises de niveaux; wrapper installé rejoué sur les familles riches v3 à v8.
- Consommateur externe par `find_package(MorseHGP3D CONFIG REQUIRED)` : **1/1**. Il exerce filtres, multiprécision, plans et provenances, intersection, incidence, centre, support, sphère, ordre et lots.
- Contrats : 21 définitions, 21 exemples de schéma et 5 fixtures validés; **58/58** tests contractuels.
- Oracle CPU exhaustif indépendant : **92/92** tests; campagne CI bornée dans les trois dimensions affines, trois cas audités et zéro échec.
- Documentation et cohérence : documentation active, cinq références locales, scope courant, registre des vingt phases et neuf modules d'oracle indépendant validés.
- Sécurité GCP : workflow ordinaire confirmé en lecture seule, syntaxe des scripts validée et **11/11** tests de garde réussis.

Les suites spécialisées qui ferment les familles non massives incluent leurs propres oracles `Fraction`, Gram, déterminants ou RREF, des entrées positives et négatives, les dégénérescences attendues, les faux témoins, les sorties non canoniques et les entrées mal typées. Elles complètent la campagne massive; elles ne sont pas remplacées par elle.

## Évaluation explicite de G1

| critère de porte | preuve | décision |
|---|---|---|
| zéro faux signe sur tout T1 livré par 2A | 11 880 000 décisions massives sans erreur; suites spécialisées et replays v1 à v8 sans désaccord | go |
| aucune branche choisie sur une indécision | cascade instrumentée, filtres stricts, replis expansion puis multiprécision, `remaining_unknown=0` | go |
| constructions exactes et témoins rejouables | centres, plans, rangs, supports, sphères et niveaux rationnels reconstruits par oracles indépendants | go |
| égalités, quasi-égalités et ordre canonique | produits croisés exacts, lots, déduplication, permutations et niveaux inter-chunks qualifiés | go |
| domaines adversariaux obligatoires | égalités, annulations, sous-normaux, exposants extrêmes, grands décalages, quasi-coplanarité et quasi-cosphéricité | go |
| reproductibilité et réduction des divergences | racines gelées, commit et build liés, reprise transactionnelle, original durable avant minimisation | go |
| package et consommateur | installation, binaire exporté et consommateur externe qualifiés | go |

**Verdict final : G1=go et porte de sortie de la Phase 2A satisfaite.** Le nombre de replis n'est pas une condition de ce verdict; toute régression future qui certifierait un seul mauvais signe rouvrirait immédiatement G1 en `no-go`.

## Limites de la décision

- La campagne massive porte sur les trois prédicats compatibles avec l'interface batch décisionnelle : distance, orientation 3D et `H_RQ`. Les autres familles T1 sont fermées par leurs suites spécialisées, replays riches, fixtures et oracles indépendants; elles ne sont pas artificiellement comptées dans les 11 880 000 décisions.
- Le top-$k$ global avec exclusions, le shell global, le rang spatial et le miniball d'un nuage complet appartiennent à l'oracle spatial et aux phases ultérieures. Ils restent soumis à G2, G3 et G4 et ne sont ni validés ni supposés présents par cette fermeture.
- G2 reste ouverte : aucune égalité exhaustive du catalogue spatial jusqu'à `n=14` n'est revendiquée ici.
- G3 reste ouverte : aucune hiérarchie globale nouvelle n'est comparée ici à Gamma, et la voie Gabriel brute reste partielle.
- G4 reste ouverte : aucune unicité ni commutation verticale globale n'est certifiée par le laboratoire de prédicats.
- `full_pi0` reste `conditional`; la preuve M.1 et les Phases 6, 12 et 13 restent ouvertes. La couche numérique commune ne transforme pas une hypothèse structurelle en théorème.
- Aucun `MorseHGP3DResult` public n'est produit par la Phase 2A et aucun `public_status=exact` n'est promu. Les statuts de proposition flottante, décision certifiée, réduction hiérarchique et publication restent séparés.
- Le backend GPU n'est pas qualifié. La Phase 2B devra comparer CPU et GPU sans différence et transmettre tout `unknown` GPU au CPU.
- Les validations locales couvrent GCC et Clang sur Linux x86_64. MSVC, Apple et ARM ne sont pas qualifiés par cette revue.
- Les performances et taux d'étages observés ne fondent aucune revendication interactive, scalable ou GPU.

## Artefacts de fermeture

- Point d'avancement détaillé : `docs/validation/PHASE2A_PROGRESS.md`.
- Configuration gelée : `morsehgp3d/tests/campaigns/phase2a_predicates_v1.json`.
- Certificat publié : `morsehgp3d/tests/campaigns/phase2a_predicates_v1.result.json`.
- Runner et reprise de campagne : `morsehgp3d/tools/predicate_campaign.py`.
- Coordinateur de niveaux : `morsehgp3d/tools/level_run_checkpoint.py`.
- Smoke, failpoints et minimisation : `morsehgp3d/tests/differential/check_predicate_campaign.py`.
- Régression du générateur v1 : `morsehgp3d/tests/fixtures/predicates/campaign_well_conditioned_zero_regression.json`.
- Régression DAZ : `morsehgp3d/tests/fixtures/predicates/orientation_daz_regression.json`.
- Registre scientifique : `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md`.

## Suite de la feuille de route

La prochaine phase active est la Phase 3, backend cible CUDA G4, profils communs, mode d'audit reproductible. Son entrée était déjà satisfaite par la Phase 0; sa fermeture exigera un environnement CUDA 12.9 reproductible, des presets, un manifeste complet, l'absence de compilation dans les mesures qualifiantes et les garde-fous GCP prévus. Aucun test réel sur VM n'est autorisé par la présente revue.

La fermeture de 2A rend en parallèle la sous-phase 17A `ready` pour son oracle CPU borné de recherche; elle n'active aucun backend public ni aucune nouvelle base de preuve. La Phase 2B reste bloquée uniquement par la Phase 3 : elle ne pourra commencer qu'après fermeture certifiée de l'environnement CUDA.

## GCP

GCP non utilisé. Aucune VM n'a été créée, démarrée, arrêtée ou modifiée pendant l'implémentation, la campagne ou la présente revue.
