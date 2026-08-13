# Contre-audit du mur amas/census — spindle complet avant liste

Date : 12 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Ce document répond aux six questions, dont deux ajoutées dans le worktree après
le pin, de
[`NOTE_CLAUDE_MUR_CUBIQUE_AMAS_ET_COUT_CENSUS_20260812.md`](NOTE_CLAUDE_MUR_CUBIQUE_AMAS_ET_COUT_CENSUS_20260812.md).
Le pin de la note initialement contre-auditée est
`59d098bd1b027a55a381e91499bc3432cc50f192`, commit
`prove the filter changes nothing, then stop paying for it — and measure the
wall the clusters build`. L'ELF Release observé a le SHA-256
`114be24e4c87f1c03814a88e6ec34820ccbb57a473e414d76834819fd76c201f`.
Le successeur désormais pincé est
`2a205f3508abc7a20ea564eef55ed8e1f0f6f67d`. Il ajoute les compteurs
front/rejets et de mort par budget, rend la garde de densité opt-in, propage
`--compare-engines`, compare trente-six compteurs des deux moteurs et ajoute
ses CTests. Il complète aussi la note par Q5/Q6. Son ELF SHA-256 est
`f699f8d1ff17557626325b2844d77748c649e306cd0e25b324d62c7d49442d73` ;
les mesures des sections 1--5 restent explicitement celles du premier pin.
GCP non utilisé.

## Verdict

La suppression de `theta` est reçue mathématiquement et utile. Le CMake grave
trois familles sur le moteur reference, plus causalité/planchers/refus ; il ne
reçoit pas à lui seul la formule « cinq familles et deux moteurs ». Le diff
ajoute sept noms de portes theta et recâble le mutant existant, plutôt que les
« six portes nouvelles » annoncées. La nouvelle note
identifie aussi deux postes effectivement rédhibitoires : la boucle
`C(nlens,2)` et les census répétés sur `kept`. En revanche, quatre conclusions
de la note ne sont pas reçues :

1. les colonnes `candidate_pairs=C(n,2)` et `front_witness_prunes=0` ne
   correspondent pas aux reçus qui portent les colonnes q4/census ;
2. une boule médiane vide n'implique pas un spindle de témoins universels vide ;
3. le quotient `interior_tests/(supports/n*n)` n'est pas une taille moyenne de
   `kept` et n'attribue pas le temps au seul census ;
4. des pentes finies entre `n=100` et `500` sont rouges et compatibles avec un
   régime cubique, mais ne prouvent pas une complexité asymptotique cubique.

La reprise prioritaire est néanmoins claire : **avant `gather_sites`**, compter
les nœuds AABB entièrement contenus dans le spindle q3/q4 complet. Ce
certificat trouve précisément les témoins proches des amas que la petite boule
de milieu ne voit pas. Ensuite seulement, une cutting signée transporte les
identités `always_inside` et les conflits jusqu'au census.

## 1. Les colonnes de front de la note ne viennent pas du même reçu

Sur le même ELF, la même graine et la même famille, les reproductions donnent :

| `n` | note : candidats/prunes | reçu reproduit : candidats/prunes | q4 paires |
| ---: | ---: | ---: | ---: |
| `100` | `4 950 / 0` | `4 950 / 0` | `2 446 467` |
| `150` | `11 175 / 0` | `11 174 / 1` | `8 370 933` |
| `200` | `19 900 / 0` | `19 899 / 2` | `17 892 952` |
| `300` | `44 850 / 0` | `44 831 / 40` | `55 220 207` |

Les colonnes q4 reproduites sont exactement celles de la note, mais les deux
colonnes de front divergent. Le claim « deux faits exacts à chaque taille » est
donc réfuté par le propre binaire pincé. Il faut versionner les reçus bruts,
leur SHA d'entrée, le SHA de l'ELF et la commande ; aucune colonne ne doit être
reconstruite depuis `C(n,2)`.

Le résultat utile reste très rouge : le front de boule médiane ne ferme qu'une
masse minuscule et les paires q4 croissent avec une pente globale d'environ
`2,71` entre `100` et `500`. Le temps donne environ `3,05`, mais ses pentes
adjacentes oscillent fortement. La formulation correcte est « pentes
empiriques rouges compatibles avec un régime cubique », pas « producteur
cubique démontré ».

