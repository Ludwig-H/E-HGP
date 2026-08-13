# Audit du worktree — crédits coniques cellulaires

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Pin, portée et verdict

Le parent observé est
`HEAD=d3329fea4b595b7bbd283e509b0fa1955fcc3b06`. Le header initial non suivi
`prototype/cell_credits.hpp` porte le SHA-256
`5309870d8c22ef245daf0660ae0520c5690fafa49ce5610a486235bfa48cd948`.
Il n'était d'abord inclus par aucune cible. Le probe et son raccord CMake sont
apparus ensuite dans le même worktree ; leur pin et leur rejeu sont séparés
ci-dessous.

Le pin logiciel audité est `01a3a3f26f5f0e7bc3c8f23fdd1a6917e1ca543b` ;
le successeur documentaire `HEAD=88eb36d20b84da76248e7588badc997fc561f42c`
ne change pas ses octets. Il porte
`cell_credits.hpp=69b02684...`, `cell_credits_probe.cpp=a03f8661...` et
`CMakeLists.txt=464d8049...`. Il commet l'enveloppe, le falsificateur et onze
portes. L'ELF Release `c8ea233a...`, Build ID `c7f573b...`, rend son selftest
aléatoire vert ; cela ne mord pas les fixtures exactes ci-dessous.

Le rejeu propre des portes `selftest|falsificateur|mutant_ids_partages` rend
`3/3` en `0,10/33,51/18,31 s`. Le falsificateur publie `3 400` sphères,
`min_interiors=29`, zéro désaccord ; le partage d'IDs meurt code `4`. Ces trois
verts ne testent ni les duplicats projectifs, ni le segment `h==2`, ni le seuil
nominal `smax`.

Verdict : **le théorème, les trois rayons et l'événement d'activation sont une
base mathématique recevable ; le hull committé produit encore faux positifs et
faux négatifs, et les directions projectives dupliquées causent un faux prune
q4 nominal.** `smax` reste figé, le falsificateur partage la faute et le probe
reboucle sur `n(n-1)`. L'enveloppe reste néanmoins la piste algorithmique la
plus directe pour transformer le complément conique mesuré sur les amas en
suffixes de cibles, sans `C(m,3)` ; cette implémentation n'est pas reçue.

### Addendum positif — Andrew live et recette locale

Le successeur non committé au-dessus de `HEAD=88eb36d` porte maintenant
`cell_credits.hpp=f9d4981d...`, `cell_credits_probe.cpp=a8c4e9ad...` et
`CMakeLists.txt=012c2690...`. Andrew rationnel remplace Jarvis, les directions
projectives égales sont fusionnées, les hulls de dimension inférieure à deux
restent `UNKNOWN`, `need_of(smax,lane)` remplace le seuil figé, et l'API rend
l'union des carriers des trois rayons.

Le build Release/CUDA OFF réussit. L'ELF `c097fc06...`, Build ID `cbe73485...`,
rend `selftest|fixtures_mutants` à `2/2` en `0,84 s` : `37 752/37 752` accords,
`471` couvertures, quatre fixtures, puis trois contradictions avec `t.d=0`,
zéro intérieur, référence `UNKNOWN`, injection `CREDIT` et code attendu `4`.
Un checker indépendant, source `d434c83c...`, ajoute `1 533` cas et obtient
`fp=0`, `fn=0`, `bad_carrier=0`, `bad_id=0`. L'ancien faux prune `h==2` est donc
réparé sur ce domaine borné ; les masses du parent ne sont pas transférées.

La suite complète `^mhgp3v_credits_` rend ensuite `12/12` en `49,81 s`. À
`pool=16/32`, Andrew réduit les tests coniques de
`43 956 521/350 267 629` à `6 608 738/20 365 901`, facteurs `6,65/17,20`.
C'est le premier gain de travail net de cette voie. Les trois campagnes
`n=60` ferment encore zéro relation q2/q3/q4 ; elles ne reçoivent ni le rappel
sur amas plus grands, ni une pente, ni le producteur factorisé.

La primitive locale à stabiliser peut être plus simple et moins coûteuse que le
live. Trier **une fois** les sites actifs par la clé rationnelle `(E/W,F/W)`, et
faire de chaque direction égale une pile `(X,PointId,s)` triée. Andrew porte les
seules piles non vides. Pour chaque rayon, chercher d'abord le rang un sur toutes
les directions, puis un couple opposé de rang deux, et seulement ensuite un
triangle du fan. Un crédit unit simultanément les trois carriers, fixe
`X_G=max X_id`, dépile atomiquement ses IDs distincts, puis reconstruit le hull
seulement lorsqu'une pile devient vide. Le glouton peut perdre du rappel, mais
chaque crédit reste sound et tout échec retourne au résiduel.

