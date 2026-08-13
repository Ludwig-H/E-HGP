# Déblocage collectif après la fenêtre locale

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Portée et pins

Ce document complète, sans le remplacer, le contre-audit des cinq questions de
Claude dans
[`AUDIT_REPONSES_ROUTE_G4_50K_PUIS_10M_20260813.md`](AUDIT_REPONSES_ROUTE_G4_50K_PUIS_10M_20260813.md).
Il cherche une source de travail collective pour les supports que la fenêtre
k-NN ne certifie pas.

La note auditée à son pin d'origine est
[`NOTE_CLAUDE_ROUTE_G4_50K_PUIS_10M_20260813.md`](NOTE_CLAUDE_ROUTE_G4_50K_PUIS_10M_20260813.md),
SHA-256
`c603be69c7b5801b98894bebcee6fd98c5e0de55ba3f167e028ce8c313092c42`.
Claude a depuis intégré ses rétractations et la coupure au premier omis au
`HEAD=471715a68950afa9bba34edc2ac5db30724ff539`, commit
`retract three claims the auditor refuted, and take the cut at the first omitted
site`. La note cohérente relue porte le SHA-256
`3115744d008ca89339a5a246a2b9a5fae7bd358f54487ecb7beddbe5562593e0`.
Son parent logiciel `519ddfbaee60007e927bb148b9fb83451d7af7bc` répare le
juge ponctuel ; il ne change ni les pentes rouges du parcours par endpoint, ni
les preuves collectives ci-dessous.

Aucun fichier d'implémentation n'est modifié par cet audit.

Deux rejeux Release, CUDA désactivé, du pin logiciel rendent chacun `39/39`
CTests `mhgp3v_cone_`, en `31,42 s` puis `34,96 s` sous charge, sur l'ELF SHA-256
`e05a2065b630475361325b22677a29db30067800a1c99f37af39dedf53a12ccd`.
Il reçoit localement les refus `smax` et cardinalité, les accords séparés par
lane et le mutant d'héritage. Ces portes ferment la dette P0 du juge ; elles ne
changent aucune pente et ne qualifient aucun SLO. UBSan refuse encore les
fixtures sur le débordement signé du mutant `narrow-i64` : le P0 arithmétique
n'est donc pas entièrement soldé.

## Verdict

Le virage de Claude vers une source générative est justifié, mais le choix
`fenetre locale puis ray sweep` n'est pas encore une ordonnance complète. Le
certificat de fenêtre valide un support déjà trouvé ; il ne dit pas comment le
trouver sans former un grand nombre de tuples et ne nomme pas les supports
absents de toutes les fenêtres.

Trois primitives exactes doivent être mises en concurrence avant une nouvelle
implémentation lourde :

1. un surgraphe directionnel implicite construit par dominance dans les `432`
   sous-cônes rationnels déjà prouvés ;
2. des groupes coniques d'au plus trois témoins, plus forts que les témoins
   ponctuels et destinés au mur des amas ;
3. une décomposition de relations endpoint--témoin, éventuellement WSPD, puis
   des tâches plates `A_endpoint times B_partner times C_witness` pour le seul
   résiduel.

Ces voies n'exigent ni mosaïque de Delaunay d'ordre supérieur, ni arrangement
relevé. Elles n'évitent un catalogue global de paires que si les résultats
denses restent sous forme de nœuds canoniques ou de blocs factorisés : émettre
`R_dir=Theta(n^2)` records recréerait exactement le catalogue interdit. Aucune
n'a de borne sparse universelle ; le résiduel et son high-water restent des
sorties physiques à mesurer. La bonne décision est donc une gate `counter-only`
commune, pas un nouveau port CUDA immédiat.

## 1. Le coût caché de la génération locale

Fixer une ancre retire un élément du tuple, mais ne rend pas l'énumération
naïve petite. Avec `M` autres sites dans la fenêtre, les nombres de propositions
q2, q3 et q4 sont respectivement `M`, `C(M,2)` et `C(M,3)`. Leur somme vaut :

| `M` | propositions par ancre | propositions à `n=50 000` |
| ---: | ---: | ---: |
| 128 | `349 632` | `17 481 600 000` |
| 256 | `2 796 416` | `139 820 800 000` |

Les identités exactes sont :

$$128+\binom{128}{2}+\binom{128}{3}=349632.$$

$$256+\binom{256}{2}+\binom{256}{3}=2796416.$$

Tester la positivité avant **émission** évite de stocker les transits non
positifs ; cela ne retire aucun de ces tests si le tuple a déjà été formé. Une
source locale n'est donc admise que si elle publie au minimum :

- `q2/q3/q4_products_considered` ;
- `positivity_tests` et `positive_candidates` ;
- `SupportKey_unique` et `BallKey_unique` ;
- `census_point_tests` ;
- histogrammes p50/p90/p99/max par ancre ;
- octets lus/écrits et high-water.

Le ratio `products_considered/SupportKey_unique` est une porte, pas un détail
de profiling. Une pente verte des seules émissions peut masquer une pente
quadratique des propositions.

## 2. Sous-cônes rationnels et cutoff radial plus fort

### 2.1 Prédicat normalisé

Pour une ancre `a`, une cible `b` et un témoin `z`, poser
`d=b-a`, `s=z-a`, `D=||d||`, `r=||s||` et `t=r/D`. Si `gamma` est l'angle de
`d` et `s`, le prédicat spindle s'écrit :

$$H=D r(\cos\gamma-t),\qquad R=D^2r^2\sin^2\gamma.$$

La lane q4 demande `H>0` et `2H^2>R`; la lane q3 demande `H>0` et
`3H^2>R`. Les `432` sous-cônes entiers de
[`AUDIT_REPONSES_CLAUDE_CHAMBRES_NIVEAUX_CUTTING_20260812.md`](AUDIT_REPONSES_CLAUDE_CHAMBRES_NIVEAUX_CUTTING_20260812.md)
garantissent, pour deux rayons d'une même cellule :

