# q4 — induction locale de la cellule à l'émission

23 septembre 2026. Revue mathématique du code `src/gen` au commit produit
`ba9980fd` (aucun diff local de ces sources). Cette note établit une
**complétude conditionnelle par arête** pour `Local28` : elle part d'une arête
déjà livrée à la voie q4, avec son cover complet. Elle ne prouve ni que le front
WSPD livre toutes les arêtes propriétaires, ni que tous ses filtres conservent
leur masque, ni que le catalogue/FULL est globalement complet.

## Hypothèses et objet

Soit `S={a,b,x,y}` un support affinement indépendant dont la boule minimale
`B(c,R)` est portée par ses quatre poids barycentriques **strictement
positifs**. Son arête propriétaire `e={a,b}` est sa plus longue arête, les
égalités étant départagées par IDs. Soit `p` le nombre de sites strictement
intérieurs à `B`, avec `p<K−2` et `K≥3`. On suppose que le producteur a livré
`e` avec le masque q4 actif, que `Q34EdgeCover::make` est appelé sur l'index
entier et que le callback poursuit normalement. Les sites de coquille peuvent
être plus nombreux que les quatre sommets de `S`.

La preuve directe [support/citron v8](../../morsehgp3D_v8/audits/q34_global_contract_20260921/SUPPORT_ET_CITRON.md)
donne une face `abx` strictement aiguë : pour `D=|a−b|²`, `m=(a+b)/2` et
`H(z)=D/4−|z−m|²`, les poids positifs donnent
`Σ λ_i H(v_i)=−2|c−m|²<0`. Donc `H(x)<0` ou `H(y)<0`, soit un angle aigu au
sommet opposé de `ab`; les deux angles aux extrémités sont aigus parce que
`ab` est maximale. On choisit pour `x` la plus petite ID parmi ces faces
aiguës. Sa graine passe le test d'acuité et de propriété
([q4_local.cpp:18–36, 480–490](../src/gen/lanes/q4_local.cpp)).

## Pourquoi son centre reste dans l'atlas

Poser `v=b−a`, `t=2(c−m)` et `D=|v|²`. Les identités de variance d'un support
positif q4 donnent `R²≤3D/8`, `|c−m|²≤D/8`, donc `|t|²≤D/2` et
`R+|c−m|≤(√3+1)√(D/8)<√D`. La boule entière, y compris profondeur et
coquille, est ainsi dans le cover fermé de rayon `|ab|` construit par
[edge_cover.cpp:25–59](../src/gen/lanes/edge_cover.cpp).

Le code choisit `h=max_i|v_i|` et deux vecteurs `A,B` orthogonaux à `v`,
avec respectivement une coordonnée transverse égale à `h`
([q4_local_partition.cpp:53–66](../src/gen/lanes/q4_local_partition.cpp)).
Dans `t=u_1 A+u_2 B`, les coordonnées transverses donnent
`|u_1|,|u_2|≤|t|/h≤√(3/2)<2` puisque `D≤3h²` : `c` est dans la cellule
racine fermée `[-2,2]²`. Pour le domaine `Positive`, `x,y` appartiennent
à l'ensemble des complétions admises car leurs distances à `a` et `b` sont
`≤|ab|` ([q4_positive_domain.cpp:86–143](../src/gen/lanes/q4_positive_domain.cpp)).
La projection de `t` sur `v⊥` est une combinaison convexe de `0`, des
projections de `2(x−m)` et de `2(y−m)` : les contributions de `a,b` se
projettent en zéro. Le polygone calculé à partir des coins de la boîte des
complétions **et de zéro** contient donc cette projection. Les refus de
[q4_local_partition.cpp:159–240](../src/gen/lanes/q4_local_partition.cpp)
utilisent des inégalités strictes sur des bornes fermées et ne peuvent classer
`Outside` une cellule qui contient `c` ; en cas de polygone dégénéré, le code
revient au disque.

