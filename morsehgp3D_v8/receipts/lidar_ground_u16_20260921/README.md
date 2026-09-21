# Nuages sans sol au profil u16 (2 cm) : entrée de calcul du moteur historique

21 septembre 2026, développeur v8 (reprise). Cadre : `exploration_v8_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`, `public_status=not_claimed`.
GCP non utilisé. Ce reçu prépare une **entrée de calcul**, il ne mesure rien et ne
juge pas la qualité sémantique du masque.

## Pourquoi

Le régime prioritaire (décision utilisateur du 21 septembre) est le LiDAR
SemanticKITTI **sans sol**, 30 000 à 60 000 sites, où les contrats temps (1 s puis
100 ms) et K (5 ou 10) doivent passer. Les préparations sans sol qualifiées
([lidar_ground_20260921](../lidar_ground_20260921/README.md)) n'existent qu'aux
profils float32 et grille 1 mm, que le seul générateur global existant
(`run_wspd_q34_parallel`, u16 à 2 cm) ne lit pas. Ce préparateur applique le même
masque par retour (Patchwork++ 3e6903a1, état neuf, RNR inactif, capture
`ground_fq64xq_6`, fichier `mask.u8` égal à `scene_XX_repeat_0.u8`) à la même
quantification que la préparation spatiale u16 : q = floor(50·x + 32768 + 1/2) en
rationnels exacts depuis le float32 décodé, déduplication globale, plans qx = 32768
puis qy = 32768 (site sur le plan du côté ≥). Règle du pilote : un site u16 est
conservé si au moins un de ses retours n'est pas étiqueté sol ; les sites à
décisions mixtes sont comptés.

## Résultat

| scène | trame | retours | sites u16 bruts | sites conservés | mixtes | moitiés x− / x≥ | quarts (x−y−, x−y≥, x≥y−, x≥y≥) |
| --- | --- | ---: | ---: | ---: | ---: | --- | --- |
| 00 | 000000 | 123 389 | 119 142 | 39 815 | 0 | 24 556 / 15 259 | 11 508, 13 048, 8 192, 7 067 |
| 01 | 000100 | 124 479 | 119 942 | 35 491 | 1 | 18 968 / 16 523 | 8 055, 10 913, 9 212, 7 311 |
| 02 | 000200 | 125 526 | 120 725 | 45 114 | 1 | 25 023 / 20 091 | 16 234, 8 789, 14 810, 5 281 |

Les effectifs float32 du pilote (39 885 / 35 551 / 45 845 retours conservés)
diffèrent des sites u16 par les fusions à 2 cm parmi les retours conservés
(70 / 60 / 731). Les sept fichiers `.u16le` par scène (6 octets par site, ordre
lexicographique) sont écrits sous `audits/lidar08_20260914/prepared/ground_u16/`
(non versionné, régénérable) ; [MANIFEST.json](MANIFEST.json) porte les sha256 du
brut, du masque et de chaque morceau, les effectifs et les conventions.

## Rejouer

```bash
python3 -B -O morsehgp3D_v8/bench/prepare_lidar_ground_u16.py read
python3 -B -O morsehgp3D_v8/bench/prepare_lidar_ground_u16.py run --out-dir /tmp/ground_u16 --manifest /tmp/GROUND_U16.json
```

`read` régénère les 21 morceaux en mémoire depuis les `.bin` bruts (non versionnés,
voir [audits/lidar08_20260914/README.md](../../audits/lidar08_20260914/README.md))
et le masque versionné, puis exige l'égalité des sha256 et des effectifs.