Cette ordonnance vise `O(m log m+c u)` pour `m` IDs, `u` directions et `c`
crédits, au lieu du live qui relance un tri par insertion `O(m^2)` à chaque
préfixe et reste donc cubique au pire. Les invariants reçus sont : partition de
tous les IDs actifs en piles, ordre rationnel strict, hull sans sommet répété,
carrier rejoué par rayon, union de trois à neuf IDs, unions de crédits deux à
deux disjointes, `+1` strict, même `h_q=smax+1-q` dans producteur/juge/reçu et
cap atomique. Si `pool<3h_q`, publier `capacity_impossible` et conserver la lane
entièrement résiduelle.

Le reçu local minimal porte `InputDigest`, pins source/ELF, `AnchorPointId`,
`CellId`, `BankKey`, `smax`, lane et `h_q`, puis pour chaque crédit `X_G`, les
trois carriers d'au plus trois IDs et leur union triée. Publier séparément les
rangs **par rayon** et l'histogramme `credit_union_size[3..9]` : le commentaire
« un à trois IDs par crédit » est faux pour l'union cellulaire. Le selftest doit
rejouer que cette union couvre réellement les trois rayons, pas seulement que
ses IDs appartiennent au pool.

Les rangs doivent être versés transactionnellement. Le live incrémente
`rank_counts` après chaque rayon, même si un rayon suivant fait finalement
échouer `cell_covered`. Le pool
`{(3,0,0),(3,1,-1),(3,-1,0)}` compte ainsi un rang un pour `r0`, puis ne produit
aucun crédit de cellule. Les planchers `min-rang-un/deux` ajoutés au CMake ne
reçoivent donc pas encore les carriers **consommés**. Accumuler les trois rangs
localement et ne les publier qu'au retour vrai, ou séparer explicitement
tentatives et crédits acceptés.

La fixture « un seul rayon » appelle actuellement l'appartenance directe de
`r0`, tandis que la boucle principale passe trois copies de `r0` à
`cell_covered`. Factoriser un unique chemin mutant, ou utiliser dans la gate un
pool bidimensionnel qui mord le second chemin, est requis pour que
`mutant_killed` porte sur les octets réellement injectés. Ajouter aussi un
nominal dupliqué `m=4`, les carriers positifs de rang deux/trois et la fixture
`smax=11/12/34` avant commit.

## Théorème admis

Après translation de l'ancre en zéro, soit `d=b-a` et `s_i=z_i-a`. Si
`d=sum_i lambda_i s_i`, avec `lambda_i>=0`, et si chaque membre vérifie
`d dot s_i>||s_i||^2`, alors :

$$\sum_i\lambda_i\left(2c\mathbin{\cdot}s_i-\left\lVert s_i\right\rVert^2\right)=\left\lVert d\right\rVert^2-\sum_i\lambda_i\left\lVert s_i\right\rVert^2>0.$$

Ici `c` est le centre de n'importe quelle sphère passant par `a,b`, donc
`2c dot d=||d||^2`. Au moins un membre du groupe est strictement intérieur à
cette sphère. Le groupe peut avoir une taille quelconque. Des groupes de
`PointId` deux à deux disjoints donnent des intérieurs distincts.

Pour `C=cone(r0,r1,r2)` et `tau(r_j)=T`, toute direction de hauteur `x` est
`d=(x/T)u`, avec `u` dans le triangle des trois rayons. Poser :

$$m_C(s)=\min_{0\leq j<3}r_j\mathbin{\cdot}s.$$

Le minimum de `d dot s` sur la section est exactement
`x m_C(s)/T`. Si `m_C(s)>0`, l'événement cellulaire entier est donc :

$$X_s=\left\lfloor\frac{T\left\lVert s\right\rVert^2}{m_C(s)}\right\rfloor+1.$$

Pour toute hauteur `x>=X_s`, H2 est stricte sur la cellule entière. Le `+1`
est indispensable ; l'égalité reste résiduelle. « Exact » signifie ici seuil
minimal pour le **continuum cellulaire worst-case**, pas classifieur complet de
chaque direction du réseau.

Les largeurs annoncées tiennent en `i64` sous le profil u16 : produits
rayon--différence sous `589 815`, numérateur d'activation sous
`38 653 526 025` et déterminant de trois différences sous
`6*65535^3<2^51`.

## Enveloppe projective à recevoir à ancre feuille

Avec `w=r0+r1+r2`, `m_C(s)>0` implique `w dot s>0`. Couper chaque direction par
`w dot u=1` donne des points projectifs. Pour un pool `G` :

$$C\subseteq\mathrm{cone}(G)\iff\frac{r_j}{w\mathbin{\cdot}r_j}\in\mathrm{conv}\left\lbrace \frac{s}{w\mathbin{\cdot}s}:s\in G\right\rbrace\quad\text{pour }j=0,1,2.$$

