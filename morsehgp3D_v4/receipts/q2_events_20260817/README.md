# Reçu — la lane q2 productrice : le census diamétral à coefficient 1

Date : 17 août 2026. Cadre : `phase=exploration_v4_hors_registre`,
`public_status=not_claimed`. La troisième et dernière lane d'événements —
la couche événementielle est désormais COMPLÈTE (q2, q3, q4) pour nourrir
la forêt.

## L'observation qui rend q2 simple

`W_2(a,b)` EST la boule diamétrale ouverte (`H = d·w − |w|² > 0` ⟺
`|z−m| < D/2`). Le census q2 est donc une requête du cover d'arête à
**coefficient 1** (`|2z−(a+b)|² <= D²`) : intérieur strict `<`, coquille
`==`, les extrémités `a, b` exactement sur la sphère (support, exclues).
Et la forme de boule est PRIMITIVE par construction :
`|2z−(a+b)|² − D² = 4(|z|² − (a+b)·z + a·b)` donne
`(A, B, C) = (1, −(a+b), a·b)` — aucun pgcd à payer. Niveau public
`D²/4` en fraction canonique. Profil `K_max <= 10` : `h_2 = 10`,
profondeur survivante `<= 9`, `interior` dimensionné à 9 (`K = d + 1`).

## Chaîne et portes (61 CTests, toutes vertes)

WSPD ternaire lane q2 → ancres survivantes (`h_cœur + h_a + h_b < h_2`,
histogrammes 8 coins) → census coefficient 1 → `Q2Event{support (l'arête
est son propre owner), ball, level, depth, interior triés}`. Exact-once
par la partition CK des paires, gravé par l'invariant de doublons.

| mesure | uniform n=400 | eight_clusters n=400 |
|---|---|---|
| événements q2 | 10 110 | juge 0/0 aussi |
| juge brut toutes-paires (records complets) | 0 manquant / 0 en trop | 0 / 0 |
| refus de coquille | **837** | — |
| `t_instruction` | 22,6 ms | — |

Les 837 coquilles exactes sur une grille u16 uniforme confirment que les
cosphéricités diamétrales ne sont PAS marginales en entier — le refus
transactionnel (et la question Q5 des ex æquo) portent sur un phénomène
massif, pas un cas d'école.

Fixture gravée `fixture_shell` : `a=(0,0,0)`, `b=(4,0,0)`, coquille
`z=(2,2,0)` (`|2z−s|² = 16 = D²`), intérieur `w=(2,1,0)` — l'arête (0,1)
est REFUSÉE en régime régulier ; le mutant `sign-le` (coquille comptée
intérieure) la publie à tort et meurt (code 4).

## État de la couche événementielle

| lane | census | record | juge | oracle indépendant |
|---|---|---|---|---|
| q2 | cover coefficient 1 (22,6 ms à n=400) | `Q2Event` | toutes-paires 0/0 | trivial (comparaisons i64) — porté par le juge |
| q3 | site-major + paquets + cover rectangulaire | `Q3Event` | tous-triangles 0/0 | obigint 384 bits, 0 désaccord |
| q4 | complétion énumérée (axial reçu, optionnel) | `Q4Event` | tous-tétraèdres 0/0 | obigint 384 bits, 0 désaccord |

Prochaine étape : la FORÊT — consommer les trois flux par `K`, macro-lots
`same_exact_level` (U320), cartes verticales, `F_K^conn`/`F_K^render`.
