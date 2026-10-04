# Contrelecture géométrie, index et catalogue — e02

Source : `e02a6c235bc4a706519cdaa15f4b1465a6275eba`. Lecture indépendante, copies avant lecture ; aucun code produit modifié, natif/build/fit/GCP exécuté. Cette capsule est un contre-audit de nos explications, pas une nouvelle qualification. Les 18 fichiers num, 6 index et 28 catalogue sont relus ; les tests et oracles sont conservés, leur relecture est ciblée (voir COVERAGE).

**Conclusion : aucun nouveau défaut géométrique dans une sortie réussie n'est établi.** Le refus POINTS/u24/192 bits de la capsule précédente reste distinct du moteur FULL et n'est pas redécouvert ici.

## Écart concret de travail : niveau q3 avant rejet

Dans `source/morsehgp3D_v11/src/catalogue/leaf.cpp:113` et `:230`, un q3 aigu est construit par `Sphere::through` avant son test propriétaire. `src/num/sphere.cpp:46–49` calcule immédiatement la formule de niveau de degré 6 ; ensuite seulement viennent propriétaire, census et canon/admission. Dans le générateur v10 épinglé 777, `generator.cpp:423–426` calcule centre3 avant propriétaire et `:262–275` calcule le niveau après admission.

Le modèle exact retrouve une feuille réellement accessible à la préparation DFS séquentielle K3/leaf6 : boîte `[1,3)×[1,3)×[2,3)`, liste `[0,1,2,3,4,6]`, triplet sites `[0,2,4]` = `(3,0,2),(1,4,0),(0,4,2)`. Il est aigu, passe G3 et ses sous-préfixes, le graphe de paires et J2 ; sa droite de centres touche la fermeture en `(5/2,11/4,2)`. Son centre `(95/58,61/29,91/58)` échoue pourtant propriétaire sur z. Le Level brut `3000/464` est construit puis jeté. Le replay est Fraction, pas une exécution produit, et ne prétend pas valider le paramétrage adaptatif de G4.

Prochain essai borné utile : candidat q3 privé transportant ancre/N/D, tag de présentation 3 et certificats actuels ; calculer **la même formule de degré 6 et le même encodage sans PGCD** après S*/admission. Préserver le test d'acuité, le repli puissance/orientation et la récursion q4 des triples obtus. N²/D² de degré 10, retag q4 et nouveau PGCD seraient des changements différents. Il faut des compteurs par stade et un A/B natif avant d'annoncer un gain.

## Explications à réduire

- Les mêmes XYZ du cube u18 ne forcent pas un census q3 Wide sous profils21/24. Pour L=2^18−1, D≤6L⁴<2^75 et |N_j|≤12L⁵<2^100 satisfont même `power_certificate.hpp:8–15` en B24 ; `predicates.cpp:36–37` choisit donc i128. Les types Level et produits de tri sont cependant plus larges (table JSON), et le test SAT de droite passe i64→i128 dans v11. Ce n'est pas un pourcentage de temps. Le SAT v10/T6 était déjà i128.
- L'expression de notre note « v10 non appariée à ces lots u21 » doit distinguer **XYZ identiques**, établis par la recoupe des archives effectuée séparément, de profils/exécutions non appariés et de dumps LiDAR interversions encore non comparés. La faiblesse du juge v10 ne prouve pas que ses chronos plus faibles proviennent d'un objet mathématique omis.
- `catalogue.hpp:46` définit correctement un ledger logique d'une passe. L'ancien commentaire `:57` parlant d'extensions effectivement visitées est trop fort : `leaf.cpp:246,252` ajoute les prolongements coupés sans visite. `census_tests` comprend les sites du support dont le contact est réutilisé (`leaf.cpp:151–160`), pas uniquement les appels side. Un arrêt précoce interdit aussi de soustraire systématiquement q×judged.
- La voie de référence rejoue deux fois la géométrie mais publie un seul ledger ; la voie fast/single_pass publie `geometry_passes=1` (`single_pass.cpp:177`). Ne pas expliquer les chronos fast actuels par deux passes désormais absentes. Les ledgers index/census ont leurs propres scopes.
- Même XYZ n'implique pas même subdivision : v10 utilise T6 et des coupes sous l'unité (`generator.cpp:598–611`), v11 T0 s'arrête à largeur1 (`boxes.cpp:143–154`). Également, graphe/mémo J2 v11 ont un seuil32 puis repli exact. Mesurer histogrammes de feuilles, rejets et coûts de préparation avant de transférer une causalité. Un port naïf T6 vers B24 ne conserve pas les bornes actuelles.

La borne supérieure séparée de puissance demeure sûre mais parfois lâche : exemple autonome q2 et boîte strictement intérieure reproduit la piste UB couplée déjà documentée par l'autre auditeur. Ce n'est ni un nouveau défaut ni une mesure de trafic d'index.

## Preuve et fermeture

`python3 check_staging.py` et `python3 -O check_staging.py` : 1 733 gardes, JSON identiques. Les 336 présentations de triangles sur coins du cube sont comparées Cramer/Gram ; le majorant global vient de la multi-affinité des déterminants projetés, pas d'une extrapolation des seuls coins testés. Aucune allocation massive ni chronométrie nouvelle.

`SOURCE_BEFORE.json` et `SOURCE_V10_BEFORE.json` fixent 244+5 dépendances ; `SOURCE_AFTER.json` rejugera Git/copies/LIVE séparément et consignera toute dérive documentaire/auteur. `COMMANDS.json` garde les appels et sorties, `VERIFICATION.json` le contrôle de fermeture. SHA256SUMS inventorie tous les payloads, excepté lui-même.
