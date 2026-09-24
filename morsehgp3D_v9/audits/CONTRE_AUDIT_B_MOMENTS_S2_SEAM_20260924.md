# Contre-audit B — moments exacts et raccord S2→S3 pour q3/q4

24 septembre 2026. Lecture indépendante du
[certificat multisite](moments_multisite_precore_20260924/NOTE.md), de son
[extension aux rectangles](CONTRE_AUDIT_B_R20_ET_TRAJECTOIRE_100MS_20260924.md)
et du raccord de production `src/gen/pipeline/wspd_q34.cpp` au WIP
`61cfba666`. **Aucun port ni gain de temps n'est acquis.** Profil visé :
grille entière 1 mm, trame SemanticKITTI entière sans sol, sortie FULL
explicite K1..5 dans 100 ms sur G4 ; K10 et brut avec sol demeurent à
tester séparément.

## 1. Mathématiques : certificat recevable, voies distinctes

Pour un bloc fixe `G` de `N` IDs distincts du sous-nuage effectivement
traité, les cinq moments `N,Z,Q` donnent une minoration de la **somme**
des marges d'intérieur sur tous les centres admissibles d'une arête.
Chaque marge positive est au plus le rayon carré. La minoration stricte
au-dessus de `(T−1)` fois ce majorant implique donc au moins `T`
intérieurs stricts ; les autres marges peuvent être nulles ou négatives.
Les tests entiers annoncés dans la note, `A3>0, A3²>12X` pour
`T3=K−1>0` et `A4>0, A4²>8X` pour `T4=K−2>0`, sont bien équivalents
aux deux minorations. Une égalité ne ferme rien. Une extrémité présente
dans `G` a une marge nulle et ne gagne aucun crédit ; les IDs doivent
néanmoins rester distincts à l'intérieur de `G`.

Pour un **même bloc fixe** sur un rectangle `A×B`, écrire
`C=(b−a)×(2Z−N(a+b))`. `H` et `C` sont affines séparément en `a` et
`b`, `D=|b−a|²` convexe séparément. Les fonctions
`3H−4(T3−1)D−√12|C|` et `2H−3(T4−1)D−√8|C|` sont donc concaves
séparément. Jensen appliqué successivement aux deux boîtes prouve que
la stricte positivité aux **64 couples de coins croisés** suffit sur
tout le produit ; chaque coin se teste par les inégalités entières
ci-dessus, sans racine flottante. Les conditions restent `D>0` pour
les arêtes réelles, même bloc/IDs, promotion avant multiplication, et
bornes i128 recalculées si l'on dépasse `N<2¹⁷`, u18, `K≤10`.
Le vérificateur de rectangles actuel exerce surtout une boîte positive
et ses translations/homothéties, pas les boîtes qui se chevauchent ni
une extrémité réutilisée ; c'est une lacune de **tests**, non un
contre-exemple à la preuve.
Le [contrôle additionnel B](b_moments_rectangle_edges_20260924/README.md)
ferme cette lacune bornée : normal/`-O`, neuf fixtures entières,
boîtes chevauchantes, garde coïncidant avec une extrémité, égalités
strictes et q3/q4 indépendantes. Trois boîtes positives ont 64 coins
croisés distincts et 729 couples entiers chacune, tous concordants.
Deux boîtes qui se chevauchent échouent uniformément malgré des arêtes
individuelles positives : le produit de boîtes contient un `a=b`
virtuel, impossible à certifier comme arête de longueur positive. Cette
perte de sélectivité est normale ; dans un BVH de couples, il faut
diviser ces boîtes, pas supprimer leurs arêtes réelles.

Les deux voies ne sont pas substituables. La fixture à sept gardes de
la note ferme q4 mais pas q3. Réciproquement, à K5,
`a=(0,1000,1000)`, `b=(1000,1000,1000)` et
`G={(500,1274,950+j):0≤j<100}` ferment q3 mais pas q4 :
`A3²=38 772 996 753 960 000 > 12X=36 036 600 000 000 000`, alors
que `A4²=17 761 532 601 760 000 < 8X=24 024 400 000 000 000`.
Ne retirer que le bit certifié ; sauter le cœur seulement si **tous**
les bits encore actifs sont fermés. Aucun compte ne se transmet au
census ni d'une voie à l'autre.

