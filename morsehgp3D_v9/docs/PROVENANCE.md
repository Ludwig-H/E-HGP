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

### Filtre témoin q3/q4 par lots (S2, sonde v17)

23 septembre 2026, après la session S1. `gen::run_wspd_q34_batched` s'exécute
en trois phases.
- **Front** : ses jobs ne font que collecter leurs rectangles.
- **Filtre** : un appel décide tous les rectangles (bornes affines), puis
  toutes les paires des rectangles survivants, sans cache.
- **Survivants** : les ouvriers ne traitent que les paires survivantes, par
  la seconde moitié d'`Engine::edge` (`filtered_edge` : cœur, couverture,
  certificat, q3/q4).

**Mêmes décisions que le moteur** (son cache ne change jamais un masque
final), donc mêmes candidats. Les identités de `validate_completion`
tiennent : recherches de rectangle = rectangles, recherches de paire =
paires développées, cache nul.

**Deux implémentations de l'appel** :
- `run_q34_filter_batch_cpu`, la référence ;
- le passage GPU `gpu::run_filter_batch`. Ce sont les noyaux de S1, suivis
  d'une compaction des survivants sur l'appareil (drapeaux, balayage
  exclusif, dispersion, dans l'ordre du moteur) et des rejets par voie.

**Chaîne** : leviers `q34_batch_filter` et `q34_gpu_filter` (le second
exige le premier ; sans GPU, refus `chain_q34_gpu_unavailable`, jamais un
repli CPU). `mhgp9_chain` lie `mhgp9_gpu` (stub hors CUDA).

**Sonde v17** : les deux leviers, et une section `q34_batch` (backend,
front, filtre, survivants, passe GPU, rectangles, survivants).

**Portes** :
- `chain_batch_filter` : candidats triés égaux entre moteur et lots, à 1 et
  4 fils, trois familles, K3/K5/K10 ; même tour FULL et même registre. Le
  levier GPU est refusé sans appareil. Deux mutants causaux sont tués :
  retrait cohérent de la voie q3, qui change les candidats ; rejet q3
  menteur, que les identités de masse refusent.
- Contrat sonde/worker : cas par lots et cas GPU, 10 mutants de la section
  `q34_batch`.

Localement, sur 08/000100/K5 : condensé FULL `dbf799c8ed83f53f` identique
entre moteur et lots CPU.

**Protocole G4 de la tour** : build CUDA (nvcc et nvidia-smi existants),
`backend=cuda_g4` et `GPU_executed` selon le plan.

### Certificats de voie morte par lots (S3, sonde v18)

23 septembre 2026, soir. Une phase s'insère entre le filtre et les
survivants de `gen::run_wspd_q34_batched` : un appel décide le certificat
de voie morte de **tous** les survivants, exactement comme
`Engine::filtered_edge`. D'abord le cœur diamétral et son certificat, puis,
pour les voies qu'il laisse ouvertes, la couverture et son certificat.

- **Les ouvriers** ne font plus que la génération q3/q4 des voies restées
  ouvertes (`Engine::certified_edge`). La couverture y est reconstruite
  pour la génération seule, sans être recomptée.
- **Un survivant mis en attente** (`deferred`, par exemple faute de mémoire
  sur l'appareil) exécute l'arête moteur complète : mêmes décisions, mêmes
  compteurs.

**Frontière de confiance.** Le chemin par lots contrôle la forme de
l'appel : tailles, masque inclus dans celui du survivant, drapeau d'attente
0 ou 1. Il contrôle aussi les identités qui lient les compteurs aux masques
rendus :
- constructions du cœur = survivants décidés ;
- formes = sites − 2 × chargements ;
- voies prouvées + ouvertes du cœur = voies des survivants ;
- voies ouvertes du cœur = voies prouvées + ouvertes de la couverture ;
- voies ouvertes de la couverture = voies des masques rendus.

Un mensonge cohérent sur une décision reste l'affaire des différentiels.

**Deux implémentations** :
- `run_q34_certificate_batch_cpu`, la référence : le prouveur produit par
  survivant ;
- `gpu::run_certificate_batch`. Des warps persistants, **un warp par
  arête**, exécutent le port portable `gpu/certificate.hpp`. Le parcours
  d'arbre sans pile et la récursion des cellules sont uniformes. Les
  balayages de frontière sont répartis sur les 32 voies par votes : la
  position d'arrêt exacte du balayage séquentiel est retrouvée dans les
  masques de vote, et les sites partiels sont compactés dans l'ordre.
  Chaque warp dispose d'une ardoise fixe de 65 536 sites (plages, formes,
  une frontière par profondeur balayée). Une arête plus grosse est mise en
  attente. Les liens d'échappement de l'index plat sont recalculés depuis la
  structure préordre et vérifiés : tout parcours avance et termine.

**Chaîne** : leviers `q34_batch_certificates` (exige `q34_batch_filter` et
`q34_dead_lanes`) et `q34_gpu_certificates` (exige le premier). Sans GPU,
refus `chain_q34_gpu_unavailable`. Le contexte CUDA et l'index plat
(échappements compris) sont préparés pendant q2 dès qu'un levier GPU est
actif.

**Condensé du catalogue.** `ChainOptions::catalogue_digest` calcule, après
la chaîne et hors chronomètre, un condensé canonique du catalogue complet,
sur la vue de l'auditeur C : boules triées par clé, puis clé, niveau exact,
arité, intérieurs triés et coquille triée.

**Sonde v18** :
- les deux leviers ;
- `--catalogue-digest` ;
- la section `q34_batch` complétée : backend et temps de l'appel des
  certificats, passe GPU, survivants mis en attente ;
- `catalogue_digest` et `times_ms.catalogue_digest`.

**Portes** :
- `gpu_certificate_port` : le port compilé pour l'hôte (groupe de 32 voies
  émulé) contre le prouveur produit, arête par arête et champ par champ, sur
  les survivants réels du filtre, trois familles, K3, K5 et K10.
  - À 2 000 sites : 775 791 arêtes identiques, dont 154 839 mises en attente
    exactement quand un cœur ou une couverture dépasse une capacité réduite
    à 64.
  - Planchers sur chaque classe de cellule et de voie ; 3 116 mutants du
    comparateur tués ; variante `scale8000`.
