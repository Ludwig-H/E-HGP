# A livré : dépendances et durée de vie des tampons

Lecture du commit **bcd742c0517f38b33ef85e212c94414377998157**, le 8 octobre 2026.
**Aucune dépendance manquante ni utilisation après libération trouvée dans le périmètre ci-dessous.**
Conclusion statique et modèle abstrait borné, sans compilation, moteur, sanitizer, CUDA ou GCP ; aucune
qualification de la première exécution de A ni fermeture de constat. Les sources sont les blobs Git épinglés
dans [capture.json](capture.json), pas le worktree produit évolutif.

Ce complément précise la [prélecture des corps A](../prelecture_t2d_corps/README.md) : la porte native
`tests/tower/pipeline_unit.cpp:153–207` possède déjà une table de lectures indépendante et contrôle la fermeture
transitive de `StepDeps`. Nous ne la rejouons pas. L'apport ici est la confrontation aux **ressources concrètes**,
aux lecteurs simultanés et à leur libération, plus un modèle distinct de publication des feuilles. Le
[problème de fenêtre diagnostique](../t2d_a_fenetre_patch/README.md) et la
[comparaison causale proposée](../t2d_a_comparaison/README.md) restent des sujets séparés.

## Historique et contraction : concurrence permise

Après `kKernel`, `forest_kernel.cpp:171–201` ne lit pour l'historique que `births`, `attach_parent`,
`event_count` et les `OrderWork::events`, tous figés. Il construit **uniquement** `survivor_events` et écrit
`counters.max_attach_depth`. M construit `event_node/event_rank`, la forme de l'arbre, puis `cell_node` ;
`finish_order` écrit `counters.classes`. Ces deux compteurs sont des scalaires distincts, sans bitfield :
être membres du même `ForestWork` n'en fait pas une même adresse mémoire concurrente.

`finish_order` rend `local/class_min/keys/child_count/cell_top`, que l'historique ne lit pas.
Les événements bruts sont rendus **seulement** par `kRows`, après **kFinish ET kHistory**
(`pipeline_run.cpp:30,343–349`). Les événements persistants de `OrderForest` ne sont pas ce tampon :
`event_cell`, `event_node`, `event_rank`, attaches et CSR des survivants restent possédés jusqu'au résultat.
V et R peuvent donc les consulter après `release_events`. Relecture statique indépendante de cette séparation
par l'auditeur `latest_perf_e`, sans exécution supplémentaire.

Les phases M ont leurs barrières globales : classes → allocation des nœuds → nœuds → parents → placement →
enfants → fin. Les tranches ne coupent pas un plateau (`slice_end`). Dans la forêt d'unions construite par T,
chaque opérande externe reçoit exactement un parent ; les écritures de `parent[child]` des tranches sont donc
disjointes. Classes, nœuds, comptes et CSR d'enfants appartiennent aux intervalles de leur tranche. Ce dernier
argument utilise les invariants du noyau ; il ne certifie pas une injection arbitraire d'événements corrompus.

## V et R : les lectures croisées sont couvertes

| Consommateur | Ressources nécessaires | Prédécesseurs effectifs |
|---|---|---|
| V-naissances(k) | `birth_node(k)` ; forme et `cell_node(k−1)` ; `lower(k)` alloué | `kLower` attend M(k) et M(k−1), puis `kBirths` |
| V-fusions(k) | `lower` des naissances(k), rang/minleaf(k), forme et historique(k−1) | V-naissances(k), historique(k−1) ; M(k−1) transitivement |
| R-collecte(k) | cibles G, birth/cell_node(k), forme et historique(k) | `kRows` attend M(k) et historique(k) |
| R-remplissage(k) | comptes complets, offsets et sortie réservée | collecte complète → placement → remplissage |

