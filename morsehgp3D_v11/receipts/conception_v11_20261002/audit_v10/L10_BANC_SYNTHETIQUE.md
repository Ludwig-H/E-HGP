# L10 — Bancs synthétiques et revendications contre HDBSCAN (audit de `morsehgp3D_v10` en vue de la v11)

Rédigé le 2 octobre 2026, 07 h 58 UTC (heure lue par `date -u`). Auditeur de la lentille L10.

```text
phase=audit_v10_pour_v11_hors_registre
sujet=morsehgp3D_v10, origin/main afb081774, lu dans build/v11-worktree (lecture seule)
lentille=L10 bancs synthetiques et revendications contre HDBSCAN
mode=audit (calculs sous /tmp/v11-audit/l10_banc_synthetique, 4 fils au plus)
public_status=not_claimed
GCP non utilisé
```

Aucune commande Git mutante, aucune commande GCP, aucun fichier écrit dans le dépôt ni dans les dossiers privés.
Seuls écrits sous `/workspaces` : ce rapport et `build/v11-persist/audit_v10/preuves_l10_banc_synthetique/` (moins de 200 Ko).

## 0. Réponse courte

1. **Les reçus préenregistrés tiennent comme mesures.** Lots A et C : pins conformes avec les artefacts locaux, plan exact (960 scènes, 0 refus), graines dev et test disjointes, moyennes, écarts et comptes recalculés à l'identique par un code indépendant, 8 scènes rejouées ici (tour 120 lignes sur 120 et témoin 40 sur 40 identiques au reçu G4).
2. **Ils ne tiennent pas comme preuve que « la tour bat HDBSCAN ».** Cinq réserves, toutes vérifiées ici :
   - à K = 8 et 10, l'écart du lot C vient de la seule famille `shells` ; sans elle il est nul ou négatif, et la tour perd la majorité des scènes ;
   - sur `shells`, l'adversaire sklearn retenu par la règle dev (feuilles, α = 2) s'effondre, alors que sklearn en EOM y égale ou dépasse la tour ;
   - à même entrée et même tête, la hiérarchie d'atteignabilité mutuelle égale la tour (famille « objet ») : l'avantage vient de la règle d'entrée et de l'exposant z, pas de l'objet exact ;
   - l'avantage n'existe qu'à grand `min_cluster_size` (√n ou 40) ; à mcs = K ou 10, la tête `cover` perd nettement (dev) ;
   - l'adversaire est bridé à `min_samples` = K ≤ 10 et à mcs = √n ; réglé librement (dev), il revient à +0,015 de la tour, médiane nulle.
3. **Ce qui est le mieux établi date du 1er au 2 octobre et n'est que du développement** : la tour contient un bon amas pour 73 à 76 % des groupes (IoU > 4/5), la hiérarchie `cover` le garde, la hiérarchie de sklearn en perd de plus en plus quand K monte (+0,005, +0,024, +0,050, +0,077 pour `cover` à K = 2, 3, 5, 10). Recalculé ici à l'identique. Aucun test scellé depuis le lot C ; le contrôle « même règle d'entrée sur la hiérarchie de HDBSCAN » manque à ce niveau.
4. **Un exemple où l'objet exact fait la différence est vérifié** : les deux triangles de la thèse (§ 6.1), en position générique. La tour à K = 3 rend ABC | DEF ; sklearn et le témoin d'atteignabilité mutuelle (32 configurations) ne les rendent jamais.
5. **L'outil de décision du dépôt est fautif** : `decide.py` au HEAD décide sur 4 scènes sur 960, accepte un ARI de 1,25 et un seuil alpha = 2. Le correctif existe dans une copie privée (R2), pas dans `origin/main`. Aucune porte ne juge les scripts du banc.
6. **Le banc v11 est à moitié écrit, hors dépôt** : plan fractionnaire 500 à 32 000 points et 2 à 20 groupes, référence MAP vérifiée sur 8,2 millions de sites, métriques IoU avec précision et rappel, collecteur strict. À porter explicitement, avec des niveaux de difficulté qui ne soient plus définis par l'échec de HDBSCAN.

## 1. Périmètre lu

Dans `build/v11-worktree/morsehgp3D_v10/` (HEAD du sujet) :

- `bench/synthetic/` : `scenes.py`, `methods.py`, `metrics.py`, `run_campaign.py`, `run_test.py`, `decide.py`, `choose_config.py`, `prereg/README.md`, `prereg/PREREG_V10_KMATCH_A_20260928.json`, `prereg/PREREG_V10_COVER_C_20260929.json` ; `bench/g4/merge_sessions.py` ;
- `docs/conception/EVAL_v2.md` (§ R, 0 à 11, 13 à 15) ; `PASSATION.md` ; `CMakeLists.txt` (portes) ; `reference/hgp10_ref.py` ;
- reçus : `test_kmatch_A_20260929`, `test_cover_C_20260929` (README, DECISION, `results.csv`, concordances), `bench_dev_20260928`, `bench_dev_selection_20260928`, `bench_dev_fill_20260928`, `bench_dev_kmatch_20260928`, `bench_dev_cover_20260929`, `bench_dev_z_samehead_20260929`, `bench_dev_objet_20260929`, `bench_dev_shrink_20260929`, `bench_dev_alloc_20260929`, `bench_dev_bigk_20260929`, `ERRATA.md`, `audit_continu_20260929/point_condensation_20260930`, `audit_independant_20260929/historique/base_6206d1d11/TETE_BANCS_PREUVES.md`, `audit_independant_20261002/battery_review`, `audit_geant_developpeur_20260930/TRACKER.md` ;
- audits : `audit_hierarchie_knn_20260929/`, `tete_multik_20260929/` (README et rapport du juge), `AUDIT_ETAT_COURANT.md` (tête), `SUIVI_AUDIT_INDEPENDANT.md`, `audit_independant_20260929/CONTRE_AUDIT_TETE_BANCS.md`, `REPONSE_CLAUDE_BATTERIE_TOUR_HIERARCHIES_20261002.md`.

Hors dépôt (lecture seule) :

