# Tranche34 : croissance et coût réel des parcours q4

## Résultat principal

Le saut des atlas entièrement morts et des branches sans feuille utile réduit fortement la navigation. Sur le scan0/K5, de8k à32k, `LiveOnly` fait passer les visites de navigation de42,142/147,628/699,047 millions à15,535/37,484/86,110 millions. Ce progrès est discret et reproductible dans les reçus ; ce n’est pas une conclusion tirée des chronos.

`Joined` prépare encore moins de familles, mais n’est pas gratuitement meilleur : à32k il ne retire que4,279 millions de visites supplémentaires par rapport àLive, tout en initialisant57,242 millions de cases de cache et en effectuant30,322 millions de bornes graine×cellule contre16,399 millions de tests de ligne pourLive. Une borne quadratique de bloc est plus coûteuse qu’une borne linéaire singleton. La baisse du seul nombre de familles ne justifie donc pas sa promotion par défaut.

Surtout, les trois parcours construisent les mêmes atlas et exécutent exactement les mêmes balayages terminaux. À32k/K5, restent866,771 millions de tests géométriques de partition d’atlas et735,508 millions de bornes préparées pour le comptage q3. Le poste de bornes de **blocs** d’atlas croît encore de×4,112 au dernier doublement. Sur le scan200, le premier doublement du comptage q3 reste×4,317. Le sous-quadratique global n’est donc pas acquis.

## Périmètre et clôture de la preuve

L’[analyse normale](GROWTH_ANALYSIS.json), son [exécution avec Python−O](GROWTH_ANALYSIS_OPTIMIZED.json) et leur [lecture de fermeture](GROWTH_READBACK.json) passent. Le dernier reçu contrôle319 fichiers inchangés :216 sources courantes, analyseur et lecteurs, captures, entrées et artefacts concernés. Aucun exécutable natif n’est relancé par [l’analyseur](analyze_growth.py).

Le rapport conserve60 observations et32 doublements :30 observations historiques de la tranche33, six observations R1 et24 R2. R1 etR2 ne sont jamais fusionnées en un faux essai moyen. Les grandes mesures publient des comptes et doubles digests, pas les listes intégrales des supports : le rapport ne revendique donc aucune comparaison de payload complet sur ces grands nuages.

Les contrôles appariés comprennent48 comparaisons de comptes/digests,25 comparaisons de tout le travail commun à mode identique,64 comparaisons de l’amont, des atlas et de tout le balayage terminal entre modes, et32 comparaisons d’amont entre les deux backends. Parmi ces contrôles figurent66 appariements33↔34 et18 appariements R1↔R2 ; ces catégories se recouvrent et ne doivent pas s’additionner. Seuls les deux pics de capacité dépendant du réemploi des workers sont normalisés : `work.q3.peak_shell_bytes` et `work.peak_edge_buffer_bytes`.

Provenances exactes :

- [Six mesures R1](performance/lidar_gnk4p2va/COMPLETION.json) : scan0,8k,K5,s8,Individual/Live/Joined,W1/W4. Elles sont closes mais restent distinctes de la qualification corrigée. Le mutant positif survivant et le snapshot R1 sont conservés dans `preflight/` ; le moteur n’a pas changé pourR2, seulement la couverture causale du test et le protocole Python.
- [Neuf mesures R2 principales](performance_r2/lidar_kk34w49h/COMPLETION.json) : scan0,K5,s8,W4, trois tailles et trois modes.
- [Trois mesures R2 K10](performance_r2/lidar_5vvvq0bh/COMPLETION.json) : scan0,s8,Live,W4.
- [Six mesures R2 s10/12](performance_r2/lidar_a__301v8/COMPLETION.json) : scan0,K5,Live,W4.
- [Six mesures R2 scans100/200](performance_r2/lidar_kluvqih4/COMPLETION.json) : K5,s8,Live,W4.
- Références33 : [modes de bornes à8k](../q34_affine_20260921/performance/lidar_cuwsvxnv/COMPLETION.json), [scan0 K5/K10](../q34_affine_20260921/performance/lidar_gladtapx/COMPLETION.json), [s10/12](../q34_affine_20260921/performance/lidar_8yueb3v7/COMPLETION.json), [scans100/200](../q34_affine_20260921/performance/lidar_h2s4wo62/COMPLETION.json).

Les références33 à s10/12 sont en **Window30**, pas enLocal28. Elles autorisent le contrôle des résultats et de tout l’amont/q3, pas une comparaison historique de navigationLocal28 pour ces séparations. Les historiques s8 et scans100/200 contiennent bienLocal28. Les exécutions historiques gardent leurs schémas originaux : les projections utilisées pour comparer des champs ne sont jamais présentées comme de nouvelles exécutions.

## Ce qui est réellement compté

La navigation désigne des passages, pas un temps CPU pondéré :

