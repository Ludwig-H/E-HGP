# Contre-audit : fermer un produit q3/q4 avant sa double boucle

23 septembre 2026. Lecture du moteur au HEAD `06f71037` et des reçus
LiDAR v12, sans modification du moteur ni nouveau chrono. Cadre :
`exploration_v9_hors_registre`, CPU de référence, entier u18/grille 1 mm,
`not_claimed`. Cette note examine le **produit d'arêtes** `A×B` avant
`Engine::expand`, distinct du [certificat par nœuds du cœur](CERTIFICAT_NOEUDS_CORE_LIDAR_20260923.md),
qui intervient après le filtre de chaque paire.

## Verdict et certificat borné

Le [front](../src/gen/wspd/front.cpp) émet des produits disjoints par voie.
Dans [`Engine::rectangle`](../src/gen/pipeline/wspd_q34.cpp), le filtre
`RectanglePair` peut retirer q3 ou q4 de tout `A×B`; si un bit survit,
`expand` énumère encore **chaque** paire du produit. Un certificat supplémentaire
est sûr s'il fournit, pour chaque voie retirée, `T3=K−1` ou `T4=K−2`
**sites préparés distincts** strictement intérieurs à toutes les boules
positives de cette voie pour chaque `(a,b)∈A×B`. K1 n'active aucune voie ;
K2 n'active que q3. Les contacts ne créditent rien et les comptes q3/q4 ne
s'additionnent pas.

Une réalisation finie, à mesurer en shadow, emploie un ticket de sites `z`
issus d'une **recherche de paire tracée dont le résultat est réutilisé**. La trace contient des
nœuds admis, disjoints par voie ; sélectionner au plus le crédit nécessaire
dans leurs plages de rangs donne au plus `(K−1)+(K−2)≤17` IDs pour K≥3.
Cette extraction doit utiliser `spatial_order()[rang]`, jamais le rang comme
ID, dédoublonner les IDs par voie et lier le ticket à l'index/propriétaire.
Un site appartenant à A ou B ne fournit aucun crédit uniforme au produit :
il est l'extrémité de certaines paires et y donne `H=0`.
Le ticket n'est qu'une proposition pour le produit : pour chaque site fixé,
[`box_witness`](../src/gen/spindle/predicates.hpp) vérifie exactement les
**64 couples de coins** de `box(A)×box(B)` en entiers. Il teste `H>0` puis
`3H²>Xi` pour q3, `2H²>Xi` pour q4 ; sa preuve par convexité séparée couvre
les boîtes continues, donc toutes les paires discrètes. Un succès q4 pour
le même `z` implique q3, mais son seuil est plus petit. Un échec de coin
laisse le produit indécis, sans conclure qu'une paire survit. Un ticket qui
atteint les deux seuils supprime le produit **sans** construire ses arêtes ;
un seul seuil retire seulement son bit.

La surcharge `box_witness` existe déjà, mais **le ticket par produit et son
raccord n'existent pas**. La trace actuelle est produite pour une paire,
pas pour `rectangle`. Si la première paire du produit fournit le ticket,
sa recherche doit être facturée et cette même paire doit ensuite avoir un
propriétaire unique : réutiliser son résultat dans `edge`, ou mesurer
explicitement la recherche répétée. Un ticket venant du filtre rectangle
serait moins cher à acquérir, mais ses crédits observés dans le sidecar de
[la piste B](PISTE_B_Q34_RECTANGLES_AVANT_EXPANSION_20260923.md) ne valent
qu'environ 0,83–1,06 par rectangle en sommant les voies sur l'échantillon ;
il ne suffit pas à présumer une fermeture à K5/K10. Aucun DFS témoin neuf
par sous-produit n'est compris dans la proposition bornée.

**Pourquoi les sites internes au facteur ne se créditent pas en bloc.**
Pour `x≠y` dans A et `b` fixé dans B,

`H(a=x,b,z=y) + H(a=y,b,z=x) = −|x−y|² < 0`.

Deux sites de A ne peuvent donc pas être chacun témoin strict pour toutes
les ancres de A autres qu'eux-mêmes ; la même identité vaut dans B. De plus,
un site choisi comme extrémité a `H=0`. Soustraire seulement une extrémité
d'un « stock » de K sites internes et créditer le reste sur **tout** `A×B`
est faux. Des témoins internes peuvent aider une **ligne** `a×B` ou un vrai
sous-produit, avec certification et comptes propres à cette région ; le
ticket uniforme du produit doit passer le test exact ci-dessus.

