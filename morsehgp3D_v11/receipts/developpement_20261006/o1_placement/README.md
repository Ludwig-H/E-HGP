# Placement du pipeline des forêts (levier O1) : trois sessions G4, adoption

6 octobre 2026. Cadre : `exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`.
Cible `us-central1-c / ehgp-v7-3b1d496aed430749ea7e049f`, arrêt `TERMINATED` certifié après chaque session
(`receipt.json` de chaque dossier). Aucune mesure ne promeut un statut public.

**Objet.** Les tâches série lourdes du pipeline des ordres concurrents (publieurs des ordres K et K−1, suiveurs
verticaux des ordres K et K−1) tournent sur le premier fil de cœurs physiques dédiés. Les tâches légères prennent
les fils frères de ces cœurs, les résolveurs tous les fils des autres cœurs (`src/tower/forest_placement.cpp`,
bit 262144 de la sonde). C'est un ordonnancement seul : aucune décision ne lit l'horloge. Le plan est inactif sans
SMT, au-delà de K = 5, ou quand la machine a moins de cœurs physiques que de tâches lourdes (3 ou 4) plus quatre.

## Sessions

| Session | Source | Ce qui a été mesuré | Verdict |
| --- | --- | --- | --- |
| `claudeo1place` | `86b3cbf14` | option perdue au déplacement de `ForestParallel` (porte native : `placements=0`) | exclue de la décision ; témoin A/A involontaire |
| `claudeo1place2` | `9d10de213` | placement actif (`placements=72`) | critère écrit non atteint |
| `claudeo1place3` | `9d10de213` | confirmation, critère écrit avant la session, bras A/A explicite | **adopté** |

Statistique : `forest_ms` (étage des forêts), K = 5, W48, trames LiDAR ng00, ng01 et ng02. Deux bras : CPU, feuilles
de 16 (masques 16379 et 278523) ; GPU avec arithmétique étroite, feuilles de 24 (81915 et 344059). Toutes les prises
rendent les vidages des empreintes des trames ; bancs `conforme`, aucun refus.

**`claudeo1place`.** Les deux bras exécutaient le même code. Les rapports par trame, de 0,875 à 1,092, mesurent donc
le bruit d'une comparaison : jusqu'à ±12 % à froid (médiane de 5 processus) et ±9 % à chaud (un processus). La
correction (`9d10de213`) transmet l'option au déplacement et contrôle sur toute machine que la demande parvient au
pipeline (mutant `placement_perdu_au_deplacement`).

**`claudeo1place2`** (critère : forêts à chaud ≤ 0,92 sur au moins deux trames du bras CPU, aucune > 1,03).

| Trame | CPU froid | CPU chaud | GPU froid | GPU chaud |
| --- | ---: | ---: | ---: | ---: |
| ng00 | 0,887 | 0,912 | 0,862 | 0,885 |
| ng01 | 0,975 | 0,839 | 0,888 | 0,944 |
| ng02 | 0,821 | **1,038** | 0,836 | 0,916 |

Onze comparaisons favorables sur douze, mais ng02 à chaud dépasse 1,03 : **critère non atteint**. Il n'a pas été
réécrit après coup ; une confirmation a été écrite à la place, sur données neuves.

**`claudeo1place3`** (plan dans `claudeo1place3/plan.json`, écrit avant la session). Statistique primaire :
médiane à froid de 6 processus neufs par trame et par mode, carré de Williams à trois modes (base, A/A, placement).
Validité : moyenne géométrique des six rapports A/A dans [0,97 ; 1,03]. Adoption : moyenne géométrique des six
rapports placement/base ≤ 0,93 et aucun rapport > 1,00.

| Bras | Trame | base (ms) | A/A | placement | A/A ÷ base | placement ÷ base |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| CPU | ng00 | 166,6 | 150,0 | 152,0 | 0,901 | 0,913 |
| CPU | ng01 | 135,8 | 128,1 | 116,8 | 0,944 | 0,861 |
| CPU | ng02 | 154,9 | 163,4 | 149,8 | 1,055 | 0,967 |
| GPU | ng00 | 166,2 | 164,8 | 149,5 | 0,992 | 0,899 |
| GPU | ng01 | 127,5 | 129,3 | 118,4 | 1,014 | 0,929 |
| GPU | ng02 | 154,0 | 164,5 | 144,6 | 1,068 | 0,939 |

Moyenne géométrique A/A **0,994** (session valide) ; placement/base **0,917**, pire rapport **0,967** : **adopté**.
Les prises placées sont aussi plus regroupées : ng01 CPU 116 à 126 ms contre 131 à 141 ms pour la base. À chaud, un
processus par mode, le bruit A/A atteint ±15 % (0,887 à 1,146) ; ces médianes restent descriptives.

## Adoption

Commit `12ce8f8f0` : la façade passe au masque 278523 (`api_detail::full_params`, `kEngineMask`, porte
`mhgp11_api_session_engine`), et `bench/points_export.cpp` suit. L'ordre seul garde le masque 7035. Le placement hors
pipeline des ordres concurrents est refusé (`ForestParallel::validate`, porte `mhgp11_tower_placement_validate`).
Mutants `placement_api_eteint` et `placement_hors_pipeline_accepte`, à tuer dans la session suivante. Le même commit
applique deux propositions de l'auditeur : lecteur de campagne aligné sur les masques de la sonde, et témoin
`pipeline_placement_cores` gardé par prise dans `gpu_ab`.

**Limites.** Gain d'environ 8 % sur l'étage des forêts à K = 5, borné par la file du publieur de l'ordre 5. Aucun
effet attendu à K = 10, où le plan est inactif. La topologie est lue dans `/sys` ; le plan compte les cœurs, il ne
certifie pas chaque appel d'affinité accepté par le noyau.

## Pièces

Par session : `plan.json`, `launch.json`, `receipt.json` (reçu du contrôleur), `gpu_ab_report_k5_16_cpu.json` et
`gpu_ab_report_k5_24_gpu.json` ; pour les deux premières, `portes.txt` (lignes de verdict des portes natives de
placement et de pipeline). Aucune donnée ni coordonnée LiDAR : seuls les noms et les empreintes des fichiers d'entrée.
`SHA256SUMS` couvre tous les fichiers sauf lui-même.
