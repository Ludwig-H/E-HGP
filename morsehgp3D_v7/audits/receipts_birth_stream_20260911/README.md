# Contracter les pivots pendant le flux, puis Kruskal sur les naissances

11 septembre 2026. Preuve conditionnelle et modèle indépendant borné, après
contrelecture du [vrai producteur par fenêtres](../../receipts/streaming_graph_20260911/README.md),
publication **069bb6a2**. Cible : simplifier son réducteur mono. Le résultat
nouveau est le calcul immédiat de φ, qui évite même de construire le
certificat intermédiaire sur hubs proposé par le brouillon ordonné.

## 1. Contrat et algorithme

Fixer K. Le graphe daté sur hubs a A sommets, L naissances explicites et R
occurrences. À K1, les points font partie de A et L. Chaque autre hub B porte
au moins une occurrence, dont le pivot `local0`, dirigée vers une terminale
T strictement plus ancienne. **Chaque occurrence B→T porte exactement la
date de naissance de son hub B**, pas une date retardée. Le flux consommé
respecte les dates croissantes,
l’ordre source à égalité et les ordinaux locaux : `local0` précède toute autre
occurrence de B. Les garanties géométriques viennent du producteur admis ;
cette preuve ne les remplace pas.

Initialiser φ seulement sur les naissances, avec leur identité stable. Pour
chaque occurrence, dans cet ordre :

1. Lire φ(T), déjà défini par l’antériorité stricte de T.
2. Pour `local0(B)`, affecter φ(B)=φ(T) et n’effectuer aucune union.
3. Pour `localj(B)`, j>0, tenter l’union de φ(B) et φ(T) dans un DSU sur L
   naissances. Si elle réussit, conserver l’arête avec ses extrémités de
   naissance, sa date source et son ordinal original.

**φ est une identité de naissance, jamais une racine DSU.** Le DSU peut se
comprimer librement dans son propre tableau. Les références de contributions,
les marques, les ancres historiques et les verticales restent attachées aux
identités originales, même après une fusion.

Les fenêtres peuvent couper un hub : φ(B) reste disponible entre fenêtres.
Le tri des requêtes pour dédupliquer les MEB est compatible avec cet algorithme
si les résultats sont ensuite consommés dans l’ordre source, comme dans le
producteur lu. Leur ordre d’arrivée asynchrone ne satisfait pas ce contrat.

## 2. Preuve par préfixe, puis par coupe

Comparer avec Kruskal sur le graphe complet des hubs, départagé par
`(date,ordinal)` dans le même flux. Avant `local0(B)`, B est isolé dans sa
forêt : il n’a encore émis aucune arête, et aucun hub de date inférieure ou
égale ne peut le viser, car toute terminale est strictement antérieure.
Le pivot est donc accepté.

L’invariant de préfixe porte sur les hubs déjà mappés : deux d’entre eux sont
connectés dans la forêt des hubs si et seulement si leurs images φ sont
connectées dans le DSU des naissances. Les hubs sans pivot encore consommé
restent des singletons non mappés. Les naissances, même futures ou isolées,
ont une identité explicite dès l’initialisation.

Au pivot, seul un nouveau hub rejoint une composante existante ; étendre φ
préserve l’invariant sans union de naissances. À toute autre occurrence,
l’invariant donne exactement la même décision d’acceptation dans les deux
DSU. L’induction conserve donc les mêmes arêtes non pivots, ordinaux et dates.

Les pivots sont une forêt P de A−L arêtes incluse dans la forêt F de Kruskal,
qui contient A−C arêtes, C étant le nombre final de composantes. Chaque fibre
de φ est un sous-arbre de P. Contracter P dans F laisse une forêt de L−C
arêtes, exactement celle produite directement. Aucun nouveau calcul de MSF
après projection n’est nécessaire. L’identité du certificat vaut pour ce
même départage ; un autre ordre dans les égalités peut choisir d’autres arêtes.

Pour toute coupe ouverte ou fermée, les arêtes et naissances actives gardent
leurs dates. Les pivots d’un hub actif sont déjà actifs, ainsi que leurs
chaînes strictement descendantes. La contraction conserve donc les composantes
datées sur les naissances. Au milieu du traitement d’un plateau, ne pas
publier sa coupe fermée avant consommation complète du plateau. Les événements
multifusions se reconstruisent ensuite par lots de date dans le reconstructeur
existant. K=n sans occurrence conserve sa naissance isolée ; un ordre vide
reste vide. Aucun lien vertical ou poids du manuscrit n’est inventé ici.

## 3. Raccord concret à la source lue

Dans `streaming_graph.hpp`, SHA256 `6f341cff`, `build` initialise déjà les
naissances avant les fenêtres. Les lignes 213–227 de la copie scellée consomment
`requests[j]` dans l’ordre original après la résolution triée. C’est le point
d’insertion de la contraction. La garde de terminale strictement antérieure
est déjà appliquée à chaque consommateur, même lors d’un hit de déduplication.

Pour éviter les recherches binaires d’extrémité par occurrence :

- Donner aux naissances de K des indices denses stables dans
  `order.graph.births`. À K1, les terminales sont des indices géométriques de
  points, pas des `PointId` externes ; garder cette convention.
- Stocker ces indices dans les slots φ de l’atlas. Pour K≥2, la terminale
  `BallId` donne son slot par `atlas.cell(target,K)`, puis son indice φ en O(1).
  Contrôler admission, sentinelle, bornes et stricte antériorité avant l’accès.
