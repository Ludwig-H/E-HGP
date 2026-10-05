# Architecture de la v11

Document normatif pour le code de `morsehgp3D_v11/`. Il fixe ce que « propre » veut dire ici, la carte des modules,
le profil numérique et les conventions de construction et de test. Les énoncés mathématiques vivent dans
`MATHEMATIQUES.md` ; ce document n'en recopie aucun.

## 1. Règles de propreté

Chaque règle est vérifiable ; `tools/check_style.py` contrôle celles qui se lisent dans le texte du code.

1. **Un module = un dossier = un en-tête public** `src/<module>/<module>.hpp`. Les autres fichiers du dossier sont
   internes. Un module ne dépend que des modules placés avant lui dans la table du § 2 ; aucun cycle.
2. **Taille** : un fichier de `src/` fait au plus 500 lignes, une fonction au plus 100. Un fichier qui approche la
   limite se découpe par responsabilité, jamais par numérotation (`part1`, `part2`).
3. **Aucun état global modifiable.** Une `Session` porte l'unique `MemoryBudget` et l'unique `Pool` ; ils sont
   passés explicitement.
4. **Erreurs** : `Outcome` et `Result<T>` ; aucune exception ne traverse une frontière de module ; jamais `assert`
   (une précondition interne violée rend `invariant_violated`). Seule exception : lire la valeur d'un `Result` qui
   porte un refus est un accès vérifié qui termine le processus ; un refus ne construit jamais de `T`. De même,
   détruire une `Session` dont le budget n'est pas revenu à zéro termine le processus (§ 7.1) : un destructeur n'a
   aucune issue à rendre (intégration de S5, audit général `a65903a7b`).
5. **Transactions** : une opération rend son résultat entier ou un refus ; jamais un préfixe publié.
6. **Aucun mutant, crochet de test ni option morte dans le produit.** Les mutants sont des correctifs appliqués à une
   copie des sources (`tests/mutants/`). Une option n'existe que si une porte l'exerce et qu'une ablation la justifie.
7. **Déterminisme** : toute sortie publiée est identique octet pour octet quel que soit le nombre de fils. Une tâche
   parallèle écrit à des positions fixées par son ordinal ; les concaténations passent par des préfixes.
8. **Mémoire** : tout grand tableau est un `Buffer<T>` réservé dans le budget avant l'allocation. `std::vector` est
   permis pour les petits états locaux hors boucle chaude.
9. **Chaque décision est entière et exacte.** Le flottant suit la doctrine du § 4.
10. **Chaque prédicat cite son budget de bits** (§ 3) et le garde par `static_assert` ; chaque élagage cite son lemme.
11. **Style** : C++20 sans extension (hors `__int128`, déclaré par `__extension__`), deux espaces, `snake_case` pour
    fonctions et variables, `PascalCase` pour les types, namespace `mhgp11`, macros `MHGP11_*`, cibles et tests
    `mhgp11_*`. Commentaires en français sans accents. Compilation `-Wall -Wextra -Wpedantic -Werror`, GCC et Clang.
    Python : PEP 8, aucune porte ne repose sur `assert`.

## 2. Modules

| Module | Rôle | Dépend de |
| --- | --- | --- |
| `core` | entiers, identifiants forts, statuts et raisons, `Result`, budget mémoire, `Buffer`, `Csr`, compteurs | — |
| `num` | entiers à budget de bits, entiers larges, niveaux rationnels, prédicats géométriques exacts, clés approchées à borne prouvée | `core` |
| `sched` | `Pool`, `parallel_for`, tri parallèle, sommes préfixes | `core` |
| `cloud` | contrôle du domaine, sites en ordre de Morton, multiplicités, table site → `PointId` | `core` |
| `io` | lecture `u32le` des nuages, empreintes SHA-256, écrivains petit-boutistes, transaction de dossier | `core`, `cloud` |
| `index` | requêtes exactes sur les sites (plus proches voisins, boules fermées) | `num`, `cloud` |
| `catalogue` | catalogue critique (boîtes de centres) | `num`, `cloud`, `sched` |
| `tower` | tour FULL (cellules, descentes, Kruskal par plateaux, verticales) ; en-tête public parapluie ; arbre d'ordre K seul et rattachement des boules (`build_order`, `WindowAttachment`) | `catalogue`, `index` |
| `supports` | hiérarchie des supports d'ordre K : supports positifs minimaux par boule, comptes dérivés, postordre et assemblage (`SupportHierarchy`) | `tower` |
| `points` | hiérarchie de points $H^{r}_{K+1}$ : pendaisons et arbre de points | `tower` |
| `head` | condensation, scores exacts, sélection, étiquettes | `points` |
| `api` | façade publique `api/api.hpp` et `Session` | `core`, `num`, `sched`, `cloud`, `io`, `index`, `catalogue`, `tower` |

