# Tranche 32 : flux q3 et q4 en modes `rectangle-pair` et `boxes` contre les énumérations exhaustives indépendantes

Auditeur B, 21 septembre 2026. Sonde du constructeur `mhgp8_wspd_q34_probe`
construite à **d1b4dbc6** (arbre de travail détaché, Release ;
`lanes/q34_witness_search.cpp` `14f120224c11bc0f…`,
`lanes/q3_ball_census.cpp` `434bd6e49eee8cc1…`), harnais
de B [q3_stream_probe.cpp](q3_stream_probe.cpp) et
[q4_stream_probe.cpp](q4_stream_probe.cpp) (copies à l'octet près des harnais
des reçus [q3](../q3_stream_crosscheck_20260921/README.md) et
[q4](../q4_stream_crosscheck_20260921/README.md) de la tranche 31), compilés
contre la même bibliothèque. `phase=exploration_v8_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
GCP non utilisé. Contrôle du **calcul** (même objet, deux codes), pas une
qualification ; coquilles non comparées.

## Ce qui est comparé

Mêmes préfixes du scan 0 (1k/2k/4k à K5, 1k/2k à K10, s = 8) et mêmes
énumérations indépendantes que pour la tranche 31, mais la sonde tourne cette
fois avec la recherche de témoins `rectangle-pair` (rejet par rectangle puis
par paire, avant toute couverture) et le census q3 `boxes` (par
boîtes de l'index, en deux phases), masque 6, Local28, quatre workers, mode
`records`. Égalités exigées : masses résiduelles par voie, seeds q3, boules q3
émises et multiensemble (support, profondeur), ensemble des boules q4
distinctes (clé entière réduite, profondeur), supports q4 sur leur sphère,
registre `q4_emitted` égal aux records ; le lecteur exige aussi que les
nouveaux modes aient rejeté une masse non nulle (mesure non vide) et, à
1k/K5, que la sonde relancée en `disabled`/`scalar` émette le même flux
(mêmes comptes, empreintes xor/somme et IDs de coquille).

## Résultat : accord exact sur les 5 lignes

| n | K | résiduel q3 / q4 | rejeté par rectangle + par paire | paires développées | seeds q3 | boules q3 sonde = harnais | boules q4 sonde = harnais | lemme q3 paires / viol. | lemme q4 paires / viol. | disabled/scalar | s sonde / q3 / q4 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1000 | 5 | 95 716 / 61 555 | 73 965 + 6 296 | 24 326 | 105 134 | 10 481 = 10 481 | 876 = 876 | 78 419 / 0 | 45 918 / 0 | oui | 0.3 / 4.7 / 29.9 |
| 2000 | 5 | 271 254 / 162 155 | 220 568 + 18 705 | 58 106 | 315 259 | 21 948 = 21 948 | 2 113 = 2 113 | 2 000 / 0 | 2 000 / 0 | — | 0.9 / 12.1 / 4.6 |
| 4000 | 5 | 735 174 / 470 334 | 625 787 + 59 792 | 142 871 | 753 308 | 45 151 = 45 151 | 4 798 = 4 798 | 1 000 / 0 | 500 / 0 | — | 2.1 / 59.4 / 16.7 |
| 1000 | 10 | 203 561 / 185 194 | 159 124 + 13 810 | 51 863 | 361 747 | 42 876 = 42 876 | 8 033 = 8 033 | 167 992 / 0 | 148 157 / 0 | — | 1.3 / 15.6 / 203.9 |
| 2000 | 10 | 597 066 / 550 698 | 505 346 + 35 785 | 119 705 | 1 023 835 | 92 994 = 92 994 | 20 194 = 20 194 | 2 000 / 0 | 2 000 / 0 | — | 3.3 / 36.3 / 24.6 |

Lecture : les modes 32 ne changent pas l'objet émis (mêmes boules q3 et q4,
mêmes profondeurs que les énumérations indépendantes et que les modes 31), et
la colonne « rejeté » mesure la masse de paires retirée avant toute couverture
sur ces préfixes. Trois petites tailles, un scan : pas de qualification, pas
de FULL.

## Rejouer

```bash
python3 -B -O morsehgp3D_v8/audits/q34_stream_crosscheck_t32_20260921/run_t32_crosscheck.py read --commit d1b4dbc6
git worktree add --detach /tmp/wt-d1b4dbc6 d1b4dbc6
cmake -S /tmp/wt-d1b4dbc6/morsehgp3D_v8 -B /tmp/wt-d1b4dbc6/build/v8-audit -DCMAKE_BUILD_TYPE=Release -DBOOST_ROOT=<boost> && cmake --build /tmp/wt-d1b4dbc6/build/v8-audit --parallel
python3 -B -O morsehgp3D_v8/audits/q34_stream_crosscheck_t32_20260921/run_t32_crosscheck.py run --worktree /tmp/wt-d1b4dbc6 --scratch /tmp/crosscheck32 --commit d1b4dbc6 --output /tmp/Q34_STREAM_CROSSCHECK_T32.json
```
