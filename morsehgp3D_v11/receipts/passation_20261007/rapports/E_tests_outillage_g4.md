# Audit final v11 : tests, portes, mutants, mesure et sessions G4

**Cadre de l'audit**
- `phase=exploration_v11_hors_registre / backend=cpu_reference / profile=quantized_u21_input_only / public_status=not_claimed`.
- **GCP non utilisé.** Lecture seule de l'instantané d'`origin/main` **ac081a06f** : aucune compilation, aucun test, aucune écriture.
- Les chemins sont relatifs à `/workspaces/E-HGP/morsehgp3D_v11/`, sauf ceux de `gcp-migration/`.

**Comment lire ce rapport**
- Un énoncé suivi d'un chemin a été lu dans l'instantané, par moi ou par trois relectures déléguées que j'ai recoupées par sondage : finm, R3, graph3/graph4, plans, juges, rapports `gpu_ab`.
- **(inf.)** marque mon inférence.
- **(note dév.)** marque une note de mémoire du développeur qu'aucun reçu ne recoupe.

## 1. Périmètre et état

**Volume (inf., comptage de lignes)**
- `src/` : environ 21 900 lignes.
- `tests/` : environ 48 700 lignes, dont 124 scripts Python de porte, 385 tests C++ et 153 sorties exactes `LINE`.
- `bench/` : 83 fichiers, dont 64 scripts Python (environ 16 400 lignes).
- `receipts/` : 125 dossiers, 9 774 fichiers, 189 Mo.

**Portes par configuration, aux derniers pins**

| Configuration | Portes | Source |
|---|---:|---|
| Release u18 | 890 | `receipts/developpement_20261006/v3_qualification/claudev3q1/result_gcc_release.json` |
| ASan/UBSan et TSan | 803 à 807 | `receipts/developpement_20261007/*/*/matrix_summary.json` |
| Échelle et LiDAR, par profil | 52 + 66 | |
| `long` | 35 à 41 | |
| Mutants | 39 CTests | 13 campagnes et 26 contrôles de manifestes |

**Campagnes complètes**
- **c40f40798** (3 octobre) : 4 073/4 073 portes et 326 mutants (`receipts/qualification_performance_20261003/README.md`).
- **98a009550** (5 octobre, reprises R1 à R4) : 3 695 portes ordinaires, **485/485 mutants** et 80 portes ASan u24 d'échelle (`receipts/developpement_20261005/qualification_finale/README.md`).
  - TSan d'échelle 80/80 et ASan/TSan ordinaires 783/783 ont été joués au pin 38b76701b, dont les fichiers moteur sont identiques.
- Plus tôt, reuse1 (ae817d09e) : 3 339/3 339 portes et 292 mutants.

**Mutants à HEAD.** Les manifestes déclarent **530 mutants** dans 13 modules, avec un plancher égal à l'effectif (`tests/mutants/*.json`). Les 45 ajoutés après R3 n'ont été joués que par lots `--only`.

**Sessions G4**
- 165 `receipt.json`, soit **126 sessions distinctes** après dédoublonnage par génération, plan et paquet (inf.).
- Statuts : 72 `completed`, 50 `failed_remote`, 2 `failed_before_start`, 2 `shutdown_uncertified`.
- 124 sessions sur 126 ont un arrêt ciblé certifié.
- Répartition par jour (heure du Pacifique, 2 au 6 octobre) : 38, 24, 15, 19 et 30.
- Une session perdue ne laisse aucun reçu (claudeflat2, note dév.) : 126 est donc un minorant.

**État à HEAD : aucune qualification complète.** Depuis 98a009550, les sessions des 6 et 7 octobre ne jouent que des mutants ciblés et les portes ordinaires sous ASan/TSan (par exemple 804/804, `developpement_20261007/cache_blocs`).

## 2. Ce qui a marché