`cli/` contient un seul exécutable, `mhgp11` (cible `mhgp11_cli`), à paramètre de sortie obligatoire
`--sortie=full|supports|points|plat` ([contrat des sorties](SORTIES.md)) ; `reference/` l'oracle exact borné en
Python ; `bench/` les bancs (synthétique, LiDAR, G4) ; `tests/` les portes, par module.

Les modules de la table qui n'ont pas encore de dossier sous `src/` (`points`, `head`) sont planifiés : leur place
est fixée d'avance, et `tools/check_style.py` ne contrôle que les dossiers présents. Le rattachement des boules et
l'arbre d'ordre K seul arrivent dans `tower` à la tranche S3. Le module `supports` existe depuis la tranche S6a :
supports positifs minimaux $\mathcal{Q}_b$ par boule, fermeture et comptes du lemme G ; son assemblage
(`SupportHierarchy`) vient avec la tranche S6b ([sorties](SORTIES.md), § 11). Le module `api` et l'exécutable
`mhgp11` existent depuis la tranche S5 (`Session`, `compute`, `publish`, `finish`, `withdraw`, `--sortie=full`) ;
`api` porte à terme les requêtes, les produits, les écrivains des quatre formats et le manifeste. Ses dépendances
**croissent avec les livraisons**, car `CMakeLists.txt` refuse la configuration dès qu'un module de la fermeture d'un
module présent manque : de `core` à `tower` pour la façade et `--sortie=full` (S5), puis `supports` (S7), `points`
(S9) et `head` (S10), chacune ajoutée à la table et à sa copie CMake dans le commit de sa tranche.

La première préparation de `cloud` est séquentielle et ne dépend pas de
`sched`. Son résultat possède un stockage privé, exposé par des vues constantes ;
le futur index peut ainsi conserver un propriétaire certifié sans alias mutable.
Un tri parallèle ultérieur devra ajouter explicitement sa dépendance et ses portes.

Les fondations (`core`, `num`, `sched`, `cloud`, `io`, `reference`) sont des **ports explicites** des composants de
la v10 durcis par son raccord R2 ; voir `PROVENANCE.md`.

## 3. Profil numérique

- Entrée : coordonnées entières $0 \leq x < 2^{B}$ par axe. $B$ = `MHGP11_COORD_BITS`, constante de compilation,
  **21 par défaut** depuis la demande du 2 octobre « u21 voire u24 » ; 18 reste disponible pour les
  comparaisons historiques et 24 pour le domaine élargi. Chaque profil compile depuis les mêmes sources
  et exige ses propres portes. Élargir B ne change ni le pas de grille ni les coordonnées des entrées.
- Les identifiants de points sont des `u32` arbitraires et uniques ; les positions égales forment un site de
  multiplicité $w \geq 1$.
- **Entiers à budget** : `num::Int<bits>` désigne le plus petit type exact capable de porter tout entier de valeur
  absolue $< 2^{\text{bits}}$ (`i64`, `i128`, puis entiers larges à mots de 64 bits). Le budget de chaque expression
  est calculé en `constexpr` à partir de $B$ ; aucun type n'est choisi à la main. Un débordement est donc une erreur
  de compilation, pas une garde à l'exécution.
- Les différences de coordonnées sont de valeur absolue $< 2^{B}$ (et non $2^{B+1}$).
- Un niveau est un rationnel exact `num / den`, `den > 0`, jamais réduit ; l'ordre des niveaux se décide par produits
  croisés exacts.

## 4. Doctrine flottante

La v10 a dû refuser une à une les options de compilation qui affaiblissent IEEE-754 (raccord R2, constat B2), sans
pouvoir fermer tous les canaux. La v11 ne s'appuie pas sur un contrat de compilation : ses usages du flottant restent
justes sous tout mode d'arrondi, avec ou sans contraction, et sous les ordres d'évaluation que chaque règle énumère.

