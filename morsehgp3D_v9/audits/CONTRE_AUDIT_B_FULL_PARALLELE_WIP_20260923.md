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
