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

### Parcours et chargement par blocs du certificat (S3, 24 septembre 2026)

L'appel des certificats coûte 145 ms à K5 sur G4 (noyau 117 ms, R18), soit
environ 12 % de la chaîne. **La sortie ne change pas** : masques, statuts,
mises en attente et registre, champ par champ. Aucun champ de sonde nouveau.

**Mesure d'abord** (jumeau hôte, groupe compteur, survivantes de 08/000000
dans l'ordre de l'appareil ; totaux égaux au registre des reçus G4) :
- pas de warp à K5 (2 043 612 arêtes) : parcours d'arbre 540,6 M visites
  (70 %), cellules 93,3 M, votes de frontière 77,4 M, passes de chargement
  59,0 M ; à K10 (4 507 278 arêtes) : 1 394 M visites sur 1 996 M ;
- par arête : médiane 294 pas, p99 1 412, maximum 5 142, pour environ
  256 000 pas par warp (3 008 warps) : **la traîne est négligeable** ;
- modèle à débit plafonné du juge des voies, recalé sur les six couples
  (trame, K) de R15–R18 (médianes des noyaux) : erreur RMS 1,0 % (plafond
  752) ou 0,8 % (sans plafond). Part du parcours 64 à 74 %, part de la traîne
  0,1 à 1,9 %. Un coût unique par pas de warp s'ajuste encore à 1,3 % : le
  noyau suit le nombre de pas de warp, et les coûts par catégorie restent
  mal séparés (mélanges presque proportionnels d'une trame à l'autre).

Conclusion : le débit domine, et le parcours en est l'essentiel. Découper
une arête en tâches ne gagnerait rien. Il faut réduire le parcours.

**Parcours par blocs** (`build_cover_chunked`, `src/gpu/certificate.hpp`).
Le parcours sans pile visite les nœuds dans l'ordre préfixe : un nœud
scindé continue à son fils gauche, le nœud suivant (garde
`validate_certificate_input` ; `flatten_escapes` sur l'hôte), un nœud admis
ou rejeté à son lien d'échappement.
- Les 32 voies décident les 32 nœuds consécutifs [base, base + 32), un par
  voie, avec exactement la décision entière de `build_cover`. C'est une
  fonction pure du nœud et de la boule, et la garde couvre tout l'index :
  décider un nœud que le parcours sautera n'écrit rien.
- Sept votes publient le mot de décision (scindé, admis, décalage du
  successeur borné à 31).
- Le parcours séquentiel est ensuite **rejoué** sur ces bits : mêmes nœuds
  visités, dans le même ordre, mêmes compteurs, mêmes plages et même arrêt de
  capacité. Une suite de nœuds scindés est prise d'un coup. Un successeur
  hors du bloc ouvre le bloc suivant.

`build_cover` reste le témoin (et le parcours des voies, `gpu/lanes.hpp`,
inchangé).

**Chargement par fenêtres** (`load_forms_chunked`). Une passe du groupe
pour 32 sites, au lieu d'une passe par plage (10 sites en moyenne). La case
s reçoit le s-ième site des plages dans l'ordre, comme dans `load_forms`,
qui reste le témoin.

**Norme partagée**. La borne inférieure exacte de la norme sur une cellule,
et la norme en un centre, sont calculées une fois pour les deux disques (q3
et q4) au lieu d'une fois par voie. Le second membre des disques est
précalculé. Mêmes valeurs i128, mêmes comparaisons.

**Compteurs de l'hôte, avant → après** (08/000000, mode fichier de la
porte ; toutes les arêtes identiques aux deux témoins et au prouveur
produit ; [reçu local](../receipts/s3_certificate_chunks_local_20260924/README.md)) :

| K | arêtes | pas de parcours (1 nœud par pas) | blocs de décision | pas de rejeu | passes de chargement |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 5 | 2 043 612 | 540 624 193 | 93 621 341 | 410 957 802 | 58 955 540 → 20 830 439 |
| 10 | 4 507 278 | 1 394 025 439 | 228 671 376 | 1 059 220 070 | 161 704 731 → 59 128 851 |

Les cellules et les votes de frontière ne changent pas. Un pas de rejeu
(une suite de nœuds scindés, ou un nœud admis ou rejeté) ne calcule aucune
décision et ne suit aucun lien d'échappement dans le bloc ; il relit (L1)
les bornes de rang d'un nœud non scindé.

**ptxas** (sm_120, CUDA 12.9, noyau `certificate_kernel`) : 128 → 124
registres, pile 384 → 368 o, aucun débordement. Les autres noyaux sont
inchangés.

**Portes** (`mhgp9_gpu_certificate_port`, `tests/gpu/certificate_port_gate.cpp`) :
- sur chaque arête (trois familles, K2, K3, K5 et K10), pour la boule du
  cœur **et** celle du cover, à pleine capacité et à capacité 64 :
  - parcours par blocs contre `build_cover` : même réponse, même travail
    (aussi en débordement), mêmes plages octet pour octet ;
  - chargement par fenêtres contre `load_forms` : même prouveur et mêmes
    trois tableaux de formes, octet pour octet ;
  - `certify_edge` des deux chemins : même statut, même masque, même
    travail ;
- le prouveur produit reste le juge du chemin par blocs ;
- planchers : blocs, blocs à plusieurs visites, formes, parcours en attente,
  et chaque sortie de bloc (fils gauche et échappement dans le bloc, fils
  gauche au-delà de la dernière voie, échappement au-delà du bloc, fin de
  l'index) ;
- mutants du comparateur : travail, plage et forme faux d'une unité ;
- **quatre mutants compilés du produit**, chacun tué (code 1) :
  - successeur hors bloc pris pour le début du bloc suivant (`walk.`) ;
  - fils gauche de la dernière voie sauté (`walk.`) ;
  - fenêtre de chargement qui recommence sa première plage (`load.`) ;
  - norme partagée comparée au facteur q3 pour la voie q4 (`port.`) ;
- **mode fichier** `--file=… --k=K [--workers=N] [--min-edges=N]
  [--device]` : toutes les survivantes d'un nuage, les deux parcours, les
  deux chargements et le prouveur produit, arête par arête. Porte
  `mhgp9_gpu_certificate_port_lidar_k5` sur la trame épinglée à K5 (label
  `lidar`, hors campagne `gate`) : 2 043 612 arêtes identiques. Avec
  `--device`, l'appel de l'appareil est comparé au jumeau octet pour octet
  (statuts, masques, travail sommé). Sans appareil, c'est un refus
  explicite (code 2, porte `mhgp9_gpu_certificate_port_device_absent`).
  K10 a été passé à la main : 4 507 278 arêtes identiques (8 min sur 4 fils).

La chaîne CPU n'exécute pas `certificate.hpp` (sa référence est le
prouveur produit) : ses condensés épinglés ne peuvent pas changer
localement. Ils sont inchangés à 08/000000 : tour, catalogue et
présentations `67450c64611075b1`, `5ad1fe09354411ba`, `a2aa4b20ca392dfe` à
K5 et `ac108f7f71096c3f`, `a6e959d227f3dafa`, `43ff64fb1c3846d9` à K10.

**Non vérifié localement** (aucun GPU) : l'exécution du noyau, l'égalité
appareil/jumeau et toute durée. Une session G4 doit :
- lancer `mhgp9_gpu_certificate_port_gate --file=<08/000000> --k=5
  --workers=8 --min-edges=2000000 --device`, puis la même chose à K10 ;
- garder le juge (`--certificate-judge`) et les six condensés épinglés ;
- lire `certificate_kernel_ms` face à la **projection** ci-dessous.

**Projection, non mesurée** : 52 à 71 ms de noyau à K5 (mesuré : 113 à
117 ms) et 132 à 181 ms à K10 (mesuré : 286 à 300 ms). Elle applique aux
deux calages du modèle, avec les compteurs ci-dessus :
- un bloc de décision coûtant 1 à 1,5 visite ;
- un pas de rejeu coûtant 0,2 à 0,35 visite ;
- une fenêtre de chargement coûtant 1 à 1,5 passe ;
- des cellules réduites de 0 à 15 %.

