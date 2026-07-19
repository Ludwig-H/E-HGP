# Progression Phase 6 — miniballs exactes et descentes

## Statut

- phase : `6`, `ready`; jalons préparatoires 6.1, 6.2 et 6.3 livrés pendant que la phase 5 reste l'unique phase `in_progress`;
- backend : `reference_cpu`;
- profil : `full_pi0`;
- mode : `certified`;
- portée courante : `canonical_top_k_selected_strict_level_arc_only`;
- porte d'entrée : satisfaite par les Phases 1 et 4 fermées;
- porte de sortie : non satisfaite; le miniball exact borné, son shell global et la famille top-$k$ exacte à son centre sont livrés, et 6.3 borne un arc canonique au seul niveau strict; aucun segment sous-niveau, DAG, pointer-jumping, attache ou différentiel indépendant n'est construit.

Ces jalons ne construisent aucune forêt et ne publient aucun `public_status`. Le premier fournit un oracle local exact pour une facette de cardinal au plus dix; le second certifie ses préconditions globales au centre; le troisième réserve le mot successeur au représentant canonique dont la miniball fraîche possède un niveau strictement inférieur. Ce dernier certificat reste un arc combinatoire de niveau, pas un chemin géométrique ni une attache.

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

Le certificat local reste insuffisant à lui seul. Même `optimal_support_count=1` ne suffit pas : le jalon 6.2 exige séparément `boundary_point_ids==support_point_ids`, classifie tout $X\setminus F$ pour exclure un shell extérieur et résout exactement la famille top-$k$ au centre.

## Préclassification globale 6.2

`build_exact_facet_descent_preconditions` construit d'abord la miniball locale, puis appelle `brute_force_closed_ball` contre tout le nuage canonique et `brute_force_top_k` au rang $k=\lvert F\rvert$ avec une exclusion explicitement vide. Son contrat est `exact_facet_miniball_global_closed_ball_exact_top_k_membership_v1` et sa portée `global_shell_and_top_k_preconditions_only`. Les deux partitions spatiales restent entières dans le résultat; `canonical_choice_ids` est seulement le représentant déterministe de la famille top-$k$, jamais un successeur.

Pour la partition globale de la boule, notons $I_X$ son intérieur strict, $U_X$ son shell, $I_{\mathrm{ext}}=I_X\setminus F$ et $U_{\mathrm{ext}}=U_X\setminus F$. Le rejeu exige que les intersections avec $F$ restituent exactement les partitions locales et que le rang fermé vaille $k+\lvert I_{\mathrm{ext}}\rvert+\lvert U_{\mathrm{ext}}\rvert$. Le cutoff top-$k$ est toujours inférieur ou égal à $\beta(F)$ puisque les $k$ points de $F$ appartiennent à la boule. S'il lui est égal, les ensembles strict et shell de la requête top-$k$ doivent être exactement $I_X$ et $U_X$; s'il lui est inférieur, tous les points top-$k$ retournés sont strictement dans la miniball de $F$.

L'activité exacte signifie $F\in\mathcal{N}_k(c_F)$. Elle équivaut à $I_{\mathrm{ext}}=\varnothing$ et ne se décide ni par `canonical_choice_ids==F`, ni par l'égalité du cutoff et de $\beta(F)$. Une facette est régulière pour la descente stricte lorsque son shell interne est exactement son support positif unique et lorsque $U_{\mathrm{ext}}=\varnothing$. La décision est fail-closed :

- facette régulière inactive : `strict_descent_admissible`;
- facette régulière active : `already_active_at_own_center`;
- support non essentiel, plusieurs supports optimaux ou shell extérieur : `unsupported_degeneracy`, avec le fait exact d'activité conservé séparément.

`verify_exact_facet_descent_preconditions` ne fait confiance à aucun de ces champs : il recalcule la miniball et les deux partitions, puis vérifie identité du nuage, partitions, borne du cutoff, essentialité, shell global, appartenance à la famille top-$k$, compteurs, décision et portée. Ce rejeu frais réutilise néanmoins les mêmes primitives exactes hôtes et n'est pas un oracle logiciel indépendant.