Le signe d'une orientation projective est celui de
`det(s_i,s_j,s_k)`, car tous les dénominateurs sont positifs. Une enveloppe 2D
exacte puis un carrier de rang un, deux ou trois pour chacun des trois rayons
fournit un crédit d'au plus neuf IDs. Retirer ces IDs et recommencer est sûr et
incomplet ; l'échec reste `UNKNOWN`.

Les cas dégénérés font partie du contrat, pas d'une perturbation :

- un rayon sur une arête ou un sommet de l'enveloppe accepte des poids nuls ;
- des sites de même direction projective gardent une pile canonique de
  `PointId`, car ils peuvent alimenter plusieurs crédits disjoints ;
- les colinéaires du bord demandent un owner exact et un carrier de rang un ou
  deux ;
- H2 demeure strictement positive pour **chaque** ID, même lorsque
  l'appartenance conique est sur le bord ;
- après chaque retrait, l'enveloppe ou sa structure dynamique doit être mise à
  jour ; une enveloppe initiale ne certifie pas les crédits suivants.

Le header live ne code encore aucun de ces objets. `kOneRayOnly` et
`kShareIds` sont seulement des valeurs d'enum, pas des mutants exécutés.
`cell_rays` exige aussi un domaine `0<=cell<432` qui n'est pas vérifié par
l'API. Une porte différentielle doit confirmer que chaque direction non nulle
appartient au cône des trois rayons rendus par son `cell_of`, y compris les
axes, plans, diagonales et les 48 actions signées.

Un premier `cell_credits_probe.cpp`, SHA-256 observé
`ad3fe1b236dae713a2ac5583bc5c1ace123b3bcb2e443e737b3799209b3cff31`, est
apparu ensuite sans raccord CMake. Il reste `ancre times 432`, trie tous les
sites, énumère `C(m,3)` dans un pool borné, puis reboucle sur toutes les cibles.
Son défaut `pool=16` rend huit crédits q4 pleins structurellement impossibles :
chaque crédit qui contient un cône 3D exige au moins trois directions, donc il
faut au moins `24` IDs disjoints. `smax` est accepté mais les besoins restent
`10/9/8`. Le contrôle ponctuel dit lui-même qu'il ne juge pas le certificat et
les mutants retournent code nul sans différentiel interne. Ce probe peut
mesurer une primitive après réparation du pool et ajout d'un vrai juge, mais ne
reçoit ni l'enveloppe ni l'ordonnance 50 k.

### Successeur raccordé et rejoué

Le successeur rend le pool configurable et raccorde le probe, sans encore
changer le header. Pins observés :

| objet | SHA-256 |
| --- | --- |
| `prototype/cell_credits.hpp` | `5309870d8c22ef245daf0660ae0520c5690fafa49ce5610a486235bfa48cd948` |
| `prototype/cell_credits_probe.cpp` | `ad3fe1b236dae713a2ac5583bc5c1ace123b3bcb2e443e737b3799209b3cff31` |
| `CMakeLists.txt` | `edf046d969244f05da629e015db74ba3325d7981122388bd52479ff63b804b79` |
| ELF Release | `1fa1ba728f416da93ee78264e947705a6dd32e17161d8350fe11d68ee034b337` |

Le Build ID de l'ELF est
`6224ae10e3608f0909c1a0ced39205f128b3275d`. Un snapshot intermédiaire rendait
`7/8` parce que `mhgp3v_credits_rangs` exigeait `10 000` crédits malgré les
`6 985` émis. Le commit final abaisse ce plancher à `5 000`; son rejeu
Release/CUDA OFF rend donc **`8/8`** en `68,78 s`. Ce passage au vert ne change
aucun résultat scientifique. Le compteur publie `68 289` succès de recherche
de carriers de rang deux, pas des crédits distincts, et les trois runs positifs
ferment zéro relation dans chaque lane.

Les ablations exposent surtout le coût du catalogue de carriers :

| pool | tests coniques | crédits émis | `credits_hwm` | fermetures q4 | temps |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 16 | 43 956 521 | 19 932 | 5 | 0 | 6,86 s |
| 32 | 350 267 629 | 29 705 | 10 | 0 | 33,94 s |

Le doublement du pool multiplie ici les tests par `7,97`, signature du mur
cubique `C(m,3)`. La porte `pool=32` exige explicitement
`min_residuel=3540=n(n-1)`, donc zéro fermeture q4 ; aucune porte ne planche
`credits_hwm>=8` ni une fermeture nominale. Le commentaire CMake « un crédit
consomme un à trois PointId » confond le carrier d'un rayon avec le crédit
couvrant trois rayons : un crédit full-dim consomme au moins trois et au plus
neuf IDs.

