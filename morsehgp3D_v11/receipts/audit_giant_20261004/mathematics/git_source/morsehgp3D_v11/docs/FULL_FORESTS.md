# Forêts FULL exactes et verticales

La version initiale est qualifiée à `c6ca345e0` dans la
[capture FULL](../receipts/full_20261002/README.md). Elle construit les arbres de
composantes de la tour K1..K et leurs applications verticales. Les attaches core/cover,
la hiérarchie de points et HDBSCAN restent des objets distincts. Aucun temps FULL
ni contrat de 200 ms n'est acquis par les tests Python décrits ici.

## Domaine et API

`FullTower` possède un `FullDomain` et au plus douze `OrderForest`. Sa factory
`build_full` termine toutes les forêts et verticales avant le déplacement final du
domaine. Un refus conserve l'entrée et rend les réservations de l'appel ; les budgets
d'origine et de construction survivent au résultat. K>n est explicitement refusé
avec `parameter_out_of_range`, avant travail de forêt et avant transfert.

Les identifiants de nœuds sont locaux à un ordre. Les naissances sont triées par
(niveau exact, centre lexicographique exact), les fusions par (niveau, plus petite
naissance descendante). L'ordre de Morton et le support canonique ne remplacent pas
le centre. Le [tri par cohortes](FULL_BIRTH_RUNS.md) utilise les rangs déjà ordonnés
du catalogue. Seules les naissances de rang égal, au moins deux, préparent une
Sphere ; `num::compare_centers` compare leurs coordonnées rationnelles exactement.
À K1, les coordonnées entières XYZ sont comparées directement. Aucun flottant ni
PGCD supplémentaire. Cette optimisation attend sa qualification native propre.

Un nœud contient son rang de niveau, son parent, sa plage d'enfants et, pour une
naissance, son SiteIdx à k=1 ou son BallIdx à k>1. Les enfants sont triés et une fusion
a au moins deux enfants. Le lookup naissance possède b paires (clé,NodeIdx) triées,
soit 8b octets sur l'ABI visée par les portes. Il ne confond pas ordre des clés
et numérotation canonique. Les déplacements vident capacités, comptes et vues source.

## Plateau atomique

Toutes les cellules de la fenêtre sont classées. Les cellules à traces strictes
sont ensuite rejouées par niveaux du catalogue. Chaque trace est résolue par
`descend` ; sa **date initiale** doit être strictement inférieure au niveau du plateau.
Le seul niveau terminal ne suffit pas. La naissance trouvée est ramenée au DSU courant.

À première visite, une ancienne composante entre dans `touched` et dans une chaîne
singleton. Les unions concatènent des chaînes disjointes d'anciennes composantes ;
le champ `top` reste celui d'avant le plateau. La racine DSU est la plus petite
naissance canonique. À la clôture seulement, les racines survivantes de `touched`
sont triées : une chaîne d'au moins deux anciennes composantes produit une unique
fusion n-aire ; une seule composante produit une continuation sans nœud.
Les enfants reçoivent alors leur parent et `top` est remplacé.

La surjection T2/T6 des traces sur les morceaux locaux suffit : plusieurs traces
peuvent donner la même composante globale, éliminée par le DSU. Aucune fusion binaire
intermédiaire n'est publiée sur un plateau. Aucun balayage de toutes les naissances
n'est fait à chaque niveau ; remise à zéro et clôture visitent seulement `touched`.
Les cellules sont toutefois scannées dans le catalogue à chaque ordre.

## Verticales fermées

Pour une naissance d'ordre k>1, on choisit une (k−1)-partie de sa population fermée,
on descend puis on relève la graine dans la forêt k−1 au niveau de la naissance.
La date initiale est contrôlée ≤ ce niveau. `ancestor_closed` suit les parents dont
le niveau est **≤**, avec égalité incluse. Pour chaque fusion, toutes les images des
enfants sont relevées au niveau de fusion puis comparées ; une image divergente
refuse `tower_invariant`. La forêt inférieure est déjà complète.