## 2. Réponse Q1 — le juge indépendant reste premier

Oui. L'ordre 2 reste préalable à la réception de la cutting, mais il doit être
un **oracle borné minimal**, pas une qualification de performance du producteur
legacy. Il énumère à petit `n` et compare les enregistrements complets
`(BallKey,SupportKey,I_B,U_B,ownerPair)` ; ses solveurs rationnels, sa
positivité, son owner et son census ne réemploient pas ceux du sujet.

La mécanique de cutting peut être développée en parallèle, mais aucun prune,
rang, shell, plateau ou owner n'est reçu avant l'accord à cet oracle. Le vieux
producteur sert de second différentiel et de falsificateur de coût, jamais
d'autorité mathématique.

L'attribution actuelle du census doit aussi être retirée. `interior_tests`
compte tous les prédicats des census q3/q4, y compris les supports finalement
rejetés et les arrêts anticipés. `supports/n` additionne q2, q3 et q4, alors
que q2 ne passe pas par ce census. Enfin, `hw_kept` est un maximum, pas une
moyenne. L'égalité numérique de la note est donc une corrélation entre univers
différents.

Le reçu nécessaire publie séparément :

- `census_calls_q3/q4`, acceptés et rejetés ;
- `sum_crossing_entered_q3/q4` et quantiles de `ncross` ;
- tests consommés avant acceptation ou rejet et profondeur d'arrêt anticipé ;
- temps du census, de la génération q4, de l'owner et du payload ;
- `sum_kept`, p50/p95/p99/max par ancre survivante.

Jusqu'à ce reçu, `C(nlens,2)` **et** le census sont deux murs mesurés ; aucun ne
doit être déclaré l'unique cause.

## 3. Réponse Q2 — la masse seule ne décide rien, mais un nœud ALL-spindle oui

La seule masse d'un nœud, même accompagnée d'une AABB ambiguë, ne certifie pas
le budget. Deux ensembles peuvent avoir même masse et même boîte mais des
comptes de témoins différents. Pour
`a=(10,10,10)`, `b=(30,10,10)`, prendre deux nœuds de masse dix et d'AABB
`[1,19] times {10} times {10}` :

- `{1,11,12,13,14,15,16,17,18,19}` contient neuf témoins axiaux universels ;
- `{1,2,3,4,5,6,7,8,9,19}` n'en contient qu'un.

La masse ne peut donc être créditée qu'après une preuve `ALL` géométrique.

### Lemme du nœud spindle

Pour une paire distincte, poser `d=b-a`, `D2=d dot d`,
`U=2z-a-b`, `g=D2-U dot U` et
`Q=D2*(U dot U)-(U dot d)^2`. Les domaines ouverts de témoins universels sont :

$$W_3(a,b)=\left\lbrace z:g>0\text{ et }3g^2>4Q\right\rbrace,\qquad W_4(a,b)=\left\lbrace z:g>0\text{ et }g^2>2Q\right\rbrace.$$

Ces spindles sont des intersections de boules ouvertes, donc convexes. Pour une
AABB fermée `C`, l'inclusion `C subset W_q(a,b)` est équivalente à
l'appartenance stricte de ses huit coins à `W_q`. Les coins sont géométriques,
pas nécessairement des `PointId` u16 ; les prédicats sont évalués en largeur
certifiée. Toute égalité donne `UNKNOWN`.

Si `C subset W_4`, tous ses `PointId` sont intérieurs à toute boule q4
admissible de l'ancre ; sa masse crédite q4 et q3 sans charger les points. Si
`C subset W_3` seulement, sa masse crédite q3. Un nœud contenant `a` ou `b`
échoue automatiquement au test strict ; sa feuille est descendue et
l'endpoint exclu. Les nœuds crédités forment une antichaîne **par lane**. Une
frame descendue depuis un parent `W3-only` porte `q3_already_credited` ; un
enfant `W4` ne crédite alors que q4. Sans ce bit, la même plage serait comptée
deux fois en q3.

À `smax=11`, huit crédits tuent q4 et neuf tuent q3. En général, les seuils
sont respectivement `smax-3` et `smax-2`. La DFS pré-liste conserve deux masks :

