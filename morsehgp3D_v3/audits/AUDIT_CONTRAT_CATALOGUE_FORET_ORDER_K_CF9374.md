# Audit du contrat catalogue--forêt de `order_k`

> **Verdict : le contenu géométrique de la fixture positive ci-dessous est correct, mais sa sérialisation n'est pas canonique au sens déjà employé par les autres chemins.** `order_k_catalogue` trie d'abord par arité ; le catalogue v2, `anchored_catalogue` et la référence trient lexicographiquement par support. La même multifusion reçoit donc deux indices publics `ForestNode::source` différents. Le juge ne peut pas détecter cette dérive, car il définit la source minimale dans l'ordre fourni par le sujet. Ce delta est un **NO-GO pour la parité de payload et pour toute qualification prétendant couvrir les indices publics**, sans constituer une nouvelle réfutation du contenu mathématique de la forêt.

## 1. Snapshot et portée

Audit strictement en lecture du code ; le seul probe a été créé et compilé sous `/tmp`. Aucun fichier source, test, commit, branche ou état Git n'a été modifié par cet audit.

| fichier | SHA-256 audité |
|---|---|
| [`prototype/order_k_bfs.hpp`](../prototype/order_k_bfs.hpp) | `cf9374b64fdc6428625a1e8f72ecb6e19e6d66a80d3249361c694ea064c6d256` |
| [`oracle/oracle_main.cpp`](../oracle/oracle_main.cpp) | `927809a35e0356a29e81dc6ed23ee9363655a4b3e4af2d12974edb8fe3ce6078` |
| API [`mhgp.hpp`](../../morsehgp3D_v2/include/mhgp/mhgp.hpp) | `4c788f0a6d087859e8910cde0b3f32d4815ff69a0f8ab63a853d32cbb69e292a` |
| implémentation [`catalogue.cpp`](../../morsehgp3D_v2/src/catalogue.cpp) | `e2a51cafec00b28537d33774f9bc9971b6b273289d09899ef41959853146a21f` |
| implémentation [`forest.cpp`](../../morsehgp3D_v2/src/forest.cpp) | `140fcb3cdc2b037ce6dc0a6f589f2926de7b6e6afe9a9edad8b613b838a6cfe8` |

Le HEAD était `7fa39b1d8c9d3b566bcd098bb4bdd2dbc107d7af`, mais les deux premiers contenus étaient des deltas live ; les empreintes ci-dessus, et non le seul commit, figent le verdict.

L'[audit hostile général de l'oracle](AUDIT_ORACLE_389A742.md) avait déjà montré qu'un ordre de catalogue inversé puis une forêt reconstruite passaient le juge. Le finding nouveau est que le chemin `order_k` produit maintenant lui-même un ordre différent et des indices source différents sur une entrée positive minimale.

## 2. P1 actif, P0 avant publication : l'ordre dit « canonique » ne l'est pas

Dans `order_k_bfs.hpp`, le comparateur final commence par `n_support`, puis compare seulement les cases actives du support. Il produit donc toutes les arités 1, puis 2, puis 3, puis 4.

Les trois autres autorités de fait observées utilisent une autre clé :

- `catalogue.cpp` compare les quatre cases de `support`, dont les queues valent `-1` ;
- `anchored_catalogue.hpp` fait de même ;
- `reference_catalogue` trie les vecteurs de support par ordre lexicographique.

Avec des queues canoniques à `-1`, ces trois ordres coïncident : `{0}`, `{0,1}`, `{0,1,2}`, `{0,1,2,3}`, `{0,1,3}`, puis `{0,2}`, etc. Ils ne coïncident pas avec l'ordre par arité. Le commentaire « `Ordre canonique : par support trie, comme partout ailleurs` » décrit donc l'inverse de ce que fait son comparateur.

### Reproduction minimale

Le probe `/tmp/orderk_forest_contract_probe.cpp`, SHA-256 `360ae05f1f57cff0d02a9b4bd6a6b86205d57ff2531b2c15fd4b52eb2cfc9e9c`, a été compilé contre les cinq contenus figés ci-dessus. Il emploie le tétraèdre régulier entier :

