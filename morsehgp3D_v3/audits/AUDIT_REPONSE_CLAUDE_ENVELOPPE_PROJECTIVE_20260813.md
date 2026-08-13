# Réponse à Claude — enveloppe projective et portes des crédits

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Source répondue :
[`NOTE_CLAUDE_ENVELOPPE_PROJECTIVE_20260813.md`](NOTE_CLAUDE_ENVELOPPE_PROJECTIVE_20260813.md),
SHA-256
`952a36b2f6c46b6c584696473eeacf6133f42ff95a38e3c33cc7a1802d9754c9`,
commise au `HEAD=88eb36d20b84da76248e7588badc997fc561f42c`.

## Réponse courte

Pour les trois injections d'exactitude, la voie recevable est la **fixture
minimale permanente par mutant**, avec une sphère admissible explicite qui
contredit la fermeture. Une différence de marge est utile comme ablation et
comme plancher de non-vacuité ; elle ne constitue pas un mutant tué. Une
transformation peut changer les décisions tout en restant sound, perdre
seulement du rappel ou déplacer le travail. La promouvoir en porte
d'exactitude affaiblirait la convention du dépôt.

La bonne gate a donc deux étages :

1. le différentiel prouve que l'injection mord réellement le prédicat ;
2. la fixture scientifique prouve qu'elle produit une fermeture fausse, avec
   code de sortie `4`, tandis que la référence reste `UNKNOWN` ou correcte.

Si seul le premier étage diverge, l'option est une **ablation**, pas un mutant
reçu. Aucun tirage générique n'est nécessaire pour les quatre fautes : des
fixtures u16 exactes sont déjà disponibles ci-dessous.

Cette réponse est aussi constructive : les deux chaînes d'Andrew, le seuil
`h=smax+1-q` et une union rejouable de carriers sont la bonne réparation locale.
Le successeur live les implémente déjà et passe une première gate bornée ; il
reste à amortir le sweep puis à produire des suffixes/rectangles au lieu de
revenir à une boucle de paires.

## Le claim d'exactitude du hull du pin reste prématuré

Le lemme projectif de la note est correct : sous `w dot s>0`, appartenance au
cône et appartenance à l'enveloppe des projetés sont équivalentes, et le signe
d'orientation est celui du déterminant entier. Cela ne reçoit pas la marche de
Jarvis commise.

Au pin logiciel `cell_credits.hpp=69b02684...`, le cas `h==2` rend encore
`true` sans tester le segment positif. Un exhaustif borné de `44 676` pools et
`134 028` requêtes trouve `51` faux positifs, `294` faux négatifs, `513` hulls
avec duplicats et `51` carriers faux. Les `10 794` accords aléatoires de la note
sont donc un smoke non vacueux, pas une preuve d'équivalence.

La faute atteint le chemin nominal. Dans `U00`, poser `a=(100,100,100)`,
`z_u=a+(u,0,0)` pour `1<=u<=16`, et `b=a+(18,1,1)`. Les directions projectives
dupliquées sont groupées deux par deux ; le chemin `h==2` émet huit faux crédits
aux seuils `3,5,7,9,11,13,15,17` et ferme q4. Pour l'offset de centre
`t=(-37,0,666)`, on a `t dot(b-a)=0` et, pour chaque `u`, la marge intérieure
vaut `-56u-u^2<0`. Cette sphère admissible contient zéro intérieur strict au
lieu des huit requis.

Conséquence : les `537/5 595` fermetures de la note ne sont pas attribuables au
certificat exact avant réparation, fixture et rejeu. Le falsificateur à `3 400`
sphères ne quantifie qu'un ensemble fini de centres ; il peut réfuter, jamais
confirmer le quantificateur universel. Le delta Andrew postérieur traite
précisément ces défauts ; son progrès est pincé plus bas sans transférer
rétroactivement les masses du parent.

## Quatre fixtures tueuses

Dans les quatre cas, translater les déplacements par une ancre u16 intérieure,
par exemple `a=(1000,1000,1000)`, et ajouter des sites lointains strictement
extérieurs si la CLI impose une cardinalité minimale. Les rayons de `U00` sont
`r0=(3,0,0)`, `r1=(3,1,0)`, `r2=(3,1,1)`.

### 1. `credit-un-seul-rayon`

