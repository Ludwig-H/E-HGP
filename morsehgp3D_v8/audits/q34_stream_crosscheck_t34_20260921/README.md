# Tranche 34 : flux q3 et q4 avec l'atlas q4 `joined` (et `rectangle-pair`, `boxes`, `affine`) contre les énumérations exhaustives indépendantes

Auditeur B, 21 septembre 2026. Sonde du constructeur `mhgp8_wspd_q34_probe`
construite à **d6e1bd9e** (arbre de travail détaché, Release ;
`lanes/q34_witness_search.cpp` `172ae93abf9c4506…`,
`lanes/q3_ball_census.cpp` `434bd6e49eee8cc1…`), commit du
constructeur dont les sources de la tranche 34 sont, par ma faute, entrées dans
l'historique sous 4c3cdb0c (voir le dialogue), harnais
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
par paire, avant toute couverture), le census q3 `boxes` (par
boîtes de l'index, en deux phases) les bornes de témoins `affine` de
la tranche 33 et le mode d'atlas q4 `joined` de la tranche 34 (cellules de seeds
jointes, atlas sans feuille sautés), masque 6, Local28, quatre workers, mode
`records`. Égalités exigées : masses résiduelles par voie, seeds q3, boules q3
émises et multiensemble (support, profondeur), ensemble des boules q4
distinctes (clé entière réduite, profondeur), supports q4 sur leur sphère,
registre `q4_emitted` égal aux records ; le lecteur exige aussi que les
nouveaux modes aient rejeté une masse non nulle (mesure non vide) et, à
1k/K5, que la sonde relancée en `disabled`/`scalar` émette le même flux
(mêmes comptes, empreintes xor/somme et IDs de coquille), donc aussi le même
flux que les tranches 31, 32 et 33.

## Résultat : accord exact sur les 5 lignes

| n | K | résiduel q3 / q4 | rejeté par rectangle + par paire | paires développées | seeds q3 | boules q3 sonde = harnais | boules q4 sonde = harnais | lemme q3 paires / viol. | lemme q4 paires / viol. | disabled/scalar | s sonde / q3 / q4 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1000 | 5 | 95 716 / 61 555 | 73 965 + 6 296 | 24 326 | 105 134 | 10 481 = 10 481 | 876 = 876 | 78 419 / 0 | 45 918 / 0 | oui | 0.4 / 4.8 / 29.9 |
| 2000 | 5 | 271 254 / 162 155 | 220 568 + 18 705 | 58 106 | 315 259 | 21 948 = 21 948 | 2 113 = 2 113 | 2 000 / 0 | 2 000 / 0 | — | 0.9 / 12.0 / 4.7 |
| 4000 | 5 | 735 174 / 470 334 | 625 787 + 59 792 | 142 871 | 753 308 | 45 151 = 45 151 | 4 798 = 4 798 | 1 000 / 0 | 500 / 0 | — | 2.1 / 60.4 / 16.7 |
| 1000 | 10 | 203 561 / 185 194 | 159 124 + 13 810 | 51 863 | 361 747 | 42 876 = 42 876 | 8 033 = 8 033 | 167 992 / 0 | 148 157 / 0 | — | 1.3 / 16.4 / 204.5 |
| 2000 | 10 | 597 066 / 550 698 | 505 346 + 35 785 | 119 705 | 1 023 835 | 92 994 = 92 994 | 20 194 = 20 194 | 2 000 / 0 | 2 000 / 0 | — | 3.8 / 35.7 / 24.3 |

Lecture : l'atlas `joined` de la tranche 34 ne change pas l'objet émis (mêmes boules q3 et q4,
mêmes profondeurs que les énumérations indépendantes et que les modes 31), et
la colonne « rejeté » mesure la masse de paires retirée avant toute couverture
sur ces préfixes. Trois petites tailles, un scan : pas de qualification, pas
de FULL.

## Rejouer

```bash
python3 -B -O morsehgp3D_v8/audits/q34_stream_crosscheck_t34_20260921/run_t34_crosscheck.py read --commit d6e1bd9e
git worktree add --detach /tmp/wt-d6e1bd9e d6e1bd9e
cmake -S /tmp/wt-d6e1bd9e/morsehgp3D_v8 -B /tmp/wt-d6e1bd9e/build/v8-audit -DCMAKE_BUILD_TYPE=Release -DBOOST_ROOT=<boost> && cmake --build /tmp/wt-d6e1bd9e/build/v8-audit --parallel
python3 -B -O morsehgp3D_v8/audits/q34_stream_crosscheck_t34_20260921/run_t34_crosscheck.py run --worktree /tmp/wt-d6e1bd9e --scratch /tmp/crosscheck34 --commit d6e1bd9e --output /tmp/Q34_STREAM_CROSSCHECK_T34.json
```
