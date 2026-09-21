# Pourquoi le calcul global à8k a pris22 minutes

La campagne `global_tsd9ofnm` reste **FAILED**, interrompue pendant16k.
Sa première commande, à8k, est terminée :1360,996439s, soit22,683274min,
sur CPU local avec quatre workers, K5, s8, q3+q4, backend q4 Local28.
32k n'a pas été commencé. Ce n'est ni une campagne8/16/32 validée,
ni une mesure G4, ni toute la tour HGP.

Le script [analyze_partial_global.py](analyze_partial_global.py) vérifie
le plan complet de trois commandes, les deux reçus réellement présents,
leurs sorties brutes/base64, leurs hashes et tous les champs du résultat8k
avec le lecteur constructeur existant. Le lecteur officiel refuse toujours
la campagne incomplète. Les résultats [normal](PARTIAL_GLOBAL_8K.json) et
[−O](PARTIAL_GLOBAL_8K_OPTIMIZED.json) sont identiques octet pour octet.
Aucun moteur n'a été réexécuté.

Les197 sources historiques, trois artefacts et quatre entrées ont des
empreintes de début/fin identiques dans la capture. Les artefacts et entrées
sont encore identiques aujourd'hui. La seule source modifiée depuis est
la gate C++ corrigée pour Boost ; son ancienne version est archivée et
son hash vérifié. Cette modification ultérieure ne requalifie pas la ligne8k.
Toutes les empreintes courantes sont également identiques avant/après
notre analyse. Le résultat8k fournit des registres et un digest, pas les
supports complets : il n'a pas ici d'oracle géométrique global indépendant
ni de second calcul apparié confirmant son digest.

## La multiplication des candidats explique le volume de travail

| Poste réellement exécuté | Compte |
|---|---:|
| Paires possibles du nuage |31 996 000|
| Rectangles WSPD résiduels |352 701|
| Arêtes développées, union q3/q4 |2 285 750|
| Couvertures préparées |2 285 750|
| Somme des sites des couvertures |5 113 906 118|
| Visites de nœuds pour préparer ces couvertures |927 106 186|
| Triangles q3 aigus et propriétaires, donc boules construites |780 661 556|
| Tests ponctuels dans leurs census q3 |361 201 093 303|
| Dont strictement extérieurs |357 937 525 263|
| Triangles q3 rejetés à profondeur4 |780 567 642|
| q3 émis |93 914|
| Familles q4 demandées |439 969 682|
| Visites dans l'atlas lors de ces requêtes q4 |6 991 494 882|
| Visites de nœuds pendant la construction des partitions q4 |6 337 495 886|
| IDs de frontière copiés pendant cette construction |3 380 526 738|
| Tests de sites dans les balayages q4 finaux |16 016 751|
| Comparaisons de tri d'événements q4 |16 402 922|
| q4 émis |10 756|

Ces compteurs ne sont **pas des durées** et plusieurs postes sont imbriqués :
les additionner donnerait un total artificiel. Il n'existe pas dans cette
capture de chronomètre séparé par voie ni par étape.

Le front retire92,856% des paires pour les deux voies. C'est un rejet réel,
mais les2,286M arêtes restantes portent encore2237 sites en moyenne dans
leur couverture —28% de tout le nuage, avec un maximum de8000. Une couverture
est une grande région certifiée contenant les boules possibles, pas une
petite liste de voisins proches. Les5,114Md sites sont une masse cumulée,
pas cinq milliards de sites stockés.

La chaîne q3 est particulièrement coûteuse :

`2,150M arêtes q3 ×363,08 triangles/arête ×462,69 tests/triangle ≈361,201Md tests`.

Le code `q3_seed` parcourt individuellement les sites des plages de couverture,
dans l'ordre spatial de l'index, jusqu'au quatrième intérieur strict ou à
la fin. Il n'utilise pas de bornes de blocs adaptées à la boule q3 pendant
ce census.99,096% des tests rencontrent un site extérieur. Le rejet anticipé
épargne déjà88,033% du volume qu'aurait lu un parcours intégral ; il reste
cependant361Md tests.99,98797% des triangles construits finissent rejetés :
8312,52 constructions et3,846M tests pour chaque q3 émis.

La voie q4 fait mieux en aval : seulement672 641 requêtes de feuilles et
16,017M sites de balayage après439,970M requêtes de familles. Le rapport
feuilles/familles vaut0,153%, mais ce n'est pas un taux exact de familles
survivantes — une famille peut visiter plusieurs feuilles. La construction
et la consultation de l'atlas restent très chères ; les3,381Md copies de
frontières se font avant la masse finale de4,081M sites actifs des feuilles.
Ce dernier petit résidu ne mesure donc pas le coût de l'obtenir.