Aucun des huit CTests n'exécute un mutant ou `--judge-echantillon`; il y a un
selftest, trois mesures et quatre refus/planchers. Le selftest couvre les 432
milieux de cellule, mais sa vérification d'activation bornée porte uniquement
sur la cellule zéro. Le successeur est donc **vert diagnostique mais non reçu**. Ces mesures
confirment que l'enveloppe projective/output-sensitive est un préalable, pas une
optimisation postérieure.

### Delta postérieur : enveloppe projective rouge

Après ce pin, Claude a commencé l'enveloppe projective directement dans le
worktree. Le snapshot du premier défaut porte
`cell_credits.hpp=a43235431f7d7bde0f742b830023850076b57eff76765bb77cd921a2f5bbc1eb`
et `cell_credits_probe.cpp=a03f8661de44174100075694c66f0ccdebe871542b8bc52f08734fcbd856adc6`.
Le CMake était encore `edf046d9...`. Aucun résultat du pin `c46d658` ne se
transfère à ces octets ; les fixtures ci-dessous ont été repincées sur le
successeur committé.

Le successeur courant
`cell_credits.hpp=69b02684ee733e11b3063a86635610582e9c576a9884d14e525729cdf5784954`
ajoute des carriers rang un et restitue deux IDs lorsqu'un poids de Cramer est
nul. Il ne canonicalise toujours pas les directions projectives dupliquées et
la branche dégénérée `h==2` reste inconditionnelle. Un exhaustif borné de
`44 676` pools et `134 028` requêtes trouve encore `51` faux positifs, `294`
faux négatifs, `513` hulls avec duplicats et `51` carriers faux. Le statut P0
persiste, même si son selftest aléatoire courant peut passer.

Il existe en outre une **fausse inclusion déterministe de rang deux**,
indépendante de ce désaccord aléatoire. Dans la cellule `U00`, prendre
`r0=(3,0,0)`, `r1=(3,1,0)`, `r2=(3,1,1)` et le pool actif
`G={(3,1,0),(3,2,0)}`. Les deux marges cellulaires valent `9` et les trois
vecteurs `G0,G1,r0` sont coplanaires. Le chemin live de `ray_in_hull`, lorsque
`h=2`, accepte pourtant `r0` après les seuls déterminants nuls. Or l'unique
écriture dans le plan est `r0=2 G0-G1` : `r0` n'appartient pas au cône positif
de `G`.

La réparation mathématique du rang deux est petite et exacte. Pour
`n=G0 cross G1` non nul, si `r=alpha G0+beta G1`, alors
`det(n,G0,r)=beta ||n||^2` et `det(n,r,G1)=alpha ||n||^2`. Il faut donc exiger
les deux signes faibles cohérents, et pas seulement la coplanarité. Si `n=0`,
les deux directions projectives sont identiques : elles forment un rang un
avec une pile canonique de `PointId`, pas un segment. Cette fixture doit être
permanente et tuer l'ancienne branche `h==2`.

Pour le générateur de **cellule pleine**, une garde immédiate encore plus
simple est `dimension_du_hull<2 => UNKNOWN` : les trois rayons de la cellule
sont affinement indépendants, donc aucun segment projectif ne peut contenir
leur triangle. Le test de segment reste nécessaire pour l'API réutilisable et
pour extraire les carriers de bord à l'intérieur d'un hull 2D.

Le cas `n=0` possède sa propre contre-fixture minimale dans la même cellule :
`G={(1,0,0),(2,0,0)}` et `r=(3,1,0)`. Les deux membres de `G` sont le même
point projectif et leur cône est le seul rayon des `x` positifs. Le live les
rend pourtant comme une enveloppe de taille deux et accepte `r`. Il faut donc
dédupliquer **la géométrie** avant la marche par `s_i cross s_j=0`, tout en
conservant tous les IDs dans une pile ordonnée : retirer un carrier dépile un
ID, et le sommet géométrique ne disparaît que lorsque sa pile devient vide.

Ce cas atteint une fermeture nominale. Poser `a=(100,100,100)`, les seize sites
`z_u=a+(u,0,0)`, `1<=u<=16`, et `b=a+(18,1,1)`. La branche `h==2` groupe les
directions dupliquées deux par deux, émet huit faux crédits aux seuils
`3,5,7,9,11,13,15,17` et ferme q4 à hauteur `18`. Pourtant l'offset de centre
`t=(-37,0,666)` vérifie `t dot(b-a)=0` et donne les marges intérieures
`-57,-116,-177,-240,-305,-372,-441,-512,-585,-660,-737,-816,-897,-980,-1065,-1152` :
il n'y a aucun intérieur strict. La faute produit donc un faux prune dans le
chemin sain, pas seulement une divergence de primitive ou de mutant.

