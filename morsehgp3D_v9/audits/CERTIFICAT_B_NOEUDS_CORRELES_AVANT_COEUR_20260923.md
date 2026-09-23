# Avant le cœur : créditer des nœuds témoins avec la borne corrélée

Audit mathématique B, 23 septembre 2026, base `346259ee0`, cadre
`exploration_v9_hors_registre`, `not_claimed`. **Proposition exacte à
tester en shadow**, sans port moteur, résultat LiDAR ou temps G4 dans
cette note. Elle combine la [borne corrélée de A](precore_cell_screen_20260923/README.md)
et la question des `K` gardes de rectangles WSPD, sans construire
`|A|²+|B|²` histogrammes locaux.

## Objet et certificat

Prendre un segment `E` de **vraies arêtes S2 survivantes** d'un rectangle
WSPD `A×B`, et une cellule `C=conv(V)` **polytopale compacte** qui
couvre une partie des centres q3/q4 admissibles de chacune de ces
arêtes. Toutes les cellules considérées ensemble doivent couvrir le
domaine de la voie avant de la fermer. Pour construire cette couverture
sans hypothèse flottante, on peut entourer chaque disque nominal de
centres d'une AABB : pour l'arête `ab`, centre `m=(a+b)/2`, prendre un
entier `r≥0` tel que `12r²≥|b−a|²` en q3, ou `8r²≥|b−a|²` en q4,
et les bornes `m_i±r`. Choisir `r` et arrondir les bornes vers
**l'extérieur** par comparaisons entières. L'AABB du segment est
l'enveloppe en coordonnées mondiales de ces boîtes,
intersectée avec la **boîte réelle du nuage** : les centres q3 aigus
et q4 positifs appartiennent à l'enveloppe convexe de leurs supports,
donc à cette boîte. Un découpage dyadique fermé de l'AABB obtenue
conserve les frontières et couvre tous ces centres. Si une autre
cellule paramétrique 2D est clippée, elle acquiert de nouveaux sommets
qui ne peuvent être oubliés ; dans un segment multi-arêtes, les plans
bissecteurs diffèrent, ce qui rend la couverture 2D particulièrement
fragile. Les cellules non polytopales requièrent une autre preuve.
Pour chaque sommet `v` de `C`, calculer exactement

`Q_E(v)=min_(a,b)∈E (|a−v|²+|b−v|²)`.

Pour un nœud `N` de l'index **global du même nuage**, de boîte fermée
`[l_N,h_N]`, le majorant de la distance de chacun de ses sites est

`U_N(v)=Σ_{i=1}^3 max((l_{N,i}−v_i)²,(h_{N,i}−v_i)²)`.

Si `2U_N(v)<Q_E(v)` à **chaque sommet** de `C`, alors **tous** les
sites réels de `N` sont strictement intérieurs à chaque boule passant
par une arête de `E` et centrée dans `C`. Le nœud entier peut donc
créditer sa **population**, au lieu de tester ses sites un par un.

Preuve : pour toute arête `e=(a,b)` et tout site `g∈N`, la fonction
`Φ_{e,g}(c)=2|g−c|²−|a−c|²−|b−c|²` est affine en `c`. À chaque sommet
`v`, `Φ_{e,g}(v)≤2U_N(v)−Q_E(v)<0`. Une fonction affine strictement
négative à tous les sommets reste négative dans la cellule entière.
Au centre réel d'une boule passant par `a,b`, les deux dernières
distances valent son rayon² : `Φ<0` signifie que `g` est **strictement
intérieur**. L'inégalité stricte traite les contacts correctement ;
une égalité ne donne aucun crédit. Le `max` de distance à une boîte se
calcule axe par axe, sans parcourir ses huit coins.

Le parcours de l'index doit créditer des nœuds **disjoints** (ne pas
descendre un nœud crédité), vérifier les sites distincts du nuage
préparé, et exclure conservativement toutes les plages originales
`A∪B` des gardes ; si un nœud les chevauche, le raffiner ou le refuser.
Il ne doit additionner ni les crédits S2, ni les crédits de cellules
différentes. Dans chaque cellule pertinente, `K−1` gardes distincts
ferment la voie q3 et aussi q4 ; `K−2` ferment q4 seule. Les domaines
q3 et q4 peuvent partager une partition, mais seules leurs cellules
respectivement intersectées doivent être prouvées.