- Allouer le DSU sur L. Lors d’une union retenue, écrire directement les
  `births[d].id` natifs dans le certificat ; ne pas y écrire `find(d)`.
- Avant de livrer l’extraction à `graph_full`, convertir φ en `BlockId`
  natifs et émettre toutes les marques. Leur écriture O(A) reste nécessaire ;
  la passe récursive de résolution des pivots disparaît.

`fc::Certificate` est un agrégat. Le juge et le reconstructeur appellent
`normalized`, qui trie des copies des naissances, arêtes et marques. Ne pas
trier en place `order.graph.births` pendant l’usage de ses indices denses.
`graph_full` attend encore des φ en identifiants natifs : la conversion est
une frontière d’API explicite. Conserver les terminales BallId dans les
buffers de géométrie, semis, caches et observateur ; celui-ci doit encore voir
toutes les occurrences, pivots et boucles compris. Les indices denses sont
propres à K ; ne pas
les partager implicitement entre ordres ni les confondre avec les BlockId.

Le tableau `hubs`, son domaine, `BinaryPile`, le certificat A−C et sa
projection/compaction finale peuvent alors disparaître de cette voie mono.
Les statistiques doivent être renommées selon les opérations réellement
exécutées, sans attribuer à ce chemin des tris ou compactions supprimés.
Le prototype C++ `OrderedReducer`, SHA256 `852627a5`, est un brouillon lu,
non qualifié par ce paquet. Le modèle ci-dessous est indépendant de ce C++.

## 4. Travail supprimé et travail conservé

Le nouveau réducteur effectue A−L affectations de pivots et R−A+L tentatives
d’union, et conserve L−C arêtes. Avec indices denses, union par rang/taille et compression des chemins, son
travail est borné
par O(A+L+R α(L)), hors construction du flux, avec résidence de φ[A],
correspondance des naissances[L], DSU[L] et certificat[L−C]. Les atlas/masques,
fenêtres, semis, marques, historiques et la sortie FULL restent à compter.
Les mêmes occurrences géométriques sont résolues dans les mêmes fenêtres :
ce changement de réducteur ne promet aucune diminution du nombre de MEB.

Sur les seuls compteurs du reçu constructeur mono n8000, s8, K1..10 :

| Quantité cumulée | Valeur |
| --- | ---: |
| A / L / C | 5 518 027 / 2 404 646 / 10 |
| R / pivots A−L | 10 456 312 / 3 113 381 |
| Tentatives d’union déduites pour la voie directe | 7 342 931 |
| Tentatives observées, ancienne pile sur hubs | 48 390 815 |
| Tentatives observées, ancienne compaction native finale | 3 910 849 |
| Arêtes finales L−C | 2 404 636 |

La ligne « voie directe » est un **calcul conditionnel**, pas une exécution
sur n8000. Les lignes anciennes proviennent du reçu scellé, dont l’objet
`results.json` est épinglé et relu par notre lecteur. Aucun ratio de temps
n’en est déduit. Le bilan publié reste négatif : 188,638222200 s pour la
référence contre 250,407612046 s pour le flux, index jusqu’à FULL retenu,
et 2 731 664 contre 2 770 676 KiB de RSS processus. Le développeur a raison
de corriger le réducteur avant de prolonger cette variante à n16k/n32k.

## 5. Validation et suite utile

Le [modèle autonome](model.py) compare la contraction immédiate à Kruskal
hors ligne sur hubs suivi d’une projection. Un juge BFS séparé confronte
leurs coupes ouvertes et fermées au graphe complet. Les fenêtres coupent
aussi des hubs ; les témoins incluent des égalités de dates, des naissances
futures/isolées et des terminales répétées. Les mutants protègent l’ordre,
l’antériorité et l’identité native de φ. Le corpus compte **14 cas, 42 essais de fenêtres, 3 276 comparaisons BFS et
neuf rejets causaux**. Le modèle alloue les naissances à leurs en-têtes
chronologiques, variante équivalente aux naissances préallouées mais inactives
avant leur date. Il matérialise les graphes et candidats pour les comparer :
sa résidence Python ne mesure pas la borne de l’algorithme proposé.
Les [captures](commands.json) et le
[résultat](result.json) distinguent les deux modes Python normal/−O.

```bash
python3 -B morsehgp3D_v7/audits/receipts_birth_stream_20260911/model.py --selftest
python3 -B -O morsehgp3D_v7/audits/receipts_birth_stream_20260911/model.py --selftest
python3 -B -O morsehgp3D_v7/audits/receipts_birth_stream_20260911/verify.py
```

La suite utile au constructeur est le raccord sur son flux réel : observer
les mêmes terminales, comparer φ natifs, marques et toutes les coupes,
puis les payloads FULL par bijection et la convention physique déclarée.
Mesurer ensuite la même paire mono isolée. La qualification géométrique,
le débit et le RSS de cette spécialisation restent ouverts.

Cette spécialisation retire le travail redondant de la voie mono ordonnée.
Les fenêtres indépendantes distribuées hors ordre continuent de relever de
la [composition de certificats](../receipts_composable_msf_20260911/README.md).
Les [horizontales indépendantes et verticales par requêtes](../receipts_parallel_objects_20260911/README.md)
restent compatibles avec ce choix de réducteur local ; aucune qualification
de parallélisme massif n’est acquise par le présent modèle.

`public_status=not_claimed`. GCP non utilisé.
