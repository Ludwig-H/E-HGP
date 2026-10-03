# Orientation exacte avec certificat global i128

Tranche préparée depuis `372c296c367961ed55c0404478e9ba7bb9dba2b1`, après le
certificat de puissance q3. Qualification G4 close dans
[reuse1](../receipts/full_regular_vertical_20261003/reuse1/README.md), source
`ae817d09e`, profils u18/u21/u24 selon sa matrice. Aucun gain temporel
isolé ni taux de certification LiDAR n'est acquis. La provenance est
ajoutée à `tests/num/source_pins.json` sans réécrire ses entrées antérieures.

## Domaine et preuve de chaque intermédiaire

Les objets restent fermés : Sphere et Q4Candidate possèdent une ancre Point,
des coefficients relatifs N et un dénominateur D>0, produits par leurs
fabriques exactes. Poser M=2^B, B=18/21/24. Le certificat exige :

$$0<D<2^{124-3B},\qquad -2^{124-2B}<N_j<2^{124-2B}\quad(j=0,1,2).$$

Les seuils D/N sont 2^70/2^88, 2^61/2^82 et 2^52/2^76 pour les trois
profils. Les comparaisons signées n'utilisent pas abs(i128 minimum), ni
flottant, ni information sur la largeur locale du support.

Pour une requête orientation(a,b,c,centre), tous les points et l'ancre
appartiennent au même cube [0,M−1]^3. Chaque différence est calculée après
conversion signée et sa magnitude est <M. La composante de
cross(b−a,c−a) est le déterminant orienté de trois points dans un carré de
côté M−1. Ce déterminant est multiaffine dans leurs coordonnées ; son
maximum absolu est atteint aux coins, où il vaut 0 ou (M−1)^2. La normale
vérifie donc |normal_j|<M². **Cette borne ne vaut pas pour deux Vec
arbitraires** : (m,m) et (m,−m) donnent −2m².

$$|D(\mathrm{anchor}_j-a_j)|<2^{124-2B},\qquad |T_j|=|N_j+D(\mathrm{anchor}_j-a_j)|<2^{125-2B}.$$

Chaque multiplication D*offset et chaque addition donnant T est ainsi
représentable en i128. Les trois produits T_j*normal_j ont magnitude <2^125.
Leur somme absolue est <3·2^125<2^127 : elle borne le résultat **et chaque
somme partielle**, sans annulation supposée. Le signe natif est exact et
rentre aussi dans le budget public center_orientation, au moins127 bits.

## Propriété, sélection et limites

Le booléen privé `orientation_i128_` est distinct du certificat de puissance.
Chaque fabrique calcule son propre certificat depuis ses coefficients.
Q4Candidate le transmet à materialize avec les mêmes D/N ; cette opération
ne recalcule pas le certificat. Les copies et affectations transportent le
drapeau avec l'ancre et les coefficients, sans référence ni cache mutable.

Le code sélectionne seulement certifié→i128, sinon l'ancien Wide. Il
n'ajoute aucun essai checked. Aucune formule de Level, de puissance, de
CenterRegion ou de comparaison de centres n'est changée. Dans la branche
Wide, T reste calculé en i128 selon sa preuve antérieure globale <2^126.

Le booléen occupe le padding attendu avant les coefficients : Sphere reste
attendue à128/144/160 octets et Q4Candidate à80. Ce sont des attentes ABI,
à mesurer par les portes G4, pas un nouveau format de sérialisation.

Chaque candidat paie la certification même si sa première face suffit à
le rejeter. La réutilisation est possible sur les quatre faces et pendant
la canonicalisation, mais son intérêt temporel reste à mesurer. Le nombre
de boules émises ne mesure pas les orientations : q4_candidates compte les
candidats non dégénérés, chacun soumis à1..4 tests initiaux de centre ; la
canonicalisation peut en ajouter. Aucun temps de génération n'est attribué
à ce prédicat sans mesure séparée.

## Portes préparées

La porte native limites juge les seuils stricts, i128 minimum, les64 triplets
de coins du carré et le contre-exemple Vec :97 contrôles. La porte publique
juge24 permutations de tétraèdres petits et extrêmes, candidats avant
materialize, Sphere matérialisée et construction eager, contre Wide :1154
contrôles. La porte propriété/contact prépare34/55/57 contrôles selon B.
Ces nombres sont statiques ; aucune compilation native locale n'a eu lieu.

Le juge indépendant reconstruit le centre par Gram/Gauss Fraction et évalue
le déterminant rationnel du plan. Les plans sont indépendants des supports.
Il vérifie séparément coefficients, certificat, signes, convexité stricte
et représentations non réduites des niveaux. Les seuils D sont atteints
exactement par de vraies sphères q4 en u21/u24 ; les coefficients négatifs,
cas dégénérés et refus d'entrée sont inclus.

La famille régulière {0,(m,m,0),(m,0,m),(0,m,m)}, m=2^B−1, donne
D=4m³ et N=(2m⁴,2m⁴,2m⁴). Le plan x=y annule deux termes ±2m⁶ ; un
autre plan donne −2m⁶. En u24, les termes ont145 bits et une troncature
signée128 inverse effectivement ce dernier signe. Le repli Wide doit donc
être conservé, même pour un contact exact.

Modèles Python normal et −O identiques :488/491/491 requêtes,
8091/8151/8152 contrôles,90 corruptions refusées par profil. Style num46,
AST et JSON passent ; les portes natives et mutants passent dans reuse1.
Les451/343/329 requêtes certifiées sont des fixtures, pas un taux LiDAR.
Les mutations de seuil strict ou de drapeau jugent le contrat du certificat,
pas une corruption géométrique : les égalités aux seuils restent encore
sûres avec cette marge. Les mutations de signe natif/Wide jugent, elles,
des résultats géométriques faux. Aucun arrêt par UB n'est demandé.

## Premier essai G4 conservé

La [capture dense1](../receipts/full_dense_20261003/dense1_failure/README.md),
source768070ddb, échoue sur les portes Fraction de la sonde ; les portes
unitaires passent. La sonde parcourait une référence vers un Point temporaire
détruit en C++20. Le correctif e937aa72f conserve ce Point localement.
L'oracle perdait aussi stderr et le code de l'enfant ASan : aucune trace
du sanitizer ne peut être reconstruite. Le correctif d60ba2981 conserve
ces diagnostics et la première requête géométrique en défaut ; cinq cas
de processus simulés passent normal/−O. Le prédicat produit est inchangé.
La qualification complète passe dans reuse1, source `ae817d09e` :
3339/3339 portes et299/299 ASan18. Ce succès ne réécrit pas l'échec historique.