- `chain_batch_certificates` :
  - candidats triés égaux au moteur, trois familles, K2 (q3 seul), K3, K5,
    K10, à 1 et 4 fils ;
  - travail des certificats, de la couverture, des voies et des émissions
    égal au chemin par lots sans certificats ;
  - chemin de mise en attente (un survivant sur trois) égal ;
  - mutants tués : masque élargi, drapeau invalide, compteur menteur, attente
    qui garde ses compteurs, retrait cohérent de la voie q3 ;
  - chaîne : même tour FULL, même registre et même condensé de catalogue que
    le moteur ; le condensé égale celui du catalogue publié et voit une
    boule retirée ou une coquille modifiée ;
  - refus explicite du levier GPU sans appareil ; leviers incohérents
    refusés avec leur raison.
- Contrat sonde/worker : cas `cert_on`, 8 mutants des certificats,
  3 mutants du condensé, comparaison entre cas sensible au condensé et au
  travail des certificats.

Localement, sur la trame entière 08/000000/K5 (W8) : moteur et lots CPU
avec certificats donnent le condensé FULL `67450c64611075b1`, le condensé
de catalogue `5ad1fe09354411ba` et un travail des certificats identique.

**Revue multi-agents avant session** (cinq relecteurs, cinq vérificateurs ;
aucun défaut bloquant, un constat réfuté, le reste corrigé) :
- **Frontière** : `__syncwarp` au début de chaque cellule balayée. Les votes
  n'ordonnent pas la mémoire sur l'appareil ; la frontière d'une profondeur
  est réécrite d'une cellule à l'autre.
- **Occupation** : nombre de warps par la requête d'occupation (250
  registres : 8 warps résidents par SM, non 16).
- **Porte du port** : branche sans cœur, K2, garde d'entrée du GPU (13
  champs forgés refusés par famille).
- **Chaîne** : option `q34_certificate_capacity` et sonde
  `--certificate-capacity=N`. Le catalogue est libéré dans la chaîne comme
  sans condensé ; le mur et le CPU du condensé sont retirés du total. Un
  échec de condensé remet les condensés à zéro et classe l'allocation en
  ressource épuisée.
- **Porte de chaîne** : tout le travail des certificats comparé ; passage
  GPU à ardoise de 64 sites, qui doit mettre en attente.

**Préflight B avant session** (portes de domaine et juge) :
- **Garde** : toute feuille doit avoir exactement un rang (une feuille à
  plusieurs rangs serait « scindée » en rien par le parcours du cover) ;
  masque hors des voies de K refusé (rien à K1, q3 seul à K2).
