# Pourquoi une seule cellule de centres ne peut presque rien gagner après S2

23 septembre 2026. Contrelecture mathématique indépendante de la
[proposition de gardes avant cœur](CONTRE_AUDIT_B_CROISSANCE_Q34_AVAL_S2_20260923.md).
Cadre : q3 aigu et q4 positif, grille entière 1 mm/u18, arête `ab`
propriétaire maximale, voie encore ouverte après le **filtre ponctuel S2
exact**. Il s'agit d'un verrou pour choisir un shadow, pas d'un résultat
chronométrique ni d'une borne globale sur le générateur.

## Lemme de redondance

Poser `d=b−a`, `D=|d|²`, `m=(a+b)/2` et `y=g−m`. Les centres des
présentations possédées appartiennent au plan `c=m+t`, `t·d=0`, avec
`|t|²≤D/12` pour q3 et `|t|²≤D/8` pour q4. Pour un garde `g`, la
condition « `g` est strictement intérieur à **toutes** les boules
passant par `a,b` et centrées dans le disque fermé de rayon `ρ` » est

`D/4−|y|² > 2ρ |proj_{d⊥}(y)|`.

En effet, la différence entre rayon au carré et distance de `g` au
centre est `D/4−|y|²+2y·t` ; son minimum sur le disque est le membre
gauche moins `2ρ |proj_{d⊥}(y)|`. Avec `H₄=D−4|y|²` et
`Ξ=|d×y|²=D|proj_{d⊥}(y)|²`, la stricte condition devient exactement

- q3 : `H₄>0` et `3H₄²>16Ξ` ;
- q4 : `H₄>0` et `2H₄²>16Ξ`.

Ce sont les tests d'admission **singleton** déjà utilisés par S2
(`gen/lanes/q34_witness_search.cpp`, spécialisation `Affine`). Le
parcours de l'index compte tout site distinct admis, jusqu'à `K−1`
pour q3 ou `K−2` pour q4. Si une cellule unique `C_E` contient le
disque nominal entier d'une arête survivante `e∈E`, un garde certifié
sur tout `C_E` aurait donc déjà été compté sur `e` par S2. Il est
impossible d'obtenir `K−1` ou `K−2` de ces gardes pour fermer sa voie
encore ouverte. Le minorant plus fort
`max(L₁,L₂)` ne contourne pas ce lemme : il ne fait que prouver la
même intériorité sur `C_E`.

La boîte `M_E` élargie d'un entier `r` tel que `8r²≥Dmax` contient
chaque disque q4 nominal **avant clipping** ; diviser `E` en segments
en reconstruisant une telle boîte entière pour chacun ne change donc
rien. Pour fermer **tout** un segment par un seul jeu de gardes, il
suffit qu'une de ses arêtes ouvertes ait son disque nominal entier
dans la cellule pour obtenir l'obstruction. Il faut soit partitionner
le **domaine des centres** en sous-cellules et autoriser des gardes
différents par cellule, soit prouver une restriction des centres plus
forte que le disque nominal.

## Exception sûre : une restriction du domaine réel

Un centre q3 aigu ou q4 positif appartient au simplexe de son support,
donc à la boîte **réelle du nuage** ; intersecter `C_E` avec cette
boîte (au minimum avec le cube numérique `[0,262143]³`) est sûr.
Lorsque l'intersection coupe le disque nominal, un garde peut être
universel sur les centres **réellement possibles** sans satisfaire le
citron S2 calculé sur le disque entier. Cette exception est
mathématiquement réelle, mais son abondance sur LiDAR est inconnue.
Le [manifeste brut 08/000000](../../morsehgp3D_v8/receipts/float32_precision_20260921/release_r2/precision_a1drpf9i/scene_00_000000_grid/MANIFEST.json)
donne des maxima encodés `158607,158284,30595` pour des minima zéro :
en `z`, la boîte réelle est donc bien plus serrée que le cube numérique.

