# Contre-audit B — sommets q4 peu profonds d'une arête

Statut : **preuve combinatoire locale**, avec mini-oracle rationnel exact indépendant ;
ni générateur v9 modifié, ni qualification du catalogue FULL ou du contrat
LiDAR/G4. La note A examinée est
`Q4_STRUCTURE_ET_BORNES.md` (révision `efc14c99`, § « Une borne linéaire »).

## Verdict et prémisses

La borne `m(d+1)` sur les **centres distincts** peu profonds et sa version
sur `s` droites de graines, `s(2d+2)`, sont correctes même avec droites
coïncidentes de signes opposés, parallèles et concurrences. Elles portent
sur l'arrangement des formes du `Q34EdgeCover` **complet pour chaque centre
q4 utile** de propriétaire `ab`, pas sur toutes les présentations de
supports cosphériques. Si le cover manque un site intérieur, la profondeur
locale peut être trop faible et la route n'est plus complète. Le moteur
`d2700314` fait encore atlas puis tri d'événements par graine
(`morsehgp3D_v9/src/gen/lanes/q4_local.cpp:331-424,437-447`) ; la sélection `O(Km)` de
la note A n'est pas une implémentation ni une borne de son coût.

Fixer `m` droites géométriques distinctes des formes non constantes
`L_z=0` du cover ; les sites restent **avec multiplicité et orientation**.
Les formes constantes négatives sur tout le plan contribuent `p₀` ; poser
`d=K−3−p₀`. Si `d<0`, aucun centre q4 utile. Sur une droite `λ`, soient
`t₁<t_r` les intersections peu profondes extrêmes. Pour chaque
intersection strictement entre elles, choisir une autre droite `μ` qui
coupe `λ` à cet endroit. Sa restriction affine change de signe entre les
extrêmes : au moins un site porté par `μ` est strictement intérieur à
`t₁` ou `t_r`. Deux intersections distinctes ne peuvent choisir la même
`μ`. Le nombre d'intersections intermédiaires est donc au plus
`depth(t₁)+depth(t_r)≤2d` ; `r≤2d+2`. Chaque sommet est incident à au
moins deux droites distinctes, d'où `#centres≤m(d+1)`. Si seuls les
centres incidents à `s` droites de graines aiguës possédées sont demandés,
le décompte sans division par deux donne `#centres≤s(2d+2)`.

Les formes opposées sur une même droite **ne s'annulent pas** : hors de
leur droite, l'une est négative sur chaque côté, donc au moins un site
charge l'extrémité pertinente. Les formes identiquement nulles sur
`λ` sont des contacts sur cette droite ; les constantes négatives sur
`λ` diminuent le budget local. Une concurrence compte comme un sommet,
mais tous ses sites négatifs à chaque extrême conservent leur
multiplicité. Les lignes parallèles ne créent pas de sommet. Ces
conventions sont nécessaires pour appliquer la borne en dégénérescence.

## Pourquoi au moins une graine existe avant le census q3

Soit une q4 strictement positive de contacts `a,b,x,y`, de centre `c`
et rayon `R`, où `ab` est l'arête propriétaire la plus longue (départage
des égalités par ID). Translater `c` à l'origine, poser
`S=a+b` et `h=|S|²/2=2R²−|a−b|²/2`.
La stricte positivité barycentrique et l'indépendance affine imposent
`h>0` : si `h=0`, `c=(a+b)/2` appartient à l'arête `ab`, non à
l'intérieur du tétraèdre. Pour chaque contact `z`, l'identité
`(a−z)·(b−z)=h−S·z` est exacte ; `S·a=S·b=h`.
Si les deux complétions `x,y` étaient non aiguës à leur sommet,
`S·x,S·y≥h`. Leur combinaison barycentrique strictement positive
donnerait `0=S·c=Σλ_z S·z≥h>0`, contradiction. Au moins une des
faces `abx`, `aby` est donc aiguë à sa complétion ; les angles en `a,b`
sont aussi aigus puisque `ab` est plus longue dans la face. Le même
départage d'arête rend cette graine possédée. **Nul besoin qu'elle soit
admise par le census q3** : le test q3 peut rejeter toutes les faces
d'une q4 admise.

Une droite géométrique portant plusieurs graines peut être visitée une
fois pour découvrir les *centres* ; cela n'émet pas toutes les incidences
de supports exigées par le flux v8. La certification ultérieure doit
reconstituer une `BallKey`, faire le census global `I/U`, tester
`c∈conv(U)`, calculer `q_min` et dédupliquer.

