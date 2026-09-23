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

La réduction des allocations par facette et la préparation parallèle
sont des directions plausibles pour l'aval. Leurs gains, mémoire et
statistiques de travail restent **non qualifiés** à ce stade, et ne
changent pas le verrou de génération q3/q4 ni la cible de 1 s de toute
la tour sur une trame SemanticKITTI entière.
