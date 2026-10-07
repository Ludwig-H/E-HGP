### Contrôles par jeu

| Jeu | n | espacement médian (mm) | mosaïques complètes (contrôle strict) | faces / point à k = 5 | secondes (mosaïques 1..5) | témoin K = 1 (gudhi), H0 / H1 | tour = mosaïque (H0), max d_B |
| --- | --- | --- | --- | --- | --- | --- | --- |
| decoupe_08_002852_deux_velos_6_51_instances | 279 | 57.2 | 50 / 50 | 977 | 21.3 | 0e+00 / 3e-14 | 0e+00 |
| synth_anneau_perce_05m | 170 | 16.1 | 100 / 100 | 742 | 12.1 | 0e+00 / 4e-15 | 0e+00 |
| synth_anneau_perce_10m | 52 | 32.8 | 100 / 100 | 505 | 2.4 | 0e+00 / 7e-15 | 0e+00 |
| synth_pieton_05m | 649 | 17.0 | 50 / 50 | 1079 | 46.0 | 0e+00 / 1e-14 | 0e+00 |
| synth_velo_05m | 575 | 16.3 | 50 / 50 | 1002 | 38.1 | 0e+00 / 3e-14 | 0e+00 |
| synth_velo_10m | 203 | 32.3 | 100 / 100 | 887 | 15.0 | 0e+00 / 1e-14 | 0e+00 |
| synth_velo_occulte_10m | 581 | 27.9 | 10 / 10 | 1086 | 52.8 | 0e+00 / 1e-14 | 0e+00 |

### Épreuve 1 — bruit apparié (d_B en mm ; δ = déplacement apparié maximal réalisé)

| Jeu | σ (mm) | δ (mm) | k = 1 : H0 / H1 | k = 2 : H0 / H1 | k = 3 : H0 / H1 | k = 5 : H0 / H1 | DTM k = 5 : H0 / H1 | max d_B / δ |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| decoupe_08_002852_deux_velos_6_51_instances | 3 | 12.31 | 3.55 / 4.61 | 4.29 / 5.39 | 4.46 / 3.24 | 3.64 / 3.98 | 3.32 / 2.62 | 0.44 |
| decoupe_08_002852_deux_velos_6_51_instances | 10 | 40.79 | 7.53 / 12.68 | 11.27 / 11.90 | 9.77 / 8.98 | 9.77 / 10.05 | 7.63 / 7.68 | 0.34 |
| synth_anneau_perce_05m | 1 | 3.29 | 1.23 / 0.89 | 1.84 / 0.70 | 1.84 / 1.02 | 1.26 / 1.07 | 0.66 / 0.38 | 0.56 |
| synth_anneau_perce_05m | 3 | 12.31 | 3.60 / 3.12 | 3.37 / 3.81 | 3.98 / 3.35 | 4.47 / 1.88 | 2.97 / 4.14 | 0.36 |
| synth_anneau_perce_05m | 10 | 37.87 | 7.03 / 7.91 | 5.60 / 5.34 | 8.11 / 8.85 | 10.98 / 5.65 | 8.23 / 7.28 | 0.29 |
| synth_anneau_perce_10m | 1 | 3.28 | 1.44 / 0.71 | 1.82 / 0.49 | 1.58 / 0.87 | 1.55 / 0.48 | 0.77 / 0.36 | 0.56 |
| synth_anneau_perce_10m | 3 | 9.80 | 4.89 / 2.90 | 3.69 / 2.18 | 5.38 / 0.75 | 3.72 / 1.72 | 3.02 / 1.10 | 0.55 |
| synth_anneau_perce_10m | 10 | 28.29 | 7.76 / 13.11 | 12.19 / 12.76 | 9.56 / 9.86 | 12.42 / 5.98 | 8.45 / 8.57 | 0.46 |
| synth_pieton_05m | 3 | 12.31 | 3.31 / 3.63 | 4.43 / 2.55 | 3.67 / 2.34 | 4.53 / 2.05 | 3.71 / 2.28 | 0.37 |
| synth_pieton_05m | 10 | 41.54 | 6.95 / 7.38 | 6.30 / 10.02 | 8.29 / 6.81 | 8.02 / 6.59 | 6.39 / 9.42 | 0.24 |
| synth_velo_05m | 3 | 12.31 | 3.75 / 3.97 | 4.51 / 2.91 | 4.84 / 3.35 | 3.63 / 3.19 | 2.61 / 1.94 | 0.39 |
| synth_velo_05m | 10 | 38.72 | 6.70 / 7.40 | 7.42 / 7.72 | 8.52 / 9.77 | 9.45 / 5.29 | 7.11 / 8.31 | 0.25 |
| synth_velo_10m | 1 | 3.29 | 1.40 / 1.33 | 1.53 / 1.42 | 1.85 / 1.47 | 1.88 / 1.31 | 1.06 / 0.80 | 0.57 |
| synth_velo_10m | 3 | 12.31 | 4.64 / 7.45 | 7.45 / 4.12 | 5.04 / 3.75 | 5.38 / 3.88 | 2.89 / 3.18 | 0.61 |
| synth_velo_10m | 10 | 33.61 | 10.51 / 11.21 | 16.69 / 9.22 | 14.42 / 8.70 | 18.66 / 6.27 | 15.00 / 5.40 | 0.56 |
| synth_velo_occulte_10m | 3 | 12.31 | 4.08 / 4.76 | 4.36 / 4.44 | 4.62 / 4.41 | 5.72 / 3.81 | 3.24 / 2.24 | 0.46 |

### Épreuve 1 — quantification du nuage fin (base = arrondi au mm de P_fin)

| Jeu | h (mm) | δ (mm) | √3 h / 2 | fusions de sites | max_k d_B H0 | max_k d_B H1 | sous δ |
| --- | --- | --- | --- | --- | --- | --- | --- |
| decoupe_08_002852_deux_velos_6_51_instances | 5 | 3.98 | 4.33 | 0 | 2.40 | 2.48 | oui |
| synth_anneau_perce_05m | 1 | 0.83 | 0.87 | 0 | 0.44 | 0.47 | oui |
| synth_anneau_perce_05m | 2 | 1.59 | 1.73 | 0 | 0.78 | 0.84 | oui |
| synth_anneau_perce_05m | 5 | 4.12 | 4.33 | 0 | 1.73 | 1.64 | oui |
| synth_anneau_perce_10m | 1 | 0.76 | 0.87 | 0 | 0.51 | 0.33 | oui |
| synth_anneau_perce_10m | 2 | 1.52 | 1.73 | 0 | 0.90 | 0.82 | oui |
| synth_anneau_perce_10m | 5 | 4.15 | 4.33 | 0 | 3.21 | 2.06 | oui |
| synth_pieton_05m | 5 | 4.09 | 4.33 | 0 | 2.22 | 2.22 | oui |
| synth_velo_05m | 5 | 4.19 | 4.33 | 0 | 2.52 | 2.41 | oui |
| synth_velo_10m | 1 | 0.80 | 0.87 | 0 | 0.55 | 0.55 | oui |
| synth_velo_10m | 2 | 1.63 | 1.73 | 0 | 1.13 | 1.04 | oui |
| synth_velo_10m | 5 | 3.91 | 4.33 | 0 | 2.77 | 2.34 | oui |

### Épreuve 1 — géométrie des représentants (Hausdorff estimé, mm), composante appariée par les labels

