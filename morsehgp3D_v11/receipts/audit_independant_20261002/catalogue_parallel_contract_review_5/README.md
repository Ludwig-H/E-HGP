# Catalogue : contrat de raccord parallèle après leaf16

Lecture indépendante du 2 octobre 2026. Produit figé `e6fe34cb082f19d0041c829dfb38ea249319ab19`,
identique à `f391bf13e` pour **tout src/**. Conception et développement copiés sur le chantier au
HEAD `665dff6f336d7360da2004dbee062cf3b87c253b`, avec double lecture hachée. Source_before/after
et SHA256SUMS ferment les pièces. Aucun build, test produit, GPU/GCP ou allocation massive.
Ce reçu apporte deux contrats de port, **aucun nouveau défaut du catalogue séquentiel**.

## 1. La frontière de jobs possède des listes, pas une partition des sites

[boxes.cpp](sources/product_e6/morsehgp3D_v11/src/catalogue/boxes.cpp) lignes 54–69 alloue chaque
liste à `parent.size()`, conserve les candidats puis transmet seulement son préfixe logique.
Les ancêtres restent vivants dans la récursion lignes 86–116. Réservoir et ajustement de boîte
relisent ces listes ; `filter_tests` ne compte ni les distances/insertions du réservoir (29–51),
ni les scans `envelope` (72–84), ni les copies/allocation des listes.

Les boîtes partitionnent les **centres**. Elles ne partitionnent pas les SiteIdx : le modèle exact
indépendant `MODEL.json/overlap`, cinq points `(x,0,0)`, x=0..4, K1, leaf4, donne les enfants
[0,2) et [2,5), avec listes {0,1,2} et {2,3,4}. La dominance est testée sur les coins des boîtes
fermées, indépendamment de l'expression produit ; le contact x=2 reste dans les deux listes.
Six éléments logiques dépassent les cinq sites ; les deux capacités allouées valent dix.
Cela interdit les bornes naïves « somme des listes de jobs <= n » ou « capacité = taille filtrée ».
Le modèle est analytique ; aucune exécution native de cette fixture n'est revendiquée.

Premier contrat de jobs proposé : frontière fixe et ordinals déterministes, boîte/depth copiés,
listes possédées par un Buffer ou une arène immuable retenue jusqu'au join. Les jobs peuvent
partager une même liste parent par handle propriétaire ; ne pas copier Cloud/index par job.
Un job démarre à l'entrée de process (avant le filtre), ou porte explicitement une continuation
ayant déjà payé le filtre : ne pas refaire les ancêtres. Ne pas envoyer le span d'un Buffer DFS
local dans une file puis libérer ce Buffer. Réduire les compteurs par somme vérifiée/max ; le
producteur et les workers ne comptent chaque nœud qu'une fois par passe.

## 2. Deux passes : admission concurrente et offsets globaux

[assemble.cpp](sources/product_e6/morsehgp3D_v11/src/catalogue/assemble.cpp) lignes 33–52 compte,
admet records+population, puis refait walk et compare les comptes/ledger. L'admission des sorties
ne couvre pas les futures allocations DFS. Le compte MemoryBudget reste sûr et le refus propre ;
[buffer.hpp](sources/product_e6/morsehgp3D_v11/src/core/buffer.hpp) lignes 76–84 précise que admit
n'est pas une réservation et exige la somme des buffers coexistants avant les tâches parallèles.

Notations : E=sizeof(Emission), s=sizeof(SiteIdx), A=sizeof(num::Point), B boules, P incidences,
r workers actifs, Q octets des buffers uniques de frontière + descripteurs/tables count/prefix,
W(c)=(A+2s)c+8c ceil(c/64), Tj pic de capacités DFS privées du job j, U préexistant.
Pour une frontière fixe sans subdivision asynchrone supplémentaire, une borne d'admission de la
seconde passe est `U + Q + E*B + s*P + r*W(c) + somme des r plus grands Tj`.
W et Tj restent distincts de Q ; il faut mesurer Tj au comptage, ou prouver une majoration.
La première passe peut encore refuser pendant la découverte de ces capacités, sans publication.
La capacité initiale c est min(n,max_leaf), **pas leaf_size** : une boîte unitaire peut terminer
avec davantage de candidats que leaf_size. Le max_leaf observé au comptage peut permettre une
capacité moindre au remplissage, à paramètres/continuations inchangés ; ce serait un port distinct.

Count donne Bj/Pj par ordinal. Préfixes u64 contrôlés avant somme/multiplication : les totaux doivent
respecter ball_limit exclusive et toutes les capacités. Fill écrit des plages disjointes fixes.
Attention au champ `Emission.population_begin` :
[catalogue.cpp](sources/product_e6/morsehgp3D_v11/src/catalogue/catalogue.cpp) lignes 47–62 écrit
l'offset courant du Collector ; finish le lit dans la population **globale**. Un Collector branché
naïvement sur subspans par job laisse cet offset local. Ajouter la base Pj, ou un contrat explicite
pour cette base. `MODEL.json/scatter` vérifie 24 ordres de complétion, des jobs vides et des plateaux
rationnels égaux ; le mutant sans base lit les mauvaises incidences. Ce n'est pas un bug actuel.

Ordre de raccord proposé :

1. Après l'élagage isolé envisagé par le développeur, attribuer count/fill/tri/assemblage et tracer
   Σ tailles parent, Σ tailles retenues, capacités DFS et réservoir ; garder la sortie exacte identique.
2. Porter frontière possédée et count/fill par ordinal, avec un worker d'abord puis plusieurs ;
   vérifier populations, compteurs et refus. Join tous les workers avant retour/libération/publication.
3. Libérer frontière, DFS et workspaces après fill, puis garder d'abord le tri global exact existant
   (niveau, S*) et les rangs d'égalité. Un futur tri parallèle réserve aussi ses tampons coexistants ;
   l'ordre d'arrivée ou un flottant ne décide jamais BallIdx/LevelRank. La tour consommera ensuite les
   plateaux globaux complets, pas des rangs locaux aux jobs.
4. Construire l'index global une fois avec le même Cloud pour les descentes/tour. Une liste certifiée
   pour une boîte ne devient pas un census global pour un centre extérieur ; ne pas remplacer sa
   coquille complète par un KNN tronqué. Aucun gain multicœur/index/tri n'est prévalidé ici.

`ABLATION.json` relit seulement les rapports publiés : pour 8k/K5, leaf32→16, par passe, préfixes
144086254→85489590, census 61812078→18529519, filtres 28939317→64529211, tests dominance
4313716→7345229, feuilles 12507→77934. Boules/niveaux/incidences/pic réservé et hash canonique
**déclaré** concordent dans les quatre succès. Les fichiers canoniques retirés de la VM ne sont pas
rehachés ici. Compteurs et durée totale ne permettent pas d'attribuer le gain à une phase ni de
qualifier FULL, une borne globale, un nouveau défaut mémoire ou une performance parallèle.

Lecture : `python3 -B judge.py` et `python3 -B -O judge.py`. Contrôles autonomes bornés et hashes
uniquement. Les résultats de MODEL ne constituent pas une qualification du futur code parallèle.

Premier lecteur refusé sur sa propre comparaison tuple Python/liste JSON ; script, flux et refus
conservés dans `initial_reader_failure/`. Correction : normalisation JSON du replay ; aucun défaut
produit ni résultat mathématique modifié.
