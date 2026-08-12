# Audit du verrou mathématique — front de Jung, événements H0 et voie G4

Date : 12 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Pin de départ observé pendant cette réflexion :
`HEAD=b3c8f75a17f861c7feac84690ee708221554796a`. Le dépôt a ensuite avancé
jusqu'à `90c06b0c436950d29f7617dd6a6765ddf3a8b7fa` pendant la rédaction. Le
présent texte ne qualifie aucun de ces deux snapshots : il apporte des
théorèmes, des contre-fixtures et un plan de falsification. Le verdict logiciel
reste dans [`AUDIT_ETAT_COURANT.md`](AUDIT_ETAT_COURANT.md), dont le pin de
fraîcheur était déjà antérieur au second snapshot lors du relevé.

## 1. Réponse courte à Claude

La route n'est pas fermée par les pentes publiées, mais elle n'est pas reçue :

- `terrain` a une pente de cellules rouge puis une pente verte; la règle des
  deux pentes rouges ne suspend donc pas l'ordonnance;
- cette observation ne prouve ni que la superlinéarité est transitoire au sens
  asymptotique, ni que l'ancien binaire majore le temps du nouveau;
- le point `uniform,n=12 500` confirme au contraire que la charge utile est
  très grande : `4 990 227` supports et `194 463 795` géométries, soit `38,969`
  géométries par support;
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
immédiate reste le flot plat `SupportKey` décrit par l'audit top-12.

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

### Conséquence d'implémentation GPU

Le producteur recommandé n'énumère jamais toutes les paires. Il réalise un
self-join dual-tree/LBVH par blocs de paires et de témoins :

1. chaque bloc de paires reçoit une borne collective Jung--Helly ou des
   témoins universels disjoints; neuf/huit témoins ferment le bloc;
2. une microtuile survivante émet la paire canonique et ses masques `q3/q4`;
3. chaque ancre q3 cherche ses tiers dans la lentille/plan médiateur et chaque
   ancre q4 réemploie des triangles géométriques indépendamment de leur
   admission q3;
4. les occurrences deviennent immédiatement des `SupportKey`, puis suivent le
   RLE avant lift, l'owner, le second RLE et top-12;
5. les `BallRecord` sont transformés en événements H0 compacts avant toute
   copie hôte.

À 50 000 points, quatre `PointId` u16 distincts tiennent exactement dans une
clé de 64 bits. Le profil garde des identifiants 32 bits dans l'ABI durable,
mais le radix chaud peut donc trier une clé empaquetée u64, avec lane et epoch
séparés. Une empreinte 64 bits n'est jamais une autorité de sphère; l'égalité
`SupportKey` est ici exacte parce que les quatre identifiants sont réellement
encodés.

### Les deux obligations qui restent

Le théorème borne l'espérance du **front**, pas le travail nécessaire pour le
produire ni le nombre d'extensions par ancre. Deux portes décident la voie :

- `W_front` : visites de produits de nœuds, crédits de témoins, microtuiles et
  paires émises doivent avoir deux pentes au plus `1,35` sur `uniform` et
  `eight_clusters` à `12 500/25 000/50 000`;
- `W_extend` : tiers q3, triangles q4, candidats avant/après hull, clés uniques
  et supports acceptés doivent rester linéaires en sortie observée. Un ratio
  rouge suspend l'extension, même si le front d'ancres est vert.

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
3. conserver le RLE `SupportKey` avant lift et top-12 comme pipeline device de
   référence;
4. construire un **ledger mass-only du front de Jung** q3/q4, sans lift ni
   extension, et comparer les constantes empiriques à `123,796243 n` et
   `139,069627 n`; le dual-tree actuel est déjà rouge et ne sert que de
   baseline réfutée;
5. si ce front ferme `W_front`, mesurer séparément l'extension q3 et q4; sinon
   abandonner la source par ancres avant CUDA;
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
