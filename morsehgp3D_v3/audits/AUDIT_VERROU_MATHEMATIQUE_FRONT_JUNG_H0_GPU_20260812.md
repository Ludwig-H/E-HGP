# Audit du verrou mathématique — front de Jung, événements H0 et voie G4

Date : 12 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Pin de départ observé pendant cette réflexion :
`HEAD=b3c8f75a17f861c7feac84690ee708221554796a`. Le dépôt a ensuite avancé
jusqu'à `407d4d1b2745f03a7237080a75daba1c7122ea0a` pendant la rédaction. Le
présent texte ne qualifie aucun de ces deux snapshots : il apporte des
théorèmes, des contre-fixtures et un plan de falsification. Le verdict logiciel
reste dans [`AUDIT_ETAT_COURANT.md`](AUDIT_ETAT_COURANT.md). Les constantes,
la fixture et les claims de trafic ci-dessous ont été contre-audités; les
corrections sont intégrées au présent texte.

## 1. Réponse courte à Claude

La route n'est pas fermée par les pentes publiées, mais elle n'est pas reçue :

- `terrain` a une pente de cellules rouge puis une pente verte; la règle des
  deux pentes rouges ne suspend donc pas l'ordonnance;
- cette observation ne prouve ni que la superlinéarité est transitoire au sens
  asymptotique, ni que l'ancien binaire majore le temps du nouveau;
- le point `uniform,n=50 000` confirme au contraire que la charge utile est
  très grande : `21 395 212` supports et `839 582 666` géométries, soit
  `39,242` géométries par support;
- la bonne première rupture reste donc le RLE `SupportKey` avant le lift et
  l'owner, déjà posé dans
  [`AUDIT_DEBLOCAGE_GPU_SUPPORTKEY_TOP12_20260812.md`](AUDIT_DEBLOCAGE_GPU_SUPPORTKEY_TOP12_20260812.md).

Le nouveau `rank_cell` de Claude est sûr dans sa version actuelle sous
l'invariant de pool, mais sa borne est volontairement faible. La section 2
donne la version exacte forte. Le verrou plus profond n'est toutefois pas ce
prune : c'est produire les supports q3/q4 sans payer le catalogue des cliques.
La section 5 propose un front d'ancres dont la taille est linéaire en espérance
sur le régime volumique Poisson et qui possède une couverture déterministe des
supports pertinents. Cette propriété de la sortie ne borne pas son producteur :
le dual-tree d'ancres existant a déjà des pentes voisines de `2,3`. Le front est
donc une expérience falsifiable, pas encore la voie produit. La rupture
immédiate reste le flot plat `SupportKey` décrit par l'audit de sentinelle hors
support.

Réponse aux questions live de Claude : seule la gate de **compteurs physiques
sélectionnés sur `uniform`** est verte. La gate contractuelle avant CUDA ne
l'est pas : `eight_clusters`, mémoire/HWM, digest d'identités, source device et
payload manquent encore. Les masses `overlap_pairs`, `potential_triples` et
`potential_quads` ont au contraire des pentes rouges; elles peuvent être
exclues de l'ordonnance, pas appelées « tous les compteurs ». La lecture `k=1`
par q2 à intérieur ouvert vide est un comparateur de poids MST, pas encore un
remplacement reçu de Yao-1 : elle paie toute la source, ne publie pas les
extrémités et ne vérifie ni coupes, ni lots égaux, ni multifusions. Enfin, la
gate threads du successeur est bornée à de petits cas; elle ne reçoit ni les
options de multiplicité, ni une identité de catalogues à 50 000 points.

Le contre-audit du parallélisme a reproduit une panne de télémétrie distincte :
sur `n=40,smax=4,uniform,seed=11`, `--multiplicity --threads=1` compte
`22 535` occurrences pour `22 543` lifts, tandis que `--threads=2` n'en compte
que `7 012` pour les mêmes `22 543` lifts; les deux commandes rendent zéro.
Jusqu'à réparation et fermeture des ledgers, `multiplicity && threads>1` doit
être refusé explicitement et ne peut recevoir `SupportKey_unique`. Ce constat
vise l'instrument; il ne prouve pas à lui seul une différence du payload normal.

## 2. Certificat directionnel exact de rayon positif

Soient un domaine convexe compact `K`, un pool fini fixé `P`, et une direction
entière non nulle `d`. Pour une convention de côté `sigma` dans `{+1,-1}`,
poser :

$$\lambda_{d}^{\sigma}(K,P)=\min_{x\in P}\ \min_{c\in K,\ \sigma d\mathbin{\cdot}(x-c)\geq0}\left\Vert x-c\right\Vert^2,$$

avec la valeur `+infini` si aucun couple n'est faisable. Pour une banque finie
de directions `D`, poser :

$$\Lambda_D(K,P)=\max_{d\in D}\max\left(\lambda_d^+(K,P),\lambda_d^-(K,P)\right).$$

### Théorème 1 — borne inférieure de rayon

Soit `U subset P` un support propre positif de centre `c_B in K` et de rayon
carré `beta_B`. Alors :

$$\beta_B\geq\Lambda_D(K,P).$$

En effet, `c_B in relint conv(U)`. Tout demi-espace **fermé** passant par
`c_B` rencontre donc `U`; pour une direction orthogonale à `aff(U)`, tous les
contacts peuvent être sur le plan et les inégalités ne sont pas strictes. Un
témoin `u` de chaque côté est faisable dans la définition correspondante et
vérifie `lambda<=||u-c_B||^2=beta_B`.