| Jeu | σ | k | nœud | vie [b, d) (mm) | naissance : H (IoU) | milieu : H (IoU), marges | avant la mort : H (IoU) |
| --- | --- | --- | --- | --- | --- | --- | --- |
| decoupe_08_002852_deux_velos_6_51_instances | 3 | 5 | vélo | [156.6, 156.7) | 43.7 (1.00) | 43.7 (1.00); marges 0.1, 0.1 | 43.7 (1.00) |
| decoupe_08_002852_deux_velos_6_51_instances | 3 | 5 | vélo (ancêtre le plus persistant) | [330.1, 660.3) racine | 90.2 (1.00) | 34.0 (1.00); marges 136.7, 193.4 | 38.3 (1.00) |
| decoupe_08_002852_deux_velos_6_51_instances | 3 | 5 | vélo | [154.2, 154.3) | 50.9 (1.00) | 50.7 (1.00); marges 0.0, 0.0 | 50.7 (1.00) |
| decoupe_08_002852_deux_velos_6_51_instances | 10 | 5 | vélo | [156.6, 156.7) | 147.0 (0.99) | 147.0 (0.99); marges 0.1, 0.1 | 147.0 (0.99) |
| decoupe_08_002852_deux_velos_6_51_instances | 10 | 5 | vélo (ancêtre le plus persistant) | [330.1, 660.3) racine | 110.1 (1.00) | 54.6 (1.00); marges 136.7, 193.4 | 43.2 (1.00) |
| decoupe_08_002852_deux_velos_6_51_instances | 10 | 5 | vélo | [154.2, 154.3) | 239.9 (0.96) | 239.9 (0.96); marges 0.0, 0.0 | 239.9 (0.96) |
| synth_anneau_perce_05m | 3 | 2 | roue | [24.5, 24.5) | 550.5 (0.29) | 550.5 (0.29); marges 0.0, 0.0 | 550.8 (0.29) |
| synth_anneau_perce_05m | 3 | 2 | roue (ancêtre le plus persistant) | [28.5, 57.0) racine | 29.1 (1.00) | 14.8 (1.00); marges 11.8, 16.7 | 10.6 (1.00) |
| synth_anneau_perce_05m | 3 | 2 | persistant #1 | [8.6, 19.4) | 3.8 (1.00) | 3.8 (1.00); marges 4.3, 6.5 | 3.8 (1.00) |
| synth_anneau_perce_05m | 3 | 2 | persistant #2 | [7.8, 18.2) | ∞ (0.00) | 3.9 (1.00); marges 4.1, 6.3 | 3.9 (1.00) |
| synth_anneau_perce_05m | 3 | 2 | persistant #3 | [8.0, 17.9) | ∞ (0.00) | 1.6 (1.00); marges 4.0, 5.9 | 45.5 (0.40) |
| synth_anneau_perce_05m | 3 | 2 | persistant #4 | [9.8, 19.5) | ∞ (0.00) | 4.9 (1.00); marges 4.0, 5.7 | 4.9 (1.00) |
| synth_anneau_perce_05m | 3 | 2 | roue/pneu (ancêtre) | [24.5, 24.5) | 550.8 (0.29) | 550.8 (0.29); marges 0.0, 0.0 | 550.8 (0.29) |
| synth_anneau_perce_05m | 3 | 2 | roue/pneu (ancêtre) | [24.5, 24.5) | 550.7 (0.29) | 550.6 (0.29); marges 0.0, 0.0 | 550.5 (0.29) |
| synth_anneau_perce_05m | 3 | 5 | roue | [45.9, 46.2) | 12.8 (1.00) | 12.8 (1.00); marges 0.1, 0.1 | 12.1 (1.00) |
| synth_anneau_perce_05m | 3 | 5 | roue (ancêtre le plus persistant) | [276.1, 552.1) racine | 62.7 (1.00) | 11.6 (1.00); marges 114.3, 161.7 | 11.6 (1.00) |
| synth_anneau_perce_05m | 3 | 5 | persistant #1 | [53.3, 275.4) | 7.7 (1.00) | 13.3 (1.00); marges 67.8, 154.3 | 133.7 (1.00) |
| synth_anneau_perce_05m | 3 | 5 | persistant #2 | [31.3, 45.6) | 9.5 (1.00) | 4.9 (1.00); marges 6.5, 7.9 | 613.5 (0.05) |
| synth_anneau_perce_05m | 3 | 5 | persistant #3 | [32.6, 45.8) | 10.0 (1.00) | 6.2 (1.00); marges 6.0, 7.2 | 614.2 (0.05) |
| synth_anneau_perce_05m | 3 | 5 | persistant #4 | [31.0, 39.0) | 3.4 (1.00) | 3.4 (1.00); marges 3.8, 4.2 | 45.3 (0.50) |
| synth_anneau_perce_05m | 3 | 5 | roue/pneu (ancêtre) | [46.2, 46.2) | 12.1 (1.00) | 12.1 (1.00); marges 0.0, 0.0 | 12.1 (1.00) |
| synth_anneau_perce_05m | 3 | 5 | roue/pneu (ancêtre) | [46.2, 46.2) | 12.1 (1.00) | 12.1 (1.00); marges 0.0, 0.0 | 12.1 (1.00) |
| synth_anneau_perce_05m | 10 | 2 | roue | [24.5, 24.5) | 588.2 (0.18) | 588.2 (0.18); marges 0.0, 0.0 | 588.2 (0.18) |
| synth_anneau_perce_05m | 10 | 2 | roue (ancêtre le plus persistant) | [28.5, 57.0) racine | 33.3 (1.00) | 27.0 (1.00); marges 11.8, 16.7 | 27.5 (1.00) |
| synth_anneau_perce_05m | 10 | 2 | persistant #1 | [8.6, 19.4) | ∞ (0.00) | 26.0 (0.33); marges 4.3, 6.5 | 47.7 (0.50) |
| synth_anneau_perce_05m | 10 | 2 | persistant #2 | [7.8, 18.2) | ∞ (0.00) | 9.4 (1.00); marges 4.1, 6.3 | 56.6 (0.25) |
| synth_anneau_perce_05m | 10 | 2 | persistant #3 | [8.0, 17.9) | ∞ (0.00) | ∞ (0.00); marges 4.0, 5.9 | 16.1 (1.00) |
| synth_anneau_perce_05m | 10 | 2 | persistant #4 | [9.8, 19.5) | ∞ (0.00) | 7.3 (1.00); marges 4.0, 5.7 | 17.6 (0.67) |
| synth_anneau_perce_05m | 10 | 2 | roue/pneu (ancêtre) | [24.5, 24.5) | 588.2 (0.18) | 588.2 (0.18); marges 0.0, 0.0 | 588.2 (0.18) |
| synth_anneau_perce_05m | 10 | 2 | roue/pneu (ancêtre) | [24.5, 24.5) | 588.2 (0.18) | 588.2 (0.18); marges 0.0, 0.0 | 588.2 (0.18) |
| synth_anneau_perce_05m | 10 | 5 | roue | [45.9, 46.2) | 444.7 (0.51) | 444.7 (0.51); marges 0.1, 0.1 | 444.7 (0.51) |
| synth_anneau_perce_05m | 10 | 5 | roue (ancêtre le plus persistant) | [276.1, 552.1) racine | 72.2 (1.00) | 27.7 (1.00); marges 114.3, 161.7 | 28.9 (1.00) |
| synth_anneau_perce_05m | 10 | 5 | persistant #1 | [53.3, 275.4) | 17.0 (1.00) | 23.1 (1.00); marges 67.8, 154.3 | 125.1 (1.00) |
| synth_anneau_perce_05m | 10 | 5 | persistant #2 | [31.3, 45.6) | 14.6 (0.75) | 32.1 (0.44); marges 6.5, 7.9 | 384.0 (0.10) |
| synth_anneau_perce_05m | 10 | 5 | persistant #3 | [32.6, 45.8) | 23.1 (0.62) | 9.1 (1.00); marges 6.0, 7.2 | 397.4 (0.12) |
| synth_anneau_perce_05m | 10 | 5 | persistant #4 | [31.0, 39.0) | 7.3 (1.00) | 7.3 (1.00); marges 3.8, 4.2 | 18.0 (0.83) |
| synth_anneau_perce_05m | 10 | 5 | roue/pneu (ancêtre) | [46.2, 46.2) | 444.7 (0.51) | 444.7 (0.51); marges 0.0, 0.0 | 444.7 (0.51) |
| synth_anneau_perce_05m | 10 | 5 | roue/pneu (ancêtre) | [46.2, 46.2) | 444.7 (0.51) | 444.7 (0.51); marges 0.0, 0.0 | 444.7 (0.51) |
| synth_anneau_perce_10m | 3 | 2 | roue | [55.4, 141.5) | 15.9 (1.00) | 16.5 (1.00); marges 33.1, 53.0 | 141.0 (1.00) |
| synth_anneau_perce_10m | 3 | 2 | roue (ancêtre le plus persistant) | [141.5, 283.0) racine | 70.2 (1.00) | 18.8 (1.00); marges 58.6, 82.9 | 9.7 (1.00) |
| synth_anneau_perce_10m | 3 | 2 | roue/jante | [31.5, 31.5) | 34.1 (0.50) | 34.1 (0.50); marges 0.0, 0.0 | 34.1 (0.50) |
| synth_anneau_perce_10m | 3 | 2 | persistant #2 | [15.5, 47.3) | 2.6 (1.00) | 2.6 (1.00); marges 11.6, 20.2 | 54.4 (0.33) |
| synth_anneau_perce_10m | 3 | 2 | persistant #3 | [16.3, 46.8) | ∞ (0.00) | 4.2 (1.00); marges 11.3, 19.2 | 42.7 (0.67) |
| synth_anneau_perce_10m | 3 | 2 | persistant #4 | [32.1, 54.2) | 63.2 (0.50) | 3.7 (1.00); marges 9.6, 12.5 | 618.7 (0.08) |
| synth_anneau_perce_10m | 3 | 2 | roue/jante (ancêtre) | [31.5, 31.9) | 95.6 (0.50) | 95.6 (0.50); marges 0.2, 0.2 | 95.6 (0.50) |
| synth_anneau_perce_10m | 3 | 2 | roue/jante (ancêtre) | [31.9, 33.0) | 190.1 (0.45) | 190.1 (0.45); marges 0.6, 0.6 | 128.0 (0.58) |
| synth_anneau_perce_10m | 3 | 5 | roue | [122.6, 193.8) | 164.4 (0.92) | 11.4 (1.00); marges 31.5, 39.6 | 185.4 (1.00) |
| synth_anneau_perce_10m | 3 | 5 | roue (ancêtre le plus persistant) | [283.3, 566.7) racine | 73.3 (1.00) | 5.3 (1.00); marges 117.4, 166.0 | 6.3 (1.00) |
| synth_anneau_perce_10m | 3 | 5 | roue/jante | [69.4, 69.6) | 26.2 (0.90) | 26.2 (0.90); marges 0.1, 0.1 | 26.2 (0.90) |
| synth_anneau_perce_10m | 3 | 5 | persistant #1 | [193.8, 282.4) | 38.0 (1.00) | 21.6 (1.00); marges 40.1, 48.4 | 171.3 (1.00) |
| synth_anneau_perce_10m | 3 | 5 | persistant #3 | [62.2, 89.0) | 24.3 (0.83) | 2.0 (1.00); marges 12.2, 14.6 | 60.1 (0.60) |
| synth_anneau_perce_10m | 3 | 5 | persistant #4 | [62.5, 89.0) | 22.7 (0.83) | 3.6 (1.00); marges 12.1, 14.4 | 3.6 (1.00) |
| synth_anneau_perce_10m | 3 | 5 | roue/pneu (ancêtre) | [282.4, 282.5) | 108.0 (1.00) | 108.0 (1.00); marges 0.1, 0.1 | 108.0 (1.00) |
| synth_anneau_perce_10m | 3 | 5 | roue/jante (ancêtre) | [69.6, 78.6) | 8.4 (1.00) | 30.2 (0.91); marges 4.4, 4.6 | 30.2 (0.91) |
| synth_anneau_perce_10m | 3 | 5 | roue/jante (ancêtre) | [78.6, 78.7) | 155.3 (0.68) | 155.3 (0.68); marges 0.0, 0.0 | 155.3 (0.68) |
| synth_anneau_perce_10m | 10 | 2 | roue | [55.4, 141.5) | 157.3 (0.85) | 21.1 (1.00); marges 33.1, 53.0 | 24.4 (1.00) |
| synth_anneau_perce_10m | 10 | 2 | roue (ancêtre le plus persistant) | [141.5, 283.0) racine | 146.8 (1.00) | 33.7 (1.00); marges 58.6, 82.9 | 62.0 (1.00) |
| synth_anneau_perce_10m | 10 | 2 | roue/jante | [31.5, 31.5) | 75.0 (0.50) | 75.0 (0.50); marges 0.0, 0.0 | 75.0 (0.50) |
| synth_anneau_perce_10m | 10 | 2 | persistant #2 | [15.5, 47.3) | ∞ (0.00) | ∞ (0.00); marges 11.6, 20.2 | 9.8 (1.00) |
| synth_anneau_perce_10m | 10 | 2 | persistant #3 | [16.3, 46.8) | ∞ (0.00) | 6.6 (1.00); marges 11.3, 19.2 | 46.5 (0.67) |
| synth_anneau_perce_10m | 10 | 2 | persistant #4 | [32.1, 54.2) | 13.4 (1.00) | 13.4 (1.00); marges 9.6, 12.5 | 56.6 (0.80) |
| synth_anneau_perce_10m | 10 | 2 | roue/jante (ancêtre) | [31.5, 31.9) | 75.0 (0.57) | 75.0 (0.57); marges 0.2, 0.2 | 75.0 (0.57) |
| synth_anneau_perce_10m | 10 | 2 | roue/jante (ancêtre) | [31.9, 33.0) | 168.2 (0.36) | 168.2 (0.36); marges 0.6, 0.6 | 168.2 (0.36) |
| synth_anneau_perce_10m | 10 | 5 | roue | [122.6, 193.8) | 170.0 (0.92) | 25.9 (1.00); marges 31.5, 39.6 | 182.8 (1.00) |
| synth_anneau_perce_10m | 10 | 5 | roue (ancêtre le plus persistant) | [283.3, 566.7) racine | 119.1 (1.00) | 17.0 (1.00); marges 117.4, 166.0 | 15.6 (1.00) |
| synth_anneau_perce_10m | 10 | 5 | roue/jante | [69.4, 69.6) | 27.0 (0.78) | 27.0 (0.78); marges 0.1, 0.1 | 27.0 (0.78) |
| synth_anneau_perce_10m | 10 | 5 | persistant #1 | [193.8, 282.4) | 76.8 (1.00) | 31.1 (1.00); marges 40.1, 48.4 | 98.9 (1.00) |
| synth_anneau_perce_10m | 10 | 5 | persistant #3 | [62.2, 89.0) | 52.0 (0.57) | 12.9 (1.00); marges 12.2, 14.6 | 68.0 (0.60) |
| synth_anneau_perce_10m | 10 | 5 | persistant #4 | [62.5, 89.0) | 26.0 (0.83) | 13.0 (1.00); marges 12.1, 14.4 | 68.6 (0.60) |
| synth_anneau_perce_10m | 10 | 5 | roue/pneu (ancêtre) | [282.4, 282.5) | 98.9 (1.00) | 97.7 (1.00); marges 0.1, 0.1 | 99.4 (1.00) |
| synth_anneau_perce_10m | 10 | 5 | roue/jante (ancêtre) | [69.6, 78.6) | 52.8 (0.70) | 52.8 (0.80); marges 4.4, 4.6 | 33.9 (0.91) |
| synth_anneau_perce_10m | 10 | 5 | roue/jante (ancêtre) | [78.6, 78.7) | 190.0 (0.63) | 190.0 (0.63); marges 0.0, 0.0 | 190.0 (0.63) |
| synth_velo_10m | 3 | 2 | vélo | [151.9, 154.1) | 36.5 (1.00) | 68.2 (1.00); marges 1.1, 1.1 | 63.9 (1.00) |
| synth_velo_10m | 3 | 2 | vélo (ancêtre le plus persistant) | [161.9, 323.8) racine | 39.8 (1.00) | 27.5 (1.00); marges 67.1, 94.8 | 59.1 (1.00) |
| synth_velo_10m | 3 | 2 | vélo/roue_arriere | [49.7, 55.5) | 158.2 (0.83) | 119.2 (0.92); marges 2.8, 3.0 | 314.2 (0.56) |
| synth_velo_10m | 3 | 2 | vélo/roue_avant | [55.0, 55.2) | 158.4 (0.94) | 158.4 (0.94); marges 0.1, 0.1 | 158.4 (0.94) |
| synth_velo_10m | 3 | 2 | vélo/cadre | [88.8, 89.9) | 83.3 (1.00) | 83.3 (1.00); marges 0.6, 0.6 | 83.3 (1.00) |
| synth_velo_10m | 3 | 2 | vélo/guidon | [42.9, 58.2) | 2.2 (1.00) | 2.2 (1.00); marges 7.1, 8.2 | 2.2 (1.00) |
| synth_velo_10m | 3 | 2 | vélo/selle | [37.4, 62.6) | 31.5 (1.00) | 5.8 (1.00); marges 11.0, 14.3 | 240.7 (0.67) |
| synth_velo_10m | 3 | 2 | persistant #1 | [31.1, 69.6) | 65.3 (0.50) | 4.9 (1.00); marges 15.4, 23.1 | 4.9 (1.00) |
| synth_velo_10m | 3 | 2 | persistant #2 | [33.6, 66.4) | 2.0 (1.00) | 2.0 (1.00); marges 13.6, 19.2 | 2.0 (1.00) |
| synth_velo_10m | 3 | 2 | persistant #3 | [29.7, 60.6) | 2.1 (1.00) | 2.1 (1.00); marges 12.7, 18.1 | 58.7 (0.67) |
| synth_velo_10m | 3 | 2 | persistant #4 | [17.2, 47.1) | ∞ (0.00) | 3.3 (1.00); marges 11.3, 18.6 | 48.1 (0.67) |
| synth_velo_10m | 3 | 2 | vélo/roue_arriere (ancêtre) | [55.5, 55.6) | 314.2 (0.57) | 314.2 (0.57); marges 0.0, 0.0 | 314.2 (0.57) |
| synth_velo_10m | 3 | 2 | vélo/roue_arriere (ancêtre) | [55.6, 55.7) | 107.2 (0.97) | 107.2 (0.97); marges 0.1, 0.1 | 107.2 (0.97) |
| synth_velo_10m | 3 | 2 | vélo/roue_avant (ancêtre) | [55.2, 55.8) | 158.4 (0.93) | 158.4 (0.93); marges 0.3, 0.3 | 158.4 (0.93) |
| synth_velo_10m | 3 | 2 | vélo/roue_avant (ancêtre) | [55.8, 56.8) | 788.8 (0.55) | 788.8 (0.55); marges 0.5, 0.5 | 788.8 (0.55) |
| synth_velo_10m | 3 | 2 | vélo/cadre (ancêtre) | [89.9, 90.5) | 88.2 (1.00) | 89.2 (1.00); marges 0.3, 0.3 | 88.2 (1.00) |
| synth_velo_10m | 3 | 5 | vélo | [167.4, 167.5) | 65.5 (1.00) | 65.5 (1.00); marges 0.0, 0.0 | 65.5 (1.00) |
| synth_velo_10m | 3 | 5 | vélo (ancêtre le plus persistant) | [213.5, 427.0) racine | 63.0 (1.00) | 20.4 (1.00); marges 88.4, 125.1 | 19.5 (1.00) |
| synth_velo_10m | 3 | 5 | vélo/roue_arriere | [89.0, 89.6) | 122.0 (0.94) | 122.0 (0.94); marges 0.3, 0.3 | 122.0 (0.94) |
| synth_velo_10m | 3 | 5 | vélo/roue_avant | [90.2, 91.4) | 118.5 (0.92) | 118.5 (0.92); marges 0.6, 0.6 | 33.7 (0.98) |
| synth_velo_10m | 3 | 5 | vélo/cadre | [101.9, 101.9) | 37.4 (1.00) | 37.4 (1.00); marges 0.0, 0.0 | 37.4 (1.00) |
| synth_velo_10m | 3 | 5 | vélo/guidon | [162.4, 167.4) | 2.3 (1.00) | 2.3 (1.00); marges 2.5, 2.5 | 2.3 (1.00) |
| synth_velo_10m | 3 | 5 | vélo/selle | [78.8, 95.9) | 16.0 (1.00) | 13.4 (1.00); marges 8.1, 9.0 | 40.5 (0.92) |
| synth_velo_10m | 3 | 5 | persistant #1 | [81.8, 121.3) | 24.3 (1.00) | 9.2 (1.00); marges 17.8, 21.7 | 3.4 (1.00) |
| synth_velo_10m | 3 | 5 | persistant #2 | [95.9, 123.6) | 12.3 (1.00) | 13.5 (1.00); marges 13.0, 14.7 | 50.3 (0.92) |
| synth_velo_10m | 3 | 5 | persistant #3 | [61.7, 89.0) | 147.0 (0.10) | 3.8 (1.00); marges 12.4, 14.9 | 3.8 (1.00) |
| synth_velo_10m | 3 | 5 | persistant #4 | [61.7, 89.0) | 27.3 (0.83) | 2.8 (1.00); marges 12.4, 14.9 | 57.6 (0.75) |
| synth_velo_10m | 3 | 5 | vélo/roue_arriere (ancêtre) | [89.6, 90.1) | 122.0 (0.94) | 122.0 (0.94); marges 0.2, 0.2 | 122.0 (0.94) |
| synth_velo_10m | 3 | 5 | vélo/roue_arriere (ancêtre) | [90.1, 90.1) | 122.0 (0.94) | 122.0 (0.94); marges 0.0, 0.0 | 122.0 (0.94) |
| synth_velo_10m | 3 | 5 | vélo/roue_avant (ancêtre) | [91.4, 92.9) | 322.5 (0.80) | 14.7 (1.00); marges 0.8, 0.8 | 14.8 (1.00) |
| synth_velo_10m | 3 | 5 | vélo/roue_avant (ancêtre) | [92.9, 93.6) | 14.8 (1.00) | 70.0 (0.97); marges 0.4, 0.4 | 70.0 (0.97) |
| synth_velo_10m | 3 | 5 | vélo/cadre (ancêtre) | [101.9, 102.0) | 36.7 (1.00) | 36.7 (1.00); marges 0.1, 0.1 | 36.7 (1.00) |
| synth_velo_10m | 10 | 2 | vélo | [151.9, 154.1) | 145.0 (1.00) | 145.0 (1.00); marges 1.1, 1.1 | 145.0 (1.00) |
| synth_velo_10m | 10 | 2 | vélo (ancêtre le plus persistant) | [161.9, 323.8) racine | 76.8 (1.00) | 51.1 (1.00); marges 67.1, 94.8 | 103.9 (1.00) |
| synth_velo_10m | 10 | 2 | vélo/roue_arriere | [49.7, 55.5) | 262.2 (0.78) | 262.2 (0.62); marges 2.8, 3.0 | 262.2 (0.62) |
| synth_velo_10m | 10 | 2 | vélo/roue_avant | [55.0, 55.2) | 208.0 (0.84) | 208.0 (0.83); marges 0.1, 0.1 | 208.0 (0.83) |
| synth_velo_10m | 10 | 2 | vélo/cadre | [88.8, 89.9) | 86.5 (1.00) | 86.5 (1.00); marges 0.6, 0.6 | 86.5 (1.00) |
| synth_velo_10m | 10 | 2 | vélo/guidon | [42.9, 58.2) | 11.8 (1.00) | 11.8 (1.00); marges 7.1, 8.2 | 350.2 (0.18) |
| synth_velo_10m | 10 | 2 | vélo/selle | [37.4, 62.6) | 23.7 (1.00) | 25.2 (1.00); marges 11.0, 14.3 | 25.5 (1.00) |
| synth_velo_10m | 10 | 2 | persistant #1 | [31.1, 69.6) | 69.5 (0.50) | 13.0 (1.00); marges 15.4, 23.1 | 1084.9 (0.02) |
| synth_velo_10m | 10 | 2 | persistant #2 | [33.6, 66.4) | 77.7 (0.33) | 6.1 (1.00); marges 13.6, 19.2 | 6.1 (1.00) |
| synth_velo_10m | 10 | 2 | persistant #3 | [29.7, 60.6) | 7.5 (1.00) | 7.5 (1.00); marges 12.7, 18.1 | 117.6 (0.40) |
| synth_velo_10m | 10 | 2 | persistant #4 | [17.2, 47.1) | ∞ (0.00) | 8.0 (1.00); marges 11.3, 18.6 | 50.5 (0.67) |
| synth_velo_10m | 10 | 2 | vélo/roue_arriere (ancêtre) | [55.5, 55.6) | 262.2 (0.64) | 262.2 (0.64); marges 0.0, 0.0 | 262.2 (0.64) |
| synth_velo_10m | 10 | 2 | vélo/roue_arriere (ancêtre) | [55.6, 55.7) | 262.2 (0.61) | 262.2 (0.61); marges 0.1, 0.1 | 262.2 (0.61) |
| synth_velo_10m | 10 | 2 | vélo/roue_avant (ancêtre) | [55.2, 55.8) | 208.0 (0.84) | 208.0 (0.84); marges 0.3, 0.3 | 208.0 (0.84) |
| synth_velo_10m | 10 | 2 | vélo/roue_avant (ancêtre) | [55.8, 56.8) | 991.3 (0.37) | 991.3 (0.37); marges 0.5, 0.5 | 783.9 (0.45) |
| synth_velo_10m | 10 | 2 | vélo/cadre (ancêtre) | [89.9, 90.5) | 94.1 (1.00) | 94.6 (1.00); marges 0.3, 0.3 | 94.1 (1.00) |
| synth_velo_10m | 10 | 5 | vélo | [167.4, 167.5) | 65.1 (1.00) | 65.1 (1.00); marges 0.0, 0.0 | 65.1 (1.00) |
| synth_velo_10m | 10 | 5 | vélo (ancêtre le plus persistant) | [213.5, 427.0) racine | 57.4 (1.00) | 45.3 (1.00); marges 88.4, 125.1 | 61.2 (1.00) |
| synth_velo_10m | 10 | 5 | vélo/roue_arriere | [89.0, 89.6) | 90.7 (0.95) | 90.7 (0.96); marges 0.3, 0.3 | 90.7 (0.95) |
| synth_velo_10m | 10 | 5 | vélo/roue_avant | [90.2, 91.4) | 123.2 (0.86) | 123.2 (0.86); marges 0.6, 0.6 | 123.2 (0.86) |
| synth_velo_10m | 10 | 5 | vélo/cadre | [101.9, 101.9) | 252.4 (0.93) | 252.4 (0.93); marges 0.0, 0.0 | 252.4 (0.93) |
| synth_velo_10m | 10 | 5 | vélo/guidon | [162.4, 167.4) | 1425.6 (0.02) | 1425.6 (0.02); marges 2.5, 2.5 | 1425.6 (0.02) |
| synth_velo_10m | 10 | 5 | vélo/selle | [78.8, 95.9) | 43.6 (0.91) | 43.6 (0.91); marges 8.1, 9.0 | 11.4 (1.00) |
| synth_velo_10m | 10 | 5 | persistant #1 | [81.8, 121.3) | 45.7 (0.71) | 13.7 (1.00); marges 17.8, 21.7 | 103.3 (0.50) |
| synth_velo_10m | 10 | 5 | persistant #2 | [95.9, 123.6) | 46.2 (0.92) | 23.4 (1.00); marges 13.0, 14.7 | 18.0 (1.00) |
| synth_velo_10m | 10 | 5 | persistant #3 | [61.7, 89.0) | 28.5 (0.83) | 6.2 (1.00); marges 12.4, 14.9 | 134.9 (0.30) |
| synth_velo_10m | 10 | 5 | persistant #4 | [61.7, 89.0) | 127.6 (0.22) | 5.5 (1.00); marges 12.4, 14.9 | 63.3 (0.75) |
| synth_velo_10m | 10 | 5 | vélo/roue_arriere (ancêtre) | [89.6, 90.1) | 90.7 (0.95) | 90.7 (0.95); marges 0.2, 0.2 | 90.7 (0.95) |
| synth_velo_10m | 10 | 5 | vélo/roue_arriere (ancêtre) | [90.1, 90.1) | 90.7 (0.95) | 90.7 (0.95); marges 0.0, 0.0 | 90.7 (0.95) |
| synth_velo_10m | 10 | 5 | vélo/roue_avant (ancêtre) | [91.4, 92.9) | 299.7 (0.75) | 299.7 (0.81); marges 0.8, 0.8 | 218.9 (0.88) |
| synth_velo_10m | 10 | 5 | vélo/roue_avant (ancêtre) | [92.9, 93.6) | 74.3 (0.97) | 74.3 (0.97); marges 0.4, 0.4 | 74.3 (0.97) |
| synth_velo_10m | 10 | 5 | vélo/cadre (ancêtre) | [101.9, 102.0) | 252.4 (0.93) | 252.4 (0.93); marges 0.1, 0.1 | 252.4 (0.93) |
| synth_velo_occulte_10m | 3 | 2 | vélo | [172.5, 176.3) | 863.7 (0.53) | 863.7 (0.53); marges 1.8, 1.9 | 38.6 (1.00) |
| synth_velo_occulte_10m | 3 | 2 | vélo (ancêtre le plus persistant) | [1437.0, 2874.0) racine | 55.4 (1.00) | 32.5 (1.00); marges 595.2, 841.8 | 27.3 (1.00) |
| synth_velo_occulte_10m | 3 | 2 | vélo/roue_arriere | [61.7, 61.9) | 313.7 (0.60) | 313.4 (0.60); marges 0.1, 0.1 | 313.4 (0.60) |
| synth_velo_occulte_10m | 3 | 2 | vélo/roue_arriere (ancêtre) | [61.9, 62.9) | 313.8 (0.60) | 313.4 (0.61); marges 0.5, 0.5 | 313.4 (0.67) |
| synth_velo_occulte_10m | 3 | 2 | vélo/roue_arriere (ancêtre) | [62.9, 63.2) | 313.7 (0.67) | 35.5 (1.00); marges 0.2, 0.2 | 28.6 (1.00) |
| synth_velo_occulte_10m | 3 | 2 | vélo/roue_avant | [58.0, 59.3) | 55.8 (0.99) | 55.8 (0.99); marges 0.7, 0.7 | 55.8 (0.97) |
| synth_velo_occulte_10m | 3 | 2 | vélo/roue_avant (ancêtre) | [59.3, 60.6) | 55.8 (0.99) | 33.3 (1.00); marges 0.6, 0.6 | 33.5 (1.00) |
| synth_velo_occulte_10m | 3 | 2 | vélo/roue_avant (ancêtre) | [60.6, 87.0) | 246.4 (0.85) | 16.7 (1.00); marges 12.0, 14.4 | 86.2 (0.99) |
| synth_velo_occulte_10m | 3 | 2 | vélo/cadre (ancêtre) | [176.3, 178.3) | 63.7 (1.00) | 99.9 (1.00); marges 1.0, 1.0 | 19.6 (1.00) |
| synth_velo_occulte_10m | 3 | 2 | vélo/cadre (ancêtre) | [178.3, 180.9) | 65.8 (1.00) | 80.4 (1.00); marges 1.3, 1.3 | 76.1 (1.00) |
| synth_velo_occulte_10m | 3 | 2 | vélo/guidon | [42.8, 58.5) | 44.0 (0.67) | 7.7 (1.00); marges 7.2, 8.4 | 234.4 (0.23) |
| synth_velo_occulte_10m | 3 | 2 | vélo/guidon (ancêtre) | [58.5, 60.6) | 7.7 (1.00) | 18.4 (1.00); marges 1.0, 1.0 | 18.4 (1.00) |
| synth_velo_occulte_10m | 3 | 2 | vélo/selle | [38.5, 63.2) | 37.1 (0.90) | 6.7 (1.00); marges 10.8, 13.9 | 6.3 (1.00) |
| synth_velo_occulte_10m | 3 | 2 | vélo/selle (ancêtre) | [63.2, 65.0) | 162.2 (0.90) | 162.2 (0.90); marges 0.9, 0.9 | 162.2 (0.90) |
| synth_velo_occulte_10m | 3 | 2 | vélo/selle (ancêtre) | [65.0, 65.3) | 162.2 (0.90) | 162.2 (0.90); marges 0.1, 0.1 | 162.2 (0.90) |
| synth_velo_occulte_10m | 3 | 5 | vélo | [195.2, 196.8) | 949.5 (0.53) | 949.5 (0.53); marges 0.8, 0.8 | 943.4 (0.53) |
| synth_velo_occulte_10m | 3 | 5 | vélo (ancêtre le plus persistant) | [1501.6, 3003.1) racine | 134.8 (1.00) | 28.1 (1.00); marges 622.0, 879.6 | 16.0 (1.00) |
| synth_velo_occulte_10m | 3 | 5 | vélo/roue_arriere | [104.0, 104.2) | 31.0 (1.00) | 31.0 (1.00); marges 0.1, 0.1 | 31.0 (1.00) |
| synth_velo_occulte_10m | 3 | 5 | vélo/roue_arriere (ancêtre) | [104.2, 104.3) | 31.0 (1.00) | 31.0 (1.00); marges 0.0, 0.0 | 31.0 (1.00) |
| synth_velo_occulte_10m | 3 | 5 | vélo/roue_arriere (ancêtre) | [104.3, 104.4) | 31.0 (1.00) | 31.0 (1.00); marges 0.1, 0.1 | 31.0 (1.00) |
| synth_velo_occulte_10m | 3 | 5 | vélo/roue_avant | [124.9, 125.0) | 53.8 (1.00) | 53.8 (1.00); marges 0.0, 0.0 | 53.8 (1.00) |
| synth_velo_occulte_10m | 3 | 5 | vélo/roue_avant (ancêtre) | [125.0, 125.4) | 53.8 (1.00) | 54.8 (1.00); marges 0.2, 0.2 | 54.2 (1.00) |
| synth_velo_occulte_10m | 3 | 5 | vélo/roue_avant (ancêtre) | [125.4, 125.8) | 54.2 (1.00) | 51.0 (1.00); marges 0.2, 0.2 | 50.5 (1.00) |
| synth_velo_occulte_10m | 3 | 5 | vélo/cadre | [98.6, 99.0) | 35.3 (0.99) | 37.8 (0.97); marges 0.2, 0.2 | 37.8 (0.97) |
| synth_velo_occulte_10m | 3 | 5 | vélo/cadre (ancêtre) | [99.0, 99.1) | 37.8 (0.97) | 37.8 (0.97); marges 0.1, 0.1 | 37.8 (0.97) |
| synth_velo_occulte_10m | 3 | 5 | vélo/cadre (ancêtre) | [99.1, 99.6) | 37.8 (0.97) | 37.8 (0.97); marges 0.2, 0.2 | 37.8 (0.99) |
| synth_velo_occulte_10m | 3 | 5 | vélo/guidon | [75.0, 78.5) | 63.3 (0.80) | 33.7 (0.90); marges 1.7, 1.8 | 94.4 (0.69) |
| synth_velo_occulte_10m | 3 | 5 | vélo/guidon (ancêtre) | [78.5, 78.5) | 33.7 (0.85) | 33.7 (0.85); marges 0.0, 0.0 | 33.7 (0.85) |
| synth_velo_occulte_10m | 3 | 5 | vélo/guidon (ancêtre) | [78.5, 86.0) | 33.7 (0.92) | 11.3 (1.00); marges 3.6, 3.8 | 34.4 (0.93) |
| synth_velo_occulte_10m | 3 | 5 | vélo/selle | [78.9, 87.6) | 33.5 (0.91) | 31.9 (0.91); marges 4.2, 4.5 | 20.2 (1.00) |
| synth_velo_occulte_10m | 3 | 5 | vélo/selle (ancêtre) | [87.6, 96.5) | 7.9 (1.00) | 19.1 (1.00); marges 4.4, 4.6 | 13.6 (1.00) |

