# Audit de distribution du tri des présentations v9

23 septembre 2026. Portée : `gather_presentations` de `src/chain/tower_chain.cpp` au commit `6200bb5a` ; cadre `exploration_v9_hors_registre`, CPU local, entrée entière u18 sans sol, `not_claimed`. Ce diagnostic complète le contre-audit B `610264da` : sa mutation prouve déjà que la porte « au moins deux plages » admet une plage vide. Aucun test G4 ni qualification de la tour FULL ne découle de cette note.

## Shadow sur une trame entière

J'ai compilé une copie instrumentée de `tower_chain.cpp` sous `/tmp` contre le générateur existant (`libmhgp9_gen.a`, SHA256 `208aabb30a764bad25ab1c99d74885bd405e84a13dcb8375622d66aa6203f70a`). Le SHA256 du source produit lu est `f390e15aff50c80808ca88ff1ffb045ad245d3b5c758ad72b7d1db55a2ff14d3`, identique au commit étudié ; aucun fichier produit n'a été modifié. La sonde exécute `--no-tower`, K=5, s=8, W=8 et les leviers par défaut sur `08/000000` sans sol, grille 1 mm, 39 885 sites. L'empreinte **FNV de sites** publiée par la sonde est `5c785760053d17ce`, identique à la première ligne R6 ; le **SHA256 des octets d'entrée** est `0baa4de14c95838ef7bd18d5a98551ca513ed830ec1eeee84f649fa97c95abaf`. Le statut `complete_relative` couvre ici le générateur, la fusion et le recensus du catalogue ; la tour n'a pas été construite. La sonde donne 1 306 699 présentations et 1 306 696 clés distinctes, mêmes comptes que R6.

| grandeur | observation K5/W8 |
| --- | ---: |
| slots non vides | 8/8 ; 49 841 à 269 191 présentations (max/min 5,40) |
| clés de l'échantillon après dédoublonnage | 4 096 |
| plages | 32/32 non vides ; 35 684 à 62 101 présentations (max/moyenne 1,52) |
| comparaisons `(clé, arité, support)` : tri préalable des slots | 27 738 460 |
| comparaisons `(clé, arité, support)` : second tri des plages | 28 514 740 |
| comparaisons de clés : assignation directe aux 32 plages | 6 533 495 |

L'assignation directe est un shadow sans mutation du résultat ; son histogramme égale exactement les 32 longueurs mesurées des plages. La répartition des sorties entre slots est fortement inégale, mais les séparateurs produisent des plages raisonnablement équilibrées pour **cette** trame. Les deux tris paient néanmoins 56,25 millions de comparaisons au total. Les temps locaux instrumentés (197 ms premier tri, 173 ms second tri, 24 ms classification) sont seulement indicatifs : l'hôte était partagé, l'instrumentation et le chemin `--no-tower` changent le périmètre, et ce n'est pas une ablation G4. Le second run ci-dessous refait le générateur, dont la distribution entre slots dépend de l'ordonnancement ; il faut comparer ses colonnes **à l'intérieur du même run**.

## Coûts et alternative

Le coût du chemin publié comprend deux tris (`Σ_s O(P_s log P_s)` puis `Σ_b O(P_b log P_b)`), une copie de `P` présentations, `S(B−1)` recherches binaires dans les slots et un échantillon de clés. L'échantillon prend `16×wanted` éléments **par slot non vide**, avec `wanted=4W` dès `P≥4096` ; il n'est pas borné par `P` avant dédoublonnage. Avec `S=W`, cela peut atteindre `64W²` clés temporaires, même si beaucoup de slots sont minuscules et répètent les mêmes éléments. Par exemple, W=512 demande jusqu'à 16 777 216 clés de 80 octets avant dédoublonnage. Les quantiles donnent aussi le même poids à chaque slot, quel que soit son volume. Ces deux propriétés sont des risques de coût et d'équilibre hors de la trame mesurée, pas un défaut d'exactitude observé.

Une voie plus simple est : compter `P`, prendre `M=min(P,16×4W)` positions globales dans la concaténation des slots avec un échantillonnage déterministe stratifié et une gigue à graine fixe (donc pondéré par leurs tailles), trier ces `M` clés **avec multiplicité**, choisir les quantiles et dédoublonner les séparateurs. Ensuite, classifier chaque présentation par recherche binaire sur les séparateurs, conserver son indice de bucket, calculer l'histogramme `(slot,bucket)`, allouer un tampon de `P`, faire un scatter par offsets exclusifs, et trier chaque bucket une seule fois selon `(clé, arité, support)`. Conserver l'indice coûte `O(P)` mémoire ; le recalculer pendant le scatter ferait une seconde série de recherches binaires. Les intervalles sont `[séparateur_{b−1}, séparateur_b)` ; l'égalité avec un séparateur va dans le bucket suivant. Ainsi une même clé ne traverse jamais deux buckets, et la concaténation des buckets triés a exactement l'ordre global. Le choix des séparateurs n'affecte que le partage du travail, pas cette preuve d'ordre. Il faut conserver les contrôles de profondeur, coquille et doublon pendant le scan final. La qualité d'équilibrage de cet échantillonnage proposé reste à mesurer.

## Tri des buckets bruts, mêmes séparateurs

Un second sidecar garde une copie des slots **dans l'ordre d'émission** avant le premier tri. Après que le code publié a choisi ses séparateurs, il classe et disperse cette copie dans un tampon unique, trie les buckets bruts en parallèle avec le même comparateur, puis compare élément par élément les deux suites finales : clé, arité, support, profondeur et coquille. Il réutilise volontairement les séparateurs publiés pour isoler le coût du tri ; il **ne mesure pas** encore la qualité de l'échantillon pondéré proposé. Le run complet donne `alt_exact=1` sur les 1 306 699 présentations. Un préflight indépendant de 1 500 sites/K5/W2 donnait aussi `alt_exact=1` sur 86 235 présentations.

| même run 08/000000 K5/W8 | chemin publié | shadow histogramme/scatter |
| --- | ---: | ---: |
| comparaisons complètes du tri préalable des slots | 27 920 260 | 0 |
| comparaisons de clés pour classer dans 32 buckets | recherches de coupures par slot | 6 533 495 |
| comparaisons complètes du tri des buckets | 27 580 129 | 24 422 805 |
| buckets non vides ; plus grand | 32/32 ; 65 089 | 32/32 ; 65 089 |
| suite finale `(clé, arité, support, profondeur, coquille)` | référence du même run | égale sur chaque élément |

La différence mesurée est de **31 077 584 appels au comparateur complet** en moins, contre 6 533 495 comparaisons de clés ajoutées pour la classification, plus les copies, offsets et l'échantillonnage futur. Le second tri des buckets bruts n'a donc pas absorbé le coût du premier sur cette trame. Ce sont des **comptes d'opérations**, pas une promesse de baisse du temps mur : le second sidecar exécute les deux chemins, garde temporairement une copie supplémentaire des présentations, et l'hôte reste partagé. Les sorties locales sont `/tmp/mhgp9_sample_sort_alt_k5.{json,stderr}` ; SHA256 du source sidecar `96d42f8af9ae19f3ada1de3f60021c692372c5136c9ed826d5da449a332a2dc2`, du binaire `f4bc81c99271d7e12ac7616d7f706eace16e3633aad6d78688c20531394e3f32`, du JSON `b81a391c0c521921b63be23c1b5861a06741b18ffe71626bd01a85f248aa8799`, et de stderr `2d3484e69988c9059dbbd475a2904516b795aa0c7b21a38718b86a1b038d80e9`.
