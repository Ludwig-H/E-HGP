# Vérification légère de la tranche S8 (module `num`)

- Date : 2026-10-05, 15:29 UTC (`date -u`).
- Cadre : phase=exploration_v11_hors_registre, backend=cpu_reference, profile=quantized_u21_input_only, public_status=not_claimed. GCP non utilisé.
- Objet : commit local `adfcdc692` (adfcdc6922dd4a05249e0b2d04e21202477121f5) sur `b319efc84`, worktree `/workspaces/E-HGP/build/v11-impl-l3` (HEAD détaché, non poussé). Rien n'a été modifié dans le worktree (`git status` vide avant et après).
- Méthode légère : lecture du diff contre DECISIONS_UTILISATEUR, SORTIES, spécification (§ 7.6, 7.8, 9.1 S8), critique (L3) et apports des auditeurs ; rejeu des seules portes de la tranche sur le build de l'implémenteur `/workspaces/E-HGP/build/v11-persist/b21-s8`. Ni reconstruction, ni sanitizer, ni campagne de mutants.

## Verdict

**Aucun point bloquant.** L'arithmétique est exacte, sans décision flottante, les refus sont explicites et la table est à positions fixes. Les portes ne sont pas vertes par vacuité. Deux corrections mineures et quelques notes pour S9 et G4 suivent.

## Portes rejouées (build b21-s8, profil 21)

| Porte | Résultat | Durée locale |
| --- | --- | --- |
| `mhgp11_num_s8_unit_*` (8) | vertes | 0,02–0,03 s chacune |
| `mhgp11_num_big` / `_opt` | `num_big_verdict conforme operations=23445 refus_capacite=1516 knuth=2500` | 3,3 / 3,5 s |
| `mhgp11_num_radical` / `_opt` | `num_radical_verdict conforme temoins=19 decisions=5900 egalites=1981 raffinees=1526 refus=0` | 1,0 / 1,0 s |
| `mhgp11_num_roots` / `_opt` | `num_roots_verdict conforme bits=21 racines=3314 sommes=4062 replis=692 egalites=512 raffinees=180` | 0,4 / 0,5 s |

- Les trois portes Python repassent à la main sous `python3 -S -B -O`, avec les mêmes lignes.
- `check_style` : `style_ok fichiers=500`. `check_docs` : 156 lignes, comme la base ; aucune ne vise PROVENANCE, ARCHITECTURE, SORTIES ni `tests/num/README.md`.
- Épingles de PROVENANCE vérifiées : les sha256 de `bench/points_radius.py`, `bench/points_flat.py` et `bench/points_flat_oracle.py` concordent.
- Les sondes de coût lidar et les mutants n'ont pas été rejoués (méthode légère).

## Lecture du code

**`Big` (`src/num/big.cpp`).**
- La capacité est cohérente : 272 mots, soit exactement 17 408 bits.
  - `add_magnitude` refuse une retenue au mot 272 ; `multiply` refuse si `bits - 1 > capacité`.
  - La borne `la + lb <= 273` tient, donc l'accumulateur de 273 mots suffit.
  - `shift_left` refuse avant d'écrire.
- Les opérandes confondus sont sûrs : addition et soustraction lisent l'indice i avant de l'écrire, `shift_left` va de haut en bas et `shift_right` de bas en haut. `divide` passe par des locaux et refuse `&q == &r`.
- La division plancher concorde avec Python dans les quatre cas de signe (vérifié à la main, et par 23 445 opérations).
- Knuth D (forme divmnu64) : `qhat < 2^64` est garanti avant le produit par `vn[n-2]`, et `rhat < 2^64` par la sortie de boucle. L'emprunt signé en i128 est correct.
- **Ajout en retour effectivement exercé.** J'ai simulé Knuth D en Python sur les cas de la porte (script hors dépôt) : 4 656 divisions, 1 179 ajouts en retour et 61 449 corrections de `qhat`. Le cas rare n'est donc pas vert par vacuité.
- `isqrt` part d'une proposition binary64 sur 63 ou 64 bits de tête (décalage pair). Le premier pas de Newton rend une valeur `>= isqrt(a)`, puis la suite décroît. Le certificat `0 <= a - x^2 <= 2x` est entier. Le flottant propose, l'entier décide : la règle F1 est respectée.
- `gcd` binaire, `residue` (convention de Python) et `divide_small` (opérandes confondus du haut vers le bas) sont corrects.

**`Rational`.** La forme est réduite, avec un dénominateur positif et zéro en 0/1 ; elle est conforme à `fractions.Fraction`.

**`RadicalSum` (`src/num/radical.cpp`).**
- Le port décision par décision est conforme à `bench/points_radius.py:33-117` :
  - `sqrt_diff_cmp` : la récursion finale devient un échange unique, et le second tour tombe toujours dans la branche `d > 0` ;
  - `sqrt_cmp2` ;
  - `radical_classes` : première classe dont le rapport est un carré, retrait des coefficients nuls ;
  - `sign_of_radicals` : cas 0, 1 et 2 classes, puis encadrements de 96 à 6 144 bits, comme Python ;
  - `RValue.cmp`, porté en `compare_dates`.
- **Équivalence de la forme entière.** Un terme c·√(n/d) s'écrit (c/d)·√(nd). Les encadrements valent alors exactement ceux de `sqrt_bounds` de Python. La précision qui tranche est donc la même, et la porte la compare.
- **Signature nécessaire, jamais suffisante.**
  - Si N1·N2 est un carré, les valuations ont même parité et le produit des parties inversibles est un carré.
  - Le caractère de Legendre est donc le même. Pour 2, u1·u2 ≡ 1 (mod 8) entraîne u1 ≡ u2 (mod 8).
  - Le filtre n'écarte donc jamais une classe que `square_ratio` accepterait.
