# Voie q4 sur les sept morceaux spatiaux de la scène 0 : validité de chaque record émis et complétude par échantillon

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
[Q4_BILATERAL_SPATIAL.json](Q4_BILATERAL_SPATIAL.json), rejoué par
`run_spatial_q4_bilateral.py read`.

## Résultat : 7 morceaux, tous les records valides, aucune boule manquante

| morceau | n | K | résiduel q4 | records q4 | valides | boules distinctes | tests de validité | paires tirées / conservées | tétraèdres positifs | boules énumérées | manquantes | s sonde / harnais |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| quarter_xpos_ypos | 29 926 | 5 | 7 414 164 | 28 854 | 28 854 | 28 481 | 1 975 875 | 2 000 / 227 | 319 833 | 5 | 0 | 109 / 2.5 |
| quarter_xpos_yneg | 30 027 | 5 | 10 202 121 | 23 513 | 23 513 | 22 483 | 952 035 | 2 000 / 141 | 27 422 | 3 | 0 | 64 / 1.4 |
| quarter_xneg_yneg | 29 128 | 5 | 21 362 288 | 52 870 | 52 870 | 51 484 | 2 878 533 | 2 000 / 77 | 644 679 | 4 | 0 | 141 / 5.0 |
| quarter_xneg_ypos | 30 061 | 5 | 21 606 715 | 84 249 | 84 249 | 83 803 | 4 169 309 | 2 000 / 101 | 608 608 | 12 | 0 | 254 / 17.2 |
| half_xpos | 59 953 | 5 | 20 414 006 | 52 472 | 52 472 | 51 067 | 2 964 331 | 2 000 / 151 | 5 015 881 | 3 | 0 | 144 / 72.2 |
| half_xneg | 59 189 | 5 | 54 171 726 | 137 708 | 137 708 | 135 871 | 7 161 583 | 2 000 / 53 | 181 235 | 4 | 0 | 276 / 6.5 |
| full | 119 142 | 5 | 110 339 751 | 190 405 | 190 405 | 187 142 | 10 288 599 | 2 000 / 61 | 1 509 280 | 1 | 0 | 426 / 18.3 |

Lecture : la direction « validité » est exhaustive sur les records (aucune boule
émise n'est fausse : propriétaire, positivité, profondeur et coquille exactes) ;
la direction « complétude » n'est établie que sur l'échantillon (une boule
manquante hors échantillon resterait invisible ; l'énumération exhaustive des
quarts, en cours, complète ce point). Rien n'est qualifié : un scan, K5.

## Rejouer

```bash
python3 -B -O morsehgp3D_v8/audits/q34_stream_crosscheck_spatial_20260921/run_spatial_q4_bilateral.py read --commit d6e1bd9e
python3 -B -O morsehgp3D_v8/audits/q34_stream_crosscheck_spatial_20260921/run_spatial_q4_bilateral.py run --worktree /tmp/wt-d6e1bd9e --scratch /tmp/bilateral --commit d6e1bd9e --output /tmp/Q4_BILATERAL_SPATIAL.json
```
