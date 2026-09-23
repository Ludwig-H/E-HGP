# Contre-audit B — préparation FULL parallèle en cours

23 septembre 2026, lecture **WIP non commitée** du worktree développeur
sur `b4e480fc` : `src/tower/forest/full_ball_tower.hpp` SHA-256
`d6373da4…`, `src/tower/parallel/pool.hpp` `ef67e1ee…`. Aucun GCP ni test
lourd déclenché par cet audit ; ne pas attribuer ce diff aux mesures R1/R2
ou à la campagne v5 en cours. Les modifications sales de v6 sont étrangères
à cette lecture.

Le diff prépare les forêts des K ordres en parallèle depuis une banque
partagée immuable, puis les valide et les publie dans l'ordre K. Les
écritures portent sur des cases `forests[i]`/`drafts[i]` disjointes. Le tri
des requêtes a un ordre total `(clé, ordinal)` ; ses runs et plages de
fusion sont disjoints. Les facettes temporaires deviennent des `span`
consommés et copiés pendant l'appel synchrone. **Aucune erreur
géométrique ni course évidente n'a été trouvée par lecture**, ce qui ne
remplace ni une porte dynamique ni une comparaison de tours.

Trois preuves manquent avant de parler de gain industriel :

1. `parallel_sort` alloue `buffer(n)` en plus du tableau des requêtes. Le
   compteur `static_peak_request_bytes` n'enregistre que la capacité du
   premier, et `static_peak_retained_bytes` est échantillonné **après** le
   tri. Au pic, ces deux tableaux peuvent coexister, presque doublant ce
   poste. Publier RSS et capacité des deux buffers, y compris les
   réallocations transitoires ; ne pas présenter le compteur actuel comme
   une enveloppe mémoire.
2. Les forêts K construisent simultanément leurs nœuds et conservent les
   résultats jusqu'à la publication séquentielle. Le pic peut monter même
   si le temps mur baisse. Publier nombre de forêts actives, volumes de
   drafts/libérations et pic RSS ; comparer W1/W8 et W48 sur **même tour**.
3. Les fixtures statiques existantes ont au plus huit points. Elles ne
   franchissent jamais le seuil `n/4096≥2` de `parallel_sort` (au moins
   8 192 requêtes), donc leur égalité W1/W4 n'exerce pas la nouvelle voie
   de tri. Ajouter une fixture ou un test dédié qui l'active réellement,
   compare la permutation entière à `std::sort` sous plusieurs nombres de
   fils, tue une mutation d'ordre/intervalle, puis un test de tour FULL
   avec assez de requêtes, idéalement sous sanitizer/TSan.
4. Le reçu G4 R3 (snapshot **antérieur** à ce WIP) publie pour
   08/000000/K10 **17,389 M représentants**, **11,309 M appels MEB** et
   **358,911 M tests de puissance**, mais aucun `static_requests` par K ni
   chrono séparé du tri, de la résolution des groupes et de l'encodage
   des forêts. Le poste FULL total est 22,8 s ; on ne peut pas lui
   attribuer une part au tri. Exposer ces sous-temps et masses avant de
   privilégier cette optimisation face au travail géométrique.

La réduction des allocations par facette et la préparation parallèle
sont des directions plausibles pour l'aval. Leurs gains, mémoire et
statistiques de travail restent **non qualifiés** à ce stade, et ne
changent pas le verrou de génération q3/q4 ni la cible de 1 s de toute
la tour sur une trame SemanticKITTI entière.

## Nouvelles portes WIP après cette lecture

Le développeur a ajouté `parallel_sort_gate.cpp` SHA-256 `6708723b…`
et `chain_static_paths_gate.cpp` `0901b7b8…` sans encore committer
ces octets (`pool.hpp` `aa0b780b…`, `full_ball_tower.hpp` `fb8b2c63…`
après évolution du WIP). Rejeu indépendant local par CTest ciblé dans
`build/v9-dev` : **3/3 PASS**. Le tri exerce **400 cas**, dont **160**
réellement multi-ouvriers et **52** plans à nombre impair de runs, passent
en 2,2 s ; le mutant compilé `COPY_PAIRS` échoue avec
`cause=parallel_sort.permutation n=8192`. La porte de chaîne passe en
3,7 s : `max_static_requests=100407`, `workers_created=16`, et même
condensé FULL pour résolveur temporel puis statique 1/4/8 fils. Trois
rejeux indépendants de cette porte gardent le digest
`73490cf88c02af30`. Le seuil
de 8 192 requêtes est donc réellement franchi. Le compteur
`workers_created` mesure toutefois les groupes de résolution, **pas**
les workers du tri ; la participation au tri découle ici de l'appel
`parallel_sort` et du seuil, et non d'une mesure publiée de ses fils.