### Épreuve 1 — tour FULL (meilleurs nœuds oracle, IoU vérité avant → après, IoU des sites)

| Jeu | σ | k | cible | IoU avant | IoU après | IoU des sites |
| --- | --- | --- | --- | --- | --- | --- |
| decoupe_08_002852_deux_velos_6_51_instances | 3 | 5 | vélo | 0.87 | 0.90 | 0.97 |
| decoupe_08_002852_deux_velos_6_51_instances | 3 | 5 | vélo | 0.83 | 0.83 | 0.98 |
| decoupe_08_002852_deux_velos_6_51_instances | 10 | 5 | vélo | 0.87 | 0.89 | 0.99 |
| decoupe_08_002852_deux_velos_6_51_instances | 10 | 5 | vélo | 0.83 | 0.81 | 0.96 |
| synth_anneau_perce_05m | 3 | 2 | roue | 1.00 | 1.00 | 1.00 |
| synth_anneau_perce_05m | 3 | 2 | roue/pneu | 0.76 | 0.76 | 1.00 |
| synth_anneau_perce_05m | 3 | 2 | roue/jante | 0.24 | 0.26 | 0.78 |
| synth_anneau_perce_05m | 3 | 5 | roue | 1.00 | 1.00 | 1.00 |
| synth_anneau_perce_05m | 3 | 5 | roue/pneu | 0.76 | 0.76 | 1.00 |
| synth_anneau_perce_05m | 3 | 5 | roue/jante | 0.24 | 0.24 | 1.00 |
| synth_anneau_perce_05m | 10 | 2 | roue | 1.00 | 1.00 | 1.00 |
| synth_anneau_perce_05m | 10 | 2 | roue/pneu | 0.76 | 0.76 | 1.00 |
| synth_anneau_perce_05m | 10 | 2 | roue/jante | 0.24 | 0.24 | 0.99 |
| synth_anneau_perce_05m | 10 | 5 | roue | 1.00 | 1.00 | 1.00 |
| synth_anneau_perce_05m | 10 | 5 | roue/pneu | 0.76 | 0.76 | 1.00 |
| synth_anneau_perce_05m | 10 | 5 | roue/jante | 0.24 | 0.24 | 1.00 |
| synth_anneau_perce_10m | 3 | 2 | roue | 1.00 | 1.00 | 1.00 |
| synth_anneau_perce_10m | 3 | 2 | roue/pneu | 0.79 | 0.79 | 1.00 |
| synth_anneau_perce_10m | 3 | 2 | roue/jante | 0.45 | 0.33 | 0.56 |
| synth_anneau_perce_10m | 3 | 5 | roue | 1.00 | 1.00 | 1.00 |
| synth_anneau_perce_10m | 3 | 5 | roue/pneu | 0.79 | 0.79 | 1.00 |
| synth_anneau_perce_10m | 3 | 5 | roue/jante | 0.33 | 0.55 | 0.15 |
| synth_anneau_perce_10m | 10 | 2 | roue | 1.00 | 1.00 | 1.00 |
| synth_anneau_perce_10m | 10 | 2 | roue/pneu | 0.79 | 0.79 | 1.00 |
| synth_anneau_perce_10m | 10 | 2 | roue/jante | 0.45 | 0.36 | 0.62 |
| synth_anneau_perce_10m | 10 | 5 | roue | 1.00 | 1.00 | 1.00 |
| synth_anneau_perce_10m | 10 | 5 | roue/pneu | 0.79 | 0.79 | 1.00 |
| synth_anneau_perce_10m | 10 | 5 | roue/jante | 0.33 | 0.36 | 0.89 |
| synth_velo_10m | 3 | 2 | vélo | 1.00 | 1.00 | 1.00 |
| synth_velo_10m | 3 | 2 | vélo/roue_arriere | 0.66 | 0.66 | 1.00 |
| synth_velo_10m | 3 | 2 | vélo/roue_avant | 0.75 | 0.70 | 0.94 |
| synth_velo_10m | 3 | 2 | vélo/cadre | 0.34 | 0.34 | 1.00 |
| synth_velo_10m | 3 | 2 | vélo/guidon | 0.75 | 0.75 | 1.00 |
| synth_velo_10m | 3 | 2 | vélo/selle | 0.91 | 0.91 | 1.00 |
| synth_velo_10m | 3 | 5 | vélo | 1.00 | 1.00 | 1.00 |
| synth_velo_10m | 3 | 5 | vélo/roue_arriere | 0.49 | 0.49 | 0.99 |
| synth_velo_10m | 3 | 5 | vélo/roue_avant | 0.58 | 0.57 | 0.98 |
| synth_velo_10m | 3 | 5 | vélo/cadre | 0.41 | 0.42 | 0.99 |
| synth_velo_10m | 3 | 5 | vélo/guidon | 0.40 | 0.60 | 0.75 |
| synth_velo_10m | 3 | 5 | vélo/selle | 1.00 | 1.00 | 1.00 |
| synth_velo_10m | 10 | 2 | vélo | 1.00 | 1.00 | 1.00 |
| synth_velo_10m | 10 | 2 | vélo/roue_arriere | 0.66 | 0.70 | 0.58 |
| synth_velo_10m | 10 | 2 | vélo/roue_avant | 0.75 | 0.67 | 0.90 |
| synth_velo_10m | 10 | 2 | vélo/cadre | 0.34 | 0.36 | 0.92 |
| synth_velo_10m | 10 | 2 | vélo/guidon | 0.75 | 0.75 | 1.00 |
| synth_velo_10m | 10 | 2 | vélo/selle | 0.91 | 1.00 | 0.91 |
| synth_velo_10m | 10 | 5 | vélo | 1.00 | 1.00 | 1.00 |
| synth_velo_10m | 10 | 5 | vélo/roue_arriere | 0.49 | 0.51 | 0.78 |
| synth_velo_10m | 10 | 5 | vélo/roue_avant | 0.58 | 0.56 | 0.92 |
| synth_velo_10m | 10 | 5 | vélo/cadre | 0.41 | 0.40 | 0.98 |
| synth_velo_10m | 10 | 5 | vélo/guidon | 0.40 | 0.33 | 0.22 |
| synth_velo_10m | 10 | 5 | vélo/selle | 1.00 | 1.00 | 1.00 |
| synth_velo_occulte_10m | 3 | 2 | vélo | 1.00 | 1.00 | 1.00 |
| synth_velo_occulte_10m | 3 | 2 | vélo/roue_arriere | 0.68 | 0.68 | 1.00 |
| synth_velo_occulte_10m | 3 | 2 | vélo/roue_avant | 0.77 | 0.76 | 0.99 |
| synth_velo_occulte_10m | 3 | 2 | vélo/cadre | 0.28 | 0.28 | 1.00 |
| synth_velo_occulte_10m | 3 | 2 | vélo/guidon | 0.75 | 0.75 | 1.00 |
| synth_velo_occulte_10m | 3 | 2 | vélo/selle | 0.91 | 0.91 | 1.00 |
| synth_velo_occulte_10m | 3 | 5 | vélo | 1.00 | 1.00 | 1.00 |
| synth_velo_occulte_10m | 3 | 5 | vélo/roue_arriere | 0.66 | 0.66 | 1.00 |
| synth_velo_occulte_10m | 3 | 5 | vélo/roue_avant | 0.69 | 0.69 | 1.00 |
| synth_velo_occulte_10m | 3 | 5 | vélo/cadre | 0.29 | 0.29 | 0.99 |
| synth_velo_occulte_10m | 3 | 5 | vélo/guidon | 0.27 | 0.60 | 0.27 |
| synth_velo_occulte_10m | 3 | 5 | vélo/selle | 1.00 | 1.00 | 1.00 |

