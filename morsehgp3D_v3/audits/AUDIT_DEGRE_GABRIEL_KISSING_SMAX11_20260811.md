# Audit mathématique — degré Gabriel, kissing number et `smax=11`

Date : 11 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Verdict

Dans l'espace euclidien, il n'existe dès la dimension deux aucune borne sur le
degré Gabriel d'un sommet qui ne dépende que de la dimension et de `smax`.
Plus fortement, le degré reste arbitraire dans chaque bucket de rang fermé
fixé. La borne 12 du kissing number en dimension trois ne s'applique donc pas,
même au bucket exact `closed_rank=11`.

Sur la grille u16 finie et avec des positions distinctes, les seules bornes
universelles immédiates sont les caps triviaux `n-1` et `2^48-1`. Elles ne
proviennent ni de `smax` ni du kissing number et n'ont aucun intérêt pour une
allocation industrielle. En particulier, `smax=11` ne borne ni le nombre de
records q2 incidents à un `PointId`, ni la taille totale de leur flux.

Le résultat ne condamne pas toute utilisation du kissing number. Une
séparation angulaire existe pour l'EMST et certaines sources plus minces; elle
peut justifier des banques directionnelles ou des propositions. Elle ne peut
jamais tronquer le catalogue Gabriel, dimensionner une ligne CSR à 12 ou
certifier la complétude des ancres q3/q4.

## 1. Convention fermée et contre-exemple exact

Pour une paire `p,q`, posons le prédicat diamétral

$$\Phi_{p,q}(x)=(x-p)\mathbin{\cdot}(x-q).$$

Le point `x` est strictement intérieur, sur la coquille ou strictement
extérieur à la boule diamétrale selon que `Phi` est négatif, nul ou positif.
Une paire de rang fermé deux vérifie donc
`Phi_{p,q}(x)>0` pour tout site tiers. C'est une condition plus forte que le
seul intérieur ouvert vide, qui autoriserait des contacts `Phi=0`.

> **Théorème.** Pour tout entier `m` et toute dimension `d>=2`, il existe un
> nuage possédant un sommet incident à `m` paires de rang fermé deux.

*Preuve.* Prendre `p=0` et `m` vecteurs unitaires distincts `u_i`, puis poser
`q_i=R u_i` pour un même `R>0`. Pour `j` distinct de `i`, on obtient

$$\Phi_{p,q_i}(q_j)=R^{2}\left(1-u_i\mathbin{\cdot}u_j\right)=R^{2}\left(1-\cos\theta_{ij}\right)>0.$$

Ainsi tout `q_j` est strictement extérieur à la boule diamétrale fermée de
`p q_i`. Chaque `p q_i` a donc le rang fermé deux et `deg(p)=m`. La
construction est plane et s'immerge telle quelle dans `R3`. Comme toutes les
inégalités sont strictes et en nombre fini, une perturbation assez petite
conserve les `m` arêtes; les distances égales et la cosphéricité ne sont pas la
cause de l'absence de borne. Fin de la preuve.

Toute notion de Gabriel d'ordre `k` définie par « au plus `k` sites tiers dans
la boule » contient le cas `k=0` et hérite immédiatement du même
contre-exemple. Les arêtes construites satisfont déjà `closed_rank=2<=11`.
Ni `RelevantGP`, ni l'agrégation des multiplicités, ni une borne sur le rang ne
rétablissent donc une borne de degré dépendant seulement de `smax`.

Le résultat vaut même bucket par bucket. Fixons un entier `h>=0`, une direction
unitaire `e`, une constante `c>0` et autant de directions unitaires distinctes
`u_i` que souhaité dans la calotte `u_i dot e>=c`. Posons `p=0`, `q_i=R u_i`
et ajoutons exactement `h` sites distincts `w_l=epsilon_l e`, avec
`0<epsilon_l<Rc`. Pour `j!=i` et pour chaque `l`,

$$\Phi_{p,q_i}(q_j)=R^{2}\left(1-u_i\mathbin{\cdot}u_j\right)>0,\qquad \Phi_{p,q_i}(w_l)=\epsilon_l\left(\epsilon_l-R e\mathbin{\cdot}u_i\right)<0.$$

Chaque boule diamétrale possède exactement `h` sites tiers strictement
intérieurs, aucun contact supplémentaire, et `closed_rank=h+2`. Son degré
dans ce bucket exact vaut le nombre arbitraire de feuilles. Les inégalités
étant strictes et en nombre fini, une perturbation générique assez petite
conserve le rang et peut séparer les rayons. Pour le bucket exact 11, il suffit
de prendre `h=9`.

En dimension trois, l'obstruction globale est plus forte : le nombre total
d'arêtes Gabriel peut être quadratique. Chazelle et al. donnent une
construction explicite de `n^2` arêtes croisées entre deux familles placées sur
des cercles orthogonaux entrelacés. Cette borne est déjà enregistrée dans
[`CATALOGUE_PAIRES_DIAMETRALES_EXACT.md`](../../docs/math/CATALOGUE_PAIRES_DIAMETRALES_EXACT.md).

