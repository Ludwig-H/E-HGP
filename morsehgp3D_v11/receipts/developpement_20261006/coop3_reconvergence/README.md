# Coop3 : régression GPU un fil corrigée ; feuille coopérative par paires rejetée

6 octobre 2026. Session gardée `v11.20261006.claudecoop3`, cible `us-central1-c / ehgp-v7-3b1d496aed430749ea7e049f`,
arrêt `TERMINATED` certifié. Source `9eee2ed4b` : la voie un fil reprend la boucle `extend` en ligne de 830473218. Cadre :
`exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`. Bancs `conforme` à K5/16
et K10/24 : dumps et registres identiques en CPU, GPU un fil et GPU coopératif, à froid comme à chaud.

## Médianes à chaud (ms)

| K, feuilles | Trame | `domain` CPU / GPU un fil / GPU coop | Exécuteur un fil (L4) / coop | Écriture un fil (L4) |
| --- | --- | --- | --- | --- |
| K5, 16 | ng00 | 209 / 222 / 237 | 59 (59) / 77 | 17 (17) |
| K5, 16 | ng01 | 170 / 191 / 196 | 54 (54) / 62 | 20 (20) |
| K5, 16 | ng02 | 204 / 209 / 230 | 52 (52) / 71 | 13 (13) |
| K10, 24 | ng00 | 827 / 703 / 760 | 342 (340) / 400 | 112 (111) |
| K10, 24 | ng01 | 658 / 591 / 604 | 302 (301) / 314 | 116 (116) |
| K10, 24 | ng02 | 779 / 675 / 704 | 321 (318) / 354 | 105 (105) |

**Critère écrit d'avance** (exécuteur GPU un fil ≤ 1,10 × L4) : **atteint**, à 1 % près des valeurs L4. La cause de la
régression de coop1 et coop2, la reconvergence des warps (reçu coop2), est confirmée par la mesure.

## Verdicts

**Feuille coopérative par paires : rejetée.** Une fois la voie un fil rétablie, elle est plus lente partout :
- exécuteur ×1,15 à ×1,38 à K5, ×1,04 à ×1,17 à K10 ;
- son comptage vaut à lui seul 1,6 à 2 × celui de la voie un fil.

Répartir les sous-arbres des paires laisse la divergence entière, et les feuilles émettrices rejouent leurs sous-arbres
pour écrire. Le code est retiré au commit suivant, comme L4. Ce qui reste de la piste « un warp par feuille » est le
parallélisme de données sur les sites (section Q de la note aux auditeurs), à juger d'abord sur le profil source de
coop2.

**Fait mesuré : à K10, feuilles de 24, le GPU un fil bat le CPU.**
- `domain` : −15 % sur ng00 (703 contre 827 ms), −10 % sur ng01, −13 % sur ng02 ;
- mur : −5 %, −4 %, −5 %.

C'est le même ordre de grandeur que dans la session L4 (`domain` −11 à −13 %).

## Pièces

| Fichier | Contenu |
| --- | --- |
| `plan.json` | Plan de session, avec le critère écrit d'avance |
| `launch.json` | Lancement |
| `receipt.json` | Contrôleur |
| `gpu_ab_report_k5.json`, `gpu_ab_report_k10.json` | Bancs |

`SHA256SUMS` couvre les autres fichiers.
