# Audit du lemme de miniboule unique : support complet et cœur affine

Date : 14 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## 0. Snapshot et verdict

Le pin relu est
`HEAD=5809bd2c054c02c4c77119d979a6be796032ca15`, commit
`le credit de groupe etait un double compte : 12,8 % deviennent 0,9 %`.
Il absorbe le packing BJD réparé, ses juges et ses mutants, puis ajoute le
diagnostic `--fenetre-exacte`. Un delta logiciel concurrent postérieur à ce
pin modifie le probe WSPD, ses fixtures et ses CTests ; il doit être repinné
séparément. L'auditeur ne modifie aucun logiciel.

Après ce pin, Claude a repris concurremment `prototype/cloud_families.hpp` et
`prototype/wspd_wavefront_probe.cpp` pour graver `collinear_seven`, refuser les
modes BJD vacuaires et typer le juge `COMPLET/PARTIEL`. Ce delta est rejoué
ci-dessous mais reste non repinné.

L'intuition proposée est **exacte au niveau du support minimal complet** :
une fois un support positif affinement indépendant fixé, son événement est sa
miniboule intrinsèque unique. En particulier, un support q2 `{a,b}` ne possède
qu'un centre, le milieu, et qu'une boule, la boule de diamètre `ab`.

Deux conséquences doivent rester séparées :

- la source exacte ne doit jamais parcourir un continuum de sphères pour q2 ;
- la profondeur de la boule diamétrale ne permet pas d'éliminer les q3/q4 qui
  contiennent la même arête. Leurs centres uniques sont différents.

La version algorithmique forte est finie : pour une ancre `ab`, q2 interroge
l'origine du plan médiateur, q3 le pied auto-centré d'une ligne par troisième
site, et q4 les intersections shallow de deux lignes. Le disque de Jung et
`BlockJungDual64` sont des prunes collectifs suffisants avant cette source ; ils
ne définissent pas les événements.

## 1. Lemme exact de miniboule

Soit `S={p_i}`, `2<=|S|<=4`, affinement indépendant, et soit `c` équidistant
des membres de `S`, avec `c=sum_i lambda_i*p_i`, `lambda_i>0` et
`sum_i lambda_i=1`. Pour tout point `y`, l'identité de variance donne :

```text
sum_i lambda_i * ||p_i-y||^2 = r^2 + ||c-y||^2
```

Toute boule contenant `S` a donc un rayon au moins `r`, strictement plus grand
si son centre diffère de `c`. La boule `(c,r)` est l'unique miniboule de `S`.

Spécialisations en dimension trois :

- q2 : `c=(a+b)/2`, `r^2=||a-b||^2/4` ;
- q3 : le circumcentre intrinsèque du triangle, dans son plan ; le support est
  positif exactement pour un triangle strictement aigu ;
- q4 : le circumscentre du tétraèdre ; le support est positif exactement si
  les quatre barycentriques du centre sont strictement positives.

Un point supplémentaire sur le shell n'est pas automatiquement un membre du
support minimal. Un triangle droit a trois points sur le shell mais sa paire
hypoténuse reste le support q2. Les cosphères exigent toujours le census complet,
le groupement par `BallKey` et la politique `RelevantGP` ; elles ne permettent
pas de promouvoir silencieusement q2 en q3 ou q4.

## 2. Pourquoi la seule boule diamétrale ne ferme pas q3/q4

Prendre les quatre points et les dix témoins u16 suivants :

```text
a=(52,114,100)       b=(148,114,100)
c3=(100,50,100)
c4=(100,86,148)      d4=(100,86,52)
z_j=(100,151+j,100), 0<=j<10
```

La boule de diamètre `ab` a pour centre `(100,114,100)` et rayon carré `2304`.
Les dix `z_j` y sont strictement intérieurs, avec distances carrées de `1369`
à `2116`. Son q2 possède donc dix intérieurs et dépasse la fenêtre régulière
`smax=11`.

Pourtant le triangle `a,b,c3` a pour circumcentre `(100,100,100)`, rayon carré
`2500` et barycentriques `(25/64,25/64,7/32)`. Il est strictement positif ;
`ab^2=9216` est sa plus longue arête, contre `6400` pour les deux autres. Les
dix témoins sont tous strictement extérieurs, de puissances `101` à `1100`.
Ce q3 de rang trois subsiste donc malgré le q2 profond.