## Scission et coût qui ne doit pas disparaître du registre

Si le ticket ne ferme pas le produit, scinder **un vrai nœud** A ou B en
ses deux enfants conserve une partition exacte :
`A×B=(A_g×B) ⊔ (A_d×B)` ou symétriquement. Chaque enfant reçoit son masque
restant ; un bit retiré par le parent le reste, et les autres sont retestés
avec zéro crédit implicite nouveau. Les produits indécis reviennent une
seule fois au chemin `expand/edge` courant. Les plages A utilisées par la
file actuelle sont des unités d'ordonnancement ; une scission géométrique
requiert les **handles de nœuds et leurs boîtes**. Le ticket et le masque
sont possédés par la tâche, l'index est immuable et partagé, et un refus de
publication dans la file traite la tâche localement. Cela conserve la
couverture unique sous W1 comme sous plusieurs workers, sans file infinie.

Soit `P=Σ_R |A_R||B_R|` la masse après le filtre rectangle, `T` le nombre
de sous-produits effectivement classifiés, `E` les paires enfin développées.
Une scission binaire en produits non vides donne `T≤2P−R` si chaque racine
résiduelle est comptée. Pour un ticket de `m≤17` sites, la seule validation
géométrique coûte au plus `128mT` tests ponctuels de coins (deux voies,
64 coins), avant arrêt précoce ; **ajouter** acquisition du ticket,
extraction d'IDs, copies de tâches et validations de propriétaire.
Cette borne empêche un nouveau DFS `O(n)` caché à chaque enfant ; elle ne
borne ni `P`, ni `E`, ni le travail total sous le carré. Un critère de masse
qui choisit où *tenter* le ticket est un aiguillage de performance : tout
échec va au chemin exact, sans plafonner les candidats ou la recherche.

Pour les paires entièrement écartées, le ledger doit satisfaire
`input_pair_mass = rectangle_pair_mass + subproduct_pair_mass + expanded_pairs`.
Par voie, ajouter `subproduct_q3_pairs` et `subproduct_q4_pairs` à
l'identité actuelle de `validate_completion`; ces masses peuvent se
recouvrir et ne s'additionnent pas comme paires distinctes. Le compte
`expanded_pairs` doit inclure la paire qui a fourni le ticket si elle est
réellement entrée dans `edge`. Compter séparément produits testés,
produits fermés, masques partiels, coins, recherches de tickets, tests
cache/DFS évités **après** scission, covers, formes du cœur et du cover,
atlas, sorties, catalogue/FULL, CPU, mur et RSS.

## Porte empirique

Le [shadow de scission](Q34_BLOCS_LIDAR_SHADOW_20260923.md) donne une
distribution grossière mais décisive sur 08/000000 sans sol, s8 : à K5,
**4,17 %** des rectangles résiduels (`|A||B|≥16`) portent **91,72 %** de
leur masse ; à K10, **3,56 %** portent **87,31 %**. Une seule scission avec
le DFS actuel ferme 3,678/23,687 M paires à K5 et 4,124/30,777 M à K10,
mais paie 40,855/62,542 M visites Z supplémentaires : **11,1/15,2
visites par paire fermée**, sans cover ni forme évités dans ce shadow.
Cela justifie de mesurer un histogramme conjoint `|A|,|B|,masse,masque`
(classes logarithmiques et grands produits individuels) avant de choisir
où retester un ticket. Le seuil 16 du shadow n'est pas un optimum.

La [campagne v12](../receipts/lidar_scaling_local_20260923/README.md)
ne publie que des totaux de produits, sans cette distribution. Sur
08/000200/K5, 16k→32k sites emboîtés, `expanded_pairs` passe de 4,433 à
21,344 M (×4,82) et `core_sites` de 40,449 à 334,344 M (×8,27), tandis
que les rectangles d'entrée font 1,791→3,459 M (×1,93). Ces chiffres
désignent un régime à sonder, pas une borne asymptotique. L'ablation doit
apparier baseline, ticket seul puis ticket+scission sur **les mêmes**
rectangles et masques, K5/K10, s8/10/12 ; comparer les flux complets
(clés, supports, profondeurs, tous IDs de coquille), catalogue et tour,
avec W1/W8, coupes 8k/16k/32k et trames entières de plusieurs séquences.
Le coût d'un certificat réussi et des sous-produits échoués doit rester
visible dans le temps total. Aucun résultat FULL/G4, GPU ou sous-quadratique
nouveau n'est acquis par cette note.
