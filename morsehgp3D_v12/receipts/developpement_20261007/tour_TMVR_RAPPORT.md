# Rapport de l'agent T, M, V, R et export FULL (tranche T2-b, voie CPU de référence)

Cadre :

```text
phase=exploration_v12_hors_registre
backend=cpu_reference
objet=full_pi0
quantification=quantized_u21_input_only (profils 21, 24, 32 compilés et testés)
public_status=not_claimed
GCP non utilisé
```

HEAD de base : `fff79a403447e1d467dc521299c285a0ceaa244d` (main, 7 octobre 2026).
Copie de travail : `v12_tour_TMV/repo` (git archive du HEAD).

## Journal (heures lues par `date -u`)

- 18:27 UTC : lecture des consignes ; copie du dépôt créée.
- 18:36–18:46 UTC : v11 gelée reconstruite dans `v11/` depuis l'archive épinglée (`source_v11.py`, 644 fichiers,
  code 0) et `mhgp12_vidage` construit dans `tour_build/` ; vidages MHGP12DP et MHGP11FUL1 de la v11 pour les neuf cas
  (`vidages/<cas>/`, trois fils, `--journal tous`, tous code 0). Les neuf FUL1 de la v11 redonnent les empreintes de
  `MESURE.md` § 4 (préfixes f87dbb19, 141bc7d6, a7563907, 3a2bfb4f, 5212a2ce, 78feb765, 61a4245b, 838a447e,
  81f89995). Les cellules des vidages v11 sont toutes des jonctions (au moins deux traces) : aucune cellule inerte.
- 18:47–19:03 UTC : module `src/tower/` (ma partie, fichiers `forest_*`, `vertical_*`, `export_*`) écrit et compilé
  au profil 21 sans avertissement : `forest.hpp` (types publics, registre `OrderForest`), `forest_internal.hpp`,
  `forest_births.cpp` (numérotation canonique, cohortes de même rang triées par `num::compare_centers`),
  `forest_kernel.cpp` (noyau T, événements de 20 octets, attaches, profondeur, CSR par survivant),
  `forest_contract.cpp` (contraction M en quatre phases par tranche), `forest_build.cpp` et `forest_stages.cpp`
  (orchestration sur le `Pool`, admission mémoire par étage), `vertical_images.cpp` (V : `LEM-T6`, `LEM-T5`,
  `component_at`), `forest_validate.cpp` (invariants globaux, naturalité), `export_full.cpp` (export FUL1).
  Raisons ajoutées en fin de table : `tower_capacity`, `tower_invariant`, `tower_query_domain`.
- 19:03 UTC : porte unitaire `mhgp12_tower_forest` (8 groupes, 16 989 contrôles) verte : hypergraphes aléatoires à
  rangs répétés et cibles « cellule » contre un Kruskal par lots (1 500 cas, 4 124 fusions d'au moins trois enfants,
  21 563 cibles « cellule »), témoins de plateau (WIT-SIX, WIT-TRI-EQ, REG:238), élément de cellule relu à la racine
  courante, cellule inerte, profondeur d'attache, domaine des opérandes (2^31 − 1 admis, 2^31 refusé avant
  allocation), `tower_query_domain`, carré K1..4 et remontée d'un cran, déterminisme (1, 3, 8 fils ; tranches 1, 97,
  65 536), refus transactionnels.
- 19:05–19:07 UTC : **`MES-M0` à l'octet, profil 21** : outil de test `mhgp12_tower_dumps` (adaptateur des vidages
  MHGP12DP de la v11 : naissances, cellules, traces, graines ; niveaux recalculés depuis S* de la première boule de
  chaque rang ; centres depuis S*) puis T, M, V, R et export FUL1. Les **neuf** cas redonnent les empreintes complètes
  de `MESURE.md` § 4, **dans les deux modes de cibles** : graines de la v11 (toutes « naissance ») et règle d'arrêt de
  la v12 dérivée des parties de descente (première plus petite boule qui est une naissance ou une jonction de l'ordre ;
  cibles « cellule » : 650 932 sur ng00 K5, 4 020 228 sur ng00 K10). 18 sur 18 identiques ; registre identique nœud par
  nœud à la forêt publiée par la v11 (FNODES, FEDGES, FLOWER) sur tous les ordres (0 écart). `LEM-T6` : remontées d'un
  cran 235 (ng00 K5) à 2 262 (ng02 K10) ; branche « naissance à k − 1 » 1 (ng01 K10) et 2 (ng02 K10). Trois fils,
  `taskset -c 0-2`. Bilan : `m0/u21_f3/bilan.txt`.
- 19:08–19:12 UTC : **déterminisme** : les neuf cas, deux modes de cibles, rejoués à 1 fil, 3 fils et 8 fils
  (`taskset -c 0-2` : huit fils logiques sur trois cœurs, consigne des trois fils), et à 8 fils avec des tranches de
  contraction de 97 événements (9 221 tranches sur ng00 K5, 45 329 sur ng00 K10, contre 16 et 71) : empreintes FUL1
  et compteurs de l'objet et du travail identiques sur les 18 jeux (`outils/determinisme.py`, 0 écart).
