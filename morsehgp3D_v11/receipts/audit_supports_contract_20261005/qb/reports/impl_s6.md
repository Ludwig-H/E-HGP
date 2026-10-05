# Tranche S6a — module `supports` : $\mathcal{Q}_b$, fermeture et comptes du lemme G

4 octobre 2026, rapport écrit à partir de 21 h 33 UTC (heure lue par `date -u`). Rôle : implémentation native de la
première partie de S6, en parallèle du contrat L0. **GCP non utilisé.** Rien n'est commité ni indexé : les fichiers
sont dans le worktree `build/v11-impl-s6` (détaché à `f98aeed67`), les constructions et essais dans `/tmp/v11-s6/`.

```text
phase=exploration_v11_hors_registre
backend=cpu_reference
profile=quantized_u21_input_only   (u24 et ASan+UBSan Debug joués localement sur le module ; u18 non joué)
public_status=not_claimed
```

Autorités suivies, dans cet ordre : `DECISIONS_UTILISATEUR.md` (nom `kparties_reliees`, aucun compte stocké),
`CRITIQUE_ET_PLAN_REVISE.md`, `SPECIFICATION_FINALE.md` (§ 2.5, 2.6, 2.9, 3.3, 4, 7.3, 7.4, 8.3, 8.5, 9.1). Lus en
plus : l'audit `de4ab58a8` (`receipts/audit_supports_followup_20261004/qb`, publié sur `main` pendant la tranche :
« extraire les supports avant la fermeture zêta », « ne pas filtrer Q_b par les cofaces »), et le contrat L0 en
cours (`build/v11-impl-l0` : `docs/SORTIES.md`, `MATHEMATIQUES.md` § 10.6–10.7, `reference/hgp11_ref/supports.py`),
lu seulement pour aligner les noms. L'assemblage (`hierarchy.cpp`) n'est pas écrit : il attend `WindowAttachment` (S3).

## 1. En bref

- **Module `supports`** (table normative et copie CMake, entre `tower` et `points`, dépend de `tower`) :
  `ball_supports` rend $\mathcal{Q}_b$ de toute boule du catalogue, énuméré sur **toute** la coquille, dans l'ordre
  (arité, `SiteIdx`) avec $S^*$ en tête, et sa fermeture $N_0..N_m$ ; `counts.hpp` donne les comptes du lemme G par une
  table `constexpr` de binômes. Deux raisons nouvelles en fin de table : `support_shell_capacity`
  (`unsupported_degeneracy`) et `supports_invariant` (`invariant_violated`).
- **Port explicite** de `mark_supports` et `closure_counts` (`bench/catalogue_euler.hpp`, sha256 `f293df6e…`),
  épinglé dans une section neuve de `docs/PROVENANCE.md` et dans `src/supports/source_pins.json`. Les supports sont
  écrits **avant** la fermeture (garde de l'audit `de4ab58a8`).
- **Portes locales vertes** (u21 Release) : 12 groupes unitaires sur les fixtures 1, 2, 3, 9, 10, 11, 12 du § 2.9,
  la coquille mixte de l'audit (1 paire, 4 triangles, 13 tétraèdres) et les bornes 24/25 ; sphere50 (code 2,
  `support_shell_capacity`, aucune ligne de boule) ; juge Fraction sur **toutes** les boules de 40 petits nuages
  (9 067 boules, 4 520 coquilles étendues) ; juges d'échantillon à 8 000 points et sur ng00 à K5 ; registres de la
  forêt d'ordre K égaux aux comptes sur 8 000, 16 000, 32 000 points et sur ng00, ng01, ng02 à K5.
- **Six mutants tués** (code exact, aucun par signal) par `run_mutants.py`.
- **Coût mesuré** (local, indicatif, un fil) : `ball_supports` seul sur toute $W_5$ des trames ; les prédicats ne
  touchent que les coquilles étendues (141, 81 et 354 boules ; $m\leq 5$) : voir § 6.

## 2. Fichiers

Chemins relatifs à `morsehgp3D_v11/` dans `build/v11-impl-s6`.

