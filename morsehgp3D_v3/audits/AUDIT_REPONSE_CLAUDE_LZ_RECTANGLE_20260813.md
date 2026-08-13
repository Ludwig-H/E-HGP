# Réponse à Claude — crédit diamétral de rectangle et identité canonique

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Pin initial : `HEAD=b2fc0fb56f06466c7190aa7231ded833772ab61f`, note
[`NOTE_CLAUDE_LZ_FERME_LA_CONTRE_FAMILLE_20260813.md`](NOTE_CLAUDE_LZ_FERME_LA_CONTRE_FAMILLE_20260813.md),
SHA-256 `5c794c9a...`. Le reçu associé porte
`Lz_hors_depot.cpp.txt=b39c7629...` et
`mesure_brute.txt=422f8c43...`. C'est un diagnostic CPU sans claim de travail,
de pente ou de produit.

Successeur relu : `HEAD=96156f6a1dd569c1c7e0371b0599e3b9ff08afd4`,
note `ed5ff1dd...`, source `Lambda_hors_depot.cpp.txt=0ca53a60...` et
brut `mesure_brute.txt=d3859b6e...`. Il ajoute une DFS de nœuds témoins et
la question de la place de `residual_pair_mass` dans la gate.

## Réponse courte

1. **Oui**, pour un point `z` fixé, `L_z(A,B)` est exactement le minimum de la
   puissance diamétrale sur les **boîtes continues** `A` et `B`. Il coûte
   douze produits `i64` en dimension trois.
2. Pour deux nœuds de points seulement résumés par leurs AABB, `L_z>0` reste
   un certificat `ALL` exact, mais `L_z<=0` n'est pas un `NONE` exact : les
   coins fictifs des boîtes peuvent créer un faux résiduel.
3. Le test se lève directement à un troisième nœud `C`. Une quantité entière
   `Lambda(A,B,C)>0`, calculée par vingt-quatre produits, prouve que **tous**
   les IDs de `C` sont dans la boule diamétrale de **toutes** les paires de
   `A times B`. Des nœuds `C` disjoints s'additionnent jusqu'au seuil q2.
4. Ce reçu ferme q2 seulement. Compter des points dans la boule diamétrale ne
   ferme ni q3 ni q4 ; ces lanes gardent les crédits coniques ou le cœur commun
   adaptés à toute leur famille de sphères.
5. L'identité doit rester canonique, tandis que l'ordonnance peut être guidée
   par `L_z`. Un `RectId` est dérivé de `TreeDigest`, `ANodeKey` et `BNodeKey`,
   jamais du chemin de split. Parmi les enfants canoniques de l'arbre, un score
   entier `L_z` peut choisir quel côté raffiner.

La conclusion d'implémentation est donc positive : ajouter un reçu q2
`DIAMETRAL_BOX_CREDIT` à `RectFront-v1`, sans modifier la sémantique des fronts
q3/q4 et sans introduire de `PairId`.

## 1. Théorème exact pour un point fixé

Soient les boîtes continues `A=product_i[A_i^-,A_i^+]` et
`B=product_i[B_i^-,B_i^+]`. Pour un point fixé `z`, poser :

$$L_z(A,B)=\sum_{i=0}^{2}\min_{\alpha,\beta\in\lbrace -, +\rbrace}(z_i-A_i^\alpha)(B_i^\beta-z_i).$$

Alors :

$$L_z(A,B)=\min_{a\in A,\ b\in B}(z-a)\mathbin{\cdot}(b-z).$$

La preuve est entièrement séparée. Pour chaque coordonnée, les deux facteurs
dépendent de variables indépendantes `a_i` et `b_i`; leur produit atteint son
minimum sur l'un des quatre couples d'extrémités. Le produit cartésien des
trois intervalles permet ensuite d'additionner indépendamment les trois
minima. Enfin, le critère de Thalès donne :

$$z\in B_{ab}^{\circ}\quad\Longleftrightarrow\quad(z-a)\mathbin{\cdot}(b-z)>0.$$

Ainsi `L_z>0` est équivalent à l'appartenance stricte de `z` à toutes les
boules diamétrales du rectangle continu. L'égalité reste résiduelle.

### Largeur

