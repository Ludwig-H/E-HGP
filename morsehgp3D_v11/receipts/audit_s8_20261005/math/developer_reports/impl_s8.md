# Tranche S8 — module `num` : entiers, table des racines, sommes de radicaux

5 octobre 2026, 15 h 24 UTC (heure lue par `date -u`). Livraison L3, préalable de `--sortie=points`.
Cadre : phase=exploration_v11_hors_registre, backend=cpu_reference, profile=quantized_u21_input_only,
public_status=not_claimed. **GCP non utilisé.**

## Commit (local, non poussé)

- `adfcdc692` (`adfcdc6922dd4a05249e0b2d04e21202477121f5`) sur `b319efc84`, worktree `build/v11-impl-l3` en HEAD
  détaché : `v11: num big integers, root table and radical sums with radical_sign_budget (slice s8)`.
  28 fichiers, +2 930 / −7. Index vérifié vide avant l'ajout, ajout chemin par chemin.

## Ce qui est livré

| Fichier | Contenu |
| --- | --- |
| `src/num/big.hpp`, `big.cpp` | `num::Big` : signe-magnitude, tableau fixe de 272 mots mais **longueur utile** (`size()`, chaque boucle ne parcourt que les mots utiles) ; la capacité 16 384 + 1 024 bits n'est qu'un plafond : tout résultat exact au-delà rend `radical_sign_budget`, jamais une troncature. Addition, soustraction, multiplication d'école, décalages, division de Knuth D (forme `divmnu64`, chiffres de 64 bits, emprunt en `i128`) avec la sémantique plancher de Python, PGCD binaire, racine entière (proposition binary64 sur les 64 bits de tête, pas de Newton entiers, puis certificat $s^2\leq a$ et $a-s^2\leq 2s$ : le flottant propose, l'entier décide), carré parfait, résidu et division par un petit entier. Opérandes confondus admis (sorties écrites dans l'ordre sûr). Aucune exception, aucune allocation. |
| `src/num/rational.hpp`, `rational.cpp` | `num::Rational` réduit (comme `Fraction`), `from_level` pour un niveau N/D non réduit. |
| `src/num/radical.hpp`, `radical.cpp` | `RadicalSum` (radicandes entiers : $c\sqrt{n/d}=(c/d)\sqrt{nd}$), budget déclaré de 16 termes, termes et classes dans des `Buffer` comptés au budget (`RadicalSum::make`, `bytes()` = 210 304 octets, un par fil) ; classes : même classe ssi $N_1N_2$ carré parfait, signature (parité des valuations et caractère quadratique des 40 premiers de `points_flat.py`) comme accélérateur seulement ; signe = port de `sign_of_radicals` (0, 1, 2 classes en forme close, sinon encadrements à 96, 192, …, 6 144 bits, refus au-delà). `sqrt_diff_cmp`, `sqrt_cmp2` (deux racines contre deux), `compare_dates` (trois contre trois, `RValue.cmp`). |
| `src/num/roots.hpp`, `roots.cpp` | `RootTable` : $R_r=\mathrm{isqrt}(\lfloor N2^{128}/D\rfloor)$ en `u128` (domaine $R_r<2^{B+65}$, sinon `arithmetic_invariant`), `allocate` / `fill(begin, end)` / `fill_ranks` à positions fixes (parallélisable, identique quel que soit W), `bracket` (somme de $j\leq 16$ racines signées à $j\,2^{-64}$ près), `sign` (encadrement puis repli exact `RadicalSum` sur $(1/D)\sqrt{ND}$). |
| `src/core/reasons.def` | `radical_sign_budget` (`resource_exhausted`, `num`) en fin de table ; `tests/core/status_test.cpp` (copie gravée, 29 raisons, plancher 98) et `docs/SORTIES.md` au même commit. |
| `docs/ARCHITECTURE.md` | § 2 (ligne `num`) et § 3 : « une expression de longueur variable (somme de radicaux) a un budget déclaré avec refus explicite, en plus des `static_assert` de degré ». |
| `docs/PROVENANCE.md` | Section S8 : port explicite décision par décision, lignes et sha256 des sources (`points_radius.py` 457b997f…, `points_flat.py` 4647de07…, `points_flat_oracle.py` 781854ad… pour les témoins). |
| `tests/num/` | `big_probe.cpp` + `big_gate.py`, `roots_probe.cpp` + `roots_gate.py`, `radical_probe.cpp` + `radical_gate.py`, `radical_port.py` (copie à la lettre de `points_radius.py:29-117`, vérifiée par l'arbre syntaxique contre le banc à chaque porte), `big_io.hpp`, `big_test.cpp`, `tests.cmake`, `README.md`. |
| `bench/roots_cost.cpp` | Mesure du coût de la table sur tous les rangs du catalogue d'ordre K (voie de production, W fils) : séquentiel et parallèle, identité des deux tables, juge d'échantillon (≈ 4 100 rangs, certificat refait en `Big`). |
| `tests/mutants/num.json` | 4 mutants neufs, plancher 53 → 57. |

## Portes (lignes exactes, durées locales)

Construction Release u21, `-DMHGP11_MODULES=core;num` (puis `core;num;catalogue` pour la mesure), dossier
`/workspaces/E-HGP/build/v11-persist/b21-s8/`, `-j 4`, aucun avertissement. Portes de core et num (`-L fast -LE
mutant`) : **188/188 vertes en 23,7 s** (`-j 4`). `check_style` : `style_ok fichiers=500`. `check_docs` : aucune ligne
nouvelle par rapport à la base (156 lignes, code 1 avant comme après).

| Porte | Ligne gravée (`LINE`) | Durée locale (ctest) |
| --- | --- | --- |
| `mhgp11_num_big` (+ `_opt`) | `num_big_verdict conforme operations=23445 refus_capacite=1516 knuth=2500` | 4,0 s (3,9 s) |
| `mhgp11_num_radical` (+ `_opt`) | `num_radical_verdict conforme temoins=19 decisions=5900 egalites=1981 raffinees=1526 refus=0` | 1,2 s (1,0 s) |
| `mhgp11_num_roots` (+ `_opt`) | `num_roots_verdict conforme bits=21 racines=3314 sommes=4062 replis=692 egalites=512 raffinees=180` (gravée au profil 21 seulement ; planchers seuls à 18 et 24) | 0,7 s (0,5 s) |
| `mhgp11_num_s8_unit_{capacity,aliasing,division,rational_form,square_signature,radical_budget,roots_limits}` + `_inventaire` | `mhgp11_test_ok tests=7 controles=89` (exécutable seul) | 0,03 s chacune |
| `mhgp11_num_roots_cost_<donnée>_k<K>` (label `lidar`, plus `scale8000/16000/32000` ou `long`) | `roots_cost_verdict conforme k=<K>` | voir la mesure ci-dessous (1,7 à 13,4 s) |

Les trois portes Python passent aussi sous `python3 -S -B -O` lancées à la main (mêmes lignes). Planchers gravés :
big ≥ 20 000 opérations, ≥ 200 refus de capacité, ≥ 2 000 divisions de Knuth ; radical ≥ 5 000 décisions,
≥ 600 égalités, ≥ 300 décisions raffinées ; roots ≥ 3 000 racines, ≥ 4 000 sommes, ≥ 400 replis, ≥ 200 égalités,
≥ 50 raffinées. Sensibilité contrôlée : une sortie de sonde corrompue sur une ligne fait rendre 1 à `big_gate.py`.

Témoins de `mhgp11_num_radical` (attendus gravés, recalculés par la copie Python portée, qui doit elle-même les
rendre) : F5 (S(A∪B) = S(A) + S(B), = 1/4 ; S(A) = 1/8), F6 (= √2/8), F8 côté T (7/6 = 5/6 + 1/3) et côté A
(7/12 = 5/12 + 1/6), F14a (somme > 0, = 2^-70 exactement, < 2^-70 + 2^-200), dates 5√2 dans les deux sens, filtre
d'annulation (t = 1/4, m = 14 000 000², q = (14 000 000 − 6/997)² contre 1/2 + 6/997 + 2^-70 : −1), quasi-égalité
du second ordre $\sqrt{n^2}-2\sqrt{n^2+1}+\sqrt{n^2+2}$ à n = 2^40 (−1 à 192 bits), même famille à n = 2^2050 + 1
(refus à l'épuisement des précisions, Python : `Refusal`), radicande de 5 201 bits (Python décide à 6 144 bits, le
C++ refuse à la capacité : divergence déclarée), 17 termes (refus). Les termes F5/F6/F8 ont été extraits une fois de
`bench/points_flat_oracle.py` (numpy, local) ; les valeurs 1/4, √2/8, 7/6, 7/12 de l'oracle sont retrouvées.

## Mutants (au manifeste, joués seuls avec `--only`)

`python3 tests/mutants/run_mutants.py --manifest tests/mutants/num.json --source . --check` :
`manifeste_ok module=num mutants=57 plancher=57`. Les quatre neufs, joués avec `--only … --floor 4 --jobs 2
--build-jobs 2` (1 min 53 s) : `mutants_ok module=num mutants=4 tues=4 dont_signal=0 dont_delai=0
dont_construction=0 plancher=4`. Cause vérifiée sur les copies gardées :

| Mutant | Motif unique | Tué par |
| --- | --- | --- |
| `carre_parfait_hors_classe` | `valuation & 1u` → `valuation` (signature) | témoins F5 et F6 : refus au lieu de l'égalité certifiée |
| `egalite_non_certifiee` | encadrement contenant 0 → 0 | quasi-égalité (0 au lieu de −1 à 192 bits) et dates du différentiel |
| `budget_ignore` | refus final → 0 | témoin à n = 2^2050 + 1 : `ok 0 3 6144` au lieu du refus |
| `encadrement_decale` | `high += r + 1` → `high += r` | bornes lo/hi de `mhgp11_num_roots` |

La campagne complète des 57 mutants de `num` n'a pas été rejouée (méthode allégée) : à faire sur G4.

## Mesure du coût de la table (local, codespace, 4 fils, Release u21)

Tous les rangs du catalogue d'ordre K (`levels()`), car les rangs référencés par la sortie points n'existent pas
encore. Identité séquentiel/parallèle et juge d'échantillon conformes partout (`different=0`, `wrong=0`). Temps
indicatifs du codespace (les temps de référence se prennent sur G4).

| Nuage | K | Sites | Niveaux | Table séquentielle | ns par niveau | 4 fils | Octets de table | Catalogue (4 fils) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| uniform_u18_n8000 | 5 | 8 000 | 597 987 | 202 ms | 338 | 50 ms | 9,6 Mo | 1,25 s |
| uniform_u18_n16000 | 5 | 16 000 | 1 232 923 | 416 ms | 337 | 103 ms | 19,7 Mo | 2,47 s |
| uniform_u18_n32000 | 5 | 32 000 | 2 535 983 | 868 ms | 342 | 229 ms | 40,6 Mo | 5,38 s |
| lidar_ng00 | 5 | 39 885 | 1 085 776 | 361 ms | 332 | 86 ms | 17,4 Mo | 3,42 s |
| lidar_ng01 | 5 | 35 551 | 941 217 | 306 ms | 324 | 75 ms | 15,1 Mo | 2,73 s |
| lidar_ng02 | 5 | 45 845 | 1 099 582 | 359 ms | 326 | 87 ms | 17,6 Mo | 3,36 s |
| lidar_ng00 | 10 | 39 885 | 5 085 986 | 1 676 ms | 329 | 414 ms | 81,4 Mo | 11,1 s |

Lecture : coût linéaire en nombre de niveaux, environ 330 ns par niveau (une division de Knuth d'environ 300 bits par
134, deux ou trois pas de Newton et le certificat), soit environ 10 % du temps du catalogue. Sur la table complète,
c'est trop pour le budget de 100 ms ; la sortie points ne remplira que les rangs référencés (`fill_ranks`), en
parallèle. Si ces rangs restent nombreux, une voie à largeur fixe (division 6 mots par 3, une seule itération de
Newton) est le levier évident ; elle n'est pas écrite (aucun besoin mesuré avant S9).

## Choix

- Capacité fixe sur la pile, longueur utile dans les boucles : la copie implicite d'un `Big` copie 2 184 octets, les
  opérations internes passent par `assign` (mots utiles seulement). Les agrégats (`RadicalSum` : termes et classes)
  vivent dans des `Buffer` du budget de l'appelant ; les temporaires (quelques `Big`, quelques dizaines de Kio) sur
  la pile.
- Raison du dépassement d'un `Big` : `radical_sign_budget` (spécification § 7.8, « tout dépassement ») ; division
  par zéro, racine d'un négatif, certificat faux, niveau hors du domaine géométrique : `arithmetic_invariant`.
- Classes par signature puis carré parfait de $N_1N_2$ : décisions identiques à `radical_classes` (même relation
  d'équivalence, mêmes coefficients au facteur 1/d près), mêmes encadrements pour des radicandes donnés réduits, donc
  même précision de décision (comparée dans `mhgp11_num_radical`). Avec des niveaux non réduits (table des racines),
  l'encadrement natif est plus serré ou égal : signe et refus comparés, précision non comparée.
- Port Python : `bench/points_radius.py` importe numpy ; la porte `fast` utilise une copie à la lettre
  (`radical_port.py`) dont chaque définition est comparée par `ast.dump` à celle du banc (code 3 sinon).
- Témoin « budget » : des coefficients rationnels de 2 000 bits auraient fait refuser le C++ à la capacité avant
  l'épuisement des précisions (le mutant `budget_ignore` aurait survécu) ; d'où la famille du second ordre à
  coefficients entiers, qui atteint 6 144 bits sans dépasser la capacité.

## Écarts

1. La consigne cite `sign_of_radicals` à `bench/points_flat.py:93-117` : cette fonction est à
   `bench/points_radius.py:93-117` (les lignes 93-117 de `points_flat.py` sont `group_classes`). Le port suit
   `points_radius.py` ; la signature de `points_flat.py` (l. 44-45 et 58-71) n'est reprise que comme accélérateur.
2. `RadSum.sign`, `Level.phi_exact` et `_inverse_date` de `points_flat.py` (annoncés par la table « ports annoncés »
   du contrat S0) ne sont pas portés : ils servent la tête plate (S10). Déclaré dans `docs/PROVENANCE.md`.
3. Divergences déclarées avec Python : refus du C++ à la capacité de `Big` (radicande de plus de 5 120 bits à la
   précision 6 144) et au-delà de 16 termes, là où Python décide.
4. Fichiers ajoutés hors de la liste de la spécification : `rational.cpp`, `big_io.hpp`, `radical_port.py`,
   `roots_probe.cpp`, `radical_probe.cpp`, `roots_gate.py`, `bench/roots_cost.cpp`. `big_test.cpp` donne la porte
   `mhgp11_num_s8_unit_*`.
5. Construction à `-j 4` (règle des 4 cœurs) et non `-j 6`.
6. Ni ASan/UBSan, ni u18/u24, ni campagne complète de mutants, ni suite `fast` entière (méthode allégée).

## À faire tourner sur G4

- Profils 18 et 24 : `mhgp11_num_big`, `mhgp11_num_radical` (lignes indépendantes du profil), `mhgp11_num_roots`
  (graver les lignes 18 et 24 : seuls les planchers s'appliquent hors 21), `mhgp11_num_s8_unit_*`.
- ASan/UBSan sur les mêmes portes (Knuth D et décalages sur opérandes confondus en particulier).
- Campagne complète `mhgp11_mutants_num` (57 mutants) aux profils de la matrice.
- `mhgp11_num_roots_cost_*` (label `lidar`, `scale*` ou `long`) sur les données G4, à W = 1, 4, 48 : temps de la table
  de référence (le codespace ne donne que des ordres de grandeur).
- La matrice `tools/g4_matrix.json` n'a pas été modifiée : vérifier qu'une configuration y sélectionne les nouvelles
  portes `fast` de num (elles le sont par label) et les portes `lidar` de coût (configuration avec catalogue).