## Contre-exemple dans le disque propriétaire

Prendre, en coordonnées entières non négatives,
`c=(13,13,13)`, `a=(25,13,18)`, `b=(1,13,18)`,
`x=(17,25,16)`, `y=(16,25,17)`.
Les quatre distances carrées à `c` valent `R²=169` et les six distances
croisées `ab,ax,ay,bx,by,xy` valent `576,212,226,404,370,2` : `ab`
est strictement propriétaire. Le déterminant du tétraèdre vaut `−288`,
donc les deux droites de contacts sont indépendantes. Les produits
`(a−x)·(b−x)=20` et `(a−y)·(b−y)=10` sont positifs ; les deux
graines sont aiguës. Le centre est un sommet de profondeur zéro du
nuage de quatre sites et satisfait **déjà** les bornes q4 du propriétaire :
`m=(a+b)/2=(13,13,18)`, `|c−m|²=25<D/8=72` et
`R²=169<3D/8=216`. Cependant chaque contact a `z≥16>c_z=13`, donc
`c∉conv{a,b,x,y}` ; cette sphère n'est pas la miniballe des contacts.
Le test convexe reste indispensable même après le filtre du disque.
La fixture de la note A prouve aussi « shallow ≠ miniballe », mais son
centre est déjà **hors** du disque `D/8`.

## Frontière de propriété : le cover d'une graine q3 ne suffit pas à q4

Contre-fixture entière, vérifiée par arithmétique exacte le 23 septembre :
`c=(30,30,30)`, `a=(39,10,42)`, `b=(50,42,21)`,
`x=(21,18,10)`, `y=(30,45,50)`, `z=(6,36,29)`.
Les quatre contacts sont à distance carrée `625` de `c`, tandis que
`|z−c|²=613<625`. Le centre est **strictement** intérieur au tétraèdre :
ses poids barycentriques sur `(a,b,x,y)` sont
`(4575,4680,14975,14336)/38566`, tous positifs. Les six arêtes ont
les carrés `ab=1586`, `ax=1412`, `bx=1538`, `ay=1370`, `by=1250`,
`xy=2410`. Ainsi `ab` est l'arête strictement la plus longue des deux
faces aiguës `abx` et `aby`, mais **pas** celle du tétraèdre : son
propriétaire q4 est `xy`.

Pourtant, avec `m_ab=(89/2,26,63/2)`, on obtient
`|z−m_ab|²=3177/2>1586=|ab|²` : le site strictement intérieur `z`
est absent du cover de `ab`. Un census q4 de cette sphère à partir du
cover de la graine q3 annoncerait `p=0` au lieu de `p=1`, alors que la
coquille a quatre contacts et que la configuration est admissible dès
`K=4`, donc aussi pour `K=5`. La propriété de l'arête q3 dans deux faces
ne certifie **jamais** le cover q4 ; il faut établir le propriétaire de
du tétraèdre avant de substituer un cover au census global.

Ce n'est **pas un défaut produit observé** : le moteur v9 publié à
`e54f727c` vérifie l'arête propriétaire du support q4 avant d'en
déclarer la profondeur globale. Cette fixture est une porte à conserver
si le développement réutilise une ligne de graine ou accélère le census
à partir d'un cover local.

## Sélection, coût caché et piste non acquise

Sur une droite de graine `λ`, chaque site du cover donne une restriction
`α_z t+β_z`. Pour un événement `τ=−β_z/α_z`, une pente positive
contribue à la profondeur si son seuil est strictement **plus grand**
que `τ`, une pente négative si son seuil est strictement **plus petit**.
Les égalités restent sur la coquille. Après avoir payé les constantes
négatives de `λ`, un événement de profondeur `≤d_λ` figure donc parmi
les `d_λ+1` plus grands seuils distincts de pente positive ou les
`d_λ+1` plus petits de pente négative. Un buffer de taille `O(K)` et
un scan exact de profondeur aux `≤2d_λ+2` valeurs donnent `O(Km)`
**par droite de graine après regroupement** ; conserver les deux
orientations et leurs multiplicités est obligatoire.