- Le budget est déclaré : 16 termes, 8 192 bits, capacité de `Big`. Au-delà, le refus est `radical_sign_budget`, jamais une égalité supposée.

**`RootTable` (`src/num/roots.cpp`).**
- Avec R = isqrt(⌊N·2^128/D⌋), on a R ≤ 2^64·√ℓ < R+1 (car (R+1)^2 ≥ ⌊X⌋+1 > X). Les bornes `lo` et `hi` sont donc correctes, et la décision n'est prise que si 0 est strictement hors de l'intervalle.
- Le repli (sign/D)·√(N·D) est exact. À u24, N·D fait au plus 356 bits, donc aucun refus de capacité sur ce chemin.
- `fill` et `fill_ranks` écrivent à positions fixes. La sonde de coût compare la voie série et la voie à 4 fils rang par rang (`different=0` selon le compte rendu).
- La sentinelle `~u128{0}` ne peut pas être une racine, puisque R < 2^89.

**Raison `radical_sign_budget`.**
- Elle est ajoutée en fin de `reasons.def`, avec `status_test.cpp` (29 lignes) et ARCHITECTURE § 3 au même commit.
- Aucune valeur existante ne change, et aucun octet de sortie (`MHGP11FUL1`, `MHGP11PH`) n'est touché.

**Portes.**
- Planchers gravés :
  - `mhgp11_num_big` : 20 000 opérations, 200 refus, 2 000 divisions de Knuth ;
  - `mhgp11_num_radical` : 5 000 décisions, 600 égalités, 300 cas raffinés ;
  - `mhgp11_num_roots` : 3 000 racines, 4 000 sommes, 400 replis, 200 égalités, 50 cas raffinés.
- Les témoins gravés sont d'abord exigés de la copie Python, ce qui donne un double contrôle.
- La copie `radical_port.py` est vérifiée à chaque exécution contre le banc, par arbre syntaxique.
- Les divergences déclarées (capacité, 17 termes) exigent que Python décide, ce qui empêche une divergence non déclarée.

## À corriger (mineur, non bloquant)

1. **`src/num/roots.cpp:29-35` (`RootTable::allocate`).**
   - Problème : les rangs sont des `u32` (`root(u32)`, `SignedRank::rank`) alors que la table admet `levels.size()` en `u64`.
   - Au-delà de 2^32 − 1 niveaux, les rangs seraient tronqués en silence. C'est aussi le cas de `static_cast<u32>(r)` dans `bench/roots_cost.cpp:122-131`.
   - Correction attendue : refuser explicitement (`arithmetic_invariant`, ou la raison de capacité du catalogue) si `levels.size() > UINT32_MAX`, et graver un cas dans `mhgp11_num_s8_unit_roots_limits`.
2. **`tests/num/radical_gate.py:8-11` et énoncé.**
   - Problème : l'énoncé demandait des témoins F5, F6 et F14a « recalculés par `bench/points_flat_oracle.py` ». Ils sont en fait gravés une fois. La divergence est déclarée dans PROVENANCE.
   - Correction attendue : une porte `long` hors `fast` (numpy disponible) qui réextrait les termes de l'oracle épinglé et les compare aux termes gravés. À défaut, une mention explicite dans le reçu G4.

## Notes

- **Mémoire en S9.**
  - `RootTable::allocate` admet un `u128` par rang du catalogue, même si `fill_ranks` n'en remplit que quelques-uns. Cela fait 81 Mo à ng00 K10 (5,09 M niveaux), à compter dans le budget de l'étage `points`.
  - Une table creuse (rangs référencés seulement) serait le levier si la mémoire compte.
  - `RadicalSum::make` n'appelle pas `admit` : le pilote doit admettre W × `RadicalSum::bytes()` (210 304 octets par fil) avant les tâches parallèles, selon le contrat de `MemoryBudget::admit`.
- **Pile.**
  - Un `Big` occupe 2 184 octets. `refine`, appelé sous `Rational::make`, puis `gcd` et `divide`, puis `knuth_divide` (deux tableaux de 2,2 Ko), empile une vingtaine de `Big`, soit environ 50 Ko.
  - C'est sans risque avec la pile de 8 Mio des fils, mais il faut le vérifier sous ASan sur G4, où les cadres grossissent.
- **Copies implicites.** Les copies de `Big` (tableau entier, mots au-delà de `size()` non initialisés) sont presque toutes élidées. Elles restent formellement des lectures de valeurs indéterminées : sans effet sous ASan ou UBSan, mais MSan les signalerait.
- **Juge de la sonde de coût.** Le juge d'échantillon de `bench/roots_cost.cpp` refait le certificat avec `num::multiply`. Il est indépendant de `isqrt` et de `divide`, mais pas de `Big`. La porte `mhgp11_num_roots` (contre `math.isqrt`) reste le juge indépendant.
- **Écarts du compte rendu.** Les écarts déclarés par l'implémenteur sont exacts : `sign_of_radicals` est dans `points_radius.py:93-117`, `RadSum.sign`, `phi_exact` et `_inverse_date` sont reportés en S10, et le build a été fait à `-j 4`.
- **Pour G4 (inchangé par rapport au compte rendu).**
  - Profils 18 et 24 : graver la ligne de `mhgp11_num_roots`.
  - Rejouer ces portes sous ASan/UBSan.
  - Lancer la campagne complète des 57 mutants `num`.
  - Lancer `mhgp11_num_roots_cost_*` à W = 1, 4 et 48.
  - Vérifier que `tools/g4_matrix.json` sélectionne les nouvelles portes `num` et lidar (configuration avec catalogue).