- 19:12–19:19 UTC : lecteur strict adapté `tests/tower/full_reader.py` (port de `bench/full_semantic.py` de la v11 à
  l'identique, profils 21, 24 et 32 ; Morton exact sur 3B bits) ; empreintes sémantiques des neuf vidages de la v11
  (`semantique/v11_*.json`) : u8000 `a45a9c64…`, u16000 `04a7b6bd…`, u32000 `5a3b2d36…`, ng00 K5 `fcca9460…`, ng01 K5
  `07277866…`, ng02 K5 `9741adbf…`, ng00 K10 `cda43901…`, ng01 K10 `7e568d4e…`, ng02 K10 `dd017552…`.
- 19:19–19:21 UTC : **oracle borné** vert (`mhgp12_tower_oracle_gate`, `tests/tower/oracle_tour.py` et l'outil
  `mhgp12_tower_oracle`) : 308 nuages sans doublon de la suite rapide et trois témoins (WIT-TRI-EQ, WIT-SIX à K2, carré
  K1..4), trois politiques de saut de la règle v12 de la référence, soit 933 cas ; cibles de `Reference.resolve_v12`
  (« naissance » et « cellule », cellules inertes comprises). Identiques à `Reference.order(k)` : nœuds, parents,
  enfants, racine, verticales, coupes ouvertes et fermées à chaque niveau d'événement ; vidage FUL1 de chaque cas relu
  par le lecteur strict puis comparé aux niveaux et centres exacts. Compteurs gravés : cibles « cellule » 3 487,
  cellules inertes 2 631, fusions d'au moins trois enfants 4 629, remontées de `LEM-T6` 105, images par naissance à
  k − 1 63, 27 faits (WIT-TRI-EQ ternaire au niveau 1/2 ; WIT-SIX deux ternaires simultanées aux ordres 1 et 2 ;
  carré K1..4 avec les verticales 4, 4, 0). 12 s ; identique sous `-O`, à 3 fils et tranches d'un événement.
- 19:22–19:27 UTC : **mutants causaux** (`tests/mutants/tower.json`, copies par `run_mutants.py`, jamais une branche
  du produit) : `plateau_non_atomique` (aucun lien de même rang : multifusions binarisées), `etoile_entiere` (REG:238 :
  la classe absorbe toute l'étoile des sommets touchés, rangs inférieurs compris), `union_par_indice`,
  `t6_sans_remontee`, `element_sans_racine`, `naissances_par_cle` (cohortes non triées par centre, CST-0107),
  `cible_de_meme_rang` (date de LEM-T3 non exigée) : **7 tués sur 7**, chacun par sa porte (`temoins`, `attache`,
  `verticales`, `refus`). Le premier jeu de `naissances_par_cle` ne se construisait pas (paramètre inutilisé) : motif
  corrigé, rejoué seul, tué. L'outil des vidages lit le format par l'en-tête des microbancs : il n'est construit que si
  ce dossier est présent (les copies de mutants ne le contiennent pas).
- 19:27–19:29 UTC : **point du contrat à corriger (§ 1, « le départage de S* par positions … ne le touche pas »)**,
  témoin minimal `outils/temoin_forme_niveau.py` : nuage (0,100,100), (10,100,100), (35,10,3), (27,14,3), (27,6,3),
  K = 2. Le niveau 25 est porté par la paire {(0,100,100), (10,100,100)} (forme q2 100/4) et par le triangle aigu
  (forme q3 409600/16384, comme `circle25_pair`). La v11 range à niveau égal par S* en rangs de Morton : le triangle
  ouvre le rang ; la v12 (T1) range par positions triées de S* : la paire ouvre le rang. Le vidage FUL1 écrit chaque
  niveau dans la forme NON réduite de la première boule de son rang (`levels[rank]`, `assembly_parallel.cpp` de la
  v11). Même registre, deux tables de niveaux : octets `a7ab024f…` (ordre v11) contre `11075c81…` (ordre des
  positions), empreinte sémantique identique `bead653d…`. Il en va de même des centres des naissances dont S* change
  (dénominateur D = 2|u×v|² ou 2|det| du support). **Conséquence** : avec le catalogue de T1 dans la chaîne,
  l'identité à l'octet de `MES-M0` exige soit que le catalogue de T1 garde, pour la table des niveaux et les supports
  des naissances, la forme de la v11 (première boule du rang dans l'ordre de Morton), soit que `MES-M0` se juge par
  l'empreinte sémantique (déjà le cas aux profils 24 et 32). À trancher par le développeur ; avec les graines et le
  catalogue de la v11 (cette tranche), les octets sont identiques.
- 19:40 UTC : messages du coordinateur. (1) Le développeur a corrigé le § 1 du contrat (`58d384721`) : `MES-M0` se juge
  par l'empreinte sémantique à tous les profils ; l'identité à l'octet avec `MESURE.md` § 4 n'est exigée que pour une
  tour nourrie par le catalogue ou les graines de la v11 (cette tranche) ; le témoin est gravé (`e87896702`,
  `reference/test_witness_forme.py`). L'exportateur écrit, à niveau égal, la forme de la première boule du rang dans
  l'ordre publié du catalogue reçu : c'est déjà le cas (`FullSource::levels`, table du catalogue, aucun réordonnancement
  propre). (2) Interface de l'étage G publiée (`v12_tour_G/INTERFACE.md`, lue sans rien modifier) : `ResolvedOrder`
  (naissances 0..births()−1 par clé, cellules par `BallIdx` croissants, jonctions et inertes, `cell_offsets`, cibles
  4 octets, bit 31 = cellule, indice de naissance dans la liste de G) : c'est exactement la convention de mon
  `ForestInput`. Décisions : types forts (`LevelRank`, `BallIdx`) dans `ForestInput` pour une adaptation sans copie ;
  noms du codage des cibles alignés sur G (`kTargetCellBit`, `kTargetIndexMask`, `kNoTarget`, `birth_target`,
  `cell_target`, `target_is_cell`, `target_index`, dans l'espace `mhgp12::tower`, à fusionner avec ceux de G dans
  `mhgp12`) ; raisons `tower_capacity` et `tower_invariant` dans un bloc séparé à fusionner avec celui de G ;
  rebasage sur `main` (le catalogue de T1 y est : `671072339`).
- 19:42–19:49 UTC : **rebasage sur `main` `b62196d74`** (catalogue de T1 présent) dans `repo2/` : mes fichiers copiés,
  `reasons.def` (bloc séparé `tower_capacity`, `tower_invariant` à fusionner avec celui de G, plus
  `tower_query_domain`), `tests/core/status_test.cpp` (table gravée des raisons : 27 lignes, dernière
  `tower_query_domain`), `cmake/modules.cmake` et `ARCHITECTURE.md` § 5 (`tower` après `catalogue`, dépendances
  `core num sched cloud io index catalogue` : celles de G plus `io` pour l'export). Types forts dans `ForestInput`
  (`LevelRank`, `BallIdx`), adaptateur sans copie `tower::forest_input(const ResolvedOrder&)` (gabarit sur les
  accesseurs publiés par G ; toutes les portes passent désormais par lui), `tower::catalogue_balls(const Catalogue&)`
  (supports S* lus en place dans le catalogue de T1). Nouvelles portes : groupe `catalogue` (carré K1..4 avec le vrai
  catalogue de T1, ordre publié AD avant AB : même registre et mêmes octets que l'adaptateur à la main en ordre de
  Morton) et `mhgp12_tower_forme_niveau` (témoin de bout en bout, attendus gravés : formes 409600/16384 contre 100/4,
  octets `a7ab024f…` contre `11075c81…` au profil 21, sémantique `bead653d…`). Tout rejoué vert au profil 21
  (unitaires 9 groupes, oracle 933 cas, ng00 K5 à l'octet).
- 19:50–19:53 UTC : **jonction G → T jugée localement** sur un instantané des sources de l'étage G (copiées en lecture
  depuis `v12_tour_G/repo`, empreintes dans `fusion/G_snapshot.sha256`, rien modifié chez G) fusionné avec mes fichiers
  dans `fusion/` (en-tête public de G suivi de mes en-têtes, raisons de G puis `tower_query_domain`, sources unies) :
  outil `mhgp12_tower_chain` (index → catalogue de T1 → `resolve_tower` → `tower::forest_input` → T, M, V, R →
  export). u8000 K5 : empreinte sémantique `a45a9c64…` égale à celle de la v11, et octets identiques à `MESURE.md` § 4
  (`f87dbb19…`) ; étages à 3 fils : catalogue 2,25 s, résolution 0,40 s, forêt 58 ms, empreinte 0,65 s. Porte
  `mhgp12_tower_chain_m0` (`mes_m0.py --chaine`, sémantique et 1 contre 8 fils) lancée sur les neuf cas.
- 19:52–20:47 UTC : **`MES-M0` sémantique de la chaîne complète du produit** (catalogue de T1 → étage G, instantané →
  T, M, V, R → export), porte `mhgp12_tower_chain_m0` (`mes_m0.py --chaine`) jouée sur les **neuf cas** au profil
  21, trois cœurs (`taskset`) : `mes_m0_chaine_ok profil=21 cas=9` — chaque vidage relu par le lecteur strict donne
  l'empreinte sémantique des vidages de la v11, et les sorties à 1 fil et à 8 fils (tranches de 97) sont identiques
  (empreinte et compteurs). C'est le critère de sortie de T2 (§ 10 du contrat), ici sur un instantané local de G ; à
  rejouer à la fusion (la porte s'enregistre seule dès que `src/tower/resolve.cpp` est présent).
- 20:47–21:15 UTC : **étape A** (repo2, profil 21, toutes les cibles construites) : unitaires (9 groupes), oracle
  (933 cas), forme des niveaux, `MES-M0` à l'octet 18 sur 18 avec contrôle de la forêt v11, déterminisme 1/3/8 fils et
  tranches de 97 : 0 écart. **Étape C** : `JUG-EMST` construit depuis `juges/emst` et joué par `mes_m0.py
  --juge-emst` sur l'ordre un des neuf vidages écrits par la v12 (`mes_m0_ok profil=21 cas=9 modes=1
  juge_emst=oui`) ; campagne de mutants sur la base rebasée : **7 tués sur 7** (`mutants_ok module=tower mutants=7
  tues=7`). Étape B (profils 24, 32) interrompue par PID au début de sa construction : le noyau change (ci-dessous).
- 21:15–21:19 UTC : **noyau T refondu** pour rester le seul passage séquentiel par ordre, comme le prototype : une
  pré-passe parallèle (morceaux de 4 096 cellules, tous ordres) traduit chaque cible en feuille canonique (naissance)
  ou garde son codage « cellule » et contrôle la date de `LEM-T3` ; le noyau ne lit plus que des feuilles
  séquentielles (préchargement à 16) et l'élément des cellules cibles. Rejoué : unitaires, oracle (933 cas), forme,
  ng00 K5 (cibles v12) à l'octet avec la forêt v11. Comparaison locale entrelacée, même cœur, machine très chargée
  (charge 17 à 19 sur 8 cœurs, chiffres gonflés, rapport seul indicatif) : noyau de l'ordre 5 de ng00 K5 18,4 à
  18,8 ms contre 27 à 60 ms pour le noyau de `MES-M4` ; pré-passe 7,7 à 13 ms (parallèle) ; contraction 23 à 39 ms
  et verticales 39 à 49 ms à un fil. Avant la refonte, le noyau lisait l'indirection naissance → nœud et les rangs à
  chaque représentant (1,8 fois `MES-M4` mesuré plus tôt sur une machine moins chargée).
- 21:19–21:31 UTC : **étape A2** après la refonte du noyau (profil 21) : `MES-M0` à l'octet 18 sur 18 (neuf cas,
  deux modes de cibles, forêt v11 identique nœud par nœud), déterminisme 1/3/8 fils et tranches de 97 : 0 écart sur
  18. Temps à un fil (`m0/u21c_f1`, minimum de trois prises) relevés sous une charge de 15 à 19 sur 8 cœurs : non
  représentatifs, voir le tableau final ; ajout `src/tower/forest_source_pins.json` (épingles des sources portées :
  `mes_m4.cpp` 320db4a12, `foret_check.py`, `full_probe.cpp`, `whole_input.hpp`, `full_semantic.py`,
  `catalogue_semantic.py` de la v11, `carre.py`), à fusionner avec `source_pins.json` de G.
- 21:40–21:46 UTC : message du coordinateur : l'étage G est intégré sur `main` (`99fa83246`, puis `9c5809919`).
  **Rebasage sur `9c5809919`** (`repo3/`, script `outils/rebase3.py`) : mes fichiers seulement, plus la fusion des
  fichiers partagés : `tower.hpp` (section de G inchangée, puis mes deux en-têtes), `module.cmake` (liste de G puis la
  mienne), `tests/tower/tests.cmake` (bloc de G puis le mien ; porte de la chaîne enregistrée sans condition),
  `tests/mutants/tower.json` (7 mutants de G puis mes 7, plancher 14), `reasons.def` (seule `tower_query_domain`
  ajoutée après le bloc de G), `tests/core/status_test.cpp` (30 lignes), `cmake/modules.cmake` et `ARCHITECTURE.md`
  § 5 (ligne `tower` : rôle complété, dépendance `io` ajoutée pour l'export — **nécessité déclarée** ; ligne « tower
  (suite) » retirée des modules prévus). Codage des cibles : celui de G (espace `mhgp12`) ; ma copie est retirée de
  `forest.hpp`, `forest_internal.hpp` inclut `tower/tower.hpp`, et `kMaxOrderItems` est vérifié égal à
  `kMaxOrderBirths` et `kMaxOrderCells` de G (`static_assert`). Aucun fichier de G réécrit. Profil 21 : construction
  complète sans avertissement (2 min), unitaires 9 groupes (17 056 contrôles), oracle 933 cas, forme des niveaux,
  `check_style` (287 fichiers), `check_constats` (73 constats), manifeste des mutants (14) : tout vert.
- 21:44–21:52 UTC : **suite rapide complète au profil 21 sur repo3** (`ctest -LE long -j3`) : **651 sur 651**
  (la sentinelle LiDAR est sautée : `MHGP12_DATA_DIR` absent ; les portes LiDAR de la tour sont jouées à part par
  leurs scripts). Portes de la tour comprises : celles de G (unitaires 9 groupes, oracle, échelles 8 000 / 16 000 /
  32 000) et les miennes (`mhgp12_tower_forest_*` 9 groupes et inventaire, `mhgp12_tower_forest_oracle_gate` et sa
  jumelle `-O`, `mhgp12_tower_forme_niveau` et sa jumelle). Patch généré contre `9c5809919` (`patch_tour_TMV.diff`,
  30 fichiers, +4 126/−6) : `git apply --check` conforme sur la tête de `main` `9b2747eff` (ses deux commits
  postérieurs ne touchent ni `src/`, ni `tests/`, ni `cmake/`, ni la table des modules), et l'application sur un
  export de `9c5809919` redonne exactement `repo3`.
- 21:52–21:59 UTC : `MES-M0` à l'octet sur repo3, deux modes de cibles, 1 contre 8 fils, et `JUG-EMST` sur l'ordre un
  des neuf vidages écrits : `mes_m0_ok profil=21 cas=9 modes=2 juge_emst=oui`. Message du coordinateur : pré-lecture
  de l'auditeur Codex (`receipts/audit_t1b_tour_prepublication_20261007/tour` et `branches`) : la première forme du
  registre R perd les **branches ouvertes** des hyperarêtes retenues (deux hypergraphes, cellules {0,1} puis {0,2}
  ou {1,2} au même rang : mêmes événements, même forêt, branches différentes) ; collecte proposée sans barrière de
  plateau ni nouvelle descente G ; A n'est pas borné par b − 1 (7 naissances, 6 événements, 27 branches). Base
  confirmée : `main` `9c5809919` ⊇ `99fa83246` (socle `7b7d025b3`, cache compté, compris). **Décision : collecte
  maintenant**, variante de raccord après M que le reçu admet (requête `component_at(l, r − 1)`, coupe ouverte),
  avec son temps R publié à part. Portes finales arrêtées par PID (le code change) ; à rejouer en entier.
- 21:59–22:03 UTC : **registre R complété** (`src/tower/registry_branches.cpp`, étage R après V) : hyperarêtes retenues
  par Kruskal (`retained_cell`, `retained_ball`, `retained_rank`, cellules qui ont uni au moins deux composantes, lues
  sur les blocs de `event_cell`) et leurs **branches ouvertes** (`branches`, CSR, une ligne par cellule retenue, nœuds
  croissants vivants à la coupe ouverte du rang). Témoin immuable par représentant (sa naissance canonique ; pour une
  cible « cellule », `minleaf` du sommet laissé par la cellule cible), puis `component_at(l, r − 1)` (variante de
  raccord après M admise par le reçu) ; comptage puis réservation exacte de la sortie (A non borné par b − 1), deux
  passes parallèles par morceaux de 2 048 lignes, temps R publié à part (`registry_ns`), compteurs du travail
  `branch_reads` (P_R) et `branches` (A). Validation globale : lignes d'au moins deux nœuds, croissants, vivants à la
  coupe ouverte. Témoins gravés (groupe `branches`) : les deux hypergraphes de l'auditeur (branches {0,2} et {1,2},
  même forêt), la famille {0,1}, …, {0,…,6} (6 événements, 6 cellules retenues, 27 branches), 300 hypergraphes
  aléatoires contre la coupe ouverte de la forêt de référence (3 112 lignes) ; mutant `branches_coupe_fermee` ajouté
  (plancher 15). ng00 K5 (cibles v12) : octets inchangés, ordre 5 : 235 307 cellules retenues, 770 782 représentants
  relus, 576 388 branches ; étage R 90 ms à trois fils sur la machine chargée. Batterie finale relancée sur repo3.
- 22:03–22:16 UTC : batterie finale sur repo3 (registre complet) : suite rapide complète au profil 21 **652 sur 652** ;
  `MES-M0` à l'octet, neuf cas, deux modes de cibles, 1 contre 8 fils, `JUG-EMST` sur l'ordre un : `mes_m0_ok
  profil=21 cas=9 modes=2 juge_emst=oui`.
- 22:16–22:28 UTC : campagne de mutants du module `tower` sur repo3 : **15 tués sur 15** (`mutants_ok module=tower
  mutants=15 tues=15`) : les 7 de G (`t1_sans_s_dans_f`, `arret_sous_la_fenetre`, `decroissance_apres_le_saut`,
  `cible_cellule_prise_pour_naissance`, `census_sans_seuil`, `table_s_etoile_muette`, `controle_croise_decale`) et
  mes 8 (`plateau_non_atomique`, `etoile_entiere`, `union_par_indice`, `t6_sans_remontee`, `element_sans_racine`,
  `naissances_par_cle`, `cible_de_meme_rang`, `branches_coupe_fermee`).
- 22:28–22:45 UTC : **chaîne complète du produit sur `main`** (catalogue de T1 → étage G intégré `resolve_tower` →
  T, M, V, R → export), `mes_m0.py --chaine`, neuf cas en trois lots : `mes_m0_chaine_ok` 3 + 3 + 3, chaque vidage
  relu par le lecteur strict donne l'empreinte sémantique des vidages de la v11, 1 contre 8 fils (tranches de 97)
  identiques, empreintes et compteurs. La jonction G → T est jugée sur la base fusionnée.
- 22:45–23:11 UTC : **profil 24** (repo3) : construction complète sans avertissement, suite rapide **652 sur 652** ;
  `MES-M0` sémantique sur les graines de la v11, neuf cas (trois lots) et cibles v12 sur ng00 K5 : vidages écrits au
  profil 24 (largeurs d'entiers du profil), relus par le lecteur strict adapté, empreintes sémantiques égales à
  celles des vidages de la v11, 1 contre 8 fils identiques (`mes_m0_ok profil=24`, quatre lignes).
- 23:11–23:42 UTC : **profil 32** (repo3) : construction complète sans avertissement, suite rapide **652 sur 652** ;
  `MES-M0` sémantique (neuf cas sur les graines de la v11, cibles v12 sur ng00 K5) : empreintes sémantiques égales à
  celles de la v11, 1 contre 8 fils identiques (`mes_m0_ok profil=32`, quatre lignes). **Batterie finale entière
  verte** (`final/codes.txt`, tous les codes à 0).
- 23:45–23:51 UTC : lecture de deux reçus de l'auditeur Codex déposés sur `main` après la base :
  `receipts/audit_registre_branches_20261007/` (corps R lu `9cc388d6…`, celui de repo3) : mathématiques et
  concurrence de R favorables ; **défaut d'admission** : `place_rows` alloue `branches.off` (`8·(R_k + 1)` octets par
  ordre, y compris un ordre sans cellule retenue) sans qu'aucune admission ne les couvre (la seconde n'annonce que
  `4·A`) : refus mémoire possible après une admission acceptée. Correctif proposé (`budget_proposed.patch`,
  non compilé) : **adopté**. `receipts/audit_tmv_traces_20261007/` : traces concordantes (652 tests, 15 mutants,
  MES-M0 9 × 2, chaîne 3 × 3) ; rappel : « chaîne sur `main` » voulait dire prototype sur la base `9c5809919`, pas la
  tête observée. Piste R de l'auditeur (traduire une fois les racines distinctes : −25 % de recherches d'historique
  sur ng00 ordre 5) : à mesurer, non adoptée ici (levier T2-c). Décisions : (1) correctif d'admission de R ;
  (2) les octets admis sont cumulés par étage dans l'état interne et une **porte mesure le pic de chaque étage**
  (T, M, V, R joués un par un, budget sans cache, tailles exactes) contre ses octets admis, sur des nuages réels
  par la chaîne catalogue → G → T/M/V/R, avec le mutant qui retire le correctif ; (3) `validate_forests` admet ses
  tampons avant de les allouer (règle du socle) ; (4) **rebase sur la tête de `main` `7e87b58d2`** (juge G durci
  `g_determinism.py` et porte `g_fausse_sonde.py` conservés tels quels), batterie entière rejouée.
- 23:51–00:00 UTC (7–8 octobre) : **repo4** = export de `main` `7e87b58d2` + `patch_tour_TMV.diff` (application sans
  conflit ; `g_determinism.py` de G = `650c63a1…`, le juge durci ; `tests.cmake` : portes `mhgp12_tower_juge_*` de G
  puis mon bloc). Changements : `admit_stage` (`forest_internal.hpp`) cumule par étage (`kStageT`, `kStageM`,
  `kStageV`, `kStageR`) les octets admis ; les sept admissions de T, M, V et R passent par elle ; seconde admission
  de R = `4·A + 8·Σ_k (R_k + 1)` (correctif de l'auditeur) ; `validate_forests` admet `max(naissances, 4 · nœuds de
  l'ordre inférieur)` avant ses deux tampons. Porte nouvelle `mhgp12_tower_forest_admission` (groupe `admission`) :
  40 nuages de 12 à 161 sites tirés dans un cube de 2^16, K de 2 à 5, chaîne index → catalogue de T1 →
  `resolve_tower` → T, M, V, R joués un par un sur l'état interne, budget sans cache : pic mesuré de chaque étage
  au-dessus de l'usage à son entrée ≤ octets admis ; registre identique à celui de `build_forests`. Premier passage :
  160 étages, aucun dépassement, 40 égalités exactes (l'étage R : formule exacte), 25 383 lignes du registre,
  685 contrôles. Mutant `admission_r_sans_decalages` (retire les décalages de la seconde admission de R) : **tué par
  code** (rejoué seul, 3 min). `check_style` 289 fichiers, `check_constats` 73 : verts. Batterie finale entière
  relancée sur repo4 (`outils/etape_final4.sh`, sorties `final4/`) à 23:59:56.
- 00:00–00:06 UTC : `main` avance à `7bde58876` puis `c903774b1` (catalogue appareil T1-b `8ba7d7287` et CST-0234
  `c903774b1` : raisons `device_unavailable` = 29 et `device_fault` = 30 ajoutées après celles de la tour, table de
  `status_test.cpp` à 31 raisons, plancher 104 ; juge G : modes `sortie_ordre_booleen` et `sortie_ordre_flottant`
  dans `tests/tower/tests.cmake`) ; le patch de 23:43 ne s'appliquait plus (`ARCHITECTURE.md`, `reasons.def`,
  `status_test.cpp`). Message du coordinateur : livrer sur `c903774b1`. La batterie repo4 (lancée à 23:59:56) est
  arrêtée par son groupe de processus à 00:03:49, pendant sa suite rapide, sans résultat perdu, et relancée entière
  sur la base de livraison. **repo5** = `git archive c903774b1` + le diff de repo4 (31 fichiers), trois rejets
  résolus à la main : `tower_query_domain` = 31, après `device_fault` ; table à 32 raisons, plancher 107
  (1 + 32 × 3 + 10), dernière raison `tower_query_domain` ; ligne `tower` d'ARCHITECTURE.md § 5 (rôle et `io`) sur le
  texte courant, ligne prévue « `tower` (suite) » retirée. Juge G durci (`g_determinism.py`, `g_fausse_sonde.py`,
  portes `mhgp12_tower_juge_*`) intact. **Patch** `patch_tour_TMV.diff` : base `c903774b1`, 31 fichiers,
  +4 566/−8, SHA-256 `6f0643acca9d6c27755f1e3dde15b166c810371eb88662a32360f6d5c2136de9` ; `git apply --check`
  conforme sur une extraction neuve de `c903774b1`, et son application redonne exactement repo5. Batterie
  `outils/etape_final5.sh` lancée à 00:05:54 (sorties `final5/`) : profil 21 (construction, suite rapide, MES-M0 à
  l'octet + JUG-EMST, chaîne neuf cas), mutants (16), profils 24 et 32 (construction, suite rapide, **chaîne G → T
  neuf cas**, que les reçus de l'auditeur notaient absente à ces profils, et MES-M0 sémantique ng00 K5 deux modes).
- 00:06–00:35 UTC : batterie sur repo5 (base `c903774b1`), profil 21 : construction complète sans avertissement ;
  **suite rapide complète 681 sélectionnés, 680 réussis, la sentinelle LiDAR sautée** (`MHGP12_DATA_DIR` absent), dont
  les portes de l'appareil du catalogue (voie hôte), les sept portes `mhgp12_tower_juge_*` de G et leurs jumelles,
  `mhgp12_core_unit_reasons` (107 contrôles, plancher 107) et `mhgp12_tower_forest_admission` ; **MES-M0 à l'octet**,
  neuf cas × deux modes de cibles, 1 contre 8 fils, JUG-EMST sur l'ordre un : `mes_m0_ok profil=21 cas=9 modes=2
  juge_emst=oui` ; **chaîne G → T** (catalogue de T1 avec la fin d'étage partagée de T1-b et CST-0234, `resolve_tower`,
  T, M, V, R, export) : `mes_m0_chaine_ok profil=21` trois lots de trois cas, empreintes sémantiques égales à celles
  des vidages de la v11, 1 contre 8 fils identiques.
- 00:35–00:49 UTC : campagne de mutants du module `tower` sur repo5 : **16 tués sur 16 par code** (aucun signal, délai
  ni échec de construction ; `mutants_ok module=tower mutants=16 tues=16`) : les 7 de G et mes 9, dont
  `admission_r_sans_decalages` (porte `mhgp12_tower_forest_admission`).
- 00:50–01:22 UTC : repo5, profil 24 : construction sans avertissement, suite rapide 681 (680 + sentinelle sautée),
  **chaîne G → T neuf cas conforme au profil 24** (`mes_m0_chaine_ok profil=24`, trois lots), MES-M0 sémantique
  ng00 K5 deux modes conforme ; profil 32 : construction, suite rapide 681 (680 + sentinelle) ; chaîne au profil 32
  interrompue (message suivant).
- 01:22–01:26 UTC : message du coordinateur : `main` à `10050a96e` (leviers G-c : `radix.cpp`, `first_probes.cpp`,
  `passes.cpp`, ajouts à `tower.hpp`, portes `mhgp12_tower_index_*`, 18 mutants ; empreinte de résolution de l'objet
  seul, `COBJ`), tête du moment `a5e0dbc77` (deux commits d'audit). Batterie repo5 arrêtée par son groupe de
  processus à 01:22:17. **repo6** = `git archive a5e0dbc77` + le patch de repo5 : deux rejets, `module.cmake` (mes
  sources après la liste de G-c) et `tests/mutants/tower.json` (18 de G puis mes 9, plancher 27) ; les deux fichiers
  sont **octet pour octet ceux du raccord préparé indépendamment par l'auditeur** (`composition_gc_tmvr_20261008` :
  module `12219c37…`, manifeste `75fa8a67…`). Reçus de l'auditeur lus : `comparateur_naissances` (défaut latent : le
  comparateur de `sort_balls` positionnait un drapeau d'égalité par effet de bord, déclenché par l'auto-comparaison du
  contrôle d'irréflexivité de `_GLIBCXX_DEBUG`, refus `tower_invariant` à tort sur toute cohorte ; sans effet en
  Release, où le tri ne compare jamais un élément à lui-même) : **correctif adopté** (comparateur pur, centres égaux
  cherchés parmi les voisins après le tri, comme `sort_sites`) ; `audit_foret_validation_20261008` (le validateur
  public ne relit pas l'historique de LEM-T5 alors que son commentaire annonçait « événements = naissances − 1 ») :
  **commentaire ramené à ce qui est contrôlé**, gardes proposées et porte sémantique des rangs d'attache laissées
  ouvertes (hors chemin produit) ; `translation_tmvr` (protocole d'une future porte) : non intégré. Patch
  régénéré (31 fichiers, +4 570/−8, `b3c78ae3…`), `git apply --check` conforme sur des extractions neuves de
  `a5e0dbc77` et de `10050a96e`, arbre identique à repo6 ; `check_style` 312 fichiers, `check_constats` 75 : verts.
  Batterie relancée à 01:26:21 (`outils/etape_final6.sh`, sorties `final6/`).
- 01:26–01:51 UTC : batterie sur repo6 (base `a5e0dbc77`, G-c + T/M/V/R), profil 21 : construction complète sans
  avertissement ; **suite rapide complète 691 sélectionnés, 690 réussis, sentinelle LiDAR sautée** (portes G-c
  `mhgp12_tower_index_*`, échelles 8 000 / 16 000 / 32 000 à l'empreinte de l'objet, juges et fausses sondes
  `travail_8` / `objet_8` compris) ; **MES-M0 à l'octet** neuf cas × deux modes, 1 contre 8 fils, JUG-EMST :
  `mes_m0_ok profil=21 cas=9 modes=2 juge_emst=oui` ; **chaîne G → T** (catalogue de T1, G-c, T, M, V, R, export) :
  `mes_m0_chaine_ok profil=21` trois lots de trois cas, empreintes sémantiques de la v11, 1 contre 8 fils identiques.
- 01:51–02:11 UTC : campagne de mutants du module `tower` sur repo6 : **27 tués sur 27 par code** (18 de G-c, 9 des
  miens ; aucun signal, délai ni échec de construction ; `mutants_ok module=tower mutants=27 tues=27 plancher=27`).
  Batterie arrêtée à 02:10:44 au début du profil 24 (le coordinateur attend la chaîne pour la session G4 suivante ;
  profils 24/32 « si le temps le permet ») : profils 24 et 32 non rejoués sur repo6 ; derniers résultats à ces profils
  sur repo5 (base `c903774b1`, même T/M/V/R hors comparateur des naissances et commentaires). Passe de temps à un
  fil sur repo6 (`temps6_f1/`, minimum de trois prises, charge 2,7 à 3,7 sur 8 cœurs) ; empreintes des octets égales
  à MESURE.md § 4. `main` à `210b5dc09` (commits d'audit, reçu G4 J, `docs/PLAN.md`) : aucun changement de `src/`,
  `tests/`, `cmake/` depuis `a5e0dbc77` ; `git apply --check` conforme sur une extraction neuve de `210b5dc09`.

## Synthèse finale (8 octobre, 02:11 UTC)

- **Patch** : `patch_tour_TMV.diff`, base `a5e0dbc77` (`main`, ⊇ `10050a96e`, `c903774b1`, `99fa83246`, socle
  `7b7d025b3`), 31 fichiers, +4 570/−8, SHA-256 `b3c78ae365a99568457bc6cd5d894a1b73d02b7b19ae3d774f9fdfa094a58bab` ;
  s'applique aussi sur `10050a96e` et sur la tête `210b5dc09` ; application sur extraction neuve = repo6.
- **Codes sur repo6** (`final6/codes.txt`) : build21=0, ctest21=0 (691 : 690 + sentinelle), m0_octets_21=0 (9 × 2,
  JUG-EMST), chaine_21_a/b/c=0, mutants=0 (27/27). `check_style` 312, `check_constats` 75 : verts.
- **Profils 24/32** : repo5 (`c903774b1`) : 24 = construction, suite rapide 681, chaîne neuf cas, MES-M0 sémantique
  ng00 K5 deux modes, tous 0 ; 32 = construction et suite rapide 681, 0. repo3 (`9c5809919`) : 24 et 32 complets.