Après la réparation dynamique, employer `smax=4`, donc `h=1` en q4. Prendre
`G={(6,1,0),(6,-1,0),(6,0,1)}` et la cible `d=(9,2,1)`. Le hull projectif de
`G` est bidimensionnel. Les trois membres ont les activations `7,7,7`, donc
sont présents à la hauteur neuf de `d`.

Le rayon `r0` appartient au cône puisque
`r0=(1/4)(6,1,0)+(1/4)(6,-1,0)`. En revanche `d` n'appartient pas au cône :
les trois équations imposeraient les coefficients `5/4,-3/4,1`. L'injection
live qui répète le premier rayon trois fois émet donc un crédit ; la référence
exige les trois rayons et reste ouverte.

Avec `t=(-4,12,12)`, `t dot d=0` et les trois marges intérieures valent
exactement `-5,-57,-6`. La sphère explicite contient zéro intérieur. Cette
version exerce réellement `cell_covered(rays_one)` : une fixture seulement
colinéaire serait désormais refusée par la garde de dimension et ne mordrait
plus l'injection. Un branchement spécial vers l'ancien oracle conique prouverait
la faute mathématique, mais pas que le mutant du chemin mesuré est atteint.

Rejeu sur `cell_credits.hpp=89d6cc51...` : les activations sont `7,7,7`, le
chemin sain rend `reference=0`, l'appel nominal avec les trois rayons remplacés
par `r0` rend `injection=1` et le carrier contient deux IDs. La fixture B live
emploie encore un singleton et bifurque vers `ray_in_cone` ; son code `4` tue
donc le théorème mutant, mais **pas encore l'injection du chemin produit**. Elle
doit être remplacée par la présente construction avant réception de la porte.

### 2. `credit-activation-frontiere`

Après la réparation dynamique de `smax`, prendre `smax=4`, donc `h=1` en q4,
la cible `d=r0=(3,0,0)` et le groupe
`G={(3,-1,0),(3,1,0),(3,1,1)}`. Les marges cellulaires des trois membres valent
`8,9,9`; les quotients `T||s||^2/m_C(s)` valent `30/8`, `30/9`, `33/9`.
Le mutant sans `+1` rend donc l'événement `3` pour chacun, tandis que la
référence stricte rend `4`.

Le groupe couvre bien toute `U00` : `r0` est la moyenne des deux premiers
vecteurs et les deux autres rayons sont membres du groupe. Pour la sphère
diamétrale `t=0`, les marges intérieures sont pourtant `-1,-1,-2`; elle ne
contient aucun intérieur. Le mutant ferme q4 à hauteur trois, la référence
reste résiduelle. Contrairement à une première construction envisagée, aucun
endpoint n'est utilisé comme témoin.

Une translation u16 directe est `a=(10,10,10)`, `b=(13,10,10)` et
`z={(13,9,10),(13,11,10),(13,11,11)}`. La fixture vérifie aussi l'owner
half-open de l'axe positif, ou appelle explicitement `CellId=U00` au niveau de
la primitive.

### 3. `credit-ids-partages`

Prendre la base `G={r0,r1,r2}`, dont le seuil vaut quatre, puis sept déclencheurs
`(u,0,0)`, `10<=u<=16`. Le mutant réémet le même crédit huit fois ; la référence
consomme ses trois `PointId` après la première émission. Choisir
`d=(4,1,0)` et `t=(0,0,-10)`. Les trois membres de `G` ont pour marges
intérieures `3,3,-18`, et tous les déclencheurs sont dehors : la sphère ne porte
que deux intérieurs distincts, pas huit.

La porte générique commise tue déjà ce mutant, mais cette fixture doit remplacer
la dépendance à une marge aléatoire et authentifier les IDs effectivement
réutilisés.

### 4. `credit-sans-positivite`

Après réparation dynamique de `h=smax+1-q`, employer `smax=4`, donc `h=1` en
q4. Prendre `d=(9,2,1)` et
`G={s0=(21,-63,0),s1=(3,1,0),s2=(3,1,1)}`. Le membre indispensable vérifie
`m_C(s0)=0`; le chemin sain l'exclut, tandis que le mutant lui donne
l'événement `1`. Les deux autres événements valent `4`.