| Fichier | État | Contenu |
| --- | --- | --- |
| `docs/ARCHITECTURE.md` | modifié (1 ligne) | ligne `supports` de la table du § 2, **mot pour mot celle de L0** (comparée par `diff`) |
| `cmake/modules.cmake` | modifié | `supports` après `tower`, `MHGP11_DEPS_supports tower`, `api` dépend aussi de `supports` ; identique à L0 |
| `src/core/reasons.def` | modifié (fin de table) | `support_shell_capacity`, `supports_invariant` |
| `tests/core/status_test.cpp` | modifié | copie gravée de la table : 27 raisons, dernière `supports_invariant`, plancher 92 |
| `src/supports/supports.hpp` | nouveau | en-tête public : `Support`, `support_capacity`, `kMaxSupports` (12 926), `closure_words`, `check_shell`, `SupportLedger`, `BallSupports`, `ball_supports`, `ball_shape` |
| `src/supports/counts.hpp` | nouveau | constantes (`kMaxShell` 24, `kMaxOrder` 12, `kMaxInterior` 11), table de Pascal `u64` sous `static_assert`, `Closure`, `Shape`, `make_shape`, `BallCounts`, `ball_counts`, `support_cofaces`, `support_gabriel_cofaces` |
| `src/supports/counts.cpp` | nouveau | `make_shape`, `ball_shape`, `ball_counts` |
| `src/supports/enumerate.cpp` | nouveau | `ball_supports`, port de `mark_supports` (`enumerate`) et de `closure_counts` (`zeta_or`, `Filler::counted`), `check_shell` |
| `src/supports/module.cmake` | nouveau | `enumerate.cpp counts.cpp` |
| `src/supports/source_pins.json` | nouveau | épingle de `bench/catalogue_euler.hpp` (commit `462dca187`, sha256) |
| `tests/supports/tests.cmake` | nouveau | portes du § 4 |
| `tests/supports/supports_test.cpp`, `supports_support.hpp` | nouveaux | portes unitaires et outils de fixture |
| `tests/supports/supports_probe.cpp` | nouveau | sonde JSON canonique, mesure sur $W_K$, registres de la forêt |
| `tests/supports/sample_judge.py` | nouveau | juge Fraction, bibliothèque standard, sans `assert` |
| `tests/mutants/supports.json` | nouveau | six mutants |
| `docs/PROVENANCE.md` | section neuve en fin | « Module supports : Q_b et comptes du lemme G (tranche S6a) » |

Non touchés : `docs/SORTIES.md`, `MATHEMATIQUES.md`, `README.md`, `tower`, le catalogue, les sondes de banc et leurs
lignes exactes, `MHGP11FUL1`, `MHGP11PH`, `docs/implementation_status.toml`. Aucun octet de sortie existant ne change :
seules deux raisons sont ajoutées **en fin** de table (les codes des raisons existantes sont inchangés).

## 3. Choix d'implémentation

**API (esquisse du § 4, adaptée aux décisions du 4 octobre).**

```cpp
namespace mhgp11::supports {
struct Support { std::array<SiteIdx, 4> sites; u8 arity; };          // sites croissants, kNone au-dela
struct BallSupports { u32 count; Closure closure; };                 // |Q_b| et N_0..N_m
Result<BallSupports> ball_supports(const FullDomain&, BallIdx, std::span<Support> out, std::span<u64> scratch,
                                   SupportLedger* = nullptr) noexcept;
Result<Shape> ball_shape(const FullDomain&, BallIdx, Order k) noexcept;   // ou make_shape(p, m, q, k)
Result<BallCounts> ball_counts(const Shape&, const Closure&) noexcept;    // kparties_reliees, compressed_parts,
                                                                          // strict_traces, cofaces, gabriel_cofaces
u32 support_cofaces(const Shape&, u32 arity) noexcept;                    // C(p+m-a, K+1-a)
u32 support_gabriel_cofaces(const Shape&, u32 arity) noexcept;            // C(m-a, t+1-a)
Outcome check_shell(u32 m) noexcept;                                      // seul controle du plafond 24
}
```

- **Coquille régulière** ($m=q$) : $\lbrace S^*\rbrace$, $N_j=[j=q]$, ni sphère ni prédicat (mesuré : 0 prédicat sur
  789 745 coquilles régulières de ng00).
- **Coquille étendue** : `check_shell(m)` ; tampons contrôlés **avant** toute écriture (brouillon
  `closure_words(m)` = $2^{m-6}$ mots, `shell.size() == m`) ; sphère `Sphere::through` de l'arité $q$ depuis $S^*$,
  niveau **égal** à celui du catalogue ; paires (`is_midpoint`), triplets (`strictly_acute` puis orientation nulle du
  centre), quadruplets (`strictly_inside`) de positions de $U_b$ ; premier support = $S^*$ (arité et sites) ; zêta en
  OU, puis $N_j$. Tout écart : `supports_invariant`. Plus de `out.size()` supports : `supports_invariant` (le
  passage de comptage de l'assemblage peut donc écrire dans un brouillon de `support_capacity(m)` cases, et le
  passage de remplissage directement dans la tranche exacte du CSR).