## Capacités et travail

Pour b≥1 naissances, il y a au plus b−1 fusions et 2b−2 arêtes. Les buffers retiennent
les capacités 2b−1 nœuds et 2b−2 enfants, avec des vues limitées aux tailles logiques.
Les calculs sont effectués en u64 ; b≥2^31 est refusé avant multiplication afin que
chaque NodeIdx valide reste strictement sous la sentinelle. Une porte scalaire teste
ces limites sans fabriquer un nuage gigantesque.

Avec Ncap=2b−1 et Ecap=2b−2, la sortie d'un ordre réserve
`R=sizeof(ForestNode)*Ncap+4*Ecap+8*b`, plus `4*Ncap` pour ses verticales si k>1.
`sizeof(ForestNode)=24` est contrôlé par la porte native. Les capacités excédant
les tailles logiques restent comptées. Les étapes coexistantes sont :

- classification : B octets de drapeaux et états fixes sans traces ;
- tri des naissances : B+R+C*sizeof(BirthRecord), records libérés ensuite ; C est
  la plus grande cohorte de naissances de rang égal si elle a au moins deux
  membres, zéro sinon et toujours zéro à K1 ;
- plateaux : B+R+b*(sizeof(ForestState)+4), une cellule et son census temporaire ;
- verticales : R+4*Ncap, le census temporaire et 12Nlower octets de balayage DSU.

Les forêts déjà construites restent réservées à toutes les étapes suivantes. Chaque
cellule réserve 52S octets sur l'ABI visée, pour ses S traces. Un census de descente
réserve 4k si saturé, sinon 4(p+m) ; un hit catalogue n'en réserve aucun. Ces termes
s'ajoutent lorsque la cellule reste vivante pendant la descente. Le budget décrit les
Buffer, pas le RSS ni les états fixes ; le propriétaire domaine se compte séparément
s'il relève d'un autre budget.

Les ledgers séparent désormais classification et rejeu réellement exécutés,
MEB, descentes, visites de plateaux et requêtes du balayage. Les changements
de travail, mémoire et diagnostics sont détaillés dans
[FULL_OPTIMISATIONS.md](FULL_OPTIMISATIONS.md), qualification distincte requise. `birth_presentations` compte les sphères réellement préparées pour le tri,
donc zéro à K1 et pour les cohortes singleton ;
`center_comparisons` compte seulement les appels au comparateur de centres à rang égal.
La recherche canonique globale de `locate` n'a pas encore son compteur de tuples.
Le coût peut rester combinatoire dans les coquilles ; marches de parents et descentes
n'ont aucune borne globale rapide annoncée. Aucun mémo, quota de chemin ou cache implicite.

## Preuves et portes préparées

Les principes de `_forest`/`_build` de la référence constructive sont repris
explicitement ; les listes `touched` et chaînes remplacent ses groupes Python.
`source_pins.json` identifie les sources lues, sans transfert de qualification.
L'oracle charge seulement `definition.py` et `model.py` : toutes les k/(k+1)-parties,
parents, numérotation canonique, coupes ouvertes/fermées et verticales. Il contrôle
également les traces par géométrie Fraction pour les compteurs structuraux.

Son modèle normal/−O passe 78 519 contrôles, 132 corruptions et 45 faits analytiques ;
34 requêtes par profil produisent 120 ordres, 867 nœuds, 1 882 coupes et 673 verticales.
La ligne de treize sites à K12 exerce une coface de taille13 : les sphères critiques
et traces évitent une MEB13 dans le produit ; l’oracle de définition garde cette coface.
Les nouvelles portes natives couvrent aussi déplacements, cohabitation, refus de chaque
allocation observée, capacités scalaires et quatre lecteurs avec budgets privés.
La fixture cb5a69ef protège le support canonique global q3 qui exclut le premier site
de coquille. Toutes ces portes natives restent à exécuter ensemble sur G4.

## Comparaison exacte des centres

