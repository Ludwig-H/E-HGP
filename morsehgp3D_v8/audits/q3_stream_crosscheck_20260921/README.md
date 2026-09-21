# Flux q3 du raccord global 31 contre une énumération exhaustive indépendante (scan 0, préfixes 1k à 4k)

Auditeur B, 21 septembre 2026. Sonde du constructeur `mhgp8_wspd_q34_probe`
construite à **4dbe3024** (arbre de travail détaché, Release ; sha256 du
binaire `0ec7a2f28063c50b…`), harnais de B
[q3_stream_probe.cpp](q3_stream_probe.cpp) compilé contre la même bibliothèque.
`phase=exploration_v8_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
GCP non utilisé. Contrôle d'exactitude du **calcul** de la voie q3 (même objet
calculé deux fois), pas une confrontation à une vérité terrain ni un jugement
de pertinence ; la voie q4 n'est pas jugée ici.

## Ce qui est comparé

Entrée : préfixes explicites de 1 000, 2 000 et 4 000 points du scan préparé
`single_000000/n8000.u16le` (sha256 `ed5aa97941551e02…`, les mêmes
préfixes que le pilote G4 R3 du constructeur), K = 5 (1k/2k/4k) et K = 10
(1k/2k), s = 8. Côté constructeur : `run_wspd_q34_parallel` par la sonde,
masque 2 (voie q3 seule), quatre workers, mode `records` (support trié,
profondeur, coquille par boule). Côté B : le harnais en mode exhaustif juge
**toutes** les paires résiduelles de la voie q3 émises par le même front :
compte exact du citron (rejetable si ≥ K − 1 sites), seeds aigus avec la
règle de propriété du raccord (arête ab maximale, égalité admise, départage
par la plus petite clé d'IDs), et pour chaque paire conservée le census
entier de chaque circumboule (Δ|z−a|² < (z−a)·N sur la couverture fermée
|2z−a−b|² ≤ 4|b−a|²) ; les boules de profondeur < K − 1 sont écrites avec
leur support et leur profondeur. Les paires rejetables sont censées
n'émettre rien (lemme du citron) ; le lemme est vérifié seed par seed sur
toutes les paires rejetables à 1k et sur les 2 000 (2k) ou 1 000 (4k)
premières.

Égalités exigées par le lecteur : masse résiduelle q3 du front, nombre de
seeds q3, nombre de boules émises (trois compteurs du moteur et deux du
harnais), et **multiensemble identique** des couples (support, profondeur)
(empreinte sha256 des listes canoniques). À 1k/K5 et 2k/K5 la sonde est
relancée avec le masque 6 : la voie q3 ne dépend pas du masque demandé.

## Résultat : accord exact sur les 5 lignes

| n | K | paires q3 résiduelles | rejetables | conservées | seeds q3 | boules émises | records = harnais | lemme paires / seeds / violations | tests moteur | tests harnais (conservées) | masque 6 | s sonde / harnais |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1000 | 5 | 95 716 | 78 419 | 17 297 | 3 680 448 | 10 481 | 10 481 = 10 481 | 78 419 / 3 575 314 / 0 | 308 288 385 | 4 836 060 | oui | 1.1 / 4.7 |
| 2000 | 5 | 271 254 | 233 814 | 37 440 | 21 906 400 | 21 948 | 21 948 = 21 948 | 2 000 / 111 615 / 0 | 3 327 513 432 | 28 359 265 | oui | 12.0 / 11.9 |
| 4000 | 5 | 735 174 | 656 899 | 78 275 | 121 684 394 | 45 151 | 45 151 = 45 151 | 1 000 / 180 592 / 0 | 33 553 852 030 | 86 707 639 | — | 90.4 / 59.4 |
| 1000 | 10 | 203 561 | 167 992 | 35 569 | 10 007 841 | 42 876 | 42 876 = 42 876 | 167 992 / 9 646 094 / 0 | 1 117 531 366 | 24 174 805 | — | 3.8 / 15.6 |
| 2000 | 10 | 597 066 | 519 342 | 77 724 | 54 205 647 | 92 994 | 92 994 = 92 994 | 2 000 / 207 118 / 0 | 10 213 243 651 | 111 989 271 | — | 29.5 / 37.2 |

Lecture : le moteur et le harnais trouvent les mêmes boules q3 avec les mêmes
profondeurs ; les seeds du moteur sont exactement les seeds aigus propriétaires
des paires résiduelles ; les « tests moteur » sont les tests ponctuels de son
census q3, les « tests harnais » ceux des seules paires conservées, le reste
étant réglé par le citron. Ce n'est pas une qualification : trois tailles
petites, un scan, pas de q4, pas de FULL.

## Rejouer

```bash
python3 -B -O morsehgp3D_v8/audits/q3_stream_crosscheck_20260921/run_crosscheck.py read
git worktree add --detach /tmp/wt-4dbe3024 4dbe3024
cmake -S /tmp/wt-4dbe3024/morsehgp3D_v8 -B /tmp/wt-4dbe3024/build/v8-audit -DCMAKE_BUILD_TYPE=Release -DBOOST_ROOT=<boost> && cmake --build /tmp/wt-4dbe3024/build/v8-audit --parallel
python3 -B -O morsehgp3D_v8/audits/q3_stream_crosscheck_20260921/run_crosscheck.py run --worktree /tmp/wt-4dbe3024 --scratch /tmp/crosscheck --output /tmp/Q3_STREAM_CROSSCHECK.json
```