- **Supports extraits avant la fermeture** : chaque support est écrit dans `out` au moment où il est trouvé ; la
  fermeture travaille ensuite dans le brouillon (le cube rend 6 supports, pas les 177 parties fermées ; porte
  `mhgp11_supports_unit_cube`). $\mathcal{Q}_b$ ne dépend pas de $K$ : les tétraèdres du cube restent à $K=1$ avec
  0 coface (même porte).
- **Un seul contrôle du plafond** (`check_shell`), appelé par `ball_supports`, `make_shape` et par les pré-passes
  d'un appel entier (la sonde l'applique à toute la sélection avant tout calcul ; l'assemblage le fera sur $W_K$).
  `closure_words` est totale (0 au-delà de 24) : sans le contrôle, le refus devient déterministe
  (`supports_invariant`), jamais un décalage hors domaine — c'est ce qui rend le mutant `coquille_sans_plafond`
  causal et tué par code.
- **Comptes** : rien n'est stocké ; `Closure` et `Shape` ne se construisent que contrôlés (constructeurs privés).
  `ball_counts` vérifie que la fermeture est celle d'une coquille de cette forme ($N_j\leq\binom{m}{j}$, $N_j=0$ sous
  $q$, $N_q\geq 1$, $N_m=1$) : aucun compte ne peut déborder. Table `u64` de $\binom{a}{b}$ pour $a\leq 35$ ;
  `static_assert` : tout binôme lu (bas $\leq 13$, ou haut $\leq 24$) tient en `u32` ; $\binom{35}{13}$,
  $\binom{24}{12}$ gravés. La somme des cofaces est bornée par $\binom{p+m}{K+1}$ (Vandermonde) et gardée en `u64`.
- **Aucun flottant, aucune exception, aucune allocation** dans le module : `out` et `scratch` appartiennent à
  l'appelant (un brouillon par fil) ; le domaine est lu en partage (porte `concurrency` : 4 fils × 50 appels).
