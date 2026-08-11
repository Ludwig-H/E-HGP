# Note mathématique — certificat de Helly sur le disque de Jung

Date : 11 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Verdict

Pour une paire réellement maximale dans un support propre positif q3 ou q4,
le disque de Jung transforme chaque point du nuage en un demi-plan fermé de
centres où ce point n'est pas strictement intérieur. Un groupe de témoins
couvre le disque exactement lorsque l'intersection du disque et de leurs
demi-plans mauvais est vide. Par le théorème de Helly dans le plan, toute
couverture possède un sous-certificat de trois `PointId` au plus.

Neuf groupes deux à deux disjoints certifient au moins neuf points strictement
intérieurs à toute sphère q3 admissible; huit groupes certifient la propriété
analogue en q4. Ils autorisent seulement la tombstone du quotient horizontal
H0 jusqu'à `K=10`. Ils ne prouvent ni l'absence d'un support ou d'une incidence
Gamma, ni les verticales, ni une borne de temps.

Ce certificat ponctuel n'est ni implémenté ni reçu. Il ne fournit aucune borne
uniforme sur un produit d'AABB et ne remplace donc pas la porte center-cover
globale. Son admission exige un juge indépendant et des compteurs de coût.

## 1. Disque admissible

Soient deux points distincts `a,b`, `d=b-a`, `D^2=d dot d` et
`M=(a+b)/2`. La paire doit être une arête de longueur maximale du support q3
ou q4 considéré. Ce support est affinement indépendant et propre positif; son
centre est celui de sa miniboule. Les ex æquo de diamètre emploient un owner
canonique exact.

Tout centre d'une sphère passant par `a,b` s'écrit `c=M+t`, avec `t dot d=0`,
et son rayon carré vaut :

$$r^{2}=\frac{D^{2}}{4}+\lVert t\rVert^{2}.$$

La borne de Jung appliquée au support donne les disques fermés suivants dans
le plan médiateur `Pi=d^perp` :

$$C_{3}=\left\lbrace t\in\Pi:\lVert t\rVert^{2}\leq\frac{D^{2}}{12}\right\rbrace,\qquad C_{4}=\left\lbrace t\in\Pi:\lVert t\rVert^{2}\leq\frac{D^{2}}{8}\right\rbrace.$$

Ces disques peuvent contenir des centres qui ne correspondent à aucun support
réel. Les couvrir entièrement est donc conservateur.

## 2. Demi-plans mauvais

Pour un point `z`, poser `U_z=2z-a-b` et `g_z=D^2-||U_z||^2`. La marge entre
le rayon carré et la distance carrée de `z` au centre est exactement :

$$\mu_z(t)=r^{2}-\lVert z-(M+t)\rVert^{2}=\frac{g_z}{4}+U_z\mathbin{\cdot}t.$$

Le point est strictement intérieur si et seulement si `mu_z(t)>0`. Son
demi-plan mauvais fermé est donc :

$$B_z=\left\lbrace t\in\Pi:g_z+4U_z\mathbin{\cdot}t\leq0\right\rbrace.$$

L'égalité appartient au mauvais côté : un point de coquille ne reçoit aucun
crédit intérieur. Contrairement au certificat plus étroit fondé sur
l'enveloppe convexe de projections, cette construction n'exige pas `g_z>0`
pour chaque membre. Les offsets et le bord du disque participent à la preuve.
Une projection axiale avec `g_z>0` donne un singleton universel; avec
`g_z<=0`, son mauvais côté est tout le plan et elle n'aide pas.

## 3. Couverture par groupes

Pour `q` égal à 3 ou 4 et un groupe fini `G` de `PointId` distincts des deux
extrémités, définir :

$$\mathrm{Cover}_q(G)\Longleftrightarrow C_q\cap\bigcap_{z\in G}B_z=\varnothing.$$

Si cette propriété est vraie, chaque centre admissible possède au moins un
`z` de `G` tel que `mu_z(t)>0`. Sinon ce centre appartiendrait à
l'intersection déclarée vide.

