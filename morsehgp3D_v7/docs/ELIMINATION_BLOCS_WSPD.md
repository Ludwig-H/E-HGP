# Élimination par blocs WSPD : témoins et travail évitable

6 septembre 2026. Priorité de génération demandée par l'utilisateur.
`public_status=not_claimed`, CPU de référence, entrée u16. Cette note
distingue renforcer les rejets et calculer moins cher les rejets existants.

## Les trois populations à garder disjointes

Pour un rectangle A×B, les minorants actuels sont :

| Crédit | Population de témoins | Universalité requise |
| --- | --- | --- |
| h_cœur(A,B) | Nuage privé de A∪B | Toutes les ancres a∈A, b∈B |
| h_a(a;B) | A privé de a | Tous les b∈B, a fixé |
| h_b(b;A) | B privé de b | Tous les a∈A, b fixé |

Un témoin est **strictement intérieur** à toute boule de la lane possédée
par l'ancre. La [preuve du front](../audits/FRONT_ET_TEMOINS_COURANT.md)
et [S1](../audits/S1_COURANT.md) justifient les fuseaux, les coins et
l'addition de ces crédits. Pour q∈{2,3,4}, le seuil de mort est
$h_q=s_{max}-q+1$. Le paramètre de séparation WSPD s=8/10/12 est distinct.
L'ancre meurt dès que h_cœur+h_a+h_b≥h_q. Les masques de lanes restent
séparés : l'emboîtement des fuseaux ne suffit pas à identifier les seuils.

## Ce qui est déjà fait, ce qui est encore répété

Le [générateur](../src/pipeline/generate.hpp) ferme une lane du rectangle
dès que son cœur atteint h_q, avant de scinder. Après séparation, il
complète ce cœur avec les coins. Dans le corps du rectangle, il construit
les histogrammes, saute toute ligne avec h_a≥need, puis teste encore
h_b pour chaque paire de chaque ligne restante, où need=h_q−h_cœur.
Ces rejets précèdent déjà le cover, les seeds et les complétions.

Les lignes de même crédit h_a ont exactement la même sélection de B :
h_b<need−h_a. Classer les crédits ou réutiliser une liste filtrée permet
de sauter les produits de classes rejetées au lieu de tester leurs paires.
Un tri par score peut changer l'ordre des ancres ; des listes stables
permettent de préserver Morton. Ce levier réduit le travail de sélection,
**pas le nombre d'ancres géométriquement survivantes**.

Avec un counting-sort par crédit écrêté à need, la sélection après
histogrammes peut coûter O(|A|+|B|+need+survivants), en énumérant les
produits de classes admissibles. Cela ne supprime pas le coût actuel des
histogrammes eux-mêmes, O(|A|²+|B|²). Saturer un histogramme à need est
sûr : seules ses valeurs strictement inférieures peuvent être transmises
à une ancre survivante, et celles-ci doivent rester exactes.

## Sous-blocs : un certificat sûr, et un faux raccourci

Pour A′⊆A et B′⊆B, conserver les **populations parentales** donne le rejet :

$$h_{\text{cœur}}(A,B)+\min_{a\in A'}h_a(a;B)+\min_{b\in B'}h_b(b;A)\geq h_q.$$

Chaque paire du sous-produit satisfait alors le même seuil. Les témoins
des deux histogrammes peuvent dépendre de l'extrémité ; il suffit que le
minorant soit valable pour chaque paire, avec disjonction des populations.

En revanche, sur le rectangle **entier**, min_A h_a=min_B h_b=0. Fixer
b∈B et choisir a∈A le plus proche de b : un témoin z∈A dans W₂(a,b)
vérifierait |z−b|²<|a−b|², contradiction. Les autres fuseaux sont inclus
dans W₂. Le même argument vaut côté B. Ces minima globaux n'améliorent
donc jamais le cœur seul ; les sous-groupes par crédit sont essentiels.