1. **Porte à code exact, registre fermé, harnais lui-même testé.**
   - `cmake/run_expect.cmake` : codes 0 à 4 exacts ; un signal est un échec ; 126/127 signifie lancement impossible, jamais « mutant tué » ; le jeton de saut ne peut pas être usurpé.
   - `cmake/gates.cmake` refuse `add_test` direct, les tests de sous-dossier, `PASS_REGULAR_EXPRESSION`, `WILL_FAIL` et `DISABLED`.
   - Le harnais a ses propres portes (`tests/support/tests.cmake`) : 16 refus injectés, puis `gate_properties` (69 contrôles), `run_mutants` (92), `g4_matrix` (37) et `gate_helper` (28).
   - Les failles initiales ont été corrigées : registre limité à la racine, témoin `Skipped` accepté, programme absent compté « TUE » (`receipts/audit_independant_20261002/gates_review`, `gates_followup`).
2. **Anti-vacuité à chaque étage.**
   - Le contrôleur impose `ctest --no-tests=error` (`check_ctest`), car CTest 3.22 rend 0 sans aucun test.
   - `tools/g4_matrix.py` classe une configuration `vacuous`, `incomplete` ou `floor_violated`, contrôle `min_tests` et `require_labels`, et recoupe le JUnit avec la sortie standard.
   - `bench/gpu_ab.py` refuse `reps < 1` et moins de deux passes à chaud.
   - Ces gardes ont mordu : les matrices coupées (fina2 à 3 642/3 695, claudequalmatrice à 4 734/5 147) sont restées rouges, et N1 a été refusé (145 portes sous un plancher de 150).
3. **Mutants causaux.**
   - Principe : patch sur une copie, témoin joué d'abord, et « TUE » seulement avec la cause donnée par le juge.
   - R3 : 485/485, soit 479 par code de sortie, 4 par ligne absente et 2 refus de construction attendus, sans aucun signal ni délai. L'auditeur les a recomptés dans `LastTest.log` (`receipts/audit_g4_repriser3_20261005/README.md`).
   - Ils ont révélé de vrais défauts de câblage : `sp_masque_16379` survivait parce que sa porte était débranchée depuis L2b ; l'option de placement se perdait au déplacement.
4. **Identité à l'octet, condition de toute mesure.**
   - 81/81 prises appariées identiques le 3 octobre.
   - `gpu_ab.py` exige à chaque prise l'empreinte du vidage et le registre du catalogue.
   - 372 prises GPU à froid et 84 à chaud, sans aucun refus (`developpement_20261004/gpu_g4`).
5. **Session gardée mûre.** `gcp-migration/v11_session.py` (2 332 lignes) porte la v10 de façon explicite.
   - Contenu : scripts gardés épinglés, fermeture par génération prouvée, réserve disque, grades de preuve distincts (commit ou instantané).
   - 124 arrêts certifiés sur 126. Les deux autres sont des échecs de démarrage dus à l'épuisement de capacité de la zone. Des lectures externes ont ensuite constaté `TERMINATED` (`developer_blockers_evidence_review_18`).
   - `--recover` a fermé graph4 après la perte de son contrôleur (`developpement_20261003/reprise_performance`).
6. **Règle écrite avant les données, jamais réécrite.**
   - Les 31 `plan.json` des 6 et 7 octobre égalent l'empreinte certifiée au lancement (vérifié).
   - Les échecs sont publiés tels quels : claudeo1place2, claudesplit1, claudeg1 (« erreur de conception de la règle, que je relève sans la corriger après coup »).
   - Bras A/A avec fenêtre de validité [0,97 ; 1,03] : gm A/A 0,994 (`developpement_20261006/o1_placement`).
7. **G4 seul juge des temps.** Trois exemples :
   - THP : −17 % en local, mais gm 1,064 sur G4 (`developpement_20261007/thp_exploration`).
   - Levier A : meilleur SASS statique, mais ×1,11 sur G4 (`wfgpu1_leviers`).
   - v3_census : instructions ×0,565, temps seulement ×0,925.
