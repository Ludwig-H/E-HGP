# Audit v8, lentille 14 : verrous B1 à B5 et plan de refonte, état au commit 12294241, confrontés au plan v9

```text
phase=exploration_v9_hors_registre (audit de complétude de l'ouverture v9)
backend=none
profile=quantized_u18_input_only (objet audité : moteur v8 u16, élargi à 18 bits par a74e90f2)
mode=audit_lecture_seule
public_status=not_claimed
GCP non utilisé. Aucune compilation, aucun ctest, aucun script du dépôt exécuté, aucune commande git mutante.
```

Base : worktree détaché `origin/main` **12294241** (v8), la v7 étant figée à
`dc57ffd5`. Tous les chemins sont relatifs à la racine du dépôt. Le plan
confronté est `morsehgp3D_v9/docs/PLAN_V9.md` **publié à 3595725a** (146 lignes,
numéros de ligne de cette version). À 21:30 UTC, le worktree d'ouverture en
porte une version modifiée **non commise** (176 lignes) ; ses ajouts sont
signalés « [non commis] » quand ils touchent une conclusion.

## 1. Résumé

- Les deux documents sont **périmés**. `VERROUS_ARCHITECTURE.md` a été modifié
  pour la dernière fois à `785d0589` (20 septembre, 14:57 UTC), et
  `PLAN_DE_REFONTE.md` à `204b0620` (21 septembre, 20:08 UTC). Aucun des deux
  ne reflète les phases 1-2 (`748ec082` à `5fdda963`), le port 18 bits
  (`a74e90f2`) ni les audits du 22 septembre. Leur dernier état formel reste
  « P0 et B1–B5 ouverts » (`morsehgp3D_v8/docs/VERROUS_ARCHITECTURE.md:440`,
  `morsehgp3D_v8/PASSATION.md:1455`). Le contrat 50k y figure encore
  (`VERROUS_ARCHITECTURE.md:415`, `PLAN_DE_REFONTE.md:667-671`).
- **État réel à 12294241.**
  - **P0** est partiel. Son objet littéral est levé : aucun histogramme
    A×A ou B×B dans le moteur. Mais la clôture n'a jamais été prononcée, et la
    priorité a été abandonnée le 20 septembre.
  - **B1** est partiel pour q2 (constantes divisées, croissance inchangée). Il
    reste ouvert pour q3/q4 : chaque paire relance sa recherche de témoins à
    zéro, soit 1,09 G visites de nœuds à K5 sur la scène 0 sans sol.
  - **B2** est partiel : réductions constantes. L'atlas q4 domine (10,7 G
    bornes de blocs à K10), et les phases 1-2 ne l'ont pas touché.
  - **B3, B4 et B5** sont ouverts et **jamais commencés en v8** :
    `src/forest/`, `src/gpu/` et `src/io/` ne contiennent que `.gitkeep`.
  - La **lacune de preuve** sur la borne de packing de la WSPD reste ouverte.
- **Chantiers § 1 à § 10 du plan de refonte.**
  - Seuls le § 3 (voie q2) et les lignes « générateur » du § 2 sont réalisés
    en grande partie.
  - La « tranche FULL minimale », demandée dès le 13 septembre (§ 10,
    `PLAN_DE_REFONTE.md:727-730`), n'a jamais été construite.
- **Ce que PLAN_V9 reprend.** Le plan couvre B2 (V9-2) et la résidence GPU de
  B5 (V9-4). Il couvre en partie B1 (la cascade) et B3 (le noyau MEB).
- **Ses oublis principaux.**
  - **B4, en entier.** Le calendrier v7 est séquentiel tel qu'il est écrit.
    Il croît plus vite que la géométrie (exposant local 1,243 contre 1,094).
  - La **voie et les métriques du résolveur** B3.
  - Le **différentiel v7/v8** de la tranche V9-1.
  - Les **familles adverses** et le protocole de croissance.
  - Le **protocole de qualification** du contrat.
  - La **sérialisation et la reprise** FULL.
- **Ses contradictions.**
  - « q2 dans le même appel, en configuration mesurée unique » est impossible
    avec les options q2 mesurées : `src/wspd/front.cpp:406-412` les réserve au
    masque 1.
  - La porte de sortie de V9-2 porte sur le seul « poste dominant », alors que
    les verrous exigent un gain net sur le travail total.
  - Le chiffre de 7 % décrit le profil d'avant la phase 1.
  - Le verdict attendu du triangle rectangle n'est pas fixé.
  - Les sondes sont gardées sous `build/`.

## 2. Périmètre, méthode, légende

Lus intégralement :

- `morsehgp3D_v8/docs/VERROUS_ARCHITECTURE.md` (445 lignes) ;
- `morsehgp3D_v8/docs/PLAN_DE_REFONTE.md` (734 lignes) ;
- `morsehgp3D_v9/docs/PLAN_V9.md`, dans sa version publiée et sa version de travail.

Relus pour établir l'état :

- **Documents v8.** `PASSATION.md` (l. 1-237 et 1440-1464),
  `docs/AUDIT_REPRISE_DEVELOPPEUR_20260921.md` (l. 59-234),
  `docs/JOURNAL_DEVELOPPEMENT_20260921.md` (entier),
  `docs/REPRISE_DEVELOPPEMENT_20260920.md` (l. 10-30),
  `docs/P0_POOL_TERMINAL_Q2.md` (l. 1-30 et 120-135),
  `docs/P0_FRONT_REEL.md` (l. 30-45).
- **Audits v8.**
  - `SYNTHESE_PRIORITES_LIDAR_20260922.md` (entier) ;
  - `CERTIFICATS_COLLECTIFS_AVANT_ATLAS_20260922.md` (entier) ;
  - `lidar_rectangles_20260922/README.md` (entier) ;
  - `WSPD_Q2_Q3_Q4.md` (l. 170-200) ;
  - `REGIME_WSPD_20260914.md` (l. 70-100) ;
  - `CONTRATS_ET_MESURES.md` (lignes des chiffres cités) ;
  - `IMPLEMENTATION_PARALLELISATION.md` (§ 5 et 6).
- **Sources v8.**
  - `src/pipeline/wspd_q34.hpp` (l. 1-140) et `src/pipeline/wspd_q34.cpp` (l. 390-530) ;
  - `src/lanes/q34_witness_search.hpp` (commentaires) ;
  - `src/wspd/front.hpp` (options) et `src/wspd/front.cpp` (l. 400-415) ;
  - `src/pipeline/wspd_q2_census.hpp` (l. 136-146) ;
  - `src/lanes/exact_ball.hpp` (l. 15-28) ;
  - quelques greps dans `tests/`.
- **Reçus v8, avec recalcul des champs JSON.**
  - `receipts/ground_baseline_20260921/` : `probe_00`, les `time_*` et le profil gprof ;
  - `receipts/ground_phase1_20260921/` : `only_probe_00`, `only_probe_02`, `probe_08` et `only_time_02` ;
  - `receipts/q2_front_proposals_20260917/README.md:100-110` ;
  - `receipts/q2_front_inheritance_20260917/README.md:67-84` ;
  - `receipts/q2_terminal_pool_20260914/README.md:109-116`.