Exemple entier K5 : `a=(5,0,0)`, `b=(15,0,0)`, et quatre gardes
`(10,3,0)`, `(10,3,1)`, `(10,3,2)`, `(10,4,0)`. Ici `D=100`, `r=4` et
la boîte de centres, clippée à `y,z≥0`, est
`C=[6,14]×[0,4]×[0,4]`. Chaque garde y est strictement intérieur :
la différence affine `2|g−c|²−|a−c|²−|b−c|²` est maximale en
`y=z=0`, où elle vaut respectivement `−32,−30,−24,−18`. Pourtant
chaque garde échoue le test singleton q3 sur le disque complet
(`H=D/4−|g−m|²` vaut `16,15,12,9`, chaque valeur étant inférieure à
`2√(D/12)|g−m|`) ; q4 échoue aussi. S2 ne les créditerait pas, tandis
que la cellule clippée ferme q3 avec quatre gardes, donc aussi q4.
Ce n'est pas un exemple de gain mesuré sur SemanticKITTI.

## Décision expérimentale

Ne pas porter le test de **cellule unique non clippée** : sur les
survivants S2, son nombre de fermetures utiles doit être nul sous les
hypothèses ci-dessus. Le shadow économique doit distinguer :

1. la boîte globale **réelle** du nuage et la fraction d'arêtes/segments
   dont le disque nominal touche cette boîte ; une cellule unique ne
   peut aider que là ou avec une restriction plus forte ;
2. plusieurs sous-cellules couvrant les centres, avec jeux de gardes
   distincts et seuils `K−1`/`K−2` appliqués **dans chaque** cellule ;
3. le coût de préparation/recherche des gardes et le nombre de formes
   cœur/cover **effectivement évitées**, en conservant le repli exact.

Une subdivision des centres trop fine ou une recherche de gardes
répétée par arête déplacerait le carré au lieu de le supprimer.

La première strate se mesure **exactement en O(S)**, sans recherche de
gardes. Dans la boîte globale réelle `[lo,hi]³`, pour une arête poser
`qᵢ=min(aᵢ+bᵢ−2loᵢ, 2hiᵢ−aᵢ−bᵢ)` et `D=|b−a|²>0`. La projection du
disque nominal sur l'axe `i` a demi-largeur
`ρ√(1−dᵢ²/D)` ; tout le disque est dans la boîte si et seulement si,
pour **chaque** axe,

- q3 : `3qᵢ² ≥ D−dᵢ²` ;
- q4 : `2qᵢ² ≥ D−dᵢ²`.

Les deux tests sont entiers et n'utilisent pas de racine carrée.
Pour une voie et un segment `E`, **une seule** arête survivante dont le
disque entier est dans la boîte exclut une fermeture de tout `E` par
un jeu unique de gardes sur sa cellule non subdivisée. Publier la
fraction de segments, arêtes et formes de référence pour lesquels
**tous** les disques pertinents touchent une frontière ; c'est un
plafond de potentiel de la cellule unique, pas une prédiction de gain.

Pour le vrai shadow, les [masses des rectangles bruts
08/000000](q34_raw_rectangle_mass_20260923/README.md) donnent un
budget de tentative préfixé : à K5/K10, les rectangles **ouverts avant
le filtre ponctuel** de produit ≥64 sont 42 020/54 822, portant
62,46/63,52 % des paires développables. Leur nombre de survivants et
leurs formes évitables sont encore inconnus. Huit sous-cellules par
segment, chacune limitée à 64 nœuds d'index et au plus `2T` gardes
réels (`T=K−1` pour fermer q3+q4), représenteraient au plus
21,5/28,1 M visites de recherche et 21,5/63,2 M comparaisons
garde–sommet à K5/K10 **si tous ces rectangles restent ciblés**.
Le coût minimal des seules comparaisons pour réussir partout serait
déjà 10,76/31,58 M : ne pas faire de ces nombres un gain annoncé.
Arrêter cette piste expérimentale si, sur mêmes trames brutes et sans
sol en 8k/16k/32k puis entières, les arêtes complètement fermées
n'épargnent pas assez de formes et de temps de chaîne pour rembourser
la recherche, le transport et les replis. Le moteur exact continue
toujours sur les cas non certifiés.
