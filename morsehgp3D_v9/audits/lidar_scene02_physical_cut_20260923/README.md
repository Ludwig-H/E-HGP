# Un retour au plan du capteur : contre-épreuve K10 sur 08/000200 sans sol

23 septembre 2026. Le [diagnostic des découpes](../DECOUPES_CAPTEUR_LIDAR_BRUT_ET_GRILLE_20260923.md)
avait relevé **un** site sans sol dont le côté `x=0` change entre le signe
float32 physique et la grille entière 1 mm. Ce reçu audit-only mesure
l'effet de ce choix sur les **deux quarts `y<0` à pleine densité**, en
reprenant la même trame 08/000200, K10/s8/W8 et le même binaire v12 que
la [matrice de densité](../lidar_density_scene02_20260923/README.md).
Il ne change ni le moteur ni la segmentation et ne suppose aucun
alignement entre captations.

Le site `61939` est un **ID de site original dans le profil grille**, pas
un ID de retour brut commun aux profils. La jointure par les deux
`raw_to_original.u32le` bijectifs retrouve le retour brut **14826** ;
son ID de site original dans le profil float32 est **61942**. Ses
coordonnées float32 sont `x=−0,0000909044 m`, `y=−13,9133911 m` ; la
grille commune l'encode à `(80271,8311,13780)`, avec `x=80271` égal à
l'origine encodée. La coupe représentée le place donc en `x≥0,y<0` ;
la coupe physique en `x<0,y<0`. Tous les autres retours des deux
quarts gardent leur côté. L'indice `splitmix64` du site-grille 61939
(compté à partir de zéro) est
**34593/45845** : ce site n'est présent ni à 1/4 ni à 1/2 dans la
matrice sans sol, donc seule la pente du dernier lien pouvait changer.

Le [lecteur](verify.py) reconstruit les deux payloads u18 depuis la
**trame entière grille**, classifie les côtés avec les mots float32,
vérifie les SHA des manifestes et des deux correspondances, puis exige
que la différence avec les deux quarts v8 soit exactement ce retour.
La grille, la translation et les coordonnées de chaque site restent
communes au plein. Les [SHA et IDs des nouvelles entrées](MANIFEST.json),
les [deux sorties brutes](quarter_x_nonneg_y_neg.stdout), les
[tentatives](ATTEMPTS.jsonl) et les [comparaisons](RESULTS.json) sont
gardés ici ; les payloads régénérables restent hors `audits/`.

| Quart | Sites, grille → physique | `core_sites`, grille → physique | Boules émises, grille → physique | Pente `core_sites` 1/2→pleine, grille → physique |
| --- | ---: | ---: | ---: | ---: |
| `x<0,y<0` | 16 262 → 16 263 | 146 435 388 → 146 436 052 (+664) | 1 865 046 → 1 865 151 (+105) | 1,628587 → 1,628447 |
| `x≥0,y<0` | 14 829 → 14 828 | 583 000 415 → 582 997 435 (−2 980) | 1 639 809 → 1 639 642 (−167) | **2,035162 → 2,035350** |

Le franchissement de la pente 2 dans le quart chaud **subsiste** après
la coupe physique. Les digests de tour changent, normalement, car les
nuages des deux quarts changent ; les deux nouvelles sondes sont
`complete_relative`, ce qui ne certifie pas les clés jamais émises.
Leur `core_sites` vérifie `dead_core_form_sites + 2×dead_core_loads`.
Le coût CPU des deux nouveaux runs est 153,216 et 195,177 CPU·s ; leurs
murs chaîne de 41,903 et 189,387 s ont subi une contention très inégale.
Ce sont des chronos descriptifs, sans inférence de gain ni contrat G4.
La pente reste un diagnostic fini sur une trame d'une seule séquence,
pas une borne sous-quadratique ou superquadratique générale.

Le binaire Release est épinglé par SHA-256
`e1ba126fbcea8ad483ebff2265f446c18e90eaefe04bf20021e76cd0df09af80`.
La commande reprise du reçu historique est
`nice -n 19 mhgp9_tower_probe <quart.u32le> 10 8 --s=8` ; son ancien
libellé de sonde `grid=unspecified` ne change pas le fait que les octets
proviennent de la grille 1 mm. Le binaire et les entrées ont été hachés
avant/après chaque processus ; sortie, options, dix ordres, identité
des comptes de catalogue et pente sont relus. Les sorties de référence
grille viennent du [résumé de scène 02](../lidar_density_scene02_20260923/SUMMARY.json),
dont le SHA est fixé dans `RESULTS.json`.

Relecture depuis la racine du dépôt, avec les sources v8 versionnées :

```sh
python3 -B morsehgp3D_v9/audits/lidar_scene02_physical_cut_20260923/verify.py \
  --binary build/v9-open-worktree/build/v9-dev/mhgp9_tower_probe
python3 -B -O morsehgp3D_v9/audits/lidar_scene02_physical_cut_20260923/verify.py --offline
(cd morsehgp3D_v9/audits/lidar_scene02_physical_cut_20260923 && sha256sum -c SHA256SUMS)
```

`verify.py --out <dossier neuf>` régénère les deux entrées avec leurs
SHA, sans lancer HGP ; `--offline` relit les sorties et le reçu même si
les sources v8 ne sont plus montées. Le lecteur utilise des contrôles
explicites qui restent actifs sous Python `-O`.
