# Contre-audit B — paires de gardes avant le cœur q3/q4

23 septembre 2026. Relecture indépendante du
[reçu de A](paired_guards_precore_20260923/README.md), commit
`4dd6f8a1e`. Audit du **certificat**, de la provenance et du sens des
mesures ; aucun port moteur, build G4 ni chrono nouveau.

## La preuve positive tient

Pour une arête `ab` propriétaire comme plus longue arête de ses futurs
supports, la variance des poids barycentriques positifs borne le
déplacement `t` du centre par `|t|²≤D/12` en q3 et `≤D/8` en q4,
où `D=|b−a|²`. La marge d'un témoin réel `g` est
`P_g(t)=D/4−|g−m|²+2(g−m)·t`. Pour deux témoins distincts `g,h`,
le minimum de `P_g+P_h` vaut `H/4−√(X/12)` ou `H/4−√(X/8)` selon la
voie. Ainsi `H>0` avec `3H²>4X` (q3) ou `H²>2X` (q4) garantit
**strictement** au moins un intrus réel de cette paire à tout centre
admissible. Quatre ou trois paires **disjointes** ferment respectivement
q3 ou q4 à K5. Les égalités ne créditent rien. La lecture algébrique
ne trouve pas d'hypothèse géométrique manquante ; le propriétaire de
plus longue arête et les sites distincts sont essentiels. Le bornage
u18 annoncé tient en i128 après promotion avant produit.

Les huit SHA de la capture passent. J'ai rejoué `verify.py` normal et
`-O` sur les deux JSON : **60 arêtes** et **299 / 383** contrôles
stricts de paires, fermetures `B=4/8/16` **20/22/27** puis
**32/34/40**. Les associations et les `F` sont revérifiables contre
les coordonnées u18 épinglées ; le replay local des 120 arêtes
retrouve `ΣF=520 631` et les deux JSON octet pour octet. Le mode
`--sample` recalcule palettes, échecs et preuves sur ces arêtes mais
reprend leurs IDs/masques du JSON ; la provenance du réservoir exige
les huit traces S2 et leurs SHA. Celles conservées localement ont
resélectionné les mêmes 60 arêtes par graine. Ces relectures LIVE
n'ajoutent pas un reçu GPU ni une nouvelle trame. Les huit parties de
trace et le binaire de fixture ne sont pas versionnés dans ce reçu
compact : les SHA les identifient, mais le rejeu autonome de la
**sélection** dépend de leur régénération ou de leur conservation ;
le rejeu des **preuves sur les arêtes épinglées** reste autonome avec
les entrées v8 régénérables.

## Ce que les fermetures ne disent pas

À `B=16`, la composition des deux réservoirs diffère :

| Graine | q3 seul | q4 seul | q3 et q4 |
| --- | ---: | ---: | ---: |
| 230923 | 0 | 17/29 fermées | 10/31 fermées |
| 230924 | 3/3 fermées | 30/38 fermées | 7/19 fermées |

Les deux ensembles de 60 arêtes sont disjoints, mais viennent **de la
même trame brute 08/000000** et sont stratifiés par `F`, pas par masque.
Le passage 27→40/60 reflète aussi davantage de q4-seul dans la seconde
graine ; ce ne sont ni deux répétitions du même cas ni une estimation
du taux de fermeture du flux entier. `F` fermable 111 883/260 032 et
168 845/260 599 n'est qu'un potentiel sur ces échantillons, pas du
travail effectivement économisé. Aucun résultat K10, sans-sol,
multi-séquence, s10/s12 ou G4 n'en découle.

Le glouton peut manquer un appariement ; ce n'est toutefois pas le
premier verrou. À palette16, les comptes de sommets candidats et
d'arêtes des graphes publiés donnent les conditions nécessaires
`P≥2T` et `graph_edges≥T` pour chaque voie active : **même un matching
optimal ne pourrait fermer plus de 34/60 puis 42/60** (contre 27 et
40 observés). Ce sont des plafonds **optimistes**, pas des matchings
trouvés ; les JSON ne donnent pas l'adjacence pour calculer l'optimum.
Le graphe n'est pas garanti biparti par les quadrants. Différer un
Blossom par arête ; accélérer d'abord la sélection de vrais sites.

