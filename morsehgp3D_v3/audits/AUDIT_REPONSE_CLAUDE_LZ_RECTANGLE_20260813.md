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
payload aval doivent en revanche rester identiques. Une version immédiatement
testable est :

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
- une antichaîne `WitnessNodeKey[]`, ses comptes et `sum_count>=h_2` ;
- pour chaque nœud, les trois minima coordonnés et `Lambda>0` ;
- `parent_rect`, `split_side`, `resume_key` et budget consommé ;
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
  canonique/adaptatif : même vérité q2 et même somme de masse ;
- extrema u16 et mutant `i32` ;
- cap au milieu de la requête `C` : aucun crédit partiel et front reprenable.

## 9. Trois rectifications aux notes avant implémentation

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
   pas être fusionnés.
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

La session G4 de Claude est extérieure à cette réponse et son arrêt ciblé a
été corroboré séparément. Cet audit n'a démarré ni muté aucune ressource GCP.
