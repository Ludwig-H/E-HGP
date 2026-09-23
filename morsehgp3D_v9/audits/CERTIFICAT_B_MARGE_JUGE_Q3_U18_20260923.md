# Certificat B — marge flottante du juge q3 u18

23 septembre 2026. Contrelecture du
[`q3_sample_judge.cpp`](c_omission_20260923/q3_sample_judge.cpp)
publié par `85a4d4ab`. Elle concerne **uniquement le juge indépendant
hors produit**, pour des points entiers u18 et des triangles strictement
aigus. La preuve est conditionnelle à l'arithmétique IEEE binary64
conforme, conversions entières comprises, **sans `fast-math`**. Elle
ne valide ni un moteur float32, ni une autre arité, ni la complétude
du catalogue au-delà des ancres échantillonnées.

## Résultat

Le juge recense exactement les intérieurs stricts et la coquille aux
feuilles en `i128`. Aux nœuds, il emploie un centre/rayon en `double`
et ne coupe l'arbre que si `dmin > r2 + margin` ou
`dmax < r2 - margin`, avec `margin = 1e-6*(r2+1)+4`. Sous les
hypothèses ci-dessus, **aucune de ces deux coupes ne peut perdre un
intérieur ou un contact**. La marge n'est donc pas simplement
heuristique pour cette population, bien que son code ne porte pas
encore cette preuve ni une vérification explicite du profil flottant.

## Borne entière et géométrique

Poser `M=262143<2^18`, `u=b-a`, `v=c-a`, `w=u×v`,
`D=2|w|²` et
`P=|u|²(v×w)+|v|²(w×u)`. Tous ces objets sont calculés avant
conversion flottante. Les produits sont bornés par
`|w_i|≤2M²`, `D≤24M⁴<2^77`, `|P_i|≤24M⁵<2^95` ; le prédicat
entier de point vérifie `|s(x)|<2^116`, dans `i128` signé.
Le triangle strictement aigu a son circoncentre **dans** le triangle :
son centre exact `O=a+P/D` est dans `[0,M]^3`, d'où
`|P_i/D|≤M`. En particulier, toutes les conversions sont finies ;
un quotient non nul a module au moins `1/D>2^-77`, et ses carrés
non nuls restent normaux, même sous FTZ/DAZ.

## Budget d'arrondi

Pour chacun des quatre modes d'arrondi IEEE, prendre la majoration
conservatrice `ε=2^-52` de l'erreur relative d'une opération normale
ou conversion entière. Les entiers des boîtes sont représentés
exactement en binary64. Les trois arrondis de `double(P_i)/double(D)`
donnent `|ox_i−P_i/D|<4εM`. L'addition de `a_i` donne
`|cx_i−O_i|<6εM`.

Le calcul des trois carrés et deux sommes du rayon donne
`|r2_calc−R²|<64εM²`. La distance d'une coordonnée du centre à
l'intervalle d'un nœud, et sa distance à l'extrémité la plus éloignée,
sont des fonctions **1-lipschitziennes** du centre. Leur valeur calculée
diffère de la vraie de moins de `8εM`; carrés et sommes donnent alors
`|dmin_calc−dmin_vrai|<80εM²` et la même borne pour `dmax`.
L'arrondi supplémentaire de `r2_calc ± margin` coûte moins de
`4εM²`. Ainsi une comparaison pourrait se tromper de moins de
`148εM² < 256εM² < 0,003907`, alors que `margin≥4`.
Une coupe « extérieur » implique donc `dmin_vrai>R²` ; une coupe
« tout intérieur » implique `dmax_vrai<R²`. Les égalités et contacts
descendent toujours. Les extrema de distance à une boîte fermée
s'encadrent ainsi tous les sites du nœud.

## Conditions à verrouiller

Ajouter au juge un contrôle compilatoire du type
`numeric_limits<double>::is_iec559 && digits==53`, épingler les flags
**sans `-ffast-math`** et rappeler la restriction « triangle strictement
aigu, entrée u18 » à côté de `census()`. Si le juge doit un jour
fonctionner hors de ces conditions, revenir aux bornes de boîte
**entières exactes** : pour chaque axe, le maximum du polynôme
convexe est à une extrémité et le minimum aux entiers voisins de
`a_i+P_i/D` (division signée plancher/plafond), puis sommer les trois
extrema avec les règles de contact strictes. Cette alternative avait
été transmise en coordination ; elle n'est **pas nécessaire** pour
certifier le filtre flottant actuel sous ses hypothèses.