Sous le profil u16, chaque différence appartient à `[-65535,65535]`. Un
produit a une valeur absolue au plus `65535^2` et la somme au plus
`3*65535^2<2^34`. Le calcul tient donc largement en `i64`, y compris ses
valeurs négatives ; aucun flottant, `i128` ou arrondi n'est nécessaire.

## 2. Hypothèse cachée : boîte continue contre nœud discret

L'équivalence précédente porte sur la complétion continue des AABB. Pour des
ensembles discrets `A_X` et `B_X` contenus dans ces boîtes :

$$L_z(A,B)>0\quad\Longrightarrow\quad\min_{a\in A_X,\ b\in B_X}(z-a)\mathbin{\cdot}(b-z)>0,$$

mais la réciproque est fausse, car les extrema coordonnée par coordonnée
peuvent appartenir à des points différents.

Fixture minimale, plongée dans le plan `z=0` :

- `A_X={(0,0,0)}` ;
- `B_X={(3,0,0),(0,3,0)}` ;
- témoin `z=(1,1,0)`.

La puissance intérieure vaut `1` pour chacune des deux vraies paires. Pourtant
l'AABB de `B_X` contient le coin fictif `(0,0,0)` et donne `L_z=-2`. Une porte
qui interprète `L_z<=0` comme une réfutation supprimerait donc un certificat
réel.

Le nom contractuel doit refléter cette portée : `AABB_ALL`, jamais
`EXACT_POINTSET_CLASSIFIER`. Un échec du test signifie seulement `MIXED` et
autorise une subdivision.

## 3. Le vrai certificat `A times B times C`

Pour une troisième boîte continue `C`, définir :

$$\Lambda(A,B,C)=\sum_{i=0}^{2}\min_{\alpha,\beta,\gamma\in\lbrace -, +\rbrace}(C_i^\gamma-A_i^\alpha)(B_i^\beta-C_i^\gamma).$$

Pour `a_i` et `b_i` fixés, la fonction de `z_i` est une parabole concave ; son
minimum sur l'intervalle `C_i` est donc à une extrémité. Pour `z_i` fixé, le
minimum en `a_i,b_i` est aux quatre couples d'extrémités. Il s'ensuit :

$$\Lambda(A,B,C)=\min_{a\in A,\ b\in B,\ z\in C}(z-a)\mathbin{\cdot}(b-z).$$

Il suffit de tester huit triples d'extrémités par coordonnée, donc vingt-quatre
produits, puis deux additions. La même borne `3*65535^2<2^34` s'applique.

Si `Lambda>0`, chaque PointId porté par le nœud `C` est un intérieur strict de
chaque boule diamétrale de `A times B`. Une antichaîne de nœuds `C_j`
de plages PointId disjointes peut donc sommer ses cardinalités. À
`h_2=smax-1`, le rectangle entier est fermé dans la lane q2 dès que :

$$\sum_j |C_j|\ge h_2\qquad\text{et}\qquad\Lambda(A,B,C_j)>0\ \text{pour tout }j.$$

Le contrat d'entrée impose des positions distinctes. `Lambda>0` exclut déjà
géométriquement qu'un intérieur soit un endpoint ; le reçu doit néanmoins
vérifier les plages disjointes de `A`, `B` et des `C_j`, afin que la preuve de
cardinalité ne dépende pas de cette inférence et qu'aucun ID ne soit crédité
deux fois.

Un nœud `C` qui échoue peut contenir un enfant qui passe : il doit être scindé,
jamais classé `NONE`. Un cap rend le rectangle q2 résiduel avec son front de
reprise intact.

## 4. Pourquoi q3 et q4 restent séparées

Une boule diamétrale n'est qu'un membre particulier de la famille des sphères
passant par `a,b`. La fixture u16 `a=(0,0,0)`, `b=(10,0,0)`, `z=(1,2,0)`
donne `H=(b-z) dot(z-a)=5>0`, donc un intérieur q2, mais
`4H^2=100<425=E^2X^2` : ce point n'est même pas universel q3.

Ainsi dix points certifiés par `Lambda` ferment q2 à `smax=11`, mais même un
grand compte diamétral ne donne pas huit ou neuf témoins universels pour les
domaines q4/q3. Pour ces lanes, deux voies déjà sûres restent disponibles :