Le désaccord de cinq membres montre séparément que le tie-break colinéaire de
Jarvis n'est pas encore reçu. Plutôt que multiplier les cas locaux, la baseline
G4 proposée plus bas — tri rationnel canonique puis deux chaînes monotones —
doit servir d'autorité candidate, différentielle contre Carathéodory sur tous
les pools bornés, avec fixtures colinéaires et directions projectives
dupliquées. Jusqu'à zéro désaccord, l'enveloppe reste **P0 rouge** et aucune
mesure de fermeture ou de pente ne lui est attribuable.

Le faux négatif aléatoire possède lui aussi une fixture exacte dans `U00`.
Avec `w=(9,2,1)`, poser `B=(300,0,0)`, `e=(2,-9,0)` et `f=(9,2,-85)`, puis
prendre, dans cet ordre, `B-e`, `B-e-f`, `B+e`, `B-e+f`, `B`. Tous ont
`w dot s=2700` et une marge cellulaire positive ; `B` est colinéaire positif à
`r0=(3,0,0)`, donc l'oracle accepte ce rayon. Le pivot live choisit pourtant
`B-e`, situé au milieu d'une arête projective. La marche produit les indices
`0,1,2,3,1`, ne revient jamais au pivot, puis la garde `count>=m` transforme ce
cycle en faux hull et rejette `r0`. Une garde de capacité ne doit jamais
authentifier un polygone : non-retour au pivot, répétition de sommet ou
orientation incohérente donnent `UNKNOWN`/refus. Le pivot doit être extrême
selon un ordre projectif total, pas seulement minimal sur une coordonnée sans
tie-break transversal.

Le tie-break live emploie en outre `n=a cross i` puis `det(n,a,next)`. C'est un
prédicat de degré quatre dans les coordonnées des sites, que la borne u16
annoncée pour un déterminant de degré trois ne couvre pas. Avec `M=65535`,
`a=(M,M,M)`, `i=(M,-M,-M)` et `next=(M,0,0)`, `next` est strictement entre les
deux autres directions dans la carte. Les deux déterminants mathématiques sont
positifs, de l'ordre de `7,38e19`, tandis que le calcul `i64` live déborde et
rend `-4503496549203964`. Ce n'est donc pas qu'une borne documentaire : le
verdict du tie-break est inversé sous le profil contractuel.

La construction robuste recommandée est Andrew exact. Choisir `e` orthogonal
à `w`, puis `f=w cross e`. Pour chaque site, stocker `W=w dot s>0`, `E=e dot s`
et `F=f dot s`, puis trier les rationnels `(E/W,F/W)` par produits croisés.
L'égalité des deux coordonnées donne une direction projective dupliquée et
conserve une pile `(X,PointId)`. Comme `det(e,f,w)>0`, le signe d'orientation
2D est celui de `det(si,sj,sk)`. Les deux chaînes suppriment les colinéaires
intérieurs ; un unique donne un point, tous colinéaires deux extrémités, sinon
un cycle strictement convexe.

Le test de segment reste lui aussi de degré deux. Pour `g=e` puis, si besoin,
`g=f`, poser
`Delta_g(u,v)=(g dot v)(w dot u)-(g dot u)(w dot v)`. Une fois la coplanarité
établie, `r` est dans le segment projectif `[a,b]` lorsque `Delta(a,r)` et
`Delta(r,b)` ont le même signe faible que `Delta(a,b)`. Si ni `e` ni `f` ne
sépare `a,b`, ils sont la même direction et relèvent du rang un. Les produits
restent sous environ `5,7e13` en `i64`.

Trois dettes orthogonales restent ouvertes. `kNeed={10,9,8}` demeure constant
alors que la CLI accepte `4<=smax<=34` : l'autorité doit employer
`h=smax+1-q` dans le sujet **et** le juge, ou refuser tout `smax!=11`.
Le cap prend ensuite les sites les plus proches avant de comparer leurs
activations. Distance et activation n'ont pas le même ordre : dans `U00`,
`s=(1,-2,0)` a `||s||^2=5`, `m_C(s)=1`, `X_s=16`, tandis que
`s'=(3,0,0)` a `||s'||^2=9`, `m_C(s')=9`, `X_s'=4`. Cette sélection reste
fail-open, mais elle doit publier la troncature et ne peut être appelée
top-activation exact. Enfin, le nouveau falsificateur ne sonde que deux axes du
plan et neuf magnitudes ; il est utile pour tuer une fixture, pas pour prouver
le quantificateur sur toutes les sphères ni rejouer les `CreditKey` et leurs
IDs. Le CMake committé exécute désormais le seul mutant `credit-ids-partages` ;
oubli du `+1`, rayon unique et positivité ignorée restent déclarés mais sans
fixture/porte tueuse.

