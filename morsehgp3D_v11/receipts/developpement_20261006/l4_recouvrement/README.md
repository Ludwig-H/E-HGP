# Recouvrement L4 des feuilles (sous-lots pendant la passe des tâches) : A/B G4, rejeté

6 octobre 2026, session gardée `v11.20261006.claudeL4`, cible `us-central1-c / ehgp-v7-3b1d496aed430749ea7e049f`,
arrêt `TERMINATED` certifié. Source `cf28afb04`, un seul binaire CUDA sm_120 (`bench/gpu_ab.py`) : CPU 16379, GPU en
série 81915, GPU recouvert 212987 (16 sous-lots fixes par ordre de réclamation, fil dédié). Trames entières sans sol
ng00, ng01, ng02 ; K5 feuilles de 16 (5 prises à froid, 12 passes à chaud) et K10 feuilles de 24 (3 et 8) ; W48.
Verdict du banc : `conforme` (dumps et registres du catalogue identiques dans tous les modes). Cadre :
`exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`.

## Médianes à chaud (ms)

| K, feuilles | Trame | `domain` CPU | `domain` GPU série | `domain` GPU recouvert | exécuteur série / recouvert | mur CPU / série / recouvert |
| --- | --- | ---: | ---: | ---: | --- | --- |
| K5, 16 | ng00 | 207 | 230 | 354 | 59 / 234 | 370 / 386 / 504 |
| K5, 16 | ng01 | 168 | 196 | 330 | 54 / 220 | 292 / 316 / 459 |
| K5, 16 | ng02 | 202 | 218 | 327 | 52 / 197 | 357 / 387 / 499 |
| K10, 24 | ng00 | 827 | 721 | 2 720 | 340 / 2 362 | 2 491 / 2 351 / 4 358 |
| K10, 24 | ng01 | 655 | 603 | 2 668 | 301 / 2 379 | 1 828 / 1 778 / 3 834 |
| K10, 24 | ng02 | 779 | 682 | 2 698 | 318 / 2 326 | 2 079 / 1 995 / 3 995 |

**Critère écrit d'avance (plan T3)** : K10 recouvert au plus 0,85 × CPU résident, K5 recouvert au plus CPU. Non
atteint : le recouvrement est 1,6 à 3,5 fois plus lent que le CPU. **Rejeté et retiré** au commit suivant.

**Cause.** L'exécuteur traite une feuille par fil ; chaque lot attend sa feuille la plus lourde (queue de 13 à 117 ms
mesurée par Nsight le 4 octobre). Un lot unique paie une queue ; 16 sous-lots exécutés en série en paient 16, et l'ordre
de réclamation (les plus lourdes d'abord) concentre les grosses feuilles dans les premiers. Tant que la feuille n'est pas
coopérative (un warp par feuille, levier J3), découper le lot ne peut rien gagner.

**Fait annexe mesuré.** À K10 avec des feuilles de 24, le GPU en série bat déjà le CPU : `domain` −11 à −13 %, mur
−4 à −6 %, sur les trois trames, à chaud comme à froid.

## Pièces

`plan.json`, `launch.json`, `receipt.json` (contrôleur), `gpu_ab_report_k5.json`, `gpu_ab_report_k10.json` (banc :
prises froides et chaudes, médianes, identités, registres). `SHA256SUMS` couvre les autres fichiers.