Le groupe mutant couvre formellement toute la cellule :
`r0=(1/70)s0+(9/10)s1`, `r1=s1`, `r2=s2`. Sans `s0`, le cône de `{s1,s2}` ne
contient pas `r0`. La fixture ne confond pas cette garde avec la carte
projective : `(r0+r1+r2) dot s0=63>0`, donc son dénominateur reste strictement
positif.

Pour `t=(7,-32,1)`, `t dot d=0` et les trois marges intérieures valent
`-21,-3,-1` : aucun intérieur strict. Une translation u16 est
`a=(100,100,100)`, `b=(109,102,101)` et
`z={(121,37,100),(103,101,100),(103,101,101)}`. Cette fixture doit venir
**après** la gate `smax`, faute de quoi le seuil figé masque le cas `h=1`.

## Gate préalable sur `smax`

Le sujet et le falsificateur partagent encore `kNeed={10,9,8}` alors que la CLI
accepte `4<=smax<=34`. Une fixture sans mutant le montre. Prendre les vingt-quatre
sites `lambda r_j`, `1<=lambda<=8`, et `d=(100,20,10)`. L'oracle conique et le
hull exact donnent huit crédits aux seuils `4,8,12,15,19,23,26,30`.

- `smax=11` : q4 exige huit crédits et ferme ;
- `smax=12` : q4 en exige neuf et reste `UNKNOWN` ;
- `smax=34` : q4 en exige trente et un et reste `UNKNOWN`.

Le sujet figé ferme les trois. Sur dix-sept sphères admissibles, le minimum
d'intérieurs vaut huit ; le juge figé annonce zéro défaut, tandis que le vrai
seuil trouve sept défauts à `smax=12` et dix-sept à `smax=34`. Cette gate est P0
avant les trois mutants encore ouverts.

Le delta live postérieur remplace maintenant la constante par
`need_of(smax,lane)=smax+1-(lane+2)` dans le sujet et le falsificateur. C'est la
bonne correction ; elle ne devient permanente qu'après ajout de cette fixture
nominale à `smax=11/12/34`, car le selftest actuel n'exerce pas ces trois
verdicts bout en bout.

## Successeur Andrew : progrès reçu et prochain pas positif

Un successeur non committé a été observé stable au-dessus du même
`HEAD=88eb36d` avec `cell_credits.hpp=f9d4981d...` et
`cell_credits_probe.cpp=a8c4e9ad...`, `CMakeLists.txt=012c2690...`. Il remplace Jarvis par un ordre exact des
rationnels projectifs, fusionne les directions égales, construit les deux
chaînes monotones, refuse un hull de dimension inférieure à deux, dérive `h` de
`smax` et rend l'union des carriers des trois rayons.

Le build Release/CUDA OFF réussit. L'ELF local `c097fc06...`, Build ID
`cbe73485...`, rend `mhgp3v_credits_selftest` vert avec `37 752/37 752`
accords, `471` couvertures et quatre fixtures. La nouvelle porte
`mhgp3v_credits_fixtures_mutants` rend le code attendu `4` et rejoue pour les
trois injections `t dot d=0`, zéro intérieur, `reference=UNKNOWN`,
`injection=CREDIT`, `mutant_reached=1` et `mutant_killed=1`; le lot ciblé fait
`2/2` en `0,84 s`. Un checker
indépendant, source `d434c83c...`, rejoue `1 533` sous-pools et permutations
sur `U00` : `fp=0`, `fn=0`, `bad_carrier=0`, `bad_id=0`. Il reçoit aussi le
nominal avec direction dupliquée. C'est un progrès substantiel : les deux P0
`h==2` et duplicats sont réparés sur ce domaine borné.

Deux petites gates rendent ce résultat durable : le selftest permanent doit
rejouer que l'union rendue couvre réellement chacun des trois rayons, et une
fixture positive `m=4` doit exercer une direction dupliquée sans être rejetée
par la seule garde `m<3`. Ensuite, le tri ne doit plus être reconstruit à chaque
préfixe. Le live appelle un tri par insertion `O(m^2)` pour chaque ajout, donc
reste `O(m^3)` au pire malgré la disparition des triples explicites. Trier une
fois par clé projective puis rechercher le premier préfixe couvrant ramène une
version simple à `O(h m log m)` scans pour `h<=33`; un sweep de hull dynamique
peut ensuite abaisser encore ce travail.