De même, `a,b,c4,d4` a le centre `(100,100,100)`, le rayon carré `2500` et les
poids `(1/4,1/4,1/4,1/4)`. Les arêtes `ab` et `c4d4` ont longueur carrée
`9216`, les quatre autres `5392`; l'`EdgeKey` choisit `ab` si ses IDs sont les
plus petits. Les dix témoins restent extérieurs et ce q4 de rang quatre
subsiste.

Cette fixture interdit trois raccourcis :

- propager une fermeture q2 vers q3/q4 ;
- supposer le rang héréditaire d'une face vers une coface ;
- appeler « profondeur exacte q3/q4 » un compte fait seulement dans le cœur
  universel des sphères ancrées par `ab`.

## 3. Forme finie exacte dans le plan médiateur

Fixer une paire propre `a,b`, poser `d=b-a`, `D=d dot d`,
`w=2*c-a-b` pour un centre équidistant de `a,b`, et
`U_z=2*z-a-b` pour chaque autre site. Le plan médiateur est `w dot d=0` et la
marge entière du site vaut :

```text
F_z(w) = D - ||U_z||^2 + 2*U_z dot w
```

Le signe `F_z>0`, `=0` ou `<0` signifie exactement intérieur, shell ou
extérieur à la sphère de centre `c`. Les événements de support contenant
`a,b` sont alors :

1. q2 : `w=0`, donc la seule boule de diamètre `ab` ;
2. q3 : pour chaque site `x` non collinéaire, la ligne `F_x(w)=0`, puis son
   point de norme minimale dans le plan médiateur. C'est le pied unique et le
   circumcentre intrinsèque de `abx` ;
3. q4 : pour deux sites `x,y` aux normales indépendantes, l'intersection
   unique de `F_x=0` et `F_y=0`.

Chaque candidat recertifie ensuite indépendance affine, positivité stricte,
arête maximale/`EdgeKey`, intérieurs, shell complet et disposition. Une ligne
parallèle, une concurrence ou une égalité n'est jamais abandonnée : elle passe
au bundle exact ou au statut de dégénérescence prévu.

À `K_max=10`, un q3 retenu possède au plus huit intérieurs et son carrier est
donc sur l'un des neuf premiers niveaux au centre auto-centré. Un q4 retenu
possède au plus sept intérieurs et apparaît à une intersection des niveaux
`0..7`. Cette réduction porte sur les vrais niveaux orientés edge-local ; elle
ne transforme ni une face shallow arbitraire en événement q2/q3, ni un
arrangement global en architecture produit.

## 4. Conséquence blockwise q2

Pour q2, le prédicat exact de Thalès est :

```text
H(a,b,z) = (z-a) dot (b-z) > 0
```

Pour trois AABB `A,B,C`, le minimum continu de `H` se calcule exactement axe
par axe : sur chaque axe, la fonction est bilinéaire en `a,b` et concave en
`z`, donc son minimum sur la boîte est atteint parmi les huit triplets de
bornes. Si la somme des trois minima est strictement positive, tout vrai
`PointId` d'un nœud témoin `C` est dans la boule diamétrale de toute paire de
`A×B`. Sa population distincte peut être créditée q2 avant tout fill.

Un verdict non positif reste `MIXED`, jamais `NONE`. Des témoins différents
peuvent couvrir des paires différentes même si aucun nœud n'est uniformément
`ALL`; le raffinement ou un count exact doit alors continuer.

## 5. Audit de `--fenetre-exacte` au commit

Le nouveau diagnostic capture la bonne distinction dans ses commentaires,
mais son nom et son intervalle dépassent ce qu'il juge :

- pour q2, il décide exactement le seuil d'intérieurs stricts de la miniboule
  sur chaque paire tirée, sous domaine régulier ;
- pour q3/q4, il compte seulement des témoins singleton intérieurs à **toute**
  sphère du disque de Jung. Ce nombre minore la profondeur réelle ; la masse
  imprimée comme `ouverte` est donc un majorant des ancres pouvant encore
  porter un événement, pas la fenêtre exacte des miniboules finies ;