- **v7.**
  - `morsehgp3D_v7/audits/NOTE_CLAUDE_DECOUPE_TOUR_20260911.md` (l. 10-110 et 170-190) ;
  - `morsehgp3D_v7/src/forest/full_ball_tower.hpp` (l. 105-112, 212-217, 293, 751-760, 919-926 et 1083 ; sha256 `83f1c78e…`) ;
  - greps de `tests/full_gabriel_gate.cpp:661-670` et `tests/full_ball_work_gate.cpp:112-118`.
- **v9.**
  - `README.md`, `PASSATION.md` et `docs/AUDIT_V8_SYNTHESE.md` ;
  - `docs/HERITAGE_V7_V8.md` et `docs/FAUSSES_PISTES.md` ;
  - `docs/audit_v8/02_chaine_q2.md` (l. 1-300), `10_heritage_v7.md` (l. 175-330) et `12_parallelisme_gpu_perf.md` (l. 180-299).

Non lu :

- les notes `P0_*` et `Q3_Q4_*`/`Q4_*` hors des lignes citées ;
- les bruts de campagne (`MEASURES.jsonl`, `record_*`) ;
- les sources privées scellées de la v7 ;
- le canal `audits/COORDINATION_MORSEHGP3D_V8.md` ;
- `audits/VERROUS_MATHEMATIQUES_20260914.md`, qui relève d'une autre lentille (11).

Légende :

- **prouvé** : preuve écrite et fixture ;
- **testé** : porte bornée ;
- **mesuré** : reçu épinglé dans le dépôt ;
- **proposé** : écrit, non implémenté ;
- **manquant** ;
- **non vérifiable** : chiffre sans reçu dans le dépôt.

États d'un verrou ou d'un chantier :

- **levé** : objet atteint et critères de clôture remplis ;
- **partiel** ;
- **abandonné** : priorité retirée par écrit ;
- **ouvert** : rien de probant.

## 3. Tableau de synthèse

| repère | énoncé (source) | état à 12294241 | preuve principale | à garder en v9 |
| --- | --- | --- | --- | --- |
| P0 | supprimer la préparation systématique O(\|A\|²+\|B\|²) des témoins locaux (`PLAN_DE_REFONTE.md:308-322` ; `VERROUS_ARCHITECTURE.md:186-188`) | **partiel** : objet levé, clôture jamais prononcée, priorité abandonnée le 20/09 | Pool terminal en O(KF) (`docs/P0_POOL_TERMINAL_Q2.md:126-129`) ; `PLAN_DE_REFONTE.md:364` « reste ouverte » ; `docs/REPRISE_DEVELOPPEMENT_20260920.md:18-19` | invariant « jamais d'histogramme carré » ; minorants certifiés ; décision de clôture écrite |
| B1 | recherches de témoins recommencées (`VERROUS_ARCHITECTURE.md:206-238`) | **partiel** en q2 ; **ouvert** en q3/q4 et sur les rangées | héritage q2 opt-in `front.hpp:39` ; recherche q3/q4 à zéro par paire `wspd_q34.cpp:485-492` ; 1,09 G visites (reçu phase 1) | théorème H par identifiants ; un juge avant d'étendre l'héritage aux voies q3/q4 |
| B2 | triangles de départ × voisinages q3/q4 (l. 240-275) | **partiel** : constantes, atlas dominant intact | atlas 10,7 G bornes, 24,2 G tests, 18,3 G IDs à K10 scène 0 ; mêmes valeurs avant et après la phase 1 | atlas LiveOnly, certificat d'atlas q3, cover partagé, compteurs |
| B3 | MEB et intrus répétés (l. 277-309) | **ouvert**, non commencé en v8 | aucun résolveur dans `src/` ; v7 : 41 986 201 MEB, 3 898 856 828 supports | voie statique par facettes de la v7, noyau MEB avec repli, métriques de descente |
| B4 | histoire et export séquentiels (l. 311-346) | **ouvert**, non commencé en v8 | `src/forest/.gitkeep` ; v7 : calendrier séquentiel, plafond d'Amdahl de 1,98× après le noyau MEB | plan d'aval parallèle, portes sur les tableaux d'histoire |
| B5 | catalogues, copies, CPU/GPU (l. 348-380) | **ouvert** | `src/gpu/.gitkeep` ; générateur à 13,7 Mo de RSS en mode digest, sans catalogue | résidence d'abord, reprise, largeurs d'indices dérivées |
| packing WSPD | borne de packing du trie réel (l. 235-238) | **ouvert** (lacune de preuve) | aucune note de clôture ; `audits/WSPD_Q2_Q3_Q4.md:184-196` | ne pas invoquer O(s³n) ; mesurer rectangles par point et F |
| § 7 des verrous | la borne de sortie FULL est une limite mathématique (l. 382-395) | **prouvé** (v7), repris | `morsehgp3D_v9/docs/HERITAGE_V7_V8.md:21` | contrats en surcoût au-delà de la sortie |
| refonte § 1 | une seule chaîne, API FULL | **ouvert** (API absente, cinq entrées q2, deux moteurs) | `docs/audit_v8/02_chaine_q2.md` § 5.6 ; `src/forest` vide | API FULL avec statut de complétion |
| refonte § 2 | objets par étape | **partiel** : générateur oui, aval non | voir § 11 | invariant de rétention du catalogue |
| refonte § 3 | éliminer tôt les rectangles | **partiel** : points 1 à 3 réalisés en q2 ; en q3/q4 seul le point 1 | `front.hpp:25-39` ; cascade proposée | règle « un bloc exclu du h parental reste disponible » |
| refonte § 4 | coût des terminales | **ouvert** | — | idem B3 |
| refonte § 5 | histoire parallèle | **ouvert** | — | idem B4 |
| refonte § 6 | objets communs payés une fois | **partiel** : générateur (index, cover et atlas partagés) ; aval absent | `wspd_q34.cpp:497-510` | 10 préparations au lieu de 19, export par sommes préfixes, manifeste FULL |
| refonte § 7 | même plan de travail CPU/GPU | **partiel** CPU (file bornée, identités) ; GPU **ouvert** | `wspd_q34.hpp:36-44` ; reçus phase 1 | kernels exécutés sur carte ; travail identique quel que soit l'ordonnancement |
| refonte § 8 | portes de preuve | **partiel** : lignes Témoins, q3/q4 et Propriété (q2) testées ; FULL, Gabriel, Plateaux, Resolver et Tour/export absentes | § 11 | porter les neuf lignes |
| refonte § 9 | mesure de croissance et contrats | **partiel** : 8k/16k/32k synthétiques ; trames sans sol à s8 seulement ; aucune tour | reçus q2 et q4 ; reçus `ground_*` | protocole de qualification ; multi-millions différé |
| refonte § 10 | premier chantier : P0 puis tranche FULL minimale | P0 **partiel** ; tranche FULL **manquante** | `src/forest/.gitkeep` | c'est la V9-1 |

## 4. P0 : supprimer la préparation quadratique des témoins locaux

**Énoncé.** Décision de l'utilisateur du 13 septembre
(`PLAN_DE_REFONTE.md:308-322`). L'objet requis est un minorant certifié du
nombre de témoins, pas l'histogramme exact. Trois critères de clôture
(l. 342-357) :