Pour toute cellule fermée `C`, l'invariant du fragment exact est : le cover
est partitionné en sites uniformément strictement intérieurs sur `C`, sites
uniformément strictement extérieurs, et frontière active `F_C`. Ainsi, à tout
centre de `C`, la profondeur vaut `inside_count + #{z∈F_C : L_z<0}` et toute
la coquille appartient à `F_C`. Les tests de bornes gardent `0` dans la
frontière ; un budget épuisé y garde aussi tous les blocs non résolus.
Chaque enfant reclassifie uniquement `F_C`, en héritant du compte exact
([q4_local_partition.cpp:290–433](../src/gen/lanes/q4_local_partition.cpp)).
Il faut distinguer l'égalité vraie `L_y(c)=0` d'une égalité de **borne** :
`node_bounds_unchecked` minore le minimum continu sur boîte×cellule (avec
arrondi vers le bas) et majore le maximum ; pour le nœud qui contient `y`,
ses bornes encadrent donc zéro, même si aucune borne calculée n'est nulle
([q4_local_partition.cpp:250–285](../src/gen/lanes/q4_local_partition.cpp)).
Dans toute cellule contenant `c`, `inside_count≤p<K−2` : ni le seuil `Deep`
à `K−2`, ni le certificat saturant à `K−1`, ne peuvent supprimer ce chemin.
Les quatre enfants couvrent leur parent en fermé ; leur convention
`owns_right/owns_top` désigne une seule feuille émettrice si `c` est sur une
frontière dyadique ([q4_local.cpp:38–43, 188–270](../src/gen/lanes/q4_local.cpp)).

## De la racine exacte au callback

Dans la carte précédente, écrire `w_z=2(z−m)`. Pour toute boule de centre
`m+(u_1A+u_2B)/2` passant par `a,b`, la forme du code est exactement
`L_z(u)=|w_z|²−D−2w_z·(u_1A+u_2B)=4·power_B(z)` ; `v·A=v·B=0` annule le
terme axial. Par conséquent `L_x(c)=L_y(c)=0`. Comme `S` est affinement
indépendant, ces deux droites du plan des centres ont une intersection unique,
égale à `c`. Le calcul d'[intersection](../src/gen/lanes/q4_local.cpp) utilise
des produits entiers exacts et une normalisation de signe, sans arrondi.
Les bornes de `L_x` sur une cellule **fermée** contenant `c` encadrent donc
zéro ; ni le parcours individuel ni les bornes conservatrices du produit
graine×cellule ne peuvent la rejeter par un test strict `min>0` ou `max<0`
([q4_local.cpp:469–478, 685–749](../src/gen/lanes/q4_local.cpp)).
Dans `LiveOnly`/`Joined`, le résumé `live` est le nombre exact de feuilles
descendantes : l'existence de cette feuille rend tous ses ancêtres non nuls,
et les bornes spatiales ne rejettent pas une graine aiguë possédée
([q4_local.cpp:614–710](../src/gen/lanes/q4_local.cpp)).

Sur la ligne des centres de `abx`, poser `N=(b−a)×(x−a)`, `G=|N|²>0`,
`P_z=G|z−a|²−W·(z−a)` et `B_z=N·(z−a)`. Le centre
`c(μ)=c_0+μN/(2G)` donne une puissance de signe `P_z−μB_z`. Pour `B_z≠0`,
la racine exacte est `μ_z=P_z/B_z`. Le site `y` a `B_y≠0` et sa racine est
précisément `c`; son événement appartient donc à la feuille fermée et ne peut
être clippé. Un autre événement dont la racine est hors de cette feuille a un
signe strict constant sur son intersection convexe avec la ligne ; le point
de référence de la cellule classe correctement cet événement comme constant
([q4_local.cpp:113–139, 374–416](../src/gen/lanes/q4_local.cpp)). Les sites
coplanaires ont, eux aussi, une puissance constante.

