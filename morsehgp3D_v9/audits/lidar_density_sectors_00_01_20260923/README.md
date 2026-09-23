# Reçu d'audit exploratoire : densité des secteurs 08/000000 et 08/000100

23 septembre 2026. Complète la matrice de densité du
[reçu 08/000200](../lidar_density_scene02_20260923/README.md) et les
[trames entières 08/000000–000100](../lidar_density_full_3scenes_20260923/README.md).
Dans chacune des deux scènes sans sol sur grille 1 mm, les sous-ensembles
globaux `1/4⊂1/2⊂1` sont intersectés avec les deux moitiés et les quatre
quarts définis par les plans du capteur. K5/K10, s8/W8, même graine
`d1da73a520260923` et même binaire Release du commit `4530644b`
(SHA-256 `e1ba126fbcea8ad483ebff2265f446c18e90eaefe04bf20021e76cd0df09af80`).

**48 nouvelles sondes** sont conservées dans `CASES.jsonl`. `SUMMARY.json`
les joint aux huit cas déjà mesurés sur les trames entières décimées et
aux 28 cas à densité entière du reçu v12 : **84 cas** au total. Les
entrées, IDs, SHA-256, FNV, options, sorties, catalogue, digests et pentes
sont contrôlés. `CONTROL.json` garde un rejeu K10 dont le digest et les
masses stables correspondent à la première exécution. Toutes les sondes
ont le statut `complete_relative`.

`generate_sectors.py`, `run_sectors.py` et `summarize_sectors.py` sont les
scripts exacts de cette capture ; `MANIFEST.json` épingle les entrées.
Le générateur consomme les entrées pleines décimées du reçu compagnon.
Les chemins `/workspaces` et `/tmp` dans les scripts correspondent à cet
hôte : pour un rejeu ailleurs, reconstruire les entrées par leurs SHA puis
adapter ces chemins. Les points LiDAR bruts ne sont pas versionnés ici.

Sur les **56 relations adjacentes de densité** des deux scènes, **sept**
pentes du total des formes de cœur calculées (`core_sites`) atteignent ou
dépassent 2 ; le sous-total `dead_core_form_sites`, qui écarte deux
extrémités pourtant calculées à chaque charge, en a **dix**. Aucune pente
des paires, des visites/bornes du cover, des émissions
q3/q4, du catalogue ou des CPU·s ne le fait. Le quart `x≥0,y<0` est le
seul secteur avec un tel franchissement dans chacune des trois scènes
étudiées. Voir la [lecture des deux axes de
croissance](../CROISSANCE_LIDAR_PLANS_ET_DENSITE_20260923.md).

Hôte CPU partagé, une seule graine, une exécution par cas, trois trames
d'une seule séquence : les temps sont indicatifs et ces pentes finies ne
prouvent ni borne asymptotique ni contrat G4.
