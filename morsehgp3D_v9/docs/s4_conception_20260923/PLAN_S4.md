
# S4 : plan par étapes pour générer sur GPU les voies q3/q4 des arêtes vivantes

```text
phase=exploration_v9_hors_registre
backend=reference_cpu (+ cuda_g4 à qualifier)
profile=quantized_u18_input_only
mode=plan_de_conception
public_status=not_claimed
```

**GCP non utilisé.** Tout a été fait en lecture seule sur `build/v9-open-worktree` (HEAD `a6d08f05f`) : aucun build, aucun ctest, aucune mesure.

- **Mesurés** : les chiffres de R13, notamment `receipts/g4_tower_r13_20260923/SUMMARY.json` et `vm/probe_{0,2}.stdout`.
- **Indicatives** : les parts CPU, qui viennent de `receipts/q34_survivor_phases_20260923` (ticks TSC sur un hôte chargé).
- **Projetés** : tous les temps d'appareil ; il leur faut un reçu G4.

Abréviations : `P` = `src/gen/pipeline/wspd_q34.cpp`, `L/` = `src/gen/lanes/`, `TC` = `src/chain/tower_chain.cpp`.

## 0. Décision en bref

1. **S4.0** — une session d'appareil qui garde tout résident : index, `order[]`, survivants, masques S3, et plages du cover **exportées par S3**. C'est la base de tout le reste ; le gain propre est faible.
2. **S4a** — la voie q3 de **toutes** les arêtes sur l'appareil, **sans atlas**, avec un recensement exact sur le cover résident.
   - Une voie de warp traite une graine ; les 32 voies lisent le même site du cover au même moment (lecture diffusée).
   - Le CPU calcule l'atlas et q4 **en même temps**.
3. **S4b** — la voie q4 sur l'appareil, **sans atlas non plus**. On utilise la fenêtre exacte `Window30` (`L/q4_window.cpp:107–257`) appliquée au cover entier, une voie par graine.
   - Une porte de coût la déclenche ; elle s'appuie sur le débit mesuré en S4a.
4. **S4b′ (repli)** — si S4b échoue à sa porte, on aplatit l'atlas sur CPU puis on porte l'atlas et le balayage LiveOnly (warp par arête), comme dans l'analyse GPU.

**Pourquoi cet ordre.** Sur GPU, l'atlas est la partie irrégulière : récursion, budgets séquentiels, `shared_ptr`. Or sur CPU il n'est qu'une accélération : q3 et q4 ont chacun un chemin exact qui s'en passe, déjà qualifié sur petites fixtures.
- q3 : `WspdQ3CensusMode::ScalarCover`, comparé à GlobalBoxes dans `tests/gen/wspd_q34_gate.cpp:706`.
- q4 : `WspdQ4Backend::Window30`, comparé à Local28 contre l'oracle global, `wspd_q34_gate.cpp:303–307`.

Sur l'appareil, un balayage régulier du cover coûte peu, alors que l'atlas coûte cher. Window30 a été fermé comme remplacement **CPU** sur une **mesure** (`docs/FAUSSES_PISTES.md:80`, médiane ×1,15). Une mesure ne ferme pas définitivement une piste (`FAUSSES_PISTES.md:10–13`), et l'usage est ici un autre backend, requalifié.

**Verdict honnête.** S4 amènerait K5 d'environ 2,20 s à environ 1,55–1,65 s. En y ajoutant S3 et les recouvrements, on arrive vers 1,3–1,4 s. **Avec la tour actuelle, 1 s n'est pas atteignable** (§8).

## 1. Budget R13 (08/000000)

| | K5 | K10 |
|---|---:|---:|
| chaîne | 2 202 ms | 7 825 ms |
| q2 / census / tour | 105 / 99 / 745 | 203 / 495 / 3 039 |
| q34 : front / filtre S2 / certificats S3 / **survivants** | 98 / 91 / 211 / **731** | 165 / 146 / 519 / **2 922** |
| arêtes ouvertes (q3 et q4 / q3 seule / q4 seule) | 569 448 / 132 230 / 7 008 | 1 332 053 / 105 368 / 25 941 |
| graines q3 / q4 | 9,32 M / 5,81 M | 33,4 M / 26,4 M |
| sites par cover (moyenne) | 280 | 452 |
| émises q3 / q4 | 691 284 / 158 496 | 2 898 219 / 1 732 548 |

