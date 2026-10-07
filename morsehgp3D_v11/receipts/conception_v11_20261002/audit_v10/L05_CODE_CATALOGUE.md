# L05 — Code du générateur de catalogue de morsehgp3D_v10

Audit pour la conception de morsehgp3D_v11. Rédigé le 2 octobre 2026 entre 05:25 et 08:03 UTC (heures lues par `date -u`).

```text
phase=audit_v10_pour_conception_v11 (hors registre)
backend=cpu_reference
profile=quantized_u18_input_only
mode=audit_independant_du_code
public_status=not_claimed
GCP non utilisé
```

Sujet : `morsehgp3D_v10/src/catalogue/` au commit `afb081774` (dernier commit qui touche le générateur : `777406b82`, J2c, 29 septembre 2026, 16:57). Empreintes sha256 des fichiers audités : `generator.cpp` `d5996feaf0df9e9f…`, `catalogue.hpp` `2f47beb9dfc1e59e…`, `support.hpp` `7cc62930f623e99b…`, `mhgp10_catalogue.cpp` `e345022a524223a9…`, `test_catalogue_oracle.py` `f581b1ce6ee20758…` ; le worktree est passé à `f2ebb7a08` pendant l'audit sans toucher ces fichiers (empreintes recontrôlées à 07:51 UTC). Lecture seule sur le dépôt et sur les dossiers privés ; tout le calcul sous `/tmp/v11-audit/l05_code_catalogue/` ; petites preuves sous `/workspaces/E-HGP/build/v11-persist/audit_v10/preuves_l05_code_catalogue/` (environ 260 Ko, aucun octet KITTI).

Vocabulaire de vérification : **lu** (établi par lecture du code ou d'un reçu), **exécuté** (rejoué par l'auditeur, résultat observé), **mesuré** (compteur ou temps relevé par l'auditeur), **non vérifié**. Un temps local est toujours indicatif : la machine (8 fils) portait une charge de 4 à 21 pendant l'audit. Font foi : les compteurs déterministes, les rapports pris dans une même exécution, les comptes d'instructions de callgrind, et les reçus G4 du dépôt pour les temps absolus.

---

## 0. Résumé

1. **Exactitude : aucun défaut trouvé.** Chaque décision d'élagage est couverte par un lemme et traite ses égalités du bon côté (§ 5.1). L'auditeur l'a contrôlé par exécution : un oracle brut indépendant (43 contrôles, 385 226 boules comparées enregistrement par enregistrement, K jusqu'à 12, poids, feuilles larges), l'invariant de restriction et l'identité d'Euler à 8 000 points, les extrêmes du domaine u18 sous ASan et UBSan, l'invariance de 1 à 48 fils (§ 5.4).
2. **La porte permanente du dépôt est trop étroite.** `mhgp10_catalogue_oracle` (40 nuages de 5 à 22 points, K ≤ 5, poids 1) laisse vivre trois mutants non équivalents : masques à plusieurs mots, frontière pilotée par la charge (perte silencieuse de 2 134 à 9 523 boules sur un quart de trame, selon le nombre de fils), réparation des bandes (§ 5.5).
3. **Le contrat de 100 ms est hors de portée du générateur tel quel.** Sur G4 à 48 fils, le catalogue seul prend 171 ms à K = 5 et 628 ms à K = 10 (reçu de session 4). Le travail consommé à 48 fils (6,04 s de CPU à K = 5, contre 3,95 s à un fil) borne le mur à environ 126 ms (132 ms avec le temps système) même si les 48 fils étaient occupés en permanence (§ 6.6).
4. **Où va le temps** (mesuré, § 6.2) : la feuille fait 77 % (K = 5) à 86 % (K = 10) de l'étage des boîtes, le filtre des nœuds 23 % à 14 %. Triplets et quadruplets pèsent 47 % à 59 %. Le générateur teste 48 à 58 candidats par boule émise sur LiDAR, à 135–270 cycles pièce, avec 10,6 % de branchements mal prédits (simulation callgrind).
5. **La feuille est entièrement scalaire**, avec ou sans `-march` (une seule opération vectorielle entière sur 2 246 instructions, 35 multiplications 128 bits, 116 sauts conditionnels). Les sessions G4 tournent en build sans `-march` : le noyau du filtre, conçu pour 1,5 cycle par test, coûte 5,2 à 5,5 cycles dans ce même build mesuré localement (§ 6.4).
6. **Parallélisme** : à 48 fils, la frontière en largeur (15,5 % du temps à K = 5), l'ordre et l'assemblage (19 %) et la libération des tampons (5 %) ne passent pas à l'échelle ; la frontière ne gagne que ×2,1 de 1 à 48 fils (§ 6.6).
7. **Mémoire** : 270 à 340 octets par boule au pic, 104 à 125 octets par boule résidents, soit 3 à 3,5 fois le format conçu, hors de tout budget (§ 6.5).
8. **Leviers chiffrés** (§ 6.7, § 12) : la variante privée J3 de la feuille est égale à HEAD octet pour octet et exécute 19 % d'instructions et 40 % de mauvaises prédictions en moins (rejoué) ; l'étage des quadruplets se réduit de ×2,4 à ×2,7 par simple réordonnancement exact, et de ×8 à ×11 avec un filtre flottant à repli exact (microbanc sur 18,8 M candidats réels, 0 désaccord). Le même filtre ne gagne que ×1,5 à ×2 sur les triplets et rien sur le recensement.
9. **Estimation fondée pour un générateur v11 en CPU seul** : ×1,8 à ×2,1 sur le catalogue à 48 fils (≈ 90 ms à K = 5, ≈ 320 ms à K = 10). Le contrat de 100 ms pour la tour FULL à K = 5 demande donc plus : lots AVX-512, GPU, ou réduction algorithmique du nombre de candidats — trois pistes non mesurées (§ 11).
10. **La conception GEN_v2 ne décrit plus le code** : arbre binaire ajusté au lieu de l'octree, stagnation réintroduite, admission pondérée non unifiée, mémo par $S^{*}$ et non par masque, pas de `LeafOracle`, pas de digest, format 3,5 fois plus lourd, portes et fixtures prévues absentes (§ 4).

---

## 1. Périmètre lu

| Fichier | Lignes | Lecture |
|---|---:|---|
| `morsehgp3D_v10/src/catalogue/generator.cpp` | 876 | intégrale, ligne à ligne |
| `morsehgp3D_v10/src/catalogue/catalogue.hpp` | 106 | intégrale |
| `morsehgp3D_v10/src/catalogue/support.hpp` | 99 | intégrale |
| `morsehgp3D_v10/cli/mhgp10_catalogue.cpp` | 114 | intégrale |
| `morsehgp3D_v10/tests/oracle/test_catalogue_oracle.py` | 161 | intégrale |
| `morsehgp3D_v10/docs/conception/GEN_v2.md` | 672 | intégrale |
| `morsehgp3D_v10/src/arith/geometry.hpp`, `geometry.cpp`, `wide.hpp` | 152, 92, 164 | intégrale (prédicats et bornes) |
| `morsehgp3D_v10/src/cloud/cloud.hpp`, `cloud.cpp`, `src/core/*`, `src/sched/*` | — | intégrale (dépendances du générateur) |
| `morsehgp3D_v10/reference/hgp10_ref.py` | 431 | partie catalogue (l. 1–260) |
| `morsehgp3D_v10/CMakeLists.txt`, `cmake/*`, `docs/SPEC_V10.md`, `PASSATION.md` | — | intégrale |
| reçus `catalogue_parallel_assembly`, `catalogue_load_balance`, `catalogue_filter_j2`, `catalogue_assembly_j2`, `catalogue_fitted_split_j2c` (29 septembre) | — | README, reçus d'agents, mutants |
| reçu `g4_session4_j2c_20260929` | — | README, plan, sorties brutes des commandes catalogue |
| audit `audits/audit_continu_20260929/catalogue/AUDIT_CATALOGUE_J2_J2C_20260929.md`, `receipts/audit_geant_developpeur_20260930/TRACKER.md` | — | comme pistes, recontrôlées |
| privé : `build/v10-perf/PLAN_PERF.md`, `build/v10-perf/feuille/`, `build/v10-persist/gpu_design/reduction_algorithmique_cpu/`, `build/v10-integration-r2/` | — | comme pistes ; la variante J3 a été reconstruite et rejouée |

Historique du générateur (`git log`, lu) : créé le 28 septembre à 12:01 (`a0d92fd6e`), racine ajustée à 12:35 (`93710d076`), puis six refontes en 29 heures — feuille v2 à masques (`6615febaf`), assemblage parallèle (`2c7b8130e`), frontière pilotée par la charge (`b58479b99`), filtre J2 (`5565f94fb`), allocateur sans initialisation (`7eee86c53`), boîtes ajustées J2c (`777406b82`). La porte oracle du dépôt date du 28 septembre à 12:35 et n'a jamais été modifiée depuis.

## 2. Méthode

