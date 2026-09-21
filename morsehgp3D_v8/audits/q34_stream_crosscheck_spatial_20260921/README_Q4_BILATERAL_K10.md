# Voie q4 à K = 10 sur les sept morceaux spatiaux de la scène 0 : validité de chaque record émis et complétude par échantillon

Auditeur B, 21 septembre 2026. Moteur de la tranche 34 gelé, sonde construite à
**d6e1bd9e** en modes `rectangle-pair`, `boxes`, `affine` et atlas q4 `live`
(bloc 64), le mode de la campagne chronométrée du constructeur ; harnais
[q4_bilateral_probe.cpp](q4_bilateral_probe.cpp) (aucune brique q4 du moteur,
`lanes/q4_local.cpp` `0b4eef4cb5b295d0…` seulement épinglé).
`phase=exploration_v8_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
GCP non utilisé. Protocole partiel bilatéral, applicable à l'échelle de la
scène là où l'énumération exhaustive de la voie q4 ne l'est pas.

## Les deux directions

1. **Validité** : chaque record q4 émis (support de quatre IDs, profondeur,
   coquille complète) est rejugé indépendamment : arête propriétaire = arête
   de longueur maximale (égalité départagée par la plus petite clé d'IDs,
   règle `owned` du raccord), tétraèdre strictement positif (coordonnées
   barycentriques du centre en i128), profondeur exacte recalculée sur la
   couverture fermée |2z − a − b|² ≤ 4|b − a|² de l'arête propriétaire par
   descente d'index, coquille complète recalculée (support compris) et
   comparée aux IDs du record, profondeur < K − 2. Exigé : records valides =
   records émis, et `q4_emitted` de la sonde = nombre de records.
2. **Complétude par échantillon** : 2 000 paires tirées uniformément dans
   la masse résiduelle q4 du même front ; pour chaque paire conservée par le
   citron exact (descente saturante, α4 = 2), énumération de tous les
   tétraèdres propriétaires positifs de profondeur < K − 2 (comme les harnais
   exhaustifs des reçus T31 à T34) ; chaque boule (clé entière réduite,
   profondeur) doit figurer parmi les records. Exigé : 0 manquante.

Les morceaux sont ceux de la préparation indépendante
[SPATIAL_PIECES_SCAN0.json](SPATIAL_PIECES_SCAN0.json), identiques octet pour
octet aux fichiers du constructeur. Reçu
[Q4_BILATERAL_SPATIAL_K10.json](Q4_BILATERAL_SPATIAL_K10.json), rejoué par
`run_spatial_q4_bilateral_k10.py read`.

## Résultat : 7 morceaux, tous les records valides, aucune boule manquante

| morceau | n | K | résiduel q4 | records q4 | valides | boules distinctes | tests de validité | paires tirées / conservées | tétraèdres positifs | boules énumérées | manquantes | s sonde / harnais |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| quarter_xpos_ypos | 29 926 | 10 | 19 461 163 | 340 163 | 340 163 | 337 652 | 39 483 308 | 2 000 / 151 | 467 496 | 30 | 0 | 274 / 8.0 |
| quarter_xpos_yneg | 30 027 | 10 | 23 278 779 | 252 852 | 252 852 | 246 950 | 17 648 253 | 2 000 / 131 | 1 741 100 | 32 | 0 | 180 / 29.9 |
| quarter_xneg_yneg | 29 128 | 10 | 44 261 406 | 593 471 | 593 471 | 584 251 | 57 888 389 | 2 000 / 72 | 403 318 | 18 | 0 | 357 / 9.8 |
| quarter_xneg_ypos | 30 061 | 10 | 51 091 295 | 956 526 | 956 526 | 953 663 | 85 104 579 | 2 000 / 97 | 546 697 | 20 | 0 | 758 / 22.1 |
| half_xpos | 59 953 | 10 | 69 170 659 | 594 653 | 594 653 | 586 224 | 57 845 862 | 2 000 / 87 | 1 501 736 | 4 | 0 | 369 / 30.6 |
| half_xneg | 59 189 | 10 | 138 079 939 | 1 560 190 | 1 560 190 | 1 548 059 | 145 493 805 | 2 000 / 57 | 233 736 | 24 | 0 | 788 / 20.4 |
| full | 119 142 | 10 | 358 297 342 | 2 158 063 | 2 158 063 | 2 137 403 | 205 936 490 | 2 000 / 52 | 1 821 913 | 22 | 0 | 1121 / 38.8 |

Lecture : la direction « validité » est exhaustive sur les records (aucune boule
émise n'est fausse : propriétaire, positivité, profondeur et coquille exactes) ;
la direction « complétude » n'est établie que sur l'échantillon (une boule
manquante hors échantillon resterait invisible ; l'énumération exhaustive des
quarts, en cours, complète ce point). Rien n'est qualifié : un scan, K10 (le reçu K5 est [README_Q4_BILATERAL.md](README_Q4_BILATERAL.md)).

## Rejouer

```bash
python3 -B -O morsehgp3D_v8/audits/q34_stream_crosscheck_spatial_20260921/run_spatial_q4_bilateral_k10.py read --commit d6e1bd9e
python3 -B -O morsehgp3D_v8/audits/q34_stream_crosscheck_spatial_20260921/run_spatial_q4_bilateral_k10.py run --worktree /tmp/wt-d6e1bd9e --scratch /tmp/bilateral10 --commit d6e1bd9e --output /tmp/Q4_BILATERAL_SPATIAL_K10.json
```
