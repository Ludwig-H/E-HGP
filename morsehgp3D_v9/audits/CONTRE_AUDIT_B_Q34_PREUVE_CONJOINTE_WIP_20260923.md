# Contre-audit B — preuve q3/q4 conjointe en chantier

23 septembre 2026. Lecture **WIP non commitée** du worktree développeur
sur `e0ae05a7` : `q34_dead_lanes.cpp` SHA-256 `5c9aa56272af…`,
`q34_dead_lanes.hpp` `290c808b70ca…`, `wspd_q34.cpp`
`5e1dce8540a2…`. Aucun test G4 ni gain de cette version n'est acquis.
Les modifications v6 concomitantes sont étrangères à cet audit.

## Exactitude et portes manquantes

`Engine::edge` n'appelle la preuve qu'avec les masques `2`, `4` ou `6`.
La nouvelle récursion conserve, pour chaque cellule fermée, les voies
certifiées hors disque ou par suffisamment d'intérieurs uniformes ; une
voie réfutée dans un seul enfant est retirée pour le parent. Le disque
q3 est inclus dans celui de q4 et les seuils restent distincts,
`T3=K−1>T4=K−2`. À la lecture, **aucune fausse exclusion n'est trouvée**.
Ce verdict n'est pas encore une qualification de la nouvelle combinaison.

Les cinq mutants préexistants ont été adaptés à la nouvelle signature,
mais ne forcent pas les transitions propres au partage. Ajouter des
fixtures q3-seule, q4-seule et deux voies, dont (i) q4 certifiée avant q3,
(ii) q3 réfutée tandis que q4 reste ouverte, à K=3/5/10. Comparer le
flux complet — BallKeys, profondeurs, coquilles et IDs — aux deux preuves
séparées et à l'oracle de petits nuages, pas seulement un condensé.
Un mutant `q4_disk_factor=8` et un mutant d'intersection des masques
doivent être tués causalement. Une campagne G4 appariée ne vient
qu'après ces portes et un reçu figé.

## Le test ponctuel au coin semble redondant

Le double incrément apparent de `failed_cells` aux lignes 176 et 181
**n'est pas un défaut produit démontré** : le chemin de réfutation
ponctuelle paraît inatteignable depuis la racine standard. Voici le
lemme qui permet de le tester ou de supprimer ce coût :

- Chaque disque de voie est strictement intérieur à la racine
  `[-2,2]²` : la borne de Gram donne `|u|²≤3/2<4` pour q4, moins
  encore pour q3. Si le coin sud-ouest `p` d'une cellule dyadique est
  dans le disque, les deux indices de grille de `p` sont donc positifs.
- La cellule voisine immédiatement au sud-ouest, à la même profondeur,
  contient `p` dans son coin nord-est **fermé** et précède la cellule
  actuelle dans l'ordre de parcours `SW,SE,NW,NE` (ordre Morton).
- Cette région antérieure ne peut être rejetée « hors disque » puisque
  `p` appartient au disque. Si la voie est encore active aujourd'hui,
  la région antérieure a été entièrement certifiée, directement ou par
  ses descendants/ancêtres : elle donne au moins `T` sites strictement
  intérieurs à `p`. Sinon la voie aurait déjà été abandonnée.

Par conséquent, pour une voie encore active, `count<T` au test ponctuel
du coin ne devrait jamais arriver. Une contre-recherche exploratoire
**non archivée** sur 117 000 petits nuages aléatoires (K=3/4/5,
profondeurs 2 à 4) n'en a pas trouvé ; elle ne remplace pas le
raisonnement ni une porte de régression. Les comptes R3
`dead_point_tests` sont **non nuls** mais ne
séparent pas les réfutations ponctuelles des échecs à profondeur maximale.
Ne pas déclarer le double incrément comme bug. Tester la suppression du
scan par comparaison différentielle exacte et un compteur de réfutations
avant de l'intégrer. S'il est conservé, la voie q4 seule peut arrêter son
scan à `T4`, alors que le WIP continue jusqu'à `T3` : surcoût sûr.

Cette piste est secondaire : sur 08/000000/K10 R3, 73,213 M tests
ponctuels contre 25,315 **milliards** de tests uniformes.

## Coût pré-cover : verrou prioritaire

Le partage intervient **après** `Q34EdgeCover::make` et `dead_.load`.
Sur le reçu CPU G4 R3 sans sol u18/1 mm, les trois trames de la même
séquence 08 donnent, selon K5/K10, 11,96–32,79 M paires développées,
1,73–4,93 M covers, 0,295–1,251 Md visites de nœuds cover et
1,790–9,281 Md formes chargées. Le cas 08/000000/K10 donne précisément
30 777 213 paires, 4 507 278 covers, 1 151 764 266 visites de nœuds,
7 796 411 934 formes et 25 314 524 281 tests uniformes. La
**construction** du cover coûte en visites de nœuds ; c'est `load` qui
énumère `Σ|cover_e|` sites. La nouvelle preuve conjointe peut réduire
les tests de cellule d'un facteur, pas ce chargement ni l'expansion.
Des scènes différentes à une seule taille ne mesurent pas un exposant
8k/16k/32k et n'établissent aucune borne sous-quadratique.

Expérience suivante, en priorité : avant de construire le cover, tenter
en *shadow* un certificat adaptatif par nœud spatial `Z` et cellule
fermée de centres `C`. La puissance `F(z,u)` est convexe en `z` et
affine en `u`, donc son **maximum** exact sur la boîte de `Z` et `C`
se trouve parmi 8×4 coins. Si les 32 valeurs sont `<0`, les sites du
nœud, disjoint des autres nœuds comptés et des supports `a,b`, sont
uniformément intérieurs à tout centre de `C` ; les
seuils demeurent `K−1`/`K−2`. Les nœuds non certifiés gardent la voie
exacte. Voir [la proposition détaillée](PISTE_B_Q34_NOEUDS_AVANT_COVER_20260923.md).
Comparer d'abord sans changer les sorties, puis activer uniquement les
rejets prouvés ; apparier W1/W8 sur les coupes capteur 8k/16k/32k de la
**même** trame et du même masque 1 mm sans sol. Publier toutes les masses
ci-dessus, les tests de nœuds/cellules, les sorties exactes et les temps
CPU/mur/RSS. Ce certificat reste par arête après expansion : il ne
promet pas à lui seul une borne sous-quadratique globale.

## Publication du port

Le parcours conjoint est publié sur `main` sous `7f64a279`. La lecture
mathématique ci-dessus ne change pas de verdict. Un différentiel natif
indépendant de l'auditeur A sur **30 000 appels synthétiques** trouve les
mêmes bits de preuve que les deux parcours séparés et 18,7 % de tests
uniformes en moins ; voir
[son analyse](DOMINATION_Q4_PARESSEUSE_PAR_BLOCS_20260923.md).
Ce pourcentage porte sur les tests de cellules de ces fixtures, ni sur
la chaîne LiDAR entière ni sur `dead_.load`; aucun exposant
sous-quadratique ni contrat G4 n'en découle.