Les verticales écrivent seulement `lower(k)`, par intervalles disjoints. R écrit ses tableaux de lignes/branches,
sans lire `lower`. V(k+1) lit la forme/histoire de k, **pas** ses verticales ou son registre : attendre V(k) ou
R(k) ajouterait une barrière sans besoin d'ownership dans ces corps. Les entrées, le Cloud et le catalogue restent
possédés par l'appel ; les spans `ForestInput` sont établis après allocation/remplissage structurel de G.
`kCheck` ne lit pas les valeurs de `targets`, encore en production, mais seulement leur taille et les tableaux
déjà figés. Les cibles sont lues plus tard, par tranches publiées.

## Feuilles : un auteur, puis un lecteur, puis la libération

Le calcul G écrit uniquement les cibles de sa tranche. Si la numérotation est déjà publiée, son worker prépare
aussi ces feuilles, puis publie `kSliceDone|kSliceLeaves` avec release. Sinon il publie sans ce dernier bit ;
le noyau, après acquire, prépare les feuilles lui-même. Une tranche n'a donc pas deux auteurs concurrents.
Le noyau est exclusif par ordre (`kStepBusy`) et consomme les tranches dans l'ordre. Son préchargement s'arrête
à `rep_offsets[end_cell]`, même si des tranches ultérieures ont déjà commencé ailleurs.

Le reset de `work.leaves` n'arrive que lorsque `kernel_slice == g_items`, après toutes ces consommations.
Un worker G qui a publié n'accède plus aux feuilles ; son éventuelle fin de tâche/époque restante n'empêche pas
ce reset. Les spans consultés simultanément portent un objet Buffer inchangé : le reset de ses métadonnées est
postérieur à tous ces lecteurs. Les buffers `cells/element` sont privés au noyau et rendus à sa clôture.

Cas limites : sans cellule, aucune pré-passe n'accède aux feuilles et le noyau clôt directement. Sans événement
(naissance unique), les phases M à zéro morceau sont terminées par propagation de dépendances ; M produit la
racine feuille et l'historique une CSR vide. Sans ligne R, `kFill` est terminé sans appeler `close_rows` : les
offsets de travail restants vivent jusqu'à la destruction de SessionRun, sans lecteur tardif ni fuite hors appel.
Sur refus, une étape ne publie pas sa réussite : ses descendants restent bloqués ; les tâches déjà réclamées
terminent avant le retour de la région Pool et le nettoyage RAII. Ceci décrit le chemin, sans nouvelle injection.

## Publication et vérification bornée

Les champs non atomiques sont publiés avant les drapeaux release. Pour les phases par morceaux, les incréments
`finished.fetch_add(acq_rel)` relaient les écritures jusqu'au dernier morceau ; les compteurs de dépendances
`pending.fetch_sub(acq_rel)` les transmettent au successeur qui observe zéro avec acquire. Les emplacements
de compteurs privés `(ordre,worker)` sont agrégés après la jonction du Pool. Ce raisonnement suppose le contrat
du Pool : un même identifiant worker n'exécute pas simultanément deux callbacks.

[check.py](check.py) vérifie les hashes Git, lit les dépendances déclarées et confronte leur fermeture à
**1 578 obligations de ressources pour K=1..12**. Un automate indépendant explore **399 états** pour zéro à
trois tranches, couvrant les deux auteurs possibles, toutes leurs publications, consommation ordonnée et reset.
**Sept mutations de modèle** sont détectées : disparition de chacune de quatre dépendances de durée de vie,
libération après la première tranche, publication avant pré-passe, préchargement hors tranche prête.
Normal et `−O` donnent le même [résultat](results.json), sans `assert` comme garde.

```sh
python check.py --repo /workspaces/E-HGP
python -O check.py --repo /workspaces/E-HGP
```

Le modèle ne simule ni chaque instruction C++, ni les caches, ni toutes les exécutions atomiques, ni le protocole
de terminaison `in_flight/epoch`. Il ne prouve pas l'équivalence géométrique globale, la formule d'admission,
les performances, les refus sous toutes les pannes ou la qualification GPU. Aucun correctif produit n'est proposé
par cette lecture favorable ; les portes natives de déterminisme, poison, fautes et concurrence restent nécessaires.
