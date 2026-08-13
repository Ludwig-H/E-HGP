# Audit-réponse — front WSPD, intervalle entier et descente témoin

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Cet audit est strictement documentaire. Aucun code MorseHGP3D n'a été modifié
et aucune VM GCP n'a été créée, démarrée ou arrêtée. GCP non utilisé.

## 1. Pin et verdict court

Le snapshot reçu est :

| objet | pin observé |
| --- | --- |
| `HEAD` | `96be8e0a7f3d49eef8d0406e77609cab61101730` |
| `prototype/rect_front.hpp` | SHA-256 `3785269a07230d35e584db94fd9a805e4bd983d7142488e89299022d412adc3f` |
| `prototype/rect_front_probe.cpp` | SHA-256 `a913e32e216dbab949f9559096cbc7be56f92d0142170a32ec6196717ca778db` |
| `CMakeLists.txt` | SHA-256 `2309042ad0e84f7dd2182aaada765cd54ffcbc0dbb49117734a5edd9d200b6ec` |
| note de Claude `DESCENTE_JOINTE` | SHA-256 `90dacb690822a29c2e5812acfee7398265a92a0f98394ed82c73e03cfe296fda` |
| script de session rectangle-front | SHA-256 `c5069e750d8c1cbee687c62ab5cd4c829827735e91a35dd5794c5e457aae4163` |

Le worktree était propre au dernier contrôle. Le verdict est le suivant :

- **reçu mathématiquement** : l'intervalle entier exact de `H` sur trois AABB,
  le certificat `ALL` q2 et les certificats suffisants `ALL` q3/q4 ;
- **reçu sous hypothèses, comme architecture de front seulement** : arrêter la
  récursion `A×B` à une séparation fixe donne une WSPD de cardinal linéaire en
  dimension fixe ;
- **non reçu** : le prédicat flottant de séparation, la canonicalité du
  split-tree, l'unicité par `PairId`, la descente témoin persistante, les portes
  q3/q4 et toute source consommée jusqu'au fold ;
- **réfuté** : `cred+pending<h_q` ne prouve pas que chaque paire est un support
  q3 ou q4 ;
- **NO-GO G4/SLO** : les portes ciblées rendent `1/4`, le script G4 n'active pas
  le mode WSPD et aucune mesure ne couvre le pipeline complet.

L'idée de Claude « arrêter à bien séparé, pas à fermé » est la bonne correction
de structure. Elle borne le **front de relations** et le travail borné effectué
par ce certificateur. Elle ne borne ni la source générative du résiduel, ni le
nombre de supports, ni `SupportKey -> BallKey -> census -> fold`.

## 2. Intervalle exact de `H` : conclusion correcte, preuve à rectifier

Poser, sur une coordonnée, `f(a,b,z)=(z-a)(b-z)`. Pour `z` fixé, `f` est affine
en `a` lorsque `b` est fixé, puis affine en `b` lorsque `a` est fixé. On peut
donc remplacer successivement `a` puis `b` par une extrémité de leurs
intervalles sans perdre un minimum ou un maximum. Pour `a,b` fixés, `f` est une
parabole concave en `z` :

- son minimum est à une extrémité de `C` ;
- son maximum sur le réseau entier est atteint par un entier voisin de
  `(a+b)/2`, écrêté à `C`.

Les trois axes sont indépendants et les AABB sont des produits cartésiens ; les
trois extrema scalaires s'additionnent. C'est la preuve correcte des valeurs
calculées par `rect_h_interval`.

La justification de la note et du commentaire source par « minimum de
bilinéaires concave, maximum convexe » est fausse si elle est comprise
jointement en `(a,b)` : avec `z=0`, on obtient `-ab`, dont la Hessienne est
indéfinie. L'argument valide est l'affinité **séparée**, coordonnée après
coordonnée.