- **Lot vide** : retour avant toute lecture des tableaux, aucun noyau ; il
  n'est pas compté comme passage GPU.
- **Juge** (`judge_certificate_filter`, levier de chaîne
  `q34_certificate_judge`, sonde `--certificate-judge`) : chaque survivant
  décidé par l'appel est recalculé par la référence CPU ; masques par arête
  et travail sommé doivent être égaux.
- **Nouveaux champs** de `q34_batch` : arêtes jugées, covers reconstruits
  par les ouvriers (hors registre), warps du noyau.
- **Portes** :
  - flux comparés avec les identifiants de coquille (première et seconde
    partie, dans l'ordre) ;
  - travail logique complet comparé (q3, q4 local et fenêtre, blocs,
    cellules, payload ; seules les capacités de tampons réutilisés sont
    exclues) ;
  - mensonge cohérent refusé par le juge.

**Protocole G4 de la tour v18** :
- les deux préflights GPU passent le juge (`--certificate-judge`) : arêtes
  jugées = survivants − attentes ;
- les six condensés épinglés par l'auditeur C
  ([audit](../audits/c_catalogue_digest_20260923/README.md)) : tour et
  catalogue des trois trames à K5 et K10, s = 8, obtenus sur le moteur, le
  lot CPU et les certificats CPU. Tout cas R13 doit les reproduire, GPU ou
  non ; une erreur commune aux deux jumeaux se voit donc. La porte de C
  (`mhgp9_catalogue_digest`, fixture de coquilles étendues, sensibilité,
  zone aveugle, deux mutants recompilés) est adoptée telle quelle ;
- préflight de mise en attente : le préflight GPU est rejoué avec une ardoise
  de 64 sites (localement, 3 404 arêtes en attente sur 55 523). Il doit
  donner la même tour, le même catalogue et le même travail des certificats,
  avec au moins une arête en attente et pas toutes ;
- `deferred == 0` exigé sur toute trame plus petite que l'ardoise par
  défaut ;
- travail des certificats comparé dès le préflight moteur ;
- jumeau de même trame, K, s **et** mêmes leviers de certificat ;
- le worker passe `--catalogue-digest` à chaque sonde ;
- la comparaison entre cas du même (trame, K, s) exige le même condensé de
  catalogue sur chaque paire GPU / jumeau moteur. C'est un condensé
  FNV-64 de la vue canonique de C, pas une égalité littérale du catalogue.
  Elle exige aussi le même travail des certificats et de la couverture
  quand les leviers `q34_dead_lanes` et `q34_dead_core` sont égaux ;
- `GPU_executed` signifie qu'au moins une tour LiDAR achevée a eu une phase
  q3/q4 sur l'appareil (filtre S2, ou certificats S3 avec au moins une
  arête décidée sur l'appareil). Ce n'est pas une tour calculée entièrement
  sur GPU ;
- le plan par défaut R13 (18 cas) :
  - chaque (trame, K) sur le chemin GPU complet puis sur son jumeau moteur ;
  - deux bras d'attribution à 08/000000, K5 et K10 : filtre GPU seul, lots
    CPU ;
  - 00/K5 à 24 fils (GPU) et à 1 fil (moteur).

### Tour allégée et sonde v19 (23 septembre 2026, soir)

- **Rangs de plateau** (`aa29245f`) : après le tri exact `by_level`, chaque
  boule reçoit l'indice de son plateau de niveau exact. La phase 0 et la
  phase A comparent ces indices au lieu de produits U320 par facette. Un lot
  singleton ne construit son action que si elle est publiée.
- **Brouillon plat** (`f93dc165`) : `FullCoverageFlatDraft` (niveaux par lot,
  plages CSR de parents et de contributions par action) sur le chemin
  statique. Le constructeur du certificat a un corps générique pour les deux
  formes ; l'API `span<const FullCoverageBatch>` est un adaptateur. Le chemin
  séquentiel garde la forme vectorielle et sert de témoin (même charge
  utile).
- **Sonde et protocole v19** : `filter_kernel_ms`, `filter_transfer_ms`,
  `certificate_kernel_ms`, `certificate_transfer_ms`. Le lecteur G4 borne
  noyau + transferts par le temps d'appareil et exige des zéros sur CPU. Le
  plan R14 comprend les six jumeaux et des paires S2 / S2 + S3 répétées et
  entrelacées.

### Voie q3 par lots sans atlas (S4a, sonde v20, 23 septembre 2026, nuit)

Après les certificats (S3), un appel génère la voie q3 de chaque survivant
certifié dont la voie q3 reste ouverte (`asked`), **sans atlas**. L'objet
émis est celui du moteur ; les compteurs forment un registre déclaré du mode
(`lanes_*`), jamais comparé au registre q3 du moteur.

- **En-tête portable** `src/gpu/lanes.hpp` (groupe de 32 voies par arête,
  `HostGroup` sur l'hôte, `WarpGroup` sur l'appareil) :
  - le cover est reconstruit par `build_cover` de S3 ;
  - ses sites suivent un **ordre de balayage fixe** : huit anneaux de
    |2z−a−b|² dans [0, 4D], puis le rang à l'intérieur d'un anneau (stable).
    Toute boule q3 possédée contient la boule de rayon |ab|/(2√3) autour du
    milieu (R ≤ |ab|/√3 pour un triangle aigu de plus grand côté ab), donc
    un recensement rejeté s'arrête tôt. `MHGP9_LANES_SCAN_RINGS` (mesure
    seulement) règle le nombre d'anneaux ; 1 donne l'ordre de rang ;
  - les graines sont tirées du cover par le prédicat du moteur (aigu strict,
    ab possédée, ex æquo par la paire d'IDs triée). Une graine possédée
    vérifie |2x−a−b|² ≤ 3D, donc l'**ensemble** des graines est celui du
    moteur (seul leur ordre change) ;
  - un recensement exact par graine, réparti par site sur les voies : la
    puissance relative non réduite G|z−a|²−W·(z−a) de `make_q3` (< 2^116,
    même signe que `ExactBall::power`), l'arrêt au (K−1)-ième site intérieur
    retrouvé exactement dans le masque de ballot ;
  - pour une graine acceptée : la clé `translated` puis `primitive` par pgcd
    binaire (même diviseur que l'Euclide du moteur), le support trié, la
    profondeur, la taille de coquille et son **empreinte** (somme et xor de
    SplitMix64 des IDs, `q34_shell_hash`).
- **Mise en attente = décision de mémoire seulement** : un cover au-delà de
  l'ardoise (65 536 sites), des boules au-delà de l'ardoise d'enregistrements
  (4 096) ou une réservation au-delà de l'arène (4 par arête + 4 096) rendent
  l'arête au CPU (traîne : `Engine::certified_edge` avec la seule voie q3).
- **Exécution** : `run_lanes_batch_host` (`src/gpu/lanes_host.hpp`, blocs
  parallèles validés dans l'ordre des arêtes, réservation déterministe ;
  toute exception est rendue après jointure de tous les fils) ;
  `run_lanes_batch` (noyau `lanes_kernel`, warps persistants, 120 registres
  sans débordement sur sm_120, arène à réservation atomique par arête,
  jamais un préfixe publié).
- **Chemin par lots** (`run_wspd_q34_batched`, `Q34LanesStage`) : l'appel
  tourne avant les ouvriers sur le CPU, **pendant** eux sur l'appareil (fil
  dédié, joint sur tout chemin). Les ouvriers font les autres voies (q4),
  puis la traîne et le puits d'enregistrements. `both_edges` compte chaque
  arête demandée dont q4 est ouverte, décidée ou en traîne (auditeur A).
- **Frontière de confiance** : `check_lanes_batch` (formes, partition exacte
  des enregistrements entre les arêtes décidées, enregistrements bien formés,
  identités du registre) ; `judge_lanes_filter` recalcule chaque arête
  décidée par la voie q3 du moteur (`engine_q3_records`, recensement
  GlobalBoxes de l'index global : un autre algorithme) et compare
  exactement clé, support, arité, profondeur et taille de coquille. Les IDs
  de coquille ne sont comparés que par leur **empreinte** (contrôle à
  collisions possibles, auditeur C) : la tour ne les consomme pas, elle
  recalcule chaque coquille par recensement et compare sa taille.
- **Chaîne** : leviers `q34_batch_q3` (CPU) et `q34_gpu_q3` (appareil),
  `q34_lanes_judge`, `q34_lanes_capacity`. Les enregistrements deviennent
  directement des présentations (sans `Q34SeedCandidate`).
- **Sonde et protocole v20** : champs `lanes_*` de `q34_batch` et du
  registre, préflight jugé, préflight à ardoise réduite (certificats 64
  sites, voies q3 24 sites), plan R15 (paires S2 + S3 GPU / S2 + S3 + S4a
  GPU répétées et entrelacées à 08/000000).
- **Portes** : `mhgp9_gpu_lanes_port` (trois familles et une fixture
  cosphérique gravée, K2/3/5/10), `mhgp9_chain_batch_q3` (condensés et
  registre des voies égaux avec et sans le levier, à ardoise réduite),
  contrat sonde/lecteur (15 mutants q3).
- **Brouillon plat public** : la surcharge `FullCoverageFlatDraft` de
  `build_full_coverage_certificate` valide chaque CSR avant sa première
  lecture (auditeur B, débordement reproduit sous ASan) :
  `coverage_flat_draft_shape`.

### Voie q4 par lots sans atlas (S4b, sonde v21, 24 septembre 2026)

Le même appel génère aussi la voie q4 des survivants certifiés dont la voie
q4 reste ouverte, **sans atlas ni fenêtre** ([conception](s4b_conception_20260924/README.md),
lemmes L1–L8 au [registre des preuves](../../docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md),
section V9-S4). L'objet émis est celui de la voie Local28 du moteur.

- **En-tête portable** `src/gpu/q4_lanes.hpp`, appelé par `edge_lanes`
  après le prologue et les recensements q3 de S4a, avec le même groupe de
  32 voies par arête et les mêmes graines. Pour chaque graine possédée
  strictement aiguë :
  - famille $P-\mu S$ et domaine L2 : grille entière symétrique de huit
    seaux ;
  - passe de lentilles dans l'ordre des anneaux, contrôlée après chaque
    paquet de 32 sites ;
  - tampon des événements des seaux vivants, puis filtre des candidats
    (positif, possédé, canonique) ;
  - groupes de racines par formes pivots (numérateur de `make_q4`) ;
  - profondeur L4, racines aux bornes L6, émission du plus petit ID positif
    L7.

  Aucun flottant.
- **Mise en attente = décision de mémoire seulement** : tampon d'événements
  (4 096 par groupe), ardoise d'enregistrements, arène (16 par arête + 4 096,
  bornée au huitième de la mémoire libre sur l'appareil). La traîne CPU passe
  par `Engine::certified_edge` avec les voies demandées.
- **Registre déclaré** `lanes4_*` (28 compteurs), jamais comparé aux
  registres q4 du moteur. `list_steps` et `group_steps` comptent toutes les
  passes du stade des survivants : listes de seau, étrangers, recherche du
  plus petit ID, localisation, comparaison, remise à zéro, positivité et son
  minimum, choix. Les identités (graines, groupes, émissions) sont vérifiées
  par `check_lanes_batch` et le lecteur. Les coûts eux-mêmes sont des
  **mesures non jugées** : `pass_site_tests`, `bucket_events`,
  `compare_steps`, `list_steps`, `group_steps`.
- **Coût borné par la mémoire** : un seau de m événements coûte au plus
  6⌈m/32⌉ passes par groupe, donc O(m²/32) par graine dans le pire cas (la
  famille u18 de l'auditeur à seaux vivants). Au-delà du tampon, l'arête est
  reportée. Sur 08/000000/K10 : `max_buffered` 1 183, `max_group` 2.
- **Frontière de confiance** : `check_lanes_batch` accepte les arités 3 et
  4. `judge_lanes_filter` recalcule chaque arête décidée par
  `engine_q3_records` et `engine_q4_records`. Comme pour q3, les IDs de
  coquille ne sont comparés que par leur empreinte. La tour ne les consomme
  pas : le recensement de la chaîne recalcule chaque coquille à partir de la
  clé et refuse une taille différente (`chain_census_shell_mismatch`).
- **Chaîne** : levier `q34_batch_q4` (exige `q34_batch_q3`) et option
  `q34_lanes_events`. Sous `catalogue_digest`, la chaîne publie aussi
  `presentation_digest`, un condensé sans ordre du multiensemble (clé,
  arité, support) de toutes les présentations. Son temps s'ajoute à celui du
  condensé du catalogue dans `times_ms.catalogue_digest` : c'est la somme des
  deux, retirée de la fusion et du total. Elle peut être non nulle sur un
  refus postérieur à la fusion. Le lecteur compare ce condensé et les
  comptes de présentations entre bras jumeaux (contrelecture S4b : un
  échange de présentations qui préserverait les boules ne passe plus).
- **Sonde et protocole v21** : levier et registre `lanes4_*`, borne des
  arêtes demandées sur les arêtes q3 ou q4 ouvertes, exemption du plancher
  d'atlas sous `q34_batch_q4`, plan R16 (paires S4a / S4a + S4b répétées et
  entrelacées à 08/000000).
- **Portes** :
  - `mhgp9_gpu_lanes_port` : q4 arête par arête contre `engine_q4_records`
    sur trois familles ; fixture cosphérique à groupes de 75 sites ; fixture
    multi-groupes de l'auditeur B ; fixture d'une racine sur une borne
    intérieure de la grille ($\mu=6=g_6$, candidat étranger du seau 5,
    décidé au seau 6) ; lot mixte q3 + q4 passé à la frontière et au juge ;
    tampon réduit ; sept mutants q4 ; graine survivante sans événement.
    Son mode `--compare` publie les exclusions (certificats reportés, arêtes
    sans voie, voies reportées) et exige au moins un plancher
    (`--min-q3-records`, `--min-q4-seeds`, `--min-q4-records`). Avec
    `--all-asked`, il refuse une exclusion. Il vérifie aussi les identités
    du registre q4, et des CTests gravent ses codes 0, 2 et 3 ;
  - `mhgp9_chain_batch_q3` : bras S4b jugé et bras à ardoise et tampon
    réduits ;
  - contrat sonde/lecteur v21 (dix mutants q4, dont les trois bornes de
    passes).

### Tâches (arête, plage de graines) des voies q3/q4 (S4b, étape 2 du plan des voies, 24 septembre 2026)

Étape 2 du plan du juge des voies ([conception](tour_voies_conception_20260924/README.md),
« T2 simplifié ») : l'appel des voies ne donne plus une arête entière à un
warp. Il découpe le travail en **tâches (arête, plage de graines)**, pour
qu'une arête lourde ne fasse plus la traîne du noyau (R16 : l'arête la plus
lourde à 0,87 de l'appel). L'objet, les enregistrements, les statuts et le
registre ne changent pas.

- **En-tête portable** `src/gpu/lanes_tasks.hpp`, trois étapes exécutées par
  les mêmes fonctions sur l'appareil (`WarpGroup`) et par le jumeau hôte
  (`HostGroup`) :
  - **P, une par arête** : validité des voies demandées, cover
    (`lanes_cover`), ordre de balayage et graines (`lanes_order`), écrits à la
    place de l'arête dans une **arène de covers** ; nombre de tâches
    `lanes_task_count(sites, graines, B)`, fonction de l'entrée seule ;
  - **T, une par tâche** : pour chaque graine de la plage, le recensement q3
    (`q3_census_range`) puis la graine q4 (`q4_seed`, T1 inchangé). Les
    enregistrements vont dans l'ardoise du groupe, puis dans **une**
    réservation d'une arène de préparation. La tâche garde ses comptes par
    phase et sa première défaillance (phase, genre). Son travail s'ajoute à
    l'emplacement de son arête (sommes et maxima entiers : l'ordre des tâches
    ne compte pas) ;
  - **C, une par arête** : `lanes_replay` rejoue la préséance séquentielle
    d'`edge_lanes` (prologue ; graines q3 dans l'ordre ; puis graines q4 dans
    l'ordre ; ardoise d'enregistrements dépassée sur le **total** de l'arête).
    Suivent la règle de l'arène finale dans l'ordre des arêtes (compteur
    toujours avancé : préfixe exclusif des comptes), le rassemblement dans
    l'ordre (arête, phase, tâche) — tous les q3 de l'arête, puis tous ses
    q4 —, et le registre réduit sur les seules arêtes décidées.
- **Pourquoi l'objet ne change pas** : chaque graine d'`edge_lanes` est
  exécutée une fois, par les mêmes fonctions, sur le même cover dans le même
  ordre de balayage. Les graines sont indépendantes : rien ne passe d'une
  graine à l'autre, sauf l'ardoise d'enregistrements et les compteurs.
  L'ordre de sortie et les mises en attente sont reconstruits à partir des
  comptes par tâche, jamais de l'ordre d'achèvement. La sortie de
  l'appareil devient canonique : l'arène suivait jusqu'ici l'ordre des
  atomiques, et sa mise en attente sur débordement n'était pas reproductible.
  Elle est désormais **égale octet pour octet** à celle du jumeau.
- **Découpage B** : une tâche regroupe les graines consécutives d'une arête
  dont la borne graines × ⌈sites/32⌉ (paquets de 32 sites des passes) ne
  dépasse pas B, avec au moins une graine. **B = 512 par défaut** : les
  paquets de passe de lentilles d'une tâche restent sous environ 0,5 ms d'un
  warp seul sur G4 (2 473 cycles par paquet dans le calage « plafond 752 »
  du juge, 2,4 GHz). B = 1 donne une graine par tâche, B = ∞ une tâche par
  arête (l'étape 1). `MHGP9_LANES_TASK_BUDGET` (builds de mesure
  seulement) change le défaut. Aucune valeur de B ne change une sortie
  décidée : la porte le vérifie pour B ∈ {1, 512, ∞}.
- **Mesures locales** (compteurs déterministes, jumeau hôte, 08/000000) :
  - K5 : 2 500 659 tâches pour 597 250 arêtes à graines (708 686 arêtes).
    Travail déclaré de la plus lourde tâche (`lanes_task_steps` : paquets
    de recensement, de passe, de filtre, de listes et de classes) :
    **7 114 pas**, contre 86 847 pour une tâche par arête ;
  - K10 : 9 581 649 tâches pour 1 337 836 arêtes à graines (1 463 362
    arêtes) ; plus lourde tâche **27 057 pas**, contre 290 184.

  Les deux lignes `lanes_tasks_compare` (mode `--compare`) donnent aussi
  `identical=1` : sur toutes les arêtes demandées, le jumeau à tâches rend
  la même sortie, octet pour octet, que le chemin à une tâche par arête.
  Sur le chemin CPU de la chaîne (jumeau, W6, hôte chargé, deux paires
  entrelacées), l'appel des voies à K5 passe de 10,9 s à 6,9 s, avec les
  mêmes condensés. C'est une indication, pas un reçu.

  La durée sur l'appareil n'est **pas** mesurée ici. Le modèle du juge
  (hors dépôt, `tasks.cpp` / `bsel`, coûts de 000000 avec T1) donne pour la
  tâche la plus lourde environ 1,7 ms à K5 et 5,9 ms à K10 à B = 512 : une
  graine survivante coûte jusqu'à 1 à 2 ms à elle seule.
- **Mémoire** :
  - **arène de covers à 20 o par site** : coordonnées 12, rang 4, position
    de graine 4, les graines à la même place que les sites. Les fonctions
    portables lisent `slab.points` comme avant. La variante à 4 o par site
    (rangs seuls, points relus dans `rank_points`) ajouterait une lecture
    dépendante dans les boucles chaudes (recensement, passe, filtre,
    comparaisons) sans mesure pour la justifier. Volumes : 91 M sites à K5
    (1,8 Go), 322 M à K10 (6,4 Go), au plus 379 M mesurés (08/000200 K10,
    7,6 Go). Capacité par défaut : un sixième de la mémoire **totale** de
    l'appareil (816 M sites sur G4), réservée pendant q2 par
    `warm_up_lanes` ;
  - **ardoises par warp** : marche de P (plages et rangs, 12 o par site, soit
    768 Kio) ; enregistrements et événements de T (560 Kio). Avant : 2,6 Mio
    par warp ;
  - **par arête** environ 470 o (plan, registre du prologue, emplacement des
    tâches, balayages, réponse) ; **par tâche** 32 o ;
  - **arène de préparation** : deux fois l'arène finale par défaut, bornée au
    huitième de la mémoire libre.

  Tous les buffers restent résidents et ne font que grandir
  (`LanesResident`). Le dimensionnement ne dépend que de l'entrée et de la
  mémoire (totale, ou libre plus résidente), jamais de l'historique.
- **Refus** : l'arène de covers, la table des tâches et l'arène de
  préparation se décident sur des totaux qui ne dépendent pas de l'ordre
  (sites réservés, tâches, enregistrements préparés). Leur dépassement est un
  **refus de capacité explicite de l'appel** (`BatchError::capacity`, lu
  entre les étapes), jamais un préfixe ni une mise en attente dépendant du
  temps. La mise en attente reste une décision de mémoire par arête : cover,
  événements, enregistrements, arène finale dans l'ordre des arêtes.