## 2. Raccord minimal, sans changer l'objet

Dans `wspd_q34.cpp:1615–1641`, S2 valide les survivants en parcourant
les rectangles puis les paires dans un ordre stable ; `rectangles` est
libéré vers `:1652`. `Q34SurvivingEdge` ne garde que `(a_rank,b_rank,mask)`.
S3 reçoit donc un vecteur dont l'indice est l'ordinal des survivants,
mais plus les limites de rectangles ; S4 utilise précisément cet ordinal
pour rattacher ses enregistrements. Le raccord audit seul peut conserver
les `R+1` offsets monotones de survivants par rectangle pendant la
validation, **O(R+S)** en temps et mémoire, puis une permutation des
ordinaux originaux pour les groupes réordonnés. Aucun ID de rectangle
par arête n'est nécessaire si les segments restent consultables par
offsets. La validation doit parcourir **tous** les `R` rectangles, même
après la dernière survivante, et écrire des bornes égales pour chacun
des rectangles rejetés ou vides ; reprendre tel quel le court-circuit
actuel `cursor<S` laisserait les derniers offsets indéfinis. Le premier
shadow CPU n'exige aucun changement d'ABI GPU : garder une vue immuable
des survivants et transporter seulement un ordinal global `j` dans les
groupes. Toute fermeture doit retourner au masque de l'arête d'origine,
et toute survivante indécise doit reprendre S3 puis S4 dans son ordre
et avec ses IDs antérieurs. Le matching de paires de gardes exige des
IDs disjoints **dans chaque voie** ; les crédits q3/q4 ne s'additionnent
pas. Le certificat multisite exige des IDs distincts dans son bloc,
mais pas une exclusion spéciale des extrémités. Les rangs `(a_rank,b_rank)`
de `Q34SurvivingEdge` sont relatifs à l'index du sous-nuage, **pas** les
IDs bruts des retours ; les comparaisons entre densités doivent joindre
par la table des IDs d'origine, jamais par le seul ordinal `j`.

Sur R20/08/000000/K5 sans sol : `R=3 133 819`, `P=23 686 751` paires
expansées, `S=2 043 612` survivantes et `F=359 707 275` formes de
cœur. Deux vecteurs u64 pour `R+1` offsets et `S` ordinaux coûtent
environ **41,4 Mo** à ces tailles, avant table de groupes et BVH ; les
versions à base locale u32 doivent garder une base globale u64 vérifiée.
Garder aussi les rectangles actuels de 24 octets pendant tout le filtre
coûterait environ **75,2 Mo** supplémentaires de pic ; le préfixe permet
précisément de les libérer si leurs boîtes ne sont plus nécessaires.
Un groupe ne doit pas rechercher ses arêtes par balayage de toutes les
`S` survivantes : cela déplacerait le coût vers `O(groupes×S)`.
Le filtre aval peut réduire `F`, les covers et les tests S3 ; il ne peut
pas réduire `P` ni les **1 110 657 775** visites de nœuds témoins S2
déjà effectuées dans ce cas. Une fermeture de
rectangles **avant S2** serait nécessaire pour agir sur `P`, mais payer
64 coins sur les 3,13 M rectangles coûterait plus de 200 M tests : il
faut choisir les rectangles lourds avec un préfiltre conservateur et
compter le coût de sélection.

Le chemin GPU actuel recherche deux fois par paire son rectangle par
recherche binaire d'offsets (`filter_runner.cu:129–140,180–191`) :
**O(P log R)**, sous-quadratique mais potentiellement cher sur les
23,7 M paires de R20. Un dispatch par segments/tuiles de rectangles
pourrait éviter ces recherches, sous réserve de traiter les rectangles
déséquilibrés et de préserver les mêmes ordinaux ; le gain n'est pas
mesuré. Les scans CUB actuels refusent `R` ou `P≥2³¹`, S3 limite aussi
son nombre d'arêtes à `2³¹−1`, et `Q34LaneRecord.edge` est u32. Ces
limites demandent un plan tuilé à bases globales vérifiées pour les
nuages de plusieurs dizaines de millions de points, pas une simple
extrapolation des trames courantes.