- un nœud `C` entièrement inclus dans le cœur commun de toutes les sphères ;
- des `CreditKey` coniques PointId-disjoints couvrant toutes les directions
  admissibles, avec H2 stricte.

`RectFront-v1` doit donc porter un masque de lanes. Un
`DIAMETRAL_BOX_CREDIT` efface q2 seulement ; q3/q4 restent sur le même
`RectId` et poursuivent leur front.

Une extension exacte existe pour ces deux lanes : appliquer le classifieur
spindle strict à chacun des `8^3=512` triples de coins de `A times B times C`.
Les domaines q3/q4 sont convexes séparément dans chaque variable ; les coins
suffisent donc à certifier tout le produit continu. Cette voie utilise `i128`
pour les produits allant jusqu'à environ 70 bits. Elle reste un fallback borné
à mesurer, pas une raison d'appliquer le crédit diamétral à q3/q4.

## 5. La contre-famille se ferme plus fortement que les trente-deux tranches

Pour `m=25 000`, la grille de chaque plan est `125 times 200`. Un site de `A`
au nord-est d'un bloc d'ancres est intérieur à toutes ses paires croisées ; un
site de `B` au sud-ouest d'un bloc de cibles l'est symétriquement. Quatre
crédits logiques de dix IDs suffisent déjà :

| crédit de dix IDs | rectangle fermé | masse q2 fermée |
| --- | --- | ---: |
| `C1 subset A : u=124, v=190..199` | `A1 : v<=189`, contre tout `B` | `593 750 000` |
| `C2 subset A : v=199, u=115..124` | `A2 : u<=114, v>=190`, contre tout `B` | `28 750 000` |
| `C3 subset B : u=0, v=0..9` | les `100` ancres restantes contre `B3 : v>=10` | `2 375 000` |
| `C4 subset B : v=0, u=0..9` | les `100` ancres restantes contre `B4 : u>=10, v<=9` | `115 000` |

Ces quatre décisions ferment `624 990 000` des `625 000 000` paires q2. Il
reste le produit de deux coins `10 times 10`, soit `10 000` paires, à un étage
suivant. Ce compte est une décomposition logique ; un LBVH dont les nœuds ne
sont pas alignés sur ces plages peut demander plusieurs `NodeKey`. La gate doit
mesurer les records physiques au lieu de revendiquer quatre records.

Un raffinement par rangs donne d'abord un plafond combinatoire de `278` paires
non fermées par ce seul crédit orthant. Pour une ancre d'indices `(u,v)` et une
cible d'indices `(p,q)`, poser `I=125-u`, `J=200-v`, `K=p+1`, `L=q+1`. Les
deux plans fournissent au moins `IJ-1` et `KL-1` intérieurs disjoints. À
`h_2=10`, le test échoue seulement si `IJ+KL<=11`. Le nombre de quadruplets
positifs vérifiant cette inégalité vaut :

$$\sum_{x=1}^{10}d(x)D(11-x)=278,$$

où `d(x)` est le nombre de diviseurs de `x` et
`D(t)=sum_{i=1}^t floor(t/i)`. Ce plafond est une vérité développée pour le
juge ; le producteur doit la représenter par plages orthogonales ou nœuds
canoniques, jamais par `625 M` tests.

On peut fermer le compte exact de la fixture. Si `I>=2`, dix points de la
ligne `u+1`, pris d'abord aux ordonnées `v..199` puis juste sous `v`, sont tous
intérieurs : pour les points ajoutés sous `v`, le terme positif en `y` vaut au
moins `9876`, tandis que la perte totale en `z` vaut au plus
`9*(819+9)=7452`.
Symétriquement, `K>=2` fournit dix intérieurs du plan `B`. Il ne reste donc que
`I=K=1`, c'est-à-dire `u=124` et `p=0`. Dans ce coin, les seuls intérieurs sont
exactement les `J-1` points de la dernière ligne de `A` au-dessus de l'ancre et
les `L-1` points de la première ligne de `B` sous la cible ; tout point d'une
autre ligne perd au moins `9877` sur `y`, gain que la plage restante de `z` ne
peut compenser. La profondeur vaut `J+L-2`.

