# Supports q3/q4 et puissances exactes sur les float32 originaux

21 septembre 2026, après `028a0f1d`. Cadre :
`phase=exploration_v8_hors_registre`, `backend=cpu_reference`,
`profile=lossless_float32_input_only`, `mode=native_ball_predicates`,
`public_status=not_claimed`. Nouvelle brique numérique, sans remplacement
implicite du moteur u16 ni héritage de ses performances.

## Ce que cette brique fait

Le [support préparé](../src/core/float32_ball.hpp) répond à deux questions :

1. Ces trois ou quatre sites définissent-ils un support strictement positif ?
   Pour q3, le triangle doit être non dégénéré et strictement aigu ; pour q4,
   le tétraèdre doit être non dégénéré et son centre circonscrit strictement
   intérieur. Un centre sur une facette est refusé, pas toléré par epsilon.
2. Un autre site est-il strictement à l'intérieur, sur la sphère, ou dehors ?
   Le contact est un résultat exact, distinct de l'intérieur strict.

Les coordonnées sont leurs mots float32 originaux, sans passage par une
grille, soustraction float32 arrondie ou centre approché pris comme vérité.
Le centre de la boule q3 appartient au plan affine du triangle ; la requête
porte bien sur la sphère entière en 3D, pas seulement sur le cercle du plan.

Cela **ne génère pas encore** les supports utiles, ne compte pas leurs
témoins, ne collecte pas leurs coquilles et ne reconstruit pas la tour.
Un support n'est pas une clé de boule : deux supports différents peuvent
définir la même boule, même entre arités. Cette déduplication reste ouverte
pour le profil natif et nécessitera les clés globales canoniques.

## Préparer une fois, partager sans cache mutable

Un objet conserve quatre points binaires (le quatrième est inutilisé en
q3), l'arité, le mode et les intervalles d'un coefficient scalaire et de
trois coefficients linéaires : **128 octets** sur les deux builds qualifiés.
Il ne porte ni cinq grands entiers de clé,
ni index local, ni vecteur de témoins. Ses données sont privées et ses
affectations interdites ; les copies de valeur sont indépendantes.

En mode filtré, les coefficients d'intervalles sont calculés une seule
fois pendant la préparation. Chaque requête les réutilise avec son propre
point et son compteur privé. Quand l'intervalle ne permet pas de décider,
le worker recalcule les coefficients exacts depuis les bits, dans des
tableaux locaux fixes. Aucun cache paresseux partagé n'est modifié.
La validité du support n'est pas recalculée aux requêtes exactes : q4 évite
ainsi dix produits et dix additions de poids barycentriques par repli.

Le mode `ExactOnly` prépare et interroge sans opération arithmétique
flottante. Le mode `Filtered` ne conclut un signe que si son intervalle est
strictement séparé de zéro. Tous les contacts passent donc par l'exact.
Les deux modes décident les mêmes objets et signes, pas les mêmes coûts.

Cette organisation permet des supports partagés entre tâches et des
temporaires privés, mais **aucun kernel GPU n'est livré ici**. Le coût et
l'espace des temporaires exacts devront être pris en compte au port GPU ;
« sans allocation dynamique » ne signifie pas « sans mémoire locale ».

## Formules et précision

Pour q3, avec `d=b−a`, `u=c−a`, `D=d·d`, `E=u·u`, `F=d·u`, la validité
requiert `F>0`, `D−F>0`, `E−F>0`, et `G=D E−F²>0`.
La puissance relative a le signe de `G |z−a|²−W·(z−a)`, avec
`W=E(D−F)d+D(E−F)u`. Aucun centre fractionnaire n'est nécessaire pour
cette décision ; l'oracle indépendant peut donc employer une autre voie,
la résolution rationnelle du centre et des coordonnées barycentriques.

Pour q4, les trois déplacements sont `d,u,v` ; les cofacteurs sont
`u×v`, `v×d`, `d×u`, et `det=d·(u×v)`. Le numérateur du centre relatif
est `N=D(u×v)+E(v×d)+V(d×u)`, avec `V=v·v`. Les trois poids non initiaux
ont les numérateurs `N·cofacteur_i` et le dénominateur **positif** `2det²`.
Le quatrième numérateur vaut `2det²−somme(des trois autres)`.

Tester les quatre poids avant de normaliser l'orientation est impératif.
Changer seulement le signe de N avant ces tests inverserait trois poids
quand det est négatif. Une fois le support validé, le signe de puissance
est `sign(det |z−a|²−N·(z−a)) × sign(det)`. Les coefficients d'intervalles
sont normalisés ensemble avec ce même signe, certifié ou calculé exactement.

