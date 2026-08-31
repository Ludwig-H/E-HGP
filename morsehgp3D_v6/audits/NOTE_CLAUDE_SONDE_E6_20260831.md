# Note Claude — sonde contrefactuelle E6 : la queue d'ancres lourdes est convertible en kills de cellules

Cadre : `phase=exploration_v6_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
GCP non utilisé. Lecture DIAGNOSTIQUE (opt-in `--sonde-e6`, lecture seule,
objet inchangé — conformité et 68/68 portes vertes avec le binaire de la
sonde) ; aucun seuil décisionnel n'est fixé ici.

## La sonde

Pour chaque seed q4 tuée par CŒUR (la population qui porte la masse
`W_sweep1` des octaves lourds : reçu `campagne_sonde_octaves_20260831`,
31–54 % des seeds lourdes, ~30–80 évaluations chacune), la sonde grave :

- si la grille de l'ancre est construite : le MINIMUM des témoins des
  cellules de corde consultées (la contrainte exacte qui a empêché le kill
  par cellules — th. 10.5), en godets 0 / <h/2 / ≥h/2 / h−1 ;
- sinon : la RAISON d'exclusion de la politique (cover < 256, ratio
  seeds/cover, veto `near_m ≥ h`, échec de construction).

## Résultat (n=16000, graine 5, reçu `sonde_e6_20260831`)

| | scanline_st | terrain_st |
|---|---|---|
| seeds sondées (grille construite) | 608 273 | 942 631 |
| … dont min ≥ h/2 ou h−1 (cellules PRESQUE mortes) | **77 %** | **60 %** |
| … dont min == 0 (témoins non communs) | 3 % | 5 % |
| sans grille — cover < 256 (ancres légères) | 4,88 M | 2,68 M |
| sans grille — veto near_m (ancres riches) | **1,28 M** | **0,76 M** |
| sans grille — ratio | 0,34 M | 0,01 M |

## Lecture

1. Sur les grilles construites, la contrainte bloquante est presque
   toujours une cellule à h/2..h−1 témoins : les témoins de cœur SONT
   presque communs aux cellules — un RAFFINEMENT (G > 8, ou raffinement
   hiérarchique des seules cellules vivantes) convertit l'essentiel de ces
   scans de cœur en kills de cellules. Le godet « min == 0 » (3–5 %) borne
   ce que le raffinement ne prendra jamais.
2. Le veto `near_m ≥ h` exclut la grille précisément sur les ancres RICHES
   (0,8–1,3 M seeds de cœur payées au scan) : l'heuristique prédisait des
   témoins trouvés « dès les premières classes radiales », mais la mesure
   donne w1/seed ≈ 22–77. Ce veto est faux sur les surfaces stationnaires.
3. La population cover < 256 est nombreuse mais vit sur les ancres légères
   (octaves ≤ 7) dont les pentes sont ~1 : pas la queue.

## Proposition E6 (chantier, à sonder en contrefactuel apparié avant tout GO)

Sur les ancres lourdes (cover ≥ 2^10) : (a) lever le veto `near_m` ;
(b) grille RAFFINÉE — même certificat 10.5, G paramétré (16) ou second
niveau sur les cellules vivantes. Attendu d'après la sonde : conversion de
la majorité des 30–60 % de `W_sweep1` lourds en coût de construction de
grille (linéaire dans le cover, une fois par ancre). La preuve sera un banc
apparié ON/OFF à objet identique (digests égaux) + pentes stationnaires,
selon la doctrine E6 du GRAND_LIVRE § 3 — jamais un claim avant.
