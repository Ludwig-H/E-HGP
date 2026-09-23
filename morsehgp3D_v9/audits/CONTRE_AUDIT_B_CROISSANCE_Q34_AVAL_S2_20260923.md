# S2 ne ferme pas encore la croissance des formes q3/q4

23 septembre 2026. Relecture B des reçus CPU locaux
[sans sol 8k/16k/32k](../receipts/lidar_scaling_local_20260923/README.md)
et des [trois densités de la trame brute K10](lidar_raw_k10_density_20260923/README.md),
en regard du raccord batch S2 **WIP**. Aucun GCP lancé ici. Ces nuages
8k⊂16k⊂32k sont des **disques emboîtés**, pas les moitiés/quarts
spatiaux par plans du capteur ; la décimation de la trame brute par ID
est encore une autre expérience. Toutes ces mesures sont finies,
mono-séquence 08 et `complete_relative`, sans preuve d'asymptotique.

Sur le sans-sol 08/000200, s8/W8, les ratios aux doublements successifs
sont :

| K | Sites | Paires développées | Formes cœur (`core_sites`) | Formes cover (`cover_sites`) | Supports q3+q4 émis |
| ---: | --- | ---: | ---: | ---: | ---: |
| 5 | 8k→16k | ×2,40 | ×2,72 | ×2,60 | ×1,89 |
| 5 | 16k→32k | **×4,81** | **×8,27** | ×3,81 | ×1,83 |
| 10 | 8k→16k | ×2,20 | ×2,41 | ×2,63 | ×1,88 |
| 10 | 16k→32k | **×4,27** | **×7,25** | ×3,74 | ×1,84 |

Les [JSON bruts K5](../receipts/lidar_scaling_local_20260923/out/s02_k5_w8_r0/)
et [K10](../receipts/lidar_scaling_local_20260923/out/s02_k10_w8_r0/)
donnent les masses ; `core_sites` et `cover_sites` comptent toutes les
formes chargées, **extrémités comprises**, pas les seules formes
restantes. À K5, la fraction de la masse de paires résiduelle WSPD
effectivement développée monte de **15,4 %→17,7 %→28,4 %**. Les
formes par charge du cœur montent de **54,0→71,7→191,1** et les
formes par cover de **176,5→230,9→419,4**. À K10, les formes par
charge montent de **94,5→111,3→263,5** ; par cover,
**273,5→354,2→610,7**. Le nombre de supports émis reste doux,
mais ne reflète donc pas le travail du cœur et du cover.

Sur la [trame brute 08/000000](lidar_raw_k10_density_20260923/README.md),
le doublement 61 694→123 389 retours donne encore **×4,34 formes cœur
à K5** et ×3,79 à K10. À pleine taille K10, les comptes bruts sont
**37 868 819 paires** développées, **1 254 254 109 formes cœur**,
**1 074 719 197 formes cover**, **11 387 391 boules** de catalogue,
**8,219 Gio** de RSS et **905,514 CPU·s** locaux. Ce n'est qu'une
trame brute d'une seule séquence, sans GPU, mais c'est déjà un coût
absolu déterminant pour le budget d'une seconde.

Le filtre S1 GPU accélère la décision des masques ; le batch S2 actuel
**ne réduit pas** les formes par cœur/cover des paires survivantes,
ni leurs sorties et FULL. Une belle pente des émissions ou 43–107 ms
de filtre ne doivent donc pas être présentés comme une croissance
sous-quadratique **du calcul complet**. Le prochain reçu intégré doit
apparier CPU/GPU sur mêmes octets et juger `R`, masse WSPD, `P`, `S`,
charges de cœur, `core_sites`, `cover_sites`, sorties, catalogue,
digest FULL, CPU·s, mur et RSS/HBM, puis répéter sur plusieurs scènes
brutes et sans sol. Les ratios locaux supérieurs à quatre révèlent des
régimes à traiter ; ils ne prouvent pas non plus une loi quadratique
universelle.
