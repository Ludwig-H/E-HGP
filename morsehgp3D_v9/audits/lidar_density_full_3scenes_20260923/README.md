# Reçu d'audit exploratoire : densité des trames LiDAR sans sol

23 septembre 2026. Complément du [reçu par sept secteurs de
08/000200](../lidar_density_scene02_20260923/README.md) : trames entières
08/000000 et 08/000100, grille 1 mm, K5/K10, s8/W8. Même seed
`d1da73a520260923`, classement `splitmix64(ID original XOR seed)`,
sélections globales emboîtées de 1/4 puis 1/2 des sites retenus. Il y a
**huit nouvelles sondes** et quatre cas à densité entière repris du
[reçu v12](../../receipts/lidar_scaling_local_20260923/README.md).
Les trois trames de l'analyse restent de la **seule séquence 08**.

`generate_crossscene.py` et son module `generate.py` sont les scripts exacts
utilisés ; `MANIFEST.json` épingle les sources et les entrées produites.
Ils conservent les chemins locaux de cette capture : sur un autre hôte,
adapter ces chemins en vérifiant d'abord les SHA des sources.
`CASES.jsonl` conserve les huit sorties brutes nouvelles ; `SUMMARY.json`
conserve leurs valeurs et hashes, les quatre cas pleins du reçu v12 et les
pentes calculées avec les tailles réelles. Le binaire Release CPU du
commit `4530644b` a le SHA-256
`e1ba126fbcea8ad483ebff2265f446c18e90eaefe04bf20021e76cd0df09af80`.
Les nouvelles sondes passent `--static=8 --grid=1mm` ; le libellé historique
des JSON v12 est `grid=unspecified`, mais leurs entrées sont attestées par
les manifestes v8. Toutes les sorties sont `complete_relative`.

Hôte CPU partagé, une graine et une exécution par cas : les chronos sont
indicatifs. Voir la [lecture des deux axes de
croissance](../CROISSANCE_LIDAR_PLANS_ET_DENSITE_20260923.md) ; ces pentes
finies ne prouvent aucune borne générale ni contrat G4.