```text
(2,2,2) (2,0,0) (0,2,0) (0,0,2)
```

À `s_max=4`, les deux générateurs rendent exactement les 15 supports critiques. La seule différence utile ici est l'ordre :

```text
order_k  : {0} {1} {2} {3} {0,1} ... {0,1,2} ... {0,1,2,3}
anchored : {0} {0,1} {0,1,2} {0,1,2,3} {0,1,3} ... {1} ...
contrôle ordre support complet : order_k=0, anchored=1
```

Le finding ne dépend donc ni d'un support manquant, ni d'une coquille, ni d'une dégénérescence : c'est une divergence pure de normalisation sur deux catalogues sémantiquement égaux.

## 3. P0 de contrat : `ForestNode::source` devient dépendant du générateur

Le contrat public v2 définit la source d'une multifusion comme la plus petite **par index de catalogue** parmi les sphères contributrices du lot. `build_forest` applique bien cette règle. Sur la fixture régulière, les six paires ont le même niveau et connectent les quatre naissances dans une unique multifusion d'ordre 1.

Résultat du même probe :

```text
merge_source_order_k=4
merge_source_anchored=1
```

Dans les deux cas, le contributeur sémantique minimal est le support `{0,1}`. Seul son index change : 4 après les quatre singletons dans l'ordre par arité, 1 dans l'ordre lexicographique canonique.

La topologie de la forêt n'est pas réfutée par cette fixture. En revanche :

- un digest, un sidecar, une comparaison byte-à-byte ou un consommateur persistant les indices voit deux résultats différents ;
- remplacer un générateur par l'autre n'est pas transparent pour l'API publique ;
- le mot « canonique » ne peut pas désigner simultanément les deux sérialisations.

La correction doit choisir une seule règle. La plus petite modification compatible avec les chemins existants consiste à remplir toutes les queues par `-1`, trier lexicographiquement les quatre cases, dédupliquer sur cette même clé, reconstruire le pool de membres dans cet ordre, puis seulement construire les forêts. Une alternative plus robuste serait de définir la source canonique par une clé sémantique stable, puis de publier son index dans un catalogue dont l'ordre est lui-même scellé.

## 4. P0 de qualification : l'oracle blanchit nécessairement cette divergence

Deux choix du juge composent le faux vert de représentation :

1. `compare_catalogues` transforme le sujet en `map` indexée par le support ; il compare les ensembles de records, jamais l'ordre de `Catalogue::spheres`.
2. Pour vérifier la source minimale d'une multifusion, `compare_forests` parcourt **le catalogue sujet** depuis l'index zéro et retient le premier contributeur admissible.

Le juge vérifie donc seulement : « la forêt respecte l'ordre arbitraire que son propre sujet vient de fournir ». Il ne vérifie pas : « le sujet respecte l'ordre canonique indépendant, puis publie l'indice attendu dans cet ordre ».

Ce comportement est acceptable pour un oracle explicitement limité à l'équivalence scientifique par ensembles. Il ne l'est pas pour une porte de payload canonique. Il faut séparer les deux statuts dans le reçu :

- `semantic_catalogue_equivalent` pour la comparaison actuelle ;
- `canonical_payload_equal` seulement après comparaison de l'ordre, des queues, du pool et des indices source.

Une fixture permanente fondée sur le tétraèdre ci-dessus doit exiger l'ordre exact des 15 supports et `source=1` pour la multifusion d'ordre 1. Une injection qui permute les sphères puis reconstruit une forêt cohérente doit rougir la seconde porte.

## 5. Champs publics de `Catalogue` : objet valide ou diagnostics absents, impossible à distinguer

Sur la même entrée positive, le probe observe :

```text
order_k diagnostics sizes = [0,0,0]
anchored diagnostics sizes = [4,4,4]
order_k degenerate_shells=0 shell_anomalies=0
```

Les trois tailles sont celles de `neighbourhood_size`, `growth_rounds` et `certified`. `order_k_catalogue` laisse également `candidate_pairs`, `candidate_triples` et `candidate_quads` à zéro, alors que son objet séparé `OrderKStatistics` comptabilise du travail de récolte. Aucun tag dans `Catalogue` ne distingue « non applicable », « non renseigné », « zéro observé » et « certifié ».

