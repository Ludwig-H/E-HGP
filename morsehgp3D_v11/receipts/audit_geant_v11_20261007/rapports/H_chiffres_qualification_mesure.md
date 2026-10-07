# Lecteur H — chiffres, qualification, protocole de mesure, sessions G4, canal d'audit (v11)

```text
phase=exploration_v11_hors_registre (close le 7 octobre 2026)
backend=cpu_reference ; voie de banc cuda_g4
profile=quantized_u21_input_only
public_status=not_claimed
GCP non utilisé
```

Lecture seule du dépôt (`main` à `e968aba8d` ; moteur gelé `ac081a06f`, sources `src/ bench/ cli/ tests/ cmake/`
identiques à `733912e65` : `git diff --stat 733912e65 ac081a06f -- …` vide). Aucune écriture sous `/workspaces/E-HGP`,
aucune commande GCP. Lecture, en plus, des paquets de sessions hors dépôt `/workspaces/.ehgp-sessions/` (volatils, lus
sans écrire). Les calculs statistiques sont faits en bibliothèque standard (`python3 -I`) à partir des rapports bruts
`gpu_ab_report*.json` ; scripts et sorties dans `…/scratchpad/agent_H/scripts/` et `*_out.txt`.

**Légende.** Sans marque : vérifié dans le fichier, le reçu, le commit ou par recalcul cité. **[I]** : inférence.
« ng00/01/02 » = trames 08/000000, 000100, 000200 sans sol. « Rapport » = new/base. « gm » = moyenne géométrique.

---

## 0. Synthèse

1. **Les temps du § 3.1 et la référence v10 du § 3.2 sont exacts au chiffre près** (39 temps v11, 21 temps v10 et
   27 rapports recalculés sur les sources brutes). Trois précisions : les temps « à froid » sont des **médianes hautes** (4ᵉ de 6 ; vraies médianes
   0,3 à 1,3 % plus basses), K10 « à froid » porte sur **3** processus, non 6 ; la référence v10 est **une seule passe**
   (la 3ᵉ de 3), hors préparation (6,7 à 8,3 ms) et en u18.
2. **Le jalon de 200 ms a été franchi une fois** : 196,8 ms à chaud sur ng01 (préréglage glibc `@tas`, `claudetas1`) ;
   la passation dit « approché à 212 ms ».
3. **Empreintes FULL** : les 9 SHA du § 3.4 sont exacts, mais les **trois K10 ne sont pas dans `records.json`** ; elles
   apparaissent le 4 octobre (`claudegpu3`). Sur 81 rapports de banc (4–7 oct.), **3 303 vidages LiDAR, 0 écart** : 802
   par trame à K5, 293 à K10 ; voie GPU 1 602 prises froides + 378 processus chauds (et non « 372 / 84 », qui sont
   toutes voies confondues d'un seul reçu).
4. **La « qualification complète » de `98a009550` est une union de 12 sessions sur deux commits** (TSan, ASan ordinaire,
   `long`, différentiels S9/S10 joués à `38b76701b`). **Le code exact du HEAD n'a jamais passé une matrice** ; la
   dernière matrice Release + échelle + LiDAR date de `38faaf272` (6 oct.), oubliée par la passation (§ 2.3), qui rend
   aussi fausse l'affirmation « SPv2 qualifiée seulement en Release u21 ».
5. **Mutants** : 530 déclarés (vérifié) ; la campagne complète force u18 (`tools/g4_matrix.json:48`) alors que le
   défaut produit est u21 (`CMakeLists.txt:62`) ; **17 des 45 mutants ajoutés après R3 n'ont jamais été joués sur G4**.
6. **Diagnostic statistique de l'audit à corriger.** Le bruit « ±9 à 12 % » est celui d'**un** rapport de médianes ;
   la statistique de décision (gm de 6 rapports) a un écart-type nul-hypothèse de **≈ 1,0–1,2 %** sur les étages
   (11 A/A naturels) et de 1,1 à 2,4 % sur les phases. Le défaut n'était pas la puissance mais **des seuils placés à
   1,7 à 7 fois l'effet minimal détectable**, au-delà même des effets réels (calés sur les gains locaux).
7. **Des cinq leviers retirés, un seul l'a été clairement à tort** (G1 AVX2 : phase −10,5 %, `domain` −3,6 %,
   p = 0,005 ; mur −1,7 %, p = 0,03). La frontière par tranches a un effet réel mais d'environ 3 ms. Le préchargement
   des graines a un effet de phase réel (−13,5 %) **sans effet d'étage** (forêts p = 0,25, mur 0,998). Les
   préchargements combinés (p ≈ 0,07) et les annonces (p ≈ 0,29) **ne montrent aucun effet établi** : « retirés de
   justesse » est faux pour eux.
8. **À chaud, la variance entre processus vaut 10 à 27 fois** celle que prédit la dispersion des passes : l'[I] de
   l'audit (§ 10.2) est confirmé. La première prise GPU (initialisation ≈ 300 ms contre ≈ 70) tombe dans le premier mode
   listé — tantôt le levier, tantôt la base (cache, `@tas`).
9. **Sessions** : « 126 / 124 » n'est pas reproductible. Hors dépôt : 138 paquets, **132 arrêts ciblés certifiés**
   (dont 2 par `--recover`), **4 échecs de démarrage non certifiés** (et non 2), 1 refus de préflight disque, 1 paquet
   de création. Dans le dépôt : 124 noms, 120 certifiés.
10. **Reçus** : **357 Mo**, et non 190 (9 782 fichiers à `ac081a06f` ; 92 archives `tar.gz` = 228 Mo ; `audit_*`
    = 175 Mo, soit 49 %). L'identité du compte est dans **528 fichiers** en clair (143 adresses + 502 chemins dérivés du
    nom de compte) et dans 91 archives sur 92.
11. **Canal d'audit** : 40 commits après le dernier passage des auditeurs (`28d70f8ab`, 6 oct. 22 h 08). Environ
    100 commits d'auditeurs (et non 92) ; quatre commits portent l'auteur `Claude` (et non tous `Ludwig-H`).
    `13a4a0a4c` (O2) : risque d'exactitude faible, garde de préchargement correcte mais non mutable. `ccdd4db75` (cache) :
    risque d'exactitude faible pour la sortie, **risque de contrat mémoire moyen** (blocs inactifs et arrondi de
    classe hors budget, jamais testé sous limite finie).

---

## 1. Tableau des affirmations vérifiées

P = `PASSATION.md` ; A = `docs/AUDIT_FINAL_V11.md`. Rapports `gpu_ab` de `claudeg1` :
`receipts/developpement_20261007/filtre_g1_avx2/claudeg1/` (ci-dessous « g1/ »).