1. prouver la sûreté des rejets ;
2. mesurer en mono, à 8k/16k/32k et s8/10/12, sur plusieurs familles ;
3. retenir une architecture sur le travail total et annoncer les régimes non
   résolus.

**Fait.**

- **Objet littéral : levé (testé).** Aucune entrée du moteur v8 ne calcule
  d'histogramme A×A ou B×B.
  - Le Pool terminal coûte O(KF), avec F la somme des tailles des facteurs,
    puis O(F+KR_s) pour regrouper les rectangles sélectionnés
    (`docs/P0_POOL_TERMINAL_Q2.md:126-129`).
  - Le front traite des nœuds partagés, sans plan par produit (tranche 7,
    `da366f7f`).
  - La voie q3/q4 fait une recherche de citron par rectangle, puis par paire
    (`src/pipeline/wspd_q34.cpp:406-418` et `485-492`).
- **Critère 1 : prouvé et testé pour q2.** Six résultats le portent :
  - certificat frère (T9) ;
  - ordre Complement (T10) ;
  - Pool (T12) ;
  - fenêtre 2K (T20) ;
  - théorème H par identifiants (T21) ;
  - contre-fixture à cinq points : le compte seul perd un support
    (`PLAN_DE_REFONTE.md:719-720`).

  Pour q3/q4 : lemme du citron et contre-exemple α3 en q4 (voir
  `morsehgp3D_v9/docs/HERITAGE_V7_V8.md:23`).
- **Critère 2 : mesuré.**
  - **Q2, quatre familles**, n ≥ 8 000, s8/10/12, W1/W4 :
    - fenêtre 2K : ×0,42 à 0,54 sur l'uniforme, ×0,51 à 0,62 sur les amas,
      ×0,64 à 0,71 sur le terrain, et **×1,01 à 1,18 sur les rangées**
      (`receipts/q2_front_proposals_20260917/README.md:103-108`) ;
    - héritage, face à la même fenêtre : ×0,86 à 0,96 hors rangées
      (`receipts/q2_front_inheritance_20260917/README.md:79-84`) ;
    - Pool64 sur les amas à K10 : 184,306 s → 19,180 s à 32k
      (`receipts/q2_terminal_pool_20260914/README.md:109-116`).
  - **q3/q4** : 34 mesures à 8k/16k/32k, K5/10, s8/10/12
    (`PLAN_DE_REFONTE.md:79-80`, reçu `q4_seed_cells_20260921`).
- **Critère 3 : jamais prononcé.**
  - La conclusion de la tranche 21 dit « le front q2 n'a plus de levier mesuré
    au-dessus de 10 % » (`PLAN_DE_REFONTE.md:723-725`).
  - La reprise du 20 septembre abandonne la priorité (« sans prolonger la
    recherche de quelques pourcents sur q2 »,
    `docs/REPRISE_DEVELOPPEMENT_20260920.md:18-19`).
  - Les défauts de l'API restent la configuration lente : Pool désactivé,
    frère désactivé, ordre `GlobalDfs`, fenêtre 1, sans héritage
    (`src/pipeline/wspd_q2_census.hpp:139-143` ; `src/wspd/front.hpp:28,39`).
  - Régimes non résolus et non annoncés comme tels : les **rangées** (voir
    ci-dessus), et la croissance du travail restant, inchangée (tranches 20-21 :
    « constante divisée, pas un exposant », `PLAN_DE_REFONTE.md:708-709`).

**État : partiel.** L'objet est atteint dans le code, mais P0 n'est ni clos
selon ses critères ni retiré par écrit. `PLAN_DE_REFONTE.md:364` dit encore
« ouverte, sans solution finale choisie ».

**À garder en v9.**

- L'invariant « jamais d'histogramme local carré », déjà dans
  `morsehgp3D_v9/docs/FAUSSES_PISTES.md:52`.
- La règle « indécis ≠ vacuité » (`PLAN_DE_REFONTE.md:318-319`).
- Le Pool en O(KF) avec repli sans réduction, dont le coût reste compté.
- Les défauts égaux à la configuration mesurée (principe 5 de PLAN_V9 : acquis).
- Une phrase de clôture explicite dans la passation v9 : « P0 fermé comme
  objet ; le résidu relève de B1 ».

## 5. B1 : recherches de témoins recommencées

**Énoncé** (`VERROUS_ARCHITECTURE.md:206-238`). Le seuil h borne les succès,
pas les échecs. À comparer :

- requêtes groupées ;
- certificats de blocs ;
- propagation d'identifiants parent → enfants ;
- front à équipe persistante.

Critères : un petit juge indépendant, des compteurs (visites, échecs,
réemplois, pire requête) et un gain net sur le travail total.

**Fait en q2 (testé et mesuré).**

- Front par nœuds partagés : proposeur en O(D+K) **par produit**
  (`VERROUS_ARCHITECTURE.md:58-63`).
- Raccord direct au census, compte et curseur hérités (l. 74-80).
- Certificat frère, ordre, Pool.
- Fenêtre 2K (tranche 20) et héritage des rangs certifiés (tranche 21), juge
  par un modèle Python indépendant et dix mutants dont quatre non sûrs
  (`PLAN_DE_REFONTE.md:713-716`).
- Le census conjoint A×B (tranche 11) est négatif.

Limites écrites par la v8 :

- « une descente est toujours payée par produit visité ; sa reprise exacte ne
  rend que ×0,94 à ×0,98 » (`VERROUS_ARCHITECTURE.md:140-143`) ;
- « les rangées ne gagnent rien, la croissance du travail restant est
  inchangée » (idem).

Les deux options gagnantes sont **opt-in** et **réservées au masque 1**. Le code
lève « widened proposals are qualified for the q2 lane alone » et « inherited
witnesses are qualified for the q2 lane alone » (`src/wspd/front.cpp:406-412`).

**Fait en q3/q4 : rien de B1.**

- « Each lane starts with ZERO credits » (`src/lanes/q34_witness_search.hpp:59`).
- « Partial counts are discarded, never used to seed another search or census »
  (`src/pipeline/wspd_q34.hpp:137`).
- Après la recherche du rectangle, chaque paire relance une recherche
  singleton depuis la racine (`src/pipeline/wspd_q34.cpp:485-492`).
- Mesuré (champs recalculés depuis les reçus épinglés) :

| scène 0 sans sol, 2 cm | produits du front | pas de descente du proposeur | visites, recherches par rectangle | visites, recherches par paire | paires développées / rejetées par paire |
| --- | ---: | ---: | ---: | ---: | ---: |
| K5, W1 (`only_probe_00`) | 8 803 901 | 170 294 360 | 225 819 102 | 1 090 846 648 | 23 957 225 / 21 934 375 |
| K10, W8 (`only_probe_02`) | 11 184 867 | 218 067 534 | 421 597 188 | 1 915 042 191 | 30 685 708 / 26 210 367 |

- Le bloc `work.witness` est **identique** entre `ground_baseline/probe_00` et
  `ground_phase1/only_probe_00` : les phases 1-2 n'ont pas touché B1.