La gate « un seul rayon » et l'injection principale doivent aussi partager la
même transformation. La fixture appelle directement l'appartenance de `r0` au
cône d'un singleton, tandis que la boucle principale passe trois copies de
`r0` à `cell_covered`, qui impose un hull bidimensionnel. Deux solutions sont
recevables : factoriser un unique chemin mutant « ne tester que `r0` », ou
conserver le chemin principal et employer la fixture tridirectionnelle donnée
plus haut. Sans cet alignement, la porte tue la faute mathématique mais pas
nécessairement les octets injectés dans la campagne.

Enfin, un crédit cellulaire consomme l'union de trois carriers, soit de trois à
neuf IDs dans le chemin sain, et non « un à trois ». Publier l'histogramme
`credit_union_size[3..9]` séparément des trois rangs par rayon évite de masquer
le coût et explique directement la borne du pool.

Le nouveau `rank_counts` doit être transactionnel. Il est actuellement
incrémenté dès qu'un premier rayon trouve son carrier, avant de savoir si les
deux autres passent. Avec le pool
`{(3,0,0),(3,1,-1),(3,-1,0)}`, `r0` donne un rang un puis la cellule complète
échoue : le compteur augmente malgré l'absence de crédit. Les nouveaux
planchers CMake mesurent donc aussi des tentatives partielles. Rendre les trois
rangs dans un buffer local et ne les verser au ledger qu'après `cell_covered=true`,
ou publier séparément `attempt_carrier_rank` et `accepted_credit_carrier_rank`.

Le premier produit utile de cette primitive n'est pas une nouvelle boucle
`(a,b)`. Pour une ancre et une cellule, les `h` crédits définissent un suffixe
de hauteur : émettre un `StarKey=(AnchorId,CellId,lane,X,CreditKeys)` et laisser
un range-report factorisé couvrir les cibles. Le jalon suivant recertifie les
mêmes IDs sur les huit coins d'un `AnchorNodeKey`, puis émet un `RectKey`; la
garde bilinéaire exacte `L_z(A,B)>0` traite H2 sans développer les `PairId`.
Le cap de banque reste fail-open et publie `bank_truncated/pool_truncated` : les
points les plus proches ne sont pas nécessairement ceux de plus petite
activation.

## Rôle légitime de la marge différentielle

Publier `min_interiors`, les quantiles, les paires différentielles et la distance
au seuil reste utile pour choisir des fixtures et mesurer la puissance du
certificat. La règle est néanmoins :

- divergence sans faux prune démontré : ablation ou diagnostic ;
- fermeture avec une sphère explicite sous le seuil : mutant tué ;
- absence de divergence : porte vacueuse ;
- défaut de l'oracle partagé, comme `smax`, à réparer avant toute interprétation.

Les fixtures ci-dessus doivent rejuger les `CreditKey`, leurs ensembles de
`PointId`, les événements et la sphère fautive par une seconde écriture exacte.
Un échantillon axial peut rester un fuzz complémentaire, jamais l'autorité.

## Déblocage positif : remplacer Jarvis, puis sortir du pairwise

Les fixtures ferment les dettes d'exactitude, mais elles ne sont pas le jalon
de performance. Le remplacement constructif proposé est un hull d'Andrew exact
dans la carte projective :

1. choisir `e` orthogonal à `w=r0+r1+r2`, puis `f=w cross e` ;
2. calculer par site `W=w dot s>0`, `E=e dot s`, `F=f dot s` ;
3. trier exactement les rationnels `(E/W,F/W)` par produits croisés `i64` ;
4. regrouper les directions projectives égales en piles `(X,PointId)` ;
5. construire les deux chaînes avec `det(si,sj,sk)` comme orientation ;
6. tester les trois rayons et extraire les carriers minimaux de rang `1/2/3` ;
7. retirer seulement les IDs consommés et reconstruire au plus `h<=33` fois
   sur le domaine CLI, ou dix fois lorsque `smax=11`.