Cette précision corrige le commentaire « strictement de chaque côté ». La
paire `(1,1,1),(3,1,1)`, de centre `(2,1,1)`, et la direction `(0,1,0)` donnent
une égalité pour les deux membres.

### Corollaire 1 — prune par rang sans construire de support

Pour la lane `q`, supposer `U subset P_q`, `t_q<=|P_q|`, et soit `R_q(K)` la
`t_q`-ième plus petite valeur de `u_K` dans ce pool fixé, avec
`t_q=smax-q+1`. Si :

$$R_q(K)<\Lambda_D(K,P_q),$$

alors aucun support positif pertinent d'arité `q` n'a son centre dans `K`.
Les `t_q` témoins de `u_K<=R_q` seraient tous strictement intérieurs puisque
`R_q<beta_B`, donc `p+q>smax`. À `smax=11`, les seuils sont respectivement dix,
neuf et huit pour q2, q3 et q4.

Le code observé utilise `l_K(x)` après un test de possibilité de côté. Cette
quantité est inférieure ou égale au minimum contraint ci-dessus : son prune est
donc fail-open, mais moins fort. La version forte demande la distance de `x` au
polytope `K` intersecté avec un demi-espace. Pour une AABB en dimension trois,
la projection exacte s'obtient en énumérant un nombre constant d'ensembles
actifs : faces de boîte, plan directionnel et leurs intersections. Aucun solveur
itératif n'est requis.

### Corollaire 2 — monotonie et contraction

