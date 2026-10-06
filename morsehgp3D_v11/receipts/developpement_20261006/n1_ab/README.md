# Levier N1 (arène de pile du parcours des boîtes) : A/B G4, hypothèse réfutée

6 octobre 2026, session gardée `v11.20261006.claudeN1`, cible `us-central1-c / ehgp-v7-3b1d496aed430749ea7e049f`,
arrêt `TERMINATED` certifié. Source `c72c5a576` (N1) contre l'archive de sa base `8ee28873f` (sans `receipts/`), banc
`bench/ab_g4.py`, trames entières sans sol ng00, ng01, ng02, K = 5, u21, mode CPU 16379, 12 prises appariées par
trame à W48 (ordre tourné et inversé) et une à W1. Cadre : `exploration_v11_hors_registre / cpu_reference /
quantized_u21_input_only / not_claimed`.

**Statut du verdict du banc : `refus`, pour une seule raison de mon fait** : plancher de portes fixé à 150 alors que
la sélection `^mhgp11_catalogue_` en compte 145. Les 145 portes passent, les deux mutants de l'arène sont tués, les
78 prises ont toutes leur dump identique à l'empreinte de la trame. Les chronos ci-dessous sont complets.

## Résultat (médianes, différence appariée N1 − base, 12 paires par trame)

| Trame | `domain` base | `domain` N1 | diff. médiane | paires négatives | CPU par passe base / N1 |
| --- | ---: | ---: | ---: | ---: | --- |
| ng00 | 222,7 ms | 221,5 ms | +5,2 ms | 4/12 | 13,8 / 13,7 s |
| ng01 | 188,4 ms | 201,8 ms | +8,5 ms | 5/12 | 10,7 / 10,6 s |
| ng02 | 226,1 ms | 221,4 ms | +2,4 ms | 4/12 | 12,7 / 12,6 s |

À W1 : `domain` 5 338 / 5 333 ms (ng00), 4 277 / 4 272 ms (ng01), 5 116 / 5 129 ms (ng02).

**Critère écrit d'avance** (note aux auditeurs, section L) : différence appariée médiane de `domain` au plus −15 ms
avec au moins 10 paires négatives sur 12. **Non atteint sur aucune trame : N1 est rejeté.** L'hypothèse du plan GPU
(« le surcoût par nœud à W48 vient des allocations et des atomiques du budget ») est réfutée : retirer toute
allocation et tout atomique par nœud ne change ni le mur ni le CPU par passe.

## Explication retenue, mesurée

`lscpu` de la VM : AMD EPYC 9B45, **24 cœurs physiques, 2 fils par cœur**, 3 instances de L3 de 32 Mio. À W48, deux
fils frères partagent un cœur. Le CPU par passe monte de 9,1 s (W1) à 13,8 s (W48) sur ng00, soit le facteur
attendu du SMT, et 9,1 s sur 24 cœurs donnent environ 380 ms, le mur mesuré. Le « coût par nœud » de 6 à 7 µs du plan
était un temps de fil SMT, pas un surcoût retirable. Le pool ne tourne pas à vide (attente sur variables de
condition, `src/sched/pool.cpp`).

Conséquence : sur CPU, la tour est bornée par le travail divisé par 24 cœurs ; pour 100 ms il faut retirer du
travail (facteur 3 à 4) ou le délester réellement vers le GPU. N1 est retiré du code au commit suivant.

## Pièces

`plan.json` (plan de session), `launch.json`, `receipt.json` (contrôleur), `ab_report.json` (banc : constructions,
portes, mutants, 78 prises avec leurs chronos par étage et leurs empreintes). `SHA256SUMS` couvre les autres fichiers.