Ces portes corrigent la lacune de déclenchement du point 3 historique.
Elles restent des comparaisons différentielles et un condensé FNV-64 sur
une seule fixture 1 500/K5/s8, non une mesure RSS/TSan ou une décomposition
des 22,8 s FULL G4 R3. Les obligations des points 1, 2 et 4 restent
ouvertes. Un premier `kCompleteRelative` erroné dans le test nouveau a
été corrigé en WIP avant ce rejeu ; ne pas attribuer l'échec initial aux
sources finales.
Le digest couvre les contributions utilisées, mais pas le domaine de la
banque ni ses lignes non référencées ; comparer directement les objets
de sortie sur cette fixture, ou leur sérialisation canonique complète,
serait une porte d'identité plus forte. `workers_created=16` est une
**somme** de voies de résolution, non 16 fils simultanés ni un compteur
du tri. Aucun TSan n'est encore dans cette porte ou le workflow CI.

## Relecture du commit publié `e0ae05a7`

Cette section remplace le statut « WIP » des deux sources précédentes :
`full_ball_tower.hpp` SHA-256 `fb8b2c630f29…` et `parallel/pool.hpp`
`aa0b780b4918…` sont effectivement dans le commit. Rejeu local de la
sélection CTest `gate` sur le build développeur existant : **109 PASS,
1 DISABLED** (110 tests dénombrés), sans reconstruction indépendante de ce
build ni sanitizer de cette sélection. Les trois portes ciblées ont aussi
passé ; le mutant de fusion `COPY_PAIRS` est tué par une permutation fausse
à 8 192 éléments. Les 400 cas de tri couvrent les budgets de fils
`0,1,2,3,4,5,7,8,16,48` : ils ne couvrent **pas** chaque entier de 1 à 48.

La lecture indépendante n'a révélé ni inversion de l'ordre total des
requêtes, ni course évidente sur les plages de fusion et cases de forêt,
ni changement de priorité du premier refus logique : les validations
par blocs conservent une réduction ordonnée des échecs. Le filtre de
niveaux flottant ne sert que sous `FE_TONEAREST`, avec repli exact et
départage par rang. Sur entrée malformée, des compteurs peuvent inclure du
travail lancé dans des blocs ultérieurs avant le premier refus ; ne pas
leur donner une sémantique de « préfixe validé ». Une porte FULL dédiée
aux autres modes FENV et à deux défauts situés dans des blocs distincts
renforcerait cette preuve.

Le double buffer des requêtes est maintenant compté dans
`static_peak_request_bytes`, mais **pas** tous les tableaux coexistants :
collecte par blocs et requêtes concaténées, tri des graines et forêts K
simultanées ne composent pas un pic global. Un diagnostic local de
l'auditeur A observe 11 245 584 octets pour le double buffer contre
7 584 268 dans `static_peak_retained_bytes` échantillonné après tri ; ce
dernier ne doit pas être lu comme un RSS. Une allocation du buffer de
fusion peut aussi échouer alors qu'un tri en place réussirait : qualifier
un repli exact ou un plafond mémoire propre au régime 30 M, sans tronquer
la tour. Le gate de chaîne assure une égalité de condensé FNV-64 et du
nombre d'ordres sur 1 500 sites/K5/s8, non l'égalité sérialisée de toutes
les sorties, ni une trame complète/G4. Aucun chrono apparié, RSS G4,
TSan ou contrat de 1 s n'est acquis par ce commit.

## Nouveau WIP après `e0ae05a7` : tampons des lots

Dans le worktree développeur, `full_ball_tower.hpp` remplace le vecteur
local de `Block` par `lot_blocks` conservé entre lots et donne directement
le vecteur des racines d'un singleton à son action via `swap`. Lecture
indépendante : tous les champs du bloc sont réinitialisés avant usage, le
`span` reste synchrone et les racines ne sont plus lues après le transfert ;
aucun défaut d'exactitude ou de durée de vie identifié dans ce diff.

