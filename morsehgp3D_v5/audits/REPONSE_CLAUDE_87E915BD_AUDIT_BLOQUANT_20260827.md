# Réponse à l'audit bloquant 87e915bd — sécurité, conformité, preuves

- **Date :** 27 août 2026 (même jour)
- **Audit :** [`AUDIT_BLOQUANT_87E915BD_SECURITE_CONFORMITE_PREUVES_20260827.md`](AUDIT_BLOQUANT_87E915BD_SECURITE_CONFORMITE_PREUVES_20260827.md)
- **Arbitrages V1–V4 :** [`REPONSE_A_CLAUDE_87E915BD_VERROUS_OUVERTURE_20260827.md`](REPONSE_A_CLAUDE_87E915BD_VERROUS_OUVERTURE_20260827.md), tous acceptés.
- **Pin jugé :** `87e915bd` ; **pin de cette réponse :** le commit qui la contient (`git log -- morsehgp3D_v5/audits/REPONSE_CLAUDE_87E915BD_AUDIT_BLOQUANT_20260827.md`).
- Cadre inchangé : `phase=exploration_v5_hors_registre`, `public_status=not_claimed`. GCP non utilisé.

L'audit a été lu et **exécuté** (ses trois harnesses ASan/UBSan, le singleton du census, les deux conformités différentielles) avant cette réponse. Il avait raison sur chaque point. Ce qui suit dit, dans son ordre de fermeture, ce qui est fermé par le code et ce qui reste ouvert.

## 1. Gardes d'API, vide/singleton, bornes — FERMÉ

- `run_pipeline` valide avant tout calcul : au moins deux points, `s >= 1`, `smax ∈ [2, 11]`, `threads >= 1`, plafond de coquille `>= 4` (`validate_run_options`, `src/pipeline/run.hpp`) ; statut `invalid_input`, aucun callback, aucun payload.
- `expand` dimensionne ses tableaux par `kmax` (plus aucun `11` en dur) ; la structure `ExpandStats::events_by_k` est un vecteur.
- Le census et la profondeur traitent un index à une position unique comme une feuille (`ix.root()`), comptent par `range_weight` sous la **précondition déclarée « positions distinctes »** (V1) ; `cover_query`, `rect_cover_handles`, `q3_ball_depth` partent aussi de `ix.root()`.
- Porte permanente `mhgp5_api_guard_gate` (`tests/api_guard_gate.cpp`) : vide, singleton, deux points, positions dupliquées (`unsupported_degeneracy`, zéro callback), coordonnée hors profil, `PointId` dupliqué, `smax ∈ {0, 1, 12, 100}`, `smax = 11` sur douze points, `smax = 2`, `s = 0`, `threads = 0`, `shell_cap = 3`, et la fixture exacte du singleton de l'audit (`at_least(1)` vrai, `at_least(2)` faux avec `count = 1`, census un intérieur, coquille sur un second point).
- Option `MHGP5_ENABLE_SANITIZERS` (ASan + UBSan sur toutes les cibles) ; la suite `gate` complète sous sanitizers est en cours au moment de cette réponse (résultat dans le reçu de campagne).

## 2. Correctif q4 et fixture ciblée — FERMÉ

- Cover q4 au **coefficient 3** aux deux étages (`generate.hpp`), avec le commentaire de doctrine : le cover est un sous-ensemble pour des minorants fail-open, le census exact passe par l'arbre.
- Fixture `mhgp5_q4_cover_fixture` (`tests/q4_cover_fixture.cpp`) : la clé différentielle de l'audit `(2712, -198919, -939434, -201167, 88336155)` sur `eight_clusters n=1200` est **présente post-RLE en arité 4**, sa profondeur globale est **exactement 8** (`ball_depth_at_least` et census à 8 intérieurs), elle est **tuée par le préfiltre** ; le mutant `q4-cover-coef4` (cover au coefficient 4, point d'injection compilé en test seulement) la fait disparaître → code 4.
- La proposition `q4_source_fixture.cpp` a été câblée **restreinte aux variantes valides** (`22`, `13+8`) ; ses commentaires « coefficient 4 = production » sont faux et à corriger (ouvert, mineur).
- Conformité rejouée sur build frais : `gate` 103/103 avant ce commit (avec les oracles), et les quatre `scale8000` sont relancés par la campagne à manifeste ci-dessous.

## 3. Mutants test-only, bras appariés, couverture code 4 — FERMÉ pour l'essentiel

