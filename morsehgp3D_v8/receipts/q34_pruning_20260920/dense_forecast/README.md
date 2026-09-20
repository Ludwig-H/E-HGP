# Pré-audit dense3D : certificats exécutés, scans seulement projetés

Auxiliaire du20 septembre2026, **hors des158 sources de qualification du
produit**. Capture close : [forecast_o1_hie63](forecast_o1_hie63/MANIFEST.json).
Les13 commandes réussissent : compilation temporaire, puis12 configurations
n=8k/16k/32k, K5/10, budgets32/64. GCP non utilisé.

## Ce qui a réellement été calculé

Le programme construit le nuage, son index et son cover, puis le pool spatial
du produit. Pour chacune des n−2 seeds connues de cette fixture, il appelle
la primitive exacte `Q34FamilyCertificate` et compte séparément les crédits
q3 et q4, saturés aux seuils K−1 et K−2. La boucle auxiliaire s'arrête pour
une seed seulement lorsque les deux voies sont certifiées rejetées.

Les inégalités scalaires de la fixture vérifient indépendamment que chaque
seed est aiguë, que l'arête fournie est strictement la plus longue, et que
tous les points appartiennent au cover. Les positions du pool sont contrôlées
contre `floor(i*m/C)` dans l'ordre spatial réel. Les12 configurations comptent
223976 constructions de certificats et6671878 tests de témoins appariés.

**Le générateur d'arêtes, les événements, leurs tris, les balayages q4 et les
candidats ne sont pas exécutés.** Il ne s'agit ni d'un test complet q3/q4,
ni d'un checksum de ses sorties, ni d'un temps de tour.

Dans le repli exact de la tranche25, toute famille q4 survivante lit les m
sites du cover pour construire ses événements. La quantité `m × S4`, où S4
est le nombre de familles q4 non rejetées, est donc un **minorant du travail
restant de ce repli**, calculé sans le lancer. Il exclut notamment les tris,
les autres voies et la collecte. Ce n'est pas un compteur de scans exécutés.

## Résultats : le résidu reste important

Les trois nombres de chaque cellule suivent8k /16k /32k ; M signifie un
million de lectures de sites projetées, non mesurées.

| Budget | K | Familles q4 survivantes | Minorant m×S4, en M | Ratios aux doublements |
|---:|---:|---|---|---|
|32|5|1448 /2680 /13142|11,584 /42,880 /420,544|×3,702 /×9,807|
|32|10|4195 /9940 /22815|33,560 /159,040 /730,080|×4,739 /×4,591|
|64|5|1141 /1685 /8064|9,128 /26,960 /258,048|×2,954 /×9,572|
|64|10|2035 /4647 /14862|16,280 /74,352 /475,584|×4,567 /×6,396|

Les dépassements de×4 sont conservés. Ils montrent que cette sélection de
témoins ne suffit pas à rendre ce régime favorable aux tailles examinées.
Ils ne prouvent **aucune loi asymptotique générale** et ne doivent pas être
extrapolés à d'autres nuages ou à des tailles supérieures.

Les temps de la boucle de certificats vont de3,079 à25,990ms ; ceux de
l'auxiliaire entier, préparation/validation/libération comprises, de6,117 à
38,020ms. Ils excluent compilation et sérialisation JSON. Ce sont des temps
locaux auxiliaires sur hôte partagé, sans comparaison de performance au
calcul complet et sans promesse GPU.

## Géométrie et limites de la série

L'arête est a=(900,1000,1000), b=(1100,1000,1000). Les seeds proviennent de
la grille x=980..1020, y=1120..1140, z=1000±(40..58), soit32718 positions
possibles. Toutes sont distinctes et admissibles ; le cover contient tout
le nuage, donc m=n.

L'ordre est déterministe : x varie le plus vite, puis y, puis les couches
z alternant signe positif/négatif et amplitude croissante. Les nuages sont
des **préfixes emboîtés** de cette grille, avec une dernière couche partielle.
La croissance ajoute donc des couches et modifie progressivement la zone
occupée ; ce n'est ni une homothétie, ni un uniforme aléatoire, ni une
densification homogène d'un domaine fixe. L'index et les quantiles spatiaux
sont reconstruits à chaque n. La sélection n'est pas supposée monotone.

## Provenance et relecture

[dense_forecast.cpp](dense_forecast.cpp) réutilise explicitement les helpers
de sérialisation/validation de la sonde cover ; son ancien point d'entrée,
renommé, n'est jamais exécuté. [run_dense_forecast.py](run_dense_forecast.py)
compile uniquement cet auxiliaire dans un répertoire temporaire, contre
`build/v8_q34_pruning_20260920/libmhgp8_p0.a`, sans reconstruire ni modifier
le build épinglé.

La capture conserve commandes, sorties brutes, environnement sélectionné,
commit/worktree, compilateur et cache. Les158 sources produit plus les deux
sources auxiliaires sont empreintées avant/après ; idem pour bibliothèque,
cache et compilateur. Le binaire temporaire est empreinté après compilation,
avant/après chaque exécution et à la fermeture. Les échecs sont conservés.

[READBACK.json](READBACK.json) ferme six commandes : lecteurs normal/−O,
avec/sans vérification des fichiers vivants, et autotests du lecteur dans
les deux modes. Résultats identiques ;14 corruptions de reçus rejetées par
mode. Ces autotests vérifient le lecteur, pas la géométrie. Aucun changement
des160 sources ni des15 fichiers de capture pendant ces lectures.

```sh
python3 morsehgp3D_v8/receipts/q34_pruning_20260920/dense_forecast/run_dense_forecast.py read morsehgp3D_v8/receipts/q34_pruning_20260920/dense_forecast/forecast_o1_hie63 --check-live
python3 -O morsehgp3D_v8/receipts/q34_pruning_20260920/dense_forecast/run_dense_forecast.py selftest morsehgp3D_v8/receipts/q34_pruning_20260920/dense_forecast/forecast_o1_hie63
```

Empreinte du manifeste :
`2d2dcc7ae76a410973e0a0de865f947f37d1589a1bc6e08c825a2c6c2b95f8eb`.
Empreinte de READBACK :
`4b0cc6e8616754fa15558ae0d3b71b6301ea77d1434b2bee4db1978b07d0ee2b`.
