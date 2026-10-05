# Tranche S9 — hiérarchie de points native et `--sortie=points`

5 octobre 2026, 16 h 55 UTC (heure lue par `date -u`). Livraison L3. Cadre : phase=exploration_v11_hors_registre,
backend=cpu_reference, profile=quantized_u21_input_only, public_status=not_claimed. **GCP non utilisé.**

## Commit (local, non poussé)

- `451301787` sur `53c027fe8`, worktree `build/v11-impl-l3` en HEAD détaché :
  `v11: native point hierarchy and --sortie=points with mhgp11pt v1 (slice s9)`. 45 fichiers, +3 556 / −93. Index
  vérifié vide avant l'ajout, ajout chemin par chemin, aucun `__pycache__`.

## Ce qui est livré

| Élément | Contenu |
| --- | --- |
| Alignement $m(1)=1$ (avant tout différentiel) | `bench/points_radius.py` gagne `qualification(k)` **en fin de fichier** (aucune ligne portée ne bouge ; `tests/num/radical_port.py` : `source_mismatches = []`) ; `points_flat_gate.py:117`, `points_flat_campaign.py:164`, `points_flat_dump.py:67,71` (métadonnée `m`), `points_campaign.py:135-138` l'appellent. **F13 rejouée** sous $m(1)=1$ : `F13_identite_k1 m(1)=1 mode=reference nuages=24 bad=0 ok=True` (4,2 s) et `mode=natif` sur l'export natif (10,2 s). |
| `src/tower/ancestor_index.{hpp,cpp}` | `AncestorIndex` : pointeurs de saut de Myers (un mot + une profondeur par nœud), `lca`, `at_depth`, `highest` (plus haut ancêtre d'un prédicat monotone, `Outcome`), exposé par `tower/tower.hpp`. |
| `src/points/` (nouveau module, dépend de `tower`) | `points.hpp` (public : `qualification`, `PointTree`, `PointHierarchy`, `PointsStats`, `hang`, `hang_qualified`), `internal.hpp`, `incidences.cpp`, `qualify.cpp`, `exact.cpp`, `hang.cpp`, `settle.cpp`, `point_tree.cpp`. |
| `src/core/reasons.def` | `points_invariant` (`invariant_violated`, `points`) en fin de table ; copie gravée `tests/core/status_test.cpp` (30 raisons, plancher 101) au même commit. |
| Table des modules | `api` dépend désormais de `points` (`docs/ARCHITECTURE.md` § 2 et `cmake/modules.cmake`) ; texte du § 2 mis à jour. |
| `src/api/` | `OutputKind::points`, `PointsRequest`, `Product::points()`, `compute` (arbre K seul au masque 7035, puis `points::hang`, étage `output`), `write_points.cpp` (écrivain `MHGP11PT` v1 + manifeste), `manifest_head` partagé ; `tree_k_sha256` V2 identique à `full` et `supports`. |
| `cli/mhgp11.cpp` | `--sortie=points` admis (plat reste refusé) ; ligne de succès `counts` : `nodes`, `levels`, `plateaus`, `blocks`, `delayed`. |
| `bench/mhgp11_formats.py` | `read_points` (bibliothèque standard, `exact=True` : plancher, drapeau strict, ordre des plateaux, égalité de date des sites stricts, par `radical_sign` écrit à neuf), `check_directory` pour `points`, comptes du manifeste. |
| Docs | `SORTIES.md` (§ 3 étape 6 : décision $K=n$ ; § 7 : `MHGP11PT` v1 **normatif** ; § 8 : `counts` de points ; § 11), `PROVENANCE.md` (section S9, port décision par décision, sha256 des sources), `HIERARCHIE_POINTS.md` § 9 (alignement fait, F13 rejouée, port natif), `ARCHITECTURE.md`, `README.md`. |

**Format `MHGP11PT` v1** (normatif, § 7 de `SORTIES.md`) : en-tête de 144 octets (magie + 17 `u64` : version, bits,
k, m, kappa, n, L, W, N, P, Bk, cinq décalages, taille) ; `SITES` ; `LEVELS` (`rank u32[L]`, puis `num` et `den`
`u64[W·L]`, niveau après niveau, valeurs NON réduites, rang 0 toujours présent) ; `NODES` (`parent`, `rank`) ;
`HANGING` (`t`, `M`, `Q`, `owner`, `floor` en `u32[n]`, `strict u8[n]`) ; `TREE` (`plateau_t/M/Q u32[P]`,
`block_plateau`, `block_parent u32[Bk]`, `site_block`, `site_plateau u32[n]`). Manifeste `counts` : `sites`, `nodes`,
`qualification`, `kappa`, `levels`, `plateaus`, `blocks`, `root_blocks`, `delayed`, `strict`.

**Contrat $K=n$** : `--sortie=points` refuse $K\geq n$ dès $K\geq 2$ (`parameter_out_of_range`, étape compute, ni
`D` ni `D.pending`) ; $K=1$ admis à tout $n\geq 1$ ($m(1)=1$). Écrit dans `SORTIES.md` § 3 (étape 6).

## Choix

- **Port décision par décision** (table de `PROVENANCE.md`, section S9) : incidences = populations des boules fortes
  ($p+q_{\min}\leq K$) de `WindowAttachment`, nœud de rattachement (pas de descente) ; à $K=1$, la feuille du site.
  Qualification $m\leq K$ : rang du nœud ; $m=K+1$ : fusions à leur naissance, naissances à la $m$-ième paire
  (nœud, site) de première incidence. Départs qualifiés, $t$ minimal, $p_1$ = nœud de la **première** incidence de
  rang $t$ (même règle que `first_points`). Rival : le **premier**, dans l'ordre des incidences, de terme strictement
  maximal — sémantique exacte de la boucle `candidate` de Python, donc mêmes rangs $(M,Q)$ publiés.
- **Aucune décision flottante.** Deux racines contre deux : encadrement entier (table, $\pm 2$ sur $2^{64}$) puis
  `num::sqrt_cmp2` (port de `sqrt_cmp2`, toujours décidé) ; trois contre trois : `RootTable::sign` (repli
  `RadicalSum`). Plancher : proposition binary64 (dichotomie sur les niveaux approchés, comme `searchsorted`), puis
  galop et dichotomie **exacts**, puis certificat $P(r)$ et non $P(r+1)$ sur tout le catalogue (le flottant propose,
  l'entier décide). Égalités de **rangs identiques** décidées sans arithmétique (mêmes rangs, mêmes niveaux) : c'est
  ce qui a fait tomber l'arbre de points de ~500 ms à ~15 ms sur les trames (les groupes stricts comparaient des dates
  identiques par le repli).
- **Table des racines** : allouée sur tout `Cat_K` (16 octets par rang) mais remplie seulement pour les rangs des
  nœuds, le rang 0 et les rangs de départ de toutes les incidences (`fill_ranks` en parallèle) ; racine d'un rang
  absent calculée à la demande (`root_of`) pour le plancher.
- **Parallélisme à positions fixes** : tri des lignes d'incidences, paires de qualification (deux passes), $m$-ième rang
  par nœud, pendaison par site (brouillons par fil, admis : 2 × plus longue ligne × 8 octets) ; arbre de points
  séquentiel. Sorties identiques sans Pool et à W1, W2, W4 (unitaire) et W1/W4 (CLI).
- **Admission** : chaque étage admet avant allocation ; le produit exact restant au budget est vérifié par
  `mhgp11_points_unit_budget` (octets = formule des colonnes).
- **Sonde** `tests/points/points_probe.cpp` sans dépendance au module `api` (paramètres recopiés) pour que la
  construction `-DMHGP11_MODULES=points` des mutants se lie.
- `hang_qualified(tree, m)` ($1\leq m\leq K+1$) sert les fixtures du banc à $m=1$ ; le produit n'emploie que $m(K)$.

## Portes (lignes exactes, durées locales Release u21, `-j4`)

Construction : `/workspaces/E-HGP/build/v11-persist/b21-s9/`, `-DMHGP11_MODULES="core;api;points;cli"`, aucun
avertissement. Suite `fast` de ces quatre unités (`-L fast -LE mutant`, `-j4`) : **151/151 vertes, 108,8 s**.

| Porte | Ligne gravée (`LINE`) | Durée locale |
| --- | --- | --- |
| `mhgp11_points_unit_{ancestors,determinism,refusals,budget}` + `_inventaire` | `mhgp11_test_ok` (180 640 contrôles au total) | 0,15 / 0,32 / 0,05 / 0,07 / 0,02 s |
| `mhgp11_points_fixtures` (+ `_opt`) | `points_fixtures_verdict conforme fixtures=13/13 nuages_k1=30 comparaisons=7000` | 1,1 s (1,1 s) |
| `mhgp11_points_oracle` (+ `_opt`, labels oracle fast) | `points_oracle_verdict conforme nuages=164 ordres=1086 comparaisons=7088 retardes=3022 plateaux=5720 replis=12` (planchers 150 / 4 000 / 500 / 10 replis exacts) | 11,5 s (17,2 s) |
| `mhgp11_cli_points` (+ `_opt`) | `cli_points_verdict conforme cas=52 appels=267 refus=4 retardes=105 plateaux=242` | 2,5 s (2,4 s) |
| `mhgp11_points_scale8000` (+ `_opt`) | `cli_points_verdict conforme cas=1 appels=5 refus=0 retardes=7801 plateaux=7871` | 20,1 s (14,0 s) |
| `mhgp11_points_scale16000` (+ `_opt`) | `… retardes=15676 plateaux=15751` | 47,5 s (67,3 s) |
| `mhgp11_points_scale32000` (+ `_opt`) | `… retardes=31491 plateaux=31458` | 140,6 s (140,7 s) |
| `mhgp11_points_lidar_ng00_k5` (+ `_opt`) | `… retardes=34509 plateaux=37684` | 71,9 s (71,4 s) |
| `mhgp11_points_lidar_ng01_k5` (+ `_opt`) | `… retardes=31286 plateaux=33496` | 57,4 s (59,8 s) |
| `mhgp11_points_lidar_ng02_k5` (+ `_opt`) | `… retardes=41792 plateaux=43148` | 71,9 s (76,7 s) |
| `mhgp11_points_vs_python` (long, numpy ; pas de LINE, planchers) | sortie : `points_vs_python_verdict conforme nuages=407 ordres=3239 sites=114560 retardes=82378 plateaux=118325` | 249,3 s |
| `mhgp11_points_vs_python_lidar_ng00_k5` (lidar long) | `… nuages=5 ordres=3 sites=39899 retardes=34513 plateaux=37689` | 42,6 s |
| `mhgp11_points_vs_python_lidar_ng01_k5` (lidar long) | `… sites=35565 retardes=31290 plateaux=33501` | 34,9 s |
| `mhgp11_points_vs_python_lidar_ng02_k5` (lidar long) | `… sites=45859 retardes=41796 plateaux=43153` | 47,8 s |
| `mhgp11_cli_contract` (modifiée : le cas « sortie points » devient « z avec points », 64 refus inchangés) | `cli_contract_verdict conforme refus64 temoins3` | 0,4 s |
| `mhgp11_core_unit_reasons`, `mhgp11_style` | `mhgp11_test_ok`, `style_ok fichiers=517` | 0,02 s, 1,1 s |

Toutes les portes Python neuves sont en bibliothèque standard (sauf `points_vs_python`, label `long`, numpy) et ont
tourné sous `python3 -S -B` et, par leurs jumelles `_opt`, sous `-O`. `mhgp11_points_vs_python` est l'identité
**exacte** (rangs `t`, `M`, `Q`, propriétaire, plancher, strict, plateaux, blocs, entrées) contre
`hang_margin_radius(order, m(K))` + `tower_point_tree` sur le même `MHGP11PH`, $K=1..5$, $m\in\lbrace 1,m(K),K+1\rbrace$,
400 nuages aléatoires, témoins, uniformes 300 / 2 000 / 8 000, et les trois trames à K5 (sites identiques un par un).

Mutants (`tests/mutants/points.json`, plancher 5) : `run_mutants.py --check` → `manifeste_ok module=points mutants=5
plancher=5` ; campagne (`--jobs 2 --build-jobs 2`, 10 min 4 s) → `mutants_ok module=points mutants=5 tues=5
dont_signal=0 dont_delai=0 dont_construction=0 plancher=5` ; juges : `qualification_decalee`, `sans_marge`,
`coupe_ouverte`, `m_k1_deux` par `mhgp11_points_fixtures`, `marge_carree` par `mhgp11_points_oracle` (verdict `code`).
Rejouée sur le commit `451301787` (après le raccourci des rangs égaux), 16 h 49 – 16 h 59 UTC : même ligne
`mutants_ok module=points mutants=5 tues=5 dont_signal=0 dont_delai=0 dont_construction=0 plancher=5`, mêmes juges. `--check` vert aussi pour `tower`
(138), `api` (22), `cli` (28), `core` (78), `num` (57), `supports` (15), `io` (22). `check_docs` : 156 lignes, code 1,
aucune ligne nouvelle (base S8 : 156).

## Mesure locale du coût (codespace, Release u21, sonde `mhgp11_points_probe`, K = 5, deux prises)

Arbre K et table des racines **à part**, comme demandé. Temps en ms (indicatifs : les références se prennent sur G4).

| Trame | W | domaine | arbre K (`tree`) | `attach` | incidences | qualification | **table des racines** | **pendaison** | **arbre de points** | points total |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| ng00 (39 885 sites) | 1 | 15 455–15 577 | 7 012–7 495 | 209–276 | 54–60 | 47–60 | 390–444 | 433–543 | 15 | 952–1 133 |
| ng00 | 4 | 5 019–5 519 | 3 113–3 555 | 216–238 | 51–57 | 49–88 | 196–296 | 160–171 | 17–34 | 509–635 |
| ng01 (35 551) | 1 | 12 007–12 607 | 4 951–5 502 | 149–172 | 35–44 | 64–83 | 323–410 | 380–425 | 12–13 | 872–938 |
| ng01 | 4 | 4 434–4 659 | 2 466–2 847 | 165–208 | 37–46 | 33–65 | 124–150 | 95–132 | 15–25 | 322–429 |
| ng02 (45 845) | 1 | 14 952–15 277 | 5 697–6 341 | 194–256 | 56–92 | 73–102 | 503–611 | 587–610 | 17 | 1 304–1 389 |
| ng02 | 4 | 5 092–5 629 | 2 443–3 337 | 197–214 | 51–60 | 55–83 | 197–199 | 195–277 | 18–28 | 539–655 |

Comptes (identiques à W1 et W4) : ng00 1 705 735 incidences, 560 399 racines remplies (sur 1 085 776 niveaux),
34 509 sites retardés, 316 776 évaluations de plancher et de remontée, 425 467 décisions par la table, **0 repli
exact**, 37 684 plateaux, 12 493 blocs ; ng01 1 416 180 / 467 635 / 31 286 / 33 496 plateaux ; ng02 1 807 322 / 576 543 /
41 792 / 43 148 plateaux. Lecture : la pendaison et l'arbre de points coûtent 0,3 à 0,7 s à W4 (0,9 à 1,4 s à W1),
dominés à parts égales par la table des racines (~330 ns par rang, ~0,5 M rangs) et la pendaison par site ; l'arbre de
points (séquentiel) est négligeable depuis le raccourci des rangs égaux. Leviers non pris (aucun besoin avant G4) :
racines à largeur fixe (division 6 mots par 3, une itération de Newton, cf. rapport S8), remplissage limité aux rangs
réellement comparés, recherche du plancher moins bavarde (≈ 9 évaluations par site retardé, remontée comprise).

## Écarts

1. Trois des douze fixtures du banc étaient écrites pour la **marge en niveau carré** (deux triangles à $m=1$, cinq
   points à $m=3$, continuité $\lbrace 0,2,4\rbrace$) : la seule règle native est en rayon ; elles sont rejouées sous
   la règle en rayon avec attendus recalculés et gravés (F2 donne encore $\lbrace 0,1\rbrace\mid\lbrace 4,5\rbrace$ ;
   F5 : le médian entre exactement au rayon 2 000 pour $d=0$ et $d=1$). Les quatre fixtures arithmétiques passent par
   la sonde `--arith` (`num::compare_dates`, `num::sqrt_cmp2`) ; F13 ($K=1$, liaison simple) s'ajoute : 13 faits.
2. `qualify_general` ($m>K+1$) et les règles témoins (`core`, `cover`, `first`, marges carrées) ne sont **pas
   portées** : `hang_qualified` admet $1\leq m\leq K+1$, sinon `parameter_out_of_range` ; l'oracle compare $m(K)$ et
   $m=1$ (le banc comparait aussi $k+2$).
3. `bench/points_radius.py` est modifié (ajout de `qualification` en fin de fichier) : sha256 `457b997f…` →
   `59a6adca…`, consigné dans `PROVENANCE.md` (section S9) ; aucune définition portée par S8 ne change.
4. Fichiers hors de la liste de la spécification : `src/points/internal.hpp`, `exact.cpp`, `settle.cpp`,
   `tests/points/points_fixtures.py` ; `tests/cli/cli_points.py` sert aussi les portes `mhgp11_points_scale*` et
   `mhgp11_points_lidar_*` (enregistrées seulement si la cible `mhgp11_cli` existe).
5. Couverture du **repli exact** dans `points` mince : 12 décisions sur toute la porte oracle (plancher 10), 0 sur les
   trames ; le repli lui-même est jugé par les portes de `num` (S8). Le raccourci des rangs égaux ne change aucune
   décision (mêmes rangs ⟹ mêmes niveaux), mais il retire du chemin exact les égalités triviales.
6. La table des racines réserve 16 octets par niveau du catalogue (17 Mo à K5 sur les trames ; 81 Mo attendus à K10
   sur ng00) bien que seuls ~50 % des rangs soient remplis.
7. Construction à `-j4` (règle des quatre cœurs), pas `-j6`. Ni ASan/UBSan, ni u18/u24, ni TSan, ni campagnes de
   mutants des autres modules, ni suite `fast` entière (méthode allégée).
8. `mhgp11_cli_contract` : le cas d'options « sortie points » (refus tant que S9 manquait) est remplacé par « z avec
   points » (option propre à `plat`), le nombre de refus (64) est inchangé.

## À faire tourner sur G4

- Matrice : `gcc_release` (u18 : vérifier que les lignes gravées des portes `points` et `mhgp11_cli_points` tiennent
  telles quelles en u18 — nuages et trames dans le domaine ; sinon graver par profil), `bits24` (mots de niveau W = 4,
  lecteur), `gcc_asan_ubsan` (u24 : `points_unit`, fixtures, oracle, `cli_points`), **`gcc_tsan`** (boucles
  parallèles de `points` : tri des lignes, paires de qualification, `nth_element`, pendaison par site, remplissage de la
  table), `poison`, `clang_release`, et la configuration `mutants` (manifeste `points` : 5 mutants, plus `--check` de
  `tower`, `api`, `cli`, `core` touchés par la tranche).
- `release_long` avec paquets épinglés (numpy) : `mhgp11_points_vs_python` (≈ 250 s local) et
  `mhgp11_points_vs_python_lidar_ng0{0,1,2}_k5` (35 à 48 s local) ; sans numpy sur la VM, ces portes échouent au
  lancement.
- Échelle et trames (labels `scale8000/16000/32000`, `lidar`) : `mhgp11_points_scale*`, `mhgp11_points_lidar_*_k5`
  (jusqu'à 141 s local pour 32 000 : sous l'échéance de 600 s par porte de `gcc_release`).
- Déterminisme à W48 : `cli_points.py --uniform=8000 --k=5 --fils=1,8,48` (non enregistré : à ajouter à la matrice
  ou jouer à la main) ; mesure du coût à W48 et à K10 (table des racines sur ~5 M niveaux) avec la sonde
  `mhgp11_points_probe --input=… --k=… --workers=48`.