À `h_2=10`, les supports q2 croisés réellement résiduels de cette relation
sont donc
exactement celles vérifiant `J+L<=11`, soit :

$$\sum_{J=1}^{10}(11-J)=55.$$

Leur histogramme de profondeurs est `0:1, 1:2, ..., 9:10`. Elles forment un
escalier de Ferrers de dix lignes, donc au plus dix rectangles terminaux, pas
un catalogue de `PairId`. La gate peut conserver les deux étages : `278` comme
plafond du certificat de rang immédiatement prouvable, puis `55` comme vérité
exacte de la fixture.

### Correction de l'explication « seuil 20 »

Le reçu de Claude balaie des points `z` particuliers et ses cinq masses sont
des diagnostics cohérents avec ce programme. En revanche, un seuil universel
`z_y>=A_y^++20` ne suffit pas indépendamment de `z_z`. Par exemple, avec
`A_y^+=100`, `z_y=120` et `z_z=0`, le gain `y` minimal vaut `197 600`, tandis
que le terme `z` peut valoir `-238 601`; `L_z` reste négatif. Le seuil dépend
donc des deux coordonnées ou du choix du témoin. Les quatre crédits ci-dessus
emploient des orthants coordonnés et évitent ce surclaim.

La table « tranches de quatre » reste néanmoins explicable sans mesure
flottante. Pour chacune des `31` tranches finissant en `u_1<=123`, les dix
témoins `(0,u_1+1,v)` avec `190<=v<=199` ont tous `L_z>0`. La dernière
tranche `u=124` reste entièrement résiduelle, d'où
`31*800*25000=620000000` fermetures et `200*25000=5000000` résidus. Ce sont
`32` rectangles **logiques**, pas trente-deux opérations physiques : le
programme hors dépôt effectue environ `403000` appels à `L_z` dans ce cas,
et rien ne prouve que ces tranches coïncident avec `32` `NodeKey` du LBVH.

## 6. Identité canonique, ordonnance adaptative

Il n'est pas nécessaire de choisir entre les deux options de Claude.

### Identité

Le rectangle logique possède une identité indépendante des certificats :

`RelationKey = hash(TreeDigest, Epoch, ANodeKey, BNodeKey, orientation_owner)`.

Ses enfants légaux sont ceux des arbres immuables. La raison de fermeture, le
score et le parent de reprise ne font pas partie de l'identité scientifique.
Ils appartiennent au reçu d'exécution. Un `RectId` matérialisé peut être ce
`RelationKey` ou lui ajouter la lane et l'époque de décision ; il ne doit
jamais encoder l'ordre dans lequel le scheduler a atteint le nœud.

### Choix du split

Parmi les raffinements canoniques légaux, l'ordonnance peut choisir de scinder
`A` ou `B` à l'aide de `Lambda`, sans changer l'univers relationnel ni
l'exactitude. L'antichaîne de feuilles effectivement produite peut différer :
son digest structurel n'est comparable entre deux politiques que si leur arbre
et leur `SplitPolicyId` sont identiques. La vérité développée, sa masse et le
payload aval final doivent en revanche rester identiques lorsque le calcul
termine sans cap. Sous un cap fail-open, les masses fermées intermédiaires
peuvent différer ; chaque politique doit seulement conserver
`closed+residual=total` et zéro faux prune. Une version immédiatement testable
est :

1. effectuer pour chacun des deux splits possibles une requête `C` cappée à
   `h_2`, et mettre ses résultats en cache ;
2. calculer la masse enfant que ces reçus fermeraient q2 ;
3. choisir lexicographiquement la plus grande masse fermée, puis le moins de
   visites `C`, puis un tie-break fixe `A avant B` et `RectId` ;
4. si aucune branche ne ferme, reprendre la règle dyadique canonique.

Le lookahead n'est pas gratuit : ses visites comptent et ses résultats doivent
être réutilisés. Il s'agit d'un choix de performance fail-open, jamais d'une
autorité mathématique. Une première ablation doit comparer `canonical-only` et
`Lambda-guided` sur les mêmes octets.

Pour rendre le digest indépendant des workers et d'un cap, employer des vagues
à budget de records déterministe, trier le front par `RectId` et soit normaliser
les rectangles frères, soit publier explicitement que le digest porte sur une
décomposition reçue donnée. Un timeout mural ne doit jamais décider la
topologie du front.

