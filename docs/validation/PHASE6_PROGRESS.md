# Progression Phase 6 — miniballs exactes et descentes

## Statut

- phase : `6`, `ready`; jalon préparatoire 6.1 livré pendant que la phase 5 reste l'unique phase `in_progress`;
- backend : `reference_cpu`;
- profil : `full_pi0`;
- mode : `certified`;
- portée courante : `local_facet_miniball_only`;
- porte d'entrée : satisfaite par les Phases 1 et 4 fermées;
- porte de sortie : non satisfaite; le miniball exact borné est livré, mais le shell global, le successeur top-$k$, la décroissance, le segment sous-niveau, le pointer-jumping, les attaches et le différentiel indépendant restent ouverts.

Ce premier jalon ne construit aucune forêt et ne publie aucun `public_status`. Il fournit seulement un oracle local exact pour une facette de cardinal au plus dix. Le support canonique choisi par l'API ne doit pas être confondu avec l'unicité d'un support essentiel ni avec l'éligibilité d'une descente.

## Réduction mathématique finie

Soit $F$ une facette finie de $\mathbb{R}^{3}$ et $B(c,r)$ sa boule englobante minimale. Les points actifs sur la frontière satisfont $c\in\mathrm{conv}(A)$. Le théorème de Carathéodory fournit une représentation convexe de $c$ portée par au plus quatre points de $A$; choisissons-en une de cardinal minimal et notons $S$ son support. Tous ses coefficients sont strictement positifs, sinon un point de coefficient nul serait supprimé. Le support $S$ est affinement indépendant : une dépendance affine permettrait de perturber les coefficients dans les deux sens en conservant leur somme et le point représenté, jusqu'à annuler au moins un coefficient sans en rendre aucun négatif, ce qui contredirait la minimalité du cardinal. Ainsi $c$ appartient à l'intérieur relatif de l'enveloppe convexe de $S$. La sphère de $S$ est donc calculée par `analyze_circumcenter_support`, et ce support appartient nécessairement à l'énumération.

Pour une facette de cardinal $k\leq 10$, le nombre de sous-supports inspectés est exactement $N(k)=\sum_{j=1}^{4}\binom{k}{j}$, avec $N(10)=10+45+120+210=385$. Les supports affinement dépendants sont rejetés. Un support `boundary_reduced` possède le même centre qu'un support positif strictement plus petit déjà énuméré; un support `exterior_circumcenter` n'est pas nécessaire au témoin de la boule minimale. Chaque support `minimal` restant est classifié exactement contre tous les points de $F$, sans arrêt anticipé.

Le vrai support de la miniball appartient donc aux candidats englobants. Minimiser leur rayon rationnel donne le rayon minimal. Le centre de la boule minimale est unique : si deux centres distincts de même rayon minimal englobaient $F$, leur milieu produirait par l'identité du parallélogramme une boule de rayon strictement inférieur. Le code exige ainsi l'égalité exacte des centres de tous les candidats au rayon optimal.

La boule peut néanmoins posséder plusieurs supports exacts. Le résultat choisit d'abord le cardinal minimal, puis le vecteur de `PointId` lexicographiquement minimal; `optimal_support_count` conserve le nombre total de supports positifs optimaux. Cette règle est une canonisation logicielle, pas une preuve d'unicité du support essentiel.

## API et certificat local

`build_exact_facet_miniball` canonise et valide la facette, énumère les supports de taille un à quatre, calcule centre et rayon avec des rationnels exacts, puis produit :

- la facette canonique;
- le support canonique;
- le centre et le rayon carré exacts;
- la partition des points de la facette entre intérieur strict et shell;
- les comptes par taille de support, les quatre décisions de support, toutes les classifications et le nombre de supports optimaux;
- `status=exact_facet_miniball_certified` et `scope=local_facet_miniball_only`.

`verify_exact_facet_miniball` traite tous ces champs comme non fiables et répète les 385 supports au maximum. Il compare facette, support, centre, rayon, partition, compteurs, statut et scope. Ce rejeu frais est fail-closed, mais réutilise volontairement le même algorithme exhaustif; il ne constitue pas un oracle logiciel indépendant. `reference/morsehgp3d_oracle/geometry.py::minimum_enclosing_ball` filtre maintenant lui aussi les circumcentres hors de l'intérieur relatif du support. Le cas exact à six points où un triple extérieur englobant possède le bon centre et le bon rayon, mais ne constitue pas un support positif, est une régression permanente dans les deux implémentations. Le raccord différentiel C++--Python reste à construire.

La future condition de descente est plus forte que ce certificat. Même `optimal_support_count=1` ne suffit pas : il faudra au moins vérifier que `boundary_point_ids==support_point_ids`, classifier tout $X\setminus F$ pour exclure un shell extérieur, résoudre exactement le top-$k$ au centre et certifier la décroissance stricte.

## Validation hôte ciblée

Le test strict couvre :

- le singleton de rayon nul;
- un triangle obtus réduit à sa paire diamétrale;
- un triangle rectangle dont le troisième point reste explicitement sur le shell;
- un triangle aigu à support trois;
- un tétraèdre régulier à support quatre et un tétraèdre non bien centré réduit à sa face opposée;
- le carré à deux diagonales optimales, support canonique `(0,3)` et quatre points de shell;
- six points cosphériques dont la miniball exige un support positif de cardinal quatre, malgré un triple extérieur englobant de même centre et de même rayon;
- dix points colinéaires, avec les comptes fermés `10/45/120/210`, 385 supports, 330 supports dépendants, 55 supports minimaux et 550 classifications exactes;
- les falsifications séparées de la facette, du support, du centre, du rayon, de la partition, des compteurs, du statut et du scope;
- les facettes vides, dupliquées, hors domaine ou de cardinal supérieur à dix.

Les builds Release stricts GCC et Clang, le CTest ciblé et les onze tests géométriques de l'oracle Python passent. Aucun noyau CUDA n'est ajouté par ce jalon et GCP n'a pas été utilisé.

## Suite immédiate

Le prochain jalon 6.2 doit classifier le shell global de la miniball contre le nuage canonique et construire la partition top-$k$ exacte au centre. Il devra séparer au moins trois sorties : facette admissible à une descente stricte, plateau ou shell dégénéré `unsupported_degeneracy`, et facette déjà active à son propre centre. La construction du successeur et la preuve machine de décroissance ne commenceront qu'après cette séparation.