Cette ordonnance supprime `C(m,3)`, le tie-break quartique qui déborde `i64` et
la terminaison cyclique de Jarvis. À `M<=128`, elle se baisse en un bitonic sort
device puis un nombre de scans borné par le seuil dynamique. Pour obtenir vite
une première autorité sûre, un
crédit peut même prendre un ID de chaque sommet géométrique du bord dès que le
hull contient le triangle de la cellule. Cela consomme plus d'IDs et perd du
rappel, mais constitue une baseline simple ; les carriers minimaux deviennent
ensuite une optimisation différentielle contre cette référence.

Le delta Andrew `cell_credits.hpp=f9d4981d...` réalise désormais cette base.
Son exhaustif rend `37 752/37 752` accords et `471` couvertures ; la nouvelle
fixture de rayon unique rend `reference=0`, `injection=1`. Les largeurs sont
compatibles avec `i64` sur les 432 cellules : les maxima observés de norme `L1`
de `(w,e,f)` sont `(24,17,264)`, les produits croisés restent sous `5,45e13` et
les déterminants sous `1,69e15`.

Deux petites gardes séparent encore ce delta d'une API reçue. Sur un retour
`false` après avoir couvert `r0`, `union_size` conserve actuellement un carrier
partiel **et `rank_counts` a déjà été incrémenté**. Les callers présents ignorent
l'union, mais les planchers de rang peuvent alors être servis par des tentatives
qui n'ont émis aucun crédit. Le contrat doit être transactionnel : accumuler
union et rangs localement, les fusionner seulement au retour `true`, et rendre
`union_size=0` sans compteur d'émission sur `false`. Un compteur distinct de
tentatives partielles reste légitime. Le selftest doit aussi rejouer que les IDs
rendus couvrent effectivement les trois rayons, et pas seulement vérifier
qu'ils appartiennent au pool. Les permutations, duplicats après retrait et
extrêmes u16 complètent cette porte. La recherche du rang un limitée aux
sommets du hull reste sound ; elle peut seulement perdre du packing lorsqu'un
site intérieur est colinéaire au rayon.

Le jalon suivant ne doit pas relancer une campagne `n(n-1)`. Le raccord positif
le plus court garde d'abord l'ancre feuille et supprime immédiatement le scan de
toutes les cibles : un `CellSuffixReporter` parcourt le LBVH cible une seule fois
par seuil `(a,j,X_q)`. Il classe une AABB par les extrema entiers des facettes
half-open de `b-a` et de la hauteur : `ALL` si les trois facettes et le seuil
sont vrais sur tout le nœud, `NONE` si une borne les réfute partout, sinon
`MIXED` et split. Une traversée transporte le masque q2/q3/q4 et émet les nœuds
`ALL` maximaux avec leur masse et leurs `CreditKey`; un cap conserve le front
`MIXED` authentifié.

Seulement après que ce reporter est vert, lever l'ancre feuille au front
canonique de rectangles. Une ancre représentative propose un carrier, les huit
coins de l'AABB le recertifient, puis chaque ID passe le classifieur H2 exact
`L_z(A,B)=sum_i min_{a_i,b_i}(z_i-a_i)(b_i-z_i)>0`. Les sorties sont des
`RectKey/BankKey/CreditKey`, jamais des `PairId`; tout échec ou cap conserve le
rectangle résiduel. C'est ce front, ses octets et son HWM qui doivent être
mesurés à `12 500/25 000/50 000`.

## Ordre de réparation

1. Stabiliser/committer Andrew et `need_of`, puis graver la fixture `smax` et la
   vérification indépendante des carriers.
2. Conserver les directions projectives dupliquées comme piles de `PointId`
   réutilisables après consommation du représentant, et recevoir les fixtures
   positives/dégénérées sous permutations.
3. Armer les quatre fixtures ci-dessus, chacune avec référence non vacueuse et
   code de sortie exact ; rendre la sortie du hull transactionnelle et rejouer
   ses carriers.
4. Raccorder le `CellSuffixReporter` ancre-feuille × LBVH cible et publier son
   front `ALL/MIXED`, ses octets et son HWM ; aucune matrice de `PairId`.
5. Amortir tri/hull entre les préfixes, puis rejouer les masses
   `pool=16/32/48`; ne conserver que les fermetures dont les `CreditKey` sont
   rejouables.
6. Lever l'ancre aux `RectKey` recertifiés par huit coins et `L_z` avant toute
   rampe `12 500/25 000/50 000` ou lowering CUDA.

Le statut reste NO-GO 50 k/G4. GCP non utilisé.