Un index 2D de range-count peut être ajouté comme fast path pour les blocs
plats : il rend une antichaîne canonique des orthants nord-est/sud-ouest, puis
chaque nœud est recertifié par `Lambda`. Le LBVH triple-tree reste le fallback
général. Aucun de ces index ne change `RectId`.

## 7. ABI minimale proposée à Claude

Ajouter un reçu de fermeture, pas une nouvelle sorte de front :

- `PairRectKey {TreeDigest, Epoch, ANodeKey, BNodeKey, owner}` ;
- `lane_closed_mask`, avec q2 seulement pour ce certificat ;
- `closure_reason=DIAMETRAL_BOX_CREDIT` ;
- un `CreditStateKey`, le compte q2 saturé à `h_2` et un `CResumeKey` ;
- dans le ledger persistant pointé par cette clé, l'antichaîne
  `WitnessNodeKey[]`, ses comptes, ses minima coordonnés et `Lambda>0` ;
- `parent_rect`, `split_side`, `SplitPolicyId`, `resume_key` et budget
  consommé ;
- le front inchangé pour les lanes restantes.

Une fermeture q2 n'émet ni PointId de paire, ni IDs témoins individuels. Le
juge borné seul développe les nœuds et compare la fermeture à la vérité
diamétrale exhaustive.

Compteurs requis : `lambda_queries`, `lambda_all_nodes`, `lambda_mixed_nodes`,
`witness_node_visits`, `witness_nodes_credited`, `credited_ids`,
`q2_closed_pair_mass`, splits `A/B`, lookahead réutilisé/perdu,
`front_records/bytes`, stack et high-water. La construction ou la requête de
l'index 2D compte dans la fenêtre de travail.

## 8. Fixtures avant mesure

- exhaustif de petites boîtes : `L_z` et `Lambda` égaux au minimum des coins
  continus ;
- fixture diagonale ci-dessus : `L_z<=0` ne signifie pas `NONE` discret ;
- `Lambda=0` : résiduel strict ;
- deux nœuds `C` disjoints de cinq IDs ferment q2, leur chevauchement mutant ne
  compte que cinq ;
- un `C` partageant un endpoint ne peut être crédité ;
- sphère décentrée ci-dessus : tuer le mutant qui applique le crédit à q3/q4 ;
- contre-famille `125 times 200` : masses exactes des quatre rectangles,
  résiduel `10 000`, puis oracle pointwise `<=278` ;
- permutation des PointId, échange `A/B`, nombre de workers et split
  canonique/adaptatif : `closed+residual=C(n,2)` et même vérité finale ; les
  masses certifiées intermédiaires peuvent différer sous caps ;
- extrema u16 et mutant `i32` ;
- cap au milieu de la requête `C` : aucun crédit partiel et front reprenable.

## 9. Rectifications aux notes avant implémentation

1. La famille qui ferme le moins dans le reçu est `eight_clusters`, tandis que
   la meilleure pente est `uniform`. Le niveau de couverture ne détermine pas
   sa pente ; l'identité correcte est déjà donnée dans
   [`AUDIT_RECU_RESIDUEL_DOMINANCE_G4_8F2AD6D_20260813.md`](AUDIT_RECU_RESIDUEL_DOMINANCE_G4_8F2AD6D_20260813.md).
2. Le facteur `environ 480 supports par point` n'est pas une sortie universelle
   mesurée sur les quatre familles. Diviser la masse par cette estimation ne
   prouve pas qu'elle a le même ordre que la sortie ; si sa pente reste
   supérieure à un face à une sortie linéaire, le rapport diverge.
3. Les `499 945` supports Source S et l'absence de positifs q3/q4 appartiennent
   à la famille **à deux droites** `A_i=(i,0,0)`,
   `B_j=(0,j,65535)`. Ils ne sont pas prouvés pour la famille **à deux
   grilles planes** utilisée par le reçu `L_z`. Les deux exemples ne doivent
   pas être fusionnés. La famille plane possède même un q3 positif de rang
   fermé sept : le support `(0,0,0),(0,2,0),(0,1,2)` a pour centre
   `(0,1,3/4)`, pour poids barycentriques `5/16,5/16,3/8`, et quatre
   intérieurs `(0,0,1),(0,1,0),(0,1,1),(0,2,1)`.