| # | Affirmation (lieu) | Valeur affirmée | Valeur trouvée | Source | Verdict |
|---|---|---|---|---|---|
| 1 | K5 GPU `868347:400` à froid (P § 2.1, A § 3.1) | 335 / 301 / 345 ms | 335,2 / 300,7 / 345,1 = **médiane haute** de 6 ; vraie médiane 332,3 / 300,5 / 343,3 | g1/`gpu_ab_report_ab_k5_24_gpu.json` `cold_medians_ms` ; `bench/gpu_ab.py:70–72` | confirmé (préciser « médiane haute ») |
| 2 | K5 GPU à chaud, meilleures passes | 251 / 212 / 255 ; 250 / 207 / 251 | 251,3 / 212,2 / 255,4 ; 249,5 / 207,5 / 251,2 | idem, `warm_medians_ms` (passes 2–10) | confirmé |
| 3 | K5 GPU chaud `domain`/forêts | 138/114, 120/91, 141/114 | 137,7/113,7 ; 120,1/90,8 ; 140,7/113,6 | idem | confirmé |
| 4 | K5 CPU `802811` froid / chaud | 343/272/329 ; 314/255/313 | 343,3/272,5/328,9 (médiane haute ; vraie 338,9/271,6/327,7) ; 313,5/255,1/313,2 | g1/`…_k5_16_cpu.json` | confirmé |
| 5 | K5 CPU chaud `domain`/forêts | 200/113, 163/92, 195/116 | 199,5/113,3 ; 162,8/91,7 ; 194,9/116,1 | idem | confirmé |
| 6 | K10 GPU `868347` froid / chaud | 1 824/1 395/1 602 ; 1 782/1 336/1 536 | identiques ; froid = médiane de **3** (P l'annonce « 6 processus ») | g1/`…_k10_24_gpu.json` | corrigé (n = 3) |
| 7 | K10 chaud `domain`/forêts | 489/1293, 398/939, 471/1065 | 488,9/1293,4 ; 397,8/938,7 ; 471,1/1064,8 | idem | confirmé |
| 8 | Jalon 200 ms « approché sur ng01 seulement (212 ms) » (P § 1, A § 1) | 212 ms | **196,8 ms** (ng01, GPU, `@tas`, moteur inchangé) ; 205,0 avec G1 AVX2 (retiré) | `receipts/developpement_20261007/retention_tas/README.md` ; balayage de tous les `warm_medians_ms` K5 | corrigé |
| 9 | Référence v10 K5 et étages (A § 3.2) | 252,0/204,2/253,6 ; cat. 163,5/136,9/164,3 ; tour 88,5/67,3/89,3 | identiques | `receipts/audit_deep_20261004/performance/context/TABLE_VERITE_G4.md:9–14` | confirmé ; mais **une** passe (3ᵉ de 3 ; ng01 : 218,6/214,3/204,2), **hors préparation** 6,7–8,3 ms, u18 |
| 10 | Référence v10 K10 (A § 3.2) | 1 124,6/861,4/1 024,3 ; tour 472,0/333,9/406,2 ; cat. 652,6/527,5/618,1 ; descentes 242,9/179,3/203,4 | identiques | idem l. 10, 12, 14 | confirmé |
| 11 | Rapports v11/v10 (A § 3.2), 27 valeurs | 1,00/1,04/1,01 … 2,74/2,81/2,62 | recalculés, tous exacts à l'arrondi | calcul | confirmé (descriptif) |
| 12 | Résolveurs K10, 29 voies, 25–35 CPU·s (A § 3.2, § 7.1) | 1 214/894/988 ms | médianes froides 1 214,4/894,5/988,2 ; 25,7–35,7 CPU·s ; 29 = 48 − (2·10 − 1) | g1/`…_k10_24_gpu.json`, `pipeline.lanes` | confirmé |
| 13 | Trajectoire : catalogue séquentiel 2 oct. (A § 3.3) | 26 018/20 741/24 093 ms | 26 017,5/20 740,8/24 092,9 | `receipts/catalogue_20261002/catalogue3/catalogue.json` | confirmé (u18 à l'époque) |
| 14 | Catalogue parallèle, `full3`, `forest3`, `c40`, `pipeline_g4` | 4 460/3 002/4 028 ; 16 885 ; 14,79→6,16 s ; 489/345/432 ; 412/352/381 | 4 459,8 (ng00) ; 16 884,7 ; 14,790→6,163 ; 489,1/345,1/432,4 ; 412,4 (ng00) | `parallel5/timing_summary.json`, `full3/full.json`, `forest3/README.md:43`, `qualification_performance_20261003/README.md:16–18`, `pipeline_g4/README.md:42` | confirmé |
| 15 | 4 oct. « FULL K5 à chaud CPU/GPU 275–343 / 323–394 » | à chaud | ce sont des **meilleures passes**, choisies par trame entre deux sessions (5 et 6) | `developpement_20261004/gpu_g4/README.md:33–45` | corrigé : la trajectoire mêle meilleures passes (4 oct.) et médianes (7 oct.) |
| 16 | « La même base dérive de ±5 à 10 % d'une session à l'autre » (A § 3.3) | ±5–10 % | 5 sessions du 7 oct. à moteur identique : chaud ±1–2,2 %, froid ±0,7–3,5 % | `claudecache1`, `claudepref1`, `claudepref2`, `claudediag1`, `claudeg1` | corrigé pour le 7 oct. ([I] la dérive a pu être plus forte avant) |
| 17 | 6 empreintes K5/K10 « dans `records.json` et dans `claudeg1` » (A § 3.4) | 6 | K5 : 3/3 dans les deux ; **K10 : 0/3 dans `records.json`**, premières dans `claudegpu3` (4 oct.) | `developpement_20261003/ecart_v10_v11/records.json` (35 enregistrements, `mode 2047`, W4 local) | corrigé |
| 18 | 3 empreintes uniformes u18 8k/16k/32k | dans `records.json` | présentes (2 fois chacune), entrées `uniform_u18_n*`, exécution locale W4 ; jamais rejouées sur G4 ni gravées en porte | idem ; aucun SHA gravé dans `tests/` | confirmé |
| 19 | Empreintes K5 stables du 3 au 7 oct. (P § 1, A § 1) | stables | 81 rapports, 2 706 prises froides + 597 processus chauds : **0 écart** (802 vidages par trame à K5, 293 à K10) | balayage de `developpement_2026100[4-7]/**/*report*.json` | confirmé, renforcé |
| 20 | GPU identique au CPU « sur 372 prises à froid et 84 à chaud » (A § 1, § 6.1) | 372 / 84 GPU | 372/84 = **toutes voies** des sessions `claudegpu3–6` (GPU seul : 147/33) ; toutes sessions 4–7 oct. : GPU 1 602/378, CPU 1 089/216, 0 écart | `developpement_20261004/gpu_g4/README.md:29` ; balayage | corrigé |
| 21 | Qualification `98a009550` : 3 695 portes (P § 2.3, A § 9.1) | 3 695 | 873+783+783+784 (R1) + 4×(52+66) (R2) = 3 695 | `developpement_20261005/qualification_finale/README.md` | confirmé ; ASan/TSan ordinaires 783/783, TSan échelle 80/80, `long` 35/35, S9/S10 joués à **`38b76701b`** |
| 22 | 485/485 mutants ; 453 u18, 15 u21, 17 u24 | 485 | 485 jugés, 479 par code, 4 par ligne, 2 par construction ; profils exacts | `receipts/audit_g4_repriser3_20261005/README.md:18–51` | confirmé |
| 23 | 530 mutants déclarés, 13 modules (A § 9.1) | 530 | 530 (api 24, catalogue 74, cli 32, cloud 16, core 82, head 12, index 15, io 22, num 61, points 6, sched 8, supports 15, tower 163) ; +45 depuis R3, 10 modifiés | `tests/mutants/*.json` ; `git show 98a009550:` | confirmé |
| 24 | « Les 530 n'ont pas tous été joués, pas au profil u21 » (P § 2.3) | — | 28 des 45 ajoutés joués par lots `--only` ; **17 jamais joués sur G4** ; configuration `mutants` en u18 | `tools/g4_matrix.json:42–48` ; `CMakeLists.txt:62` ; `mut_*.txt` des reçus | confirmé, chiffré |
| 25 | « SPv2 qualifiée seulement en Release u21 (15/15) » (P § 2.3, A § 8.4) | u21 seul | 15/15 u21 à `07428324e`, **puis** `38faaf272` : portes oracle SPv2 en Release u18/u21/u24 et ASan u24, portes d'échelle et LiDAR `*_supports_*` aux trois profils | `developpement_20261006/v3_qualification/claudev3q*/result_*.json` | corrigé (restent : mutants SPv2, TSan, K10) |
| 26 | Différentiels S9/S10 non rejoués après `b0f2a0a9e` | — | vrai ; mais les deux mutants de `b0f2a0a9e` sont tués à `38faaf272` (`claudev3q1/mut_head.txt`) | `v3_qualification/README.md` (« hors périmètre ») | confirmé, nuancé |
| 27 | « 807/807 pour `claudeg1` » | 807 | ASan/UBSan u24 et TSan u21 807/807 — sur **`b6fd3796d`** (HEAD + AVX2, retiré ensuite) | g1/`matrix_summary.json`, `receipt.json` (commit) | confirmé ; ne qualifie pas le code exact du HEAD |
| 28 | 126 sessions, 124 arrêts certifiés, 2 échecs de démarrage (P § 3, A § 11) | 126 / 124 / 2 | hors dépôt : 138 paquets ; 130 `receipt.json` certifiés + 2 reprises certifiées ; **4** `shutdown_uncertified` (meb2, parallel1, parallel2 le 2 oct., `us-central1-b` ; claudereservoir SPOT `ZONE_RESOURCE_POOL_EXHAUSTED` le 6 oct.) ; 1 refus préflight (graph3) ; 1 création. Dépôt : 124 noms, 120 certifiés | `/workspaces/.ehgp-sessions/v11.*/receipt.json`, `external_closure.json` | corrigé |
| 29 | « 26 sessions sur 38 en `failed_remote` le 2 oct. » (A § 9.2) | 26/38 | en dates UTC des noms : 17 sur 26–27 ; le rapport E comptait en heure du Pacifique | paquets et reçus | corrigé (base de date) |
| 30 | 19 sessions le 5 oct. | 19 | 19 paquets datés du 5 oct. | idem | confirmé |
| 31 | 360 commits ; 58/82/58/60/74/28 par jour | 360 | 360 à `ac081a06f` (362 au HEAD avec les deux de passation) ; répartition exacte | `git log -- morsehgp3D_v11/` | confirmé |
| 32 | « dont environ 92 d'auditeurs » (A § 2) | 92 | ≈ 100 (86 sujets « audit… » + 14 « v11 audit: ») | `git log` | corrigé |
| 33 | « Tous les commits portent l'auteur `Ludwig-H` » (A § 12) | tous | 4 portent l'auteur `Claude` : `ef75dafac`, `479f53f0b`, `a45daff3a`, `b87285378` (3 oct.) | `git log --author=Claude` | corrigé |
| 34 | ≈ 40 commits non relus après le 6 oct. 22 h 08 | ≈ 40 | 40 (38 jusqu'à `ac081a06f` + 2 de passation) ; dernier commit d'auditeur `28d70f8ab` | `git log --since` | confirmé |
| 35 | Leviers retirés : mesuré / seuil (A § 10.2) | 0,865/0,85 ; 0,895/0,85 ; 0,859/0,80 ; 0,966/0,90 ; 0,950/0,90 ; lot partagé froid 0,916–0,977/0,90 | recalculés exactement sur les rapports bruts (juges `statistics.median`) | `developpement_20261007/*/judge.py` + rapports ; `REPONSE_CLAUDE_SUPPORTS` § Y | confirmé |
| 36 | « −3 à −17 % sur la phase, pas réfutés, retirés de justesse » (P § 4) | 5 positifs | effets significatifs pour 3 sur 5 ; combinés p ≈ 0,07, annonces p ≈ 0,29 ; écarts au seuil de 1,5 à 6,6 points | § 3.4 ci-dessous | corrigé |
| 37 | Répartition de `domain` ng00 GPU (A § 5.1) | 138,0 ; frontière 20,7 (0,9/14,4/0,7/4,6) ; passe 37,5 (Σ 1,78 CPU·s) ; lot 51,8 (31 902 / 91 679) ; Level+rass. 3,2 ; 10,0/4,4/3,4/3,2 ; 0,8/2,9 | 137,99 ; 20,67 (0,88/14,42/0,74/4,61) ; 37,48 (1,78 s) ; 51,8 (31 902 sur 123 581) ; 2,8+0,4 ; 9,98/4,43/3,41/3,24 ; 0,76/2,86 | `developpement_20261007/diagnostic_domaine/claudediag1/gpu_ab_report_diag_k5_24_gpu.json` (une passe chaude) | confirmé |
| 38 | Publieur 5 : cellules ≈ 65 ms à 145 ns, clôtures ≈ 29 ms à 66 ns ; 438 011 / 448 698 | — | 65 ms / 448 698 = 145 ns ; 29 / 438 011 = 66 ns ; comptes exacts | `cache_blocs/README.md` ; `pipeline.orders[4]` | confirmé |
| 39 | Volume des reçus (P § 10, A § 11) | ≈ 190 Mo, 9 774 fichiers, ≈ 2 000 copies C++ | **356,9 Mo** (tailles de blobs à `ac081a06f`), 9 782 fichiers ; 2 041 fichiers C++/CUDA (13,5 Mo) ; 92 `tar.gz` = 228 Mo ; `audit_*` 175 Mo (49 %) | `git ls-tree -r -l` ; `du -sb` (358,4 Mo au HEAD) | corrigé (volume) ; « la moitié » confirmé |
| 40 | Adresse du compte dans 143 fichiers (P § 10) | 143 | 143 fichiers avec l'adresse (122 `recovery_command`, 16 `gcloud_account`, 2 `user`…) **+ 502** fichiers avec un chemin `/home/<nom dérivé de l'adresse>` ; union **528** ; 91 archives sur 92 | `grep -rlI` (sans recopier l'identité) | corrigé (sous-estimé) |
| 41 | 31 `plan.json` (6–7 oct.) égaux à l'empreinte certifiée au lancement | 31 | 31/31 SHA présents dans `launch.json`/`receipt.json` ; les `judge.py`, eux, **ne sont hachés nulle part au lancement** | calcul | confirmé |
| 42 | Première prise GPU 599,8 ms contre 343–357 ; « toujours dans le bras de tête » | — | `claudecache1` : 599,8 (init. 303,3 ms) ; dans les 18 rapports K5 qui commencent par une prise GPU : 566–675 ms, init. 243–331 ms (≈ 70 ensuite) ; elle tombe dans le **premier mode listé** : le levier (sessions `new,base`) ou la base (cache, `@tas`, O1, partage) | balayage des `cold[0]` | confirmé, direction variable |
| 43 | Bruit A/A ±0,5 % à W1 contre 9 % à W48 | — | W1 : 0,9972/1,0044/0,9995 (**une paire par trame**) ; W48 : jusqu'à 9 % sur la passe unique | `developpement_20261004/mesures_g4_ab8_diag1/README.md:24–38` | confirmé (n = 3) |
| 44 | Bras A/A gm 0,994, fenêtre [0,97 ; 1,03] | 0,994 | 0,9937 sur `forest_ms` | `developpement_20261006/o1_placement/claudeo1place3` | confirmé |
| 45 | La sonde prend 10 à 13 arguments positionnels | 10–13 | `argc` 11 à 14 | `bench/full_probe.cpp:380` | confirmé |
| 46 | 9 portes sur 9 vertes avec `placements=0` | — | 9/9 `Passed`, `pipeline_equivalence … placements=0` | `o1_placement/claudeo1place/portes.txt` | confirmé |
| 47 | Masque 212987 = trois variantes le même jour | — | `gpu_coop` (coop1–3), `gpu_recouvert` (L4), `gpu_rejoue` (réservoir), tous le 6 oct. | rapports `developpement_20261006` | confirmé |
| 48 | TSan exige `setarch -R` : 6/40 sans, 40/40 avec | — | mesuré **dans le codespace**, pas sur G4 | `tools/g4_matrix.py:547–548` | confirmé, nuancé |
| 49 | Garde disque ≈ 1,15 Go | — | graph3 refusé : 1 145 962 496 o libres pour 1 146 125 531 exigés (1 Gio + 2 × 32 Mio + marge) | `.ehgp-sessions/v11.20261003.graph3/receipt.json` ; `v11_session.py:135` | confirmé (la passation compte deux fois les 2 × plafond) |
| 50 | Cache de blocs : restitution 14 → 0,8 ms ; chaud ×0,923, froid ×0,974 | — | 14,2–14,5 → 0,8 ms ; 0,923 ; 0,974 (médianes hautes ; 0,972 en vraies médianes) | `cache_blocs/README.md`, recalcul | confirmé |
| 51 | O1 0,917 ; V3 0,925 ; constantes 0,966 ; O2 0,904 ; cohortes 0,529 / 0,942 | — | 0,917 ; 0,925 ; 0,966 ; 0,904 ; 0,529 / 0,942, tous significatifs (permutation p ≤ 0,003) | rapports bruts | confirmé |
| 52 | Clang jamais qualifié | — | `clangxx: "absent"` sur la VM (g++ 11.4.0, CMake 3.22.1, Python 3.10.12, 48 fils) | `matrix_summary.json` des sessions du 7 oct. | confirmé |
| 53 | `maxRunDuration` 4 200 s ; budget de matrice 2 100–2 200 s | — | 76 reçus sur 76 à 4 200 s ; `budget_seconds` 2 200 ; la cible n'admet pas d'autre durée (allowlist du script gardé en `europe-west4`) | `v11_session.py:872–878` ; `tools/g4_matrix.json` | confirmé |

**Bilan** : 53 affirmations ; 30 confirmées telles quelles, 9 confirmées avec une précision de portée, 14 corrigées.
Aucune n'a été jugée invérifiable, sauf l'énoncé universel « aucun résultat FULL faux » : il ne se prouve pas, mais il
est compatible avec 0 écart sur 3 303 vidages.

---

## 2. État réel de la qualification au HEAD

### 2.1 Chaîne des qualifications

| Commit (date) | Sessions | Contenu, profils | Commentaire |
|---|---|---|---|
| `38b76701b` (5 oct.) | finb, fins, finl, finp9, finp10, finmesure | ASan+UBSan u24 et TSan u21 ordinaires 783/783 ; TSan échelle + LiDAR 80/80 ; `long` 35/35 (u21) ; S9 et S10 (u21) | même moteur que `98a009550` |
| `98a009550` (5 oct.) | repriser1–4 | Release u18 873, u21 783, u24 783, `poison` (u21) 784 ; échelle + LiDAR 52 + 66 aux quatre profils ; **485/485 mutants (base u18)** ; ASan u24 échelle 80 | « dernière qualification complète » = union des 12 sessions |
| `07428324e` (6 oct.) | claudesupkr | SPv2, 15/15, Release u21 | ni mutants ni sanitizers |
| `38faaf272` (6 oct., 21 h 33) | claudev3q1, claudev3q2 | Release u18 890, u21 800, u24 800, ASan u24 800 ; échelle + LiDAR 52 + 66 aux trois profils ; 4 mutants | **dernière matrice Release et échelle** ; ni TSan, ni `long`, ni `poison`, ni mutants complets, ni S9/S10 |
| `13a4a0a4c` (6 oct.) | claudeo2a | TSan u21 800/800 ; 2 mutants | |
| `5734ca6e8`, `3e6f88c7f`, `04b00810d`, `ccdd4db75`, `b6fd3796d` (6–7 oct.) | births1, front1, ann1, cache1, g1 | ASan u24 et/ou TSan u21 ordinaires 803 à 807 ; mutants ciblés | **portes ordinaires entières** sous sanitizer, et non « courtes » ; aucune Release, aucune échelle |
| HEAD (`733912e65` ≡ `ac081a06f`) | aucune | — | ce code exact n'a passé **aucune** matrice ; le plus proche est `b6fd3796d` (HEAD + G1 AVX2, retiré ensuite) |

### 2.2 Dette vérifiée dans le code

- **Mutants au profil u18.** La configuration `mutants` impose `-DMHGP11_COORD_BITS=18` (`tools/g4_matrix.json:42–48`).
  `gcc_release` est elle aussi en u18 (l. 15–20 ; lots d'échelle l. 497 et 529). Le défaut produit est u21
  (`CMakeLists.txt:62`). Les options propres d'un mutant passent après celles de la campagne
  (`tests/mutants/run_mutants.py:243`), d'où les 17 mutants u21 et 17 u24 déclarés au HEAD (et 1 `poison`).
- **17 mutants jamais joués sur G4** (ajoutés après R3, absents de tout `mut_*` des reçus) :
  - catalogue : `aigu_sommet_c_oublie`, `j2_etroit_second_plan_faux` ;
  - cli (SPv2) : `sp_etoile_permutee`, `sp_internes_gardees`, `sp_naissances_retirees`, `sp_selection_par_role` ;
  - tower : `j2_etroit_largeur_fausse`, `j2_memoire_issue_perdue`, `placement_affinite_non_rendue`,
    `placement_leger_sur_coeur_lourd`, `placement_publieur_haut_echange`, `q3_numerateur_etroit_faux`,
    `q4_produit_mixte_positif`, `rang_local_interieur_faux`, `reservoir_chaine_perdue`, `reservoir_pas_de_chaine`,
    `reservoir_population_lente`.

  Plusieurs n'apparaissent que dans les rapports locaux du workflow `wfgpu1_leviers/workflow/`. S'y ajoutent 2 des 10
  mutants modifiés depuis R3. Le manifeste `supports.json` (15 mutants, dernier changement `0cc9cbec4`) n'a jamais été
  joué sur G4 depuis SPv2.
- **`--check` ne vérifie que la forme du nom de porte** (`run_mutants.py:157–160` : regex `mhgp11_…`), pas son
  existence dans le module. C'est ainsi que `claudesplit1` n'a rien jugé.
- **Runner de matrice.** La commande `ctest` est construite sans `--output-on-failure` (`tools/g4_matrix.py:537–538`)
  et lancée l. 732. L. 734–735, `LastTest.log` est préféré au `.tmp` même plus récent. La passation cite « l. 735 » :
  c'est la seconde moitié du constat de l'auditeur (« 535–538, 735–741 »). Fichier inchangé depuis `643fe47d7`
  (2 oct.).
- **`bench/sorties_g4.py:251`** : `provenance=dict(commit=args.commit or None, …)` recopie `--commit` (l. 215) sans
  lire `V11_SOURCE_PIN`, que lisent pourtant `bench/full_paired.py:99` et `bench/full_qualification_context.py:40`.
  Fichier inchangé depuis `be05bfad8`.
- **Isolation.** Les sondes de matrice sont déclarées `isolation: not_certified` (`tools/g4_matrix.py:14–15, 749–754`).
  `bench/gpu_ab.py:64–67` lance chaque prise par `subprocess.run`, sans groupe de processus ni contrôle de quiescence,
  contrairement à `bench/ab_g4.py:28–46` (`start_new_session`, `killpg`, vérification).
- **Différentiel v10.** Les portes `mhgp11_reference_diff_v10*` ne sont enregistrées que si `MHGP11_V10_FROZEN_DIR`
  existe (`reference/tests.cmake:124–130`). `tools/g4_matrix.json` ne la fournit pas : aucune matrice ne les joue, et
  elles ne sont pas non plus listées comme sautées.
- **Repli `unresolved` en série** : `fallback` (`src/catalogue/single_pass_batch.cpp:142–147`), sans Pool.
- **Une porte lit un reçu** : `bench/full_baseline_source.py:12` pointe sur `receipts/qualification_performance_20261003`,
  importé par `full_paired` et donc par `mhgp11_tower_full_paired_protocol` (`tests/tower/tests.cmake:216`). Elle est
  rouge en checkout partiel (`v3_census/README.md:35`).
- **Clang** : absent de la VM, configuration `clang_release` optionnelle.
- **Le produit ne joue pas le mode mesuré** : l'API et le CLI utilisent le masque 278523 et des feuilles de 16 à tout K
  (`src/api/internal.hpp:22`, `src/api/compute.cpp:26`). Le cache n'est activé que dans la sonde
  (`bench/full_probe.cpp:340–345`). Les temps de la passation ne sont donc pas ceux de `mhgp11`.

### 2.3 Ce qui n'a jamais tourné depuis

| Élément | Dernier passage | Commits moteur postérieurs non couverts |
|---|---|---|
| Release ordinaire, échelle, LiDAR | `38faaf272` | `13a4a0a4c`, `2045ec27c`, `5734ca6e8`, `b0150926d`, `0ff64512a`, `ccdd4db75`, `c80c12012` |
| `long` (identités K10 à l'échelle) | `38b76701b` | tous ceux des 6 et 7 oct. |
| `poison` | `98a009550` | idem, dont le cache, qui réutilise des blocs non initialisés |
| Campagne complète de mutants | `98a009550` (u18) | 45 ajoutés, dont 17 jamais joués |
| S9 et S10 contre Python | `38b76701b` | `b0f2a0a9e`, puis tous les changements de tour |
| Échelle sous sanitizers | `98a009550` (ASan u24), `38b76701b` (TSan u21) | — |

[I] Le risque est atténué de fait : chaque banc des 6 et 7 octobre a recomparé les vidages des trois trames, à K5 et à
K10, aux empreintes de référence. C'est un juge d'identité fort sur trois trames, mais il ne remplace ni les oracles
bornés, ni les portes de refus, ni l'échelle synthétique.

---

## 3. Protocole de mesure

### 3.1 Comment les décisions ont été prises

- **Plan figé.** Le `plan.json` est haché au lancement (31/31 vérifiés). Sa `note` porte la règle en texte libre :
  statistique, seuil de phase, garde d'étage.
- **Banc.** `bench/gpu_ab.py`. À froid, un processus neuf par prise, ordre des modes selon un carré de Williams
  (`gpu_ab.py:283–293`), trames toujours dans l'ordre ng00 → ng01 → ng02. À chaud, **un processus par (trame, mode),
  toujours dans l'ordre des modes** (`gpu_ab.py:294–297`), médiane des passes 2 à 10. L'identité du vidage et du
  registre est exigée à chaque prise (l. 268–274), ce qui est un point fort.
- **Juge.** Un `judge.py` par session. Pour les cinq leviers retirés : gm de 6 rapports de médianes de 6 processus
  (`statistics.median`, vraie médiane), sur une métrique de phase avec seuil (0,80 à 0,90), plus une garde
  « gm(étage) ≤ 1,00 ». `cache_blocs` lit en revanche les médianes du rapport, donc **hautes**
  (`gpu_ab.py:70–72` ; même défaut dans `bench/ab_g4.py:284`). Aucun juge n'est haché au lancement : leurs SHA ne sont
  que dans le `SHA256SUMS` commis après la session.
- **Puissance.** Aucune règle ne la calcule. Deux plans évoquent le bruit pour **renoncer** à une garde par rapport
  (`claudebirths1`, `claudefront1` : « dominé par le bruit A/A ±9 à 12 % »). Un seul ajuste son seuil sur une
  dispersion observée (`claudepref2`).

### 3.2 Biais vérifiés

1. **Première prise GPU.** Dans les 18 rapports K5 qui commencent par une prise GPU, le premier processus paie 243 à 331 ms d'initialisation (≈ 70 ms
   ensuite) et un mur de 566 à 675 ms (330 à 423 pour les autres). Il tombe dans le premier mode listé : le levier
   quand `--variants new,base`, la base quand les modes sont « base, levier ».
   - Effet sur la statistique du juge : ≤ 0,1 % (la médiane l'absorbe).
   - Effet sur toute analyse appariée par moyenne : l'écart-type de la cellule GPU ng00 passe à 0,2–0,33, contre 0,02
     à 0,05 ailleurs.
   - [I] Le mode persistance du GPU n'est ni réglé ni relevé (aucun `nvidia-smi -pm` dans `bench/` ni dans le worker).
2. **Médiane haute** pour 6 prises : +0,3 à +1,3 % sur les temps de la passation, dans les deux sens selon le bras.
3. **À chaud, un seul processus par bras et ordre fixe.** C'est le défaut le plus lourd. A/A explicite de
   `claudeo1place3` (deux processus du même binaire) : écart-type des log-rapports **0,042** sur le mur, 0,013 sur
   `domain`, 0,089 sur les forêts. La dispersion intra-processus, elle (médiane sur 336 processus K5), vaut 0,014 /
   0,007 / 0,032 par passe : elle prédit 0,008 / 0,004 / 0,018 pour un rapport de deux médianes de 9 passes. La
   variance entre processus est donc **27, 10 et 24 fois** plus grande. L'[I] de A § 10.2 est confirmé.
4. **Position dans la paire** (à froid, prise d'échauffement exclue) : être premier fait gagner 0,5 à 0,6 % en voie GPU
   (mur et `domain`, z ≈ −2,3). Le carré de Williams l'équilibre à froid ; rien ne l'équilibre à chaud.
5. **Statistique hors cible** : la passe unique CPU pour G1 (effet dilué), le froid pour un levier GPU (ouverture CUDA),
   le levier A jugé à K5/16 au lieu de K5/24. Ces trois cas sont reconnus dans les reçus.
6. **Chemins bifurquants** : `reservoir3` gardé malgré un critère non atteint ; confirmation du partage passée du froid
   au chaud, déclarée avant les données.

### 3.3 Bruit mesuré, prise d'échauffement exclue

Écart-type des log-rapports appariés par prise (σ_d, après retrait de l'effet moyen de chaque cellule ; 13 sessions,
156 degrés de liberté par ligne pour les étages) :

| Métrique | CPU (σ_d) | GPU (σ_d) |
|---|---:|---:|
| mur | 0,070 | 0,037 |
| `domain` | 0,096 | 0,034 |
| forêts | 0,082 | 0,079 |
| frontière (`prefix_ms`) | 0,044 | 0,051 |
| passe unique + frontière | 0,111 | 0,063 |
| cellules du publieur 5 | 0,095 | 0,096 |
| cellules + clôtures, publieur 5 | 0,118 | 0,099 |
| CPU du publieur 5 | 0,120 | 0,116 |

**Statistique du juge sous l'hypothèse nulle** (gm de 6 rapports de médianes de 6), estimée par onze A/A naturels :
des métriques qu'un levier ne touche pas, plus le bras A/A de `claudeo1place3`. Les gm d'étage vont de 0,984 à 1,018,
avec un écart-type de log **0,0105**. Sur les phases : `prefix` ≈ 0,011, passe unique + frontière ≈ 0,017–0,019,
publieur ≈ 0,016–0,024. L'**effet minimal détectable** (unilatéral 5 %, puissance 80 %, exp(−2,49·σ)) vaut donc
≈ 2,6 % sur un étage, 2,7 % sur la frontière, 4 à 6 % sur les phases du publieur.

### 3.4 Les cinq leviers retirés, rejugés sur les données brutes

| Levier (session) | Phase : G, seuil | z sous H0 | P(garder) si l'effet vrai = G | Étage : G (p de permutation) | Mur, froid : G (p) | Lecture |
|---|---|---:|---:|---|---|---|
| G1 AVX2 (`claudeg1`) | 0,895 ; 0,85 | −6,2 | 0,2 % | `domain` **0,964 (0,005)** | **0,983 (0,03)** | gain réel jusqu'au mur ; retiré **à tort** au regard des preuves |
| Frontière par tranches (`claudefront1`) | 0,859 ; 0,80 (36/36 paires négatives) | −14 | ≈ 0 | `domain` 0,980 (0,27) ; 0,966 sans échauffement | 0,985 (0,10) | réel mais ≈ 3 ms (≈ 1 % du mur) ; retrait défendable |
| Préchargement des graines (`claudepref1`) | 0,865 ; 0,85 | −7 à −9 | 13–19 % | forêts 0,986 (0,25) | 0,998 (0,88) | effet de phase réel qui **ne passe pas** à l'étage ; retrait non fautif |
| Préchargements combinés (`claudepref2`) | 0,950 ; 0,90 | −2,3 (perm. p = 0,067) | 0,7 % | forêts 0,983 (0,11) | 1,000 (0,98) | aucun effet établi |
| Annonces 1 024 (`claudeann1`) | 0,966 ; 0,90 | −1,4 (perm. p = 0,29) | 0,2 % | forêts 0,973 (0,24) | 0,980 (0,10) | aucun effet établi |

Lecture d'ensemble :
- Les effets de phase de G1, de la frontière et des graines se situaient à **6 à 14 écarts-types** du bruit de la
  statistique.
- La règle exigeait cependant 15 à 20 % sur la phase. C'était un objectif de taille d'effet, calé sur le local (G1 :
  −28 à −36 % à W8), et non un test.
- La garde « gm(étage) ≤ 1,00 » est presque sans pouvoir : un levier neutre la passe une fois sur deux, une régression
  de +1 % une fois sur six.
- Cette réanalyse est post hoc. Elle sert à rejuger le protocole, pas à réadopter un levier sans prises neuves.

### 3.5 Protocole chiffré pour la v12

N = nombre total de paires par mode (trois trames). MDE = 2,80·σ_d/√N (bilatéral 5 %, puissance 80 %).

| Métrique | N pour 2 % | N pour 3 % | N pour 5 % |
|---|---:|---:|---:|
| mur CPU / GPU | 96 / 26 | 42 / 12 | 15 / 4 |
| `domain` CPU / GPU | 176 / 23 | 78 / 10 | 28 / 4 |
| forêts (les deux voies) | 122–129 | 54–57 | 19–20 |
| frontière | 38–50 | 17–22 | 6–8 |
| phases du publieur | 172–277 | 76–122 | 27–43 |

1. **Unité de réplication : le processus**, à froid comme à chaud. Un premier processus GPU est jeté par session ; mieux
   encore, activer le mode persistance et le relever au reçu.
2. **À froid, K5 : 20 processus par bras, par trame et par voie**, soit 60 paires par voie. MDE : mur 2,5 % en CPU et
   1,3 % en GPU, forêts 2,9 %, `domain` 3,5 % en CPU et 1,2 % en GPU, phases du publieur 3,4 à 4,3 %. Coût mesuré :
   0,9 à 1,0 s par processus K5, soit environ 240 processus en 4 à 5 minutes. `claudeg1` n'a dépensé que 744 s de
   commandes sur 4 200. Les 10 processus proposés par la passation donnent des MDE plus grandes d'un facteur √2. À K10
   (≈ 5 s par processus) : 10 par bras, par trame et par voie, soit environ 10 minutes.
3. **À chaud : au moins 5 processus par bras et par trame**, en ordre entrelacé (Williams), 10 passes chacun, médiane
   des passes 2 à 10 par processus. Avec σ ≈ 0,042 entre processus, 30 processus par bras donnent un MDE de ≈ 2 % sur
   le mur.
4. **Statistique** : moyenne (ou Hodges–Lehmann) des log-rapports appariés par (répétition, trame, voie). Intervalle à
   95 % par bootstrap stratifié ou test de permutation par inversion de signe, comme ici. Rapport aussi par cellule
   (effets hétérogènes : G1 CPU 0,905–0,984, GPU 0,827–0,876).
5. **Règle** :
   - on adopte si la borne haute de l'intervalle sur l'étage visé est < 1, avec N choisi pour 80 % de puissance à
     l'effet attendu ;
   - on contrôle la non-infériorité sur le mur et les autres étages, à une marge de 1 % ;
   - on exige que l'effet de phase soit significatif, comme contrôle de mécanisme ;
   - correction de Holm si plusieurs métriques décident.
6. **Bras A/A dans chaque session**, avec une fenêtre de ±2 SE attendues (≈ ±1,5 % sur le mur pour N = 60), et non ±3 %.
7. **Leviers algorithmiques** : à W1 (σ_d ≈ 0,004 d'après un A/A à trois paires) ou sur compteurs. Trois paires y
   détectent 1 %.
8. **Petits gains cumulables** : ne pas les jeter. Les verser dans un lot candidat jugé en bloc contre la base, puis
   faire une ablation, sur prises neuves avec règle séquentielle déclarée.
9. **Juge unique en bibliothèque**, couvert par CTest, haché au lancement ; vraie médiane ; refus des cellules sans
   paire.

---

## 4. Canal d'audit

### 4.1 Registre des constats ouverts (au HEAD)

| ID | Constat | Lieu | Gravité | État |
|---|---|---|---|---|
| OUV-01 | Runner sans `--output-on-failure` ; `LastTest.log` préféré au `.tmp` | `tools/g4_matrix.py:537–538, 734–735` | moyenne (diagnostics perdus) | ouvert |
| OUV-02 | `--commit` recopié dans la provenance | `bench/sorties_g4.py:215, 251` | faible | ouvert ; erratum du reçu adopté |
| OUV-03 | Isolation des descendants non certifiée ; `gpu_ab` sans groupe de processus | `tools/g4_matrix.py:14–15, 749–754` ; `bench/gpu_ab.py:64–67` | moyenne | ouvert |
| OUV-04 | Clang jamais qualifié | VM : `clangxx` absent | faible | ouvert |
| OUV-05 | Portes natives q3 extrême, q4 à 2^20 et 2^20+1, préfixe obtus, qmin = 2 | `AUDIT_CONTRATS…:1858` ; couverture hôte `leaf_narrow` | moyenne | partiel |
| OUV-06 | Repli U2 parallèle approuvé, non fait | `src/catalogue/single_pass_batch.cpp:142` | moyenne (u24, nuages épars) | ouvert |
| OUV-07 | Mutants en u18 ; 17 jamais joués ; `--check` sans existence de porte | `tools/g4_matrix.json:48` ; `run_mutants.py:157–160` | haute pour la v12 | ouvert |
| OUV-08 | Cache de blocs : mémoire inactive et arrondi de classe hors budget (Y2) | `src/core/buffer.cpp`, `ccdd4db75` | moyenne | non relu |
| OUV-09 | Garde du préchargement O2 non mutable sans sanitizer (X, Y3) | `src/tower/forest_concurrent.cpp:80–86` | faible | non relu |
| OUV-10 | Différentiel canonique v10/v11 sur trames entières jamais fermé ; portes conditionnelles absentes des matrices | `reference/tests.cmake:124–130` | haute | ouvert |
| OUV-11 | Retrait des enveloppes M3/E4 de la voie CPU approuvé, non fait | `src/catalogue/leaf.cpp` | faible (+1,2 à 1,4 % de passe unique à W1) | ouvert |
| OUV-12 | Formule `8C⌈C/64⌉` au lieu de 16C | `docs/CATALOGUE.md:158` | faible | ouvert |
| OUV-13 | Lien mort | `receipts/audit_dialogues_20261004/README.md` | faible | ouvert |
| OUV-14 | Corrigendum du préenregistrement E1 manquant | `plans/e1_prereg_*` | moyenne | ouvert |
| OUV-15 | claudequalA attribuée à `b319efc84` au lieu de `00bd979ac` | `receipts/developpement_20261005/qualification_sorties/README.md:13` | faible | erratum |
| OUV-16 | Identité du compte dans 528 fichiers et 91 archives | `gcp-migration/v11_session.py:1063, 1134–1142` ; chemins `/home/…` du worker | moyenne (confidentialité) | décision de l'utilisateur |
| OUV-17 | Une porte lit un reçu | `bench/full_baseline_source.py:12` | faible | ouvert |
| OUV-18 | Juges de banc ad hoc non hachés ; `gpu_ab`/`ab_g4` sans autotest CTest | `bench/` | moyenne | ouvert |
| OUV-19 | SPv2 : mutants, TSan et K10 non joués | `tests/mutants/supports.json`, `cli.json` (`sp_*`) | moyenne | ouvert |
| OUV-20 | Réservoir : épuisement après allocations partielles non porté en porte | `receipts/audit_reservoir_followup_20261006/README.md` | faible | [I] statut non relu depuis |
| OUV-21 | « Mutants de `b0f2a0a9e` à qualifier » (`audits/README.md`) | — | — | dépassé : tués à `38faaf272` ; à clore |

### 4.2 Questions sans réponse

| Groupe | Contenu | Remarque |
|---|---|---|
| T1 | Prédicteur exact et bon marché du travail d'une feuille | `REPONSE_CLAUDE_SUPPORTS` § T |
| T2 | Le découpage des seules feuilles lourdes est-il couvert par les preuves Q1–Q2 ? | ce n'est pas une question de « prédicteur », contrairement à P § 8 |
| V1 | Invariant structurel par nœud de l'arbre radix | § V |
| V2 | Un contrat de budget supposait-il la forme médiane ? | § V |
| X (= Y3) | Rendre mutable la garde du préchargement d'O2 | § X, § Y |
| Y1 | Cause de la perte des pages de 2 Mio sur 48 fils | § Y |
| Y2 | Compte honnête de la réutilisation des blocs | tranchée seule par `ccdd4db75` |
| Polyèdre (8) | Huit questions, déposées à 02 h 32 UTC le 7 oct. | `audits/REPONSE_CLAUDE_POLYEDRE_ORDRE_K_RESULTATS_20261007.md:56–74` |

U1 et U2 ont reçu réponse (`AUDIT_CONTRATS…:42–80`). B.6 (partition T > 0) a reçu une proposition (QE contre Dr,
4B+5+T, 6 219 gardes `Fraction`), jamais mesurée.

### 4.3 Commits non relus

Ce sont les 40 commits postérieurs à `28d70f8ab`. Ceux qui touchent un contrat ou la concurrence :
- `13a4a0a4c` (O2) ;
- `2045ec27c` (exécuteur partagé : un `std::thread` par lot en parallèle du Pool, fusion dans l'ordre du lot,
  `MemoryBudget` partagé par les deux côtés) ;
- `5734ca6e8` (tranches de cohortes, course évitée en relecture interne) ;
- `ccdd4db75` (cache) ;
- `0ff64512a` (lectures d'horloge échantillonnées dans les publieurs) ;
- `c80c12012` (sous-chronos de la frontière, resté au HEAD).

La passation n'en nomme que deux.

### 4.4 Avis sur `ccdd4db75` (cache de blocs)

Lu : `src/core/buffer.cpp` et `src/core/buffer.hpp`, la porte `block_cache`, cinq mutants, le reçu `cache_blocs`.

- **Concurrence : risque faible.**
  - Un seul mutex par cache.
  - Le bloc est empoisonné avant d'être publié, désempoisonné après retrait, dans les deux cas en propriété exclusive.
  - L'invariant `idle ≤ capacity` est tenu sous le verrou ; la soustraction non signée est donc sûre.
  - Le destructeur rend les blocs inactifs.
  - Couverture : TSan 804/804 et une porte à 8 fils.
- **Exactitude : risque faible pour la sortie, non nul.**
  - Le cache est éteint dans le produit (l'API ne le crée pas ; seule la sonde l'active, `bench/full_probe.cpp:345`).
  - « Un Buffer n'initialise pas ses éléments » (`buffer.hpp:21`). Un bloc repris contient donc les octets de son
    usage précédent, et non les zéros que rend en pratique un `mmap` neuf : une lecture-avant-écriture latente
    deviendrait non déterministe.
  - Le profil `poison`, qui remplit en `0xA5` (`buffer.cpp:161–162`), n'a jamais tourné avec le cache.
  - Garde empirique : 0 écart sur toutes les prises en modes de référence depuis le 7 oct. (5 sessions).
- **Contrat mémoire : risque moyen, et c'est le vrai point.**
  - `used` et `peak` ne voient ni les blocs inactifs (jusqu'à 4 Gio dans la sonde), ni l'arrondi de classe : jusqu'à
    +9 % par bloc vivant, un pas de 2^(1/8).
  - L'empreinte réelle peut donc atteindre limite + cache + 9 %. Le pic publié ne la borne plus : c'est le défaut de
    « plafond menteur » relevé en v4.
  - La porte n'utilise que des budgets `kUnlimited` : aucun refus sous limite finie n'est testé.
  - C'est l'inverse de la proposition Y2 du développeur, faite une heure et demie plus tôt (« compté dans le budget
    comme réserve »), et cela n'a pas été relu.
- **Recommandation v12** : un cache compté (réserve admise au budget), arrondi compris, rendu à la fermeture de la
  `Session` ; une porte « limite finie + cache » ; une passe `poison` avec le cache actif.

### 4.5 Avis sur `13a4a0a4c` (publieur O2, première partie)

Lu : le diff de `cells.*`, `forest_build.cpp`, `forest_concurrent.cpp`, `forest_internal.hpp`, `forest_plateau.cpp`, et
le reçu `o2_publieur`.

- **Exactitude : risque faible.**
  - Le parent DSU sort de `ForestState` vers un tableau dense. Le champ supprimé rend toute lecture oubliée
    incompilable.
  - Les trois initialisations (`site_births`, `birth_block`, `prepare_states`) et les admissions au budget passent à
    `kForestStateBytes` = 20 + 2·4 = 28 octets, inchangés.
  - `find`, `unite_roots` et `close` lisent `parents`.
  - Deux mutants sont tués, dont un nouveau sur `parents[s]`. Les initialisations de `site_births` et
    `prepare_states` ne sont pas mutées.
  - Les vidages sont identiques sur toutes les prises ultérieures (§ 1, n° 19).
- **Concurrence : correcte par raisonnement, non épinglée par une porte.**
  - `prefetch_seeds` (`forest_concurrent.cpp:80–86`) ne lit `seeds[4·job+i]` que si
    `job / width < confirmed`.
  - `confirmed` n'avance que dans `await_job` (l. 90–99), après lectures `acquire` de l'état du bloc. Il vaut un
    préfixe contigu, parce que le publieur attend chaque bloc dans l'ordre et que chaque bloc contient au moins un job.
  - La lecture est donc ordonnée après l'écriture des résolveurs.
  - Un mutant `>=` → `>` provoquerait une course sur `seeds` (UB) sans effet visible sur la sortie, puisque seule
    l'adresse préchargée change. Seul TSan peut la voir (800/800 joué une fois).
- **Recommandation** : extraire le prédicat de garde en fonction pure testée à la borne `job/width == confirmed`, et
  ajouter un compteur de débogage « lectures hors confirmé = 0 ».

---

## 5. Exploitation G4

### 5.1 Garde-fous vérifiés dans `gcp-migration/v11_session.py`

| Garde-fou | Lieu |
|---|---|
| Cible exacte : nom, zone, `selfLink`, label `project=e-hgp`, `g4-standard-48` | l. 856–862 |
| SPOT, action STOP, maintenance TERMINATE, `automaticRestart=false` | l. 864–866 |
| OS Login exigé | l. 869–871 |
| `maxRunDuration` dans [30 s ; 8 h] et égal à `--max-run-seconds` | l. 114, 872–878 |
| Clé de session à TTL max_run + 5 min | l. 952 |
| Scripts gardés épinglés par SHA | l. 93 |
| Verrou partagé avec la v10 | l. 105 |
| Fermeture par la génération prouvée | — |
| Réserve disque préallouée | — |
| Codes 0 / 2 / 3 / 74 / 75 / 76 | — |

`--recover` ferme et retire la clé, mais **ne rapatrie rien** (l. 2125–2229). Les VM ont tourné sous glibc 2.35,
noyau 6.8.0-1070-gcp, ≈ 177 Gio de RAM (`MemTotal` 185 463 724 kB, en-têtes `host` de `gpu_ab`).

### 5.2 Pièges payés, vérifiés

| Piège | Où |
|---|---|
| Capacité épuisée (`ZONE_RESOURCE_POOL_EXHAUSTED`, `STOCKOUT`) : trois démarrages manqués le 2 oct. en `us-central1-b`, un SPOT le 6 oct. en `us-central1-c` | `external_closure.json` des paquets meb2, parallel1, parallel2 ; `reservoir_cases_chainees/README.md` |
| Nouvelle VM sans outils | `tools1` |
| Préemption SPOT au démarrage | claudeab5 et claudeab6 |
| CMake 3.22 sans dialecte CUDA20 | claudegpu1 |
| Garde disque à quelques ko près | graph3 |
| Contrôleur perdu, fermé par `--recover` sans résultats | graph4, claudeflat2 |
| Un vidage de 272 Mo au-delà du plafond des résultats | `reservoir_cases_chainees/README.md:24–25` |
| Matrices coupées à l'échéance | fina2, claudequalb, `release_long` |
| `setarch -R` pour TSan (mesure du codespace) | — |

### 5.3 Hygiène des reçus

- **Volume** : 357 Mo, dont 228 Mo d'archives de paquets (les trois plus grosses, 32 Mo chacune, dans
  `qualification_performance_20261003/captures/sources/`) et 2 041 fichiers C++/CUDA copiés.
- **Identité du compte** : voir le n° 40 du tableau. Origines : `gcloud_account` écrit au préflight (l. 1063),
  `CLOUDSDK_CORE_ACCOUNT` dans `recovery_command` (l. 1134–1142), chemins personnels de la VM dans les résultats de
  matrice et les reçus.
- **Juges non hachés** au lancement.
- **Deux noms de session réutilisés** à des dates différentes (`claudediag1` le 4 et le 7 oct.).

### 5.4 Ce que la v12 doit changer

1. **Reçu sans identité.** Remplacer les chemins personnels par `$HOME` ou un jeton, et lire le compte au lancement
   sans l'écrire.
2. **Reçu sans arbres.** Citer un SHA et un manifeste ; une archive seulement pour un instantané, et alors hors dépôt.
3. **Reprise qui rapatrie** depuis le disque de la VM, quand c'est possible, avant l'arrêt.
4. **Préflight de mesure**, inscrit au reçu :
   - mode persistance du GPU, gouverneur et boost du CPU ;
   - `uptime` et charge ;
   - un processus d'échauffement jeté.
5. **Banc avec groupe de processus et quiescence**, comme `ab_g4.py`. Juge en bibliothèque, haché au lancement.
6. **Matrice à la VM** : profil produit u21 pour `gcc_release` et pour les mutants, u24 en matrice ; lots de 30 minutes
   au plus ; `--output-on-failure` ; portes conditionnelles (v10) déclarées « absentes », jamais silencieuses.
7. **Registre de dette de qualification commit par commit**, alimenté par le reçu de chaque session.
8. **Durée maximale** : la cible n'accepte que 4 200 s ; dimensionner les sessions en conséquence, ou étendre
   l'allowlist du script gardé, ce qui exige une revue.

---

## 6. Corrections à reporter dans la passation

1. § 2.1 : préciser « médiane haute » et n = 3 à K10.
2. § 1 et A § 1 : le jalon de 200 ms a été franchi (196,8 ms, `@tas`).
3. A § 3.4 : les K10 viennent de `claudegpu3` et non de `records.json`.
4. A § 1 et § 6.1 : remplacer « 372 / 84 » par 1 602 / 378 prises GPU, 0 écart.
5. P § 2.3 : ajouter `38faaf272` ; SPv2 est qualifiée en u18, u21 et u24 hors mutants et TSan ; le code exact du HEAD
   n'a passé aucune matrice ; 17 mutants n'ont jamais été joués.
6. P § 4 et A § 10.2 : seuls G1 (clairement) et la frontière (faiblement) sont des pertes. Combinés et annonces sont
   sans effet établi. Le défaut tient aux seuils, non à la puissance.
7. A § 11 : sessions 138 / 132 / 4 ; reçus 357 Mo ; identité dans 528 fichiers et 91 archives.
8. A § 2 et § 12 : ≈ 100 commits d'auditeurs ; quatre commits d'auteur `Claude`.
9. A § 3.3 : la trajectoire mêle meilleures passes et médianes ; la dérive entre sessions est de ±1 à 3,5 % le 7 oct.
10. P § 8 : T2 n'est pas une question de prédicteur ; ajouter `2045ec27c`, `5734ca6e8` et `0ff64512a` aux commits
    sensibles non relus.

*Pièces de travail* : `scratchpad/agent_H/scripts/{levers,robust,perm,boot,power,warm,position,aa_phase,first_take,inventory}.py`
et leurs sorties `*_out.txt`. Aucune n'est versée au dépôt.