Une fixture nominale sépare aussi `smax` de tout mutant. Dans `U00`, avec
`r0=(3,0,0)`, `r1=(3,1,0)`, `r2=(3,1,1)`, prendre les vingt-quatre sites
`lambda r_j`, `1<=lambda<=8`, et la cible `d=(100,20,10)`. Carathéodory et le
hull exact donnent huit crédits aux seuils `4,8,12,15,19,23,26,30`. Ils ferment
justement pour `smax=11`, mais restent inconnus pour `smax=12` (`h=9`) et
`smax=34` (`h=31`) ; le sujet figé à huit fermerait les trois. Sur dix-sept
sphères admissibles, le minimum d'intérieurs vaut huit : le juge figé annonce
zéro défaut, tandis que le vrai seuil trouve sept défauts à `smax=12` et dix-sept
à `smax=34`.

Une ordonnance simple évite aussi de reconstruire Jarvis à chaque préfixe
d'activation. La baseline sûre prend tous les sites positifs du pool, trie une
fois leurs coordonnées projectives rationnelles par produits croisés, construit
les deux chaînes monotones après filtrage des IDs déjà pris, extrait les trois
carriers, fixe `X_G=max_{s in G} X_s`, retire leurs IDs et répète. Elle est
incomplète et peut choisir un seuil plus haut, mais chaque crédit reste exact ;
son coût est un tri puis `h` scans linéaires. Un tier de rappel cherche, pour
chaque crédit, le premier seuil parmi les activations restantes par recherche
binaire et reconstruit le hull : coût `O(h M log M)` après le tri, toujours sans
triples. Cette séparation permet de mesurer explicitement le prix du seuil
minimal au lieu de le cacher dans une reconstruction par préfixe.

La baseline de sûreté la plus courte n'a même pas besoin de trianguler des
carriers : si le triangle des trois rayons est inclus dans le hull, prendre un
ID canonique de **chaque sommet géométrique du bord** donne directement un
groupe dont le hull contient la cellule. Retirer un ID par sommet et
reconstruire produit des couches disjointes exactes ; les piles de directions
dupliquées permettent au même sommet géométrique de survivre à la couche
suivante. Cette variante consomme plus d'IDs et peut relever `X_G`, mais elle
constitue une référence simple. Le carrier d'au plus neuf IDs vient ensuite
comme optimisation de rappel, différentielle contre ces couches et contre
l'oracle Carathéodory.

Le delta `cell_credits.hpp=69b02684...` ajoute correctement la suppression
d'un poids de Cramer nul : lorsque le hull est valide, le carrier rang deux
ainsi rendu est sûr et économise un ID. Il ne corrige toutefois ni le segment
`h=2`, ni les duplicats, ni le pivot cyclique, ni l'overflow de degré quatre.
Son scan rang un ne parcourt en outre que les **sommets du hull** ; une direction
présente à l'intérieur, comme `B` dans la fixture à cinq membres, reste invisible.
Pour récupérer réellement les carriers rang un, scanner tout `avail` ou tenir
trois tables de directions des rayons avant le hull. Cette dette affecte le
rappel/packing, pas la sûreté une fois le hull corrigé.

## Extension nouvelle : crédit commun à un bloc d'ancres

Rester à `AnchorId` feuille risque de recréer une tâche par ancre et, pour les
deux orientations, un join de relations presque pairwise. La géométrie permet
une recertification exacte d'un même groupe absolu `G` sur un bloc d'ancres.

Soit `K=conv(G)` et un rayon non nul `r`. Pour une ancre `a` :

$$r\in\mathrm{cone}(G-a)\iff\text{il existe }t>0\text{ tel que }a+t r\in K.$$

L'ensemble `K-{t r:t>0}` est convexe. Si les huit sommets d'une AABB d'ancres
y appartiennent pour chacun des trois rayons, toute l'AABB y appartient. La
stricte garde `t>0` est nécessaire : avec `t=0`, `G={a}` passerait sans fournir
aucune direction.

La construction complète n'exige même pas de hull 3D. Pour chaque rayon `r_j`,
choisir deux formes entières qui engendrent son plan orthogonal et projeter le
pool en 2D. Sous la garde stricte
`min_G r_j dot z>max_A r_j dot a`, on a :

$$r_j\in\mathrm{cone}(G-a)\iff\pi_j(a)\in\mathrm{conv}\left(\pi_j(G)\right).$$

En effet, l'égalité des projections donne `k-a=t r_j` pour un point
`k in conv(G)`, et la garde impose `t>0`. Pour chacun des `8*3` couples
`(coin a_v,rayon r_j)`, un point-in-hull 2D extrait un carrier canonique d'au
plus trois IDs. Leur union `G` emploie au plus `72` IDs et certifie la cellule
pour toute l'AABB, puisque la projection de la boîte est l'enveloppe de ses
huit coins. Retirer `G` et répéter au plus `h` fois est sûr et fail-open. Les
carriers peuvent différer entre coins ; la convexité est précisément ce qui
permet leur union.

H2 se recertifie sans décorréler les extrema. Pour chaque ID absolu `z`, chaque
coin `a_v` et chaque rayon, poser :