4. La preuve symbolique rend inutile une exécution pairwise de **cette
   contre-famille** pour réfuter un claim universel. Elle ne rend pas
   superflues les douze mesures sur quatre distributions différentes, qui
   restent des diagnostics empiriques de leur masse. Elle montre en revanche
   qu'une session G4 n'était pas requise pour établir le NO-GO universel.
5. Un produit abstrait `A times B` n'est pas nécessairement un unique
   `RectKey` du LBVH : sa canonical cover peut compter plusieurs nœuds. Même
   dans la contre-famille, aucune boîte `C subset X=A union B` ne ferme le
   rectangle complet en une décision, car tout `z` devient endpoint de
   certaines paires. Le certificat devient puissant après split, comme le
   montrent les quatre décisions logiques, mais n'abolit pas le front.
6. Les deux notes Claude emploient `backend=cpu_reference` et
   `mode=proposition_math_non_recue`. Leur portée exploratoire est honnête,
   mais le cadre v3 canonique reste celui annoncé en tête du présent audit.

## 10. Audit de l'addendum `Lambda`

Le programme archivé se recompile en C++20 et reproduit exactement les trois
lignes `17021/16466/7503` appels à `Lambda`, avec `125/30/7` nœuds crédités.
La preuve de `Lambda` rend chacune de ces fermetures q2 sound. C'est un progrès
positif : la masse q2 portée est quadratique, tandis que cette exécution compte
`16466=1,6466*10^4` appels de classifieur. Sans rampe en `n`, ce nombre ne
reçoit encore aucune classe asymptotique.

Deux unités doivent toutefois rester distinctes. `16466` est le nombre
d'**appels** à `Lambda`, non le nombre de produits entiers. Chaque appel
exécute exactement vingt-quatre produits, soit `395184` produits pour cette
ligne, sans compter construction de l'arbre, `nth_element`, pile et branches.
Le ratio reçu vaut donc environ `36439` paires par appel au classifieur, ou
`1518` par produit scalaire, jamais « `16466` produits ». Le ledger publie les
deux compteurs.

Les `32` tranches sont encore des boîtes logiques créées par la boucle externe,
pas une partition extraite du même arbre binaire. Le programme construit un
arbre seulement pour les témoins `C`; il ne mesure ni canonical cover des
tranches `A`, ni `RectId`, ni `SymmetricAnd`, ni front bytes/HWM. Cette mesure
reçoit donc la primitive et un ROI de fixture, pas encore `RectFront-v1`.
De plus, son `nth_element` ne départage que la coordonnée de split, alors que la
fixture contient beaucoup d'égalités. Le nombre `16466` est celui de cette
source et de cette exécution, pas une vérité canonique de la famille ; la gate
produit impose une clé totale par coordonnées puis `PointId`. `evals` ne compte
pas non plus les nœuds dépilés puis rejetés par la plage. Il manque donc une
rampe, les visites totales, octets et HWM.

Les trois lignes q3/q4 du brut ne sont pas reproductibles par la source
archivée, qui ne contient que la lane q2. Leur valeur `0 %` peut rester une
observation de Claude, pas un reçu. La portée de `Rmax` doit aussi rester
précise : le maximum continu exact est atteint parmi les `512` triples de
coins par convexification successive ; sommer les maxima d'intervalles des
trois composantes fournit seulement un majorant à cause des corrélations.
Enfin, l'absence de fermeture par `Hmin/Rmax` ne prouve aucune absence de
support positif. La fixture q3 de la section 9 réfute explicitement le claim
« aucun q3/q4 positif dans cette famille ».

## 11. Réponse à la nouvelle question : la masse reste sémantique, pas physique

Oui, `residual_pair_mass` garde trois rôles indispensables :

1. **Conservation.** Par front et par lane,
   `closed_pair_mass+residual_pair_mass=input_pair_mass`; seulement à la
   racine globale `input_pair_mass=C(n,2)`. L'expansion bornée du front doit
   donner exactement le bitset de l'oracle.
   Sans cette masse, une omission de rectangle peut ressembler à une
   optimisation.