On répartit la phase des survivants d'après les parts de cycles du reçu `q34_survivor_phases`, renormalisées après S3. La reconstruction du cover est estimée à environ 0,79 × « cover : construction ».

| poste | K5 | K10 |
|---|---:|---:|
| atlas | ≈33 % ≈ 240 ms | ≈39 % ≈ 1 140 ms |
| q3 | ≈33 % ≈ 240 ms | ≈20 % ≈ 590 ms |
| q4 | ≈30 % ≈ 220 ms | ≈38 % ≈ 1 110 ms |
| reconstruction du cover | ≈4 % ≈ 27 ms | ≈3 % ≈ 75 ms |

## 2. Patron commun (celui de S3, généralisé)

**Leviers de chaîne, tous coupés par défaut.** Ils vont dans `tower_chain.hpp`, à côté des lignes 56–63 et 104–113. La chaîne refuse les combinaisons incohérentes, comme dans `TC:546–553`.
- `q34_device_session` (S4.0) ;
- `q34_batch_q3` (référence CPU), `q34_gpu_q3` (appareil) ;
- `q34_batch_q4w`, `q34_gpu_q4w` ;
- `q34_lanes_judge`, `q34_lanes_capacity` ;
- en repli : `q34_flat_atlas`, `q34_gpu_atlas_q4`.

**API** dans `wspd_q34.hpp`, à côté de `Q34CertificateFilter` (`:343–367`) :
- `Q34LanesBatch` contient :
  - par survivant : `decided_lanes` (u8 ⊂ masque S3) et `deferred` ;
  - `records` ;
  - le travail `Q34LanesWork` ;
  - `backend`.
- `Q34LanesFilter` ;
- `run_q34_lanes_batch_cpu(index, kmax, edges, masks, lanes, workers)` ;
- `judge_lanes_filter(inner, workers, work*)`, calqué sur `P:1137–1165` ;
- `check_lanes_batch`, calqué sur `P:1048–1085` : identités graines = émises + rejets, somme des tailles de coquille = `shell_ids`, arêtes décidées ⊂ masque.

**Enregistrement émis** (taille fixe, environ 112 o), avec les champs dont la chaîne a besoin (`TC:284`, `:637–645`) :
- arité ;
- 4 IDs de support triés ;
- profondeur ;
- taille de la coquille ;
- **empreinte de coquille** indépendante de l'ordre (somme et xor d'un mélange 64 bits des IDs) ;
- clé 5 × i128 primitive ;
- (ordinal d'arête, ordinal d'émission).

Les IDs de coquille n'existent que dans l'émulation hôte et la référence CPU. Un nouveau puits de chaîne convertit les enregistrements en `Presentation` sans passer par `Q34SeedCandidate`. `gather_presentations` (`TC:311`, `:787`) retrie tout, ce qui rend la sortie déterministe quel que soit le nombre de warps.

**Phase 3** (`P:1432–1460`). Pour l'arête j, le CPU lance `engine.certified_edge(a, b, masks[j] & ~decided_lanes[j])` si ce reste est non vide ; une arête différée par S3 reste sur `surviving_edge`. Pour cela :
- `certified_edge` (`P:491–497`) doit accepter un masque réduit ;
- `generate_lanes` (`P:602–635`) construit déjà l'atlas d'une arête q4 seule dans l'entrée q4 (`L/q4_local.cpp:838–853`), avec les mêmes options, donc le même atlas et les mêmes compteurs q4.

**Recouvrement CPU/GPU.** Le noyau part de manière **asynchrone** juste après S3. Les 48 fils exécutent la phase 3 pendant ce temps :
- ce qui n'est pas porté ;
- les arêtes lourdes **routées d'avance** au CPU, sur `cover_sites × graines` que S3 connaît.

On joint à la fin. Les arêtes différées pour débordement passent ensuite sur le CPU (traîne). C'est la réponse à la piste fermée « petits lots synchrones avec reconstruction hôte » (`FAUSSES_PISTES.md:105`).

**Mise en attente = décision de mémoire seulement**, déclenchée par :
- l'ardoise d'enregistrements de l'arête pleine ;
- le tampon d'ex æquo d'une voie plein ;
- l'arène globale pleine.

La publication est **atomique par arête** : une seule réservation `atomicAdd` en fin d'arête, jamais un préfixe publié.