- **Build** : copie des sources sous `/tmp`, `cmake -DCMAKE_BUILD_TYPE=Release`, g++ 13.3, `-O3 -DNDEBUG` sans `-march` (comme les sessions G4, qui utilisent g++ 11.4) ; variante `-DMHGP10_MARCH=x86-64-v3`. Le binaire reproduit les empreintes des reçus : dump du quart 01 `414aa4d47ffe1c55` (K = 5) et `7c46e50a72c08087` (K = 10), dump de la trame 02 `8a850649ff103c1c` (K = 5), grand livre de `ledger_j2c.jsonl`.
- **Compteurs** : `mhgp10_catalogue` sur les trois trames du contrat (08/000000, 08/000100, 08/000200 sans sol), une trame brute avec sol (123 389 points, quantifiée par l'auditeur sous `/tmp`), et les nuages synthétiques `syn_uniform_density` et `syn_clusters_density` à 8 000 et 32 000 points ; K = 5 et 10 ; 1 et 4 fils (`baseline_compteurs.txt`).
- **Temps par étage** : copie instrumentée par `rdtsc` (`sonde_rdtsc.diff`), dont le dump et le grand livre restent identiques à HEAD ; profil d'instructions et de branchements par ligne sous callgrind (`callgrind_resume.txt`).
- **Candidats réels** : une copie vide les triplets, quadruplets et juges de la trame 02 (`sonde_vidage_candidats.diff`) ; quatre microbancs les rejouent (`microbancs/`).
- **Exactitude** : oracle brut indépendant écrit par l'auditeur (`oracle/brute_oracle.cpp`, aucun code commun avec la v10), comparateur enregistrement par enregistrement (`oracle/compare.py`), dix mutants (`mutants.diff`), porte du dépôt rejouée sur ces mutants, invariants d'échelle (`invariants/`).
- **Limites** : pas de `perf` sur cette machine ; pas d'AVX-512 ni de GPU ; aucun temps local n'est une mesure de contrat.

---

## 3. L'algorithme réel, reconstitué depuis le code

Références : `generator.cpp` sauf mention. Notations : $K$ l'ordre maximal servi, $M$ la taille de feuille, $n$ le nombre de sites.

### 3.1 Objet produit (`catalogue.hpp:1-12`, `66-91`)

Toutes les boules critiques (support de 2 à 4 sites affinement indépendants, centre dans l'intérieur relatif de l'enveloppe du support) avec leur intérieur strict $I$ (poids $p$), leur coquille complète $U$, leur plus petit cardinal de support $q_{\min}$ et leur support canonique $S^{*}$, admises si $p+q_{\min}\le K+1$ quand aucun site de la coquille n'est pondéré, et si $p\le K-1$ sinon (`generator.cpp:262`). Sortie en tableaux parallèles dans l'ordre (niveau exact, $S^{*}$), avec un rang dense par niveau exact distinct.

### 3.2 Repères et préparation (`634-651`, `cloud.cpp:23-73`)

- Sites = positions distinctes triées par clé de Morton, poids = multiplicité (`cloud.cpp:38-70`).
- Deux jeux de coordonnées : `C.P` (entiers d'entrée) pour les centres, le recensement et les niveaux ; `C.X = P << 6` (`kT = 6`, l. 22) pour tout ce qui touche les boîtes ; `C.X2` $=\lVert X\rVert^{2}$.
- Refus : `kmax` hors de $[1,12]$, `cloud.bits > 18` (l. 635-636). Moins de 2 sites : catalogue vide.

### 3.3 Racine (`659-674`)

Cube de coin bas égal au minimum des coordonnées et de côté la plus petite puissance de deux strictement supérieure à l'étendue. Tout centre critique est dans l'enveloppe convexe des sites, donc dans ce cube demi-ouvert.

### 3.4 Nœud : `process` (`567-620`) et `filter_node` (`496-559`)

Pour une boîte $Q=[lo,hi)$ et la liste `parent` du nœud père :

1. **Réservoir** (l. 501-516) : les $\min(\lvert parent\rvert,3K)$ sites les plus proches du centre de la boîte, par insertion, clé $(\lVert 2X-(lo+hi)\rVert^{2},\text{rang})$.
2. **Dominateurs Y** (l. 518-531) : le plus petit préfixe du réservoir de poids au moins $3K$ ; `s0` marque le préfixe de poids au moins $K$.
3. **Filtre** (l. 533-557) : un site $x$ est retiré si le poids de ses dominateurs dans Y atteint $K$. $y$ domine $x$ si $A(x)-A(y)>\sum_{i}\max(0,2h_{i}x'_{i}-2h_{i}y'_{i})$ avec $x'=X-lo$, $A=\lVert x'\rVert^{2}$, $h=hi-lo$ : c'est $\max_{c\in\bar{Q}}(\lVert y-c\rVert^{2}-\lVert x-c\rVert^{2})<0$. Comptage sans branchement, d'abord sur `s0` puis sur le reste si le seuil n'est pas atteint. L'ancienne garde est absorbée (un site que la garde retire a au moins $K$ dominateurs dans Y).
4. **Ajustement** (l. 577-596) : $S=Q\cap[env.lo,env.hi+1)$ où `env` est l'enveloppe fermée de la liste filtrée. Liste vide ou $S$ vide sur un axe : nœud ignoré (`skipped_bbox`).
5. **Arrêt ou coupe** (l. 597-609) : on coupe $S$ en deux au milieu de son plus long côté tant que la liste dépasse $M$, que ce côté dépasse 1 unité et que la stagnation (liste inchangée sous un côté de $2^{6}$ unités) n'a pas duré 9 niveaux. Les deux moitiés sont demi-ouvertes et refiltrent la liste du nœud.
6. **Feuille** (l. 610-619) : liste de plus de `max_leaf` (256) sites : refus `resource_exhausted/wide_leaf`, différé à la fin ; sinon énumération sur $S$.

Table $M(K)$ (l. 641) : 12 pour $K\le3$, 16 pour $K\le6$, 24 pour $K\le10$, 28 au-delà.

### 3.5 Frontière et tâches (`675-746`)

Phase en largeur : chaque tour traite toute la frontière par un `parallel_for` (l. 717-721) ; un nœud qui se coupe copie sa liste dans un `shared_ptr` et pousse ses deux enfants (l. 601-606). Dès que la frontière compte $64P$ tâches ($P$ fils), seules les tâches dont la boîte contient plus de $n/(64P)$ sites de leur liste parente sont encore développées (l. 700-716) ; les autres sont gardées. Les tâches gardées sont triées par charge décroissante puis exécutées en profondeur, une par appel de `process` récursif, par un second `parallel_for` de grain 1 (l. 732-744).

### 3.6 Feuille : `enumerate_leaf_masks` (`319-465`)

Copies locales des coordonnées et des poids (l. 324-335), puis quatre étapes :

1. **Masques de dominance** (l. 359-375) : pour chaque paire $(i,j)$, la forme affine $g(c)=\lVert X_{j}-c\rVert^{2}-\lVert X_{i}-c\rVert^{2}$ est évaluée à ses deux coins extrêmes ; $\max g<0$ met $j$ dans `Dom[i]`, $\min g>0$ met $i$ dans `Dom[j]`.
2. **Paires** (l. 378-395) : écartée si l'un domine l'autre (la bissectrice manque la boîte fermée) ou si le poids de `Dom[i] | Dom[j]` dépasse $\theta_{2}=K-1$ ; vivante si ce poids est au plus $\theta_{3}$ ; jugée si le milieu est dans la boîte demi-ouverte.
3. **Triplets** (l. 398-430) : pris parmi les paires vivantes par intersection de masques ; écartés si le poids de l'union dépasse $\theta_{3}$, s'ils sont alignés, ou si la droite des centres manque la boîte fermée (`center_line_meets`, l. 122-144, test du zonogone) ; vivants si le poids est au plus $\theta_{4}$ ; jugés si le triangle est strictement aigu et le centre dans la boîte.
4. **Quadruplets** (l. 433-464) : pris quand les quatre triplets sont vivants ; poids de l'union au plus $\theta_{4}$ ; centre (`center4`), centre dans la boîte, centre strictement intérieur au tétraèdre, puis jugement.

Seuils : $\theta_{3}=K-2$ et $\theta_{4}=K-3$ si tous les poids de la liste valent 1, $K-1$ sinon (l. 341). Les masques tiennent sur 1, 2 ou 4 mots de 64 bits, ou sur un nombre de mots lu à l'exécution au-delà de 256 sites (l. 467-473).

### 3.7 Juge (`227-281`)

Recensement exact du candidat sur toute la liste de la feuille (`geom::side` en `__int128`), avec sortie dès que le poids intérieur dépasse le seuil de la présentation. Coquille égale au générateur : boule régulière, $S^{*}$ = générateur. Sinon coquille étendue : `canonical_support` (l. 147-185) cherche le plus petit support dans l'ordre lexicographique, et un mémo linéaire des $S^{*}$ déjà vus dans la feuille évite la double émission. Puis admission, écriture d'un enregistrement de 104 octets et des identifiants de $I$ et de $U$ dans des `std::vector` propres au fil, et calcul du niveau exact (`emitted_level`, l. 196-221).

### 3.8 Ordre et assemblage (`751-873`)

Collecte d'une clé de 32 octets par boule (approximation `double` du niveau, $S^{*}$, origine), tri parallèle par échantillonnage régulier (`sched/sort.hpp`), repérage en série des bandes où deux approximations voisines diffèrent de moins de $2^{-40}$ en relatif, tri exact de chaque bande par (niveau exact, $S^{*}$), comparaison exacte de tous les voisins, refus si un niveau décroît ou si un $S^{*}$ apparaît deux fois, rangs et décalages par sommes préfixes, copie finale.

---

## 4. Le code confronté à la conception GEN_v2

`GEN_v2.md` s'intitule « conception finale » (28 septembre, 15:19). Le générateur a changé cinq fois ensuite (assemblage parallèle, frontière, J2, allocateur, J2c). Écarts relevés (tous **lus** dans le code ; la colonne « contrôle » indique ce qui a en plus été exécuté).

| Sujet | GEN_v2 | Code à `afb081774` | Contrôle |
|---|---|---|---|
| Arbre | octree, racine fixe $[0,2^{B+T})^{3}$, boîtes dyadiques, second membre par décalage (§ 6.1, § 6.2) | arbre binaire, boîte ajustée à l'enveloppe de la liste, coupe au milieu du plus long côté, produits par axe (l. 567-609) ; racine = cube ajusté au nuage (l. 659-674) | lu |
| Stagnation | « aucune règle de stagnation » (§ 3.10, § 7, constat B1) | `kStagnationLimit = 9` (l. 23-29, 598-611) | lu ; jamais déclenchée sur LiDAR ni synthétique générique (0 feuille bloquée sur toutes les exécutions du § 6.1) |
| Admission pondérée | admission **unique** $p_{w}+q_{\min}\le K+1$, clause pondérée « supprimée » (§ 2.3) | clause pondérée conservée : $p\le K-1$ si la coquille contient un site de poids > 1 (l. 262, `catalogue.hpp:3`) | exécuté : l'oracle de l'auditeur, écrit avec la règle du code, est conforme sur 14 986 boules pondérées |
| Domaine | $B\le24$, voie large i256 au-delà de 19 bits, $K\le16$ (§ 2.1, § 5.3) | $B=18$ figé, refus au-delà (l. 636) ; $K\le12$ (`types.hpp:33`) | exécuté (refus à `--k=13`, à la coordonnée 262 144) |
| Bornes | « chaque borne est un `static_assert` » (§ 5.2) | un seul `static_assert` (l. 33, filtre des nœuds) ; les bornes des centres, du recensement et des niveaux sont des commentaires (`geometry.hpp:1-8`) | lu ; bornes recalculées au § 5.2 |
| Lemme Z | i64 si l'étendue locale tient sur 18 bits, sinon i128 (§ 5.2) | toujours i128 pour les produits finaux (l. 135-141) | lu |
| Allocations | « aucune allocation : arène par tâche, une liste par profondeur » (§ 6.2) | un `std::vector<u32>` par nœud (l. 570), une copie en `shared_ptr` par nœud de frontière (l. 601) | mesuré : 124 982 `malloc` pour 123 159 nœuds (quart 01, K = 5) |
| Enveloppe | k-DOP à 13 directions en option (§ 3.6, § 6.2) | boîte englobante seule (l. 577-594) | lu |
| Mémo des coquilles étendues | par **masque de coquille** U (lemme U), plus raccourci q4 avant le calcul du centre (§ 3.7, § 6.3) | liste linéaire de $S^{*}$, consultée après un recensement complet (l. 253-255) ; pas de raccourci q4 | mesuré : coût en $m^{4}$ sur amas cosphérique (§ 5.3) |
| Limite de feuille | $M_{\mathrm{hard}}=128$ ; la sphère de 144 points (fixture F11) est **refusée** (§ 3.10, § 7) | `max_leaf = 256` (`catalogue.hpp:98`) ; la sphère de 144 points est acceptée | exécuté : 20,1 s à K = 5 pour 2 169 boules |
| Table $M(K)$ | 16 / 24 / 32 (§ 7) | 12 / 16 / 24 / 28 (l. 641) | lu ; calibrage local à 1 fil (reçu J2c), jamais rejoué sur G4 |
| Conversion des niveaux | conversion « fidèle » avec bit collant, seuil de bande $2^{-49}$ (§ 6.4) | Horner naïf en `double` (`geometry.cpp:8-16`), seuil $2^{-40}$ (l. 784) | lu : erreur relative d'au plus 6 arrondis, marge de $2^{10}$ sous le seuil |
| Niveaux publiés | `level_rep[rang]` = plus petit $S^{*}$ du rang, 4 octets ; niveau recalculé à la demande (§ 6.4, § 8) | `Level` complet de 56 octets par rang (`catalogue.hpp:78`) | mesuré (§ 6.5) |
| Format | 11 octets fixes + 4 par identifiant + au plus 4 ; compteurs sur u8 ; table à part pour les coquilles étendues (§ 8) | 42 octets fixes + 4 par identifiant + 56 par niveau ; `support[4]` pour toutes les boules ; `p`, `u`, `n_interior` en u32, `pop_off` en u64 | mesuré : 104,2 et 124,5 octets par boule contre 29,5–33,5 et 43,4–47,4 prévus |
| Digest | `catalogue_digest` SHA-256 sans niveau ni rang, `id_digest` (§ 6.5) | aucun digest dans le code ; les reçus hachent le dump texte de la CLI, qui n'imprime pas les niveaux | lu |
| Interface vers la tour | `LeafOracle` : `locate`, `knn_closed`, `nearest_strict_intruder`, `lookup` (§ 9) | absent ; la tour reconstruit ses requêtes avec un arbre k-d séparé (`cloud/site_tree.*`) | lu (`grep`) |
| Têtes de l'arbre | filtres des nœuds de tête parallèles sur les sites (§ 10.1) | un nœud = un fil, même pour la racine (45 845 sites) | lu, mesuré (§ 6.6) |
| Portes | oracles O1 et O2 sur ≥ 1 000 nuages dont 300 pondérés, K jusqu'à 16 ; fixtures F1–F16 ; juges J1–J6 ; mutants M1–M16 en portes à code 4 ; portes de coût ; P-THREADS, P-PERM, P-MEM (§ 12, § 14) | une seule porte CTest : 40 nuages de 5 à 22 points, K ∈ {1, 2, 3, 5}, poids 1, plus une translation comparée à elle-même | exécuté (§ 5.5) |

`docs/SPEC_V10.md` § 3 (« l'objet tel qu'il est implémenté », 28 septembre) est elle aussi périmée : elle décrit un octree, « l'une de K gardes » et la racine dyadique, tous remplacés le 29 septembre.

Deux écarts vont dans le bon sens et sont à garder : l'arbre binaire ajusté (J2c) retire 33 à 40 % des nœuds et 54 % des feuilles (reçu `catalogue_fitted_split_j2c_20260929`, compteurs rejoués) et rend locale la dégénérescence cosphérique (§ 5.3) ; la garde absorbée dans la dominance (J2) retire un prédicat.

---

## 5. Exactitude

### 5.1 Chaque élagage, son lemme, ses égalités

| # | Décision (lignes de `generator.cpp`) | Lemme (GEN_v2 § 3, reçu J2c § 3) | Traitement des égalités | Contrôle par l'auditeur |
|---|---|---|---|---|
| E1 | retirer un site dont les dominateurs dans Y pèsent au moins $K$ (536-556) | D, D-loc, proposition L : la liste reste K-certifiée quel que soit Y | dominance **stricte** (`pa - ya > d`) : un site à égalité en un point de la boîte fermée ne domine pas ; un site ne se domine pas lui-même | mutant `m_filterK` tué par l'oracle (6 contrôles sur 8) |
| E2 | ignorer un nœud de liste vide ou de boîte ajustée vide (573-594) | K (enveloppe) et lemme de l'ajustement : un centre admis est dans l'enveloppe fermée de la liste | borne haute `env.hi + 1`, demi-ouverte : un centre posé sur la face haute de l'enveloppe est gardé | mutant `m_fit` (sans le `+ 1`) tué 8 sur 8, et par la porte du dépôt (120 écarts sur 161) |
| E3 | couper en deux moitiés demi-ouvertes (600-607) | partition de $S$ : chaque centre dans une seule feuille | centre sur le plan de coupe : moitié haute, des deux côtés du même test (`center_in_box`, `midpoint_in_box`) | oracle sur grilles entières (centres sur des plans de coupe), 0 doublon |
| E4 | arrêter la descente (taille, côté ≤ 1, stagnation) (598-599) | aucun lemme requis : toute feuille à liste certifiée est exacte (théorème C) | — | mutant `m_stag` (limite 1) : dumps identiques sur 9 entrées, seul le coût change |
| E5 | masques de dominance de la feuille (359-374) | M : un dominateur d'un site de coquille est strictement intérieur | `< 0` et `> 0` stricts sur la boîte fermée | mutant `m_dom` tué 6 sur 8 |
| E6 | écarter une paire dont l'un domine l'autre (382) | bissectrice = non-dominance mutuelle | bissectrice tangente à la boîte fermée : gardée | idem |
| E7 | poids de l'union > $\theta_{2}$ ; vivante si ≤ $\theta_{3}$ (385-389) | M et S (survie du support canonique) | `>` pour rejeter, `<=` pour garder | mutant `m_th3` tué 7 sur 8 |
| E8 | milieu dans la boîte (391) | unicité de la feuille du centre | $2\,lo\le A+B<2\,hi$ en entiers | oracle (aucune boule en double) |
| E9 | triplet : trois paires vivantes, poids ≤ $\theta_{3}$ (402-412) | S | — | `m_th3` |
| E10 | triplet aligné (414) | support affinement indépendant | produit vectoriel exactement nul | oracle sur 40 points alignés et sur un plan |
| E11 | droite des centres hors de la boîte (416) | Z (zonogone) | test **fermé** : rejet seulement si `l > r` ; la tangence est gardée | formule revérifiée à la main (§ 5.2) ; mutant `m_zono` (`>=`) tué 6 sur 8 |
| E12 | triplet vivant si poids ≤ $\theta_{4}$ (418-422) | S | — | — |
| E13 | q3 jugé si triangle strictement aigu et centre dans la boîte (424-426) | centre dans l'intérieur relatif ⟺ triangle strictement aigu | triangle rectangle : la boule sort par sa paire antipodale, coquille étendue | oracle (14 567 coquilles étendues) |
| E14 | quadruplet : quatre triplets vivants, poids ≤ $\theta_{4}$ (442-453) | S | le test de poids est inopérant quand $\theta_{4}\le2$ (0 rejet sur 9 380 910 à K = 5, mesuré) | — |
| E15 | q4 jugé si non coplanaire, centre dans la boîte, strictement intérieur (456-459) | support de cardinal 4 | centre sur une face : la boule sort en q3 ou q2 | oracle |
| E16 | sortie du recensement dès $p>\theta$ (239) | S et théorème C (b) | `>` strict | mutant `m_census` tué 8 sur 8 |
| E17 | mémo sur $S^{*}$ des coquilles étendues (253-255) | $S^{*}$ identifie la boule | — | oracle : 0 doublon, $S^{*}$ égal au plus petit support (385 226 boules) |
| E18 | admission (262) | W pour la règle positionnelle ; clause pondérée = sur-ensemble | — | oracle pondéré de l'auditeur (règle du code) |
| E19 | ordre : clé `double`, bandes à $2^{-40}$, tri exact (773-799) | erreur relative de la clé < $2^{-50}$ | niveaux exacts égaux regroupés | voir le défaut de contrôle au § 5.5 (`m_band`) |

Le filtre E14 se démontre inutile aux petits ordres : si les quatre unions de trois masques ont un poids au plus $t$ et l'union des quatre un poids $>t$, chaque masque porte un élément qu'il est seul à porter, donc chaque union de trois en compte au moins 3 ; il faut $t\ge3$, soit $K\ge6$ en feuille sans poids.

### 5.2 Bornes arithmétiques, recalculées

Avec $E<2^{18}$ la plus grande différence de coordonnées d'entrée :

| Quantité | Borne recalculée | Type | Verdict |
|---|---|---|---|
| centre q3 : numérateur, dénominateur | $24E^{5}<2^{94{,}6}$ ; $24E^{4}<2^{76{,}6}$ | i128 | tient |
| centre q4 : numérateur, dénominateur | $18E^{4}<2^{76{,}2}$ ; $12E^{3}<2^{57{,}6}$ | i128 | tient |
| recensement q3 (`geom::side`) : chaque membre | $144E^{6}<2^{115{,}2}$ | i128 (comparaison sans soustraction) | tient |
| centre dans la boîte, q3 | $<2^{101{,}6}$ | i128 | tient |
| intérieur du tétraèdre (`orient_center`, appelé seulement sur un centre de forme q4) | $180E^{6}<2^{115{,}5}$ | i128 | tient ; la garde `qgen >= 4` (l. 171) est ce qui l'autorise |
| filtre des nœuds : clé du réservoir, second membre | $27\cdot2^{48}$ ; $3\cdot2^{49}$ | i64 | tient (`static_assert` l. 33) |
| dominance de feuille (coordonnées absolues) | $<2^{51{,}2}$ | i64 | tient |
| lemme Z : $P$, mineurs, produits finaux | $<2^{52{,}2}$ ; $<2^{49}$ ; $<2^{77{,}2}$ | i64, i64, i128 | tient |
| niveaux et comparaison | num $972E^{8}<2^{153{,}9}$, dén $144E^{6}<2^{115{,}2}$, produits croisés $<2^{269{,}1}$ | I192, I128, 320 bits | tient |

Les bornes de `geometry.hpp:1-8` sont écrites pour des différences $<2^{19}$ : elles sont justes et prudentes. Elles ne sont gardées par aucun `static_assert`, et le générateur refuse tout nuage de plus de 18 bits (l. 636) : c'est ce refus, et lui seul, qui les protège.

Formule du lemme Z revérifiée : avec $u=A-B$, $v=A-D$, $P=(2(\lVert A\rVert^{2}-\lVert B\rVert^{2})-2(lo+hi)\cdot u,\ 2(\lVert A\rVert^{2}-\lVert D\rVert^{2})-2(lo+hi)\cdot v)$, la droite rencontre la boîte fermée si et seulement si $\lvert v_{k}P_{0}-u_{k}P_{1}\rvert\le2\sum_{j\ne k}h_{j}\lvert u_{k}v_{j}-v_{k}u_{j}\rvert$ pour chaque axe $k$ où $(u_{k},v_{k})\ne(0,0)$ ; le code (l. 125-143) calcule exactement cela.

### 5.3 Bords du domaine (exécuté, `bords/`)

| Cas | Comportement observé |
|---|---|
| coins du cube $[0,262143]^{3}$, points quasi extrêmes, grand triangle | 20 contrôles conformes à l'oracle Fraction du dépôt (K = 1, 2, 3, 5) |
| 3 000 points uniformes sur tout $[0,262143]^{3}$ (étendues maximales), nuages en trois bandes aux extrêmes | ASan et UBSan muets (11 exécutions, K = 5 à 12) ; Euler $\chi_{k}=1$ pour $k=1..10$ sur le catalogue à K = 12 (1 690 156 boules) ; restriction K = 10 contre K = 12 : égalité |
| un site ; deux sites | catalogue vide, code 0 ; une boule, un nœud |
| $K$ plus grand que le nombre de points | pas de refus, catalogue correct (8 coins, K = 12 : 27 boules) |
| 40 points alignés ; 153 sites coplanaires avec doublons | conformes à l'oracle indépendant |
| doublons de position | catalogue conforme (règle du code) ; la tour, elle, refuse toute entrée pondérée (`tower.cpp`, raison `multiplicity_unsupported`) |
| trois trames du contrat ; deux trames brutes avec sol | aucun doublon à 1 mm (123 389 et 126 267 positions distinctes) ; aucun refus ; 0 feuille bloquée |
| amas cosphérique à intérieur vide (points entiers d'une sphère) | 8 feuilles bloquées portant tout l'amas ; coût en $m^{4}$ : 72 points 1,0 s, 96 points 3,4 s, 144 points 20,1 s, 168 points 39,3 s à K = 5 (un fil) pour 933 à 2 757 boules ; 312 points : refus `wide_leaf` |
| `--leaf` inférieur à $K+3$ | descente non bornée, aucun budget de nœuds : 8 coins, K = 5, `--leaf=6` dépasse 15 s contre 0,00 s par défaut |
| fichier tronqué (2 points + 1, 4, 8 ou 11 octets) | accepté, `status ok`, n = 2 (`mhgp10_catalogue.cpp:40` ; même boucle dans les CLI tour et cluster) |
| `--k=abc`, `--threads=x` | exception non rattrapée, fin par signal 6 ; `--k=2x` et `--leaf=-1` acceptés |
| plus de $2^{32}$ boules | aucun refus (l. 767, 803 : compteurs u32) ; hors d'atteinte avant le mur mémoire — lu, non exécuté |

### 5.4 Contre-épreuves exécutées par l'auditeur

1. **Oracle brut indépendant** (`oracle/`). Écrit pour cet audit en C++ : tous les sous-ensembles de 2, 3 et 4 positions, centre par les coordonnées barycentriques du système de Gram en `__int128`, déduplication par (centre, rayon) réduits, recensement brut, support canonique par ordre de Morton recalculé. Le comparateur exige le même multiensemble $(q_{\min},p,u,I,U)$, le même $S^{*}$, des rangs denses depuis 0 égaux au rang du niveau exact, les drapeaux et les poids. **43 contrôles conformes, 385 226 boules** (14 567 à coquille étendue, 14 986 à coquille pondérée), K ∈ {1, 2, 3, 5, 10, 12}, dont : feuilles forcées à 110, 200 et 280 sites (masques à 2 et 4 mots, puis nombre de mots lu à l'exécution), sphères de 144 et 312 points, nuages collés aux deux bords du domaine u18.
2. **L'oracle n'est pas vert par vacuité** : il tue les sept mutants qui changent le catalogue sur ses entrées (`oracle/mutants_oracle.txt`).
3. **Invariant de restriction** à l'échelle : le catalogue à $K$ égale, enregistrement par enregistrement et dans le même ordre, la restriction du catalogue à $K+2$ aux boules $p+q_{\min}\le K+1$. Égalité sur le quart 01 (8 074 sites) à K = 3, 5, 10 et sur deux nuages synthétiques de 8 000 points.
4. **Identité d'Euler par ordre** à l'échelle. Pour un nuage sans coquille étendue ni doublon, avec $N(q,p)$ le nombre de boules de support $q$ et d'intérieur $p$ : $\chi_{k}=[k=1]\,n+\sum_{q=2}^{4}\sum_{j=1}^{q}(-1)^{q-j}\binom{q-1}{j-1}N(q,k-j)=1$. Le coefficient binomial est la multiplicité du point critique : près du centre, la $j$-ième statistique d'ordre des $q$ distances a pour lien inférieur l'ensemble des directions où au moins $j$ formes linéaires sur $q$ sont négatives, qui a le type d'homotopie du squelette de dimension $q-1-j$ du bord d'un simplexe à $q$ sommets, soit un bouquet de $\binom{q-1}{j-1}$ sphères (esquisse de l'auditeur, non rédigée en preuve ; sans ce coefficient l'identité est fausse dès $k=2$). Vérifiée numériquement ($\chi_{k}=1$) pour $k=1..5$ sur deux nuages de 8 000 points (catalogues à K = 7 de 1,30 M et 1,18 M boules) et pour $k=1..10$ sur le catalogue à K = 12 (4 947 753 boules). Sur les nuages à coquilles étendues (3 à 48), le comptage naïf s'écarte de quelques unités, comme attendu : il faut alors le quotient local de la tour.
5. **Invariance au nombre de fils** : dump identique à 1, 3, 8 et 48 fils (quart 01, K = 10), compteurs de l'arbre égaux.
6. **Sanitizers** : build ASan + UBSan sans reprise, 11 exécutions (extrêmes du domaine, 3 000 points sur tout le domaine u18 à K = 12, coquilles étendues, poids, sphère de 144 points, quart 01 à K = 10) : aucun rapport. C'est le contrôle empirique des bornes du § 5.2 aux étendues maximales, que les trames LiDAR n'atteignent pas (listes de feuille ≤ 15 bits).
7. **Épingles** : trame 02, 1 407 885 boules à K = 5 (518 233 / 746 547 / 143 105 par $q_{\min}$, 572 étendues) et 5 483 320 à K = 10 (977 534 / 3 019 193 / 1 486 593, 1 301 étendues) ; trame brute avec sol 2 822 052 boules à K = 5 (1 243 étendues) et 11 387 391 à K = 10 (2 598 083 / 6 665 118 / 2 124 190, 2 399 étendues) : égaux à GEN_v2 § 11.2.

Ce que ces contrôles n'établissent pas : la complétude sur une trame LiDAR entière à coquilles étendues n'est jugée ici que par la restriction (cohérence entre deux K) et par les épingles héritées de la v9 — la lentille L01 rapporte, avec un juge d'Euler qui traite les coquilles étendues, une somme égale à 1 aux ordres 1 à 10 sur les trois trames (`L01_MATH_CATALOGUE.md`, exécution E4 ; cité, non rejoué ici). Une borne de pire cas du nombre de nœuds n'existe pas.

### 5.5 Les trous de la porte permanente

`tests/oracle/test_catalogue_oracle.py` (rejouée : 161 contrôles, 0 écart, 14 132 boules, 277 s) tire 40 nuages de **5 à 22 points** (l. 114-130), K ∈ {1, 2, 3, 5} (l. 140), sans doublon, à 2 fils (l. 48), et compare les ensembles $(q,p,I,U)$, la validité du support et la cohérence des rangs. Elle ne voit donc jamais : une liste de plus de 22 sites, un masque de plus d'un mot, la coupe de frontière (il faut $64P$ tâches), $K>5$, un poids, la minimalité de $S^{*}$, la valeur des niveaux, l'ordre à niveau égal. Sa vérification de translation (3 000 points) compare le binaire à lui-même.

Dix mutants, joués contre la porte du dépôt, l'oracle de l'auditeur et les invariants d'échelle (`mutants.diff`, `oracle/porte_du_depot_sur_mutants.txt`, `invariants/invariants_sur_mutants.txt`) :

| Mutant | Faute | Porte du dépôt | Oracle de l'auditeur | Invariants à 8 000 points |
|---|---|---|---|---|
| `m_fit` | boîte ajustée sans `+ 1` | tué (120 / 161) | tué 8 / 8 | — |
| `m_th3` | seuil des paires vivantes à $K-3$ | tué (118) | tué 7 / 8 | restriction : tué ; Euler à K + 2 : aveugle (boules au bord de l'admission) |
| `m_zono` | zonogone strict | tué (25) | tué 6 / 8 | Euler : aveugle (nuage générique, aucune tangence) |
| `m_dom` | dominance de feuille large | tué (63) | tué 6 / 8 | — |
| `m_census` | sortie du recensement à $p\ge\theta$ | tué (119) | tué 8 / 8 | restriction : tué |
| `m_multiword` | `bits_above` faux au-delà du premier mot | **survit** (0 / 161) | tué seulement par la feuille forcée (`--leaf=120`) | aveugles |
| `m_frontier` | tâche sans site dans sa boîte abandonnée | **survit** (0 / 161) | — | Euler : tué ($\chi=12,23,3,14,7$) ; restriction : tué ; dump : 208 290 boules à 1 fil, 200 901 à 4 fils au lieu de 210 424, `status ok` |
| `m_band` | bandes flottantes jamais réparées | **survit** (0 / 161) | non vu (l'ordre à niveau égal n'est pas comparé) | aveugles ; visible seulement sur la trame 02 entière : 1 ligne du dump sur 1 407 885, **sans refus** |
| `m_stag` | stagnation après 1 niveau | survit (équivalent pour l'exactitude) | — | — |
| `m_filterK` | dominance d'arbre comptée à $K-1$ | interrompu par l'auditeur (le mutant ne termine pas à K = 1 : aucun budget de nœuds) | tué 6 / 8 | — |

Trois enseignements. (a) La frontière et les masques larges ne sont gardés que par des différentiels ponctuels contre le binaire précédent, jamais par une porte permanente. (b) Le contrôle final (l. 830) refuse une inversion stricte de niveaux et un $S^{*}$ répété, mais **pas** un ordre faux entre deux boules de même niveau exact : `m_band` publie un catalogue non canonique sans refus. (c) Les deux invariants d'échelle sont complémentaires — Euler voit une perte de boules d'intérieur ≤ $K-1$, la restriction voit les seuils au bord de l'admission — et aucun n'est une porte du dépôt.

---

## 6. Performance

### 6.1 Compteurs déterministes (mesuré ; identiques à 1 et 4 fils)

Build Release sans `-march`. « Candidats » = tests de dominance de paires + triplets testés + quadruplets testés.

| Entrée | Sites | K | Boules | Boules par site | Nœuds (par site) | Feuilles | m moyen / max | Tests du filtre par boule | Candidats par boule (paires / triplets / quadruplets) | Recensements par boule |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|
| trame 00 sans sol | 39 885 | 5 | 1 306 696 | 32,8 | 781 865 (19,6) | 352 967 | 13,9 / 16 | 308 | 56,6 (24,8 / 23,9 / 7,8) | 2,50 |
| trame 00 sans sol | 39 885 | 10 | 5 512 670 | 138,2 | 1 137 143 (28,5) | 530 147 | 21,6 / 24 | 237 | 63,1 (21,6 / 26,5 / 15,0) | 2,09 |
| trame 01 sans sol | 35 551 | 5 | 1 095 926 | 30,8 | 636 589 (17,9) | 284 367 | 13,9 / 16 | 299 | 54,0 (23,8 / 22,7 / 7,5) | 2,42 |
| trame 01 sans sol | 35 551 | 10 | 4 383 302 | 123,3 | 931 949 (26,2) | 430 465 | 21,6 / 24 | 244 | 63,4 (22,1 / 26,5 / 14,8) | 2,02 |
| trame 02 sans sol | 45 845 | 5 | 1 407 885 | 30,7 | 734 083 (16,0) | 323 224 | 13,9 / 16 | 287 | 48,1 (21,1 / 20,3 / 6,7) | 2,35 |
| trame 02 sans sol | 45 845 | 10 | 5 483 320 | 119,6 | 1 065 585 (23,2) | 485 833 | 21,6 / 24 | 235 | 57,9 (19,9 / 24,3 / 13,7) | 1,97 |
| 08/000000 brute avec sol | 123 389 | 5 | 2 822 052 | 22,9 | 1 759 813 (14,3) | 736 981 | 13,8 / 16 | 362 | 50,7 (23,6 / 21,3 / 5,7) | 2,36 |
| 08/000000 brute avec sol | 123 389 | 10 | 11 387 391 | 92,3 | 2 567 193 (20,8) | 1 144 086 | 21,5 / 24 | 278 | 58,8 (22,4 / 24,8 / 11,6) | 1,88 |
| uniforme | 8 000 | 5 | 594 381 | 74,3 | 157 633 (19,7) | 78 812 | 14,1 / 16 | 117 | 32,8 (12,5 / 14,6 / 5,7) | 2,79 |
| uniforme | 8 000 | 10 | 3 115 804 | 389,5 | 214 701 (26,8) | 107 351 | 21,8 / 24 | 73 | 28,6 (7,9 / 12,1 / 8,7) | 2,31 |
| uniforme | 32 000 | 5 | 2 533 312 | 79,2 | 666 531 (20,8) | 333 248 | 14,1 / 16 | 123 | 32,5 (12,4 / 14,4 / 5,7) | 2,80 |
| uniforme | 32 000 | 10 | 13 487 653 | 421,5 | 918 599 (28,7) | 459 300 | 21,9 / 24 | 76 | 28,5 (7,8 / 12,0 / 8,7) | 2,34 |
| huit amas | 8 000 | 5 | 555 892 | 69,5 | 175 451 (21,9) | 86 936 | 14,1 / 16 | 144 | 38,3 (14,7 / 17,0 / 6,6) | 2,76 |
| huit amas | 32 000 | 10 | 12 864 247 | 402,0 | 1 032 179 (32,3) | 514 615 | 21,8 / 24 | 92 | 32,8 (9,1 / 13,9 / 9,8) | 2,33 |

Lecture :

- Le coût suit la sortie : de 8 000 à 32 000 points, les tests du filtre par boule ne montent que de 5,6 % (117 → 123, profondeur de l'arbre) et les candidats par boule ne bougent pas.
- Un nuage volumique émet 2,1 à 3,5 fois plus de boules par site qu'une trame LiDAR (surfaces) ; à K = 10, 32 000 points uniformes donnent 13,5 M boules.
- Le sol double la sortie (2,82 M boules à K = 5 contre 1,31 M pour la même trame sans sol), à coûts unitaires voisins (tests du filtre par boule +17 %, candidats par boule −10 %).
- Temps locaux à un fil, indicatifs (charge 4 à 18) : 3,8 à 6,9 µs par boule ; à 4 fils sur 4 cœurs chargés : 1,2 à 3,0 µs. Détail dans `baseline_compteurs.txt`.

### 6.2 Répartition du temps par étage (mesuré, sonde `rdtsc`, un fil)

Parts de l'étage des boîtes (frontière + tâches), qui fait 89 % du temps du catalogue à un fil ; l'ordre et l'assemblage font les 11 % restants (0,69 s sur 6,2 s à K = 5 ; 2,7 s sur 28 s à K = 10).

| Entrée | Cycles par boule | Filtre des nœuds (dont réservoir / noyau) | Dominance de feuille | Paires | Triplets | Quadruplets | Juges (dont recensement / émission) |
|---|---:|---|---:|---:|---:|---:|---|
| trame 02, K = 5 | 10 200 | 22,7 % (5,6 / 15,5) | 6,0 % | 6,9 % | 29,1 % | 17,6 % | 16,6 % (9,0 / 5,9) |
| trame 02, K = 10 | 11 500 | 14,2 % (2,8 / 10,6) | 4,6 % | 4,6 % | 29,0 % | 29,6 % | 17,5 % (10,4 / 5,8) |
| uniforme 8 000, K = 5 | 7 800 | 12,9 % (3,4 / 8,4) | 5,2 % | 5,9 % | 29,9 % | 20,5 % | 24,7 % (14,0 / 8,2) |
| uniforme 8 000, K = 10 | 7 700 | 7,2 % (1,5 / 5,2) | 3,3 % | 3,3 % | 25,2 % | 30,4 % | 30,2 % (18,6 / 9,4) |
| uniforme 32 000, K = 5 | 10 000 | 15,3 % (3,6 / 10,5) | 4,8 % | 5,6 % | 28,9 % | 20,8 % | 23,7 % (12,9 / 8,6) |
| uniforme 32 000, K = 10 | 8 700 | 7,8 % (1,6 / 5,7) | 3,2 % | 3,2 % | 24,9 % | 31,1 % | 29,5 % (18,4 / 9,1) |

Coûts unitaires (trame 02, K = 5 puis K = 10, tics TSC à 2,445 GHz) : 5,5 et 5,2 cycles par test de dominance du filtre ; 26 et 37 cycles par site parent pour la sélection du réservoir ; 145 et 135 cycles par triplet candidat ; 271 et 248 cycles par quadruplet candidat ; 34 et 31 cycles par site recensé ; 608 et 671 cycles par boule émise. Deux passes à K = 5 donnent les mêmes parts à 0,2 point près (`profil_rdtsc_etages.txt`). Ces parts recoupent, sans en dépendre, le profil privé du 29 septembre (`build/v10-perf/feuille/profil/base/profil.txt`).

### 6.3 L'entonnoir des candidats (mesuré, trame 02)

| Étape | K = 5 | K = 10 |
|---|---:|---:|
| paires de feuille | 29 738 790 | 108 996 239 |
| … dont la bissectrice coupe la boîte | 21 785 596 | 66 214 213 |
| … de poids admissible | 18 894 584 | 57 829 684 |
| … de milieu dans la boîte (jugées) | 922 737 | 1 508 565 |
| triplets candidats (masques) | 28 874 710 | 135 684 632 |
| … de poids admissible | 28 587 237 | 133 332 887 |
| … dont la droite touche la boîte | 20 416 010 | 94 209 150 |
| … aigus | 9 082 255 | 40 039 358 |
| … de centre dans la boîte (jugés) | 1 903 388 | 5 904 117 |
| quadruplets candidats | 9 380 910 | 75 313 530 |
| … de centre dans la boîte | 4 959 056 | 39 627 374 |
| … strictement intérieurs (jugés) | 478 586 | 3 405 462 |
| recensements | 3 304 711 | 10 818 144 |
| … interrompus par le seuil | 1 896 818 | 5 334 806 |
| boules émises | 1 407 885 | 5 483 320 |

- Rendement global : 2,1 % (K = 5) et 1,7 % (K = 10) des candidats deviennent une boule.
- Le filtre le plus sélectif des quadruplets — le centre est-il strictement dans le tétraèdre : 7,4 % de oui — est appliqué **en dernier**, après le centre et le test de boîte en `__int128` sur les 9,38 M candidats.
- Un même triplet est testé en moyenne dans 3,8 feuilles (7 532 667 triplets distincts pour 28 587 237 incidences, jusqu'à 69 feuilles) ; un quadruplet dans 1,36 feuille (`feuilles_histogrammes_et_duplication.txt`).
- 57 % des recensements s'arrêtent au seuil, mais tard : 11,5 sites visités sur 14 en moyenne.

### 6.4 Boucles chaudes, branchements, vectorisation

Callgrind avec simulation des branchements, quart 01, K = 5 (`callgrind_resume.txt`) : 6,94 G instructions pour 210 424 boules (33 000 par boule), 390 M branchements dont **41,5 M mal prédits (10,6 %)**, soit 197 par boule.

- `enumerate_leaf_masks<1>` et les prédicats qui y sont inlinés : 45 % des instructions, 63 % des mauvaises prédictions. `filter_node` : 35 % des instructions (le noyau de dominance à lui seul 22,6 %), 21 % des mauvaises prédictions.
- Lignes les plus mal prédites : la valeur absolue en `__int128` du lemme Z (`if (l < 0) l = -l`, 9,4 % des mauvaises prédictions), la boucle courte du noyau du filtre (9,3 %), le choix du coin par le signe dans la dominance de feuille (7,1 %, compilé en sauts), l'insertion du réservoir (7,0 %), les boucles d'itération de bits des triplets et quadruplets (13,8 % au total), l'aiguïté (4,1 %), la comparaison du recensement (3,5 %).
- **Aucune vectorisation de la feuille.** L'assembleur de `enumerate_leaf_masks<1>` compte 2 246 instructions dont une seule opération vectorielle entière, 172 `imul`, 35 multiplications 128 bits et 116 sauts conditionnels ; `-march=x86-64-v3` n'y change que le `popcnt`.
- **Le noyau du filtre n'est vectorisé qu'avec `-march`.** En build par défaut, `filter_node` ne contient aucune instruction vectorielle entière ; avec `x86-64-v3` il est auto-vectorisé (80 `vpcmpgtq`, 80 `vpsubq`, 64 `vpaddq`). Les sessions G4 construisent **sans** `-march` (`receipts/g4_session4_j2c_20260929/results/provenance/cmakecache.txt` : `-O3 -DNDEBUG`). GEN_v2 annonçait 1,5 cycle par test ; le build mesuré sur G4 en coûte 5,2 à 5,5 ici. Effet de `x86-64-v3` : −39 % d'instructions dans `filter_node` (2,41 → 1,47 G, callgrind, dump identique), ×1,08 en temps sur tout l'étage (médianes de 3 passes alternées). Le gain reste modeste parce que la boucle vectorisée parcourt les dominateurs (5 puis 10 au plus par site) et non les sites (40 par nœud en moyenne). Essai de l'auditeur avec les boucles interverties, comme l'écrivait GEN_v2 § 6.2 (`essai_filtre_boucles_interverties.diff`) : dumps identiques, 1,26 G instructions avec `x86-64-v3` (−48 % par rapport au build par défaut) malgré 14 % de tests en plus ; les temps locaux, pris sous une charge de 17 à 24, ne départagent pas les deux ordres.
- Allocations : un `malloc` par nœud (124 982 pour 123 159 nœuds), 0,2 % des instructions ; ce n'est pas un poste de coût à un fil.

### 6.5 Mémoire (mesuré)

| | K = 5 (trame 02) | K = 10 (trame 02) |
|---|---:|---:|
| identifiants par boule ($\lvert I\rvert+\lvert U\rvert$) | 4,63 | 8,10 |
| niveaux distincts par boule | 0,781 | 0,895 |
| catalogue résident : 42 o + 4 o par identifiant + 56 o par niveau | 104,2 o par boule (147 Mo) | 124,5 o par boule (683 Mo) |
| transitoire : enregistrement de 104 o + identifiants + clé de tri de 32 o + 1 o | 155,5 o par boule (219 Mo) | 169,4 o par boule (929 Mo) |
| pic RSS mesuré, 1 fil (4 fils) | — | 297 (319) o par boule |
| pic RSS du reçu G4, 1 fil (48 fils) | 269 (309) o par boule | 297 (341) o par boule |
| format conçu (GEN_v2 § 8) | 29,5 à 33,5 o par boule | 43,4 à 47,4 o par boule |

- Le niveau exact (`geom::Level`, 56 octets : deux entiers larges avec drapeau de signe) est écrit dans chaque enregistrement transitoire puis recopié par rang, alors qu'il se recalcule depuis $S^{*}$.
- Pour une coquille régulière (99,96 % des boules), `support[4]` (16 o) répète la coquille déjà présente dans `pop`.
- Aucun de ces tableaux ne passe par `Buffer<T>` ni par un `MemoryBudget` (`catalogue.hpp:32-55`, `UninitVector` = `std::vector`) : le générateur ne peut pas refuser pour mémoire ; une allocation qui échoue lève `std::bad_alloc`.
- L'émission coûte 608 à 860 cycles par boule (5,7 à 9,4 % de l'étage) : écriture de l'enregistrement, deux `insert`, calcul du niveau, croissance par doublement des `std::vector`.

### 6.6 Parallélisme : ce que disent le reçu G4 et les compteurs

Reçu `receipts/g4_session4_j2c_20260929` (EPYC 9B45, 24 cœurs et 48 fils, g++ 11.4, `-O3` sans `-march`, un processus neuf par commande), trame 02 — **lu** dans les sorties brutes :

| Poste | K = 5, 1 fil (s) | K = 5, 48 fils (s) | Gain | K = 10, 1 fil (s) | K = 10, 48 fils (s) | Gain |
|---|---:|---:|---:|---:|---:|---:|
| frontière en largeur | 0,0559 | 0,0266 | ×2,1 | 0,0953 | 0,0396 | ×2,4 |
| tâches des boîtes | 3,5085 | 0,1033 | ×34,0 | 14,7407 | 0,4416 | ×33,4 |
| ordre (collecte, tri, bandes) | 0,1639 | 0,0138 | ×11,9 | 0,6216 | 0,0481 | ×12,9 |
| assemblage : comparaisons | 0,1483 | 0,0051 | ×29 | 0,6056 | 0,0197 | ×31 |
| assemblage : rangs | 0,0771 | 0,0096 | ×8,0 | 0,3171 | 0,0397 | ×8,0 |
| assemblage : copie | 0,0818 | 0,0040 | ×20 | 0,3631 | 0,0163 | ×22 |
| hors étages (retour de `build_catalogue`) | 0,0042 | 0,0087 | — | 0,0183 | 0,0229 | — |
| **catalogue** | **4,0396** | **0,1711** | ×23,6 | **16,7617** | **0,6279** | ×26,7 |

Parts à 48 fils : boîtes 60 % (K = 5) et 70 % (K = 10) ; frontière 15,5 % et 6,3 % ; ordre et assemblage 19 % et 20 % ; hors étages 5,1 % et 3,6 %.

- **La frontière en largeur ne passe pas à l'échelle.** Compteurs rejoués (`frontiere_parts.txt`, mêmes nombres de tâches que le reçu : 366, 10 287 et 19 482 à 1, 24 et 48 fils) : à 48 fils, le seuil de charge vaut $n/(64P)=14$ sites, soit la taille d'une feuille ; la phase en largeur traite alors 22 441 nœuds (3,1 % des nœuds) en **29 tours à barrière**, mais ces nœuds portent **25,4 % des visites de sites du filtre** (7,8 M sur 30,7 M) ; à K = 10 : 2,1 % des nœuds, 16,6 % des visites, 29 tours. Les premiers tours ont 1, 2, 4… nœuds de 45 845 sites chacun, filtrés par un seul fil ; chaque tour paie un réveil du pool, une copie de liste par nœud et une concaténation en série. Les tâches finales sont minuscules (37 nœuds en moyenne).
- **L'étage des rangs** gagne ×8 quand ses voisins gagnent ×20 à ×31 : la boucle (l. 827-834) incrémente par boule des cases voisines de `inc`, `pops` et `bad` partagées entre fils, et relit au hasard les enregistrements de 104 octets. Essai local (`essai_rangs_accumulateurs_locaux.diff`, 8 fils, 7 passes alternées) : médiane 0,067 → 0,059 s (−12 %), minimum 0,064 → 0,047 s ; l'étage de copie, témoin, ne bouge pas.
- **Le repérage des bandes est en série** (l. 782-790) et lit toutes les clés.
- **Hors étages** : c'est la destruction des tampons transitoires (155 à 170 octets par boule) au retour de la fonction — mesuré en local : la préparation interne fait moins de 10 % de ce poste (`mesures_diverses.txt` § 10) ; 23 ms à K = 10 sur G4.
- **Plancher de travail.** À 48 fils le processus consomme 6,04 s de CPU à K = 5 (3,95 s à un fil : le SMT renchérit le travail de 53 %) et 25,6 s à K = 10. Même avec 48 fils occupés en permanence, le mur ne descendrait pas sous $6{,}04/48\approx0{,}126$ s à K = 5 (0,132 s en comptant les 0,31 s de temps système) ni sous $25{,}6/48\approx0{,}53$ s à K = 10. Aucune correction d'ordonnancement ne suffit : il faut réduire le travail.

### 6.7 Microbancs sur les candidats réels (mesuré, `microbancs/`)

Les candidats de la trame 02 ont été vidés par une copie du générateur (9 380 910 quadruplets, 28 587 237 triplets et 3 304 711 juges à K = 5 ; un sur huit à K = 10), puis rejoués hors contexte. Chaque variante doit rendre les mêmes décisions, élément par élément.

**Quadruplets** (cycles par candidat, minimum de 3 à 5 passes, charge 16 à 19) :

| Variante | K = 5 | K = 10 | Rapport à HEAD |
|---|---:|---:|---:|
| A — séquence du HEAD : centre, boîte, intérieur, tout en `__int128` | 297 à 325 | 305 à 308 | 1 |
| B — exacte, réordonnée : intérieur strict d'abord, par les coordonnées barycentriques entières du système de Gram | 113 à 126 | 120 à 127 | ×2,4 à ×2,7 |
| C — filtre en `double` à borne d'erreur a priori sur ces barycentriques, repli exact B si indécis | 70 à 91 | 80 à 83 | ×3,6 à ×4,2 |
| D — le même filtre par blocs de 1 024 en tableaux séparés, sans branchement, puis exact sur les survivants ; hors conversion d'entrée | 39 (base), 28 (`x86-64-v3`) | 37, 28 | ×8 à ×11 |

- 0 désaccord sur 9 380 910 et 9 388 620 quadruplets ; replis exacts de C : 0,040 % et 0,014 %. Le coût A recoupe la mesure en situation (271 et 248 cycles).
- La borne d'erreur ($2^{-44}M^{3}$, $M$ la plus grande entrée de Gram) est une majoration de l'auditeur, validée ici par l'accord des décisions, **pas une preuve rédigée**.
- La conversion d'entrée du banc (enregistrements entiers lus en mémoire vive vers des tableaux de `double`, 37 à 41 cycles) n'est pas comptée dans D : dans une feuille, les coordonnées locales sont déjà en cache.

**Triplets** : séquence du HEAD (droite, aigu, centre, boîte) 163 à 198 cycles ; filtre `double` par blocs 134 (base) et 81 (`x86-64-v3`) : **×1,5 à ×2,0**. 0 désaccord sur 28 587 237 ; replis exacts 0,10 %. Le test de droite laisse passer 71 % des candidats : il y a peu à écarter tôt.

**Recensement** : `geom::side` en `__int128` avec sortie anticipée, 277 à 360 cycles par juge ; marge en `double` sur tous les sites puis repli exact, 296 à 333 : **aucun gain**. Le recensement exact est déjà court (quatre produits 128 × 64 bits par site, 30 à 34 cycles) ; son levier est ailleurs (ne pas tester les sites que les masques décident : 55 à 62 % d'après le profil privé, non rejoué).

**Feuille J3 privée** (`build/v10-perf/feuille/timing_v3d/src`, jamais intégrée) : reconstruite et rejouée par l'auditeur. Dump et grand livre identiques à HEAD sur le quart 01 ; dumps identiques sur 8 entrées extrêmes ou dégénérées (domaine u18 entier, grille $16^{3}$, sphère de 144 points) ; 30 contrôles conformes à l'oracle indépendant (180 346 boules, `oracle/variante_j3_contre_oracle.txt`) ; **−19,3 % d'instructions** (6,94 → 5,60 G) et **−40 % de mauvaises prédictions** (41,5 → 24,7 M) sur tout le catalogue, soit environ un tiers d'instructions en moins dans la feuille (≈ 4,2 → 2,8 G) ; le filtre des nœuds, inchangé (2,4 G), devient alors le premier poste avec 43 % des instructions ; étage des boîtes ×1,33 en passes alternées locales à un fil (trame 02, K = 5).

### 6.8 Ce qui empêche la vectorisation et le passage sur GPU (lu)

1. **Contrôle dépendant des données à chaque candidat** : quatre boucles imbriquées d'itération de bits à longueur variable, quatre à cinq tests à sortie anticipée par candidat, un appel de `judge` au cœur de la boucle la plus interne.
2. **Arithmétique 128 bits partout dans la feuille** : centres, test de boîte, lemme Z, orientations, recensement. Aucun jeu d'instructions vectoriel n'a de produit 64 × 64 → 128.
3. **Coordonnées en tableaux de structures** (`P3` de trois `i64`, 24 octets) et globales : la feuille recopie ses sites (l. 329-335) mais garde des valeurs absolues sur 24 bits au lieu de coordonnées locales courtes.
4. **Sorties de longueur variable par `std::vector::push_back`** dans le juge (intérieur, coquille, enregistrement, identifiants), mémo à recherche linéaire, `canonical_support` à boucles imbriquées.
5. **Récursion** avec une liste allouée par nœud, et un arbre dont la forme n'est connue qu'en le parcourant ; boîtes explicites non dyadiques depuis J2c (pas d'indexation implicite d'octree).
6. **Trois gabarits de masques** plus un chemin à nombre de mots variable : sur LiDAR, toutes les feuilles ont $m\le24$ (histogramme : 5 à 16 à K = 5, 10 à 24 à K = 10) ; un masque de 32 bits suffirait au chemin rapide.

Deux faits favorables, mesurés (`feuilles_histogrammes_et_duplication.txt`) : l'étendue d'une liste de feuille ne dépasse jamais $2^{15}$ mm sur la trame 02 (92,6 % sous $2^{12}$ mm, 40,2 % sous $2^{9}$ mm à K = 5), et le plus long côté d'une boîte de feuille reste sous $2^{18}$ unités de $2^{-6}$ mm. Des coordonnées locales à la feuille tiennent donc sur 15 bits : le recensement q3 ($144E^{6}$) tient en i64 pour 40 % des feuilles et laisse 30 bits de marge en i128 pour toutes.

### 6.9 Ce que les leviers connus donnent, et ce qu'ils ne donnent pas (estimation)

Estimation de l'auditeur, **pas une mesure de bout en bout**. Elle applique aux parts du § 6.2 les rapports mesurés ou rejoués : quadruplets ÷8 (filtre flottant, § 6.7) ou ÷2,5 (exact seul) ; triplets ÷1,4, dominance ÷1,3, paires ÷1,2 (répartition du gain rejoué de J3, qui donne ×1,29 au total par ce modèle contre ×1,33 mesuré) ; noyau du filtre ÷2 (`x86-64-v3`, mesure locale bruitée : 3,0–3,9 → 1,3–1,9 G cycles) ; recensement ÷2 et émission ÷2 (non rejoués).

| Poste (G4, 48 fils, trame 02) | K = 5 mesuré (ms) | K = 5 estimé (ms) | K = 10 mesuré (ms) | K = 10 estimé (ms) | Fondement |
|---|---:|---:|---:|---:|---|
| frontière | 27 | ≈ 6 | 40 | ≈ 11 | les nœuds de frontière portent 25 % (K = 5) et 17 % (K = 10) des visites du filtre, soit ≈ 6 % et ≈ 2,4 % du travail des boîtes, s'ils étaient aussi parallèles que le reste |
| boîtes | 103 | 57 à 65 | 442 | 219 à 257 | parts × rapports ci-dessus, recoupées par le modèle à compteurs du § 6.10 : ×1,6 à ×1,8 (K = 5), ×1,7 à ×2,0 (K = 10) |
| ordre et assemblage | 33 | ≈ 16 à 19 | 124 | ≈ 62 à 73 | clés compactes, tri par base, une seule lecture ; ×1,7 annoncé par le plan privé à 8 fils, non rejoué |
| hors étages | 9 | ≈ 3 | 23 | ≈ 6 | tampons réutilisés d'une trame à l'autre |
| **catalogue** | **171** | **≈ 82 à 93** | **628** | **≈ 298 à 347** | |

Lecture : les leviers **fondés** donnent environ ×1,9 en CPU seul. Le catalogue à K = 5 occuperait alors presque tout le budget de 100 ms avant même la tour (67 à 89 ms pour la tour FULL à K = 5 d'après le même reçu). Le contrat de 100 ms à K = 5 demande donc un facteur 2 à 3 supplémentaire sur la feuille, et K = 10 un facteur 6 : voir les pistes non mesurées du § 11.

### 6.10 Ablations de paramètres rejouées (compteurs déterministes)

**Taille de feuille $M$** (`--leaf`, trame 02, `ablation_M.txt`). Le « coût modèle » est la somme des compteurs pondérés par les coûts unitaires du § 6.2 ; au $M$ par défaut il redonne le coût mesuré à 4 % près (9 783 contre 10 219 cycles par boule à K = 5 ; 11 388 contre 11 532 à K = 10).

| K | M | Nœuds | Tests du filtre par boule | Candidats par boule (dont triplets / quadruplets) | Coût modèle (cycles par boule) |
|---:|---:|---:|---:|---|---:|
| 5 | 8 | 34 377 727 | 2 451 | 289,7 (26,6 / 2,1) | 35 267 |
| 5 | 12 | 2 204 985 | 479 | 60,6 (19,5 / 3,9) | 10 747 |
| 5 | **16** | 734 083 | 287 | 48,1 (20,3 / 6,7) | **9 783** |
| 5 | 20 | 397 389 | 226 | 49,6 (22,4 / 10,2) | 10 823 |
| 5 | 32 | 150 445 | 165 | 72,0 (31,4 / 26,0) | 17 443 |
| 10 | 14 | 41 397 033 | 2 120 | 336,7 (40,1 / 6,3) | 34 905 |
| 10 | 20 | 2 572 841 | 365 | 70,4 (25,7 / 10,3) | 11 961 |
| 10 | **24** | 1 065 585 | 235 | 57,9 (24,3 / 13,7) | **11 388** |
| 10 | 28 | 575 647 | 176 | 56,3 (24,4 / 17,8) | 12 105 |
| 10 | 40 | 203 377 | 112 | 70,6 (27,8 / 33,6) | 17 113 |

- La table $M(K)$ du code est bien au minimum du modèle (16 à K = 5, 24 à K = 10).
- **Les candidats par boule ont un plancher** : 48 à K = 5, 56 à K = 10. Les triplets par boule varient peu avec $M$ dans la plage utile (19,5 à 25) et ne descendent jamais sous 19,5 à K = 5 ni sous 24,3 à K = 10 : une feuille plus petite reteste les mêmes triplets dans plus de feuilles. Aucun réglage de $M$ ne réduit ce travail ; seule une autre énumération le pourrait (§ 11, Q7).
- Près de $M=K+3$ l'arbre explose : 34,4 M nœuds à K = 5 pour $M=8$ (×47), 41,4 M à K = 10 pour $M=14$ (×39).
- Avec les coûts unitaires des leviers fondés (filtre ÷2, dominance ÷1,3, paires ÷1,2, triplets ÷1,4, quadruplets ÷8, juge ÷1,6), le minimum reste à $M=16$ et $M=24$–28 et le gain de l'étage vaut ×1,82 (K = 5) et ×2,02 (K = 10) : recalibrer $M$ n'ajoute rien.

**Réservoir de dominateurs** ($R\cdot K$ sites, $R=3$ dans le code ; `ablation_reservoir.txt`). Dumps **identiques** pour $R=1,2,3,4,6$ : la proposition L (la liste reste certifiée quel que soit Y) est vérifiée en acte sur la trame entière. $R=1$ multiplie les nœuds par 5 (K = 5) et 6,7 (K = 10). Entre $R=2$ et $R=4$ le coût modèle varie de −4 % à +5 % : optimum plat, $R=3$ est un choix raisonnable.

**Bits sous-unitaires $T$** : voir § 7 (dumps identiques à T = 6, 3 et 0).

---

## 7. Dette

**Code mort ou sans effet (lu).**

- `generator.cpp:727-730` : la boucle « frontière épuisée avant la cible » ne s'exécute jamais ; la boucle `while` qui précède ne sort que frontière vide (condition de boucle, ou `break` de la l. 715).
- `Ctx::cloud` (l. 66) n'est jamais lu ; `<mutex>` et `<cmath>` sont inclus sans usage.
- `core/types.hpp:21-25` déclare `SiteIdx`, `BallIdx`, `LevelRank`, `NodeIdx` « pour qu'un rang et un identifiant n'aient jamais le même type » ; aucun n'est utilisé dans `src/` : sites, boules et rangs sont des `u32` nus.
- Cinq raisons de `reasons.def` ne sont émises nulle part (`arith_guard`, `window_empty`, `nested_parallelism`, `leaf_unsplittable`, `device_unavailable`) ; une boule émise deux fois sort sous `census_mismatch`, nom trompeur (l. 836).
- Le repli `sup = gen` de `canonical_support` (l. 183, « ne devrait pas arriver ») masquerait une faute au lieu de rendre `invariant_violated`.
- Le test de poids des quadruplets (l. 451-453) ne peut rien rejeter pour $K\le5$ (§ 5.1).

**Duplication.**

- Deux `canonical_support` : `generator.cpp:147-185` (i128, court-circuité par l'arité du générateur) et `support.hpp:56-97` (arithmétique large, utilisé par la tour). Même objet, deux prédicats.
- `process` porte deux modes (frontière avec copie de liste, récursion) ; la lecture u32le est recopiée dans trois CLI avec le même défaut de troncature.
- `emitted_level` (l. 187-221) n'existe que pour reproduire la **représentation** (numérateur, dénominateur) que publiait l'ordre d'énumération de la feuille v1 : 26 lignes et une boucle en $\lvert U\rvert^{4}$ sur les coquilles étendues pour garder un dump identique, alors que le niveau calculé depuis $S^{*}$ est canonique.

**Leviers sans ablation dans le dépôt.**

- `kT = 6` : ablation faite pour cet audit (`ablation_kT.txt`). À T = 3 et T = 0, les dumps de la trame 02 (K = 5 et 10) et du nuage uniforme sont **identiques** et le travail change de moins de 0,3 %. T ne sert qu'aux réseaux entiers (grille $16^{3}$ : ×3 à ×4 de temps et listes de 48 à 56 sites à T = 0). Il coûte 6 bits sur la borne i64 du filtre ($B+T\le29$).
- `kStagnationLimit = 9` : jamais atteint sur LiDAR ni sur synthétique générique ; l'équivalence annoncée avec trois niveaux d'octree est fausse pour un pavé anisotrope (déjà relevé par l'audit continu, P3).
- Réservoir de $3K$ dominateurs : ablation faite pour cet audit (§ 6.10), optimum plat entre $2K$ et $4K$.
- Cible de $64P$ tâches, seuil de charge $n/(64P)$, seuil de bande $2^{-40}$, grains 64 / 16 / 4096, `max_leaf = 256` : valeurs posées, sans mesure d'alternative dans les reçus.
- Table $M(K)$ : calibrée en local à un fil (reçu J2c), confirmée ici par les compteurs (§ 6.10) ; le calibrage G4 prévu (P-CAL) n'a pas eu lieu.

**Structure.**

- `build_catalogue` : 241 lignes, dix étapes (préparation, racine, frontière, tâches, collecte, tri, bandes, comparaisons, rangs, copie). `enumerate_leaf_masks` : 146 lignes, quatre niveaux de boucles, instanciée quatre fois.
- Tout le générateur vit dans l'espace de noms anonyme d'un fichier : ni `center_line_meets`, ni `filter_node`, ni le juge ne peuvent être testés seuls ; `tests/unit/unit_main.cpp` n'en teste aucun.
- Chronomètres et compteurs de tâches sont des champs de la structure de données publiée (`Catalogue::t_*`, `tasks`, `bands`).
- Travail utile resté hors dépôt : feuille J3 (`build/v10-perf/feuille/`), frontière par vol de tâches (`build/v10-perf/frontiere/`), gardes de la série R2 (`build/v10-integration-r2/` : budget de nœuds, règle $M\ge K+3$, refus du dépassement u32, `bad_alloc` rendu en refus). Rien de cela n'est reproductible depuis `afb081774`.

---

## 8. Constats numérotés

Gravité : **bloquant** = rend faux ou invalide un résultat ou un contrat ; **majeur** = à traiter dans la conception v11 ; **mineur** ; **info**.

| Id | Gravité | Fait | Preuve | Vérification | Conséquence pour la v11 |
|---|---|---|---|---|---|
| L05_CODE_CATALOGUE-01 | info | Aucun défaut d'exactitude trouvé dans le générateur à `afb081774`. Chaque élagage a son lemme et traite ses égalités du côté sûr. | § 5.1 à § 5.4 ; `oracle/campagne_head.txt` (43 contrôles, 385 226 boules, K ≤ 12, poids, feuilles larges) ; `invariants/sorties_invariants.txt` (restriction, Euler à 8 000 points) ; `bords/sorties_extremes_u18.txt` ; dumps égaux aux reçus et de 1 à 48 fils | exécuté | L'algorithme et ses lemmes se portent. Les contre-épreuves de cet audit deviennent des portes. |
| L05_CODE_CATALOGUE-02 | bloquant (contrat 1) | Le catalogue seul prend 171 ms à K = 5 et 628 ms à K = 10 sur G4 à 48 fils ; le travail consommé (6,04 s et 25,6 s de CPU) borne le mur à 126 ms et 534 ms même à parallélisme parfait. | `receipts/g4_session4_j2c_20260929/results/cmd/005_*` et `007_*` (`stdout`, `time.txt`) | lu | Porter le générateur tel quel invalide le contrat de 100 ms. Pour tenir en 50 ms et laisser l'autre moitié du budget à la tour, il faut réduire le travail d'un facteur ≈ 3,5 (K = 5) et ≈ 12 (K = 10) ; les leviers fondés donnent ≈ ×1,9 (§ 6.9). |
| L05_CODE_CATALOGUE-03 | majeur | La seule porte permanente du générateur (40 nuages de 5 à 22 points, K ≤ 5, poids 1, 2 fils) laisse vivre trois mutants non équivalents : masques à plusieurs mots, frontière pilotée par la charge (2 134 à 9 523 boules perdues sur un quart de trame, `status ok`, selon le nombre de fils), réparation des bandes. | `tests/oracle/test_catalogue_oracle.py:114-140` ; `oracle/porte_du_depot_sur_mutants.txt` ; `mutants.diff` ; `mesures_diverses.txt` § 2 et § 3 | exécuté | Portes permanentes dès le premier commit : oracle avec K = 10 et 12, poids, feuilles de plus de 64, 128 et 256 sites, minimalité de $S^{*}$, niveaux ; invariance aux fils ; Euler et restriction à 8 000, 16 000 et 32 000 points ; mutants à code 4. |
| L05_CODE_CATALOGUE-04 | majeur | La frontière en largeur ne passe pas à l'échelle : ×2,1 de 1 à 48 fils, 15,5 % du mur à K = 5. À 48 fils elle traite 3,1 % des nœuds mais 25,4 % des visites du filtre, en 29 tours à barrière ; la racine (45 845 sites) est filtrée par un seul fil. | reçu G4 (ci-dessus) ; `frontiere_parts.txt` ; `generator.cpp:682-726` | lu, mesuré | Ordonnancement récursif par vol de tâches ; filtre des grosses listes découpé sur les sites ; plus de phase en largeur. Gain estimé : 27 → ≈ 6 ms à K = 5. |
| L05_CODE_CATALOGUE-05 | majeur | Ordre et assemblage font 19 à 20 % du mur à 48 fils ; l'étage des rangs ne gagne que ×8 (écritures partagées par boule, lectures au hasard d'enregistrements de 104 octets), le repérage des bandes est en série, et la destruction des tampons coûte 3,6 à 5,1 % de plus. | reçu G4 ; `generator.cpp:782-790`, `827-834` ; `mesures_diverses.txt` § 4 (−12 % en médiane avec des accumulateurs locaux) | lu, mesuré | Émettre des enregistrements compacts dans leur forme finale ; clé de tri de 12 à 16 octets ; aucune passe qui relit les enregistrements au hasard ; tampons réutilisés entre trames. |
| L05_CODE_CATALOGUE-06 | majeur | Mémoire : 104 à 125 octets par boule résidents et 270 à 340 au pic, contre 30 à 47 conçus ; `Level` de 56 octets par rang, `support[4]` redondant pour 99,96 % des boules ; aucun tableau du catalogue ne passe par le budget mémoire. | `catalogue.hpp:32-91` ; `invariants/invariance_fils_et_tailles.txt` ; § 6.5 ; `time.txt` du reçu G4 | lu, mesuré | Format compact décidé avant le code (niveau recalculé depuis $S^{*}$, compteurs sur u8, table à part des coquilles étendues) ; budget mémoire réel avec refus. |
| L05_CODE_CATALOGUE-07 | majeur | La feuille fait 77 à 86 % de l'étage des boîtes et reste entièrement scalaire : 48 à 58 candidats testés par boule émise, 135 à 270 cycles par triplet ou quadruplet, 10,6 % de branchements mal prédits ; le test le plus sélectif des quadruplets (7,4 % de oui) est fait en dernier. | `profil_rdtsc_etages.txt` ; `callgrind_resume.txt` ; § 6.3, § 6.4 | mesuré | Feuille par étages sur lots (dominance, paires, triplets, quadruplets, recensement) avec compaction, coordonnées locales, tests les plus sélectifs d'abord. |
| L05_CODE_CATALOGUE-08 | majeur | Les sessions G4 mesurent un build sans `-march` : le noyau du filtre des nœuds n'y est pas vectorisé (5,2 à 5,5 cycles par test mesurés localement dans ce build, contre 1,5 conçu) et le `popcount` est logiciel. | `receipts/g4_session4_j2c_20260929/results/provenance/cmakecache.txt` ; désassemblage de `filter_node` (`mesures_diverses.txt` § 7) | lu, mesuré | Fixer un jeu d'instructions de référence pour G4 (l'hôte a AVX-512, dont `avx512dq` et `avx512ifma` : `receipts/g4_session5_scale_20260929/vm_facts.txt`) avec une porte d'identité des dumps entre jeux ; ne plus dépendre de l'auto-vectorisation. |
| L05_CODE_CATALOGUE-09 | info | Leviers chiffrés : feuille J3 privée égale à HEAD, −19,3 % d'instructions, −40 % de mauvaises prédictions, ×1,33 ; quadruplets ×2,4 à ×2,7 en exact réordonné, ×8 à ×11 avec filtre flottant et repli exact (0 désaccord sur 18,8 M candidats) ; triplets ×1,5 à ×2,0 ; recensement : aucun gain. | `callgrind_resume.txt` ; `microbancs/sorties_microbancs.txt` ; `mesures_diverses.txt` § 8 | exécuté, mesuré | Partir de la forme J3 ; l'essentiel du gain des quadruplets est exact (intérieur avant centre) ; le flottant n'ajoute que ×1,06 à ×1,11 sur l'étage et se décide sur preuve écrite ; ne rien attendre d'un recensement flottant. |
| L05_CODE_CATALOGUE-10 | majeur | La « conception finale » GEN_v2 et la spécification § 3 ne décrivent plus le code : octree contre arbre binaire ajusté, stagnation, admission pondérée, mémo, format, digest, `LeafOracle`, portes et fixtures prévues absentes. | § 4 (18 écarts, lignes citées) | lu | Écrire la conception v11 du générateur à partir du code réel et de cette table, pas de GEN_v2 ; tenir le document à jour dans le même commit que le code. |
| L05_CODE_CATALOGUE-11 | mineur | Un amas cosphérique à intérieur vide coûte $m^{4}$ : 144 points, 20 s pour 2 169 boules ; la fixture F11 devait être refusée ($M_{\mathrm{hard}}=128$), elle est acceptée (`max_leaf = 256`) ; le mémo par masque et le raccourci q4 du lemme U ne sont pas codés. | `bords/sorties_spheres_cout.txt` ; `generator.cpp:253-255`, `catalogue.hpp:98` | exécuté | Mémo par masque de coquille avant le calcul du centre ; budget de travail par feuille avec refus. Utile pour les bancs synthétiques sur grille. |
| L05_CODE_CATALOGUE-12 | mineur | Le contrôle final ne vérifie pas l'ordre des $S^{*}$ à niveau exact égal : sans réparation des bandes, la trame 02 sort avec une ligne déplacée et `status ok`. La réparation n'est exercée que par des trames entières. | `generator.cpp:830` ; `mesures_diverses.txt` § 3 | exécuté | Contrôler l'ordre total (niveau, $S^{*}$) ; graver une fixture de niveaux exacts égaux à approximations différentes. |
| L05_CODE_CATALOGUE-13 | mineur | Aucune borne sur la descente : pas de budget de nœuds, pas de garde $M\ge K+3$, compteurs de boules en u32 sans refus. `--leaf=6` à K = 5 sur 8 points dépasse 15 s ; un filtre fautif ne termine pas. | `bords/sorties_bords_cli.txt` ; `generator.cpp:599`, `767`, `803` | exécuté, lu | Budget de nœuds et de boules avec refus transactionnel ; règle de taille de feuille (gardes déjà écrites dans la série R2 privée). |
| L05_CODE_CATALOGUE-14 | mineur | La CLI accepte un fichier tronqué (`status ok`), finit par signal sur une option illisible et accepte `--k=2x`. | `cli/mhgp10_catalogue.cpp:26-42` ; `bords/sorties_bords_cli.txt` | exécuté | Lecteur strict (taille multiple de 12, refus à code 2), analyse d'options sans exception. |
| L05_CODE_CATALOGUE-15 | majeur | Le domaine u18 et $T=6$ sont câblés dans l'arithmétique ; les bornes ne sont gardées que par le refus d'entrée. Or $T$ est neutre sur LiDAR (dumps identiques à T = 0, +0,2 % de nœuds) et les listes de feuille ont une étendue ≤ 15 bits. | `generator.cpp:22`, `33`, `636` ; `ablation_kT.txt` ; `feuilles_histogrammes_et_duplication.txt` | mesuré | Repère local à la feuille et arithmétique par paliers ; $T$ paramètre, nul par défaut ; chaque borne en `static_assert`. C'est aussi la voie vers une grille plus large que 18 bits. |
| L05_CODE_CATALOGUE-16 | mineur | Le chemin pondéré du générateur applique une clause d'admission que GEN_v2 dit supprimée, n'est couvert par aucune porte du dépôt et ne sert à rien en aval : la tour refuse toute entrée pondérée. | `generator.cpp:262` ; `GEN_v2.md` § 2.3 ; `tests/regression/test_multiplicity_refusal.py` | lu, exécuté (conforme à l'oracle de l'auditeur avec la règle du code) | Décider : ou les multiplicités sont au contrat v11 (règle unique, oracle pondéré, tour), ou le générateur les refuse aussi. À 1 mm, 0 doublon sur les cinq trames examinées. |
| L05_CODE_CATALOGUE-17 | mineur | Dette : boucle morte (l. 727-730), identifiants forts jamais utilisés, cinq raisons jamais émises, deux `canonical_support`, `emitted_level` de compatibilité, fonction de 241 lignes, tout en espace de noms anonyme, leviers utiles restés hors dépôt. | § 7 | lu | Modules courts et testables ; pas de compatibilité de représentation ; tout levier intégré ou abandonné, jamais « privé ». |
| L05_CODE_CATALOGUE-18 | info | Un triplet est testé dans 3,8 feuilles en moyenne (jusqu'à 69), un quadruplet dans 1,36 ; 2,1 % des candidats deviennent une boule. | `feuilles_histogrammes_et_duplication.txt` ; § 6.3 | mesuré | Le nombre de candidats par boule est le vrai levier algorithmique restant (§ 11, Q7). |

---

## 9. Ce qui est solide et mérite un port explicite en v11

1. **Le principe des boîtes de centres à listes certifiées** et ses lemmes : dominance (D, D-loc), listes par induction (L), recensement local exact (C), ajustement à l'enveloppe (K, J2c § 3.1), union des dominés (M), survie du support canonique (S), droite des centres (Z), unicité par $S^{*}$. La table du § 5.1 sert de liste de contrôle : une décision, un lemme, un côté pour l'égalité, un mutant.
2. **Les prédicats entiers exacts** de `arith/geometry.*` et `arith/wide.hpp` et leurs bornes u18 (recalculées au § 5.2, conformes aux extrêmes du domaine).
3. **La boîte ajustée et la coupe binaire** (J2c), avec la borne haute `env.hi + 1` : 33 à 40 % de nœuds et 54 % de feuilles en moins, et une dégénérescence cosphérique qui reste locale (8 feuilles bloquées, pas une ligne de feuilles).
4. **Le filtre des nœuds en un seul prédicat** (garde absorbée), en forme locale à la boîte, sans branchement, avec un réservoir dont le choix ne touche que le coût.
5. **La feuille à masques de dominance** avec ses seuils $\theta_{q}$ et ses paires et triplets vivants — à porter sous la forme J3, vérifiée égale octet pour octet.
6. **L'ordre canonique** (niveau exact, $S^{*}$) par clé flottante, bandes et tri exact : sortie identique de 1 à 48 fils (rejoué).
7. **Le refus transactionnel** : aucune sortie partielle ; un refus de feuille large n'interrompt pas les autres fils et sort à la fin.
8. **La préparation** : sites = positions distinctes en ordre de Morton, poids, table site → `PointId`.
9. **La discipline de reçu par lot** : différentiel octet pour octet contre le binaire précédent, build empoisonné, ThreadSanitizer, mutants. Elle a tenu six refontes en 29 heures sans changer un octet du catalogue (dumps des reçus retrouvés à l'identique par l'auditeur).
10. **Les épingles** : trame 02 (1 407 885 et 5 483 320 boules, comptes par $q_{\min}$, 572 et 1 301 coquilles étendues) ; trame brute avec sol (2 822 052 boules à K = 5, 11 387 391 à K = 10) ; dumps du quart 01 (`414aa4d47ffe1c55…`, `7c46e50a72c08087…`) et de la trame 02 (`8a850649ff103c1c…`, `d6abe0dba4d9…`) ; uniforme 8 000 (594 381 boules à K = 5, 4 947 753 à K = 12).
11. **Les contre-épreuves de cet audit**, à transformer en portes : oracle brut indépendant avec feuilles forcées, invariant de restriction, identité d'Euler à multiplicités binomiales, invariance aux fils, extrêmes du domaine.
12. **Les gardes de la série R2 privée** : budget de nœuds, règle $M\ge K+3$, refus du dépassement u32, allocation échouée rendue en refus.

## 10. Ce qu'il ne faut pas refaire

1. Un fichier de 876 lignes dont toutes les fonctions sont dans un espace de noms anonyme : rien n'est testable seul, les égalités des lemmes ne sont atteintes que par des nuages fabriqués.
2. Valider une refonte par le seul différentiel contre la version précédente, adossé à une porte oracle de 22 points : les chemins nouveaux (masques larges, frontière, bandes) ne sont jamais jugés contre une vérité.
3. Une frontière en largeur à barrières, avec une cible ($64P$ tâches, seuil $n/(64P)$) qui, à 48 fils, descend jusqu'à la taille d'une feuille.
4. Des enregistrements transitoires de 104 octets relus au hasard par trois passes, un niveau exact de 56 octets stocké par rang, des tableaux hors budget.
5. Une fonction de compatibilité (`emitted_level`) pour préserver la représentation d'un niveau entre deux versions : la représentation doit être canonique par construction.
6. Garder dans le produit un chemin (poids) que l'étage suivant refuse et qu'aucune porte ne couvre.
7. Laisser une « conception finale » et une « spécification telle qu'implémentée » diverger du code dès le lendemain.
8. Mesurer sur G4 un build sans `-march` alors que les noyaux ont été conçus et chiffrés avec ; mesurer le catalogue par un tir unique en processus neuf.
9. Câbler la largeur des coordonnées et les bits sous-unitaires dans les bornes entières du chemin chaud.
10. Tester en dernier le prédicat le plus sélectif (quadruplets) ; calculer un centre en 128 bits avant de savoir s'il sert.
11. Des options de coût sans garde (`--leaf`) et une descente sans budget.
12. Développer des leviers mesurés et vérifiés hors dépôt sans les intégrer ni les abandonner par écrit.

---

## 11. Questions ouvertes

- **Q1 — Où trouver le facteur manquant ?** Après les leviers fondés (≈ ×1,9), il reste ≈ ×2 à ×3 à K = 5 et ≈ ×6 à K = 10 pour que le catalogue tienne dans la moitié de 100 ms. Trois pistes, **aucune mesurée** : (a) lots AVX-512 sur l'hôte G4 (produits 64 bits vectoriels, `avx512ifma`, masques et compression natifs) ; (b) feuille sur GPU — le plan privé estime son noyau entre 17 et 52 ms à K = 5 pour une efficacité SIMT de 13 %, non vérifié ; (c) moins de candidats par boule (Q7).
- **Q2 — Le filtre flottant à repli exact est-il admis ?** GEN_v2 le laissait ouvert (O-F1) sous condition d'un gain ≥ ×1,5 sur la feuille. Mesuré ici : ×8 à ×11 sur les quadruplets seuls, soit ≈ ×1,18 (K = 5) à ≈ ×1,35 (K = 10) sur l'étage des boîtes — mais le réordonnancement **exact** en donne déjà ×1,12 et ×1,22 : le gain propre du flottant n'est que ×1,06 à ×1,11 sur l'étage, peu sur les triplets, rien sur le recensement. Sur ces seules mesures, le critère de GEN_v2 n'est pas atteint. S'il est quand même retenu, il faut une preuve écrite de la borne, une porte de repli non vide et un comportement défini hors arrondi au plus proche.
- **Q3 — La tour a-t-elle besoin d'un catalogue entier, résident et trié globalement ?** C'est 20 % du mur à 48 fils et tout le mur mémoire. Un tri par ordre $k$, ou par tranches de niveaux consommées au fil de l'eau, changerait le contrat entre les deux étages.
- **Q4 — Multiplicités.** Règle d'admission unique (lemme W) ou clause du code ? Et sont-elles au contrat v11, alors que la tour les refuse et que les cinq trames examinées n'ont aucun doublon à 1 mm ?
- **Q5 — Au-delà de 18 bits.** Le repère local de feuille (≤ 15 bits mesurés) rend les prédicats indépendants de la largeur globale ; reste le filtre des nœuds ($B+T\le29$ en i64) et la clé de Morton. Faut-il encore des bits sous-unitaires, alors que T = 0 donne le même catalogue sur LiDAR ?
- **Q6 — Pire cas.** Aucune borne sur le nombre de nœuds ; la stagnation n'est qu'une heuristique jamais atteinte sur données réelles. Quel budget, quel refus, quelle famille adverse gravée ?
- **Q7 — Candidats par boule.** 48 à 58 sur LiDAR ; un triplet est retesté dans 3,8 feuilles ; il faut 48 quadruplets distincts pour une boule q4 émise. La taille de feuille n'y change rien (plancher de 48 et 56 candidats par boule, § 6.10). Existe-t-il une énumération de feuille complète qui en examine moins (partage des triplets entre feuilles voisines, parcours des droites de centres plutôt que des boîtes) ? Toute réponse doit garder le théorème C.
- **Q8 — Juge de complétude permanent sur une trame LiDAR à coquilles étendues.** L'identité d'Euler simple ne s'y applique pas (572 coquilles étendues à K = 5) ; la restriction ne compare que le générateur à lui-même ; les épingles viennent de la v9. La lentille L01 a écrit pour cet audit un juge d'Euler à coquilles étendues ; rien de tel n'est dans le dépôt, pas plus que le juge des boîtes par échantillon de GEN_v2 § 12.4. Lequel devient la porte d'échelle de la v11 ?
- **Q9 — Bancs synthétiques sur grille.** Une grille entière $16^{3}$ coûte 1,3 à 1,8 fois plus de CPU par boule que le LiDAR à T = 6 et 3 à 4 fois plus à T = 0 (mesures locales bruitées), et un amas cosphérique coûte $m^{4}$ : le contrat 3 (comparaison à HDBSCAN sur données synthétiques) doit dire si ces entrées sont servies, refusées ou perturbées en amont.
- **Q10 — Trame avec sol.** 2,82 M boules à K = 5 et 11,39 M à K = 10 pour 123 389 points (3,2 Go de pic à K = 10) : le double de la trame sans sol, donc le double du budget de temps et de mémoire.

---

## 12. Recommandations pour le générateur v11

### 12.1 Garder

| Quoi | Pourquoi |
|---|---|
| Boîtes de centres, listes certifiées, feuille à masques, lemmes du § 5.1 | exact, sensible à la sortie (coût par boule stable de 8 000 à 32 000 points), vérifié ici par quatre voies indépendantes |
| Boîte ajustée, coupe binaire, garde absorbée | −33 à −40 % de nœuds, un seul prédicat d'arbre |
| Ordre canonique par clé flottante et réparation exacte | sortie indépendante du nombre de fils |
| Refus transactionnel, sites pondérés en ordre de Morton | contrats simples, déjà éprouvés |
| Épingles et dumps du § 9, comme tests de non-régression de la v11 | l'objet ne change pas |

### 12.2 Changer

| Quoi | Gain attendu | Fondement |
|---|---|---|
| **Portes d'abord** : oracle brut indépendant avec K = 10 et 12, poids, feuilles forcées au-delà de 64, 128 et 256 sites ; invariance aux fils ; restriction et Euler à 8 000, 16 000 et 32 000 points ; dix mutants à code 4 ; fixtures d'égalité (centre sur un plan de coupe, sur la face haute de l'enveloppe, droite tangente, sites équidistants, niveaux égaux à clés différentes) | aucun gain de temps ; ferme trois mutants survivants | § 5.5, exécuté |
| **Feuille par étages sur lots** : sites de la feuille en tableaux séparés et coordonnées locales ; dominance, paires, triplets, quadruplets et recensement en passes sans branchement séparées par une compaction ; intérieur du tétraèdre avant le centre ; masque de 32 bits pour $m\le32$ et chemin générique testé au-delà | boîtes ×1,6 à ×2,0 en exact et avec filtre sur les quadruplets ; davantage en AVX-512, non mesuré | J3 rejouée (−19 % d'instructions, −40 % de mauvaises prédictions) ; microbancs § 6.7 ; histogrammes § 6.8 |
| **Arithmétique par paliers** dans le repère local : i64 quand l'étendue de la liste le permet (40 % des feuilles à K = 5), i128 sinon, large au-delà ; chaque borne en `static_assert` ; $T$ paramètre | prérequis de la vectorisation et de toute grille au-delà de 18 bits | § 5.2, § 6.8, `ablation_kT.txt` |
| **Ordonnancement** par vol de tâches récursif ; filtre des listes de plus de quelques milliers de sites découpé sur les sites ; arène par fil | frontière 27 → ≈ 6 ms (K = 5), 40 → ≈ 11 ms (K = 10) | § 6.6 |
| **Émission compacte et assemblage en une passe** : ($S^{*}$, compteurs, identifiants) écrits une fois ; niveau recalculé depuis $S^{*}$ ; clé (approximation, indice) de 12 à 16 octets triée par base ; bandes repérées en parallèle ; contrôle de l'ordre total | ordre et assemblage ÷2 ; mémoire ÷3 (≈ 30 à 47 o par boule résidents) ; hors étages ≈ 0 avec des tampons réutilisés | § 6.5, § 6.6 ; format de GEN_v2 § 8 |
| **Jeu d'instructions de référence** sur G4 avec porte d'identité des dumps ; noyaux écrits explicitement plutôt qu'auto-vectorisés | filtre des nœuds ÷2 à ÷3 | § 6.4 |
| **Budgets et refus** : nœuds, boules (u32), mémoire, travail par feuille ; règle $M\ge K+3$ ; mémo des coquilles étendues par masque avant le calcul du centre | pas de temps ; supprime les explosions du § 5.3 | gardes R2 privées ; lemme U de GEN_v2 |
| **Mesure** : passes chaudes répétées dans un processus résident, temps CPU par fil, compteurs de sous-étapes dans le grand livre (candidats par étape, replis exacts) | rend les gains attribuables | § 6.3 ; reçus G4 à tir unique |
| **Module** : un fichier par étage (boîte, filtre, feuille, juge, ordre), chacun testable, prédicats hors de l'espace de noms anonyme ; conception tenue à jour dans le même commit | — | § 7 |

### 12.3 Supprimer

- La phase en largeur et sa boucle morte (l. 683-744).
- `emitted_level` et toute compatibilité de représentation ; le second `canonical_support`.
- Les trois gabarits de masques larges comme chemin produit (les garder, testés, comme chemin lent des feuilles dégénérées).
- Le chemin pondéré tant que la tour refuse les poids — ou le garder avec sa règle décidée et son oracle (Q4).
- La stagnation telle quelle, remplacée par un budget explicite (Q6).
- Les champs de chronométrage dans `Catalogue`, le `popcount` logiciel, les identifiants forts déclarés mais inutilisés (ou les utiliser vraiment), les raisons jamais émises.

### 12.4 Ordre de travail conseillé

1. Écrire la conception du générateur v11 depuis le code réel (§ 3) et la table des écarts (§ 4), avec le format de sortie et le contrat avec la tour (Q3) tranchés.
2. Poser les portes du § 12.2 sur un port minimal et exact (sans optimisation), épingles comprises.
3. Structurer la feuille par étages et le repère local ; rejouer les microbancs sur le code réel, étage par étage, avant chaque levier.
4. Ordonnancement et assemblage.
5. Session G4 de calibrage ($M(K)$, jeu d'instructions, 1, 24 et 48 fils, passes chaudes), puis seulement la décision GPU.

---

## Annexe A — Preuves déposées

Dossier `/workspaces/E-HGP/build/v11-persist/audit_v10/preuves_l05_code_catalogue/` (40 fichiers dont `SHA256SUMS`, environ 260 Ko) :

| Fichier | Contenu |
|---|---|
| `baseline_compteurs.txt`, `resume_compteurs.py` | compteurs et temps locaux de 30 exécutions (3 trames du contrat, 2 familles synthétiques à 8 000 et 32 000 points, K = 5 et 10, 1 et 4 fils ; une trame avec sol à K = 5 et 10) |
| `profil_rdtsc_etages.txt`, `sonde_rdtsc.diff` | parts par étage, entonnoir ; différentiel de la copie instrumentée |
| `callgrind_resume.txt`, `essai_filtre_boucles_interverties.diff` | instructions et mauvaises prédictions par fonction et par ligne ; HEAD contre J3, contre `x86-64-v3` et contre le filtre à boucles interverties |
| `frontiere_parts.txt` | part de l'arbre traitée en largeur à 1, 4, 24 et 48 fils |
| `feuilles_histogrammes_et_duplication.txt` | tailles de liste, étendues, côtés de boîte ; duplication des triplets et quadruplets |
| `ablation_kT.txt`, `ablation_M.txt`, `ablation_reservoir.txt` | bits sous-unitaires, taille de feuille, réservoir : dumps et compteurs ; modèle de coût par boule |
| `mesures_diverses.txt` | trame avec sol, mutants de frontière et de bande, essai des rangs, extraits du reçu G4, désassemblage, passes alternées |
| `mutants.diff`, `essai_rangs_accumulateurs_locaux.diff`, `sonde_vidage_candidats.diff` | différentiels exacts appliqués aux copies sous `/tmp` |
| `oracle/` | oracle brut indépendant, comparateur, campagne et ses 43 lignes de résultat, mutants contre l'oracle, porte du dépôt jouée sur les mutants, variante J3 contre l'oracle |
| `invariants/` | restriction, Euler, invariance aux fils, tailles des structures, invariants joués sur les mutants |
| `microbancs/` | quatre sources et leurs sorties |
| `bords/` | scripts et sorties : CLI, sphères entières, extrêmes u18, sanitizers et étendue maximale |

## Annexe B — Reproduire

```bash
W=/tmp/v11-audit/l05_code_catalogue
rsync -a --exclude receipts --exclude audits --exclude docs --exclude bench /workspaces/E-HGP/build/v11-worktree/morsehgp3D_v10/ $W/src/morsehgp3D_v10/
cmake -S $W/src/morsehgp3D_v10 -B $W/build-rel -DCMAKE_BUILD_TYPE=Release && cmake --build $W/build-rel -j3 --target mhgp10_catalogue
$W/build-rel/mhgp10_catalogue /workspaces/E-HGP/build/v10-g4-data-s1/lidar02_full.u32le --k=5 --threads=1
# oracle independant : g++ -O2 -std=c++20 oracle/brute_oracle.cpp -o brute_oracle ; python3 oracle/compare.py <catalogue> <oracle> IN.u32le K [--leaf=..]
# invariants : python3 invariants/restrict_check.py <catalogue> IN.u32le K ; python3 invariants/euler.py <sortie JSON a K+2>
# mutants : appliquer un bloc de mutants.diff a une copie, reconstruire, rejouer les portes
```

Les dossiers `/tmp/v11-audit/l05_code_catalogue/{prof,dumpq,micro,mut,cg,j3,abl_T0,abl_T3,fixranks}` contiennent les copies modifiées, leurs builds et leurs sorties brutes ; ils ne survivent pas à un redémarrage de la machine.