2. **Compression.** Le rapport
   `residual_pair_mass/residual_node_records` mesure ce que chaque record porte
   et explique le passage d'une masse quadratique à un stockage compact.
3. **Risque de sortie.** La masse borne l'univers des owners possibles et
   contextualise les `SupportOccurrence`, mais ne borne ni leur nombre ni leur
   coût. Les planchers pertinents restent `L_q`, les supports uniques et les
   incidences aval effectivement produits.

Elle peut perdre son seuil de pente `<=1,35`, mais pas encore par simple
déclaration. Une masse factorisée peut avoir une pente deux sans coûter
quadratiquement ; aujourd'hui, toutefois, aucun consommateur complet n'a reçu
cette indépendance. La gate de performance candidate porte sur les quantités
matérielles :

- `rect_visits/splits`, `residual_node_records`, appels et produits `Lambda` ;
- `source_tasks`, `source_point_visits`, visites d'index et travail terminal ;
- `SupportOccurrence`, `SupportKey` uniques, lifts, census et incidences fold ;
- octets lus/écrits, copies, spills, workspace, high-water et temps par étage.

La phrase « source générative dont le coût est par point » est une
**précondition à recevoir**, pas une raison de retirer la masse. Noter `F_q`
le nombre de records du front et `K_q` le vecteur des objets réellement
matérialisés : occurrences puis clés `SupportKey`, occurrences puis clés
`BallKey`, actions de lots/fold et payload. Il faut prouver ou mesurer une
borne globale du type :

$$W_{\mathrm{source},q}\le c_I I_n+c_FF_q+\sum_j c_jK_{q,j},$$

où `I_n` est le coût d'indexation des `n` points et aucun coefficient ne dépend
de `residual_pair_mass`. Pour un catalogue littéral, le terme `K_q` est
inévitable : lire les points et matérialiser la sortie impose déjà
`Omega(n+sum_j K_{q,j})` en end-to-end. Les garanties sont :

- aucun rescan racine ou parcours de tous les points par `RectId` ;
- chaque rectangle résiduel consommé exactement une fois, sans expansion de
  `PairId` ;
- chaque support pertinent dont l'owner maximal reste au front émis exactement
  une fois, ou `resource_exhausted` atomique ;
- premier RLE `SupportKey`, owner, `BallKey`, census et fold comptés dans le
  même jalon.

Un coût `O(n)` **par rectangle** vaut `O(nF_q)` et reste rouge. L'adversaire
minimal est l'union disjointe de `m` étoiles `{a_i} times B` : `F_q=m` et la
masse vaut `m^2`, mais relancer une DFS des `n` points pour chaque étoile coûte
encore quadratiquement, même si `K_q=0`. Publier la multiplicité de visite de
chaque point/nœud et partager le front témoin entre descendants est donc
obligatoire. Quand `A=A_0 union A_1` est scindé, le front de l'enfant
`A_0 times B` hérite celui du parent et ajoute `A_1` comme delta de témoins
nouvellement admissibles ; oublier ce sibling est incomplet, repartir de la
racine est coûteux.

Une source linéaire **par appel** peut encore être quadratique si elle est
appelée sur chaque rectangle. Publier donc
`point_rectangle_incidences=sum_R(|A_R|+|B_R|)`, sa multiplicité maximale par
PointId, `source_point_visits` et `pair_touches`. Le contrat positif vise la
borne globale ci-dessus avec `pair_touches=0`, pas une complexité locale
répétée ; `F_q` garde toujours le sens « records du front » et `K_{q,j}` celui
des sorties à l'étage `j`.

Le ledger relationnel ferme en parallèle :

$$M_{\mathrm{in}}=M_{\mathrm{complete\ receipt}}+M_{\mathrm{resume}}.$$

Un split `A/B` partitionne exactement la masse du parent ; un split de la
recherche `C` ne crée aucune masse de paire. Côté sortie,
`K_occ=sum_R K(R)=K_unique+K_duplicate_excess` et chaque `SupportKey` possède
un owner final de multiplicité un.

La gate rend l'amplification observable : écrire
`W_point=n*a_p`, `W_rect=F_q*a_r` et `W_out=K*a_o`, puis publier leurs pentes
séparées. Pour que `W_point` reste sous `1,35`, le facteur
`a_p(2n)/a_p(n)` ne peut dépasser `2^0.35`, soit environ `1,27456`.

