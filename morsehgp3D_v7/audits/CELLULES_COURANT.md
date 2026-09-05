# S1 : certificats par cellules

4 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

Cette lecture ferme le certificat entier des cellules, son raccord géométrique aux centres q3/q4 et la surcouverture du localisateur sous le contrat binaire64 explicité ci-dessous. **Le rejet d'une ancre par `all_dead` ne dépend pas du localisateur flottant.** Aucun run ni changement produit ; les [hashes relus](receipts_20260904/cell_sources.json) épinglent cette analyse. Les clauses propriétaire, seed et cover sont établies séparément dans [S1_COURANT.md](S1_COURANT.md) ; les crédits de fuseau le sont dans [FRONT_ET_TEMOINS_COURANT.md](FRONT_ET_TEMOINS_COURANT.md).

## Un compte strict et sans double crédit

Fixons une ancre AB de milieu m, de longueur D, et un centre $c=m+p$ dans son plan bissecteur. Sa boule passant par A et B a pour rayon carré $D^2/4+\lVert p\rVert^2$. Pour un site z, poser $w'=2z-A-B$. L'intérieur strict est exactement :

$$\lVert z-c\rVert^2-R^2<0\quad\Longleftrightarrow\quad 4w'\cdot p>\lVert w'\rVert^2-D^2.$$

Dans la cellule de sommets $p=(i'u+j'v)/G$, cette condition devient `4*i'*du + 4*j'*dv > rhs`, avec `du=w'·u`, `dv=w'·v` et `rhs=G*(|w'|²-D²)`. Elle est affine en p. Sa validité stricte aux quatre sommets implique donc sa validité stricte sur toute la cellule **fermée**, arêtes comprises.