La portée exacte de `Lambda_max` doit être nommée
`integer_lattice_u16_aabb_envelope`. Elle n'est pas l'enveloppe continue : en
une dimension, `A={0}`, `B={1}`, `C=[0,1]` donnent un maximum entier nul, mais
un maximum continu égal à `1/4`. `Lambda_min`, lui, coïncide aussi avec le
minimum continu car le minimum en `z` reste aux extrémités.

Sous u16, `|H|<3·65535²<2^34`, donc `i64` suffit pour `H`. Les produits
`E2·X2`, `4H²` et `3H²` demandent jusqu'à environ 70 bits ; la promotion avant
multiplication vers deux limbes ou `i128` est obligatoire. Le chiffre de
quarante-huit produits décrit le seul intervalle `H`. Le classifieur q3/q4
courant ajoute deux maxima de distance, soit vingt-quatre carrés, puis les
produits larges : environ soixante-quinze multiplications logiques, et non
quarante-huit.

Fixtures déterministes à garder :

- `A={0}, B={2}, C=[0,2]` : les extrémités donnent zéro mais `z=1` donne un ;
- `A={0}, B={2}, C={10}` : le sommet non écrêté surestime le maximum ;
- `A={0}, B={1}, C=[0,1]` : distinction réseau entier/continu ;
- milieu impair, milieu hors `C`, égalité `H=0` et extrêmes u16.

## 3. Les trois lanes : ce que les bornes décident réellement

Pour un triple ponctuel, poser `E2=||z-a||²`, `X2=||b-z||²` et
`H=(z-a)·(b-z)`. L'identité de Lagrange donne
`R=E2·X2-H²`. Les prédicats universels reçus sont donc :

$$q3: H>0\text{ et }4H^2>E2\,X2,\qquad q4: H>0\text{ et }3H^2>E2\,X2.$$

Ainsi les tests courants sont sûrs :

- q2 `ALL` si `Hmin>0` ;
- q3 `ALL` si `Hmin>0` et `4Hmin²>E2max·X2max` ;
- q4 `ALL` si `Hmin>0` et `3Hmin²>E2max·X2max`.

Ils sont seulement suffisants. Par exemple, en une dimension embarquée dans
3D, `A={0}`, `B={10}`, `C=[1,9]` est entièrement collinéaire : chaque point est
universel q3/q4, alors que le produit indépendant des deux maxima peut laisser
le code en `MIXED`.

Le `NONE` courant des lanes q3/q4 est uniquement le sous-cas q2
`Hmax<=0`. Il est sûr parce que `W4` est inclus dans `W3`, lui-même inclus dans
la boule diamétrale, mais il est incomplet. Une extension peu coûteuse est
disponible. Poser `U=max(Hmax,0)`, et noter `LE` et `LX` les minima exacts des
distances carrées entre `A,C` et `B,C`. Alors :

$$4U^2\leq LE\,LX\Longrightarrow NONE_{q3},\qquad 3U^2\leq LE\,LX\Longrightarrow NONE_{q4}.$$

L'égalité rend bien `NONE`, puisque les spindles sont ouverts. Une variante
plus corrélée emploie un minorant exact `Rlb` : `3U²<=Rlb` pour q3 et
`2U²<=Rlb` pour q4. Tout échec de ces tests reste `MIXED/UNKNOWN`.

L'ABI ne doit pas accepter un `int lane` quelconque et traiter silencieusement
toute valeur autre que zéro ou un comme q4. Un enum fermé et validé est requis.

Mutants minimaux q3/q4 : coefficients `4/3`, `>` changé en `>=`, oubli de
`H>0`, produit i64, confusion de lane et point exactement sur chaque frontière.
Le juge courant compare seulement `Hmin/Hmax` ; il ne juge aucun verdict q3/q4.

## 4. Correction P0 : le majorant est sain, l'étiquette `POSITIVE` ne l'est pas

