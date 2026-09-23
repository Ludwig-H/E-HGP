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

## Mini-reçu reproductible

Depuis la racine du dépôt, sans écrire de fichier :

```sh
python3 morsehgp3D_v9/audits/check_q4_false_vertex_inside_disc_20260922.py
python3 -O morsehgp3D_v9/audits/check_q4_false_vertex_inside_disc_20260922.py
```

Le script vérifie les deux fixtures entières, dont la non-propriété du
cover q3 pour q4, puis les bornes `m(d+1)` et
`s(2d+2)` pour `d=0..3` sur trois familles dégénérées explicites et
400 familles aléatoires reproductibles de droites entières. Il compare
la sélection top/bottom à **toutes** les intersections exactes sur les
droites de graines, et recoupe les profondeurs 1D/2D. Il ne génère
ni cover WSPD, ni atlas, ni tour FULL ; ce mini-reçu n'est pas un gate
de qualification produit.