Ces coûts sont des hypothèses. Le trafic L2 des blocs (32 nœuds de 40 o
chargés par bloc, contre un nœud par visite) n'est pas modélisé : seule une
session G4 dira s'il pèse.

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
    1, 3, 4 et 8 fils, fenêtres de 97 et 16 384 arêtes. Identités : B = ∞ donne
    une tâche par arête à graines, B = 1 une tâche par graine. Avec cover,
    enregistrements, événements ou arène réduits, les arêtes en attente sont
    celles du chemin à une tâche, avec un plancher > 0 par genre. Les arènes
    exactement pleines sont acceptées ; une case de moins refuse l'appel.
    Neuf répliques gravées couvrent la préséance attente/panne, que les
    arêtes réelles n'exercent jamais, et six découpages ;
  - le mode `--compare` compare aussi, sur toutes les arêtes demandées d'un
    nuage fichier, le jumeau à tâches (B = 512 et B = ∞) au chemin à une
    tâche, octet pour octet (ligne `lanes_tasks_compare`). Avec `--device`,
    il compare de même l'appel de l'**appareil** et publie ses sous-chronos
    (ligne `lanes_device_compare`). Sans appareil, c'est un refus explicite
    (code 2, porte `mhgp9_gpu_lanes_port_device_absent`), jamais un vert par
    vacuité ;
  - **cinq mutants** compilés, chacun tué (code 1) par le contrôle qui le
    vise : dernière plage partielle oubliée (`split.count`) ; placement par
    ordre d'achèvement, émulé sur l'hôte par des tâches exécutées à rebours
    (`tasks.bytes`) ; segments q3 et q4 permutés (`tasks.bytes`) ; compteurs
    d'une arête en attente ajoutés (`tasks.small_bytes`) ; préséance
    attente/panne inversée (`replay.`).
- **Non vérifié localement** (aucun GPU dans le conteneur) : l'exécution des
  noyaux, l'égalité octet pour octet appareil/jumeau, les chemins de refus
  sur l'appareil et toute durée. Une session G4 doit :
  - lancer `mhgp9_gpu_lanes_port_gate --file=<08/000000> --k=5 --compare
    --all-asked --min-q4-records=1 --device`, puis la même chose à K10
    (égalité octet pour octet appareil/jumeau) ;
  - montrer les sous-chronos P/T/C (sonde v23) et le noyau K5 face à la
    projection de 68 à 76 ms ;
  - comparer les condensés des bras jumeaux.

### q2 pendant les appels de l'appareil (levier `q2_during_device`, sonde v24, 24 septembre 2026)

Sur le chemin par lots, le CPU attend pendant les appels de l'appareil
(filtre, certificats, voies : environ 380 ms à 08/000000/K5 dans R18). q2
ne dépend que de l'index : sous le levier, il part sur un fil à part dès que
le front q34 est construit (crochet `after_front` de
`run_wspd_q34_batched`), et il est joint après q34 sur tous les chemins.
- **Ardoises** : les présentations de q2 vont dans des ardoises à part,
  ajoutées à celles de q34 avant la fusion, qui trie tout. Même objet.
- **Échecs** : un échec de q2 est rapporté avant un échec de q34, comme dans
  l'ordre séquentiel. Si le crochet n'est jamais appelé, q2 tourne après
  q34.
- **Temps** : `times_ms.q2` est le mur propre de q2, depuis son lancement.
  `times_ms.q2_wait` est l'attente du fil principal après q34, soit
  `max(0, fin de q2 − fin de q34)` : elle ne dépasse jamais `q2` par
  construction. Le lecteur somme les étapes avec `q2_wait` au lieu de `q2`
  sous le levier.
- **Préparation de l'appareil** : `q34_batch.gpu_prepare_ms` (son mur) et
  `gpu_prepare_wait_ms` (l'attente du premier appel qui la rejoint). Sans
  q2 avant le front, elle n'est plus cachée que par le front ; R19 mesure
  si elle retarde le filtre.
- **Portes** : bras q2 recouvert dans `mhgp9_chain_batch_q3` (mêmes
  condensés, mêmes comptes q2) et refus sans `q34_batch_filter` ; cas réel
  `q2_overlap_on` du contrat sonde/lecteur (même résultat logique que le cas
  séquentiel) ; mutants de l'autotest (attente au-delà de q2, q2 compté deux
  fois, préparation absente ou attente au-delà).
- **Plan R19** : paires q2 séquentiel / q2 recouvert, répétées et
  entrelacées à 08/000000, à K5 et à K10.