Le juge n'inspecte aucun de ces champs. Il peut donc qualifier la sémantique des sphères tout en laissant croire qu'il a qualifié l'objet public complet.

Ces diagnostics proviennent historiquement de l'algorithme v2 et ne doivent pas être artificiellement réinterprétés comme des voisinages `order_k`. Il faut soit un résultat variant/tagué avec diagnostics propres à chaque backend, soit un contrat explicite de valeurs absentes. Dans tous les cas, le statut de complétude et la cause de rejet doivent vivre dans l'objet public ou dans un résultat indissociable du catalogue ; un `Catalogue` nu ne doit pas être publiable comme succès simplement parce que ses champs par défaut valent zéro.

## 6. Sorties partielles : comportement courant favorable, porte absente

Le wrapper `order_k_catalogue` accumule d'abord ses records dans `kept` et `members_pool`, puis ne copie vers le `Catalogue` final qu'après la navigation et la récolte. Sur les rejets observés, il rend donc actuellement un catalogue vide : c'est le bon comportement transactionnel.

Deux détails doivent toutefois être scellés :

- `order_k_vertices` renvoie directement le préfixe visité lorsqu'il découvre tardivement une égalité de pinceau ; son appelant doit impérativement lire `out_of_domain` ;
- la branche hors domaine de l'oracle vérifie que les forêts sont supprimées, mais n'exige jamais que `catalogue.spheres` et `catalogue.members` soient vides.

Une recherche déterministe sur sept points de la grille `[0,7]` a trouvé au seed 20 :

```text
(5,7,6) (7,3,5) (5,1,4) (3,5,0) (7,3,2) (1,0,2) (6,6,3)
order_k_vertices : out_of_domain=1, sommets renvoyés=1, cocircular_pencil=1
order_k_catalogue : out_of_domain=1, spheres=0, members=0
statistiques du wrapper : emitted_arity[1]=7, vertices_visited=1
```

Le catalogue est bien atomique dans ce snapshot, mais le compteur nommé `emitted_arity` compte des records jamais publiés après l'abandon. Il doit être renommé en émission provisoire, ou séparé en `accepted_before_abort` et `published`.

La porte manquante est une injection `partial_catalogue_on_reject` : avec le drapeau hors domaine vrai et les forêts vides, toute sphère, tout membre ou tout statut public de succès résiduel doit faire échouer la campagne. Le contrat de `order_k_vertices` doit de son côté annoncer explicitement `partial_diagnostic`, ou vider son résultat en cas d'erreur s'il devient une API produit.

## 7. Normalisation locale vérifiée favorablement

Sur le contenu `cf9374…`, les points suivants sont corrects et ne doivent pas être perdus lors de la réparation :

- `emit` remplit toutes les cases inutilisées de `support[4]` avec `-1` ;
- les supports récoltés sont strictement croissants ;
- les singletons, paires, triangles et tétraèdres sont dédupliqués par leurs chemins respectifs dans la plage produit de 50 000 identifiants ;
- après le tri, le pool de membres est reconstruit et chaque `members_begin` pointe vers une tranche contiguë correcte.

Cette observation n'étend pas le domaine de `key4` au-delà de 65 535, limite déjà documentée dans l'[audit du BFS](AUDIT_ORDER_K_BFS_A8111F0.md). Elle dit seulement qu'aucun doublon ni queue invalide n'a été observé sur la fixture contractuelle présente.

## 8. Porte minimale proposée

Avant de qualifier `order_k` au-delà de l'équivalence scientifique bornée :

1. définir une clé canonique unique de support et l'appliquer à tous les générateurs ;
2. comparer dans l'oracle l'ordre exact, les queues `-1`, la partition compacte du pool et les champs publics applicables ;
3. construire et vérifier les sources de forêt contre cet ordre indépendant du sujet ;
4. ajouter la fixture régulière des 15 supports et les injections ordre, queue, pool et source ;
5. rendre le rejet transactionnel obligatoire et tester qu'aucun catalogue partiel ne survit ;
6. séparer dans les statuts l'équivalence sémantique, la canonicité du payload et la qualification du backend.

GCP non utilisé.