## Théorème de stricte admissibilité

Soit une facette régulière inactive et $G\in\mathcal{N}_k(c_F)$. L'absence de shell extérieur impose que tout point de $G\setminus F$ soit strictement intérieur à la miniball de $F$. Comme $F$ n'appartient pas à la famille top-$k$, tout choix $G$ omet au moins un point du support essentiel. Si $\beta(G)=\beta(F)$, la boule de centre $c_F$ serait aussi la miniball de $G$; l'unicité de son centre et la caractérisation convexe des miniballs imposeraient alors $c_F$ dans l'enveloppe convexe d'un sous-ensemble propre du support essentiel, ce que ses coordonnées barycentriques strictement positives interdisent. Ainsi tout choix futur satisfait $\beta(G)<\beta(F)$.

Le statut de cette implication reste `conditional_theorem` : le logiciel hôte valide exactement ses prémisses pour la facette courante, mais ne désigne aucun membre de la partition comme successeur et ne recalcule aucune miniball successeure. En particulier, `strict_descent_admissible` n'est pas un certificat d'arc. Inversement, lorsque les prémisses échouent, 6.2 certifie seulement que la stricte décroissance n'est pas établie; il ne peut pas affirmer qu'un plateau existe. Un shell top-$k$ multivalué n'est pas un plateau lorsque l'implication universelle ci-dessus s'applique.

## Arc canonique de niveau 6.3

`build_exact_facet_descent_arc` rejoue d'abord 6.2. Uniquement lorsque la décision source vaut `strict_descent_admissible`, il pose $G$ égal à `canonical_choice_ids`, conserve ces identifiants comme `successor_facet_point_ids` et construit une nouvelle `ExactFacetMiniballResult` sur cette facette. Il exige successivement que $G$ soit le choix canonique, que $G\in\mathcal{N}_k(c_F)$, que $G\neq F$, que $\beta(G)\leq d_k(c_F)$ et que $\beta(G)<\beta(F)$. Les deux dernières décisions sont exactes sur `ExactLevel`; aucune tolérance ni implication non rejouée ne remplace la comparaison.

Le résultat porte `proof_basis=exact_descent_preconditions_canonical_top_k_member_fresh_miniball_strict_level_v1` et `scope=canonical_top_k_selected_strict_level_arc_only`. Ses décisions sont :

- `strict_descent_arc_certified`, avec les deux payloads cibles présents et les cinq faits d'arc vrais;
- `no_arc_already_active_at_own_center`, sans aucun payload cible et avec les quatre compteurs `(1,0,0,0)`;
- `no_arc_unsupported_degeneracy`, avec la même absence de payload et les mêmes compteurs;
- `not_certified`, réservé à l'état initial.

Les compteurs ordonnés sont `precondition_classification_count`, `canonical_top_k_selection_count`, `successor_miniball_build_count` et `exact_level_comparison_count`. Dans la branche stricte ils valent `(1,1,1,2)`, la dernière valeur comptant séparément la borne par le cutoff et la baisse depuis la source. `verify_exact_facet_descent_arc` reconstruit les préconditions, redérive le choix canonique depuis la partition fraîche, rejoue toute la miniball cible, compare les identifiants et les niveaux, puis ferme présence des optionnels, cinq faits, compteurs, décision, portée et identité du nuage. Une source stricte donnant $G=F$ ou $\beta(G)\geq\beta(F)$ constitue une contradiction fail-closed; elle n'est jamais rabattue sur une branche sans arc.

La régularité de $G$ à son propre centre n'est pas requise pour certifier l'arc courant. Une étape ultérieure devra relancer 6.2 sur $G$ avant toute nouvelle transition. Le rejeu 6.3 utilise les mêmes primitives exactes hôtes et n'est pas un oracle logiciel indépendant.

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

Le test 6.2 ajoute six discriminants exacts :