$$D_{v,j}(z)=r_j\mathbin{\cdot}(z-a_v).$$

Exiger tous les `D_{v,j}>0`, puis définir :

$$X_{A,z}=\max_{v,j}\left(\left\lfloor\frac{T\left\lVert z-a_v\right\rVert^2}{D_{v,j}(z)}\right\rfloor+1\right).$$

Cette borne est exacte sur toute la boîte : pour `x` fixé,
`x r_j dot(z-a)-T||z-a||^2` est concave en `a`, donc son minimum est à un coin,
et le minimum directionnel est à un rayon. Le seuil d'un crédit est le maximum
de ses événements.

Pour un nœud cible `B` dont toutes les différences `B-A` sont dans la cellule,
la hauteur minimale à comparer au seuil est
`min_B ell-max_A ell` — et non `min_B ell-min_A ell`. Après
`h=smax+1-q` crédits disjoints reçus, le record factorisé naturel devient
`(AnchorNodeKey,CellId,lane,X,TargetSuffixNodeKeys,CreditKeys)`.

Cette extension maintient les deux orientations au niveau des mêmes rectangles
et évite de joindre après coup des records `a times B` avec `A times b`. Les
boîtes trop larges échouent et se scindent ; aucune ancre représentative ne
sert d'autorité.

La construction à 24 intersections est complète mais peut employer jusqu'à
`72` IDs par crédit, donc `720` à `h=10`. Elle doit rester un tier de secours ou
un oracle de rappel, pas la première baseline G4. Un tier 1 beaucoup plus léger
laisse une ancre canonique **proposer** un carrier plein rang d'au plus trois
IDs pour chaque rayon, soit une union d'au plus neuf IDs, puis recertifie ces
mêmes carriers sur les huit coins. Pour un triple absolu `z1,z2,z3`, le
déterminant `det(z1-a,z2-a,z3-a)` et chacun des trois numérateurs de Cramer sont
affines en `a`. Un déterminant strict de signe constant et des numérateurs
faibles de ce signe aux huit coins certifient donc l'appartenance conique dans
toute l'AABB. Le représentant n'est jamais l'autorité ; l'échec scinde `A` ou
tombe au tier complet.

Pour un nœud cible déjà fixé, on peut fusionner l'activation et le test H2 sans
division. Poser `x0=min_B ell-max_A ell>0`. Pour chaque ID `z` et rayon `r_j`,
la forme suivante est concave en `a` :

$$F_{z,j}(a)=x_0 r_j\mathbin{\cdot}(z-a)-T\left\lVert z-a\right\rVert^2.$$

Si `F>0` aux huit coins, elle est positive dans toute la boîte. Le tier 1 coûte
au plus, pour dix crédits de neuf IDs, `960` déterminants coniques et `2 160`
marges de coins, bien moins qu'un hull 3D à 720 IDs. Mesurer rappel et splits
contre le tier complet avant tout choix device.

Il existe un minorant H2 de boîte encore plus direct, qui doit servir de gate de
référence conservatrice. L'identité exacte est :

$$\left(b-a\right)\mathbin{\cdot}\left(z-a\right)-\left\lVert z-a\right\rVert^2=\left(z-a\right)\mathbin{\cdot}\left(b-z\right).$$

Pour des AABB `A,B` et un ID `z`, le minimum continu se sépare coordonnée par
coordonnée et chacun des quatre couples d'endpoints doit être essayé :

$$L_z(A,B)=\sum_{i=0}^{2}\min_{a_i\in\left\lbrace A_i^-,A_i^+\right\rbrace,\ b_i\in\left\lbrace B_i^-,B_i^+\right\rbrace}\left(z_i-a_i\right)\left(b_i-z_i\right).$$

Ainsi `L_z(A,B)>0` pour chaque membre du crédit est un H2-`ALL` sûr sur tous les
points du rectangle, en douze produits par ID, sans hauteur, racine ni
`PairId`. `L_z` est le minimum exact sur les AABB continues, pas nécessairement
sur les ensembles finis portés par les nœuds : `L_z<=0` reste donc
`MIXED/UNKNOWN`, jamais `NONE`. Le seuil cellulaire `X_G` est le fast path comprimé ;
ce test bilinéaire traite son résiduel avant tout split et évite les ambiguïtés
d'extrema décorrélés.

Une gate positive de raccord tient déjà en quatre paires. Après translation par
`o=(100,100,100)`, prendre `A={o,o+(1,0,0)}`,
`B={o+(100,20,10),o+(110,20,10)}` et, pour `1<=lambda<=8`, les crédits communs
`G_lambda={o+lambda r0,o+lambda r1,o+lambda r2}`. Les huit coins recertifient
les trois rayons et chaque membre vérifie `L_z(A,B)>0`. Un unique `RectKey`
ferme donc les quatre couples dirigés q4 à `smax=11` et reste résiduel à
`smax=12`. Au petit n, son expansion doit donner multiplicité exactement une,
masse quatre et digest invariant sous permutation. Cette fixture est le jalon
constructif entre le solveur leaf et la rampe, sans aucun `PairId` au chemin
produit.