## 2. Pourquoi le kissing number 12 ne s'applique pas

Le kissing number trois vaut 12 pour des sphères de même taille, aux intérieurs
disjoints, tangentes à une sphère centrale. Cette disjonction impose une
séparation angulaire. Les boules diamétrales vides d'un graphe Gabriel peuvent,
elles, se recouvrir arbitrairement.

La différence apparaît déjà dans les inégalités. Soient deux voisins `q_i` et
`q_j` de `p`, de rayons `r_i<=r_j`, séparés par l'angle `theta`. Le fait que
`q_i` soit hors de la boule diamétrale de `p q_j` donne seulement

$$r_i^{2}-r_i r_j\cos\theta\geq0\quad\Longrightarrow\quad\cos\theta\leq\frac{r_i}{r_j}.$$

Lorsque les rayons sont proches, cette contrainte autorise un angle
arbitrairement petit. En revanche, si les deux arêtes appartiennent à un EMST,
l'échange de l'arête la plus longue impose
`||q_i-q_j||>=r_j`, donc

$$\cos\theta\leq\frac{r_i}{2r_j}\leq\frac{1}{2},$$

et fournit alors l'angle de 60 degrés auquel un argument de kissing peut
s'appliquer. L'hypothèse décisive est l'échange MST, absente du graphe Gabriel.

La convention du RNG doit elle aussi rester explicite. Avec une lune ouverte,
l'étoile à rayons égaux ci-dessus a un degré RNG arbitraire : une autre feuille
est sur la frontière d'une des deux boules et n'appartient pas à leur
intersection ouverte. Si les longueurs incidentes sont deux à deux distinctes,
en revanche, deux voisins RNG de rayons `r_i<r_j` ne peuvent former un angle
`theta<=pi/3`, car

$$\left\Vert q_i-q_j\right\Vert^{2}\leq r_i^{2}+r_j^{2}-r_i r_j=r_j^{2}-r_i(r_j-r_i)<r_j^{2}.$$

Le point `q_i` serait alors strictement dans la lune ouverte de `p q_j`. Les
directions sont donc séparées de plus de 60 degrés et le kissing number donne
au plus 12 voisins. Une lune fermée traite aussi les égalités, mais définit un
autre graphe. Dans tous les cas, borner un sous-graphe RNG ne borne jamais son
surgraphe Gabriel.

## 3. Fixture u16 permanente demandée

Le contre-exemple suivant tue un cap 12 dans le profil exact du dépôt, sans
contact de coquille et sans ex aequo de distance à l'ancre. Poser
`p=(600,600,600)` et prendre les treize vecteurs

```text
( 5, 0, 0)  (-5, 0, 0)  ( 0, 5, 0)  ( 0,-5, 0)
( 0, 0, 5)  ( 0, 0,-5)  ( 3, 4, 0)  ( 3,-4, 0)
(-3, 4, 0)  (-3,-4, 0)  ( 3, 0, 4)  ( 3, 0,-4)
(-3, 0, 4)
```

En les numérotant de 1 à 13, définir `q_i=p+(100+i)v_i`. Toutes les
coordonnées appartiennent à `[35,1165]`, chaque `||v_i||^2` vaut 25, les
treize distances à `p` sont distinctes et, pour deux vecteurs distincts,
`v_i dot v_j<=20`. Pour tout `i` et tout `j` distinct de `i`,

$$\Phi_{p,q_i}(q_j)=(100+j)\left(25(100+j)-(100+i)v_i\mathbin{\cdot}v_j\right)\geq101(25\mathbin{\cdot}101-20\mathbin{\cdot}113)=26765>0.$$

La gate attendue exige les treize records `p-q_i`, chacun avec
`closed_rank=2`, d'abord sous `smax=2`, puis sous `smax=11`, et après
permutation des `PointId`. Un mutant `degree_cap_12`, `stop_after_12` ou une
arène de largeur fixe doit perdre un record, rompre le ledger et mourir. Cette
fixture teste le degré; les fixtures extra-shell existantes restent séparées
pour tester la distinction `Phi>0` contre `Phi>=0`.

### 3.1 Fixture u16 dans le bucket exact 11

Une seconde fixture tue l'interprétation plus forte « le bucket exact 11
aurait un degré borné par 12 ». Poser `p=(32768,32768,32768)`, ajouter les neuf
témoins `p+(l,0,0)` pour `1<=l<=9`, puis les treize feuilles `p+v` suivantes :

```text
(65,  0,  0)
(60, 25,  0)  (60,-25,  0)
(60, 24,  7)  (60, 24, -7)  (60,-24,  7)  (60,-24, -7)
(60, 20, 15)  (60, 20,-15)  (60,-20, 15)  (60,-20,-15)
(57, 20, 24)  (57,-20,-24)
```

Chaque norme carrée vaut 4 225, le produit scalaire de deux vecteurs distincts
vaut au plus 4 200 et chaque abscisse `v_x` vaut au moins 57. Pour deux
feuilles distinctes et pour `1<=l<=9`,