### Épreuve 2 — ajouts (d_B H0 de A_k et de la DTM, mm ; Betti 0 exacts : composantes nouvelles / fusions au moins)

| Jeu | ajout | m | k | m < k | d_B H0 A_k | d_B H0 DTM | A_k : nouvelles / fusions | DTM : nouvelles / fusions | sandwich : violations / tests |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| decoupe_08_002852_deux_velos_6_51_instances | groupe_proche | 1 | 1 | non | 5.37 | 5.37 | 1 / 2 | 1 / 2 | 0 / 1570 |
| decoupe_08_002852_deux_velos_6_51_instances | groupe_proche | 1 | 2 | oui | 6.21 | 4.44 | 3 / 3 | 2 / 2 | 0 / 1797 |
| decoupe_08_002852_deux_velos_6_51_instances | groupe_proche | 1 | 3 | oui | 5.96 | 3.76 | 4 / 4 | 1 / 3 | 0 / 1520 |
| decoupe_08_002852_deux_velos_6_51_instances | groupe_proche | 1 | 5 | oui | 8.65 | 6.11 | 6 / 5 | 2 / 2 | 0 / 1474 |
| decoupe_08_002852_deux_velos_6_51_instances | groupe_proche | 4 | 1 | non | 9.91 | 9.91 | 4 / 1 | 4 / 1 | 0 / 1570 |
| decoupe_08_002852_deux_velos_6_51_instances | groupe_proche | 4 | 2 | non | 26.45 | 18.39 | 3 / 4 | 2 / 2 | 0 / 898 |
| decoupe_08_002852_deux_velos_6_51_instances | groupe_proche | 4 | 3 | non | 29.73 | 23.57 | 4 / 4 | 2 / 1 | 0 / 761 |
| decoupe_08_002852_deux_velos_6_51_instances | groupe_proche | 4 | 5 | oui | 4.21 | 7.36 | 5 / 4 | 2 / 2 | 0 / 1486 |
| decoupe_08_002852_deux_velos_6_51_instances | groupe_lointain | 2 | 1 | non | 267.26 | 267.26 | 2 / 0 | 2 / 0 | 0 / 1570 |
| decoupe_08_002852_deux_velos_6_51_instances | groupe_lointain | 2 | 2 | non | 277.77 | 261.27 | 2 / 0 | 2 / 0 | 0 / 898 |
| decoupe_08_002852_deux_velos_6_51_instances | groupe_lointain | 2 | 3 | oui | 26.63 | 20.75 | 2 / 0 | 1 / 1 | 0 / 1522 |
| decoupe_08_002852_deux_velos_6_51_instances | groupe_lointain | 2 | 5 | oui | 2.06 | 0.00 | 2 / 0 | 1 / 1 | 0 / 1478 |
| synth_anneau_perce_05m | groupe_proche | 1 | 1 | non | 3.75 | 3.75 | 1 / 0 | 1 / 0 | 0 / 730 |
| synth_anneau_perce_05m | groupe_proche | 1 | 2 | oui | 2.86 | 1.07 | 1 / 0 | 1 / 0 | 0 / 1228 |
| synth_anneau_perce_05m | groupe_proche | 1 | 3 | oui | 2.42 | 0.53 | 1 / 0 | 1 / 1 | 0 / 1896 |
| synth_anneau_perce_05m | groupe_proche | 1 | 5 | oui | 2.14 | 0.08 | 3 / 0 | 1 / 0 | 0 / 1873 |
| synth_anneau_perce_05m | groupe_proche | 2 | 1 | non | 8.62 | 8.62 | 2 / 1 | 2 / 1 | 0 / 730 |
| synth_anneau_perce_05m | groupe_proche | 2 | 2 | non | 4.38 | 6.90 | 5 / 3 | 3 / 2 | 0 / 613 |
| synth_anneau_perce_05m | groupe_proche | 2 | 3 | oui | 2.65 | 4.44 | 6 / 6 | 5 / 2 | 0 / 1869 |
| synth_anneau_perce_05m | groupe_proche | 2 | 5 | oui | 7.59 | 4.21 | 9 / 5 | 4 / 4 | 0 / 1857 |
| synth_anneau_perce_05m | groupe_proche | 4 | 1 | non | 3.75 | 3.75 | 4 / 0 | 4 / 0 | 0 / 730 |
| synth_anneau_perce_05m | groupe_proche | 4 | 2 | non | 4.33 | 4.33 | 4 / 0 | 3 / 0 | 0 / 613 |
| synth_anneau_perce_05m | groupe_proche | 4 | 3 | non | 10.65 | 7.95 | 4 / 0 | 2 / 1 | 0 / 943 |
| synth_anneau_perce_05m | groupe_proche | 4 | 5 | oui | 7.13 | 6.78 | 4 / 0 | 3 / 1 | 0 / 1887 |
| synth_anneau_perce_05m | groupe_proche | 5 | 1 | non | 9.10 | 9.10 | 5 / 1 | 5 / 1 | 0 / 730 |
| synth_anneau_perce_05m | groupe_proche | 5 | 2 | non | 4.78 | 6.90 | 4 / 3 | 3 / 2 | 0 / 613 |
| synth_anneau_perce_05m | groupe_proche | 5 | 3 | non | 11.25 | 8.80 | 7 / 5 | 3 / 3 | 0 / 943 |
| synth_anneau_perce_05m | groupe_proche | 5 | 5 | non | 18.64 | 15.51 | 5 / 6 | 2 / 4 | 0 / 936 |
| synth_anneau_perce_05m | groupe_proche | 10 | 1 | non | 3.02 | 3.02 | 10 / 1 | 10 / 1 | 0 / 730 |
| synth_anneau_perce_05m | groupe_proche | 10 | 2 | non | 5.64 | 5.64 | 7 / 1 | 5 / 1 | 0 / 613 |
| synth_anneau_perce_05m | groupe_proche | 10 | 3 | non | 12.73 | 10.04 | 7 / 3 | 4 / 1 | 0 / 943 |
| synth_anneau_perce_05m | groupe_proche | 10 | 5 | non | 23.05 | 18.05 | 7 / 2 | 3 / 2 | 0 / 936 |
| synth_anneau_perce_05m | epars_proche | 4 | 1 | non | 3.75 | 3.75 | 4 / 2 | 4 / 2 | 0 / 730 |
| synth_anneau_perce_05m | epars_proche | 4 | 2 | non | 3.17 | 3.17 | 7 / 6 | 7 / 4 | 0 / 613 |
| synth_anneau_perce_05m | epars_proche | 4 | 3 | non | 6.94 | 4.23 | 10 / 9 | 5 / 4 | 0 / 943 |
| synth_anneau_perce_05m | epars_proche | 4 | 5 | oui | 3.67 | 5.39 | 19 / 11 | 5 / 6 | 0 / 1889 |
| synth_anneau_perce_05m | groupe_lointain | 2 | 1 | non | 188.73 | 188.73 | 2 / 0 | 2 / 0 | 0 / 730 |
| synth_anneau_perce_05m | groupe_lointain | 2 | 2 | non | 186.52 | 184.99 | 6 / 0 | 2 / 0 | 0 / 613 |
| synth_anneau_perce_05m | groupe_lointain | 2 | 3 | oui | 1.70 | 11.28 | 8 / 0 | 5 / 0 | 0 / 1886 |
| synth_anneau_perce_05m | groupe_lointain | 2 | 5 | oui | 1.11 | 0.00 | 10 / 0 | 1 / 1 | 0 / 1872 |
| synth_anneau_perce_05m | groupe_lointain | 5 | 1 | non | 188.35 | 188.35 | 5 / 0 | 5 / 0 | 0 / 730 |
| synth_anneau_perce_05m | groupe_lointain | 5 | 2 | non | 185.91 | 184.61 | 5 / 0 | 2 / 0 | 0 / 613 |
| synth_anneau_perce_05m | groupe_lointain | 5 | 3 | non | 182.97 | 182.71 | 12 / 0 | 2 / 0 | 0 / 943 |
| synth_anneau_perce_05m | groupe_lointain | 5 | 5 | non | 178.46 | 179.17 | 20 / 0 | 2 / 0 | 0 / 936 |
| synth_anneau_perce_10m | groupe_proche | 1 | 1 | non | 7.76 | 7.76 | 1 / 0 | 1 / 0 | 0 / 300 |
| synth_anneau_perce_10m | groupe_proche | 1 | 2 | oui | 4.96 | 1.96 | 2 / 0 | 1 / 0 | 0 / 577 |
| synth_anneau_perce_10m | groupe_proche | 1 | 3 | oui | 11.05 | 2.24 | 3 / 1 | 1 / 1 | 0 / 482 |
| synth_anneau_perce_10m | groupe_proche | 1 | 5 | oui | 11.06 | 4.44 | 4 / 2 | 3 / 4 | 0 / 311 |
| synth_anneau_perce_10m | groupe_proche | 2 | 1 | non | 7.76 | 7.76 | 2 / 0 | 2 / 0 | 0 / 300 |
| synth_anneau_perce_10m | groupe_proche | 2 | 2 | non | 10.04 | 10.04 | 3 / 1 | 2 / 0 | 0 / 285 |
| synth_anneau_perce_10m | groupe_proche | 2 | 3 | oui | 4.70 | 1.46 | 4 / 2 | 2 / 1 | 0 / 488 |
| synth_anneau_perce_10m | groupe_proche | 2 | 5 | oui | 12.52 | 7.17 | 5 / 3 | 2 / 3 | 0 / 301 |
| synth_anneau_perce_10m | groupe_proche | 4 | 1 | non | 7.76 | 7.76 | 4 / 0 | 4 / 0 | 0 / 300 |
| synth_anneau_perce_10m | groupe_proche | 4 | 2 | non | 12.25 | 12.25 | 3 / 0 | 2 / 0 | 0 / 285 |
| synth_anneau_perce_10m | groupe_proche | 4 | 3 | non | 23.03 | 17.80 | 3 / 0 | 2 / 1 | 0 / 243 |
| synth_anneau_perce_10m | groupe_proche | 4 | 5 | oui | 8.87 | 9.63 | 6 / 0 | 3 / 0 | 0 / 320 |
| synth_anneau_perce_10m | groupe_proche | 5 | 1 | non | 7.76 | 7.76 | 5 / 0 | 5 / 0 | 0 / 300 |
| synth_anneau_perce_10m | groupe_proche | 5 | 2 | non | 9.94 | 9.94 | 6 / 0 | 3 / 0 | 0 / 285 |
| synth_anneau_perce_10m | groupe_proche | 5 | 3 | non | 22.37 | 16.89 | 6 / 3 | 3 / 1 | 0 / 243 |
| synth_anneau_perce_10m | groupe_proche | 5 | 5 | non | 34.85 | 26.86 | 4 / 4 | 2 / 4 | 0 / 156 |
| synth_anneau_perce_10m | groupe_proche | 10 | 1 | non | 7.91 | 7.91 | 10 / 0 | 10 / 0 | 0 / 300 |
| synth_anneau_perce_10m | groupe_proche | 10 | 2 | non | 12.73 | 12.73 | 10 / 0 | 4 / 0 | 0 / 285 |
| synth_anneau_perce_10m | groupe_proche | 10 | 3 | non | 23.09 | 18.35 | 8 / 1 | 6 / 1 | 0 / 243 |
| synth_anneau_perce_10m | groupe_proche | 10 | 5 | non | 38.23 | 29.69 | 7 / 3 | 4 / 3 | 0 / 156 |
| synth_anneau_perce_10m | epars_proche | 4 | 1 | non | 7.77 | 7.77 | 4 / 1 | 4 / 1 | 0 / 300 |
| synth_anneau_perce_10m | epars_proche | 4 | 2 | non | 10.18 | 10.18 | 7 / 5 | 6 / 2 | 0 / 285 |
| synth_anneau_perce_10m | epars_proche | 4 | 3 | non | 14.16 | 9.35 | 8 / 6 | 5 / 4 | 0 / 243 |
| synth_anneau_perce_10m | epars_proche | 4 | 5 | oui | 17.61 | 14.14 | 7 / 9 | 5 / 5 | 0 / 310 |
| synth_anneau_perce_10m | groupe_lointain | 2 | 1 | non | 194.98 | 194.98 | 2 / 0 | 2 / 0 | 0 / 300 |
| synth_anneau_perce_10m | groupe_lointain | 2 | 2 | non | 187.96 | 186.85 | 2 / 0 | 2 / 0 | 0 / 285 |
| synth_anneau_perce_10m | groupe_lointain | 2 | 3 | oui | 2.18 | 11.48 | 5 / 0 | 3 / 1 | 0 / 486 |
| synth_anneau_perce_10m | groupe_lointain | 2 | 5 | oui | 2.61 | 0.00 | 6 / 0 | 0 / 0 | 0 / 312 |
| synth_anneau_perce_10m | groupe_lointain | 5 | 1 | non | 194.99 | 194.99 | 5 / 0 | 5 / 0 | 0 / 300 |
| synth_anneau_perce_10m | groupe_lointain | 5 | 2 | non | 188.37 | 187.23 | 3 / 0 | 3 / 0 | 0 / 285 |
| synth_anneau_perce_10m | groupe_lointain | 5 | 3 | non | 181.24 | 182.90 | 5 / 0 | 2 / 0 | 0 / 243 |
| synth_anneau_perce_10m | groupe_lointain | 5 | 5 | non | 173.59 | 176.55 | 9 / 0 | 1 / 0 | 0 / 156 |
| synth_pieton_05m | groupe_proche | 1 | 1 | non | 3.51 | 3.51 | 1 / 0 | 1 / 0 | 0 / 2408 |
| synth_pieton_05m | groupe_proche | 1 | 2 | oui | 0.23 | 0.14 | 2 / 0 | 1 / 0 | 0 / 5143 |
| synth_pieton_05m | groupe_proche | 1 | 3 | oui | 0.33 | 0.04 | 3 / 2 | 1 / 1 | 0 / 6300 |
| synth_pieton_05m | groupe_proche | 1 | 5 | oui | 0.82 | 0.64 | 10 / 4 | 1 / 2 | 0 / 3965 |
| synth_pieton_05m | groupe_proche | 4 | 1 | non | 3.05 | 3.05 | 4 / 1 | 4 / 1 | 0 / 2408 |
| synth_pieton_05m | groupe_proche | 4 | 2 | non | 4.08 | 4.08 | 3 / 4 | 2 / 2 | 0 / 2570 |
| synth_pieton_05m | groupe_proche | 4 | 3 | non | 10.66 | 8.07 | 4 / 10 | 2 / 3 | 0 / 3147 |
| synth_pieton_05m | groupe_proche | 4 | 5 | oui | 10.04 | 9.08 | 5 / 5 | 2 / 9 | 0 / 3938 |
| synth_pieton_05m | groupe_lointain | 2 | 1 | non | 131.06 | 131.06 | 2 / 0 | 2 / 0 | 0 / 2408 |
| synth_pieton_05m | groupe_lointain | 2 | 2 | non | 129.88 | 127.28 | 3 / 0 | 2 / 0 | 0 / 2570 |
| synth_pieton_05m | groupe_lointain | 2 | 3 | oui | 1.48 | 7.44 | 2 / 0 | 2 / 1 | 0 / 6294 |
| synth_pieton_05m | groupe_lointain | 2 | 5 | oui | 0.88 | 0.00 | 2 / 0 | 1 / 1 | 0 / 3948 |
| synth_velo_05m | groupe_proche | 1 | 1 | non | 3.75 | 3.75 | 1 / 1 | 1 / 1 | 0 / 1910 |
| synth_velo_05m | groupe_proche | 1 | 2 | oui | 1.61 | 2.05 | 2 / 3 | 1 / 2 | 0 / 3019 |
| synth_velo_05m | groupe_proche | 1 | 3 | oui | 6.57 | 2.70 | 4 / 2 | 3 / 3 | 0 / 3775 |
| synth_velo_05m | groupe_proche | 1 | 5 | oui | 5.82 | 3.79 | 5 / 8 | 2 / 5 | 0 / 2510 |
| synth_velo_05m | groupe_proche | 4 | 1 | non | 3.75 | 3.75 | 4 / 1 | 4 / 1 | 0 / 1910 |
| synth_velo_05m | groupe_proche | 4 | 2 | non | 5.37 | 5.37 | 4 / 2 | 3 / 1 | 0 / 1510 |
| synth_velo_05m | groupe_proche | 4 | 3 | non | 11.34 | 9.12 | 3 / 3 | 2 / 3 | 0 / 1891 |
| synth_velo_05m | groupe_proche | 4 | 5 | oui | 14.22 | 11.76 | 5 / 6 | 2 / 5 | 0 / 2497 |
| synth_velo_05m | groupe_lointain | 2 | 1 | non | 182.60 | 182.60 | 2 / 0 | 2 / 0 | 0 / 1910 |
| synth_velo_05m | groupe_lointain | 2 | 2 | non | 181.83 | 178.29 | 3 / 0 | 2 / 0 | 0 / 1510 |
| synth_velo_05m | groupe_lointain | 2 | 3 | oui | 6.35 | 11.86 | 3 / 0 | 1 / 1 | 0 / 3782 |
| synth_velo_05m | groupe_lointain | 2 | 5 | oui | 1.77 | 0.00 | 3 / 0 | 0 / 1 | 0 / 2506 |
| synth_velo_10m | groupe_proche | 1 | 1 | non | 3.82 | 3.82 | 1 / 1 | 1 / 1 | 0 / 953 |
| synth_velo_10m | groupe_proche | 1 | 2 | oui | 7.86 | 7.86 | 2 / 2 | 2 / 2 | 0 / 1522 |
| synth_velo_10m | groupe_proche | 1 | 3 | oui | 6.69 | 7.08 | 3 / 3 | 2 / 1 | 0 / 1168 |
| synth_velo_10m | groupe_proche | 1 | 5 | oui | 3.17 | 3.01 | 5 / 6 | 2 / 2 | 0 / 847 |
| synth_velo_10m | groupe_proche | 2 | 1 | non | 7.75 | 7.75 | 2 / 1 | 2 / 1 | 0 / 953 |
| synth_velo_10m | groupe_proche | 2 | 2 | non | 3.59 | 2.56 | 3 / 2 | 2 / 2 | 0 / 760 |
| synth_velo_10m | groupe_proche | 2 | 3 | oui | 10.61 | 5.31 | 3 / 3 | 2 / 3 | 0 / 1173 |
| synth_velo_10m | groupe_proche | 2 | 5 | oui | 3.26 | 1.24 | 5 / 4 | 3 / 2 | 0 / 851 |
| synth_velo_10m | groupe_proche | 4 | 1 | non | 5.84 | 5.84 | 4 / 2 | 4 / 2 | 0 / 953 |
| synth_velo_10m | groupe_proche | 4 | 2 | non | 9.57 | 9.57 | 5 / 3 | 4 / 2 | 0 / 760 |
| synth_velo_10m | groupe_proche | 4 | 3 | non | 18.77 | 14.68 | 4 / 4 | 3 / 2 | 0 / 584 |
| synth_velo_10m | groupe_proche | 4 | 5 | oui | 25.46 | 20.82 | 10 / 6 | 2 / 2 | 0 / 850 |
| synth_velo_10m | groupe_proche | 5 | 1 | non | 7.73 | 7.73 | 5 / 1 | 5 / 1 | 0 / 953 |
| synth_velo_10m | groupe_proche | 5 | 2 | non | 6.79 | 6.79 | 5 / 0 | 4 / 1 | 0 / 760 |
| synth_velo_10m | groupe_proche | 5 | 3 | non | 19.76 | 14.20 | 4 / 1 | 2 / 2 | 0 / 584 |
| synth_velo_10m | groupe_proche | 5 | 5 | non | 22.55 | 20.37 | 4 / 2 | 2 / 0 | 0 / 423 |
| synth_velo_10m | groupe_proche | 10 | 1 | non | 7.75 | 7.75 | 10 / 2 | 10 / 2 | 0 / 953 |
| synth_velo_10m | groupe_proche | 10 | 2 | non | 10.97 | 10.97 | 7 / 3 | 5 / 2 | 0 / 760 |
| synth_velo_10m | groupe_proche | 10 | 3 | non | 25.14 | 19.47 | 9 / 4 | 4 / 2 | 0 / 584 |
| synth_velo_10m | groupe_proche | 10 | 5 | non | 32.30 | 27.80 | 7 / 8 | 3 / 2 | 0 / 423 |
| synth_velo_10m | epars_proche | 4 | 1 | non | 7.75 | 7.75 | 4 / 1 | 4 / 1 | 0 / 953 |
| synth_velo_10m | epars_proche | 4 | 2 | non | 12.11 | 12.11 | 4 / 4 | 4 / 3 | 0 / 760 |
| synth_velo_10m | epars_proche | 4 | 3 | non | 14.88 | 10.23 | 8 / 3 | 6 / 3 | 0 / 584 |
| synth_velo_10m | epars_proche | 4 | 5 | oui | 10.35 | 8.60 | 8 / 8 | 7 / 2 | 0 / 849 |
| synth_velo_10m | groupe_lointain | 2 | 1 | non | 154.26 | 154.26 | 2 / 0 | 2 / 0 | 0 / 953 |
| synth_velo_10m | groupe_lointain | 2 | 2 | non | 151.05 | 146.65 | 2 / 0 | 2 / 0 | 0 / 760 |
| synth_velo_10m | groupe_lointain | 2 | 3 | oui | 5.26 | 10.66 | 3 / 0 | 1 / 1 | 0 / 1168 |
| synth_velo_10m | groupe_lointain | 2 | 5 | oui | 3.83 | 0.00 | 7 / 0 | 0 / 1 | 0 / 846 |
| synth_velo_10m | groupe_lointain | 5 | 1 | non | 146.83 | 146.83 | 5 / 0 | 5 / 0 | 0 / 953 |
| synth_velo_10m | groupe_lointain | 5 | 2 | non | 145.50 | 141.12 | 4 / 0 | 3 / 0 | 0 / 760 |
| synth_velo_10m | groupe_lointain | 5 | 3 | non | 139.84 | 138.39 | 3 / 0 | 2 / 0 | 0 / 584 |
| synth_velo_10m | groupe_lointain | 5 | 5 | non | 139.89 | 135.02 | 6 / 0 | 2 / 0 | 0 / 423 |

