# Fouille v10 — tour, tête, points, clustering, portes : ce qui se transpose utilement dans la v11

4 octobre 2026, rapport écrit de 12 h 35 à 12 h 40 UTC (heures lues par `date -u`), après la lecture des sources. Auditeur de la source « v10 : tour,
tête, points, clustering, portes » de l'audit géant des transpositions (`../CONTEXTE.md`, cartes
`../cartes/CARTE_V11.md` et `../cartes/CARTE_V10_VITESSE.md` lues d'abord).

```text
phase=exploration_v11_hors_registre (audit des transpositions, lecture seule)
backend=cpu_reference
profile=quantized_u21_input_only (v11 mesurée) ; quantized_u18_input_only (v10 mesurée)
public_status=not_claimed
GCP non utilisé ; aucune construction ni exécution native ; aucune commande git qui écrit
```

Rappel de l'utilisateur : « l'objectif du contrat est toujours 100 ms » (tour FULL K = 1..5 des trames SemanticKITTI
sans sol, grille 1 mm, moteur entier exact, G4 à 48 fils ; si possible K = 10).

Légende : **M** mesuré (reçu nommé, [G4] ou [loc] pour le codespace), **E** estimé (arithmétique sur des mesures,
méthode donnée), **C** conjecturé (mécanisme plausible, non mesuré).

## Sources (abréviations)