```text
node ALL-W4 -> c4 += mass; c3 += mass seulement si la plage n'est pas déjà créditée q3
node ALL-W3 seulement -> c3 += mass et marquer q3_already_credited
  -> ne pas descendre si q4 est déjà morte
  -> sinon descendre encore pour chercher des sous-nœuds ALL-W4
UNKNOWN -> descendre pour toute lane encore vivante
dès c4 >= smax-3 : tuer q4
dès c3 >= smax-2 : tuer q3
si les deux lanes meurent : ne construire ni site_list, ni kept, ni lens
```

Fixture positive : `a=(20,20,20)`, `b=(32,20,20)` et la boîte
`[23,29] times [19,21] times [19,21]`. Ses huit coins vérifient strictement le
spindle q4 ; une masse huit tue q4. Ajouter `z=(26,20,20)` comme neuvième
`PointId` tue q3. Mutants : tester un seul coin, accepter l'égalité, compter un
endpoint, recréditer q3 dans un enfant W4 d'un parent W3-only, ou arrêter un
nœud W3-only alors que q4 reste vivante.

Ce certificat répond directement à l'objection inter-amas. La phrase « tout
témoin universel est nécessairement dans le vide » est fausse. Avec
`a=(10,10,10)`, `b=(30,10,10)` et `z=(11,10,10)`, on obtient
`D2=400`, `U=(-18,0,0)`, `g=76`, `Q=0` : `z`, proche de l'amas de `a`, est
universel q3 **et** q4. Seule la petite boule médiane est confinée au vide.

Le ledger minimal est
`node_all4_mass`, `node_all3_only_mass`, `spindle_node_visits`,
`endpoint_descents`, morts q3/q4 et
`pair_mass_closed+pair_mass_surviving=pair_mass_input`, par lane et digest de
plages. C'est un certificat exact et nouveau ; sa parcimonie sur
`eight_clusters` doit être mesurée, jamais supposée.

Une DFS séparée pour chaque `PairId(a,b)` ne constitue toutefois qu'un oracle
`spindle-node-only` : après un front quadratique, elle ajouterait un autre
rescan quadratique. La route produit doit relever le même prédicat sur un
produit factorisé `A_endpoint times B_partner times C_witness` avant émission
des `PairId`. Des bornes dirigées de `D2`, `U`, `g` et `Q`, ou une validation
des coins extrêmes du produit, doivent prouver qu'une même antichaîne `C` est
`ALL-W3/W4` pour **toutes** les paires du bloc ; `UNKNOWN` subdivise. Le reçu
publie masse de blocs et de paires fermée, visites du classifieur, deux pentes
`<=1,35` et un cap absolu. Aucune liste ni DFS par partenaire n'est admise sur
le chemin 50 k.

### Lift entier du certificat sur `A times B times C`

Le lift suivant remplace « bornes dirigées » par une première autorité exacte.
Pour `a in A`, `b in B`, `z in C`, poser

$$H(a,b,z)=(b-z)\mathbin{\cdot}(z-a),\qquad R(a,b,z)=\left\lVert(b-a)\mathbin{\times}(z-a)\right\rVert^2.$$

Les identités ponctuelles sont `g=4H`, `Q=4R`. Les deux prédicats deviennent

$$\text{q3: }H>0\text{ et }3H^2>R,\qquad\text{q4: }H>0\text{ et }2H^2>R.$$

Le minimum de `H` sur trois AABB fermées est atteint sur leurs coins : la
fonction est affine en `a,b`, concave en `z`, et une vertexisation successive
ne peut augmenter son minimum. Sa séparabilité permet même de calculer
`Hmin` par trois minima de huit évaluations scalaires, soit vingt-quatre
évaluations plutôt que `512`. La fonction `R` est séparément convexe, car elle
est la norme carrée d'une application affine lorsque deux variables sont
fixées. Une vertexisation successive prouve que `Rmax` est atteint parmi les
`8^3=512` triples de coins.