Soient maintenant `G_1,...,G_h` des groupes deux à deux disjoints. Pour un
centre admissible fixé, choisir un témoin intérieur dans chaque groupe donne
`h` `PointId` distincts. Par conséquent :

- q3 : neuf groupes donnent `p>=9`, donc `p+q>=12`;
- q4 : huit groupes donnent `p>=8`, donc `p+q>=12`.

Le théorème d'inertie H0 autorise alors la tombstone horizontale jusqu'à
`K=10`, avec la `BallKey`, le support positif et les groupes rejouables. Deux
groupes additionnés ne partagent aucun `PointId`.

## 4. Sous-certificat de taille trois

La famille formée de `C_q` et des demi-plans `B_z` est une famille de convexes
du plan. Si son intersection est vide, Helly fournit une sous-famille d'au plus
trois convexes dont l'intersection est déjà vide.

Si cette sous-famille contient `C_q`, elle contient au plus deux demi-plans
mauvais : un ou deux témoins suffisent. Si elle ne contient pas `C_q`, au plus
trois demi-plans mauvais ont déjà une intersection vide dans tout le plan et
couvrent donc aussi le disque. Toute couverture possède ainsi un sous-groupe
de taille au plus trois.

Helly borne le reçu d'une couverture trouvée; il ne fournit pas une méthode
linéaire pour maximiser le nombre de groupes disjoints. Un greedy de
singletons, paires puis triples est exact lorsqu'il réussit et incomplet
lorsqu'il échoue. Cet échec conserve la paire.

Helly s'applique à un crédit de couverture à la fois. Il ne prouve pas qu'un
recouvrement de multiplicité neuf ou huit se décompose en autant de groupes
disjoints; cette disjonction doit être construite et vérifiée explicitement.

## 5. Décision exacte d'un petit groupe

Choisir une base rationnelle du plan médiateur. Dans ses coordonnées `x`, le
disque s'écrit `x^T G x<=H_q^2` pour une matrice de Gram définie positive, et
chaque demi-plan mauvais s'écrit `alpha_i^T x<=b_i`, avec des coefficients
rationnels issus des entiers u16.

Pour décider si le polyèdre des demi-plans rencontre le disque, minimiser
`x^T G x` sous ces contraintes. Avec au plus trois demi-plans, le minimum d'un
polyèdre non vide est atteint dans l'un des cas suivants :

1. `x=0` est admissible;
2. une contrainte est active, au projeté de zéro sur sa droite;
3. deux contraintes indépendantes sont actives, à leur intersection.

Énumérer ces candidats rationnels, vérifier toutes les inégalités, puis
comparer la norme carrée minimale à `H_q^2` suffit. Si aucun candidat n'est
admissible, les demi-plans sont incompatibles. Le groupe couvre le disque si
le polyèdre est incompatible ou si sa norme minimale est strictement
supérieure à `H_q^2`. Une égalité reste fail-open.

Le premier juge doit employer des rationnels multiprécision et ne partager ni
la base, ni le solveur, ni les branches de dégénérescence du sujet. Une version
entière bornée demande une preuve d'amplitude séparée.

### Formules entières pour la sous-banque diamétrale stricte

Poser `S_ij=D^2(U_i dot U_j)-(U_i dot d)(U_j dot d)`. Si `S_ii=0` et `g_i>0`,
le témoin `i` est un singleton universel. Sinon, le pied de sa droite mauvaise
respecte la contrainte `j` exactement lorsque :

$$g_jS_{ii}-g_iS_{ij}\leq0.$$

Ce pied rencontre le disque fermé sous `3g_i^2<=4S_ii` en q3 ou
`g_i^2<=2S_ii` en q4. Pour deux frontières indépendantes, poser :

$$\Delta=S_{ii}S_{jj}-S_{ij}^{2},\qquad N=g_i^{2}S_{jj}-2g_ig_jS_{ij}+g_j^{2}S_{ii}.$$

Leur intersection respecte une troisième contrainte `k` exactement lorsque :

$$F_k=g_k\Delta-S_{ki}(S_{jj}g_i-S_{ij}g_j)-S_{kj}(S_{ii}g_j-S_{ij}g_i)\leq0.$$