- **Registre** `SupportLedger` (boules, régulières, étendues, supports, tests de milieu, d'angle, d'orientation,
  d'inclusion), mis à jour sur succès seulement : sert à la mesure et aux portes, jamais à une décision.

## 4. Portes enregistrées (`tests/supports/tests.cmake`) et jouées localement

Constructions : u21 Release (`-DMHGP11_MODULES="core;supports"`, `-j2`) ; ASan+UBSan Debug et u24 Release limités à
`supports`. Toutes les portes Python sont doublées sous `-O` par l'aide `mhgp11_python_gate` ; les scripts ont aussi
été joués à la main sous `python3 -S -B` et `python3 -S -B -O` (sorties identiques).

| Porte | Labels | Ligne exacte ou critère | u21 | u24 | ASan |
| --- | --- | --- | --- | --- | --- |
| `mhgp11_supports_unit_{square,right_triangle,growth,cube,octahedron,circle,lines,mixed,shell_bound,refusals,constants,concurrency}` + `_inventaire` | unit fast | planchers = contrôles mesurés (870 au total) | vert | vert | vert |
| `mhgp11_supports_shell_capacity` | unit fast | code 2, `supports_probe_verdict refus support_shell_capacity` | vert | vert | vert |
| `mhgp11_supports_judge_small` (+ `_opt`) | oracle fast | `supports_sample_judge_couverture boules=9067 etendues=4520 multiples=1545 tetraedres=1554 brutes=9066` | vert (154 294 contrôles) | vert | vert |
| `mhgp11_supports_sample_judge_scale8000` (+ `_opt`) | scale8000 | `supports_sample_judge_couverture boules=200 etendues=0 multiples=0 tetraedres=43 brutes=200` | vert | — | — |
| `mhgp11_supports_sample_judge_grid8000` (+ `_opt`) | scale8000 | `supports_sample_judge_couverture boules=300 etendues=149 multiples=49 tetraedres=178 brutes=300` | vert | — | — |
| `mhgp11_supports_sample_judge_lidar_ng00_k5` (+ `_opt`) | lidar | `supports_sample_judge_couverture boules=400 etendues=200 multiples=3 tetraedres=28 brutes=400` | vert | — | — |
| `mhgp11_supports_registers_uniform8000_k5` (+ `_opt`) | scale8000 | `supports_probe_verdict conforme k=5 n=8000 boules=597998 choisies=395667 etendues=0 supports=395667 multiples=0 coquille_max=4 entree=3be1324202d28360` | vert | — | — |
| `mhgp11_supports_registers_grid8000_k5` (+ `_opt`) | scale8000 | `supports_probe_verdict conforme k=5 n=8000 boules=468514 choisies=337517 etendues=125064 supports=502847 multiples=55729 coquille_max=14 entree=5a5697d925f68c1e` | vert | — | — |
| `mhgp11_supports_registers_uniform16000_k5` (+ `_opt`) | scale16000 | `supports_probe_verdict conforme k=5 n=16000 boules=1233046 choisies=819004 etendues=0 supports=819004 multiples=0 coquille_max=4 entree=3469c29b4c34b7e3` | vert | — | — |
| `mhgp11_supports_registers_uniform32000_k5` (+ `_opt`) | scale32000 | `supports_probe_verdict conforme k=5 n=32000 boules=2536732 choisies=1690045 etendues=0 supports=1690045 multiples=0 coquille_max=4 entree=aea3dec129cef911` | vert | — | — |
| `mhgp11_supports_registers_lidar_ng00_k5` (+ `_opt`) | lidar | `supports_probe_verdict conforme k=5 n=39885 boules=1306696 choisies=789886 etendues=141 supports=789889 multiples=3 coquille_max=5 entree=975c390e5912fabe` | vert | — | — |
| `mhgp11_supports_registers_lidar_ng01_k5` (+ `_opt`) | lidar | `supports_probe_verdict conforme k=5 n=35551 boules=1095926 choisies=652958 etendues=81 supports=652959 multiples=1 coquille_max=4 entree=6b918ef47e9ae56e` | vert | — | — |
| `mhgp11_supports_registers_lidar_ng02_k5` (+ `_opt`) | lidar | `supports_probe_verdict conforme k=5 n=45845 boules=1407885 choisies=832386 etendues=354 supports=832394 multiples=8 coquille_max=5 entree=6e11fa8bc5ee6432` | vert | — | — |
| `mhgp11_supports_registers_lidar_ng0{0,1,2}_k10` | lidar long | code 0, six registres égaux ; ligne à graver après G4 | non joué | — | — |
| `mhgp11_mutants_supports_manifest` (+ `_opt`) | mutant fast | `manifeste_ok module=supports mutants=6 plancher=6` | vert | vert | — |
| `mhgp11_mutants_supports` | mutant long | `mutants_ok module=supports mutants=6 tues=6 dont_signal=0 dont_delai=0 dont_construction=0 plancher=6` | 6/6 tués (code) | — | — |
| `mhgp11_style` (et `test_check_style`) | fast | `style_ok` (430 fichiers, arbre entier) | vert | — | — |

Toute la suite `fast` des unités `core` et `supports` passe en u21 : 121 portes sur 121 (la sentinelle `lidar` du
socle est sautée sans `MHGP11_DATA_DIR`, comme prévu ; dernier passage à 21 h 53 UTC, sur les sources finales). Les
20 portes `scale8000`, `scale16000`, `scale32000` et `lidar` (hors `long`) passent en 539 s, une à une
(`RUN_SERIAL`), avec leurs lignes exactes. Sous ASan+UBSan Debug, les 16 portes rapides du module passent, juge des
petits nuages compris (62 s et 72 s). En u24, les mêmes 16 portes passent, dont la coquille mixte à l'échelle
`kCoordMax/10` du profil 24 bits. u18 n'a pas été joué.

**Ce que jugent ces portes.**
- **Unitaires.** Attendus calculés par la **définition** (boules minimales de toutes les parties de chaque fixture,
  en Fraction, brouillon hors dépôt `/tmp/v11-s6/expect_fixtures.py`), puis recoupés par les formules du lemme G :
  $\mathcal{Q}_b$ exacte et ordonnée, $N_j$, et les sept comptes à chaque $K$ de la fixture. Fixtures : carré
  (K1–K4, dont le double compte à K3), triangle droit (hypoténuse seule, origine orpheline), `growth_ABCZ` (BZ et
  ACZ), cube (4 diamètres + 2 tétraèdres, aucun triangle, 177 parties fermées, tétraèdres gardés à K1 avec 0
  coface, permutation de l'entrée et `PointId` réétiquetés jusqu'à `0xFFFFFFF7`), octaèdre (tiroirs), cercle à
  $B_t$ (immersions $n=4$ et $n=255$, cette dernière dans u18), ligne 0,4,6,8,12, deux losanges (dont la boule de
  niveau 52 : $p=2$, `cofaces` 11), ligne 0,2,4, coquille mixte de l'audit (18 supports gravés, puis la même coquille
  à l'échelle `kCoordMax/10` du profil), 24 sites cocycliques admis (226 supports), 25 refusés, refus de l'API.
- **Sphere50.** 84 sites, $\mathrm{Cat}_2$ de 435 boules ; refus avant tout calcul, aucune ligne de boule publiée.
- **Juge d'échantillon** (`sample_judge.py`). $U_b$ et $I_b$ par force brute sur les $n$ sites (entiers après mise
  au dénominateur commun, fenêtre exacte en $x$) ; $\mathcal{Q}_b$ par poids barycentriques de Gram strictement
  positifs ; ordre par rangs de Morton recalculés (et `SiteIdx` publiés contrôlés) ; $N_j$ par énumération des
  parties, recoupé partie par partie par l'enveloppe **faible** (M1) ; comptes par formules, puis par dénombrement
  brut si $\binom{p+m}{K+1}\leq 20\,000$ ; empreinte de l'entrée lue par la sonde. Le juge rejette une sonde
  corrompue (essai local : une coface, un support retiré, une fermeture, un ordre inversé → code 1).
  `uniform18` n'a aucune coquille étendue dans $\mathrm{Cat}_5$ (comme la porte euler à 8 000) : le plancher de
  coquilles étendues est porté par une **grille cosphérique** de 8 000 points (cases distinctes d'une grille
  $32^3$ au pas 1 000, graine fixe) et par ng00 (200 coquilles étendues tirées).