8. **Un fil pour trancher.** Le bruit A/A est d'environ ±0,5 % à W1, contre jusqu'à 9 % à W48 (`developpement_20261004/mesures_g4_ab8_diag1`). C'est ce qui a mesuré le gain du q3 différé (−1,0 à −1,3 %) et le coût des enveloppes M3/E4 (+1,2 à +1,4 %).
9. **Portes Python reproductibles.** Elles tournent en Python 3.10 nu, avec une jumelle `-O` et une règle statique qui interdit `assert`. Les reçus se rejouent en normal et en `-O`. `bench/verify_full_captures.py` a 55 autotests par mode.
10. **Le bon contre-modèle existe déjà.** La comparaison à HDBSCAN est préinscrite : partition dev/test, correction de Holm, prédictions écrites d'avance (`plans/e1_prereg_*.json`).

## 3. Ce qui n'a pas marché

1. **G4 a servi de boucle de compilation.**
   - Cause : la consigne « aucun build ou test natif dans le Codespace » (`README.md`, section Construction).
   - Effet : le 2 octobre, 26 sessions sur 38 finissent en `failed_remote`, souvent pour des fautes qu'une compilation locale aurait vues :
     - `-Werror=misleading-indentation` (`receipts/catalogue_adaptive_20261002/adaptive2_failure`) ;
     - un mutant refusé par `maybe-uninitialized` (`full_census_20261003/census1_failure`) ;
     - un attendu faux (`full_parallel_20261003/forest1_failure`).
   - La pratique change le 5 octobre (note dév.).
2. **La matrice dépasse la session.**
   - `maxRunDuration` vaut 4 200 s ; la matrice a un budget de 2 100 puis 2 200 s.
   - Configurations coupées : fina2 (quatre profils), claudequalmatrice, claudequalb (1 533/1 648) et `release_long`. Dans ce dernier, CTest est tué en −9 et `LastTest.log` tombe à 121 octets.
   - Trois causes :
     - les jumelles `_opt` doublent les portes Python d'échelle et LiDAR ;
     - `-L ^long$` aspire aussi les campagnes de mutants, étiquetées `long` (`qualification_finale/claudefinl/matrix_summary.json`) ;
     - chaque configuration porte un profil entier.
   - Bilan : **19 sessions le 5 octobre** pour qualifier S8, S9, L2b et S10.
3. **Attendus gravés pour un seul profil.** Les empreintes de route, gravées en u21, ont fait échouer six portes en u18 et u24. Elles ont aussi rendu rouge le témoin API : 23 mutants n'ont pas été jugés (`receipts/audit_g4_finm_20261005`). Réparé en be05bfad8 et 98a009550.
4. **Mutants partiellement couverts.**
   - Profil de base u18, alors que le défaut est u21 : 453 mutants joués en u18, 15 en u21 et 17 en u24.
   - Aucune variante TSan.
   - `--check` ne vérifie pas que la porte citée existe dans le module : claudesplit1 n'a jugé aucun mutant (« No tests were found »).
   - Les rapports `mutants_<module>.json` ne sont pas rapatriés.
   - Plusieurs mutants sont invalides sous `-Werror`.
