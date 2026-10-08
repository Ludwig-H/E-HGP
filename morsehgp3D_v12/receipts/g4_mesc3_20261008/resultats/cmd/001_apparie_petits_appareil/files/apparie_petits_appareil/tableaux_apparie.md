# Pilote apparie : aa = (defaut), ref = (defaut), sans_cache = --cache=0

Reference : `ref` ; A/A : `aa` ; voie appareil, K5, 48 fils, 10 tours x 10 passes. Jugement : **refuse** (A/A hors de la fenetre de +/- 1.5 % : p5000).

| bras | trame | rapport a la reference (IC 95 %) | verdict |
| --- | --- | --- | --- |
| `aa` | p1000 | 1.002 (0.996-1.008) |  |
| `aa` | p150 | 1.000 (0.995-1.008) |  |
| `aa` | p5000 | 0.973 (0.944-1.001) |  |
| `sans_cache` | p1000 | 0.999 (0.993-1.006) |  |
| `sans_cache` | p150 | 0.997 (0.991-1.003) |  |
| `sans_cache` | p5000 | 1.025 (0.980-1.075) |  |

Murs chauds medians des prises (ms) et etages (mediane des medianes de processus) :

| trame | bras | mur | P | C | G | queue ou T+M+V+R | CPU par passe | pic (Mio) |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| p1000 | `aa` | 8.3 | 0.1 | 5.2 | 2.1 | 0.9 | 37.2 | 26 |
| p1000 | `ref` | 8.3 | 0.1 | 5.2 | 2.1 | 0.9 | 37.6 | 26 |
| p1000 | `sans_cache` | 8.3 | 0.1 | 5.2 | 2.0 | 0.9 | 37.4 | 26 |
| p150 | `aa` | 4.9 | 0.0 | 3.8 | 0.7 | 0.3 | 14.4 | 10 |
| p150 | `ref` | 4.9 | 0.0 | 3.8 | 0.7 | 0.3 | 14.1 | 10 |
| p150 | `sans_cache` | 4.8 | 0.0 | 3.8 | 0.7 | 0.3 | 13.7 | 10 |
| p5000 | `aa` | 14.8 | 0.2 | 7.5 | 4.9 | 2.2 | 142.0 | 109 |
| p5000 | `ref` | 15.7 | 0.2 | 7.5 | 5.1 | 2.8 | 141.1 | 109 |
| p5000 | `sans_cache` | 15.5 | 0.2 | 7.5 | 5.6 | 2.2 | 146.3 | 108 |
