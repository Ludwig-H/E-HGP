# Tranche 32 à 8 000 points : flux q3 et q4 en modes `rectangle-pair` et `boxes` contre les énumérations exhaustives indépendantes

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

Le scan 0 à 8 000 points entier (première taille d'intérêt du plan de tests),
K = 5 et K = 10, s = 8, mêmes énumérations indépendantes que pour la tranche 31
(harnais mono-fil, lemme du citron sur 300 paires rejetables q3 et 200 q4 par
ligne) ; la sonde tourne avec la recherche de témoins `rectangle-pair` (rejet par rectangle puis
par paire, avant toute couverture) et le census q3 `boxes` (par
boîtes de l'index, en deux phases), masque 6, Local28, quatre workers, mode
`records`. Égalités exigées : masses résiduelles par voie, seeds q3, boules q3
émises et multiensemble (support, profondeur), ensemble des boules q4
distinctes (clé entière réduite, profondeur), supports q4 sur leur sphère,
registre `q4_emitted` égal aux records ; le lecteur exige aussi que les
nouveaux modes aient rejeté une masse non nulle (mesure non vide) et, à
1k/K5, que la sonde relancée en `disabled`/`scalar` émette le même flux
(mêmes comptes, empreintes xor/somme et IDs de coquille) ; à 8k/K5 ces comptes
sont ceux que le constructeur publie pour la série 32.

## Résultat : accord exact sur les 2 lignes

| n | K | résiduel q3 / q4 | rejeté par rectangle + par paire | paires développées | seeds q3 | boules q3 sonde = harnais | boules q4 sonde = harnais | lemme q3 paires / viol. | lemme q4 paires / viol. | disabled/scalar | s sonde / q3 / q4 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 8000 | 5 | 2 150 109 / 1 359 325 | 1 940 894 + 167 441 | 344 856 | 1 911 457 | 93 914 = 93 914 | 10 756 = 10 756 | 300 / 0 | 200 / 0 | oui | 4.7 / 349.7 / 112.1 |
| 8000 | 10 | 4 490 003 / 4 329 619 | 4 171 837 + 324 331 | 717 777 | 8 099 443 | 409 195 = 409 195 | 116 985 = 116 985 | 300 / 0 | 200 / 0 | — | 19.2 / 956.2 / 489.5 |

Lecture : les modes 32 ne changent pas l'objet émis (mêmes boules q3 et q4,
mêmes profondeurs que les énumérations indépendantes et que les modes 31), et
la colonne « rejeté » mesure la masse de paires retirée avant toute couverture
à 8 000 points. Une taille, un scan : pas de qualification, pas de FULL, pas de
pente.

## Rejouer

```bash
python3 -B -O morsehgp3D_v8/audits/q34_stream_crosscheck_t32_8k_20260921/run_t32_8k_crosscheck.py read --commit d1b4dbc6
git worktree add --detach /tmp/wt-d1b4dbc6 d1b4dbc6
cmake -S /tmp/wt-d1b4dbc6/morsehgp3D_v8 -B /tmp/wt-d1b4dbc6/build/v8-audit -DCMAKE_BUILD_TYPE=Release -DBOOST_ROOT=<boost> && cmake --build /tmp/wt-d1b4dbc6/build/v8-audit --parallel
python3 -B -O morsehgp3D_v8/audits/q34_stream_crosscheck_t32_8k_20260921/run_t32_8k_crosscheck.py run --worktree /tmp/wt-d1b4dbc6 --scratch /tmp/crosscheck32_8k --commit d1b4dbc6 --output /tmp/Q34_STREAM_CROSSCHECK_T32_8K.json
```