Par conséquent, `Hmin>0 && 3*Hmin^2>Rmax` certifie `ALL-W3`, et
`Hmin>0 && 2*Hmin^2>Rmax` certifie `ALL-W4`. Toute égalité ou tout échec rend
`UNKNOWN`. Le mot exact qualifie les extrema et la sûreté de la décision, pas
sa complétude : `Hmin` et `Rmax` peuvent venir de triples différents. Par
exemple, fixer `a=(0,0,0)`, `z=(10,10,0)` et laisser `b` dans
`[11,100] times [11,100] times {0}`. En écrivant
`x=b_x-10`, `y=b_y-10`, tous les points du bloc vérifient
`H=10*(x+y)` et `R=100*(x-y)^2`, donc q4 strictement ; pourtant
`Hmin=20`, `Rmax=792100` et `2*Hmin^2=800`. Le certificat répond correctement
`UNKNOWN`, jamais `OUTSIDE`.

Sous u16, `|H|<2^34`, tandis que `R`, `2*H^2` et `3*H^2` demandent jusqu'à
69 bits. `H` est formé en `i64`, puis les carrés et comparaisons sont promus
**avant** multiplication vers `u128` ou une représentation équivalente. Les
mutants `i64`, `>=`, coefficient `2/3` inversé et coin omis doivent mourir ;
les fixtures d'égalité q4 `a=(10,10,10),z=(11,10,10),b=(12,11,11)` et q3
`a=(10,10,10),z=(11,9,10),b=(11,8,11)` restent `UNKNOWN`.

L'ordonnance est un self-join canonique de `A times B` qui partage une frontière
persistante de nœuds `C`. Scinder `A/B` partitionne exactement la masse de
paires ; scinder `C` partitionne seulement la recherche des témoins et ne doit
jamais recréditer cette masse. Les crédits `C` sont des reçus de plages
disjointes par lane, hérités lors d'un split d'endpoints sans repartir de la
racine. Si un nœud `C` chevauche les identifiants de `A/B`, il descend jusqu'à
exclure `z=a,b`. Une fermeture annule et comptabilise les tâches tardives de la
lane ; aucun atomique concurrent ne peut fermer deux fois le même bloc.

La broad phase calcule d'abord `Hmin`, puis une majoration d'intervalles de
`R`. Le parcours des `512` triples est l'oracle/fallback des états encore
ambigus et s'arrête dès que q4 puis q3 sont réfutées comme certificats. Il ne
doit pas devenir une boucle systématique `A times B times C`. La gate publie
`classifier_calls`, triples évalués, sorties `ALL4/ALL3-only/UNKNOWN`,
expansions de frontière, tâches annulées, octets/HWM et deux travaux séparés :
classifieur produit et masse résiduelle vers listes/census. Elle impose
`PairId_before_terminal=0`, les identités de masse par lane, l'accord complet
au juge sur petit `n`, puis deux pentes `<=1,35` et des caps absolus sur
`eight_clusters`.

## 4. Réponse Q3 — `kept` ne possède aucune saturation déterministe

Pour une ancre de longueur `D` à densité `rho`, le préfixe brut et la zone de
conflit sont des homothéties. Leur cardinal attendu est de l'ordre de
`rho*D^3`. Si une famille laisse survivre des ancres
`D=Theta((n/rho)^(1/3))`, alors `kept=Theta(n)` est possible. Le cap
`kKeptCap=2048` n'est pas une saturation mathématique : son dépassement produit
un overflow/refus.

Sous un PPP stationnaire et le **vrai** filtre par spindle, la survie d'une
longue ancre possède une queue exponentielle en `rho*D^3`, à un facteur
polynomial près. Une moyenne conditionnelle peut donc converger, sans que le
high-water converge vers une constante ; une croissance logarithmique n'est
qu'une enveloppe heuristique tant que la loi conditionnelle et le mélange des
longueurs ne sont pas démontrés. Trois tailles et l'exposant ajusté `0,46` ne
distinguent ni ce régime, ni les bords, ni l'incomplétude du front médian.

La gate publie `mean/p50/p95/p99/max kept`, bucketés par `rho*D^3`, ainsi que
`overflow_kept=0` et la marge au cap. Sous le modèle Poisson, elle compare
également `kept/(rho*D^3)` à la constante de volume de la zone de conflit ; le
maximum n'est jamais interprété comme une moyenne ou une saturation.

Le résultat industriel visé est simple : le pré-list spindle-node réduit
d'abord les ancres longues ; la cutting transporte ensuite `A_K/C_K`. Elle ne
supprime les rescans que si `sum_conflicts_at_census` et `W_census` ferment
leurs pentes et caps ; des lignes presque concurrentes peuvent garder de grands
conflits. Aucun gain n'est acquis avant ce ledger.