5. **La statistique de décision est le défaut principal.**
   - **Bruit à W48.** L'A/A involontaire de claudeo1place donne ±12 % à froid (5 processus) et ±9 % à chaud. À chaud, un A/A explicite va de 0,887 à 1,146.
   - **Seuils sans calcul de puissance.** Ont été retirés des leviers favorables :

     | Levier | Mesuré | Seuil |
     |---|---:|---:|
     | Préchargement des graines | 0,865 | 0,85 |
     | Filtre G1 AVX2 | 0,895 | 0,85 |
     | Frontière par tranches | 0,859 | 0,80 |
     | Annonces des publieurs | 0,966 | 0,90 |
     | Préchargements combinés | 0,950 | 0,90 |
     | Lot partagé, à froid | 0,916 à 0,977 | 0,90 |

     Le développeur le conclut lui-même : il fallait des seuils à la taille d'effet réaliste, de 3 à 10 % (note dév.).
   - **Statistique mal ciblée.**
     - Le froid pour un levier GPU, alors que l'ouverture de CUDA coûte environ 75 ms (`lot_partage`).
     - La passe unique CPU, qui dilue l'effet de G1.
     - Le levier A jugé à K5/16, alors que K5/24 était la configuration de référence GPU.
   - **À chaud, un seul processus par (trame, mode), dans un ordre fixe** (`bench/gpu_ab.py`). La faible dispersion des passes masque la variance entre processus (inf.).
   - **Première prise biaisée.** La première prise GPU à froid vaut 599,8 ms, contre 343 à 357 ms pour les autres. Elle tombe toujours dans le bras de tête (`cache_blocs/claudecache1/gpu_ab_report_ab_k5_24_gpu.json`).
   - **Médiane haute** pour un nombre pair de prises (`median()`).
   - **Juges ad hoc.** Chaque session a son `judge.py`, dont l'empreinte n'entre pas au reçu. La règle n'est qu'un texte libre dans `note` (2 000 caractères au plus). Le juge imprime « banc non conforme » puis continue.
   - **Chemins bifurquants.** La confirmation du lot partagé est passée du froid au chaud et du seuil 0,90 à 0,95 (`lot_partage_confirmation`). reservoir3 a été gardé alors que son critère n'était pas atteint.
   - **Rotation biaisée avant d5b1d0179** : base passait toujours avant new, et ces campagnes n'ont pas été rééquilibrées (`audits/AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md`).
6. **Les bancs de décision n'ont aucune porte.** `gpu_ab.py` et `ab_g4.py` n'ont pas d'autotest CTest. Les auditeurs y ont trouvé, entre autres :
   - un banc GPU vert avec `reps=0` ;
   - un cache de variantes non lié au SHA de l'archive ;
   - `None == None` accepté par le juge GPU ;
   - un lecteur borné à 131 071 ;
   - une IoU arrondie avant le seuil strict de 1/2.

   Corrigés en 22a6af6aa, 12ce8f8f0 et 38faaf272, sans rétroqualification.
7. **Interfaces opaques.**
   - Les modes sont des masques entiers : 16379, 278523, 802811, `868347:400`.
   - La sonde prend 10 à 13 arguments positionnels et rend 2 sans message (`bench/full_probe.cpp`).
   - Conséquence : 9 portes sur 9 passent en affichant `placements=0` (`o1_placement/claudeo1place/portes.txt`).
8. **Environnement Python différent.** claudequal1 tombe sur `import numpy` (`developpement_20261004/qualification_p1p2`). Aucune règle de `tools/check_style.py` n'interdit les imports tiers.
9. **Portes couplées aux reçus.**
   - `bench/full_baseline_source.py` lit `receipts/qualification_performance_20261003`. La porte `full_paired_protocol` est donc rouge en checkout partiel (`v3_census`).
   - Les paquets sources de ce reçu sont en `export-ignore` : sa relecture ne se rejoue pas depuis le seul dépôt.

## 4. Pièges et leçons

**TSan.** Il exige `setarch -R` : 6 démarrages réussis sur 40 en natif, contre 40 sur 40 avec (`tools/g4_matrix.py`, `sanitizer_wrapper`).

**CTest et CMake**
- `--no-tests=error` est obligatoire.
- Les labels `mutant` + `long`, et le motif `-R mutants_` (note dév.), lancent les campagnes complètes.
- CMake 3.22 ne s'arrête pas à `--` en mode script, et ne connaît pas le dialecte CUDA20 (claudegpu1).

