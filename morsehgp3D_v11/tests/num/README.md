# Première tranche numérique exacte

`src/num/num.hpp` expose les entiers à budget, les niveaux rationnels non réduits,
les points validés, les sphères par 1 à 4 sites et leurs prédicats exacts.
Le profil actif reste u18 ; les mêmes expressions et portes sont préparées pour
les compilations 21 et 24 bits. Aucune porte n’a été exécutée localement pendant
la préparation : la qualification appartient à la matrice G4 du développeur.

`source_pins.json` épingle les trois sources R2 effectivement lues. Il faut
requalifier ce port ; il ne donne encore ni catalogue, ni identité canonique de
boule, ni tri parallèle, ni tour FULL. Les niveaux sont comparables exactement
mais ne constituent pas une sérialisation canonique réduite. Aucun filtre
flottant ni contrat de performance n’est introduit.

Les fabriques `Point::make` refusent toute coordonnée hors de `[0,2^B)`.
`Level::make` refuse les numérateurs négatifs, les dénominateurs non positifs et
les valeurs dépassant leurs budgets. `Sphere::through` rend un succès avec une
option vide si les sites sont affinement dépendants ; un centre valide n’est
pas nécessairement dans l’enveloppe convexe, propriété testée séparément.
Les violations d’une borne interne rendent `arithmetic_invariant`. `Wide`
expose son stockage de primitive ; `Point`, `Level` et `Sphere` ferment leur
construction et ne permettent pas de modifier leurs coordonnées par l’API.

Pour `M=2^B`, chaque différence de coordonnées a une magnitude strictement
inférieure à M. Les sommes partielles satisfont les mêmes majorants absolus.

| Expression | Majorant strict de magnitude | Budget |
| --- | --- | --- |
| produit scalaire | 3 M² | 2B+2 |
| composante du produit vectoriel | 2 M² | 2B+1 |
| déterminant | 6 M³ | 3B+3 |
| N3 = (uu v − vv u) × (u × v) | 24 M⁵ | 5B+5 |
| D3 = 2‖u × v‖² | 24 M⁴ | 4B+5 |
| N4 de Cramer | 18 M⁴ | 4B+5 |
| D4 = 2 det | 12 M³ | 3B+4 |
| puissance D‖z−a‖² − 2N·(z−a) | 216 M⁶ | 6B+8 |
| orientation du centre | 288 M⁷ | 7B+9 |
| test de milieu, chaque membre | 96 M⁵ | 5B+7 |
| numérateur réduit de rayon q3 | 27 M⁶ | 6B+5 |
| dénominateur réduit de rayon q3 | 48 M⁴ | 4B+6 |
| numérateur du rayon q4 | 972 M⁸ | 8B+12 choisi |
| dénominateur du rayon q4 | 144 M⁶ | 6B+8 |

Le numérateur q3 provient de t, dont chaque composante est bornée par 6 M³,
puis de deux produits avec une composante de u×v. Pour les prédicats, on borne
N + D(a−p) par 48 M⁵ et chaque composante normale par 2 M². Cette preuve ne
suppose pas que le centre reste dans la boîte des sites. À 24 bits, N3 tient
dans 125 bits de magnitude et le test de milieu dans 127 : les opérations
natives signées restent représentables. La puissance et l’orientation de
centre passent aux entiers larges lorsque leurs budgets l’exigent.

Les comparaisons de niveaux multiplient à largeur complète numérateur et
dénominateur ; aucune approximation n’intervient. L’addition, la soustraction
et la réduction de largeur conservent la sortie précédente sur refus, y
compris lorsque sortie et entrée partagent le même objet.

Les portes déclarées dans `tests.cmake` sont les six groupes unitaires et
inventaire, le refus de compilation d’un budget invalide, et
`mhgp11_num_fraction` / `mhgp11_num_fraction_opt` (`oracle;fast`). Le juge
Fraction résout le système de Gram par élimination exacte, distincte des
formules de centre du produit. Il confronte 504 configurations, dont des
dépendances affines, permutations, centres extérieurs et coordonnées extrêmes,
et 160 paires d’entiers signés allant jusqu’à 256 bits. Il contrôle aussi
l’intérieur strict par des coordonnées barycentriques calculées séparément.
Le manifeste `tests/mutants/num.json` vise neuf erreurs de résultat, à rejouer
sur G4 ; leur seule déclaration ne signifie pas qu’elles sont tuées.