- **F1.** Aucune décision n'est prise en flottant.
- **F2.** Noyaux entiers portés en binaire64 : toutes les valeurs, coefficients et produits intermédiaires compris,
  et toute somme partielle dans un ordre quelconque, sont des entiers de valeur absolue $< 2^{53}$. Le calcul est
  alors exact, quel que soit le mode. Le degré et la largeur des coordonnées ne suffisent pas à l'établir : la borne
  se démontre sur la somme des valeurs absolues des termes développés.
- **F3.** Clés approchées : obtenues à partir d'entiers exacts par conversions, produits, quotients et sommes de
  termes de même signe seulement ; aucune soustraction entre approximations. La borne d'erreur se propage **par
  expression**, jamais par compte d'instructions (une approximation réutilisée compte autant de fois qu'elle est
  lue ; correction des audits du 2 octobre 2026). Soit $u = 2^{-52}$ ; une clé $\tilde{x}$ d'un réel $x \neq 0$ porte
  un exposant $E$ tel que $\tilde{x} / x \in \left[ (1-u)^{E}, (1-u)^{-E} \right]$. Règles : conversion d'un entier
  exact, $E = 1$ (0 si l'entier est de valeur absolue $< 2^{53}$) ; produit de $n$ facteurs dans un ordre quelconque,
  $E = \sum_i E_i + n - 1$ (le carré d'une clé vaut donc $2E + 1$) ; quotient, $E = E_a + E_b + 2$ (ce qui couvre aussi
  le calcul par l'inverse) ; somme de $n$ termes de même signe dans un ordre quelconque, $E = \max_i E_i + n - 1$.
  Chaque règle ne suppose que ceci : une opération élémentaire dont le résultat est un nombre normal rend ce résultat
  multiplié par un facteur de $\left[ 1-u, (1-u)^{-1} \right]$, ce qui vaut pour tout mode d'arrondi IEEE-754 et pour
  une multiplication-addition contractée. Les exposants sont calculés en `constexpr` avec l'expression, et le domaine
  (ni débordement ni nombre dénormalisé) est démontré avec le budget de bits des entiers d'origine. Les seules
  transformations couvertes sont celles-là : arrondi, contraction, ordre des produits et des sommes de même signe,
  quotient par l'inverse ; les expressions n'emploient aucune autre opération flottante.
- **F4.** Les clés approchées sont **strictement positives** (un niveau nul se décide en exact, avant toute
  comparaison approchée). Deux clés $\tilde{x}$ et $\tilde{y}$ d'exposants $E_x$ et $E_y$ ne sont déclarées ordonnées, $x < y$, que
  si $\tilde{x} < c \, \tilde{y}$ pour une constante $c \leq (1-u)^{E_x + E_y + 1}$ (le produit par $c$ est lui-même
  arrondi) ; $c = 1 - 2^{-40}$ convient tant que $E_x + E_y + 1 \leq 4096$, ce que garde un `static_assert`. Sinon la
  comparaison est rejouée en exact.
- **F5.** Défense en profondeur, sans rôle dans les preuves : refus de `__FAST_MATH__` à la compilation, et auto-test
  des hypothèses F2 et F3 au démarrage d'une `Session`. L'auto-test ne remplace aucune preuve. Il est livré par la
  tranche S5 (`src/api/selftest.cpp`, raison `environment_selftest`, code 3) et refuse, sur quinze témoins calculés
  sur des opérandes `volatile` : une exception flottante démasquée, contrôlée avant toute opération ; un mode
  d'arrondi hors des quatre modes IEEE ; un noyau entier inexact (six noyaux) ou une précision étendue
  ($(2^{53}+1)-2^{53}$ doit valoir 0 ou 2) ; un résultat qui n'est pas l'un des deux voisins binaire64 du résultat
  exact (huit témoins, F3). FTZ et DAZ sont admis.
- **F6.** Filtres de signe à borne statique, pour les prédicats polynomiaux qui soustraient (orientation, côté d'une
  sphère, centre dans une boîte), quand la valeur exacte sort de F2. Entrées et coefficients sont des entiers exacts,
  **chacun certifié** dans son domaine (une largeur mesurée sur une feuille ne se transmet pas à un site extérieur à
  cette feuille). On développe l'expression **telle qu'elle est évaluée, avant toute annulation** : $M$ majore sur le
  domaine la somme des valeurs absolues des termes, avec $M(a \pm b) = M_a + M_b$ et $M(ab) = M_a M_b$ ; l'exposition
  $E$ suit $E = 0$ pour une feuille exacte dans binary64 (sinon sa conversion contribue a $E$ selon F3),
  $E(a \pm b) = \max(E_a, E_b) + 1$, $E(ab) = E_a + E_b + 1$, une
  multiplication-addition contractée étant majorée par la forme non contractée, et se borne sur tous les ordres
  d'évaluation permis ; une valeur réutilisée garde son exposition à chaque lecture. Alors, sans débordement ni
  sous-flux, $\lvert \tilde{v} - v \rvert \leq \gamma M$ avec $\gamma = (1-u)^{-E} - 1 \leq 2Eu$ dès que $Eu \leq 1/2$.
  Le seuil est lui-même une majoration certifiée : avec $M \leq 2^{q}$ et $E \leq 2^{e}$, $\tau = 2^{q+e-51}$.
  Son exposant est gardé séparément : $-1074 \leq q+e-51 \leq 1023$ pour une puissance de deux binary64 finie et
  non nulle, et au moins $-1022$ si le seuil doit rester normal. Hors domaine, repli exact avant de construire
  le seuil. L'absence de débordement de l'expression ne garantit pas celle de son majorant avant annulation.
  Le signe n'est décidé que si $\lvert \tilde{v} \rvert > \tau$ ; sinon, égalité comprise, le prédicat est
  rejoué en exact. Portes exigées sur les expressions réelles : zéro et signes $\pm 1$ près d'un grand permanent,
  réutilisations et carrés, permutations et parenthésages, quatre modes d'arrondi, contraction active et inactive,
  bornes exactes du domaine, repli effectivement déclenché.

## 5. Construction et portes

- `CMakeLists.txt` inclut `src/<module>/module.cmake` (sources de la bibliothèque `mhgp11`) et
  `tests/<module>/tests.cmake` (portes) pour chaque module présent. Un module n'édite jamais un fichier partagé.
- Options : `MHGP11_MODULES` (modules à construire ; tous ceux présents par défaut), `MHGP11_COORD_BITS` (21),
  `MHGP11_SANITIZE` (ASan + UBSan), `MHGP11_TSAN`, `MHGP11_POISON` (tampons empoisonnés), `MHGP11_MARCH` (jeu
  d'instructions, vide par défaut), `MHGP11_MUTANT_JOBS` (parallélisme du lanceur de mutants).
- Une porte se déclare par les fonctions d'aide de `cmake/gates.cmake` ; un `add_test` direct est refusé. Chaque
  porte Python rapide a une jumelle `_opt` jouée sous `python3 -O`.
- Codes de sortie exacts (`cmake/run_expect.cmake`) : 0 conforme, 1 désaccord d'un juge, 2 refus avant calcul,
  3 plancher ou invariant violé, 4 mutant tué. Un arrêt par signal est toujours un échec.
- Toute porte porte un plancher de couverture contre le vert par vacuité.
- Labels CTest : `unit`, `oracle`, `diff_v10`, `scale8000`, `scale16000`, `scale32000`, `lidar`, `mutant`, `fast`
  (les portes `fast` n'exigent ni NumPy ni scikit-learn) et `long` (plus d'une minute : campagnes de mutants, suite
  complète de la référence, stress). En local : `ctest -LE long` ; les portes `long` et la matrice des
  configurations (`tools/g4_matrix.py`) passent sur G4.
- Les petites tailles sont des oracles de correction ; toute conclusion de coût se mesure à 8 000, 16 000 et 32 000
  points et sur les trames LiDAR du contrat.
- Compilations et calculs hors du dépôt (`/tmp`), binaires de campagne figés depuis `git archive`.

## 6. Conformité

La v11 calcule le même objet que la v10. La conformité se prouve par : (a) l'oracle exact borné de `reference/` ;
(b) des campagnes appariées contre le binaire figé de la v10 (sorties canoniques identiques octet pour octet sur les
mêmes entrées, trames LiDAR du contrat comprises) ; (c) des invariants globaux à l'échelle ; (d) des mutants tués.
Aucun benchmark ni accord moyen ne promeut un statut public.

## 7. Contrats fixés à l'ouverture

Décisions demandées par les audits du 2 octobre 2026 ; elles valent pour toutes les tranches.

### 7.1 Budget mémoire et propriétaires

- Tout tableau dont la taille dépend de l'entrée (points, sites, boules, niveaux, nœuds, incidences), y compris les
  tampons de tri, les brouillons par fil et les sorties, est un `Buffer` réservé dans le `MemoryBudget` **avant**
  l'allocation. `std::vector` n'est permis que pour une taille bornée par une constante de compilation ou par le
  nombre de fils.
- Aucun agrandissement par doublement : compter, réserver, remplir (deux passes), ou blocs d'arène pris dans le
  budget. L'ancien et le nouveau tampon d'une recopie sont tous deux comptés tant qu'ils coexistent.
- Un brouillon par fil a une capacité bornée par une quantité démontrée (taille de feuille, ordre) ; il ne garde pas
  la capacité d'une requête passée.
- La `Session` possède le budget et le `Pool` ; elle survit à tous les résultats qu'elle a servis. À sa destruction,
  un budget non revenu à zéro est une violation d'invariant : `~Session` termine le processus (règle 4 ; porte
  `mhgp11_api_session_destroyed_live`, mutant `session_detruite_sans_controle`). `Session::close` constate la même
  violation sans arrêt (`budget_not_released`, code 3). Un `Product` garde le jeton d'identité de sa `Session`, et
  `publish` refuse le produit d'une autre `Session` (`parameter_out_of_range`, avant toute création).
- Chaque étage publie son pic d'octets réservés ; le pic d'une opération publique est mesuré, pas estimé. Le CLI
  accepte un plafond ; au-delà, refus `resource_exhausted`, avant tout résultat partiel.

### 7.2 Opération atomique

Une opération publique (`build_catalogue`, `build_tower`, hiérarchie de points, sortie `--sortie` du CLI) rend un
résultat complet ou un refus. Une sortie du CLI est un **dossier** $D$ (transaction de la tranche S4, 4 octobre
2026) : ses fichiers sont écrits dans `D.pending/`, le manifeste en dernier, puis le dossier est publié par un seul
renommage sans remplacement (`renameat2`, `RENAME_NOREPLACE`). Un lecteur concurrent voit donc le dossier entier ou ne
le voit pas. Un refus pris avant la publication ne publie aucun dossier ; un refus constaté après (fermeture de la
`Session`, ligne d'état) retire le dossier publié. Si la synchronisation du parent puis son retour arrière, ou bien le
retrait, échouent aussi, le dossier publié reste complet : l'appel le déclare par un état distinct,
`published_complete`, avec l'empreinte du manifeste, conservée dès sa fermeture ([sorties](SORTIES.md), § 9). Ce
n'est ni un succès de durabilité ni une sortie partielle. Un arrêt brutal peut laisser un `D.pending` orphelin, qui
fait refuser l'appel suivant et n'est jamais retiré automatiquement ; il ne laisse jamais un dossier publié partiel.
Le manifeste reste le seul témoin d'achèvement. Le contrat de la v11 est la trame entière en mémoire :
ni segment, ni point de reprise ; le régime massif (dizaines de millions de sites) est hors de ce contrat et
demandera sa propre décision.

### 7.3 Domaines d'identifiants, formats et profils

- Domaines distincts, types distincts : `PointId` (u32 arbitraire, externe), `SiteIdx` (rang de Morton, u32),
  `BallIdx` (rang canonique d'une boule, u32 dans le profil de trame), `LevelRank` (rang dense d'un niveau exact,
  u32), `NodeIdx` (nœud d'une forêt), décalages de tableaux (u64). Tout dépassement d'un domaine est un refus
  `resource_exhausted` explicite, jamais une troncature.
- Une entrée déclare son pas de grille exact, son origine et la table retour → site ; les rangs exacts des niveaux
  sont conservés jusqu'aux sorties, une vue flottante des niveaux n'est jamais l'objet publié.
- Profil initial : coordonnées 18 bits. Compiler en 21 ou 24 bits ne qualifie pas ces profils : chacun demande ses
  portes. Le palier 32 bits décidé le 30 septembre reste une étape ultérieure du même plan.
- Multiplicités : les positions égales forment un site de poids $w \geq 1$, publié par `cloud`. Tant que la
  sémantique pondérée de la tour n'est pas écrite dans `MATHEMATIQUES.md`, la tour refuse une entrée pondérée
  (`unsupported_degeneracy`), comme la v10.