**Quatre niveaux de juge par étape :**
1. **Qualification d'objet de la référence CPU.** Par arête, sur les survivants réels de 3 trames × K3/5/10, le multiensemble trié (arité, support, clé, profondeur, IDs de coquille) de la référence est égal à celui du moteur produit actuel. On exige aussi les condensés de tour et de catalogue égaux entre levier ON et OFF.
2. **Porte de port hôte** (`tests/gpu/lanes_port_gate.cpp`, calquée sur `certificate_port_gate.cpp`) : l'en-tête portable `src/gpu/lanes.hpp` (`MHGP9_HD`, `HostGroup`), comparé arête par arête et compteur par compteur à la référence CPU. Elle comprend :
   - des mutants causaux ;
   - des planchers ;
   - une ardoise réduite : exactement les arêtes au-delà de la capacité sont mises en attente, les autres sont identiques.
3. **Préflights G4** (`gcp-migration/tower_worker_v9.py:423–431`, `:506–548`) :
   - `--lanes-judge` : chaque arête décidée est recalculée par `run_q34_lanes_batch_cpu`, multiensemble et travail sommés compris ;
   - un préflight à ardoise réduite, qui doit produire au moins une mise en attente (plancher) ;
   - l'égalité `engine_tower_digest == tower_digest`.
4. **Invariants globaux à l'échelle**, sur chaque cas LiDAR :
   - le census de chaîne par clé (`TC:848–849`, profondeur et taille de coquille) ;
   - Euler ;
   - l'égalité des condensés GPU contre le jumeau moteur (`cross_worker_comparisons`).

   Seul le jumeau attrape une boule manquante.

**Arithmétique de l'appareil.**
- i128 seulement pour multiplier et additionner.
- Clés par **pgcd binaire** : pas de `%` i128 (`L/exact_ball.cpp:41–58` utilise Euclide).
- `scaled_floor` évité.
- Un selftest hôte/appareil aux bornes prouvées (< 2^117), sur les fixtures `extreme18` (`wspd_q34_gate.cpp`).
- Les `logic_error` deviennent un statut `fault`, qui aboutit à `kInvariantViolated`, comme en S3 (`TC:242`).

## 3. S4.0 : session d'appareil et plages S3 résidentes

**Aujourd'hui.** Le filtre S2 (`filter_runner.cu:240–245`) et l'appel S3 (`:559–640`) allouent l'index, le téléversent puis le libèrent à chaque appel. Les plages vivent dans l'ardoise du warp (`certificate.hpp:144–153`, `build_cover :234–294`), qui est écrasée à l'arête suivante. Le CPU reconstruit ensuite 708 686 covers (`P:491–497`).

**Ce qui reste résident** : un objet `gpu::DeviceSession`, créé par `GpuPreparation` (`TC:110–143`) et libéré avec la chaîne.

| donnée | taille K5 (K10) |
|---|---|
| nœuds, `escapes`, `rank_points` | ≈ 4 Mo |
| **`order[]` rang → ID (u32), nouveau** : nécessaire pour les départages par ID, l'orientation (a,b) triée par ID (normale de `Q4FamilySeed`, `L/q4_family.cpp:52–77`) et le test canonique `y<x` | 160 Ko |
| survivants S2 (a_rank, b_rank, masque) et statuts et masques S3 | ≈ 25 Mo (55 Mo) |
| **arène des plages** : par arête ouverte (offset, compte, `cover_sites`) ; le leader S3 y recopie `slab.ranges` après décision, avec une réservation atomique par arête | ≤ 8 o × Σ`cover_sites` : borne 1,6 Go (5,6 Go), réel bien moindre. Il faut **imprimer `retained_ranges`** (champ de `CoverWork`, absent de la sonde) |

Une arène pleine ne provoque pas d'attente S3 : l'arête passe simplement au CPU, qui reconstruit son cover.

**Porte.** Pour chaque arête ouverte, les plages de l'arène sont égales à `Q34EdgeCover::make(...)->ranges()`, dans l'émulation hôte et, en préflight, après téléchargement.
- Mutants : une plage sautée ; un offset décalé d'une unité ; les plages non fusionnées.

**Gain attendu.** De quelques ms à environ 15 ms : deux téléversements d'index et un aller-retour des survivants, compris dans les 189 ms de `certificate_device_ms`.

**Arrêt.** Si `certificate_ms` se dégrade de plus de 5 % ou si un condensé diffère.