- `strict-cutoff-equal` : $F=\left\lbrace(-1,0,0),(1,0,0)\right\rbrace$ et l'intrus $(0,0,0)$ donnent intérieur global singleton, shell global égal au support, cutoff top-2 égal à un et décision `strict_descent_admissible`; le rayon carré $1/4$ du représentant canonique est seulement un diagnostic de fixture, pas un successeur publié;
- `strict-cutoff-lower` : $F=\left\lbrace(-2,0,0),(2,0,0)\right\rbrace$ et les deux intrus $(-1,0,0),(1,0,0)$ donnent un cutoff top-2 égal à un, strictement inférieur à $\beta(F)=4$, tout en fermant la même décision régulière;
- `foreign-shell` : la même facette avec l'intrus et $(0,1,0)$ sur le shell conserve une activité fausse mais retourne `unsupported_degeneracy` à cause du shell extérieur;
- `nonessential-plateau-capable` : une facette triangulaire rectangle possède `optimal_support_count=1` mais une frontière strictement plus grande que son support; avec un intrus, un choix top-3 descend et un autre conserve le niveau, donc la seule décision certifiée est `unsupported_degeneracy`;
- `already-active` : une paire diamétrale avec un point strictement extérieur à sa boule retourne `already_active_at_own_center`;
- `active-canonical-choice-differs` : une paire appartient à la famille top-2 en présence d'un troisième point sur le shell même lorsque le représentant canonique diffère; le shell extérieur impose néanmoins `unsupported_degeneracy` et le booléen d'activité reste vrai.

Les falsifications 6.2 portent séparément sur la miniball embarquée, chacune des deux partitions optionnelles, les trois décisions booléennes, les compteurs, la décision, la portée et l'identité du nuage. Les builds Release stricts GCC et Clang, le CTest ciblé et les onze tests géométriques de l'oracle Python couvrent la chaîne locale; aucun raccord différentiel indépendant supplémentaire, aucun noyau CUDA et aucune qualification G4 ne sont ajoutés. GCP n'a pas été utilisé.

La validation 6.3 ajoute deux arcs stricts :

- `strict-arc-cutoff-equal` reprend $X=\left\lbrace(-1,0,0),(0,0,0),(1,0,0)\right\rbrace$ et la paire extrême; elle exige le choix canonique formé de la première extrémité et de l'intrus, $c_F=(0,0,0)$, $c_G=(-1/2,0,0)$ et la chaîne exacte $\beta(G)=1/4<d_2(c_F)=\beta(F)=1$;
- `strict-arc-cutoff-lower` reprend $X=\left\lbrace(-2,0,0),(-1,0,0),(1,0,0),(2,0,0)\right\rbrace$ et la paire extrême; elle exige la paire intérieure, $c_G=c_F=(0,0,0)$ et la chaîne exacte $\beta(G)=d_2(c_F)=1<\beta(F)=4$.

Les cas actif et non pris en charge de 6.2 retournent respectivement `no_arc_already_active_at_own_center` et `no_arc_unsupported_degeneracy`, sans optionnel cible et avec les compteurs `(1,0,0,0)`. Les mutations suppriment un payload strict, injectent un payload dans une branche sans arc, remplacent le choix canonique par un autre membre top-$k$, désaccordent les identifiants et la miniball cible, falsifient chacun des cinq faits et des quatre compteurs, puis changent décision, portée et identité de nuage. La cible unitaire passe en Release strict sous GCC et Clang; le statut logiciel 6.3 est `validated_host_software`, sans oracle indépendant supplémentaire.

## Suite immédiate

Le prochain jalon pourra certifier le segment géométrique de $c_F$ vers $c_G$ avant toute fermeture de labels, tout DAG, tout pointer-jumping ou toute attache. Une égalité de niveaux hors du gate strict restera `unsupported_degeneracy` tant que le quotient multivalué de plateau n'est pas démontré et implémenté. Aucun segment ni assemblage de plusieurs arcs n'est couvert par `canonical_top_k_selected_strict_level_arc_only`.
