# Critique de complétude de l'audit des transpositions v11

4 octobre 2026, rédigé de 14 h 17 à 14 h 23 UTC (heures lues par `date -u`). Critique de complétude du verdict
[`AUDIT_TRANSPOSITIONS_V11.md`](AUDIT_TRANSPOSITIONS_V11.md) (13 h 57) et des rapports qu'il juge ; contexte commun :
[`CONTEXTE.md`](CONTEXTE.md). Rappel de l'utilisateur : « l'objectif du contrat est toujours 100 ms ».

```text
phase=exploration_v11_hors_registre (critique, lecture seule)
backend=cpu_reference
profile=quantized_u21_input_only
public_status=not_claimed
GCP non utilisé ; aucune construction ni exécution native ; aucune commande git qui écrit
```

**Méthode.** Lus en entier : le verdict, les deux cartes, les deux plans. Pour les douze fouilles et les douze
contre-vérifications : en-têtes, sources, verdicts et passages cités. Inventaire par `grep` des reçus existants contre
les reçus cités. Lus en plus, hors du corpus de l'audit : les archives G4 v11 versées mais jamais exploitées
(`claudeab1`, `claudeab4`, `claudeprof1`), deux sessions G4 **non versées** de `/workspaces/.ehgp-sessions/`
(`claudecat1`, 3 octobre vers 19 h 45 UTC ; `claudeab8`, 4 octobre de 13 h 36 à 14 h 08 UTC), les reçus v10
`g4_session2_perf_20260929` et `g4_session5_scale_20260929`, `build/v10-persist/`, `build/v11-persist/perf_wip_20261003/`
et `diag1/`, les cinq commits v11 postérieurs au verdict. Les chiffres nouveaux sont recalculés par
[`critique/calculs_critique.py`](critique/calculs_critique.py) (bibliothèque standard, archives lues en mémoire, rien
d'écrit ; code 0 en normal et sous `-O` à 14 h 15 UTC), sauf mention.

Étiquettes : **M** mesuré (reçu G4 nommé, versé ou non), **E** estimé, **C** conjecturé, **L** lu dans le code ou un
document.

## 0. En bref

1. **Les temps W48 sont bimodaux à binaire identique** (P1) : le verdict y voit du bruit gaussien. L'écart entre les
   deux états (30 à 55 ms par étage) vaut les plus gros leviers V1–V4, ne change aucun octet de sortie, et sa cause
   n'est pas cherchée.
2. **Le pipeline allonge la résolution jusqu'à ×1,44** (P2) : le budget du verdict prend 116 ms pour un coût propre de
   la résolution ; sans pipeline, la même voie résout en 69–84 ms.
3. **Deux A/B G4 tranchent déjà des leviers et n'ont pas été lues** (P3) : `claudecat1` mesure un filtre G1 sans
   branche **plus lent** (préambule ×2,1, W1 +8 %) ; `claudeab8` mesure le q3 différé à −1,5 à −1,8 % de passe unique
   et le recensement par masques (tranche (a) de J3) **sans effet**.
4. **La cible « 54–77 ms au pire de la plage » suppose temps ∝ travail** (P4) ; les seuls temps mesurés sur les
   127 trames c08 donnent un exposant de 0,76 en boules et un pire cas ×1,60, non ×1,87.
5. **Personne ne décrit la machine** (P5) : trois domaines L3, appariement SMT inconnu (le W24 « un fil par cœur » de
   `diag1` est supposé), effet de session ×1,16 sur la même base, temps volé jamais relevé.
6. **Trois leviers manquent à toutes les listes** (P6) : pages de 2 Mio (un `madvise` existait dans une copie v10),
   PGO/LTO/compilateur, attente hybride du pipeline (correctif prêt).
7. **Le plan `diag1` (T0) a une faute d'ordre** (P7) : avec quatre variantes, l'ordre tournant de `bench/ab_g4.py`
   place toujours `qr` en position paire et son double A/A `qr2` en position impaire, défaut signalé par les auditeurs
   à 14 h 03 et non corrigé.
8. Angles morts : K = 10 (mémoire, partage des fils du pipeline), port natif des points (porte bornée à k ≤ 4), GPU
   (aucun outillage CUDA dans les scripts de session v11), décisions à poser à l'utilisateur sur ce que FULL doit
   rendre (P8–P11). Plusieurs « États » du verdict sont déjà périmés (§ 5).

## 1. Liste priorisée des manques

### P1 — Bimodalité des étages à binaire identique : ni vue, ni expliquée (parallélisme, SMT)

- **Constat (M).** La base `a45daff3a` est le **même exécutable** dans `claudeab1`, `claudeab4` et `claudeab7`
  (`builds.base.sha256 = 0089f43e…` dans les trois `ab_report.json`) : quinze prises W48 par trame, compteurs de
  travail identiques. Sa résolution régulière (voie par étages) vaut **soit 99,0–100,8 ms (neuf prises, ± 1 %), soit
  130,8–154,5 ms (six prises)** sur ng00 ; 72,7–74,3 contre 104,0–119,1 ms sur ng01 (six et neuf prises) ; 82,8–84,4
  contre 110,5–149,1 ms sur ng02 (neuf et six). Sa passe unique s'étale de 156,7 à 214,7 ms (ng00). Le moteur courant
  `b87285378` (dix prises : `claudeab7` bras new, `claudecat1` bras base, même binaire `f5513318…`) : FULL 369–464 /
  308–439 / 351–439 ms, passe unique 159–211 / 125–255 / 152–202 ms.
- **Lecture du verdict.** « Bruit » (σ du log des rapports 0,11–0,14, F2). La qualification c40 notait déjà
  « résolution ng00 100,3–157,7 ms entre trois prises » (CARTE_V11 § 2.3) ; personne n'y a lu deux états discrets.
- **Pourquoi c'est prioritaire.** (a) Supprimer l'état haut rendrait 30 à 55 ms par étage sans toucher au travail ni
  aux sorties ; aucune transposition retenue ne vaut davantage. (b) Un bruit à deux états ne se traite pas par un
  nombre de paires calculé sur un σ : quand un bras est bimodal et l'autre non (la résolution pipelinée est unimodale,
  115–127 ms), le test des signes mesure la fréquence de l'état haut, pas un gain. (c) L'état bas est serré à ± 1 % :
  une fois l'état identifié, W48 deviendrait presque déterministe et des leviers de 1–2 % y seraient décidables.