- `MHGP5_MUTANT` n'est compilé que sous `MHGP5_TESTING` (posé par `mhgp5_executable` sur les cibles de `tests/`) ; dans une cible produit (`mhgp5_product_executable`, le pilote `mhgp5`) la macro est la constante `false` et `mutants_enable` refuse tout nom. Le pilote produit **n'a plus** d'option `--inject`.
- `conformity_v4 --inject=<m>` exécute d'abord son **bras nominal apparié** (sous-processus, mêmes arguments) et exige sa conformité au reçu ; la mise à mort est la divergence mutant/nominal, jamais une divergence préexistante (faux positif de l'audit fermé).
- `wspd-cap-terminal` et `wspd-split-heaviest` ont une porte à code 4 appariée (front mutant strictement plus gros que le nominal).
- Couverture code 4 des 43 mutants du registre : tous sauf **`attach-detector-disabled`**, **`cover-rect-dmin`**, **`q4-no-canonical`**, **`q4-seed-core-nonstrict`** (les quatre fixtures que l'audit nomme « réellement absentes » — OUVERT, prochain jalon).
- `mutants_gate.py` vérifie toujours registre ↔ sites ; il ne vérifie pas encore « cible CMake + code 4 » (OUVERT ; la liste ci-dessus est tenue à la main jusque-là).

## 4. Oracles câblés — FERMÉ

Label `oracle` (aussi `gate`) : `mhgp5_obig_selftest` (juge du juge, 79 304 cas contre `__int128`, frontière 2^383/2^384), `mhgp5_level_cmp` (9,3 M de paires de niveaux contre l'oracle 384 bits, mot-haut gravé), `mhgp5_q3_oracle` (tous les triangles, fixtures u16 extrêmes, six mutants dont deux d'oracle), `mhgp5_q4_oracle` (tous les tétraèdres, puissances équatoriales, grille 14³ à frontières, cinq mutants), `mhgp5_q4_source_fixture` (22, 13+8, mutant `q4-seeds-from-q3-live`), `mhgp5_forest_judge` (miniboule indépendante, cliques complètes, Kruskal à lots, 10 nuages, 879 événements, 0 désaccord, neuf mutants). `forest_judge.cpp` compile sous `-Werror` (paramètre inutilisé, `SubjectK::events`, tri par insertion contre le faux positif GCC 13).

## 5. Contrat transactionnel, résidence par K, applications verticales — FERMÉ / FERMÉ / OUVERT

- **Transaction** : les gardes de capacité de **tous** les ordres sont décidées sur les comptes (`count_events_by_k`, sans matérialisation) **avant le premier callback** ; un refus ne suit donc jamais un callback. Une violation d'invariant (un défaut) reste possible après un callback : les callbacks sont déclarés **provisoires** jusqu'au statut terminal (`docs/ARCHITECTURE.md` § 2 et § 4). C'est le protocole `provisional` demandé par V3, sans invalidation atomique côté consommateur (celle-ci lui appartient).
- **Résidence** : l'expansion est **par ordre K** (`expand_events_k`) ; les boules censusées sont le seul objet amont résident, les événements d'un seul K existent à la fois. La conformité au digest v4 est conservée (l'ordre du support d'un événement régulier est trié comme dans l'expansion de plateau : l'ordre d'émission des deltas en dépend — constat gravé en commentaire).
- **Applications verticales** : non livrées. Le README v5 et `ARCHITECTURE.md` disent désormais « dix forêts horizontales K = 1..10 », jamais « forêt complète » ni « tour ».

## 6. Provenance, reçus, contrôle documentaire — FERMÉ / EN COURS / FERMÉ

- `docs/PROVENANCE.md` couvre maintenant tous les modules produit (`generate`, `expand`, `digest`, `run`, `fold`, `plateau`, `render`, `device`, CLI, oracle).
- Reçus : la campagne appariée à **manifeste immuable** (pin, `sha256` du binaire et du reçu, toolchain, machine, commande, par cas : code, temps, RSS, digests, cardinalités) est relancée sur ce commit ; elle produira `receipts/conformite_v4/campagne_v5_<pin>.txt`.
- `tools/check_docs.py` inclut la v5 (README, `docs/`, `receipts/`, notes/questions/réponses de l'implémenteur) : 210 fichiers validés.
- `docs/PISTES_FERMEES.md` existe (héritage v3 + v4, sélection axiale et `build_forest_legacy` fermées, cover q4 au coefficient 4, patterns d'erreur).

## Ce qui reste ouvert, par risque

1. Quatre mutants sans porte à code 4 (`attach-detector-disabled`, `cover-rect-dmin`, `q4-no-canonical`, `q4-seed-core-nonstrict`).
2. `mutants_gate.py` ne contrôle pas « cible CMake + code 4 ».
3. Les applications verticales entre ordres (objet « tour ») ; le payload versionné de V3.
4. Commentaires périmés de `q4_source_fixture.cpp` ; Boost absent (la troisième autorité `cpp_int` n'est pas exercée ici).
5. Campagnes `scale16000` / `scale32000` et ASan : en cours, reçus à suivre.
