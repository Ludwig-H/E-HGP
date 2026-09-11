# Résoudre par fenêtres et composer des certificats de toute la filtration

11 septembre 2026. Suite constructive de [la décomposition parallèle](../receipts_parallel_objects_20260911/README.md), sur le constructeur `full_ball_tower.hpp` `83f1c78e`. Cette note apporte une preuve, deux raccords possibles et un modèle Python indépendant. Aucun remplacement du moteur ni gain de temps ou de RSS n’est revendiqué. GCP non utilisé.

```text
phase=exploration_v7_hors_registre
backend=cpu_reference
profile=quantized_u16_input_only
mode=audit_independant_math_and_architecture
public_status=not_claimed
```

**Il n’est pas nécessaire de garder simultanément toutes les arêtes ni toutes les requêtes développées.** Les fenêtres peuvent produire chacune une forêt couvrante minimale (MSF), puis ces certificats se réduisent entre eux. Cela préserve toutes les coupes de la vraie hiérarchie, dans un ordre de réception quelconque. Le nouveau choix architectural porte sur le domaine de ces certificats : hubs, ou seules naissances.

Le principe général de remplacer des sous-graphes par des certificats et de diviser l’ensemble des arêtes est classique : Eppstein, Galil, Italiano et Nissenzweig, *Sparsification—A technique for speeding up dynamic graph algorithms*, JACM 1997. [Notice primaire de l’auteur](https://ics.uci.edu/~eppstein/pubs/p-sparsification.html), consultée le 11 septembre 2026. La preuve ci-dessous explicite son application aux dates, plateaux et objets HGP ; aucune nouveauté du principe général n’est revendiquée.

## 1. Certificat composable, aux deux côtés de chaque coupe

Fixer un graphe fini, des identités de sommets V et leurs dates de naissance β. Toute arête e doit satisfaire w(e)≥max(β(u),β(v)). Les arêtes des hubs satisfont cette condition avec égalité du côté émetteur ; celles du graphe réduit ont leurs deux naissances strictement antérieures. Rangs exacts et dates rationnelles donnent la même filtration, à condition de conserver les égalités.

Découper arbitrairement les arêtes en lots Eᵢ et choisir une MSF Fᵢ de chaque lot. Toute arête e omise possède dans Fᵢ un chemin dont chaque poids est au plus w(e) : sinon remplacer une arête plus lourde de ce chemin par e diminuerait la forêt. Tous les sommets intermédiaires sont eux aussi nés au plus à w(e), par la garde sur les extrémités. Ce chemin est donc actif partout où e l’est, pour une coupe ouverte comme fermée. Comme Fᵢ⊂Eᵢ, aucune connexion n’est ajoutée.

L’union des Fᵢ conserve ainsi toutes les partitions de l’union des Eᵢ. Reprendre sa MSF conserve encore ces partitions. Par récurrence, **tout arbre de réduction des lots est correct**, même si leurs dates se recouvrent et qu’une arête plus légère arrive tard.

Sur un même graphe, une clé totale globale `(rang géométrique, extrémités canoniques, ordinal unique stable)` donne davantage : la MSF finale est identique à la MSF directe, indépendamment des lots et de leur arbre de réduction. Une arête omise localement est le maximum strict d’un cycle pour cet ordre total ; elle ne peut appartenir à l’unique MSF globale ainsi départagée. Les événements publics se groupent toujours par **rang géométrique seul**.

Les naissances isolées restent dans V, avec leur date. Les contributions, masques, populations et marques d’admission ne sont pas éliminés avec les arêtes. Les fusions des lots ne sont jamais des fusions FULL : deux arêtes à la même date, traitées séparément, peuvent participer à une seule multifusion ternaire globale. Reconstruire l’histoire après composition du certificat, ou qualifier séparément un algorithme équivalent ; concaténer les histoires locales est faux.

## 2. Raccord en une passe : comprimer avant de connaître φ

Reprendre les objets et les prémisses de [la réduction aux naissances](../receipts_filtered_graph_20260911/README.md) : census complet admis, quotient local exact, représentants stricts et terminales antérieures. Pour un K fixé, A compte les sommets originaux, points K1 compris ; L compte les naissances ; R compte les occurrences de représentants. Pour toute la tour, additionner ces nombres sur les K en gardant des identités distinctes.

1. Depuis l’atlas, développer seulement une fenêtre de représentants, résoudre ses terminales et émettre les arêtes originales B–T datées de λ_B. Leur classement géométrique ne dépend pas de l’histoire des composantes.
2. Comprimer ces arêtes par MSF et fusionner les certificats. En parallèle, enregistrer exactement un pointeur pivot par hub non naissant : celui d’un ordinal fixé avant exécution, jamais la première réponse asynchrone.
3. Après résolution de tous les pivots, calculer φ par descente ou doublement de pointeurs strictement décroissants. Le certificat global sur hubs a au plus A−C arêtes, C étant le nombre de composantes finales.
4. Projeter **toutes** les arêtes retenues `(u,v,w)` en `(φ(u),φ(v),w)`, supprimer les boucles, puis reprendre une MSF sur les L naissances. Sa taille finale est L−C.

**Pourquoi la projection tardive est correcte.** Une arête originale e supprimée possède un chemin de remplacement dans la MSF des hubs à des dates au plus w(e). Projeter ce chemin donne un chemin, éventuellement ponctué de boucles, entre les mêmes naissances. Celles-ci sont nées au plus à la date de leur hub. Le certificat projeté conserve donc toutes les coupes du graphe original projeté ; ce dernier est le graphe réduit déjà prouvé, les pivots se projetant en boucles. Les tables de marques conservent l’admission du hub, même si sa naissance φ(B) est plus ancienne.

Cette route réalise R résolutions d’occurrences avant mutualisation, en une traversée. Elle permet de libérer chaque fenêtre de clés développées et de résultats terminaux après consommation. Son certificat travaille sur A sommets avant la projection. Les pointeurs/φ et métadonnées des hubs restent de taille O(A) ; leurs fenêtres géométriques et les résumés en attente ont des budgets séparés.

### Identité FULL et identité du certificat

Si le départage est recalculé avec les extrémités après projection, la route précédente peut choisir une autre MSF que la réduction préalable vers les naissances. Elle conserve les partitions et multifusions ; promettre les mêmes arêtes serait excessif.

Contre-fixture permanente du modèle : a=0, b=1, c=2 naissent à 0 ; C=3 et B=4 apparaissent à 1. Les arêtes aC, aB, bC, bB, cC valent toutes 1 ; pivots C→c et B→a. Kruskal par extrémités sur les hubs garde aC, aB, bC, cC. Après projection et suppression des boucles, il reste ac, bc. La MSF par extrémités du graphe réduit complet {ab,ac,bc} choisit ab, ac. Les deux produisent **la même fusion ternaire à 1**. Ce témoin abstrait n’est pas présenté comme un census 3D réalisable.

Le numérotage public doit donc dériver de l’histoire canonique, des identités et des règles de première utilisation, pas de l’ordre des arêtes du certificat interne.

## 3. Variante en deux passes : certificats directement sur L naissances

Si A est beaucoup plus grand que L, on peut réduire le domaine des certificats avant leur émission :

1. Résoudre par fenêtres un pivot pour chacun des A−L hubs non naissants. Conserver seulement les pointeurs, puis calculer φ. Deux tableaux suffisent au doublement de pointeurs ; la valeur finale peut remplacer le pointeur.
2. Régénérer les représentants en omettant l’occurrence pivot, résoudre les R−A+L occurrences restantes et émettre directement `(φ(B),φ(T),λ_B)`.
3. Comprimer et réduire ces fenêtres sur les L naissances. Garder séparément les données nécessaires aux contributions, admissions et références inférieures.

Le total reste R occurrences sans cache ; les pivots ne sont pas résolus deux fois. La régénération et les comptages ne sont pas gratuits. L’atlas emprunté, les clés géométriques et les ordinals doivent rester immuables entre passes. La reconnaissance des naissances utilise les représentants stricts de l’atlas admis, sans dépendre du calendrier.

Dans les deux routes, le dédoublonnage global `(K,F triée)` est une optimisation du travail, pas une obligation de justesse. On peut dédoublonner dans chaque fenêtre et utiliser un cache borné entre fenêtres. **Une même facette présente dans plusieurs fenêtres peut alors refaire sa MEB** : compter les clés distinctes, les occurrences et les résolutions réellement exécutées. Les semis modifient le coût et le chemin de calcul ; chaque résultat doit conserver sa décision certifiée et la garde de terminal strict pour son consommateur. Budgets et compteurs restent cumulatifs sur toutes les fenêtres et les deux passes, y compris en cas de refus.

Le code actif épinglé matérialise au contraire `requests` puis `static_targets` sur les R occurrences dans `prepare_static_order`. Les deux propositions retirent cette nécessité architecturale, sans prétendre que le raccord est déjà codé. Elles conservent catalogue, atlas, marques et sortie explicite ; elles ne ramènent pas toute la mémoire à celle d’une MSF.

## 4. Résidence et parallélisme : choisir le calendrier de réduction

Soit M le nombre d’arêtes émises, b la largeur maximale d’une fenêtre, J leur nombre non nul et N le domaine des certificats : A pour la première route, L pour la seconde. Un certificat local contient fᵢ=|Vᵢ|−cᵢ arêtes, donc au plus min(|Eᵢ|,N−1) si N≥1. Un lot déjà forestier ne se comprime pas. La somme de tous les fᵢ peut rester de l’ordre de M.

| Réduction | Buffers d’arêtes et travail à prévoir |
| --- | --- |
| Accumulateur séquentiel, MSF(certificat ∪ fenêtre) | O(N+b) arêtes, mais peut retraiter jusqu’à N arêtes à chacune des J fenêtres. La résidence basse ne suffit pas à qualifier le temps. |
| Pile binaire, un certificat par puissance de deux du nombre de fenêtres | O(min(M,N(1+log J))+b) arêtes, buffers de fusion inclus à facteur constant. Ce n’est pas automatiquement O(N). |
| Réduction équilibrée de tous les lots en parallèle | Réductions disjointes indépendantes, mais tous les certificats de feuilles en attente peuvent retenir O(M) arêtes. Borner tâches actives et file des certificats, ou déclarer un stockage externe. |

Ce sont des bornes de **données d’arêtes**, pas de RSS : compter en plus sommets, remappage local, DSU/contractions, travail du tri/MSF, temporaires, catalogue, cache et export. Le juge Python initialise volontairement un DSU sur tout V par appel ; ce coût n’est pas une proposition industrielle. Le helper produit devra travailler sur les extrémités réellement présentes ou amortir ses initialisations.

Les MSF de fenêtres et les fusions de certificats disjoints offrent des tâches parallèles. La dernière réduction et la reconstruction FULL restent des calculs à réaliser ; les algorithmes MSF et dendrogramme de la note précédente s’appliquent à ces objets. Le théorème ne livre ni leur implantation ni leur débit. Des certificats intermédiaires peuvent accepter des lots hors ordre ; ils ne permettent pas de publier prématurément une ancienne coupe tant que des arêtes plus légères peuvent encore arriver.

## 5. Témoin et prochain raccord utile

Le [modèle](msf_model.py) compare les partitions par BFS indépendant de Kruskal : dix graphes de composition, largeurs 1/2/3/lot entier, lots directs ou inversés, stratégies équilibrée, accumulateur et pile binaire. Les poids comprennent des plateaux, des doublons, des boucles, des naissances futures et une arête légère tardive. Il vérifie aussi les multifusions globales et les identités exactes des arêtes sous une même clé totale. Des fixtures supplémentaires vérifient la projection tardive et le contre-exemple de départage ci-dessus.

Cinq mutants sont réfutés : forêt maximale arbitraire, sommets isolés perdus, arêtes antidatées, fusions locales publiées comme globales et arête légère tardive ignorée. Les compteurs exacts et la non-vacuité figurent dans [result.json](result.json). Python normal et `-O` doivent donner les mêmes octets ; aucune porte n’utilise `assert`.

Le premier raccord conseillé est la route en une passe sur les hubs : elle évite la barrière φ préalable et réutilise le flux original d’arêtes. Mesurer A/L et la résidence de ses certificats avant de retenir la variante à deux passes. Dans les deux cas, confronter sur les petits vrais census déjà disponibles les graphes/coupes, contributions datées, ancres, verticales et export canonique à Builder/T2. Conserver les négatifs déjà présents dans les paquets atlas et calendrier ; leurs premières gates sont désormais réalisées et ne sont pas à redemander comme nouvelles.

```bash
python3 -B morsehgp3D_v7/audits/receipts_composable_msf_20260911/msf_model.py
python3 -B -O morsehgp3D_v7/audits/receipts_composable_msf_20260911/msf_model.py
python3 -B morsehgp3D_v7/audits/receipts_composable_msf_20260911/verify.py
python3 -B -O morsehgp3D_v7/audits/receipts_composable_msf_20260911/verify.py
```

Le lecteur vérifie les captures et leurs sources sans réexécuter le modèle. Ce paquet ne certifie pas la géométrie du census, la complétude globale, les profils pondérés ou les coûts de plateaux ; il réutilise les prémisses FULL existantes, y compris la naissance terminale K=n. Les variantes historiques D–Q restent inchangées.