Elle rencontre le disque fermé sous `3*N<=4*Delta` en q3 ou `N<=2*Delta` en q4.
Les cas `Delta=0` sont parallèles et restent couverts par les pieds à une
contrainte et les vérifications croisées. Sous u16, `S_ij` approche 70 bits et
`F_k` peut demander environ 180 bits : le test général ne tient pas dans
`i128`. Une voie produit exige un entier assez large ou un filtre fixe prouvé
avec repli multiprécision exact.

## 6. Relations avec les autres certificats

### Singleton de Jung

Pour `s=z-a` et `A=d dot s-||s||^2`, on a `g_z=4A` et
`||d cross U_z||^2=4||d cross s||^2`. La couverture par un singleton redonne :

$$\text{q3: }A>0\ \text{ et }\ 3A^{2}>\lVert d\mathbin{\times}s\rVert^{2},\qquad\text{q4: }A>0\ \text{ et }\ 2A^{2}>\lVert d\mathbin{\times}s\rVert^{2}.$$

Pour une ancre `a` et un témoin `z` fixes, un nœud de cibles peut être crédité
seulement si des bornes entières reçues prouvent l'inégalité pour tous ses
membres. Une boîte indécise descend. Aucun nombre fixe de banques
directionnelles n'est affirmé complet.

### Enveloppe convexe et profondeur

La condition `0 in conv{Proj(U_z)}` avec tous les `g_z>0` couvre tout le plan
médiateur, donc le disque. Elle reste un certificat suffisant simple. Helly sur
le disque est plus général parce qu'il utilise les offsets.

La profondeur fermée fournit une autre preuve globale à partir des témoins
diamétraux et d'un sweep angulaire. Aucune domination générale n'est affirmée
entre profondeur et packing greedy de groupes de Helly. Les deux gardent des
sorts et gains marginaux séparés. Le sweep complet reste un oracle borné tant
que sa collecte par paire redémarre à la racine.

## 7. Direction d'implémentation

1. Conserver le cœur singleton comme premier crédit bon marché.
2. Maintenir, par ancre, une petite banque de candidats avec `PointId` stables.
3. Chercher des groupes disjoints de taille un, deux ou trois sur les paires
   encore ambiguës.
4. Publier seulement les groupes qui passent le test exact du disque et un
   rejeu indépendant.
5. Conserver toute paire lorsque la banque, le greedy ou l'arithmétique échoue.
6. Mesurer séparément crédits singleton, groupes, tests de faisabilité, octets,
   paires conservées et coût aval évité.

Une extension à un produit de boîtes d'extrémités exige un théorème uniforme
sur le disque et chacun des demi-plans. La preuve ponctuelle ne doit pas être
transformée en test AABB par coins sans preuve adaptée. La porte structurelle
globale reste le center-cover `P15-HOCUDA-P1`; sa tranche P1a profile seulement
le prune q4 mass-only et ne reçoit aucune complétude.

## 8. Fixtures et mutants minimaux

La réception exige au moins :

- les contacts exacts q3/q4 sur le bord du disque, qui doivent refuser;
- un singleton axial universel et un singleton axial inutile;
- une paire de demi-plans qui couvre seulement grâce au rayon borné;
- trois demi-plans deux à deux compatibles mais globalement incompatibles;
- un groupe annoncé couvrant avec un centre mauvais rationnel explicite;
- neuf ou huit groupes valides avec un `PointId` réutilisé;
- l'échange fautif des rayons `D^2/12` et `D^2/8`;
- les mutants `<=` vers `<` du côté mauvais et `>` vers `>=` au bord;
- permutations, extrêmes u16 non colinéaires et groupes greedy manqués.

L'oracle borné énumère les supports propres positifs, leurs ancres maximales et
leurs centres exacts, puis vérifie que chaque prune possède le nombre annoncé
de points strictement intérieurs. Un accord moyen ou un digest de la sortie du
sujet ne remplace pas ce rejeu.

GCP non utilisé.