À chaque instant, les nœuds `ALL`, les nœuds `MIXED` encore en file et les
feuilles `MIXED` forment, avec les nœuds `NONE`, une partition du domaine
témoin. Par conséquent `cred+queued+stuck` est bien un majorant du nombre de
témoins universels possible pour chaque paire du rectangle, à condition que la
partition soit authentifiée et que les crédits portent des `PointId` disjoints.

La conclusion dépend toutefois de la lane :

- pour q2, sous positions d'endpoints distinctes,
  `cred+queued+stuck<h_2` prouve que chaque paire possède moins de dix
  intérieurs diamétraux. La paire elle-même est alors un support q2 de rang
  pertinent ;
- pour q3/q4, la même inégalité prouve seulement que la paire **n'est pas
  éliminée par ce certificat universel** comme arête maximale candidate. Elle
  ne fabrique ni troisième/quatrième site affine indépendant, ni support bien
  centré, ni positivité, ni owner.

Un nuage collinéaire est le mutant décisif : il peut laisser moins de neuf ou
huit témoins universels autour d'une paire, mais il ne contient aucun triangle
ou tétraèdre propre. L'issue q3/q4 doit donc s'appeler par exemple
`KEEP_ANCHOR` ou `DELEGATED_TO_SOURCE`, jamais `POSITIVE_SUPPORT`. Cette
correction est conforme à l'inégalité déjà documentée `L_q<=U_q` : survivre à
un certificat universel ne prouve pas l'existence d'un support.

Même en q2, « toutes les paires sont des supports » reste une décision de
source, pas un payload Morse complet. Les `SupportKey` doivent encore être
émises de façon canonique et consommées jusqu'au census et au fold.

Le budget courant présente en outre un off-by-one : après une classification
de racine, une itération peut commencer avec une unité restante puis classer
deux enfants. `budget=24` autorise alors vingt-cinq classifications. La porte
doit vérifier `evals_per_rect<=budget` ou définir explicitement un quantum
d'expansions distinct d'un quantum de classifications.

## 5. WSPD : borne reçue pour le front, pas pour le pipeline