- Individual/Live : visites de l’arbre des graines + visites de la droite dans l’atlas ; Live ajoute les visites de préparation du résumé.
- Joined : visites de l’antichaîne + visites de produits graine×cellule + visites de préparation du résumé. Les anciens `edge.node_visits` ne sont **pas** ajoutés : ils comptent des prédicats déjà exécutés à l’intérieur de ces passages.
- Les lectures d’enfants du résumé, initialisations de cache, tests spatiaux, tests de propriété, préparations de familles, bornes graine×cellule, scans, tris et collectes restent séparés. `cache_misses` et les tests ponctuels de graines désignent les mêmes opérations ; `block_sites` et `cache_entries_initialized` la même masse initialisée. Les additionner doublerait le coût.
- Les populations représentées par les boîtes, telles que `rejected_sites`, ne deviennent pas des visites scalaires. Les bornes préparées mais non visitées du census sont incluses dans `count_bounds_prepared`, pas ajoutées une seconde fois.

Toutes les métriques principales ont leur formule dans `metric_definitions`. Tous les compteurs bruts, rapports de croissance, passages de zéro à une valeur positive et rapports supérieurs à4 sont conservés dans le JSON. Il n’y a pas de score unique mélangeant comparaisons, copies, populations et octets.

## Scan0/K5 : la comparaison des trois modes

Valeurs en millions d’opérations, sauf mémoire. Ordre des triplets :8k /16k /32k, s8,W4,R2.

| Poste payé | Individual | Live | Joined |
|---|---:|---:|---:|
| Navigation |42,142 /147,628 /699,047|15,535 /37,484 /86,110|15,273 /36,269 /81,831|
| Familles préparées |2,312 /7,931 /28,257|0,778 /1,924 /4,537|0,414 /0,863 /1,795|
| Tests ligne ou bornes graine×cellule |8,361 /32,403 /177,950|2,635 /6,860 /16,399|5,746 /13,570 /30,322|
| Bornes spatiales sur les graines |12,909 /34,599 /96,644|7,130 /15,999 /35,109|7,218 /16,219 /35,585|
| Cases de cache initialisées |0|0|11,899 /26,451 /57,242|
| Pic couplé q4 par arête, octets |49296 /56464 /60560|49296 /56464 /60560|49296 /61248 /71968|

À8k, les deux modes nouveaux évitent48707 des155605 atlas, entièrement morts. Ils paient néanmoins leur construction. Le résumé des autres atlas visite581822 cellules et lit474924 références d’enfants. À32k, il visite2,890 millions de cellules et lit2,450 millions de références. Ce gain appartient aussi àLive ; l’attribuer au produit Joined serait faux.

Le supplément mémoire ne se limite pas au cache visible : à32k, Joined paie un pic auxiliaire38272octets, dont un cache de32512octets et une pile de4344octets ; Live paie1896octets. Le pic couplé tient compte des phases réellement simultanées, sans sommer des pics disjoints. Les sommes des pics workers sont publiées séparément : à32k elles valent220288/217232/253680octets pourIndividual/Live/Joined. Ce ne sont ni des pics simultanés globaux ni leRSS, et le nuage/index partagés restent à compter à part.

L’aval identique coûte, aux trois tailles :12,960/27,234/57,555 millions de sites balayés et19,076/38,706/78,909 millions de comparaisons de tri/coquilles/groupes. Le cache ne supprime aucune de ces incidences nécessaires au chemin actuel.

## Croissance des grands postes avecLive

Chaque case donne les rapports8k→16k /16k→32k. Un rapport inférieur à4 sur ces deux pas est une observation, pas une borne asymptotique.

| Régime s8 | Navigation | Tests de ligne | Familles | Géométrie atlas, totale | Comptage q3 | Sites balayés q4 | Tris/groupes q4 |
|---|---:|---:|---:|---:|---:|---:|---:|
|scan0,K5|2,413 /2,297|2,603 /2,390|2,473 /2,358|3,057 /3,505|3,165 /3,442|2,101 /2,113|2,029 /2,039|
|scan0,K10|2,434 /2,252|2,536 /2,287|2,499 /2,263|2,590 /2,902|2,543 /2,653|2,160 /2,106|2,131 /2,075|
|scan100,K5|2,314 /2,325|2,446 /2,446|2,308 /2,368|2,463 /3,031|2,455 /1,928|2,179 /2,179|2,101 /2,144|
|scan200,K5|2,402 /2,353|2,564 /2,482|2,469 /2,412|3,827 /2,713|4,317 /1,402|2,120 /2,175|2,113 /2,242|

À32k/K10, malgré cette croissance favorable de la navigation, on paie encore3,082 milliards de tests géométriques de partition,2,127 milliards de bornes de comptage q3,291,593 millions de sites balayés q4 et305,410 millions de comparaisons de tri/groupes. Le tri n’est pas l’unique chantier, et ces compteurs ne sont pas convertibles directement en durées comparables.