Ce coût n'inclut pas la construction/lecture du cover de `C` sites, son
regroupement exact, les contacts `U` à matérialiser, le census global,
le test convexe, `q_min`, ni le catalogue. Le regroupement coûte au moins
`Ω(C)` et, par tri déterministe simple, `O(C log C)`. Après lui, le
schéma direct lit au moins les `m` groupes pour chacune des `s` graines
(`Ω(sm)`) ; la sélection et les contrôles ont une borne `O(sKm)`.
Si `s≈m` à `K` fixé, ce schéma peut donc payer un carré local.
La borne de **sortie** `O(Km)` ne donne donc pas encore un algorithme
sous-quadratique. Le disque universel `|c−m|²≤D/8` ne réduit même pas
`s` à lui seul : le centre q3 de chaque graine aiguë possédée est sur
sa droite et vérifie déjà `|c₃−m|²≤D/12`. Une sélection collective de
niveaux peu profonds ou des gardes certifiés propres à plusieurs
droites sont des pistes, à condition de chiffrer construction du cover,
dégénérescences, census et sorties ; aucun gain LiDAR n'est acquis.

Piste bibliographique **non transférée** :
[Everett–Robert–van Kreveld, 1993](https://doi.org/10.1145/160985.160994)
annoncent `O(m log m+Km)` pour les `≤K` niveaux *ordinaires* de droites.
Leur application à la séparation bleu/rouge ne suffit pas à conclure
qu'ils construisent tous les sommets de profondeur de demi-plans
**orientés arbitrairement**. À abscisse fixée, cette profondeur vaut,
hors contacts, `n_bas + #haut_sous − #bas_sous` : elle peut augmenter
**ou diminuer** à chaque franchissement de ligne, contrairement au rang
monotone d'un niveau ordinaire.
[Har-Peled–Sharir, 2016, §1 et lemme 2.5](https://www.math.tau.ac.il/~michas/k_depth.pdf)
traitent précisément les demi-plans mixtes et prouvent une borne de
**complexité**, non l'algorithme exact de construction souhaité ; leur
cadre est en position générale. Il reste à prouver un transfert
algorithmique pour profondeur stricte, concurrences, coïncidences de
signes opposés et multiplicités, en payant aussi le cover. Aucune borne
`O(m log m+Km)` n'est acquise pour la route q4.

Relecture bibliographique du 23 septembre :
[Chan, *Low-Dimensional Linear Programming with Violations*, §1](https://tmc.web.engr.illinois.edu/vio.pdf)
décrit explicitement la construction des niveaux peu profonds des
demi-plans **supérieurs et inférieurs**, puis leur intersection, avec
`O(n log n+nk)` pour le problème 2D en position générale. La
[preuve annexe de Halperin–Har-Peled–Mehlhorn–Oh–Sharir](https://sarielhp.org/p/20/max_level/max_level.pdf)
donne `O(n log n+nk)` pour les niveaux ordinaires et une voie de
perturbation symbolique permettant de rabattre certains résultats sur
un arrangement dégénéré. Ces sources rendent plus crédible une
**construction collective à deux familles** que le scan `s·m` par
graine ; elles ne fournissent pas directement l'énumération exacte des
centres q4 à profondeur *stricte* avec lignes de signes opposés
coïncidentes, poids de sites, coquilles, verticales et propriétaire.
Après regroupement de `C` sites en `m` droites, une cible conditionnelle
est `O(C log C + mK polylog m + I)` par arête, où `I` est le coût de
report des contacts/intérieurs nécessaires. Le terme `Ω(C+I)` est
inévitable même si `m` est petit. Remplacer naïvement un groupe de poids
par `O(K)` lignes perturbées ferait déjà apparaître un facteur `K²`
dans la construction des niveaux ; `K≤10` n'annule pas l'obligation de
mesurer le total sur toutes les arêtes.

Le registre **brut, non accepté** G4 R2 sur 08/000000 sans sol à 1 mm
rend ce terme décisif : `Σ cover_sites=2 963 451 407` à K5 et
`7 805 426 490` à K10, pour `n=39 885` (`n²=1 590 813 225`).
`cover_sites` est une population **logique** de ranges répétées, pas
autant de lectures actuellement effectuées ; mais un constructeur de
niveaux qui énumère/regroupe chaque site de **chaque cover complet** paierait
au moins cette masse, déjà supérieure à `n²` sur ce régime important.
À K10, même si chacune des `51 344` arêtes à cover sans voie q4
contenait les `39 885` sites, les `4 455 934` covers q4 totaliseraient
encore au moins `5 757 571 050` sites logiques, soit plus de `3,6n²`.
La conclusion K10 vaut donc aussi pour un constructeur limité aux
arêtes q4 ; ce minorant ne découle pas de l'agrégat K5 seul.
La même soustraction conservatrice à K10 donne, sur 08/000100
(`n=35 551`), au moins `2 493 423 316` sites de covers q4
(`1,97n²`), et sur 08/000200 (`n=45 845`) au moins
`6 505 041 335` (`3,10n²`). Les trois scans sont d'une seule
séquence ; ces ratios n'établissent pas une loi asymptotique, mais
montrent **dans chacun de ces régimes mesurés** qu'une lecture exhaustive
par cover q4 paierait déjà plus de `n²` unités élémentaires ; seul un
protocole de croissance apparié peut établir son exposant.
La route collective doit donc filtrer/partager les formes avant ce
regroupement, ou réduire le nombre/couverture d'arêtes, puis publier le
nombre réel de sites lus. Une bonne borne `O(mK)` *après* matérialisation
du cover ne clôt pas le P0 sous-quadratique global. Le
[certificat de domination par gardes et blocs](DOMINATION_Q4_PARESSEUSE_PAR_BLOCS_20260923.md)
est une piste exacte **avant** cette matérialisation ; son seuil doit
rester `K−1` si l'atlas partagé sert aussi q3, et son coût total `E×C×Z`
reste à mesurer.

Les comparaisons exactes de racines **sur une même droite** se réduisent
aux produits i128 déjà bornés dans cette note, mais un balayage global
peut comparer des intersections de deux paires de droites sans ligne
commune : ses produits croisés exigent potentiellement environ 160 bits.
Ni un cisaillement pour effacer les verticales ni une perturbation
symbolique ne conservent automatiquement les bornes u18/i128 publiées.
Il faut écrire et qualifier ces prédicats, prouver l'arrêt du reporting
à profondeur `K−2`, puis payer census global et FULL avant de déclarer
cette route sous-quadratique en régime LiDAR.

Le lemme de cover propriétaire admet, pour les supports positifs q4, une
petite marge exacte : tout intérieur/contact vérifie
`|z−(a+b)/2|² < ((2+√3)/4)D < 15D/16`, soit le prédicat entier
`4|2z−a−b|² < 15D`. Pour q3 la borne analogue est `3D/4`.
Ce resserrement de constante pourrait réduire la population du cover,
mais il faut mesurer les boîtes, raffinements et coûts aval sur LiDAR ;
il ne traite ni le facteur `s·m` ni la complétude du front.

Le complément A `2291da13` factorise exactement la comparaison de deux
racines par le déterminant de trois formes. Sous les bornes u18 de ces
formes, sa valeur absolue est inférieure à `2^121` et son signe tient en
i128 : conserver seulement les signes du pivot et des pentes évite de
former les produits croisés de 160 bits. Les pivots négatifs, pentes
nulles et racines égales restent des branches distinctes. Ce gain de
largeur ne change pas le facteur `s·m` du schéma direct.

## Grand shell : recherche d'un support sans carré de paires

Autre verrou, **distinct** de la découverte des centres peu profonds :
une fois la coquille entière `U` d'une boule q4 connue, trouver **un**
tétraèdre strictement positif dont `ab` est l'arête propriétaire ne
requiert pas mathématiquement de tester les `O(|U|²)` paires. Il existe
une réduction exacte conditionnelle vers dominance 2D et maximum de
produit scalaire 3D. Elle n'est **ni implémentée ni chronométrée** ; si le
flux exige toutes les présentations plutôt qu'une preuve d'existence,
sa taille de sortie peut elle-même être quadratique.

Écrire `A=a−c`, `B=b−c`, `n=A×B`. Si `n=0`, `a,b` sont antipodaux sur
la sphère, donc `c` appartient déjà au segment `ab` : aucun tétraèdre
strictement positif de cette boule ne peut avoir `ab` comme arête.
Sinon, pour chaque contact `Z=z−c`, poser
`D_Z=det(A,B,Z)`,
`P_Z=det(Z,B,n)/D_Z` et `Q_Z=det(A,Z,n)/D_Z` lorsque `D_Z≠0`.
Un support positif doit avoir un contact `X` avec `D_X>0` et un `Y`
avec `D_Y<0`. La résolution de ses quatre poids barycentriques donne
exactement
`c∈int conv(a,b,x,y) ⇔ P_Y>P_X et Q_Y>Q_X` ; les deux égalités sont
**exclues**. On filtre d'abord chaque contact dont `ax` ou `bx` viole
la propriété de `ab` (longueur, puis plus petite paire d'IDs en cas
d'égalité). L'arête finale `xy` vérifie `|xy|²≤|ab|²` si et seulement si
`(x−c)·(y−c)≥R²−|ab|²/2`. La dominance choisit les `Y` possibles ;
le maximum de ce produit scalaire parmi eux décide l'inégalité. Une
égalité de longueur est admise seulement si la paire `xy` vient **après**
`ab` dans l'ordre des IDs, ce qui se traite par un second index filtré
sur les IDs de `Y` lorsque `X` satisfait lui aussi cette condition.

Une structure de plages 2D sur `(P_Y,Q_Y)`, avec un convexe des vecteurs
`Y` dans chaque nœud canonique et une requête d'extrême exacte de type
[Dobkin–Kirkpatrick](https://dpd.cs.princeton.edu/Papers/DobkinKirkpatrick.pdf),
donne une **cible théorique** de prétraitement et de recherches
`O(u log³u)`, espace `O(u log²u)` pour `u=|U|`, au lieu de `u²` tests
de paires. Les coordonnées égales en dominance restent exclues ; les
coquilles coplanaires, convexes de dimension inférieure, nombreux centres
et tables dupliquées exigent des branches exactes et une mesure de
mémoire. La réduction ne supprime ni la construction de `U`, ni les
`Σ cover_sites` super-quadratiques observés ci-dessus, ni les coûts de
catalogue/FULL. Elle est surtout un recours pour les rares grandes
coquilles, après histogramme des tailles et des sorties.

Sous la grille u18, les comparaisons projectives n'imposent pas
directement des fractions géantes : choisir quatre contacts affinement
indépendants donne par Cramer un dénominateur homogène normalisé
`0<Δ<2^60` et des numérateurs de centre `<2^79`. Les vecteurs entiers
`Z'=Δz−N` ont alors des composantes de valeur absolue `<2^80`.
L'identité
`det(Y,B,X)=D_X r_Y(P_Y−P_X)`, avec
`r_Y=D_Y/|n|²`, et son analogue pour `Q`, ramène le signe d'une
différence de clés à un déterminant 3×3 de valeur absolue `<2^243` :
**256 bits signés suffisent pour ces tris**. Cette borne ne certifie pas
encore tous les prédicats ni la construction du convexe ; un oracle
Fraction et des mutants de signes, égalités et propriété restent requis.
La fixture `c=0`, `a=(−5,0,0)`, `b=(3,−4,0)`, `x=(0,0,−5)`,
`y=(0,3,4)` montre pourquoi le dernier test `xy` est indispensable :
la positivité est stricte et les quatre autres arêtes sont au plus
`|ab|²=80`, mais `|xy|²=90`.

Un [mini-oracle Fraction indépendant](check_q4_owner_dominance_20260923.py)
vérifie la réduction de décision (pas l'arbre de plages ni le convexe
3D) contre un solveur barycentrique et toutes les arêtes. Rejoué en
Python normal et `-O`, il passe cinq fixtures causales, 315 coquilles
aléatoires entières (3 992 paires de côtés opposés dont 635 positives,
1 281 égalités de coordonnées projectives) et 320 centres u18 pour les
bornes homogènes ; SHA-256 `be054948d39ef2b4e3434e46afa5b3a73290f872086db39ad3d91a701bb925a8`.
Il ne certifie ni le coût ni les cas de convexes dégénérés d'une future
structure native.

## Mini-reçu reproductible

Depuis la racine du dépôt, sans écrire de fichier :

```sh
python3 morsehgp3D_v9/audits/check_q4_false_vertex_inside_disc_20260922.py
python3 -O morsehgp3D_v9/audits/check_q4_false_vertex_inside_disc_20260922.py
python3 morsehgp3D_v9/audits/check_q4_owner_dominance_20260923.py
python3 -O morsehgp3D_v9/audits/check_q4_owner_dominance_20260923.py
```

Le script vérifie les deux fixtures entières, dont la non-propriété du
cover q3 pour q4, puis les bornes `m(d+1)` et
`s(2d+2)` pour `d=0..3` sur trois familles dégénérées explicites et
400 familles aléatoires reproductibles de droites entières. Il compare
la sélection top/bottom à **toutes** les intersections exactes sur les
droites de graines, et recoupe les profondeurs 1D/2D. Il ne génère
ni cover WSPD, ni atlas, ni tour FULL ; ce mini-reçu n'est pas un gate
de qualification produit.
