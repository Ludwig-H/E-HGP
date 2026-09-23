# Lentille 4 — chaîne générateur → catalogue → tour (auditeur C)

Cadre : `phase=exploration_v9_hors_registre`, `backend=reference_cpu`, `profile=quantized_u18_input_only`, `mode=audit_independant`, `public_status=not_claimed`. Code lu dans le worktree détaché `origin/main` **0125dc18**. Le commit développeur non poussé est maintenant **4530644b** (et non plus 4cde1502) ; par rapport à 0125dc18, il ne touche `src/` que dans `src/chain` (+18 lignes de registre, `git diff 0125dc18 --stat`). GCP non utilisé. Aucune écriture de dépôt. Un seul petit contrôle a été compilé et exécuté (§ 5.3) : 2,85 s CPU de compilation et 0,45 s CPU d'exécution, sous `nice -n 19`.

Légende : **prouvé** (théorème ou lecture de code), **testé** (porte ou fixture), **mesuré** (reçu), **supposé**.

## 1. Ce que la chaîne doit produire

Objet (manuscrit, § 6.2, Déf. 20–22 ; chap. 8, Déf. 25, 28, Prop. 6, Th. 5). Pour chaque K, on considère les K-parties σ, présentes au rayon r si leur miniboule a un rayon ≤ r. Deux K-parties sont reliées quand leur union, de K+1 points, est encore présente. Par la Prop. 6, seules comptent les cofaces **de Gabriel** : l'intérieur strict de leur miniboule est contenu dans σ.

La tour FULL v7 lit tout cela dans un **catalogue de boules**. Chaque ligne donne la clé exacte, le niveau r², l'intérieur strict I, la coquille complète U et q_min, avec |I| + q_min ≤ min(K_max+1, n) (`src/chain/tower_chain.cpp:493–494`). La boule sert aux ordres K ∈ [|I|+q_min−1, min(K_max, |I|+|U|)] (`src/tower/forest/full_ball_tower.hpp:965–966`). Le contrat de la chaîne est donc double : l'**exactitude** de chaque ligne émise et la **complétude** du catalogue, c'est-à-dire toutes les boules de la fenêtre.

## 2. Déroulé pas à pas de `run_tower_chain` (`tower_chain.cpp:285–603`)