- **Registres de la forêt** (`--registers`). La sonde parcourt toute $W_K$ et compare six registres de
  `build_forest` (voie séquentielle de référence) aux comptes du module : `classified_cells` $=\lvert W_K\rvert$,
  `classification.combinations` $=\sum\binom{m}{t}$, `replayed_cells` = cellules (`strict_traces` > 0),
  `cells.combinations` = $\sum\binom{m}{t}$ des cellules, `trace_resolutions` $=\sum$ `strict_traces`, naissances
  $=$ `births()`. Arithmétiques distinctes (`bounded_meb` des traces contre prédicats de `num` et zêta) : c'est la
  contre-épreuve $S$ du § 2.6 sommée sur $W_K$, disponible sans le journal S3. Égalité sur toutes les entrées
  jouées, dont la grille (125 064 coquilles étendues, $m\leq 14$, 55 729 boules à plusieurs supports). Les
  $\lvert\mathrm{Cat}_5\rvert$ égalent ceux des portes `mhgp11_catalogue_euler_*`, et les $\lvert W_5\rvert$ des trames
  le registre `work.cells` de l'ordre 5 du reçu `c40_paired` (27 prises par trame, toutes égales).

## 5. Mutants (`tests/mutants/supports.json`)

| Mutant | Motif (unique) | Porte qui le tue | Issue locale |
| --- | --- | --- | --- |
| `triangle_droit_accepte` | `strictly_acute` remplacé par « non dégénéré » (`enumerate.cpp`) | `mhgp11_supports_unit_right_triangle` | tué (code) |
| `drapeau_q4_presentation` | `inside.value()` remplacé par `sphere.q4_presentation_strictly_inside()` | `mhgp11_supports_unit_cube` | tué (code) |
| `arret_premier_support` | garde de capacité de `mark` remplacée par `if (count == 1) return {};` | `mhgp11_supports_unit_square` | tué (code) |
| `fermeture_omise` | `zeta_or` rendu vide (`if (m != 0) return;`) | `mhgp11_supports_unit_square` | tué (code) |
| `cofaces_ordre_k` | $K+1\to K$ dans `ball_counts` et `support_cofaces` (champ `aussi`) | `mhgp11_supports_unit_square` | tué (code) |
| `coquille_sans_plafond` | contrôle de `check_shell` remplacé par `static_cast<void>(m);` | `mhgp11_supports_shell_capacity` | tué (code 3 au lieu de 2) |