**Hors S4 mais à noter.** La marche parallèle exacte par frontière dans S3 (analyse GPU §3) garde les mêmes compteurs et vise les 211 ms de S3. C'est un levier séparé, compté au §8.

## 4. S4a : voie q3 de toutes les arêtes, sans atlas

**Restructurations CPU d'abord**, chacune avec sa porte.

- **Ra1. Graines tirées du cover.** Un balayage des rangs du cover avec le prédicat de lentille, l'aigu strict et le propriétaire (`P:747–756`) remplace la marche de l'index (`P:725–790`). La lentille est contenue dans la boule 3D/4, elle-même dans le cover, et l'ordre préfixe des feuilles est l'ordre des rangs : la **séquence** de graines doit être identique.
  - Porte `mhgp9_q3_cover_seeds_gate` : égalité de **séquence** par arête, sur 3 trames × K3/5/10.
  - Fixture gravée d'égalité `ax == D` (départage par `edge_key`).
  - Mutants `≤` au lieu de `<` dans `edge_key`, et aigu non strict.
- **Ra2. Recensement sur la puissance relative non réduite** `G|v|²−W·v`. C'est le polynôme de `make_q3` avant `primitive` (`L/exact_ball.cpp:85–105`, même expression que `Q4FamilySeed::power`, `L/q4_family.cpp:79–86`, < 2^116). La réduction n'a lieu qu'à l'acceptation : on évite 4,79 M `make_q3` pour 691 k émissions.
  - Porte de fixture : signe identique à `ExactBall::power` sur `extreme18` et sur des coquilles cosphériques (cas `== 0`).
  - Mutant : omettre `primitive` à l'émission. La clé change et le condensé doit tomber.
- **Ra3. Recensement de l'objet sans atlas.** On utilise le mode produit **existant** `ScalarCover` (`P:686–708`), avec `q3_atlas_consultation=false`. Il est exact parce que la boule q3 positive possédée est contenue dans le cover (commentaire `P:662–666`).
  - Porte `mhgp9_chain_batch_q3_gate` : pour chaque arête, le multiensemble q3 du réglage (ScalarCover, sans consultation) est égal à celui du réglage produit (GlobalBoxes, atlas, recensement de feuille). La comparaison se fait sur les 3 trames × K3/5/10, avec les condensés ON/OFF.
- **Ra4.** `run_q34_lanes_batch_cpu(lanes=2)` avec des tampons plats réutilisés par fil et des spans de plages, sans `shared_ptr` ni consommateur `std::function`.
  - Registre déclaré : les champs de recensement de ScalarCover (`census_range_visits`, `census_point_tests`, intérieurs, coquille, extérieurs, `depth_rejections`, `early_unread_sites`), plus `cover_seed_sites`.
  - Changements à déclarer : `ball_builds` ne compte plus que les acceptations, et les compteurs `q3_atlas.*` tombent à zéro.
  - `validate_completion` est adapté à ce mode.

**Répartition sur l'appareil.** Un warp persistant par arête ouverte en q3.
1. Les voies balaient les sites du cover, une par site, et appliquent le prédicat de graine. Un `ballot` compacte les graines **dans l'ordre des rangs**, par paquets de 32.
2. La voie i prend la graine i : elle calcule G et W en i128, puis parcourt **tous** les sites du cover en lecture diffusée. Elle compte les intérieurs jusqu'à K−1 (arrêt propre à la voie, compteurs exacts) et accumule la coquille (compte et empreinte).
3. Une graine acceptée produit la clé primitive par pgcd binaire, et l'enregistrement va dans l'ardoise de l'arête.

Il n'y a pas de pile ni de marche d'arbre : on supprime la pile GlobalBoxes de 55 cadres (`L/q3_ball_census.cpp:67–175`) et 112 M bornes i128. **Les 132 k arêtes q3 seules quittent entièrement le CPU**, avec 99,2 % des recensements globaux.

**Volume.** Environ Σ⌈graines/32⌉ × `cover_sites` ≈ 0,2 G pas de warp à K5 et ≈ 0,65 G à K10, à environ 100 instructions entières par puissance.

**Projection.** Noyau K5 de 10 à 40 ms, K10 de 60 à 150 ms. Le calcul est limité par les instructions et non par la latence, contrairement à S3 (1/17 de fil par warp). C'est précisément ce que le reçu S4a doit **mesurer**.

