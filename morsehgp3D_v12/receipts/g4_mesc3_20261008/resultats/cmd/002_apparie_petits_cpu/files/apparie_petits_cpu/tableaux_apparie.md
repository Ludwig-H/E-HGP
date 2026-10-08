# Pilote apparie : aa = (defaut), ref = (defaut), sans_cache = --cache=0

Reference : `ref` ; A/A : `aa` ; voie cpu, K5, 48 fils, 10 tours x 10 passes. Jugement : **refuse** (A/A hors de la fenetre de +/- 1.5 % : p5000).

| bras | trame | rapport a la reference (IC 95 %) | verdict |
| --- | --- | --- | --- |
| `aa` | p1000 | 0.989 (0.956-1.022) |  |
| `aa` | p150 | 0.992 (0.965-1.021) |  |
| `aa` | p5000 | 1.015 (0.986-1.042) |  |
| `sans_cache` | p1000 | 0.980 (0.942-1.016) |  |
| `sans_cache` | p150 | 0.985 (0.926-1.040) |  |
| `sans_cache` | p5000 | 0.995 (0.971-1.020) |  |

Murs chauds medians des prises (ms) et etages (mediane des medianes de processus) :

| trame | bras | mur | P | C | G | queue ou T+M+V+R | CPU par passe | pic (Mio) |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| p1000 | `aa` | 27.5 | 0.1 | 24.2 | 2.2 | 0.9 | 293.6 | 10 |
| p1000 | `ref` | 27.6 | 0.1 | 24.4 | 2.3 | 0.9 | 298.3 | 10 |
| p1000 | `sans_cache` | 27.5 | 0.1 | 24.3 | 2.1 | 0.9 | 293.3 | 10 |
| p150 | `aa` | 10.6 | 0.0 | 9.6 | 0.7 | 0.3 | 49.7 | 6 |
| p150 | `ref` | 10.7 | 0.0 | 9.8 | 0.7 | 0.3 | 49.8 | 6 |
| p150 | `sans_cache` | 10.7 | 0.0 | 9.7 | 0.7 | 0.3 | 49.9 | 6 |
| p5000 | `aa` | 59.8 | 0.3 | 51.9 | 5.1 | 2.3 | 1321.9 | 46 |
| p5000 | `ref` | 58.8 | 0.3 | 50.9 | 5.1 | 2.3 | 1327.9 | 46 |
| p5000 | `sans_cache` | 58.4 | 0.2 | 51.2 | 4.7 | 2.2 | 1340.1 | 44 |