- L'audit du 22 septembre propose une cascade :
  - paire représentante en exclusion seule ;
  - plans h + h_a + h_b ;
  - réemploi de la décision des singletons.

  Référence : `morsehgp3D_v8/audits/SYNTHESE_PRIORITES_LIDAR_20260922.md:9-29`.
  Elle a été mesurée sur des échantillons de rectangles, à paires survivantes
  identiques : ×2,55 à ×4,46 (l. 43-58). **Non vérifiable** au sens strict :
  seul `audits/cascade_rectangles_20260922/RESULTS.json` est dans le dépôt,
  les sources sont dans une archive jointe à une conversation (l. 68-69).
- **Rangées** :
  - avec des témoins ponctuels seuls, le résidu reste quadratique
    (`VERROUS_ARCHITECTURE.md:70-72`) ;
  - certificat collectif d'arête : prototype d'audit prouvé et testé
    (0 faux minorant sur 8 736 requêtes,
    `audits/CERTIFICATS_COLLECTIFS_AVANT_ATLAS_20260922.md:140-147`) ;
  - pas de gain de pipeline démontré (l. 194).

**État : partiel en q2 ; ouvert en q3/q4 et sur les rangées.**

**À garder en v9.**

- Transporter des identifiants, jamais un compte (fausse piste v9 l. 24).
- Une seule descente par produit.
- Les compteurs `witness_descent_steps`, `node_visits`, et la distinction
  rectangles/paires.
- La règle « un bloc exclu du h commun parental reste disponible pour les
  lignes, colonnes et enfants » (`SYNTHESE_PRIORITES_LIDAR_20260922.md:73-74`).
- Un **juge indépendant avant toute extension du théorème H aux voies q3/q4**
  (`VERROUS_ARCHITECTURE.md:68-69`).

## 6. B2 : triangles de départ × voisinages q3/q4

**Énoncé** (`VERROUS_ARCHITECTURE.md:240-275`). Le travail est de type S×C.
Garder l'acquis v7 : le balayage q4 ne rescanne pas le cover. Il faut
confronter chaque voie à des oracles et compter incidences, racines et
complétions. La réduction du travail se mesure séparément de l'accélération
parallèle.

**Fait (tranches 22 à 34, puis phase 1).**

- Famille q4 balayée par événements (`9ae4e28b`) : l'acquis v7 est conservé.
- Un cover par arête, partagé entre q3 et q4 (`wspd_q34.cpp:497-499`).
- Rejet familial et corde resserrée, carte des centres, fragments exacts,
  couches duales et fenêtre [L, U], toutes en option.
- Atlas Local28 LiveOnly, avec l'atlas en i64 à Q = 2^20 (`748ec082`).
- Graines q3 certifiées par l'atlas (`0948d2d0`, option
  `q3_atlas_consultation`, `wspd_q34.hpp:28-35`).
- Portes :
  - racines égales (`tests/q4_family_gate.cpp:303`) ;
  - profondeur qui redescend (`:198`) ;
  - cover qui perd une boule (`tests/q34_cover_gate.cpp:251`) ;
  - support non positif (`tests/exact_ball_gate.cpp:87`).

**Mesuré (reçus épinglés, recalculés).**

| scène sans sol, 2 cm | covers / sites de cover payés (max) | graines q3 → boules construites → émissions q3 | atlas : bornes de blocs / tests ponctuels / IDs copiés |
| --- | --- | --- | --- |
| scène 0, K5 W1, référence (`ground_baseline/probe_00`) | 2 022 850 / 2,895 G (20 718) | 179 748 732 → 179 748 732 → 663 443 | 3,159 G / 7,091 G / 5,412 G |
| scène 0, K5 W1, phase 1 (`only_probe_00`) | idem | 179 748 732 → 31 013 253 → 663 443 | **identiques** |
| scène 0, K10 W8 (`only_probe_02`) | 4 475 341 / 7,689 G (30 923 sur 39 815 sites) | 463 118 817 → 35 417 767 → 2 830 806 | 10,718 G / 24,196 G / 18,307 G |
| scène 2, K10 W8 (`probe_08`) | 4 835 825 / 8,875 G | 713 387 669 → 35 412 104 | 22,247 G / 44,536 G / 32,453 G |

Lecture :

- La phase 1 réduit le census q3. Les bornes de census passent de 5,835 G à
  1,057 G à K5 sur la scène 0.
- Les trois compteurs d'atlas sont **inchangés** entre la référence et la
  phase 1.
- Sur les adversaires, le carré persiste :
  - « le carré reste dans les feuilles » (`PLAN_DE_REFONTE.md:171-174`) ;
  - l'adversaire K10 des couches duales reste quadratique (l. 185-187).
- La voie q3 n'a aucun juge indépendant à l'échelle d'une trame
  (`morsehgp3D_v9/docs/AUDIT_V8_SYNTHESE.md`, § 6, gravité moyenne).

**État : partiel.** Aucune borne n'est établie. Le poste dominant, l'atlas,
est intact.

**À garder en v9.**

- Le cover partagé, LiveOnly et le certificat d'atlas des graines q3.
- Les compteurs du tableau ci-dessus, en l'état.
- Les fixtures citées.
- Les pistes fermées : Joined, Window30 en remplacement, couches duales en
  remplacement.
- Mesurer les atlas **évités**, pas le seul filtre.

## 7. B3 : rattachements MEB et recherches d'intrus

**Énoncé** (`VERROUS_ARCHITECTURE.md:277-309` ; refonte § 4, `PLAN_DE_REFONTE.md:549-568`).

- Constat v7 : 41 986 201 appels MEB, 3 898 856 828 supports essayés, et
  389,668 s de constructeur FULL sur une sonde de 418,873 s. Vérifié dans
  `morsehgp3D_v8/audits/CONTRATS_ET_MESURES.md:43,74,91-92`, qui cite des
  reçus v7.
- À comparer : supports proposés par le contexte, certification exacte avec
  repli, regroupement par facettes entières.

**Fait en v8 : rien.** `grep -i 'meb|resolver' src/` est vide.
`src/forest/` ne contient que `.gitkeep`.

**Existant v7, non porté.**

- **Voie statique.** Elle développe les demandes, trie les facettes entières
  et résout une fois par clé unique (`morsehgp3D_v7/src/forest/full_ball_tower.hpp:751-839`).
  Elle est désactivée par défaut : `static_threads = 0` (l. 293, 1083), et le
  chemin nominal reste alors le cache temporel
  (`morsehgp3D_v8/audits/IMPLEMENTATION_PARALLELISATION.md`, § 5).
- **Réductions mesurées en v7.** Cache exact, résolution statique (−33 % de
  MEB), semis (−5,7 %) (`morsehgp3D_v9/docs/audit_v8/10_heritage_v7.md:184-186`).
- **Noyau MEB des auditeurs.** Testé sur 198 000 cas hors moteur. Mesuré de bout en bout sur flux réel : ×1,72 sur la phase géométrique et
  ×1,357 sur la tour entière, à `payload_digest` identique
  (`morsehgp3D_v7/audits/NOTE_CLAUDE_DECOUPE_TOUR_20260911.md:88-90`).

**État : ouvert, non commencé.**

**À garder en v9.**

- Porter la **voie statique comme défaut** du résolveur.
- Porter le noyau MEB avec certification complète : positivité, confinement,
  coquille, clé.
- Garder le repli exact.
- Compteurs : demandes totales et uniques, MEB, supports, intrus, distribution
  des longueurs de descente, replis.