**VM**
- Épuisement de capacité en us-central1-b, puis nouvelle VM en c sans g++ ni CMake (session tools1).
- Préemption SPOT au démarrage (claudeab5 et claudeab6).
- `maxRunDuration` à 3 600 s au lieu de 4 200 s (`developpement_20261002/graph1_preflight_refusal.json`).

**Codespace**
- Les redémarrages hors calendrier vident `/tmp` et tuent le contrôleur. `--recover` certifie alors l'arrêt mais ne rapatrie rien (graph4 ; claudeflat2, note dév.).
- La garde disque exige environ 1,14 Go : graph3 a été refusé pour 163 Ko. Le contournement par liens vers `/tmp` a ensuite fait perdre le paquet (`reprise_performance`).
- Un vidage de 272 Mo a dépassé le plafond des résultats (reservoir2).

**Côté local**
- `rsync -a` après la restauration d'un mutant laisse l'objet muté en place (note dév.).
- `pkill -f` tue son propre shell.
- Éditer un script bash pendant qu'il tourne le corrompt.
- L'index Git est partagé avec les auditeurs.
- L'heure se lit par `date -u`, jamais de mémoire.

**Mesure**
- W48 libre contre W24 épinglé change deux facteurs à la fois.
- CPU plus attente ne partitionne pas le mur.
- `full_timing` vide vers `/dev/null`, donc aucune identité de sortie n'est établie.
- `sorties_g4.json` porte une métadonnée recopiée d'un ancien plan.

## 5. Dettes et problèmes ouverts

- **Aucune qualification à HEAD.** Manquent la matrice complète, les 530 mutants au profil u21 et l'échelle sous sanitizers. Restent aussi à jouer :
  - la garde des arbres de points (b0f2a0a9e) ;
  - les mutants de placement de l'API ;
  - la porte du repli U2 ;
  - les portes GPU numériques extrêmes ;
  - six campagnes L/u21 sans résultat.
- **Outillage.**
  - Clang est absent de la VM ; ASan18 ne couvre qu'une partie des modules.
  - L'isolation des descendants n'est pas certifiée.
  - Avec `default_build=false`, aucun hachage de binaire n'est conservé.
  - Pas de `--output-on-failure`.
- **Contrats de temps : 100 ms et 200 ms non tenus.**
  - Meilleur moteur, K5 GPU à chaud : 251,3 / 212,2 / 255,4 ms (`filtre_g1_avx2`).
  - Le réglage glibc `@tas` atteint 196,8 ms, sur ng01 seulement.
  - K10 GPU : 1,3 à 1,8 s.
  - Une seule séquence (08) et trois trames.
- **Différentiel v10/v11 sur trames entières : ouvert.** Les portes `diff_v10` exigent `MHGP11_V10_FROZEN_DIR`, qui ne figure pas dans `tools/g4_matrix.json`.
- **Reçus lourds** : ils contiennent des copies d'arbres sources et l'e-mail du compte dans `recovery_command`.

## 6. Recommandations concrètes pour la v12

**Garder**
1. `run_expect.cmake`, `gates.cmake`, le registre fermé et l'autotest du harnais par injection de fautes.
2. `v11_session.py`, en port explicite, avec une reprise qui **rapatrie** les résultats depuis le disque de la VM.
3. L'identité du vidage et du registre à chaque prise, comme condition de validité d'une mesure.
4. Les mutants par patch sur copie : témoin d'abord, cause exigée, plancher égal à l'effectif.
5. Des reçus immuables avec `SHA256SUMS` et rejeu en normal et en `-O`, mais sans copies d'arbres ni identité de compte.

**Simplifier**