`compare_roots` compare exactement `P_i/B_i` en tenant compte des **deux**
signes des dénominateurs ([q4_family.cpp:94–107](../src/gen/lanes/q4_family.cpp)).
Le compte initial du sweep est un état formel avant les événements **retenus**,
plus les classes constantes certifiées sur la feuille ; il n'affirme rien sur
la profondeur physique hors de cette feuille. Cela suffit, car seules les
racines situées dans la feuille sont examinées pour une sortie.
Au groupe de racine `c`, la boucle soustrait toutes les sorties (`B_z<0`)
avant d'ajouter les entrées (`B_z>0`) : `inside` est alors exactement `p`,
y compris si plusieurs contacts des deux sens coïncident. Le groupe actif
plus la coquille coplanaire est la coquille entière de `B`; aucun site classé
uniformément strict ne peut être sur cette frontière
([q4_local.cpp:417–467](../src/gen/lanes/q4_local.cpp)). Le seuil `p<K−2`
laisse passer ce groupe. `owned` et `ExactBall::make_q4` réussissent sur `S`.
Puisque `x` est la plus petite des deux éventuelles faces aiguës, la condition
`y<x && acute(a,b,y)` ne rejette pas cette présentation. Si la boucle émet
avant `y` un autre complément positif du **même groupe** `(e,x,c)`, il a
le même centre et le même rayon, donc la même clé de boule. Au moins un
callback présente cette clé ([q4_local.cpp:438–462](../src/gen/lanes/q4_local.cpp)).

## Raccord amont, obligation distincte

La [preuve statique dédiée au passage de l'arête
propriétaire](Q34_PROPRIETAIRE_PASSAGE_AMONT_20260923.md) traite maintenant
cette obligation sous les invariants du front, de l'index et des prédicats
entiers, sans étendre la présente induction à `Window30` ou à FULL.

La note amont démontre la partition des paires et l'impossibilité de réunir
`K−2` témoins **distincts et strictement intérieurs** lorsque `p<K−2`.
Elle traite séparément les bornes `H_min/Ξ_max`, le cache revalidé sur la
paire, le cœur diamétral et l'indépendance du masque q4 vis-à-vis de q3.
Ces résultats restent **sous les invariants certifiés de l'index et des
prédicats** et demandent une porte globale non triviale pour la chaîne
complète. Le backend optionnel `Window30` relève d'une preuve différente,
par couches duales et rangs d'événements : aucune étape propre à sa fenêtre
n'est déduite ici de l'atlas.

## Portes et limite de conclusion

Les portes existantes ont déjà de vrais juges indépendants :
`q4_family_gate.cpp:161–204` trie les racines rationnelles et vérifie les
groupes mixtes ; `q4_local_gate.cpp:398–425` place une racine positive au
coin dyadique exact `(0,−1)` et garde le complément dont `min=0` ;
`:145–210, 500–507` compare les sorties `clip_events` OFF/ON à un oracle
rationnel et teste un événement clippé mais intérieur constant ;
`wspd_q34_gate.cpp:111–207` **énumère tous** les supports q3/q4 positifs de
ses petits nuages et compare support, clé, profondeur et coquille du flux
global. Il serait donc erroné de dire qu'aucun oracle global n'existe.

La [porte complémentaire de flux entier](q4_global_12sites_20260923/README.md)
rejoue la fixture rationnelle de douze sites
de [q4 sans faces q3 admises](check_q4_without_q3_faces_20260922.py) : à
`K=3`, la clé primitive `(1,−40,−40,−40,900)` a profondeur zéro et une
coquille de quatre sites, alors que ses quatre faces q3 ont profondeur deux.
Le sidecar compare **108 flux complets** à l'énumération rationnelle
indépendante de `global_oracle` : deux ordres d'IDs, `s=8,10,12`, `Local28`
avec clipping/saturation OFF/ON, `Window30`, et un preset optimisé, en mono,
`W1` et `W4`. Son reçu LIVE est `PASS` (990 quadruplets examinés, 338 boules
rationnelles distinctes, 3 580 assertions) ; il exerce ensemble le masque q4
indépendant, le propriétaire, l'atlas et la canonisation. Le
[rembourrage 8k–32k](COMPLETUDE_Q4_CLE_REMBOURREE_20260923.md) vérifie la
présence de cette clé mais s'arrête à son premier callback : ce n'est pas ce
gate de flux entier. Les 108 flux restent un domaine fini, pas une preuve
universelle de l'induction ni une qualification de trame complète.

Cette induction est une **revue de programme conditionnelle**, pas une preuve
machine de l'arithmétique, des filtres WSPD ou de l'exécution concurrente.
Elle donne les invariants et les comparaisons strictes à conserver lors des
optimisations ; la qualification de la chaîne globale requiert encore ses
propres obligations et des reçus épinglés.