Ce renforcement **ne change pas** le
[lemme de redondance](CERTIFICAT_B_REDONDANCE_CELLULE_UNIQUE_20260923.md) :
une cellule unique contenant le disque nominal complet d'une arête
encore ouverte ne gagne rien sur S2, même avec `Q_E` exact et des
nœuds entiers. Il faut des **sous-cellules de centres à gardes
possiblement différents**, ou une restriction des centres réellement
plus forte que le disque.

## Travail borné, puis repli exact

Le flux S2 sait déjà énumérer les segments `E` en `O(R+S)`, où `R`
est le nombre de rectangles ouverts et `S` les arêtes survivantes ;
ne jamais reformer `A×B`. Le calcul de `Q_E(v)` coûte `|E|` évaluations
par sommet **distinct** : une grille de 2×2×2 cellules AABB n'a qu'au
plus **27 sommets distincts**, et non 8×8, si les valeurs sont
mutualisées. Avec un nombre **fixé** de cellules et sommets, cela reste
`O(S)`, mais le facteur réel doit être mesuré. Les minorants par boîtes
`L₁/L₂` moins chers peuvent être essayés avant `Q_E`, qui ne se calcule
que pour les segments/cellules encore prometteurs. La recherche des nœuds
ajoute au plus `R×cellules×budget_visites` propositions si le budget
d'effort est fixe. Mettre des limites sur **les tentatives de preuve**
(par exemple 2/4/8 cellules et 64 nœuds visités chacune), jamais sur
les arêtes exactes : une preuve épuisée retourne sans omission au
cœur/cover/générateur existants. Un découpage adaptatif sans budget,
une recherche de gardes repartie pour chaque arête ou un scan global
des sites pour chaque cellule peuvent réintroduire un carré caché.

Pour les **AABB mondiales subdivisées dyadiquement** proposées ici,
tous les sommets deviennent entiers après une échelle commune `2^p`.
Ce n'est **pas** vrai pour une cellule 2D paramétrique clippée :
ses nouveaux sommets peuvent avoir d'autres dénominateurs, et exigent
des rationnels exacts ou une AABB extérieure vérifiée. Si chaque
coordonnée de notre cellule AABB est clippée dans le cube u18
`[0,M]³`, `M=2¹⁸−1`, chaque membre de la
comparaison en **sommes de distances directes** est
`≤6(2^p M)²<2⁶³` pour `p≤12` ; i64 signé suffit si les opérandes
sont promus avant soustraction/carré. La forme algébrique réarrangée
`2|v|²+min_e(h_e−2s_e·v)` peut avoir des **intermédiaires** plus
grands : l'évaluer en i128 vérifié ou rester aux distances directes.
Hors du domaine u18/dyadique, utiliser `i128` vérifié ou replier ;
aucun `double` ne tranche `<`.
Les nœuds admis se prêtent à
une réduction parallèle des populations, et `Q_E(v)` à une réduction
par arêtes, mais la découverte des nœuds et les petites tâches doivent
être empaquetées pour le GPU. Ce certificat n'enlève **pas** à lui seul
la matérialisation actuelle de `P` paires par S2 ni son refus
`P>2³¹−1` : le tuilage borné reste un chantier séparé pour plusieurs
dizaines de millions de points.

## Porte de réception proposée

Commencer par la [fixture corrélée A](precore_cell_screen_20260923/README.md),
où `Q_E` réussit et les bornes par boîtes échouent ; ajouter un nœud
contenant plusieurs gardes admis, un contact strict remplacé par
égalité, un nœud recouvrant `A` ou `B`, deux nœuds ancêtre/descendant
qui ne doivent pas être additionnés, et une cellule omise. Ajouter
un cas u18 proche de `M` à `p=12` : la somme directe tient en i64,
mais un produit `2(a+b)·v` de la forme réarrangée peut dépasser
`2⁶³−1`. Un mutant
de chacun doit produire une réponse géométrique incorrecte ou un
refus typé, pas seulement un crash. Comparer masques, catalogue
**clé par clé** et tour FULL contre le chemin sans certificat.

Le shadow doit publier par segment/cellule les arêtes et formes
**physiquement** épargnées, évaluations de `Q_E`, visites de nœuds,
populations créditées, reprises, temps et mémoire. Comparer brut et
sans-sol, tailles 8k/16k/32k puis **trames entières de plusieurs
séquences**, `K=5/10`, `s=8/10/12`, CPU mono/multi puis G4 seulement
si le coût total est prometteur. Le plafond global à cellule unique
mesuré à [1,345 % de la masse cœur](bbox_clipping_s2_20260923/README.md)
sur une trame brute K5 ne prédit pas le rendement des sous-cellules.
