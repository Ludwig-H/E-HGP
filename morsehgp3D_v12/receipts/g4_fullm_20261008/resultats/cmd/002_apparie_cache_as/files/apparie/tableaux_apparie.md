# Pilote apparie : aa = (defaut), cache = --cache=8589934592, ref = (defaut), seq = --sequentiel

Reference : `ref` ; A/A : `aa` ; voie appareil, K5, 48 fils, 10 tours x 10 passes. Jugement : **juge**.

| bras | trame | rapport a la reference (IC 95 %) | verdict |
| --- | --- | --- | --- |
| `aa` | ng00 | 1.004 (0.993-1.019) | controle A/A |
| `aa` | ng01 | 0.999 (0.987-1.011) | controle A/A |
| `aa` | ng02 | 0.991 (0.980-1.001) | controle A/A |
| `cache` | ng00 | 0.929 (0.922-0.938) | adopte |
| `cache` | ng01 | 0.924 (0.920-0.928) | adopte |
| `cache` | ng02 | 0.918 (0.912-0.924) | adopte |
| `seq` | ng00 | 1.515 (1.506-1.525) | rejete |
| `seq` | ng01 | 1.465 (1.446-1.482) | rejete |
| `seq` | ng02 | 1.529 (1.520-1.537) | rejete |

Murs chauds medians des prises (ms) et etages (mediane des medianes de processus) :

| trame | bras | mur | P | C | G | queue ou T+M+V+R | CPU par passe | pic (Mio) |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| ng00 | `aa` | 94.2 | 2.0 | 27.9 | 57.0 | 6.1 | 2829.8 | 1147 |
| ng00 | `cache` | 87.5 | 2.0 | 25.7 | 53.5 | 6.2 | 2571.0 | 1163 |
| ng00 | `ref` | 94.5 | 1.9 | 27.5 | 57.1 | 6.2 | 2830.9 | 1144 |
| ng00 | `seq` | 143.5 | 1.9 | 28.1 | 52.6 | 60.9 | 2713.0 | 1071 |
| ng01 | `aa` | 77.6 | 1.8 | 24.2 | 45.1 | 5.3 | 2192.0 | 961 |
| ng01 | `cache` | 72.0 | 1.8 | 22.5 | 39.9 | 7.7 | 1973.2 | 968 |
| ng01 | `ref` | 77.9 | 1.8 | 24.2 | 44.3 | 5.5 | 2186.7 | 956 |
| ng01 | `seq` | 114.2 | 1.7 | 24.4 | 39.5 | 48.3 | 2059.9 | 893 |
| ng02 | `aa` | 95.9 | 1.9 | 28.1 | 49.0 | 15.0 | 2642.0 | 1185 |
| ng02 | `cache` | 88.4 | 1.9 | 25.8 | 45.9 | 14.4 | 2414.7 | 1203 |
| ng02 | `ref` | 96.6 | 1.9 | 28.2 | 49.2 | 15.1 | 2654.2 | 1184 |
| ng02 | `seq` | 147.6 | 1.9 | 27.9 | 48.1 | 69.2 | 2545.9 | 1103 |

Session v12set (information, second passage, mediane des processus) :

| bras | mediane (ms) | maximum (ms) | trames | refus |
| --- | ---: | ---: | ---: | --- |
| `aa` | 161.5 | 316.6 | 37 | - |
| `cache` | 152.6 | 305.9 | 37 | - |
| `ref` | 161.7 | 319.1 | 37 | - |
| `seq` | 226.4 | 429.6 | 37 | - |
