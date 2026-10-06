# Reservoir3 : chemin chaud du puits rétabli, critère non atteint ; feuilles de 24 à K5 gagnantes sur GPU

6 octobre 2026. Session gardée `v11.20261006.claudereservoir3`, cible `us-central1-c / ehgp-v7-3b1d496aed430749ea7e049f`,
arrêt `TERMINATED` certifié. Source `79fa5e9f7`. Cadre : `exploration_v11_hors_registre / cpu_reference /
quantized_u21_input_only / not_claimed`. Trois bancs `conforme` : dumps et registres identiques dans tous les modes.

## Critère écrit d'avance : non atteint

Critère : comptage ≤ 1,05 × j2memo et exécuteur ≤ 0,85 × j2memo.

| K, feuilles | Trame | Comptage (j2memo) | Rapport | Exécuteur (j2memo) | Rapport | Écriture |
| --- | --- | --- | ---: | --- | ---: | ---: |
| K5, 16 | ng00 | 39,0 (30,2) | 1,29 | 45,1 (48,8) | 0,92 | 0,3 |
| K5, 16 | ng01 | 32,0 (24,9) | 1,29 | 37,2 (44,5) | 0,84 | 0,3 |
| K5, 16 | ng02 | 36,4 (28,4) | 1,28 | 42,3 (42,9) | 0,99 | 0,4 |
| K10, 24 | ng00 | 226,4 (171,6) | 1,32 | 241,3 (261,5) | 0,92 | 1,7 |
| K10, 24 | ng01 | 183,2 (139,1) | 1,32 | 195,3 (229,2) | 0,85 | 1,5 |
| K10, 24 | ng02 | 209,5 (159,4) | 1,31 | 224,4 (244,1) | 0,92 | 1,8 |

**Ce que montre la mesure.**
- Le réservoir supprime l'écriture.
- Le comptage reste 1,28 à 1,32 fois plus lent qu'en j2memo, même avec le chemin chaud rétabli.
- Les SASS locaux en donnent la cause (`nvcc` 12.9, sm_120, noyau de comptage) :

| Variante | Instructions | `BSSY` | `CALL` |
| --- | ---: | ---: | ---: |
| ancien puits seul, sur la nouvelle arène | 10 408 | 107 | 0 |
| j2memo | 10 352 | 107 | 0 |
| ancien puits avec appel au chemin froid hors ligne | 11 864 | 131 | 3 |

  C'est donc la seule présence de l'appel (un par instanciation du recensement) qui coûte, et non l'arène.

**Décision.**
- Le code est gardé : il reste meilleur que j2memo sur toutes les trames, avec un exécuteur ×0,84 à 0,99 à K5 et
  ×0,85 à 0,92 à K10.
- `domain` GPU à K5 : 218, 182 et 209 ms, contre 232, 196 et 224 en j2memo.
- Retrouver le comptage de j2memo sans perdre le réservoir est le levier A du workflow GPU suivant.

## Feuilles de 24 à K5 : mesure descriptive, sans critère

| Trame | `domain` CPU, feuilles 16 | `domain` CPU, feuilles 24 | `domain` GPU, feuilles 24 | Parcours GPU | Exécuteur GPU |
| --- | ---: | ---: | ---: | ---: | ---: |
| ng00 | 209,9 | 230,3 | 191,8 | 35,0 | 87,2 |
| ng01 | 171,4 | 189,2 | 169,1 | 28,6 | 81,4 |
| ng02 | 205,2 | 226,3 | 191,7 | 33,3 | 86,1 |

Le GPU en feuilles de 24 bat la meilleure voie CPU (feuilles de 16) sur `domain` de 9, 1 et 7 % : c'est la première
fois que le GPU gagne à K5. Le parcours y tombe à 29 à 35 ms ; l'exécuteur (81 à 87 ms) devient l'étage dominant. La
latence d'une seule feuille lourde sur un fil GPU fait la queue du comptage (relevé local du travail par feuille,
section T de la note).

## Pièces

| Fichier | Contenu |
| --- | --- |
| `plan.json` | Plan de session, avec le critère écrit d'avance |
| `launch.json` | Lancement |
| `receipt.json` | Contrôleur |
| `gpu_ab_report_k5.json`, `gpu_ab_report_k10.json` | Bancs |
| `gpu_ab_report_k5_feuilles24.json` | Feuilles de 24 à K5 |

`SHA256SUMS` couvre les autres fichiers.
