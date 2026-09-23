# LiDAR brut 08/000000 : croissance en densité de la tour K10

23 septembre 2026. Reçu exploratoire **clos à trois tailles de la scène
entière**, complément de la [matrice K5 à sept secteurs physiques et trois
densités](../lidar_raw_physical_scaling_20260923/README.md). Aucun demi ni
quart K10 n'est mesuré ici : pour cet axe spatial, utiliser la matrice K5
existante. Le nuage brut conserve le sol, les 123 389 retours originaux et
les coordonnées de la grille entière isotrope 1 mm (u18) ; aucune pose ou
hypothèse d'alignement de passages n'entre dans la sélection.

Le manifeste d'entrée est **octet pour octet celui du reçu K5** (SHA-256
`6e64125abddbfcd589ba61cf747e493e455adec4beeaea81c2739ce98fe16095`).
`generate.py` du reçu K5 classe globalement les IDs originaux par
`splitmix64(ID XOR d1da73a520260923)` ; ses préfixes emboîtés retiennent
30 847 ⊂ 61 694 ⊂ 123 389 retours. Les coordonnées des sites retenus
restent inchangées et la translation entière reste celle du plein. Le
lecteur vérifie les SHA des trois entrées, leurs IDs, leur emboîtement et
leur identité avec les entrées K5.

Même binaire v12 que K5, SHA-256
`e1ba126fbcea8ad483ebff2265f446c18e90eaefe04bf20021e76cd0df09af80` ;
K10/s8, huit workers CPU, huit fils statiques, six leviers v12 activés,
`nice 19`, hôte local partagé. Trois sondes, une par taille, sans répétition.
Elles renvoient toutes `complete_relative` : les clés émises sont
recoupées, mais l'absence d'autres clés n'est pas certifiée. Préparation
Python hors chronos. `chain_cpu_s` est la somme CPU de la chaîne ; le mur
et `times_ms.q34` sont fortement sensibles à la contention de cet hôte.

| fraction | sites | CPU·s chaîne | formes cœur chargées | charges cœur | paires développées | boules catalogue | RSS pic GiB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 1/4 | 30 847 | 173,008 | 103 153 147 | 1 721 708 | 5 402 246 | 2 760 701 | 2,131 |
| 1/2 | 61 694 | 383,704 | 323 686 030 | 3 614 332 | 12 861 574 | 5 669 283 | 4,189 |
| entière | 123 389 | 905,514 | **1 238 630 455** | 7 811 827 | 37 868 819 | **11 387 391** | **8,219** |

Pentes finies `p=log(W_b/W_a)/log(n_b/n_a)`, calculées avec les effectifs
réels des entrées :

| travail | 1/4→1/2 | 1/2→entière |
| --- | ---: | ---: |
| formes cœur effectivement matérialisées | 1,650 | **1,936** |
| charges cœur | 1,070 | 1,112 |
| paires développées | 1,251 | 1,558 |
| CPU·s chaîne | 1,149 | 1,239 |
| boules du catalogue émis | 1,038 | 1,006 |

Sur **ces trois tailles**, K10 ne reproduit pas le franchissement
`p_formes=2,136` observé à K5 de la demi à la scène entière sur les
mêmes entrées. Cela ne rend pas K10 peu coûteux : à densité entière il
matérialise **2,245 fois** les formes de K5, **4,035 fois** les boules et
atteint **8,219 GiB** contre **1,931 GiB** de RSS. La tour K10 contient
15 887 830 nœuds cumulés dans le reçu. Le verrou industriel concerne à
la fois le traitement des formes par cœur, les sorties/catalogues et la
mémoire ; améliorer seulement le temps de dispatch des jobs ne lève pas
ces masses. Les pentes sous 2 du travail CPU et des formes K10 ne sont
aucune preuve de sous-quadraticité globale : une scène, une graine, trois
tailles finies, et la géométrie du sous-échantillon change.

Le lecteur recoupe statut/raison, entrée FNV, options et six leviers,
catalogue, dix ordres, digest de tour, stdout/stderr hachés, puis exige
que les comptes des cinq premiers ordres soient identiques à ceux du
reçu K5 pour chaque entrée identique. Cette égalité de **compteurs** ne
remplace pas une identité de clés ni une preuve des clés absentes. La
contrelecture indépendante des trois cas et de toutes les pentes a été
positive. Les temps q3/q4 muraux (notamment 1/4→1/2) subissent une forte
contention de la machine et ne doivent pas guider la conclusion de
complexité. Aucun GPU/G4, profil float32 natif, autre scène ou autre
séquence n'est mesuré ici.

Pour relire depuis les sources versionnées, régénérer d'abord les entrées :

```sh
python3 morsehgp3D_v9/audits/lidar_raw_physical_scaling_20260923/generate.py \
  --repo /workspaces/E-HGP --out /tmp/mhgp9-raw-k10-density-20260923
python3 morsehgp3D_v9/audits/lidar_raw_k10_density_20260923/run_and_check.py verify
cd morsehgp3D_v9/audits/lidar_raw_k10_density_20260923 && sha256sum -c SHA256SUMS
```

`run_and_check.py run` reproduit les trois sondes uniquement avec le
binaire épinglé exact ; toute tentative, y compris un échec, est conservée
avant arrêt. Le lecteur normal passe ; `python3 -O ... verify` refuse comme
prévu, car il ne peut exécuter les assertions de contrôle. Les fichiers
`CASES.jsonl`, `*.stdout`, `*.stderr`, `SUMMARY.json` et `SHA256SUMS` ferment
ce reçu. Les payloads d'entrée sont régénérables et ne sont pas dupliqués
dans `audits/`.