La comptabilité du moteur devra distinguer `precore_q3_proofs`,
`precore_q4_proofs` et arêtes entièrement fermées. Au baseline R20,
`S=core_builds=dead_core_loads` et `core_closed_edges+cover_builds=S` ;
un shadow sans saut effectif conserve ces identités, tandis qu'un port
doit publier les nouvelles identités et les **formes réellement
matérialisées**, pas appeler `core_sites` baseline un gain déjà payé.

## 3. Porte expérimentale avant port G4

Les reçus déjà publiés ne permettent **pas** de calculer cette
sélectivité après coup. R20 archive des JSON agrégés, sans arêtes
survivantes individuelles. Le
[panneau des 21 sous-nuages](s4a_cpu_scene02_physical_panel_20260924/README.md)
archive ses entrées u32/IDs et 42 sorties agrégées, mais pas leurs
arêtes, masques ou `F_e`. Les anciennes traces complètes d'arêtes
brutes étaient temporaires et ne sont plus présentes ; seul le
[`dominant_group.bin`](paired_guard_group_bvh_20260923/README.md)
statique de 67 827 arêtes **avec sol** subsiste, sans ordinal S2 ni
masque post-S3. Un smoke exact de huit arêtes issues de ce groupe avec
un bloc voisin de 64 sites ne ferme aucune voie, pour `ΣF_e=44 174` :
c'est un contrôle de lecture/arithmetic **non représentatif**, pas une
estimation de sélectivité. La prochaine mesure sans sol exige donc une
trace d'audit au raccord S2→S3 ou un rejeu autonome du même front et
filtre ; les sommes JSON ne suffisent pas.

Une sidecar sans modification du moteur est possible via le front et
`run_q34_filter_batch_cpu`, puis les API de cœur/cover/prover, mais c'est
une **vraie réexécution CPU** de S2/S3, pas un traitement des reçus ni
un chrono G4. Les trois scènes sans sol ont 39 885, 35 551 et 45 845
sites ; leurs survivants cumulés valent environ 6,014 M à K5 et
13,108 M à K10. Même un enregistrement minimal de 16 octets par arête
représente environ **306 Mo pour les six cas** ; avec ordinal u64, environ
459 Mo, avant offsets et sous-nuages. La tranche entière 08/000200/K10
du panneau CPU a pris environ 85 s de mur, 570 CPU·s et 3,94 Go RSS :
prévoir des minutes et plusieurs Go pour la campagne complète. Une
première fixture sur un seul quartier de 1 288 sites doit valider le
schéma, les IDs et les identités `S=core_builds=dead_core_loads`,
`ΣF_e=core_sites`, puis seulement étendre aux trames. Les ordres GPU et
CPU pouvant différer, comparer multisets et sommes entre backends,
pas les ordinaux de deux exécutions différentes.

Commencer par un ledger sans modification de sortie au raccord S2→S3 :
par segment/masque, nombre d'arêtes, `F` baseline par arête, travail
dispatch/BVH/blocs/64 coins/replis, `F` effectivement évité et sorties
**littérales** (clés, coquilles par ID, catalogue et tour). Comparer
blocs de moments, paires de gardes et repli actuel à budget identique ;
ne pas transférer les 153,8 M formes fermables d'un **seul groupe brut
favorable** à toute la trame sans sol. Tester les trois trames sans sol
complètes puis brutes, K5/K10, s8/10/12, coupes physiques et densités
emboîtées 8k/16k/32k. Publier croissance du travail total, mémoire,
temps mur/CPU/GPU et sortie FULL matérialisée. G4 R21 reste suspendu à
ses propres portes de préflight et d'épingles ; aucun GCP utilisé ici.
