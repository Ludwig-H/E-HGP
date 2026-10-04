# Supports et liaisons à un plateau d'ordre K

Reçu privé, 4 octobre 2026. Source Git figée : `ee2b48b4c306f7da403657fb5291aa78569e6296`.
Lecture seule du produit ; aucun natif, build, fit ou cloud. Les trois copies Git sont sous `source/`.
Le rapport `workflow_lecture_foret_k.md` est une capture WIP de conception, pas du produit qualifié ; ses empreintes avant/après sont dans `BEFORE.json` et `AFTER.json`.

## Résultat borné reproductible

```sh
python3 -B -S check.py > normal.json
python3 -B -O -S check.py > optimized.json
```

Les deux sorties sont identiques : `PASS`, 139 gardes, exactement 28 MEB (21 K-parties et 7 cofaces) et 24 permutations de quatre cellules. Le solveur Gram/Fraction est autonome ; aucun oracle produit importé. `EXPECTED.json` fixe le résumé attendu. Les gardes utilisent des exceptions et restent actives sous `-O`.

À K=5, les sites sont les quatre sommets `(10,10,10)`, `(10,-10,-10)`, `(-10,10,-10)`, `(-10,-10,10)` du tétraèdre régulier et les trois intérieurs `(0,0,0)`, `(1,0,0)`, `(0,1,0)`.
Les six parties « trois intérieurs + deux sommets » naissent à β=200, en six composantes strictes distinctes. Les quatre cofaces « trois intérieurs + trois sommets » apparaissent toutes à β=800/3. Elles fusionnent ces six anciennes composantes en un seul plateau fermé, qui contient 18 K-parties : six anciennes et douze nouvelles. Le graphe d'intersection fermé possède 60 arêtes.

Pour CHAQUE boule de face : p=3, m=q_min=3 ; son support est le triangle de face et le centre a les trois poids 1/3. Elle porte six K-parties de sa population. Trois sont les traces comprimées strictes `I ∪ A`, et trois omettent un intérieur et contiennent toute U ; ces trois dernières naissent à β=800/3.

Le point précis à corriger dans la lecture WIP §3 est donc : `C(m,K-p)` ne compte pas toutes les K-parties de `P_b`, mais seulement les parties comprimées contenant TOUT I. Le nombre total est `C(p+m,K)`. En particulier `C(m,K-p) − strict_traces` ne compte pas les nouveaux sommets de Γ_K : il vaut zéro ici, alors qu'il y en a trois par boule. Le produit actuel ne commet pas cette confusion ; elle concerne les futurs champs du format.

Chaque boule touche exactement trois composantes globales avant le plateau. Pourtant l'ordonnanceur DSU effectue successivement 2, 2, 1 puis 0 unions. Chacune des quatre boules peut produire zéro ou deux unions selon l'ordre de traitement. Le propriétaire fermé final est le même dans les 24 permutations. Un rôle « boucle/fusion » dérivé des seules unions effectuées par une cellule serait donc un artefact de l'ordonnanceur.

## Contrat de transfert proposé

Pour une boule b dont la fenêtre `[p+q_min-1,p+m]` contient K, toutes les K-parties de P_b ont MEB au plus λ_b. Si `|P_b| > K`, les échanges Johnson entre ces parties sont également actifs au seuil fermé : leur union contient au plus P_b et reste dans la boule. Elles ont donc un unique propriétaire dans Γ_K(λ_b). Le cas `|P_b|=K` n'a qu'une partie. C'est le raccord qui permet d'affecter tous les Q∈Q_b à UN nœud de la forêt K, après fermeture complète du plateau. Il ne requiert pas de laminariser les points.

Chaque Q positif affinement indépendant détermine une seule boule : M1 donne MEB(Q)=b. Les atomes `(b,Q)` peuvent donc être affectés une fois au nœud vivant à λ_b. Les listes propres de nœuds partitionnent ces atomes, et leurs unions sur les sous-arbres sont emboîtées. Leurs réalisations géométriques peuvent se recouvrir entre branches ; ce n'est pas une erreur de forêt. Le squelette de JETON n'est pas le K-polyèdre de la définition 21 de la thèse, lequel est l'ensemble des sites apparaissant dans une composante de Γ_K (imprimée58 ; théorème2 imprimées60–61).

Les champs éventuels doivent être nommés : `compressed_parts`, `strict_traces`, `strict_global_components`, `cofaces`, `nerve_edges` ou `performed_unions` décrivent des quantités différentes. Le nombre de représentants d'un raffinement n'est pas non plus un nombre intrinsèque de liaisons. Pour un rôle géométrique, dédupliquer les composantes à la coupe STRICTE avant le plateau ; pour l'affectation, utiliser la coupe FERMÉE après tout le plateau.

Conserver aussi λ_b dans chaque payload : un nœud peut recevoir des liaisons internes après sa naissance. L'union complète sur son sous-arbre décrit sa géométrie jusqu'à sa fin de vie, pas nécessairement son instantané de naissance. Un instantané au seuil a filtre en plus `λ_b≤a`.

Les trois intérieurs du témoin sont dans les boules des triangles de face ; ces triangles ne sont donc pas des simplexes du Delaunay ordinaire à boule vide. Dans l'ordre K, les `I∪A` sont des choix de K plus proches voisins au centre, parmi les ex æquo. Le cardinal q≤4 d'un support n'est ni K, ni le nombre de K-parties, ni le nombre d'arêtes de Γ_K.

## Limites

Ce reçu prouve les comptes sur UN nuage et démontre les distinctions de contrat ci-dessus. Il ne qualifie pas l'API future, les limites de coquille, l'énumération de Q_b, la mémoire, les compteurs natifs, une stabilité du squelette ou un gain de temps. L'ensemble Q_b doit être reconstruit depuis U complète : les présentations d'arité supérieure à q_min peuvent avoir été rejetées par les seuils d'admission du catalogue, alors que la boule est admise par un support plus petit. Cette obligation est étudiée séparément par l'autre auditeur.