- Fixtures :
  - terminale comparée **avant** normalisation ;
  - semis rejeté puis repli ;
  - même rayon ou même coquille.

## 8. B4 : histoire et export derrière un calendrier séquentiel

**Énoncé** (`VERROUS_ARCHITECTURE.md:311-346` ; refonte § 5, `PLAN_DE_REFONTE.md:570-594`).
Enchaînement : graphe daté par K, forêt minimale, histoire, regroupement par
vraie connexité. RCTT et PANDORA sont des pistes, sous réserve d'une preuve
d'adaptation aux dates égales et aux multifusions.

**Fait en v8 : rien.**

**Existant v7, mesuré (note d'audit instrumentée, découpe reproduite
indépendamment).**

| n = 8 000, moteur d'origine | géométrie | calendrier | prologue | épilogue |
| --- | ---: | ---: | ---: | ---: |
| part de `tower_s` (63,97 s) | 62,8 % | 16,8 % | 15,4 % | 5,0 % |

Source : `NOTE_CLAUDE_DECOUPE_TOUR_20260911.md:24-27` ; reproduction à 16,0 /
62,7 / 16,2 / 4,4 %, dans l'ordre prologue / géométrie / calendrier / épilogue
(l. 179-181).

- Après le noyau MEB, le calendrier passe à 22,3 %. Le plafond d'Amdahl de la
  seule géométrie tombe à 1,98× (l. 44-58).
- Exposants locaux : calendrier 1,243, géométrie 1,094 (l. 66-70).
- À 50k, les deux leviers donneraient environ 102 s, dont 90 % de calendrier et
  d'épilogue séquentiels (l. 78-98). C'est une **allocation indicative, non
  mesurée**.
- À 32k : histoires 50,467 s, export 92,044 s (`morsehgp3D_v8/audits/CONTRATS_ET_MESURES.md:208-209`).
- Prototypes privés des graphes datés et de la contraction des pivots : testés
  et mesurés sur CPU4, mais hors `src/`, sans comparaison physique à 32k
  (`docs/audit_v8/10_heritage_v7.md:188,305`).

**État : ouvert, non commencé.** Même avec un générateur gratuit, le calendrier
v7 empêche le contrat d'une seconde.

**À garder en v9.**

- Le plan de refonte § 5.
- Les obligations de preuve RCTT et PANDORA : dates égales, multifusions,
  histoires en peigne.
- Portes :
  - plateaux disjoints de même date ;
  - plateaux traversant des lots ;
  - marques forgées.
- Comparer les **tableaux d'histoire**, pas seulement le digest :
  `payload_digest` est aveugle à la renumérotation des populations
  (`NOTE_CLAUDE_DECOUPE_TOUR_20260911.md`, § 4ter ; `VERROUS_ARCHITECTURE.md:428-430`).

## 9. B5 : résidence, copies et aller-retour CPU/GPU

**Énoncé** (`VERROUS_ARCHITECTURE.md:348-380` ; refonte § 6-7).

- Constat v7 : RSS de 16 206 376 KiB à 50k/K10 (`CONTRATS_ET_MESURES.md:43`) ;
  kernels de 0,189 s pour une phase de 4,540 s, dont 2,932 s de reconstruction
  hôte (`audits/IMPLEMENTATION_PARALLELISATION.md:281`).
- Critères :
  - pics simultanés ;
  - octets transférés ;
  - largeurs d'indices des objets dérivés ;
  - échec après un lot réussi ;
  - reprise dans un nouveau processus ;
  - grandes coquilles.

**Fait en v8.**

- Aucun GPU : `src/gpu/.gitkeep`, `GPU_executed=false` dans les reçus G4.
- Le générateur réside peu, car il n'est mesuré qu'en mode digest : RSS
  maximal de 13 656 KiB, `peak_edge_buffer_bytes` de 197 184 à K10 W8 sur la
  scène 0 (`ground_phase1_20260921/only_time_02…txt`, `only_probe_02…json`).