- **Cause : C, non mesurée.** vCPU retenu par l'hyperviseur sur une VM SPOT (temps volé) ; tâche traînarde d'un
  partage statique ; placement entre les trois domaines L3 (P5).
- **Où chercher.** Archives versées `morsehgp3D_v11/receipts/developpement_20261003/pipeline_g4/sessions/claudeab{1,4,7}/results.tar.gz`
  et sessions non versées `/workspaces/.ehgp-sessions/v11.20261003.claudecat1/`, `v11.20261004.claudeab8/` (≈ 120
  prises W48 déjà payées). À ajouter à chaque prise : temps volé (`/proc/stat`) avant et après, `single_task_max_ns`
  (calculé dans `src/catalogue/single_pass.cpp` l. 160–162, non publié), début et fin des tâches de la passe unique et
  des lanes de la voie par étages (`e49ea4690` ne couvre que les tâches du pipeline), nombre de `parallel_for` et
  d'attentes du Pool par FULL.

### P2 — Le pipeline coûte jusqu'à ×1,44 à la résolution : budget et leviers V2/V4 mal posés

- **Constat (M).** Voie liée **sans** pipeline ni naissances par blocs (`claudeab1`, bras new, 48 lanes, W48) :
  résolution 83,2 / 83,3 / 83,8 / 84,1 / 116,0 ms sur ng00, médianes 83,8 / 74,0 / 68,9 ms sur les trois trames.
  **Avec** pipeline (`claudeab4` et `claudeab7`, bras new, dix prises) : 115,2–126,9 / 85,1–93,5 / 96,4–105,8 ms,
  médianes 117,2 / 86,6 / 99,0 ms, soit ×1,40 / ×1,17 / ×1,44, plus que le ×1,23 du seul nombre de résolveurs (48/39,
  `pipeline_lanes`, `src/tower/forest_pipeline.cpp` l. 109–113). Le pipeline réduit pourtant les forêts de 194,9 /
  164,8 / 181,2 à 166,8 / 131,7 / 157,6 ms (médianes) en cachant publication et verticales.
