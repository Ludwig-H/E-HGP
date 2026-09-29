# Reçu — bancs : délais de `scale_run` et complétude de `decide` (29 septembre 2026)

```text
phase=exploration_v10_hors_registre
backend=cpu_reference
profile=quantized_u18_input_only
mode=correction_bancs
public_status=not_claimed
```

Base : `0bce6cc00` (copie privée `src/`, commit `base`). GCP non utilisé. Aucune scène générée sur les graines
`test` ou `test_v10b` : les lots A et C archivés sont seulement relus (reçus immuables, non modifiés). Aucun source
C++ touché : le correctif ne porte que sur trois scripts Python du banc, deux portes et leur enregistrement CMake.

Patch : `bancs.patch` (chemins `morsehgp3D_v10/...`, applicable par `git apply` à la racine du dépôt ; vérifié sur
une extraction neuve de `0bce6cc00`). Preuves : `avant/` (défauts reproduits) et `apres/` (correctif).

## 1. Constats reproduits sur la base non modifiée (`avant/`)

### (a) `scale_run.run_json` : le calcul survit au délai

Sonde de l'audit continu, jouée telle quelle depuis la copie (elle importe le vrai `run_json`) :

```bash
python3 -B src/morsehgp3D_v10/audits/audit_continu_20260929/timeout/reproduce.py      # code 0
python3 -B -O src/morsehgp3D_v10/audits/audit_continu_20260929/timeout/reproduce.py   # code 0
```

Sortie (`avant/a1_timeout_reproduce_normal.json`, `a2_timeout_reproduce_O.json`, reçus bruts `a1_…receipt…`,
`a2_…receipt…`) : `run_json` rend `[-9, null, 1.03, null, null]` après 1 s, alors que l'enfant est encore vivant,
état `S` (`child_alive_after_timeout_return: true`, `regression_reproduced: true`), en normal comme sous `-O`.
Cause : `subprocess.run(timeout=…)` ne tue que `/usr/bin/time`, le calcul est son enfant.

### (b) `decide.py` décide sur un lot incomplet ; `merge_sessions.py` accepte une scène hors plan

Sonde de l'audit indépendant (copiée dans `avant/sondes/`, sha256 `292b3cc3…`, identique à l'original non suivi) :

```bash
python3 -B avant/sondes/evidence_bench_completeness_audit.py <base> <base>   # code 0
```

Sortie (`avant/b1_evidence_bench_completeness.jsonl`) : lot C archivé complet (30 720 couples, 0 manquant) ;
`partial_decide` : une scène sur 960, `complete=false`, **code 0** et `DECISION.json` écrit sur 1 scène ;
`merge_unknown_unit` : scène hors plan fusionnée, **code 0**.

Étendue du défaut, mesurée avec l'**ancien** `decide.py` (numpy présent, décision complète) sur les fixtures de la
nouvelle porte (`avant/sondes/avant_decide_numpy.py`, `avant/c3_ancien_decide_avec_numpy.jsonl`) : 15 lots
incohérents distincts sont décidés avec le code 0 et un `DECISION.json` écrit — une scène sur 32, scène manquante
(31 décidées), doublon de valeur différente (écrasé en silence), scène hors plan (remplaçante ou en plus : 33
scènes), famille, chaîne de bruit ou graine fausse, `ari_s=nan`, `refused=2`, `run.json` déclarant
`complete=false`, autre plan, autre nombre de scènes, plan altéré sous la même épingle, fusion partielle (16
scènes). Seuls le couple manquant dans une scène présente, la méthode inconnue, l'autre préenregistrement (code 2)
et la colonne absente (plantage, code 1) étaient arrêtés. L'ancienne fusion déclarait aussi « 32 scènes sur 32 »
(`complete=true`) quand une scène hors plan remplaçait une scène prévue : elle comptait, sans comparer les noms.

### (c) Les deux nouvelles portes, jouées contre l'arbre non corrigé

Arbre `0bce6cc00` + portes finales copiées (`avant/c0_arbre_ancien_sha256.txt`) :

