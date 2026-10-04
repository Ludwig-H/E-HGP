# Certificat global i128 préparé avec la sphère q3

Source de départ `220047c0725273810ff420d97196ac5639da9378`.
Cette tranche est préparée pour une qualification distincte. Aucun test natif
local, gain temporel ou taux de certification LiDAR n'est acquis ici.
La provenance s'ajoute à `tests/num/source_pins.json` sans remplacer celle de
l'essai contrôlé décrit dans [PREDICATS_I128_CONTROLES.md](PREDICATS_I128_CONTROLES.md).

## Domaine et preuve

Poser $M=2^B$, avec B=18/21/24. Les coefficients relatifs à l'ancre sont ceux
de la fabrique q3, sans réduction ni réancrage ; D est strictement positif.
Le helper privé certifie simultanément les quatre inégalités suivantes :

$$0<D<2^{123-2B},\qquad -2^{124-B}<N_j<2^{124-B}\quad(j=0,1,2).$$

Les seuils D/N valent respectivement 2^87/2^106, 2^81/2^103 et
2^75/2^100 en u18/u21/u24. Les comparaisons signées strictes n'utilisent
ni valeur absolue signée, ni négation d'un coefficient, ni conversion Wide.
Les puissances de deux et leurs négations sont représentables en i128.

Pour tout Point certifié z, l'ancre a est également un Point du profil.
La soustraction signée donne donc $|z_j-a_j|<M$ sur tout le domaine global,
quelle que soit la largeur locale des trois points du support.
La norme est inférieure à $3M^2<2^{50}$ ; les facteurs $-2(z_j-a_j)$ ont
magnitude inférieure à $2M$. Ils sont exacts avant les multiplications i128.
Le certificat entraîne :

$$|D\lVert z-a\rVert^2|<3\cdot2^{123},\qquad |{-2N_j(z_j-a_j)}|<2^{125}.$$

La somme des magnitudes des quatre termes est inférieure à
$15\cdot2^{123}<2^{127}$. Elle borne **chaque produit et chaque somme
partielle**, sans supposer d'annulation. La voie signée native est donc sûre
pour tous les points du profil. Les budgets historiques de la fabrique et
`require_fit<Budget::side>` restent nécessaires et inchangés ; le helper
sur coefficients synthétiques n'est pas une API de construction de Sphere.

Pour `power_bounds`, les deux normes séparables sont chacune inférieures à
$3M^2$, et les six facteurs linéaires ont magnitude inférieure à $2M$.
Les mêmes quatre majorants s'appliquent séparément à chaque borne, même si
le centre est extérieur au domaine. `checked_bounds` conserve ses contrôles
d'ordre et de largeur avant publication. Cette preuve concerne Box, dont
les deux extrémités sont des Point ; elle n'est pas transférée à CenterRegion,
à l'orientation, aux niveaux, aux comparaisons de niveaux ou de centres.

## Propriété et voies de calcul

Seule `Sphere::through(a,b,c)` calcule le certificat après la construction
exacte des coefficients. Le booléen privé `q3_power_i128_` est initialisé dans
chaque constructeur, copié et affecté avec tous les coefficients. Le getter
`q3_power_i128_certified()` ne permet pas de fabriquer une sphère. Les autres
arités ont ce drapeau spécifique q3 à false ; leurs preuves natives existantes
restent sélectionnées par l'arité. Q4Candidate ne reçoit aucun nouveau champ.

Le booléen est placé après le Point et le tag d'arité, avant les coefficients
alignés. Les tailles attendues restent Sphere128/144/160 et Q4Candidate80 ;
seule la prochaine G4 pourra attester ces ABI. La trivialité et l'alignement
requis par Buffer sont conservés par le code, sans cache mutable ni allocation.
Les coefficients et les Level non réduits restent identiques.

Un q3 certifié utilise la voie native existante pour power, side et bounds.
Sinon, l'essai contrôlé par builtins puis le repli Wide restent intégralement
présents. Un certificat absent n'affirme donc pas qu'une requête particulière
déborde. Les contacts et les annulations conservent leurs valeurs exactes.

## Portes préparées et portée

Les tests natifs prévus comprennent :

- limites privées D/N strictes, coefficients négatifs et i128 minimum,
  puis huit combinaisons de signes extrêmes contre Wide :54 contrôles ;
- permutations de supports petits et extrêmes, copies/affectations, puissances
  et bornes contre Wide, tailles :279 contrôles (plancher275) ;
- passage petit→grand→petit, q1/q2/q4 distincts, annulation H=0 avec produit
  trop grand, égalité exacte au seuil D et support local19bits :29/58/66
  contrôles u18/u21/u24 (plancher29).

Une requête proche de l'ancre d'une sphère globale non certifiée produit
une puissance négative représentable : la voie contrôlée reste exercée
après introduction du certificat. Les deux anciens mutants du résultat
rapide contrôlé sont à raccorder à cette nouvelle porte, car les petites
sphères certifiées ne passent plus par cette branche.

Le juge Gram/Gauss Fraction réutilise les fixtures de l'essai contrôlé, ajoute
les deux côtés du seuil D et son égalité exacte dans de vraies sphères, et
un triangle obtus u24 dont D passe mais un coefficient N dépasse son seuil.
Il juge les coefficients, les résultats publics, les bornes et le certificat
séparément ; il ne reprend pas le produit vectoriel de la fabrique.

Modèles Python normal/−O identiques :330/333/334 requêtes,
3945/3989/4005 contrôles,34 corruptions refusées par profil.
Les nombres certifiés/non certifiés sont322/0,215/110 et123/203 parmi les
requêtes valides ; **ce sont des fixtures, pas un échantillon de LiDAR**.
Un premier contrôle auxiliaire refusait à tort les coefficients négatifs,
car son dictionnaire conservait seulement la dernière permutation par nom ;
ce contrôle parcourt désormais toutes les géométries. Aucun défaut produit
n'était impliqué dans cet échec de modèle.

Les mutants proposés relâchent les seuils D/N ou omettent la publication du
certificat par la fabrique. Les deux égalités relâchées peuvent encore être
sûres arithmétiquement : ces mutations jugent le contrat strict du booléen,
pas une corruption géométrique. Le drapeau omis juge la publication du
certificat ; les deux anciens mutants contrôlés modifient réellement le résultat.
Leurs gates contrôlent le drapeau avant d'exécuter
une puissance potentiellement trop large : pas de mort revendiquée par UB.
Compilation, exécution native, mort causale et temps FULL restent à qualifier.