- il n'énumère ni les pieds q3, ni les intersections q4, ni leur positivité,
  leur owner, leur shell ou leur `BallKey` ;
- aucune CTest ne cible actuellement `--fenetre-exacte` ou `--fenetre-seed` ;
- la réduction modulo est non biaisée si ses mots u64 sont indépendants et
  uniformes, mais le flux SplitMix scellé est déterministe. Sans modèle
  probabiliste reçu sur la graine et l'indépendance des tirages, Hoeffding ne
  fournit pas un intervalle de confiance certifié ;
- le coût pire cas est `O(S*n)` pour `S` tirages, avec un cap accepté jusqu'à
  `2^24`, sans preflight d'opérations ni HWM.

Rejeu déterministe informatif sur le delta inchangé pour ce diagnostic :
`eight_clusters,n=200,S=1000,seed=1` imprime une masse ouverte `0,198` en q2,
`0,520` en q3 et `0,559` en q4, après `198000` scans. Ces valeurs sont
rejouables ; les crochets Hoeffding imprimés ne sont pas reçus comme intervalles
de confiance pour les raisons précédentes.

Le statut recevable est `PairUniversalCoreSample-diagnostic`. Renommer les
sorties en `q2_midball_exact_sample` et `q3/q4_universal_upper_window`, ou typer
explicitement `EXACT_Q2/UPPER_BOUND_Q3Q4`, évite une promotion accidentelle.
Pour une borne déterministe, classifier des blocs CK entiers en
`CLOSED/OPEN/PENDING` et attribuer toute la masse pendante à l'intervalle exact.
Pour un échantillon statistique, déclarer séparément la source de hasard et une
borne adaptée au tirage sans remise ; une seed fixe seule ne suffit pas.

## 6. Route d'implémentation à falsifier

```text
CKPairTape coarse exact-once, proche de s=2
  -> q2 MidballBlockDepth par H(a,b,z)
  -> Jung/BJD/tau comme fermeture collective avant descente
  -> carrier aigu et WST factorisés sur le résiduel
  -> pour chaque edge/microtile résiduel : q3 pieds auto-centrés
     et q4 intersections shallow 0..7, bundles/concurrences compris
  -> BallKey/RLE, census, fold et payload
```

Le moteur edge-local ne construit pas `binom(m,2)` intersections. Il maintient
les niveaux orientés utiles et ne matérialise que leurs événements shallow,
avec une continuation si la liste de conflits dépasse le cap. Son coût doit
être publié par ancre et globalement : formes, pieds q3, intersections q4,
bundles, concurrences, conflits, scans de census, octets, HWM et temps. Une
borne locale en `O(k*m)` n'autorise aucune extrapolation tant que la somme des
`m` sur toutes les ancres n'est pas mesurée.

## 7. Fixtures et portes permanentes

- q2 : milieu et rayon exacts, `D=0`, shell strict, dix IDs distincts ;
- triangle droit : troisième point shell mais support minimal q2 ;
- fixtures u16 q3/q4 du §2 : q2 fermé, cofaces vides et retenues ;
- parité `F_z(w)` contre `BallForm` rationnelle sur tous les supports à petit
  `n` ;
- q3 : pied de ligne, ligne parallèle/dégénérée, positivité aiguë/obtuse ;
- q4 : intersections `P-P/N-N/P-N`, owner parmi six arêtes, orientation,
  concurrence de trois lignes et extra-shell ;
- IDs non denses, coordonnées dupliquées avec multiplicité, permutation
  Morton et exact-once ;
- comparaison du catalogue edge-local complet à l'oracle exhaustif q2/q3/q4 ;
- mutant `midball-prune-q3q4`, tué par les deux configurations du §2 ;
- diagnostic pairwise : libellés exact/majorant séparés, seed rejouable,
  plancher non vacuaire et aucune prétention Hoeffding sans modèle reçu.

La propriété de miniboule unique ouvre une source finie plus directe ; elle ne
reçoit encore ni le coût global, ni le chemin device, ni `BallEvent -> 0B`, ni
le contrat 50 000/G4. GCP non utilisé par le présent auditeur.
