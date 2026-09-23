# Sans sol, quart physique chaud : croissance selon trois graines globales

23 septembre 2026. Ce reçu prolonge la [matrice de densité 08/000200](../lidar_density_scene02_20260923/README.md) sur le quart `x≥0,y<0` de SemanticKITTI 08/000200, où les formes réellement calculées par le cœur v12 avaient une pente locale K10 supérieure à 2. La [contre-épreuve de frontière](../lidar_scene02_physical_cut_20260923/README.md) fixe ici **les signes float32 physiques du capteur** : le quart plein contient **14 828 sites**, contre 14 829 dans le quart encodé 1 mm historique. Un retour brut voisin de `x=0` change de côté ; il était absent aux densités 1/4 et 1/2 de la graine historique. Aucun des trois calculs de pente ci-dessous ne mélange les deux définitions du plein.

Le masque Patchwork++ de la **trame entière** est figé avant la sélection : 125 526 retours bruts, 45 845 sites conservés (états non-ground ou unknown), 79 681 retirés. Le lecteur joint les profils grille et float32 par leurs deux bijections `raw_to_original`, vérifie leurs ensembles de retours conservés et le masque, puis classe **globalement les 45 845 sites admissibles** par `splitmix64(ID_original_grille XOR graine)`. Les premiers `floor(n/4)` et `floor(n/2)` forment des ensembles emboîtés ; le quart physique est intersecté ensuite, sans reranking sectoriel. Les points transmis au moteur gardent leurs octets u18 de la grille 1 mm **de la scène entière**, leur ordre et leur origine commune. Les SHA des IDs de sites, des retours bruts et des entrées sont dans [MANIFEST.json](MANIFEST.json). La graine historique `d1da73a520260923` est reproduite octet pour octet aux deux densités réduites ; les nouvelles sont `7d1c9a5eb3f24680` et `2b85d41e0c93a76f`.

La sonde est le binaire CPU v12 Release figé `e1ba126fbcea8ad483ebff2265f446c18e90eaefe04bf20021e76cd0df09af80`, K10/s8/W8/static8, six leviers de référence actifs, `nice 19`. Les quatre nouveaux calculs utilisent `--grid=1mm` ; le plein physique antérieur indique `grid=unspecified` dans son JSON mais son entrée vérifiée contient exactement les mêmes octets de grille 1 mm. Le paramètre de grille est un libellé de la sonde. Le plein physique est relu depuis son [reçu scellé](../lidar_scene02_physical_cut_20260923/README.md) ; il n'est pas recalculé.

`p=log(W_b/W_a)/log(n_b/n_a)` emploie **les effectifs réels du quart**. `core_sites` compte les formes calculées et écrites lors du chargement des cœurs, extrémités incluses ; `dead_core_form_sites` les exclut du compteur malgré leur calcul. Ce reçu donne donc en priorité le coût `core_sites` :

| Graine | Sites 1/4 → 1/2 → plein | Formes `core_sites` (M) | `p_core` 1/4→1/2 / 1/2→plein | `p_paires` | `p_CPU` |
| --- | ---: | ---: | ---: | ---: | ---: |
| Historique | 3 630 → 7 339 → 14 828 | 32,985 → 139,309 → 582,997 | **2,046 / 2,035** | 1,700 / 1,742 | 1,426 / 1,372 |
| `s1` | 3 609 → 7 387 → 14 828 | 34,673 → 153,448 → 582,997 | **2,077** / 1,916 | 1,609 / 1,725 | 1,379 / 1,383 |
| `s2` | 3 717 → 7 459 → 14 828 | 34,833 → 142,728 → 582,997 | **2,025 / 2,048** | 1,694 / 1,727 | 1,368 / 1,410 |

Le premier doublement franchit 2 dans les trois graines ; le second dans deux sur trois. La pente locale change donc avec le sous-échantillon, mais l'alerte sur la matérialisation des formes ne dépend pas uniquement de la graine initiale. Les paires développées restent sous 2 sur les six liens. Les CPU·s mesurés du flux entier restent sous 2 également, sans annuler ce travail payé ni prouver un temps de tour industriel. La fréquence de ces quarts chauds dans d'autres trames ou séquences, et le comportement au-delà de 14,8 k sites dans ce secteur, restent ouverts. Trois graines choisies et deux doublements finis ne sont pas une preuve asymptotique ni un test statistique de tous les LiDAR.

Les chronos HGP **excluent** la segmentation, le chargement des entrées v8 et la préparation Python des décimations. Le reçu v8 du masque entier 08/000200 mesure séparément **29,927 ms CPU local** lecture→masque (une capture de référence) ; ce chiffre n'est ajouté à aucune durée de ce reçu. Les nouveaux calculs K10 ont pris 27,735/74,463 CPU·s (`s1`) et 28,574/74,091 CPU·s (`s2`) pour les niveaux réduits ; le plein scellé indique 195,177 CPU·s. Les temps sur hôte partagé sont descriptifs ; ni FULL certifié, ni GPU/G4, ni borne sous-quadratique globale n'en découlent.

[CASES.jsonl](CASES.jsonl) garde les commandes, dates, SHA avant/après du binaire et des entrées, issues et chronos externes ; les quatre stdout/stderr bruts sont conservés. Le lecteur reconstitue les entrées depuis les sources v8 versionnées, contrôle l'identité des sorties, le FNV, les options, les dix ordres, les comptes internes du catalogue, `core_sites=dead_core_form_sites+2×dead_core_loads`, puis recalcule [SUMMARY.json](SUMMARY.json). Le statut est `complete_relative` : la cohérence interne du catalogue ne certifie pas les clés jamais proposées. Les empreintes du dossier sont dans [SHA256SUMS](SHA256SUMS).

Relecture sans relancer HGP, depuis la racine du dépôt :

```sh
python3 -B morsehgp3D_v9/audits/lidar_ground_hot_quarter_multiseed_20260923/run_and_check.py verify --out /tmp/mhgp9-ground-hot-quarter-reader
python3 -B -O morsehgp3D_v9/audits/lidar_ground_hot_quarter_multiseed_20260923/run_and_check.py verify --out /tmp/mhgp9-ground-hot-quarter-reader-O
(cd morsehgp3D_v9/audits/lidar_ground_hot_quarter_multiseed_20260923 && sha256sum -c SHA256SUMS)
```

Le mode `run` rejoue uniquement les quatre nouvelles sondes manquantes avec le binaire épinglé. Les fichiers d'entrée régénérables restent hors `audits/`.
