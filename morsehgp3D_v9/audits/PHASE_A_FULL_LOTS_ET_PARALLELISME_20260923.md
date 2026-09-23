# Audit — ouvrir le parallélisme *dans* un niveau FULL, sans changer la tour

23 septembre 2026. Lecture du produit publié à `b22b3706` (source moteur
inchangée depuis `ec6d1b74`). Cadre : `reference_cpu`, grille u18 1 mm,
`not_claimed`. La [contrelecture de septembre 22](CONTRE_AUDIT_B_PARALLELISME_Q34_FULL_20260922.md)
posait déjà le principe du pré-niveau figé, mais décrivait l'ancien moteur
où les ordres K n'étaient pas encore parallèles. Cette note précise la
couture **actuelle** ; elle ne qualifie aucune nouvelle optimisation.

## Ce qui est et n'est pas séquentiel

`run_orders_parallel()` prépare les cibles statiques, construit les lots
de chaque K en parallèle entre K, attribue ensuite les IDs de populations
dans l'ordre canonique, puis calcule les images verticales entre K. Dans
[`order_lots()`](../src/tower/forest/full_ball_tower.hpp#L646-L668),
en revanche, les niveaux exacts d'un **même** K sont traités
successivement ; les appels `order_block()` d'un niveau puis
`order_lot()` sont séquentiels. Le tri des requêtes est déjà parallèle.
La partie difficile n'est donc pas « rendre Kruskal parallèle » en bloc,
mais identifier la frontière de lecture figée à l'intérieur de chaque
niveau, puis mesurer si les lots sont assez gros pour l'exploiter.

Pour un ordre K et un niveau exact L, figeons l'historique, les ancres
des boules de niveau strictement inférieur à L et la fonction racine
`ρ` **avant** le lot. Chaque bloc `b` de L peut alors calculer son
ensemble de racines `R_b` en lisant cet état. Les blocs d'un lot sont
reliés si leurs ensembles de racines ont un élément commun ; les
composantes connexes de ce graphe blocs–racines sont exactement les
groupes que `order_lot()` construit avec son DSU. Chaque groupe émet
une action, ses parents uniques triés et ses contributions. Un groupe
sans parents n'est valide aujourd'hui que s'il contient **un** bloc et
**une** contribution ; un groupe à parent unique sans contribution
reste inertiel.
Publier toutes les nouvelles ancres **après** la fermeture du lot évite
qu'un bloc lise un autre bloc du même niveau comme antécédent.

Cette indépendance mathématique ne rend pas le code actuel thread-safe :

- [`order_root()`](../src/tower/forest/full_ball_tower.hpp#L524-L532)
  comprime `o.compressed` par écriture. Une lecture
  immuable des chaînes sur l'état pré-lot donne la même racine, mais
  son coût et sa profondeur doivent être mesurés.
- [`order_block()`](../src/tower/forest/full_ball_tower.hpp#L545-L569)
  consomme `o.static_targets[o.static_cursor++]` pour
  chaque facette. Les cibles sont rangées selon l'ordinal de collecte
  séquentielle dans `prepare_static_order()`. Un `fetch_add` concurrent
  changerait l'association facette–cible : il faut attribuer à chaque
  bloc son intervalle d'ordinaux par préfixe du nombre **exact** de
  facettes, puis lire ces positions sans curseur partagé. Le premier
  prototype doit garder [`order_lot()`](../src/tower/forest/full_ball_tower.hpp#L572-L643)
  séquentiel : il teste cette seule couture avant de paralléliser DSU.
- `o.st` est modifié dans chaque bloc. Des compteurs privés puis une
  réduction vérifiée évitent la course ; les échecs et les statistiques
  partielles doivent garder un ordre de publication déterministe.
- Les IDs des nouveaux nœuds, les parents, les contributions et les
  ancres observables exigent le même ordre canonique qu'aujourd'hui :
  groupes triés par leur plus petit indice de bloc, parents triés,
  contributions dans l'ordre des blocs, allocation par préfixe, puis
  publication du lot entier. Un DSU concurrent arbitraire peut trouver
  les mêmes composantes tout en changeant les IDs ; cela ne suffit pas.
- La phase A lance déjà plusieurs ordres K. Lancer en plus un pool de
  workers par **chaque** lot pourrait sursouscrire massivement les 48 fils ;
  il faut un budget partagé et mesurer l'occupation réelle.

## Petite porte falsifiable avant une architecture GPU

Isoler **un vrai lot** avec plusieurs blocs et des racines partagées,
capturer l'état pré-lot, puis comparer chemin actuel et traitement batch
sur 1/4/8 workers et plusieurs ordres de lancement (puis W48 si la
granularité le justifie). Vérifier pour
chaque bloc ses facettes, cibles et racines ; pour chaque groupe ses
parents, contributions, cible, ancre et nouvel ID ; enfin comparer la
tour entière avec `same_payload()` de `tests/tower/full_ball_tower_gate.cpp`,
qui contrôle niveaux, parents, successeurs, populations, contributions
et images verticales, et non seulement un digest. Ajouter les portes
sanitizer/TSan et panne de worker avant activation. Un refus doit être
transactionnel et conserver une raison déterministe.

Sur R7b (paquet antérieur `8e8b83a3`, **CPU seul**), la meilleure trame
K10 donne **3,199 s** pour FULL et
**159 086** `grouped_lots` agrégés sur la tour, mais le reçu ne publie
ni `singleton_lots`, ni histogramme des tailles de lots, ni temps de
`order_block`/racines/DSU par K et niveau. Les **2,85 Gcycles** de blocs
et **1,80 Gcycles** de lots cités dans la coordination pour K10 viennent
d'un harnais hors dépôt sans sortie brute, répétitions ni reçu G4 :
diagnostic, pas mesure qualifiée. Avant de lancer un kernel par niveau,
publier nombre de niveaux, quantiles/max des blocs et racines par lot,
fraction des singletons, longueurs de chaînes et temps mur/CPU/RSS.
Si la majorité du temps porte sur des millions de lots minuscules,
une frontière GPU par niveau serait dominée par l'ordonnancement ; il
faudrait une autre formulation batched/offline, non prouvée ici.

Même un succès de cette porte ne traite ni l'expansion q3/q4 avant les
paires, ni les phases restantes de FULL, ni la complétude globale des
clés. Aucun gain sous-quadratique ou contrat 1 s/GPU n'est revendiqué.