- **Conséquence.** Le budget du verdict (§ 2.4 : « résolution 115,8 ms → ≤ 30 ») traite un coût de structure
  (≈ 30 ms sur ng00) comme un coût de résolution ; les gains V3 et V7 sont convertis en mur comme si la résolution
  pipelinée était le chemin critique. Dès que V4 rend la publication courte, « résolution sur 48 fils puis
  publication parallèle » peut battre le pipeline : le verdict ne compare jamais les deux. À K = 10, le partage 29/10/9
  [AUD] retire 40 % des fils à la résolution (≥ ×1,66, E) ; aucune estimation K = 10 n'en tient compte.
- **Où chercher.** Les trois archives ci-dessus ; `e49ea4690` (lanes : dernier début, première fin, CPU sommé) ;
  ajouter à T0 une ablation « pipeline coupé à W48 » sur le moteur courant (même source, sorties identiques).

### P3 — Deux A/B G4 qui tranchent des leviers, jamais lues

- **`claudecat1` (M, non versée).** « Filtre G1 binary64 exact sans branche et compteurs de feuille accumulés » contre
  `b87285378`, verdict du banc « conforme » : W1 FULL ×1,078 / ×1,089 / ×1,084, passe unique ×1,13–1,14, **préambule
  ×2,11–2,18** (85,6 → 180,9 ms sur ng00) ; W48 médianes 392,4 → 472,0 / 350,0 → 385,4 / 408,7 → 446,5 ms. C'est la
  seule mesure G4 d'un noyau G1 sans branchement : V6 (« −5 à −20 ms », C) est à réviser, au moins pour cette forme ;
  les deux leviers étant groupés, la part des compteurs n'est pas isolée, et V5 (compteurs, porté depuis en
  `0c358261c`) n'a toujours aucune mesure seule.
- **`claudeab8` (M, non versée, terminée à 14 h 08).** Base `17514012b`, q3 différé `56216392e`, lemme R `9b9244a00` :
  à W1, passe unique −1,8 / −1,6 / −1,5 % pour le q3 différé, **−0,2 à +0,4 % pour le lemme R** (la tranche (a) de J3,
  « 43 % des tests décidés par masque », ne rend rien de mesurable) ; à W48 les médianes des rapports appariés donnent
  q3/base 1,083 / 1,056 / 0,988, c'est-à-dire « plus lent » là où W1 dit « plus rapide ». Le gain V1 (−25 à −55 ms) reste
  entier à démontrer par les tranches (b) à (e). Le développeur tire la même lecture à 14 h 11 (`66372e621`).
- **Où.** `/workspaces/.ehgp-sessions/v11.20261003.claudecat1/results/extracted/`,
  `/workspaces/.ehgp-sessions/v11.20261004.claudeab8/results/extracted/` ; le correctif
  `build/v11-persist/perf_wip_20261003/leaf_counters_pipeline_spin.patch` garde les compteurs essayés. Verser
  `claudecat1` en reçu (règle « reçu obligatoire pour toute mesure ») : ces dossiers sont hors git.

### P4 — « Cible effective 54–77 ms » : proportionnalité temps/travail supposée, contredite par les temps existants

- **Constat.** Le verdict (§ 1 item 7, F4) tire la cible des nœuds d'ordre 5 (∝ n^1,39) en supposant le temps ∝ travail
  (E). Le même reçu `pts4_review_20261003` porte des **temps** (`full_ns`, K = 1..10, 4 fils par processus,
  22 processus simultanés, hors contrat, M) : `full_ns` ∝ boules^0,76 ∝ n^1,10 ; trames de 50–60 k sites contre la
  médiane des onze trames de 38–42 k : **×0,93 en médiane, ×1,60 au pire** (c08_003412), au lieu de ×1,3 et ×1,87
  (référence différente : le verdict rapporte à 08/000000, lourde pour sa taille). La v10 a mesuré sur G4 à 48 fils
  (`morsehgp3D_v10/receipts/g4_session5_scale_20260929/exposants.txt`) des exposants de mur quart → moitié → trame de
  0,48 à 1,09 pour le catalogue et de 0,79 à 1,37 pour la tour à K = 5 : à cette taille, le mur croît moins vite que
  le travail.
- **Conséquence.** La cible au pire de la plage n'est pas établie ; d'après PTS4 elle serait plutôt de 62 à 100 ms sur
  ng00 (E, fragile : contention, K = 10). À trancher par mesure, pas par compte.
