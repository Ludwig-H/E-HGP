# Reçu — durcissement de l'oracle rationnel q3 (P0 de l'audit `ebc8236`)

Date : 17 août 2026. Cadre : `phase=exploration_v4_hors_registre`,
`public_status=not_claimed`. Base : commit `5964214` (événement q3
transactionnel complet). Machine : conteneur CPU de session (pas de G4 —
aucune commande GCP émise).

Réponse aux cinq P0 (ordre du § 8) de
`audits/AUDIT_ORACLE_RATIONNEL_Q3_EBC8236_20260817.md`.

## 1. `abort()` remplacé par un statut fail-closed

`oracle/obigint.hpp` ne contient plus aucun `std::abort()`. Toute saturation
de largeur (add, mul, mul fin, précondition de `sub_mag` désormais VÉRIFIÉE
par le borrow final) lève un drapeau collant `overflow_flag()` et rend un
résultat empoisonné documenté comme tel. Contrat d'usage : l'appelant vérifie
`overflow_seen()` avant de convertir un verdict ; drapeau levé ⟹ refus typé
(code 3, message `REFUS numeric_failure`), jamais un verdict ni un signal.
`static_assert(N >= 2)` grave la précondition de `from_i128` sur le type
générique. La journalisation est débrayable (`overflow_log()`) car le
selftest provoque des débordements attendus par milliers ; le drapeau, lui,
n'est jamais débrayable.

## 2. Le juge du juge : `mhgp4_obig_selftest` contre `cpp_int`

Troisième autorité : `boost::multiprecision::cpp_int` (Boost 1.83, en-têtes
seuls), tests uniquement — jamais dans l'oracle ni dans la production.
Enregistré seulement si Boost est présent (même précédent que le témoin GMP
optionnel de v3). L'autorité décide aussi QUAND le débordement est attendu :
tout résultat de magnitude au moins `2^384` doit lever le drapeau, tout
résultat strictement inférieur doit être exact et sans drapeau.

Mesures (graine gravée `0x0b1d5a17c0ffee01`, aucune horloge) :

| mesure | valeur |
|---|---|
| valeurs du pool (occupations 1 à 6 limbes, motifs tout-à-un, puissances pures, zéros signés, `INT128_MIN/MAX`) | 95 |
| paires testées add/sub/mul/cmp | 9 025 |
| triplets distributifs `a(b+c) == ab+ac` (plancher 1 000) | 1 242 |
| désaccords | 0 |
| `2^320 · 2^63 = 2^383` | exact, sans drapeau |
| `2^320 · 2^64 = 2^384` | drapeau levé, processus vivant |
| `(2^384 - 1) + 1` | drapeau levé, processus vivant |
| mutant `--inject=mul-carry-lost` | 4 635 désaccords, code 4 |

Le mutant jette la retenue du produit long aux positions `i+j >= 2`
seulement : le tuer prouve que les limbes hauts sont réellement traversés,
pas seulement déclarés.

## 3. Fixtures u16 extrêmes et compteurs de limbes

Quatre nuages ajoutés à l'oracle géométrique (audit § 3.1) :

- **équilatéral maximal** : `(0,0,0), (M,M,0), (M,0,M)` à `M = 65535`, plus
  `(0,M,M)` et `(M,M,M)` — les cinq premiers points cosphériques (centre
  `(M/2,M/2,M/2)`), toutes les arêtes du triangle de base à `2M²` : égalités
  d'owner départagées par EdgeKey sous les plus grands coefficients de la
  grille ;
- **presque rectangle mais aigu** : `(0,0,0), (40000,0,0), (20000,20001,0)`,
  produit scalaire à l'apex `40001` sur des arêtes de `1,6·10^9` — la marge
  de Thalès minimale sans quitter u16 ; un point profond `(20000,10000,1000)`
  et un point externe complètent ;
- **grande cosphère** : centre `(32768,32768,32768)`, coquille = 24
  permutations signées de `(12000,16000,0)`, rayon 20000, plus le centre et
  un intérieur excentré — les plans de coordonnées portent des octuplets
  cocycliques, source massive d'extra-shells à haute amplitude ;
- **rejeu translaté au bord** : le presque-rectangle translaté de
  `(25535,45534,63535)` — coordonnées jusqu'à 65535 exactement ; le Cramer en
  coordonnées absolues ne cache aucune largeur liée à l'origine.

Compteurs publiés (plus haut limbe non nul observé, indices 0..5) :

| grandeur | limbe max observé | borne prouvée |
|---|---|---|
| `det` | 1 | `< 2^74` (limbe 1) |
| `num_i` | 1 | `< 2^109` (limbe 1) |
| `\|z·det - N\|²` | 2 | `< 2^254` (limbe 3) |
| produit du niveau | 3 | `< 2^323` (limbe 5) |