Campagne locale : `run_mutants.py --jobs 2 --build-jobs 1`, u21 Release, témoin vert, ligne finale
`mutants_ok module=supports mutants=6 tues=6 dont_signal=0 dont_delai=0 dont_construction=0 plancher=6` (6 min 54 s).
Elle a été jouée deux fois, la seconde sur les sources finales (compte rendu `/tmp/v11-s6/mutants_supports_final.json` :
manifeste sha256 `ab4ef9e7…`, sources sha256 `4d232f7b…`).
`boules_propres_decroissantes` et `supports_tri_pointid` viendront avec l'assemblage (S6b), comme prévu.

## 6. Mesures locales (indicatives, Codespace partagé, un fil ; aucune revendication de temps)

`ball_supports` seul sur toute $W_5$ (sonde `--window`, phase `measure`), puis `ball_supports` + `ball_shape` +
`ball_counts` + agrégats (phase `supports`). Catalogue construit par la voie séquentielle ou à deux fils, hors mesure.

| Trame | $\lvert W_5\rvert$ | régulières : nombre, temps, par boule | étendues : nombre, temps, par boule | milieux, angles, orientations, inclusions | avec comptes et agrégats |
| --- | ---: | --- | --- | --- | ---: |
| ng00 | 789 886 | 789 745 ; 12,3 ms ; 15,6 ns | 141 ; 0,12 ms ; 0,87 µs | 440, 162, 18, 11 | 39,8 ms |
| ng01 | 652 958 | 652 877 ; 10,5 ms ; 16,1 ns | 81 ; 0,07 ms ; 0,88 µs | 246, 84, 0, 1 | 33,0 ms |
| ng02 | 832 386 | 832 032 ; 13,2 ms ; 15,9 ns | 354 ; 0,26 ms ; 0,72 µs | 1 115, 417, 36, 31 | 43,1 ms |

Sorties brutes : `/tmp/v11-s6/measure_k5.txt` (phases `measure` et `supports`, 21 h 43 UTC, charge moyenne de la
machine 5 à 8). Les registres de la forêt (`build_forest` séquentiel) prennent 8,3 s sur ng00 : ils ne servent qu'au
contrôle, jamais au produit.

Lecture : les prédicats ne portent que sur les coquilles étendues ($m\leq 5$ sur les trames, `extended_by_shell`
`{"5": 2}` sur ng00) ; sur ng00, 440 milieux, 162 angles, 18 orientations et 11 inclusions pour toute $W_5$. Le coût est
dominé par le parcours des 0,65 à 0,83 million de boules régulières (une copie de $S^*$ chacune), parallélisable par
tranches dans l'assemblage. À l'opposé, la grille cosphérique de 8 000 points coûte 1,26 million de tests de milieu,
1,63 million d'angles, 0,52 million d'orientations et 1,57 million d'inclusions pour 125 064 coquilles étendues
(276 ms avec les comptes, un fil) : le coût suit les coquilles étendues, pas $n$.

## 7. Écarts à la spécification, et pourquoi

1. **Signature de `ball_supports`.** La spec rendait `Result<u32>` (nombre écrit). Je rends `Result<BallSupports>` =
   (nombre, fermeture $N_j$) : la contre-épreuve $S_{\mathrm{journal}}=\binom{m}{t}-N_t$ et les `cofaces` de la
   boule exigent $N_j$, que l'assemblage n'a aucune autre façon d'obtenir sans refaire la fermeture. Le registre
   `SupportLedger*` facultatif sert la mesure.
2. **`Support` sans champ `cofaces`.** Décision 5 et § 2 de `DECISIONS_UTILISATEUR.md` (« aucun compte n'est
   stocké ») : les comptes sont des fonctions (`support_cofaces`, `support_gabriel_cofaces`, `ball_counts`). La
   structure `Ball` (avec `node`, `role`) n'est pas définie : elle dépend de `BallRole` et de `WindowAttachment`
   (S3). `BallCounts` porte déjà `kparties_reliees` sous ce nom.
3. **Capacité de `out`.** Au lieu d'exiger d'avance `support_capacity(m)` cases, un dépassement pendant
   l'énumération rend `supports_invariant` ; une tranche exacte de CSR suffit donc au passage de remplissage. Le
   brouillon, lui, est contrôlé d'avance (`closure_words(m)`).