- **Où.** `receipts/pts4_review_20261003/case_metadata.json.gz` (champ `full_ns`) ; reçu v10 `g4_session5_scale_20260929`
  (exposants, RSS, 21 secteurs LiDAR, jusqu'à un million de sites) ; `bench/full_timing.py` (`0cf20f98c`, tranches
  30–40 / 40–50 / 50–60 / 60 k et plus). Le plan `diag1` ne joue que ng00–ng02 : les trames lourdes manquent.

### P5 — La machine n'est pas décrite : trois L3, appariement SMT, THP, effet de session

- **Constat (M, L).** `env/lscpu.txt` des archives : 24 cœurs, 2 fils par cœur, **L3 de 96 Mio en 3 instances**, un
  seul nœud NUMA ; aucun rapport ne le dit (`grep` de CCD, L3, siblings : vide). L'appariement des fils logiques
  (`thread_siblings_list`, `lscpu -e`) n'est consigné nulle part, alors que `bench/full_timing.py` et `diag1` épinglent
  « un fil par cœur » par `taskset 0-23` (C : vrai seulement si les frères SMT sont i et i + 24). Mode THP non consigné
  (`AnonHugePages` 4 096 kio au départ, `env/meminfo.txt`). Toutes les sessions tournent sur la même instance
  `ehgp-v7-3b1d…` (us-central1-c) mais à des générations différentes : la même base donne des médianes FULL de 378,4 à
  439,7 ms sur ng01 selon la session (×1,16) ; le moteur `b87285378` 412,4 (`claudeab7`) ou 392,4 ms (`claudecat1`)
  sur ng00 ; la base `17514012b`, quasi le même moteur (correctifs P1/P2), 399,7 / 318,1 / 416,4 ms (`claudeab8`).