**Gain attendu.** Le CPU ne garde que l'atlas, q4 et la reconstruction des arêtes q4.
- K5 : `edges_ms` passe de 731 à environ 470–500 ms, soit −230 à −260 ms ; chaîne vers 1,96 s.
- K10 : de 2 922 à environ 2 340 ms, soit environ −580 ms.
- Condition : le noyau doit rester **caché** derrière le CPU.

**Arrêt :**
- gain G4 inférieur à 120 ms à K5 ;
- `kernel_ms` q3 supérieur à 150 ms à K5 : le modèle « voie par graine » est faux, et on ne lance pas S4b ;
- tout désaccord de juge ou de condensé : fixture minimale permanente, mise à jour de `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md`, arrêt.

**Mutants de port :**
- K−2 au lieu de K−1 ;
- départage du propriétaire inversé ;
- orientation par rang au lieu de l'ID ;
- empreinte de coquille ignorée ;
- publication avant la fin de l'arête ;
- pgcd faux ;
- ordre de compaction du `ballot` décalé.

**Planchers** : arêtes q3 seules, coquille de plus de 3 sites, mises en attente > 0 sur ardoise réduite, arrêts précoces > 0.

## 5. S4b : voie q4 sur l'appareil, fenêtre exacte sur le cover

**Restructurations CPU d'abord :**
- **Rb1.** `run_q4_window_cover_seed` : la routine par graine de `L/q4_window.cpp:107–257`, avec trois changements :
  - l'ensemble conservé devient **le cover entier en ordre de rang**, à la place de `retained_ids()` en ordre d'ID, sans pelage Shallow29. Le certificat « fort » de `L/q4_shallow_set.hpp` devient vide : aucun site n'est retiré ;
  - les tas sont remplacés par des **tableaux triés de T = K−2** (3 à K5, 8 à K10), une structure définie et portable ;
  - le test d'appartenance binaire est supprimé.
- **Tri par ID obligatoire des groupes d'ex æquo `lower` et `upper` et de la coquille constante avant les présentations.** En Window30 ils héritent de l'ordre des ID. En ordre de rang, la première présentation valide pourrait changer, et avec elle le support émis.
  - Mutant dédié : groupes laissés en ordre de rang.
- Graines : la même liste que Ra1. Le lane map signale que les graines de E3 et E5 coïncident ; la porte Ra1 le vérifie.

**Porte d'objet.** Par arête, le multiensemble q4 de la variante est égal à celui du produit Local28/LiveOnly, sur 3 trames × K3/5/10. Il faut en plus les fixtures des pièges de `morsehgp3D_v8/docs/FAUSSES_PISTES.md:526–545` :
- fenêtre ponctuelle L = U sur huit sites cosphériques ;
- ex æquo d'extrémité sans quota (second passage complet) ;
- contribution fixe des événements hors fenêtre.

Chacun a son mutant. Planchers : `point_windows`, `disjoint_rejected_seeds`, `fixed_depth_rejected_seeds`, `groups_without_support`, `canonical_rejections`, supérieurs à 0. Il faut relire la liste v8 et `docs/audit_v8/15_…` avant le port (`FAUSSES_PISTES.md:16`).

**Porte de coût (décision S4b ou S4b′).** Une sonde CPU avec reçu compte, pour la variante :
- Σ graines × `cover_sites` au premier passage ;
- la part des graines qui passent au second ;
- les insertions ;
- les appels à `compare_roots`.

On multiplie ces comptes par le coût par voie et par site **mesuré en S4a**. On continue avec S4b si la projection reste sous 150 ms à K5 et sous 1 s à K10, sinon on passe à S4b′.

**Répartition sur l'appareil.** On réutilise les paquets de graines de S4a ; le noyau est fusionné quand q3 et q4 sont ouverts.
- La voie i garde en registres sa `Q4FamilySeed` : normale i64, G, W et 6 mineurs i128.
- **Premier passage** sur le cover en lecture diffusée : `side` en i64. Si `side = 0`, un signe de puissance ; sinon une insertion dans les tableaux triés, avec des comparaisons `compare_roots` en i128 (< 2^97).
- Ensuite, rejet si les constantes atteignent T, puis calcul de la fenêtre [L,U] et rejet si elle est disjointe.
- **Second passage**, seulement pour les voies vivantes : classement par rapport à L et U, avec des tampons d'ex æquo par voie de capacité E. Un débordement met l'arête en attente.
- Tri par insertion des au plus 2H−2 événements intérieurs, groupes, puis présentations : `owned`, `make_q4` positif, canonicité. Tout cela est rare (0,027 émission par graine à K5).