$$\Phi_{p,p+v_i}(p+v_j)=4225-v_i\mathbin{\cdot}v_j\geq25>0,\qquad \Phi_{p,p+v_i}(p+(l,0,0))=l(l-v_{i,x})<0.$$

Chacune des treize paires incidentes possède donc exactement neuf sites tiers
strictement intérieurs, aucun contact et `closed_rank=11`. Toutes les
coordonnées restent dans `[32743,32833]`. La gate doit exiger les treize
records dans le bucket exact 11 et tuer le même mutant de cap 12 que la fixture
de rang deux.

## 4. Ce que `smax=11` borne réellement

`smax` borne le contenu d'une boule publiée, pas le nombre de boules. Pour un
record q2 de rang fermé `R<=11`, le census contient au plus neuf `PointId`
autres que les deux extrémités. Ce record particulier propose donc au plus neuf
triplets et 36 quadruplets de même miniboule. Cette borne locale ne rend pas la
source q2 complète pour les supports propres q3/q4 : une sphère décalée peut
être utile alors que la boule diamétrale de l'une de ses arêtes dépasse le rang
11.

Pour une ancre q4 déjà certifiée, `smax=11` borne aussi la profondeur shallow à
`kappa=7`; la preuve locale existante donne au plus `8m_e` sommets shallow pour
`m_e` cordes actives. Là encore, elle borne le travail après découverte d'une
ancre, jamais le nombre d'ancres ni la somme globale des `m_e`.

Les conséquences industrielles sont donc :

- utiliser des offsets 64 bits vérifiés, `count+scan`, des segments reprenables
  avec backpressure, jamais une adjacence de largeur 12; le chemin industriel
  sans budget doit terminer exactement ou échouer sur une ressource physique
  réelle;
- fermer le ledger q2 sur l'univers complet des paires implicites, y compris
  lorsque la sortie réelle est quadratique;
- employer EMST, RNG sous convention déclarée, ou Yao48 comme propositions ou certificats fail-open,
  jamais comme remplacement silencieux du catalogue ou des ancres;
- mesurer distributions, maxima, octets et débordements par famille; une
  petite moyenne observée ne devient pas une borne de correction.

## 5. Baseline de Poisson : une moyenne, jamais un cap

Sous le Palm d'un processus de Poisson homogène simple d'intensité `lambda` sur
tout `R^d`, après insertion d'un voisin `y`, le nombre `J_y` d'autres points
dans sa boule diamétrale est de loi Poisson de paramètre
`t=lambda*kappa_d*||y||^d/2^d`. Campbell--Mecke, les coordonnées polaires et ce
changement de variable donnent

$$\mathbb{E}^{0}\!\left[\mathrm{deg}_{h}(0)\right]=\lambda\int_{\mathbb{R}^{d}}\sum_{j=0}^{h}e^{-t}\frac{t^{j}}{j!}\,dy=2^{d}\sum_{j=0}^{h}\frac{1}{j!}\int_{0}^{\infty}e^{-t}t^{j}\,dt=2^{d}(h+1).$$

Chaque bucket exact `J_y=j` contribue donc `2^d` au degré moyen. La frontière
porte presque sûrement zéro site, si bien que `closed_rank=2+J_y`. En
dimension trois, `smax=11` signifie `h=9` : dix buckets de degré moyen 8 et un
degré cumulé moyen 80.

Dans une grande fenêtre volumique à 50 000 sites, en négligeant le bord, cette
baseline suggère environ `50000*80/2=2 000 000` records q2 non orientés, soit
environ 200 000 par bucket. Développer seulement le fanout local de ces records
donnerait environ 9 millions de propositions de triplets et 24 millions de
propositions de quadruplets avant déduplication. Ces nombres ne sont ni une
source complète q3/q4, ni un résultat sur les familles G4 du dépôt.

La formule suppose l'espace entier, l'homogénéité et un processus diffus
simple. Une fenêtre dont les témoins sont tronqués, une intensité inhomogène,
une loi binomiale à `n` fixé, la quantification u16, les doublons ou les égalités
demandent des corrections et détruisent la constante exacte. Même dans le
modèle uniforme plan, le degré maximal croît : Devroye, Gudmundsson et Morin
obtiennent `Theta(log n/log log n)` en probabilité. La baseline 80 est donc une
moyenne de sortie, jamais une borne du degré maximal, de sa queue, du nombre de
candidats inspectés ou du temps.

## Références primaires

- O. R. Musin, [*The kissing problem in three dimensions*](https://arxiv.org/abs/math/0410324), preuve que le kissing number trois vaut 12.
- B. Chazelle et al., [*Selecting Heavily Covered Points*](https://www.cs.princeton.edu/~chazelle/pubs/SelectHeavyCoveredPts.pdf), lemme 5.1 et construction quadratique du graphe Gabriel en dimension trois.
- L. Devroye, J. Gudmundsson et P. Morin, [*On the Expected Maximum Degree of Gabriel and Yao Graphs*](https://arxiv.org/abs/0905.3584), degré maximal sous modèle uniforme aléatoire.

GCP non utilisé.