- **Conséquence.** Les « FULL attendus » des tranches T2 à T7 (§ 4.1 du verdict) partent d'une seule session (412 ms) ;
  W24 peut mesurer 12 cœurs × 2 au lieu de 24 × 1 ; la contention entre domaines L3 (table de populations de 42 Mio,
  arènes de 170–181 Mio, plus qu'un L3 de 32 Mio) n'est pas instruite.
- **Quoi.** Consigner une fois `lscpu -e`, `/sys/devices/system/cpu/cpu*/topology/thread_siblings_list`,
  `/sys/kernel/mm/transparent_hugepage/enabled` ; par prise, le temps volé ; calculer la liste W24 depuis l'appariement
  réel ; rejouer la base dans chaque session (le banc le fait) et ne jamais comparer des médianes de sessions distinctes.

### P6 — Trois leviers absents de toutes les listes

| Levier | Preuve et source | Doctrine | Gain |
| --- | --- | --- | --- |
| Pages de 2 Mio sur les grands tableaux (`madvise(MADV_HUGEPAGE)`) | présent dans une copie privée de la v10 (`morsehgp3D_v10/receipts/audit_continu_20260929/order_head_corrected/observed_verif/src_tout/morsehgp3D_v10/src/catalogue/catalogue.hpp` l. 47–54, tableaux ≥ 4 Mio), absent de la v10 publiée, de R2 et de la v11 (L) ; profil W48 v11 : fautes de page 3,12 % du CPU, `do_anonymous_page` 2,47 %, verrou noyau 1,55 %, `__pte_offset_map_lock` 1,51 % ([PROF1], M) | sorties inchangées ; vaut à froid, sans la décision « processus résident » de V9 | C ; à mesurer à W48 et W24 |
| PGO, LTO, compilateur récent | aucune mention dans l'audit ni dans les dépôts v6 à v11 (`grep` de `fprofile`, `flto`, `PGO`, `BOLT` : vide) ; la G4 n'a que g++ 11.4, sans Clang (`env/` des archives) ; code à branchements (DFS de feuille, parcours du recensement) | sémantique inchangée ; épingler `-ffp-contract` et l'empreinte du binaire au reçu | C ; à décider à W1 |
| Attente hybride du pipeline (4 096 `pause` avant le futex) | correctif prêt, jamais mesuré : `build/v11-persist/perf_wip_20261003/leaf_counters_pipeline_spin.patch` (`pipeline_spin` dans `await_job` et `ProgressView::block`) | aucune décision n'en dépend | candidat V2 ; risque : le publieur qui tourne vole son frère SMT résolveur |

### P7 — Le protocole recommandé est contourné, et `diag1` porte une faute d'ordre

- `claudeab8`, « prévue » selon le verdict, a tourné de 13 h 36 à 14 h 08 sans bras A/A ni W24, avec cinq prises W48 :
  le protocole que F2 déclare incapable de trancher sous 30–40 ms (P3 en montre l'effet). La partie statistique de F2
  existe depuis 14 h 11 (`bench/ab_summary.py`, `66372e621`).
- **Faute d'ordre, urgente.** `bench/ab_g4.py` (`54c167bb6`, l. 191–192) tourne l'ordre d'un cran puis l'inverse une
  prise sur deux ; les auditeurs ont signalé à 14 h 03 (`3a56bc5ca`, `receipts/audit_ports_20261004/ab_order/`) que,
  pour un nombre pair de variantes, chacune garde la parité de sa position. Le plan
  `build/v11-persist/diag1/plan.json` (14 h 06) joue `--variants qr,qr2,r1` plus `new`, soit quatre variantes :
  `qr` est toujours en position paire, son double A/A `qr2` toujours impaire, et `qr` passe avant `qr2` dans quatre
  prises sur cinq (simulation des l. 191–192). Le bras A/A mesurerait l'effet de position. Corriger l'ordre (inverser
  après un cycle complet de N rotations, recommandation des auditeurs) ou jouer un nombre impair de variantes, avant
  la session.
- **Couvert par `diag1`** : bras A/A, première référence K = 10 (feuilles 16 et 24), W24 contre W48. **Manquent** :
  trames lourdes (P4), ledger du recensement par arité, compteurs de décision (`resolve1`, répétitions de cellule,
  « la liste de la feuille aurait suffi »), c(L), ablation destructive de `close`, ablation « pipeline coupé » (P2),
  temps volé, appariement SMT et THP (P5), nombre de paires fixé d'avance, préfixe Kmax (F8) sur le dump K = 10.

### P8 — K = 10 : angles morts

- **Mémoire jamais estimée pour la v11.** À K = 5, 306–377 Mio réservés au pic pour 1,10–1,41 M boules, soit ≈ 280–290 octets
  par boule (E, [AB7]) ; la v10 mesurait ≈ 280 octets par boule au pic à K = 10 et 1 823 780 kio de RSS sur 08/000000
  (M, `g4_session5_scale_20260929`) ; d'où ≈ 1,5 Gio sur ng00 et ≈ 4,3 Gio sur une trame de 16 M boules (E), sous le
  plafond de 8 Gio mais avec quatre fois plus de premiers contacts de pages à froid. PTS4 ne publie aucune mémoire.
- **Partage des fils** 29/10/9 du pipeline (P2) absent de toutes les projections ; trois projections C qui diffèrent
  (1,5–1,8 s carte v11 § 4 ; 1,5–2 s carte v10 § 3.5 ; 1,5–2,5 s plan T0).
- **Filets à K + 2 = 12** : c'est la limite exacte de `kMaxMebSites` ; le coût de Cat12 et du juge d'Euler Python sur
  ≈ 8–9 M boules (E) n'est pas estimé.
- **Porte points** bornée à k ≤ 4 (P9) alors que P08 joue k = 10.

### P9 — Port natif de la hiérarchie de points

- La porte points qualifiée (`receipts/points_gate_qualification_20261004/`, session `claudepts6`) est bornée à neuf
  sites, **k ≤ 4**, m ∈ {1, k + 1, k + 2} (L) ; E1 et P08 jouent k = 2, 3, 5, 10. Le différentiel « Python ↔ natif » du
  plan (PLAN_AUTRES § 5) ne ferait que recopier Python à k ≥ 5 : il faut un oracle ou un juge d'échantillon à k = 5 et
  10 avant le port.
- Reçus « points » jamais lus : `points_code_review_20261004` (domaine n < m refusé, frontière d'API),
  `points_math_followup_20261004` (une réunion infimum peut ne pas être atteinte), `hm_review_20261003` et
  `hm_followup_20261003` (plafond de niveau à corriger, stabilité 3δ), `eom_exact_audit_20261004` (inverse exact de la
  date EOM), `measure_metric_20261004`, `palm_obstruction_20261003`, `full_points_20261003`,
  `audit_full_hierarchie_20261002`, sept reçus `flat_*_20261004`.
- Temps de H^r_{k+1} jamais mesuré seul sur G4 (3,1 s à k = 5 sous contention, [PTS4]) ; « gain natif ≥ ×10 » est C.

### P10 — Décisions à poser à l'utilisateur, oubliées

Outre froid ou chaud, maximum ou médiane, GPU, K = 10 et retrait du sol (§ 6.1 du verdict) : **ce que FULL doit rendre
dans les 100 ms** (le dump fait 255–329 Mo et « processus − FULL » vaut 479–600 ms à W48, [Q] `analysis.md`
l. 44–54) ; si la hiérarchie de points et la tête plate comptent dans le budget ; si un binaire construit hors de la VM
(compilateur récent, PGO) est admis.

### P11 — GPU : dossier solide, trois trous

c(L) n'est ni dans `diag1` ni dans `e49ea4690` (il faut un chrono par sous-arbre de la passe unique) ; aucune sonde de
débit valide (S7 invalide, `claudegpu0` seulement nommée) ; `gcp-migration/v11_session.py` et `v11_worker.sh` ne
contiennent aucune mention de CUDA ni de nvcc (L), alors que la boîte à outils de la VM est hors du PATH (CUDA 12.9,
reçu v10 `g4_session5_scale_20260929`) et qu'une tentative v7 a été refusée faute d'outils
(`morsehgp3D_v7/receipts/terminal_batch_g4_20260911/`).

## 2. Contradictions entre rapports

| Sujet | Rapport A | Rapport B ou reçu | Lecture |
| --- | --- | --- | --- |
| « Passe unique au plafond SMT » | CARTE_V11 § 2.3 et § 6 (lecture 1, marquée M) ; PLAN_VITESSE § 1.4 ; verif/v7 | CARTE_V10 § 3.3 ; v10 `g4_session2_perf_20260929` : boîtes 0,653 → 0,369 → 0,214 s à 12 / 24 / 48 fils (SMT ×1,72) | même machine, étage équivalent : pas un plafond matériel ; le verdict s'en garde, le plan non |
| Meilleure prise v11 | CARTE_V11 § 2.1 et PLAN § 0 : 327,7 ms (source courante), 320,4 ms (toutes v11) | `claudeab4` 305,3 ; `claudecat1` (même binaire que b872) 307,7 ; `claudeab8` 299,0 ms (ng01) | chiffres faux, sans effet sur le verdict |
| Passe unique « mesurée » sur ng00 | verdict § 2.1 : 195,0 ms (médiane indépendante) | verdict § 2.4 : 180,3 ms (prise médiane du FULL) | deux conventions, non dites |
| Gain V1 | fouille v10_moteur : −30 à −75 ms | plan : −38 à −60 ms (prudent −25) ; verdict : tableau −25 à −55, texte −38 à −60 | à aligner ; `claudeab8` : tranche (a) nulle |
| Référence K = 10 | 1,5–1,8 s (CARTE_V11 § 4) | 1,5–2 s (CARTE_V10 § 3.5) ; 1,5–2,5 s (PLAN T0) | trois conjectures |
| Gonflement ×1,5 du CPU W48 | PLAN § 1.3 : « levier à part entière » | verif/v8 § 3 : le CPU de fil ne sépare pas SMT et contention | un débit SMT de ×1,3 par cœur donne mécaniquement ≈ ×1,5 de CPU (E) : pas un travail récupérable tant que W24 n'a pas parlé |
| `-march` « ne pas refaire » | verdict § 5.1 | règle F2 du même verdict (décider un levier de travail à W1) ; [PROF1] : W48 seul, trois prises non appariées, CPU v4 13,22–13,29 s contre 13,84–13,89 s sur ng00 | essai sous-dimensionné ; à rejouer à W1 |

## 3. Affirmations non vérifiées, chiffres à requalifier

- « Cible effective 54–77 ms » (§ 1 item 7) : E non vérifié, contredit en partie (P4).
- « Gros levier immédiat : l'ordonnancement du pipeline » (§ 1 item 5, V2) : appuyé sur ng00 seul. Trois prises sans
  queue sur ng00 (373,7 ; 391,1 ; 378,1 ms, `claudeab4` compris), **aucune** sur ng01 et ng02 (dix prises chacune,
  queue minimale 14,9 et 16,7 ms) ; et dans le pipeline, la passe unique de ng00 varie de 167,6 à 252,3 ms, autant que
  la queue (0,9–46,2 ms) : la queue n'est pas la seule source de la dispersion du FULL.
- Budget « résolution ≤ 30 ms » et gains V3/V7 convertis en mur : un tiers du temps de résolution pipelinée est
  structurel (P2).
- « Feuille de 16 sites codée en dur » (§ 1 item 9, V8) : vrai pour `bench/points_export.cpp` l. 377 ; le banc FULL
  prend `--leaf` (`bench/full_timing.py`, plan `diag1`).
- « V6 : noyau sans branchement, −5 à −20 ms » (C) : la seule mesure G4 d'une variante est négative (P3).
- « V5 : compteurs, −2 à −7 ms » : jamais mesuré seul ; `claudecat1` le groupe à un levier négatif.
- « FULL attendus » par tranche (§ 4.1) : projections depuis 412 ms d'une seule session (P5).

## 4. Sources non lues

| Corpus | Cités / existants | Non lus qui comptent |
| --- | --- | --- |
| reçus v11 (`receipts/`) | 22 / 51 | archives `claudeab1`, `claudeab4` (versées dans `pipeline_g4/sessions/`) ; `full_pair_graph_20261003/graph2_failure`, `full_dense_20261003/dense1_failure` ; reçus points (P9) ; `catalogue_profiles_20261002`, `catalogue_q4_20261002` (profils u18/u21/u24 mono-fil, 8k et 16k, K = 10 expirés à 30 s) ; `audit_ports_20261004` (14 h 03) |
| journaux de sessions G4 (`/workspaces/.ehgp-sessions/`, hors git) | 0 / 144 dossiers (80 de la v11) | `v11.20261003.claudecat1` et `v11.20261004.claudeab8`, sans reçu versé (P3) |
| reçus v10 | 8 / 40 | `g4_session2_perf` (courbe 1 / 12 / 24 / 48 fils), `g4_session5_scale` (exposants, RSS, un million de sites), `g4_session3_j2`, `catalogue_load_balance` (24 → 48 fils : 1,19 → 1,11 s avant correctif), `bench_dev_bigk`, `g4_sessions_batterie_20261001`, `audit_geant_developpeur_20260930` |
| `build/v10-persist/` | 2 chemins cités | `alloc/`, `scale/`, `bigk/`, `prof_cat/`, `cat_lidar02_k10.json` (K = 10, 6 fils : catalogue 20,8 s, RSS 1 894 984 kio), `CONNAISSANCES_V10_20260928.md` |
| reçus v9 / v8 / v7 / v6 / v5 | 9/78, 2/51, 8/136, 4/21, 2/24 | verdicts « aucun levier » fondés sur les documents ; recevables (architectures 2 à 100 fois plus lentes), mais les mesures W24 et SMT (v9 `g4_tower_r8_20260923` ; v9 `docs/audit_v8/12_parallelisme_gpu_perf.md`) n'ont pas servi |
| `build/v11-persist/` | `conception/`, `audit_v10/` | `perf_wip_20261003/` (P6), `diag1/` (P7), `fondations/` (outillage G4) |

## 5. Ce qui a changé depuis le verdict (13 h 57 → 14 h 23 UTC)

| Heure | Fait | Effet sur le verdict |
| --- | --- | --- |
| 13 h 57 | `ec55578d9` : M3 et E4 de J3 commis (dumps identiques sur ng00 et ng02) | V1 (b) « en cours » → porté |
| 13 h 58 | `0cf20f98c` : `bench/full_timing.py` (F4, lot H) ; plan `diag1` écrit (mis à jour à 14 h 06) | F4 « non commis » → commis ; T0 engagé en partie (P7) |
| 14 h 03 | `3a56bc5ca` : les auditeurs relisent q3 et R, répondent sur le ledger J3, signalent la faute d'ordre du banc | P7 |
| 14 h 05 | `e49ea4690` : début, CPU de fil et attente de chaque tâche du pipeline | F3, volet pipeline, fait ; ni la passe unique ni la voie par étages |
| 14 h 08 | `claudeab8` terminée, fermeture certifiée à 14 h 09 | P3 : q3 différé −1,5 à −1,8 % à W1, lemme R nul |
| 14 h 11 | `66372e621` : `bench/ab_summary.py` (médiane des rapports appariés, test des signes) | partie statistique de F2 faite |

FIN