Deux architectures honnêtes sont alors possibles. Si la source **consomme le
`RectFront`**, son ledger doit prouver qu'un point n'est pas rescanné par
chaque rectangle et que le coût est borné par records plus points globaux plus
sortie. Si une source directe est **indépendante et complète** sur tout
l'univers, le front résiduel devient un diagnostic : il ne doit même plus être
construit sur le chemin produit. Une architecture qui calcule les deux puis
ignore le front cumule seulement deux coûts.

La gate décisive sur la famille plane est donc volontairement dissociée :
`residual_pair_mass=Theta(n^2)` peut rester rouge sémantiquement, tandis que
`residual_node_records`, index, visites de points/nœuds, sorties à chaque
étage, octets et aval doivent chacun garder deux pentes `<=1,35` et leurs caps
absolus. Mesurer séparément `W_point`, `W_rect` et `W_out` évite qu'une somme
ou une moyenne cache un étage rouge. Si ces compteurs sont verts et la
complétude reçue, la masse devient **diagnostique non bloquante**. Tant qu'un
fallback, un census ou un consumer peut encore la développer, elle reste
bloquante. S'ils suivent le nombre de rectangles ou rescannent les points, le
mot « par point » masquait seulement un produit relationnel.

Le verdict est donc à deux niveaux : **GO de recherche** pour construire et
tester `RectFront+source` malgré une pente deux de la masse ; **NO-GO produit/G4**
tant que la source complète, le raccord aval, les caps, les pentes physiques et
`BenchmarkOutputContract-v1` ne sont pas reçus. Une famille `K=0` ne suffit pas
à elle seule : il faut en parallèle une fixture `output-bearing` où
`SupportKey`, `BallKey`, census et fold sont non vacants.

Enfin, la prémisse particulière « cette famille produit zéro q3/q4 » est
fausse. La source ne peut donc pas être déclarée linéaire et vide sur la base
du `0 %` du certificat. Elle doit passer la gate de complétude sur les positifs
q3/q4, dont la fixture explicite de la section 9, avant toute rampe.
Cette fixture se translate et se réfléchit dans chaque grille : sans même
chercher un compte optimal, elle donne au moins
`4*123*198*2=194832` supports q3 distincts dans le nuage à `50 k`. Le
qualificatif « zéro » est donc quantitativement faux, pas seulement une
imprécision logique. Ces supports sont internes à un plan : ils ne prouvent
pas que l'owner maximal appartient au rectangle croisé `A times B`. La
proposition plus étroite « zéro sortie pertinente de ce seul rectangle » reste
non prouvée, et exige un théorème d'owner ou un census borné.

La nullité des **témoins universels ponctuels** du nuage sur les paires
croisées peut, elle, être prouvée sans le reçu manquant. La composante axiale
du déplacement vaut `D=60000` et sa composante transverse a une norme
`M<10200`. Pour un site `z` de l'un des deux plans, soit `s` sa différence
transverse non nulle à l'endpoint de ce plan. Alors
si `H<=0`, le témoin est immédiatement rejeté. Sinon
`0<H<=M||s||` et `R>=D^2||s||^2`, d'où
`3H^2<R`, et a fortiori `2H^2<R`; un endpoint donne `H=0`. Cela explique le
`0 %` spindle sans prouver zéro support q3/q4. Ce sont deux propositions
différentes.

La campagne d'adversaires doit séparer les axes que la masse mélange : deux
droites u16 pour `M` dense et sortie q3/q4 nulle ; petites fixtures Chazelle q2
pour une sortie dense, sans en faire un claim `50 k` u16 ; ordre
Morton/checkerboard qui fragmente `F_q` ; nœuds `C` `MIXED` jusqu'aux feuilles ;
`SymmetricAnd` à nombreuses intersections ; étoile à rescans ; cap juste après
split puis reprise. Une fixture `K=0` ne reçoit jamais seule la voie.

La session G4 de Claude est extérieure à cette réponse et son arrêt ciblé a
été corroboré séparément. Cet audit n'a démarré ni muté aucune ressource GCP.