- `test_scale_run_timeout.py` : **code 1, 8 échecs sur 8** (`avant/c1_porte_delai_sur_ancien.txt`) — enfant et
  petit-enfant survivants au délai, signal au lancement non géré, pas de JSON natif, pas de fichier d'appels, écart
  de boules non vu (`ok`, code 0), tour survivante au délai en CLI, SIGTERM et SIGINT : `scale_run` meurt (−15,
  −2) et la tour survit ;
- `test_decide_completeness.py` : **code 1, 29 échecs sur 35** (`avant/c2_porte_completude_sur_ancien.txt`) —
  sous `python3 -S` l'ancien `decide.py` ne peut même pas refuser (import de numpy en tête, code 1) ; fusion hors
  plan acceptée.

Par la forme exacte de CTest (`cmake -P cmake/run_expect.cmake`, `EXPECTED=0`, `EXPECT_LINE`), les deux portes
échouent contre l'arbre non corrigé : `code de sortie 1, attendu 0` (`avant/c4_run_expect_ancien.txt`). Aucun
processus dormeur ne subsiste après ces exécutions (nettoyage par PID).

## 2. Correctif

### `bench/scaling/scale_run.py`

- Chaque appel est lancé par `subprocess.Popen(..., start_new_session=True)` : son propre groupe de processus (et
  session), `/usr/bin/time` et le calcul compris.
- `close_group` : au délai comme à toute exception, `SIGKILL` au groupe entier **avant** de récolter l'enfant
  direct (le numéro de groupe reste réservé), puis récolte des descendants revenus au processus (Linux :
  `PR_SET_CHILD_SUBREAPER`, sinon récolte par init), et retour seulement quand `killpg(pgid, 0)` échoue (groupe
  vide). Délai de grâce 10 s, sinon `GroupNotClosed` : mesure arrêtée, code 3. SIGINT/SIGTERM/SIGHUP sont
  bloqués pendant la fermeture puis délivrés après.
