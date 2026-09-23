# Reçu d'audit exploratoire : densité LiDAR 08/000200 sans sol

23 septembre 2026. Source : `scene_02_grid` du reçu v8
`lidar_ground_20260921`, 45 845 sites sur grille 1 mm. Les 28 nouvelles
sondes sont `1/4` et `1/2` de la scène entière et de ses deux moitiés et
quatre quarts, K5/K10, s8/W8 ; un cas à densité entière est rejoué comme
témoin. Les 14 autres cas à densité entière viennent du
[reçu v12](../../receipts/lidar_scaling_local_20260923/README.md).
Toutes les sorties sont `complete_relative`, pas une preuve des clés
géométriques absentes du catalogue.

`generate.py` est le générateur exact utilisé. Il classe les IDs originaux
avec `splitmix64(ID XOR d1da73a520260923)`, choisit globalement les
11 461 puis 22 922 premiers, conserve l'ordre des sites d'origine et
intersecte avec les sept secteurs définis par les plans du capteur.
`MANIFEST.json` épingle les SHA-256 de chaque entrée et les effectifs.
Le script conserve les chemins locaux de cette capture ; pour un rejeu
ailleurs, fournir les mêmes entrées v8 et vérifier leurs hashes avant
d'adapter ces chemins.

`CASES.jsonl` conserve les **28 sorties brutes de nouvelles sondes et le
témoin rejoué**. `SUMMARY.json` conserve les 42 cas analysés, les valeurs
de travail, digests et hashes, puis les pentes calculées avec les effectifs
réels. Le binaire Release CPU du commit `4530644b` a le SHA-256
`e1ba126fbcea8ad483ebff2265f446c18e90eaefe04bf20021e76cd0df09af80`.
Les nouvelles commandes emploient `--static=8 --grid=1mm`, tandis que les
cas v12 historiques avaient le libellé `grid=unspecified` ; leur géométrie
1 mm est attestée par le manifeste v8. Leurs digests et masses principales
coïncident lors du témoin rejoué ; quelques comptes de cache dépendent de
l'ordre concurrent W8.

Hôte CPU local partagé, une graine et une exécution par cas : les chronos
sont indicatifs. Voir la [lecture des deux axes de
croissance](../CROISSANCE_LIDAR_PLANS_ET_DENSITE_20260923.md) pour les
conclusions et les limites.