## Le tri n'est pas le travail dominant en nombre d'opérations

La totalité des coquilles q3 acceptées demande327 815 comparaisons de tri,
contre361Md évaluations de puissance pour les census. Les événements q4
demandent16,403M comparaisons de tri. Ces opérations n'ont pas toutes le
même coût, mais leur échelle ne permet pas d'attribuer les22min à un énorme
tri final. Cette entrée ne calcule même pas encore le catalogue dédupliqué,
Kruskal ou les parents des hiérarchies HGP.

La préparation commune nuage+index prend3,448737ms ; la fusion/normalisation
des digests prend0,000471ms. Le mode `digest` ne stocke aucun tableau complet
de supports. La quasi-totalité du temps mesuré englobe front, développement
des arêtes, calcul des candidats et callbacks : leur ventilation temporelle
est inconnue. Le contexte de charge partagée interdit un gain CPU stable
ou une extrapolation à50k/G4 à partir de ce seul chrono.

## Parallélisation : ce que montrent réellement les quatre workers

| Worker | Jobs | Visites front | Arêtes | q3 | q4 |
|---|---:|---:|---:|---:|---:|
|0|55|738 594|959 555|65 349|6 087|
|1|3|41 665|355 006|772|58|
|2|2|317 081|688 041|27 793|4 611|
|3|4|6 260|283 148|0|0|

Les64 jobs sont tous terminés ;44 visites du préfixe s'ajoutent aux visites
privées. Les arêtes sont réparties de12,39% à41,98% du total. Le nombre de
jobs, le nombre d'arêtes et le nombre de sorties ne sont pas des temps CPU :
une arête sans sortie peut coûter très cher avant son rejet. Les données ne
mesurent ni les temps par worker ni leurs attentes ; elles ne prouvent donc
pas un facteur précis de déséquilibre temporel ou d'accélération W1/W4.
Les arêtes et leurs sous-calculs restent atomiques dans ce répartiteur.

Les223 936octets rapportés pour les buffers d'arête sont la **somme des pics
privés**, pas un pic simultané ni le RSS. Nuage et index partagés retiennent
respectivement240 000 et981 504octets ; le faible stockage ne supprime pas
les lectures et calculs répétés.

## Priorités algorithmiques proposées, pas encore validées ici

1. **Remplacer le census scalaire q3 par un census exact par blocs.**
   Réutiliser l'index global : bloc strictement extérieur ignoré, bloc
   strictement intérieur compté en une fois, bloc ambigu subdivisé. Saturer
   au seuil4 ; collecter la coquille entière en cas d'acceptation. Les bornes
   de puissances exactes et leurs largeurs arithmétiques sont à prouver et
   tester, pas à reprendre implicitement de q2.
2. **Éviter de construire les780M triangles finalement presque tous profonds.**
   Chercher des certificats communs à un bloc de troisièmes sommets ou à un
   rectangle WSPD, avec seuil q3 explicite. Un meilleur ordre de témoins peut
   aider mais ne retire pas, à lui seul, le facteur « nombre de seeds ».
3. **Partager ce qui est vraiment commun à q3/q4.** Sur1 223 684 arêtes,
   les deux voies survivent. Leur génération actuelle répète les recherches
   de triangles aigus/propriétaires. Une génération partagée est une piste,
   à condition de conserver les deux masques : q4 ne dépend jamais d'un
   q3 accepté. Les compteurs globaux ne donnent pas exactement le gain futur.
4. **Interroger les régions peu profondes q4 avant d'énumérer chaque seed.**
   Les millions de feuilles rejetées en bloc sont utiles, mais leur effet
   arrive après la génération de439M familles. La sélection par blocs des
   droites de centres et la réduction des frontières recopiées sont à
   comparer au coût total de construction ; préserver les tangences exactes.
5. **Paralléliser les continuations coûteuses après réduction du travail.**
   Une file fine d'arêtes ou de blocs peut mieux répartir ces jobs ; elle
   doit partager préparation/index et ne pas répéter les preuves. Répartir
   les361Md tests entre davantage de CPU/GPU ne règle pas leur croissance.

Aucune croissance8→16→32 ne peut être calculée :16k n'a produit aucun
résultat. Cette ligne réfute l'idée que les petits coûts mesurés sur neuf
arêtes productives suffisaient à prévoir le coût global. Elle ne prouve,
à elle seule, ni un exposant asymptotique ni une borne sous-quadratique.