6. **Un seul banc de décision.** Le plan JSON porte une règle **structurée** : statistique, régime, bras, seuil, plafond par rapport, conduite en cas d'échec. Un juge unique en bibliothèque, couvert par CTest, refuse tout banc non conforme et inscrit son empreinte au reçu.
7. Des **modes nommés** et versionnés (`ref_cpu_k5`, `ref_gpu_k5`), et une sonde à options nommées qui explique ses refus.
8. Une matrice découpée d'avance en lots de 30 minutes au plus, aux durées mesurées. Garder la règle statique contre `assert` ; réserver les jumelles `-O` aux petites portes.
9. Aucune porte ne lit `receipts/` : les fixtures vont dans `tests/fixtures/`.

**Refaire**

10. **Le protocole statistique.**
    - Trier les leviers algorithmiques à W1 ou sur compteurs déterministes ; réserver W48 au parallélisme et à la mémoire.
    - À W48 :
      - au moins 10 processus par bras et par trame, entrelacés aussi à chaud ;
      - un processus d'échauffement jeté ;
      - un bras A/A dans chaque session.
    - Décider sur des log-rapports appariés, avec un intervalle par bootstrap ou un test des signes.
    - Tirer le seuil de la variance A/A (effet minimal détectable) et de l'effet attendu (3 à 10 %), sur la phase touchée, avec le mur comme garde.
    - Mesurer sur plus de trames réelles et de séquences.
    - Prévoir un protocole par lot pour les petits gains cumulables, et déclarer et compter toute confirmation.
11. **La cadence de qualification.**
    - À chaque jalon : la matrice complète et tous les mutants au profil par défaut.
    - Tenir un registre de la dette de qualification, commit par commit.
    - Un `--check` qui vérifie la présence des portes citées.
    - Rapatrier les rapports par mutant, et ajouter une variante TSan pour les mutants de concurrence.
12. **La parité d'outillage.**
    - Une image locale identique à la VM : GCC 11.4, CMake 3.22.1, Python 3.10 nu.
    - Des constructions locales Release avec ccache, et des portes ciblées.
    - Les portes Python lancées par `python3 -I -S`, et une règle de style qui interdit les imports tiers.
    - Clang installé sur la VM.
13. **L'hygiène d'exploitation.**
    - Un préflight qui contrôle `uptime` et `df`.
    - Jamais de données ni de liens dans `/tmp`.
    - Un plafond de résultats dimensionné.

## 7. Références clés

**Harnais :** `CMakeLists.txt`, `cmake/gates.cmake`, `cmake/run_expect.cmake`, `cmake/expect_refusal.cmake`, `tests/support/tests.cmake`, `tests/support/mhgp11_gate.py`, `tools/check_style.py`.

**Mutants :** `tests/mutants/run_mutants.py`, `tests/mutants/*.json`.

**Matrice :** `tools/g4_matrix.py`, `tools/g4_matrix.json`, `tools/g4_prepare_host.py`.

**Bancs :** `bench/gpu_ab.py`, `bench/ab_g4.py`, `bench/full_paired.py`, `bench/full_probe.cpp`, `bench/verify_full_captures.py`.

**Sessions :** `/workspaces/E-HGP/gcp-migration/v11_session.py`, `/workspaces/E-HGP/gcp-migration/README_V11.md`.

**Reçus :**
- `receipts/qualification_performance_20261003/`
- `receipts/developpement_20261005/qualification_finale/`
- `receipts/audit_g4_repriser3_20261005/`
- `receipts/audit_g4_finm_20261005/`
- `receipts/developpement_20261004/mesures_g4_ab8_diag1/`
- `receipts/developpement_20261006/o1_placement/`
- `receipts/developpement_20261006/lot_partage/`
- `receipts/developpement_20261006/lot_partage_confirmation/`
- `receipts/developpement_20261007/cache_blocs/`
- `receipts/developpement_20261007/filtre_g1_avx2/`
- `receipts/developpement_20261007/thp_exploration/`
- `receipts/developpement_20261003/reprise_performance/`
- `receipts/audit_independant_20261002/gates_review/`

**Notes :** `audits/README.md`, `audits/AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md`, `plans/e1_prereg_*.json`.