- **Jumeau hôte** (`run_lanes_tasks_host`, `run_lanes_batch_host`) : les
  trois étapes par fenêtres de 16 384 arêtes (seuls les covers d'une fenêtre
  vivent à la fois), fils coordonnés par barrières. Une exception ouvre
  toutes les barrières et est rendue après jointure. La sortie ne dépend ni
  du nombre de fils, ni de la fenêtre, ni de l'ordre des tâches.
- **Appareil** (`filter_runner.cu`) : noyaux `lanes_plan_kernel`,
  `lanes_fill_kernel`, `lanes_task_kernel`, `lanes_replay_kernel` et
  `lanes_gather_kernel`, deux balayages CUB, deux lectures de compteurs par
  l'hôte (après P, après T). Registres ptxas (sm_120, CUDA 12.9) :
  - T : 128 registres, 128 o de débordement en écriture et 100 en lecture ;
    l'ancien `lanes_kernel` en avait 224 et 124. Le travail q3 d'une tâche
    est versé à son arête dès la fin de la phase q3 ;
  - P : 96 registres, sans débordement ;
  - rassemblement 72, remplissage 36, réplique 24 registres, sans débordement.

  Sous-chronos de `LanesOutput` : `plan_ms` (P et son balayage), `task_ms`
  (lecture des compteurs, table, T), `compact_ms` (C et son balayage).
  Compteurs : `tasks`, `max_task_steps`.