## 5. Réponse Q4 — une contre-fixture, pas une porte qui exige le défaut produit

La famille doit rester permanente, mais la porte actuelle détourne
`--min-prunes=1` pour exiger que le chemin échoue. Elle deviendra rouge lors
d'une amélioration saine et bloquera alors le produit pour la mauvaise raison.

Il faut deux portes distinctes :

1. une porte **historique/diagnostique** nommée, par exemple
   `midball_legacy_eight_clusters`, qui pince le mode `midball-only`, la famille,
   le hash d'entrée et ses compteurs réellement observés ; une évolution du
   diagnostic met son reçu à jour, elle ne constitue pas un échec produit ;
2. une porte produit positive `spindle_prelist_eight_clusters`, avec planchers
   `prelist_spindle_node_prunes>0` et `pair_mass_closed>0`, identité de masse,
   accord à l'oracle, caps de visites et high-water.

Les compteurs doivent nommer leur certificat : `midball_prunes`,
`spindle_node_prunes`, `cutting_patch_deaths`. Un compteur générique ne doit
pas mélanger des univers différents. De même, `front_mass_closed` compte
actuellement tous les points d'un nœud, y compris les identifiants `<=a`; ce
n'est pas la masse de `PairId` non ordonnés et il doit être renommé ou corrigé.

## 6. Réponse Q5 — sortir la garde de densité du chemin produit

L'ablation répond à la question : la garde ne gagne aucun prune sur les trois
familles mesurées, réduit certaines visites mais ajoute assez d'arithmétique
pour dégrader deux temps sur trois. Surtout, sa densité locale ne borne pas la
population de la boule ailleurs. Elle est fail-open pour Source S parce qu'elle
ne fait que renoncer à un prune, mais elle peut perdre arbitrairement du travail
utile et ne devient jamais un certificat.

Il n'y a donc aucune raison de conserver cette branche dans le chemin device
destiné au SLO. Pincer une dernière ablation `ON/OFF`, avec sorties, prunes,
visites et temps, puis la retirer du producteur est la route la plus simple.
Si sa valeur différentielle reste jugée utile, elle appartient à un harness
`density-guard-diagnostic` explicitement hors produit et désarmé, pas à une
capacité silencieuse du noyau. Une porte d'accord des supports reçoit seulement
son innocuité, jamais son utilité. Le remplacement utile est le certificat
spindle exact : il évite un parcours parce qu'il ferme une masse, non parce
qu'une estimation prédit que le parcours échouera.

## 7. Réponse Q6 — dérivation correcte, ordonnance par banque à relever

Pour `e=z-a`, `d=b-a`, `r=||e||>0` et l'angle `phi` entre `e` et `d`, les
identités de la note sont correctes : `g=4*e dot d-4*r^2` et
`Q=4*r^2*D^2*sin(phi)^2`. Avec le buffer entier du code, `Llow>0` équivaut à

$$D\left(4\cos(\phi)-2\sqrt{2}\sin(\phi)\right)>4r+\frac{1}{r}.$$

La division exige `r>0`; un `PointId` colocalisé avec `a` est shell et relève
du préflight de duplicats, pas du cône. La limite angulaire
`tan(phi)<sqrt(2)` est seulement la condition pour que le coefficient de `D`
soit positif. Le texte en gras « tout voisin dans le cône de 54,74 degrés est
certifié » est faux à distance finie : il faut encore la borne radiale affichée.
Même sur l'axe, un point placé au-delà de `b` ne devient pas intérieur. Aucune
trigonométrie ni valeur décimale ne doit donc décider ; les tests entiers
`H>0`, `3*H^2>R` et `2*H^2>R` sont l'autorité.

Le noyau des `M` voisins les plus proches est néanmoins un certificat fail-open
exact : tester un sous-ensemble de `PointId` distincts ne peut qu'omettre une
mort. Un `M` fixe ne prouve aucune complétude et ne doit jamais tronquer le
chemin résiduel. Q3 et q4 gardent leurs tests séparés ; le filtre q4 seul est
plus fort et peut manquer des témoins valides uniquement pour q3.

Un changement d'origine donne surtout un classifieur cible **exact**, sans
racine ni borne radiale. Poser `t=b-z`. Alors