$$\cos^2\gamma\geq\frac{9}{11},\qquad\sin^2\gamma\leq\frac{2}{11}.$$

### 2.2 Cutoff q4

Si `t<=3/5`, alors la pire marge vérifie :

$$2\left(\frac{3}{\sqrt{11}}-\frac{3}{5}\right)^2>\frac{2}{11}\geq\sin^2\gamma.$$

La seule stricte inégalité est
`2/sqrt(11)>3/5`, équivalente après carrés à `100>99`. L'égalité
`t=3/5` est donc encore strictement intérieure au spindle. Le sujet peut
décider sans racine :

$$25r^2\leq9D^2\Longrightarrow z\text{ est un témoin universel q4 lorsque }ab\text{ est l'arête maximale}.$$

À `smax=11`, huit `PointId` distincts satisfaisant cette implication ferment la
candidature de `ab` comme ancre maximale q4. En général, le seuil de mort d'un
support de cardinal `q` est `h=smax+1-q`. Ils ne prouvent pas que `ab` ne peut
être une arête non maximale d'un autre support ; ce support sera traité par sa
propre arête maximale canonique.

### 2.3 Cutoff q3

Pour q3, le cutoff conservateur `t<=5/8` est sûr. En effet,
`3/sqrt(11)>9/10`, donc :

$$3\left(\cos\gamma-\frac{5}{8}\right)^2>3\left(\frac{11}{40}\right)^2=\frac{363}{1600}>\frac{1}{5}>\sin^2\gamma.$$

Le test entier est :

$$64r^2\leq25D^2\Longrightarrow z\text{ est un témoin universel q3 lorsque }ab\text{ est l'arête maximale}.$$

À `smax=11`, neuf témoins distincts ferment la candidature d'ancre maximale q3.
Pour q2, la paire est son propre support maximal : dix témoins certifiés par le
test q4, plus fort, constituent déjà un cutoff sûr. Un seuil q2 plus large
pourra être ajouté séparément sans coupler les trois lanes.

### 2.4 Réduction à une hauteur de cellule

Après la symétrie d'une chambre, toute cellule vit dans
`x>=y>=z>=0`. Pour la hauteur entière `tau(v)=v_x` :

$$\left\lVert v\right\Vert^2\leq3\tau(v)^2.$$

Si `d` et `s` appartiennent au même sous-cône et
`tau(d)>=3*tau(s)`, alors :

$$\frac{\left\lVert s\right\Vert^2}{\left\lVert d\right\Vert^2}\leq\frac{3\tau(s)^2}{\tau(d)^2}\leq\frac{1}{3}<\frac{9}{25}.$$

Le témoin est donc q4 sans calcul de distance. Pour chaque
`(a,cellule)`, il suffit de connaître les 8e, 9e et 10e plus petites hauteurs.
Pour ce certificat suffisant, une cible dont la hauteur est au moins trois fois
le seuil correspondant est fermée comme **candidature d'ancre maximale** ; les
autres sont `uncertified_by_432_cutoff`, pas des supports mathématiquement
résiduels. Elles peuvent encore être fermées par le spindle exact, des témoins
d'autres cellules ou un groupe conique. Si la cellule contient moins de `h`
témoins, son seuil est infini et toutes ses cibles restent non certifiées par ce
seul cutoff.

Trois seuils entiers un peu plus forts restent disponibles sans racine. Pour
q2, `tau(d)>=2*tau(s)` implique `r/D<=sqrt(3)/2<9/10<cos(gamma)`, donc `H>0`.
Pour q3, le cutoff `t<=13/20` est sûr puisque `cos(gamma)>9/10` donne
`3(cos(gamma)-t)^2>3/16>2/11`; la condition de hauteur
`3*tau(d)>=8*tau(s)` l'implique. Pour q4,
`8*tau(d)>=23*tau(s)` donne `r^2/D^2<=192/529<4/11`, donc
`t<2/sqrt(11)`. Les facteurs de hauteur candidats deviennent ainsi `2`, `8/3`
et `23/8` pour q2, q3 et q4. Ils restent des certificats suffisants à recevoir
par fixtures de frontière et mutants ; ils ne prouvent ni complétude ni
sparsité.

Le facteur trois garantit aussi que `b` n'est jamais l'un des témoins crédités.
Le préflight doit encore imposer `D>0`, exclure `a` et rejeter les positions
colocalisées dans le profil courant ; leur agrégation relèverait d'une future
sémantique distincte. Chaque frontière appartient à une cellule half-open
canonique. Une dominance faible décrit le cône fermé, pas cette partition :
l'owner des arêtes et sommets coniques doit être engagé par des comparaisons
strictes/non strictes exactes, une perturbation lexicographique symbolique ou
une décomposition constante explicite des strates. Des inégalités faibles dans
toutes les cellules dupliqueraient les crédits.

## 3. Le calcul collectif est une requête de dominance

Un sous-cône simplicial fixé s'écrit avec trois formes linéaires entières
`ell_1,ell_2,ell_3`. Pour la cellule `j`, transformer une seule fois chaque
site par :

$$\Phi_j(x)=\left(\ell_{j1}(x),\ell_{j2}(x),\ell_{j3}(x),\tau_j(x)\right).$$

La condition `z-a` dans la cellule devient trois comparaisons de dominance
entre `Phi_j(z)` et `Phi_j(a)`. Le top-h est une sélection orthogonale. Si
`tau_h` désigne la **hauteur relative** `tau_j(z_h)-tau_j(a)`, le résiduel
ajoute `tau_j(b)<tau_j(a)+3*tau_h`. Avec la quatrième coordonnée absolue de
`Phi_j(z_h)`, la même formule s'écrit
`tau_j(b)<3*tau_j(z_h)-2*tau_j(a)` ; confondre les deux seuils est un mutant.
Une range-tree statique donne comme borne
théorique simple, pour un nombre fixé de cellules :

$$O\left(n\log^4 n+R_{\mathrm{dir}}\right),$$

où `R_dir` est le nombre de records dirigés réellement reportés. La construction
orthogonale standard paie en outre jusqu'à `O(n log^3 n)` d'espace et de
prétraitement pour quatre coordonnées, multiplié par la grande constante des
`432` cellules. Cette borne n'est pas une prescription GPU. Elle prouve
seulement que le certificat peut être évalué sans boucle explicite sur
`C(n,2)`. Si `R_dir` est dense, la sortie doit rester une union de nœuds
canoniques ou de rectangles avec ledger de masse ; un record par relation est
un NO-GO mémoire. Une ordonnance device candidate emploie formes entières
fixes, radix, scans segmentés et streaming par lots d'ancres.

Le futur `SymmetricAnd` possède ici un classifieur exact simple. Pour l'ancre
`a`, la cellule half-open `j` et la lane `q`, pré-calculer le seuil absolu
`F_q(a,j)=3*tau_j(z_h)-2*tau_j(a)`, ou `+infini` si la cellule est sous-pleine.
Alors :

$$R_q(a,b)\Longleftrightarrow\bigvee_j\left[b-a\in C_j^{\mathrm{ho}}\ \wedge\ \tau_j(b)<F_q(a,j)\right].$$

Sur un bloc `A times B` certifié dans une même cellule `j`, l'orientation est
`ALL_dir` si `max_{b in B} tau_j(b)<min_{a in A} F_q(a,j)` et `NONE_dir` si
`min_{b in B} tau_j(b)>=max_{a in A} F_q(a,j)`. Deux `ALL_dir` opposés donnent
`ALL` mutuel ; un seul `NONE_dir` donne `NONE` mutuel ; sinon le bloc est
`MIXED`. La cohérence de cellule se prouve par les trois formes de facette et
l'owner half-open, sinon le bloc se scinde. Mutants dédiés : confusion seuil
absolu/relatif, `<` remplacé par `<=`, cellule voisine et transpose oubliée.

Le ledger dirigé minimal par lane porte donc sur les candidatures d'ancre
maximale, pas sur toutes les incidences d'une paire dans tous les supports :

$$\text{maxanchor\_closed\_directed}_q+\text{uncertified\_by\_432\_directed}_q=n(n-1).$$

Une paire non orientée est fermée si **au moins une**
orientation possède un certificat ; elle reste non certifiée par ce cutoff
seulement si les deux orientations le sont. Autrement dit, la fermeture se
fusionne par `OR` et l'incertitude par `AND` :

$$\text{maxanchor\_closed\_PairId}_q+\text{uncertified\_by\_432\_PairId}_q=\binom{n}{2}.$$

### 3.1 Fusion factorisée des deux orientations

Le radix/RLE par `PairId` ne réalise la fusion que lorsque les records sont
matérialisables, notamment dans le juge à petit `n`. À grand `n`, le résiduel
dirigé doit d'abord être une **partition** canonique de relations, pas une simple
couverture :

$$R_q=\bigsqcup_{i\in I_q}A_i\mathbin{\times}B_i,\qquad A_i\cap B_i=\varnothing.$$

Chaque couple dirigé possède un unique `RectId=i`. Pour deux rectangles `i,j`,
poser `X_ij=A_i cap B_j` et `Y_ij=B_i cap A_j`. La fusion exacte est :

$$R_q\cap R_q^{\mathsf{T}}=\bigsqcup_{i\neq j}\left(A_i\cap B_j\right)\mathbin{\times}\left(B_i\cap A_j\right).$$

La disjonction vient de l'unicité des deux owners dirigés. Dans l'univers non
orienté, garder seulement `i<j` donne l'owner `(i,j)` et la masse exacte :

$$M_q=\sum_{i<j}\left|A_i\cap B_j\right|\left|B_i\cap A_j\right|,\qquad C_q=\binom{n}{2}-M_q.$$

Ici `M_q` est la masse résiduelle non orientée et `C_q` la masse fermée par le
cutoff considéré.

Ce join reste output-sensitive : avec des étoiles, un résiduel dense recrée
`Theta(n^2)` intersections. Le chemin 50k conserve donc un nœud paresseux
`SymmetricAnd(R_q,Transpose(R_q))` et le pousse dans la partition canonique des
paires fournie par les blocs LCA `L times R`, ou par une WSPD exacte. Sur un
bloc disjoint `A times B`, il rend `ALL` seulement si les deux orientations sont
incluses, `NONE` seulement si leur intersection est certifiée vide, sinon
`MIXED` et scinde déterministement `A` ou `B`. Les blocs `ALL` forment une
antichaîne et couvrent chaque `PairId` exactement une fois. Une limite atteinte
rend `resource_exhausted` atomique ; `MIXED` n'est jamais promu en `NONE`.

Le `BlockKey` contient digest d'arbre, chemins ordonnés, lane et masques
d'identité. À petit `n`, le juge seul développe les blocs et vérifie bitset,
masse, SHA-256 des `PairId` triés et invariance permutation/workers. À grand
`n`, un digest structurel authentifie les `BlockKey` triées, cardinalités et
hashes de sous-ensembles ; deux routes ne comparent ce digest que sous la même
partition canonique. Si les facteurs proviennent d'arbres secondaires non
laminaires, leurs intersections peuvent elles-mêmes exploser : elles restent
mesurées et cappées, jamais supposées constantes.

Le ledger distingue `R_pair_mass`, masse sémantique potentiellement
`Theta(n^2)`, de `R_node_records`, volume physique soumis aux pentes et au
high-water. Compteurs supplémentaires : `rectangle_join_candidates`,
`set_intersections`, `intersection_pieces`, `symmetric_all/none/mixed`, splits
`A/B`, cache d'intersection, `factorized_residual_blocks`, `block_bytes` et
workspace. Mutants : couverture non disjointe, oubli de la transposée, `OR` au
lieu de `AND`, diagonale non masquée, `MIXED` promu `NONE`, enfant omis/dupliqué
et tie-break dépendant des workers. Sans cette opération reçue, la fusion
réintroduit le catalogue global précisément à l'étape qu'elle prétend éviter.

Compteurs obligatoires : `transform_bytes`, `sort_bytes`, espace de l'index,
`dominance_queries`, `top_h_candidates`, `range_nodes`, `directed_records`,
`unique_uncertified_PairId` au petit `n`, `R_pair_mass`, `R_node_records`,
cellules sous-pleines, p50/p90/p99/max par ancre et high-water des deux buffers.
Un mutant `h-1`, un mutant comptant la cible comme témoin, un mutant de frontière
et un mutant utilisant la cellule voisine doivent chacun provoquer une
divergence de bitset ou une fausse fermeture contre l'oracle borné ; une
identité de masse seule peut rester vraie malgré la faute.

Il n'existe pas de claim sparse universel. Deux amas serrés séparés, sans assez
de témoins dans les cellules orientées de l'un vers l'autre, peuvent laisser
`Theta(|A||B|)` candidatures maximales résiduelles. La gate mesure donc à la fois
`R_pair_mass` et `R_node_records`; elle ne les remplace pas par le nombre de
cellules fermées.

Le raccord avec la fenêtre porte sur deux univers différents. La fenêtre
certifie des `SupportKey`, tandis que la dominance classe des candidatures
d'arête maximale. Connaître un support local ancré sur une paire ne permet pas
de soustraire cette paire : elle peut porter une autre extension hors fenêtre.
Sans reçu de complétude d'extension par paire, la composition exacte autorise
les doublons puis les élimine par RLE et exige seulement :

$$\mathcal{S}_{\mathrm{manquante}}\subseteq\mathrm{extensions}(\mathrm{maxanchor\_residual\_PairId}).$$

Chaque support manquant doit donc conserver au moins une arête maximale
canonique dans le résiduel. Une union disjointe fenêtre/résiduel au niveau des
supports nécessite un owner supplémentaire et doit être prouvée, jamais déduite
du seul ledger de paires.

## 4. Certificat de groupes coniques

Le témoin ponctuel est inutile lorsqu'aucun site n'appartient seul au spindle
universel. Un groupe de deux ou trois sites peut pourtant couvrir toutes les
sphères. Le théorème porte uniformément sur un à trois membres ; la borne trois
vient du théorème de Carathéodory conique en dimension trois.

### 4.1 Théorème

Fixer une paire `(a,b)`, poser `d=b-a` et `s_i=z_i-a`. Les centres des sphères
passant par `a,b` s'écrivent `c=(a+b)/2+t` avec `t dot d=0`. La marge de
puissance du témoin `z_i` vaut :

$$\mu_i(t)=d\mathbin{\cdot}s_i-\left\lVert s_i\right\Vert^2+2s_i\mathbin{\cdot}t.$$

Supposons, pour `1<=m<=3` :

- `d` appartient au cône positif des `s_i`, `1<=i<=m` ;
- `d dot s_i>||s_i||^2` pour chaque site.

Il existe des poids `lambda_i>=0` tels que leur somme vaille un et que
`sum lambda_i s_i` soit un multiple positif de `d`. Plus explicitement, si
`d=sum alpha_i s_i` avec `alpha_i>=0`, alors `A=sum alpha_i>0` puisque
`d!=0`; poser `lambda_i=alpha_i/A` donne
`sum lambda_i s_i=d/A`. Par conséquent :

$$\sum_i\lambda_i\mu_i(t)=\sum_i\lambda_i\left(d\mathbin{\cdot}s_i-\left\lVert s_i\right\Vert^2\right)>0.$$

Pour toute sphère passant par `a,b`, il existe donc au moins un membre du groupe
strictement intérieur ; ce membre peut dépendre de la sphère. À `smax=11`, huit,
neuf ou dix crédits deux à deux disjoints,
qu'ils soient singletons, paires ou triples, donnent respectivement huit, neuf
ou dix intérieurs distincts et ferment q4, q3 ou q2. En général le seuil vaut
`h=smax+1-q`. La disjonction porte sur tous les `PointId` de tous les crédits,
héritage compris.

### 4.2 Fixture destinée au mur des amas

Une première fixture minimale emploie `d=(20,0,0)`, `s1=(2,3,0)` et
`s2=(2,-3,0)`. Leur moyenne est parallèle à `d` et chaque marge fixe vaut
`d dot s_i-||s_i||^2=27>0`, alors que chaque singleton cesse d'être intérieur
pour certains centres. La fixture u16 suivante exerce ensuite un triple :

```text
a=(100,100,100), b=(120,100,100)
s1=(2,3,0), s2=(2,-3,3), s3=(2,-3,-3)
lambda=(1/2,1/4,1/4)
```

Le barycentre pondéré des trois `s_i` est parallèle à `d`. Cette fixture doit
tuer un chemin qui exige un même témoin intérieur pour tous les centres.

### 4.3 Version factorisée par cellule

Pour une cellule conique fermée `C` de rayons extrêmes `r_j`, choisir une forme
linéaire `w` strictement positive sur `C`. Un groupe `G` est candidat si
`C` est contenu dans `cone(G)`. Cette inclusion exige un certificat exact :
chaque rayon extrême appartient à `cone(G)` par barycentriques ou déterminants
entiers, les cas de rang déficient traitent leurs faces séparément, et
`w dot r_j>0` pour chaque rayon. Pour chaque `s` du groupe, poser :

$$\kappa_s=\min_j\frac{r_j\mathbin{\cdot}s}{w\mathbin{\cdot}r_j}.$$

Si tous les `kappa_s` sont strictement positifs, alors tout `d` de `C`
vérifie `d dot s>=kappa_s*w dot d`. Le cutoff collectif est donc :

$$w\mathbin{\cdot}d>\max_{s\in G}\frac{\left\lVert s\right\Vert^2}{\kappa_s}.$$

Les divisions ne font pas partie de l'autorité : le sujet compare les produits
entiers croisés, publie les bornes de bits des déterminants et envoie toute
égalité au résiduel. Un packing glouton de groupes peut accélérer mais n'est pas
complet ; son échec met le cutoff à l'infini. Son ordre doit être canonique,
géométrique et invariant par permutation et scheduling. Énumérer tous les
triples serait déjà `Theta(m^3)` et résoudre le packing disjoint est un verrou
algorithmique, pas un détail du certificat. Pour couvrir une cellule entière,
les `s_i` peuvent en outre devoir venir de cellules voisines.

Compteurs obligatoires : `group_candidates_considered` ventilé en
singleton/paire/triple, `cone_membership_solves`, déterminants,
`groups_accepted`, `packing_attempts`, conflits de `PointId`, splits de cellules,
crédits perdus face à un packing oracle borné, octets et high-water. Le probe
fixe avant le run un cap de candidats et de bytes ; l'atteindre rend la cellule
fail-open et transfère son bloc au résiduel avec reçu, jamais avec un packing
partiel présenté comme complet.

Mutants obligatoires : remplacer le cône positif par le seul espace linéaire,
accepter l'égalité de puissance, réutiliser un `PointId`, tester seulement
l'axe de la cellule et inverser une orientation de déterminant.
Un triple de rang déficient n'est jamais accepté par les seuls signes d'un
déterminant nul : il reste fail-open ou passe à un solveur conique exact qui
traite explicitement les faces de dimension inférieure.

## 5. WSPD et cœur commun de Jung

La décomposition bien séparée est utile comme partition de relations, jamais
comme approximation d'une décision.

Soient deux blocs `A,B` contenus dans des boules de centres `c_A,c_B` et de
rayons `r_A,r_B`. Poser `S=r_A+r_B`, `d=||c_B-c_A||` et
`m_0=(c_A+c_B)/2`. Pour toute paire `a in A,b in B`, son milieu est à distance
au plus `S/2` de `m_0` et sa longueur vérifie `D>=d-S`.

Si `ab` est l'arête maximale d'un support positif q3 ou q4, la géométrie de
Jung implique que la boule ouverte de centre `(a+b)/2` et de rayon `D/4` est
contenue dans la **circumboule** de ce support. L'épaisseur minimale vaut
`D/(2*sqrt(3))` en q3 et `(sqrt(3)-1)D/(2*sqrt(2))` en q4, toutes deux
strictement supérieures à `D/4`. Par triangle, tous les supports admissibles
de toutes les candidatures `a in A,b in B` possèdent donc, **si `d>3S`**, le
cœur commun suivant. Pour tout support admissible `U` ayant `ab` comme arête
maximale, de centre circonscrit `c_U` et de rayon `rho_U` :

$$R_{AB}=\frac{d-3S}{4}>0,\qquad B^{\circ}(m_0,R_{AB})\subseteq B^{\circ}(c_U,\rho_U).$$

En effet, tout `z` de ce cœur vérifie
`||z-(a+b)/2||<(d-3S)/4+S/2=(d-S)/4<=D/4`. Huit ou neuf `PointId` distincts
dans ce cœur ferment respectivement q4 ou q3 pour tout le bloc de candidatures
d'ancre maximale. Une borne inférieure rationnelle de `d` remplace la racine
dans l'implémentation. La frontière n'est jamais créditée.

Une WSPD de séparation fixe possède un nombre linéaire de blocs en dimension
fixe et couvre chaque paire exactement une fois. Cette propriété est
combinatoire ; le test du cœur reste exact. Elle vient de
[Callahan et Kosaraju](https://doi.org/10.1145/200836.200853).
La WSPD ne garantit pas à elle seule `d>3S` pour toute convention de
séparation : chaque bloc avec `d<=3S` conserve simplement un rayon nul et reste
fail-open.

Ce premier étage peut néanmoins visiter tout l'index pour une boule vide. Il
publie donc `range_node_visits`, points de frontière, crédits uniques et masse
résiduelle, pas seulement le nombre de requêtes. Deux amas sans point au milieu
laissent le bloc inter-amas entier : il reste implicite et passe à l'étage
suivant, jamais à une expansion de paires.

## 6. Relation-tree pour le résiduel `A times B times C`

Dans un arbre binaire, les deux relations `L times R` et `R times L` de chaque
nœud interne partitionnent exactement tous les couples ordonnés distincts
`(endpoint,witness)` par leur plus bas ancêtre commun. Une WSPD peut raffiner
ces relations pour les rendre directionnellement cohérentes. Cela construit un
squelette linéaire sans banque réallouée pour chaque endpoint ; ce n'est pas
une borne sur le join avec les partenaires.

Le résiduel se traite par tâches plates
`(A_endpoint,B_partner,C_witness,state)`. Une gate à petit `n` vérifie que
l'intersection canonique des partitions endpoint--témoin et endpoint--partenaire
compte chaque triple distinct exactement une fois. Elle tue au moins un enfant
`C` dupliqué, un enfant `C` omis et un échange `A/B` : partitionner séparément
deux relations ne suffit pas à prouver leur join.

Les décisions de boîte sont :

- `ALL` utilise d'abord les intervalles `Hmin/Rmax`, puis les `8^3=512`
  triples de coins exacts seulement dans la bande ambiguë ;
- `NONE` utilise un **majorant certifié** `H_ub` sur tout
  `A times B times C`, jamais le maximum des coins, et un **minorant certifié**
  `R_lb`, jamais le minimum des coins : `H_ub<=0` ou
  `c*max(0,H_ub)^2<=R_lb`, avec `c=3` pour q3 et `c=2` pour q4 ;
- l'échec de `ALL` n'est jamais assimilé à `NONE` ;
- les identités `z=a` et `z=b` sont retirées par relations disjointes et
  microtuiles à masques, pas par descente systématique jusqu'aux singletons ;
- la frontière `C` est persistante par deltas ou DAG ; aucun enfant ne copie
  six bitsets complets ;
- le split choisit le gain de largeur par octet entre `A`, `B` et `C`, sans
  priorité fixe `C-first`.

Le maximum de `H`, concave en `z`, et le minimum de `R`, convexe séparément, ne
sont pas déterminés par les coins. Deux fixtures permanentes doivent tuer cette
fausse simplification : pour `a=(10,0,0)`, `b=(14,0,0)` et
`C={(9,0,0),(12,0,0),(15,0,0)}`, les extrêmes donnent `H=-5` mais le point
central donne `H=4,R=0`; pour `a=(10,10,0)`, `b=(14,10,0)` et
`C={(12,8,0),(12,10,0),(12,12,0)}`, les extrêmes donnent `R=64,H=0` mais le
point central `R=0,H=4`. Les constructions de `H_ub/R_lb` doivent donc être
publiées et jugées séparément. Le `<=` du test `NONE` est sûr, car le spindle
est strict : même l'égalité exclut `cH^2>R`. En revanche, une égalité dans le
test `ALL` reste `UNKNOWN`.

Le vrai compteur du fallback est `corner_triple_evals`, pas son seul nombre
d'appels. Un million de fallbacks pleins représente déjà `512` millions de
triples exacts. Le layout publie `task_bytes`, `state_bytes`,
`frontier_entries_created`, high-water des deux queues et octets copiés. `ALL`
crédite seulement un bloc de témoins ; la lane ne ferme qu'après accumulation de
`h=smax+1-q` `PointId` uniques. Même alors, il ne décide ni census, ni positivité,
ni support, ni fold.

## 7. Gate commune avant toute nouvelle route G4

Claude peut départager les voies sans implémenter le producteur complet. Trois
sujets `counter-only` reçoivent le même nuage et rendent le même **univers** et
le même **schéma** de ledger par lane. Chacun partitionne cet univers selon la
force de ses propres certificats : les bitsets peuvent donc différer, mais toute
fermeture reste sound et tout support non fermé conserve une arête maximale dans
le résiduel :

1. dominance 432 avec cutoff ponctuel ;
2. dominance 432 enrichie des groupes coniques ;
3. WSPD/cœur commun puis relation-tree sur son résiduel.

La voie groupes enrichit la voie ponctuelle : à petit `n`, elle doit donc aussi
vérifier `closed_432 subset closed_groups` et
`residual_groups subset residual_432`. La voie WSPD utilise un certificat
incomparable et n'est soumise à aucune inclusion artificielle.

À petit `n`, chacun matérialise seulement pour le juge ses bitsets de
candidatures maximales `closed_q2/q3/q4` et
`uncertified_by_432_q2/q3/q4`. Chaque
fermeture est rejouée par un oracle ponctuel ou de groupe indépendant. Le
digest de l'incertitude non orientée emploie `OR(closed)` et
`AND(uncertified)` et reste
invariant par permutation, nombre de workers, découpe et échange d'orientation.
Le juge vérifie en plus que tout support absent de la sous-source fenêtre possède
une arête maximale canonique dans le résiduel.

Le travail `W` d'une pente est une valeur physique brute, avec
`e=log2(W(2n)/W(n))`. La masse sémantique couverte ou incertaine n'entre pas
dans cette porte tant qu'elle reste factorisée ; tâches, visites, solves,
records, octets et high-water y entrent toujours. Chaque sujet déclare avant le
run ses caps absolus de tâches, records et bytes. Un cap de classifieur local
peut transférer fail-open son bloc entier au résiduel avec reçu. Un cap rencontré
pendant le count/fill du ledger exact rend au contraire `resource_exhausted`
atomique : sans masse ni digest complets, il ne peut être compté comme un vert.

Les rampes exploratoires peuvent commencer à `500/1 000/2 000`; une voie qui
reste plausible passe ensuite directement à `12 500/25 000/50 000` sur un seul
ELF et les quatre familles contractuelles, plus deux amas séparés et la famille
à deux droites. Deux pentes successives doivent être `<=1,35` pour **chaque**
compteur dominant, les records, les octets et le high-water. Le temps CPU sous
contention n'est pas une porte.

À `n=50 000`, les masses de référence sont :

$$\binom{50000}{2}=1249975000,\qquad50000\mathbin{\cdot}49999=2499950000.$$

Une arène de seize octets par record dirigé saturerait déjà environ `40 GB`
sur la masse pleine, avant radix, ping-pong, index et payload. Le résiduel doit
donc rester factorisé ou être prouvé sparse avant émission. Le count et le fill
portent la même identité ; une insuffisance physique refuse atomiquement.

Le G4 ne devient utile qu'après :

- deux pentes vertes du front et du résiduel ;
- un cap d'octets absolu et un high-water incluant radix/workspace ;
- un lowering reçu des comparaisons de 68 à 72 bits ;
- un résiduel authentifié, rejouable et sans rescan racine ;
- le raccord au vrai `BenchmarkOutputContract-v1`.

## 8. Ce que la littérature ferme, et rien de plus

La recherche externe ne fournit pas le graphe Morse sparse demandé par Claude.
Elle apporte des garde-fous et un algorithme local précis :

- les mosaïques de Delaunay d'ordre supérieur construisent bien l'objet global
  que l'architecture v3 interdit de matérialiser ; la taille totale des premiers
  ordres en dimension trois peut être quadratique en `n` pour `K` fixé
  ([Edelsbrunner--Osang](https://arxiv.org/abs/2011.03617)) ;
- le Yao classique traite l'EMST des points, pas les facettes, carriers, lots et
  incidences silencieuses de MorseHGP3D
  ([Funke--Sanders](https://arxiv.org/abs/2303.07858)) ;
- les algorithmes de niveaux justifient la construction locale bornée de la
  section suivante ; ils ne prouvent ni une source globale sparse, ni le fold
  Morse ([Agarwal et al.](https://doi.org/10.1137/S0097539795281840)).

La voie produit doit donc composer les certificats internes ci-dessus. Le plein
arrangement global et la mosaïque restent des oracles bornés ou des réfutations
de complexité, jamais des plans d'implémentation.

## 9. Inversion locale : identité reçue, générateur encore conditionnel

### 9.1 Identité exacte et flats auto-centrés

Fixer une ancre `a`, écrire `s_z=z-a` et inverser chaque autre site de la
fenêtre par `y_z=s_z/||s_z||^2`. Une sphère passant par `a`, de centre `a+t` et
de rayon `||t||`, vérifie exactement :

$$z\in\mathrm{int}\,B(a+t,\left\lVert t\right\rVert)\iff2t\mathbin{\cdot}s_z>\left\lVert s_z\right\rVert^2\iff t\mathbin{\cdot}y_z>\frac{1}{2}.$$

La sphère devient donc le plan orienté `P_t : t dot y=1/2` dans l'espace
inversé. Ses intérieurs sont les points strictement d'un côté et son shell les
points du plan. Dans une carte où `t_j!=0`, écrire `P_t` comme le graphe de la
coordonnée `j`, puis appliquer la dualité point--plan standard. Le plan `P_t`
devient un point `x_t`; chaque `y_z` devient un plan dual ; le nombre `p`
d'intérieurs est exactement le niveau supérieur si `t_j>0`, inférieur si
`t_j<0`.

Six cartes `(j,signe)` couvrent tout `t!=0`. L'owner projectif choisit
canoniquement le plus grand `|t_j|`, puis l'axe et le signe en départage exact ;
une implémentation homogène peut éviter de construire six copies. Aucune
division n'est nécessaire à l'autorité : `y_z` est stocké par les quatre
entiers homogènes `(s_x,s_y,s_z,||s||^2)` et tous les signes sont des
déterminants croisés.

Un support contenant `a` et de cardinal `q` impose `q-1` plans parmi
`P_z : t dot y_z=1/2`. Dans l'espace des centres `t`, son lieu est donc une face
de dimension deux pour q2, une droite pour q3 et un point pour q4, sous
indépendance affine. Son niveau vaut `p`, le nombre d'intérieurs stricts.
Source S demande les flats incidents de profondeurs propres `p<=9/8/7`, puis
applique indépendance, positivité barycentrique et `p+q<=11`.

Cette correspondance est une **injection nécessaire**, pas encore la bijection
annoncée dans une première version de cet audit. Une face q2 porte une infinité
de centres, dont un seul est le milieu de la paire ; une droite q3 en porte une
infinité, dont un seul est le circumcentre du triangle. Il faut donc extraire le
point de norme minimale sur le flat, puis vérifier qu'il appartient encore à la
cellule shallow considérée. Pour q4, l'intersection est déjà ponctuelle. Aucun
théorème d'arrangement ne remplace ensuite le test de positivité. Réciproquement,
tout support local certifiable fournit bien un tel minimum de flat de profondeur
`p`, mais la complétude algorithmique exige de visiter **chaque flat incident
dont ce minimum est shallow**, y compris ceux dont l'intérieur relatif traverse
plusieurs cellules de profondeur.

### 9.2 Borne shallow et walk canonique en position simple

Agarwal, de Berg, Matousek et Schwarzkopf donnent, sous leurs conventions de
niveaux et de position générale, une construction incrémentale randomisée en
temps espéré :

$$O\left(Mk^2+M\log^3M\right).$$

La source primaire est
[« Constructing Levels in Arrangements and Higher Order Voronoi Diagrams »](https://doi.org/10.1137/S0097539795281840).
Le transfert à notre orientation possède une preuve autonome en position
simple. Dans l'espace des centres, poser
`H_i(t)=2s_i dot t-||s_i||^2` et
`D_K={t:#{i:H_i(t)>0}<=K}`. Cet ensemble est étoilé par rapport à zéro : le long
du segment `[0,t]`, un plan négatif en `t` ne devient jamais positif. Toute
cellule shallow pointée rejoint donc la cellule de zéro ; son 1-squelette est
atteignable depuis une graine de profondeur zéro.

Un argument de Clarkson--Shor borne les sommets. Un vertex défini par trois
plans et portant au plus `K` conflits du côté opposé à zéro survit sur
l'enveloppe de la cellule de zéro d'un échantillon de taille
`r` proportionnelle à `M/(K+1)` avec probabilité de l'ordre de `(r/M)^3`.
Cette cellule convexe a `O(r)` sommets ; on obtient donc
`O(M(K+1)^2)` vertices, puis le même ordre pour arêtes et faces en arrangement
simple. Cette preuve reçoit l'orientation radiale ; elle ne reçoit ni les
dégénérescences u16, ni les constantes et le temps optimal de l'algorithme du
papier.

Un walk CPU immédiatement falsifiable est alors : construire exactement les
facettes de la cellule de zéro ; enfiler leurs `VertexKey=(i,j,k)` ; pour chaque
`LineKey=(i,j)`, trouver par scan exact l'événement précédent et suivant du
pinceau, transporter la profondeur de l'arête ouverte et n'enfiler le voisin
que sous le seuil. Dédoublonner `PlaneKey/LineKey`, projeter zéro sur chacun de
ces flats, puis rescanner profondeur, shell et positivité. Son coût simple est
`O(M^2(K+1)^2)`, pas le temps optimal du papier. À `M<=128`, il devient un
probe différentiel sans `C(M,3)` ; il ne constitue toujours pas une route 50 k.

Avant ce walk, brancher sur le rang de `span{s_i}`. Au rang un, seuls les q2
directs subsistent. Au rang deux, q4 est impossible et q2/q3 demandent au pire
`M+C(M,2)` propositions, soit `8 256` à `M=128`, ou un arrangement 2D shallow.
Le walk 3D exige le rang trois et la position simple. Une concurrence de plus de
trois plans, une cosphère shallow ou une arête à événements multiples passe au
bundle exact/résiduel ; elle n'autorise aucune perturbation scientifique.

Avec `k=9` fixe, la comparaison suivante n'est donc plus qu'un **budget de
complexe conditionnel**, pas un proxy des supports garantis :

| `M` | `M+C(M,2)+C(M,3)` | proxy structurel `81M` |
| ---: | ---: | ---: |
| 64 | `43 744` | `5 184` |
| 128 | `349 632` | `10 368` |
| 256 | `2 796 416` | `20 736` |

Le proxy ne contient ni les constantes, ni `M log^3 M`, ni l'extraction des
minima de flats, ni le census. À `M=64` et `n=50 000`, `81Mn` vaut
`259 200 000` unités combinatoires avant constantes : assez bas pour mériter un
oracle `counter-only`, pas assez bas pour revendiquer une seconde ni une source.

### 9.3 Forme industrielle bornée

Ce résultat n'autorise pas à reconstruire une mosaïque globale sous un autre
nom. Le candidat admissible est :

1. produire top-M et premier omis depuis l'index global exact, sans scan et tri
   de tout le nuage par ancre ;
2. pour `M<=128`, brancher par rang puis construire dans la seule mémoire d'une
   ancre le shallow-facet-walk exact en position simple ; garder l'incrémental
   optimal du papier comme successeur conditionnel, pas comme dépendance de la
   première gate ;
3. inventorier tous les flats q2/q3 incidents, calculer leur minimum de norme
   exact et tester sa profondeur ; extraire q4 des sommets ; vérifier exactement
   cellule, profondeur, positivité,
   `SupportKey`, `I_B`, `U_B` et `BallKey` ;
4. appliquer le certificat strict de fenêtre, émettre les occurrences de toutes
   les ancres certifiantes, puis attribuer owner et dédupliquer par RLE ;
5. détruire le complexe avant l'ancre suivante et envoyer toute ancre au-dessus
   de `M=128`, toute coupe ambiguë et toute dégénérescence encore non traitée
   vers le résiduel collectif.

Le papier expose la position générale pour simplifier. Une perturbation
symbolique ne peut pas devenir la sémantique scientifique du profil u16 : une
cosphérie exacte porte un shell réel. La première version sûre route l'ancre
dégénérée au résiduel. La version complète peut employer la perturbation
uniquement pour la topologie, puis regrouper le cluster par `BallKey` exacte et
reconstruire tout le shell avant décision. Aucun point de shell n'est compté
comme intérieur et aucune égalité n'est certifiée par la fenêtre.

Compteurs obligatoires : `dual_planes`, `level_cells_0/1/2/3`, simplices de la
triangulation canonique, insertions, nettoyages, listes de conflits, maximum et
histogramme des conflits, déterminants, cellules rejetées par profondeur,
supports positifs, `BallKey` uniques, ancres dégénérées transférées, octets et
high-water. Le juge reconstruit l'inversion et le niveau sans recevoir la
fenêtre ni la coupure du sujet. Les mutants changent le côté du niveau, `9` en
`8`, oublient une carte projective, acceptent une égalité, perdent une arête q3
et séparent une cosphérie.

Cette voie répond partiellement à la question de Claude : quitter la DFS par
endpoint est justifié, et la complétude du walk simple vient de l'étoile radiale
et du 1-squelette shallow. Il reste un **candidat d'oracle local borné** : les
dégénérescences, le coût optimal et tout support absent de la fenêtre restent
au résiduel. La dominance 432 et `A times B times C` demeurent les certificats
globaux. Si le compteur du complexe, celui des flats ou leur high-water garde
deux pentes `>1,35`, il ne passe pas sur CUDA.

## 10. Ordre recommandé à Claude

1. Conserver le reçu local `39/39` du commit `519ddfb` comme gate P0 et ne
   transférer aucun `30/30` historique à un successeur logiciel.
2. Fermer d'abord le juge du worktree fenêtre, puis comparer sur les mêmes petits
   nuages son énumération brute au complexe inversé `<=9` ; mêmes
   `(BallKey,SupportKey,I_B,U_B)` ou refus.
3. Écrire en parallèle le probe `counter-only` de dominance 432, avec bitsets par
   lane et oracle borné. Cette étape teste la factorisation sans Source S.
4. Ajouter les groupes coniques derrière une ablation, d'abord sur la fixture
   minimale puis `eight_clusters` ; conserver le cutoff infini si le packing
   échoue.
5. Mesurer la masse résiduelle pondérée par `PairId`, pas le pourcentage de
   cellules. Si deux pentes restent rouges, ne pas porter ce chemin sur CUDA.
6. Pour le résiduel encore lourd, comparer le cœur commun WSPD au relation-tree
   plat `A times B times C`; ne jamais développer un bloc inter-amas.
7. Seulement si cette gate ferme le front, raccorder la sous-source de fenêtre,
   le générateur positivity-first, le census, les owners et le fold.
8. Mesurer enfin le payload complet sur G4 gardée. Le front horizontal réduit
   ne qualifie ni la seconde, ni les 100 ms du contrat officiel.

GCP non utilisé.