### Épreuve 2 — groupes lointains (séparation > 2 r*)

| Jeu | m | k | m < k | diagrammes identiques sous r* | naissance prédite (mm) | barres nées à la valeur prédite |
| --- | --- | --- | --- | --- | --- | --- |
| decoupe_08_002852_deux_velos_6_51_instances | 2 | 1 | non | non | 0.00 | 281 |
| decoupe_08_002852_deux_velos_6_51_instances | 2 | 2 | non | non | 13.29 | 1 |
| decoupe_08_002852_deux_velos_6_51_instances | 2 | 3 | oui | oui | — | 0 |
| decoupe_08_002852_deux_velos_6_51_instances | 2 | 5 | oui | oui | — | 0 |
| synth_anneau_perce_05m | 2 | 1 | non | non | 0.00 | 172 |
| synth_anneau_perce_05m | 2 | 2 | non | non | 7.33 | 1 |
| synth_anneau_perce_05m | 2 | 3 | oui | oui | — | 0 |
| synth_anneau_perce_05m | 2 | 5 | oui | oui | — | 0 |
| synth_anneau_perce_05m | 5 | 1 | non | non | 0.00 | 175 |
| synth_anneau_perce_05m | 5 | 2 | non | non | 2.02 | 1 |
| synth_anneau_perce_05m | 5 | 3 | non | non | 3.63 | 1 |
| synth_anneau_perce_05m | 5 | 5 | non | non | 6.89 | 1 |
| synth_anneau_perce_10m | 2 | 1 | non | non | 0.00 | 54 |
| synth_anneau_perce_10m | 2 | 2 | non | non | 16.25 | 1 |
| synth_anneau_perce_10m | 2 | 3 | oui | oui | — | 0 |
| synth_anneau_perce_10m | 2 | 5 | oui | oui | — | 0 |
| synth_anneau_perce_10m | 5 | 1 | non | non | 0.00 | 57 |
| synth_anneau_perce_10m | 5 | 2 | non | non | 3.51 | 1 |
| synth_anneau_perce_10m | 5 | 3 | non | non | 7.92 | 1 |
| synth_anneau_perce_10m | 5 | 5 | non | non | 19.02 | 1 |
| synth_pieton_05m | 2 | 1 | non | non | 0.00 | 651 |
| synth_pieton_05m | 2 | 2 | non | non | 7.62 | 1 |
| synth_pieton_05m | 2 | 3 | oui | oui | — | 0 |
| synth_pieton_05m | 2 | 5 | oui | oui | — | 0 |
| synth_velo_05m | 2 | 1 | non | non | 0.00 | 577 |
| synth_velo_05m | 2 | 2 | non | non | 8.91 | 1 |
| synth_velo_05m | 2 | 3 | oui | oui | — | 0 |
| synth_velo_05m | 2 | 5 | oui | oui | — | 0 |
| synth_velo_10m | 2 | 1 | non | non | 0.00 | 205 |
| synth_velo_10m | 2 | 2 | non | non | 10.24 | 1 |
| synth_velo_10m | 2 | 3 | oui | oui | — | 0 |
| synth_velo_10m | 2 | 5 | oui | oui | — | 0 |
| synth_velo_10m | 5 | 1 | non | non | 0.00 | 208 |
| synth_velo_10m | 5 | 2 | non | non | 2.74 | 1 |
| synth_velo_10m | 5 | 3 | non | non | 9.27 | 1 |
| synth_velo_10m | 5 | 5 | non | non | 21.01 | 1 |

