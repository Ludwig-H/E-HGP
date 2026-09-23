# LiDAR : moitiés et quarts par les plans du capteur

23 septembre 2026. La consigne utilisateur est de diagnostiquer chaque scène
LiDAR entière, ses **deux moitiés** et ses **quatre quarts** découpés par les
plans verticaux `x=0` puis `y=0` du repère capteur. Ils contiennent
l'origine du capteur et l'axe `z` ; le premier est parallèle à `y`, le
second à `x`. Les sous-nuages emboîtés
8k/16k/32k du runner de pente sont des disques autour d'un centre médian :
ils ne remplacent pas ces découpes par plans, encore moins la trame entière.
Le masque sans sol est produit sur la trame entière avant les coupes. Faire
les mêmes diagnostics de découpe sur la trame brute entière et sur le
sous-nuage sans sol ; les reçus examinés ici ne contiennent que ce dernier.

## Ce qui est vérifié dans les trois reçus disponibles

Les sept fichiers de chaque `scene_XX_grid` du reçu v8
`lidar_ground_20260921/release/ground_fq64xq_6` utilisent les coordonnées
entières **après grille 1 mm**, translatées par une origine commune. Leur
manifest déclare `x=0`, `y=0`, sans sous-échantillonnage, et l'origine du
capteur encodée. J'ai lu les points et IDs de chaque fichier et vérifié :
chaque ID de morceau garde les coordonnées du `full`, les deux moitiés sont
disjointes et couvrent le `full`, les quatre quarts aussi, et chaque paire
de quarts reforme sa moitié. Un point de coordonnée entière égale à l'origine
encodée appartient au côté non négatif. Les effectifs sont :

| 08/ | Full | `x<0` / `x≥0` | quarts `x−y−` / `x−y+` / `x+y−` / `x+y+` |
| --- | ---: | ---: | ---: |
| 000000 | 39 885 | 24 591 / 15 294 | 11 536 / 13 055 / 8 225 / 7 069 |
| 000100 | 35 551 | 19 019 / 16 532 | 8 074 / 10 945 / 9 221 / 7 311 |
| 000200 | 45 845 | 25 730 / 20 115 | 16 262 / 9 468 / 14 829 / 5 286 |

Le [préparateur v8](../../morsehgp3D_v8/bench/prepare_lidar_precision.py) partitionne
les **sites représentés** suivant leur signe sur la grille, avant la
translation entière. Le manifest est explicite :
`equivalence_to_raw_sign_partition=false`. Ce n'est pas toujours la même
partition que le signe des coordonnées float32 originales du retour LiDAR.

J'ai recroisé les `raw_to_full.u32le` de la préparation 1 mm (SHA-256 du
manifest vérifié), les `full.original_site_ids.u32le`, les trois trames
SemanticKITTI float32 originales et les sites retenus. Après le masque sans
sol, **0/39 885**, **0/35 551** puis **1/45 845** sites changent de côté
entre signe brut et signe représenté. Dans 08/000200, l'ID original `61939`
a `x≈−0,0000909 m`, `y≈−13,9133911 m` ; la grille l'arrondit à `x=0`,
puis le place dans `x≥0,y<0`. Sur le signe float32 brut, il serait dans
`x<0,y<0` : les effectifs physiques deviendraient 25 731 / 20 114 pour
les moitiés, et 16 263 / 9 468 / 14 828 / 5 286 pour les quarts. Aucune
fusion de retours n'a eu lieu dans ces trois préparations ; ce déplacement
est seulement dû à l'arrondi. Les changements sur **tous** les retours
bruts avant retrait du sol sont 3, 1 et 4 respectivement.

## Convention à épingler pour les prochaines campagnes

Le découpage déjà archivé est exact **dans la géométrie quantifiée** et
respecte des plans passant par l'origine du capteur représentée. Pour un
diagnostic demandé sur le **LiDAR physique brut**, choisir les côtés avec
les mots float32 d'origine (`x<0`, `y<0`), puis conserver les IDs des retours
et le masque avant de quantifier chaque morceau selon **la même grille et
la même translation globales**. Nommer la convention dans le manifest et
publier le nombre de sites dont le côté change à l'arrondi ; ne pas
substituer silencieusement la coupe sur grille à la coupe brute. Le moteur
HGP reste exact sur le sous-nuage quantifié explicitement sélectionné.

Si une future scène fusionne des retours de côtés opposés en un seul site
quantifié, la partition des **retours bruts** reste disjointe mais les
ensembles de sites dédupliqués de chaque morceau peuvent se chevaucher.
Publier ces fusions et les deux correspondances plutôt que d'imposer une
propriété de disjonction des sites qui ne découle pas du brut. Pour des
captations distinctes sans recalage, chaque découpe utilise son propre
repère et son propre capteur ; aucune origine ou orientation commune n'est
présumée.
