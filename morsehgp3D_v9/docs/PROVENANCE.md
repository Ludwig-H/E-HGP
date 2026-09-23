# Provenance des ports de la v9

22 septembre 2026. Cadre : `exploration_v9_hors_registre`, `backend=reference_cpu`
(premier moteur v9), `profile=quantized_u18_input_only`, `public_status=not_claimed`.
Chaque port est explicite, épinglé et requalifié par les portes v9 ; la v7
et la v8 restent des sources différentielles, jamais des autorités.

## Tour FULL : `src/tower/` (espace `mhgp9::tower`)

Source : `morsehgp3D_v7/src` au pin `dc57ffd5` (dernier commit de ces sources :
`324f6192`). Seule la fermeture d'inclusion utile de
`forest/full_ball_tower.hpp` est portée ; `pipeline/expand.hpp` (et le fold v4
`forest/fold.hpp` qu'il tirait) est remplacé par `forest/ball_data.hpp`.

Changements de fond pour le domaine 18 bits (M = 262 143) :

| fichier | changement | raison |
| --- | --- | --- |
| `core/types.hpp` | `kCoordBits = 18`, `kCoordMax = 262143` | domaine d'entrée |
| `core/morton.hpp` | clé 54 bits (masque 18 bits, masques d'étalement 21 bits) ; `static_assert` de l'étalement | à 16 bits, deux positions distinctes recevaient la même clé |
| `tree/cloud_index.hpp` | cellule de préfixe : 10 bits hauts inutilisés, 18 niveaux | clé 54 bits |
| `pipeline/census.hpp` | minimiseur par axe borné à `kCoordMax` (et non 65 535) ; bornes de clé A < 2^76, \|B\| < 2^96, \|C\| < 2^116 | sinon le minimum d'une boîte au-delà de 65 535 était surestimé : faux élagage d'intérieurs |
| `forest/plateau.hpp` | produits normale · (centre − sommet) de `triangle_closed` et `tetra_closed` en entiers signés 192 bits | termes jusqu'à 2^134 à 18 bits : débordement i128 (comportement indéfini) |
| `forest/local_plateau.hpp` | gardes de clé A < 2^76, \|B\| < 2^96, \|C\| < 2^116 ; positions validées par `p3_in_profile` | domaine 18 bits |
| `forest/full_ball_tower.hpp` | mêmes gardes de clé ; raison `full_ball_u18_domain` ; inclusion de `ball_data.hpp` au lieu d'`expand.hpp` | domaine 18 bits, retrait du fold v4 |
| tous | `namespace mhgp7` → `mhgp9::tower`, macros `MHGP7_*` → `MHGP9_*` | espace de noms |

Bornes vérifiées à la main, à confirmer par la porte `arith_u18` : q3 A < 2^76,
\|B\| < 2^96, \|C\| < 2^116, puissance < 2^117, niveau (numérateur < 2^113,
dénominateur < 2^79, produits croisés < 2^192) ; q4 det < 2^60, \|N'\| < 2^79,
clé A < 2^60, \|B\| < 2^81, \|C\| < 2^100, puissance < 2^102, niveau
(numérateur < 2^160 en U192, dénominateur < 2^120, produits croisés < 2^280 en
U320). Les autres formules ont été relues sans changement nécessaire.

Empreintes sha256 (16 premiers caractères), source au pin puis port :

| fichier | v7 `dc57ffd5` | v9 |
| --- | --- | --- |
| `core/device.hpp` | `1f7150b21138c445` | `741377a5f8c05997` |
| `core/intmath.hpp` | `6f807dbd01c07ba2` | `28a1fc6279e3b449` |
| `core/morton.hpp` | `67e9f2bc5d388f99` | `8ad508002ac68264` |
| `core/mutants.hpp` | `870266d181c67b2d` | `50c7c50e36f401ef` |
| `core/types.hpp` | `913e9e89ebf40b7a` | `3ab5e0378b8b2bf5` |
| `core/wide.hpp` | `a4ac26dd4968e0d4` | `9f88b3f76ad02401` |
| `forest/anchor_meb.hpp` | `386072c8a02bbb83` | `cbd97a2db29af65b` |
| `forest/ball_data.hpp` | (extrait d'`expand.hpp`) | `cb53d281765a9840` |
| `forest/full_ball_tower.hpp` | `83f1c78e0656f08c` | `d643b729ec185165` |
| `forest/full_certificate.hpp` | `463724b74c7c31e1` | `fd36213350637ce2` |
| `forest/full_coverage_certificate.hpp` | `7608e70ec0bf7df7` | `aca8799cc52349a4` |
| `forest/local_plateau.hpp` | `df56fbf33ea30882` | `bfbe4e09cdf4da4a` |
| `forest/plateau.hpp` | `9d40af95f4429c02` | `c04ffdad4575d959` |
| `lanes/keys.hpp` | `c4fcf2044ebe2bbf` | `f4dce29efe679491` |
| `lanes/level.hpp` | `acd6641e0616c926` | `41345a9ae86f54d1` |
| `lanes/q2.hpp` | `11049293b7ad2f71` | `b6527c183cac2d09` |
| `lanes/q3.hpp` | `4155a1c39193b68c` | `5e1df08077af156a` |
| `lanes/q4.hpp` | `58aac9bd57ac1a9b` | `24bfe7b15e32adee` |
| `parallel/pool.hpp` | `5c20aabbe673e2ba` | `e7680ed4dbdc0f69` |
| `pipeline/census.hpp` | `95732fd89771ac08` | `f35bdf0d87a2e667` |
| `tree/cloud_index.hpp` | `8c5acf166ce378b0` | `2426a0cfa42689ae` |

Portes et oracles portés (`tests/tower/`, `oracle/tower/`) : `full_certificate`,
`full_coverage_certificate`, `anchor_meb`, `local_plateau`, `full_ball_tower`
(voies temporelle et statique 1/4 fils) et le juge T2 `census_tower_oracle.hpp`,
avec l'oracle rationnel `local_plateau_oracle.hpp`. Fixtures ajoutées aux
extrêmes 18 bits : tétraèdre de côté 262 142 et cinq sommets aux coins du cube
262 143 ; refus à 262 144 ; planchers exacts ajustés en conséquence.

## Générateur exact : `src/gen/` (espace `mhgp9::gen`)

Source : `morsehgp3D_v8/src` au commit `3f0d188f` (tranche 18 bits commise le
22 septembre ; identique à `c5ba6e7f`), les 65 fichiers hors voie float32, et
la bibliothèque `mhgp8_p0` entière (24 unités de compilation). Changements :
`namespace mhgp8` → `mhgp9::gen`, macros `MHGP8_*` → `MHGP9G_*`, préfixe des
messages d'exception. Aucun changement de logique. Le retrait des prototypes
abandonnés (arbre de plages du nuage, dispatcher `Donate`, T1, T24–T29) suit la
[carte des sources](audit_v8/16_carte_du_code_v8.md) et viendra avec les
portes du générateur portées.

Portes du générateur (`tests/gen/`, 23 septembre 2026) : les 27 portes natives
de `morsehgp3D_v8/tests` au même commit `3f0d188f`, leurs oracles Boost
(`exact_ball_oracle.hpp`, `oracle/`) et les fixtures de front, avec les mêmes
changements mécaniques d'espace et de macros ; chaque porte est jugée sur sa
voie `--selftest` (code 0). Les scripts Python de mutation de la v8
(`wspd_q34_mutations.py` et voisins) deviennent `tests/gen/mutants.json` : une
copie mutée de l'unité est compilée et liée avant `libmhgp9_gen.a`, et la porte
doit rendre le code 1 et le stderr causal exact (`tests/gen/run_mutant.cmake`).
Un site non unique est refusé à la configuration ; le seul mutant désactivé
l'était déjà en v8.

Écart assumé à la source v8 : le census q3 sur feuille exacte de l'atlas
(`Q4LocalCellCertificate`, `Q4LocalOptions::retain_q3_fragments`,
`WspdQ34Options::q3_leaf_census`), levier proposé par l'auditeur A. Une q3
possédée par l'arête ab a son centre dans une cellule fermée de l'atlas de ab
et sa boule dans le cover (rayon au plus $|ab|/\sqrt{3}$, décalage du centre
au plus $|ab|/(2\sqrt{3})$) : profondeur = compte certifié de la feuille +
sites de frontière de puissance négative, coquille = sites de frontière de
puissance nulle. Une cellule profonde sans fragment reste un minorant et
retombe sur le census global. L'option est refusée sans la consultation de
l'atlas sur Local28. Porte `wspd_q34` : variante feuille sur toutes les
fixtures (oracle rationnel identique), identités de registre généralisées,
fixture du contre-audit B (`a=(14,20,20)`, `b=(26,20,20)`, `x=(20,29,20)`,
`y=(20,16,20)`, `m=(20,20,20)`, K3 : racine profonde exacte au compte 1,
servie par un fragment conservé) et planchers ; deux mutants causaux
(coquille comptée intérieure sur la feuille, compte certifié oublié).

Code neuf v9 (23 septembre 2026) : le **certificat de voie morte**
(`lanes/q34_dead_lanes.{hpp,cpp}`, option `WspdQ34Options::dead_lanes`,
défaut de la chaîne). Mesure qui le motive : sur 08/000000 sans sol à K5, les
arêtes qui n'émettent **rien** prennent 86 % des cycles q3/q4 instrumentés
(97 % hors filtre de paires), avec des covers de 1 704 sites en moyenne contre
43 pour les arêtes vivantes (erratum du reçu `q34_dead_edges_20260923`).
Énoncé : les centres des boules possédées par ab vérifient
$|c-m|^2 \le |ab|^2/12$ (q3 aiguë) et $|c-m|^2 \le |ab|^2/8$ (q4 positive),
par l'identité barycentrique du rayon ; une voie est vide si des cellules
dyadiques fermées recouvrent son disque, chacune disjointe du disque ou
portant au moins $T$ sites distincts du cover strictement intérieurs pour
tous ses centres ($T=K-1$ en q3, $K-2$ en q4). Formes affines des sites du
cover calculées une fois par arête, maximum exact aux coins en i64 ; un échec
ne dit rien et la voie exacte tourne inchangée. Porte `wspd_q34` : variantes
avec et sans certificat sur toutes les fixtures contre l'oracle rationnel,
identités de masse étendues aux voies prouvées, fixture d'une q3 presque
équilatérale au bord du disque, cinq mutants causaux (seuils K−2 en q3 et
K−3 en q4, contact compté intérieur, un seul coin, disque rétréci).
Les deux voies sont prouvées en **une seule** récursion (le disque q3 est
dans le disque q4 : une cellule qui tient K−1 intérieurs vaut pour les deux,
K−2 pour q4 seule).

Code neuf v9 (23 septembre 2026) : le **cache des nœuds témoins** du filtre de
paires (`q34_cached_witness_rejections`, option
`WspdQ34Options::pair_witness_cache`, défaut de la chaîne). Pour A = {a} et
B = {b}, la recherche Affine est exacte : une voie est rejetée ssi elle a au
moins T témoins stricts, et chaque nœud admis ne contient que des témoins
stricts. La recherche tracée enregistre ses nœuds admis et leurs voies
(disjoints par voie) ; pour la paire suivante de même extrémité a, ces nœuds
sont retestés avec la **même** admission exacte, crédités seulement pour
leurs voies : les voies qui atteignent T sont rejetées sans recherche, les
autres passent par la recherche complète. Sur 08/000000 K5, 70 % des paires
sont rejetées par le cache ; condensés inchangés sur les trois trames. Porte
`q34_witness_cache` (24 804 paires : égalité avec la recherche sur la même
paire, inclusion dans la recherche d'une autre paire) et trois mutants
causaux (crédit toutes voies, borne basse de Ξ, seuil q4 à K−3) ; variante de
la porte `wspd_q34` contre l'oracle rationnel.

Code neuf v9 (23 septembre 2026) : le **noyau diamétral** du certificat de voie
morte (`Q34EdgeCover::make_diametral`, option `WspdQ34Options::dead_core`,
levier `q34_dead_core`, défaut de la chaîne). Le certificat ne crédite que des
sites distincts strictement intérieurs à toutes les boules d'une cellule :
n'importe quel sous-ensemble du cover donne donc une preuve valide, avec
moins de crédits. Il est d'abord tenté sur la boule diamétrale fermée
$|2z-a-b|^2 \le |b-a|^2$ (a et b sur son bord) ; le cover n'est construit, et
le certificat rejoué, que pour les voies restées ouvertes. Sur 08/000000 à
K5, 1,14 M des 2,04 M arêtes sont closes par le noyau et les formes chargées
tombent de 2,96 G à 0,61 G (0,36 G de noyaux + 0,25 G de covers restants,
soit `dead_core.form_sites + dead.form_sites` contre `dead.form_sites`) ;
CPU q3/q4 du harnais hors dépôt −18 % à K5 et −16,5 % à K10, flux émis
identique : indications de développement, pas un reçu. Le noyau porte
`complete() == false` et `require_complete_q34_cover` le refuse à tout autre
consommateur (census, atlas, graines, fenêtre, domaine positif, réserve de
témoins). Identités : expansées = covers + arêtes closes par
le noyau + paires rejetées ; noyaux = covers + arêtes closes ; voies ouvertes
après le noyau = voies prouvées + ouvertes sur le cover. Portes : variante
`wspd_q34` contre l'oracle rationnel (voies prouvées par le noyau et voies
prouvées seulement sur le cover après lui), appartenance exacte du noyau et
inclusion dans le cover et refus par les huit consommateurs (`q34_cover`),
trois mutants tués (deux par le ledger, un par la garde).

Code neuf v9 (23 septembre 2026) : le **MEB proposé** de la descente des
ancres FULL (`anchor_meb_proposed`, `tower/forest/anchor_meb.hpp`), qui rend
le même résultat que l'énumération de référence (`anchor_meb` : première paire
maximale, puis tous les triplets et quadruplets en ordre lexicographique) —
clé, niveau, emplacements du support, coquille sélectionnée — avec moins de
travail. Un Welzl en double **propose** seulement un support ; la même
tentative exacte le vérifie (support positif, tous les sites contenus). Par
unicité du MEB euclidien, un support vérifié est le MEB, et tout support
valide est sur son bord exact : le premier support valide de la référence se
trouve donc par la même énumération restreinte aux sites du bord. Une
proposition refusée retombe sur l'énumération complète ; les valeurs en double
ne décident rien. Porte `anchor_meb_proposed` (28 956 ensembles de 1 à 10
sites : grilles u18, 2^16 et minuscules, points entiers cosphériques, cube et
octaèdre ; supports essayés 704 665 → 72 029), variante à propositions
faussées une fois sur trois (5 013 replis, mêmes résultats) et mutant sans
canonisation tué. Sur 08/000000/K10 (catalogue local, W8) : cycles MEB
154 → 70 G, phase de cibles statiques 17,0 → 12,3 s, tour 24,7 → 18,9 s,
condensé inchangé ; libellé de comptabilité
`anchor_meb_first_maximal_pair_then_double_welzl_proposal_exact_boundary_canonical_v3`
(sonde v10).

## Voie GPU S1 : `src/gpu/` (espace `mhgp9::gpu`, code neuf)

23 septembre 2026. Première brique GPU de la v9, pour une expérience de
débit bornée : le filtre témoin exact q3/q4.

- **Port** : `witness_filter.hpp` porte `filter_impl` avec bornes
  d'exclusion de `lanes/q34_witness_search.cpp`, en version hôte et device.
  Il couvre la spécialisation affine des paires et la générale des boîtes.
  Arithmétique entière identique (i64, i128), même ordre de DFS, mêmes
  crédits, pile bornée à 55 cadres (borne prouvée de l'index).
  `flat_index.hpp` en fait une copie plate de l'index.
- **Exécution** : `filter_runner.cu` fait un fil par rectangle, un balayage
  exclusif CUB des masses survivantes, puis un fil par paire développée
  (ordre du moteur, sans cache). `filter_runner_stub.cpp` le remplace sans
  CUDA. L'option CMake `MHGP9_ENABLE_CUDA` est désactivée par défaut
  (sm_120) ; les unités C++ gardent `-Wall -Wextra -Wpedantic -Werror`.
- **Porte `gpu_witness_filter_port`** : le port compilé pour l'hôte est
  comparé à `filter_q34_witnesses`, requête par requête. Elle couvre les
  rectangles réels du front et un échantillon des paires développées, trois
  familles, K3, K5 et K10, et exige des totaux de nœuds visités égaux. À
  2 000 sites : 1,71 M rectangles et 1,20 M paires identiques ; témoin de
  sensibilité (K−1) à 211 k divergences ; variante `scale8000`.
- **Sonde `mhgp9_gpu_filter_probe`** (schéma `mhgp9_gpu_filter_probe_v1`) :
  elle reproduit la population de la chaîne (front, rectangles, paires).
  Trois références CPU : rectangles, paires sans cache, paires avec le cache
  de ligne du moteur ; ce dernier doit donner les mêmes masques. Elle refait
  ensuite le passage sur GPU et exige des masques et des totaux de visites
  égaux. Sur 08/000100/K5 en local (W8) : 2,35 M rectangles, 12,0 M paires ;
  filtre CPU 2,4 s (rectangles) plus 2,8 s (paires avec cache).
- **Sonde v2, après la revue multi-agents et les notes de B** :
  - clé `options.inject` ; mutant causal `--inject=pair_mask`, qui inverse le
    masque GPU d'une paire et doit être détecté exactement une fois ;
  - une violation de pile sur le GPU est un désaccord (code 1), car le CPU a
    passé les mêmes requêtes ;
  - code 3 réservé au GPU absent ou à une erreur CUDA ou hôte ;
  - nuage vide ou sites dupliqués : refus d'entrée en code 2, avec une porte
    `gpu_filter_probe_empty_input` ;
  - toute exception de la sonde donne le code 3, jamais un abandon.

## Chaîne : `src/chain/` (espace `mhgp9`, code neuf)

`run_tower_chain` enchaîne le générateur (configuration mesurée des reçus v8 :
q2 `SharedBlocks`, frère saturant, `ComplementFirst`, Pool64, propositions
{2, 16, héritage} sur son propre front ; q3/q4 masque 6, Local28 LiveOnly 64,
témoins `RectanglePair` affines, census q3 par boîtes, consultation de
l'atlas), le catalogue canonique et la tour. Le catalogue recoupe les deux
implémentations et refuse toute divergence : clé v8 contre clé recalculée par
les formules v7 depuis le support ; compte d'intérieurs et taille de coquille
du générateur contre un census exact sur l'index de la tour ; `q_min` de la
coquille contre la plus petite arité présentée. Une coquille de plus de
12 sites est un refus de domaine explicite (`unsupported_degeneracy`).

Juge : `tests/chain/chain_census_tower_gate.cpp`, le juge T2 de la v7 appliqué à
la chaîne réelle (inventaire rationnel exhaustif, modèle Γ, K = 1..10,
séparations 8/10/12, un et quatre fils de chaîne).

### Invariant d'Euler du catalogue (sonde v13)

Proposition de l'auditeur C (`audits/NOTE_C_INVARIANT_EULER_20260923.md`),
prouvée sans position générale par le nerf (contrelecture B,
`audits/CONTRELEC_EULER_PAR_NERF_20260923.md`). Pour
K ≤ min(Kmax − 2, n), la somme n·[K = 1] + Σ e_K(B) sur les boules du
catalogue vaut 1 si le catalogue est complet. Définitions :
- p est le nombre d'intérieurs de B ;
- e_K(B) est le coefficient de t^{K−1} dans t^p Σ_T (t − 1)^{|T|−1} ;
- T parcourt les sous-coquilles dont l'enveloppe contient le centre
  (`ShellTable::contains_center`) pour une coquille étendue, et T = U seule
  pour une coquille régulière.

C'est une condition **nécessaire** seulement : une somme juste ne certifie pas
chaque clé. Le calcul se fait pendant le recensement, sur ses ouvriers. Le
Kmax pris en compte est celui qui est **demandé**, d'où la borne min(Kmax−2, n) :
deux sites à Kmax = 10 vérifient les ordres 1..2 sans faux échec. Sous Kmax < 3,
le statut est `not_checkable`, jamais un succès vacant. Une violation fait
refuser la chaîne (`invariant_violated`, `chain_catalogue_euler_violated`) :
aucune tour n'est publiée sur ce catalogue.

Portes :
- `mhgp9_chain_euler` : 90 exécutions, 444 ordres vérifiés dont 306 à K10,
  1 321 boules à coquille étendue. Cas couverts : carré plan dégénéré (contre-exemple de B à la
  formule générique, Kmax = 1..10), deux sites, cube, octaèdre et son
  centre, points entiers d'une sphère, 36 nuages aléatoires sur grilles
  minuscules et u18.
- Deux mutants de la chaîne recompilée, tués dès le carré : coquille étendue
  traitée comme régulière, terme des sites oublié.
- La porte T2 exige `holds` sur les 6 catalogues qu'elle juge
  exhaustivement.
- Sur LiDAR (000200 16k), les condensés sont inchangés, avec `holds` à K5
  (ordres 1..3) et à K10 (ordres 1..8).

### Mesures publiées par la sonde v13

- `q34_occupancy` : pour les ouvriers q3/q4, fils démarrés, murs extrêmes,
  sommes de CPU de fil et d'attente sur la file, tâches publiées et
  consommées (`WspdQ34WorkerTiming`, jusque-là non publié).
- `tower_phases_ms` : murs des phases de la tour (`FullBallTimes`), à savoir
  la validation, la phase 0 par K, les lots par K, les populations, les images
  par K, la banque et l'encodage par K. Sur la voie séquentielle, chaque ordre
  est mesuré entier.

Ce sont des mesures, jamais comparées entre exécutions. Le lecteur G4 borne :
- les murs des ouvriers par le mur de q34 ;
- le CPU des ouvriers par fils × mur ;
- la somme des phases par le chrono de la tour.

Il exige aussi que les phases relèvent exclusivement de la voie statique ou
de la voie séquentielle, et que la phase 0 soit égale à la somme de ses
valeurs par K. Porte `probe_worker_contract` : 49 mutants tués.

### Ordonnancement des jobs du front q3/q4 (sonde v14)

La session G4 R8 montre que les fils q34 passent 35 à 49 % de leur temps à
attendre la file de tâches à K5 et W48. En local (000000 K5, W8), un seul job
du front dure 14,8 s sur 22 s de q34 : la préparation en largeur laisse un
produit dense entier. Deux leviers d'ordonnancement sont désormais actifs par
défaut, épinglés dans le plan et publiés ; ils ne changent pas l'objet.
- `q34_jobs_by_mass` : la préparation scinde d'abord le produit en attente de
  plus grande masse de paires (`make_wspd_front_jobs(..., mass_first)`), puis
  range les jobs par masse décroissante.
- `q34_fine_jobs` : 64 jobs par fil au lieu de 16.

En local, le plus long job passe de 14,8 à 2,9 s et l'attente de 11,9 à
0,01 s, avec un condensé identique. `q34_occupancy` publie désormais
`job_sum_s` et `max_job_ms`.

Portes :
- `wspd_front_jobs` : chaque plan est aussi préparé par masse et comparé au
  front monolithique jugé par l'oracle indépendant (préfixe + jobs égaux, masses
  non croissantes, plus de 1 000 plans).
- `wspd_q34` : chaque cas parallèle est rejoué par masse, soit 425 cas
  identiques à l'oracle rationnel et aux compteurs mono.
- Porte de raccord : 53 mutants tués.

La session G4 R9 ([reçu](../receipts/g4_tower_r9_20260923/README.md)) mesure
l'effet de ces deux leviers en ablation appariée, avec des condensés égaux
ON/OFF. À K5, la chaîne passe de 3,69 / 5,24 / 6,30 s à 2,80 / 3,80 / 3,99 s.
À K10, elle passe de 9,51 / 13,60 / 14,95 s à 8,60 / 11,75 / 11,89 s.
L'attente de file tombe de 35–49 % à environ 1 %, et le plus long job de
2,5–3,7 s à 0,23–0,31 s.

### Tour statique : phase A recouvrant la phase 0 (sonde v15)

Levier `tower_overlap_static`, actif par défaut et épinglé. La phase 0 (cibles
statiques) est calculée par K décroissant sur les fils géométriques, pendant
qu'un fil par ordre attend ses cibles puis lance sa phase A ; l'ordre 1 démarre
aussitôt. Même objet :
- la phase A d'un ordre ne lit que son propre état, le catalogue, les
  programmes et l'index, qui sont immuables ;
- la phase 0 n'écrit que les membres statiques du constructeur, un ordre à la
  fois.

Échecs : un échec de phase 0 est rapporté pour le plus petit K, avant tout
échec de lot ou d'image, comme sur la voie classique. Chronos : `lots` devient
le reste de la phase A après la phase 0, et chaque `lots_by_k` est borné par
la fenêtre phase 0 + reste.

Porte `order_failure_priority` : elle couvre les deux voies, avec un nouveau
point de panne de phase 0 (échec de plus petit K sur trois scénarios par
voie) et les deux mutants de priorité et de compteurs, placés aussi dans la
voie recouverte. En local (W8, 16k K10), la tour passe de 4,5–4,96 s à
4,26–4,28 s, avec un condensé identique.

### Ordonnancement des jobs du front q2 (sonde v16)

Le recensement q2 découpait lui aussi son front en largeur (16 jobs par fil,
compteur atomique). En local (000000 K5, W8), un seul job durait 1,64 s sur
1,72 s de q2. Levier `q2_jobs_by_mass`, actif par défaut et épinglé : le plan
est préparé par masse décroissante (`WspdQ2Schedule::mass_first`), avec
64 jobs par fil. En local, q2 passe de 1 711 à 1 048 ms, avec les mêmes paires
acceptées et le même catalogue. Porte `wspd_q2_parallel` : chaque exécution
parallèle est rejouée avec le plan par masse, ce qui donne la même sortie et le
même travail mono (au moins 400 exécutions).