- **Préparation en deux étapes** (après R19, qui mesure une attente du
  filtre jusqu'à 102 ms) : le fil de préparation part à l'entrée de la
  chaîne. L'étape A ouvre le contexte, puis aplatit l'index dès que la
  chaîne le fournit ; le filtre et les certificats n'attendent qu'elle.
  L'étape B réserve en arrière-plan les ardoises résidentes des voies.
  `gpu_prepare_ms` est le mur de l'étape A. La revue d'avant R20 a trouvé
  un accès après libération : l'objet est déclaré avant l'index de la
  chaîne et n'en gardait qu'un pointeur, si bien qu'un refus du générateur
  pendant l'étape A libérait l'index sous le fil. La préparation partage
  désormais la propriété de l'index, et une chaîne déjà terminée n'entre
  pas dans l'étape A. La porte `mhgp9_chain_batch_q3` ajoute ce refus
  (cœur mort sans son certificat, levier GPU du filtre, q2 recouvert) ; elle
  passe sous ASan.
### Étape 3 du plan des voies (24 septembre 2026, après R18)

Plan du juge des voies, « Étape 3 — réduire le travail de masse », précédé
de la correction de l'étape C mesurée par R18. L'objet, les enregistrements
(octets et ordre), les statuts et les mises en attente ne changent pas ;
seuls des compteurs déclarés changent, et ils sont nommés ci-dessous.

#### Étape C sans boucle sur les tâches d'une arête

**Diagnostic** (R18, 08/000000) : C coûte 32,2 ms à K5 contre 2,6 ms à
000100 et 20,2 ms à 000200 ; 80,9 ms à K10 (000000), 7,0 et 41,0 ms sur les
deux autres trames. Un harnais hôte (hors dépôt) rejoue le front dans
l'ordre d'appel de G4 (48 ouvriers × 64 jobs) et retrouve **exactement** les
tâches de R18 (2 500 659, 1 083 881, 3 016 003 à K5 ; 9 581 649, 3 514 578,
10 212 503 à K10). Il publie, par arête, sites, graines, tâches et
enregistrements. Deux boucles séries sur les tâches d'une arête faisaient
la traîne de C :
- `lanes_replay_kernel`, **un fil par arête**, parcourait phases × tâches ;
- `lanes_gather_kernel`, **un warp par arête**, parcourait de même toutes
  ses tâches, une lecture dépendante par tâche, même vide.

Une arête de 18 630 sites a 583 paquets de 32 sites, plus que B = 512 : une
graine par tâche, soit **10 473 tâches** pour ses 10 473 graines. Les plus
longues chaînes séries (étapes réplique / rassemblement) valent :

| trame, K | tâches max. d'une arête | réplique | rassemblement | C mesuré (R18) |
| --- | ---: | ---: | ---: | ---: |
| 000000, K5 | 10 473 | 18 830 | 19 376 | 32,2 ms |
| 000100, K5 | 934 | 1 510 | 2 004 | 2,6 ms |
| 000200, K5 | 4 235 | 8 470 | 8 895 | 20,2 ms |
| 000000, K10 | 14 865 | 20 946 | 22 839 | 80,9 ms |
| 000100, K10 | 964 | 1 830 | 4 078 | 7,0 ms |
| 000200, K10 | 4 235 | 8 470 | 11 324 | 41,0 ms |

(rassemblement : 9 024 warps à pas fixe, comme le noyau.) À K5, C suit la
somme des deux chaînes à 0,7–1,2 µs par pas ; à K10 s'y ajoute le débit
(23 M pas de rassemblement au total). Les arêtes de tête n'ont **aucun**
enregistrement : ce n'est que du parcours.

**Correction** (`lanes_tasks.hpp`, `filter_runner.cu`, `lanes_host.hpp`) :
- T publie, par tâche, ses enregistrements par phase (`LanesTaskRecords`)
  et, en cas de défaillance, sa position séquentielle
  `lanes_fail_position` = (phase, indice dans la table) dans l'emplacement
  de son arête, par minimum atomique (`fail_first`, sans dépendance à
  l'ordre d'achèvement) ;
- un balayage CUB inclusif, en place, des enregistrements des tâches ;
- C1, **un fil par arête, O(1)** : `lanes_replay_scan`. Le rejeu
  séquentiel rend « en attente » au premier élément dont le préfixe dépasse
  l'ardoise, s'il précède ou est la première défaillance $i_f$, sinon le
  genre de $i_f$. Le préfixe ne décroît pas, donc cela équivaut à
  préfixe$(i_f)$ > ardoise : deux lectures du balayage suffisent ;
- balayage exclusif des comptes (règle de l'arène, inchangée) ;
- C2, un warp par arête : réponse et registre (inchangés, sans copie) ;
- C3, **un warp par 32 tâches consécutives** : chaque tâche copie ses
  enregistrements à leur place (`lanes_task_destinations` : tous les q3 de
  l'arête dans l'ordre des tâches, puis tous ses q4), en parties de 16 o
  réparties sur les voies ; le champ `edge` est posé au passage ;
- plus de lecture hôte entre T et C : le débordement de l'arène de
  préparation est lu par les noyaux sur le compteur de l'appareil (même
  règle : toute arête non en panne en attente, sans enregistrement ni
  registre).

Le jumeau hôte exécute les mêmes fonctions (balayage, `lanes_replay_scan`,
`lanes_edge_kept`, `lanes_task_destinations`), le rassemblement par tâche.
`lanes_replay` reste le **témoin séquentiel**.

**Portes** : `mhgp9_gpu_lanes_port` (toutes sections, sortie octet pour
octet contre le chemin à une tâche par arête) ; les neuf répliques gravées
jugent désormais le produit et le témoin, placées derrière une tâche d'une
autre arête (base du balayage soustraite), plus **20 000 tables tirées**
(1 à 6 tâches, défaillances et capacités aléatoires, planchers de pannes et
d'attentes > 0) où produit et témoin doivent coïncider. Les cinq mutants de
l'étape 2 restent tués : préséance inversée dans `lanes_replay_scan`,
segments permutés et ordre d'achèvement dans `lanes_task_destinations`.
Trame épinglée, `--compare --all-asked` : `equal=1` et `identical=1` à K5
et K10. ptxas (sm_120, CUDA 12.9) : T 128 registres, 112 o de débordement en
écriture et 84 en lecture (128 et 100 avant) ; C1 46, C2 40, C3 40
registres, sans débordement.

**Non mesuré** : la durée de C sur G4. Projection (non un reçu) : trois
noyaux sans chaîne série et trois balayages CUB, de l'ordre de 1 ms à K5.

#### L11 : élagage exact du cover par le disque des centres

Lemme L11 du registre des preuves (V9-S4, `proved_here`) : avec
$w=2z-a-b$, $\nu=D-\lvert w\rvert^2$ et $\delta=D\lvert w\rvert^2-(w\cdot v)^2$,
un site tel que $\nu<0$ et $\nu^2>2\delta$ est strictement extérieur à
toute sphère passant par $a$ et $b$ de centre dans le disque des centres
$\Delta_4$. Il n'est donc ni intérieur ni sur une coquille des boules q3 des
graines, ni lentille d'aucun seau (chaque seau a son bout intérieur dans
$\Delta_4$), ni graine, ni membre d'une classe émise.

- **Code** (`lanes.hpp`) : `lanes_order` calcule une fois par site sa
  classe de balayage (`lanes_scan_class`, rangée dans `slab.ranges`, libre
  après le remplissage des rangs), retire les sites élagués de l'ordre de
  balayage, et rend le nombre de sites gardés : les graines, les
  recensements, les passes et le découpage en tâches ne voient plus qu'eux.
  Le test est entier (`lanes_axial_terms` en i64, $\nu^2$ et $2\delta$ en
  i128 ; bornes gravées par `static_assert` sur le domaine u18). L'arène de
  covers réserve toujours le cover entier (même règle de refus).
  `lanes_seed_code` isole le prédicat de graine, inchangé.
- **Registre** : `seed_tests` reste la taille du cover (chaque site est
  classé une fois : test L11, puis test de graine s'il est gardé) ; le
  nombre de sites élagués est tenu dans `Q3Work::pruned_sites`, publié par
  la sonde au protocole v25. Changent : recensements, passes, événements
  tamponnés (un site gardé peut lire un paquet plus tôt), tâches.
- **Mesures hôte** (08/000000, compteurs déterministes) :

  | compteur | K5 avant | K5 après | K10 avant | K10 après |
  | --- | ---: | ---: | ---: | ---: |
  | sites élagués / cover | 0 / 91 079 913 | 40 914 459 (44,9 %) | 0 / 322 415 174 | 143 777 889 (44,6 %) |
  | points des recensements q3 | 145 783 580 | 118 736 176 | 980 035 984 | 785 560 284 |
  | paquets de passe q4 | 17 312 783 | 15 065 569 | 106 636 691 | 87 941 750 |
  | certifiées au premier paquet | 5 338 976 | 5 372 666 | 20 885 337 | 21 035 206 |
  | événements tamponnés | 9 066 964 | 9 417 751 | 78 105 353 | 80 314 568 |
  | tâches | 2 500 659 | 2 009 427 | 9 581 649 | 7 306 047 |
  | plus lourde tâche (pas) | 7 114 | 7 104 | 27 057 | 27 034 |

  Graines, certifications, survivantes, classes et émissions inchangées.
- **Portes** :
  - `mhgp9_gpu_lanes_port` : sur chaque famille et chaque K, aucun site
    élagué n'est une graine possédée (sinon code 3, `cause=prune.seed`),
    avec un plancher de sites élagués > 0 ; toutes les autres sections
    inchangées ;
  - fixture minimale `tests/gpu/fixtures/prune_annulus.u32le` (quatre
    points, K3) trouvée par recherche hors dépôt avec une variante qui
    n'élaguait que les sites **non graines** situés entre le disque
    $16\lvert c-m\rvert^2\leq D$ et $\Delta_4$ : un tel site décide d'une
    boule q3. Le produit l'égale au moteur ;
  - mutant `MHGP9_LANES_MUTANT_PRUNE_SMALL_DISK` (disque
    $16\lvert c-m\rvert^2\leq D$) tué sur les familles
    (`edge.seeds_or_emitted`) et sur la fixture (`compare.q3`) ;
  - build `MHGP9_LANES_PRUNE=0` (porte `mhgp9_gpu_lanes_port_unpruned`,
    K3 et K5) : même objet, vert ;
  - trame épinglée, `--compare --all-asked` : `equal=1`, `identical=1` à K5
    et K10 ; condensés de la chaîne épinglés à K5 et K10.
- **ptxas** : P passe de 96 à 80 registres (la classe est calculée une fois
  par site, et non plus à chaque vote), sans débordement ; T inchangé.

#### L10 : ordre de balayage axial en 26 classes entières

Les anneaux ne regardaient que $\lvert w\rvert^2$ et ignoraient la distance
à l'axe $ab$. L'ordre axial range les sites gardés par
$t=\nu/\sqrt{\delta}$, fonction croissante de la part de $\Delta_4$ dont les
sphères contiennent le site : toute pour $t\geq\sqrt{2}$, aucune sous
$-\sqrt{2}$ (sites élagués par L11). Classe
$k=12-\max\lbrace i\in[-13,12] : t\geq i/8\rbrace$, décidée exactement
(`lanes_axial_at_least` : $64\nu^2$ contre $i^2\delta$ selon les signes de
$\nu$ et $i$, bornes gravées), par bissection en cinq pas ; rangs croissants
dans une classe. Les classes tiennent sur cinq bits de vote ; le sixième
porte l'élagage. `MHGP9_LANES_SCAN_AXIAL=0` rend les anneaux.

- **Objet inchangé** (lemme L10 du registre) : tout ordre fixé des sites
  gardés donne les mêmes boules ; seuls les compteurs déclarés changent (et,
  au plus, une mise en attente de mémoire du tampon d'événements).
- **Mesures hôte** (08/000000, après L11 → après L10) :

  | compteur | K5 | K10 |
  | --- | ---: | ---: |
  | points des recensements q3 | 118 736 176 → 89 758 300 | 785 560 284 → 620 739 849 |
  | paquets de passe q4 | 15 065 569 → 11 680 121 | 87 941 750 → 68 118 302 |
  | certifiées au premier paquet | 5 372 666 → 5 719 526 | 21 035 206 → 23 227 114 |
  | événements tamponnés | 9 417 751 → 9 129 566 | 80 314 568 → 74 919 196 |
  | pas de listes / de classes | 4 979 772 / 1 400 340 → 4 958 219 / 1 395 718 | 34 309 433 / 15 947 469 → 33 927 024 / 15 804 487 |
  | plus lourde tâche (pas) | 7 104 → 5 810 | 27 034 → 19 999 |

  Depuis l'étape 2 (avant L11) : paquets de passe −32,5 % à K5 et −36,1 %
  à K10, points de recensement −38,4 % et −36,7 %. Les tâches ne changent
  pas (elles ne dépendent que des sites gardés et des graines).
- **Portes** : build aux anneaux (`mhgp9_gpu_lanes_port_rings`, K3 et K5)
  vert. Un défaut de la seule classe ne change que l'ordre, donc ne peut
  être tué par une porte d'objet ; le mutant vise le tri par comptage élargi
  à cinq bits (`MHGP9_LANES_MUTANT_AXIAL_FOUR_BITS` : la passe de comptage
  relit les classes 16 à 25 comme 0 à 9), tué par `edge.records`. Une
  première version du mutant, appliquée aux deux passes, ne faisait que
  permuter le balayage : elle survivait, conformément au lemme. Trame
  épinglée : `equal=1`, `identical=1` à K5 et K10 ; condensés de la chaîne
  épinglés à K5 et K10.
- **ptxas** : P 112 registres, 104 o de pile (les 26 compteurs de classes),
  sans débordement ; T inchangé.

#### L15 : passe fusionnée q3 + q4 par graine

Pour une arête aux deux voies, une tâche T lisait le cover deux fois par
graine : le recensement de la boule q3 (toutes les graines de la plage),
puis la passe de lentilles q4. `lanes_task_fused` (`lanes_tasks.hpp`) les
fait lire **les mêmes paquets de 32 sites dans le même ordre** ; $P$ est
calculé une fois par site : le vote de lentilles garde par voie le signe de
$P=f_z(0)$ (bit du milieu, $g_4=0$ ; `LaneSigns`), que le recensement relit.
Chacun s'arrête où il s'arrêterait seul (le recensement à son $(K-1)$-ième
site intérieur, la passe à la certification) et l'autre continue. Lemme L15
du registre (`proved_here`).

- **Enregistrements** : q3 en haut de l'ardoise (du dernier au premier), q4
  en bas ; la tâche porte `layout = 1` et `lanes_task_slab_index` les rend
  dans l'ordre séquentiel (q3 de la plage, puis q4) à la copie vers l'arène
  de préparation, sur l'appareil comme sur l'hôte.
- **Préséance** : une panne q3 arrête la tâche sans enregistrement q4 ; une
  défaillance q4 (panne, tampon d'événements) ferme la voie q4 et le
  recensement va au bout de la plage : c'est l'ordre d'`edge_lanes`. Si un
  enregistrement ne tient pas dans l'ardoise, la tâche est **rejouée sans
  fusion** (`fallback`) : la règle d'ardoise des phases séparées décide ;
  sans débordement, aucune de ces règles n'aurait joué.
- **Refonte sans effet** (mêmes enregistrements et même registre, vérifié
  sur la trame épinglée contre L10) : recensement par paquets
  (`q3_census_chunk`, compteurs dérivés en fin de graine par
  `q3_census_count`), graine q4 en `q4_pass_begin` / `q4_pass_chunk` /
  `q4_seed_finish`. Pour contenir les registres de la passe fusionnée :
  - les neuf signes $P-g_kS$ viennent de **quatre produits** (grille
    symétrique : $g_{4\pm i}=\pm t_i$), au lieu de neuf, exactement ;
  - le repère $(d,u,\lvert d\rvert^2,\lvert u\rvert^2)$ de la famille n'est
    plus vivant pendant la passe : l'étage des survivantes le recalcule
    (`Q4Frame`).
- **Compteurs** (`LanesOutput::fused`, sur toutes les tâches de l'appel,
  sommes sans ordre) : graines fusionnées, paquets lus, paquets lus par le
  seul recensement après l'arrêt de la passe, paquets consommés par le
  recensement (ce qu'il aurait lu seul), tâches rejouées. Le registre
  déclaré des deux voies reste celui des phases séparées. Publiés par la
  sonde au protocole v25.
- **Mesures hôte** (08/000000, après L10) :

  | | K5 | K10 |
  | --- | ---: | ---: |
  | graines fusionnées | 6 635 428 | 29 281 785 |
  | paquets lus par la passe fusionnée | 11 054 684 | 64 573 165 |
  | dont recensement seul | 61 123 | 109 531 |
  | paquets de recensement absorbés | 7 211 245 | 35 357 588 |
  | lectures séparées → fusionnées | 18 265 929 → 11 054 684 | 99 930 753 → 64 573 165 |
  | tâches rejouées sans fusion | 0 | 0 |

  Un paquet de recensement ne calcule que $P$ ; avec les poids SASS de la
  conception (`q3_power` 162, paquet de lentilles 905 par site), le travail
  retiré vaut environ 11 % de celui de la passe de lentilles à K5 et 9 % à
  K10. Les quatre produits retirent en outre cinq produits i128 × i64 par
  site de chaque passe. **Projection**, à mesurer sur G4.
- **Portes** : la sortie entière des tâches (registre compris) est égale
  octet pour octet au chemin à une tâche par arête, qui garde les phases
  **séparées** : c'est la porte différentielle de la fusion, sur toutes les
  familles et les deux trames K5/K10 (`identical=1`). Les tâches sans
  fusion (`LanesInput::fused_pass = false`) rendent les mêmes octets et des
  compteurs de fusion nuls. Planchers : graines fusionnées, paquets de
  recensement seul et replis (ardoise d'enregistrements réduite) > 0.
  Mutants tués : recensement fusionné arrêté à $K-2$
  (`q4.judge_refused_real`), passe de lentilles arrêtée avec le recensement
  (`tasks.bytes`, registre q4).
- **Mesure appariée G4** : `mhgp9_gpu_lanes_port_gate --compare --device`
  avec et sans `--unfused` donne les sous-chronos T de l'appareil avec et
  sans fusion, sur les mêmes arêtes.
- **ptxas** : T reste à 128 registres ; la passe fusionnée porte l'état du
  recensement pendant le vote de lentilles : 256 o de pile, 550 o de
  débordement en écriture et 616 en lecture (112 et 84 avant ; chemin non
  fusionné seul : 96 o de pile, 114 et 132). Ce sont des tailles de code, pas
  un trafic mesuré : si la passe fusionnée ralentit le vote de lentilles sur
  G4 plus que les paquets de recensement retirés ne rapportent, la mesure
  appariée le montrera.

#### Sonde et protocole v25

Commit séparé de l'agent, écrit en v24 ; l'intégrateur l'a fusionné avec
la v24 déjà publiée (recouvrement q2, préparation de l'appareil) sous le nom
v25 : union des champs des deux.
- **Registre** (arêtes décidées) : `lanes_pruned_sites` (L11), dans
  `gen::Q34LanesWork` et le registre de la chaîne. Identités, à la frontière
  (`check_lanes_batch`) et au lecteur : `pruned_sites <= seed_tests` et
  `acute_sites + pruned_sites <= seed_tests` (chaque site du cover est classé
  une fois ; un site élagué n'est jamais testé comme graine) ;
  `seed_tests == cover_sites` inchangée.
- **`q34_batch`** (appel entier, deux dorsales, égaux entre le jumeau et
  l'appareil) : `lanes_fused_seeds`, `lanes_fused_chunks`,
  `lanes_fused_q3_chunks`, `lanes_fused_census_chunks`,
  `lanes_fused_fallbacks` (L15). Le lecteur exige :
  - graines ≤ paquets, paquets du recensement seul et paquets consommés par
    le recensement ≤ paquets, paquets > 0 exactement quand il y a des
    graines fusionnées, replis ≤ tâches ;
  - sans arête reportée : graines fusionnées ≤ graines q4, paquets de la
    passe fusionnée ≤ paquets de passe q4, paquets consommés par le
    recensement ≤ points de recensement ;
  - tout à zéro sans le levier q4, ou sur le chemin moteur.
- **Mutants** : cinq dans l'autotest du protocole (appareil), deux sur le
  chemin moteur, trois dans le contrat sonde/lecteur (CPU) ; planchers du
  contrat : sites élagués > 0 sur le cas q3, graines fusionnées > 0 et aucun
  repli sur le cas q4.
- Porte `mhgp9_gen_wspd_q34` : inventaire des mots du registre (27 pour
  `Q34LanesWork`, 531 pour `WspdQ34Work`).
- **Levier `q34_lanes_fused`** (ajouté à l'intégration, même protocole v25) :
  la chaîne passe `LanesInput::fused_pass` depuis ses options (désactivé par
  défaut, exige `q34_batch_q4`, refus
  `chain_q34_lanes_fused_requires_batch_q4`). Le lecteur exige les compteurs
  de fusion nuls sans lui. La porte `mhgp9_chain_batch_q3` ajoute un bras
  fusionné (mêmes condensés, mêmes enregistrements et même registre déclaré
  que les phases séparées, plancher de graines fusionnées > 0, onze refus) ;
  le contrat sonde/lecteur compare `q4_on` (fusionné) et `q4_unfused`. Le
  plan R20 remplace le bras `gpu_q2seq` de R19 par `gpu_unfused` : paires
  répétées et entrelacées à 00 K5/K10, pour mesurer L15 sur la même VM
  (la passe fusionnée porte plus de débordements de registres dans T).

### Session d'appareil et sonde v26 (24 septembre 2026, après R20)

R20 montre que l'étape A de la préparation (129 à 189 ms) est presque
entièrement la création du contexte CUDA (`cudaFree(0)`), que le filtre
attend encore 11 à 82 ms à K5. Ce coût est payé une fois par processus :
un flux LiDAR à 10 Hz ouvre le contexte au démarrage, pas à chaque trame.
- **API** : `gpu::open_device_session(lanes_capacity, lanes_events)`
  (`src/gpu/filter_runner.hpp`) ouvre le contexte primaire (`warm_up`) puis
  réserve les ardoises résidentes des voies (`warm_up_lanes`). Elle rend les
  deux murs et une erreur vide en cas de succès ; une erreur est laissée aux
  appels de l'appareil, qui la classent. La chaîne ne l'ouvre jamais : sa
  préparation trouve alors le contexte prêt.
- **Sonde v26** : levier `device_session`, qui est un levier de processus et
  non de chaîne. Sous ce levier, la sonde ouvre la session avant l'horloge
  de la chaîne. Elle publie `device_session` (`opened`, `context_ms`,
  `reserve_ms`), hors de `chain_total`. Sans appareil, la chaîne refuse
  ensuite explicitement (`chain_q34_gpu_unavailable`).
- **Lecteur** : le levier exige un levier de l'appareil. Sans lui, la
  section est à zéro et fermée. Un cas complet sous le levier exige une
  session ouverte et un temps de contexte non nul. L'autotest ajoute six
  mutants (session non ouverte, sans temps, réservation sans ouverture,
  section absente, session sur le chemin moteur).
- **Règle du premier cas** : le préflight épingle à ON chaque levier
  qu'active un cas du plan, et non plus tous les leviers. Un levier mesuré
  plus lent peut ainsi rester à OFF dans tout le plan.
- **Plan R21** : L15 (`q34_lanes_fused`) désactivée dans tout le plan ; bras
  GPU avec la session ; bras `gpu_cold` sans elle, en paires répétées et
  entrelacées à 00 K5/K10. Le coût à froid reste publié : c'est la
  différence entre les bras, plus `context_ms`.

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

### Queue de la tour en pipeline (E4, 24 septembre 2026)

Étape E4 du plan de la tour ([conception](tour_voies_conception_20260924/README.md)).
Même objet : condensés épinglés, `tower_work` champ par champ et IDs de
population inchangés. Option du constructeur `pipelined_tail` et de la chaîne
`tower_pipelined_tail`, **active par défaut** ; elle n'agit que sur la voie
statique recouverte (`tower_overlap_static`). Coupée, elle rend la queue
d'avant, gardée comme témoin : `assign_populations`, puis les images, après la
jointure. La sonde ne publie pas encore l'option (protocole du meneur).

**IDs de population par décalage statique.** La construction séquentielle
nomme la population d'une boule à sa première contribution, en parcourant
K = 1..Kmax, puis les lots, les actions et les contributions. Le bloc d'une
boule contribue à l'ordre K si `count_block_at` lui donne un masque de
coquille non vide ou son intérieur, ce qui ne dépend que de la boule et de K :
- une boule régulière ne contribue qu'à K = p + u (son ordre de facettes
  p + u − 1 ne contribue pas) ;
- une coquille étendue contribue aux rangs où sa table laisse un site de
  coquille hors de toute composante stricte locale, parfois à plusieurs
  ordres (le cercle d'un carré contribue à K3 et à K4).

Chaque bloc paraît au plus une fois par ordre, et la phase A publie tout bloc
contributeur. Soit $f(b)$ le premier ordre contributeur de $b$ et $o(K)$ le
nombre de boules telles que $f(b) < K$, calculés en parallèle à la fin de la
validation. L'ID de $b$ vaut alors $n + o(f(b)) + r(b)$, où $r(b)$ est le rang
de $b$ parmi les boules de premier ordre $f(b)$, dans la suite des
contributions de cet ordre. Ce sont les IDs du témoin.

**Pipeline.** Dès la fin de sa phase A, l'ordre K lance un fil auxiliaire
(phase B) qui nomme ses nouvelles boules et construit leurs lignes en
parallèle, aux places $n + o(K) + r$. Les références à une boule de premier
ordre plus bas sont gardées et nommées après la jointure
(`population_deferred_refs`). Pendant ce temps, le fil de l'ordre attend la
phase A de K − 1, puis calcule ses images verticales (phase C, sur les rangs
de plateau depuis E1). B et C touchent des parties disjointes du brouillon
(contributions, parents). Le fil de l'ordre 1 dimensionne une seule fois le
tableau des lignes, à l'ouverture de la fenêtre, pendant la phase 0 ; les
fils auxiliaires l'attendent avant toute écriture.

Variante écartée (`fcf708d2`, annulée par `5394a975`) : dimensionner ce
tableau après la phase 0, une fois son arène de requêtes libérée. Aucun gain
de résidence mesurable (pic de la sonde à K10 : 3,79–4,13 Go pour la base,
3,95–4,14 Go pour les deux variantes ; harnais de la tour seule : 3,87 Go
pour le témoin, 3,76 Go en pipeline). En revanche, toutes les phases B
attendent la fin de la phase 0 : à K10, la part exposée des populations
passait de 31–127 ms à 660–1 082 ms (local, W8).

**Échecs.** Tout ce qui est calculable est calculé : B exige A(K), C exige
A(K) et A(K − 1). Après la jointure, l'échec retenu est celui de la boucle
séquentielle : la phase 0, puis les autres exceptions par K, puis, par K, les
lots, les populations et les images. Un point de panne des populations
(`MHGP9_TESTING`) rejoint ceux des lots et des images, sur toutes les voies.

**Durée de vie (correctif de revue).** Une exception autre qu'une panne (une
allocation refusée par l'arène de la phase 0, un fil qui ne peut être lancé)
déroule la portée de la voie recouverte pendant que des fils d'ordre
tournent encore. Les chronos de fin de phase, le début de fenêtre et les
fermetures `cancel` / `publish` étaient déclarés après la jointure : libérés
avant elle, ils recevaient encore les écritures des fils (écriture après
libération sous ASan, là où `d1d03839` rendait un refus propre
`full_ball_allocation_failed`). Tout objet touché par un fil d'ordre ou son
auxiliaire est désormais déclaré avant la jointure. Porte
`mhgp9_chain_order_failure_unwind` (mode `--unwind` de la porte de
priorité) : points de panne `MHGP9_TESTING` d'allocation en phase 0 (K5,
premier ordre de la phase 0 descendante, puis K3) et de lancement d'un fil
d'ordre (K1, K3), sur les trois voies et avec 1, 4 et 8 fils, soit 28
contrôles. Le statut attendu est un refus de ressource
(`full_ball_allocation_failed`, `full_ball_thread_launch_failed`). Chaque fil
marque une pause de 50 ms après sa phase A, pour écrire après que
l'appelant a quitté sa portée. Planchers : sur les 12 cas recouverts où le
fil de K1 existe, pause prise, naissances comptées après la jointure,
compteurs recouverts et en pipeline ; tour complète sous pause au condensé
de la tour sans pause. Le mutant `TIMERS_AFTER_JOIN` rétablit l'ancien ordre
des déclarations. Il n'est enregistré et tué que sous ASan
(`-DMHGP9_SANITIZE=ON`), par le rapport d'écriture après libération.
Exécution locale : le mutant meurt au premier cas (écriture de 8 octets
après libération dans un fil d'ordre, 19 s) ; la porte corrigée passe sous
ASan (301 s) et en Release (25 s). Condensés épinglés reproduits à K5 et K10
sur 08/000000, `tower_work` égal champ par champ.

**Banque.** Le domaine est strictement croissant. S'il commence à 0 et finit
à n − 1, c'est exactement {0, …, n − 1} : l'appartenance devient p < n, au
lieu d'une recherche dichotomique, que gardent les autres domaines. Même
réponse, même contrôle, même confiance.

**Chronos.** Sur la voie en pipeline, les trois temps suivants
s'additionnent :
- `lots` : la fenêtre jusqu'à la fin de la **dernière** phase A, moins la
  phase 0 ;
- `populations` : la part exposée ensuite, jusqu'à la fin de la dernière
  phase B ;
- `images` : le reste de la fenêtre.

`images_by_k` reste le temps d'images de l'ordre **dans** la phase `images`
(sa part après la dernière phase B), ce qui garde valides les bornes du
lecteur G4. Les étapes propres de chaque ordre, recouvertes, sont dans
`images_own_by_k` et `populations_by_k` (nouveaux, non publiés par la sonde).

**Portes.**
- `mhgp9_tower_full_ball_pipelined_cpu2` et `_cpu4` : toutes les fixtures de
  l'oracle par la voie en pipeline, appariées à la voie temporelle et au
  témoin (mêmes IDs, lignes, références, topologie, travail champ par
  champ). Planchers de la porte : au moins 14 références différées sur au
  moins 12 fixtures, 150 ordres en pipeline (168 mesurés) et plus de 5 000
  contrôles appariés au témoin (7 624 mesurés). Les refus passent aussi par
  cette voie.
- `mhgp9_chain_tower_tail` : chaîne sur 1 500 sites et six carrés plantés,
  à K5, avec 1, 2, 3, 4 et 8 fils, et le témoin à 4 et 8 fils. Même
  condensé, même banque, mêmes références, mêmes champs de `tower_work`.
  Planchers : une étape B chronométrée par ordre K ≥ 2, au moins une
  référence différée par carré.
- `mhgp9_chain_order_failure_priority` : trois voies (classique, recouverte
  témoin, recouverte en pipeline) et onze scénarios, dont cinq avec des
  pannes de populations.
- `mhgp9_tower_population_bank` : un juge linéaire sur des domaines dense,
  décalé, presque dense et creux, aux ids de bord.
- Mutants tués : décalage compté sur les contributions et non à la première
  rencontre (tour et chaîne), références différées jamais renommées,
  domaine dense inclusif. Les deux mutants de priorité restent tués, sur la
  voie en pipeline.

**Mesures locales** (08/000000, W8, trois paires entrelacées par K, base
`d1d03839` contre ce code, même source de sonde ; hôte partagé et très chargé,
charge 17 à 46 sur 8 cœurs : **indicatif**). Les condensés épinglés sont
reproduits et `tower_work` est égal champ par champ dans les 12 exécutions.
Compteurs déterministes : 897 776 lignes à K5, 4 414 230 à K10 ; aucune
référence différée sur cette trame (227 et 444 coquilles étendues, aucune ne
contribue à deux ordres) ; ordres en pipeline = Kmax.

| K | bras | tour (ms) | populations | images | banque | encodage | queue |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 5 | base | 4 813 / 4 629 / 4 811 | 280 / 273 / 211 | 122 / 190 / 187 | 166 / 138 / 147 | 302 / 266 / 371 | 870 / 868 / 915 |
| 5 | E4 | 3 941 / 4 278 / 2 774 | 46 / 48 / 40 | 106 / 112 / 56 | 19 / 11 / 10 | 258 / 150 / 124 | 429 / 320 / 229 |
| 10 | base | 45 512 / 44 146 / 44 611 | 2 645 / 2 808 / 2 790 | 898 / 1 230 / 1 014 | 1 871 / 1 518 / 1 720 | 1 434 / 1 076 / 1 328 | 6 847 / 6 632 / 6 852 |
| 10 | E4 | 35 674 / 38 783 / 38 619 | 127 / 103 / 31 | 245 / 271 / 113 | 129 / 209 / 148 | 1 458 / 1 311 / 1 076 | 1 959 / 1 894 / 1 368 |

La queue (populations + images + banque + encodage) passe, en médiane, de
870 à 320 ms à K5 et de 6 847 à 1 894 ms à K10 ; l'encodage (E5) en est
désormais la plus grosse part. Le mur de la tour baisse en médiane de 4 811
à 3 941 ms à K5 et de 44 611 à 38 619 ms à K10, mais il suit surtout la
phase 0, dont la dispersion sur cet hôte est du même ordre. Harnais de la
tour seule sur le
même catalogue (hors dépôt, banque dense dans les deux bras) : populations
et images exposées de 152–235 à 43–80 ms à K5 et de 1 272–1 784 à 47–53 ms
à K10. Toute durée G4 reste à mesurer.

### Pool persistant de la tour (étape E2 du plan de la tour, 24 septembre 2026)

Option `tower_persistent_pool` de `ChainOptions` (option `persistent_pool`
du constructeur et de `build_full_ball_tower`), **active par défaut**. La
sonde ne la publie pas : son protocole est gelé, l'intégration revient au
responsable du protocole. Même objet : condensés et `tower_work` identiques.

Avant : `parallel_detail::run_threads` créait puis joignait T fils à chaque
appel de `parallel_items`, `parallel_ranges` et `parallel_sort` (trois appels
par tri). À 08/000000 K5, une tour fait 58 appels à plus d'un ouvrier :
validation, phase 0 (dix par ordre), populations, images, banque, encodage.

Après (`src/tower/parallel/pool.hpp`, `TaskPool`) :
- **un pool de W participants** (le fil appelant et W − 1 fils) est créé au
  début de `Builder::run`, avant la validation (`times.pool_ms`), et détruit
  après l'encodage ;
- **seul le fil propriétaire s'en sert** : un `PoolScope` l'installe dans un
  pointeur `thread_local`, et les primitives appelées par ce fil y exécutent
  leur boucle de tirage, l'appelant étant l'ouvrier 0. Tranches, indices
  d'ouvrier et valeur rendue ne changent pas, la sortie est bit-identique ;
- **appels imbriqués** : la part du propriétaire pendant un travail, les fils
  du pool et les fils tiers (coureurs de la phase A recouvrante, qui appellent
  `order_prepare_lean`) gardent leurs propres fils. Aucun travail n'attend
  un pool occupé, donc pas d'interblocage ;
- **fermeture** : un fil du pool entre dans le travail de sa génération par
  compare-and-swap tant qu'il est ouvert. Quand la boucle du propriétaire a
  tout tiré, il ferme le travail et n'attend que les fils entrés. Un fil
  réveillé après la fermeture saute le travail sans en lire les champs. La
  fin d'un travail n'attend donc jamais un fil que l'ordonnanceur n'a pas
  encore servi (un fil créé par appel doit, lui, tourner une fois pour être
  joint) ;
- **environnement flottant** : chaque travail porte celui du propriétaire
  (`fegetenv` / `fesetenv`), comme un fil créé pour l'appel en hériterait.
  Le filtre certifié des niveaux lit le mode d'arrondi ;
- **admission** : les fils attendent que tous existent. Un échec de création
  les annule, les joint et relance `std::system_error` avant tout travail
  (refus `full_ball_thread_launch_failed`, comme avant). Le crochet
  `MHGP9_TESTING` de lancement s'applique à la création du pool ;
- **exceptions** : la première est capturée par les enveloppes, l'arrêt est
  demandé, puis elle est relancée sur le propriétaire après la sortie des
  fils entrés.

Compteurs de `FullBallStats`, hors `tower_work`, pour les portes :
- `pool_threads` et `pool_jobs` ;
- `helper_threads` : fils créés par la voie par appel pendant la
  construction. C'est le delta d'un compteur de processus, exact quand la
  tour est seule, comme dans la chaîne ;
- `runner_threads`.

Portes (`tests/tower/task_pool_gate.cpp`, cible de test avec la chaîne
recompilée) :
- `mhgp9_tower_task_pool_unit` (`--unit`) juge le pool seul :
  - indices : l'appelant est l'ouvrier 0, un fil par indice ;
  - les trois primitives sont servies par le pool avec les mêmes sorties,
    de 2 à 8 participants ;
  - jonction : jamais de retour avant la sortie d'un fil entré ;
  - première exception relancée, pool réutilisable ensuite ;
  - imbrication et fil tiers, sous un chien de garde de 30 s ;
  - environnement flottant (FE_UPWARD, FE_DOWNWARD) ;
  - fil en retard qui saute un travail fermé (crochet de retard) ;
  - échec de lancement du pool et de la voie par appel.
- `mhgp9_tower_task_pool_fixtures` : nuage de trois grappes (1 500 sites),
  K5 et K8. La tour construite sur le même catalogue est identique pool actif
  et coupé : condensé, `tower_work` et 87 autres compteurs déterministes, à
  1, 2, 3, 4 et 8 fils, phase A recouvrante ou non. Elle vérifie aussi
  l'option de chaîne et le refus de ressource à l'échec de lancement, pool
  actif ou coupé. Planchers : au moins 50 travaux du pool par tour,
  W − 1 fils du pool, moins de fils créés qu'avec le pool coupé.
- `mhgp9_tower_task_pool_lidar_k5` (label `lidar`, environ 2 à 4 min en
  local) : sur la trame 08/000000, les condensés épinglés `67450c64611075b1`
  / `5ad1fe09354411ba` et la même identité à 1, 2, 3, 4 et 8 fils. Plancher :
  58 travaux du pool.
- Mutants tués (code 1) :
  - six mutants compilés du pool : pool partiel publié, retour avant la
    jonction, file d'attente sur un pool occupé (interblocage), environnement
    flottant périmé, pool contourné, fil en retard entrant dans un travail
    fermé ;
  - le mutant d'exécution `parallel-admit-partial-launch` de la voie par
    appel, qu'aucune porte v9 ne tuait.
- Sous `MHGP9_TSAN` (lancé avec `setarch -R`, sans quoi TSAN refuse la
  disposition mémoire du noyau), sans rapport :
  - les portes `--unit` et `--fixtures` du pool ;
  - `population_bank`, `parallel_sort`, `full_ball_tower --static-4` ;
  - `chain_static_paths` et `order_failure_priority`.

Fils créés par une tour à 08/000000 (compteurs déterministes, pool actif
contre coupé) :

| K | W | travaux du pool | fils créés, pool actif | fils créés, pool coupé | évités |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 5 | 8 | 58 | 52 | 502 | 450 |
| 5 | 48 | 58 | 269 | 2 643 | 2 374 |
| 10 | 8 | 108 | 97 | 953 | 856 |
| 10 | 48 | 108 | 514 | 5 397 | 4 883 |

Avec le pool restent les W − 1 fils du pool, un coureur par ordre et les fils
de `order_prepare_lean` appelés par les coureurs (au plus W par ordre).

Mesures locales, **indicatives** : hôte de 8 cœurs partagé, charge de 23 à
37 pendant toutes les mesures.
- **Coût par appel** (200 appels de `parallel_items` à éléments vides) :
  - fils par appel : 0,57 ms de CPU à W8 et 3,7 ms à W48, surtout du
    noyau ; 4,5 à 15 ms de mur ;
  - pool : 0,5 µs (W8) à 2,7 µs (W48) de mur.
  - Une première version, où chaque travail attendait l'accusé de tous les
    fils du pool, coûtait encore 5 à 20 ms de mur par appel sous cette
    charge. D'où la fermeture décrite plus haut.
- **Sonde, W8, trois paires entrelacées** (base `d1d038393` contre pool),
  en ms :
  - K5 : tour 5 198 / 4 639 / 5 311 contre 5 207 / 5 234 / 3 142 ;
    validation 981 / 785 / 872 contre 738 / 935 / 615 ; phase 0 2 767 /
    2 698 / 2 919 contre 2 848 / 2 891 / 1 759 ;
  - K10 : tour 45 509 / 44 603 / 42 941 contre 38 221 / 43 168 / 42 214 ;
    validation 6 058 / 4 844 / 3 673 contre 4 195 / 4 389 / 3 608 ;
    phase 0 32 438 / 32 073 / 31 772 contre 27 775 / 31 739 / 30 487.
- **Tour seule** sur le même catalogue, pool actif et coupé entrelacés
  (écart apparié médian) :
  - K5 W8, six paires : phase 0 −162 ms (5 paires sur 6), validation −8 ms ;
  - K10 W8, quatre paires : phase 0 +2,5 s (1 paire sur 4) ;
  - K10 W48, quatre paires : phase 0 −307 ms (3 sur 4).

Le bruit de cet hôte (±15 % d'une tour à l'autre) couvre l'effet attendu :
aucun gain de mur local n'est établi. Le gain sur G4 (48 fils, W − 1
créations et jonctions par appel retirées du chemin du propriétaire) reste
une **projection** jusqu'à une session qui mesure `tower_persistent_pool`
en paires entrelacées (levier à ajouter à la sonde).

### Ordonnancement des jobs du front q2 (sonde v16)

Le recensement q2 découpait lui aussi son front en largeur (16 jobs par fil,
compteur atomique). En local (000000 K5, W8), un seul job durait 1,64 s sur
1,72 s de q2. Levier `q2_jobs_by_mass`, actif par défaut et épinglé : le plan
est préparé par masse décroissante (`WspdQ2Schedule::mass_first`), avec
64 jobs par fil. En local, q2 passe de 1 711 à 1 048 ms, avec les mêmes paires
acceptées et le même catalogue. Porte `wspd_q2_parallel` : chaque exécution
parallèle est rejouée avec le plan par masse, ce qui donne la même sortie et le
même travail mono (au moins 400 exécutions).

### Tour statique : regroupement de la phase 0 sans tri (24 septembre 2026, après R20)

À R20 (G4, 08/000000, K5), le tri parallèle des requêtes de la phase 0 coûtait
28 ms à l'ordre 5 : un tri par échantillonnage qui disperse et trie des
enregistrements `FullBallBatchRequest` entiers de 56 o (clé de 40 o,
consommateur, ordinal). La résolution n'a besoin que de trois choses :
- la partition des requêtes en classes de clé égale ;
- la **première** requête de chaque classe, c'est-à-dire l'ordinal minimal,
  dont le consommateur donne `before` ;
- une cible par classe, recopiée à chaque ordinal de la classe.

**Voie hachée** (`resolve_hashed_order`, défaut). Les requêtes ne bougent
plus. Une table à adressage ouvert de mots de 8 o, `(étiquette << 32) |
(ordinal + 1)`, donne en une passe parallèle la case de classe de chaque
requête. L'étiquette est la moitié haute d'un hachage 64 bits de la clé.
- Une étiquette égale n'est qu'une candidate : la classe est décidée en
  comparant les **clés entières**. Une collision fait sonder plus loin et ne
  fusionne jamais deux clés.
- Le mot garde l'ordinal minimal de sa classe, par comparaison-échange.
- Les graines ne sont plus triées non plus : une seconde table des mêmes mots
  les indexe, et une clé égale trouvée à l'insertion donne le refus du témoin
  (`full_ball_static_duplicate_seed`). La recherche de graine de la classe et
  celles de `static_terminal` passent par cet index, avec comparaison exacte.
- Les classes sont résolues par site minimal croissant (seaux de l'indice de
  site, d'ordre de Morton, au plus 2^16, comptage puis dispersion). En ordre
  de hachage, la résolution coûtait environ 25 % de CPU de plus que dans
  l'ordre des clés du témoin (K5, W1), à cause de la localité des boules
  terminales et des nœuds de l'index.
- Chaque classe est résolue une fois, avec le même `static_terminal`, dont
  les seules entrées sont la clé et le premier consommateur. Son mot devient
  `(cible << 32) | rang de niveau du premier consommateur`.
- Une passe de collecte écrit la cible de chaque requête et vérifie sa
  chronologie (`full_ball_static_request_chronology`, comme le témoin).
- Des pipelines de préchargement couvrent la case de départ de chaque
  requête, puis, pour chaque classe, le mot, la première requête, son
  consommateur et la case de graine.

**Même objet.** Les classes, les premières requêtes et les cibles sont celles
du témoin. Tous les compteurs de travail sont des sommes ou des maxima sur
les classes, donc indépendants de l'ordre :
- `static_requests`, `static_unique`, `static_seeded` ;
- les compteurs de résolution : `anchor_hits`, `key_lookups`, `intruder_*`,
  `interior_ranges`, `same_radius_steps`, `descending_steps`,
  `max_chain_steps`, `static_post_seed_*` et `resolve_work` ;
- les voies de résolution (`static_lanes_used`, `static_workers_created`),
  planifiées sur le même nombre de classes.

Seuls changent l'ordre de résolution des classes et les capacités
échantillonnées `static_peak_*` : il n'y a plus de second tampon de requêtes,
et les tables sont comptées dans `static_peak_group_bytes` et
`static_peak_seed_bytes`. La disposition des tables peut dépendre de
l'ordonnancement, jamais une réponse. Les sous-chronos gardent leur place :
`static_sort_by_k` mesure l'index des graines, `static_groups_by_k` les
classes, `static_resolve_by_k` la résolution et la collecte.

**Témoin et options.** La voie triée reste le témoin :
- option du constructeur `hash_grouping`, défaut `true` ;
- `ChainOptions::tower_hash_grouping`, défaut `true` ;
- le résolveur par lots (`FullBallBatchResolver`) garde le témoin, parce que
  sa vue attend des requêtes triées et uniques ;
- un ordre de 2^31 requêtes ou graines, ou plus, passe par le témoin (moitiés
  de 32 bits des mots), avec le même objet.

La sonde v25 ne publie pas encore ce levier : le protocole l'intégrera.
`FullBallStaticTrace` (portes seulement) enregistre, par ordre, les cibles
statiques et le premier ordinal de la classe de chaque requête.

**Porte** `static_grouping` (`tests/tower/static_grouping_gate.cpp`). Pour
chaque ordre K ≥ 2, elle compare le témoin et la voie hachée **octet pour
octet** : cibles statiques, premières requêtes, puis condensé, statut et tous
les champs de travail de `FullBallStats` (les compteurs de voies à W égal).
- W parcourt 1, 2, 3, 4 et 8, phase A recouvrante ou non ;
- nuages : grappes u18 de 1 500 et 240 sites, grille cosphérique 3 × 3 × 3 ;
- planchers : voie effectivement prise à chaque ordre, classes à plusieurs
  requêtes, graines trouvées, plus d'un ouvrier à W ≥ 2, au moins 8 192
  requêtes à un ordre.

Variante à **hachage faible** (quatre valeurs, cible de test) : des clés
distinctes partagent étiquette et case de départ, et la comparaison exacte
doit les séparer. Les planchers exigent des refus d'étiquette dans les
classes et dans l'index des graines. Trois mutants sont tués (code 1) :
- étiquette prise pour l'égalité des clés des classes (`cause=grouping.firsts`) ;
- étiquette prise pour l'égalité des clés des graines
  (`cause=grouping.targets`) ;
- première requête prise à l'ordinal maximal (`cause=grouping.firsts`).

Sur la trame 08/000000 (label `lidar`), la porte vérifie les condensés épinglés
de la tour et du catalogue (K5 `67450c64611075b1` / `5ad1fe09354411ba`, K10
`ac108f7f71096c3f` / `a6e959d227f3dafa`). Elle compare ensuite le témoin à
8 fils à la voie hachée à 1, 2, 3, 4 et 8 fils.

**Mesures locales** (08/000000, hôte partagé de 8 cœurs, chargé par
d'autres sessions : indicatives seulement). Condensés épinglés reproduits et
`tower_work` identique champ par champ à chaque exécution.
- **Temps CPU par fil**, tour seule sur le catalogue K5, W1, ordre 5 :
  - témoin : tri des requêtes 229–243 ms, tri des graines 43–45 ms,
    résolution 1 681–1 767 ms ;
  - voie hachée : index des graines 18 ms, classes 44–48 ms, dispersion
    6 ms, résolution 1 694–1 717 ms, collecte 11 ms.
- **Recherche de graine** (K5, W1, compteur de cycles) : environ 915 cycles
  par recherche dichotomique, contre environ 440 par l'index (1,03 M
  recherches à l'ordre 5).
- **Somme des temps CPU** de la résolution à W8 : 1 717–1 720 → 1 677–1 681 ms
  à l'ordre 5 ; 38,5–38,8 → 37,9–38,0 s sur les ordres 2 à 10 du catalogue
  K10. S'y ajoutent les classes (0,76–0,78 s) et la collecte (0,18 s).
- **Tour seule, même binaire** (catalogue gardé, W8, paires entrelacées,
  charge 14 environ) : phase 0 à K5 1 514 / 1 640 / 1 678 / 1 517 →
  1 250 / 1 267 / 1 363 / 1 309 ms, tour 2 668 / 2 977 / 2 980 / 2 723 →
  2 503 / 2 422 / 2 502 / 2 383 ms ; temps CPU du processus à K10
  65,3 / 65,3 / 65,8 → 61,4 / 61,0 / 61,4 s.
- **Sonde v25 à W8**, paires entrelacées base `d1d038393` → voie hachée,
  sommes sur K = 2..Kmax, prises quand la charge était retombée (12 à 16) ;
  à K5 la voie hachée passe d'abord dans chaque paire, à K10 la base :

| K | tri + groupes (ms) | phase 0 (ms) | tour (ms) |
| --- | --- | --- | --- |
| 5 | 514 / 530 / 480 → 107 / 126 / 117 | 1 816 / 1 847 / 1 941 → 1 197 / 1 364 / 1 446 | 3 181 / 3 246 / 3 301 → 2 343 / 2 695 / 2 800 |
| 10 | 2 656 / 1 511 / 1 541 → 470 / 244 / 362 | 16 574 / 9 534 / 9 987 → 11 560 / 6 431 / 8 687 | 22 966 / 12 542 / 13 573 → 16 924 / 8 924 / 12 058 |

À l'ordre 5 de K5, le tri et les groupes passent de 193 / 209 / 213 ms à
38 / 50 / 38 ms. Les séries prises sous une charge de 30 à 40 donnent la
même baisse du tri et des groupes (70 à 83 %), mais une phase 0 et une tour
dominées par la dérive de la charge : la validation, dont le code n'a pas
changé, y varie jusqu'à 45 % d'un bras à l'autre. La mesure qui compte est
celle de G4, à 48 fils.
