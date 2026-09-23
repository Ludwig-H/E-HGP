# R15 G4 : gain S4a réel, budget de tour et croissance encore ouverts

23 septembre 2026. Contrelecture en lecture seule du
[reçu R15](../receipts/g4_tower_r15_20260923/README.md), publié par
`7ceadffad` sur le paquet source `8b47a75a9`. Les 326 entrées de
`SHA256SUMS` passent ; `SUMMARY.json` et `vm/receipt.json` donnent
18/18 cas `complete_relative`, 12 cas GPU, 12 comparaisons de sorties
égales et aucun cas batch non apparié. Les six condensés épinglés du
catalogue et de la tour sont reproduits avec et sans S4a. Le préflight
réduit reporte 3 404 arêtes de certificats et 11 976 voies q3, avec la
même sortie. C'est une preuve expérimentale positive du raccord sur
ces entrées, pas une preuve de complétude des clés jamais proposées.

Les paires S3/S4a **répétées et entrelacées dans la même session G4** sur
08/000000 mesurent, en secondes de `chain_s` :

| Tour | S3 GPU seul | S3 + S4a GPU | Gain S4a |
| --- | ---: | ---: | ---: |
| K5, première paire | 1,988 | 1,811 | 0,177 |
| K5, deuxième paire | 1,970 | 1,807 | 0,163 |
| K10, première paire | 7,095 | 6,721 | 0,374 |
| K10, deuxième paire | 7,120 | 6,711 | 0,409 |

Les `lanes_asked` sont toutes décidées sur les huit exécutions GPU S4a
normales, qui couvrent six combinaisons scène/K :
aucun report par capacité n'y masque ce gain. Sur les deux mesures
08/000000/K5, l'appel q3 prend 132–152 ms, dont 54–56 ms d'appareil
(28 ms de noyau, 26–27 ms de transfert) et environ 4–5 ms de phase de
traîne ; l'attente publiée est presque nulle. À K10, l'appareil q3
prend 202–204 ms, dont 92–98 ms de noyau et 105–112 ms de transfert.
Ces transferts sont aujourd'hui cachés derrière le CPU q4 ; leur coût
devra être réexaminé si q4 migre lui aussi sur GPU. `lanes_census_point_tests`
reste un compte logique, non le nombre physique de ballots par graine.

Les meilleures chaînes S4a des trois trames sans sol de la **seule
séquence 08** sont **1,446 / 1,811 / 1,858 s à K5** et
**5,242 / 6,721 / 6,693 s à K10** (000100/000000/000200). Le budget de
1 s n'est atteint sur aucune. À K5, soustraire de chaque chaîne S4a le
temps `q34_batch.edges_ms` mesuré donne respectivement
**1,072 / 1,269 / 1,350 s** ; à K10, **3,396 / 4,281 / 4,214 s**.
C'est un **reste arithmétique conditionnel** si les autres postes restent
fixes et si toute la phase des arêtes CPU devenait gratuite. La migration
q4 seule ne suffit donc pas aux budgets observés ; accélérer cette phase
peut aussi dévoiler l'attente q3 aujourd'hui recouverte. Les phases de
tour, de validation, de préparation et de filtre doivent être mesurées
et réduites avec S4b. Cette soustraction ne prédit pas le temps d'une
future implémentation qui changerait ces postes.

R15 traite trois trames entières **sans sol**, sur grille optionnelle
1 mm/u18. Il ne mesure ni trame brute avec sol, ni float32 original, ni
demi-scène, ni quart, ni densité 1/2 ou 1/4, ni autre séquence. Il ne
permet donc aucune pente de croissance pour S4a. La
[matrice locale antérieure](CROISSANCE_LIDAR_PLANS_ET_DENSITE_20260923.md)
couvre trois trames sans sol × sept secteurs physiques × trois densités
emboîtées × K5/K10, mais provient de v12 CPU : ses pentes de formes
ne sont pas des chronos ou bornes R15. Dans cette matrice, dix des
84 liens de densité atteignent `p_core_sites≥2`. Le
[quart physique sans sol `x≥0,y<0` de 08/000200/K10](lidar_ground_hot_quarter_multiseed_20260923/README.md)
est un sentinelle utile : son premier doublement donne une pente
`core_sites>2` avec trois graines globales sur trois. Le réexécuter avec
S4a, ses deux densités réduites et le plein, puis étendre aux deux
moitiés, quatre quarts et trames entières aux mêmes IDs, testerait
l'effet du port sur ce verrou. Publier par cas les digests appariés,
le temps de chaîne, les formes du cœur, graines×cover par arête,
ballots physiques, validations d'index, mémoire/arène, reports et temps
de traîne. La coupe se fait sur les signes float32 physiques du capteur
avant la grille ; le masque sans sol est figé sur la trame entière avant
les coupes et décimations.

Deux [gardes de réception S4a](AUDIT_S4_WIP_EXCEPTIONS_WORKERS_20260923.md)
restent constructivement ouvertes : les reports d'arène par défaut ne
sont pas impossibles sous 65 536 sites, et le jumeau moteur du
préflight doit garder ses planchers positifs de census q3 de feuille et
de cache. R15 lui-même possède ces deux comptes positifs
(82 056 / 321 903) et ne présente aucun report normal ; ces lacunes
ne réfutent pas son gain mesuré. Aucun contrat FULL, sous-quadratique
global, 1 s ou 100 ms n'en est déduit.
