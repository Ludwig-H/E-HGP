# Tour T/M/V : contrelecture avant publication, 7 octobre 2026

**Prototype non publié**, lu sans modification ni construction native. À la capture, `main` = `9428db65b25e1660e78e34ce273e155d99b4fac3` n'a pas de `src/tower/` ; le contrat de tour est identique dans les deux copies. `capture.json` épingle 16 fichiers, dont le rapport de livraison et `INTERFACE_TMV.md`. La portée est une inspection et un témoin algébrique léger, pas une nouvelle qualification FULL ou une mesure de vitesse.

## 1. Les branches des hyperarêtes restent à conserver dans R

`OrderForest` (`forest.hpp:136`) est explicitement présenté comme une **première forme** du registre. Il conserve `event_cell`, les attaches et la forêt contractée, mais aucune liste des branches ouvertes `ant(b)` des cellules retenues. `finish_order` (`forest_stages.cpp:141`) libère les événements binaires ; conserver ces seuls événements ne suffirait d'ailleurs pas.

Témoin : trois naissances 0, 1, 2 de rang 0 et deux cellules de même rang 1. Le premier hypergraphe porte `{0,1}`, puis `{0,2}` ; le second `{0,1}`, puis `{1,2}`. Les deux cellules sont retenues dans chaque cas. Avant le rang 1, les trois composantes sont distinctes : les branches de la seconde cellule sont donc respectivement `{0,2}` et `{1,2}`. Après la première union, 0 et 1 ont déjà la même racine courante. La seconde union produit alors exactement le même événement binaire dans les deux cas. Événements, attaches, `event_cell`, `cell_node` et tous les champs mathématiques de `OrderForest` coïncident ; les deux forêts ont la même fusion ternaire, correcte.

`check.py` reproduit cette projection sur ces deux entrées T abstraites ; `result.json` conserve les champs et les deux `ant`. Rejeu normal et `-O` identiques. **C'est un modèle d'hypergraphes et d'identifiants d'entrée T, pas un contre-exemple géométrique HGP complet ni LiDAR.** Il prouve que le résumé produit ne détermine pas les branches ouvertes dans le domaine de cette interface. Le catalogue et les cibles de G pourraient servir à les recalculer, mais ce serait un travail supplémentaire à chiffrer, pas une vue du seul registre actuel.

Conséquence de livraison : `CONTRAT_TOUR.md` § 5 demande les hyperarêtes avec leurs branches ; `ARCHITECTURE.md` § 4.4 demande un registre produit au fil du calcul. Le plan prévoit aussi une tranche « T3 — registre et vues », et l'interface de livraison parle de « première forme ». **Le report explicite de cette partie à T3 est donc possible**, en la laissant ouverte et en précisant son coût ; cette capture ne démontre aucun défaut de pi0. Elle ne permet simplement pas de déclarer R complet. Pour compléter R, conserver une CSR des branches à la coupe **ouverte** par cellule retenue, y compris les représentants devenus redondants après d'autres unions du même plateau. Les racines courantes après ces unions ne remplacent pas cette information. Une reconstruction a posteriori contredirait l'objectif de production au fil du calcul tant qu'elle n'est pas déclarée et mesurée.

## 2. Plateaux, attaches et verticales : invariants correctement traduits à la lecture

Le noyau exige des cibles de rang strictement inférieur à la cellule et relit l'élément d'une cellule cible à sa racine courante. Chaque union de composantes distinctes produit un événement ; racine unique équivaut ici à `événements = naissances - 1`. La contraction relie uniquement un événement à ses opérandes événements **de même rang**, et les tranches ne coupent jamais un rang. Chaque sommet binaire consommé par une union l'est une seule fois : les écritures de parents sont donc disjointes entre classes. Cela respecte les multifusions et évite de contracter les événements de rang inférieur.

L'union par taille donne la borne de profondeur d'attache : chaque fois qu'une composante devient perdante, la taille du contenant double au moins ; au plus `floor(log2(naissances))` attaches. `component_at` suit les attaches de rang **≤** à la requête, puis cherche le dernier événement admissible du survivant : c'est bien la coupe fermée. La garde `rang(naissance) ≤ rang(requête)` est présente. Pour les naissances, une seule remontée au parent de même rang suffit après contraction, puisque les rangs des parents sont alors strictement croissants. Ce sont des vérifications de structure sur les sources épinglées ; aucune porte native n'a été rejouée ici.

## 3. Admission et compteurs : portée de la contrelecture

Les formules de T et M majorent les nouveaux tampons bruts de leur étage, en plus du `used` déjà vivant ; le nombre de naissances/cellules est borné à `2^31-1` avant ces calculs. M réserve conservativement jusqu'à une tranche par événement, alors que les tranches réelles sont comptées : sûr mais potentiellement pessimiste. Les verticales réservent d'abord leurs sorties puis, séparément, les descripteurs et compteurs des morceaux. Les compteurs logiques sont écrits par ordre ou par morceau, puis additionnés ; les temps et le nombre de tranches sont séparés.

La copie examinée emploie encore l'ancien `core/buffer.hpp` sans marge d'admission du cache. Il faut donc la rebaser sur le correctif cache déjà contre-éprouvé et tester la chaîne livrée ; aucune qualification mémoire de cette copie ancienne n'est transférée. La porte de refus consultée utilise un budget illimité et juge des invariants d'entrée/transaction, pas des seuils mémoire finis. Ces limites n'établissent pas un nouveau défaut d'admission de la tour.

Rejeu du seul modèle, depuis la racine du dépôt :

```sh
python3 -B -S morsehgp3D_v12/receipts/audit_t1b_tour_prepublication_20261007/tour/check.py
python3 -O -B -S morsehgp3D_v12/receipts/audit_t1b_tour_prepublication_20261007/tour/check.py
```

Pas de build, de modification du prototype, de lecture de données de scène ou de GCP. Cadre `exploration_v12_hors_registre`, `cpu_reference`, `full_pi0`, `quantized_u21_input_only`, `public_status=not_claimed`.