Pour `K' subset K` et le même pool fixé, les minima contraints ne peuvent
qu'augmenter et `u_K'(x)` ne peut que diminuer. Un certificat reste donc vrai
dans tout descendant. En outre, tout centre pertinent appartient à
`conv(A_q(K))`. Pour chaque `d`, le slab exact :

$$\min_{x\in A_q(K)}d\mathbin{\cdot}x\leq d\mathbin{\cdot}c_B\leq\max_{x\in A_q(K)}d\mathbin{\cdot}x$$

peut contracter le domaine actif avant de recalculer les bornes. Une direction
de covariance quantifiée, une direction issue d'un GJK flottant ou une normale
estimée ne sont que des propositions; le test entier final est l'autorité. Le
reçu conserve la direction entière, les deux témoins, `R_q`, les deux lambdas,
la comparaison stricte et le repli d'arithmétique large.

Ce théorème peut réduire le volume vide, mais il ne borne pas le nombre de
supports. Il ne faut pas le confondre avec la rupture de source recherchée.

## 3. Ce qu'une réduction H0 peut vraiment économiser

Pour un ensemble complet de générateurs saturés `Sigma`, fixer `k` et former
le graphe dont les sommets sont les générateurs `S` avec `|S|>=k`, de naissance
`b(S)=beta(S)`, et dont une arête `ST` existe lorsque `|S intersect T|>=k`, au
niveau `max(b(S),b(T))`. À toute coupe de niveau `a`, ce graphe est exactement
le graphe d'intersection qui calcule `pi0(L_k(a))`.

### Théorème 2 — sparsification après découverte

Une forêt couvrante minimale de Kruskal de ce graphe, avec les naissances des
sommets et chaque lot égal traité atomiquement, conserve toutes les composantes
aux coupes ouvertes et fermées. Elle emploie au plus `|Sigma|-1` arêtes par
ordre. Les forêts des ordres successifs n'ont pas besoin d'être imbriquées; une
verticale retrouve, dans la forêt inférieure, la racine d'un générateur
canonique de la composante supérieure.

Une boule régulière positive d'indice un porte `p+q=k+1`. À `K<=10`, son
record local a au plus onze labels et au plus quatre bras stricts. Une fois la
boule **découverte** et les racines pré-lot connues, son effet H0 se réduit à
une étoile d'au plus trois unions. Le bon token de bras est
`(BallId,removed_support_slot)`, jamais la copie développée de ses `k` labels.

Cette réduction économise les arêtes de cycle et les incidences H0 inertes.
Elle n'économise ni les naissances inconnues, ni la découverte de `Sigma`, ni
les memberships nécessaires à l'overlap. L'union des `PointId` d'une composante
n'est pas un résumé suffisant : deux générateurs peuvent chacun rencontrer un
tiers en moins de `k` labels alors que l'union de couverture en rencontre `k`.

## 4. Pourquoi le raccourci LP-type ne ferme pas la découverte

La miniboule d'un candidat possède une base géométrique de taille au plus
quatre. Cela ne rend pas le **choix externe** du prochain événement LP-type.

Déjà pour la plus proche paire bichromatique, prendre sur une droite
`F={rouge 0, bleu 10}`, `G=F union {bleu 100}` et `h=rouge 101`. La valeur du
minimum vaut dix sur `F` et `G`; ajouter `h` ne change pas `F`, mais abaisse le
minimum de `G` à un. L'axiome de localité LP-type échoue. Mettre toutes les
sphères candidates dans le ground set redonne un LP-type tautologique de taille
combinatoire et cache simplement l'énumération.

Plus grave, la prochaine fusion n'est pas forcément un contact direct entre
deux composantes strictes. Voici une fixture u16 affine-3 pour `k=2` :

```text
A=(15,22,3)  B=(7,14,3)  C=(23,14,3)
D=(15,6,3)   E=(0,17,1)  F=(16,0,0)
```

Les triangles `ABE` et `CDF` naissent à `127/2<64` et donnent deux racines
non triviales. Au niveau 64, `ABC`, `ABD`, `ACD` et `BCD` forment un plateau
qui relie ces racines via les facettes latentes `AC` ou `BD`. Aucune coface
individuelle de niveau 64 ne contient une facette de chacune des deux racines
strictes. Le temps de rencontre est donc un chemin minimax dans le graphe
implicite, pas la valeur d'une unique miniboule bichromatique.

Cette fixture doit tuer les mutants suivants : contact direct seulement et
traitement séquentiel du lot 64. Une comparaison de `pi0` seule ne tue pas
nécessairement la suppression de `AC/BD` ou l'omission d'un co-minimiseur, car
des chemins redondants subsistent. La porte catalogue distincte compare toutes
les permutations et attend une seule `GeometricBallKey` de shell
`{A,B,C,D}` avec ses quatre cofaces de plateau.

Conclusion : Boruvka/MSF compresse l'aval si un oracle exact fournit les
minima sortants, tous les ex aequo, les carriers latents et un certificat
d'absence en dessous. Cet oracle est précisément la source manquante. Il ne
faut pas annoncer qu'une base de miniboule de taille quatre le résout.

## 5. Proposition positive — le front canonique de Jung

Le théorème déterministe déjà reçu dit que tout support q3/q4 positif possède
une arête de longueur maximale, et qu'une telle arête ne peut être supprimée si
elle n'a pas respectivement neuf ou huit témoins universels de Jung. Le verrou
algorithmique devient donc : produire uniquement les paires qui n'atteignent
pas ce certificat, puis étendre ces ancres.

Ce front possède une justification moyenne exacte sous Poisson qui n'avait pas
encore été exploitée.

### Théorème 3 — intensité Poisson du front d'ancres

Considérer un processus de Poisson homogène tridimensionnel d'intensité `rho`.
Pour une paire à distance `D`, noter `W_q(a,b)` le spindle de témoins
individuellement universels pour les centres de Jung q3 ou q4. Son volume est
homogène de degré trois : `vol(W_q)=v_q D^3`. Les constantes exactes s'obtiennent
par intégration du solide de révolution défini par les prédicats de Jung :

$$v_3=\frac{\pi}{4}-\frac{\pi^2}{9\sqrt{3}}=0{,}1522627458681087,\qquad v_4=\frac{7\pi}{24}-\frac{3\pi}{8\sqrt{2}}\arctan\left(\sqrt{2}\right)=0{,}1204803754461729.$$

Pour vérifier ces constantes, normaliser `D=1`, placer le milieu à l'origine,
noter `z` la coordonnée axiale physique et `r` la distance à l'axe. Avec
`a_3=1/sqrt(3)` et `a_4=1/sqrt(2)`, le rayon de section du spindle est :

$$r_q(z)=\frac{\sqrt{1+a_q^2-4z^2}-a_q}{2},\qquad -\frac{1}{2}\leq z\leq\frac{1}{2},\qquad v_q=\pi\int_{-1/2}^{1/2}r_q(z)^2\,dz.$$

Dans un PPP stationnaire infini, en localisant les **milieux** des paires dans
un domaine `Omega`, le nombre attendu de paires non ordonnées ayant au plus
`h-1` témoins vaut exactement :

$$\mathbb{E}[N_{q,h}(\Omega)]=\frac{2\pi h}{3v_q}\rho\lvert\Omega\rvert.$$

Preuve : Campbell--Mecke donne le facteur
`rho^2 |Omega| 4 pi D^2 dD / 2`; le nombre de témoins est Poisson de moyenne
`rho v_q D^3`; sommer les probabilités de zéro à `h-1`, poser
`z=rho v_q D^3`, puis employer `integral_0^inf e^{-z} z^j dz=j!` donne la
formule.

Avec `h=9` pour q3 et `h=8` pour q4 :

| lane | espérance bulk d'ancres survivantes |
| --- | ---: |
| q2, au plus neuf intérieurs diamétraux | `40 rho |Omega|` |
| q3, moins de neuf témoins Jung universels | `123,796243 rho |Omega|` |
| q4, moins de huit témoins Jung universels | `139,069627 rho |Omega|` |

Les trois régions sont imbriquées : `W4 subset W3 subset W2`, où `W2` est la
boule diamétrale et `v2=pi/6`. Il est donc inutile de matérialiser trois copies
d'une paire : un seul `PairId` porte un masque de lanes. Pour
`t=rho D^3`, les nombres de témoins dans les trois couronnes disjointes sont
des Poisson indépendantes `Z4`, `Z34`, `Z23`, de paramètres respectifs
`v4 t`, `(v3-v4)t`, `(v2-v3)t`. Avec `N4=Z4`, `N3=Z4+Z34` et
`N2=Z4+Z34+Z23`, l'union des événements de survie se décompose exactement en :

$$A=\lbrace N_4\leq7\rbrace\mathbin{\dot\cup}\lbrace Z_4=8,Z_{34}=0\rbrace\mathbin{\dot\cup}\lbrace Z_{23}=0,(Z_4,Z_{34})\in\lbrace (8,1),(9,0)\rbrace\rbrace.$$

Après la même intégration de Campbell--Mecke, l'intensité physique coalescée
est :

$$C_{\mathrm{front}}=\frac{2\pi}{3}\left(\frac{8}{v_4}+\frac{v_4^8}{v_3^9}+\frac{v_4^8(9v_3-8v_4)}{v_2^{10}}\right)=141{,}183364803884.$$

La décomposition numérique vaut `139,069626544` pour q4, seulement
`2,113713855` de surcroît q3 et `0,000024405` de surcroît q2. Ainsi le front
final attendu est d'environ `7,06` millions de `PairId+mask` à 50 000 points,
et non `15,15` millions. La valeur `302,866 n` reste correcte comme somme des
**occurrences de lanes avant coalescence**, pas comme taille physique du front.

La couverture déterministe ne dépend pas du modèle Poisson : pour tout support
minimal propre positif q3/q4 pertinent, chaque arête maximale a `D>0`, son
centre appartient au disque de Jung, et tout `PointId` dans le spindle est
strictement intérieur à sa boule. Son spindle contient donc au plus `p<=8`
témoins en q3 et `p<=7` en q4; l'ancre survit bien le seuil correspondant.

La loi moyenne demande en revanche des précautions. Pour un PPP tronqué à une
boîte, un nuage uniforme conditionné à exactement `n`, `eight_clusters` ou la
quantification u16, elle n'est qu'une baseline bulk avec correction de bord,
pas une identité. Les spindles et boules témoins sont ouverts : leur frontière
a mesure nulle sous PPP continu, mais doit rester traitée exactement en u16.
Les comptes portent sur des `PointId` distincts, jamais sur des visites; deux
identifiants de même position ne sont pas fusionnés. Une paire de longueur
nulle reste fail-open. Les supports impropres ou non positifs appartiennent à
leurs lanes de dégénérescence. Enfin, surfaces, cosphères et doublons peuvent
encore rendre le front quadratique au pire. Cette constante n'est donc ni une
borne déterministe, ni une preuve de débit du producteur; elle rend la piste
quantitativement falsifiable sur les deux familles bloquantes.

### Front intermédiaire par boule de milieu

Le spindle complet n'est pas nécessaire au premier étage. Pour une paire
`a,b` de longueur `D` et de milieu `m`, les inclusions suivantes découlent
directement des prédicats norm-only déjà reçus :

$$B\left(m,\frac{D}{\sqrt{12}}\right)\subset W_3(a,b),\qquad B\left(m,\frac{D}{\sqrt{15}}\right)\subset W_4(a,b),$$

où les boules témoins sont lues ouvertes. La seconde borne est légèrement plus
petite que le plus grand rayon radial du spindle q4 et reste donc strictement
sûre. Neuf points dans la première boule ou huit dans la seconde éliminent la
paire de la lane correspondante.

Cette relaxation a elle aussi une intensité Poisson exacte. Ses coefficients de
volume sont `pi/(18 sqrt(3))` et `4 pi/(45 sqrt(15))`; les fronts survivants
valent donc respectivement :

$$108\sqrt{3}\,\rho\lvert\Omega\rvert\simeq187{,}0615\,\rho\lvert\Omega\rvert,\qquad 60\sqrt{15}\,\rho\lvert\Omega\rvert\simeq232{,}3790\,\rho\lvert\Omega\rvert.$$

Avec q2, la somme des occurrences de lanes vaut environ `459,44 n`. Comme les
trois boules de milieu sont elles aussi imbriquées, la même coalescence donne
seulement `233,807309 n` `PairId+mask`, soit `11,69` millions à 50 000 points
en bulk avant le test de spindle complet. Ce front est plus large que le front
final coalescé `141,183365 n`, mais sa primitive est seulement un range-count
de boule de milieu. Il fournit donc une cascade concrète :

```text
univers implicite des paires
  -> boule de milieu : seuil 10/9/8 par lane
  -> spindle Jung complet ou certificat collectif
  -> paires q2 et ancres q3/q4
  -> extension indépendante q3/q4
