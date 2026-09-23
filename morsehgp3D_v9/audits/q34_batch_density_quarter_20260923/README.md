# S2 v17 : densité d'un quart LiDAR brut, moteur et batch CPU appariés

23 septembre 2026. Diagnostic local de la source constructeur S2/v17
publiée à `a6d81f9ce`, construite en Release avec CUDA désactivé sur le
commit local `803216ba2` avant son réancrage. Le diff entre ces commits est
vide pour `morsehgp3D_v9/src/`, `CMakeLists.txt` et `bench/tower_probe.cpp` ;
les SHA source du reçu concordent. L'[audit des
sept secteurs et trois densités v12](../lidar_raw_physical_scaling_20260923/README.md)
avait repéré le quart physique `x≥0,y<0` de la trame brute 08/000000. Ce
rejeu reprend **exactement ses trois payloads u18/1 mm**, sans changer le
masque, les IDs ni la translation commune. Les plans passent par le capteur
dans les coordonnées float32 d'origine, avant quantification. Les trois
sélections globales par hash d'ID sont emboîtées ; le manifeste d'entrée a
le SHA-256 `6e64125abddbfcd589ba61cf747e493e455adec4beeaea81c2739ce98fe16095`.
Il n'y a aucune hypothèse d'alignement avec une autre captation.

Le binaire de sonde a le SHA-256
`076312fb9502dee5a7f0a5fbe549b68c942c511892b4c0ba5583770b511758d7`.
K5/s8/W8/static8, dix leviers ordinaires actifs ; seul
`q34_batch_filter` passe de 0 à 1, `q34_gpu_filter=0` dans les six cas.
Les commandes, hashes d'entrée/sortie, sorties brutes et compteurs sont
dans le [reçu](receipt.json), [rejugeable](verify.py). Les six sondes
retournent `complete_relative`. Pour chacune, **tous les ordres**, le
catalogue, le travail de tour, le générateur et le digest FULL sont égaux
entre moteur et batch CPU ; le lecteur vérifie aussi les comptes de cœur
et d'arêtes. Cette égalité relative ne prouve pas les clés jamais émises.

| Densité dans le même quart | Sites | Formes cœur calculées, les deux voies | Visites des paires témoins moteur / batch | CPU·s chaîne moteur / batch | RSS KiB moteur / batch |
| --- | ---: | ---: | ---: | ---: | ---: |
| 1/4 | 7 692 | 3 548 514 | 11 856 751 / 15 115 264 | 7,359 / 7,323 | 144 876 / 162 368 |
| 1/2 | 15 619 | 14 655 072 | 32 443 837 / 47 534 893 | 17,504 / 17,742 | 305 712 / 324 500 |
| Entière | 31 391 | 52 302 311 | 97 757 691 / 162 481 090 | 44,225 / 46,253 | 585 328 / 597 424 |

Les pentes finies des **formes réellement préparées** sont **2,002320**
puis **1,822627** entre les effectifs réels. Le batch n'a pas changé les
arêtes, les charges ni cette masse : à pleine densité, il effectue
**66,2 %** de visites de paires témoins de plus, car il supprime le cache
du moteur ; le CPU de chaîne est **4,6 %** plus élevé et le RSS **2,1 %**
plus élevé dans cet essai. C'est un coût de référence CPU, pas le temps
qu'aurait le filtre CUDA sur G4. La section batch du plein mesure
353,097 ms de front, 2 452,457 ms de filtre et 3 474,699 ms d'arêtes.
`q34_occupancy.cpu_sum_s` passe pourtant de 36,317 à 23,022 s parce que
sa phase de filtre batch n'est pas dans les intervalles de workers : ce
champ ne compare pas le CPU complet des deux voies.

La voie batch est donc **fonctionnellement cohérente sur ces trois cas**,
mais elle ne résout pas le traitement par cœur qui croît défavorablement
ici. La priorité pour la suite est de comparer la chaîne S2 complète sur
les mêmes 21 entrées (sept secteurs × trois densités), puis K10, avec les
coûts front, filtre, transferts, aval, FULL et mémoire. Le
[`run_lidar_scaling.py` v17](../../bench/run_lidar_scaling.py) ne fait pas
cette expérience : ses 8k/16k/32k sont des disques croissants et ses sept
secteurs n'ont que la densité entière ; il force les leviers batch/GPU à
`false`. Une seule répétition sur hôte CPU partagé, un quart et une trame
d'une seule séquence ne qualifient ni le GPU/G4, ni le contrat industriel,
ni une borne asymptotique sous-quadratique.

Relecture des sorties archivées depuis les sources versionnées :

```sh
python3 morsehgp3D_v9/audits/lidar_raw_physical_scaling_20260923/generate.py \
  --repo /workspaces/E-HGP --out /tmp/mhgp9-v17-density-inputs
python3 morsehgp3D_v9/audits/q34_batch_density_quarter_20260923/verify.py \
  --inputs /tmp/mhgp9-v17-density-inputs \
  --source build/v9-open-worktree/morsehgp3D_v9 \
  --probe /tmp/mhgp9-v17-batch-density-replay-1533/build/mhgp9_tower_probe
```

`--source` et `--probe` sont facultatifs si ce build temporaire n'existe
plus ; les sorties restent liées aux SHA source/binaire du reçu et aux
octets d'entrée régénérés. Le lecteur fonctionne également sous
`python3 -O`. Le replay de calcul complet demanderait de reconstruire le
commit `a6d81f9ce` et d'exécuter les six commandes inscrites au reçu.
