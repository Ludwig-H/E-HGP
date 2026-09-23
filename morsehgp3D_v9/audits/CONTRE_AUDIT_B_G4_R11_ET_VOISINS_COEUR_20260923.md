# Contrelecture B — R11 G4 et shadow des voisins du cœur

23 septembre 2026. Réception du [reçu R11](../receipts/g4_tower_r11_20260923/README.md)
publié en `ed11c6c3` et lecture du
[shadow voisins](../receipts/knn_core_probe_20260923/README.md) du même
commit. B n'a ni démarré GCP ni modifié les moteurs. Cadre R11 :
**CPU** sur G4 SPOT, grille entière 1 mm, trames **sans sol** d'une seule
séquence 08, s8, W48, tour K1..5 ou K1..10, `complete_relative`.
Ce n'est ni GPU ni contrat de trame brute multi-séquence.

## R11 : amélioration réelle de l'attente q2, pas du travail q3/q4

Les 388 fichiers du manifeste SHA passent `sha256sum -c`. Le paquet
`f685461a` est la sonde v16, et le reçu hôte/worker est `completed` :
24/24 cas sont `complete_relative`, 18 comparaisons d'objet logique
égales et tous les condensés de tour ON/OFF identiques. Ces 18
comparaisons relient les quatre cas de chacun des six groupes à une
référence ; ce ne sont pas 18 essais indépendants ni une comparaison
octet par octet de tout le payload FULL. La preuve d'arrêt ciblé GCE
`TERMINATED` est archivée. Le validateur du protocole courant accepte
la capture ; aucun GPU n'a exécuté la chaîne.

| trame sans sol | meilleure chaîne K5 ON | meilleure chaîne K10 ON | q2 OFF→ON, moyenne des deux répétitions K5 | q2 OFF→ON, K10 |
| --- | ---: | ---: | ---: | ---: |
| 08/000100, 35 551 sites | 2,537 s | 7,682 s | 235→74 ms | 413→145 ms |
| 08/000000, 39 885 sites | 3,210 s | 10,364 s | 450→103 ms | 756→201 ms |
| 08/000200, 45 845 sites | 3,517 s | 10,526 s | 496→113 ms | 815→217 ms |

Les ratios q2 des **moyennes appariées** vont de **2,84× à 4,38×**.
Le README R11 et la coordination annoncent « 3,3× à 4,5× », hors
de cette plage : 000100/K10 donne seulement 2,84× (et K5 3,16×).
La chaîne entière gagne en moyenne 0,135 à 0,576 s selon le cas,
toujours sous comparaison ON/OFF du **même paquet R11**. Le levier
change **à la fois** l'ordre par masse et le grain 16→64 jobs/worker :
R11 ne sépare pas leurs effets. Il ne réduit pas le nombre de candidats
q3/q4. Le JSON ne publie pas le temps de partition q2 ni la distribution
des durées par job ; l'attribution fine au plus long job reste inférée
de l'essai local non épinglé. La porte locale vérifie la neutralité des
sorties/travail sur des petits cas, mais pas la granularité 64 de
production ([contrelecture v16](CONTRE_AUDIT_B_Q2_MASS_FIRST_V16_20260923.md)).

Le meilleur K5 reste à **2,537 s de chaîne**, **2,772 s de mur externe**
avec lecture et condensé ; q3/q4 prend 1,663 s et FULL 0,598 s.
Le meilleur K10 reste à 7,682 s de chaîne, 8,687 s externe, dont
q3/q4 4,459 s et FULL 2,379 s. Les écarts R10→R11 ne peuvent pas
être attribués au seul q2 : le paquet R11 incorpore aussi des
modifications de la tour publiées après la capture R10. L'ablation
**interne R11**, elle, isole le levier q2. Aucun nouveau test de pente
8k/16k/32k, s10/s12, brut, multi-séquence ou GPU n'est acquis.

## Shadow voisins : proxy sûr mais non transférable en l'état

Le shadow 16k/08/000000/W8 choisit les 17 sites les plus proches de
chaque extrémité **dans le cœur déjà construit**. Ils conservent
96 % des fermetures du cœur à K5 et 86 % à K10, sur une seule coupe ;
ceci mesure une force de preuve relative, non un temps ou un gain de
production. Le patch construit et parcourt le cœur, trie ses sites
pour chaque arête, puis calcule **aussi** la preuve du cœur entier :
les comptes 19,9/257,6 M et 54,0/597,5 M sont des populations logiques
de sites, pas des accès supprimés par l'exécution shadow.

Le produit envisagé pré-calculerait au contraire les voisins
**globaux** de chaque site avant de construire le cœur. Ces listes
ne sont pas les listes mesurées. Contre-exemple entier :
`a=(0,0,0)`, `b=(100,0,0)` ; 17 points `(0,j,0)` près de `a`, 17
points `(100,j,0)` près de `b`, `j=1..17`, tous hors boule diamétrale ;
dix points `(40+j,0,0)`, `j=0..9`, sont strictement intérieurs à
**toute** sphère par `a,b`. Les 17 voisins globaux de chaque extrémité
(hors `a,b`) préfèrent les points extérieurs, alors que les voisins
du cœur contiennent les dix témoins universels. La promesse de
96 %/86 % ne se transfère donc pas au chemin proposé sans cœur.

La prochaine ablation doit choisir les listes globales effectivement
réutilisables, mesurer leur construction, leur preuve **avant** le
cœur et le repli complet, puis facturer formes/covers/atlas, CPU, mur
et RSS sur 8k/16k/32k. Elle doit comparer toutes les sorties exactes.
Le shadow actuel n'a pas de journal brut de chronos ni d'effet aval ;
son estimation « 4 à 8 % du CPU q3/q4 » n'est pas un gain mesuré.