Dans [count_site_t](../src/lanes/cell_grid.hpp#L147), si `du>0`, chaque ligne de sommets témoins est un suffixe ; une cellule est témoin si son sommet gauche appartient aux deux suffixes de ses lignes. Le maximum de leurs débuts donne exactement cette intersection. Si `du<0`, les témoins forment deux préfixes et le sommet droit doit appartenir aux deux : leur minimum donne la borne stricte `ci<H`. Si `du=0`, chaque ligne est entière ou vide. Les tableaux de différences puis `accumulate` codent ces intervalles, sans approximation géométrique.

Un site emprunte une seule branche et incrémente au plus une fois une cellule. Le cover ne contient chaque position qu'une fois ; A et B sont explicitement exclus. Les compteurs portent ainsi sur des témoins distincts. Le même site peut témoigner pour plusieurs cellules, mais leurs comptes ne sont **jamais additionnés** : chaque cellule doit atteindre séparément $h_q=s_{\max}+1-q$. Aucun `EndpointCredit` n'est ajouté dans cette grille, ni aucun compte d'un filtre antérieur. Il n'existe donc pas de condition supplémentaire de disjonction avec les crédits de fuseau. Les tailles de cover admises sont inférieures à $2^{31}$, ce qui borne également chaque somme `u32` de cette construction.

Si le vrai centre est dans une cellule ayant au moins $h_q$ témoins, sa boule possède au moins $h_q$ intérieurs stricts et ne peut satisfaire $p+q\leq s_{\max}$. Le shell n'est pas crédité : une égalité à l'un des sommets empêche le crédit uniforme de ce site dans cette cellule. Les mutants de non-stricteté et de seuil modifient précisément cette implication ; aucun résultat de leur exécution n'est ajouté par cette lecture.

## Le domaine des centres est couvert

Pour un support positif possédé par AB, les bornes de rayon de S1 donnent $\lVert c-m\rVert^2\leq D^2/12$ en q3 et $\lVert c-m\rVert^2\leq D^2/8$ en q4. Les deux vecteurs entiers u,v construits par [bisector_basis](../src/lanes/sector_kill.hpp#L65) sont perpendiculaires à AB. Un succès exige :

$$t\lVert u\times v\rVert^2\geq D^2\lVert u-v\rVert^2,\qquad t\lVert u\times v\rVert^2\geq D^2\lVert u+v\rVert^2.$$

Ici t désigne le paramètre entier `rho2_den`, égal à 12 ou 8. Les quotients par les normes de $u-v$ et $u+v$ sont les distances carrées de l'origine aux quatre droites portant les arêtes du losange de sommets ±u, ±v. Les tests certifient donc que ce losange contient le disque requis. La non-dégénérescence est recontrôlée par le déterminant de Gram positif dans `build` ; un échec ne tue aucune ancre.

Dans les coordonnées $p=\alpha u+\beta v$, ce losange est $|\alpha|+|\beta|\leq1$. `cell_needed(i,j)` teste exactement si le minimum de $|\alpha|+|\beta|$ sur la cellule fermée est au plus un. Les cellules nécessaires couvrent donc tout le domaine admissible, y compris sa frontière. Quand `all_dead` est vrai, chaque centre admissible appartient à au moins une de ces cellules mortes. Ce rejet d'ancre est entièrement entier ; les critères `near_m`, densité et ratio de seeds ne participent pas au certificat.

## Les appels utilisent le bon centre ou toute la corde

Pour un seed aigu ABX, noter $G_3$ son déterminant de Gram et W le vecteur de `Q3Form`. Son centre est $A+W/(2G_3)$ ; relativement au milieu m, il vaut $N/(2G_3)$ avec $N=W-G_3(B-A)$. [seed_center_coords](../src/pipeline/generate.hpp#L594) forme exactement ses produits scalaires avec u et v. Le dénominateur est positif puisque seuls les seeds aigus atteignent cet appel.

Pour q4, tous les centres équidistants de A,B,X sont sur la droite $p=(N+\mu n)/(2G_3)$, où $n=(B-A)\times(X-A)$. S1 établit la condition fermée $2\mu^2\leq J$, avec $J=D^2(3G_3-2AX^2BX^2)$. [seed_chord_coords](../src/pipeline/generate.hpp#L611) prend $\widehat{\mu}=\lfloor\sqrt{\lfloor J/2\rfloor}\rfloor+1$, strictement supérieur à $\sqrt{J/2}$ pour tout J entier non négatif. Ses deux extrémités couvrent donc tous les centres des complétions positives du seed, sans omettre les racines de frontière. `J<0` rend ici `false`, sans tuer le seed.

Les coordonnées de base sont linéaires le long de cette corde. Une boîte contenant les coordonnées exactes de ses deux extrémités contient donc celles de toute la corde. Il suffit que toutes ses cellules **nécessaires** soient mortes ; ses cellules extérieures au losange ne représentent aucun centre admissible. Le retour `true` de `cell_dead` pour les indices hors grille n'est sûr qu'avec cette couverture : sur une frontière extérieure du losange, au moins une cellule fermée intérieure doit aussi être consultée. Le contrat de consultation de toutes les cellules fermées contenant le point assure ce raccord.

## Le localisateur : une borne autonome suffisante

Hypothèses d'évaluation : binaire64, arrondi au plus proche pour conversions et opérations, graphe arithmétique conservé sans réassociation, environnement inchangé pendant le calcul. Poser $\varepsilon_{64}=2^{-53}$, abrégé e dans les majorants de ce paragraphe. Les flottants non nuls concernés sont normaux. En effet, les entrées converties sont des entiers signés sur 128 bits, `den` et le déterminant entier Δ sont au moins un, et G vaut 8 ou 16. Les produits avant division restent sous $2^{257}$ ; l'échelle positive dépasse $2^{-255}$ et vaut au plus 16. Les opérations suivantes restent sous $2^{263}$, loin du débordement binaire64. Les soustractions des produits d'entiers arrondis donnent zéro ou un flottant de valeur absolue au moins un. Les coordonnées non nulles restent donc très au-dessus du seuil des sous-normaux. Le terme absolu $2^{-40}$ protège également les bornes finales au voisinage de zéro. Ces bornes larges suffisent ; aucune hypothèse de bon conditionnement de Gram n'est nécessaire.

Avec les entiers exacts fournis à `locate`, définir $T_1=p_u(v\cdot v)$, $T_2=p_v(u\cdot v)$, $T_3=p_v(u\cdot u)$, $T_4=p_u(u\cdot v)$, $s=G/(\mathrm{den}\,\Delta)>0$ et $S=s\sum_{j=1}^{4}|T_j|$. Les coordonnées exactes de cellule sont $a=(T_1-T_2)s$ et $b=(T_3-T_4)s$. Le déterminant est calculé **en entier**, puis converti ; il n'est pas obtenu par soustraction flottante de deux produits de Gram.

Pour chaque produit $\widehat{T}_j$, deux conversions et un produit donnent $|\widehat{T}_j-T_j|\leq4e|T_j|$. La soustraction suivante donne une erreur d'au plus $6e(|T_1|+|T_2|)$ pour la première coordonnée, et de même pour l'autre. Les deux conversions du dénominateur, son produit et la division donnent $|\widehat{s}/s-1|\leq5e$. Après le produit final, on obtient donc la borne commune, volontairement arrondie par excès :

$$|\widehat{a}-a|\leq16eS,\qquad |\widehat{b}-b|\leq16eS.$$

Les constantes 4, 6, 5 et 16 suivent en majorant chaque facteur d'arrondi par $1\pm e$ ; les termes d'ordre $e^2$ tiennent strictement dans les marges indiquées à $e=2^{-53}$. Pour l'epsilon calculé, les trois sommes positives de `mag` et les trois arrondis de chaque produit donnent $\widehat{\mathrm{mag}}\geq(1-e)^6\sum_j|T_j|$. D'autre part, $\widehat{s}\geq(1-e)^4s$. Le produit, la multiplication normale par la puissance de deux $2^{-46}$ et l'addition finale donnent :

$$\widehat{\epsilon}\geq128e(1-e)^{12}S+8192e(1-e)>120eS+8000e.$$

La multiplication par $2^{-46}$ est exacte dans le domaine normal ; la borne conserve les arrondis du produit `mag*scale` et de l'addition. Le terme positif final rend l'inclusion **stricte**, même si S est nul. Il faut encore compter les arrondis de `aG-eps` et `aG+eps`, et pas seulement celui de `aG`. Comme $|\widehat{a}|\leq(1+16e)S$, la marge de chaque côté après cet ultime arrondi est au moins :

$$(1-e)\widehat{\epsilon}-17eS-16e^2S>(103e-136e^2)S+8000e(1-e)>0.$$

Ainsi `floor(lo)..floor(hi)` contient toutes les cellules fermées contenant la coordonnée exacte, notamment les deux indices de part et d'autre d'un entier exact. Pour `segment_box`, prendre le maximum des deux epsilons conserve la borne de chaque extrémité ; min/max et l'arrondi au plus proche sont monotones. La boîte contient strictement les coordonnées des deux extrémités, donc toute leur interpolation affine. La surcouverture requise pour les seeds est établie sous les hypothèses d'évaluation annoncées ; le coefficient commenté `9,001` n'est pas utilisé comme autorité.

## Politique et contrat d'évaluation

`anchor_grid_stage` remet les deux flags `built` à zéro avant toute politique. Une grille non demandée, un environnement refusé, une capacité dépassée ou un échec de base conserve l'ancre. Les replis de G16 vers G8 ne retournent un rejet qu'après construction réussie et certificat ; sinon les boucles de seeds continuent. Les appels par seed sont eux-mêmes gardés par `grid.built`. Activer plus souvent la grille ne change donc pas la règle de rejet, sous les mêmes contrats numériques.

La garde `float_filter_runtime_enabled`, `FLT_EVAL_METHOD==0` et le rejet des NaN/hors domaine mettent en œuvre une partie du contrat. Ils ne vérifient pas à eux seuls toutes les options du compilateur, l'absence de réassociation ni la stabilité de l'environnement ; ces prémisses doivent rester attachées aux commandes de construction et d'exécution. L'oracle rationnel existant réfute des erreurs de localisation, mais sa présence seule ne prouve pas le majorant. Le [complément de largeur](ARITHMETIQUE_CELLULES_COURANTE.md) ferme désormais les opérations entières et les casts dans ce profil. Le [domaine CPU](DOMAINE_CPU_COURANT.md) fixe les options exécutées et les prémisses numériques. Cette séparation ferme le raccord des cellules sans promouvoir S1 global. GCP non utilisé.