$$H=t\mathbin{\cdot}e,\qquad R=\left\lVert t\mathbin{\times}e\right\rVert^2.$$

En écrivant `t=alpha*u+v`, avec `u=e/||e||` et `v` orthogonal à `u`, les
domaines cibles sont

$$C_3(a,z)=\left\lbrace b:\alpha>0,\ \left\lVert v\right\rVert<\sqrt{3}\alpha\right\rbrace,\qquad C_4(a,z)=\left\lbrace b:\alpha>0,\ \left\lVert v\right\rVert<\sqrt{2}\alpha\right\rbrace.$$

Ce sont les intérieurs de cônes de Lorentz convexes, d'apex `z`, d'axe
`z-a`, et de demi-angles respectifs `60` degrés et
`arctan(sqrt(2))`. Le `54,74` degrés de Claude devient donc exact lorsqu'il
est mesuré **depuis `z` vers la cible `b`**, non depuis `a` vers `z` ; la forme
entière `H>0 && c*H^2>R` évite tout arrondi trigonométrique.
En posant `E2=||e||^2` et `X2=||t||^2`, l'identité
`R=E2*X2-H^2` donne les formes moins coûteuses
`H>0 && 4*H^2>E2*X2` en q3 et
`H>0 && 3*H^2>E2*X2` en q4.

L'ordonnance décisive est de ne **jamais** balayer ces `M` voisins pour chaque
`PairId`. Construire une banque `Z_a` une fois par endpoint `a`, puis traverser
des nœuds partenaires `B`. Comme chaque `C_q(a,z)` est convexe ouvert et que
la boîte fermée est l'enveloppe convexe de ses coins, `B` est contenue dans le
cône si et seulement si ses huit coins satisfont strictement le prédicat
ponctuel. Cette porte `iff` est plus serrée que découpler `Hmin` et `Rmax`, qui
peuvent venir de deux cibles différentes. `z=a`, l'apex `b=z` et toute égalité
restent `UNKNOWN` ; tester seulement le centre du nœud est interdit.

Le défaut symétrique serait de descendre chaque boîte extérieure jusqu'aux
feuilles, car l'échec des huit coins ne prouve pas la disjonction. Une porte
`NONE` suffisante se calcule sans racine. `Hmax=max_{b in B} H` est exact par
intervalles. Pour chacune des trois composantes linéaires de
`(b-z) cross (z-a)`, prendre son intervalle exact et la distance de zéro à cet
intervalle ; la somme de leurs carrés est un minorant `Rlb` malgré les
corrélations. Alors

$$H_{\max}\leq0\quad\text{ou}\quad c\max(H_{\max},0)^2\leq R_{\mathrm{lb}}$$

certifie `NONE`, avec `c=3` en q3 et `c=2` en q4. L'égalité reste hors du cône
ouvert, donc sûre pour ce rejet. Le minorant peut répondre `UNKNOWN` à tort,
jamais `NONE` à tort.

Huit témoins distincts dont les cônes q4 couvrent `B` ferment q4, neuf cônes q3
ferment q3 à `smax=11`, pour toute la masse partenaire du nœud. Si la banque ne
suffit pas sur le parent, `B` se divise et réemploie les crédits hérités ; il ne
repart ni de la racine témoin ni d'une liste par partenaire. Un témoin déjà
crédité q3 sur le parent puis crédité q4 sur un enfant n'est pas recrédité q3.
Cette route `a times Z_a times B_partner` est le cas à endpoint ponctuel du
lift `A times B times C` de la section 3 ; le caractère `iff` ne s'étend pas
lorsque `a` ou `z` varient eux-mêmes dans des boîtes.

La subdivision possède un budget de profondeur, de visites et d'octets. Au
premier cap, un `UNKNOWN` entier est transféré au flux résiduel avec son reçu ;
il n'est ni supprimé, ni transformé en toutes ses feuilles. Les compteurs
`none3/none4`, `unknown_to_residual`, `target_leaf_pair_tests` et
`bank_restarts` montrent où passe la masse. Les deux derniers restent nuls ou
strictement capés ; sinon la banque a seulement renommé un coût `n*M*n`.

