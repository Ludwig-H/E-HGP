# Noyau numérique exact

`src/num/num.hpp` expose les entiers à budget, les niveaux rationnels non réduits,
les points validés, les sphères par 1 à 4 sites et leurs prédicats exacts.
Le profil se choisit par `MHGP11_COORD_BITS`, parmi 18, 21 et 24.
La première tranche est qualifiée sur ces trois profils sur G4 à `a97180667`.
La nouvelle sélection des voies de puissance décrite ci-dessous attend sa propre
qualification. Aucun build ni test natif local n'a été exécuté dans cette reprise ;
les reçus G4 gardent aussi les premiers échecs.

`source_pins.json` épingle les trois sources R2 effectivement lues. La qualification
porte sur ce port ; il ne donne encore ni catalogue, ni identité canonique de
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
natives signées restent représentables. L’orientation de centre passe aux
entiers larges lorsque son budget l’exige. La puissance emploie les bornes par
présentation décrites ci-dessous ; son type de résultat conserve le budget
générique de cette table.

Les comparaisons de niveaux multiplient à largeur complète numérateur et
dénominateur ; aucune approximation n’intervient. L’addition, la soustraction
et la réduction de largeur conservent la sortie précédente sur refus, y
compris lorsque sortie et entrée partagent le même objet.

Les portes déclarées dans `tests.cmake` sont les sept groupes unitaires et
inventaire, le refus de compilation d’un budget invalide, et
`mhgp11_num_fraction` / `mhgp11_num_fraction_opt` (`oracle;fast`). Le juge
Fraction résout le système de Gram par élimination exacte, distincte des
formules de centre du produit. Il confronte 504 configurations, dont des
dépendances affines, permutations, centres extérieurs et coordonnées extrêmes,
et 160 paires d’entiers signés allant jusqu’à 256 bits. Il contrôle aussi
l’intérieur strict par des coordonnées barycentriques calculées séparément.
La sonde publie aussi le signe `side`, une exécution entièrement large de la
puissance issue du harnais et l’arité de présentation. Fraction juge la valeur
et le signe indépendamment, puis compare cette exécution large supplémentaire.
Les 504 configurations restent inchangées : 454 non dégénérées, soit 7 526
contrôles attendus avec les entiers. Le manifeste vise désormais treize erreurs
de résultat ; les neuf premières historiques ont été qualifiées sur u18, les
quatre nouvelles doivent exercer explicitement u21 ou u24.

## Puissance native par arité — qualification en attente

`Sphere::presentation_arity()` est fixé par les fabriques et conservé par les
copies. Il décrit le support fourni, pas `qmin` ni une identité canonique.
Q1, q2 et q4 utilisent `i128` aux trois profils ; q3 utilise `i128` en u18,
puis `Wide` en u21/u24. `side` peut lire directement le signe natif, tandis
que `power` conserve sa conversion vérifiée vers `SideInt`.

La preuve borne la somme des magnitudes des termes de $H=D\lVert z-a\rVert^2-2N\cdot(z-a)$ ; elle couvre donc chaque produit et chaque somme partielle, même si le résultat final est nul.

| Présentation | Borne stricte de toute somme partielle | Budget suffisant | Voie native |
| --- | --- | --- | --- |
| q1 | 3 M² | 2B+2 | 18/21/24 |
| q2 | 12 M² | 2B+4 | 18/21/24 |
| q3 | 216 M⁶ | 6B+8 | 18 |
| q4 | 72 M⁵ | 5B+7 | 18/21/24 |

Pour q4, une composante de `(b-a)×(c-a)` est le déterminant orienté de trois
points dans un carré de côté `M−1`. C’est une fonction multiaffine des six
coordonnées : le maximum de sa magnitude se trouve aux coins, où ses valeurs
sont `0` et `±(M−1)²`. D’où `|cross_j|<M²`, seulement pour cet ancrage commun.
Cramer donne alors `D<6M³` et `|N_j|<9M⁴`. Le premier terme et chacun des
trois termes suivants sont strictement inférieurs à `18M⁵` en magnitude.
En u24, `72M⁵<2¹²⁷` ; les entiers signés `i128` suffisent sans supposer que
le centre appartient à la boîte. Les budgets des vecteurs arbitraires restent
inchangés dans le produit.

La porte `power_paths` confronte toutes les arités aux calculs larges du harnais,
avec petits supports, requêtes globales, coquilles et permutations déjà présentes
dans Fraction. Elle fixe aussi les tailles attendues de `Sphere` sur les ABI
G4, 128/144/160 octets selon le profil ; ce ne sont pas des tailles de format
de fichier ni une promesse d’ABI publique.

Le témoin `L=2^B−1`, `a=(0,0,0)`, `b=(L,L,0)`, `c=(L,0,L)`, `z=(0,L,L)`
donne q3 `N=(4,2,2)L⁵`, `D=6L⁴` et `H=4L⁶` : 110/128/146 bits de magnitude.
À `z=b`, `H=0` mais le premier produit vaut `12L⁶`, déjà hors `i128` en
u21/u24. Ces deux faits vérifient le repli et les annulations ; une borne sur
la seule valeur finale serait insuffisante. Quatre mutants visent le signe
natif q4 en u24, les termes du repli q3 en u21 et une coquille mal classée
par `side` natif en u24, et un q3 tagué q4 en u21. Cette dernière mutation
est refusée par le contrôle d’arité avant tout calcul hors précondition,
afin de mourir par code du juge et pas par un débordement signé. Leurs morts
restent à établir sur G4.

## Qualification historique avant ces nouvelles voies

Qualification G4 close sur `a97180667` : les six groupes natifs jouent 145 contrôles
aux profils 18/21/24 ; chaque oracle normal/−O joue 6 164 contrôles. Les neuf
mutants num meurent par code du juge au profil 18. Release, ASan/UBSan, TSan et
poison passent ; Clang était absent. Les échecs antérieurs du banc restent
conservés dans [`DEVELOPPEMENT.md`](../../docs/DEVELOPPEMENT.md).
