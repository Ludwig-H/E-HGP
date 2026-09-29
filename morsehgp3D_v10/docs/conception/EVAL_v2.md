# EVAL_v2 — Protocole d'évaluation de morsehgp3D_v10

Qualité de clustering contre HDBSCAN, et performance LiDAR. Révision de `EVAL_v1.md` après critique adverse.

```text
phase=conception_v10_hors_registre
sous_systeme=protocole_evaluation
backend=cpu_reference (qualite) ; cpu_reference + cuda_g4 (performance LiDAR)
profile=quantized_u18_input_only
mode=benchmark_only
public_status=not_claimed
GCP non utilise pour cette conception
```

Rédigé le 28 septembre 2026 par le concepteur v10 du sous-système « évaluation ». Version 2.

Sources lues, en plus de celles de v1 (lentilles L05 à L15, `FAIRNESS.md`, `DATASETS.md`, `PROTOCOLE_POINTS_THESE_SIPU_20260927.md`, banc `synthetic_bench_20260928`, reçu R22, contrats LiDAR v8, sources sklearn 1.9.1) :
- la critique adverse d'EVAL_v1 et ses scripts `design/crit_EVAL/` (`alpha_cx.py`, `alpha_cx2.py`, `ami_grouped.py`, `chainlink_check.py`, journaux, `SHA256SUMS`) ;
- les conceptions sœurs `ARCH_v1.md`, `TOWER_v1.md` (§ 5.7 digests, § 7.3 complétude, `kcat`), `GEN_v1.md` (§ 1.2 boule critique, § 1.3 admission), `CLUSTER_v1.md` (§ 2.3 témoin, § 3 condensation, § 7 têtes) ;
- les sorties `cble` de la lentille L13 (`audit_v9/L13_algo_alternatives/out_*.json`, `RUNS.txt`) et `design/crit_ARCH/lidar02_K5_dom3_M16.{json,time}` ;
- `morsehgp3D_v8/docs/PRECISION_FLOAT32_ET_GRILLE_20260921.md` (règle d'arrondi LiDAR) ;
- `sklearn/cluster/_hdbscan/hdbscan.py` (l. 254 : `distance_matrix /= alpha` dans la voie brute ; l. 355 : `kneighbors`), `_linkage.pyx` (l. 189 : `pair_distance /= alpha`), `_tree.pyx` (l. 160–235 : `_condense_tree`).

Sondes de cette révision, rejouables, sous `design/eval_v2_probe/` (journaux et `SHA256SUMS` à côté) :
- `interleave_check.py` : bornes d'entrelacement tour / atteignabilité mutuelle à alpha 1 et 2 ;
- `sk_alpha_algo.py` : `alpha` est sans effet dans la voie `brute` de sklearn ;
- `chainlink_fix.py` : géométrie corrigée de F12.

Ce document ne revendique rien. Il fixe les règles qui rendront une éventuelle revendication honnête, rejouable et falsifiable. Aucune campagne de ce protocole ne promeut `public_status=exact` : l'exactitude de la tour relève de ses propres portes (T2, juges, certificats), pas d'un score de clustering.

---

## R. Réponse à la critique (traçabilité)

Chaque point de la critique est corrigé, ou réfuté par une preuve écrite. Aucun n'a été réfuté sur le fond. Deux sont acceptés avec une nuance, signalée.

### R.1 Points bloquants

| Point | Verdict | Correction | Où |
|---|---|---|---|
| `alpha` oublié : C∩X est à l'échelle d'alpha = 2 | **Accepté**, avec deux compléments. (1) À K = 1, alpha = 1 et alpha = 2 donnent les mêmes étiquettes dans notre grille, car eps y est un quantile ; l'ancienne PO-E5 n'était donc pas fausse à K = 1, mais le témoin était mal mis à l'échelle dès K ≥ 2. (2) Constat nouveau : dans sklearn 1.9.1, `alpha` est un simple changement d'échelle dans la voie `brute`, où la matrice est divisée avant le calcul des distances-cœur (20 nuages sur 20, `sk_alpha_algo.log`) | Sources `mr1` et `mr2`, `sk1` et `sk2`. Témoin E1 primaire : `mr2`. Alpha ∈ {1, 2} dans G_dev, dans les oracles et dans `hdb_dev_sk`. `hdb_match` à alpha = 2. `algorithm="kd_tree"` épinglé et contrôlé (EG7c). PO-E5 et PO-E6 réécrites et démontrées (§13) | D5, D6, D7, D20 ; §2.3, §4, §13 |
| H3 biaisé : `mr_P` hérite du réglage de la tour | **Accepté** | H3 : `tw_P` contre `mr2_P*`, où P* est réglé sur `dev` pour la source `mr2`, avec le même J, la même grille et le même budget. `mr2_P` et `mr1_P*` passent en diagnostic | D7 ; §4.3, §6.3 |
| Contrat LiDAR : exactitude non établie dans la zone aveugle | **Accepté**, adapté à TOWER_v1, dont la tour publique est bornée à Kmax ≤ 10 | (1) Cohérence de préfixe : l'empreinte d'ordre ≤ 5 de K10 est égale à celle de K5. (2) Référence certifiée à `kcat` = Kmax + 2 (TOWER § 7.3), dont le préfixe d'ordre ≤ Kmax doit égaler la passe chronométrée (tour K12 restreinte à l'ordre ≤ 10 si le moteur l'accepte en mode référence). (3) Juge de clés absentes à plancher explicite : 29 956 clés vraies par (K, strate), taux 10^−4 détecté à 95 %. Deux mutants causaux (EP-M1, EP-M2) | D13 ; §12.5 |

### R.2 Défauts majeurs

| Point | Verdict | Correction | Où |
|---|---|---|---|
| F12 `chainlink` : âmes alternées tangentes | **Accepté** et vérifié (`chainlink_fix.log`) | Décalage d = 4/3. Toutes les âmes distinctes sont alors à ≥ 2/3, ce qui est optimal pour ce motif ; les tores consécutifs restent enlacés. Nouvelle porte EG-F sur toutes les familles G | D25 ; §3.1 |
| Multiplicités incohérentes | **Accepté** | sklearn reçoit les lignes brutes ; `tw` et `mr` reçoivent les sites et leurs multiplicités ; `mult.u32le` entre au contrat et `weight[]` dans `tree.bin` ; métriques calculées sur les lignes ; PO-E18 ; EG18 | D10 ; §2.1, §2.4, §3.9 |
| Troncature Kmax_run = 6 à 16k/32k | **Accepté** | Kmax_run = 10 à toutes les tailles. Toute réduction est symétrique pour toutes les grilles, et décidée avant le choix de P à partir de coûts mesurés sans étiquettes | D19 ; §4.3, §11 |
| Faille d'amendement post-test | **Accepté** | Seule une porte présente au commit PREREG et exécutée automatiquement autorise une réexécution sur les mêmes graines. Toute nouvelle porte ou fixture impose `test2` | D11 ; §7.5 |
| Leviers LiDAR adoptés sur les trames de test | **Accepté** | A/B sur 3 trames de conception plus 20 trames `dev` hors 08, disjointes du test (écart ≥ 20 scans). Les trames de test servent une seule fois par version préenregistrée | D23 ; §12.1, §12.7 |
| Faisabilité LiDAR non chiffrée | **Accepté**, chiffres recalculés | Budgets en µs CPU par boule et par cœur. Mesure `cble` en temps utilisateur : 14,6 µs par boule à K5 sur 08/000200. Projection : C(K5, 1 s) en CPU est **non tenu** à 60k sites (1,2 s pour le générateur seul sur 24 cœurs). Porte go/no-go explicite, avec facteur de conversion κ et efficacité parallèle η | D22 ; §12.6 |
| Shells : loi du rayon non écrite, porte `p_boules` qui mesure les données | **Accepté**, avec une nuance chiffrée | Rayon fixe R0 = 8,9206 (densité 1 à 8k) dans la campagne de qualité. Famille de performance `shells_rho` (loi v9, densité constante). `p_boules` publiée comme propriété des données ; seule porte d'algorithme : `p_travail − p_boules ≤ 0,15` | D26 ; §3.1, §12.8 |
| S_null contournable par l'abstention | **Accepté** | N2 et N3 : S_one = rappel du plus grand groupe sur les inliers (abstention = 0). N1 : S_N1 = 1 − fraction de fausse structure (l'abstention y est légitime) | D21 ; §3.6, §5 |
| EG8 infaisable telle qu'écrite | **Accepté** | EG8 : (a) arbre `tw` à K = 1 identique à `mr2` à K = 1 (structure et rangs) ; (b) tête sur `tw` à K = 1 et z = 1 égale à sklearn HDBSCAN(ms = 1, alpha = 2, kd_tree) sur les scènes sans plateau, écarts comptés sinon | §9.3 |

### R.3 Défauts mineurs

