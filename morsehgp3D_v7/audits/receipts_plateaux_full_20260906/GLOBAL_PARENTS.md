# Parents globaux des trois plateaux réels 50k

6 septembre 2026, suite de 22003315. **Les trois ambiguïtés locales sont décidées : le bloc 174406 est déjà connecté avant son plateau ; les blocs 254569 et 996863 relient chacun deux parents globaux distincts.** Les preuves utilisent le nuage complet régénéré et quelques miniboules rationnelles. Aucun catalogue de facettes, moteur C++ ou GCP n’est exécuté ; `public_status=not_claimed`.

## Deux certificats suffisants

Écrire β(F)=ρ(F)². À l’ordre K, une facette F est active strictement avant R si β(F)<R. Toute première arête d’un chemin quittant F passe par une coface F∪{z}, pour z extérieur à F, de cardinal K+1. Cela vaut pour le graphe élémentaire qui restitue les composantes de la vraie hiérarchie K-NN ; aucune hypothèse Gabriel sur cette coface n’est nécessaire.

**Connexion.** Pour deux représentants F₁ et F₂ partageant K−1 sites J, un point z fournit le chemin F₁→J∪{z}→F₂ dès que β(F₁∪{z})<R et β(F₂∪{z})<R. Deux témoins de boules contenant ces cofaces, de rayons strictement inférieurs, suffisent déjà ; les reçus calculent ici les MEB exactes.

**Isolement.** Si β(F)<R et β(F∪{z})≥R pour tout z hors F, la composante globale stricte contenant F est exactement {F}. Une autre facette stricte distincte appartient nécessairement à un autre parent. Pour vérifier tous les z sans calculer toutes leurs MEB, utiliser :

$$\beta(F\cup\lbrace z\rbrace)\geq\frac{1}{4}\max_{x\in F}\left\Vert x-z\right\Vert^{2}.$$

Un point dont le maximum vaut au moins 4R est donc éliminé, **égalité comprise**. Il reste à calculer la MEB exacte de chaque survivant. Passer le filtre de distances ne prouve pas l’existence d’une coface stricte. Réciproquement, l’échec d’une recherche de pont à un point ne prouve pas l’isolement : le certificat négatif doit couvrir toutes les cofaces incidentes à une même facette.

Le scan coûte O(nK) distances entières par facette testée ; seuls ses survivants demandent un calcul géométrique supplémentaire. Il n’énumère ni les K-facettes globales ni la mosaïque de Delaunay d’ordre supérieur. Un index pourrait éliminer une boîte dès qu’un x∈F est à distance minimale au moins 2√R de cette boîte. Cette observation est une possibilité de filtrage, sans implémentation ni gain mesuré ici ; ces certificats suffisants ne remplacent pas le resolver général.

## Résultats sur l’entrée épinglée

Les identifiants sont ceux de l’entrée à 50 000 points, seed 3, coordonnées dans [0,65535], digest `3f7c6dd47bcba4222e511c94f90aaeeeb80198b0d5ac8a6721e4ff55feedab3f`. Les populations I/U sont celles du [census indépendant](LOCAL_DIAGNOSTICS.md). Pour chaque triangle de coquille, A et C sont les extrémités du diamètre et V le troisième site ; les représentants sont I∪{A,V} et I∪{C,V}.

| Boule | K | R exact | Certificat | Parents globaux du bloc avant lot |
| ---: | ---: | --- | --- | ---: |
| 174406 | 5 | 14352441/4 | Pont extérieur 45617 | 1 |
| 254569 | 2 | 2904043/2 | Facette {32276,34292} isolée | 2 |
| 996863 | 6 | 6675549/2 | Facette {23184,23681,25389,34559,42468,43571} isolée | 2 |

Pour **174406**, J=I∪{42779} et z=45617, de coordonnées (57873,31035,50862). Les deux cofaces ont pour rayons carrés 183094651363677846733/60362664148548 et 65739899631925920225/20392115340898, tous deux strictement inférieurs à 14352441/4. Les deux classes locales ont donc la même image globale. Comme leur couverture locale réunit déjà S, ce bloc n’apporte ni fusion ni point supplémentaire. Son obligation d’ancre reste distincte.

Pour **254569**, les deux paires strictes naissent aux rayons carrés 2194345/4 et 3613741/4. Le scan de tous les points hors {32276,34292} donne un minimum du maximum des deux distances carrées égal à 5808086=4R, atteint uniquement par 4912. Aucun survivant strict ne demande une MEB supplémentaire. La facette isolée et l’autre représentant sont donc deux parents globaux distincts, même s’ils partagent le point 32276.

Pour **996863**, le filtre autour de la facette indiquée ne laisse que les identifiants 19323, 21608, 34650 et 38604. Leurs quatre cofaces ont toutes une MEB de rayon carré **strictement supérieur** à 6675549/2. La facette elle-même est stricte : son isolement global est ainsi certifié. Les supports et valeurs rationnelles complets sont dans le reçu.

Ces deux derniers blocs forcent une vraie fusion au niveau indiqué. **Le nombre deux concerne les images globales de leurs représentants avant le lot**, pas nécessairement l’arité totale du nœud public : d’autres boules de même rayon peuvent rejoindre le même groupe atomique par ces parents. Leur catalogue et ce regroupement ne sont pas reconstruits ici. Pour 174406, le bloc est inerte, mais son parent pourrait participer à un événement dû à une autre boule du même lot.

Les trois naissances déjà prouvées aux ordres K6, K3 et K7 sont inchangées. La quatrième boule, 1251653, reste localement inerte dans K1..10, avec son ancre K10. Aucun de ces verdicts ne qualifie le raccord FULL, l’export ni la complétude du producteur.

## Vérification reproductible

Le [juge](real_parent_certificates.py) réutilise la régénération indépendante épinglée, lie les identifiants aux coordonnées et compare les MEB par arithmétique rationnelle. Les [sorties normales](real_parent_certificates_normal.json) et [optimisées](real_parent_certificates_optimized.json) sont identiques, codes 0 : 24 MEB de sept points au plus, 837 supports examinés, trois census complets et quatre scans de distances, dont celui qui réfute un faux isolement. Quatre mutants sont rejetés : certificat des survivants omis, coface stricte ignorée, identifiant changé et digest changé. Les frontières ouverte/fermée et l’insuffisance du filtre sont non vacues sur ces mêmes cas réels. Le [reçu de revue](global_parent_review.json) déclare les sources, la capture et la portée.

```bash
taskset -c 1 python3 -B morsehgp3D_v7/audits/receipts_plateaux_full_20260906/real_parent_certificates.py
taskset -c 1 python3 -B -O morsehgp3D_v7/audits/receipts_plateaux_full_20260906/real_parent_certificates.py
```
