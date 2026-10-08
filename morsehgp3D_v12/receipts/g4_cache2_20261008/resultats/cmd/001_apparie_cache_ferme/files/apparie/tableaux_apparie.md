# Pilote apparie : aa = --cache=0, cache = (defaut), ref = --cache=0

Reference : `ref` ; A/A : `aa` ; voie appareil, K5, 48 fils, 10 tours x 10 passes. Jugement : **juge**.

| bras | trame | rapport a la reference (IC 95 %) | verdict |
| --- | --- | --- | --- |
| `aa` | ng00 | 0.995 (0.985-1.005) | controle A/A |
| `aa` | ng01 | 1.008 (1.001-1.014) | controle A/A |
| `aa` | ng02 | 0.995 (0.987-1.002) | controle A/A |
| `cache` | ng00 | 0.922 (0.917-0.927) | adopte |
| `cache` | ng01 | 0.925 (0.920-0.931) | adopte |
| `cache` | ng02 | 0.912 (0.906-0.917) | adopte |

Murs chauds medians des prises (ms) et etages (mediane des medianes de processus) :

| trame | bras | mur | P | C | G | queue ou T+M+V+R | CPU par passe | pic (Mio) |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| ng00 | `aa` | 94.1 | 2.0 | 27.5 | 57.4 | 6.0 | 2821.9 | 1145 |
| ng00 | `cache` | 87.2 | 2.0 | 25.5 | 54.3 | 5.4 | 2575.6 | 1162 |
| ng00 | `ref` | 94.7 | 2.0 | 27.6 | 57.9 | 5.4 | 2824.7 | 1145 |
| ng01 | `aa` | 77.9 | 1.7 | 24.0 | 45.0 | 5.6 | 2185.8 | 957 |
| ng01 | `cache` | 71.8 | 1.9 | 22.4 | 40.0 | 7.6 | 1981.4 | 968 |
| ng01 | `ref` | 77.8 | 1.7 | 23.9 | 44.9 | 5.6 | 2179.5 | 958 |
| ng02 | `aa` | 96.4 | 1.9 | 28.1 | 49.5 | 14.6 | 2656.3 | 1190 |
| ng02 | `cache` | 88.1 | 1.9 | 25.6 | 46.2 | 14.3 | 2424.7 | 1203 |
| ng02 | `ref` | 96.4 | 1.9 | 28.2 | 49.3 | 15.0 | 2662.1 | 1189 |

Session v12set (information, second passage, mediane des processus) :

| bras | mediane (ms) | maximum (ms) | trames | refus |
| --- | ---: | ---: | ---: | --- |
| `aa` | 161.3 | 325.0 | 37 | - |
| `cache` | 147.8 | 297.5 | 37 | - |
| `ref` | 160.2 | 318.9 | 37 | - |