0. **Refus d'appel** (`290–298`) : K ∉ [1,10], s < 8, W = 0, n < 2 ou n > INT32_MAX donnent `invalid_input`. `kmax_effective = min(K, n)`.
1. **Préparation** (`302–309`, `src/gen/pipeline/prepared_cloud.cpp:37–97`). Copie privée des points, domaine [0, 2^18) et unicité des sites par tri de clés 54 bits. Un doublon est refusé (`invalid_argument`, puis `invalid_input`, `:305–308`). Suit l'arbre de plages.
2. **Index générateur** `make_q2_cloud_index` (`:311`).
3. **Voie q2** (`:314–343`). W consommateurs, un vecteur `slots[w]` par ouvrier. Chaque paire acceptée devient une `Presentation` d'arité 2 : support trié, clé v8 recalculée par `ExactBall::make_q2`, `depth=|I|`, `shell=|U|`. La coquille inclut les deux extrémités (`src/gen/pipeline/q2_census.hpp:62–71`). La configuration du front est épinglée en dur (`:334–338`).
4. **Voies q3/q4** si K ≥ 2 (`:345–418`). Options épinglées (Local28, RectanglePair, GlobalBoxes, Affine, LiveOnly/64, consultation d'atlas) et cinq leviers (`tower_chain.hpp:50–70`). Le rappel écrit dans `slots[slot]` ; le contrat garantit un seul appel à la fois par slot (`src/gen/pipeline/wspd_q34.hpp:253–260`). Les graines q3 sont strictement aiguës et leur propriétaire est l'arête la plus longue, départagée par IDs (`src/gen/lanes/q34_seed.hpp:41–45`). Le registre est recopié (`:379–416`).
5. **Fusion** `gather_presentations` (`:88–153`), un tri d'échantillonnage.
   - Chaque slot est trié par (clé, arité, support).
   - Les séparateurs de clés, strictement croissants, sont tirés à positions fixes par slot. `lower_bound` place une même clé dans une seule plage.
   - Chaque plage est rassemblée, triée et balayée : même `depth` et même `shell` dans une clé, aucune paire (arité, support) en double (`:132–142`).
   - Le représentant d'une clé est sa plus petite arité, puis son plus petit support. Les slots sont libérés (`:144`).
   - La correction de l'ordre ne dépend pas de la qualité des séparateurs (B, `CONTRE_AUDIT_B_SAMPLE_SORT_PUBLIE_20260923.md`).
6. **Index de la tour** (`:433–446`) : second index, arbre radix sur Morton 54 bits (`src/tower/core/morton.hpp:26–44`). Le nom `morton48` est un héritage, pas un défaut. `geo_of_id` y est construit mais jamais relu.
7. **Recensement exact** de chaque clé distincte (`:448–525`), en `parallel_for` à grain 256.
   - Clé et niveau sont recalculés depuis le support par les formules v7 (`:173–191`) et comparés à la clé v8 (`:469`).
   - `ball_census` s'appelle avec plafond d'intérieur = `depth` émis et coquille non plafonnée (`:470`, `src/tower/pipeline/census.hpp:174–208`). On exige ensuite |I| = depth et |U| = shell.
   - |U| > 12 est **compté**, sans troncature (`:478–481`). |I| > 9 est un invariant violé (`:482`).
   - Si |U| ≠ arité, q_min est recalculé par `ShellTable` et doit égaler l'arité présentée minimale (`:484–492`). On vérifie ensuite la fenêtre de rang (`:493`) et on remplit `BallData` avec des IDs triés.
   - Les plages sont libérées **dans** le chrono du recensement (`:517`). Si au moins une coquille dépasse 12, le statut est `unsupported_degeneracy` (`:519–522`).
8. **Tour FULL** (`:527–555`). Voie statique à W fils par défaut. `validate_catalogue` certifie l'ordre strict par un seul balayage (`full_ball_tower.hpp:898–900`). La passe 1, parallèle, fait les contrôles locaux et la **positivité du support déclaré** des boules régulières (`:844–848`, `src/tower/forest/anchor_meb.hpp:85–105`). La passe 2, **série**, recalcule q_min et un témoin MEB pour les coquilles étendues, la fenêtre et les programmes (`:939–969`). Le résumé des ordres parcourt tous les nœuds (`tower_chain.cpp:543–553`).
9. **Statuts** (`:558–583`), puis `total_ms` (`:584`), CPU du processus (`:585–586`) et condensé FNV **hors chrono** (`:587–601`).

**Parallélisme.** q2, q3/q4, tri des plages, recensement et tour sont parallèles. La préparation, les deux index, l'allocation initialisée du catalogue (`std::vector<BallData> balls(unique)`, `:450`), la passe 2 de validation, le résumé d'ordres et les libérations restent série.

**Mémoire** (ABI LP64 : `Presentation` 112 o, d'après B ; `BallData` **224 o**, `sizeof` recompilé ici).
- Pendant la fusion coexistent slots et plages, 2×112P octets.
- Pendant le recensement : plages (112P) + catalogue (224B) + représentants (8B).
- À 08/000000/K10 (R7b : P = 5 512 675, B = 5 512 670), cela fait 617 Mo + 1,235 Go + 44 Mo ≈ **1,90 Go** au même instant, hors index et états.
- Le RSS processus R7b vaut 4,93–5,20 Gio à K10 et 1,13–1,38 Gio à K5 (sorties `receipts/g4_tower_r7b_20260923/vm/probe_*.stdout`). `keep_catalogue` recopie 224B de plus (portes seulement).

## 3. Ce que garantit `complete_relative`, et ce qu'il ne garantit pas

**Prouvé par lecture**, avec `run_tower=true` :
- **G1.** Chaque clé émise est exacte. La clé v8 égale la clé v7 recalculée ; |I| et |U| sont recensés sur **tout** le nuage ; les présentations d'une même clé concordent ; il n'y a aucun doublon de présentation ; les clés sont distinctes et strictement triées.
- **G2.** Le q_min stocké est vrai :
  - coquille étendue : q_min recalculé = plus petite arité présentée (`tower_chain.cpp:484–492`), puis revérifié par la tour avec un support minimal et un témoin MEB (`full_ball_tower.hpp:944–958`) ;
  - coquille régulière (|U| = arité) : c'est **la tour seule** qui prouve la positivité stricte (q3 aigu, centre q4 intérieur). Or un support positif et affinement libre n'a aucun sous-support de même boule, donc q_min = arité.
- **G3.** Chaque ligne est dans la fenêtre, avec |U| ≤ 12 et |I| ≤ 9.
- **G4.** La tour est la tour FULL exacte **de ce catalogue** : c'est l'autorité `full_tower_relative_to_supplied_complete_exact_ball_censuses` (`full_ball_tower.hpp:19–20`). Chaque ordre finit en une seule composante (`:374`, `:675`).

**Ce que `complete_relative` ne garantit pas :**
- **N1. Une clé entièrement omise** (aucune présentation, à aucune arité) échappe à tous les recoupements : seules les clés émises sont recensées (`tower_chain.cpp:463–507`). C'est déjà dit par A (`CONTRE_AUDIT_A_MATH_MOTEUR_20260922.md` § 5), B et `ETAT_COURANT.md`.
- **Précision ajoutée : la tour détecte une partie des omissions.**
  - La résolution d'une facette descend par intrus stricts. Si elle atteint une boule sans intrus, cette boule est dans la fenêtre de l'ordre K. Si elle est absente du catalogue, l'invariant `full_ball_missing_weak_terminal` part (`:1418–1427` ; voie statique `:1057–1071`).
  - Une boule omise qui n'est **terminal d'aucune descente** reste invisible : naissance absente, ou fusion absente rattrapée plus tard par une autre boule.
  - À **K = 1**, il n'y a aucune descente (`:1398`) : seule la connexité finale est contrôlée. Une arête d'arbre couvrant minimal omise dont le graphe de Gabriel reste connexe donne des niveaux faux sans refus.
- **N2. Omission partielle.** Si une clé perd sa présentation d'arité minimale mais garde une présentation d'arité supérieure, le cas est détecté :
  - coquille étendue : par le contrôle q_min ;
  - coquille régulière : par le refus de positivité de la tour.
  
  Dans ce second cas, le statut publié est `invalid_input` et non un invariant (L4-05).
- **N3.** Avec `run_tower=false` (sonde `--no-tower`, chemin catalogue de T2), le statut reste `complete_relative`. Pourtant G2 n'y est pas prouvé pour les coquilles régulières : `tower_chain.cpp:484` saute q_min quand |U| = arité. Or l'en-tête (`tower_chain.hpp:14–15`) annonce un q_min recalculé sans cette restriction (L4-06).
- **N4.** Il n'y a ni multiplicités (sites distincts exigés), ni identités externes : PointId est le rang d'entrée (`tower_chain.hpp:142`) (L4-10).

## 4. Chronomètre : frontières et coûts mesurés

- **Frontières.** La lecture est chronométrée à part par la sonde (`bench/tower_probe.cpp:158–166`). La préparation est **dans** `chain_total`. Le condensé est hors chrono, synchrone (`:587–601`) ; il n'existe pas de `digest_cpu_s` (déjà relevé par B). Les temporaires du `try`, y compris le catalogue et les deux index, sont détruits à la fermeture du bloc, donc **avant** `total_ms` (`:558`, `:584`). Le résultat retourné est détruit par l'appelant. Le worker G4 exige Σ phases ≤ `chain_total` et lecture + total + condensé ≤ mur externe (`gcp-migration/tower_worker_v9.py:533`, `:553–562`).
- **Mesure nouvelle, recalculée sur les 24 sorties R7b** : `chain_total − Σ(prepare, gen_index, q2, q34, merge, tower_index, census, tower)`.
  - Ce résidu **non attribué** vaut **12–98 ms à K5** et **182–430 ms à K10**.
  - Il contient le résumé d'ordres et les libérations de fin de bloc : catalogue de 0,25–1,23 Go, index tour, index et nuage du générateur.
  - Le recensement contient aussi la construction **série** du catalogue initialisé (224B octets) et la libération des plages (112P octets).
  - Ce découpage est **supposé** d'après le code, faute d'instrumentation. Une micro-mesure locale sur hôte chargé (charge 13,6) n'est pas probante et n'est pas retenue.
- **Plomberie de la chaîne**, hors q2, q3/q4 et tour : prepare + gen_index + merge + tower_index + census + résidu. Elle vaut **174–275 ms à K5** et **698–1 002 ms à K10** (mesuré R7b, par exemple 08/000100/K10 : 741 ms, dont census 364 et résidu 263). À K10, la plomberie seule consomme donc tout le budget d'une seconde. Cela précise le « 1,154 s hors q3/q4 et FULL » d'`ETAT_COURANT.md`, qui inclut q2.
- **Recensement** : 71–97 visites de nœuds par clé (R7b : `census_nodes`/`unique_keys`, 78 M à 535 M visites), descente depuis la racine pour chaque clé.

## 5. La porte T2 de la chaîne

### 5.1 Ce qu'elle fait

`tests/chain/chain_census_tower_gate.cpp`, inscrite `CMakeLists.txt:128–133`, avec mutants `:162–173`.
- **Nuages** : trois fixtures, line12 (12 sites colinéaires), shell14 (14 sites, dont une coquille de 12) et spatial12 (12 sites), chacune en deux variantes d'IDs (denses, ou clairsemés et renversés).
- **Ordre** : **K_max = 10 codé en dur** (`:174`, `:178`, `:181`, `:207`).
- **Séparations et fils** : s = 8/10/12 ; chaîne à 1 fil (s8, s12) ou 4 fils (s10) ; tour à 0/1/4 fils.
- **Tour publiée** : `run_tower=true` à W1/W4, s8, jugée contre le modèle Γ (`:195–219`). Le constat A § 5 (« la porte appelle `run_tower=false` ») est donc **corrigé**, de même que l'inscription des trois mutants d'oracle.
- **Oracle** indépendant, en rationnels Boost : supports positifs, clé primitive par pgcd, census rationnel (`:55–87`, `oracle/tower/local_plateau_oracle.hpp:77–99`). Le modèle Γ est exhaustif par masques (`tests/tower/census_tower_oracle.hpp`), et comparé à un second modèle pour n ≤ 8 (`--historical`).
- **Limite de variété** : pour le catalogue, la chaîne reçoit les points dans l'ordre de l'index (Morton), et les deux variantes donnent le même appel (`:95–99`). Les permutations ne sont exercées que sur la tour publiée.

### 5.2 Peut-elle voir une omission du générateur ?

Oui, pour ses trois nuages : l'inventaire exhaustif compare cardinalité, clés, niveaux, arités, I et U (`:120–126`). Mais :
- **(a)** Son seul mutant de catalogue retire une boule **après** la chaîne (`:109`). C'est une mutation du harnais, aucun mutant du générateur n'est jugé par cette porte (même réserve que B pour sa porte publique).
- **(b)** **À K_max = 10 sur 12–14 sites, l'élagage est presque vide.** Oracle exact recalculé : sur 66/210/253 boules à support positif, le catalogue en garde 65/207/252. Il n'y a que **1 à 3 rejets par fixture**, alors qu'à K = 5 on en compte 21/34/76.
- Les certificats qui dépendent de K (saturation à K−1, voies mortes, seuils de profondeur) sont précisément ceux dont une faute **omettrait** des clés. T2 ne les exerce donc presque pas.
- **K_max = 5, repli du contrat, n'est jamais jugé par oracle au niveau de la chaîne.** Les portes `static_paths` et `order_failure` tournent à K5 mais sont différentielles. `wspd_q34_gate` juge K5 à s = 12 dans sa grande boucle (`tests/gen/wspd_q34_gate.cpp:701`, `:734` : s = 8+2·(K mod 3)), et ne prend que quelques cas K5/s8 en configuration partielle.
- Le plan annonçait des juges « K = 1 et K = n » (`docs/PLAN_V9.md:134–135`), absents au niveau de la chaîne.

### 5.3 Contrôle réalisé (testé, local, hors dépôt)

Pilote `chain_dump.cpp` lié aux bibliothèques du développeur (`build/v9-open-worktree/build/v9-dev/libmhgp9_chain.a` SHA-256 `c0086453…`, `libmhgp9_gen.a` `208aabb3…`, la même que celle de l'audit de distribution B). Il est comparé à un oracle Python en fractions (`oracle_catalogue.py`). Cinq nuages : les trois de T2, plus deux nuages de 14 sites de type LiDAR (deux lignes de balayage et un poteau ; une grappe aplatie).

Plan : K_max = 1..10, s = 8/12, W = 1/2 (tour 0/2 fils). Résultat :
- **200/200 appels `complete_relative`** ;
- **24 472 lignes de catalogue égales à l'oracle** : clé, q_min, I et U ;
- **0 écart** ;
- condensé identique entre s et W pour chacun des 50 couples (nuage, K).

À K5, ce sont 645 lignes attendues et 285 boules positives à rejeter par configuration, contre 907 et 23 à K10. Ajouter K5 à T2 passerait donc aujourd'hui. Le contrôle ne prouve rien au-delà de ces cinq nuages.

## 6. Défauts et fragilités (détail dans `findings`)

- **L4-01.** T2 limité à K_max = 10.
- **L4-02.** Aucun juge d'omission à l'échelle ; propositions : un arbre couvrant minimal (EMST) exact pour K = 1, et un juge d'échantillon par voisinages pour K ≥ 2.
- **L4-03.** Aucune porte causale ne déclenche les refus propres à la chaîne (clé v8/v7, profondeur, coquille, q_min, doublon, fenêtre, coquille > 12). Les chaînes de raison n'apparaissent que dans `src/chain`.
- **L4-04.** Plomberie et résidu non attribué.
- **L4-05.** Typage des refus : `invalid_input` pour des incohérences internes.
- **L4-06.** `run_tower=false` publie `complete_relative` sans positivité vérifiée.
- **L4-07.** Raison d'échec non déterministe si plusieurs échecs.
- **L4-08.** Temps de phase perdus sur exception hors fusion et recensement.
- **L4-09.** `geo_of_id` et `keep` morts (déjà relevé par B, toujours présents).
- **L4-10.** Domaine d'entrée : doublons, rang comme identité, code 3 de la sonde.
- **L4-11.** Travail redondant (tri global pour 1–13 doublons, recensement depuis la racine, revalidation v7).

Aucun défaut d'exactitude n'a été trouvé dans les chemins lus. Les dépassements u32 sont gardés : n ≤ INT32_MAX ; les champs u8 sont contrôlés avant conversion ; `BallId` est borné par la tour (`full_ball_tower.hpp:881`). Seul le type de ce dernier refus est discutable.

## 7. Autres implémentations pour le contrat (LiDAR, K5/K10, G4)

Aucune ne change l'objet ; les gains sont bornés par R7b.

1. **Catalogue direct, sans `Presentation` ni tri global.** P − B vaut 1 à 13 présentations sur 1,1–5,5 M (R7b). Le tri global sert surtout à produire l'ordre des clés pour la voie pré-triée de la tour. Proposition :
   - partitionner par hachage de clé dès l'émission ;
   - dédoublonner par seau, avec le choix déterministe (arité, support) ;
   - laisser la tour trier par (niveau, clé) : le départage par clé est déjà sa règle de stabilité (`full_ball_tower.hpp:971–990`).
   
   Gain : la fusion (34–122 ms) et la double résidence 2×112P. À fusionner avec la proposition histogramme/dispersion de l'audit de distribution.
2. **Recensement dans le balayage des plages.** Chaque plage recense ses propres représentants juste après son tri : localité, plus de tableau de pointeurs, équilibre mesuré à 1,52–1,59 fois la moyenne.
   - Allouer le catalogue **non initialisé** et le toucher d'abord en parallèle.
   - Remplacer la descente depuis la racine par une recherche à partir de la feuille d'un site du support, avec le même prédicat exact.
3. **Libérations hors chemin critique.** Pour un usage en flux de trames, réemployer des arènes d'une trame à l'autre (plus d'allocation ni de libération par trame), ou libérer de façon asynchrone, avec pages larges sur madvise.
   - À publier : `release_ms` et le découpage de `census_ms`.
   - À publier aussi : la politique THP du G4, absente des reçus.
4. **Un seul index spatial** pour le générateur et la tour : 20–27 ms, significatif à 100 ms.
5. **Ne pas revalider deux fois avec le même code.** Pour un catalogue issu de la chaîne, la passe 1 de la tour recalcule clé et niveau v7 avec les mêmes formules que la chaîne (`tower_chain.cpp:173–191` contre `full_ball_tower.hpp:851–875`) : c'est redondant, pas indépendant. Garder la positivité et fusionner le reste, après mesure du coût de la passe 1.
6. **Juges d'échelle** : ce n'est pas de la vitesse, mais c'est un préalable à toute revendication.
   - K = 1 : les fusions K1 publiées doivent égaler {d²/4} d'un EMST exact indépendant (Borůvka et k-d tree, O(n log² n)), puisque K = 1 est exactement le Single-Linkage (manuscrit, remarque après la Déf. 22).
   - K ≥ 2 : graines tirées, 24 voisins, supports de 2 à 4 sites contenant la graine, recensement exact indépendant, et appartenance au catalogue exigée si la boule est dans la fenêtre. Plancher de couverture, et mutant d'omission planté tué à 8k/16k/32k.

## 8. Reproduction

Répertoire `/tmp/claude-1000/-workspaces-E-HGP/71988aca-a49a-4d33-96db-f05468331005/scratchpad/agents/lentille4/` :

| Fichier | SHA-256 (préfixe) |
| --- | --- |
| `oracle_catalogue.py` | `876bfe68…` |
| `fixtures.json` | `942b09d6…` |
| `chain_dump.cpp` | `88b6fb13…` |
| `compare.py` | `a87a91ce…` |
| `dump.txt` | `b4940898…` |

Commandes :
- compilation : `g++ -std=c++20 -O1 -pthread -I<dev>/morsehgp3D_v9/src/gen -I<dev>/morsehgp3D_v9/src chain_dump.cpp libmhgp9_chain.a libmhgp9_gen.a` (GCC 13.3.0) ;
- exécution : `./chain_dump < fixtures.txt > dump.txt`, puis `python3 compare.py dump.txt`.

Les chiffres R7b sont recalculés depuis `receipts/g4_tower_r7b_20260923/vm/probe_{0..23}.stdout`. Ces fichiers sont temporaires et ne sont pas un reçu versionné.