Le [nouvel entier signé fixe](../src/core/fixed_signed.hpp) emploie
54 mots u32, soit 1728 bits de magnitude et un signe séparé. Un nombre
float32 fini devient un entier dans l'unité commune `2^-149` ; toutes
les opérations suivent explicitement les degrés homogènes des polynômes.
Addition, soustraction et multiplication sont contrôlées ; un dépassement
ne peut pas se transformer en troncature silencieuse.

Avec `|coordonnée entière|<2^277` et `|déplacement|<M=2^278`, les majorants
conservateurs sont `|G|≤12M⁴`, `|W_i|≤36M⁵`, puissance q3 `≤144M⁶`,
`|det|≤6M³`, `|N_i|≤18M⁴`, puissance relative q4 `≤72M⁵` et tous les
intermédiaires des poids q4 `≤396M⁶<2^1677`. La capacité suffit donc aux
opérations réellement codées. Ce n'est pas une permission de former plus
tard les produits naïfs de degré9 des comparateurs de racines.

Le chemin flottant travaille en unités physiques, non dans l'unité entière.
Chaque addition/soustraction/produit est encadré vers l'extérieur ; les
extrémités minuscules sont élargies aux doubles normaux afin de rester
conservatrices sous FTZ/DAZ. Les bornes de degré au plus6 restent sous
`2^783`, loin du débordement double. Une différence entre deux coordonnées
float32 n'est jamais supposée représentable exactement en double.
Compilation stricte, sans fast-math, sans contraction implicite FMA.

## Travail, limites et suite

Dix-huit compteurs séparent préparations q3/q4, décisions filtrées, replis,
supports acceptés/refusés, requêtes, décodages et opérations arithmétiques.
Une décision filtrée de préparation peut être un refus du support ; ne pas
la lire comme un support accepté. Les additions incluent les soustractions ;
copies, négations et coût des mots entiers ne sont pas ces compteurs.
Un mode invalide est refusé avant tout compteur ; les autres exceptions
peuvent laisser le travail déjà réalisé, jamais un signe fictif.

Le nombre d'opérations par préparation/requête est borné indépendamment
du nombre de sites. La multiplication entière est quadratique en **nombre
de mots actifs** (au plus54), pas en taille du nuage. Cela ne prouve rien
sur le nombre de supports proposés ni sur le coût total du futur census.
Les mesures8k/16k/32k de l'index précédent ne sont pas réinterprétées comme
des mesures de cette brique ou de la chaîne q3/q4 native.

Prochain raccord : clés canoniques globales créées seulement aux émissions,
comparaisons réduites des événements q4, bornes certifiées de blocs puis
partage des graines et tâches intérieures. Ne pas passer les fichiers
float32 dans les anciens lecteurs u16. Aucune nouvelle campagne GCP n'est
utile avant ce raccord ; les contrats FULL/G4 restent ouverts.

## Qualification

Les [captures locales](../receipts/float32_ball_20260921/README.md) Release
et Clang ASan/UBSan passent, douze commandes chacune :

- 1636 cas contre résolution rationnelle indépendante du centre ;
- 53 entrées invalides refusées, 13 mutations de résultats détectées ;
- 1287 contrôles natifs, 548 requêtes contrôlées, quatre modes d'arrondi
  et quatre combinaisons FTZ/DAZ ;
- 128 lectures concurrentes avec quatre threads sur deux supports partagés ;
- 948 contrôles de l'entier fixe, dont dix débordements refusés avec
  préservation des opérandes et six encodages non finis rejetés.

| Cas, pas nombre de supports distincts | Positifs | Refusés | Intérieur / contact / extérieur |
|---|---:|---:|---:|
| q3 | 294 | 155 | 81 / 117 / 96 |
| q4 | 731 | 456 | 160 / 408 / 163 |

Les tests comprennent les permutations, triangles rectangles/obtus,
tétraèdres coplanaires et centres sur facettes, voisins float32 de ces
frontières, zéros signés et annulations avec 277 exposants d'écart.
Le test `ExactOnly` vérifie aussi l'absence de modification des drapeaux
d'exception flottante. Ces fixtures adversariales ne mesurent pas le taux
de repli attendu sur LiDAR. Aucun chrono moteur ni résultat GPU n'en découle.

Trois erreurs sont également introduites dans des copies du code puis
recompilées : poids nul admis, signe d'orientation omis, exposants normaux
mal décodés. Chacune produit la mauvaise réponse géométrique attendue,
avec sortie normale du programme ; ni crash ni simple corruption de
compteur ne vaut détection. Voir les [mutations causales](../receipts/float32_ball_20260921/mutations/README.md).