- Aucun catalogue ni aucune forêt n'est retenu.
- Volume émis à K10 sur la scène 0 : 4 560 557 enregistrements q3/q4,
  15 411 422 IDs de support et 15 459 391 IDs de coquille, pour
  `memory.id_bytes = 8`. Retenu tel quel, cela ferait environ 247 Mo d'IDs
  (**estimation d'audit**, q2 non compté).
- Outillage utile : injection déterministe d'échecs d'allocation dans dix
  portes (`morsehgp3D_v9/docs/HERITAGE_V7_V8.md:51`).
- Le format q2 n'est pas transportable : IDs `std::size_t`, vues empruntées
  pendant le rappel (`docs/audit_v8/02_chaine_q2.md`, § 5.12).
- Limites v7 : `BallId` sur u32 et masque de contribution u16
  (`full_ball_tower.hpp:109,215,921-925`), coquille ≤ 12 et intérieurs ≤ 9
  (`morsehgp3D_v7/docs/TOUR_FULL_PAR_BOULES.md:37-43`, cité par
  `docs/audit_v8/10_heritage_v7.md`, § 5).
- Aucune distribution des tailles de coquille n'est publiée. La capacité de
  pointe du tampon de coquille q3 vaut 128 octets, soit 16 IDs, à K10 sur la
  scène 0 (`work.q3.peak_shell_bytes`).

**État : ouvert.**

**À garder en v9.** La résidence d'abord (V9-4), le contrat `lot_ring` de la
v6 dans sa version commise, l'injection d'échecs d'allocation, les portes de
lots et de reprise. La largeur de tout identifiant dérivé est vérifiée comme
l'est la coordonnée.

## 10. Lacune de packing de la WSPD et choix de s

- **Obligation.** Justifier la borne de packing du trie réel
  (`VERROUS_ARCHITECTURE.md:235-238` ; `audits/WSPD_Q2_Q3_Q4.md:184-196`).
- **Construction v8.** Bissection au milieu de la plus longue étendue
  (`src/lanes/q34_witness_search.hpp:84-87`), convention `box_gap_diameter_v1`
  (`docs/P0_FRONT_REEL.md:35-38`). Ce commentaire parle encore de 49 cadres
  et de « seize réductions » alors que la pile compte 55 cadres en 18 bits :
  commentaire 16 bits résiduel.
- **Régime mesuré** (convention v4) : environ 90 rectangles de plus par point à
  chaque doublement, de 8k à 256k (`audits/REGIME_WSPD_20260914.md:78-86`).
- **Ce qui manque.** Aucune note de clôture ni aucune entrée au registre des
  preuves.
- **État : ouvert.** En v9, ne pas invoquer O(s³n). Mesurer les rectangles et F
  sur les trames ; les compteurs `emitted_rectangles` et
  `emitted_factor_sites` existent déjà.

## 11. Chantiers § 1 à § 10 du plan de refonte, point par point

| chantier | exigence précise | état à 12294241 | preuve |
| --- | --- | --- | --- |
| § 1 API FULL | entrée, métrique, Kmax, convention de coupe, nœuds, parents, contributions, verticales, statut de complétion (l. 467-470) | **manquant** | `src/forest`, `src/io`, `cli` vides |
| § 1 propriétaire | préparation immuable qui possède index, catalogue et rangs ; refus d'un faux certificat ou d'un autre nuage (l. 477-483) | **partiel** : nuage et index possédés, refus du nuage étranger testé pour q2 ; pas de catalogue | `tests/cloud_owner_gate.cpp:360-393` ; `wspd_q34.hpp` (« index is owned throughout ») |
| § 1 une seule copie | « ne pas maintenir plusieurs copies concurrentes du moteur dans `build/` » (l. 474-476) | **en tension** : environ 150 builds `v8*` (13 Go), surtout des builds d'autorité épinglés exigés par les lecteurs de captures | `docs/audit_v8/01_trajectoire_contrats.md:281` |
| § 2 lignes front, q2, témoins | descripteurs, masques, compte et curseur | **réalisé** | tranches 4, 7, 8 et 12 |
| § 2 ligne q3 | grain « graine × plage de sites » | **partiel** : graine canonique par arête maximale ; grain = arête, jamais scindée | `docs/JOURNAL_DEVELOPPEMENT_20260921.md:37-38` ; `wspd_q34.hpp:208` |
| § 2 ligne q4 | racines exactes, scans segmentés | **partiel** : racines exactes ; balayage mono par arête | `9ae4e28b` |
| § 2 lignes catalogue, rattachements, histoire, contributions | une boule canonique… | **manquant** | clé `ExactBall` commune seulement (`src/lanes/exact_ball.hpp:23-28`) |
| § 2 invariant de l'auditeur B | élimination par voie, **rétention par boule canonique** ; supports depuis la plus longue arête, départage déterministe ; fixtures « lors du raccord du catalogue » (l. 503-510) | générateur **testé** (`tests/q3_q4_owner_independence_gate.py`) ; rétention **manquante** | — |
| § 3, points 1 à 5 | proposer puis certifier ; transporter au plus h_q−1 IDs ; h + h_a + h_b disjoints ; blocs positifs et négatifs ; raffiner | q2 : 1 à 4 réalisés (fenêtre, héritage, Pool) ; q3/q4 : 1 seul, 2 à 4 proposés par la cascade | `front.hpp:25-39` ; `SYNTHESE_PRIORITES_LIDAR_20260922.md:12-24` |
| § 3 choix de s | comparer s8/10/12 sans choisir sur les seuls candidats finals (l. 545-547) | synthétique : réalisé ; trames sans sol : s8 seulement | reçus q2 et q4 ; `ground_*` |
| § 4 terminales | proposer, certifier, replier ; regrouper par facette | **manquant** | § 7 |
| § 5 histoire | graphe daté, forêt minimale, contraction | **manquant** | § 8 |
| § 6 objets communs | 10 préparations d'index historiques au lieu de 19 ; marques du premier parcours ; export par sommes préfixes ; manifeste FULL distinct de l'archive F, offsets larges, lots identifiables, journal de reprise (l. 598-613) | générateur **partiel** (index, cover et atlas partagés) ; aval **manquant** | `wspd_q34.cpp:497-510` |
| § 7 CPU | équipe persistante, destinations fixes, **travail identique** quand seul l'ordonnancement change, opérations spéculatives comptées (l. 617-622) | **partiel** : file bornée, `published = consumed`, compteurs géométriques identiques de W1 à W8 ; pas de plan plat | `wspd_q34.hpp:36-44` ; reçus phase 1 |
| § 7 GPU | résidence, compactions ; « chaque kernel devra être exécuté sur carte » (l. 624-628) | **manquant** | `src/gpu/.gitkeep` |
| § 8 portes | neuf lignes (l. 637-647) | Témoins **testé** ; q3/q4 **testé** en partie ; Propriété **testé** pour q2 ; Massive **partiel** (échecs d'allocation, coquilles q2 non plafonnées) ; Objet FULL, Gabriel, Plateaux, Resolver, Tour/export **manquants** | § 4 à 9 |
| § 9 croissance | 8k/16k/32k, s8/10/12, mono puis 4 CPU, familles adverses (l. 655-659) | **réalisé** pour q2 et q4 synthétiques ; trames sans sol : trois scènes, s8, W1/W8 | reçus |
| § 9 contrat | session G4 après raccord complet : deux échauffements, dix nuages frais, p95 (l. 667-671) | **non applicable** (aucune tour) ; quatre sessions G4 CPU sur le flux seul | `receipts/q34_spatial_20260921` |
| § 9 multi-millions | résidence et formats d'abord (l. 673-677) | **ouvert** | — |
| § 10 | P0 en mono avec juges ; **tranche FULL minimale** en soutien (l. 727-734) | P0 partiel ; tranche FULL **jamais faite** | `src/forest/.gitkeep` |

Feuille de route des verrous (`VERROUS_ARCHITECTURE.md:408-417`) :

| point | exigence | état |
| --- | --- | --- |
| 1 | lire P0 et comparer ses architectures | fait |
| 2 | instrumenter B1/B2 | fait : compteurs du front, des recherches, des graines et de l'atlas |
| 3 | petits oracles **et tranche FULL minimale** ; traiter B3 ; préparer B4/B5 | oracles faits ; tranche FULL, B3, B4 et B5 non faits |
| 4 | mono puis multi-CPU à 8k/16k/32k, s8/10/12, familles adverses | fait en synthétique (W ≤ 4 pour q2, W ≤ 8 pour q3/q4) |
| 5 | qualifier la tour 50k puis 1..5, puis 100 ms | périmé (contrat trame puis sans sol) ; non fait |

Portes exigées par les verrous (l. 425-434) :

- **B1** : réemploi réellement utilisé. Fait en q2.
- **B2** : passes q3/q4 réellement traversées. Planchers présents.
- **B3** : proposition réussie et repli pris. Absent.
- **B4** : coupes ouvertes et fermées, tableaux d'histoire. Absent.
- **B5** : lots de tailles variables, échec après un lot, reprise. Absent.

## 12. Confrontation avec `morsehgp3D_v9/docs/PLAN_V9.md`

### 12.1 Ce que le plan reprend correctement

- **Ordre des travaux.** Principe 4, l. 20-22 : mesurer de bout en bout, réduire
  le nombre d'opérations, paralléliser, puis porter sur GPU. C'est le titre et
  l'ordre du plan de refonte.
- **Tranche FULL.** V9-1 (l. 54-74) est enfin la « tranche FULL minimale »
  du § 10, avec la sémantique v7 et non le fold v4.
- **Catalogue.** L'union q2 ∪ q3 ∪ q4 dédoublonnée (l. 60-62) réalise
  l'invariant de rétention de l'auditeur B, sans en graver la fixture (voir O7).
- **B2 dans V9-2** (l. 83-89) : certificat collectif d'arête, saturation de
  l'atlas, une seule traversée par arête, frontières sans copie.
- **B3, en partie** (l. 90) : noyau MEB.
- **Refonte § 7.** Flottants certifiés (l. 91-93) et plan plat commun CPU/GPU
  (l. 100-102).
- **B5.** Résidence d'abord et critère d'arrêt des 50 % d'hôte (l. 114-119).
- **P0 et B1.** Défauts égaux à la configuration mesurée (principe 5, l. 23-25) :
  corrige la dette des options q2 opt-in.
- **Registre des preuves** (l. 142-144) : conforme au plan de refonte et au
  `CLAUDE.md`.
- **[non commis]** La version de travail ajoute quatre points :
  - le multi-millions « différé, pas abandonné » (principe 6) ;
  - l'extraction de la distribution des tailles de coquille et d'intérieur
    avant le port ;
  - les deux consommateurs du format de sortie ;
  - les sources v6 de la résidence GPU.

  Ces ajouts traitent la question multi-millions du § 9 de la refonte (en la
  différant) et une partie de l'oubli O8 (tailles de coquille, format de
  sortie) ; ils ne touchent ni B4, ni B3, ni les différentiels, ni les
  familles adverses.

### 12.2 Oublis

| n° | gravité | oubli | source de l'exigence | ce qu'il faudrait écrire |
| --- | --- | --- | --- | --- |
| O1 | haute | **B4 absent.** Aucune phase ne parallélise le calendrier, l'histoire ou l'export. V9-3 (l. 100-110) ne traite que le générateur ; V9-4 mentionne seulement des « étages de l'aval ». `HERITAGE_V7_V8.md:75` cite les prototypes, mais aucune phase ne les porte. | `VERROUS_ARCHITECTURE.md:311-346` ; `PLAN_DE_REFONTE.md:570-594` ; v7 : calendrier séquentiel d'exposant 1,243, plafond de 1,98× | une étape V9-3 « aval » : graphe daté, forêt minimale, contraction (preuve pour dates égales et multifusions), portes sur les tableaux d'histoire, part séquentielle mesurée |
| O2 | haute | **Aucun différentiel dans V9-1.** Rien ne compare la tour v9 à la tour v7 (uniforme u16 à 8k/16k/32k, digests et tableaux d'histoire), ni le flux v9 au flux v8 (`xor`, `sum`, compteurs sur les trois trames). La porte de sortie se limite à « bit-identiques en relecture » (l. 73). | `PLAN_DE_REFONTE.md:732-733` (« aucune réécriture géante sans différentiel ») ; `VERROUS_ARCHITECTURE.md:428-430` | deux différentiels obligatoires en sortie de V9-1 ; comparer les tableaux, pas seulement le digest |
| O3 | haute | **B3 incomplet.** V9-1 ne dit pas quelle voie du résolveur v7 est portée : la voie statique par facettes est désactivée par défaut (`static_threads = 0`). V9-2 ne demande ni compteurs MEB et supports, ni distribution des longueurs de descente, ni porte « terminale avant normalisation ». | `VERROUS_ARCHITECTURE.md:300-309` ; `PLAN_DE_REFONTE.md:551-568` ; `full_ball_tower.hpp:293,751` | porter la voie statique comme défaut ; compteurs et fixtures B3 dans V9-1 |
| O4 | moyenne | **B1 résiduel en q3/q4 non nommé.** Recherche par paire relancée depuis zéro (1,09 G visites à K5, 1,92 G à K10 sur la scène 0) ; extension du théorème H aux voies q3/q4 sous condition de juge. V9-2 ne cite que le réemploi des singletons. | `VERROUS_ARCHITECTURE.md:68-69,136-143` ; `wspd_q34.cpp:485-492` | un levier « témoins hérités par voie, avec juge indépendant », jugé sur ces compteurs |
| O5 | moyenne | **Familles adverses et protocole de croissance absents.** La porte de V9-2 se mesure sur les trois trames seulement. Le principe 3 garde 8k/16k/32k « pour les pentes » sans dire sur quelle famille. Le protocole spatial (trame, moitiés, quarts) décidé le 21/09 n'est pas repris. | `VERROUS_ARCHITECTURE.md:413-414` ; `PLAN_DE_REFONTE.md:65-73,348-352` ; régressions connues : rangées ×1,01–1,18 (T20), adversaire K10 des couches duales | chaque levier de V9-2 est aussi jugé sur uniforme, amas et rangées à 8k/16k/32k, plus le protocole spatial, sans régression cachée |
| O6 | moyenne | **Protocole de qualification du contrat absent** : échauffements, nombre de trames, p95, séquences. V9-5 renvoie les autres séquences à plus tard, alors que le contrat parle de « plusieurs scènes » (`AUDIT_V8_SYNTHESE.md` : une seule séquence, 08). | `PLAN_DE_REFONTE.md:667-671` | écrire le protocole avant la première session de qualification |
| O7 | moyenne | **Fixtures de l'invariant de catalogue absentes** : un rejet q3 ne doit pas effacer une boule q2 ; départage déterministe de l'arête maximale. | `PLAN_DE_REFONTE.md:503-510` | les ajouter à la liste des juges de V9-1 (l. 67-68) |
| O8 | moyenne | **Sérialisation et reprise.** Manifeste FULL distinct de l'archive F, offsets larges, lots identifiables, journal de reprise ; échec après un lot réussi et reprise dans un nouveau processus ; largeur des indices dérivés (`BallId`, masques). V9-0 ne borne que les coordonnées (l. 41-42). | `PLAN_DE_REFONTE.md:610-613` ; `VERROUS_ARCHITECTURE.md:367-376,431-432` | étendre la table `static_assert` aux objets dérivés ; porte de reprise en V9-4 |
| O9 | moyenne | **Portes du § 8 non reprises** : minima isolés ; recouvrement sans fusion ; K = 1 égal au single-linkage **à l'échelle** (invariant global calculable sur la trame) ; plateaux (disjoints de même date, coupés entre lots, grande arité) ; lignes Resolver ; lignes Propriété (vue périmée, mutation d'un propriétaire emprunté) ; lignes Tour/export (naturalité, refus de publication partielle). | `PLAN_DE_REFONTE.md:637-647` | compléter la liste des juges de V9-1 |
| O10 | moyenne | **§ 6 de la refonte côté aval** : index historiques préparés une fois (10 au lieu de 19), marques du premier parcours (delta v7 non commis), export par sommes préfixes. | `PLAN_DE_REFONTE.md:598-608` | à inscrire dans V9-2 (opérations) ou V9-3 |
| O11 | basse | **Lacune de packing et choix de s.** Le principe 3 fixe s = 8 ; `AGENTS.md` (v9, l. 23-25) dit s ∈ {8, 10, 12}, 8 par défaut. Aucune comparaison s10/12 n'est prévue sur la tour. | `VERROUS_ARCHITECTURE.md:235-238` ; `PLAN_DE_REFONTE.md:545-547` | inscrire la lacune au registre ; une ablation s10/12 sur la tour complète |
| O12 | basse | **GPU et parallélisme** : « chaque kernel exécuté sur carte » (le terminal v7 n'a été que compilé) ; « travail identique quand seul l'ordonnancement change » (V9-3 ne demande que les sorties bit-identiques, l. 107) ; opérations spéculatives comptées. | `PLAN_DE_REFONTE.md:619-628` | ajouter ces trois critères à V9-3 et V9-4 |
| O13 | basse | **Suivi des verrous.** Le plan ne reprend ni la liste B1–B5 ni ses critères « avant de déclarer le verrou traité ». Les deux documents v8 sont périmés et donnent encore le contrat 50k. | `VERROUS_ARCHITECTURE.md:194-200,440-445` | un registre B1–B5 dans la passation v9, avec compteurs et état ; déclarer les deux documents v8 historiques |

### 12.3 Contradictions

| n° | gravité | contradiction | preuve | résolution proposée |
| --- | --- | --- | --- | --- |
| C1 | haute | V9-1 demande « en configuration mesurée unique, voie q2 **dans le même appel** que q3/q4 » (l. 58-59). Or la fenêtre 2K et l'héritage de témoins, qui portent les gains q2 (×0,42–0,71 puis ×0,86–0,96), ne sont qualifiés **que pour le masque 1** : le code lève une exception sinon. Les voies q3/q4 n'en héritent pas « faute de juge ». Enfin, `run_wspd_q34_parallel` n'accepte que les masques 2, 4 et 6. | `src/wspd/front.cpp:406-412` ; `VERROUS_ARCHITECTURE.md:68-69` ; `src/pipeline/wspd_q34.hpp:116` | Trancher par écrit dans V9-1 : (a) appel unique sans fenêtre ni héritage, en publiant la perte q2 ; (b) deux fronts, q2 seul et q3/q4, dans le même lanceur chronométré ; ou (c) requalification de la fenêtre et de l'héritage pour le masque 7 avec juge indépendant. Ne pas écrire « configuration mesurée » tant qu'elle n'existe pas pour ce masque. |
| C2 | moyenne | Porte de sortie de V9-2 : « ×10 d'opérations en moins sur le **poste dominant** à K10 » (l. 95-96). Les verrous exigent un **gain net sur le travail total**, sans déplacement du coût (`VERROUS_ARCHITECTURE.md:231-233,308-309,409-410`), et la refonte refuse de clore sur un gain constant (`PLAN_DE_REFONTE.md:353-357`). Un ×10 sur un poste de 50 à 60 % ne donne au plus que ×2 sur le total. La synthèse v9 demande elle-même un ordre de grandeur sur le nombre total d'opérations (`AUDIT_V8_SYNTHESE.md`, § 4). | textes cités | Porte : travail total de l'appel complet (tour comprise) et pentes à 8k/16k/32k, en plus du poste dominant. |
| C3 | basse | « Un filtre qui pèse environ 7 % du profil » (l. 86). Ce chiffre vient du seul gprof épinglé, celui de la **référence d'avant la phase 1** (`ground_baseline_20260921/gprof_scene00_k5_w1`, filtres ≈ 7,0 %). Les compteurs du filtre sont identiques après la phase 1, tandis que le temps W1 passe de 888,2 à 453,2 s de CPU utilisateur. La part devient donc d'environ 14 % (**estimation d'audit** ; le journal dit 13 %, non épinglé). Le filtre porte aussi le résidu B1. Enfin, la recommandation la plus récente de l'auditeur place la cascade en tête (`SYNTHESE_PRIORITES_LIDAR_20260922.md:7-29`). | reçus cités | Corriger le chiffre, rattacher la cascade à B1, et répondre par écrit à l'audit (règle du plan, l. 133-135). Le gain reste borné à environ 10 % du total si l'on retient ×3 sur le filtre. |
| C4 | basse | V9-1 liste la fixture « triangle rectangle » (l. 68) sans verdict. `HERITAGE_V7_V8.md:94` attend un refus `unsupported_degeneracy`, alors que le principe 1 (l. 10-11) et V9-1 (l. 65-66) portent l'extension non régulière, et que la refonte demande des portes positives de plateaux (`PLAN_DE_REFONTE.md:641`). En v7, le refus vient du producteur horizontal régulier (`morsehgp3D_v7/tests/full_gabriel_gate.cpp:661-670`), pas de la tour par boules à extension non régulière. | textes cités | Fixer le verdict attendu selon le producteur porté, et graver les deux. |
| C5 | basse | V9-0 conserve les sondes « sous `build/v9_*` » (l. 45). La refonte interdit plusieurs copies concurrentes du moteur dans `build/` (`PLAN_DE_REFONTE.md:474-476`), et la v8 en a souffert : environ 150 builds, lecteurs de captures liés à des builds locaux. | `docs/audit_v8/01_trajectoire_contrats.md:281` | Exiger qu'un reçu se rejoue depuis son commit et sa recette. Un binaire conservé est une commodité, jamais une autorité. |
| C6 | basse | La première session G4 de V9-3 prévoit W1, W24 et W48, K5 et K10, sur les trois trames (l. 108-110). La refonte n'achète du temps G4 que « pour une expérience qui tranche » (`PLAN_DE_REFONTE.md:676-677`), et la contre-vérification v9 juge W1 à K10 infaisable dans un budget utile de 900 s (`docs/audit_v8/12_parallelisme_gpu_perf.md`, correction 21). | textes cités | Scinder en sessions, chacune nommant sa question. |

### 12.4 Amendements minimaux proposés au plan v9

1. **V9-1.**
   - Trancher C1 (q2 dans l'appel).
   - Porter la voie statique du résolveur.
   - Ajouter les différentiels v7 et v8 et la comparaison des tableaux
     d'histoire.
   - Ajouter les fixtures O7 et O9.
   - Publier par phase les compteurs de B1 à B4 : visites de recherche, atlas,
     MEB et supports, calendrier.
2. **V9-2.**
   - Porte sur le travail total et les pentes (C2).
   - Levier d'héritage de témoins par voie sous juge (O4).
   - Familles adverses et protocole spatial (O5).
   - Chiffre du filtre corrigé (C3).
3. **V9-3.**
   - Étape « aval parallèle » (O1).
   - Travail identique entre ordonnancements et opérations spéculatives
     comptées (O12).
   - Sessions G4 scindées (C6).
4. **V9-4.**
   - Kernels exécutés sur carte.
   - Porte de reprise : échec après un lot, nouveau processus.
   - Largeurs d'indices dérivés (O8, O12).
5. **Passation v9.**
   - Registre B1–B5 avec critères et compteurs.
   - Clôture écrite de P0 comme objet.
   - Lacune de packing inscrite au registre des preuves.
   - Documents v8 déclarés historiques (O13).

## 13. Chiffres non vérifiables et limites de ce rapport

- **Cascade** (×2,55 à ×4,46), **présélection** et **collectif LiDAR** (221
  rejets q4 sur 763 arêtes) : sources dans des archives jointes à une
  conversation (`SYNTHESE_PRIORITES_LIDAR_20260922.md:68-69` ;
  `lidar_rectangles_20260922/README.md:105`). Seuls `RESULTS.json` et le
  README sont dans le dépôt.
- **Distribution des rectangles** du front (96,89 à 97,38 % de moins de
  64 paires) : même réserve (`lidar_rectangles_20260922/README.md:27`). La
  masse moyenne recalculable depuis le reçu vaut 102 195 901 / 3 118 479, soit
  environ 32,8 paires par rectangle (K5, scène 0).
- **Tour 50k à environ 102 s** après les deux leviers v7 : allocation
  indicative, non mesurée.
- **Profil après la phase 1** (atlas ≈ 52 %, filtres 13 %) : journal, sans
  reçu. Les « environ 14 % » de C3 et les « environ 247 Mo » du § 9 sont des
  estimations de ce rapport, pas des mesures.
- Ce rapport n'a exécuté aucun test. Les « testé » et « mesuré » renvoient à
  des portes et reçus lus, pas rejoués. La tranche non commise du worktree
  partagé (atlas saturant, gardes de domaine) n'est pas prise en compte, sauf
  mention.