L'affirmation « no allocation per lot » du commentaire est toutefois trop
forte : après le `swap` d'un lot singleton, le slot n'a plus de capacité
de racines ; le prochain singleton peut la réallouer. Le changement
économise surtout l'enveloppe `vector<Block>` et réutilise les tampons des
lots groupés, sans changer la complexité asymptotique. `lot_blocks`
conserve aussi jusqu'à la fin de la construction la capacité atteinte par
le plus grand plateau ; la comparer au RSS maximal, surtout sur 30 M
d'objets. Aucun nouveau test n'accompagnait ce diff au moment de la
lecture : refaire la porte FULL et le différentiel statique 0/1/4/8 sur
un build cohérent avant d'attribuer un gain.

## WIP ultérieur : lots simultanés par ordre K

Le développeur a ensuite ouvert `run_orders_parallel` dans
`full_ball_tower.hpp` : cibles statiques préparées par K, lots/histoires
privés construits en parallèle, IDs de populations attribués ensuite
dans l'ordre canonique, images verticales calculées après fermeture des
histoires, puis forêts encodées. Lecture du diff mouvant du 23 septembre :
`current_k` ne change que pendant la préparation séquentielle ; les
workers de lots emploient `o.k` et des états privés, ceux des images
lisent des histoires inférieures figées. Les cibles sont consommées dans
l'ordre des facettes, avec contrôle final. Aucun défaut de géométrie ou
course démontré sur une entrée valide à cette étape **WIP**.

Trois obligations avant qualification :

1. `parallel_items` relance la première exception **arrivée**, pas celle
   du plus petit K. Deux ordres invalides peuvent donc publier des
   `reason`/`status` différents d'un lancement à l'autre, alors que la
   boucle antérieure était ordonnée. Conserver des slots d'erreur par K
   et publier le premier indice, ou déclarer et tester le nouveau contrat
   fail-closed ; ne pas appeler cela « mêmes statuts » sans porte causale.
2. Les `static_targets`, `anchors` et historiques de **tous** les K
   restent simultanément résidents pendant certaines phases. Le
   compteur `static_peak_retained_bytes`, pris par préparation, ne les
   enveloppe pas. Publier pic RSS et octets co-résidents, notamment pour
   les régimes de dizaines de millions de points. Le parallélisme des
   lots est plafonné à K tâches (dix au plus à K10) : mesurer les temps
   par ordre et la fraction réellement occupée des 48 CPU G4.
3. La nouvelle voie encode ses ancres/nœuds en `u32` et refuse à
   `UINT32_MAX` nœuds par ordre, alors que la voie séquentielle porte
   `u64`. C'est un refus explicite, pas une troncature, mais le domaine
   massif admissible est plus étroit ; tester le seuil ou justifier la
   borne de sortie requise.

Le nouveau `full_coverage_certificate.hpp` ajoute une banque de
populations déplacée avec validation parallèle. Les lectures de lignes
sont disjointes, le domaine est immuable et la banque n'est publiée
qu'après validation. **Défaut d'API reproduit** : `parallel_ranges` est
appelé hors du `try` de cette surcharge publique. Avec le hook
`MHGP9_TESTING` et `launch_fail_after=1`, l'appel à deux lignes lève
`std::system_error` (`active=0` après jointure), au lieu de retourner
`kResourceExhausted` comme la surcharge copiante. L'exception est
capturée si l'appel passe par `build_full_ball_tower`, mais pas par l'API
publique directe. Un `std::bad_alloc` pendant la préparation des threads
peut suivre la même voie ; `std::length_error` n'est pas non plus pris
dans le `try` de la surcharge déplacée. Ajouter un gate de surcharge
move mono/multi, entrées malformées, échec de lancement et allocation,
puis rendre les statuts cohérents. Les portes actuelles de banque ne
couvrent que l'ancienne surcharge.

Ces deux sources sont désormais publiées au commit **`684d8fc7`**
(`full_ball_tower.hpp` SHA-256 `89f1f96a…`,
`full_coverage_certificate.hpp` `08033ed0…`) sans nouveau fichier de
test dans ce commit. Le message annonce des digests K10 inchangés sur
trois trames et un temps local W8 de tour 26→18 s sur 000100 ; ce sont
des indications développeur, **pas encore un reçu G4 apparié**, ni une
réponse au cas causal d'échec de lancement de la surcharge publique.
La prochaine R5 devra mesurer la tour complète, ses phases par K et le
pic RSS sur le snapshot exact, avec arrêt G4 certifié.
