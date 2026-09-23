# Atlas q4 : arrêter à K−2 quand seule la voie q4 survit

23 septembre 2026. Lecture du moteur `0b29b6c3`. Proposition exacte pour
réduire du travail, sans gain temporel mesuré ni modification du moteur.

`Q4LocalAtlas::Impl::build` demande actuellement à `root_until`,
`child_until` et `refine_until` un certificat terminal à **K−1** dès que
`saturate_deep=true`, y compris pour une arête de masque **4** (q4 seule).
Après une partition *complète*, le même code jette déjà la cellule si son
compte strict est **≥K−2**. Le sweep q4 rejette aussi les événements à
`inside≥K−2` ; `q4_shallow` suit le même seuil. Le raccord
`Engine::edge` n'appelle `q3_edge` que si le bit 2 survit, et construit un
atlas q4 seul pour le masque 4 **avec le backend Local28**. Le backend
Window30 ne construit pas cet atlas. Voir `src/gen/lanes/q4_local.cpp`,
`q4_shallow.cpp` et `src/gen/pipeline/wspd_q34.cpp` au commit ci-dessus.

Pour un masque 4, un `Q4LocalPartitionResult` saturé à **K−2** sur une
cellule fermée C certifie au moins K−2 sites distincts strictement dans
toute boule q4 de centre dans C passant par l'arête propriétaire. Chaque
candidat de cette cellule dépasse donc le seuil admis `depth<K−2`.
L'atlas peut marquer C `Deep` et **détruire** le fragment partiel, sans
balayer les événements ni recueillir la coquille. Aucun compte partiel
n'est promu en compte exact. L'égalité `depth=K−2` est rejetée. Pour le
masque 6, garder K−1 est utile : le même atlas peut alors rejeter une
graine q3 à `depth≥K−1`. Un seuil K−2 sur ce masque resterait sûr avec le
census q3 actuel lorsque son minorant est insuffisant, mais pourrait
augmenter son coût ; ce n'est pas l'ablation proposée.

Une [fixture entière autonome](check_q4_lane_threshold_20260923.py) sépare
les deux seuils à K5. Prendre `a=(0,10,10)`, `b=(10,10,10)`, les gardes
`(5,10,10),(5,11,10),(5,10,11)`, un autre site `(5,9,6)` et les
complétions `x=(5,16,10), y=(5,10,16)`. L'arête ab est maximale dans le
support q4 positif. Dans la cellule dyadique locale
`u,v∈[1/8,1/4]`, les formes **4×puissance** des trois gardes sont
`−100`, `−96−40u`, `−96−40v` : elles sont strictement négatives sur
la cellule fermée. La quatrième forme `−32+40u+160v` change de signe.
Le support abxy a son centre à `u=v=11/60`, dans cette cellule,
avec coordonnées barycentriques strictes `(25,25,11,11)/72` et profondeur
exacte **3**. Le certificat K−2=3 peut terminer cette cellule ; celui à
K−1=4 ne peut pas la déclarer profonde sur le même centre. L'oracle
Python normal et `-O` vérifie ces signes rationnels, le profil u18, le
support et la profondeur. Il ne teste ni le front, ni le parcours natif,
ni une sortie FULL.

Le reçu v8 sans sol 1 mm, 08/000000/K5, compte 1 872 168 arêtes q4 et
1 545 198 arêtes mixtes : **326 970 arêtes q4 seules**, soit 17,5 %
des arêtes q4. C'est une part d'arêtes, **pas** une part des tests de
partition. Le registre actuel fusionne les travaux des deux rôles ; il
ne permet pas de calculer le gain possible. Avant de prioriser le port,
publier séparément par masque les cellules, bornes de blocs, tests de
points, copies d'IDs et temps d'atlas, ainsi que les certificats,
`unvisited_site_mass` et `discarded_frontier_ids` déjà disponibles dans
`Q4LocalSaturationWork`. `saturation_work.prefixes` reste un **sous-ensemble**
du travail physique `partition`, sans addition.

Le [reçu G4 R6](../receipts/g4_tower_r6_20260923/README.md) resserre
la priorité pratique : après les filtres et certificats de voies, les
trois trames sans sol ont seulement **6 166–8 338 arêtes q4 seules à K5**
et **23 208–33 131 à K10** (`q4_edges−both_edges`), soit **1,2–2,3 %**
des arêtes q4 survivantes selon scène/K. Les comptes sont identiques
dans les paires cœur ON/OFF. Ce n'est toujours **pas** une part des
cellules ni du temps d'atlas : une arête q4 seule peut coûter davantage
qu'une mixte. Mais le taux historique 17,5 % ci-dessus ne représente
plus le flux de la chaîne R6 ; instrumenter par masque avant ce port.

Port conseillé : rôle immuable `Q4Only` ou `Q3Q4` à la factory de l'atlas,
seuil `K−2` seulement pour le premier quand la saturation est activée,
et validation du certificat contre le seuil choisi. Une cellule `Deep`
n'expose qu'un **minorant** par `certified_inside_count`, jamais une
frontière complète. La porte native doit exercer la fixture avec
`requested_lane_mask=4`, comparer les présentations q4 exactes et la tour
FULL avant/après, puis mesurer les deux rôles sur les mêmes trames
entières à K5/K10. Un gain sur le seul nombre de certificats ou la seule
part d'arêtes ne vaut pas un gain de tour.

## Interaction avec le census q3 sur feuille porté

Au commit **`e54f727c`** (`tower_chain.cpp`, `wspd_q34.cpp`,
`q4_local.cpp`),
`q3_leaf_census=true` place `retain_q3_fragments=true` dans les options
de **toutes** les arêtes. Sur un masque 4, l'atlas Local28 retient alors
les fragments exacts des cellules `Deep` au compte K−2, mais aucun q3
n'est appelé et le sweep q4 saute ces cellules. Ces frontières retenues
n'ont donc **aucun consommateur**. Copier les options à la création de
l'atlas q4 seul et y mettre `retain_q3_fragments=false` préserve toutes
les sorties ; réserver la conservation aux atlas mixtes dont la voie q3
est effectivement active. Cela se combine avec le seuil K−2 ci-dessus,
sans le remplacer.

Pour juger le coût réel, distinguer les cellules `Deep` exactes retenues
et leurs octets, puis leurs consultations q3, des véritables feuilles
actives : le compteur agrégé `q3_leaf_censuses` ne révèle pas combien de
census sont sauvés par cette résidence. Une porte q4 seule, option de
conservation off/on, doit rendre les mêmes présentations et comptes
géométriques, avec octets retenus non croissants quand elle est désactivée.
Les chronos locaux WIP ne sont pas appariés sur toutes les options et ne
chiffrent pas un gain de RSS attribuable à cette seule correction.
