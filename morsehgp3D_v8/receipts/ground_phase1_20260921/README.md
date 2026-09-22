# Campagne appariée après les tranches de phase 1 et 2 sur les trois nuages sans sol

21 et 22 septembre 2026, développeur v8. Cadre : `exploration_v8_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`public_status=not_claimed`. GCP non utilisé. Même lanceur, mêmes entrées et
même grille que la [base de temps](../ground_baseline_20260921/README.md) ;
sonde construite à **0e2c18ca** (sha256 `bb7f01fceccc…`, copie figée pendant
la campagne) avec le jeton `atlas` (schéma de sonde v5) : atlas q4 à
l'échelle 2^20 en i64 (748ec082), graines q3 certifiées par l'atlas (0948d2d0)
et rectangles résiduels publiés comme tâches à la file bornée de l'équipe
(5224ff4e). Sorties et compteurs d'émission identiques à la référence sur les
neuf lignes ; identité W1/W8 vérifiée par le lecteur.

Ce reçu mesure un **flux de candidats q3/q4**, pas la tour ; hôte partagé de
8 vCPU (4 cœurs physiques SMT) ; il oriente le développement sans rien qualifier.

## Résultats

`BASELINE.json` (statut `partial`, six lignes : scènes 01 et 02) : campagne
complète exécutée **en concurrence avec la fin de la campagne de référence**
(sentinelle périmée, voir ci-dessous) ; ses trois lignes de la scène 0 ont
perdu leurs fichiers bruts, écrasés par la reprise (mêmes noms de fichiers
dans le lanceur d'alors) : leurs valeurs figurent ci-dessous comme **non
vérifiables**. `BASELINE.only.json` (fichiers `only_*`) : la scène 0 rejouée
sur hôte calme (charge < 3 avant chaque ligne, un harnais d'audit sur un
cœur), vérifiable. Le lanceur refuse désormais de recouvrir un reçu ou un
fichier de mesure existant.

| scène | K | W | référence mur / CPU (s) | phase 1+2 mur / CPU (s) | scène 0 rejouée au calme mur / CPU (s) | rapport CPU | rapport mur (calme) |
| --- | --- | --- | --- | --- | --- | ---: | ---: |
| 00 | 5 | 1 | 889,5 / 888,7 | 455,2 / 455,1 (non vérifiable) | 453,3 / 453,3 | ×1,96 | ×1,96 |
| 00 | 5 | 8 | 298,0 / 1 283,4 | 120,6 / 741,4 (non vérifiable) | 108,0 / 741,5 (686 %) | ×1,73 | ×2,76 |
| 00 | 10 | 8 | 911,1 / 3 876,5 | 359,9 / 2 210,1 (non vérifiable) | 323,0 / 2 212,1 (684 %) | ×1,75 | ×2,82 |
| 01 | 5 | 1 | 742,9 / 741,6 | 370,7 / 370,6 | — | ×2,00 | — |
| 01 | 5 | 8 | 273,2 / 992,8 | 98,4 / 601,8 | — | ×1,65 | — |
| 01 | 10 | 8 | 761,8 / 2 891,5 | 337,2 / 1 665,2 | — | ×1,74 | — |
| 02 | 5 | 1 | 2 043,8 / 1 886,9 (contaminée) | 852,2 / 852,0 | — | ×2,21 | — |
| 02 | 5 | 8 | 1 245,6 / 2 052,0 (contaminée) | 255,2 / 1 340,1 | — | ×1,53 | — |
| 02 | 10 | 8 | 3 533,9 / 5 860,2 (contaminée) | 824,1 / 3 811,0 | — | ×1,54 | — |

Lecture : à charge comparable (scène 0 au calme), le travail CPU est divisé par
1,95 (K5) et par 1,75 (K10), et le mur à huit workers par 2,8, l'occupation
passant de 425–430 % à 684–686 % des 800 % disponibles grâce à la file de
tâches. Les rapports CPU des scènes 01 et 02 sont du même ordre (×1,53 à
×2,21) malgré la contamination croisée. Le régime le plus dur reste la scène
02 (45 114 sites) : K10 à huit workers 824 s mur, 3 811 CPU·s. Contre le
contrat (48 CPU·s pour 1 s sur 48 cœurs), K10 sur la scène 0 demande encore
×46 de travail en moins à parallélisme parfait ; le chemin GPU et la
réduction du travail restent devant.

## Charge concurrente à déclarer

La campagne a démarré à 22:52 UTC alors que la référence n'avait pas fini :
une sentinelle `baseline.done` périmée (créée par un premier lancement échoué
à 21:14) a déclenché l'enchaînement. Les lignes des scènes 01 et 02 des deux
campagnes se sont chevauchées (charge avant ligne jusqu'à 16) ; d'où la
reprise de la scène 0 au calme dans `BASELINE.only.json`. Les compteurs et
les sorties ne dépendent pas de la charge.

## Rejouer

```bash
python3 -B -O morsehgp3D_v8/bench/run_ground_baseline.py read --output morsehgp3D_v8/receipts/ground_phase1_20260921
python3 -B -O morsehgp3D_v8/bench/run_ground_baseline.py read --output morsehgp3D_v8/receipts/ground_phase1_20260921 --variant only
python3 -B -O morsehgp3D_v8/bench/run_ground_baseline.py run --probe <sonde 0e2c18ca> --extra atlas --output /tmp/ground_phase1
python3 -B -O morsehgp3D_v8/bench/run_ground_baseline.py run --probe <sonde 0e2c18ca> --extra atlas --only 00,5,1 00,5,8 00,10,8 --output /tmp/ground_phase1
```