## Ordonnance CPU puis G4 proposée

1. Partir des `RectId` disjoints du dual-tree et certifier la cellule de toutes
   les différences par extrema half-open.
2. Pour chaque `(AnchorNodeKey,CellId)` réellement actif, collecter sous cap un
   pool candidat et construire ses trois hulls 2D projetés et ses événements.
3. Extraire d'abord un crédit proposé de taille au plus neuf et le recertifier
   par formes affines/concaves aux huit coins ; employer le crédit complet de
   taille au plus 72 par les 24 intersections seulement en tier de secours ou
   oracle. Retirer les IDs et répéter au plus `h<=33` fois sur le domaine CLI,
   ou dix fois à `smax=11`.
4. Fermer par seuil des nœuds cibles entiers, puis appliquer `L_z(A,B)>0` sur le
   résiduel du seuil. En cas de cap, dégénérescence non traitée ou crédit
   manquant, conserver le rectangle dans le front ; ne jamais repartir de la
   racine par ancre.
5. Employer les crédits du cœur et de dominance seulement avec un ledger
   commun de `PointId` disjoints. Une première composition sûre les consomme
   séquentiellement et exclut les IDs déjà crédités ; additionner des compteurs
   opaques est interdit.

Sur G4, un CTA peut traiter un pool borné en SoA : événements, tri par seuil,
orientations `i64`, petite enveloppe, Cramer de coins, puis count--scan--fill des
records de suffixe. Comparer au moins deux variantes : enveloppe reconstruite
après chaque crédit et couches convexes qui prennent tout le bord comme crédit
plus gros. La seconde perd du rappel mais simplifie la baseline exacte.

La baseline device évite Jarvis répété. Pour `M<=128`, elle bitonic-trie une
fois les coordonnées projectives rationnelles par produits croisés entiers et
`PointId`, puis reconstruit les deux chaînes monotones en `O(M)` après chaque
retrait. Les directions projectives dupliquées restent des piles d'IDs : les
dédupliquer définitivement détruirait des crédits disjoints ultérieurs. Le coût
visé est `O(M log^2 M+hM)`, pas `O(C(M,3))` ni `O(hMH)`.

Caps initiaux de falsification, à mesurer et non à promouvoir comme contrat :
`RectTask` de 32 octets, deux fronts de `2^22` records soit `256 MiB`, requêtes
dominance shardées par `2^20`, pool groupe `M=128`, au plus 64 vagues. Tout cap
atteint émet un front résiduel authentifié ou refuse atomiquement ; il ne
tronque jamais un crédit ni ne repart silencieusement de la racine.

## Gates et compteurs

- oracle petit pool : toutes les fermetures cellulaires incluses dans le juge
  exhaustif de sphères, puis comparaison du nombre de crédits au packing exact
  seulement comme mesure de rappel ;
- fixtures singleton, paire antipodale, triangle, rayon sur bord, directions
  projectives dupliquées, H2 égale, `h-1/h`, ID réutilisé et groupe qui contient
  le barycentre mais omet un rayon extrême ; la paire projective
  `{(3,1,0),(3,2,0)}` doit notamment refuser le rayon `(3,0,0)` ;
- bloc d'ancres : huit coins dont un seul rate l'ombre `K-cone(r)`, `t=0`, rang
  deux positif, H2 qui échoue seulement à un coin/rayon, égalité `L_z=0`, boîte
  large à scinder ;
- mutants effectifs : oubli du `+1`, un seul rayon, partage d'ID, acceptation de
  `m<=0`, omission d'un coin d'ancre, `min_A` employé au lieu de `max_A` pour la
  hauteur cible ;
- compteurs : pools proposés/actifs/tronqués, événements, tris, hull rebuilds,
  orientations, tailles de bord, carriers rang 1/2/3, intersections de coins,
  crédits, conflits ID, seuils, nœuds cibles `ALL/MIXED`, front résiduel, bytes
  et HWM ;
- conservation par lane : masses fermées des étages séquentiels plus masse du
  front égale à la masse d'entrée, avec `RectId` disjoints et aucune expansion
  de `PairId` hors juge borné ;
- rampes mêmes octets à `12 500/25 000/50 000`, deux exposants au plus `1,35`
  pour chaque compteur physique dominant, puis seulement lowering CUDA.

Le GO de ce header n'est donc pas « il compile » : c'est la démonstration que
les crédits survivent à la recertification par blocs et que le nombre de tâches,
visites, records et octets baisse réellement sur `eight_clusters` sans reporter
le travail dans le front résiduel.

GCP non utilisé.