Recalculer le cœur d'un enfant puis ajouter les histogrammes du parent
est généralement faux. Sur l'axe x, prendre A={0,1}, B={10,11}, puis
A′={0}, B′={11}. Le nouveau cœur compte les deux points 1 et 10 ; les
histogrammes parentaux les comptent encore une fois chacun. La somme
donne quatre témoins au lieu de deux. Avec smax=4/q2, le seuil trois
éliminerait à tort une boule de rang quatre. Ce contre-exemple concerne
le certificat géométrique local, pas une qualification moteur.

Solutions correctes : conserver l'exclusion A∪B pour le nouveau cœur,
recalculer une partition complète des témoins pour l'enfant, ou combiner
par maximum des minorants susceptibles de recouvrement. Ne jamais
additionner simplement leurs valeurs en perdant leurs populations.

## Renforcer le certificat sans énumérer les complétions

Un compte de ligne h_ligne pour {a}×B, hors {a}∪B, peut capter des témoins
manqués par le cœur du rectangle. Il recouvre potentiellement h_cœur+h_a,
mais reste disjoint de h_b. Le minorant sûr devient
max(h_ligne,h_cœur+h_a)+h_b. Le coût d'une nouvelle requête par ligne
doit être confronté au travail qu'elle élimine, pas payé systématiquement.

Pour calculer les histogrammes par sous-arbres de témoins, la piste est
un crédit universel pour un bloc Z, avec a fixé et b dans sa boîte. Le
fuseau est convexe en z et séparément convexe en b : tester tous les
coins de ces deux boîtes fournit un certificat suffisant strict. Un
minorant de H et un majorant de Ξ fournissent un test moins précis :
H_min>0 et t H_min²>Ξ_max, avec t=3 pour q3, t=2 pour q4. Ξ_max se calcule
par intervalles des composantes du produit vectoriel. L'[auditeur a prouvé
le certificat et borné son arithmétique i64/i128](../audits/receipts_block_histograms_20260906/README.md).
Les carrés se calculent après conversion en i128 ; les égalités ne
créditent rien. Le premier prototype fixe a, crédite un sous-arbre entier
certifié et descend sinon, jusqu'aux tests ponctuels actuels. Compter les
positions, comme les histogrammes actuels, et non leurs multiplicités.
Ce n'est pas encore un nouveau chemin produit qualifié.

Deux raccourcis sont exclus par la contrelecture. La boule-cœur centrale
actuelle ne peut créditer aucun témoin interne à A/B en q3/q4 lorsque s≥8 :
elle est trop éloignée des facteurs, même après restriction des ancres.
Ensuite, hmax4_boxes(U,B,Z)≤0 avec une boîte d'ancres U variable ne permet
pas de jeter Z pour chaque ancre de U ; il suffit qu'une ancre soit
défavorable. La contre-fixture séparée s8 donne une ancre avec H=384 et
Ξ=160000 malgré le majorant −816 du bloc. Le rejet reste valable à a fixé.

La saturation ou le crédit par blocs devront mesurer les nœuds visités,
blocs crédités et tests ponctuels réellement payés. Le volume logique
|A|(|A|−1)+|B|(|B|−1) ne représenterait alors plus ce travail physique.
Les listes stables gardent l'ordre de A et de B ; regrouper A par score
change le préfixe d'émission. Leur stockage temporaire doit aussi être
mesuré, et seules les listes de seuils effectivement demandées sont utiles.

## Première économie indépendante : ne pas recompter q2