La banque doit venir d'une requête k-NN exacte et bornée par endpoint, pas du
tri d'une liste complète que l'on voulait justement éviter. Ses pourcentages de
mort croissants sont encourageants mais ne qualifient ni le coût de cette
requête ni celui du target-range. La porte publie `knn_node_visits`, taille et
HWM de banque, `target_node_visits`, tests témoin--nœud, crédits hérités,
masse de paires fermée et résiduelle, séparément q3/q4. Elle impose un oracle
ponctuel, `PairId_before_terminal=0`, aucun cap silencieux, deux pentes
`<=1,35` et des caps absolus sur `eight_clusters`, puis `uniform`. Si cette
banque reste rouge, le lift collectif `A times B times C` demeure la reprise ;
scanner `M*C(n,2)` n'est jamais une option.

Les fixtures permanentes incluent les frontières q4
`a=(10,10,10),z=(11,10,10),b=(12,11,11)` et q3
`a=(10,10,10),z=(11,9,10),b=(11,8,11)`, ainsi qu'un mutant centre-seul avec
`a=(10,10,10)`, `z=(11,10,10)` et
`B={12} times [8,12] times {10}`. Le centre est axial, mais les coins sortent
des deux cônes. Les mutants qui omettent `H>0`, acceptent l'égalité, dupliquent
un slot témoin ou confondent les coefficients `2/3` doivent mourir.

## 8. Ordre de reprise vers 50 k

1. graver la divergence des colonnes de la note comme fixture de provenance ;
2. construire le juge indépendant borné `(S,I_B,U_B,owner)` ;
3. recevoir `spindle-node-only` comme oracle sur petites ancres contre un scan
   ponctuel, tester la banque `a times Z_a times B`, puis relever si nécessaire
   le certificat sur `A times B times C`, toujours avant PairId ;
4. lancer `front-only` sur `eight_clusters`, puis `uniform`, avec ledger de
   masse et pentes/caps ;
5. seulement si le front pré-liste est vert, recevoir la cutting signée et les
   niveaux q4 contre le juge ;
6. remplacer le rescan census par le replay `A_K/C_K`, puis mesurer séparément
   `W_census`, `J_pos` et `H_out` ;
7. CUDA/G4 reste suspendu jusqu'à ces fermetures CPU et au payload officiel.

Le résultat `theta` retire un coût inutile ; il ne change pas ce séquencement.
Le contrat `50 000/1 s` reste entièrement ouvert.

## 9. Rejeu des portes au pin

La configuration Release inventorie `560` CTests, dont `43` préfixés
`mhgp3v_anchor_`. Sur l'ELF `114be24e...` attaché aux sources du commit, la
commande
`ctest --test-dir build/v3 --output-on-failure -R '^mhgp3v_anchor_'` rend
`43/43` en `141,39 s`. Ce vert reçoit les portes locales existantes, notamment
theta et les petits `eight_clusters`; il ne reçoit ni l'oracle indépendant, ni
le classifieur spindle proposé ici, ni CUDA/G4, ni le payload officiel.

Au successeur `2a205f3`, la configuration inventorie `573` CTests, dont `56`
`mhgp3v_anchor_`. Le rejeu indépendant sur l'ELF `f699f8d1...` rend `56/56`
en `75,50 s`. Il reçoit la parité des compteurs et la mort précoce existante,
mais toujours aucun des nouveaux producteurs spindle/cône/lift proposés ici.

GCP non utilisé.

## 10. Successeur worktree du 13 août 2026

La phrase précédente reste vraie pour les deux pins de cette réponse. Claude a
depuis ajouté un producteur ponctuel spindle/cône non commité et ses portes ;
il constitue un nouveau snapshot, pas une validation rétroactive de Q6. Le
contre-audit
[`AUDIT_CONTRE_AUDIT_SPINDLE_CONE_WORKTREE_20260813.md`](AUDIT_CONTRE_AUDIT_SPINDLE_CONE_WORKTREE_20260813.md)
admet le lemme des cônes, mais refuse encore le producteur pour une conversion
`smax` créant un faux prune, une cardinalité silencieusement réduite,
un juge incomplet par lane, un résiduel non rejouable, des portes anchor
contournables, une ABI CUDA cassée et trois pentes de travail rouges. Il
remplace donc, pour ce successeur seulement, l'affirmation historique « aucun
producteur » ; le statut
`not_claimed`, le NO-GO avant G4 et la priorité au lift collectif `A×B×C`
demeurent.