```

Une réalisation exacte peut ordonner les produits de nœuds AABB par classes
dyadiques de distance. Pour un bloc d'extrémités, calculer une boule ou une AABB
témoin contenue dans **toutes** les boules de milieu du bloc; si son range-count
contient le seuil de `PointId` distincts, tout le produit est pruné. Sinon le
plus gros nœud est divisé, puis une microtuile finit par le prédicat ponctuel.
La frontière se ferme par l'identité
`pair_mass_pruned+pair_mass_microtiles=C(n,2)` pour chaque masque de lane, sans
jamais matérialiser les paires prunées. Une borne flottante peut ordonner les
tâches; containment et cardinal exacts restent entiers et fail-open.

Cette ordonnance n'a pas encore une preuve de travail `O(n)` : l'espérance de
la **sortie du front** ne suffit pas à borner les visites de produits AABB. Elle
donne néanmoins un producteur précis à falsifier, plus simple que le
center-cover de tout le disque et directement compatible avec un LBVH
résident. Les compteurs de produits visités et de range-count sont donc une
partie obligatoire de `W_front`.

### Contre-mesure du producteur et de l'extension

Le probe d'ancres déjà présent réfute son parcours actuel comme producteur : de
`n=500` à `n=1 000`, les visites q3 passent d'environ `4,85` à `23,84`
millions et les visites q4 de `5,06` à `25,44` millions, soit des pentes
respectives proches de `2,30` et `2,33`. Entre `97 %` et `99 %` des paires
atteignent encore les terminaux. Ce rouge ne réfute pas le théorème du front;
il réfute le dual-tree courant qui tente de le produire.

L'extension doit rester une porte séparée. Même sous Poisson bulk, étendre
naïvement chaque ancre q3 par tous les tiers de la lentille compatibles donne
environ `3 192,8 n` occurrences, soit `159,6` millions à 50 000 points. Une
borne volontairement lâche qui apparie tous les carriers q4 de cette lentille
atteint environ `11,64` milliards de paires avant les tests de diamètre et de
positivité. Ces nombres ne sont pas des prédictions du producteur retenu; ils
interdisent seulement de déduire `W_extend` de la taille linéaire du front.

### Théorème 4 — l'extension est une enveloppe top-9 affine

Fixer une ancre distincte `a,b`. Poser `d=b-a`, `D2=d dot d`,
`U_z=2z-a-b`, `g_z=D2-U_z dot U_z` et, pour un centre `c` du plan médiateur,
`w=2c-a-b`. On a `w dot d=0` et la marge entière :

$$F_z(w)=g_z+2U_z\mathbin{\cdot}w=4\left(R(c)^2-\left\Vert z-c\right\Vert^2\right).$$

Le signe de `F_z` est donc exactement intérieur, shell ou extérieur. Surtout,
l'ordre décroissant des fonctions `F_z` est exactement l'ordre croissant des
distances à `c`; le terme `4R(c)^2` est commun à tous les sites. Les domaines
de Jung s'écrivent sans racine :

$$w\mathbin{\cdot}d=0,\qquad \left\Vert w\right\Vert^2\leq\frac{D^2}{3}\quad(q3),\qquad \left\Vert w\right\Vert^2\leq\frac{D^2}{2}\quad(q4).$$

Si `abx` est un support q3 pertinent ancré par une arête maximale, `F_x=0`
au centre et au plus huit autres sites ont `F>0`. Le carrier `x` appartient
donc aux neuf plus proches sites hors `a,b`, égalités incluses. Si `abxy` est
un q4 pertinent, `F_x=F_y=0` et au plus sept sites ont `F>0`; **les deux**
carriers appartiennent encore au top-9. Ainsi :

- q3 se trouve en évaluant le centre intrinsèque des seules lignes qui
  apparaissent dans les neuf niveaux supérieurs;
- q4 se trouve aux intersections des lignes de ces neuf niveaux;
- aucune paire de lignes située sous le neuvième niveau ne peut être un
  support pertinent.

Opérationnellement, séparer `c` sites `always_inside`, dont `F_z>0` sur tout
le disque, des lignes qui le coupent. Le préfixe utile parmi ces dernières est
top-`(9-c)`, égalités incluses; les profondeurs maximales sont `8-c` en q3 et
`7-c` en q4. Un top-8 fixe est incomplet lorsque `c=0,p=8` ou `c=0,p=7`.

C'est une réduction exacte, pas une heuristique de voisinage fixe autour des
extrémités. Les neuf voisins dépendent du centre mobile dans le disque. En
position générale, les premiers `k` niveaux d'un arrangement de `m` lignes ont
une complexité cumulative `O(km)`. Ici, le nombre de sommets q4 shallow est au
plus `m(8-c)`, au lieu de `C(m,2)`. Sous u16, une concurrence est groupée
par centre rationnel et toutes ses égalités sont conservées; si le plateau ne
tient pas le contrat de dégénérescence, la branche rend le statut explicite au
lieu de choisir deux lignes.

Une écriture q4 sans normalisation est directement device-friendly. Pour deux
carriers `x,y`, poser :

$$\delta=d\mathbin{\cdot}(U_x\mathbin{\times}U_y),\qquad W=-g_x(U_y\mathbin{\times}d)-g_y(d\mathbin{\times}U_x).$$

Lorsque `delta` est orienté positif et non nul, `w=W/(2 delta)` et le signe de
puissance de tout `z` est celui de `g_z delta+U_z dot W`. Coordonnées u16,
produits en `i128` et repli large suffisent sans base orthonormée, division ni
racine; positivité, diamètre, owner et égalités restent des décisions séparées.
L'orientation porte sur le couple `(delta,W)` : si `delta<0`, multiplier les
deux par `-1` ou échanger `x,y` avant leur calcul. Remplacer seulement `delta`
par sa valeur absolue inverserait le census.

### Théorème 5 — un q4 est un sweep 1D depuis une face positive

Le même arrangement possède une factorisation plus régulière. Fixer une face
positive `T=abx`, son circumcentre exact `c_T`, son rayon carré `R_T2` et la
normale entière `N=(b-a) cross (x-a)`. Tous les centres de sphères passant par
`T` s'écrivent `c(lambda)=c_T+lambda N`. Pour tout site `z`, poser :

$$H_z=\left\Vert z-c_T\right\Vert^2-R_T^2,\qquad J_z=N\mathbin{\cdot}(z-c_T)=N\mathbin{\cdot}(z-a).$$

Sa puissance signée le long du pinceau est exactement
`H_z-2 lambda J_z`. Si `J_z!=0`, le site traverse le shell une seule fois au
rationnel `lambda_z=H_z/(2J_z)`. Si `J_z=0`, il est toujours intérieur,
extérieur ou shell selon le signe de `H_z`. Un q4 positif avec partenaire `y`
exige notamment `H_y>0`; les trois autres barycentriques et les trois tests de
distance à `a,b,x` se décident après élimination des dénominateurs.

Le filtre `H>0` vaut seulement pour choisir un **apex**. Les sites `H<0`
restent dans le sweep car ils contribuent au census. Le cas `J=0,H<0` est
toujours intérieur; `J=0,H>0` toujours extérieur; `J=H=0` est un membre de
shell persistant qui appartient à chaque lot et au rang fermé, jamais un site
à ignorer.

Le sweep exact trie les `lambda_z` par produits croisés. En `lambda` croissant,
`J<0` sort et `J>0` entre. À un lot égal, il retire d'abord tous les sortants,
mesure la profondeur **stricte** sans aucun membre du co-shell, traite le lot
atomiquement, puis ajoute les entrants. Il ne séquentialise jamais deux
contacts égaux. L'intervalle de Jung est fermé et garde les deux signes de
`lambda`. Les
plateaux hors contrat sont groupés par `GeometricBallKey` puis routés vers le
quotient reçu ou vers `unsupported_degeneracy`.

Cette factorisation est complète pour q4 : tout tétraèdre positif et toute
arête maximale `ab` possèdent au moins une des deux faces `abx,aby` positive.
Dans le plan médiateur, écrire les projections des carriers `u,v`, leurs
offsets par `2u dot eta=h_x`, `2v dot eta=h_y`, puis le centre
`eta=alpha u+beta v` avec `alpha,beta>0`. Alors
`2||eta||^2=alpha h_x+beta h_y`; le membre gauche est strictement positif, donc
`h_x>0` ou `h_y>0`. En effet, `eta=0` placerait le centre au milieu de l'arête
`ab`, donc sur la frontière du tétraèdre et non dans son intérieur relatif.
L'owner choisit d'abord l'arête maximale canonique, puis la
plus petite face positive **adjacente à cette arête** si les deux conviennent.
Il doit énumérer les faces positives
**géométriques**, même lorsqu'elles sont q3 hors fenêtre : le rang d'une face
n'est pas héréditaire vers le tétraèdre.

Le sweep 1D rend la décision q4 très compatible avec un radix segmenté GPU,
mais ne résout pas seul la découverte. Le lancer pour chaque face en rescannant
tous les sites réintroduirait un coût quadratique. Il doit consommer les lignes
et conflits déjà fournis par la cutting top-9; il constitue le terminal exact
de cette cutting, pas un producteur indépendant. Sous PPP bulk, l'énumération
préalable de toutes ces faces-graines atteindrait encore environ `4 079,61 n`,
soit `203,98` millions à 50 000 points.

La fixture u16 translatée
`a=(7,10,10), b=(13,10,10), x=(8,8,10), y=(8,12,8)` tue le mutant « les deux
faces doivent être positives » : le q4 est strictement positif et `ab` est son
unique arête maximale, mais `abx` est obtuse tandis que `aby` est positive.

### Deux top-k distincts : génération mobile et sentinelle terminale

Le top-9 ci-dessus ne remplace pas par simple identité la sentinelle terminale
du contre-audit `407d4d1`. Ce sont deux certificats différents :

- pendant la génération q3/q4, le centre varie dans le disque médiateur et les
  neuf voisins sont pris dans `X minus {a,b}`; conserver tous les ex aequo du
  neuvième niveau donne l'enveloppe mobile où chercher les carriers;
- après validation d'un support `U` d'arité `q`, le centre et le rayon sont
  fixés. Les `12-q` vrais plus proches `PointId` de `X minus U` forment alors
  la sentinelle de profondeur fixe pour `smax=11` dans l'interface précise
  « identifiants retournés plus distance maximale, sans information sur les
  omis ».

Pour cette seconde primitive, si moins de `12-q` identifiants restent, il faut
scanner tout `X minus U` sans définir de distance d'ordre : `(I,E)` est complet
et `p+|E|<=|X|<=11`. Sinon, avec `delta` la distance maximale retournée :
`delta<beta` prouve `p+q>=12`; `delta>beta` rend l'intérieur et l'extra-shell
complets; `delta=beta` rend tous les intérieurs et au moins un contact hors
support, avec `p+q<=11`, mais des contacts égaux omis peuvent encore exister.
L'égalité prouve donc seulement la pertinence du support selon son arité; elle
ne décide ni le shell complet ni `p+|E|<=11` et route vers le range-report,
le quotient de plateau ou un refus fermé. Le top-12 global reste sûr, mais il
n'est pas minimal dans cette interface une fois `U` connu. Un range-count capé
ou le seuil certifié du premier omis est une autre interface et peut décider
avec moins d'identifiants matérialisés.

Après RLE par `GeometricBallKey`, une requête mutualisée choisit un support
canonique propre positif d'arité minimale `q_min`, exclut seulement ce support
et conserve tous les autres `SupportKey` du run. Ce `q_min` doit être certifié
par un producteur complet de tous les supports de la boule ou par un certificat
indépendant; le minimum seulement observé dans un producteur incomplet ne peut
rejeter le run. Employer `q_max` ou exclure leur union peut masquer
respectivement un support pertinent ou une extra-shell. Même après le census,
`p+q<=11` se décide par support alors que `p+|E|<=11` se décide par boule.
Inversement,
si l'enveloppe mobile fournit déjà un certificat complet de toutes les marges
strictement positives et nulles, la sentinelle devient une porte différentielle
ou un fallback; cette complétude doit être comparée support par support à
l'oracle borné. Le ledger ferme
`envelope_certified + knn_fallback + plateau = supports`; la branche certifiée
effectue zéro requête kNN et publie la même identité `(SupportKey,I,E)` que la
sentinelle et l'oracle.

### Ne pas matérialiser les lignes

Le théorème local ne justifie pas un CSR `ancre x ligne`. Le volume normalisé
des sites dont la droite coupe le disque vaut :

$$V_{\mathrm{cross},3}=\frac{\pi^2}{3\sqrt{3}},\qquad V_{\mathrm{cross},4}=\frac{3\pi^2}{8\sqrt{2}}.$$

Sous la mesure de Palm du front Poisson, cela donne en moyenne environ `62,37`
lignes q3 et `97,75` lignes q4 par ancre de leur lane. En coalesçant d'abord les
lanes, un CSR crossing minimal paierait encore environ `13 831,22 n`, soit
`691,56` millions d'occurrences à 50 000 points; ce n'est pas une borne
informationnelle pour un oracle implicite. Écrire ces lignes avant le shallow
annulerait une grande part du gain.

Le layout proposé est donc une **enveloppe top-9 implicite**. Pour un patch `K`
du disque, choisir un représentant `c0`, un rayon extérieur `delta_K` et obtenir
exactement le neuvième voisin de `c0` dans `X minus {a,b}`, à distance `r9`.
Les deux extrémités de l'ancre ne consomment aucun slot et tous les sites à la
distance `r9` restent actifs. Tout site susceptible d'être
top-9 quelque part dans `K` appartient à `B(c0,r9+2 delta_K)`. Un parcours
LBVH élimine donc les nœuds extérieurs sans énumérer le nuage. Pour le résidu,
calculer les extrema exacts `L_z,U_z` de `F_z` sur `K`; si `theta` est la
neuvième plus grande valeur `L_z`, la condition stricte `U_z<theta` retire
sûrement `z`. Une égalité reste active.

Une shallow cutting 2D randomisée de Las Vegas, ou son prototype par patches,
emploie ensuite `count/scan/fill` sur ces conflits. Les q3 sont publiés dans le
patch half-open de leur centre intrinsèque; les q4 dans celui de leur
intersection. Un patch encore lourd se divise. Un terminal ne teste toutes les
paires de conflits que sous un préflight d'octets et de travail; sinon il
continue, appelle un fallback exact ou retourne `resource_exhausted`.

La baseline de sortie rend cette piste crédible sans la recevoir. En bulk PPP,
les q3 positifs `p<=8` valent environ `218,274787 n` et les q4 positifs `p<=7`
environ `222,066099 n`. Rapportés au front coalescé, cela fait seulement
`3,118929` vrais supports q3/q4 par `PairId+mask` en moyenne de Palm, soit
environ `22,02` millions de supports à 50 000 points. Cette moyenne ne borne
aucune ancre : les points
`a=(-1,0,0)`, `b=(1,0,0)`,
`x_theta=(0,sqrt(3) cos(theta),sqrt(3) sin(theta))` donnent arbitrairement
beaucoup de triangles équilatéraux vides partageant `ab`. La distribution
p95/p99/max et la file lourde sont donc contractuelles.

Le verrou mathématique restant est maintenant précis : produire les premiers
neuf niveaux sur tous les disques sans payer ni la matrice des paires ancres,
ni le CSR des lignes, et borner le travail des listes de conflits. Sur
`eight_clusters`, des paires inter-amas peuvent avoir un spindle vide et rendre
le simple front quadratique. Le center-cover collectif par patches doit les
fermer **avant émission de PairId**; le théorème Poisson ne les excuse pas.
Cette famille est donc la première falsification, pas un test différé.

### Conséquence d'implémentation GPU

Le producteur recommandé n'énumère jamais toutes les paires. Il réalise un
self-join dual-tree/LBVH par blocs de paires et de témoins :

1. chaque bloc de paires reçoit une borne collective Jung--Helly ou des
   témoins universels disjoints; neuf/huit témoins ferment le bloc;
2. une microtuile survivante émet la paire canonique et ses masques `q3/q4`;
3. chaque ancre alimente l'enveloppe top-9 implicite du plan médiateur; q3 lit
   ses lignes intrinsèques et q4 ses sommets shallow, sans CSR complet ni
   dépendance envers l'admission q3;
4. l'arête maximale canonique et le patch half-open owner émettent chaque
   `SupportKey` propre une fois; le RLE avant lift vérifie cette identité;
5. le census exact de l'enveloppe alimente directement la branche certifiée;
   top-`(12-q)` hors support et census pool-relatif restent oracles/fallbacks;
6. les `BallRecord` sont transformés en événements H0 compacts avant toute
   copie hôte.

L'owner génératif de l'étape 4 est exact sous complétude du front : q3 choisit
la plus petite `PairId` parmi ses arêtes de longueur maximale; q4 applique la
même règle à ses six arêtes et traite les carriers comme un ensemble non
ordonné. Le centre appartient ensuite à un unique patch half-open. Chaque
support propre possède donc une seule ancre et un seul patch owner, soit
`occurrences=SupportKey_unique` avant plateaux. Sans ce tie-break, les plafonds
sont trois émissions q3 et six émissions q4. Un oracle borné, les permutations
de `PointId`, les centres sur frontières et les mutants « pas de tie-break » et
« arête minimale » doivent recevoir cette propriété.

À 50 000 points, quatre `DensePointIndex:u16` distincts tiennent exactement
dans une clé de 64 bits. Une bijection immuable liée à `cloud_epoch` les relie
aux `PointId` durables arbitraires; l'ordre de sortie est défini après remap,
sauf preuve que l'affectation dense le préserve. Une empreinte 64 bits n'est
jamais une autorité de sphère; l'égalité `SupportKey` est exacte parce que les
quatre indices locaux sont réellement encodés et liés à cette bijection.

### Les deux obligations qui restent

Le théorème borne l'espérance du **front**, pas le travail nécessaire pour le
produire ni le nombre d'extensions par ancre. Deux portes décident la voie :

- `W_front` : visites de produits de nœuds, crédits de témoins, microtuiles et
  paires émises doivent avoir deux pentes au plus `1,35` sur `uniform` et
  `eight_clusters` à `12 500/25 000/50 000`;
- `W_extend` : patches, visites LBVH, occurrences de conflits, somme des
  `C(m_leaf,2)`, tests de profondeur exacts, clés uniques et supports acceptés
  doivent rester linéaires en sortie observée. Un ratio rouge suspend
  l'extension, même si le front d'ancres est vert.

Le résultat espéré suggère fortement une wavefront GPU, mais ne justifie pas un
port littéral du DFS CPU. Employer des tâches SoA, `count/scan/fill`, arènes
préallouées, files bucketées par taille, bitsets warp seulement sous cap reçu et
CSR forward au-delà. Tout terminal stalled au-dessus du budget exact se divise,
appelle un producteur alternatif exact ou rend `resource_exhausted`; il
n'alloue jamais une matrice dense ou `C(m,4)` sans préflight.

## 6. Budget physique et cible réaliste

Google documente `g4-standard-48` comme une instance à une RTX PRO 6000 et
[96 Go de mémoire GPU](https://docs.cloud.google.com/compute/docs/accelerator-optimized-machines).
La [fiche NVIDIA](https://www.nvidia.com/en-us/data-center/rtx-pro-6000-blackwell-server-edition/)
annonce 1 597 Go/s. La capacité n'est pas le verrou pour environ 24 millions
de supports; le nombre de passes larges l'est.

Un modèle de trafic volontairement simple, dont les multiplicités de bras,
visites et rondes ne sont pas encore reçues, donne environ :

- `24,6 Go` pour un radix 128 bits de 24 millions de records de 32 octets;
- `20,8 Go` pour 81 millions de tokens de bras de 16 octets triés sur 64 bits;
- `24,6 Go` pour trente-deux visites LBVH de 32 octets par support;
- `35 Go` pour vingt-sept scans Boruvka de 81 millions de bras de 16 octets.

Ce total d'environ `105 Go` est un plancher de modèle : il exclut précisément
le producteur du front, son extension, les prédicats exacts et une partie des
écritures. Les 66 ms obtenues en divisant par le pic théorique ne qualifient
donc aucun temps end-to-end; elles montrent seulement que la bande passante
n'interdit pas à elle seule la seconde. Un tri global d'une
`GeometricBallKey` de cinq `i128` ajouterait un trafic important : router par
fingerprint compact, puis comparer exactement dans chaque bucket ou feuille
owner. Le hash ne décide jamais l'égalité scientifique.

Le verdict honnête est donc : la seconde secondaire est physiquement plausible
si le front et l'extension ferment leurs pentes; les 100 ms principaux ne le
sont pas encore sous ce modèle. Aucun chiffre CPU divisé par un facteur
arbitraire ne remplace un profil device.

## 7. Contrat : ce qu'une route H0 directe a le droit de viser

Le résultat public complet de la spécification exige encore
`critical_catalog`, `gamma_cofaces`, `coverage_log` et les verticales. Le
`BenchmarkOutputContract-v1` chronométré exige dix forêts, verticales, lots et
certificat minimal; il ne chronomètre pas l'expansion complète du catalogue de
replay.

Une route par événements H0/MSF peut viser le payload chronométré uniquement
après migration explicite de sa `proof_basis` et preuve des verticales. Elle ne
peut pas sérialiser une forêt d'événements sous les champs Gamma existants et
revendiquer l'exactitude publique. Avant cette migration, le front de Jung et
le fold direct restent un diagnostic horizontal nommé séparément.

Un Voronoi/Delaunay local d'ordre supérieur répété dans les feuilles n'est pas
le remède : il reconstruit la structure interdite sous un autre nom. De même,
compléter tout support dans un tétraèdre `p`-hefty est un oracle exact sous GP,
mais la famille u16 à deux droites possède plus de 28 milliards de tels
tétraèdres shallow à 50 000 points alors qu'elle n'a aucun support positif
q3/q4. La complétion est donc un falsificateur borné, jamais le producteur.

## 8. Décision proposée

Ordre de travail recommandé à Claude :

1. graver le théorème `R_q<Lambda_D`, y compris la direction orthogonale et les
   égalités, puis comparer sa version faible actuelle à la projection contrainte;
2. retirer tout claim asymptotique des deux sécantes et ne pas appeler l'ancien
   temps une borne supérieure du successeur;
3. conserver le RLE `SupportKey` avant lift et top-`(12-q)` hors support comme
   pipeline device de référence et oracle différentiel;
4. construire un **ledger mass-only du front de Jung coalescé**, sans lift ni
   extension, et comparer sa baseline bulk à `141,183365 n`; lancer
   `eight_clusters` avant toute optimisation Poisson, car le dual-tree actuel
   est déjà rouge et ne sert que de baseline réfutée;
5. si ce front ferme `W_front`, prototyper l'enveloppe top-9 sans CSR de lignes,
   avec q3 direct et sweep q4 1D; mesurer `patches`, conflits, queues lourdes et
   profondeur exacte dans `W_extend`; sinon abandonner la source par ancres;
6. seulement après ces deux portes, implémenter le pipeline plat GPU et mesurer
   `uniform` plus `eight_clusters` avec le payload officiel;
7. en parallèle, tester sur petits oracles le rapport entre toutes les
   `BallKey` et les seuls événements H0 effectifs. Poursuivre un cut-oracle lazy
   uniquement si cette compression est matériellement grande; le théorème MSF
   seul ne suffit pas.

Cette séquence attaque le verrou mathématique sans attendre un théorème externe
qui n'existe pas encore et sans confondre compression post-découverte et source
sparse.

GCP non utilisé.