- `build/v10-tour-vers-points/` : `DIAGNOSTIC/RAPPORT_DIAGNOSTIC.md` et `RESUME_AGENT_DIAGNOSTIC.md`, `selection/RAPPORT.md` et `selection/fusion/tvpc.flat.csv.gz`, `batterie_ab/RAPPORT_TOUR_PUIS_HIERARCHIE.md`, `VERIFICATION_mesures.md`, `tables/groupes_A_B.csv.gz`, `mesure_vc/RAPPORT_MESURE_VC.md` et `VERIFICATION.md`, `banc2/plan_map/README.md` et `VERIFICATION.md`, `code/` ;
- `build/v10-comparaison-pr/` (protocole et lot dev du 1er octobre), `build/v10-integration-r2/` (copie R2 de `decide.py`), `build/v10-lotC-bc413ff56/`, `build/v10-bench-c764e121a-static/` ;
- `build/workflows/EXIGENCES_TESTS_UTILISATEUR.md` ; notes de mémoire (`clustering-depuis-la-tour.md`, `hdbscan-echoue-deja-k2.md`, `exposant-z-ponderation.md`, `hdbscan-sklearn-reference.md`) ;
- `build/v9-open-worktree/morsehgp3D_v9/experiments/synthetic_bench_20260928/` (générateur et calibration d'origine) ;
- source installée de scikit-learn 1.9.1, `sklearn/cluster/_hdbscan/hdbscan.py` (empreinte `6f030ac6b5bfbb88…`, celle qu'épingle EVAL_v2).

Non lu ou survolé : `GEN_v2`, `TOWER_v2`, `ARCH_v2`, `CLUSTER_v2` (autres lentilles) ; les règles `regles2/` (ER0h, vote, maturité) ne sont lues que par leurs résumés ; le LiDAR relève de la lentille L11.

## 2. Méthode

- **Lu** : le texte ou le code est cité avec son chemin et sa ligne.
- **Exécuté** : un calcul refait ici avec mon propre code, ou un outil du sujet lancé sur une entrée construite pour l'épreuve.
- **Mesuré** : une expérience nouvelle, petite, sur graines dev ou sur une fixture.
- Statut d'un résultat du sujet : **préenregistré** (test scellé, une exécution), **dev vérifié** (développement, relu par un vérificateur ou un auditeur), **dev seul**.

Les graines `test` et `test_v10b` n'ont servi qu'à rejouer des lignes déjà publiées (8 scènes du lot C, scripts et binaires épinglés) : aucun bras nouveau n'a été mesuré sur elles. Toute analyse nouvelle des reçus de test est marquée « a posteriori » : elle ne remplace pas la règle préenregistrée, elle en mesure la robustesse.

| Contrôle exécuté | Script (dossier de preuves) | Sortie |
| --- | --- | --- |
| Recalcul des lots A et C sans `decide.py` | `recalcul_lots.py` | `recalcul_lots.txt`, `lotC_shells_par_n.txt`, `lotC_sans_shells_par_n.txt`, `lotC_par_niveau_sans_shells.txt` |
| Rejeu de 8 scènes du lot C, artefacts épinglés | `replay_lotC.py`, `replay_sklearn_only.py` | `replay_lotC_8000.log`, `replay_sklearn_only.log` |
| `decide.py` du HEAD et de la copie R2 sur trois lots fautifs | `decide_defauts_head/COMMANDES.txt` | `decide_defauts_head/` (quatre sorties) |
| sklearn : égalités, ordre des lignes | `sklearn_egalites.py` | `sklearn_egalites.txt` |
| Calibration, graines, `hdb_ms1` = `hdb_ms2`, métriques, centres, epsilon | `controles_divers.py` | `controles_divers.txt` |
| Effet du défaut de condensation et de mcs (lot dev du 1er octobre) | lecture de `results.csv` | `tt1_impact_dev.txt` |
| HDBSCAN réglé librement contre la tête v10-b (dev, 8 000 points) | lecture des reçus dev | `dev_hdbscan_libre.txt` |
| Batterie A/B : agrégats de tête, puis par famille, niveau, bruit, n, groupes | `recalcul_batterie_ab.py`, `recalcul_batterie_ab_facteurs.py` | `recalcul_batterie_ab.txt`, `recalcul_batterie_ab_facteurs.txt` |
| Étage sélection : moyennes de tête et table par mcs | `recalcul_selection.py` | `recalcul_selection.txt` |
| Empreinte flottante des scènes, ici contre G4 | lecture de la table et régénération | `digest_flottant.txt` |
| Deux triangles : oracle exact, sklearn, tête v10, témoin MR | `deux_triangles.py` | `deux_triangles.txt` |

Machine : codespace de 8 cœurs, charge 18 à 27 pendant l'audit. Aucun temps n'est rapporté comme mesure.

## 3. Constats

Gravité : **bloquant** (rend faux ou invalide un résultat ou un contrat), **majeur** (à traiter dans la conception v11), **mineur**, **info**. Aucun constat n'est bloquant : rien n'est revendiqué publiquement (`not_claimed`) et les mesures publiées sont exactes. Ce sont leurs lectures qui débordent.

### L10_BANC_SYNTHETIQUE-01 — Lot C : à K = 8 et 10, « la tour bat HDBSCAN » tient à la seule famille `shells` (majeur)

- **Fait.** Recalcul a posteriori du lot C (960 scènes de test, tête v10-b remplie contre sklearn réglé sur dev) :

| K | Écart moyen publié | Médiane | Gains / pertes | Écart sans `shells` (840 scènes) | Gains / pertes sans `shells` |
| ---: | ---: | ---: | --- | ---: | --- |
| 1 | +0,075 | +0,002 | 559 / 326 | +0,087 | 558 / 258 |
| 2 | +0,073 | +0,003 | 595 / 287 | +0,084 | 587 / 235 |
| 3 | +0,091 | +0,009 | 673 / 287 | +0,031 | 553 / 287 |
| 5 | +0,064 | +0,003 | 639 / 315 | +0,013 | 519 / 315 |
| 8 | +0,040 | −0,0003 | 455 / 501 | −0,001 (p = 0,69) | 335 / 501 |
| 10 | +0,031 | −0,0003 | 454 / 498 | −0,004 (p = 0,14) | 335 / 497 |

  - À K = 8 et 10, le `DECISION.md` du reçu montre lui-même cinq familles sur huit en perte significative (Holm) : `anisotropic` −0,038 et −0,040, `filaments` −0,032 et −0,054, `bridge`, `hierarchical`, `spherical` −0,007 à −0,009. La phrase de tête dit « sans perte significative ailleurs » : « ailleurs » y désigne les autres paires de K, pas les familles, et le lecteur ne le voit pas.
  - À K = 5, l'écart sans `shells` (+0,013) passe sous la marge préenregistrée de 0,02.
  - Lot A, K = 3 : +0,049 publié, −0,018 sans `shells`.
  - « L'écart croît avec n » (README du reçu) : sans `shells`, il est plat à K = 8 et 10 (−0,002, 0,000, −0,001 puis −0,004, −0,002, −0,005 à 8 000, 16 000 et 32 000 points).
- **Preuve.** `preuves_l10_banc_synthetique/recalcul_lots.txt`, `lotC_sans_shells_par_n.txt` ; `receipts/test_cover_C_20260929/DECISION.md:5`, `:131-164`.
- **Vérification.** Exécuté (recalcul indépendant ; test de retournement de signe à 20 000 tirages pour les p).
- **Conséquence v11.** Ne pas porter « la tour bat HDBSCAN à tous les K testés » comme un acquis. La règle de décision doit exiger la robustesse : écart sans chaque famille, médiane ou test de signe, part de scènes gagnées, aucune famille en perte significative pour dire « domine » (le niveau R2 d'EVAL_v2 § 6.6 le prévoyait).

### L10_BANC_SYNTHETIQUE-02 — Sur `shells`, l'adversaire retenu s'effondre ; sklearn en EOM y égale la tour (majeur)

- **Fait.** À K ≥ 3, la règle dev retient pour sklearn « feuilles, α = 2, remplissage ». Sur `shells` (8 coquilles), cette configuration rend 17 à 31 amas et un ARI_s de 0,36 à 0,81, qui baisse quand n monte. Sur les mêmes scènes, sklearn en EOM, α = 1, sans remplissage, rend 8,0 amas et, en moyenne par K, 0,968 à 0,983 contre 0,958 à 0,975 pour la tour : au-dessus d'elle à chacun des six K.

| K | n | Tour | sklearn réglé sur dev | sklearn EOM α = 1 |
| ---: | ---: | ---: | ---: | ---: |
| 3 | 8 000 / 16 000 / 32 000 | 0,966 / 0,973 / 0,979 | 0,556 / 0,464 / 0,357 | 0,977 / 0,979 / 0,985 |
| 10 | 8 000 / 16 000 / 32 000 | 0,939 / 0,963 / 0,972 | 0,811 / 0,692 / 0,551 | 0,967 / 0,958 / 0,980 |

  - Avec la meilleure des deux têtes sklearn mesurées **par famille**, sklearn devance la tour dans 6 familles sur 8 à K = 10 (lecture descriptive : c'est un oracle par famille, la tour n'en a pas).
  - Lecture juste du lot C : la tête EOM-z de la tour est plus régulière d'une famille à l'autre qu'aucune des deux têtes sklearn ; elle ne domine pas.
- **Preuve.** `lotC_shells_par_n.txt`, `recalcul_lots.txt` (table par famille).
- **Vérification.** Exécuté.
- **Conséquence v11.** L'adversaire ne peut pas être une seule configuration choisie par une moyenne sur toutes les familles. Garder au moins trois adversaires obligatoires : sklearn par défaut (EOM, α = 1), sklearn réglé à `min_samples` = K, sklearn réglé librement.

### L10_BANC_SYNTHETIQUE-03 — À même entrée et même tête, la hiérarchie de HDBSCAN égale la tour (majeur)

- **Fait.**
  - Famille « objet » du lot C (préenregistrée) : tour moins MR₂-bord = −0,0005, −0,0004, +0,0015, +0,0002 pour K = 2, 3, 5, 8, et +0,0098 à K = 10. Ce dernier écart vient de `shells` seule (+0,103) ; sans elle : −0,0034, avec 299 gains pour 515 pertes.
  - Le témoin héritait du z et du remplissage choisis pour la tour (`mrb_K10` : z = 6, b1.5), contre la règle D7 d'EVAL_v2 (témoin réglé pour lui-même). Le biais joue pour la tour, et il n'y a pourtant pas d'écart.
  - Dev, reçu `bench_dev_objet_20260929` : la meilleure coupe horizontale de MR₂-bord est supérieure ou égale à celle de la tour dans 14 cellules gaussiennes sur 16 (strictement dans 12).
  - Niveau B (batterie du 1er octobre) : les sources sont la tour, `cover`, `cover1`, `core`, HDBSCAN et deux témoins natifs. **Aucun témoin MR-bord.** L'écart `cover` moins HDBSCAN (+0,05 à K = 5) n'est donc pas attribué : `core`, qui a la même règle d'entrée que HDBSCAN, est **sous** HDBSCAN (0,747 contre 0,776).
- **Preuve.** `recalcul_lots.txt` ; prereg C, liste `methods` ; `receipts/bench_dev_objet_20260929/README.md` (table du § 2) ; `recalcul_batterie_ab.txt` (sources présentes).
- **Vérification.** Exécuté (lot C, batterie) ; lu (dev objet).
- **Conséquence v11.** Ce que la v10 a montré contre sklearn est l'effet d'une **règle d'entrée** (première couverture, analogue des points-bord de DBSCAN) et d'un **exposant z**, tous deux applicables à la hiérarchie de HDBSCAN. La v11 doit garder le témoin MR-bord aux trois niveaux, sinon elle ne saura pas ce que l'objet exact apporte.

### L10_BANC_SYNTHETIQUE-04 — L'avantage en coupe plate n'existe qu'à grand mcs (majeur)

- **Fait.** Les lots A et C fixent mcs = √n des deux côtés (89 à 179 points). Sur dev, à mcs égal, EOM, sans epsilon ni remplissage (768 scènes, mIoU, K = 5) :

| mcs | HDBSCAN | `cover` z = 1 | `cover` z = 2 | `cover` z = 3 |
| --- | ---: | ---: | ---: | ---: |
| K | 0,556 | 0,388 | 0,177 | 0,087 |
| 10 | 0,612 | 0,592 | 0,588 | 0,452 |
| 20 | 0,639 | 0,664 | 0,704 | 0,703 |
| 40 | 0,658 | 0,707 | 0,730 | 0,738 |
| √n | 0,650 | 0,711 | 0,730 | 0,739 |

  - Avec les z du lot C (4 à 6), l'écart d'ARI_s à sklearn vaut +0,03 à +0,11 à mcs = √n, mais −0,02 à −0,21 à mcs = 20, −0,28 à −0,52 à mcs = 10, et −0,44 à −0,54 à mcs = K pour K ≥ 3 (96 scènes dev, quatre niveaux).
  - Cause décrite par le développeur : en entrée `cover`, toute boule de K points fonde une feuille de masse K. Sur les cinq trames LiDAR : 5 732 amas par trame contre 867 pour HDBSCAN à mcs = K.
- **Preuve.** `recalcul_selection.txt` (identique au § 5.2 de `selection/RAPPORT.md`), `tt1_impact_dev.txt`.
- **Vérification.** Exécuté (recalcul en flux de `tvpc.flat.csv.gz` et du lot dev PR).
- **Conséquence v11.** Toute comparaison se publie sur la grille mcs ∈ {K, 10, 20, 40, √n}, jamais à un seul mcs. Le verrou à lever avant tout nouveau test scellé est l'existence des amas à petit mcs (c'est le régime du LiDAR).

### L10_BANC_SYNTHETIQUE-05 — Appariement K = `min_samples` : convention juste, adversaire bridé (majeur)

- **Fait.**
  - Convention vérifiée : sklearn compte le point (`hdbscan.py:355`, `kneighbors(X, min_samples)` sur l'échantillon lui-même ; note de la classe, lignes 597 à 601). Le niveau d'entrée d'un point est donc le même des deux côtés à K = `min_samples`.
  - Conséquence peu dite : à α = 1, `min_samples` = 1 et 2 donnent la même hiérarchie (la distance-cœur à 2 est la distance au plus proche voisin). Dans le lot C, `hdb_ms1` et `hdb_ms2` sont identiques sur 960 scènes sur 960. La paire « K = 2 » oppose donc la tour d'ordre 2 à la liaison simple, et le lot C n'oppose que cinq adversaires distincts aux six ordres de la tour.
  - L'adversaire est privé de son meilleur `min_samples`. Le 28 septembre, sklearn réglé librement (12 ou 20) égalait ou devançait la tour de 0,004 ; le premier projet de préenregistrement, qui l'opposait à la tour, a été retiré sur la directive K = `min_samples`. Recalcul sur dev, 128 scènes de 8 000 points : meilleur sklearn libre (`min_samples` = 12, feuilles, α = 2, b2.5) 0,7813 ; tête v10-b à K = 5 : 0,7965. Écart apparié +0,015, médiane 0,000, 62 gains pour 64 pertes ; `shells` +0,112, `unbalanced` +0,080, `filaments` −0,043, `anisotropic` −0,020.
  - Cette comparaison n'a jamais été testée sur un espace scellé. Le reçu `bench_dev_bigk_20260929` ne couvre que sklearn en feuilles α = 2.
- **Preuve.** `controles_divers.txt` (§ 3), `dev_hdbscan_libre.txt` ; `bench/synthetic/prereg/README.md:74-77`.
- **Vérification.** Lu (source sklearn) ; exécuté (lot C, reçus dev).
- **Conséquence v11.** Publier les deux lectures : appariée (demande de l'utilisateur) et adversaire libre (« il est impératif d'être meilleur que HDBSCAN » ne se juge que contre le meilleur réglage de HDBSCAN). Dire que K = 2 chez sklearn est la liaison simple.

### L10_BANC_SYNTHETIQUE-06 — Niveaux de difficulté définis par l'échec de HDBSCAN, non comparables entre nombres de groupes (majeur)

- **Fait.**
  - `scenes.py:21-58` : les séparations sont calibrées pour que HDBSCAN(min_cluster_size = 20) tombe à un ARI de 0,95, 0,70, 0,40 et 0,15 à n = 2 000, 8 groupes. Reproduit ici sur trois graines : `hard` 0,30 à 0,48 ; `extreme` 0,14 à 0,19 (0,46 pour `hierarchical`). La moitié du banc est, par construction, un ensemble de cas où un réglage de HDBSCAN échoue.
  - Dans le lot C à K = 1 et 2, la tour perd en `easy` (−0,07) et gagne +0,22 et +0,20 en `extreme`.
  - `centres(g)` normalise la distance minimale à 1, mais le nombre de voisins à cette distance change avec g : 1,5 en moyenne à 8 groupes, 0,33 à 12, 0,9 à 20. Un même niveau n'est pas la même difficulté à 8 et à 12 groupes ; le creux à 8 groupes de la batterie (IoU 0,789) en porte la trace.
  - La calibration est faite à n = 2 000 et appliquée jusqu'à 32 000 points.
- **Preuve.** `controles_divers.txt` (§ 1 et 4) ; `recalcul_lots.txt` (table par niveau) ; `bench/synthetic/scenes.py:21-58`, `:92-109`.
- **Vérification.** Exécuté.
- **Conséquence v11.** Appliquer la décision D4 d'EVAL_v2, jamais implantée : niveaux définis sans aucune méthode (niveau de Bayes du MAP pour les familles à recouvrement, rapport d'écart pour les supports disjoints), calibrés par (famille, nombre de groupes), avec le nombre de voisins proches publié.

### L10_BANC_SYNTHETIQUE-07 — `decide.py` au HEAD décide sur un lot incomplet ou invalide ; le correctif n'est pas dans le dépôt (majeur)

- **Fait.** `bench/synthetic/decide.py` au HEAD a l'empreinte `d360ed0e…`, celle qu'épingle le lot C. Trois épreuves :
  - lot de 4 scènes sur 960, `complete=false` dans `run.json` : code 0, `DECISION.json` et `DECISION.md` écrits ;
  - ARI_s = 1,25 sur toutes les lignes de la tour : code 0 ;
  - préenregistrement à `alpha = 2`, `delta_min = −1` : code 0 et la phrase « La tour bat HDBSCAN pour K=5, K=8 ».
  - `decide.load` ne cherche les méthodes manquantes que parmi les scènes présentes (`decide.py:35-46`) ; `main` ne lit ni `complete`, ni le plan (`:136-144`). `bench/g4/merge_sessions.py` n'exige pas qu'une unité soit dans le plan.
  - La copie R2 (`build/v10-integration-r2/src/morsehgp3D_v10/bench/synthetic/decide.py`, `4d1265f0…`) refuse les trois cas (code 2) et accepte le lot C complet (`--check-only`, code 0). Elle n'est pas dans `origin/main` ; `tests/regression/` n'y contient pas `test_decide_completeness.py`.
  - Aucune porte CTest ne juge `metrics.py`, `scenes.py`, `decide.py` ni `run_test.py` : le harnais d'EVAL_v2 § 9 (portes EG1 à EG18, dix-huit mutants) n'a pas été écrit.
  - Les lots A et C archivés ne sont pas touchés : plans exacts, 7 680 et 30 720 lignes, vérifiés ici et par l'auditeur indépendant.
- **Preuve.** `preuves_l10_banc_synthetique/decide_defauts_head/` (quatre sorties) ; `sha256sum` du HEAD ; constats E1 et BN1 des auditeurs.
- **Vérification.** Exécuté.
- **Conséquence v11.** Porter la version R2, pas celle du HEAD ; y ajouter ce que BN1 laisse ouvert (schéma complet du préenregistrement, unicité des noms et des colonnes). Le harnais du banc reçoit ses propres portes et mutants avant la première campagne.

### L10_BANC_SYNTHETIQUE-08 — sklearn HDBSCAN dépend de la machine et de l'ordre des lignes (majeur)

- **Fait.**
  - Cause lue : `hdbscan.py:165`, `np.argsort` (tri non stable) sur les poids de l'arbre couvrant. Sur la grille entière, les poids de fusion ont des égalités exactes dès K = 3 à α = 1, et dès K = 2 à α = 2 (mesuré sur trois scènes de 2 000 points : 70 à 429 paires consécutives égales parmi 1 999 poids triés ; aucune à K = 1, ni à K = 2 avec α = 1).
  - Rejeu de 8 scènes du lot C avec les scripts et binaires épinglés : tour 120 lignes sur 120 et témoin MR 40 sur 40 identiques au reçu G4 ; sklearn 47 lignes sur 96 différentes, jusqu'à 0,036 d'ARI_s (`filaments`, `hdb_ms3` : 10 amas ici, 9 sur G4), écart absolu moyen 0,0008. Le reçu annonçait « au plus 0,0028 » sur 59 scènes.
  - Même machine, lignes permutées : jusqu'à 0,088 d'ARI_s sur une scène `shells` (feuilles, α = 2, K = 3) ; moins de 0,002 ailleurs.
  - Les décisions des lots sont appariées sur une seule machine : elles n'en dépendent pas. Les moyennes par cellule bougent de 0,003 au plus (vérificateurs du développeur).
- **Preuve.** `replay_lotC_8000.log`, `replay_sklearn_only.log`, `sklearn_egalites.txt` ; `receipts/test_cover_C_20260929/concordance_local_g4.txt`.
- **Vérification.** Exécuté et mesuré.
- **Conséquence v11.** Toute coupe plate de sklearn se publie avec son étendue sur des ordres d'égalité (au moins l'ordre des lignes permuté), sur la même machine que la méthode comparée. Pour la hiérarchie, juger la famille à plateaux contractés, qui n'en dépend pas (établi le 2 octobre).

### L10_BANC_SYNTHETIQUE-09 — Les reçus des lots A et C sont exacts et rejouables (info, acquis)

- **Fait.** `SHA256SUMS` des deux reçus conformes. `run_test.check_pins` passe avec les artefacts locaux (`build/v10-lotC-bc413ff56`, `build/v10-bench-c764e121a-static`) : binaires, six scripts, versions, manifeste du plan. Moyennes par méthode, écarts, gains et pertes recalculés : identiques à `DECISION.md` (quatre décimales). Graines : 2 048 dev, 960 `test`, 960 `test_v10b`, intersections vides ; les 23 CSV dev des reçus ne contiennent aucune graine de test. Concordance local contre G4 du reçu C retrouvée : tour 885 sur 885, témoin 295 sur 295, sklearn 411 différentes sur 708.
- **Preuve.** `recalcul_lots.txt`, `controles_divers.txt` (§ 2), `replay_lotC_8000.log`.
- **Vérification.** Exécuté.
- **Conséquence v11.** Le protocole (préenregistrement JSON, pins vérifiés avant toute scène, graines dérivées par SHA-256 de la spécification et du nom de l'espace, refus compté zéro, binaires statiques et Python portable pour G4) est à porter tel quel.

### L10_BANC_SYNTHETIQUE-10 — Niveaux A et B du 1er au 2 octobre : nombres confirmés, statut de développement (info, acquis)

- **Fait.** Les agrégats de tête de `RAPPORT_TOUR_PUIS_HIERARCHIE.md` sont retrouvés depuis `tables/groupes_A_B.csv.gz` (1 468 764 lignes) : par ordre et par source, nombre de cibles, exactes, parts au-dessus de 1/2 et de 4/5, IoU moyen, pour les groupes (14 400), les classes MAP (12 794), le bruit ignoré (1 800) et les sous-modes (3 144). Zéro écart. Les témoins natifs C++ égalent les lignes Python (`native_core` = `core`, `native_cover` = `cover`).
- **Preuve.** `recalcul_batterie_ab.txt`, `recalcul_batterie_ab_facteurs.txt` (par famille, niveau, bruit, n et nombre de groupes à K = 5 ; écart `cover` moins HDBSCAN par scène : +0,0053, +0,0238, +0,0495, +0,0767).
- **Vérification.** Exécuté. Le rapport porte en outre deux vérifications adverses du flux du développeur et une relecture documentaire de l'auditeur indépendant (`receipts/audit_independant_20261002/battery_review`).
- **Conséquence v11.** C'est la mesure la plus propre de la v10 : trois lectures (exacte, approchée, compatible), précision et rappel, deux références, inventaire strict. Elle reste sur graines dev, sans z, sans coupe plate : ni un test, ni une victoire.

### L10_BANC_SYNTHETIQUE-11 — Le rapport vérifié et le banc étendu sont hors dépôt ; le dépôt porte des nombres remplacés (majeur)

- **Fait.**
  - `RAPPORT_TOUR_PUIS_HIERARCHIE.md` (sha256 `1640fa47…`, 02 h 58), ses deux vérifications, le plan étendu, `map_ref.py`, le rapport de sélection, la mesure du vote et toutes leurs tables n'existent que sous `build/v10-tour-vers-points/`, dossier ignoré par Git, sur un disque plein à 96 %.
  - Le dépôt ne porte que `audits/REPONSE_CLAUDE_BATTERIE_TOUR_HIERARCHIES_20261002.md` (00 h 41), écrite avant la vérification. Exemple : « Classes MAP, 13 282 classes […] tour 0,888 / 0,771 / 0,863 » mêle le bloc iid ; le rapport vérifié donne 12 794 classes et 0,895 / 0,789 / 0,870 (retrouvé ici). Les lignes LiDAR y sont sans séparation des rôles de trames, que les vérificateurs ont jugée bloquante.
- **Preuve.** `recalcul_batterie_ab.txt` ; `audits/REPONSE_CLAUDE_BATTERIE_TOUR_HIERARCHIES_20261002.md` (§ 2) ; `batterie_ab/SHA256SUMS`.
- **Vérification.** Exécuté et lu.
- **Conséquence v11.** La v11 cite le rapport vérifié, avec son empreinte, et le copie dans ses reçus avec ses tables réduites. Un résultat qui ne vit que dans `build/` n'est pas un reçu.

### L10_BANC_SYNTHETIQUE-12 — Défaut de condensation de la tête C++ : réel, non corrigé au HEAD, sans effet mesurable à mcs = √n (mineur)

- **Fait.** La tête `src/head/head.cpp` ne teste pas mcs au départ d'un point attaché (`receipts/audit_continu_20260929/point_condensation_20260930`). `PASSATION.md:20` : « Impact sur les scores historiques à rejouer ». Le lot dev du 1er octobre le mesure (96 scènes, 1 536 condensations par entrée) : en entrée `cover`, le défaut n'agit jamais (0 cas ; la tête native égale la condensation par cohortes sur 3 072 lignes sur 3 072) ; en entrée `core`, il agit sur la structure dans 1 404 cas. Effet sur l'ARI_s en entrée `core`, sélection EOM : à mcs = √n, 1 ligne sur 384 change (0,056), moyenne −0,0002 ; à mcs = K, 94 lignes sur 384 (jusqu'à 0,195) ; à mcs = 10, 34 lignes (jusqu'à 0,32), moyenne −0,006. En sélection par feuilles, aucune ligne ne change.
- **Preuve.** `tt1_impact_dev.txt` ; `build/v10-comparaison-pr/results/dev_v10pr_20261001b/DEV_SUMMARY.json` (clé `tt1`).
- **Vérification.** Exécuté (recalcul depuis `results.csv`) ; non rejoué sur les scènes de test.
- **Conséquence v11.** Les lots A (entrée `core`, mcs = √n) et C (entrée `cover`) ne sont pas matériellement faussés. À petit mcs, le défaut compte : la v11 porte la condensation par cohortes de rang exact (`condense_pr.py` et sa référence), pas `head.cpp`.

### L10_BANC_SYNTHETIQUE-13 — Deux triangles : le seul exemple synthétique vérifié où l'objet exact décide (info)

- **Fait.** Six points, deux triangles équilatéraux de côté 2 000 dont deux sommets se font face à 1 980 (variante générique de la thèse § 6.1, r = 1 000).
  - Oracle exact Γ₃ : ABC | DEF de 1,1547 r à 1,9222 r. Γ₂ : ABC | CD | DEF sur la même plage (couvertures chevauchantes).
  - Tête v10 (binaire du lot C), K = 3, entrées `core` et `cover`, mcs 2 et 3, z 1 et 3 : ABC | DEF dans les huit cas.
  - sklearn, 720 ordres des lignes : `min_samples` 1 ou 2, tout bruit ; `min_samples` 3 et mcs 2, trois partitions selon l'ordre (tout bruit ; AB | CD ; CD | EF) ; jamais les triangles.
  - Témoin d'atteignabilité mutuelle, α 1 et 2, entrées `core` et `border`, K 2 et 3 : tout bruit dans les 32 configurations.
  - Limite : à K = 2 la tête v10 ne rend pas non plus les triangles (`cover` : AB | CD | EF à mcs 2, bruit à mcs 3). La tour les contient ; la projection sur les points ne les livre pas.
  - Dans la version dégénérée (CD = 2 000), sklearn rend ABC | DEF pour les 720 ordres : un effet de la binarisation du plateau, pas de la hiérarchie.
- **Preuve.** `deux_triangles.txt`, `deux_triangles.py`.
- **Vérification.** Exécuté.
- **Conséquence v11.** Graver la variante générique comme fixture permanente du banc (tour, hiérarchie de points, sklearn, témoin MR). Elle répond à « des exemples où la hiérarchie HGP réussit ». En tirer une famille à l'échelle (amas en quasi-contact par un sommet) pour savoir si l'avantage de l'objet se mesure au-delà de six points.

### L10_BANC_SYNTHETIQUE-14 — Écarts au protocole EVAL_v2 : déclarés, mais ils changent la portée (majeur)

- **Fait.** Les préenregistrements listent leurs écarts (`deviations_from_EVAL_v2`). Les plus lourds :
  - 8 familles v9 au lieu de 16, pas de témoins nuls, pas de suites publiques ;
  - une seule comparaison appariée par K au lieu du test d'intersection-union sur six adversaires ;
  - ARI_s seul, avec une garde AMI qui compte le bruit comme une classe ; ni IoU, ni F1 objet, ni précision ou rappel ;
  - `hierarchical` n'a qu'une vérité : la tour y rend 24 amas pour 8 groupes (ARI_s 0,46 à tous les K), sklearn EOM 20 (0,59) ; c'est une convention d'étiquettes, pas une mesure ;
  - les points des ponts de `bridge` sont du bruit vrai (−1), pas des lignes ignorées (−2) comme le voulait D16 ;
  - grilles de tête inégales (48 candidats pour la tour, 24 pour sklearn au lot C), z sans analogue chez sklearn, z choisi sur les seules scènes dev de 8 000 points.
- **Preuve.** Prereg A et C (clés `deviations_from_EVAL_v2`, `dev_basis`) ; `lotC_par_niveau_sans_shells.txt` ; `controles_divers.txt` (§ 4).
- **Vérification.** Lu et exécuté.
- **Conséquence v11.** Écrire le protocole que l'on exécutera vraiment, court, et le tenir. EVAL_v2 est un bon cahier des charges ; il n'a été réalisé qu'à un quart.

### L10_BANC_SYNTHETIQUE-15 — Le générateur n'est pas reproductible bit à bit d'une machine à l'autre (mineur)

- **Fait.** Sur 200 scènes dev (n ≤ 2 000) régénérées ici avec le `scenes.py` épinglé, 83 ont l'empreinte float64 enregistrée sur G4 ; 117 diffèrent (produits matriciels et QR de la rotation). Les sites quantifiés sont égaux partout où ils ont été comparés (rejeu : lignes de la tour identiques sur 8 scènes ; 59 scènes dans le reçu C ; 36 sur 36 selon les vérificateurs).
- **Preuve.** `digest_flottant.txt`.
- **Vérification.** Exécuté.
- **Conséquence v11.** Identité d'une scène = empreinte des sites quantifiés et des étiquettes, refus sinon. Générateur sans BLAS (paramètres dyadiques, élément par élément, comme `melange_iid.py`).

### L10_BANC_SYNTHETIQUE-16 — « sklearn tel quel » a une exception déclarée : epsilon (mineur)

- **Fait.** Avec scikit-learn 1.9.1 et numpy 2.5.3, `HDBSCAN(cluster_selection_method='leaf', cluster_selection_epsilon > 0)` lève `TypeError`. Les campagnes du 1er octobre passent, pour epsilon > 0, par `hdb_select.py` : la hiérarchie vient de sklearn, la sélection est un portage Python de `_tree.pyx`, contrôlé contre sklearn à epsilon nul et contre la bibliothèque `hdbscan` 0.8.44 sinon.
- **Preuve.** `controles_divers.txt` (§ 5) ; `build/v10-tour-vers-points/code/hdb_select.py:1-17`.
- **Vérification.** Exécuté et lu.
- **Conséquence v11.** Écart à la règle « jamais réimplémenté », acceptable s'il reste déclaré et testé. Préférer une paire de versions où l'appel officiel fonctionne, et épingler les deux.

### L10_BANC_SYNTHETIQUE-17 — Depuis le lot C, tout est développement ; l'exposant z n'est pas un paramètre stable (info)

- **Fait.** Aucun test scellé n'a suivi le 29 septembre. Les espaces `test_tour_points`, `test_etendue_20261001`, `test_batteries_iou`, `test_v10pr_d` sont réservés ; les rapports disent qu'aucune scène n'en a été engendrée (non vérifié ici). z a été : ẑ (lot A), puis 4 à 6 choisis à 8 000 points (lot C), puis 3 à mcs ≥ 40, 2 à mcs = 20, 1 à mcs ≤ 10 (étage sélection), avec z = 6 « toujours dominé » en mIoU. Les configurations du synthétique ne se transfèrent pas au LiDAR (mcs 40 et z 3 contre mcs K, z 1 et epsilon).
- **Preuve.** `selection/RAPPORT.md` § 5.2, § 6.1 ; `recalcul_selection.txt` ; `audits/audit_hierarchie_knn_20260929/AUDIT_HIERARCHIE_KNN_20260929.md` § 1.6.
- **Vérification.** Lu ; table par mcs exécutée.
- **Conséquence v11.** Suivre l'ordre de l'utilisateur : la tour, puis la hiérarchie, z en dernier. Publier toujours z = 1, et la grille de mcs.

## 4. (a) Ce qui est établi, par niveau

Niveau **A** : la vérité est-elle dans la tour (meilleur amas discret de FULL_K par cible) ? Niveau **B** : dans une hiérarchie de points (meilleur bloc) ? Niveau **C** : dans une coupe plate rendue sans la vérité ?

### 4.1 Degré de vérification

| Résultat | Statut | Qui l'a vérifié |
| --- | --- | --- |
| Lot A (C, K = 1, 2, 3) | préenregistré, une exécution, 960 scènes `test` | auditeur indépendant (empreintes du reçu) ; ce rapport (plan exact, recalcul) |
| Lot C (C, K = 1, 2, 3, 5, 8, 10 ; lot B ; famille « objet ») | préenregistré, amendé avant lecture, 960 scènes `test_v10b`, G4 | auditeur indépendant (30 720 couples, dates des commits) ; ce rapport (recalcul, rejeu de 8 scènes) |
| Batterie A puis B (1 728 scènes, 500 à 32 000 points, 2 à 20 groupes, bloc iid, MAP) | dev vérifié | deux vérificateurs adverses du flux du développeur ; relecture de l'auditeur indépendant ; ce rapport (agrégats de tête et tables par facteur) |
| Étage sélection (C, 768 scènes medium et hard) | dev, hors-sac | ce rapport (cinq moyennes de tête et la table par mcs) ; pas de second juge complet |
| Vote sur la tour condensée (C à z fixé) | dev vérifié | un vérificateur adverse ; lu seulement ici |
| Têtes multi-K, affectation par bassins, grands K | dev | un juge (multi-K) ; lus seulement ici |

### 4.2 Niveau A — la tour (dev vérifié ; recalculé ici)

1 728 scènes, 14 400 groupes par ordre.

| Lecture | K = 2 | K = 3 | K = 5 | K = 10 |
| --- | ---: | ---: | ---: | ---: |
| Groupes exactement égaux à un amas | 3 000 | 2 964 | 2 930 | 2 801 |
| Part au-dessus de 4/5 | 0,730 | 0,742 | 0,756 | 0,763 |
| IoU moyen | 0,823 | 0,830 | 0,839 | 0,847 |
| Classes MAP (12 794, n ≤ 8 000) : IoU moyen | 0,851 | 0,860 | 0,870 | 0,879 |

- **Par difficulté** (K = 5) : 0,975 / 0,941 / 0,771 / 0,668. **Par famille** : `shells` et `hierarchical` 0,988 ; `unbalanced` 0,549 ; `heteroscedastic` 0,609 (le MAP y est bas aussi : 0,731 et 0,809).
- **Par n**, sur 24 types de cellule appariés : 0,853 à 500 points, 0,813 à 32 000 ; −0,040 [−0,069 ; −0,016], hétérogène (15 types en baisse, 8 en hausse). La part exacte tombe de 0,278 à 0,060. K = 10 ne rapporte que +0,02 sur K = 2 à toute taille.
- **Par nombre de groupes** : pas de pente ; creux à 8 groupes (voir constat 06).
- **Bloc iid**, seul endroit où le MAP est la règle de Bayes : à 6 écarts-types, IoU 0,985 à 0,993 ; 8 composantes à 3 écarts-types, 0,44 à 0,51 (le MAP lui-même n'y vaut que 0,68 contre le tirage) ; deux composantes à 1,5 ou 3 : un seul amas.
- **Ce que cela ne dit pas** : A n'est pas une borne de B (un bloc peut être plus pur que tout amas) ; « non exact » veut dire « aucun bloc exporté exact trouvé » ; « 1 − IoU moyen » n'est pas une part de groupes absents.
- HDBSCAN n'a pas de niveau A : la comparaison commence au niveau B.

### 4.3 Niveau B — hiérarchies de points (dev vérifié ; recalculé ici)

IoU moyen du meilleur bloc, 14 400 groupes :

| Hiérarchie | K = 2 | K = 3 | K = 5 | K = 10 | Exacts, K = 5 |
| --- | ---: | ---: | ---: | ---: | ---: |
| tour (A) | 0,823 | 0,830 | 0,839 | 0,847 | 2 930 |
| `cover` | 0,817 | 0,822 | 0,827 | 0,830 | 2 935 |
| `cover1` | 0,820 | 0,824 | 0,828 | 0,831 | 2 958 |
| `core` | 0,770 | 0,757 | 0,747 | 0,739 | 2 745 |
| HDBSCAN (sklearn, plateaux contractés) | 0,811 | 0,797 | 0,776 | 0,751 | 2 894 |

- **`cover` moins HDBSCAN, apparié par scène** : +0,005, +0,024, +0,050, +0,077 (K = 2, 3, 5, 10) ; intervalles hors de zéro, même en tirant les cellules en grappes ; cellules gagnées, égales, perdues à K = 5 : 323, 29, 80 sur 432. Avec l'ordre d'égalité le plus favorable à HDBSCAN : +0,049 [+0,037 ; +0,061] à K = 5.
- **À K = 2 les deux hiérarchies se valent** : +0,005, et +0,002 [−0,001 ; +0,005] en lecture compatible.
- **Par n** (K = 5) : +0,057 à 500 points, +0,036 à 32 000. **Par difficulté** : +0,018 (`easy`) à +0,070 (`hard`). **Par bruit** : +0,053 / +0,047 / +0,040 / +0,058. **Par groupes** : +0,037 (2) à +0,060 (5).
- **Par famille** (K = 5) : `unbalanced` +0,107, `spherical` +0,076, `bridge` +0,074, `anisotropic` +0,068, `filaments` +0,053, `heteroscedastic` +0,028 ; **`hierarchical` −0,003 et `shells` −0,006**.
- **Où HDBSCAN est devant** : présence exacte sous 20 % de bruit (420 groupes contre 323 pour `cover`, 465 pour `core`) ; cause mesurée : un point de bruit entré tôt dans le bloc (à bruit ignoré, `cover` 932, HDBSCAN 798, `core` 708).
- **Classes MAP**, K = 5 : tour 0,870, `cover` 0,855, HDBSCAN 0,788, `core` 0,755. **Bloc iid**, K = 5 : 0,665 / 0,645 / 0,511.
- **Non établi** : que cet avantage soit celui de l'objet exact (constat 03).

### 4.4 Niveau C — coupe plate

**Préenregistré (ARI_s, mcs = √n, 8 groupes, 8 000 à 32 000 points)** :

| Lot | K | Écart | Lecture vérifiée ici |
| --- | ---: | ---: | --- |
| A (entrée `core`, z = ẑ, rempli) | 1 | +0,053 | effet de tête seul (hiérarchies identiques) ; 767 égalités sur 960 |
| A | 2 | +0,092 | +0,024 sans `shells` |
| A | 3 | +0,049 | −0,018 sans `shells` ; quatre familles en perte significative, dont `bridge` −0,135 et `spherical` −0,123 |
| C (entrée `cover`, z dev, rempli) | 1, 2 | +0,075, +0,073 | adversaire unique (liaison simple, EOM) ; gains sur `spherical` +0,30, `bridge`, `anisotropic` ; perte sur `hierarchical` −0,12 |
| C | 3 | +0,091 | +0,031 sans `shells` |
| C | 5 | +0,064 | +0,013 sans `shells` |
| C | 8, 10 | +0,040, +0,031 | nul sans `shells` ; majorité de scènes perdues |
| C, sans remplissage (descriptif, sklearn EOM α = 1) | 1 à 10 | +0,034 à +0,105 | robuste sans `shells` (+0,039 à +0,121) ; médiane +0,00 à +0,05 ; perte sur `hierarchical` −0,14 à −0,16 |
| C, « objet » (tour contre MR₂-bord) | 2 à 10 | −0,001 à +0,010 | parité |
| C, lot B (entrée `core` à K = 5, 8, 10) | 5, 8, 10 | +0,011 (ns), −0,025, −0,034 | sklearn bat cette tête à K = 8 et 10 |

- **Par n** : l'écart rempli croît avec n à K ≥ 3 (+0,063 à +0,120 à K = 3), par `shells` seule ; sans elle, +0,014 à +0,049 à K = 3 et plat à K ≥ 8.
- **Par difficulté**, sans `shells`, rempli : K = 3 : +0,047 / +0,052 / +0,027 / −0,002 ; K = 10 : −0,006 / 0,000 / +0,013 / −0,021.
- **Par bruit** : écart plus faible à 10 % de bruit qu'à 0 (K = 5 : +0,051 contre +0,076).

**Dev (mIoU, 768 scènes medium et hard, 2 000 et 8 000 points, 3, 8 et 20 groupes)** : meilleure configuration de chaque côté sur des grilles de 120, +0,045 à +0,057 par K, intervalle hors-sac hors de zéro ; médiane de l'écart nulle ; `cover` avec mcs = 40 et z = 3 contre sklearn mcs = √n ou 40. Le témoin `mreach` (arbre de sklearn, tête du produit) explique +0,039 des +0,062. Dépendance à mcs : constat 04. Vote sur la tour condensée, taille de cœur (`VC[coeur,W1]`, z = 1, 2 000 points, K = 5) : 0,690 / 0,721 / 0,711 / 0,703 à mcs = K / 10 / 20 / √n contre 0,580 / 0,642 / 0,648 / 0,658 pour HDBSCAN ; cette règle échoue les deux triangles (20 jugements sur 125) ; aucune règle ne passe partout.

**En une phrase.** Solide : la mesure (lots A et C exacts), la présence dans la tour (A), la supériorité moyenne de la hiérarchie `cover` sur celle de sklearn à K ≥ 3 (B, dev). Non solide : toute lecture « la tour bat HDBSCAN » au niveau C hors de mcs ≥ 40, hors de l'appariement K ≤ 10, et à K ≥ 5 hors de `shells` ; toute attribution à l'objet exact.

## 5. (b) Revendiqué, puis corrigé ou infirmé

| Sujet | Énoncé d'origine | Ce qu'il en reste | Source |
| --- | --- | --- | --- |
| Sélection par DBCV | « DBCV choisit mieux que la meilleure configuration fixe » (28 sept., 0,752 contre 0,746) | Infirmé le jour même sur une grille large : 0,678 (tour, K ≤ 10), 0,644 (sklearn, α ∈ {1, 2}) ; DBCV choisit des configurations à 0,18–0,30 sur `shells` | `receipts/bench_dev_20260928`, `bench_dev_selection_20260928` |
| Exposant ẑ | Tête du lot A : λ = r^(−ẑ), « meilleur que t^(−p) » | Abandonné le 29 : effondre `shells` à K = 10 ; z = 1 « pire choix à tout K » ; puis z = 4 à 6 (lot C, 8 000 points, ARI_s), puis z = 3 à mcs ≥ 40 et z = 1 à mcs ≤ 10 (sélection, mIoU). z dépend de n, de mcs, de K et de la métrique | audit K-NN § 1.6 ; `selection/RAPPORT.md` § 5.2 |
| Remplissage | Remplissage complet (28), puis borné b(ρ) : « +0,03 à +0,04 aux deux, écart inchangé » | Le remplissage fait l'essentiel du score de sklearn en feuilles (+0,14 à +0,19) et peu pour la tour ; son k vaut max(K, 5) contre EVAL_v2 (écart déclaré) ; la « complétion » corrigée ne gagne presque rien (+0,003), l'ancien gain venait d'une limite globale ; avec et sans remplissage, sklearn n'a pas la même tête, et la conclusion à K ≥ 8 change (constats 01 et 02) | `bench_dev_fill_20260928`, `bench_dev_z_samehead_20260929` § 4, `DIAGNOSTIC` § 6 |
| Plafond de Bayes, bassins de Morse | « La meilleure coupe atteint le plafond de Bayes » ; « Bayes − Morse = prix du modèle, hors de portée » | Diagnostics supervisés, pas des plafonds (erratum) ; MAP v1 remplacé par `map_ref.py` v2 ; la tour dépasse la classe MAP pour 459 groupes ; la phrase reste dans `PASSATION.md:113` (constat DC3 de l'audit géant) | `receipts/ERRATA.md:12` ; batterie § 1.5 |
| Défaut de condensation | « Impact sur les scores historiques à rejouer » | Mesuré : nul en entrée `cover`, au plus 0,001 à mcs = √n en entrée `core` (dev) ; tête C++ non corrigée | constat 12 |
| Entrée `cover` | « x entre à α_K(x) : amas discrets du théorème 2 » | Faux de ce que lit la tête : le point sort à la fusion qui absorbe sa naissance ; `cover` et `core` visent deux objets | `receipts/ERRATA.md:8` ; DC1 |
| Lot A | « La tour bat HDBSCAN à K = 1, 2, 3 » | Vrai au sens préenregistré ; à K = 1 effet de tête ; la tête a été abandonnée ; à K = 3, perd sans `shells` | constat 01 |
| Lot B | Promis par le lot A | Perd à K = 8 et 10 (−0,025, −0,034), comme prédit | `test_cover_C_20260929/DECISION.md:14-16` |
| Lot C | « À tous les K de 1 à 10 » | Six K testés (erratum) ; réserves des constats 01 à 05 | `receipts/ERRATA.md:17` |
| Grands K | « Un K plus grand n'aide pas le clustering » | Seul le témoin MR₂-bord a été mesuré au-delà de 10 ; hors `shells`, +0,0002 à K = 16 (erratum) ; la batterie montre au contraire que le meilleur ordre glisse vers K = 10 quand n monte | `receipts/ERRATA.md:16` ; batterie § 1.4 |
| Têtes multi-K | Voie d'un « avantage structurel » | Aucune ne gagne 0,02 ; `mixq` +0,005 reproductible, non propre à la tour | `audits/tete_multik_20260929/RAPPORT_JUGE.md` |
| « La date fait la perte, pas le propriétaire » | Diagnostic du 1er octobre | Égalité de moyennes seulement ; les blocs diffèrent sur 4 à 22 % des groupes | `DIAGNOSTIC/RAPPORT_DIAGNOSTIC.md` (correctifs) |
| « Un rayon commun ne suffit pas » (synthétique) | Analyse de la batterie | Non établi : artefact de la règle du premier rang ; le meilleur rayon commun coûte environ 0,03 | `VERIFICATION_mesures.md` constat 2 |
| LiDAR, `cover` moins HDBSCAN | +0,057 en lecture compatible | +0,014 sur les 728 instances ; +0,010 sur les trames choisies sur les échecs de HDBSCAN, −0,001 sur les témoins | `VERIFICATION_mesures.md` constats 1 et 10 |

## 6. (c) Validité méthodologique

| Point | État | Détail |
| --- | --- | --- |
| Appariement K = `min_samples` | juste pour le niveau d'entrée, bridant pour l'adversaire | constat 05 |
| Convention de K de sklearn | vérifiée dans la source installée | `hdbscan.py:355`, `:597-601` ; la bibliothèque `hdbscan` de McInnes ne compte pas le point (décalage d'un) |
| alpha | α n'agit que sous `kd_tree` et `ball_tree` (`hdbscan.py:254` divise toute la matrice dans la voie `brute`) ; le banc épingle `kd_tree` et refuse α ≠ 1 ailleurs (`methods.py:75-84`) | `hdb_lib` appelle `HDBSCAN()` avec `algorithm="auto"` (déclaré) |
| Réglage de HDBSCAN | symétrique en règle, pas en moyens | mcs fixé à √n ; `min_samples` fixé à K ; epsilon exclu des lots ; 24 candidats contre 48 ; une seule configuration pour huit familles (constat 02) |
| Fuite dev / test | aucune fuite de graines | constat 09. Restent : mêmes familles et niveaux en dev et en test ; dev réutilisé pour des dizaines de choix ; niveaux calibrés sur HDBSCAN (constat 06) |
| Amendement du lot C | déclaré avant lecture, commits datés | l'absence de lecture de l'exécution locale interrompue est une déclaration, non une preuve |
| sklearn et la machine | réel, petit en moyenne, parfois 0,04 sur une scène | constat 08 |
| ARI_s | correct | fixture 4/7 retrouvée ; bruit vrai et prédit en singletons ; ne mesure ni les objets ni la précision et le rappel |
| IoU, F1 objet, précision, rappel | introduits le 1er octobre (`metrics_iou.py`, juge brut à 300 tirages) | absents des lots A et C |
| Trois lectures | exacte, approchée, compatible : bien séparées dans le rapport vérifié | les moyennes de scènes ne sont pas des parts de groupes (réserve appliquée) |
| Référence MAP | solide | trois portées jamais mêlées ; 8 256 000 sites redérivés par un vérificateur, 0 désaccord ; absente à 16 000 et 32 000 points ; ni concurrent ni plafond d'IoU |
| Statistique | test correct, règle trop permissive | retournement de signe stratifié, Holm, bootstrap de McCarthy–Snowden : conformes ; mais la règle « bat » ne regarde que la moyenne (constat 01) |

## 7. (d) Défauts d'outillage relevés par les auditeurs, et leur état au HEAD

| Défaut | Relevé par | État au HEAD `afb081774` |
| --- | --- | --- |
| `decide.py` : lot incomplet, ARI > 1, schéma non validé, doublons | auditeur indépendant (E1), audit géant (BN1) | **ouvert** ; reproduit ici ; corrigé dans la copie R2 `4d1265f0…` seulement |
| `merge_sessions.py` accepte une unité hors plan ; `run_test.py --resume` garde une scène hors plan | E1, reçu `bench_corrected` | ouvert au HEAD |
| `methods.hdbscan_labels` : alpha sans effet en `brute` | audit K-NN (IMP-04) | corrigé (`methods.py:80-81`) |
| Échecs dev omis au lieu de compter zéro | audit K-NN (règle D8) | corrigé dans `alloc_dev.py` (commit `24ac5fc51`) ; **ouvert** dans `run_campaign.py:137-141`, figé par les pins des lots : il écrit « ECHEC » et passe à la scène suivante |
| `scale_run.py` : moteur orphelin au délai | audit géant (BN2) | ouvert au HEAD (copie corrigée) |
| Tête : `--allow-single`, z non fini ou négatif accepté, coût quadratique sur un peigne | auditeur indépendant (H2, H3, H4) | hors lots A et C ; à ne pas porter |
| Condensation des départs de points | audit continu | ouvert (constat 12) |
| Collecteur des campagnes du 1er octobre : 33 données fausses acceptées sur 35 ; huit unités vides comptées faites | agent de diagnostic | réparé hors dépôt (`reparations/collecte_stricte.py`, 35 mutants) |
| `bayes_ref.py` : garde sur les seules tailles ; `bridge` mal modélisé | audit continu | remplacé hors dépôt par `map_ref.py` v2 |
| MAP v1 : cinq contournements de la certification ; composante iid non tirée écartée | audit continu | corrigés hors dépôt, fixture gravée, mutant tué |
| `completion_oracle`, `table_completion_locale` : limite globale, empreinte ignorée, clés répétées | audit continu | corrigés hors dépôt |
| Chemin rapide du vote : cumul flottant global, rang lu dans la table flottante | audit continu | réparé dans une copie ; impact nul sur les unités contrôlées |
| Référence modale : bassins d'EM et non de gradient ; convergence vers une selle ; coût quadratique | audit continu | corrigée en partie ; facultative |
| Empreinte flottante des scènes différente d'une machine à l'autre | vérificateurs | connu (constat 15) |
| HDBSCAN non déterministe d'une machine à l'autre | développeur, vérificateurs | connu (constat 08) |
| Sessions G4 : statut `failed_remote` dû à l'absence de pip ; archive `tvpab2` jamais rapatriée ; lien dur de `pack_g4.py` | rapports de campagne | hors lentille ; à traiter par l'outillage de session |

## 8. (e) Le banc que la v11 doit avoir

Il répond aux exigences de l'utilisateur du 1er octobre (`build/workflows/EXIGENCES_TESTS_UTILISATEUR.md`) : beaucoup de scènes, trois facteurs variés, vérité et MAP, IoU d'abord avec précision et rappel, HDBSCAN réglé loyalement. Les trois quarts existent déjà hors dépôt ; il manque les niveaux géométriques, les témoins et la règle de décision.

### 8.1 Plan

| Axe | Valeurs | Remarque |
| --- | --- | --- |
| Tailles n | 500, 1 000, 2 000, 4 000, 8 000, 16 000, 32 000 | plan fractionnaire orthogonal de `plan_etendu.py` (96, 96, 96, 48, 48, 24, 24 cellules) : mêmes combinaisons à chaque taille, donc courbes en n appariées. Décision sur 8 000, 16 000 et 32 000 ; les petites tailles servent d'oracle et de tendance |
| Groupes g | 2, 3, 5, 8, 12, 20 | publier le nombre de voisins à la distance minimale (constat 06) |
| Difficulté | quatre niveaux **géométriques** | familles à recouvrement : niveau de Bayes visé (mIoU du MAP contre le tirage, par exemple 0,99 / 0,95 / 0,85 / 0,70) ; supports disjoints : rapport d'écart ; calibrés par (famille, g) sur un espace `calib`, sans aucune méthode de clustering |
| Bruit | 0 ; 0,05 ; 0,1 ; 0,2 | uniforme dans la boîte élargie ; lecture « bruit ignoré » en plus ; ponts de `bridge` en classe à part |
| Familles | les huit de `scenes.py` (continuité avec la v10, sites quantifiés identiques) ; le bloc iid à neuf mélanges (seul endroit où le MAP est la règle de Bayes) ; `hierarchical` à deux vérités ; une famille **contact** tirée des deux triangles ; trois témoins nuls (cube uniforme, sphère creuse, gaussienne seule) | les familles `atom`, `chainlink`, `moons`, `spirals`, `helices`, `mixdim` d'EVAL_v2 § 3.1 sont un second lot, après le premier test |
| Ordres K | 2, 3, 5, 10, et K = 1 comme contrôle d'outil | `min_samples` = K, point compté ; au-delà de 10, mesure à part si le moteur le permet (la batterie montre que le meilleur ordre glisse vers K = 10 quand n monte) |
| Coupe plate | mcs ∈ {K, 10, 20, 40, √n} ; EOM et feuilles ; z ∈ {1, 2, 3} lus séparément ; epsilon et remplissage en options symétriques | jamais un seul mcs ; jamais un « meilleur z » sans la table complète |
| Réplicats | 4 par cellule en dev ; 5 à 10 en test | unité statistique : la scène |

### 8.2 Sources et adversaires, aux trois niveaux

| Rôle | Définition | Niveaux |
| --- | --- | --- |
| Tour | amas discrets de FULL_K | A |
| Hiérarchies de la v11 | la règle retenue, plus `cover` et `core` comme repères historiques | B, C |
| HDBSCAN, hiérarchie | `_single_linkage_tree_` de sklearn, plateaux contractés (famille indépendante de l'ordre) ; enveloppe sur les ordres d'égalité en diagnostic | B |
| HDBSCAN par défaut | `HDBSCAN()` | C |
| HDBSCAN apparié | `min_samples` = K, même mcs, même sélection | C |
| HDBSCAN libre | réglé sur dev par la même règle et le même budget que la tour : `min_samples` ∈ {1 … 10, 12, 16, 20}, grille de mcs, EOM ou feuilles, α ∈ {1, 2}, epsilon | C |
| Témoin MR-bord | hiérarchie d'atteignabilité mutuelle munie de la règle des points-bord et de la même tête ; de préférence construite sur l'arbre de sklearn (`sk_mr_tree`, `mr_border` de `build/v10-persist/audit_hier/auditeur_objet/objet_lib.py`), pour ne pas réimplémenter HDBSCAN | B, C : **attribution**, pas un adversaire |

sklearn est appelé tel quel, sur la même machine que la méthode comparée, avec au moins trois ordres de lignes ; la décision prend l'ordre le plus favorable à HDBSCAN et publie l'étendue.

### 8.3 Références et métriques

- **Références** : étiquettes du générateur ; MAP à trois portées (`exact_iid`, `exact_marginal`, `plug_in`), jamais mêlées ; sous-modes de `hierarchical` ; MAP présent aussi à 16 000 et 32 000 points (384 unités manquaient).
- **Niveaux A et B** : par cible, IoU du meilleur amas ou bloc, avec sa précision et son rappel ; trois lectures séparées : exacte (comptes), approchée (parts au-dessus de 1/2, 4/5, 9/10 ; IoU moyen), compatible (antichaînes `f1`, `m80`, `iou` ; meilleur rayon commun au sens du mIoU, pas le premier rang).
- **Niveau C** : mIoU par objet (appariement un à un optimal, objet non apparié = 0) ; précision, rappel et F1 objet (IoU > 1/2) ; précision et rappel des points ; couverture ; nombre d'amas ; sur- et sous-segmentation. ARI_s gardé comme colonne de continuité avec les reçus v10.
- **Témoins nuls** : part de fausse structure, sans prime à l'abstention (EVAL_v2 D21).
- **Agrégats** : moyenne par cellule, bootstrap apparié par scène et par grappes de cellules, gains, égalités et pertes, médiane, écart sans chaque famille, tables par n, g, difficulté, bruit, famille.

### 8.4 Protocole

1. **Portes du harnais avant toute campagne** : fixtures de métriques contre un juge brut, disjonction des espaces de graines, complétude et schéma à la décision, symétrie des grilles et du remplissage, mutants tués (liste d'EVAL_v2 § 9.5 réduite à ce qui est exécuté).
2. **Fixtures gravées** : deux triangles génériques et dégénérés (tour, hiérarchies, sklearn sur les 720 ordres, témoin MR) ; les deux témoins de contamination (filament 77 sur 78, coquille 91 sur 92) ; la fixture MAP iid (graine 18, site 62) ; K = 1 : tour = liaison simple = sklearn `min_samples` 1.
3. **Dev** dans l'ordre de l'utilisateur : A, puis B avec le témoin MR-bord, puis C à z fixés. Hors-sac ou validation croisée dès qu'une configuration est choisie.
4. **Préenregistrement** : JSON, pins (binaires statiques, scripts, versions, manifeste du plan, manifeste MAP), espace de graines neuf, une seule exécution, décision calculée.
5. **Règle « meilleur que HDBSCAN »**, pour chacun des trois adversaires : écart moyen ≥ marge avec borne basse > 0 ; médiane ≥ 0 ; écart > 0 sans chacune des familles ; écart > 0 à chaque taille de décision ; aucune perte significative à aucun mcs de la grille déclarée ; refus ≤ 1 %. « Domine » exige en plus aucune famille en perte significative. La phrase d'attribution (contre le témoin MR-bord) est obligatoire.
6. **Reçus dans le dépôt** : rapport vérifié, tables réduites, empreintes ; rien d'essentiel ne reste sous `build/`.

### 8.5 Scripts à porter explicitement (provenance à épingler)

| Source | Empreinte (début) | Rôle | Réserve |
| --- | --- | --- | --- |
| `morsehgp3D_v10/bench/synthetic/scenes.py` | `61ea9abc…` | huit familles, `quantize18` | remplacer `SEPARATION` et `CALIBRATION` par des niveaux géométriques ; retirer BLAS du tirage |
| `run_campaign.seed_of`, `run_test.py` (`check_pins`, refus compté zéro, reprise) | `7e59000d…`, `87ba7917…` | graines et exécution scellée | reprise : n'accepter que des scènes du plan |
| `build/v10-integration-r2/src/morsehgp3D_v10/bench/synthetic/decide.py` et `tests/regression/test_decide_completeness.py` (commit R2 `865f5e6`) | `4d1265f0…`, `b2a4e26f…` | décision avec complétude et schéma | ajouter la règle de robustesse du § 8.4 |
| `morsehgp3D_v10/bench/synthetic/metrics.py` | `76d6c92d…` | ARI_s | colonne de continuité |
| `build/v10-tour-vers-points/code/metrics_iou.py` et son test | `757f9d9d…` | IoU, F1 objet, précision, rappel, juge brut | — |
| `build/v10-tour-vers-points/banc2/plan_map/plan_etendu.py` | `b76a64a8…` | plan fractionnaire orthogonal, disjonction des graines | — |
| `…/plan_map/map_ref.py`, `map_intervalles.py`, `melange_iid.py`, `etendu_tvp.py` et leurs tests | `0d9925be…`, `5e0d9c95…`, `70048061…`, `33399866…` | MAP à trois portées, bloc iid exact, certificat de rejeu | réserves 1 et 2 du vérificateur (empreinte flottante, module chargé) |
| `…/plan_map/rapport_etendu.py`, `batterie_ab/code_rapport/` | `671c0f04…` | trois lectures, rapport écrit depuis les tables, sceaux | — |
| `build/v10-tour-vers-points/code/oracle_ab.py` | `551ce25a…` | oracles d'antichaîne et de rayon | prendre le meilleur rayon, pas le premier rang |
| `…/code/condense_pr.py` et sa référence | `c7d4315a…` | condensation exacte par cohortes | remplace `src/head/head.cpp` |
| `…/code/hdb_select.py` et son test | `dc5e14d5…` | epsilon de sklearn sous numpy 2.5 | seulement si l'appel officiel échoue encore |
| `…/DIAGNOSTIC/reparations/collecte_stricte.py`, `juge_niveaux.py`, `transport_fraction.py` | `3a45d39d…`, `a022c915…`, `eb14fbc9…` | collecte à statuts, 35 mutants | — |
| `build/v10-persist/audit_hier/auditeur_objet/objet_lib.py` ; `morsehgp3D_v10/tests/head/mreach*.cpp` (`mhgp10_mreach_cluster`, binaire `83c680b3…`) | — | témoin MR-bord : sur l'arbre de sklearn (Python), ou exact en entiers (C++) | témoin de test, jamais un produit ni un adversaire ; la version sklearn respecte « HDBSCAN jamais réimplémenté » |
| `morsehgp3D_v10/bench/g4/lot_runner.py`, `pyenv_run.py` ; `merge_sessions.py` dans sa version R2 (`bd61d245…`) | — | exécution G4 par sessions, Python portable | la version R2 exige l'appartenance au plan ; celle du HEAD non |

## 9. Ce qui est solide et mérite un port explicite

1. Le **protocole scellé** des lots A et C : préenregistrement JSON, pins vérifiés avant la première scène, espaces de graines dérivés par SHA-256, refus compté zéro, amendement daté avant lecture, binaires statiques et Python portable sur G4. Vérifié ici de bout en bout.
2. La **lecture en trois niveaux** (tour, hiérarchie, coupe) et en **trois lectures** (exacte, approchée, compatible), avec précision et rappel, bruit ignoré, et l'ordre « la tour, la hiérarchie, z en dernier ».
3. La **référence MAP v2** et le **bloc iid** exact.
4. Le **plan fractionnaire orthogonal** (432 cellules, 1 728 scènes, 500 à 32 000 points, 2 à 20 groupes), reconstruit à l'octet.
5. La **famille « objet »** (même entrée, même tête sur l'atteignabilité mutuelle) : c'est elle qui a empêché une fausse attribution.
6. La **vérification adverse à deux lentilles** (mesures ; références et équité), avec recalcul complet depuis les lignes : elle a trouvé trois lectures fausses avant publication.
7. La **métrique ARI_s** et les **métriques IoU** avec leur juge brut ; le **collecteur strict**.
8. Les **faits mesurés** à réutiliser comme attentes : K = 1 tour = liaison simple ; hiérarchie `cover` au-dessus de celle de sklearn à K ≥ 3 ; `core` au-dessous ; sklearn dépend de l'ordre des égalités ; la contamination de `cover` par un point de bruit ; la fragmentation de `cover` à mcs = K.
9. La **fixture des deux triangles**, en version générique.

## 10. Ce qu'il ne faut pas refaire

1. Calibrer la difficulté sur l'échec d'un adversaire.
2. Opposer la tour à **une seule** configuration de HDBSCAN choisie par une moyenne sur toutes les familles, puis écrire « bat HDBSCAN ».
3. Décider sur la seule moyenne : exiger médiane, comptes de scènes et écart sans chaque famille.
4. Fixer mcs à √n et z par une grille d'un seul côté ; régler z sur une seule taille ; chercher un z avant d'avoir mesuré A et B.
5. Brider `min_samples` de l'adversaire à K sans publier l'adversaire libre.
6. Attribuer un écart à « l'objet exact » sans le témoin MR-bord.
7. Publier une coupe plate de sklearn sans son étendue sur les ordres d'égalité ; comparer des lignes sklearn entre machines.
8. Laisser l'outil de décision sans contrôle de complétude ni de schéma, et le harnais sans portes.
9. Laisser le rapport vérifié et les tables hors du dépôt ; répondre aux auditeurs avec des nombres antérieurs à la vérification.
10. Utiliser comme « plafond » un MAP à paramètres estimés sur les étiquettes, ou des bassins calculés par montée discrète ; lire « 1 − IoU moyen » comme une part de groupes absents ; lire A comme une borne de B.
11. Compter une vérité unique sur `hierarchical` ; compter les ponts comme du bruit vrai sans le dire.
12. DBCV comme sélecteur sur une grille large ; ẑ global comme échelle d'EOM ; les têtes multi-K comme voie de gain (aucune n'a gagné 0,02).
13. Écrire un protocole de 1 250 lignes (EVAL_v2) et n'en exécuter qu'un quart sous forme d'écarts déclarés.
14. Identifier une scène par l'empreinte de ses points flottants.

## 11. Questions ouvertes

1. **Attribution au niveau B.** La hiérarchie MR-bord (α = 1 et 2) égale-t-elle `cover` en meilleur bloc et en antichaîne, comme elle l'égale en coupe plate ? Mesure peu coûteuse sur le plan étendu ; elle décide de ce que la v11 peut dire de l'objet exact.
2. **Où l'objet exact se voit-il au-delà de six points ?** Une famille « contact » (amas réguliers qui se touchent par un sommet ou une arête, à l'échelle) sépare-t-elle la tour de l'atteignabilité mutuelle de façon mesurable à n ≥ 500 ?
3. **Petit mcs.** Quelle règle d'existence des amas tient à la fois mcs = K (taille de cœur : +0,11 sur HDBSCAN, mais échec des deux triangles) et les fixtures (taille couverte : fragmentation) ? Aucune règle v10 ne passe partout.
4. **Adversaire libre.** L'avantage de +0,015 (dev, 8 000 points) contre HDBSCAN à `min_samples` = 12 survit-il à un test scellé, à 16 000 et 32 000 points, en mIoU ?
5. **Dépendance à n.** La tour perd 0,04 d'IoU de 500 à 32 000 points à K fixe et le meilleur ordre glisse vers K = 10 : faut-il K croissant avec n (K = 20, 40) ? Bloqué en v10 par l'ordre maximal du moteur.
6. **Bruit.** Quel critère d'entrée sépare un point de bord d'un point de bruit voisin (Q15 du développeur), sans perdre le rappel que `core` perd ?
7. **Référence HDBSCAN.** Départage canonique des égalités, ou étendue publiée (Q14) ? Ce rapport recommande l'étendue, avec l'ordre le plus favorable à HDBSCAN pour décider.
8. **`hierarchical`.** Quelle vérité compte : le groupe ou le sous-mode ? La v11 doit le déclarer avant toute campagne.
9. **Lot A et lot B sous condensation exacte.** L'effet mesuré sur dev est négligeable à mcs = √n ; il n'a pas été rejoué sur les scènes de test.
10. **Espaces de test réservés** (`test_tour_points`, `test_etendue_20261001`, `test_batteries_iou`, `test_v10pr_d`) : déclarés jamais engendrés ; non vérifié ici. La v11 doit prendre des espaces neufs.

## 12. Recommandations pour la v11, dans l'ordre

1. **Ne rien revendiquer de la v10 contre HDBSCAN.** Formulation exacte de l'acquis : « sur le banc à huit familles, à `min_samples` = K ≤ 10 et mcs = √n, la tête v10-b a un ARI_s moyen supérieur à celui d'une configuration sklearn réglée sur dev ; à même entrée et même tête, la hiérarchie de HDBSCAN fait jeu égal ; à K ≥ 8 l'écart tient à une famille ».
2. **Graver d'abord les fixtures** du § 8.4 : elles coûtent une heure et fixent ce que « réussir » veut dire (deux triangles : ABC | DEF attendu de la tour à K = 3 et de la hiérarchie de points ; à K = 2, cible à définir).
3. **Porter le harnais avec ses portes** (§ 8.5), `decide.py` depuis R2, avant la première campagne.
4. **Refaire les niveaux** : géométriques, par famille et par nombre de groupes.
5. **Rejouer A et B sur le plan étendu avec le témoin MR-bord et le MAP à toutes les tailles.** C'est la première campagne utile : elle dit si la hiérarchie de points de la v11 vaut mieux que « HDBSCAN plus la règle des points-bord ».
6. **Ne passer au niveau C qu'ensuite**, sur la grille de mcs complète, à z = 1, 2, 3 lus séparément, contre les trois adversaires.
7. **Un seul test scellé**, quand une règle passe à la fois les fixtures et le petit mcs sur dev ; règle de décision robuste écrite d'avance.
8. **Tout résultat cité dans le dépôt y vit** : rapport vérifié, tables réduites, empreintes.

## 13. Limites de cet audit

- Les lots A et C ne sont rejoués que sur 8 scènes de 8 000 points ; le reste est recalculé depuis les lignes des reçus.
- La batterie A/B est recoupée sur ses agrégats de tête et ses tables par facteur, l'étage sélection sur cinq moyennes de tête et la table par mcs, tous depuis les lignes fusionnées ; leurs calculs natifs sur G4 ne sont pas rejoués.
- Le rapport de la mesure du vote, les règles ER0h et « maturité », les têtes multi-K et l'étude des grands K sont lus, pas recalculés.
- Les analyses « sans `shells` », « adversaire libre » et « meilleure tête par famille » sont a posteriori : elles mesurent la robustesse d'un verdict, elles ne le remplacent pas.
- Machine chargée : aucun temps n'a de valeur.

## 14. Preuves

Dossier `build/v11-persist/audit_v10/preuves_l10_banc_synthetique/` :

| Fichier | Contenu |
| --- | --- |
| `recalcul_lots.py`, `recalcul_lots.txt` | lots A et C : moyennes, écarts, médianes, gains et pertes, sans `shells`, par famille, par n, par niveau, par bruit |
| `lotC_shells_par_n.txt`, `lotC_sans_shells_par_n.txt`, `lotC_par_niveau_sans_shells.txt` | détail de `shells`, test de signe sans `shells`, `hierarchical` |
| `replay_lotC.py`, `replay_lotC_8000.log`, `replay_sklearn_only.py`, `replay_sklearn_only.log` | rejeu de 8 scènes du lot C avec les pins |
| `decide_defauts_head/` | `decide.py` du HEAD sur trois lots fautifs ; copie R2 sur les mêmes |
| `sklearn_egalites.py`, `sklearn_egalites.txt` | égalités et ordre des lignes |
| `controles_divers.py`, `controles_divers.txt` | calibration, graines, `hdb_ms1` = `hdb_ms2`, métriques, centres, epsilon |
| `tt1_impact_dev.txt` | défaut de condensation et dépendance à mcs (lot dev du 1er octobre) |
| `dev_hdbscan_libre.txt` | HDBSCAN réglé librement contre la tête v10-b (dev) |
| `recalcul_batterie_ab.py`, `recalcul_batterie_ab.txt`, `recalcul_batterie_ab_facteurs.py`, `recalcul_batterie_ab_facteurs.txt` | batterie A/B : agrégats de tête et tables par facteur |
| `recalcul_selection.py`, `recalcul_selection.txt` | étage sélection : moyennes de tête, table par mcs |
| `digest_flottant.txt` | empreintes flottantes ici contre G4 |
| `deux_triangles.py`, `deux_triangles.txt` | oracle exact, sklearn (720 ordres), tête v10, témoin MR |