- Signaux : `run` installe un gestionnaire pour SIGTERM, SIGHUP et SIGINT (seulement si leur disposition est celle
  par défaut : un signal ignoré, `nohup` ou `&`, reste ignoré). Il tue et récolte l'appel en cours puis sort avec
  128 + signal. Un signal reçu entre `fork` et la garde de l'appel est différé puis levé sous garde. C'est
  nécessaire : dans sa propre session, l'appel n'est plus atteint par un signal envoyé au groupe de `scale_run`
  (le worker G4 envoie SIGTERM à la commande, puis au groupe de l'étape).
- JSON natif conservé : `run_json` rend un enregistrement (argv, code, `timed_out`, stdout et stderr bruts, mur,
  CPU, RSS) ; chaque appel est écrit, une ligne JSON par appel, dans `--calls` (défaut `<out>.calls.jsonl`), avec
  `file`, `k`, `threads` et `call` (`catalogue` ou `tower`). Le stdout brut garde le JSON octet pour octet.
- Compteurs liés : si `balls` de l'appel tour diffère de `balls` de l'appel catalogue, statut `balls_mismatch`
  (aucune colonne de tour écrite) et code de sortie 3 à la fin du passage.
- Garde de `cpu_s` quand `/usr/bin/time` est absent (l'ancien code levait `TypeError` sur `round(None)`).
- Docstring : l'affirmation « les compteurs sont déterministes » est corrigée. Les compteurs de l'objet (boules,
  nœuds, catalogue) le sont ; `tower_steps_*` dépend de l'ordre d'arrivée des fils au-delà d'un fil, comme le
  documente déjà `src/tower/tower.hpp` (voir § 4). C'est pourquoi seule l'égalité de `balls` est exigée.

La mesure elle-même ne change pas : `/usr/bin/time -f '%e %U %S %M'` reste l'instrument (mêmes colonnes, même
précision). Les statuts existants (`catalogue_timeout`, `tower_timeout`, `…_refused_<code>`, `skipped_budget`) sont
inchangés ; le délai se lit désormais sur un drapeau explicite (`timed_out`), plus sur le code −9.

### `bench/synthetic/decide.py`

- Avant tout calcul : `check_run` reconstruit le manifeste du plan **en Python nu** (`plan_specs` : même boucle que
  `run_campaign.plan`, même filtre que `run_test.plan_manifest`, ordre canonique de `FAMILIES`/`LEVELS` lu dans la
  source de `scenes.py` par `ast`, sans l'importer). Il exige ensuite :
  - sha256 reconstruit = épingle `manifest_sha256` du préenregistrement ;
  - `run.json` : même plan, même nombre de scènes, et ni `complete` ni `computed` ne déclarent une campagne
    incomplète ;
  - lignes (`check_rows`) : chaque couple (scène prévue, méthode) présent exactement une fois ; aucune scène hors
    plan ni méthode inconnue ; métadonnées égales à la spécification **à la chaîne CSV près** (famille, niveau,
    bruit, n, graine — la cellule de pondération en dépend) ; `refused` dans {0, 1} ; ARI_s et AMI finis ;
    colonnes requises présentes.
- Sinon : une ligne `REFUS : …` par défaut, code 2, rien n'est écrit.
- numpy n'est importé qu'après cette vérification : refus et `--check-only` tournent sans numpy (VM G4).
- Nouvelle option `--check-only` : la vérification seule, code 0 (ligne `lot_conforme_au_plan`) ou 2.
- La statistique (`load`, `Paired`, `compare`, jugement, écritures) n'est pas modifiée.

### `bench/g4/merge_sessions.py` (même défaut)

- Plan reconstruit par `decide.plan_specs` (import Python nu) et comparé à l'épingle.
- Chaque session doit porter `plan_sha256` = épingle, pas seulement le même plan que les autres sessions.
- Toute scène hors plan est refusée (code 2).
- `complete` vaut `ensemble fusionné == ensemble du plan`, et `scenes` le nombre de scènes du plan. L'ancien calcul
  était un compte, `len(seen) == segments[0]['scenes']`. Une fusion partielle reste écrite (code 0,
  `complete=false`) ; c'est `decide.py` qui la refuse.

### `CMakeLists.txt`

Deux portes dans le bloc `if(Python3_FOUND)`, forme `run_expect.cmake`, code exact 0 et ligne finale exigée dans
la même exécution (`EXPECT_LINE`), labels `gate;regression;fast`, délai CTest 300 s.

## 3. Portes ajoutées

| Porte | Script | Contrôles |
| --- | --- | --- |
| `mhgp10_regression_scale_run_timeout` | `tests/regression/test_scale_run_timeout.py` | 8 |
| `mhgp10_regression_decide_completeness` | `tests/regression/test_decide_completeness.py` | 35 |

**Porte du délai.** Vrais processus qui dorment, flux fermés comme dans la sonde de l'audit, délai de 3 s :

- `run_json` rend −9, et enfant ET petit-enfant sont tués et récoltés au retour ;
- SIGTERM au lancement : différé, puis appel tué et récolté ;
- JSON natif conservé octet pour octet ;
- avec de faux binaires de même interface : ligne `ok` et deux appels dans `.calls.jsonl` ; écart de boules →
  `balls_mismatch`, code 3 ; délai de la tour → `tower_timeout`, tour tuée ;
- SIGTERM → 143 et SIGINT → 130, avec l'appel tué et récolté.

**Porte de complétude.** Plan préenregistré de fixture sur les graines **dev** (32 scènes, 3 méthodes, ordre des
familles volontairement non canonique) et ses variantes :

- 1 lot complet (et sa forme fusionnée) accepté ;
- 18 variantes incohérentes refusées en 20 contrôles (code 2, ligne `REFUS`, aucun `DECISION.json`), dont deux
  rejouées en décision complète, sans `--check-only` : la sonde de l'audit et la scène manquante ;
- lots A et C archivés acceptés par `--check-only` ;
- lot C moins une ligne refusé ;
- sonde de l'audit rejouée à l'identique sur le lot C (une scène, `complete=false`, décision complète) : refusée ;
- fusion : scène hors plan, autre plan et scène calculée deux fois refusés ; fusion partielle écrite puis refusée par
  `decide` ; fusion complète écrite puis acceptée ;
- oracle de la porte, réécrit indépendamment de `decide.py`, qui retrouve les épingles des lots A et C.

Les scripts du banc y tournent sous `python3 -S` : sans site-packages, donc sans numpy, sur tout hôte.

Les deux portes : Python nu (bibliothèque standard seule), aucun `assert`, aucun `__pycache__` écrit dans l'arbre,
tout processus lancé est tué par son PID à la fin (jamais par nom).

Résultats :

| Exécution | Porte du délai | Porte de complétude |
| --- | --- | --- |
| arbre non corrigé | code 1, 8 échecs | code 1, 29 échecs |
| corrigé, `python3` (numpy présent) | 0 | 0 |
| corrigé, `/usr/bin/python3` 3.12.3 **sans numpy** | 0 | 0 |
| corrigé, `/usr/bin/python3 -O` | 0 | 0 |
| corrigé, `-X dev`, `ResourceWarning` toujours affichés (`apres/e1_devmode/`) | 0, aucun avertissement | 0, aucun avertissement |
| corrigé, CTest `-L gate` (§ 5) | Passed 7,4 s | Passed 7,0 s |

Mutants causaux de `scale_run.py`, tous tués (`apres/sondes/mutants_scale_run.py`,
`apres/mutants/resume_scale_run.txt`) :

| Mutant | Contrôles qui le tuent |
| --- | --- |
| sans report des signaux au lancement | `signal_pendant_le_lancement` |
| sans récolte du groupe | `delai_enfant_et_petit_enfant` (zombies au retour) |
| `proc.kill()` au lieu de `killpg` | délai direct, délai CLI, SIGTERM, SIGINT (`GroupNotClosed`, code 3) |
| sans nouvelle session | délai, lancement, délai CLI, SIGTERM, SIGINT |
| sans contrôle des boules | `run_compteurs_de_boules_egaux` |
| sans fichier d'appels | trois contrôles CLI |
| sans gestionnaire de signaux | SIGTERM, SIGINT |

Syntaxe vérifiée contre la grammaire 3.10 (`ast.parse(feature_version=(3, 10))`), Python système de la VM G4 ;
aucune API postérieure à 3.10 (`start_new_session`, pas `process_group`).

## 4. Preuves : aucune sortie changée sur les entrées valides

- **Re-decide du lot C** : le nouveau `decide.py` rend `DECISION.json` et `DECISION.md` identiques octet pour octet
  à l'archive (sha256 `847a238d…`, `e92d3b62…`) ; l'ancien aussi (contrôle de déterminisme sur cette machine).
  Mur 2 min 35 s (`apres/h_redecide/`).
- **Lot A** : nouveau `decide.py` = ancien (`DECISION.json` `286f86a1…`, `DECISION.md` `85d13cba…`, ce dernier
  égal à l'archive). Le `DECISION.json` archivé du lot A diffère des deux par la seule clé `secondary: {}` ajoutée
  par `c764e121a`, après l'épingle du lot A : écart antérieur au correctif, toutes les autres clés sont égales.
- **Fusion** : les sessions c1 (338 scènes) et c2 (622) du lot C, reconstituées depuis le `results.csv` archivé
  (ordre des segments) et leurs `run.json` de segment, refusionnées par l'ancien et le nouveau
  `merge_sessions.py` (ce dernier en Python nu, `-S`) : `results.csv`, `run.json` et `done.u32le` identiques entre
  eux et à l'archive (`313312ef…`, `ab6df492…`) (`apres/i_refusion/`).
- **Plan en Python nu** : `decide.plan_specs` = `run_test.plan_manifest` (liste des spécifications, sha256, noms
  d'unités) sur 302 plans sur 302 : 300 plans dev tirés au hasard (sous-ensembles et ordres arbitraires) et les
  deux préenregistrements A et C, épingles comprises (`apres/d1_plan_differentiel.json`).
- **Sonde de l'audit après correction** : `partial_decide` → code 2, aucune décision ; `merge_unknown_unit` → code 2
  (`apres/b1_evidence_bench_completeness.jsonl`).
- **`scale_run.py`**, vrais binaires de référence (`v10-wt`, copiés dans `/tmp`), entrées de `scale_inputs.py`
  (graine 3) :
  - à **1 fil**, 7 entrées à K = 5 (7 069 à 9 468 sites, quatre synthétiques à 8 000 et trois quarts LiDAR ;
    160 k à 595 k boules) et une à K = 10 (811 610 boules) : les 22 colonnes déterministes du CSV sont identiques
    entre ancien et nouveau, en-têtes compris (`apres/g_scale_diff_t1/`). C'est un différentiel de sorties, pas une
    mesure de coût ;
  - RSS et CPU restent du même ordre, même instrument ; seul le mur varie, la machine étant chargée (charge 23 à
    39) ;
  - à 2 fils, `tower_steps_*` diffère… mais aussi entre trois exécutions directes du **même** binaire sur la même
    entrée : 252 041, 252 038, 252 043 à 2 fils, 252 037 deux fois à 1 fil. Toutes les autres colonnes sont égales.
    Propriété documentée du moteur, pas du correctif ;
  - chaque ligne `ok` est cohérente avec ses deux JSON natifs : `balls` des deux appels, sommes des nœuds et des
    pas, `judged`.
- **`ctest -L gate`**, build Release neuf (`build/`, `--parallel 2`) : voir § 5.
- Toutes ces preuves ont été produites avec les versions finales des fichiers du patch, sauf
  `apres/g_scale_diff_t2/` : première tentative à 2 fils, avec une version intermédiaire de `scale_run.py`, gardée
  pour l'écart `tower_steps` (voir son `LISEZMOI.txt`). Versions finales : `scale_run.py` `f8d5e891…`,
  `decide.py` `bab58df8…`, `merge_sessions.py` `2526cc3d…`. Le différentiel à 1 fil a été rejoué sur la version
  finale (`final_k5.csv` : 0 écart, 7 lignes sur 7 cohérentes avec leurs 14 JSON natifs).

## 5. Suite complète des portes

`ctest --test-dir build -L gate --output-on-failure` sur le build Release de la copie corrigée : **11 portes sur 11
vertes**, code 0, 2 296 s (`apres/f1_ctest_gate.txt`). La durée vient de la charge de la machine partagée (charge
moyenne 23 à 39 sur 8 cœurs), pas des portes ajoutées.

| Porte | Durée (s) |
| --- | ---: |
| `mhgp10_unit` | 2,3 |
| `mhgp10_catalogue_oracle` | 1 044,2 |
| `mhgp10_tower_oracle` | 1 027,0 |
| `mhgp10_head_condensation_vs_sklearn` | 28,4 |
| `mhgp10_regression_level_collision` | 29,5 |
| `mhgp10_points_cover` | 45,2 |
| `mhgp10_regression_batch_equivalence` | 97,9 |
| `mhgp10_regression_mreach_border` | 7,3 |
| `mhgp10_regression_multiplicity_refusal` | 0,2 |
| `mhgp10_regression_scale_run_timeout` (nouvelle) | 7,4 |
| `mhgp10_regression_decide_completeness` (nouvelle) | 7,0 |

Stabilité : la porte du délai, rejouée dix fois de suite sous une charge moyenne de 33 à 37, est verte dix fois sur
dix (`apres/k_repetitions_porte_delai.txt`). Les deux portes, avec leurs fichiers finaux, sont vertes sous
`python3` (numpy présent), sous `/usr/bin/python3` sans numpy et sous `/usr/bin/python3 -O`
(`apres/j_portes_interpretes.txt`). Après chaque passage, aucun processus dormeur ne reste et aucun `__pycache__`
n'est écrit par les nouvelles portes.

## 6. Sorties modifiées sur entrées valides

Aucune sortie existante ne change. Sur une entrée valide :

- `scale_run.py run` écrit le même CSV : colonnes, statuts, valeurs des compteurs, même instrument de temps. Il
  ajoute le fichier `<out>.calls.jsonl` (JSON natifs) et accepte l'option `--calls`. Codes : toujours 0, sauf
  `balls_mismatch` (3), groupe non fermé (3) et signal (128 + n), qui ne surviennent pas sur une entrée valide
  menée à terme.
- `decide.py` écrit des décisions identiques (lot C octet pour octet, lot A égal à l'ancien) et accepte l'option
  `--check-only`.
- `merge_sessions.py` écrit une fusion identique (lot C octet pour octet).

## 7. Limites et points ouverts

- Seul SIGKILL envoyé à `scale_run` lui-même, sans SIGTERM préalable, laisse l'appel en cours vivre dans sa propre
  session, hors du groupe de l'étape G4 (limite écrite dans la docstring). Le worker G4 envoie SIGTERM d'abord et
  laisse 10 s. `scale_run` tue alors l'appel en quelques millisecondes (portes SIGTERM et SIGINT). Fermer aussi ce
  cas exigerait de remplacer `/usr/bin/time` par `wait4` et `PR_SET_PDEATHSIG` : c'est un changement d'instrument
  de mesure, non fait ici.
- `tower_steps_*` n'est pas déterministe au-delà d'un fil (§ 4). Les exposants « pas de tour » recalculés par
  l'audit sur la session 5 (48 fils) portent donc une variabilité d'exécution, de l'ordre de 1e-5 en relatif ici.
  Elle est sans effet sur ses conclusions, mais ce n'est pas un compteur de l'objet.
- `decide.py` change de sha256 (`d360ed0e…` épinglé par `PREREG_V10_COVER_C_20260929`, `f1859ec0…` par le lot A).
  Les deux préenregistrements sont exécutés. Leur décision refaite est identique (C) ou égale à l'ancien code (A).
  Un préenregistrement futur épinglera le nouveau `decide.py`. `run_test.check_pins` refuserait une nouvelle
  exécution sous l'épingle C, ce qui est voulu (une exécution par préenregistrement).
- `run_test.py --resume` garde encore toute scène complète en méthodes, même hors plan ou dupliquée : script épinglé,
  hors du périmètre. `decide.py` refuse désormais un tel lot en fin de campagne.
- La sonde `audit_continu_20260929/timeout/reproduce.py` ne s'applique plus après correction. Elle exige que
  l'enfant partage le groupe du worker, ce qui est justement supprimé : rejouée sur le code corrigé, elle échoue sur
  `unexpected child process group` (code 1). Son reçu montre pourtant le correctif : `timed_out` après 1,01 s,
  enfant dans le groupe de `/usr/bin/time`, et plus aucun descendant à récolter par son nettoyage
  (`reaped_owned_descendants: []`, `apres/l_sonde_audit_delai_apres*`). La porte permanente la remplace.
- `decide.py`, lignes 266–267 préexistantes (`json.load(open(...))`) : deux `ResourceWarning` sous `-X dev`, hors
  correctif et laissés tels quels.
- `merge_sessions.py` dépend maintenant de `bench/synthetic/decide.py` (import Python nu).
- `README.md`, `PASSATION.md` et `receipts/ERRATA.md` ne sont pas modifiés ; ils sont à mettre à jour par
  l'intégrateur. À y reporter : les deux portes, `--check-only`, `.calls.jsonl`, la phrase corrigée sur les
  compteurs.
- Sanitizers C++ (ASan/UBSan, TSan) non pertinents : aucun code C++ modifié. Leur équivalent ici est fait : mode
  développement Python, `-O`, Python nu, `-S`, mutants, absence de processus et de fichiers résiduels.

## 8. Fichiers

- Patch : `bancs.patch` (sha256 `39a669b9…`, 1 246 lignes, 6 fichiers ; `git apply --check` puis `git apply` sur
  une extraction neuve de `0bce6cc00` donnent un arbre identique à la copie de travail). Il s'applique aussi sans
  conflit sur `12aa92110`, HEAD du dépôt à la fin de ce travail : entre les deux, seul `bench/g4/cuda_probe.cu` a
  changé dans les chemins concernés.
- Modifiés : `morsehgp3D_v10/bench/scaling/scale_run.py`, `morsehgp3D_v10/bench/synthetic/decide.py`,
  `morsehgp3D_v10/bench/g4/merge_sessions.py`, `morsehgp3D_v10/CMakeLists.txt`.
- Nouveaux : `morsehgp3D_v10/tests/regression/test_scale_run_timeout.py`,
  `morsehgp3D_v10/tests/regression/test_decide_completeness.py`.
- Preuves : `avant/` (a1, a2, b1, c0 à c4, sondes) et `apres/` (b1, d1, e1, f1, g, h, i, j, k, l, mutants,
  sondes) ; `SHA256SUMS` couvre le patch, ce reçu et toutes les preuves.
