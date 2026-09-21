# Flux q4 (Local28) du raccord global 31 contre une énumération exhaustive indépendante (scan 0, préfixes 1k à 4k)

Auditeur B, 21 septembre 2026. Sonde du constructeur `mhgp8_wspd_q34_probe`
construite à **4dbe3024** (arbre de travail détaché, Release ; sha256 du
binaire `0ec7a2f28063c50b…`, `lanes/q4_local.cpp`
`0d52f1446b1a4d4a…`), harnais de B
[q4_stream_probe.cpp](q4_stream_probe.cpp) compilé contre la même bibliothèque
mais n'appelant aucune brique q4 du moteur (front et index seulement).
`phase=exploration_v8_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
GCP non utilisé. Contrôle d'exactitude du **calcul** de la voie q4 (même objet
calculé deux fois), pas une confrontation à une vérité terrain ni un jugement
de pertinence ; les coquilles ne sont pas comparées.

## Ce qui est comparé

Entrée : préfixes explicites de 1 000, 2 000 et 4 000 points du scan préparé
`single_000000/n8000.u16le` (sha256 `ed5aa97941551e02…`), K = 5
(1k/2k/4k) et K = 10 (1k/2k), s = 8. Côté constructeur : `run_wspd_q34_parallel`
par la sonde, masque 4 (voie q4 seule), backend Local28, quatre workers, mode
`records` (support à quatre IDs, profondeur, coquille). Côté B : pour
**toutes** les paires résiduelles de la voie q4 du même front, compte exact du
citron L_4 (rejetable si ≥ K − 2 sites), puis pour chaque paire conservée
l'énumération de tous les tétraèdres (a,b,x,y) à x < y dans la couverture
fermée |2z−a−b|² ≤ 4|b−a|², propriétaires de ab (règle `owned` du raccord :
cinq autres arêtes ≤ |ab|², égalité admise sauf clé d'IDs plus petite),
strictement positifs (δ = det(u,v,w) ≠ 0 et quatre coordonnées barycentriques
du centre > 0, en i128), de profondeur exacte < K − 2 (sites de la couverture
strictement intérieurs, hors support). Chaque boule est écrite avec sa clé
entière réduite A|z|² + B·z + C et sa profondeur. Le lemme du citron q4 est
vérifié tétraèdre par tétraèdre sur toutes les paires rejetables à 1k, sur
2 000 paires à 2k et 500 à 4k.

Égalités exigées par le lecteur : masse résiduelle q4 du front, **ensemble
identique de boules distinctes (clé, profondeur)** (la clé de chaque record de
la sonde est recalculée en entiers Python depuis son support, et chaque
support doit être sur sa propre sphère), registre `q4_emitted` égal au nombre
de records ; à 1k et 2k (K5) la sonde est relancée avec le masque 6 : la voie
q4 ne dépend pas du masque demandé.

## Résultat : accord exact sur les 5 lignes

| n | K | paires q4 résiduelles | rejetables | conservées | tétraèdres propriétaires | positifs | émis sonde = harnais | boules distinctes sonde = harnais | lemme paires / positifs / violations | masque 6 | s sonde / harnais |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1000 | 5 | 61 555 | 45 918 | 15 637 | 3 284 932 | 37 710 | 876 = 876 | 876 = 876 | 45 918 / 1 059 078 / 0 | oui | 1.2 / 38.6 |
| 2000 | 5 | 162 155 | 127 921 | 34 234 | 22 882 205 | 309 827 | 2 113 = 2 113 | 2 113 = 2 113 | 2 000 / 22 650 / 0 | oui | 8.3 / 5.0 |
| 4000 | 5 | 470 334 | 398 272 | 72 062 | 83 821 522 | 1 966 921 | 4 798 = 4 798 | 4 798 = 4 798 | 500 / 1 231 / 0 | — | 52.2 / 33.7 |
| 1000 | 10 | 185 194 | 148 157 | 37 037 | 22 095 715 | 175 643 | 8 033 = 8 033 | 8 033 = 8 033 | 148 157 / 3 869 169 / 0 | — | 13.9 / 263.7 |
| 2000 | 10 | 550 698 | 469 080 | 81 618 | 106 485 169 | 1 176 379 | 20 194 = 20 194 | 20 194 = 20 194 | 2 000 / 57 838 / 0 | — | 31.6 / 33.9 |

Lecture : le moteur et le harnais trouvent les mêmes boules q4 avec les mêmes
profondeurs, et chaque boule est émise une fois de chaque côté (pas de plateau
cosphérique sur ces préfixes). Les « tétraèdres propriétaires » et
« positifs » sont ceux des seules paires conservées ; sur les paires
rejetables, aucun tétraèdre positif propriétaire n'a de profondeur < K − 2.
Ce n'est pas une qualification : trois tailles petites, un scan, pas de
coquilles, pas de FULL.

## Rejouer

```bash
python3 -B -O morsehgp3D_v8/audits/q4_stream_crosscheck_20260921/run_q4_crosscheck.py read
git worktree add --detach /tmp/wt-4dbe3024 4dbe3024
cmake -S /tmp/wt-4dbe3024/morsehgp3D_v8 -B /tmp/wt-4dbe3024/build/v8-audit -DCMAKE_BUILD_TYPE=Release -DBOOST_ROOT=<boost> && cmake --build /tmp/wt-4dbe3024/build/v8-audit --parallel
python3 -B -O morsehgp3D_v8/audits/q4_stream_crosscheck_20260921/run_q4_crosscheck.py run --worktree /tmp/wt-4dbe3024 --scratch /tmp/crosscheck4 --output /tmp/Q4_STREAM_CROSSCHECK.json
```