### Épreuve 2 — suppressions (Ω_{k+m}^P ⊆ Ω_k^Q ⊆ Ω_k^P)

| Jeu | m | k | d_B H0 | nouvelles / fusions (Betti 0) | sandwich : violations / tests |
| --- | --- | --- | --- | --- | --- |
| decoupe_08_002852_deux_velos_6_51_instances | 1 | 1 | 5.37 | 2 / 1 | 0 / 2655 |
| decoupe_08_002852_deux_velos_6_51_instances | 1 | 2 | 3.04 | 4 / 4 | 0 / 1648 |
| decoupe_08_002852_deux_velos_6_51_instances | 1 | 3 | 3.03 | 5 / 5 | 0 / 1471 |
| decoupe_08_002852_deux_velos_6_51_instances | 1 | 5 | 6.78 | 6 / 7 | — / — |
| synth_anneau_perce_05m | 1 | 1 | 3.75 | 1 / 1 | 0 / 1701 |
| synth_anneau_perce_05m | 1 | 2 | 3.96 | 1 / 2 | 0 / 1531 |
| synth_anneau_perce_05m | 1 | 3 | 2.76 | 6 / 2 | 0 / 2187 |
| synth_anneau_perce_05m | 1 | 5 | 2.23 | 2 / 5 | — / — |
| synth_anneau_perce_05m | 4 | 1 | 4.24 | 3 / 4 | 0 / 730 |
| synth_anneau_perce_05m | 4 | 2 | 4.38 | 8 / 6 | — / — |
| synth_anneau_perce_05m | 4 | 3 | 2.99 | 13 / 12 | — / — |
| synth_anneau_perce_05m | 4 | 5 | 6.08 | 8 / 14 | — / — |
| synth_anneau_perce_10m | 1 | 1 | 7.91 | 1 / 1 | 0 / 576 |
| synth_anneau_perce_10m | 1 | 2 | 9.39 | 1 / 2 | 0 / 436 |
| synth_anneau_perce_10m | 1 | 3 | 15.56 | 2 / 2 | 0 / 344 |
| synth_anneau_perce_10m | 1 | 5 | 13.29 | 2 / 2 | — / — |
| synth_anneau_perce_10m | 4 | 1 | 17.05 | 2 / 4 | 0 / 284 |
| synth_anneau_perce_10m | 4 | 2 | 42.00 | 3 / 6 | — / — |
| synth_anneau_perce_10m | 4 | 3 | 32.46 | 2 / 5 | — / — |
| synth_anneau_perce_10m | 4 | 5 | 19.56 | 3 / 6 | — / — |
| synth_pieton_05m | 1 | 1 | 3.51 | 0 / 1 | 0 / 6375 |
| synth_pieton_05m | 1 | 2 | 1.55 | 1 / 3 | 0 / 5564 |
| synth_pieton_05m | 1 | 3 | 1.13 | 2 / 3 | 0 / 5744 |
| synth_pieton_05m | 1 | 5 | 1.32 | 2 / 4 | — / — |
| synth_velo_05m | 1 | 1 | 3.75 | 1 / 1 | 0 / 4498 |
| synth_velo_05m | 1 | 2 | 2.81 | 2 / 2 | 0 / 3662 |
| synth_velo_05m | 1 | 3 | 1.75 | 3 / 3 | 0 / 4301 |
| synth_velo_05m | 1 | 5 | 0.55 | 4 / 3 | — / — |
| synth_velo_10m | 1 | 1 | 7.75 | 1 / 1 | 0 / 1926 |
| synth_velo_10m | 1 | 2 | 3.59 | 2 / 2 | 0 / 1379 |
| synth_velo_10m | 1 | 3 | 6.69 | 3 / 3 | 0 / 1067 |
| synth_velo_10m | 1 | 5 | 5.71 | 5 / 5 | — / — |
| synth_velo_10m | 4 | 1 | 7.75 | 2 / 4 | 0 / 971 |
| synth_velo_10m | 4 | 2 | 12.23 | 5 / 6 | — / — |
| synth_velo_10m | 4 | 3 | 6.69 | 6 / 8 | — / — |
| synth_velo_10m | 4 | 5 | 10.08 | 9 / 14 | — / — |