Surtout, le shadow sélectionne ses candidats en balayant **123 389
sites par arête**, soit **7 403 340 lectures** pour 60 arêtes contre
260 032 formes de cœur dans la première graine (×28,47) ; son
appariement vient **en plus**. Porté tel quel, ce premier stade
restaurerait un terme `Ω(nE)` pour `E` arêtes tentées. De plus, les
60 arêtes ont été sélectionnées parce que leur `F≥1000` était **déjà
connu par une trace après construction du cœur**. Pour l'utiliser
réellement *avant* le cœur, il faut un déclencheur bon marché
disponible avant `Q34EdgeCover::make_diametral`, pas un second scan
qui mesure `F` ; après le cœur, la preuve peut encore épargner le
chargement des formes et l'aval, mais pas la construction déjà payée.

Un déclencheur pré-cœur **existe déjà** dans le flux S2 : le nombre de
survivantes par [segment de rectangle WSPD](s2_segment_mass_20260923/README.md).
Sur la trame brute 08/000000/K5, les segments d'au moins 16 arêtes
contiennent **396 481 arêtes (9,95 %)** mais **478 635 662 / 559 661 741
formes du cœur (85,52 %)**. Cette longueur est connue après S2, avant
le cœur, sans connaître `F` par arête. Un buffer privé de 15 arêtes par
segment pourrait décider au 16e de router le segment vers une tentative
de paires bornée ; les segments courts restent sur la voie actuelle.
Ce seuil n'est pas universel : les quarts et densités du
[panel S2](s2_segment_panel_20260923/README.md) montrent une
concentration très différente. Mesurer son coût et le résidu plutôt
que transférer les 85,52 % à d'autres scènes.

Une première porte de **recherche indexée bornée** est désormais
[mesurée par A](paired_guard_index_shadow_20260923/README.md) sur les
mêmes 120 arêtes lourdes : palettes B16 identiques au scan, 67/120
fermetures, 48 553 nœuds dépilés et 83 768 boîtes évaluées ; à
64 nœuds/quadrant, seulement 21/120 fermetures positives. La
[variante par nœuds](paired_guard_node_blocks_20260923/README.md)
remonte ce budget à 32/120 avec cap4, mais reste lui aussi par
arête. Ces reçus ne couvrent ni le flux entier ni les rectangles.
Une palette partielle peut fermer une voie positivement ; si le
budget s'épuise sans preuve, repli inchangé.
Leurs SHA et lecteurs normal/`-O` passent ; le lecteur des nœuds
recontrôle **758** paires de blocs contre les coordonnées LIVE u18,
et le diagnostic du seuil `D` se reproduit octet pour octet depuis
les huit traces épinglées. Aucune contradiction arithmétique trouvée.
Cependant les **53 arêtes négatives** à B16 non borné paient à elles
seules 21 517 nœuds et 37 326 boîtes ; cap4/budget256 n'ajoute que
**1,85 %** de `F` fermable net aux points B16, malgré 184 736 tests
de paires de blocs sur 120 arêtes. Le seuil `D≥2²²` qui garde les
67 fermables du panel solliciterait **909 278** arêtes S2 du plein :
le coût moyen du panel, s'il persistait, serait de l'ordre de
**368 M dépilements de nœuds** et **1,48 Md tests de paires** pour
les points B16. C'est un scénario de charge, **pas une extrapolation
mesurée**, car le panel est choisi par `F`. L'output ASan/UBSan
mentionné par A n'est pas conservé avec le reçu, même si les preuves
positives ont un lecteur indépendant.
Le certificat peut aussi être [relevé à tout un rectangle
WSPD](CERTIFICAT_B_PAIRES_GARDES_RECTANGLE_20260923.md) par 64 tests
de coins pour une paire commune : les segments lourds fournissent
alors une tentative par rectangle, pas une recherche par arête. Ce
chemin ne sauve toutefois **pas** le filtre/expansion S2 déjà payés ;
une élimination avant S2 exige un déclencheur connu au front.
Mesurer sur **toutes** les arêtes S2 le coût des tentatives et du
déclencheur, visites de nœuds/feuilles, `F` et covers réellement évités,
résidu q3/q4, puis chaîne complète. Inclure 8k/16k/32k et trames
brutes/sans-sol de plusieurs séquences, s8/10/12, avant une ablation
G4. Un budget constant par arête ne suffit pas à prouver le
sous-quadratique si `E(n)` ou l'aval restent trop grands. Ce schéma
est prometteur **mathématiquement**, pas encore industriellement.
