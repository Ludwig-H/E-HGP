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