### Postes supérieurs à4 à ne pas masquer

Les postes suivants sont inchangés par le nouveau parcours et sont réellement payés ; les branches de résultat ou sous-ensembles ne s’ajoutent pas à leurs parents :

- **Scan0/K5,16k→32k** : bornes de blocs d’atlas×4,112 ; divisions Z×4,008. Le total atlas×3,505 n’efface pas ces deux sous-postes. Les descripteurs non examinés géométriquement faute de budget font×5,108 puis×5,849 ; ils restent manipulés comme frontière et sont distincts des tests géométriques.
- **Scan200/K5,8k→16k** : bornes q3 préparées×4,317 et count+shell×4,277 ; seules bornes de boîtes q3×4,380, visites de nœuds×4,326, divisions×4,320 et bornes préparées mais non visitées×4,298. Préparations/appels de census et boules q3×4,210, tests de propriété×4,091. Les saturations/rejets×4,240 sont un sous-ensemble de ces appels, pas du travail indépendant à additionner.
- **Même scan200, premier pas, atlas** : visites de partition×4,074, bornes de blocs×4,355, divisions Z×4,181 et copies des IDs de frontière×4,021. Les descripteurs non examinés font×7,053, ceux testés mais restés ambigus×4,422. Les masses admises ou retirées de boîtes ne sont pas des sites visités un à un.
- **Construction terminale** : scan100,8k→16k, raffinements8→62,×7,750 et arrêts à la profondeur9→65,×7,222. Scan200,16k→32k, raffinements72→30607,×425,097 et arrêts352→30773,×87,423 ; les raffinements finalement profonds passent50→30604,×612,080, résultat inclus dans ces constructions. Scan0/K10, dernier pas, arrêts339→1510,×4,454. Les passages de zéro à une valeur positive restent publiés séparément.

Les autres ratios bruts supérieurs à4 concernent notamment des classes minoritaires de rectangles émis, classifications de constantes des balayages, contacts de frontières et sites restant après une émission. Ils sont tous listés avec les valeurs brutes dans `growth[].raw_above_quadratic_step`, y compris les maxima, stockage et populations qui ne constituent pas du travail scalaire. Par exemple les décisions de frontière q4 vont jusqu’à×7,068 surscan100 ; les IDs de frontière correspondants jusqu’à×7,472. Cela n’est pas masqué par les tris agrégés sous×2,25. En revanche le nombre théorique de paires, toujours voisin de×4, n’est pas une mesure de visites réellement effectuées.

Individual reste dans le témoin R2 : ses visites de droite surscan0/K5 font×4,018/×5,603, ses tests de ligne×3,876/×5,492. Live/Joined corrigent bien ce poste précis, pas l’ensemble des postes ci-dessus.

## Séparation WSPD s8/10/12

Pourscan0/K5/Live, les candidats, q3, atlas et balayages q4 sont identiques aux trois séparations. À32k, le front fait4,632/5,402/6,082 millions de visites de produits ; les recherches de témoins rectangulaires et par paire totalisent212,533/198,495/193,989 millions de bornes H et171,302/157,060/152,212 millions de bornes Xi. Les derniers rapports de H sont2,556/2,477/2,447 et ceux deXi2,645/2,550/2,512. Une séparation plus grande échange donc ici davantage de travail du front contre moins de recherche de témoins, sans modifier l’aval. Les reçus ne permettent pas de conclure à un gain temporel stable.

## Temps et conclusion de développement

Les campagnes se sont chevauchées avec d’autres charges CPU. Les observations sont conservées, pas promues en mesure de speedup isolée. Exemple révélateur à8k/K5 : R1/W4 donne5,518/4,921/5,475 secondes pourIndividual/Live/Joined, alors queR2 donne4,576/4,798/3,967 secondes avec le même travail géométrique. À32k/R2 les observations72,304/91,426/113,677 secondes ne suivent même pas l’ordre des volumes de navigation. R1/W1 donne15,317/12,669/13,399 secondes et reste une observation distincte.

La priorité suivante ressort des volumes : diminuer la construction et le raffinement exacts des atlas, puis le nombre/coût des census q3, surtoutscan200 ; ne pas poursuivre le seul compteur de familles préparées. Joined mérite un examen ciblé du coût des bornes de blocs et du cache, pas une promotion fondée sur la suppression des anciennes visites de droite.

Cette campagne ne calcule toujours pas la tourFULL. Elle ne qualifie ni le contrat50k en1seconde ou100ms, ni le GPU/G4, ni les dizaines de millions de sites. Aucun GCP n’a été utilisé pour cette tranche. Les données sont des préfixes quantifiés de trois scans LiDAR réels, pas une preuve sous-quadratique dans tous les régimes.