Plancher gravé : le produit du niveau doit atteindre le limbe 3 (code 3
sinon) — les fixtures extrêmes mordent réellement au-delà des « entiers de
cour de récréation ».

**Question `OBig<5>` (audit § 3.2), résolution** : la borne prouvée `2^323`
exige le limbe 5 (`2^323 > 2^320`) — un `OBig<5>` n'est donc PAS prouvable
sûr et la largeur 6 est conservée. La morsure mesurée s'arrête au limbe 3 :
l'écart entre largeur prouvée et largeur mordue est désormais EXPLICITE
(compteurs publiés), couvert au pire par l'échec fermé du § 1, et les limbes
4–5 sont exercés par le selftest arithmétique (pool à 6 limbes, frontière
exacte `2^383` / `2^384`). C'est la distinction demandée entre largeur
prouvée et porte expérimentale — sans fixture de sharpness artificielle.

## 4. Renommage et ballkeys dégénérées uniques

`shells_seen` devient `supports_with_extra_shell` (il compte des supports,
pas des boules). Le compteur `ballkeys_degenerees_uniques` demandé « lorsque
BallKey existe » est ajouté (la `Q3BallKey` existe depuis `5964214`) :

| mesure (8 nuages, 39 852 triangles) | valeur |
|---|---|
| événements | 5 792 |
| `supports_with_extra_shell` | 905 |
| `ballkeys_degenerees_uniques` | 335 |
| désaccords | 0 |

Réponse à la question du § 6 : les plateaux u16 ne sont pas 905 accidents
indépendants — 905 supports à coquille ne portent que 335 boules distinctes ;
la même cosphère est bien reproposée par de nombreux triangles (les grands
cercles cocycliques de la cosphère en portent chacun des dizaines).

## 5. Mutants

| mutant | cible | verdict |
|---|---|---|
| `sign-p` (reçu) | confusion intérieur/coquille | code 4 |
| `prune-ge` (reçu) | élagage `mn >= 0` qui masque une coquille | code 4 |
| `cramer-swap` (nouveau) | deux numérateurs du centre échangés | code 4 |
| `level-4g` (nouveau) | facteur 4 omis dans `DEX/(4G)` | code 4 |
| `mul-carry-lost` (nouveau) | retenue perdue du produit long, limbes hauts | code 4 (géométrie ET selftest) |

Mutant « signe du déterminant oublié dans une comparaison non quadratique »
(audit § 7.2) : NON implémenté, à dessein — l'oracle n'a aucune comparaison
non quadratique ; toutes ses comparaisons portent sur des carrés homogènes où
le signe de `det` s'élimine par construction (§ 1.2 de l'audit lui-même). Un
tel mutant n'aurait pas de site d'injection honnête dans ce code ; il
redeviendra pertinent si un chemin non quadratique apparaît (tri radial par
centre, par exemple).

## 6. Indépendance renforcée (audit § 4)

- primitives `dot/cross/norm2/sub` RÉÉCRITES LOCALEMENT dans le test
  (`odot/ocross/onorm2/osub`) : le chemin oracle n'appelle plus
  `p3_dot/p3_cross/p3_norm2` de la production ;
- l'oracle reçoit des `InputPoint{id, position}` déjà formés — l'id est le
  rang dans le nuage d'origine ; le renommage interne de `CloudIndex` n'est
  plus visible du chemin oracle (une table `rang -> index unique` sert
  uniquement à interroger le sujet) ;
- premier pas du P1 : le niveau public du sujet (`q3_exact_level`, fraction
  canonique `num/den`) est désormais jugé par l'oracle via
  `num·det² == den·|a·det - N|²` (largeur `< 2^324 < 2^384`).

## 7. Portes (32 CTests, toutes vertes)

Nouvelles : `mhgp4_obig_selftest_gate` (0), `mhgp4_obig_mutant_carry` (4),
`mhgp4_q3_oracle_mutant_cramer` (4), `mhgp4_q3_oracle_mutant_level4g` (4),
`mhgp4_q3_oracle_mutant_carry` (4). Le plancher de limbes et le refus
`numeric_failure` vivent dans la porte d'accord `mhgp4_q3_oracle_accord`.

Reste ouvert (P1 de l'audit) : l'oracle d'ÉVÉNEMENT complet — `SupportKey`
exact-once, `BallKey` primitive, `InteriorIds/ShellIds` triés, `ExactCenter`
canonique, hyperincidence — à confronter aux records de
`bench/q3_events_probe.cpp` et non plus aux seuls prédicats.