| Point | Correction |
|---|---|
| D2, raison fausse (l'AMI en singletons est faisable) | AMI_s par EMI à marges regroupées (0,04 s à 32k, égale à sklearn à 10^−12 près) ; **seconde garde** avec F1_H (D2) |
| §4.4, rapports faux | Délais justifiés par le coût attendu de la tour (65 à 95 fois aux tailles décisives), et rapports à sklearn corrigés (327 à 2 500 fois) (§4.4) |
| mreach : 0,15 s (ARCH) contre 1 à 3 s (EVAL) | Harmonisé : Borůvka sur kd-tree (CLUSTER § 2.3), estimé à 0,1–0,3 s CPU à 32k par (K, alpha) ; Prim dense entier ≈ 1 s CPU (ARCH), comme référence de qualification ; mesure E6 (§2.3) |
| EG15 irréalisable | Chemin `eval/compat_v9.py` : générateur v9 épinglé comme sujet différentiel, points flottants, graines v9 (§9.1, EG15) |
| `tree.bin` contradictoire, sans poids | Format v2 : `level_rank` unique pour feuilles et nœuds internes, `entry_rank` supprimé, `weight[]` ajouté (§2.4) |
| EG14 non invariant | Restreint à `tw`, `mr1` et `mr2` ; départage par coordonnées (§2.6, EG14) |
| §2.6 « O(n) » faux | O(32 n) typique plus O(n_f log n) de repli ; taux de repli publié (§2.6) |
| Bootstrap à R = 6 : variance sous-estimée | Bootstrap stratifié de McCarthy–Snowden, R_c − 1 tirages par cellule (PO-E17) (§6.2) |
| EG3 tautologique | Nuls de moyenne nulle, asymétriques et hétéroscédastiques ; label `eval_slow`, 1 000 campagnes × 2 000 retournements (§6.2, EG3) |
| Arrondi LiDAR différent de v8 | Règle v8 partout : floor(x/pas + 1/2) en rationnels exacts ; continuité R22 vérifiée par digest de sites (EP6) (D9) |
| D10 : « non altérées » faux | Reformulé : quantification 18 bits, erreur ≤ h/2 publiée (birch : h ≈ 3,8) (D10) |
| Lanceur en pool : `ru_maxrss` faux | Un fork par couple (méthode, scène) depuis un serveur préchargé ; on publie `rss_peak − rss_base` (§4.4) |
| EG13 tautologique | Remplacé : les étiquettes de l'argmax sont rejouées par l'API publique (sklearn ou CLI de tête) (EG13) |
| `hdb_match` indéfini pour une tête multi-K ; refus d'un membre de grille | K_ref(P) déclaré au préenregistrement (K_lo pour T2) ; un membre refusé vaut 0 dans l'argmax, oracle refusé si tous refusent (§4.2, §4.3) |
| Condensation ambiguë | Sémantique exacte écrite : aucun enfant gros → le cluster **se termine** (sklearn `_condense_tree`, cas « deux petits ») ; règle N-aire (§2.5) |
| Contrat LiDAR : gigue masquée ; tête exclue | p95 sur toutes les passes publié ; contrat « chaîne » G + Q + B2 + H publié (§12.2, §12.6) |
| Coût de développement non chiffré | ≈ 30 h CPU par passe `dev` complète ; arbres mis en cache par commit moteur ; passe tour `dev` sur G4 CPU (§11.2) |
| S-pub : puissance symbolique | Déclarée : 8 jeux notés, p minimal 0,0078 ; publication descriptive, sans mot « généralise » (§6.3) |
| Holm sur un test d'intersection-union | R1 = test d'intersection-union (IUT, sans correction entre adversaires) ; Holm entre les 4 revendications (§6.3) |

---

## 0. Décisions tranchées

Chaque décision porte sa raison. Les numéros `Dk` sont repris dans l'objet structuré. Les décisions marquées (v2) sont nouvelles ou modifiées.

| # | Décision | Raison |
|---|---|---|
| D1 | **Métrique primaire : ARI_s** (bruit vrai et bruit prédit deviennent chacun des singletons), calculée sur toutes les **lignes** notées, hors lignes « ignorées ». | L'ARI qui fait du bruit une classe confond couverture et qualité (+0,156 par simple remplissage, L14-F4 ; ARI = 1 à 50 % de couverture, FAIRNESS). ARI_s pénalise le rejet d'un inlier et l'absorption du bruit. Elle se calcule en O(n) (PO-E1), et a été contrôlée contre sklearn dans 200 cas sur 200. |
| D2 (v2) | **Deux gardes : F1 hongrois macro (F1_H) et AMI_s** (AMI en convention singletons). Une perte significative sur l'une ou l'autre bloque la revendication visée. AMI_nc est publiée. | Une garde empêche de chercher la métrique favorable ; AMI_s est l'homologue informationnel d'ARI_s dans la même convention de bruit. Elle est faisable : l'EMI ne dépend que des multiensembles de marges ; en les regroupant, le calcul prend 0,04 s à 32k avec 30 % de bruit, égal à sklearn à 10^−12 près (`crit_EVAL/ami_grouped.log`). |
| D3 | **Unité statistique : la scène**, avec une graine indépendante dérivée par SHA-256 de sa spécification. | En v9, l'unité réelle était la graine (5 graines, p ≥ 0,0625). |
| D4 | **Niveaux géométriques**, définis sans aucune méthode : type O (plafond MAP), type G (rapport d'écart D). Calibration sur un espace `calib` disjoint. | En v9, les niveaux étaient définis par l'échec de HDBSCAN(20, 20) : biais de sélection et bimodalité (L09-F4). |
| D5 (v2) | **« Battre HDBSCAN » = revendication R1 = test d'intersection-union sur six adversaires** : `hdb_dev` (réglé sur `dev` comme la tour, sources `mr1` et `mr2`), `hdb_dev_sk` (appel sklearn pur réglé sur `dev`, alpha ∈ {1, 2}), `hdb_match` (alpha = 2), `hdb_match_fill`, `hdb_lib`, `hdb_these_K1`. Marge δ_min = +0,02 d'ARI_s sur la strate 8k/16k/32k. | À réglage égal, seul l'avantage de la tour reste. `alpha` est un paramètre public de sklearn, et C∩X est à l'échelle d'alpha = 2 (PO-E5, PO-E6) : un HDBSCAN privé d'alpha n'est pas l'adversaire loyal. |
| D6 (v2) | **Oracles sur la même grille de tête.** `tw_oracle_full` : 3 456 configurations. `hdb_oracle_full` : les mêmes, multipliées par alpha ∈ {1, 2}, soit 6 912. Cette asymétrie de cardinalité **dessert la tour** ; elle est déclarée. Le comparateur symétrique `hdb_oracle_full_a2` (3 456) est publié en diagnostic. | Un oracle HDBSCAN qui ignore un paramètre public n'est pas l'oracle HDBSCAN. La tour n'a pas d'analogue d'alpha. Une asymétrie au détriment de la tour ne peut pas fabriquer une victoire. |
| D7 (v2) | **Témoin géométrique E1 = `mr2_P*`** : même famille de têtes que P, réglée sur `dev` pour la source `mr2` avec le même J, la même grille et le même budget. H3 oppose `tw_P` à `mr2_P*`. `mr2_P` (paramètres de P) et `mr1_P*` sont publiés en diagnostic. | À K = 1, la tour est égale à `mr2` (PO-E5). À K ≥ 2, `mr2` en est bien plus proche que `mr1` : ratios de rayons ≤ 1,36 contre ≤ 1,95 (`interleave_check.log`) ; coupes à ARI 0,98 contre 0,50 (`alpha_cx_n55.log`). Réutiliser P sur le témoin hérite du réglage de la tour : c'est un biais. |
| D8 | **Aucun refus silencieux.** Refus = ARI_s 0 et défaite aux tests de signe. Un taux de refus > 1 % dans la strate décisive bloque. | Biais du survivant en v9 (L15-12). |
| D9 (v2) | **Quantification sur 18 bits, règle v8 partout** : q = floor(x/h + 1/2) en rationnels exacts. Synthétiques : pas isotrope h = étendue maximale / (2^18 − 1). LiDAR : pas de 1 mm et translation entière commune de v8 ; domaine [0 ; 2^18), un point hors domaine refuse la trame. | Une seule règle, et continuité bit à bit avec R22 (porte EP6). Le choix entre pair et demi-supérieur est sans effet sur la qualité ; la continuité, elle, se vérifie. |
| D10 (v2) | **Multiplicités.** Scènes générées : zéro doublon au gel (retirage déterministe). Suites publiques et LiDAR : sklearn reçoit les **lignes brutes** quantifiées ; `tw` et `mr` reçoivent les **sites et leurs multiplicités**. Toutes les métriques se calculent sur les lignes. mcs se compte en lignes, c'est-à-dire en masse. | sklearn n'a pas de `sample_weight`. Avec les lignes brutes, sa distance-cœur compte les doublons comme la tour pondérée : PO-E18. Les données publiques sont quantifiées (erreur ≤ h/2, publiée), pas autrement modifiées. |
| D11 (v2) | **Préenregistrement versionné.** Le test s'exécute une seule fois. Une réexécution sur les mêmes graines n'est permise que si l'échec vient d'une porte **présente au commit PREREG et exécutée automatiquement**. Tout autre correctif (nouvelle fixture, nouvelle porte, réglage) impose `test2`. | Sinon, on ne chercherait des défauts que lorsque la tour perd (jardin des chemins qui bifurquent). |
| D12 | **Revendication générée par `decide.py`.** Aucun chiffre de README sans ligne de reçu. | « beats HDBSCAN's oracle » (cda636b5e). |
| D13 (v2) | **Contrat LiDAR sur le mur résident B2.** Exactitude de chaque passe : digest égal à une référence **certifiée** (`kcat` = Kmax + 2), plus cohérence de préfixe K5 ⊂ K10, plus juge de clés absentes à plancher. On publie le p95 des médianes par trame (contrat), le p95 sur toutes les passes, le maximum, B0, et le contrat « chaîne » G + Q + B2 + H. | L'égalité entre bras qui partagent l'algorithme n'atteste pas l'absence d'omission ; la zone aveugle v9 (fin de fenêtre, 26 à 29 % du catalogue à K5) est connue (L10-04). L'objet d'ordre k ne dépend pas de Kmax (PO-E15) : on dispose donc d'un contrôle bon marché qui ne vérifie pas tout. |
| D14 | **Strate décisive = 8k/16k/32k.** | Règle du dépôt. |
| D15 | **`hierarchical2` a deux vérités** ; il contribue par max(ARI_s grossier, ARI_s fin). | Sous-amas à 6,3 σ contre groupes vrais à 5,2 σ (L14-F6). |
| D16 | **Les points de pont sont « ignorés »** (−2). | Vérité générative ambiguë. |
| D17 (v2) | **Délais et mémoire préenregistrés** par taille, fixés à environ 65 à 95 fois le coût attendu de la tour à W8 aux tailles décisives. Tout dépassement est un refus. | Une méthode lente ne doit pas « gagner » en ne rendant rien. |
| D18 | **Les suites publiques ne servent jamais au réglage** ; blocs « déjà vus » et « jamais vus ». Puissance déclarée symbolique (8 jeux notés). | Surajustement. |
| D19 (v2) | **Kmax_run = 10 à toutes les tailles.** Une réduction éventuelle (Kmax_run = K′) vaut pour **toutes** les grilles (tour, oracles, G_dev) et se décide à la fin de la mesure de coût E6-coût, sans étiquettes, **avant** le choix de P. | La troncature à 16k/32k rendait H4 asymétrique et changeait la définition de P selon la taille. L'économie était faible : environ 50 h CPU, soit environ 1,6 h de mur sur G4. |
| D20 (v2) | **sklearn épinglé à `algorithm="kd_tree"`**, contrôlé par une fixture (EG7c). | Dans la voie `brute`, `alpha` divise la matrice avant le calcul des distances-cœur : ce n'est qu'un changement d'échelle (20 nuages sur 20, `sk_alpha_algo.log`). Un `auto` qui basculerait vers `brute` annulerait silencieusement alpha. |
| D21 (v2) | **Témoins nuls notés sans prime à l'abstention.** N1 (aucune structure) : S_N1 = 1 − FS. N2 et N3 (un seul groupe vrai) : S_one = rappel des inliers dans le groupe prédit qui en contient le plus. | S_null valait 1 pour « tout bruit » : sur N2 et N3, l'abstention était récompensée. |
| D22 (v2) | **Porte go/no-go LiDAR avant toute session de contrat G4**, exprimée en µs CPU par boule et par cœur, avec facteur hôte κ et efficacité parallèle η, écrits dans une formule. | Les budgets en ns de mur par boule, sans conversion, ne décidaient rien. `cble` mesure 14,6 µs CPU par boule sur LiDAR à K5, contre 12,1 µs de budget total à 24 cœurs pour 1 s à 60k sites. |
| D23 (v2) | **Trames LiDAR à trois rôles.** Conception : trois trames de 08. Dev : 20 trames hors 08, deux par séquence. Test : 30 trames, trois par séquence. Écart dev–test ≥ 20 scans. Les A/B de leviers se font sur conception + dev ; le test ne sert qu'une fois par version préenregistrée. | Adopter des leviers sur les trames de test accumule un biais de sélection. |
| D24 (v2) | **Structure statistique.** R1 = IUT (chaque adversaire à α = 0,05, p_R1 = max). Holm à α = 0,05 entre les quatre revendications {R1, attribution H3, oracle H4, meilleure hiérarchie}. Test primaire : retournement de signe stratifié (bootstrap sauvage de Rademacher). IC : bootstrap de McCarthy–Snowden. | Holm à l'intérieur d'une IUT est inutilement conservateur. Le retournement de signe reste valide asymptotiquement sous une moyenne nulle hétéroscédastique (PO-E7), ce que contrôle EG3. R_c = 6 sous-estimait la variance de 1/6. |
| D25 (v2) | **Géométrie des familles contrôlée** par la porte EG-F : séparation minimale des supports ≥ marge déclarée, à bruit et épaisseur nuls. F12 : décalage 4/3. | Le défaut de F12 (tangence) serait passé dans l'agrégat avec un poids de 1/16. |
| D26 (v2) | **Loi du rayon des coquilles.** Qualité : R0 = sqrt(1000/(4π)) = 8,9206, fixe (géométrie fixe, n varie). Performance : on ajoute `shells_rho` (loi v9, densité surfacique 1, R ∝ √n). Porte d'algorithme : `p_travail − p_boules ≤ 0,15` seule ; `p_boules` est une propriété des données. | Deux questions distinctes : la consistance statistique (géométrie fixe) et la sensibilité à la sortie à géométrie locale constante (régime LiDAR). |

---

## 1. Ce que le protocole doit rendre impossible (héritage v9 et v1)

| Défaut constaté | Référence | Parade v10 |
|---|---|---|
| Revendication tirée d'un sous-ensemble fumée | L09-F1, L14-F1 | D11, D12 ; plan complet |
| Oracle HDBSCAN à diagonale ms = mcs | L09-F2, L14-F3 | D6 ; mutant `oracle_diagonal` |
| Équité rompue : tour à K = 2 et √n, HDBSCAN à ms = 20 | L09-F3 | `hdb_match`, table K ↔ ms (§4.3) |
| Décalage K+1 = ms | L14-F11 | ms = K (`hdbscan.py:355` : le point compte) ; `hdb_these_K1` à part |
| Niveaux calibrés sur l'échec d'un réglage | L09-F4 | D4 |
| Bruit compté comme une classe | L09-F6, L14-F4 | D1, D2 |
| Réglage sur les graines d'évaluation | L09-F7 | Espaces disjoints ; D11 |
| Survivants silencieux | L15-12 | D8 ; EG11 |
| Forêt de Gabriel à racines multiples (E5) | L08-F2, L14-F2 | EG10 |
| Condensation qui continue sans enfant gros | L08-F1, L11-F2 | §2.5 ; EG8 ; mutant |
| Chrono de composant présenté comme contrat | L07-F3 | §12.2 |
| Nuages KITTI versionnés | L15-06 | §12.1 |
| **(v1)** Adversaires à alpha = 1 seul, témoin mal mis à l'échelle | critique EVAL_v1 | D5 à D7, D20 |
| **(v1)** Témoin réglé pour la tour | critique EVAL_v1 | D7 |
| **(v1)** Exactitude LiDAR jugée par égalité entre bras | critique EVAL_v1, L10-04 | D13 ; §12.5 |
| **(v1)** Famille tangente (F12) | critique EVAL_v1 | D25 ; EG-F |
| **(v1)** Grilles tronquées selon la taille | critique EVAL_v1 | D19 |
| **(v1)** Amendement post-test par une nouvelle porte | critique EVAL_v1 | D11 |
| **(v1)** Leviers adoptés sur le test | critique EVAL_v1 | D23 |
| **(v1)** Abstention récompensée par S_null | critique EVAL_v1 | D21 |

---

## 2. Objet évalué et interface commune

### 2.1 Contrat d'une méthode

Toute méthode M, tour ou adversaire, s'exécute par le lanceur sur une scène S. Elle reçoit, selon sa famille d'entrée :
- **entrée « sites »** (`tw`, `mr1`, `mr2`, têtes) : `sites.u32le` (n_s × 3, valeurs < 2^18) et `mult.u32le` (n_s entiers ≥ 1) ;
- **entrée « lignes »** (tout appel sklearn) : `rows.u32le` (N × 3), les lignes brutes quantifiées dans l'ordre de la scène ;
- dans les deux cas, un dictionnaire de paramètres fermé : aucune valeur par défaut implicite, tout paramètre inconnu est refusé.

`row_site.u32le` (N entiers) relie chaque ligne à son site. Il sert au lanceur, jamais aux méthodes. Pour les scènes générées, N = n_s, toutes les multiplicités valent 1, et `row_site` est l'identité.

Elle rend un enregistrement `run.json` et des fichiers binaires :

| Champ | Type | Obligatoire | Sens |
|---|---|---|---|
| `status` | enum `complete`, `refused_domain`, `resource_exhausted`, `timeout`, `crash`, `numeric_failure`, `invalid_output` | oui | Seul `complete` produit des étiquettes notées |
| `labels.i32le` | n_s × i32 (entrée sites) ou N × i32 (entrée lignes) | si `complete` | −1 = bruit ; ≥ 0 = groupe. Le lanceur diffuse les étiquettes de sites aux lignes par `row_site` |
| `tree.bin` | §2.4 | pour les métriques d'arbre, E1 et les oracles | Hiérarchie de points complète, non condensée |
| `params` | JSON canonique | oui | Paramètres effectifs, `zhat` et `alpha` compris |
| `counters`, `times_ms` | JSON | oui | Compteurs déterministes et temps par phase |
| `rss_peak_mb`, `rss_base_mb` | int | oui | Lus par le lanceur (§4.4), jamais auto-déclarés |
| `versions` | JSON | oui | Commit, sha256 du binaire, versions sklearn, numpy et scipy |

Le lanceur contrôle la cohérence : longueur, identifiants ≥ −1, arbre acyclique à racine unique, poids des feuilles égaux à `mult`. Une sortie incohérente reçoit `invalid_output`, c'est-à-dire un refus.

### 2.2 Hiérarchie de points C∩X de la tour (source `tw`)

- Le site x, de multiplicité m(x), entre dans L_K au niveau a_K(x) = d_K(x)^2. Ici d_K(x) est la distance au K-ième point du **multiensemble**, x compris avec sa multiplicité : si m(x) ≥ K, alors a_K(x) = 0.
- Les nœuds internes sont les composantes de π0(L_K(a)) ∩ X pour a croissant (coupe fermée).
- **Égalités atomisées** : tous les événements de même niveau exact forment une seule multifusion N-aire.
- **Racine unique** pour tout K.

Le calcul appartient au sous-système tour/tête (CLUSTER § 2.2). L'évaluation n'exige que la forme et les portes EG8 à EG10.

### 2.3 Atteignabilité mutuelle exacte au même K (sources `mr1` et `mr2`)

**Niveaux.** On note ρ(x) = d_K(x), la même quantité que pour `tw` (multiplicités comprises). Les niveaux sont des entiers, mis à l'échelle pour le rester :
- `mr1` : w1(x, y) = max(ρ(x)^2, ρ(y)^2, |x − y|^2), c'est-à-dire alpha = 1 ;
- `mr2` : w2(x, y) = max(4ρ(x)^2, 4ρ(y)^2, |x − y|^2) = 4 · max(ρ(x), ρ(y), |x − y|/2)^2, c'est-à-dire alpha = 2.

Sur 18 bits, |x − y|^2 < 3 · 2^36 et 4ρ^2 < 3 · 2^38 < 2^40 : tout tient exactement en u64. Le rayon associé vaut sqrt(w1) pour `mr1` et sqrt(w2)/2 pour `mr2`. Il ne sert qu'à λ, jamais aux égalités.

**Construction.** Borůvka sur kd-tree entier, clé lexicographique (w, min id, max id), d'après CLUSTER § 2.3 : au plus ⌈log2 n⌉ tours, O(n log n) en pratique. Le dendrogramme à plateaux atomisés est unique : tous les arbres couvrants minimaux ont la même suite de composantes.

**Qualification.** Prim dense en entiers (O(n^2), référence de test seulement) doit donner le même multiensemble de niveaux et le même dendrogramme atomisé, sur 200 scènes `dev` avec n ≤ 8k, K ∈ {1, 2, 5, 10} et alpha ∈ {1, 2}.

**Coûts estimés**, remplacés par la mesure E6 :
- Borůvka : 0,1 à 0,3 s CPU à 32k par (K, alpha) ;
- Prim : ≈ 1 s CPU à 32k (ARCH § coûts : « 1 CPU·s, 0,15 s à W8 »). EVAL_v1 annonçait « 1 à 3 s » sans préciser CPU ou mur : c'est corrigé.

**Pourquoi un constructeur exact, et pas l'arbre sklearn.** sklearn calcule en float64 après racine carrée et binarise les plateaux, par un `argsort` non stable (`hdbscan.py:165`). Or les plateaux sont fréquents en atteignabilité mutuelle : toutes les arêtes dominées par ρ(x) ont le même poids. FAIRNESS a relevé 7 écarts sur 80 après atomisation. E1 doit comparer deux hiérarchies soumises au même régime d'égalités exactes.

Les sources `sk1` et `sk2` sont l'arbre `_single_linkage_tree_` de sklearn (kd_tree, alpha = 1 ou 2, binarisation préservée). Elles servent aux adversaires « HDBSCAN standard ».

**Rapport à la tour** (PO-E5, PO-E6, démontrées au §13) :
- à K = 1, `tw` = `mr2` exactement ;
- pour tout K, avec t = rayon de fusion C∩X et m_α = rayon minimax de `mr_α` : m2/2 ≤ t ≤ 2·m2 et m1/2 ≤ t ≤ (3/2)·m1 ;
- mesuré (n = 36, 6 graines, `interleave_check.log`) : t/m2 ≤ 1,36 et m2/t ≤ 1,11, contre m1/t jusqu'à 1,95. `mr2` est le témoin naturel ; `mr1` est l'HDBSCAN standard.

### 2.4 Format `tree.bin` (version 2)

```text
magic "MHGP10TR", version u32 = 2
n_leaves u32                          # sites (feuilles 0..n_leaves-1)
n_nodes u32                           # noeuds internes n_leaves..n_nodes-1
source u8 (0=tw, 1=mr1, 2=mr2, 3=sk1, 4=sk2), K u8, reserve u16
parent[n_nodes] u32                   # racine : UINT32_MAX
level_rank[n_nodes] u32               # feuille : rang du niveau d'entree a_K(x) ; interne : rang du niveau de fusion.
                                      # Rangs communs aux feuilles et aux internes ; rang egal <=> niveau exact egal
level_radius[n_nodes] f64             # rayon du niveau (meme valeur pour un meme rang) ; sert a lambda seulement
weight[n_leaves] u32                  # multiplicite du site (= mult)
sha256 du payload en pied
```

- Le champ `entry_rank` de v1 est supprimé : il doublonnait `level_rank` des feuilles.
- Pour `sk1` et `sk2`, les feuilles sont les lignes (poids 1) et les rangs viennent des distances float64 de sklearn. L'égalité de rang y signifie l'égalité float64, pas l'égalité exacte. C'est déclaré.
- La table rang → niveau rationnel est fournie séparément par la tour (TOWER § 5.7) ; l'évaluation n'en a pas besoin.

### 2.5 Tête commune (exigences de l'évaluation)

La tête est conçue par CLUSTER (§ 3 à § 7). L'évaluation lui impose de s'appliquer **à toute source** (`tw`, `mr1`, `mr2`, `sk1`, `sk2`) avec les mêmes paramètres :

| Paramètre | Domaine | Remarque |
|---|---|---|
| `K` | Kgrid = {1, 2, 3, 4, 5, 6, 8, 10} | Ordre de la tour ou min_samples (point compris) |
| `mcs` | {5, 10, 20, 50, 100, `sqrt`} | En masse (lignes) ; `sqrt` = round(√N), N = nombre de lignes |
| `sel` | {`eom`, `leaf`} | Égalité EOM → parent, comme sklearn |
| `eps` | {0, `q50`, `q90`} | Quantile des rayons de fusion **propres à la source** ; indépendant de toute échelle, donc de alpha à K = 1 ; sémantique `epsilon_search` et `traverse_upwards` de sklearn (`_tree.pyx:578-632`) |
| `asc` | {F, T} | `allow_single_cluster` ; racine unique née à λ = 0 |
| `z` | {1, `zhat`} | λ = r^(−z) ; `zhat` = MLE de Levina–Bickel / MacKay–Ghahramani, k = 10, point exclu, calculé une fois par scène sur les sites (pondérés), commun à toutes les sources |
| `fill` | {`none`, `b2`, `full`} (plus `b1.5` et `b3` en dev seulement) | §2.6 |
| type de tête | T1 (EOM-densité à K fixe), T2 (tranche γ), T3 (§ 9.1 sur facettes, tour seule) | CLUSTER § 7 |

**Condensation : sémantique exacte** (sklearn `_condense_tree`, `_tree.pyx:160-235`, généralisée aux multifusions N-aires). Un nœud u du cluster c se scinde au niveau de rayon r_u, avec L = λ(r_u) ; on note B l'ensemble de ses enfants de masse ≥ mcs.
- **|B| ≥ 2** : chaque enfant de B devient un nouveau cluster, né à L ; les atomes des enfants hors B sortent de c à L.
- **|B| = 1** : l'enfant gros **continue** c ; les atomes des autres enfants sortent de c à L.
- **|B| = 0** : **c se termine à L** ; tous ses atomes restants sortent à L. C'est le cas « deux petits » de sklearn : aucune descente ne continue.

Autres règles :
- Un atome de masse ≥ mcs qui atteint la file sort à sa propre date d'entrée (CLUSTER § 3).
- Un niveau de rayon nul donne λ = +∞, comme sklearn (`INFTY`, `_tree.pyx:179`).
- Pour une scission binaire, cette règle **est** celle de sklearn. Sur un plateau binarisé par sklearn, les deux formulations ne diffèrent que par des clusters de durée nulle ; ces écarts sont comptés (EG7).

**Portes exigées de la tête** : EG7, EG8, EG9, EG10 et EG18 (§9.3).

### 2.6 Politiques de bruit, identiques pour toutes les sources

On note core_K(x) = d_K(x), la même quantité pour `tw`, `mr1`, `mr2` (sites pondérés) et pour `sk1`, `sk2` (lignes) au même K (PO-E4, PO-E18).

- `none` : les étiquettes de la tête, telles quelles.
- `full` : chaque site de bruit reçoit l'étiquette du site classé le plus proche.
  - Distance exacte en entiers.
  - Égalité départagée par l'**ordre lexicographique des coordonnées** (x, y, z) du site classé. Les sites sont distincts, donc le départage est total et invariant par permutation.
  - Pour les sources de lignes (`sk1`, `sk2`), le départage se fait sur les coordonnées, puis sur le plus petit indice de ligne. Chez sklearn, deux lignes de même position peuvent recevoir des étiquettes différentes : sur un plateau binarisé, elles peuvent tomber dans deux enfants gros nés au même λ. Ces conflits aux doublons sont comptés et publiés ; les sources `sk` sont exclues d'EG14.
- `b<ρ>` (remplissage borné, L14 `campaign3.py`) :
  - soit q le site classé le plus proche de p (départage ci-dessus) et c son groupe ;
  - p reçoit c si core(p) ≤ ρ · Q95_c, où core est la distance au max(K, 5)-ième voisin, point compris, et Q95_c le quantile 95 % (type 7) de core sur les membres de c, pondéré par les multiplicités ;
  - *correction du 29 septembre 2026* (audit de la hiérarchie K-NN) : ce paragraphe disait core_K, en contradiction avec CLUSTER_v2 § 8.1. Le banc (`bench/synthetic/methods.py`, `bounded_fill`) et les préenregistrements des lots A et C appliquent max(K, 5). Le banc départage aussi les égalités de plus proche dans l'ordre de cKDTree, et non dans l'ordre lexicographique ci-dessus : divergence déclarée, non mesurée ;
  - sinon, p reste du bruit.

**Implémentation.** Une liste des 32 plus proches voisins est précalculée par scène. Le site classé est cherché d'abord dans cette liste ; à défaut, un repli exact interroge un kd-tree des sites classés. Coût : O(32 n) plus O(n_f log n), où n_f est le nombre de replis. Le taux de repli est publié : à ν = 0,3, il n'est pas négligeable.

---

## 3. Données

### 3.1 Familles générées (3D)

Unité de longueur : σ = 1. Génération en float64, puis quantification (§3.8). Il y a 16 familles notées et 3 témoins nuls (§3.6). Types de niveau : **O** (supports qui se recouvrent, cible de plafond MAP) et **G** (supports disjoints, cible du rapport d'écart D).

| id | Famille | g | Support (dimension locale) | Paramètre θ | Niveau | Piège isolé |
|---|---|---:|---|---|---|---|
| F01 | `spherical` | 8 | Gaussiennes isotropes, centres `centres(8)` × δ (3) | δ | O | Seuil de densité entre groupes proches |
| F02 | `anisotropic` | 8 | Gaussiennes diag(2 ; 1 ; 0,35), rotation aléatoire (3) | δ | O | Chaînage d'amas allongés |
| F03 | `heteroscedastic` | 8 | σ ∈ {0,4 ; 1 ; 2,5} en cycle (3) | δ | O | Échelle unique |
| F04 | `unbalanced` | 8 | Poids 32/2^min(j, 5) (3) | δ | O | Petits groupes contre mcs |
| F05 | `hierarchical2` | 4 × 3 | 4 groupes à écart 8, chacun de 3 sous-gaussiennes (3) | δ_sub | O (niveau fin) | Choix du niveau |
| F06 | `hepta` | 7 | Centrale plus 6 sur les demi-axes à δ (3) | δ | O | FCPS Hepta |
| F07 | `tetra` | 4 | Sommets d'un tétraèdre régulier d'arête δ (3) | δ | O | Contact |
| F08 | `atom` | 2 | Noyau (boule uniforme de rayon 1) et coquille de rayon R, épaisseur gaussienne 0,02 R (3 et 2) | R | G | Densités imbriquées ; z décisif |
| F09 | `shells` | 8 | Sphères creuses de **rayon fixe R0 = 8,9206**, épaisseur radiale gaussienne 0,15, centres (2 R0 + θ) · `centres(8)` (2) | θ, écart entre surfaces | G | Surfaces (régime LiDAR) |
| F10 | `bridge` | 8 | Gaussiennes σ = 1 à δ = 7, ponts de largeur 0,25 notés −2 (3, pont ≈ 1) | fraction de pont | G (D_pont) | Chaînage par pont |
| F11 | `filaments` | 8 | Segments de longueur 3, tube gaussien 0,18 | δ | G | z déclaré faux (L14-F7) |
| F12 | `chainlink` | 4 | 4 tores enlacés : rayon majeur 1, plans alternés xy/xz, **décalage 4/3 le long de x**, tube gaussien σ_t (1) | σ_t | G | Non convexe, enlacé |
| F13 | `moons` | 2 | Demi-lunes (`make_moons`), bruit plan σ_p, extrusion z ~ U(−0,15 ; 0,15) (≈ 2) | σ_p | G | Non convexe plongé |
| F14 | `spirals` | 3 | Bras archimédiens r = 0,5 φ, φ ∈ [π/2 ; 3π], bruit plan σ_p, extrusion U(−0,15 ; 0,15) (≈ 2) | σ_p | G | Spirales |
| F15 | `helices` | 2 | Double hélice, rayon 1, pas 2, 4 tours, déphasage π, tube σ_t (1) | σ_t | G | Filaments enlacés |
| F16 | `mixdim` | 8 | 2 plaques 2×2 (épaisseur 0,02), 3 segments de longueur 3 (tube 0,05), 3 gaussiennes σ = 0,3, sur `centres(8)` × δ (1, 2 et 3 mêlés) | δ | G | Dimensions mêlées |

**F12, justification du décalage** (`chainlink_fix.log`). Avec deux cercles unités de plans orthogonaux, centres décalés de d le long de x :
- deux tores consécutifs sont à distance d'âme min(d, 2 − d) et restent enlacés si 0 < d < 2 ;
- deux tores alternés (même plan) sont à 2d − 2 ;
- d = 4/3 égalise ces deux distances à 2/3 : c'est le décalage qui maximise la séparation minimale ;
- mesures à tube nul : consécutifs 0,6667 et enlacés (une traversée de disque) ; alternés 0,6667 ; 0–3 : 2,0000. À d = 1, les alternés 0–2 et 1–3 étaient à 1,2·10^−16.

**F09, loi du rayon (D26).**
- R0 est fixe à toutes les tailles : c'est la densité surfacique 1 à n = 8000, ν = 0, g = 8.
- À 32k, la densité vaut 4, l'espacement moyen au plus proche voisin vaut 0,25, et l'épaisseur gaussienne 0,15 (largeur utile ±2σ = 0,6) devient comparable. Localement, le régime s'épaissit. C'est une propriété des données, publiée ; ce n'est pas un défaut d'algorithme.
- La famille `shells_rho` (performance seulement, §12.8) garde la loi v9 R = sqrt(n_g/(4π)).

Règles communes du générateur (inchangées depuis v1) :
- effectifs exacts ;
- bruit uniforme dans la boîte englobante du signal élargie de 10 % ;
- ponts pris sur le budget du signal ;
- `truth_fine` pour `hierarchical2` ;
- permutation des lignes par la graine ;
- constantes des formes dans `FAMILIES_v2.toml`.

**Porte EG-F (géométrie des supports).** Pour chaque famille G et chaque niveau, à bruit nul, épaisseur nulle et tube nul, on échantillonne densément les supports (10^5 points par composante) et on mesure la distance minimale entre composantes distinctes par kd-tree. Elle doit être ≥ la marge déclarée dans `FAMILIES_v2.toml` (> 0). Plancher : les 12 familles G × 4 niveaux. Un décalage ou une tangence comme celle de F12 en v1 fait échouer la porte en code 3.

### 3.2 Niveaux : calibration géométrique

Inchangé depuis v1, pour l'essentiel. Quatre niveaux par famille, calibrés à n = 8000, ν = 0, puis même θ à toutes les tailles.
- **Type O** : ARI_MAP (convention ARI_s) ; cibles 0,99 / 0,95 / 0,85 / 0,70 ; pour `hierarchical2`, cible sur le niveau fin, niveaux `c1` à `c4`.
- **Type G** : D = q05(d_inter)/médiane(d_intra) ; cibles 6 / 3,5 / 2,2 / 1,4 ; `bridge` : D_pont, cibles 4 / 2,5 / 1,6 / 1,2.

Procédure :
1. bissection sur 20 graines `calib`, tolérance ±0,01 (O) ou ±0,05 (G) ;
2. contrôle de monotonie sur une grille de 16 valeurs ;
3. gel dans `FAMILIES_v2.toml`.

Porte EG-L : recalcul sur `calib_check`, écart ≤ 0,02 (O) ou ≤ 0,1 (G).

Le panneau descriptif (ARI_MAP, D, `hdb_lib`, `hdb_these_K`, liaison simple au vrai k, GMM au vrai k, bimodalité de Sarle) se publie **sur `calib_check` seulement**. Une cible peut être corrigée une seule fois, avant le premier run `dev`, si le plafond supervisé est < 0,5 ou si toutes les références sont ≥ 0,995. ARI_MAP est un indicateur, pas une borne (PO-E9).

Conséquence de D26 : avec R0 fixe, la calibration de F09 porte sur θ à R0 constant ; la variation de densité avec n fait partie de l'axe des tailles.

### 3.3 Bruit, lignes ignorées, vérités à deux niveaux

- ν ∈ {0 ; 0,1 ; 0,3} pour chaque famille notée.
- Vérité : ≥ 0 pour un groupe, −1 pour le bruit (singleton dans ARI_s et AMI_s), −2 pour une ligne ignorée (retirée avant toute métrique).
- `hierarchical2` : deux vérités.

### 3.4 Tailles, répétitions, plans

| Espace | Tailles | Répétitions par cellule | Scènes (192 cellules notées + 7 nulles = 199 par taille) |
|---|---|---|---|
| `calib` | 8000 (ν = 0) | 20 | génération et MAP/D seulement |
| `calib_check` | 8000 (ν = 0) | 20 | 1 280 |
| `cost` (v2) | 8000 / 16000 / 32000 | 1, sur 32 cellules tirées | 96 ; coûts seulement, **aucune métrique lue** (§7.1) |
| `dev` | 500 / 2000 / 8000 / 16000 / 32000 | 3 / 3 / 3 / 1 / 1 | 2 189 |
| `test` | 500 / 2000 / 8000 / 16000 / 32000 | 10 / 10 / 10 / 6 / 6 | 8 358, plus l'axe g (80) = 8 438 |
| `test2` | réserve, jamais générée sans amendement | identique | — |

- Volume du test : 199 × 393 000 ≈ 78,2 M lignes, plus 0,64 M pour l'axe g (`spherical` et `filaments`, n = 8000, medium, ν = 0, g ∈ {2, 4, 16, 32}, 10 répétitions ; hors décision).
- Six répétitions à 16k/32k suffisent : 72 scènes indépendantes par famille et par taille. Le bootstrap corrige le biais de petite cellule (§6.2).

### 3.5 Graines et déterminisme

Inchangé depuis v1 :

```text
seed(split, spec, rep) = int.from_bytes(sha256(b"ehgp-v10-eval/v2|" + split + b"|" + canon(spec) + b"|" + str(rep)).digest()[:8], "little")
canon(spec) = json.dumps(spec, sort_keys=True, separators=(",", ":"))
RNG = numpy.random.Generator(numpy.random.PCG64(seed)) ; sous-flux par SeedSequence(seed).spawn
```

- Espaces : `calib`, `calib_check`, `cost`, `dev`, `test`, `test2`, `perf`. Porte EG5 : disjonction des graines et des digests.
- Versions épinglées. Le manifeste contient le sha256 de chaque scène ; le lanceur refuse toute scène dont le digest diffère.
- Les données générées ne sont jamais versionnées.

### 3.6 Témoins nuls (sans structure)

| id | Scène | Vérité | Bruit | Score (D21) |
|---|---|---|---|---|
| N1 | Cube uniforme [0 ; 1]^3 | aucune structure : « tout bruit » et « un seul groupe » sont également justes | sans objet | S_N1 = 1 − FS, avec FS = (nombre de lignes dans les groupes prédits autres que le plus grand)/N ; abstention = 1, un groupe = 1, deux moitiés = 0,5 |
| N2 | `golfball` : sphère unité, épaisseur radiale 0,01 (2) | un seul groupe | {0 ; 0,1 ; 0,3} | S_one = max_j #{lignes du groupe prédit j ∩ inliers vrais}/#{inliers vrais} ; abstention = 0, deux moitiés ≈ 0,5 |
| N3 | Gaussienne N(0, I) | un seul groupe | {0 ; 0,1 ; 0,3} | S_one |

On publie aussi k̂, la fraction de bruit prédit et la fraction de vrai bruit absorbé. Pour ν > 0, l'ARI_s est aussi publiée : elle n'est pas dégénérée quand la vérité contient des singletons.

Conséquence assumée : avec asc = F, ni HDBSCAN ni la tour ne peuvent rendre la racine. Leur meilleur score sur N2 et N3 est celui de l'enfant le plus gros. C'est une propriété réelle de la configuration, et le réglage peut choisir asc = T.

Les témoins nuls sont hors agrégat ARI. Ils ont leur règle de non-infériorité (§6.6).

### 3.7 Suites publiques épinglées

Inchangées depuis v1 (FCPS au commit `c9c55f4a…`, SIPU en texte LF, licences, blocs « déjà vus » et « jamais vus », Chameleon exclus), avec deux corrections :
- **Doublons (D10).** sklearn reçoit les lignes brutes quantifiées ; `tw` et `mr` reçoivent les sites et leurs multiplicités. Les étiquettes de sites sont diffusées aux lignes. Métriques sur les lignes. On publie les sites de multiplicité ≥ 2, la multiplicité maximale, et le nombre de sites de multiplicité ≥ K pour chaque K de Kgrid : ils entrent au niveau 0, donc à λ = +∞, des deux côtés.
- **Quantification.** Plongement 2D exact en z = 0, puis quantification 18 bits relative (§3.8). L'erreur maximale est publiée (birch1/birch2 : pas h ≈ 3,8 unités de données). Aucune autre transformation.

Puissance (D18) : le bloc « jamais vus » compte 8 jeux notés en ARI_s (EngyTime, Lsun, Target, TwoDiamonds, WingNut, compound, pathbased, R15), plus GolfBall, noté en S_one. La p-valeur minimale d'un Wilcoxon bilatéral sur 8 jeux est 2/2^8 = 0,0078. La publication est **descriptive** (victoires, défaites, Δ par jeu) ; aucune phrase ne dit « généralise ».

### 3.8 Quantification commune sur 18 bits (D9)

Pour chaque scène synthétique ou publique :
- origine o_j = min_i X_ij ;
- pas h = max_j (max_i X_ij − o_j)/(2^18 − 1), rationnel exact depuis les binary64 ;
- q_ij = floor((X_ij − o_j)/h + 1/2), calculé en `fractions.Fraction`.

Contrôle rationnel : |q_ij h + o_j − X_ij| ≤ h/2. On publie h (numérateur et dénominateur), o et l'erreur maximale.

LiDAR : §12.1.

Doublons après quantification :
- **scènes générées** : retirage déterministe (sous-flux indexé par (indice, tentative), au plus 8 tentatives), 0 doublon au gel ;
- **scènes publiques** : sites avec multiplicité (§3.7).

sklearn reçoit les lignes q en float64 : les entiers < 2^18 y sont exacts.

### 3.9 Formats des scènes et des résultats

```text
scene/<split>/<scene_id>/
  sites.u32le  mult.u32le  rows.u32le  row_site.u32le
  truth.i32le        N i32 par ligne (>=0 groupe, -1 bruit, -2 ignore)
  truth_fine.i32le   (hierarchical2)
  scene.json         {spec, split, rep, seed, n_sites, n_rows, h_num, h_den, origin, max_err, dup_redraws,
                      mult_max, n_mult_ge_K{K}, map_ari | D, zhat, digests{sites,mult,rows,truth}, generator_sha256, numpy_version}
run/<campaign>/<method>/<scene_id>/
  labels.i32le, tree.bin (optionnel), run.json
```

`results.csv.gz` a une ligne par (méthode, scène) du plan, refus compris :

```text
campaign, split, scene_id, family, level, noise, n_rows, n_sites, rep, seed, sites_sha8,
method, source, alpha, head, K, K_ref, mcs, sel, eps, asc, z, zhat, fill, params_sha8,
status, ARI_s, ARI_nc, ARI_cov, coverage, coverage_in, AMI_s, AMI_nc, F1_H,
noise_P, noise_R, noise_F1, absorbed_noise, k_hat, k_true, ARI_s_coarse, ARI_s_fine, ARI_s_best,
DP, DP_fine, BNF1, BNF1_fine, S_N1, S_one, fill_fallback_rate,
wall_s, cpu_s, rss_peak_mb, rss_base_mb, label_sha8, oracle_choice, oracle_refused_members
```

Les méthodes « oracle » écrivent une ligne par scène : celle de leur argmax. L'égalité se départage par ordre lexicographique de la configuration canonique. Un membre de grille refusé vaut ARI_s = 0 dans l'argmax ; il est compté dans `oracle_refused_members`. Si tous les membres refusent, l'oracle refuse.

### 3.10 Piste exploratoire : instances SemanticKITTI (hors décision)

Inchangée depuis v1 : trames **dev** seulement (§12.1), jamais les trames de test de performance. ARI_s et F1_H sur les points « thing », DBSCAN euclidien usuel en plus des adversaires.

---

## 4. Méthodes comparées

### 4.1 Nommage

Grammaire : `<source>[_a<alpha>]_<tete>_K<K>_m<mcs>_<sel>_e<eps>_a<asc>_z<z>_f<fill>`. Exemple : `mr2_T1_K2_msqrt_eom_e0_aF_zzhat_fb2`.

Sources : `tw`, `mr1`, `mr2`, `sk1`, `sk2` (§2.3), et `tf` (facettes FULL, tête T3). Les références portent le nom de leurs paramètres ; `hdbscan_default` n'existe plus.

### 4.2 Adversaires HDBSCAN

La bibliothèque est sklearn 1.9.1, sources épinglées (`_tree.pyx` `3865db026a7b3fdd…`, `hdbscan.py` `6f030ac6b5bfbb88…`, `_linkage.pyx` `544621e8…`). Paramètres fixés : `algorithm="kd_tree"` (D20), `leaf_size=40`, `n_jobs=1`, `copy=True`. Le lanceur refuse toute autre version et tout autre algorithme. alpha n'est jamais laissé par défaut : il est écrit dans `params`.

| Nom | Définition | Étiquette | Rôle |
|---|---|---|---|
| `hdb_lib` | `HDBSCAN()` aux défauts (mcs = 5, ms = 5, alpha = 1, eom, eps = 0, asc = F), kd_tree | sans vérité | Usage naïf ; IUT |
| `hdb_these_K1` | mcs = round(√N), ms = 3, alpha = 1, eom | sans vérité | Protocole de la thèse et de HGP-old ; IUT |
| `hdb_these_K` | mcs = round(√N), ms = 2, alpha = 1, eom | sans vérité | Convention ms = K ; publié |
| `hdb_match` | ms = K_ref(P), **alpha = 2**, mcs, sel, eps (quantile converti en `cluster_selection_epsilon` absolu sur l'arbre sklearn) et asc de P ; z = 1 ; sans remplissage | sans vérité | HDBSCAN apparié à l'échelle de la tour ; IUT |
| `hdb_match_a1` | `hdb_match` à alpha = 1 | sans vérité | Diagnostic |
| `hdb_match_fill` | `hdb_match`, puis `fill_P` avec core_{K_ref} | sans vérité | Isole le remplissage ; IUT |
| `hdb_dev` | argmax de J sur `dev` (§7.2), sur G_dev = sources {`mr1`, `mr2`} × grille de tête complète × têtes de points {T1, T2} | réglé sur `dev` | **Adversaire décisif** ; IUT |
| `hdb_dev_sk` | argmax de J sur `dev`, appels sklearn purs : sources {`sk1`, `sk2`} × Kgrid × mcs × sel × eps × asc, z = 1, sans remplissage (1 152 configurations) | réglé sur `dev` | Ce que peut faire un utilisateur de sklearn seul ; IUT |
| `hdb_oracle_std` | argmax d'ARI_s par scène, sources {`sk1`, `sk2`} × 576 = 1 152 configurations ; z = 1, sans remplissage | supervisé | Oracle HDBSCAN standard |
| `hdb_oracle_fill` | idem × fill ∈ {`none`, `b2`, `full`} = 3 456 | supervisé | Avec politique de bruit |
| `hdb_oracle_full` | sources {`mr1`, `mr2`} × grille complète de `tw_oracle_full` (3 456) = 6 912 | supervisé | **Adversaire de H4** (asymétrie conservatrice, D6) |
| `hdb_oracle_full_a2` | source `mr2` seule, 3 456 | supervisé | Comparateur symétrique de H4, diagnostic |
| `hdb_oracle_wide` | `hdb_oracle_std` avec ms ∈ Kgrid ∪ {12, 16, 20, 32, 64} | supervisé | Diagnostic au-delà de Kmax |

Kgrid = {1, 2, 3, 4, 5, 6, 8, 10}. ms = 1 est inclus : c'est le choix de l'oracle dans 118 cas sur 160 (L09).

**Mise en œuvre des sources `sk`.** Un ajustement sklearn par (ms, alpha), soit 16 par scène, pour obtenir `_single_linkage_tree_` (attribut privé, schéma vérifié). La tête commune tourne ensuite en mode « binarisation préservée ».

Autres références, publiées, hors décision : `gmm_bic`, `optics_xi` (n ≤ 8000), `sl_truek`, `gmm_truek`, `map_model`, comme en v1.

### 4.3 Tour

**`tw_P`.** Configuration primaire unique, choisie sur `dev` (§7.2) et gelée. La tête est T1, T2 ou T3 (CLUSTER § 7). Correspondance imposée :

| Tour | HDBSCAN | Justification |
|---|---|---|
| K (point compris, multiplicités comprises) | min_samples = K sur les lignes | `kneighbors(X, min_samples)` inclut le point (`hdbscan.py:355`) ; PO-E4, PO-E18 |
| Échelle des fusions (rayon de boule, ≈ d/2 à une fusion de paire) | alpha = 2 | PO-E5, PO-E6 |
| mcs en masse | min_cluster_size en lignes | Même unité |
| λ = r^(−z) | λ = 1/d (z = 1) | z = 1 donne l'EOM standard |
| eps, quantile de ses propres fusions | `cluster_selection_epsilon` = même quantile de l'arbre sklearn, en distance | Sans unité commune |
| racine unique, `asc` | `allow_single_cluster` | L08-F9 |
| fill | fill identique (§2.6), même core_K | PO-E4 |

**K_ref(P)** (pour `hdb_match` et S-arbre).
- Tête à K fixe (T1, T3) : K_ref = K_P.
- Tête multi-K (T2, tranche γ) : K_ref = K_lo, déclaré au préenregistrement.
- Raison : avec mcs = √N, EOM sélectionne aux grandes échelles, où k(s) = K_lo. Les autres ms de [K_lo ; K_hi] sont publiés en diagnostic ; ils ne sont pas ajoutés à l'IUT, où `hdb_dev` les couvre déjà.

**`tw_V`.** Variantes préenregistrées, publiées, non confirmatoires. Par défaut : K ∈ {2, 3, 5} × z ∈ {1, `zhat`} × sel × fill ∈ {`none`, `b2`}, soit 24 variantes, plus les lignes secondaires déclarées par CLUSTER.

**`tw_oracle_full`.** argmax d'ARI_s sur Kgrid × mcs (6) × sel (2) × eps (3) × asc (2) × z (2) × fill (3) = 3 456 configurations, tête T1.

**Témoin E1 (D7).**
- **`mr2_P*`** : argmax de J sur `dev`, source `mr2`, même grille de têtes applicables aux points que P, même budget (§7.2). Si P est T1 ou T2, la grille contient le type de P. Si P est T3 (facettes, tour seule), la grille est celle des têtes de points : E1 se lit alors « tête sur facettes contre meilleure tête de points à l'échelle de la tour », et le déclare.
- Diagnostics : `mr2_P` (paramètres de P, là où ils s'appliquent) et `mr1_P*`.

**Règle de budget symétrique.** Toute option de tête applicable à une hiérarchie de points, explorée pour la tour sur `dev`, figure aussi dans G_dev, dans la grille de `mr2_P*` et dans celle de `mr1_P*`. Seules les options propres à la tour (masses de facettes, verticales exactes) en sont dispensées.

**Kmax d'exécution (D19).** Kmax_run = 10 à toutes les tailles. Une réduction à K′ n'est possible qu'à la fin d'E6-coût (§7.1) et vaut alors pour Kgrid partout. Elle est écrite dans `PREREG` avant le choix de P.

### 4.4 Délais, mémoire, lanceur

**Lanceur.**
- Un **fork par couple (méthode, scène)**, depuis un serveur préchargé (Python avec numpy et sklearn importés ; binaires C++ lancés par `posix_spawn` depuis ce fils).
- `wait4` fournit `ru_maxrss` du fils. `rss_base` est lu dans `/proc/self/status` au début du fils. On publie `rss_peak − rss_base`.
- `setrlimit(RLIMIT_AS)` et le délai de mur au W déclaré (W = 8 pour la tour, W = 1 pour sklearn et les têtes). Tout dépassement est un refus.

| n | 500 | 2 000 | 8 000 | 16 000 | 32 000 | ≥ 10^5 (public) |
|---|---:|---:|---:|---:|---:|---:|
| Délai de mur (s) | 20 | 60 | 300 | 900 | 1 800 | 1 800 |
| Mémoire (Gio) | 4 | 4 | 8 | 16 | 24 | 32 |
| Tour attendue à W8, Kmax 10, pire famille (s) | ≈ 0,3 | ≈ 1,2 | ≈ 4 à 5 | ≈ 9 à 10 | ≈ 20 | — |
| sklearn, un ajustement (s, L07 §8) | 0,008 | 0,033 | 0,51 | ≈ 2 | 5,5 | — |

**Justification (corrigée).**
- Les délais valent environ 65 à 95 fois le coût attendu de la tour à W8 aux tailles décisives (8k : 67 ; 16k : 95 ; 32k : 90). Coût attendu : ≈ 420 boules par site × n × 10 µs CPU (générateur et tour), divisé par 8 fils.
- Ils valent 327 à 2 500 fois un ajustement sklearn.
- Un dépassement signale une pente pathologique, pas un aléa.

Mémoire à 32k et K10 : 13,5 M boules (L13 : 421,6 par site en uniforme). Cela fait 0,40 Gio à 32 o par boule, objectif v10 ; 10,2 Gio à 810 o par boule, niveau v9. Dans les deux cas, cela tient sous 24 Gio.

---

## 5. Métriques

Notations :
- y est la vérité par ligne, après retrait des −2 ; ŷ est la prédiction par ligne ;
- n est le nombre de lignes notées, N_p = C(n, 2) ;
- n_ij, r_i et s_j sont les effectifs de la table de contingence **sur les étiquettes ≥ 0 seulement**.

| Métrique | Définition exacte | Coût | Rôle |
|---|---|---|---|
| **ARI_s** | a = Σ_ij C(n_ij, 2), b = Σ_i C(r_i, 2), c = Σ_j C(s_j, 2), e = bc/N_p, m = (b + c)/2 ; ARI_s = (a − e)/(m − e). Si m = e : 1 si les partitions sont identiques, 0 sinon | O(n) | **Primaire** |
| **AMI_s** (v2) | AMI de sklearn (normalisation arithmétique) sur les étiquettes où chaque −1 devient un singleton. EMI par regroupement des marges égales : Σ sur les couples de tailles distinctes (a, b) de mult(a)·mult(b)·Σ_{n_ij} terme hypergéométrique (`crit_EVAL/ami_grouped.py`) | O(A_d·B_d·min(a, b)), A_d et B_d = nombre de tailles distinctes ; 0,04 s à 32k | **Garde** |
| **F1_H** | Appariement hongrois maximisant Σ F1(i, j), avec F1(i, j) = 2 n_ij/(r_i + s_j) (r_i inclut les lignes de i prédites bruit, s_j le vrai bruit dans j) ; moyenne macro, 0 pour une classe non appariée | O(k1·k2 + k^3) | **Garde** |
| ARI_nc, AMI_nc | sklearn, −1 traité comme une classe | O(n) | Héritage, publiées |
| ARI_cov, coverage, coverage_in | comme v1 | O(n) | Publiées ensemble |
| noise_P, noise_R, noise_F1, absorbed_noise | rejet = classe positive ; absorbed_noise = #{y = −1, ŷ ≥ 0}/#{y = −1} | O(n) | |
| k̂, Δk | | O(n) | |
| ARI_s_coarse, ARI_s_fine, ARI_s_best | `hierarchical2` | O(n) | ARI_s_best entre dans l'agrégat |
| **DP** | Pureté de dendrogramme sur la hiérarchie complète, atomisée, pondérée par les lignes | O(n log^2 n) par fusion petite-dans-grande (PO-E2) | Qualité d'arbre |
| **BNF1** | Pour chaque classe vraie c : max sur les nœuds v de 2 n_c(v)/(\|c\| + \|v\|) ; moyenne macro | O(n log^2 n) | « La classe existe-t-elle comme nœud ? » |
| DP_fine, BNF1_fine | contre la vérité fine | | |
| S_N1, S_one | §3.6 | O(n) | Témoins nuls |

Remarques :
- ARI_s ignore toute paire qui touche un singleton. Fixture : vérité [0, 0, 1, 1], prédiction [0, 0, −1, −1] donne ARI_nc = 1 et ARI_s = 4/7 (0,5714).
- Métriques d'arbre d'un oracle : arbre de son argmax, jamais un maximum sur les arbres.
- Pour `sk1` et `sk2`, DP et BNF1 se calculent sur l'arbre binarisé : une subdivision de même niveau ne peut pas améliorer DP (FAIRNESS).

---

## 6. Statistique

### 6.1 Unité, appariement, pondération

- L'unité est la scène s : Δ_s = score_T(s) − score_A(s).
- Poids w_s = 1/(F · |Σ| · C_f · R_{c(s)}), avec F = 16 familles, C_f = 12 cellules par famille et par taille, et R_c le nombre de répétitions. On pose Δ̄ = Σ_s w_s Δ_s.
- `hierarchical2` contribue par ARI_s_best ; les témoins nuls sont hors agrégat.
- Strate décisive Σ* = {8000, 16000, 32000} ; strate secondaire {500, 2000}.

### 6.2 Intervalles et tests (D24)

**IC : bootstrap stratifié de McCarthy–Snowden.**
- Dans chaque cellule c, on tire **R_c − 1** répétitions avec remise, puis on recalcule Δ̄ pondéré.
- La variance bootstrap de la moyenne de cellule vaut alors s_c^2/R_c, l'estimateur sans biais, au lieu de (R_c − 1)/R_c · s_c^2/R_c (PO-E17). Sans cette correction, la variance est sous-estimée de 1/6 à R_c = 6 et de 1/10 à R_c = 10.
- B = 10 000 tirages, IC percentile à 95 %, graine = sha256(préenregistrement).

**Test primaire : retournement de signe stratifié.**
- Chaque Δ_s est multiplié par un signe de Rademacher indépendant, puis on recalcule Δ̄ pondéré. On fait 100 000 tirages ; la p-valeur bilatérale vaut (1 + #{|Δ̄*| ≥ |Δ̄|})/(1 + B).
- C'est un bootstrap sauvage de Rademacher. Il est exact sous l'hypothèse de symétrie. Il est asymptotiquement valide sous la seule hypothèse de moyenne nulle, avec des scènes indépendantes et hétéroscédastiques (condition de Lindeberg sur les w_s Δ_s) : la variance de permutation Σ w_s^2 Δ_s^2 estime de façon consistante celle de Δ̄ (PO-E7).

**Par famille.** Même test sur 264 scènes. On publie aussi en robustesse le Wilcoxon (`zero_method="pratt"`), le test des signes, les victoires, défaites et égalités (|Δ| ≤ 10^−9), et la fraction de cellules à Δ moyen positif.

**Calibrage (porte EG3, label `eval_slow`).**
- 1 000 campagnes nulles × 2 000 retournements, sur la structure exacte du plan (4 224 scènes pondérées, et 264 pour la famille).
- Trois lois de Δ_s de moyenne nulle : (a) exponentielle centrée, asymétrique ; (b) t à 3 degrés de liberté, centrée ; (c) hétéroscédastique par cellule, avec σ_c ∈ [0,02 ; 0,4] et un signe d'asymétrie variant par famille.
- Exigence : taux de rejet à α = 0,05 ≤ 0,06, et ≥ 0,04 pour exclure l'excès de conservatisme, sur le global et sur la famille.
- Coût : ≈ 10^10 multiplications-additions par BLAS, soit moins de 2 minutes. Ce n'est pas un test unitaire.

### 6.3 Revendications confirmatoires (D24)

Toutes les comparaisons se font en ARI_s, sur la strate Σ* et les 16 familles.

| Revendication | Test | Comparaisons |
|---|---|---|
| **R1** (« bat HDBSCAN ») | IUT : chaque comparaison au seuil α = 0,05 (bilatérale, sens favorable exigé) ; p_R1 = max des p | H1 `tw_P` contre `hdb_dev` ; H1s contre `hdb_dev_sk` ; H2a contre `hdb_match` (alpha = 2) ; H2b contre `hdb_match_fill` ; H2c contre `hdb_lib` ; H2d contre `hdb_these_K1` |
| **ATT** (attribution à la géométrie) | un test | H3 `tw_P` contre `mr2_P*` |
| **R3** (oracle) | un test | H4 `tw_oracle_full` contre `hdb_oracle_full` (6 912 configurations) |
| **ARB** (meilleure hiérarchie) | IUT sur DP et BNF1 | `tw` contre `mr2` au même K = K_ref(P) (arbres complets) |

- **Holm** à α = 0,05 sur {p_R1, p_ATT, p_R3, p_ARB} (4 revendications).
- **Gardes de R1.** Pour chacun des six adversaires, test de permutation sur Δ F1_H et sur Δ AMI_s. Une perte significative (p < 0,05, sans correction : garde volontairement sévère) sur l'une ou l'autre bloque R1.

Tests secondaires, chacun avec sa propre correction de Holm :
- **S-fam** : par famille, H1, H2a, H3 et H4 ; Holm sur 16 familles.
- **S-petit** : H1 et H2a sur {500, 2000}.
- **S-pub** : H1 et H2a sur les suites publiques. Blocs séparés, publication descriptive, Wilcoxon cité avec sa p minimale atteignable (0,0078 sur 8 jeux).
- **S-nul** : non-infériorité (§6.6).
- **Diagnostics sans test** : `tw_P` contre `mr2_P`, `mr1_P*`, `hdb_match_a1`, `hdb_oracle_full_a2`, `hdb_oracle_wide`.

### 6.4 Puissance attendue

L14 : IC de ±0,026 sur 250 exécutions, donc un écart-type apparié de Δ d'environ 0,2.

| Portée | Scènes | Erreur type de Δ̄ | Effet détectable (puissance 0,9) |
|---|---:|---:|---|
| Global Σ* | 16 × 12 × 22 = 4 224 | ≈ 0,0031 (≈ 0,0034 avec l'inflation de pondération) | ≈ 0,013 à α_Holm = 0,05/4 |
| Par famille, Σ* | 264 | ≈ 0,012 | ≈ 0,055 à α = 0,05/16 |
| Par famille et par taille | 72 à 120 | ≈ 0,02 | descriptif |

δ_min = 0,02 est tenable globalement. Par famille, on ne revendique rien sous 0,055.

### 6.5 Agrégats publiés

Inchangé : chaque agrégat porte son filtre exact. Tables obligatoires : global Σ*, par taille, famille, bruit, niveau et cellule. On publie moyennes, IC de Δ, p brute, p de Holm ou d'IUT, victoires, défaites et égalités, refus, couverture moyenne et k̂ moyen.

### 6.6 Règle de décision « battre HDBSCAN »

La décision est un calcul (`decide.py`).

| Niveau | Conditions (toutes requises) | Phrase autorisée |
|---|---|---|
| **R0** | défaut | « Aucune revendication de supériorité sur HDBSCAN. » |
| **R1 — bat en moyenne** | (i) p_R1 rejetée par Holm et chacune des six comparaisons dans le sens favorable ; (ii) Δ̄ ≥ δ_min = 0,02 et borne basse de l'IC > 0 pour chacune ; (iii) Δ̄ > 0 contre `hdb_dev` séparément à 8k, 16k et 32k ; (iv) gardes F1_H et AMI_s sans perte significative ; (v) refus de `tw_P` ≤ 1 % sur Σ* ; (vi) portes EG vertes au commit PREREG | « Sur le banc v10 préenregistré <sha8> (16 familles, n = 8000/16000/32000, N scènes), <P> bat HDBSCAN (six adversaires, dont HDBSCAN réglé sur dev avec alpha ∈ {1, 2}) : Δ ≥ +x [a ; b] d'ARI_s contre le plus fort (<A>), p_IUT = … ; pertes par famille : <S-fam> ; fausse structure : <S-nul> ; refus : r/N ; E1 : Δ = … [..]. » |
| **R2 — domine** | R1, plus aucune famille en perte significative (S-fam) contre `hdb_dev` et `hdb_match`, plus la non-infériorité S-nul | « … sur chacune des 16 familles … » |
| **R3 — oracle** | H4 rejetée par Holm, sens favorable, Δ̄ ≥ 0,02 | « À grille de tête égale, l'oracle de la tour dépasse l'oracle HDBSCAN, alpha ∈ {1, 2} compris, de … » |
| **Attribution** | H3 rejetée par Holm, sens favorable | Sinon, formule obligatoire : « la même famille de têtes, réglée de la même façon sur l'atteignabilité mutuelle à alpha = 2, fait aussi bien (Δ = … [..]) : le gain vient de la tête, pas de la géométrie exacte ». Si H3 est significativement négative : « la géométrie de la tour dégrade cette tête ». |
| **Meilleure hiérarchie** | ARB rejetée par Holm, sens favorable sur DP et BNF1 | « À K = K_ref, la hiérarchie de la tour contient mieux les classes que HDBSCAN à alpha = 2 (DP, BNF1). » |
| **Suites publiques** | — | Descriptif seulement (§3.7) |

**Non-infériorité S-nul.** Deux bornes à tenir, calculées par bootstrap sur les scènes nulles :
- borne basse de l'IC de S_N1(`tw_P`) − S_N1(`hdb_dev`) ≥ −0,05 sur N1 ;
- borne basse de l'IC de S_one(`tw_P`) − S_one(`hdb_dev`) ≥ −0,05 sur N2 et N3.

Sous R1, un échec est cité dans la phrase ; sous R2, il bloque.

**Conséquence pour le produit** (règle issue de L14) :
- H3 non rejetée : la sortie de clustering par défaut peut être la tête sur `mr2`, moins chère, et la tour garde ses sorties topologiques exactes ;
- H3 significativement négative : la tour n'est pas utilisée pour le clustering ;
- ce choix de produit ne modifie aucune revendication.

---

## 7. Déroulement

### 7.1 Phases

| Phase | Entrée (porte) | Sortie | Étiquettes vues |
|---|---|---|---|
| E-a Harnais | — | Code `eval/` ; portes EG1 à EG6, EG11 à EG18, EG-F vertes ; EG15 (continuité v9) | fixtures |
| E-b Calibration | E-a | `FAMILIES_v2.toml`, EG-L, panneau `calib_check` | `calib`, `calib_check` |
| **E-c0 Coût** (v2) | E-b, T2 de la tour verte | Temps, mémoire et compteurs sur l'espace `cost` ; décision écrite (Kmax_run, répétitions 16k/32k, lieu d'exécution) | **aucune** : le lanceur ne lit pas les vérités de `cost` |
| E-c Développement | E-c0 et les portes de tête EG7 à EG10 sur `dev` | Reçus `dev` (E0 à E7) ; choix de P, `hdb_dev`, `hdb_dev_sk`, `mr2_P*` et `mr1_P*` | `dev` |
| E-d Préenregistrement | E-c | `PREREG_EVAL_V10_<date>.toml` et manifeste du test dans le même commit | aucune du test |
| E-e Test | commit E-d dans HEAD, hash vérifié | `results.csv.gz`, `DECISION.json`, phrase | `test`, une fois |
| E-f Publication | E-e | Reçu `receipts/eval_v10_<date>/` | — |

### 7.2 Choix de P, `hdb_dev`, `hdb_dev_sk`, `mr2_P*` et `mr1_P*` : une seule procédure

```text
J(config) = moyenne ponderee des ARI_s sur les scenes dev (poids §6.1, strate = toutes les tailles dev)
```

Contraintes (identiques pour toutes les méthodes réglées) :
- zéro refus sur `dev` ;
- F1_H moyen ≥ meilleur F1_H de la grille − 0,01, et AMI_s moyen ≥ meilleur AMI_s − 0,01 ;
- S_N1 ≥ S_N1(`hdb_lib`) − 0,05 et S_one ≥ S_one(`hdb_lib`) − 0,05.

On retient argmax J. Une égalité à 0,002 près se départage par la configuration la plus simple : K croissant, puis alpha = 1 (sources `mr`/`sk`), z = 1, `fill=none`, `eps=0`, `asc=F`, tête T1. Même code, même budget, même fonction : la seule différence entre ces cinq réglages est l'ensemble des sources admises.

### 7.3 Expériences de développement (exploratoires, reçus `dev`)

| id | Question | Comparaison |
|---|---|---|
| E0 | Le banc discrimine-t-il ? | Panneau de références × familles × niveaux |
| E1-dev | Gain géométrique ? | Même tête sur `tw`, `mr2` et `mr1`, K ∈ {1, 2, 3, 5}, z ∈ {1, `zhat`} ; contrôle K = 1 : `tw` = `mr2` |
| E2 | z | z ∈ {1, 2, 3, `zhat`} |
| E3 | K | fixe (1 à 10), tranches γ (T2) |
| E4 | Bruit | `none`, `b1.5`, `b2`, `b3`, `full` |
| E5 | Qualité d'arbre | DP et BNF1 : `tw` contre `mr2` et `mr1`, par K |
| E6 | Coûts | tour, tête, `mr`, sklearn, séparés (le cœur est mesuré en E-c0) |
| E7 | Têtes propres à la tour | T3 contre `mr2_P*` |

Aucune expérience de développement ne produit de revendication ; leurs chiffres portent le préfixe « dev ».

### 7.4 Préenregistrement (squelette)

```toml
[prereg]
id = "PREREG_EVAL_V10_2026MMDD"
commit_engine = "<sha>"; commit_head = "<sha>"; commit_eval = "<sha>"
binary_sha256 = { tower = "...", head = "...", mreach = "...", oracle_cli = "..." }
sklearn = "1.9.1"; sklearn_algorithm = "kd_tree"; numpy = "..."; scipy = "..."
sklearn_sources_sha256 = { tree_pyx = "3865db02...", hdbscan_py = "6f030ac6...", linkage_pyx = "544621e8..." }
families_toml_sha256 = "..."; plan_manifest_sha256 = "..."
gates_at_prereg = ["EG1", "...", "EG18", "EG-F", "EG-L"]   # seules portes autorisant une reexecution sur les memes graines
[primary]
head = "T1"; K = 2; K_ref = 2; mcs = "sqrt"; sel = "eom"; eps = "0"; asc = false; z = "zhat"; fill = "b2"
[hdb_dev]
source = "mr2"; head = "T1"; K = 2; mcs = "sqrt"; sel = "eom"; eps = "0"; asc = false; z = "zhat"; fill = "b2"
[hdb_dev_sk]
source = "sk2"; K = 3; mcs = "sqrt"; sel = "eom"; eps = "0"; asc = false
[witness]
mr2_Pstar = { head = "T1", K = 2, ... }; mr1_Pstar = { ... }
[variants]
list = [ ... ]
[grids]
Kgrid = [1,2,3,4,5,6,8,10]; alpha = [1,2]; mcs = [5,10,20,50,100,"sqrt"]; sel = ["eom","leaf"]
eps = ["0","q50","q90"]; asc = [false,true]; z = [1,"zhat"]; fill = ["none","b2","full"]
[decision]
alpha_test = 0.05; iut_R1 = ["H1","H1s","H2a","H2b","H2c","H2d"]; holm_claims = ["R1","ATT","R3","ARB"]
guards = ["F1_H","AMI_s"]; delta_min = 0.02; refusal_cap = 0.01
bootstrap = "mccarthy_snowden"; bootstrap_B = 10000; permutations = 100000; tie = 1e-9
strata_decisive = [8000,16000,32000]
[budgets]
timeout_s = {500=20, 2000=60, 8000=300, 16000=900, 32000=1800}
mem_gib = {500=4, 2000=4, 8000=8, 16000=16, 32000=24}
kmax_run = 10                      # toutes tailles, toutes grilles (D19)
```

### 7.5 Amendements après le test (D11)

- **Réexécution sur les mêmes graines.** Elle n'est permise que si les trois conditions suivantes tiennent :
  - l'échec vient d'une porte listée dans `gates_at_prereg` ;
  - cette porte a été exécutée **automatiquement** par la campagne de test ;
  - le correctif ne touche que le code jugé par cette porte.
  On publie alors les deux versions, et la revendication cite l'amendement.
- **Tout autre changement** impose un nouvel espace `test2`, un nouveau préenregistrement, et la publication des résultats `test` :
  - une fixture ou une porte ajoutée après le PREREG, même sans étiquette ;
  - un réglage, une tête ou une politique.
- Un résultat de test n'est jamais retiré. Le nombre de campagnes de test exécutées sur une version est publié.

---

## 8. Pièges connus et parades

| Piège | Symptôme | Parade |
|---|---|---|
| Niveaux définis par un réglage | « medium » à 0,73 mesuré 0,40 | D4 |
| Vérité à un seul niveau | Sous-amas comptés comme erreur | D15 ; ARB |
| Bruit compté comme une classe | +0,156 par remplissage | D1, D2 |
| Remplissage d'un seul côté | — | `hdb_match_fill` ; EG12 |
| Oracle à diagonale | Oracle battu par un réglage fixe | D6 ; mutant |
| K ↔ ms décalé | 0,04 d'écart | ms = K ; `hdb_these_K1` à part |
| **Échelle alpha oubliée** (v2) | Témoin à alpha = 1, alors que C∩X suit alpha = 2 : ARI des coupes 0,50 contre 0,98 à K = 2 | D5 à D7 ; mutant `alpha_one_only` |
| **alpha sans effet dans la voie brute de sklearn** (v2) | alpha = 1 et alpha = 2 donnent des étiquettes identiques (20/20) | D20 ; EG7c ; mutant `sklearn_brute` |
| **Témoin réglé pour la tour** (v2) | H3 penche mécaniquement vers la tour | D7 ; EG12 |
| **Grilles tronquées selon la taille** (v2) | H4 asymétrique ; P change de définition | D19 ; mutant `truncate_kmax` |
| **Famille géométriquement cassée** (v2) | F12 : âmes tangentes | D25 ; EG-F |
| **Multiplicités hors contrat** (v2) | core_K de sklearn ≠ celui de la tour | D10 ; EG18 |
| **Abstention récompensée** (v2) | « tout bruit » = 1 sur N2/N3 | D21 |
| Réglage sur l'évaluation | `leaf` et `asc` après les scores | D11 ; EG16 |
| **Amendement opportuniste** (v2) | Défauts cherchés seulement quand la tour perd | D11 ; `gates_at_prereg` |
| Graines partagées | Unité = 5 graines | D3 ; EG5 |
| Survivants silencieux | Refus écartés | D8 ; EG11 |
| z déclaré par famille | Filaments « d = 1 », alors que ẑ ≈ 3 | `zhat` commun |
| Forêt de Gabriel | 18 → 416 racines | EG10 |
| Condensation divergente | shells 0,41 au lieu de 1,00 | §2.5 ; EG8 |
| Plateaux et binarisation | 7/80 écarts | `mr` atomisé pour E1 ; écarts `sk` comptés (EG7) |
| Données 2D en z = 0 | Moteur non qualifié en coplanaire | Refus compté |
| Délais non déclarés | Tour tuée à 32k | D17 |
| Suites publiques surajustées | SIPU connues | D18 |
| `ru_maxrss` d'un pool | Maximum sur la vie de l'ouvrier | Fork par couple (§4.4) |
| **Leviers LiDAR réglés sur le test** (v2) | Biais de sélection cumulé | D23 ; EP4 |
| **Exactitude LiDAR par égalité entre bras** (v2) | Omissions partagées invisibles | D13 ; EP1 à EP3 ; mutants EP-M |

---

## 9. Harnais : code, structures, portes, fixtures, mutants

### 9.1 Implantation

Dans `morsehgp3D_v10/`. Python PEP 8 et unittest, sans `assert` de production (tout doit tenir sous `python3 -O`). C++20 en en-têtes seuls pour les outils natifs.

| Fichier | Lignes visées | Contenu |
|---|---:|---|
| `eval/families.py` | 480 | 19 générateurs plus `shells_rho` (performance), `generate(spec, seed)` |
| `eval/family_geometry.py` (v2) | 120 | Porte EG-F : échantillonnage dense des supports, séparation minimale |
| `eval/levels.py` | 220 | MAP, D, bissection, `FAMILIES_v2.toml` |
| `eval/quantize.py` | 140 | floor(x/h + 1/2) rationnel, retirage, sites, multiplicités, `row_site` |
| `eval/plan.py` | 170 | Espaces (dont `cost`), graines, manifeste, disjonction |
| `eval/public.py` | 200 | Manifeste public, lecteurs FCPS et SIPU |
| `eval/adversaries.py` | 300 | Enveloppes sklearn (kd_tree imposé, alpha explicite, version et schéma vérifiés), GMM, OPTICS, MAP |
| `eval/runner.py` | 360 | Serveur préchargé, fork par couple, `setrlimit`, délais, statuts, `rss_base` |
| `eval/metrics.py` | 380 | ARI_s, AMI_s (EMI groupée), F1_H, ARI_nc, AMI_nc, bruit, S_N1, S_one, DP, BNF1 |
| `eval/stats.py` | 300 | Poids, bootstrap McCarthy–Snowden, retournement de signe, Wilcoxon, IUT, Holm |
| `eval/decide.py` | 280 | Préenregistrement, revendications R0 à R3, ATT, ARB, phrase générée |
| `eval/compat_v9.py` (v2) | 120 | Sujet différentiel : importe `bench_datasets.generate` et `run_baselines` du commit v9 épinglé (chemin passé explicitement, jamais dans le chemin produit), graines 2026092800..04, points flottants non quantifiés |
| `eval/receipt.py`, `eval/replay.py` | 300 | Reçu, `SHA256SUMS`, rejeu |
| `eval/tests/test_*.py` | 1 500 | Portes EG |
| `src/witness/mreach_exact.hpp` | 300 | d_K pondéré exact, Borůvka entier (alpha ∈ {1, 2}), Prim dense de qualification, dendrogramme atomisé → `tree.bin` v2 |
| `bench/mhgp10_mreach_cli.cpp` | 100 | CLI du témoin |
| `bench/mhgp10_oracle_cli.cpp` | 260 | Grille de têtes sur un `tree.bin` et une vérité → argmax et étiquettes. Outil d'évaluation seulement, jamais sur le chemin produit |
| `perf/judge_absent.py` (v2) | 260 | Juge des clés absentes (§12.5), arithmétique rationnelle indépendante du moteur |
| `perf/prefix_check.py` (v2) | 80 | Cohérence de préfixe des empreintes (§12.5) |
| `perf/gonogo.py` (v2) | 120 | Porte go/no-go (§12.6), sortie écrite dans le reçu |

La tête (`src/cluster/`) appartient à CLUSTER. Porte : l'ARI_s calculée en C++ pour l'argmax égale celle de Python à 10^−12 près.

### 9.2 Structures de données

- `Scene` : sites, mult, rows, row_site, vérités, `SceneMeta`.
- `RunRecord` : les champs de §2.1, en JSON canonique ; `params_sha8`.
- `ResultRow` : colonnes de §3.9 ; clé primaire (campaign, method, scene_id).
- `Prereg` : TOML (§7.4), lecture stricte.
- `Decision` : pour chaque revendication et chaque comparaison : Δ̄, IC, p, p_IUT, p_Holm, sens, niveau atteint, S-fam, phrase.
- `PerfRow` (v2) : schéma `mhgp10_run_v1` (§12.4).

### 9.3 Portes (CTest `eval_gate` ; unittest ; codes 0/1/2/3/4)

| Porte | Contenu | Plancher anti-vacuité |
|---|---|---|
| EG1 | Métriques contre force brute, fixtures nommées | ≥ 500 cas, dont ≥ 100 avec bruit des deux côtés ; AMI_s contre sklearn sur étiquettes « singletonisées », écart ≤ 10^−10 |
| EG2 | DP et BNF1 contre énumération de paires | ≥ 300 arbres, dont ≥ 50 avec multifusions, ≥ 30 avec poids |
| EG3 | Calibrage sous des nuls de moyenne nulle, asymétriques et hétéroscédastiques (§6.2) ; fixtures Holm et IUT ; déterminisme du bootstrap ; McCarthy–Snowden contre la formule s^2/R | 1 000 campagnes × 3 lois ; label `eval_slow` |
| EG4 | `decide.py` sur 10 tables fixtures : R0, R1, R2, R3, ATT négative, blocage par refus, par F1_H, par AMI_s, un adversaire IUT perdu, incohérence de taille | 10/10 |
| EG5 | Espaces disjoints, une ligne par couple, regénération de 1 % | 100 % pour la disjonction |
| EG6 | Quantification : floor(x/h + 1/2), borne h/2, retirage, 0 doublon | ≥ 1 collision forcée ; ≥ 1 demi-entier exact de chaque signe |
| EG7 | (a) tête sur `sk1` et `sk2` aux configurations standard = étiquettes sklearn ; (b) `mr_α` contre `sk_α` : écarts seulement aux plateaux, comptés | 200 scènes `dev` × 12 configurations × 2 alphas |
| EG7c (v2) | Fixture : sklearn `brute` à alpha = 2 = `brute` à alpha = 1 (étiquettes) et ≠ `kd_tree` à alpha = 2 ; le lanceur refuse `algorithm ≠ kd_tree` (code 2) | 20 nuages |
| EG8 (v2) | (a) `tree.bin` de `tw` à K = 1 = `tree.bin` de `mr2` à K = 1 : structure, rangs et poids identiques. (b) Tête sur `tw` à K = 1 et z = 1 = sklearn HDBSCAN(ms = 1, alpha = 2, kd_tree) sur les scènes sans égalité de niveau dans l'arbre couvrant ; sinon, écarts comptés et restreints aux plateaux. (c) Mutant `condense_no_big_child` tué | 20 scènes `dev` à 2k et 8k, dont shells et filaments ; ≥ 10 scènes sans plateau pour (b) |
| EG9 | Niveaux d'entrée égaux entre `tw`, `mr1` et `mr2` au même K, en entiers d_K(x)^2 (PO-E4) | tous les sites de 20 scènes, K ∈ {1, 2, 5, 10} |
| EG10 | Une racine par K ≤ Kmax_run sur chaque scène évaluée ; fixtures E5 (5 points) et L11 (4 points, K = 2) | 100 % des scènes |
| EG11 | Survivants : code 2 s'il manque une ligne ; un plantage produit une ligne `crash` | fixture |
| EG12 | Symétrie des paramètres, avec quatre règles. (1) `hdb_match` : ms = K_ref(P), alpha = 2, mcs, sel, eps et asc de P. (2) Grilles de tous les oracles et réglages : même Kgrid (Kmax_run), mêmes axes de tête. (3) Paramètres de `mr2_P*` = ceux écrits dans `PREREG.witness`, et ≠ P par construction s'ils sont réglés séparément. (4) Remplissage identique dans chaque couple | 100 % des couples |
| EG13 (v2) | Les étiquettes de l'argmax de chaque oracle, rejouées par l'API publique (appel sklearn pour `sk`, CLI de tête pour `tw` et `mr`), redonnent les étiquettes et l'ARI_s de l'oracle | 100 % des scènes, sur un échantillon de 2 % de rejeu effectif |
| EG14 (v2) | Déterminisme : étiquettes identiques à W = 1 et W = 8, invariantes par permutation des lignes (au renommage près), pour `tw`, `mr1` et `mr2` seulement ; départage par coordonnées | 50 scènes, 3 permutations |
| EG15 (v2) | Continuité, via `compat_v9.py` : sur le plan v9 (points flottants v9, graines v9), le harnais redonne les ARI_nc sklearn publiées par L09 et L14 (0,527 / 0,683 / 0,738), scène par scène, à 10^−12 près | 250 exécutions |
| EG16 | Pas de fuite : sans PREREG commité, le lanceur refuse les vérités `test` ; les vérités `cost` ne sont jamais lues (code 2) | fixture |
| EG17 (v2) | Témoin : Borůvka = Prim dense (même multiensemble de niveaux, même dendrogramme atomisé) | 200 scènes ≤ 8k × K ∈ {1, 2, 5, 10} × alpha |
| EG18 (v2) | Multiplicités : sur des scènes à doublons, core_K de `tw` et de `mr` (sites pondérés) = core_K de sklearn (lignes) ; étiquettes diffusées cohérentes ; mcs en lignes | 20 scènes, multiplicités 2 à 12 ; ≥ 1 site de multiplicité ≥ K |
| EG-F (v2) | Géométrie des supports (§3.1) | 12 familles G × 4 niveaux |
| EG-L | Gel des niveaux | 64 cellules × 20 graines |

### 9.4 Fixtures gravées

Celles de v1 (ARI_s 4/7, bruit absorbé, `hierarchical2`, F1_H, DP binaire contre multifusion, condensation à 8 facettes de L11, plateau à trois cofaces de L08, refus factices, tables de décision), plus :
- **K = 1** : 6 points, avec `tw` = `mr2` = sklearn(ms = 1, alpha = 2) ; les niveaux attendus sont écrits ((d/2)^2 exacts) ;
- **alpha** : 7 points où `mr1` et `mr2` diffèrent en partition à K = 2 ; les deux partitions attendues sont écrites ;
- **condensation « deux petits »** : scission de deux enfants < mcs, après laquelle le cluster se termine ; stabilité attendue écrite ;
- **multiplicités** : 5 sites, multiplicités (3, 1, 1, 2, 1), K = 3 : core_K attendus, et site de multiplicité 3 à λ = +∞ ;
- **S_N1 et S_one** : abstention, un groupe, deux moitiés, sur N1 et N3 ;
- **chainlink** : les âmes à d = 4/3 sont à ≥ 2/3, et le mutant d = 1 échoue à EG-F.

### 9.5 Mutants à tuer (`--inject=<nom>`, code 4 attendu)

| Mutant | Tué par |
|---|---|
| `ari_noise_class` | EG1 (fixture 4/7) |
| `ami_noise_class` (v2) | EG1 |
| `drop_refusals` | EG11 |
| `oracle_diagonal` | EG13 et une fixture d'argmax hors diagonale |
| `seed_shared` | EG5 |
| `fill_one_arm` | EG12 |
| `match_k_plus_one` | EG12 |
| `alpha_one_only` (v2) : `hdb_match` et oracles à alpha = 1 seul | EG12 |
| `witness_uses_P` (v2) : témoin à paramètres de P au lieu de P* | EG12 |
| `truncate_kmax` (v2) : Kgrid ≤ 6 à 16k/32k pour la tour seule | EG12 |
| `sklearn_brute` (v2) | EG7c |
| `mult_dropped` (v2) : sklearn reçoit les sites au lieu des lignes | EG18 |
| `bootstrap_R_draws` (v2) : R_c tirages au lieu de R_c − 1 | EG3 |
| `weights_by_scene` | EG4 |
| `holm_off`, `iut_as_mean` (v2) | EG4 |
| `condense_no_big_child` | EG8 (c) |
| `root_per_forest` | EG10 |
| `chainlink_d1` (v2) | EG-F |

---

## 10. Reçus et rejouabilité

Inchangé depuis v1 : dossier immuable `receipts/eval_v10_<campagne>_<date>/` contenant README généré, `PREREG.toml`, `PLAN.json`, `FAMILIES_v2.toml`, `results.csv.gz`, `DECISION.json`, `tables/*.csv`, `host.json`, `command.txt`, `binaries.sha256`, `SHA256SUMS` et `replay.py`.
- ≈ 8 438 scènes × ≈ 30 méthodes ≈ 253 000 lignes, environ 10 Mo compressés.
- Aucun chemin /tmp ; rejeu depuis un clone neuf.
- Contrôle CI : aucune empreinte de données tierces dans le dépôt.
- Ajout v2 : `COST.json` (phase E-c0), avec la décision de budget et sa raison.

---

## 11. Coûts attendus et parallélisme

### 11.1 Campagne de test (Kmax_run = 10 partout)

Hypothèses :
- `cble` mesure 6 à 10 µs par boule sur les synthétiques (L13, mur × fils) ; on retient 7 µs de générateur plus ≤ 3 µs de tour (ARCH, TOWER), soit 10 µs par boule ;
- boules par site à K10 : ≈ 420 en volumique (L13 : uniforme 421,6 ; amas 369,6), ≈ 120 à 130 en surface (plans 127,6), soit ≈ 330 en moyenne pondérée sur les 19 familles et le bruit.

Ces hypothèses sont remplacées par E-c0 avant le préenregistrement.

| Poste (plan de test) | Sites | Coût unitaire | Total CPU |
|---|---:|---|---:|
| Tour, n ≤ 8k | 20,9 M | 330 × 10 µs par site | ≈ 19 h |
| Tour, 16k | 19,1 M | idem | ≈ 17,5 h |
| Tour, 32k | 38,2 M | idem | ≈ 35 h |
| Arbres `mr` (8 K × 2 alphas par scène, Borůvka) | 78,8 M | ≈ 0,2 s par arbre à 32k | ≈ 3 h |
| sklearn : 16 ajustements par scène (`sk1`, `sk2`) + 6 adversaires nommés | 8 438 scènes | 32k : 5,5 s × 22 ; 16k : ≈ 2 s × 22 ; 8k : 0,51 s × 22 | ≈ 60 h |
| Grilles de têtes (condensations par (source, K, mcs, z) ≈ 480 par scène ; sélections, remplissages, métriques ≈ 17 000 configurations) | 8 438 | 32k : ≈ 30 s par scène | ≈ 20 h |
| Métriques d'arbre, statistiques, EG3 | — | — | ≈ 1 h |
| **Total** | | | **≈ 155 h CPU** |

Exécution :
- sur G4 en CPU seul (24 cœurs, 48 fils, ≈ 30 équivalents-cœurs), pool de processus, W = 8 pour la tour et W = 1 pour le reste ;
- concurrence bornée par la mémoire déclarée (185 Go ; à 810 o par boule, 16 tours 32k K10 simultanées) ;
- environ 5 à 6 h de mur, en **deux sessions gardées** (n ≤ 8k, puis 16k et 32k), `maxRunDuration` ≤ 4 h chacune, reprenables ligne par ligne ;
- les temps de cette campagne ne sont pas des résultats de performance.

### 11.2 Développement (v2, chiffré)

- **Passe `dev` complète.** 15,8 M sites : tour ≈ 14,5 h, sklearn ≈ 12 h, têtes ≈ 4 h, soit **≈ 30 h CPU**. Sur l'hôte local (8 CPU partagés, charge 5 à 13), cela représente plusieurs jours de mur.
- **Cache.** Les arbres (`tree.bin` v2 et arbres sklearn) sont mis en cache par (commit moteur, scène, K, source) dans `build/eval_cache/`, jamais versionné, compressé en zstd (≈ 2 à 3 Go pour tout `dev`). Une itération de tête ne recalcule que les têtes : ≈ 4 h CPU, soit ≈ 1 h de mur local.
- **Lieu.** La passe tour et sklearn de `dev` à 16k/32k, ainsi qu'E-c0, tournent sur G4 CPU dans une session gardée d'environ 1 h 30. Les tailles ≤ 8k et les itérations de tête tournent en local. Si le disque local a moins de 20 Go libres, les itérations de tête tournent aussi sur G4.
- Une nouvelle version du moteur invalide le cache de la tour, pas celui de sklearn.

### 11.3 Objectifs de performance de la chaîne de clustering (hors décision de qualité)

- Tour, tête et sortie en ≤ 1 s à 8k et en ≤ 5 à 10 s à 32k, en CPU W8, à K ≤ 5.
- Tête seule ≤ 2 fois sklearn HDBSCAN au même n (L07-F2).

---

## 12. Protocole de performance LiDAR (juge principal de la complexité)

**La complexité se juge d'abord sur le régime LiDAR** (demande de l'utilisateur) :
- trames SemanticKITTI sans sol de 30 000 à 60 000 sites, grille de 1 mm sur 18 bits ;
- K5, puis K10 ;
- contrats de 1 s, puis de 100 ms, sur G4 g4-standard-48 (RTX PRO 6000 Blackwell, EPYC 9B45 24c/48t, 185 Go).

Les pentes synthétiques 8k/16k/32k (§12.8 d) sont un diagnostic secondaire de sensibilité à la sortie ; elles ne remplacent jamais une mesure sur trames.

### 12.1 Entrées (D23)

**Trois rôles de trames.**

| Rôle | Trames | Usage |
|---|---|---|
| Conception | 08/000000, 08/000100, 08/000200 (continuité R22) | Profilage, go/no-go, A/B |
| Dev | 20 trames : 2 par séquence s ∈ {00, 01, 02, 03, 04, 05, 06, 07, 09, 10}, en i′_{s,j} = floor(((j + u_s + 1/2)/3) · N_s), j ∈ {0, 1} | A/B de leviers, étalonnage G4, croissance cumulée (§12.8 c), piste qualité (§3.10) |
| Test | 30 trames : 3 par séquence, en i_{s,j} = floor(((j + u_s)/3) · N_s), j ∈ {0, 1, 2} | **Une seule mesure par version préenregistrée** |

- On a `u_s = int(sha256("ehgp-v10-perf|" + s).hexdigest(), 16) / 2^256`.
- Porte EP4 : écart ≥ 20 scans entre toute trame dev et toute trame test d'une même séquence. Par construction, l'écart vaut ≈ N_s/6, soit ≥ 45 scans pour la séquence 04 (N = 271).
- La liste est gelée dans `perf/FRAMES_v2.toml`, avec le sha256 de chaque `.bin` brut et du masque de sol.
- Aucun filtrage par taille : une trame hors [30k ; 60k] est gardée et signalée.

**Retrait du sol.** Patchwork++ à la révision épinglée par le pilote v8, paramètres publiés, état neuf par trame. Coût G mesuré à part (1 fil et W fils). Masque privé, avec son sha256.

**Quantification (D9).**
- Règle v8 exacte (`PRECISION_FLOAT32_ET_GRILLE_20260921.md`) : q = floor(x/pas + 1/2), pas = 1 mm en rationnel exact à partir du float32. Les demi-entiers vont vers le côté positif, négatifs compris.
- Translation entière commune de v8 : pas de translation par morceau.
- Domaine [0 ; 2^18) : un point hors domaine refuse la trame (compté), sans écrêtage.
- Doublons : sites avec multiplicité (aucun sur les trames sans sol de 08, GEN § 1.3).
- **Porte EP6 (continuité).** Sur les trois trames de conception, le sha256 de `sites.u32le` égale celui des entrées v9 de référence utilisées par R22 et par L13 (08/000200 : `full_full.u32le` du panneau `s4a_cpu_scene02_physical_panel_20260924`, sha256 `a4bbc86d…`, 45 845 sites ; nombre de boules K5 égal à celui de R22, 1 407 885). À défaut d'égalité, la différence de règle est publiée et la continuité avec R22 n'est pas revendiquée.

**Licence et transport.** Aucun octet KITTI versionné. `data/` est ignoré par Git. La CI refuse les empreintes du manifeste. Les données sont copiées en privé vers la VM pendant la session gardée et vérifiées par sha256 avant tout calcul.

### 12.2 Frontières du chronomètre

Fixées par écrit ; les changer est un amendement, jamais un gain.

| Mesure | Début | Fin | Contient | Rôle |
|---|---|---|---|---|
| **B0** `wall_cold_process` | `posix_spawn` | `waitpid` | Contexte CUDA, bassins, lecture, calcul, écriture NVMe | Publiée |
| **B1** `wall_resident_io` | Chemin transmis | Sortie fermée | B2 + E/S | Diagnostic |
| **B2** `wall_resident` | Sites en mémoire hôte | Tour FULL canonique en mémoire hôte | Transferts, allocations hors bassin, synchronisations, ordres 1..Kmax, parents, contributions, plateaux, verticales | **Contrat** |
| **B3** phases | — | — | Σ phases + `unattributed` = B2 | Drapeau si `unattributed` > 3 % |
| **B4** `device_busy` | — | — | Union des intervalles noyaux et copies | Occupation = B4/B2 |
| G | float32 brut | Masque de sol | Patchwork++ | Publié |
| Q | Sites sans sol en float32 | `sites.u32le` + `mult` | Quantification et déduplication | Publié |
| H | Tour en mémoire | Étiquettes à K_ref(P) | Tête P | Publié |
| **C** (v2) `wall_chain` | float32 brut | Étiquettes | G + Q + B2 + H, mesuré d'un bloc | **Contrat « chaîne »**, publié à côté |

Hors B2 et publiés : digest, juges, écriture. Bassin persistant déclaré par K ; sa réservation compte dans B0 (v9 : 39 ms à K5, 190 ms à K10).

### 12.3 Plan d'exécution

- **Bras** : `gpu` (48 fils hôtes) ; `cpu48` (même algorithme, W = 48, digests égaux) ; `cpu24` en diagnostic.
- **Ordres** : K5 (Kmax = 5) et K10 (Kmax = 10).
- **Mode résident** : un processus par (bras, K) ; blocs A-B-B-A ; par bloc, un tour de chauffe (publié, exclu) puis 3 tours mesurés, chacun dans une permutation différente, sans jamais la même trame deux fois de suite ; 6 passes mesurées par (trame, bras, K).
- **Mode froid** : 3 processus par (trame, bras, K), en ordre entrelacé.
- **Statistiques** : par trame, médiane et MAD ; entre trames, p50, **p95 des médianes** (contrat ; rang le plus proche, soit la 29e valeur sur 30) et maximum. On publie aussi le **p95 sur toutes les passes** (180 valeurs par bras et par K), qui porte la gigue intra-trame. B2, B0 et C sont publiés séparément.
- **Charge de l'hôte** : une session à la fois ; relevés `nvidia-smi -q -d CLOCK,TEMPERATURE,PERFORMANCE`, `/proc/loadavg` et gouverneur, avant et après.

### 12.4 Compteurs par phase (schéma `mhgp10_run_v1`, une ligne JSON par passe)

Inchangé depuis v1, avec quelques ajouts :

| Groupe | Champs |
|---|---|
| Entrée | `frame`, `role` (conception, dev, test), `raw_returns`, `ground_free_returns`, `sites`, `grid_merges`, `bbox` |
| Générateur | `leaves`, `sum_m`, `guard_checks`, `dominator_checks`, `pair_tests`, `triple_tests`, `quad_tests`, `predicate_filter_evals`, `predicate_exact_fallbacks`, `balls[q][p]`, `balls_total`, `extended_shells` |
| Catalogue | `dedup_collisions`, `sort_ms`, `bytes` |
| Tour, par K | `minima`, `junctions`, `descent_steps`, `meb_calls`, `uf_ops`, `full_nodes`, `contributions`, `plateaus`, `verticals` |
| Sortie | `out_bytes`, `ranks` |
| Appareil | `kernels`, `kernel_ms`, `h2d_bytes`, `d2h_bytes`, `device_peak_mb`, `pool_mb` |
| Hôte | `cpu_s` par phase (`CLOCK_THREAD_CPUTIME_ID` agrégé), `cpu_process_s` (`CLOCK_PROCESS_CPUTIME_ID`), `threads`, `rss_peak_mb`, `rss_base_mb` |
| Temps | `B0`, `B1`, `B2`, phases B3, `B4`, `G`, `Q`, `H`, `C` |
| Exactitude | `merkle_prefix[k]` pour k ≤ Kmax, `digest_ref`, `equal` |

Grandeurs dérivées publiées :
- débit en boules/s ;
- ns de mur par boule ;
- **µs CPU par boule** = `cpu_process_s`/`balls_total` ;
- occupation CPU et occupation de l'appareil ;
- octets par boule ;
- nœuds FULL par site.

### 12.5 Exactitude de chaque passe chronométrée (D13)

**Références par (trame, K)**, calculées une fois, hors chronomètre, par le backend `cpu_reference` en **mode certifié**. Les empreintes utilisées sont `tower_merkle_v10` (TOWER § 5.7) : niveaux réduits, `PointId`, aucune numérotation. `merkle_prefix(k0) = H(k0, H(racine_1), …, H(racine_k0))`, où H(racine_k) contient la forêt d'ordre k et ses verticales vers k − 1. Obligation PO-E15 à la charge de TOWER : aucune donnée dépendant de Kmax ou de `kcat` n'entre dans H(racine_k).

Le jugement de chaque (trame, K) comporte six contrôles.

1. **Référence certifiée `kcat` = Kmax + 2** (TOWER § 7.3, mode certifié) : catalogue énuméré jusqu'à la profondeur Kmax + 2, tour jusqu'à Kmax. Euler couvre alors les ordres ≤ Kmax, alors qu'en v9 il s'arrêtait à Kmax − 2 (L15-07). Si le moteur accepte Kmax = 12 en mode référence (hors tour publique), la référence K10 est la restriction à l'ordre ≤ 10 d'une tour K12 ; sinon, `kcat` = 12. La première forme juge aussi la logique propre à l'ordre Kmax de la tour ; la seconde ne juge que le catalogue. Le reçu déclare laquelle a tourné.

2. **Cohérence de préfixe (porte EP1).** Sur chaque trame, trois égalités :
   - `merkle_prefix(5)` de la passe K10 (`kcat` = 10) = `merkle_prefix(5)` de la passe K5 (`kcat` = 5) ;
   - `merkle_prefix(10)` de la passe K10 = celui de la référence K10 (`kcat` = 12) ;
   - `merkle_prefix(5)` de la passe K5 = celui de la référence K5 (`kcat` = 7).
   L'ordre 5 est intérieur à la fenêtre K10 : la zone aveugle de fin de fenêtre de K5 (q2 à p = 4, q3 à p = 3) y est générée par la voie intérieure. C'est un invariant global à coût de deux exécutions, pas une vérification exhaustive.

3. **Juge d'échantillon du catalogue émis.** Un oracle exact indépendant (`perf/judge_absent.py`, rationnels Python) teste une boule sur 1 024 : centre, intérieur I, coquille U, support canonique S*, admission (GEN § 1.2 et § 1.3).

4. **Juge des clés absentes, à plancher explicite (porte EP3).**
   - **Strates de fin de fenêtre** du catalogue public (`kcat` = Kmax) et de la référence : q2 à p = Kmax − 1, q3 à p = Kmax − 2, q4 à p = Kmax − 3 (admission p + q_min ≤ Kmax + 1, cas d'égalité).
   - **Cadre de tirage déclaré** : x uniforme parmi les sites ; les autres sommets du tuple T (1, 2 ou 3 sites) uniformes et distincts parmi les **2·Kmax plus proches voisins** de x.
   - **Pour chaque candidat**, en arithmétique exacte indépendante du moteur : centre circonscrit dans aff(T), test c ∈ relint conv(T), I et U par requête de boule entière, S*(B), p, admission et appartenance à la strate. Un candidat est une **clé vraie** s'il appartient à la strate. On cherche S*(B) dans le vidage des clés du catalogue (hors chrono).
   - **Plancher** : m = 29 956 clés vraies par (K, strate), sur les 30 trames de test cumulées, avec ≥ 1 000 par trame. Si le taux d'omission dans le cadre est ≥ 10^−4, la probabilité de ne rien voir est (1 − 10^−4)^m ≤ 0,05 (PO-E16).
   - **Couverture du cadre publiée** : fraction des boules de la strate du catalogue dont le support canonique tombe dans le cadre, estimée sur 10^4 boules tirées du catalogue. Hors du cadre, la cohérence de préfixe (contrôles 1 et 2) reste le filet.
   - Coût : quelques 10^5 candidats par (K, strate), requêtes kd-tree entières. Quelques minutes par campagne en Python, hors chronomètre.

5. **Invariants globaux de la tour** : une racine par K ; naturalité des verticales sur un échantillon ; Euler en mode certifié.

6. **Juge K = 1** contre l'EMST exact (kd-tree entier), comme en v1.

**Chaque passe chronométrée.**
- Son `merkle_prefix(Kmax)`, calculé hors B2, doit égaler celui de la référence certifiée.
- Une passe à digest différent est invalide, et la trame échoue au contrat. Une nouvelle passe n'est jamais relancée en silence. Le nombre de passes invalides est publié.
- Une omission détectée (contrôles 1, 2 ou 4) invalide la référence de la trame. Elle devient une **fixture minimale permanente** (règle du dépôt), avec mise à jour de `STATUT_PREUVES_ET_HEURISTIQUES.md` avant de continuer.

**Mutants causaux** (sur les trames de conception et une scène synthétique à 8k ; code 4 attendu) :
- **EP-M1** `drop_window_end` : le générateur omet toute la strate q2 à p = Kmax − 1. Il est tué par EP1, car le préfixe diffère.
- **EP-M2** `drop_window_end_sparse` : il omet une boule sur 1 000 des strates de fin de fenêtre. Il est tué par EP3 (taux 10^−3 : probabilité de non-détection ≈ e^−30).

### 12.6 Contrats, budgets et porte go/no-go (D22)

**Contrat C(K, T).** Il est satisfait si, sur les 30 trames de test :
- le p95 des médianes B2 est ≤ T ;
- le maximum des médianes B2 est ≤ 1,2 T ;
- 100 % des passes ont un digest égal à la référence certifiée ;
- les contrôles 1 à 6 du §12.5 sont verts ;
- aucune trame n'est refusée.

La phrase publie dans la même ligne le p95 sur toutes les passes, B0 (p50, p95) et le contrat « chaîne » C (p95 des médianes, tête P comprise). Si le p95 de B0 dépasse 2 T, elle porte « processus froid : x s ».

**Budgets.**
- Boules par site : ≈ 33 à K5 et ≈ 138 à K10 (R22 : 30,7 à 32,8 et 119,6 à 138,2). Pire trame de 60k sites : ≈ 2,0 M boules à K5, ≈ 8,3 M à K10.
- Budget CPU par boule = T · W_eff/boules, avec W_eff = 24 cœurs physiques. Entre crochets : W_eff = 30 (gain SMT supposé de 25 %).

| Contrat | Boules au pire | Débit requis | Mur par boule | **CPU par boule et par cœur, tout compris** | Référence v9 (R22 ; chaîne / processus) |
|---|---:|---:|---:|---:|---|
| C(K5, 1 s) | 2,0 M | ≥ 2,0 M/s | ≤ 500 ns | ≤ 12,1 µs [15,2] | 0,76–0,98 s / 1,52–1,87 s |
| C(K10, 1 s) | 8,3 M | ≥ 8,3 M/s | ≤ 120 ns | ≤ 2,9 µs [3,6] | 2,27–2,96 s / 4,78–6,03 s |
| C(K5, 100 ms) | 2,0 M | ≥ 20 M/s | ≤ 50 ns | ≤ 1,2 µs [1,5] | hors d'atteinte connue (L07-F9) |
| C(K10, 100 ms) | 8,3 M | ≥ 83 M/s | ≤ 12 ns | ≤ 0,29 µs [0,36] | idem |

**Mesures disponibles** (générateur seul, prototype `cble`, hôte partagé, 2 fils, `nice 19`).

| Entrée | K | Boules | Temps utilisateur | µs CPU par boule (utilisateur) | µs par boule (mur × fils) |
|---|---:|---:|---:|---:|---:|
| LiDAR 08/000200 (45 845 sites), dom3 M16 | 5 | 1 407 885 | 20,53 s | **14,6** | 22,5 |
| LiDAR 08/000200, M16 sans dom3 | 5 | 1 407 885 | — | — | 31,3 |
| LiDAR 08/000200, dom3 M24 | 10 | 5 483 320 | — | — | 17,7 |
| Uniforme, amas, plans, 32k | 5 et 10 | 0,9 à 13,5 M | — | — | 6,1 à 9,4 |

Le rapport LiDAR/synthétique vaut 2 à 4. Le mur × fils surestime le CPU sur un hôte partagé ; le temps utilisateur fait foi.

**Projection (sans la tour, efficacité parallèle parfaite, κ = 1).**
- C(K5, 1 s) : 2,0 M × 14,6 µs/24 = **1,2 s pour le seul générateur** à 60k sites (0,86 s à 40k). **Non tenu en CPU** sans un gain ≥ 1,5 à 2× sur le générateur **et** une tour ≤ 2 µs par boule, ou sans la voie GPU.
- C(K10, 1 s) : 8,3 M × 14,6 à 17,7 µs/24 = **5,0 à 6,1 s**. Il faut ≥ 5 à 6× sur le générateur, tour comprise, ou la voie GPU.
- Les contrats à 100 ms exigent un facteur 10 de plus : aucun algorithme connu ne les tient.

**Porte go/no-go (EP5, écrite dans le reçu avant toute session de contrat).**

```text
c_ball      = mediane sur les trames conception + dev de cpu_process_s(B2 complet) / balls_total   [mesure locale, W=8]
kappa       = t_G4 / t_local par fil, meme binaire, W=1, trames de conception       [1,0 tant que non mesure]
eta         = efficacite parallele a W=48 sur G4 (B2(W=1) / (48 B2(W=48)))         [0,7 tant que non mesure ; eta_8 local publie]
W_eff       = 24
t_fixed     = transferts + allocations hors bassin (bras gpu : mesure en etalonnage ; cpu : 0)
B2_pred(f)  = balls(f) * c_ball * kappa / (W_eff * eta) + t_fixed
GO(C(K,T))  <=> p95 sur les trames dev de B2_pred, extrapole a 60k sites par la pente beta (§12.8 a), <= 0,8 T
```

- **Bras `gpu`** : il n'a pas de mesure locale. Il faut un B2 mesuré en **session d'étalonnage** G4 sur les trames de conception et dev, extrapolé de la même façon, ≤ 0,8 T.
- **Sessions d'étalonnage.** Elles ne portent que sur les trames de conception et dev. Elles fixent κ et η, et mesurent le bras `gpu`. Elles ne revendiquent rien.
- **Sessions de contrat.** Elles tournent sur les trames de test, **seulement** avec un GO écrit.
- Un NO-GO se publie tel quel, avec les chiffres qui le fondent.

Mémoire : ≤ 32 o par boule côté hôte, objectif non contractuel. Pic de l'appareil publié.

### 12.7 A/B d'un levier (D23)

- Deux bras A et B, en blocs A-B-B-A, sur les **3 trames de conception et les 20 trames dev**, soit 23 trames. **Jamais sur les trames de test.**
- Δ_t = médiane_B(t) − médiane_A(t), en relatif ; Wilcoxon sur 23 trames ; IC bootstrap de la médiane des Δ_t.
- Adoption si les digests sont égaux, si la borne haute de l'IC du Δ relatif est < 0, et si le gain médian est ≥ 2 %.
- Sinon, le levier est neutre ou négatif, et consigné dans `docs/FAUSSES_PISTES.md` avec son reçu.
- On ne publie jamais une somme de médianes par K comme un gain.
- Un levier adopté devient le chemin par défaut, sans option produit.
- **Mesure du test par version.** Une version gelée (`perf/PREREG_PERF_<version>.toml` : sha du binaire, trames, plan, GO) est mesurée **une fois** sur les trames de test. Toutes les versions mesurées sont publiées, avec leur nombre. La revendication cite la dernière et ce nombre.

### 12.8 Croissance et sensibilité à la sortie

(a) **Entre trames.** Régression B2 = α + β · `balls_total` par K et par bras (moindres carrés et Theil–Sen), sur les trames dev pour la projection du go/no-go, puis sur les trames de test après le contrat. On publie β (ns par boule) et α (ms fixes).

(b) **Selon K.** Balayage Kmax = 1..10 sur les trames de conception : boules(K), B2(K), compteurs(K), coût marginal par ordre. Drapeau si le travail par boule croît de plus de 1,3 fois de K5 à K10 (L04-F4).

(c) **LiDAR cumulé** (régime d'intérêt pour la croissance). On cumule 1, 2, 4 et 8 trames sans sol consécutives de 08 et d'une séquence **dev**, recalées par les poses, dédoublonnées à 1 mm : de ≈ 40k à ≈ 320k sites. Compteurs en local, temps sur G4 (optionnel). Pentes par doublement : p_travail (maximum sur les compteurs de travail déclarés de §12.4) et p_boules. **Porte : p_travail − p_boules ≤ 0,15.**

(d) **Synthétiques 8k/16k/32k** (diagnostic secondaire). Les 19 familles plus `shells_rho` (D26), espace `perf`, 3 répétitions, K5 et K10. Compteurs en local ; temps sur G4 (bras `cpu48`).
- **Seule porte d'algorithme** : p_travail − p_boules ≤ 0,15 par famille et par doublement.
- p_boules est publié comme **propriété des données**. À géométrie fixe, la densité croît avec n et le nombre de boules par site peut croître ; c'est le cas de `shells` (R0 fixe).
- `shells_rho` (densité constante, étendue croissante) est la famille qui ressemble le plus au LiDAR, et c'est l'échec cubique de v9 : on y attend p_boules ≈ 1.

Les temps locaux ne sont jamais publiés comme résultats de performance : en local, seuls les compteurs et les exposants font foi.

### 12.9 Sessions G4 (voie gardée ; une seule session SPOT utile à la fois)

**Préconditions locales** :
- portes CPU vertes ;
- `perf/FRAMES_v2.toml` et le plan commités ;
- autotests des scripts v10 ;
- compilation CUDA locale (compilation seule) ;
- configuration CMake en version 3.22 (échec S1 de v9) ;
- EP5 écrit pour une session de contrat.

**Paquet** : construit depuis un commit, worktree propre ; binaires construits sur la VM et capturés dans le reçu (sha256).

**Cycle de vie** :
1. `gcp-migration/start_and_verify.sh` : cible fixe, label `project=e-hgp`, `maxRunDuration` ≤ 4 h, double coupe-circuit ;
2. worker ;
3. rapatriement ;
4. `stop_and_verify.sh` ;
5. certification `TERMINATED` sur exactement cette cible, écrite dans le reçu.

**Worker** :
1. vérification sha256 des données ;
2. références certifiées et juges (§12.5) ;
3. plan chronométré (§12.3) ;
4. écriture ligne à ligne, avec la clé (trame, bras, K, bloc, passe).

**Reprise** : une préemption donne un reçu `incomplete`. On ne fusionne des lignes qu'à sha du binaire, type d'hôte et pilote identiques.

**Types et budgets de session** :

| Session | Contenu | Durée estimée |
|---|---|---|
| Étalonnage LiDAR | κ, η, bras `gpu` et `cpu48` sur conception + dev, balayage (b) | ≈ 1 h |
| Contrat LiDAR (par K) | Références certifiées (30 trames × 2 K × ≈ 30 s, soit ≈ 30 min), juge des clés absentes (≈ 10 min), résident (≈ 40 min), froid (≈ 25 min) | ≈ 1 h 50, en une ou deux sessions |
| Qualité `dev` (E-c0 + 16k/32k) | §11.2 | ≈ 1 h 30 |
| Qualité `test` | §11.1 | 2 × ≈ 3 h |

**Schéma** : `mhgp10_run_v1` gelé avant la première session ; au plus 3 versions sur toute la vie de v10.

### 12.10 Reçu de performance

`receipts/perf_lidar_v10_<date>/` contient :
- `FRAMES_v2.toml`, le plan et le PREREG_PERF ;
- les sha du paquet et des binaires ;
- `env.json` ;
- `gonogo.json` (EP5, ou étalonnage) ;
- `runs.jsonl` ;
- `references.json` : empreintes, préfixes, juges, couverture du cadre, clés vraies par strate ;
- `SUMMARY.json` et README générés ;
- le certificat `TERMINATED` ;
- `SHA256SUMS` et `replay.py`.

Aucun octet KITTI. Le README affiche d'abord B2 (p50, p95 des médianes, p95 de toutes les passes, max), puis B0 et C, puis les phases.

### 12.11 Voie GPU ultérieure, du point de vue de l'évaluation

- Un chemin GPU n'a pas de revendication de qualité propre. Il doit être égal en digest à `cpu_reference` sur un échantillon de 5 % du plan de qualité et sur 100 % des trames de performance.
- Un étage ne passe sur GPU que s'il dépasse ≈ 30 % du profil B3 du bras `cpu48` (L05). Le gain se mesure par A/B sur B2 (trames de conception et dev), jamais sur un noyau isolé.
- Au vu du §12.6, la voie GPU est la route réaliste vers C(K5, 1 s) et la seule vers C(K10, 1 s). Le go/no-go le dira sur mesure, pas sur déclaration.

---

## 13. Obligations de preuve

| id | Énoncé | Statut visé | Porte |
|---|---|---|---|
| PO-E1 | ARI_s en O(n) = ARI de sklearn sur étiquettes « singletonisées » : C(1, 2) = 0, donc les singletons n'entrent dans aucune somme, et N_p est inchangé | proved_here ; 200/200 cas | EG1 |
| PO-E2 | DP par fusion petite-dans-grande = définition par paires : #paires de c de plus bas ancêtre commun v = C(n_c(v), 2) − Σ_{u enfant de v} C(n_c(u), 2), multifusions et poids compris | proved_here | EG2 |
| PO-E3 | Tête rejouée sur `sk_α` en mode binarisé = `_condense_tree`, `_compute_stability`, `_do_labelling`, `epsilon_search` et `traverse_upwards` de sklearn 1.9.1 | contrôle différentiel | EG7 |
| PO-E4 | Au même K, niveau d'entrée C∩X de x = (distance au K-ième point, x compris)^2 = (distance-cœur sklearn à ms = K)^2 ; core_K est donc la même quantité pour toutes les sources | proved_here (définitions ; `hdbscan.py:355`) | EG9 |
| **PO-E5 (v2)** | À K = 1 : ρ ≡ 0 et `tw` = liaison simple aux niveaux (d/2)^2 = `mr2` (niveaux w2/4) exactement, plateaux atomisés compris ; donc tête(`tw`) = HDBSCAN(ms = 1, alpha = 2, kd_tree) aux plateaux près. `mr1` = `mr2` × 2 sur tous les niveaux : avec eps en quantile, étiquettes identiques pour toute tête de la grille, et stabilités multipliées par 2^z | proved_here : ρ(x) = 0 à K = 1 (le point compte), et L_1(r) ∩ X se connecte en x, y dès que \|x − y\| ≤ 2r | EG8 |
| **PO-E6 (v2)** | Entrelacement (rayons) : t = rayon de fusion C∩X de x et y ; m_α = rayon minimax de `mr_α`. Alors m2/2 ≤ t ≤ 2·m2 et m1/2 ≤ t ≤ (3/2)·m1 | proved_here (preuve ci-dessous) ; mesuré : t/m2 ≤ 1,36, m2/t ≤ 1,11, m1/t ≤ 1,95 (`interleave_check.log`) | — (interprétation d'E1) |
| PO-E7 (v2) | Le retournement de signe stratifié est exact sous symétrie de Δ_s, et asymptotiquement valide sous la seule moyenne nulle (scènes indépendantes, hétéroscédastiques, condition de Lindeberg), car c'est un bootstrap sauvage de Rademacher dont la variance conditionnelle Σ w_s^2 Δ_s^2 est consistante | argument ; simulation EG3 | EG3, EG5 |
| PO-E8 | Monotonie de la règle de refus (test de signe) ; effet sur la moyenne ≤ \|ARI_min\| × taux de refus | proved_here (partie signe) ; borne de l'ARI à confirmer au registre | EG11 |
| PO-E9 | ARI_MAP est un indicateur, pas une borne | déclaré | — |
| PO-E10 | Monotonie de θ ↦ statistique de niveau | empirique | EG-L |
| PO-E11 | Erreur de quantification ≤ h/2 par coordonnée, avec floor(x/h + 1/2) : \|x/h − q\| ≤ 1/2 | proved_here | EG6 |
| PO-E12 | Digest canonique de la tour indépendant de la représentation | à la charge de TOWER | §12.5 |
| **PO-E13 (v2)** | sklearn 1.9.1 : voie `brute` → la matrice est divisée par alpha avant `mutual_reachability_graph`, donc distance-cœur et paire sont toutes deux divisées : changement d'échelle pur. Voie `kd_tree` → `mst_from_data_matrix` ne divise que la paire (`_linkage.pyx:189`) | lecture de source épinglée ; fixture 20/20 | EG7c |
| **PO-E14 (v2)** | IUT : si chaque H_A est testée au seuil α, le rejet de l'intersection (« bat tous les A ») a un risque ≤ α sans correction | théorème externe (Berger 1982) | EG4 |
| **PO-E15 (v2)** | L'objet d'ordre k (π0 de L_k(a) et verticales k → k − 1) ne dépend que de k et du multiensemble X, pas de Kmax ni de `kcat`. Obligation d'implémentation pour TOWER : H(racine_k) n'encode que cet objet | proved_here (définition de L_k) ; obligation TOWER | EP1 |
| **PO-E16 (v2)** | Si le taux d'omission dans le cadre de tirage est ≥ ρ, alors avec m tirages indépendants de clés vraies, P(aucune omission vue) = (1 − ρ)^m ≤ e^{−ρm} ; m = 29 956 donne ≤ 0,05 à ρ = 10^−4 | proved_here (tirage avec remise ; sans remise, c'est plus fort) | EP3 |
| **PO-E17 (v2)** | Bootstrap de McCarthy–Snowden : m_c = R_c − 1 tirages avec remise dans la cellule c, donc Var*(moyenne) = σ̂_c^2/(R_c − 1) = s_c^2/R_c, avec σ̂_c^2 = (1/R_c) Σ (x − x̄)^2 | proved_here (algèbre) | EG3 |
| **PO-E18 (v2)** | Multiplicités : sur le multiensemble des lignes, la K-ième distance (x compris) est la même que celle du site pondéré ; `kneighbors` rend les doublons à distance 0, et la valeur au rang K ne dépend pas de leur ordre. Donc core_K(sklearn, lignes) = core_K(`tw`/`mr`, sites pondérés) | proved_here | EG18 |

**Preuve de PO-E6.** Notations : ρ(x) = d_K(x) ; C∩X connecte x et y au rayon t = inf{r : x et y dans la même composante de L_K(r)} ; w2(x, y) = max(ρ(x), ρ(y), |x − y|/2), w1(x, y) = max(ρ(x), ρ(y), |x − y|), et m_α est le minimax de w_α sur les chaînes de points.

*Majoration.* Soit d = |x − y| et z sur le segment [x, y], avec |z − x| ≤ d/2 (sinon, on échange x et y). Les K points les plus proches de x sont à ≤ ρ(x) de x, donc à ≤ ρ(x) + d/2 de z. Le segment est donc contenu dans L_K(r) pour r = max(ρ(x), ρ(y)) + d/2, et t(x, y) ≤ max(ρ(x), ρ(y)) + d/2. Or cette borne vaut au plus 2·w2(x, y) et au plus (3/2)·w1(x, y). Comme t est une ultramétrique (minimax sur les composantes), t ≤ 2·m2 et t ≤ (3/2)·m1.

*Minoration.* Soit γ un chemin de x à y dans L_K(r). Tout z de γ a au moins K points à distance ≤ r, donc son plus proche point p(z) est à ≤ r. On suit les cellules de Voronoï traversées par γ. Au passage de la cellule de p à celle de p′, au point z du bissecteur, on a |p − p′| ≤ |p − z| + |z − p′| ≤ 2r. De plus, ρ(p) ≤ |p − z| + r ≤ 2r. Chaque arête consécutive vérifie donc w2 ≤ max(2r, r) = 2r et w1 ≤ 2r. Donc m2 ≤ 2t et m1 ≤ 2t.

*Égalité à K = 1.* ρ ≡ 0 donne t(x, y) = minimax de |x − y|/2 = m2.

Ces bornes de pire cas ne départagent pas alpha = 1 et alpha = 2. L'égalité à K = 1 et les mesures (ratios ≤ 1,36 contre ≤ 1,95) fondent le choix de `mr2` comme témoin (D7). À inscrire au registre `STATUT_PREUVES_ET_HEURISTIQUES.md` (proved_here) avant le code d'E1.

---

## 14. Risques

1. **Coût de la tour à Kmax = 10 sur des milliers de scènes** (≈ 72 h CPU estimées pour la tour, ≈ 155 h au total). Parade : mesure E-c0 sans étiquettes, puis réduction symétrique préenregistrée avant le choix de P (répétitions 6 → 4 à 16k/32k, puis Kmax_run → 8).
2. **E1 sans gain géométrique.** C'est l'issue la plus probable, et elle l'est davantage maintenant que le témoin est à la bonne échelle : à K = 1, `tw` et `mr2` sont identiques, et à K ≥ 2 leurs coupes s'accordent à ARI 0,96 à 1,00 dans les sondes. Ce n'est pas un risque de protocole mais un résultat : la tour se justifie alors par ses sorties exactes, et le clustering passe par `mr2`.
3. **`hdb_dev` à alpha = 2 peut rendre R1 inaccessible.** Si le meilleur HDBSCAN réglé est `mr2` avec la même tête, la seule différence avec `tw_P` est la géométrie exacte, que les sondes montrent très proche. R1 est alors conditionné par H3. C'est voulu : « battre HDBSCAN » ne doit pas pouvoir venir d'un paramètre public de sklearn.
4. **Surajustement au développement.** Les familles sont les mêmes en `dev` et en `test`. La généralisation ne se lit que sur 8 jeux publics « jamais vus », avec une puissance symbolique.
5. **Domaine du moteur** (SIPU coplanaires en z = 0, cosphéricités de grille) : des refus peuvent bloquer R1 par le plafond de 1 %. C'est voulu.
6. **Attribut privé `_single_linkage_tree_`.** Parade : version épinglée, schéma vérifié ; les adversaires nommés passent par l'API publique.
7. **Contrat LiDAR probablement NO-GO en CPU** à K5 1 s et certainement à K10 1 s, au vu de `cble` (14,6 µs contre 12,1 µs de budget total). La voie GPU porte le contrat. Le risque est de ne publier que des NO-GO ; c'est un résultat honnête.
8. **Cadre du juge des clés absentes** (2·Kmax plus proches voisins) : il ne couvre pas toutes les boules de fin de fenêtre. Parade : couverture publiée, et cohérence de préfixe comme filet global.
9. **Tour K12 non disponible** en mode référence : la référence K10 ne juge alors que le catalogue à `kcat` = 12, pas la logique d'ordre 10 propre à la tour. Parade : EP1 (K5 ⊂ K10) juge cette logique à l'ordre 5. Résidu déclaré.
10. **Logistique KITTI** (10 séquences en privé, envoi vers la VM conforme à la licence) et **disponibilité de G4** (préemptions SPOT).
11. **Disque local** : cache `dev` de 2 à 3 Go, alors que le codespace a déjà été saturé (nettoyage du 23 sept.). Parade : têtes sur G4 si l'espace libre est < 20 Go.

---

## 15. Questions ouvertes pour l'utilisateur

1. **Adversaire décisif et alpha.** « Battre HDBSCAN » exige ici de battre six adversaires, dont HDBSCAN réglé sur `dev` avec alpha ∈ {1, 2} (D5). Or, à K = 1, la tour **est** HDBSCAN(ms = 1, alpha = 2). Confirmez-vous que la revendication doit survivre à ce paramètre public de sklearn, quitte à ce qu'elle devienne très difficile (risque 3) ?
2. **Politique de bruit visée par le produit** : abstention, remplissage borné ou couverture complète ? ARI_s reste neutre ; la question porte sur la politique de P.
3. **Données KITTI** : les séquences 00 à 10 (hors 08) sont-elles disponibles en privé pour les 20 trames dev et les 30 trames de test, et l'envoi vers la VM G4 est-il conforme à votre usage de la licence ?
4. **Coût** : acceptez-vous ≈ 155 h CPU pour la campagne de test (≈ 5 à 6 h de mur sur G4 en CPU, deux sessions gardées) et ≈ 30 h par passe `dev` complète (tour sur G4, têtes en local) ? Sinon, faut-il préenregistrer d'emblée la réduction symétrique (4 répétitions à 16k/32k) ?
5. **Contrat LiDAR** : la projection dit NO-GO en CPU pour C(K5, 1 s) à 60k sites. Voulez-vous que la première session G4 soit une **session d'étalonnage** (κ, η, bras `gpu` sur les trames de conception et dev), plutôt qu'une tentative de contrat ?
6. **Contrat de latence** : on garde B2 comme contrat, avec B0 et la chaîne C publiés à côté. Préférez-vous que la chaîne C (sol, quantification, tour, tête) soit le contrat principal ?
7. **Suites publiques de grande taille** (birch1, birch2, worms_2) et **piste qualité LiDAR** (§3.10, trames dev seulement) : les conserver ?