4. **`counts.cpp` en plus de `counts.hpp`.** `make_shape`, `ball_shape` et `ball_counts` rendent des `Result` ; ils
   ne sont pas `constexpr`. La table de binômes reste `constexpr` dans l'en-tête.
5. **Juge d'échantillon : trois portes au lieu d'une**, et une grille cosphérique. `uniform18` n'a aucune coquille
   étendue dans $\mathrm{Cat}_5$ ; le plancher « coquilles étendues jugées » ne peut tenir que sur la grille et sur
   ng00. La porte `uniform18` garde le plancher sur les tétraèdres réguliers (43).
6. **Portes ajoutées hors liste** : `mhgp11_supports_judge_small` (juge borné rapide, toutes les boules de petits
   nuages) et `mhgp11_supports_registers_*` (contre-épreuve globale par les registres de la forêt, à 8 000, 16 000,
   32 000 et sur les trames). Elles coûtent peu et jugent des fautes que les fixtures ne voient pas.
7. **Fixture 12** : jugée ici pour $\mathcal{Q}_b$ et les comptes seulement ; « non-naissance au niveau 4, fusion à
   trois enfants, deux graines menant au même nœud » relèvent du rattachement (S3, S6b).
8. **`mhgp11_supports_fraction`** (différentiel contre l'oracle S1 de L0) n'est pas enregistrée : l'oracle n'existe
   pas encore dans ce worktree. La sonde `--all` publie déjà les champs nécessaires ; les clés par support sont
   nommées comme dans `supports.py` de L0 (`cofaces_support`, `gabriel_cofaces_support`). Restent à traduire : sites
   en coordonnées, ordre des supports (L0 : coordonnées ; natif : `SiteIdx`), boules hors $W_K$ à filtrer.
9. **`tests/core/status_test.cpp`** modifié : la copie gravée de la table des raisons l'exige (« une raison nouvelle
   s'ajoute en fin de table ET ici »). L'intégration avec `environment_selftest` (S5) devra fixer l'ordre final.

## 8. Ce que le contrat L0 devra documenter

- `docs/SORTIES.md` (§ comptes dérivés) : les noms C++ (`ball_counts` → `BallCounts`, `support_cofaces`,
  `support_gabriel_cofaces`) des quantités `cofaces` (support) et `gabriel_cofaces` (support) ; $N_j$ est publié par
  l'API comme `Closure` (jamais stocké) ; un dépassement de capacité de sortie est `supports_invariant`.
- `docs/SORTIES.md` (raisons) : `support_shell_capacity` est émise par `check_shell`, **seul** contrôle du plafond,
  avant tout calcul, pour la boule (`ball_supports`) comme pour l'appel entier (pré-passe) ; `supports_invariant`
  couvre : $S^*$ dégénéré, niveau recalculé différent de celui du catalogue, premier support différent de $S^*$,
  tampons trop courts, champs du catalogue incohérents, fermeture étrangère à la forme.
- `MATHEMATIQUES.md` § 10.6–10.7 : rien à changer (lemmes F et G identiques au code) ; on peut citer la
  contre-épreuve globale par les six registres de `build_forest` (§ 4) comme réalisation sommée du contrôle
  $S_{\mathrm{journal}}=\binom{m}{t}-N_t$ en attendant le journal.
- `README.md` : le module `supports` existe (S6a) ; `ARCHITECTURE.md` (texte de L0 « modules planifiés ») : retirer
  `supports` de la liste des modules sans dossier.
- Coquilles étendues mesurées (à requalifier sur G4) : $\lvert W_5\rvert$ = 789 886 / 652 958 / 832 386 ; coquilles
  étendues de $W_5$ = 141 / 81 / 354 ; boules à plusieurs supports = 3 / 1 / 8 ; $m_{\max}$ = 5 / 4 / 5.

## 9. À jouer sur G4 (session gardée ultérieure)

- Matrice : GCC u21 et u24 Release (suite `fast` complète), u18, ASan+UBSan, **TSan** sur
  `mhgp11_supports_unit_concurrency` (et la sonde), poison ; Clang si disponible.
- Labels `scale8000`, `scale16000`, `scale32000`, `lidar` (portes du § 4) ; **K10** :
  `mhgp11_supports_registers_lidar_ng0{0,1,2}_k10` (label `long`), puis graver leurs lignes.
- Campagne `mhgp11_mutants_supports` (label `long`) ; rejeu de `mhgp11_mutants_core` (table des raisons modifiée).
- Mesure : `ball_supports` sur $W_5$ et $W_{10}$ des trames à W1 et W48 (phase `measure`), avant de décider du
  découpage parallèle de l'assemblage.

## 10. Ce que je n'affirme pas

- Aucune qualification G4 ; les temps du § 6 sont des mesures locales sur un Codespace partagé.
- La complétude de $\mathcal{Q}_b$ n'est jugée que là où le juge recalcule $U_b$ (échantillons, petits nuages) ; les
  registres de la forêt jugent des sommes, pas chaque boule (le contrôle par boule viendra avec le journal S3).
- Le juge d'échantillon s'appuie sur le lemme F pour $N_j$ (parties contenant un support), recoupé par l'enveloppe
  faible pour $m\leq 16$ ; le dénombrement brut des cofaces s'appuie sur M1.
- L'assemblage (postordre, tri des boules, CSR, `build_support_hierarchy`) n'est pas écrit.

## Annexe : empreintes des fichiers livrés (worktree `build/v11-impl-s6`, 21 h 50 UTC)

Chemins relatifs à `morsehgp3D_v11/`, sha256 des octets du worktree (rien n'est commité ni indexé).

| Fichier | sha256 |
| --- | --- |
| `cmake/modules.cmake` | `c60dd9c3737995f2eced7d2769ad98f6ac6d68a05699c24752ba75bc21554602` |
| `docs/ARCHITECTURE.md` | `6212482760bb27495574699930d161b881db21109f50b93de648fbff40d3b108` |
| `docs/PROVENANCE.md` | `99bcadb96d4a8e626dba4bf1db87b2b1c3e638f45a4d659424989d70dd2b8297` |
| `src/core/reasons.def` | `a0d38d86466f797616395500f9071a332b0e8e9291ce2b045c2cbccdd9632c75` |
| `tests/core/status_test.cpp` | `ecc22534892f6f7023c4d9e4697559551a2920650cfc4691175a7a95f30ef6be` |
| `src/supports/counts.cpp` | `98f7a8c95e2c3e752e6b9d5cb2ef47314da79d5e821932735c29ad4ff67bd1fb` |
| `src/supports/counts.hpp` | `3fbc70e28df0901964cf366c371684e043b6feb78da12b25cedf3f648ea472d3` |
| `src/supports/enumerate.cpp` | `c539c51d61b615b86dc481e62ef2353f5adaef88c4603351b305590bbc8c3435` |
| `src/supports/module.cmake` | `95342e4228ddb1f92d2ccdf487f2b4fc2e08c7c7804ffecb6db2e91fdac8535e` |
| `src/supports/source_pins.json` | `91884d5dc3ae730742d6920c25690e2b511274507fa6cf2b255ee4787c049fd7` |
| `src/supports/supports.hpp` | `b732a08ea6ba89fa175df639a3f55b5100658bb729d7def8b7b5d0437d7282ec` |
| `tests/supports/sample_judge.py` | `1c8285c81213411c3f9e0ed28b45c912e447232be5e246043ec5a4141ca5caad` |
| `tests/supports/supports_probe.cpp` | `2e1dd632478c568b7383d5a8ced8afb1447a85318b893c93bcfd98ff88fcbbab` |
| `tests/supports/supports_support.hpp` | `4863cdf0456418c18be60d3937bd643857652d19134ff3c68b22f4570673cd11` |
| `tests/supports/supports_test.cpp` | `536bc3a3238f8b3be4dd63990113086580558b85ee82af1fcdab1bb5d2d63879` |
| `tests/supports/tests.cmake` | `324cd2d8cb21d376eb72d337f4a582ec2408c4e56ba3da982ab366f33e3e5781` |
| `tests/mutants/supports.json` | `ab4ef9e7445ec3e7f1c062aa8742a6499fca3a6d5fabd49f309fa9584338c72d` |

Intégration avec L0 : `cmake/modules.cmake` et la ligne `supports` de `docs/ARCHITECTURE.md` sont identiques à celles
de L0 (le reste de la table de L0 change d'autres lignes) ; `docs/PROVENANCE.md` n'ajoute qu'une section en fin ;
`src/core/reasons.def` et `tests/core/status_test.cpp` s'ajustent à l'ordre final des raisons (`environment_selftest`
de S5 avant ou après ces deux-ci, selon l'ordre de livraison).
