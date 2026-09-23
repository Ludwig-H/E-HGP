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

## Contre-épreuve d'un extrême géométrique

Dans le quart `x≥0,y<0`, les minimums `z` **encodés** des densités
1/4, 1/2 et pleine sont 11 522, 8 573 et 0 unités de 1 mm. Le dernier
minimum appartient à l'unique ID original `122516`, absent des deux
échantillons réduits. Il modifie donc l'étendue réellement occupée, bien
que le secteur fixé par les plans du capteur reste identique.

[`ablate_outlier_122516.py`](ablate_outlier_122516.py) retire exactement
ce site des 14 829 points du quart plein, sans changer l'ordre des 14 828
autres. Il valide les SHA-256 des points et IDs v8 puis le SHA-256 de
l'entrée produite (`40aa7bd17d429316493a276b6738041a20bd0d41b3f22840f7618cfb36669923`).
La nouvelle étendue `z` commence à 8 573. Le même binaire Release figé
`e1ba126f…`, K10/s8/W8, `--static=8 --grid=1mm`, fournit sa
[sortie brute](OUTLIER_ABLATION.json) (`complete_relative`, SHA-256 du JSON
`61656746dd6cb700cab7165bd462213c4db467e9f5d9c2416c6631d10134410e`).
Commande après génération de l'entrée :

```text
nice -n 19 mhgp9_tower_probe.e1ba126f FULL_MINUS_ONE.u32le 10 8 --s=8 --static=8 --grid=1mm
```

Les formes comptées **hors extrémités** par `dead_core_form_sites` passent
de **579 000 541** à **578 914 981**
(`−0,0148 %`), et la pente 1/2→pleine recalculée avec les effectifs réels
de **2,042542** à **2,042528**. Les paires développées passent de
9 801 104 à 9 797 635 ; les visites du cover du cœur de 672 387 377
à 672 136 385. La chute de l'étendue par cet ID **n'explique donc pas**
la pente des formes sur ce quart. Cela ne contrôle ni les autres valeurs
extrêmes, ni une autre graine, ni la variabilité des chronos sur l'hôte
partagé. Le digest change naturellement avec le nuage de 14 828 sites.