Le comparateur neuf `num::compare_centers` ordonne les trois coordonnées
`(a_i D+N_i)/D`, avec D>0 certifié par la factory Sphere. Pour le profil B,
chaque numérateur global a une valeur absolue strictement inférieure à
`2^(5B+6)` ; à B=24, il tient donc dans i128 signé. Les produits croisés
ont une valeur absolue inférieure à `2^(9B+11)`, au plus `2^227` : quatre
limbes suffisent. Le code compare les deux produits, sans les soustraire.
Ni égalité des rayons ni positivité du support ne sont supposées ; un centre
hors boîte ou une autre présentation du même centre restent valides.

L'oracle Gram/Fraction est indépendant des N/D natifs :916 paires par
profil,922 contrôles de modèle et quatre corruptions détectées, en normal/−O.
Les portes natives ajoutent centres égaux entre arités, ancres permutées,
coordonnée finale décisive, dernière unité du domaine, centre extérieur de
grandeur quadratique et quatre modes d'arrondi. Trois mutants ciblent ancre,
dénominateur et axe final. Leur exécution G4 reste à faire.

## Banc entier préparé

`bench/full_probe.cpp` prend tous les retours du payload déclaré ; aucun
préfixe ni tirage. Le temps FULL englobe index, CatK/lookup, forêts K1..K et
verticales fermées. Lecture, Cloud et création du Pool sont publiés séparément ;
la sérialisation et les destructions restent hors de ce temps. Le pic de
préparation est conservé avant remise à zéro du pic moteur. Le banc ne couvre
ni segmentation du sol, ni projection sur les points, ni GPU.

Le dump conserve niveaux et centres rationnels, parents, enfants et verticales.
Le lecteur contrôle la structure entière et la naturalité de chaque enfant ;
il ne remplace pas l'oracle géométrique indépendant des petites fixtures.
Le plan G4 exige d'abord la qualification native, puis déclare24 essais entiers
u21/u24 : trois processus par trame à K5, un à K10. Le délai par enfant est60s,
le budget de campagne600s ; tout essai omis est explicite. Les deux profils
reçoivent les mêmes coordonnées à1mm : ils ne changent pas la précision d'entrée.

## Différentiel natif v10 préparé

`tests/tower/full_v10_diff.py` reconstruit sur G4 les deux CLI de la v10
`c764e121aa52f2e5dd9b85fbe308c9c3511ff55e`, depuis34 fichiers Git inchangés.
Le paquet externe de71 562octets et chaque membre sont épinglés dans
`tests/tower/v10_frozen_manifest.json`. Aucun installateur n'est lancé :
les seuls prérequis sont GCC/C++20, CMake, Make, Threads et Python3.10.

Sur14 petites fixtures communes u18, les profils natifs v11 u18/u21/u24
sont comparés à la v10 ; ce n'est pas une qualification v10 au-delà de18bits.
Le catalogue v10 et la MEB Fraction servent à identifier ses propres
naissances ; aucune forêt attendue ni résultat v11 ne dirige sa numérotation.
Chaque résultat est ensuite sérialisé indépendamment selon les naissances
`(niveau, centre)`, les fusions `(niveau, naissance minimale)`, les parents,
les enfants et les verticales. Les octets de ce format commun doivent être
identiques. Les dumps bruts ont des conventions différentes et leur égalité
n'est pas revendiquée ; les attaches de points ne font pas partie du test.

Les flux natifs, dumps originaux, deux sérialisations communes, empreintes
des sources/binaires et commandes sont conservés, y compris en cas d'échec.
Le modèle pur normal/−O passe42 positifs et19 corruptions ; le différentiel
natif reste à exécuter. Ce contrôle borné complète l'oracle de définition,
sans transférer les qualifications historiques ni prouver le contrat massif.

Le plan ajoute un différentiel natif v10 figé sur quatorze petites fixtures :
les dumps différents sont convertis indépendamment dans un format canonique
commun, puis comparés octet pour octet. Cette première porte ne qualifie pas
encore le différentiel sur les trames LiDAR entières exigé pour la conformité.