- **API inchangée** : `LanesInput` gagne `task_budget`, `cover_capacity` et
  `staging_capacity`, tous à 0 par défaut. La chaîne et le registre ne
  changent pas.
- **Sonde et protocole v23** : `q34_batch` publie `lanes_tasks` et
  `lanes_max_task_steps` sur les deux dorsales (fonctions de l'entrée et de
  B, égales entre le jumeau et l'appareil), et `lanes_plan_ms`,
  `lanes_task_ms` et `lanes_compact_ms` sur l'appareil seulement (nuls sur
  le CPU). Le lecteur exige :
  - P et C non nuls quand l'appareil a tourné, et P + T + C dans le noyau ;
  - une tâche porte au moins un pas déclaré, et aucune tâche sans arête
    demandée ;
  - sans arête reportée : au plus une tâche par graine, des tâches dès
    qu'il y a des graines, et la plus lourde tâche bornée par la somme des
    pas du registre.

  Mutants : cinq dans l'autotest du protocole (appareil), trois dans le
  contrat sonde/lecteur (CPU), un sur le chemin moteur.
- **Portes** :
  - `mhgp9_gpu_lanes_port`, section 7. Sur chaque famille et chaque K, et
    sur les quatre fixtures (multi-groupes B, borne intérieure, tampon vide,
    arène de l'auditeur), la sortie entière des tâches est égale octet pour
    octet à celle du chemin à une tâche par arête (`single_task_batch`, un
    exécuteur indépendant de `lanes_tasks.hpp`). Variantes : B ∈ {1, 512, ∞},
    1, 3 et 4 fils, fenêtres de 97 et 16 384 arêtes. Identités : B = ∞ donne
    une tâche par arête à graines, B = 1 une tâche par graine. Avec cover,
    enregistrements, événements ou arène réduits, les arêtes en attente sont
    celles du chemin à une tâche, avec un plancher > 0 par genre. Les arènes
    exactement pleines sont acceptées ; une case de moins refuse l'appel.
    Neuf répliques gravées couvrent la préséance attente/panne, que les
    arêtes réelles n'exercent jamais, et six découpages ;
  - le mode `--compare` compare aussi, sur toutes les arêtes demandées d'un
    nuage fichier, le jumeau à tâches (B = 512 et B = ∞) au chemin à une
    tâche, octet pour octet (ligne `lanes_tasks_compare`) ;
  - **cinq mutants** compilés, chacun tué (code 1) par le contrôle qui le
    vise : dernière plage partielle oubliée (`split.count`) ; placement par
    ordre d'achèvement, émulé sur l'hôte par des tâches exécutées à rebours
    (`tasks.bytes`) ; segments q3 et q4 permutés (`tasks.bytes`) ; compteurs
    d'une arête en attente ajoutés (`tasks.small_bytes`) ; préséance
    attente/panne inversée (`replay.`).
- **Non vérifié localement** (aucun GPU dans le conteneur) : l'exécution des
  noyaux, l'égalité octet pour octet appareil/jumeau, les chemins de refus
  sur l'appareil et toute durée. Une session G4 doit montrer les
  sous-chronos P/T/C et le noyau K5 face à la projection de 68 à 76 ms, et
  comparer les condensés des bras jumeaux.

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