La décomposition classique de Callahan--Kosaraju couvre chaque paire de points
distincts exactement une fois avec `O(s^3 n)` paires de sous-ensembles en
dimension trois, pour une séparation `s` fixe et un fair-split tree. C'est une
base valide pour le `RectFront` ; voir
[Callahan et Kosaraju](https://doi.org/10.1145/200836.200853).

La récursion du snapshot devient seulement **WSPD-style** lorsque
`--stop-wsp>0` : elle scinde le bloc de plus grand rayon et s'arrête au premier
rectangle bien séparé, même si le certificateur ne le ferme pas. Les arrêts
`CLOSED` antérieurs ne peuvent qu'en réduire le nombre. Les microblocs
self-feuille restent à traiter séparément.

La réception industrielle demande encore :

1. un split-tree canonique avec `PointId` comme dernier tie-break ;
2. une politique explicite des positions dupliquées et plateaux de rayon nul ;
3. un prédicat de séparation entier ou rationnel ;
4. une preuve structurelle et un oracle borné de multiplicité un par `PairId` ;
5. le paramètre `s`, la convention de séparation et les digests dans chaque
   record et reçu.

L'identité `sum(pair_mass)=C(n,2)` est nécessaire mais insuffisante : une
omission et un doublon de même masse peuvent se compenser. Le sujet courant
réordonne en outre les coordonnées sans conserver de `PointId`, de sorte qu'il
ne peut pas encore produire ce témoin d'unicité.

Le prédicat courant en `double/sqrt` n'altère pas la vérité géométrique des
résultats déjà certifiés : une séparation ne fait que déléguer un résidu. Il
altère cependant les `RectId`, le déterminisme CPU/device et les hypothèses de
la borne. Pour `s=p/q`, un test entier conservateur simple emploie les centres
d'AABB doublés. Si `D2` est leur distance carrée et
`R2=max(sum(width_A²),sum(width_B²))`, alors :

$$q^2D2\geq(p+2q)^2R2$$

implique `d-r_A-r_B>=s·max(r_A,r_B)`. Ce test peut perdre des séparations quand
les rayons diffèrent, mais ne peut pas en inventer ; ses produits tiennent
aisément dans deux limbes sous u16 avec `p,q` bornés.

Le théorème du front ne dit rien des coûts suivants :

- la construction du split-tree ;
- la descente témoin `A×B×C` ;
- la source exacte du rectangle résiduel ;
- les sorties réellement produites ;
- `SupportKey -> BallKey -> census -> fold`.

Le nombre de records WSPD peut être linéaire alors que la somme des populations
de leurs blocs, ou un rescan de membres par record, est quadratique. Il ne faut
donc jamais scanner `A` ou `B` pour chaque rectangle.

## 6. Ce n'est toujours pas une descente jointe persistante

Pour chaque rectangle `A×B`, `witness_outcome` construit une nouvelle
`priority_queue`, remet `C=root`, remet les crédits à zéro puis abandonne son
état au retour. Une scission de `A` ou `B` répète ce préfixe témoin chez les
enfants. Le budget borne chaque répétition, mais ne constitue ni une
continuation ni une descente jointe.

L'invariant persistant recommandé est une antichaîne disjointe par rectangle et
par lane :

`Credit_R,q ⊔ None_R,q ⊔ Mixed_R,q = C-root`.

`Credit` contient les nœuds `ALL`, `None` les nœuds `NONE` et `Mixed` la
frontière à reprendre. Raffiner `C` remplace exactement un nœud par ses enfants.
Lorsque `A×B` est scindé, `ALL` et `NONE` hérités restent valides par monotonie
sur un sous-rectangle ; seuls les nœuds `Mixed` sont reclassifiés. Aucun enfant
ne repart de la racine.

ABI conceptuelle minimale :

`WitnessContinuation{CloudDigest,TreeDigest,Epoch,RectKey,lane_mask,mixed_front,credit_ranges,none_digest,heap_cursor,eval_count,wide_product_count,heap_nodes,bytes_hwm}`.

États : `CLOSED`, `PENDING_CONTINUATION`, `DELEGATED_RESIDUAL` et
`RESOURCE_EXHAUSTED`. Un quantum de travail rend une continuation du même état ;
il ne modifie jamais la sémantique. Sur GPU, l'alternative est une wavefront
globale `(RectKey,CNodeKey,lane_mask)` et la porte impose qu'aucun tuple
`(RectKey,CNodeKey,lane,version)` ne soit évalué deux fois.

Cette persistance réduit les reprises, mais ne prouve pas à elle seule une
borne linéaire du triple join. Elle doit publier `unique_tasks`,
`repeated_tasks`, héritages, reclassifications, pushes/pops et HWM.

## 7. Rejeu local : progrès de structure, constante encore trop haute

Le build Release ciblé a produit l'ELF
`mhgp3v_rect_front_probe`, SHA-256
`a59ff2567c9e1eadb942e00a5385aba1d03666d553f9ec6088fb1b29e3a09ab8`.
Avec `s=2`, `budget=24`, `leaf=8`, sans selftest dans le chrono diagnostique :

| lane / taille | rectangles visités | terminaux physiques | classifications `A/B/C` | masse fermée |
| --- | ---: | ---: | ---: | ---: |
| q2 / `2 000` | `48 460` | `24 408` | `1 187 226` | `16,11 %` |
| q2 / `8 000` | `272 602` | `137 007` | `6 696 445` | `34,79 %` |
| q3 / `8 000` | `275 378` | `138 395` | `6 813 725` | `0 %` |
| q4 / `8 000` | `275 378` | `138 395` | `6 813 725` | `0 %` |

La pente q2 des visites entre `2 k` et `8 k` vaut `1,246`, mais une seule pente
n'est pas une gate. Les trois lanes séparées totalisent `20 323 895`
classifications à `8 k`, soit environ `2 540` par point. Une extrapolation
strictement linéaire de ce seul palier donne environ `127 M` classifications à
`50 k`, avant source et aval. Ce n'est ni un temps ni une prédiction G4 ; c'est
un signal que le premier gain de constante doit être une wavefront commune aux
trois lanes, et non trois traversées.

Les quatre CTests ciblés ont rendu `1/4` :

- le test positif q2 passe ;
- les deux mutants et le refus de domaine exécutent bien les codes `4/4/2`,
  mais échouent dans le harnais ;
- les appels CMake omettent l'argument regex attendu par
  `mhgp3v_add_expected_code_test_for`, donc `--family=uniform` devient par erreur
  le code de sortie contractuel.

La gate de pente calcule et imprime `rect_visites`, mais ne refuse que selon la
masse résiduelle. Avec deux tailles seulement, son compteur exigeant deux
pentes consécutives ne peut jamais déclencher. En mode WSPD, c'est en outre la
mauvaise grandeur : la masse résiduelle peut être quadratique tandis que les
records et évaluations restent linéaires.

## 8. Réponse à `RESOURCE_CAP` : ni faux terminal exact, ni retry sans état

Un cap de diagnostic peut terminer **ce certificateur** et produire
`DELEGATED_RESIDUAL`. Il ne ferme pas le rectangle et ne qualifie pas une
sortie exacte. Le chemin produit doit ensuite :

- soit reprendre la même continuation jusqu'à décision ;
- soit déléguer le rectangle à une source exacte complète ;
- soit échouer atomiquement `resource_exhausted` sur une ressource réelle.

Scinder puis recommencer depuis `C=root` n'est pas une solution industrielle :
le même travail est répété sans borne globale. Un budget configurable qui
change la sémantique est interdit ; un quantum fixe d'ordonnancement est permis
si le résultat est invariant au quantum et au tie-break.

L'arrêt WSPD donne une réponse partielle élégante : un rectangle non décidé
mais bien séparé devient `DELEGATED_RESIDUAL`. Il borne le fast path. Il ne rend
pas gratuite la source déléguée.

## 9. Proposition mathématique prioritaire pour réduire la constante G4

Les seuils ne sont que `10/9/8`. Une DFS complète du nuage témoin par rectangle
est donc disproportionnée. La voie recommandée est une **banque bornée de
témoins proposée en lot, puis recertifiée exactement**.

### 9.1 Cœur commun centré au milieu des blocs

Supposer `A,B` contenus dans des boules de centres `c_A,c_B` et de rayons
`r_A,r_B`. Poser `S=r_A+r_B`, `d=||c_B-c_A||` et
`m_0=(c_A+c_B)/2`. Pour toute paire `a∈A,b∈B`, son milieu est à distance au plus
`S/2` de `m_0` et `||b-a||>=d-S`.

Il en résulte deux cœurs exacts :

$$d>2S\Longrightarrow B^\circ\!\left(m_0,\frac{d-2S}{2}\right)\subset B^\circ\!\left(\frac{a+b}{2},\frac{||b-a||}{2}\right).$$

Pour q3/q4, lorsque `ab` est bien l'arête maximale owner du support candidat,
la boule ouverte de rayon `||b-a||/4` autour de son milieu est contenue dans
toute circumboule admissible. Donc :

$$d>3S\Longrightarrow B^\circ\!\left(m_0,\frac{d-3S}{4}\right)\subset B^\circ(c_U,\rho_U).$$

Dix `PointId` distincts dans le premier cœur ferment q2 ; neuf ou huit dans le
second ferment respectivement q3 ou q4 sous la précondition d'owner maximal.
Les comparaisons de rayon se font en carrés entiers avec frontière fail-open.

Cette route ne demande pas d'augmenter globalement `s` vers douze. La constante
« environ douze » de la note WSPD concernait un autre certificat, avec un nœud
témoin de rayon `rho`; la WSPD `A/B` ne borne pas ce `rho`. Garder `s=2` ou une
petite valeur fixe, tester le cœur réel de chaque rectangle et ne raffiner que
les échecs évite l'explosion `s³`.

### 9.2 Banque top-`L` bichromatique

Pour chaque rectangle du front commun :

1. former le centre rationnel `m_0` ;
2. obtenir en lot les `L` points les plus proches, avec `L` petit et fixe
   (`16` pour commencer, puis ablation `12/16/24/32`) ;
3. tester chaque `PointId` une seule fois contre les trois certificats exacts
   du rectangle ;
4. fermer une lane dès que son seuil `10/9/8` est atteint ;
5. déléguer tout échec, sans aucune conclusion négative.

La recherche top-`L` ne porte aucune autorité scientifique : elle propose des
IDs. Même une banque incomplète ne peut créer de faux positif, car chaque ID est
recertifié sur tout `A×B`; elle peut seulement perdre une fermeture. Une banque
exacte permet en plus de décider rapidement l'occupation des cœurs communs.

Une construction possible est d'ajouter les `O(n)` centres rationnels du front
comme couleur requête et d'adapter l'algorithme all-kNN de la WSPD au problème
bichromatique. La borne `O(L(n+F))` après construction, pour `F=O(n)`, est une
**obligation de preuve**, pas encore un résultat reçu ici. L'alternative
immédiate est une wavefront LBVH multi-requêtes mesurée, sans claim de pire cas.

Sur G4, cette forme possède le bon grain : SoA de centres, top-`L` en registres
ou petit scratch, un masque de lanes, arithmétique deux limbes seulement à la
recertification et aucun `priority_queue`/malloc par rectangle. Le même lot
alimente q2/q3/q4.

### 9.3 Fallback persistant seulement sur les ambiguïtés

La banque ferme les cas denses et le cœur commun. Les rectangles restants
entrent dans la continuation `Credit/None/Mixed`, avec le `NONE` spécifique de
lane de la section 3. La source générative intervient seulement après ces deux
étages et reçoit un `RectRecord` authentifié, jamais une expansion de toute sa
masse en `PairId`.

## 10. Ordre concret remis à Claude

Avant toute nouvelle session G4 :

1. corriger le claim q3/q4 `POSITIVE_SUPPORT` en `KEEP_ANCHOR` et ajouter le
   mutant collinéaire ;
2. réparer les trois appels du harnais, le budget `24->25`, puis ajouter un juge
   indépendant par lane et par `PairId` ;
3. rendre le mode WSPD explicite et obligatoire dans la gate, avec séparation
   entière, `PointId`, politique des doublons et `RectKey` canonique ;
4. produire un front unique avec masque q2/q3/q4 ;
5. mesurer la banque bornée `top-L + common-core`, puis la continuation
   persistante seulement sur son résiduel ;
6. raccorder dans le même jalon au moins une tranche régulière
   `SupportKey -> BallKey -> census -> fold` ;
7. seulement alors mesurer `12 500/25 000/50 000` sur les quatre familles, avec
   deux pentes de `front_records`, visites uniques, classifications, source,
   sorties, octets et HWM, puis le vrai `warm_e2e`.

Le script G4 commis ne doit pas être lancé en l'état : il omet `--stop-wsp`,
mesure le CPU de la G4, compile une autre cible CUDA, masque plusieurs échecs
par des pipelines vers `tail` sans `pipefail` dans le shell distant et neutralise
un échec de `stop_and_verify` par `|| true`. Ce dernier point viole le passage de
relais fail-closed : un arrêt ciblé illisible doit bloquer, jamais être avalé.

## 11. Non-claims

Cet audit ne reçoit aucun temps G4, aucun p95, aucun octet/HWM, aucun producteur
de `BenchmarkOutputContract-v1` et aucune hiérarchie complète. Il reçoit un
outil mathématique utile et un front conditionnellement linéaire ; il refuse de
transformer cette borne locale en claim pipeline. Le contrat `50 000` sous une
seconde et, a fortiori, sous `100 ms`, reste ouvert.