Le premier compte du rectangle calcule déjà q2 entièrement. Le booléen
`with_corners` ne change que q3/q4. Au terminal, réutiliser le compte q2
et limiter le second parcours aux lanes q3/q4 encore ouvertes ne change
donc ni les cœurs transmis ni les décisions. Cette petite variante est
intégrée au générateur. Le [différentiel et le gate permanent](../receipts/wspd_terminal_q2_reuse_20260906/README.md)
passent en O2 et ASan/UBSan : 174 appels par bras, mêmes objets et comptes
de coins, 1 283 visites évitées sur les fixtures. Elle ne renforce pas le
certificat et cette qualification locale n'est pas un gain de temps.
Le [rejeu CTest ciblé](../receipts/wspd_q2_ctest_20260906/README.md) ferme
ensuite 19/19 tests Release, sans réattribuer la suite globale.
L'auditeur a ensuite fait renforcer le gate par un cœur q2 valant exactement
un, et non seulement inférieur au seuil. Le [supplément O2/SAN](../receipts/wspd_q2_positive_core_20260906/README.md)
passe et réfute le mutant qui omet le transfert de cette valeur. Les
premiers 19 CTests restent rattachés à leur version antérieure du gate.

La [mesure fraîche 8k](../receipts/full_wspd_q2_separation_20260906/README.md)
donne 131,482 s contre 133,038 s avant ce delta. Les visites et coins du
front fusionné sont identiques : q3/q4 imposent encore les parcours. Cette
variation de −1,17 % sur deux passages ne prouve pas une accélération
robuste. Les mêmes sources donnent 132,138 s à s10 et 137,247 s à s12,
avec les mêmes dix forêts ; les coins diminuent mais les visites augmentent.

Le prototype de terminal à un seul comptage avec coins a ensuite été
testé séparément, sans intégration ([captures conservées](../receipts/wspd_terminal_once_negative_20260906/README.md)) : 174 fronts identiques, puis les
754 686 rectangles du cas uniforme 8k identiques littéralement. En O2,
le front seul prend 37,767→38,287 s sur une paire non répétée. Les visites
baissent de 563 616 452 à 547 864 549, mais les coins passent de
167 115 088 à 335 509 837 : les lanes auparavant tuées par le passage
économique paient maintenant les coins. Cette variante n'est pas retenue
en l'état ; son coût n'est pas comparé au binaire FULL O3 de 131,482 s.

Le [prototype d'histogrammes par blocs](../receipts/wspd_histogram_blocks_20260906/README.md) à ancre fixée est correct sur 126
comparaisons O2/SAN, puis sur 8 436 096 valeurs comparées littéralement
à 8k/s8. Mais le passage instrumenté donne 93,819 ms au scalaire,
186,560 ms aux blocs forcés et 101,318 ms au dispatch scalaire jusqu'à huit.
Le chronométrage inclut les appels, la copie des résultats et les compteurs,
pas seulement le prédicat géométrique. Aucun facteur ne dépasse huit
points (maximum observé : sept) : le dispatch à huit n'active donc jamais les blocs sur ce cas.
La route courante reste scalaire ; le certificat de blocs conserve son
intérêt potentiel pour de grands facteurs, non rencontrés ici. Il ne faut
pas déplacer l'effort vers un poste de l'ordre de 0,1 s en supposant qu'il
explique les dizaines de secondes du front.

Ce lot uniforme est clos. Un régime distinct mérite toutefois la mesure
des grands facteurs : deux amas compacts très éloignés forment un rectangle
racine séparé A×B avec h_cœur=0, puisque A∪B=X. Ses histogrammes scalaires
coûtent alors |A|²+|B|² ; le résultat uniforme ne permet pas d'écarter le
certificat par blocs dans ce régime. Une sonde séparée 8k/16k/32k doit
trancher ce point, pendant la préparation du raccord multi-CPU.

Viennent ensuite les essais multi-CPU locaux,
puis G4 SPOT 48 CPU et GPU autorisés par l'utilisateur, avec des mesures
de bout en bout distinctes des composants.

Les [mesures du triplet](RESULTATS_MONO_FULL_SANS_QUOTAS_20260906.md)
motivent ces travaux : à 32k, le front WSPD paie 151,786 s et les corps
des rectangles 132,697 s. Aucun gain de ces nouvelles pistes n'est déjà
inclus dans ces chiffres. Aucun usage GCP.
