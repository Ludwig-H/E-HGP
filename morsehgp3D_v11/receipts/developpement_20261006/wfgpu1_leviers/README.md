# Workflow GPU (quatre leviers) et session wfgpu1 : C adopté, A rejeté par la mesure

6 octobre 2026. Cadre : `exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`.

## Workflow `wf_d7937a06-901` (14 h 30 – 16 h 27 UTC)

Huit agents ont travaillé sur la base `905fad2e1` : un implémenteur par levier, chacun dans son worktree de `/tmp`,
sans commit, puis un vérificateur adverse par levier. Le script est dans `workflow/script_workflow.js`, et les retours
structurés des huit agents dans `workflow/resultats_agents.json`.

| Levier | Implémenteur | Contre-lecture | Suite |
| --- | --- | --- | --- |
| A, réservoir sans appel (blocs entrelacés, `__syncwarp`) | prêt pour G4 | favorable sous réserves (couverture des refus défensifs, deux mutants à comportement indéfini, porte redondante, attribution) | mesuré, rejeté |
| B, feuilles lourdes au CPU (hybride) | abandonné : 1,1 % du travail routé à K5, prédicteur sur le chemin critique | favorable sous réserves, abandon confirmé | non intégré |
| C, arithmétique étroite (J2, q3, q4 en i32/i64 sous étendue ≤ 2^20) | prêt pour G4 | favorable sous réserves (nouvelle classe de feuilles non résolues, refaites en série ; aucune sur les trames) | mesuré, adopté |
| D, queue des feuilles lourdes (ordre par travail) | abandonné sur modèle ; sonde de diagnostic livrée | favorable sous réserves (le modèle n'est pas réfuté ; mutant survivant, refus non gardés) | non intégré |

Les rapports et les patches des quatre leviers sont gardés tels que livrés (`workflow/`). Seul C entre dans le code.

## Session G4 `v11.20261006.claudewfgpu1`

Cible `us-central1-c / ehgp-v7-3b1d496aed430749ea7e049f`, arrêt `TERMINATED` certifié. Source `05db6f5b7` (code de
`905fad2e1`).

Le banc `bench/gpu_ab.py --variants` a construit avec CUDA cinq variantes, comparées en mode GPU 81915 dans l'ordre de
Williams :
- `new` : la source ;
- `sync` : `__syncwarp` seul ;
- `A`, `C` et `AC` : les patches du workflow.

Les sommes des archives sont dans `session/archives_variantes.sha256`. Pour `AC`, la section `tests/mutants/tower.json`
de C a été retirée : le manifeste est sans effet sur la mesure, et les deux patches y entrent en conflit.

**Exactitude.**
- Les trois bancs sont `conforme`.
- Toutes les variantes rendent les mêmes dumps et registres, égaux aux empreintes CPU de la session reservoir3 sur
  les trois trames, à K5/16, K5/24 et K10/24.
- `unresolved` vaut 0 dans toutes les variantes.
- `gpu_sanitizer` sur `AC` (memcheck, racecheck, synccheck) : zéro erreur.

**Critère écrit d'avance** (plan, sur les médianes à chaud par trame) :
- A est adopté si son comptage vaut au plus 0,90 × `new` à K5/16 et à K10/24 sur les trois trames ;
- C est adopté si son comptage vaut au plus 0,95 × `new` aux mêmes points ;
- AC est adopté si A et C le sont tous deux ;
- `sync` remplace A si son comptage vaut au plus 1,02 × A partout.

**Rapport du comptage à `new`** :

| Configuration | Trame | `sync` | A | C | AC |
| --- | --- | ---: | ---: | ---: | ---: |
| K5/16 | ng00 | 1,01 | 1,11 | 0,83 | 0,92 |
| K5/16 | ng01 | 1,00 | 1,11 | 0,84 | 0,92 |
| K5/16 | ng02 | 1,00 | 1,12 | 0,84 | 0,94 |
| K10/24 | ng00 | 1,00 | 0,98 | 0,85 | 0,84 |
| K10/24 | ng01 | 1,00 | 0,99 | 0,85 | 0,85 |
| K10/24 | ng02 | 1,00 | 1,00 | 0,86 | 0,86 |
| K5/24 (descriptif) | ng00 / ng01 / ng02 | 1,00 / 1,00 / 1,00 | 0,95 / 0,92 / 0,93 | 0,86 / 0,87 / 0,87 | 0,83 / 0,81 / 0,83 |

**Verdicts.**
- **C est adopté** : il passe le critère sur les trois trames.
- **A est rejeté** : il est plus lent à K5/16 et neutre à K10, alors que son SASS statique (10 352 instructions, 0
  `CALL`) était le meilleur des variantes. Le compte statique d'instructions ne prédit pas le temps.
- **AC n'est pas adopté**, puisque A ne l'est pas.
- **`sync` est sans effet** : le repli collectif de l'épilogue n'est pas joué à l'exécution, comme le rapport de A le
  prévoyait.

**`domain` avec C** (ms) :

| Configuration | ng00 | ng01 | ng02 | Référence |
| --- | ---: | ---: | ---: | --- |
| K5/16 | 211 | 177 | 205 | CPU reservoir3 : 210, 171, 205 |
| K5/24 | 177 | 158 | 182 | meilleure voie actuelle à K5 |
| K10/24 | 589 | 475 | 563 | CPU : 835, 660, 796 |

À K5/24, la voie GPU avec C est la meilleure à K5. Mur K5/24 : 352, 285 et 349 ms. L'étage des forêts (CPU, 127 à
176 ms selon la prise) n'est pas touché par ces leviers.

## Pièces

| Dossier | Contenu |
| --- | --- |
| `workflow/` | Rapports, patches, script, retours des agents |
| `session/` | Plan avec critère, lancement, contrôleur, trois bancs, sanitizer, sommes des archives |

`SHA256SUMS` couvre l'ensemble.