| Abr. | Source (lecture seule) |
| --- | --- |
| AB7 | v11 `receipts/developpement_20261003/pipeline_g4/sessions/claudeab7/results.tar.gz` (origin/main) : moteur `b87285378`, mode 16379, cinq prises W48 et une W1 par trame ; `files/t_new_lidar_ng0*_w*_r*.stdout` relus par moi ; `files/perf_new_self.stdout` (profil W1 de ng00, 9,62 s d'échantillons) |
| PROF1 | v11 `.../sessions/claudeprof1/results.tar.gz`, `report_w1_children.stdout` (base `a45daff3a`, W1, ng00, 10,93 s d'échantillons, parts inclusives) |
| S1 | v10 `receipts/g4_session1_20260929/results/cmd/008_c0_lidar02_k5_w1/stdout` (tour à 1 fil, trame 02) |
| S4 | v10 `receipts/g4_session4_j2c_20260929/results/cmd/0{08..13}_tower_*/stdout` (`777406b82`, W48, trois passes par processus) |
| TV2 | v10 `docs/conception/TOWER_v2.md` (conception de la tour v10) ; spike `build/v10-persist/design/tower_v2_spike/` |
| L06, L07, L08 | `build/v11-persist/audit_v10/L06_CODE_TOUR.md`, `L07_CODE_TETE_CLI.md`, `L08_TESTS_PORTES.md` et leurs `preuves_*` |
| CT | `build/v11-persist/conception/CONCEPTION_TOUR.md` (conception privée de la tour v11, 2 octobre) et `preuves_tour/noyau_v11.{cpp,log}` |
| PR | `build/v11-persist/conception/PISTES_DE_RUPTURE.md` |
| NV, Q100 | v11 `receipts/audit_dialogues_20261004/NOTE_CLAUDE_AUDIT_V11_20261003.md.snapshot` ; `audits/QUESTION_CLAUDE_VITESSE_100MS_20261004.md` (commit `d597ed9ba`, 12 h 15 UTC aujourd'hui) |
| LC | v10 `receipts/test_cover_C_20260929/README.md` (lot C préenregistré, G4) |

Code v11 lu sur origin/main `d597ed9ba` (dernier commit touchant `src/` : `3bd4d734e`, correctif P1 de l'attente du
pipeline, sans effet sur les chemins chauds discutés ici). Lignes v11 citées par `grep -n` sur ces fichiers.

## 0. Verdict

1. **Les pertes de la tour v11 sur la v10 sont à travail logique égal et se logent dans trois postes mesurés.**
   (a) Le pas de descente : résolution 2 918,5 ms contre 851,7 ms à un fil sur la même trame 02 (×3,43) pour
   4,89 M contre 4,53 M pas [G4, AB7 contre S1]. (b) La publication séquentielle par ordre : ordre 5 en 46,0 ms
   contre 28,4 ms pour le Kruskal par lots de la v10 (×1,62, même trame, un fil) [G4], qui laisse à W48 une queue
   médiane de 21 à 34 ms après la dernière résolution [G4, AB7]. (c) Le census des descentes : ≈ 3,9 µs par appel
   contre ≈ 1,1 µs pour la boule fermée de la v10 [E, § 2].
2. **Ces trois postes sont exactement ceux que la conception de la tour v11 (CT, 2 octobre), tirée de TV2 et de
   L06, avait déjà traités par des décisions écrites, prototypées et en partie mesurées** : forêt sans lots (D-F1,
   prototype `noyau_v11.cpp`, forêt et numérotation identiques, ×2,5 à ×2,9), recensement aux k plus proches
   (D-G4) avec banc contre le k-d de la v10 (M5), semis compact à empreinte additive (D-G2), arrêt à la première
   cellule (D-G1), ancres Merkle v10 (D-M2). **D-F1, D-G1, D-G2, D-G4 et D-M2 ne sont pas dans le code v11** ;
   D-F2, D-I1 et D-V1 y sont sous une autre forme (§ 3, vérifié fichier par fichier). Le transport utile n'est donc
   pas une idée neuve : c'est l'implantation de décisions déjà conçues, que les mesures G4 du moteur actuel
   justifient maintenant chiffre à l'appui.
3. **Neuf idées retenues** (§ 3), dont six de vitesse. Gain cumulé estimé sur FULL K = 5 à W48 : de l'ordre de
   −70 à −110 ms sur ng00 (non additif), soit FULL ≈ 300–340 ms et forêts ≈ 85–110 ms, c'est-à-dire **le niveau de
   la tour v10, pas 100 ms**. Le contrat ne se joue pas dans la tour seule : la passe unique du catalogue
   (167–180 ms à W48) dépasse à elle seule le budget. À K = 10 (jamais mesuré en v11), les idées 01 et 02 lèvent
   chacune un poste estimé à 100–190 ms à W48.
4. Hors vitesse : un outil de test qui manque et coûte peu (ancres Merkle v10 sur trames entières, idée 03), des
   fixtures de longues descentes qui doivent précéder toute retouche du census (idée 08), et un bras témoin
   MR_k-bord pour attribuer les gains de E1 (idée 09).

## 1. Méthode et périmètre

- **v10 lue** : `src/tower/tower.cpp` (descente `resolve` l. 799–993, mémo l. 951–973 et 991, Kruskal par lots
  l. 1041–1116, pointeurs de saut l. 721–739, semis par ordre l. 743–759 et 1403–1435, étage G par lots de 32
  l. 1441–1494, lancement du Kruskal l. 1498–1516, index plat `FlatIndex` l. 78–128) ; `src/cloud/site_tree.cpp`
  (construction l. 26–61, `nearest` l. 130–181, `closed_ball` l. 183–227) ; `src/head/`, `src/points/`,
  `cli/mhgp10_tower.cpp`, `cli/mhgp10_cluster.cpp`, `tests/head/mreach.hpp` ; `PASSATION.md`, `README.md` ;
  `docs/conception/TOWER_v2.md` (§ 6, 11, 12, 13, 14, 16, 17) ; `audits/tete_multik_20260929/RAPPORT_JUGE.md` ;
  reçus S1, S4, LC, `receipts/audit_continu_20260929/persistent_anchor_stream_20260930/README.md`.
- **build/v10-\*** : `v10-persist/gpu_design/` (juge et trois conceptions, côté catalogue), `v10-persist/design/
  tower_v2_spike/`, `v10-verrou-points/juge_final/` (plan de port d'ER0h), `v10-lidar-demos/` (harnais, sonde
  K HDBSCAN), `v10-batteries-iou/`, `v10-audit-full-hierarchy-20261002/`.
- **Audit v10 du 2 octobre** : L06 en entier, L07 § 0, L08 § 0 et § 5.6, preuves `krbench/`, `isometrie/`,
  `oracles/`.
- **Conception v11** : CT (§ 0–5, 9–11, annexes), PR § 3–4, `preuves_tour/noyau_v11.log`.
- **v11** : `src/tower/{forest_parallel,forest_pipeline,forest_plateau,forest_internal,population_lookup,descent,
  locate,canonical,full_domain}.*`, `src/index/{index.hpp,build.cpp,census.cpp,census_workspace.cpp}`,
  `src/core/buffer.cpp`, `src/sched/sched.hpp`, `bench/full_probe.cpp`, `bench/points_export.cpp`,
  `bench/points_radius.py` (en-tête), `docs/{FULL_FORESTS,PERFORMANCE_FULL,HIERARCHIE_POINTS,SORTIE_PLATE,
  AUDIT_V10_SYNTHESE}.md`, `reference/README.md`, `plans/e1_prereg_*.json`, NV, Q100, l'audit de l'auditeur
  `AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md` (version du 4 octobre) et son reçu `audit_heritage_20261004`.
- **Calculs** : petits scripts Python sur les JSON d'AB7, PROF1, S1 et S4, dans le scratchpad de la session ; les
  valeurs reprises sont recopiées ci-dessous avec leur origine.

## 2. Où la tour v11 perd : mesures appariables

### 2.1 Même trame (08/000200), K = 5, un fil, G4

| Grandeur | v10 (S1) | v11 (AB7, `t_new_lidar_ng02_w1_r0`) | Rapport |
| --- | ---: | ---: | ---: |
| résolution (`t_resolve` ; `phases.regular_ns`) | 851,7 ms | 2 918,5 ms | ×3,43 |
| pas de descente, tous ordres | 4 532 640 | 4 889 688 | ×1,08 |
| pas d'ordre ≥ 2 hors semis (v10 : MEB ; v11 : pas sans succès de table) | 976 190 | 1 039 332 | ×1,065 |
| arrêts sur semis (v10) ; succès de table d'ordre ≥ 2 (v11) | 3 318 856 | 3 612 483 | +293 627 |
| arrêts sur le mémo par cellule (v10) | 290 510 | — | |
| boules fermées (v10, juge 1/32 compris) ; census (v11) | 211 457 | 208 111 | ×0,98 |
| tests de points par census (v11) | — | 70,9 (14 758 563 / 208 111) | |
| Kruskal par lots (v10) ; publication (v11), ordres 1 à 5 | 5,3 / 9,6 / 14,3 / 20,0 / 28,4 ms | 13,7 / 23,5 / 29,0 / 36,6 / 46,0 ms | ×1,6 à ×2,6 |
| verticales (v10) ; balayage (v11), ordres 2 à 5 | 3,9 / 9,1 / 13,2 / 18,5 ms | 23,7 / 27,6 / 32,7 / 39,0 ms | ×2,1 à ×6,1 |

Lecture **M** : même objet, même travail logique à quelques pour cent ; les 293 627 succès de table de plus en v11
sont, à 3 117 près, les 290 510 arrêts mémo de la v10. L'écart est un coût par unité.

### 2.2 Composition de la résolution régulière v11 (ng00, W1)

Parts du CPU du processus, AB7 (`b872`, 9,62 s, parts propres) sauf mention PROF1 (`a45daff3a`, 10,93 s, parts
inclusives) ; temps = part × total ; « par unité » = temps / compteur d'AB7 (ng00 W1 : 3 419 932 traces d'ordre
≥ 2, 1 175 096 pas sans succès de table, 291 515 census, 21,15 M tests de points).

| Poste | Part | Temps | Par unité | Statut |
| --- | ---: | ---: | ---: | --- |
| census `CensusWorkspace::query` (inclusif, PROF1) | 10,32 % | 1,13 s | 3,87 µs par census | M, division E |
| pas sans succès de table `descend_each_step` (inclusif, PROF1) | 24,73 % | 2,70 s | ≈ 2,3 µs par pas | M, division E |
| voie liée : `bound` 2,91 + `resolve_job` 2,64 + `prefetch` 0,56 | 6,1 % | 0,58 s | ≈ 170 ns par trace | M, division E |
| sondes dans les pas : `hit` 1,07 + `find` 2,28 ; `find_support` 2,73 ; `birth_node` 1,00 | 7,1 % | 0,68 s | ≈ 300, 210 et 100 ns | M, division E |
| machinerie du pas : `descend_each_step` propre 2,05, `visit_located_part` 0,94, `descent_step` 0,40, `add_descent` 0,67, `cell_add` 0,47 | 4,5 % | 0,44 s | ≈ 370 ns par pas | M, division E |
| MEB bornée (inclusif, PROF1) | 2,58 % | 0,28 s | | M |

Équivalents v10 (L06-05, parts `rdtsc` d'une copie instrumentée, un fil, local ; appliquées au temps G4 de S1) :
boule fermée 26,3 % des cycles de `resolve`, soit ≈ 1,06 µs par boule fermée ; semis 15 %, soit ≈ 30 ns par
sonde ; CT § 3.1 donne 4 600 cycles par boule fermée et 180 cycles par sonde de semis (local). Statut **E**,
fragile (parts locales d'une trame, temps G4 d'une autre).

### 2.3 Queue de publication à W48 (AB7, quinze prises, mêmes sorties octet pour octet)

| Trame | `phases.publish_ns` (ms), prises r0 à r4 | Forêts (ms) | Résolution (ms) |
| --- | --- | --- | --- |
| ng00 | 36,2 / **0,9** / 39,6 / 33,5 / **1,1** | 172,2 / **137,1** / 188,7 / 171,0 / **137,7** | 115,8 / 115,2 / 126,9 / 117,6 / 116,6 |
| ng01 | 29,7 / 20,7 / 32,5 / 14,9 / 20,8 | 133,0 / 135,9 / 135,7 / 120,3 / 130,3 | 86,4 / 93,5 / 86,6 / 86,6 / 85,1 |
| ng02 | 23,9 / 16,7 / 45,0 / 34,1 / 38,7 | 146,0 / 137,4 / 168,1 / 157,7 / 169,9 | 96,4 / 96,6 / 99,5 / 99,3 / 105,8 |

**M** : treize prises sur quinze ont une queue de 14,9 à 45,0 ms, portée par les publieurs des ordres hauts
(la plus longue est celle de l'ordre 5 dans dix prises, de l'ordre 3 ou 4 dans les trois autres, souvent à quelques
millisecondes de l'ordre 5 ; champ `orders[k].timings.plateaus_ns`) ; deux prises
de ng00 n'en ont pas (≈ 1 ms) et leurs forêts tombent à 137 ms pour une résolution identique. La queue n'est donc pas du
travail obligatoire : c'est le publieur séquentiel de l'ordre 5 qui ne suit pas. La qualification `c40` mesurait ce
publieur seul, en étage séparé à W48 : 51,5 / 35,7 / 47,2 ms (`CARTE_V11.md` § 2.3, reçu [Q]).

## 3. Idées retenues

Ordre : par rendement estimé vers 100 ms, puis outils. Chaque fiche dit la source exacte, l'état dans la v11, la
doctrine, le gain et son fondement.

### v10_tour_01 — Forêt d'un ordre sans lots : noyau union-find minimal, matérialisation parallèle, historique d'attache

- **Sources.** TV2 § 6.5 (l. 497–604 : noyau T2a, matérialisation cartésienne T2c), décision D-T5 (l. 1465),
  § 11.1–11.3 (l. 975–1029 : ordres par K décroissant, noyau recouvert par l'étage G), § 17.1 (l. 1517–1568 :
  spike `pdendro.cpp`) ; L06-04 (l. 338–383) et `preuves_l06_code_tour/krbench/` ; CT D-F1, D-F2, D-F3, D-V1
  (§ 0.1, § 4.1–4.5, § 5, annexes A.4–A.6) et `preuves_tour/noyau_v11.{cpp,log}`. Origine v10 : le Kruskal par
  plateaux `tower.cpp` l. 1041–1116, dont TV2 avait mesuré le coût et prescrit le remplacement, jamais implanté.
- **Mécanisme.** Un fil par ordre parcourt les jonctions dans l'ordre des rangs et fait des unions arête par arête
  (union par taille, demi-compression), sans lot et sans nœud : il écrit des événements binaires (rang, deux
  opérandes, plus petite feuille), l'attache de chaque perdant et le sommet de chaque jonction. Les multifusions
  naissent ensuite en parallèle, par contraction des événements liés de même rang (théorème T4 de CT, forme
  événementielle de la règle cartésienne, théorème J de L02) ; numérotation (rang, plus petite naissance), celle
  de la v11. Les requêtes « composante vivante à la coupe fermée r » passent par l'historique d'attache (lemme T5,
  profondeur ≤ log2 des naissances) ; l'image verticale d'une fusion devient une requête indépendante (lemme T6
  pour les naissances), ce qui supprime le balayage séquentiel par ordre.
- **Preuves.**
  - [loc, M] `noyau_v11.log` sur les vrais vidages du Kruskal de la trame 00 : forêt **et** numérotation identiques
    aux ordres 1 à 5 et 10 ; noyau 2,49 à 2,90 fois plus rapide que le Kruskal par lots (ordre 5 : 47,6 → 18,2 ms ;
    ordre 10 : 140,9 → 54,2 ms) ; 2 444 942 images de jonctions calculées de deux façons, 0 écart ; 1,33 à 1,73
    saut d'attache en moyenne (8 au plus), 4,5 à 12,8 sondes de dichotomie.
  - [loc, M] `krbench` (L06) : noyau nu ÷2,7 (31,7 → 11,8 ms, ordre 5), noyau avec multifusions à la volée ÷1,6 ;
    ordre 10 : ÷2,6 et ÷1,6 ; forêt identique. TV2 § 17.1 : noyau T2a 6,6–9,2 ms contre 27,4–46,9 ms (K = 5) et
    15,9–22,3 contre 76,8–127,5 ms (K = 10), égalités vraies dans toutes les passes, y compris sur des plateaux
    grossis où le Kruskal par lots monte à 116–120 ms et le noyau reste à 7,5 ms.
  - [G4, M] § 2.1 : la publication v11 est 1,6 à 2,6 fois **plus lente** que le Kruskal par lots qu'elle remplace,
    donc ≈ 4 fois plus lente que le noyau prototypé. § 2.3 : queue médiane 33,5 / 20,8 / 34,1 ms, absente dans
    deux prises.
- **Dans la v11 ?** Non. `forest_plateau.cpp` : `regular_cell` (l. 100–120 : trois additions vérifiées, `find`,
  `touch`, `unite_roots` par graine) et `close` (l. 69–98 : tri par tas des racines touchées, parcours des chaînes,
  création des nœuds) ; une tâche de publication par ordre (`forest_pipeline.cpp` l. 73–84) ; blocs de résolution
  triés par première boule, tous ordres mêlés (l. 160–164) ; verticales par balayage DSU suivi (`follow_verticals`).
  Aucune occurrence de « sans lots » ni de « contraction des plateaux » dans `docs/`, `audits/` et `src/` de la v11.
  Ni Q100 ni le reçu `audit_heritage_20261004` ne traitent la publication.
- **Gain attendu.** K = 5 : noyau d'ordre 5 ≈ 11–13 ms au lieu de 41,8–46,0 ms à W1 [E : Kruskal v10 G4 27,5–31,7 ms
  ÷ 2,6] ; matérialisation 1,5–3 ms et verticales 2–4 ms en parallèle à W48 [E, CT § 4.5 et § 5]. La queue tombe
  vers 1–5 ms : **FULL médian −15 à −30 ms** [E ; borne haute appuyée sur les prises sans queue, M]. K = 10 : la
  publication d'ordre 10 serait de 119 à 146 ms séquentiels en v11 [E : 90,3 ms v10 × 1,32–1,62] et le balayage
  d'ordre 10 d'environ 100 ms [E : 49,6 ms v10 × 2,1] ; le noyau en ferait 20 à 35 ms recouverts [E, CT § 9]. C'est
  le premier plancher séquentiel de K = 10 qui tombe.
- **Doctrine.** Entier, aucun flottant ; sorties identiques octet pour octet (porte d'équivalence contre la voie
  actuelle, gardée comme référence, comme TV2 § 13.3 garde « lotkruskal ») ; aucune structure globale interdite.
  Inscrire T4–T6 au registre : la ligne « contraction des plateaux par composantes fortement connexes »
  (`docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md` l. 240) est encore `proof_obligation`, et TV2 présente sa preuve
  PO-T18 (l. 801, § 9.2) comme son remplaçant, sans que le registre ait été mis à jour. Mutants de TV2 § 13.3 : `mat_no_plateau_grouping`,
  `kernel_junction_on_head`, `mat_successor_max`, plus les trois fautes que la porte v10 laissait passer (L06-03 :
  multifusion binarisée, image verticale ouverte, attache ouverte).
- **Piste fermée ?** Non (v5 `PISTES_FERMEES.md` l. 49–52 ne ferme que le « fold » legacy comme témoin ; le
  Kruskal à lots y survit comme oracle).
- **Mesure préalable bon marché** [C] : publier le temps CPU du fil (`CLOCK_THREAD_CPUTIME_ID`) de chaque tâche du
  pipeline à côté de `finished[t]` ; si le publieur d'ordre 5 est affamé (fil SMT partagé avec un résolveur), une
  réservation de cœur réduirait déjà une part de la queue avant le nouveau noyau.

### v10_tour_02 — Census des descentes aux k plus proches, sur un index serré (structure du `SiteTree` v10)

- **Sources.** v10 `src/cloud/site_tree.cpp` l. 26–61 (coupe médiane sur l'axe le plus étendu, boîtes entières
  serrées, feuilles de 16), l. 130–181 (`nearest` : parcours du plus proche d'abord, borne qui se resserre),
  l. 183–227 (`closed_ball`) ; `tower.cpp` l. 897–934 (saut aux k plus proches) ; L06-05 (l. 385–441) ; TV2 § 6.2
  (`knn_closed`) ; CT D-G4, D-I1, D-I2, § 3.5–3.6 et banc M5 (§ 11).
- **Mécanisme.** Remplacer la requête saturante actuelle (arbre binaire sur les rangs Morton, boîtes réunies de bas
  en haut, parcours préfixe avec pointeurs d'échappement, arrêt aux k premiers intérieurs rencontrés) par le contrat
  « k plus proches » de CT § 3.5 : saturé, rendre les k plus proches du centre (clé exacte puis `SiteIdx`) ; complet,
  rendre I et toute U ; sur un arbre k-d à coupes spatiales et boîtes serrées, parcouru du plus proche d'abord.
  Élagage par les bornes exactes actuelles ou par une borne flottante certifiée à sens unique (D-I2, PO-I1) ;
  toutes les décisions sur les sites restent exactes.
- **Preuves.**
  - [G4, M] AB7 : 291 515 census et 21,15 M tests de points sur ng00 (72,5 par census ; 78,5 à l'ordre 5) ;
    PROF1 : census 10,32 % du CPU W1, soit **3,87 µs par appel** [E] ; NV A6 : ~30 bornes par requête.
  - [G4, M] NV § 4 : le filtre F6 des signes de `power` et de ses bornes, conforme, ne gagnait que ≈ 1 % du CPU :
    l'arithmétique n'est pas le coût ; c'est le parcours (nœuds et sites visités).
  - [E] v10 : ≈ 1,06 µs par boule fermée (§ 2.2) alors qu'elle rendait **toute** la boule fermée (10,1 sites en
    moyenne, L06-05). CT § 3.6 estimait l'index Morton implicite à 1 000–1 500 cycles par requête ; l'index implanté
    coûte ≈ 3,9 µs, soit de l'ordre de 15 000 cycles [E] : dix fois l'estimation, trois à quatre fois le k-d de la
    v10. Le banc M5 qui devait départager (« index retenu si au moins 2 fois moins cher que le k-d ») n'a jamais
    été joué.
  - [loc, M] L06-03, mutant `JUMP_ANY` : des témoins intérieurs quelconques au lieu des k plus proches coûtent
    +3 % de MEB et +7 % de sauts (sorties identiques).
- **Dans la v11 ?** Non. `index/index.hpp` l. 91 : « Ni census pondéré ni K plus proches » ; `index/build.cpp`
  l. 54–71 (coupe au milieu des rangs Morton, boîtes réunies) ; `index/census_workspace.cpp` l. 47–64 (parcours
  préfixe, saturation aux premiers trouvés). Q100 point 4 pose exactement la question (voie (b), k-d sous F1–F6) ;
  les extrema q2 couplés de l'auditeur (`audit_heritage_20261004`, 17 → 13 bornes dans son modèle, tests ponctuels
  inchangés) sont complémentaires, pas un substitut.
- **Gain attendu.** K = 5 : census ÷2 à ÷3,6 → −0,56 à −0,82 s de CPU W1 sur ng00 → **−19 à −28 ms** sur la
  résolution à W48 [E : accélération W1 → W48 mesurée ×29,6 sur ng00, AB7], plus −3 % de MEB et −7 % de sauts
  par la politique des plus proches [E]. K = 10 : la v10 faisait 1,83 M boules fermées sur 00 ; à 3,9 µs, ≈ 7 à 8
  CPU·s, soit ≈ 240–260 ms à W48 ; à 1,06–1,1 µs, ≈ 65–75 ms : **−165 à −190 ms** [E, même accélération ×29,6
  supposée, et nombre de census v11 supposé égal à celui des boules fermées v10, comme à K = 5].
- **Doctrine.** Pas de marge flottante figée (N1 de la carte v10 : `kMargin = 0,02` de `site_tree.cpp` l. 64 ne se
  porte pas) ; le contrat « k plus proches » rend les témoins indépendants de la forme de l'arbre, donc les ledgers
  stables quand l'index change, et il donne la politique de saut de la v10. Construction séquentielle du k-d :
  3,4–4,6 ms sur G4 en v10 (CT § 9, L04 C10) ; à paralléliser ou à remplacer par une variante Morton serrée si M5
  la donne aussi bonne.
- **Coût.** Moyen. D'abord le banc M5 : vider les ≈ 0,29 M sphères hors catalogue de ng00 et chronométrer
  l'index actuel, le k-d v10 porté en décisions exactes, et la variante nearest-first. **Après** l'idée 08.
- **Piste fermée ?** Non.

### v10_tour_03 — Ancres v10 et juges d'échelle : différentiel canonique v10/v11 sur trames entières

- **Sources.** L06 constat 01 (l. 213–265) et § 9.4 ; `preuves_l06_code_tour/isometrie/tower_merkle.py`
  (empreinte de Merkle indépendante de la numérotation : niveaux réduits, hachages d'enfants triés, verticales) et
  `anchors.jsonl` (empreintes de la v10 `afb081774` : forêt et verticales par ordre, arités des fusions, naissances,
  nœuds, niveaux distincts ; K = 5 sur les trames 00, 01, 02 et **K = 10 sur 00**) ; `oracles/emst_judge.py` et
  `emst_lidar00.log` ; L08 § 5.6 (`euler_echelle.py`, `euler_general.py`) ; TV2 § 13.4 ; CT D-M2 (« campagne
  appariée possible dès le premier jour ») et D-Q1.
- **Mécanisme.** Calculer la même empreinte sur les vidages FULL de la v11 (un convertisseur sur
  `tests/tower/full_v10_codec.py` suffit) et la comparer aux ancres ; ajouter le juge EMST de l'ordre 1 (Delaunay
  Qhull, poids entiers, Kruskal scipy) aux tailles 8 000, 16 000, 32 000 et sur les trames, et l'invariant d'Euler
  par ordre au catalogue.
- **Preuves.** [loc, M] ancres présentes ; EMST sur la trame 00 : multiensemble des niveaux de fusion × 4 égal à
  celui des d² de l'EMST (39 884 arêtes) ; Euler E_K = 1 pour K = 1 à 10 sur les trois trames (L08 § 5.6).
  [G4, M] les cardinaux v11 coïncident déjà avec les ancres sur ng00 (nœuds et naissances par ordre : 79 681 /
  39 885 … 576 371 / 341 081, AB7) : seule l'empreinte établirait l'égalité des objets.
- **Dans la v11 ?** Non à l'échelle. NV A5 : « Le différentiel canonique v10/v11 sur trames LiDAR entières reste
  ouvert … Seul témoin indépendant à l'échelle » ; `FULL_FORESTS.md` : différentiel v10 natif sur quatorze petites
  fixtures seulement ; aucune porte Euler ni EMST (`git grep` vide sous `tests/`, `bench/`, `cmake/`).
- **Gain attendu.** Exactitude, pas de temps : premier juge indépendant de la tour entière pour K ≥ 2 sur une trame
  du contrat, et **premier contrôle de K = 10** de la v11 sur une trame. Il protège les idées 01, 02, 04 à 07, qui
  ne doivent changer aucune sortie.
- **Coût.** Faible : ≈ 100 lignes de Python et un vidage par trame (session G4 ou exécution locale du
  développeur). Comparaison en O(n), conforme à la règle « jamais de vérification exhaustive ».

### v10_tour_04 — Semis compacts par ordre, empreinte additive et étiquettes de sondage : la localité des consultations

- **Sources.** v10 `tower.cpp` l. 78–128 (`FlatIndex` : étiquette de 32 bits et valeur dans un mot, la clé n'est
  relue que si l'étiquette concorde), l. 743–759 et 1403–1435 (une table de semis par ordre, `spop` de k mots par
  naissance), l. 1441–1494 (lots de 32 : empreintes calculées d'abord, cases puis populations préchargées),
  l. 1128–1147 (parcours à plat, ordre par ordre) ; TV2 D-T2 (l. 1462 : empreinte additive, celle d'un représentant
  vaut H(P_b) − h(u)) ; CT D-G2, § 3.3 et banc M3 ; PR R2.1 (§ 3.2).
- **Mécanisme.** (a) Une table par ordre, lignes de k mots plus le nœud ; (b) empreinte additive : l'empreinte d'une
  trace se déduit en O(1) de celle de la population de sa boule ; (c) résolution ordre par ordre pour que la table
  chaude tienne dans une tranche de L3 ; (d) une étiquette de 32 bits dans les cases de la table des supports.
- **Preuves.** [G4, M → E] § 2.2 : voie liée ≈ 170 ns par trace, `hit` ≈ 300 ns, `find_support` ≈ 210 ns,
  `birth_node` ≈ 100 ns ; la table v11 est **unique pour tous les ordres**, 44 229 728 octets pour 857 891 entrées
  (AB7), lignes de K + 3 mots ; `FullDomain::find_support` (`full_domain.cpp` l. 91–102) relit
  `balls_data()[ball].support` à chaque sonde, faute d'étiquette. [loc, E] v10 : ≈ 180 cycles par sonde de semis
  (CT § 3.1).
- **Dans la v11 ?** En partie : table I ∪ U → boule avec étiquette, voie liée, préchargement à trois étages
  (`forest_parallel.cpp` l. 142–167) ; mais une table pour tous les ordres, hachage multiplicatif de la partie triée
  (`population_lookup.cpp` l. 18, 176, 194), blocs dans l'ordre global des boules (`forest_pipeline.cpp`
  l. 160–164), table des supports sans étiquette.
- **Gain attendu.** [C] −0,3 à −0,5 s de CPU W1 sur ng00 → **−10 à −17 ms** à W48. À décider par le banc M3 de CT
  (« garder la disposition de base si l'écart est sous 15 % »).
- **Doctrine.** L'empreinte n'adresse que ; seule l'égalité exacte des sites décide (déjà la règle v11).
- **Lien.** La résolution ordre par ordre est l'ordonnancement par K décroissant de TV2 § 11.1 et CT D-F2 : elle ne
  vaut qu'avec l'idée 01 (sinon la queue passe aux ordres bas et aux balayages).

### v10_tour_05 — Arrêt à la première cellule de fenêtre et suivi de pointeurs : le mémo de la v10, rendu déterministe

- **Sources.** v10 `tower.cpp` l. 951–973 et 991 (mémo par cellule `atlas.val`, écrit par `atomic_ref`) ; S1 :
  290 510 arrêts mémo sur la trame 02 ; L06-05 ; CT D-G1 (§ 3.2, lemme T3, annexe A.3) ; PR § 3.1 (la descente v10
  est à 7–8 % du minimum de sa règle ; 67,5 % des premiers pas hors semis tombent sur une jonction du catalogue).
- **Mécanisme.** `resolve1` : la descente s'arrête sur la première cellule de fenêtre (b, k) qui n'est pas une
  naissance ; la cible est cette cellule, dont la valeur finale est celle de son premier représentant, suivie après
  coup (chaîne acyclique, rangs strictement décroissants), lue par la publication à la coupe ouverte de sa jonction
  et par les verticales à une coupe fermée de niveau au moins β(F).
- **Preuves.** [G4, M] § 2.1 : +63 142 pas sans succès de table en v11 (+6,5 %) et 293 627 succès de table de plus,
  qui remplacent les arrêts mémo de la v10. [loc, M] PR § 3.1 : v10 à 1,07–1,08 fois le minimum. Donc v11 ≈ 1,15
  fois le minimum [E].
- **Dans la v11 ?** Non. `descent.cpp` `DescentBuilder::run` (l. 133–148) continue par `strict_trace` sur toute
  cellule de fenêtre qui n'est pas une naissance ; `FULL_FORESTS.md` : « Aucun mémo, quota de chemin ou cache
  implicite » ; mémos de lane retirés de 16379 (2,6 % de succès, NV § 4). Q100 point 5 propose le mémo partagé
  daté : `resolve1` en donne le gain sans écriture partagée ni date à prouver, et avec des compteurs déterministes.
- **Gain attendu.** −13 à −15 % des pas sans succès de table (≈ 150 000 sur ng00) → −0,3 à −0,35 s de CPU W1 →
  **−8 à −12 ms** à W48 [E] ; moins si l'idée 02 a déjà réduit le coût du pas.
- **Doctrine.** Déterministe ; les dates initiales publiées ne changent pas (elles viennent du premier pas) ; les
  graines deviennent « naissance ou cellule » ; lemme T3 à inscrire au registre ; porte : graines finales égales à
  celles de la voie actuelle après suivi.

### v10_tour_06 — Un pas de descente sans ledger copié : compteurs locaux par lane, additionnés une fois

- **Sources.** v10 `tower.cpp` l. 799–993 : `ResolveCounters cnt`, incréments nus, `Facet` de 13 mots par valeur,
  aucun résultat intermédiaire ; carte v10, levier G4 (même principe dans les feuilles du catalogue) ; Q100 point 1.
- **Mécanisme.** En v11, `DescentLedger` compte 37 mots (296 octets, `descent.hpp` l. 13–22) ; chaque pas sans
  succès de table rend un `DescentStep` (≈ 400 octets) par `Result`, recopié dans `StepQuery::consume`
  (`descent.cpp` l. 161) et `current = next.value()` (l. 196), puis additionné par `add_descent` (copie de travail
  et 37 additions vérifiées, l. 18–70) dans `descend_each_step`, puis encore dans `resolve_job`
  (`forest_parallel.cpp` l. 106). Remplacer par un ledger de lane mutable, incréments nus bornés a priori (une lane
  compte moins de 2^40 événements), une seule addition vérifiée par lane ; ledger publié identique.
- **Preuves.** [G4, M → E] § 2.2 : 0,44 s de machinerie pour 1,18 M pas, ≈ 370 ns par pas.
- **Dans la v11 ?** Non (vérifié ci-dessus) ; Q100 ne le propose que pour le catalogue.
- **Gain attendu.** [C] moitié de la machinerie → −0,2 s W1 → **−5 à −8 ms** à W48. Petit, mais sans risque et du
  même contrat que Q100 point 1.

### v10_tour_07 — Processus résident et passes chaudes, arènes réutilisées entre trames

- **Sources.** v10 `cli/mhgp10_tower.cpp` l. 3–4, 42, 52, 88 (`--repeat=R`, passes chaudes dans le même
  processus) ; S4 (trois passes) ; CT D-M1 et banc M7 (« arènes neuves ou réutilisées … publier aussi la passe
  froide ») ; protocole froid et chaud demandé par le reçu `audit_deep_20261004`.
- **Mécanisme.** Garder processus et arènes d'une trame à la suivante (tampons dimensionnés une fois, pages déjà
  touchées ; grandes pages en option) ; publier côte à côte la prise froide et la prise chaude.
- **Preuves.** [G4, M] S4, catalogue + tour FULL à W48, passes 1 → 3 : 259,8 → 252,0 ms (−3,0 %), 218,6 → 204,2
  (−6,6 %), 265,5 → 253,6 (−4,5 %) à K = 5 ; −0,2 / −2,4 / −1,8 % à K = 10. [G4, M] profil W48 de la v11 : fautes de
  page 3,1 % du CPU en cumul, verrou noyau 1,55 %, `__pte_offset_map_lock` 1,51 % (PROF1, relevé par la carte v10
  § 3.3) ; les étages qui passent mal à l'échelle (préambule ×4, résidu du domaine ×2,4, contextes ×5) sont ceux qui
  touchent de grands tampons neufs (carte v11 § 2.2).
- **Dans la v11 ?** Non : `Buffer` alloue par `operator new` et rend par `operator delete` à chaque usage
  (`core/buffer.cpp` l. 32–48) ; `bench/full_probe.cpp` n'a pas de passes répétées.
- **Gain attendu.** [E] −3 à −7 % → **−11 à −29 ms** sur 352–412 ms ; [C] davantage sur les étages limités par le
  premier contact des pages.
- **Doctrine.** Mêmes sorties ; budget compté à l'identique (une réservation réutilisée reste comptée).
  **Décision de l'utilisateur** : le chiffre chaud ne vaut pour le contrat que si celui-ci est une latence par
  trame dans un flux ; sinon il se publie à côté du froid.

### v10_tour_08 — Fixtures de longues descentes : nuages « noyau serré et halo » et six nuages à 3–4 sauts

- **Sources.** L06 constat 01 (l. 221) et `preuves_l06_code_tour/oracles/oracle_sauts.py` avec ses journaux :
  noyau de 7, 9 ou 11 sites et halo épars, n = 14 ; 599, 593 et 459 nuages sur 600 sautent aux deux derniers ordres
  (K = 6, 8, 10) ; sept nuages jugés exhaustivement par Γ_k, 13 746 coupes, **0 écart**, 546 sauts dont 252 aux
  ordres 6 à 10. TV2 § 17.4 (l. 1589–1620) et `tower_v2_spike/long_descent_any.py` : six nuages de 10 à 13 points
  à 3 et 4 sauts, 23 387 contrôles conformes ; TV2 § 13.3 : le mutant `knn_from_support_site` survit à 15 des 16
  fixtures de la v1 et meurt sur les quatre nuages à longues descentes.
- **Dans la v11 ?** En partie. `reference/README.md` l. 93–95 : « Le régime des longues descentes à grand K … n'est
  qu'effleuré : 145 sauts sur la suite rapide … 1 256 sur six nuages de 24 à 32 points, où seul le binaire figé de
  la v10 confirme B » ; `reference/hgp11_ref/families.py` l. 248 : sauts seulement à K ≤ 3 sur les nuages larges.
- **Gain attendu.** Exactitude : porte nécessaire **avant** les idées 02 et 05, qui changent le chemin des
  descentes ; mutants de la politique de saut tués à K = 6 à 10.
- **Coût.** Faible : générateurs et coordonnées gravées, oracle Γ_k borné (n ≤ 14), exclu de la règle « pas de
  vérification exhaustive ».

### v10_tour_09 — Bras MR_k-bord dans E1 : séparer l'effet de l'entrée des points de celui de la hiérarchie

- **Sources.** v10 `tests/head/mreach.hpp` l. 1–25 (hiérarchie d'atteignabilité mutuelle exacte, entrée « bord » :
  x entre à min_y max(α² core2(y), |x − y|²) dans la composante de y) et porte `mhgp10_regression_mreach_border` ;
  LC l. 66–84 (famille « objet », préenregistrée, G4) : tour contre MR₂-bord à même entrée et même tête,
  Δ = −0,001 / −0,000 / +0,001 / +0,000 / +0,010 à K = 2 / 3 / 5 / 8 / 10 ; conclusion du reçu : « l'avantage sur
  HDBSCAN vient de l'entrée des amas discrets et de la tête (z), qu'on peut aussi donner à la hiérarchie
  d'HDBSCAN » ; L03 et `HIERARCHIE_POINTS.md` § 7 (MR₂-bord rattrape aussi le vélo C).
- **Dans la v11 ?** Non dans E1 : `plans/e1_prereg_lidar_20261004.json`, lignes T_eom1/2/3, T_leaf, A_*, R0, R0L ;
  le bras A (même tête sur l'arbre de `sklearn`, entrée cœur) attribue T − A à « la hiérarchie », alors que la v10
  l'a trouvé dû à l'entrée. `HIERARCHIE_POINTS.md` § 8 listait MR₂-bord pour E1 ; le préenregistrement l'a omis.
- **Gain attendu.** Interprétation : décomposer T − A en entrée et hiérarchie, témoin négatif conforme à la
  doctrine de `Zoltan/`. Sans lui, un gain de T sur R0 ne dit pas que la tour en est la cause.
- **Coût.** Faible (arbre de `sklearn` et règle de bord en Python, ou port de `mreach.hpp`). Ajouter en ligne
  **descriptive déclarée** avant toute lecture de cette ligne, sans toucher la famille primaire préenregistrée.

## 4. Fausses bonnes idées écartées

| Idée v10 | Pourquoi l'écarter | Source |
| --- | --- | --- |
| Mémo partagé par cellule tel quel (Q100 point 5) | gain borné : +6,5 % de pas en v11 sans lui (M), ≈ 5–7 % de la résolution (E) ; écritures atomiques relâchées et travail dépendant des fils ; date de validité à prouver ; l'idée 05 donne le même gain de façon déterministe | `tower.cpp` l. 951–991 ; § 2.1 |
| Kruskal par lots de la v10 et pointeurs de saut | 1,6 fois plus rapide que la publication v11 mais 2,6–3,2 fois plus lent que le noyau ; plancher séquentiel (N4 de la carte v10) ; à garder comme référence de porte | `tower.cpp` l. 721–739, 1041–1116 ; L06-04 |
| MEB proposée en double puis certifiée (DWelzl) | la MEB bornée v11 ne pèse que 2,6 % du CPU W1 (inclusif) contre 28,5 % des cycles de `resolve` en v10 : rien à gagner | `tower.cpp` l. 250–547 ; PROF1 ; NV § 4 |
| Boule fermée entière (`closed_ball`) | Θ(n) au pire : 1 258 sites sur une trame, la moitié du nuage sur une famille contrastée ; ne porter que la structure de l'arbre et le contrat des k plus proches (idée 02) | L06-05 |
| Marges flottantes figées u18 (`kApproxMargin`, `kMargin` = 0,02) | valables pour 18 bits seulement, arrondi supposé ; tout filtre seulement sous F1–F6 | `tower.cpp` l. 27 ; `site_tree.cpp` l. 64 ; N1 |
| Filtre F6 seul sur l'index actuel | mesuré ≈ 1 % du CPU : le coût est dans le parcours | NV § 4 |
| Jointure triée sur empreinte à la place de la table | « coûte autant de défauts de cache à la vérification » : garder une table compacte à empreinte additive (idée 04) | TV2 D-T2 ; CT § 3.3 |
| Ordres par K décroissant sans noyau rapide | déplace la queue vers les ordres bas, résolus en dernier (publications d'ordre 1 et 2 de 10 à 24 ms à W1, balayage d'ordre 2 de 17 à 24 ms, qui la suit) ; ne vaut qu'avec l'idée 01 | AB7 W1 par ordre (M) ; conséquence E |
| Option D&C de la forêt | 5 à 10 fois le travail du noyau, sans gain à W4 ; seulement si un GPU rend le noyau visible | TV2 § 6.5, § 17.1 |
| Étage G sur GPU | « Projection, pas promesse » : rien de mesuré ; hors `cpu_reference` ; à reprendre avec Q100 point 7 après les idées 02 et 04–06 | TV2 § 14 |
| Contrat sur un seul ordre (`only_order`) | change l'objet du contrat (FULL K1..5) ; utile au plus aux campagnes de points (≈ 20 % d'une campagne K = 10, E) | PR § 4 ; `tower.cpp` l. 1182–1188 |
| Entrée `cover` par plus petit indice | dépend du rang de Morton aux ex æquo (47 sites changent sous isométrie) ; la v11 publie l'ensemble | L06-02 |
| Code de la tête v10 (`src/head/head.cpp`) | condensation fausse aux cohortes, niveaux et EOM en flottant ; la v11 a déjà sa tête plate exacte | L07 § 0 ; `SORTIE_PLATE.md` |
| Têtes multi-K de la v10 (tranche oblique, persistance à travers K, antichaîne à ordres mêlés) | au mieux +0,0046 hors échantillon (dev, ARI) ; à retenir comme signal pour la piste « cohérence sur l'axe k » de la sortie plate : ne pas en attendre un grand gain ; non fermée (objectif LiDAR différent : fusions fugaces) | `audits/tete_multik_20260929/RAPPORT_JUGE.md` § 0 |
| ER0h (vote du juge final v10) | 125 cibles sur 125 mais aucune constante uniforme ; déjà écartée par la v11 | `HIERARCHIE_POINTS.md` § 6 |
| Remplissage `asc20_b2` de la tête v10-b | +0,003 à +0,009, sous la marge de 0,02 | `PASSATION.md` v10 |
| Numéroter les naissances par (rang, S*) pour éviter le tri des cohortes par centre | S* dépend du rang de Morton : non équivariant ; la v11 a choisi le centre | `FULL_FORESTS.md` |
| Appel groupé `--k-list` du CLI v10 | déjà couvert : `points_export.cpp` exporte plusieurs ordres d'un même FULL | `cli/mhgp10_cluster.cpp` ; `bench/points_export.cpp` l. 6 |
| Pliage chronologique des témoins de l'ancrage persistant (auditeur v10) | pas une idée nouvelle : c'est déjà la provenance de H^r_{k+1} ; il servira au port natif de la hiérarchie de points, pas au contrat FULL | `receipts/audit_continu_20260929/persistent_anchor_stream_20260930/README.md` ; `HIERARCHIE_POINTS.md` (Provenance) |

## 5. Ce que ces idées font au budget de 100 ms

ng00, K = 5, W48, prise médiane d'AB7 (r0, FULL 412,4 ms) ; gains **E**, non additifs sur la résolution (chaque
levier y réduit l'assiette des autres).

| Poste (ms) | Aujourd'hui, prise r0 (M) | Idées | Après (E) |
| --- | ---: | --- | ---: |
| résolution régulière | 115,8 | 02, 04, 05, 06 | ≈ 60–80 |
| queue de publication | 36,2 (médiane des cinq prises : 33,5) | 01 | ≈ 1–5, plus 3–7 de matérialisation et de verticales parallèles |
| contextes, classification, naissances | 20,1 | — (07 en partie) | ≈ 18–20 |
| forêts | 172,2 (médiane : 171,0) | | ≈ 85–110 |
| domaine (catalogue) | 239,8 | 07 en partie | ≈ 225–235 |
| **FULL** | **412,4** | toutes, avec 07 | **≈ 300–340** |

- Ces idées ramènent la tour v11 **au niveau de la tour v10** (88,5 ms sur 00 à W48, S4), pas en dessous ; la
  cible de CT (tour à 22–32 ms) supposait en plus un étage P à 4–6 ms et un étage G à 10–16 ms, jamais mesurés.
- Le contrat de 100 ms reste hors d'atteinte par la tour seule : la passe unique du catalogue (167–180 ms à W48)
  doit être divisée par trois à quatre (autres sources de l'audit), et le squelette séquentiel (≈ 20 ms de
  contextes et naissances, ≈ 20 ms de préambule) aussi.
- À K = 10, jamais mesuré en v11 : sans l'idée 01, une publication d'ordre 10 de l'ordre de 120–145 ms et un
  balayage d'environ 100 ms seraient séquentiels [E] ; sans l'idée 02, le census coûterait ≈ 240–260 ms à W48 [E].
  Mesurer K = 10 avec l'idée 03 comme juge avant toute conclusion.
- Ordre de travail proposé : 08 et 03 (portes), puis 01 (le plus sûr, prototype existant), M5 puis 02, M3 puis 04,
  05, 06 ; 07 et 09 en parallèle (protocole et analyse).

FIN