### Épreuve 3 — décimation (d_B H0 en mm ; k fixé contre k' = arrondi de k ρ)

| Jeu | ρ | δ transport max / médian (mm) | espacement (mm) | k = 2 : fixé / ajusté (k') | k = 3 : fixé / ajusté (k') | k = 5 : fixé / ajusté (k') | k = 5 DTM fixé / ajusté |
| --- | --- | --- | --- | --- | --- | --- | --- |
| decoupe_08_002852_deux_velos_6_51_instances | 0.5 | 356.90 / 82.67 | 57.2 | 33.99 / 67.24 (1) | 31.74 / 83.77 (2) | 58.09 / 73.11 (2) | 33.77 / 53.99 |
| synth_anneau_perce_05m | 0.9 | 33.75 / 17.20 | 16.1 | 4.42 / 4.42 (2) | 5.74 / 5.74 (3) | 7.02 / 4.46 (4) | 5.66 / 4.69 |
| synth_anneau_perce_05m | 0.5 | 50.01 / 18.00 | 16.1 | 23.75 / 15.04 (1) | 27.54 / 23.75 (2) | 32.99 / 19.78 (2) | 20.91 / 15.22 |
| synth_anneau_perce_10m | 0.9 | 38.39 / 32.56 | 32.8 | 12.52 / 12.52 (2) | 15.56 / 15.56 (3) | 14.24 / 22.12 (4) | 10.49 / 16.18 |
| synth_anneau_perce_10m | 0.5 | 110.77 / 36.00 | 32.8 | 62.33 / 64.24 (1) | 27.33 / 61.69 (2) | 18.40 / 45.57 (2) | 14.23 / 49.99 |
| synth_pieton_05m | 0.5 | 102.90 / 20.83 | 17.0 | 20.99 / 43.81 (1) | 25.00 / 16.51 (2) | 28.21 / 19.59 (2) | 12.83 / 25.07 |
| synth_velo_05m | 0.5 | 138.25 / 18.63 | 16.3 | 18.79 / 19.89 (1) | 45.49 / 31.76 (2) | 27.02 / 48.60 (2) | 26.86 / 50.16 |
| synth_velo_10m | 0.9 | 62.81 / 32.23 | 32.3 | 18.69 / 18.69 (2) | 13.95 / 13.95 (3) | 23.11 / 18.16 (4) | 14.20 / 12.99 |
| synth_velo_10m | 0.5 | 105.10 / 37.34 | 32.3 | 45.08 / 63.10 (1) | 36.34 / 27.65 (2) | 47.44 / 29.03 (2) | 34.76 / 23.60 |