**Volume.** Environ 0,17 G pas de warp au premier passage à K5, environ 0,6 G à K10 (T=8, plus d'insertions).

**Projection du noyau fusionné q3+q4.** K5 de 50 à 120 ms, K10 de 150 à 400 ms. La traîne des arêtes lourdes reste sur le CPU pendant l'appel.

**Gain attendu.** Le CPU n'a plus d'atlas ni de q4.
- K5 : `edges_ms` passe d'environ 490 à environ 60–150 ms ; chaîne vers **1,55–1,65 s**.
- K10 : de 2,34 s à environ 0,3–0,8 s.

**Arrêt :**
- la porte de coût échoue ;
- le noyau K5 mesuré dépasse 200 ms ;
- tout désaccord d'objet : fixture minimale permanente et mise à jour du registre des preuves avant de continuer.

## 6. S4b′ (repli) : l'atlas aplati puis porté

**Rc1. Atlas plat sur CPU.**
- Les cellules deviennent un tableau d'enregistrements fixes (cellule, état, enfants, compte, décalage et longueur de frontière).
- Les frontières vont dans une arène d'IDs de nœuds u32 en pile : l'enfant ajoute, le retour dépile.
- On remplace `Q4LocalAtlas::Impl` et `build` (`L/q4_local.cpp:131–272`) et le constructeur `Q4LocalFragment` (`L/q4_local_partition.cpp:364–433`), en gardant **exactement** l'ordre des tests :
  - le budget `z_test_budget=512` et le préfixe séquentiel (`:392–395`) ;
  - `node_budget` ;
  - `stop_after` (K−1).

**Porte.** Tous les compteurs logiques de `Q4LocalAtlasWork`, `Q4LocalPartitionWork`, `Q4LocalSaturationWork` et `Q4LocalSweepWork` sont égaux arête par arête, avec le multiensemble q4.
- Exemptions listées : `*_bytes`, `peak_*`, `retained_*`, `sort_comparisons` et `shell_sort_comparisons`.
- Un reçu CPU propre est exigé, car l'allocation, estimée à 20–30 par arête, peut rapporter seule.

**Port** (analyse GPU §3). Un warp par arête :
- DFS uniforme avec au plus 8 cadres ;
- frontière classée en parallèle sur les voies. Le budget est reproduit exactement : chaque voie marche son sous-arbre en comptant ses tests, un préfixe de warp situe le budget, et la voie qui le franchit remarche en tronquant ;
- graines LiveOnly tirées du cover ;
- balayage `sweep` (`:374–468`) avec un warp par graine et un tri par rang (ordre (racine, ID) total).

**Coût.** C'est l'étape la plus risquée : registres, longueur des arènes. Mêmes gains visés que S4b.

## 7. Risques

- **Mémoire.**
  - L'arène des plages est inconnue tant que `retained_ranges` n'est pas imprimé (borne 1,6 et 5,6 Go).
  - Les enregistrements font environ 95 Mo à K5 et environ 520 Mo à K10.
  - Le RSS hôte grossit de la même quantité jusqu'à `gather_presentations`.
- **Volume de sortie à K10.** Environ 4,6 M enregistrements. Il faut un **anneau de tampons épinglés** et une conversion vers `Presentation` par les fils pendant le noyau. Leçon de v6 C6 : le fil hôte pesait 88 % de l'étage.
- **Divergence.**
  - L'arrêt K−1 est propre à chaque voie : le coût est le maximum sur les voies.
  - Il n'y a que 12 à 20 graines par arête : 40 à 60 % des voies travaillent.
  - Le second passage q4 est partiel ; les émissions sont rares mais divergentes (`make_q4`, pgcd).
  - La queue de `cover_sites` est routée d'avance vers le CPU.
- **Registres.** L'état i128 de la famille et, à K10, deux tableaux de 8 sites dépassent 128 registres. Il faut des routines i128 `__noinline__`, `ptxas -v`, et deux noyaux (q3, q4) si le noyau fusionné déborde.
- **Doctrine.** La réouverture de Window30 ne concerne que l'appareil. Le défaut CPU reste Local28 et la fermeture n'est pas contestée. Tout désaccord devient une fixture minimale permanente.
- **Compteurs redéfinis** (graines issues du cover, `ball_builds`, `q3_atlas.*`) : à déclarer dans le registre du mode, jamais à comparer au registre du moteur.
- **Juge coûteux.** `--lanes-judge` recalcule tout sur CPU : préflights seulement, jamais sur les cas mesurés.

## 8. Estimation honnête : K5 < 1 s ?

Cas 08/000000, chaîne 2,202 s. Temps en s ; toutes les lignes après « R13 » sont **projetées**.

| scénario | chaîne K5 |
|---|---:|
| R13 | 2,202 |
| + S4a | ≈ 1,96 |
| + S4b (ou S4b′) | ≈ 1,55–1,65 |
| + marches S3 par frontière et session unique (certificats 211 → 80–120 ms) | ≈ 1,40–1,50 |
| + recouvrement q2 (CPU) ↔ S2–S4 (appareil), levier de chaîne séparé | ≈ 1,30–1,40 |
| + étape 1 de la tour D5 (−5 à −10 ms à K5, `PLAN_ETAPE1_SAUT.md:259–263`) | ≈ 1,29–1,39 |
| **plancher avec la tour actuelle** : tour 0,745 + census 0,099 + front 0,098 + reste 0,069 + appareil ≥ ≈ 0,15 | **≈ 1,16** |

**Conclusion.** S4 est le plus gros levier unique (−0,55 à −0,65 s à K5). Il est nécessaire, mais **pas suffisant**.

Passer sous 1 s exige en plus une tour de 0,45 à 0,5 s au plus, et la tour ne s'y prête pas en l'état :
- la phase A mono-fil de `lots_by_k[5]` (378,5 ms) est le plancher de la fenêtre chevauchée (`docs/d5_conception_20260923/CARTE_TOUR.md:247–254`) ;
- s'y ajoutent 262 ms hors fenêtre : validate, populations, images, bank, encode ;
- le plafond de l'étape 1 D5 est de −104 ms.

Sur la plus petite trame (08/000100 : chaîne 1,738 s, survivants 541 ms, tour 584 ms), l'ensemble donne environ 1,05–1,15 s : proche, mais au-dessus. **Il faut donc un chantier de tour (phase A parallèle) en même temps que S4.** Sinon le contrat K5 < 1 s reste hors d'atteinte sur les trames de 40 à 46 k sites.

## 9. Séquence de livraison

1. **S4.0** : session, `order[]`, arène des plages, porte des plages, impression de `retained_ranges`, puis reçu G4 (jumeaux ON/OFF, 3 trames × K5/K10 × 2 répétitions, condensés égaux).
2. **Ra1 à Ra4** (CPU, reçus locaux), puis `lanes.hpp` pour q3, la porte hôte, `judge_lanes_filter`, les préflights, puis le reçu G4 de S4a.
3. **Rb1**, sa porte d'objet et sa sonde de coût, puis la décision S4b ou S4b′, puis le port, les juges et le reçu G4.
4. Tout désaccord arrête la séquence : fixture, registre des preuves, et la passation dans le même commit.

Fichiers principaux lus (tous sous `/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/`) :
- `src/gen/pipeline/wspd_q34.cpp`, `src/gen/pipeline/wspd_q34.hpp`, `src/chain/tower_chain.cpp`, `src/chain/tower_chain.hpp` ;
- `src/gpu/certificate.hpp`, `src/gpu/filter_runner.cu`, `src/gpu/filter_runner.hpp` ;
- `src/gen/lanes/q4_local.cpp`, `src/gen/lanes/q4_local_partition.cpp`, `src/gen/lanes/q4_window.cpp`, `src/gen/lanes/q4_family.cpp`, `src/gen/lanes/exact_ball.cpp` ;
- `receipts/g4_tower_r13_20260923/`, `receipts/q34_survivor_phases_20260923/README.md` ;
- `docs/FAUSSES_PISTES.md`, `docs/d5_conception_20260923/PLAN_ETAPE1_SAUT.md`, `docs/d5_conception_20260923/CARTE_TOUR.md` ;
- hors de ce dossier : `/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v8/docs/FAUSSES_PISTES.md` et `/workspaces/E-HGP/build/v9-open-worktree/gcp-migration/tower_worker_v9.py`.
