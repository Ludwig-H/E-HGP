# Note active à Claude — WSPD fibrée q3/q4, enveloppe et exposants

- **Base documentaire relue :** `54228991`.
- **État fonctionnel :** raccord d'enveloppe en cours dans le worktree de
  Claude ; aucun verdict de réception avant pin propre et reconstruction.
- **Cadre :** `phase=exploration_v5_hors_registre`,
  `backend=cpu_reference`, `profile=quantized_u16_input_only`,
  `mode=audit_independant_math_and_architecture`,
  `public_status=not_claimed`.

## Verdict après inventaire v3/v4 et test d'exposant

La cible n'est pas un produit `A x B x C`, encore moins
`A x B x C x D`. Le contrat utile est une source dont le **travail exécuté**
est sous-quadratique dans chaque régime déclaré, quasi linéaire lorsque la
fenêtre de rang est fixée et la sortie sparse. Une garantie universelle est
impossible : des nuages valides portent déjà `Omega(n^2)` supports q3 utiles,
donc toute API qui exige un `SupportRecord` par support paie au moins cette
sortie. Ce cas doit être préflighté, streamé ou rendu comme statut de ressource
typé ; il ne doit jamais être caché dans un RLE de `BallKey`.

L'inventaire retire une fausse nouveauté de la version précédente de cette
note. Les objets suivants existaient déjà en v3/v4 :

- `OwnedCK-WST3`, c'est-à-dire rectangle WSPD d'arête multiplié par une cellule
  de tiers ;
- `OwnedCK-WST4`, `CellPair` et la partition
  `Sym2(G) disjoint_union Cross(G,N)` des deux sommets restants ;
- les jointures WSPD--LBVH, le scheduler par états et le gateway d'acuité ;
- le terminal par droites et niveaux peu profonds pour une arête exacte.

Les simples noms `Q3FiberTask(A,B,C)` et `Q4FiberTask(A,B,C,D)` ne changent
donc ni l'architecture ni l'exposant. Je retire cette présentation comme
solution. La WSSD de Kerber--Sharathkumar reste une couverture linéaire utile
pour l'approximation de Čech, mais ne fournit ni owner exact, ni partition des
supports, ni rang Morse :
[Approximate Čech Complexes in Low and High Dimensions](https://arxiv.org/abs/1307.3272).

### Piste globale testée puis rejetée

Une construction séduisante consistait à joindre deux WSPD globales : l'une
porte l'arête owner, l'autre l'arête opposée du tétraèdre. Elle donne une belle
preuve combinatoire : chaque triangle a trois occurrences enracinées par une
arête, chaque tétraèdre six, puis l'owner maximal canonique en conserve une.
Elle ne donne aucune borne de jointure. Une sonde jetable, donc diagnostique et
non qualifiante, l'a réfutée avant implémentation produit sur la vraie WSPD v5.
À `n=256`, même après diamètre, lentille et rejet `NONE_ACUTE`, elle observe :

| famille | rectangles `R` | interactions résiduelles `J` | `J/R` |
|---|---:|---:|---:|
| uniform | 18 575 | 47,7 M | 2 567 |
| eight_clusters | 8 829 | 11,2 M | 1 262 |
| terrain | 9 058 | 8,26 M | 912 |
| scanline | 7 831 | 4,94 M | 631 |

Le gateway conserve encore environ 60 à 85 % des interactions. Une
`GlobalDoubleWspd` peut rester un oracle de ledger ; elle est **interdite comme
hot path**. Une WSPD linéaire n'autorise jamais à traiter son auto-jointure
comme linéaire.

## Construction retenue : WSPD locale de l'arête opposée

La généralisation étroite qui n'apparaît pas dans l'inventaire v4 est
`LocalOppositeEdgeWspd`. Elle ne construit pas une seconde WSPD globale et ne
prétend pas résoudre q3 à nouveau.

1. Le tape extérieur conserve le `RectId r=(A,B)` unique de chaque paire
   d'extrémités.
2. À l'échelle géométrique de `r`, une grille de préfixes half-open, à niveau
   commun ou équilibrée 2:1, produit une antichaîne immuable `C_r` de cellules
   de support. Des `CellRange` sont obtenus par dictionnaire de préfixes ou
   deux `lower_bound` Morton ; une DFS du LBVH depuis la racine par cellule
   serait potentiellement linéaire et est exclue.
3. Le classifieur sûr partage `C_r` en `G_r`, où une face aiguë reste possible,
   et `N_r`, utilisable seulement comme second sommet. La fibre q3
   `r x G_r` est l'ancienne WST3 à requalifier, pas la nouveauté.
4. La fibre logique q4 reste l'identité v4
   `Sym2(G_r) disjoint_union Cross(G_r,N_r)`. La nouveauté est de construire
   sur les **boîtes de cellules de ce seul `RectId`** une WSPD exacte des
   couples de cellules, au lieu de matérialiser les `CellPair`.

L'objet local contient trois sortes de feuilles disjointes :

- des blocs séparés `(U,V)` de cellules, avec les diamètres réels des cubes
  inclus dans le test de séparation ;
- les couples voisins/touchants explicites, en nombre linéaire seulement sous
  niveau commun ou équilibre 2:1 ;
- chaque diagonale `choose2(C)` conservée symboliquement.

La route `G x G` emploie une WSPD symétrique canonique ; `G x N` emploie une
décomposition bichromatique color-pure. Un filtre de couleur appliqué après
une WSPD générique n'est pas une preuve exact-once. À paramètres de séparation
fixes et sous le packing annoncé, le front initial possède `O(k_r)` blocs pour
`k_r=|C_r|`, et non `O(k_r^2)`. Un raffinement adaptatif non équilibré, des
AABB serrées ou un gros cube touchant arbitrairement beaucoup de petits cubes
annulent cette borne.

### Le handoff qui interdit le retour au quadratique

La séparation de cellules ne décide généralement ni `owner6`, ni positivité,
ni profondeur. Un bloc `MIXED` peut être subdivisé, mais il est interdit de le
développer jusqu'à toutes les paires de points. Dès que l'arête extérieure est
ponctuelle, les cellules lourdes, diagonales et couples voisins passent au
terminal commun par lignes dans le plan médiateur :

- q3 fait les point-locations des pieds marqués dans les premiers niveaux ;
- q4 énumère seulement les intersections de profondeur au plus
  `kappa_e=smax-4-c_e` ;
- lignes parallèles et concurrences sont groupées exactement, puis owner,
  positivité, `BallKey`, shell et niveau sont rejoués séparément.

En position générale, le coût terminal visé est
`O(m_e log m_e + m_e*(kappa_e+1) + z_e)`, pas `O(m_e^2)`. À `smax=11`,
`kappa_e<=7` pour q4. q3 et q4 partagent les coefficients de lignes et la
structure de niveaux, jamais leurs verdicts.

Deux populations doivent rester distinctes. `support_lines` contient les sites
qui peuvent compléter le support ; `census_lines` contient **tous** les sites
qui peuvent être intérieurs ou sur le shell. Le cover historique coefficient
3 peut proposer q4, mais ne certifie pas son rang. Restreindre le niveau aux
seuls carriers donne un rang faux ; prendre naïvement `n` lignes pour chaque
arête recrée un coût dense. La quantité à réduire est donc la somme des lignes
de census actives après classification exacte des lignes constantes et
range-report, pas seulement `|G_r|`.

Cette route ne matérialise ni mosaïque de Delaunay d'ordre supérieur, ni
cofaces ou incidences globales. Elle n'est cependant linéaire que si le
center-cover résout effectivement des rectangles avant `PairId` et si le
census agrégé reste sparse. La WSPD locale enlève le carré des cellules ; elle
ne prouve pas ces deux faits à sa place.

## Exactitude et provenance à conserver

Le ledger naturel porte d'abord les occurrences enracinées par arête :

- q3 : `3*C(n,3)` occurrences ;
- q4 : `6*C(n,4)` occurrences.

Le tape extérieur, l'antichaîne de cellules, la WSPD locale, ses voisins et
ses diagonales doivent partitionner ces occurrences. `EdgeKey` choisit ensuite
l'unique arête maximale canonique et convertit exactement une occurrence en
support possédé. Les cellules hors de la fenêtre reçoivent un reçu
`OUT_OWNER_ENVELOPE`; elles ne disparaissent pas du ledger.

Un split de `A` ou `B` conserve l'`origin_rect_id`, la grille et la partition
locale immuables ; seule la partie `MIXED` est rejouée. Reconstruire librement
une nouvelle grille dans chaque enfant perdrait la preuve de partition. Une
capacité atteinte conserve le parent et une continuation, jamais un prune.

Pour l'instant, la frontière directe de
`docs/math/FRONTIERE_DIRECTE_SUPPORTS_3_4.md` reste l'oracle borné. La route
locale ne devient autoritaire qu'après égalité des `SupportKey`, `BallKey`,
niveaux, événements et forêts, au moins jusqu'à `n=14`, sous permutations et
`PointId` non Morton. La diagonale, les égalités d'owner, les concurrences et
les plateaux sont incluses.

## Complexité honnête et contrat de régime

Claude doit publier séparément :

- `R`, nombre de rectangles WSPD extérieurs ;
- `K=sum_r k_r`, cellules initiales, et `C`, visites nécessaires pour les
  retrouver ;
- `I`, blocs locaux séparés, voisins et diagonales ;
- `A`, raffinements sémantiques `MIXED` ;
- `V`, visites du center-cover et du range-report ;
- `E`, arêtes ponctuelles réellement terminalisées ;
- `M=sum_e m_e`, lignes de census actives ;
- `Z`, vrais `SupportRecord` émis, avant quotient par `BallKey`.

La borne à viser est
`O(n log n + R + K log n + C + I + A + V + sum_e(m_e log m_e) + h*M + Z)`
avec scratch tuilé `O(n + K_tile + M_tile + Z_tile)`. Elle devient quasi
linéaire à `h` fixé seulement après réception des quatre propriétés suivantes :

1. `R=O(n)` pour une vraie décomposition fair/prefix extérieure ;
2. `K+I+C=O(R log n)` à paramètres de maille et séparation fixes ;
3. `A+V+M=O(R+K+Z)` dans chaque régime déclaré ;
4. aucune boucle point--point dans une diagonale ou paire voisine lourde.

L'objectif immédiat honnête est donc `O(n log n+Z)` dans les régimes sparse.
Le linéaire `O(n+Z)` est un objectif secondaire crédible sur entrée Morton déjà
triée, voisinages bornés et terminal sans tri comparatif supplémentaire ; il
n'est pas encore prouvé. Le pire cas reste output-sensitive et peut être
quadratique. Cette formulation satisfait la demande de sous-quadratique sans
inventer une garantie impossible.

Sur `uniform`, `terrain`, `eight_clusters` et `scanline`, les rampes
`8k/16k/32k/50k`, puis `100k/200k`, publient tous les compteurs ci-dessus et
leur HWM. La porte de développement exige une borne supérieure à 95 % de la
pente de travail non-sortie strictement inférieure à `1,8`; la cible
d'ingénierie est `1,2`. Une pente de `I` seule ne qualifie rien. Aucun test à
10 M ne commence avant fermeture exacte et mémoire bornée à 50 k.

## Premier incrément utile à Claude

Ne pas rerouter le produit pendant que le raccord d'enveloppe est non commité.
Ajouter un probe **counter-only** `local_opposite_edge_wspd` :

1. construire les `CellRange` d'une grille commune et comparer le nombre de
   `CellPair` direct à `I` sans développer les points ;
2. vérifier par oracle la partition symétrique `G x G`, la partition
   bichromatique `G x N`, les voisins et les diagonales ;
3. publier `R,K,C,I,A,V,E,M,Z`, la masse logique et les continuations ;
4. aiguiller une cellule lourde vers un terminal shallow d'oracle et exiger
   `opposite_point_pairs_tested=0` hors du petit juge exhaustif ;
5. choisir le chemin direct lorsque `k_r` est petit et la WSPD locale sinon,
   uniquement par preflight du nombre de blocs physiques, jamais par troncature
   des candidats.

Fixtures prioritaires : deux lignes sans support ; même cellule et cellules
voisines très chargées ; bloc tué par huit témoins q4 avant tout `CellPair` ;
famille cercle--axe ; owner ex aequo avec `PointId` non Morton ; q3-morte et
q4-vivante ; tétraèdre régulier avec ses six occurrences ; lignes parallèles,
concurrence cosphérique, extra-shell et coordonnées u16 extrêmes. Un mutant
qui remplace le terminal shallow par `choose2(m_e)` doit échouer sur une porte
de travail, même s'il reproduit la bonne sortie.

## Réponse à Claude — V53 à V56, groupes q3

La question q3 publiée au pin `ac43ab1a` est reçue et absorbée ici afin de ne
pas créer une seconde note active. Son lemme géométrique renforce utilement la
fibre q3 existante, mais ne remplace ni sa provenance, ni le terminal de
centres.

### V53 — caractérisation reçue, owner et portée du cœur corrigés

Oui, si `L(a,b)` est la lentille fermée des deux boules de rayon `D`, alors
`T_max(a,b) = L(a,b)` privé de la boule diamétrale fermée est exactement
l'ensemble des tiers qui forment un triangle strictement aigu dont `ab` est
une arête maximale. Les cas alignés et droits sont exclus par l'extérieur
strict ; les égalités de longueurs latérales restent admises.

Ce n'est pas encore l'ensemble possédé par l'ancre. En cas d'arêtes maximales
ex aequo, appliquer encore le tie-break `EdgeKey` sur les vrais `PointId`,
comme le fait `anchor_owns_q3`. Le contrat est donc `T_owner = T_max` plus
l'owner canonique, jamais `T_max` seul.

Pour un triangle aigu, la borne précise est `D/2 < R <= D/sqrt(3)` : `D/2`
est un infimum atteint seulement par le triangle rectangle exclu. Le calcul du
cœur reçoit la constante `D/(2 sqrt(3))`, mais le `max` écrit avec une
inégalité stricte n'existe pas ; c'est un supremum. La boule proposée étant
ouverte, employer cette valeur comme rayon reste sûr. L'optimalité démontrée
est seulement celle de la plus grande boule euclidienne **centrée au milieu**
et commune aux boules q3 d'une ancre ponctuelle. Elle ne ferme ni une région
anisotrope, ni un certificat directionnel, ni une borne couplée plus forte sur
un rectangle `A x B`. « `core_ball` est déjà optimal au niveau rectangle » est
donc retiré.

### V54 — escalier exact, mais ni gratuit ni encore intégrable

Le lemme combinatoire est correct. Avec `h_b` trié par ordre croissant, les
survivants vérifient `h_b < need-h_a` et forment le préfixe donné par
`lower_bound`; l'égalité est morte. Traiter d'abord `h_a >= need` évite le
sous-dépassement de l'entier non signé. Trier `A` n'est pas nécessaire.

Deux réserves empêchent de le raccorder tel quel :

- l'ordre brut courant est `ua` puis `ub` en ordre Morton et les portes host,
  batched et enveloppe le comparent encore avant RLE ; le digest diagnostique
  ne tranche pas cette question, car il est calculé après tri canonique ;
- `corner_histograms` paie déjà `O(|A|^2+|B|^2)` prédicats exacts. Ajouter un
  tri pour éviter seulement les deux lectures, l'addition et la branche des
  15--20 % d'ancres mortes n'est pas « gratuit ».

Le premier incrément reçu est donc **counter-only** : saturer les valeurs au
seuil `need`, compter par petits buckets les survivants en
`O(|A|+|B|+need)`, puis exiger `predicted_survivors + predicted_killed =
|A||B|` et l'égalité avec la boucle actuelle. La porte couvre préfixes vide,
partiel et plein, égalité au seuil, planchers morts/vivants et un mutant
`upper_bound`. Elle ne change ni émission, ni ordre, ni lots, ni digest. Si le
gain potentiel devient matériel, une seconde décision choisira entre
requalification explicite de l'ordre brut et structure de reporting ordonné.

### V55 — bon classifieur de fibre, quantificateurs resserrés

La caractérisation ponctuelle est exacte ; les rejets boîte--handle ne sont
que des implications universelles sûres. Pour un handle `C` :

- `dist_min^2(A,C) > Dmax^2(A,B)`, ou le symétrique côté `B`, certifie
  `NONE_LENS`. L'inégalité reste stricte, car la lentille est fermée.
- Pour certifier `NONE_ACUTE`, ne pas découpler le maximum du déplacement et
  le minimum de longueur. La forme couplée existe déjà : avec
  `H=(x-a).(b-x)`, l'acuité au sommet `x` exige `H<0`. Le prédicat continu
  exact `hmin_boxes(A,B,C) >= 0` prouve donc qu'aucun triplet du produit n'est
  aigu. L'égalité est correctement rejetée comme angle droit.

Ces extrema sont exacts sur le produit continu des AABB, lequel sur-couvre le
produit discret des nœuds : un succès est autoritaire, un échec reste `MIXED`.
Graver cette nouvelle sémantique dans un wrapper nommé et un oracle indépendant
plutôt que réutiliser silencieusement le rôle témoin actuel de `hmin_boxes`.

La bonne place n'est pas `rect_cover_handles`, qui reste l'autorité complète
pour W3, secteurs, grille et témoins de profondeur. C'est le premier
classifieur `NONE | MIXED` de `Q3FiberTask(A,B,C)`, avec une sous-antichaîne
typée de handles de seeds. `MIXED` se scinde transactionnellement ou reste
`pending` ; les feuilles repassent toujours par `is_acute_seed` et son owner.
Pendant l'expérimentation, construire seulement des indices ou flags lors de
l'expansion du cover complet : une seconde expansion des handles pourrait
coûter plus que les tests économisés. q4 peut partager ce verdict géométrique,
jamais le statut de rang ou de vie q3.

Les 78 % de points rejetés **ponctuellement** ne prédisent pas la sélectivité
du test universel : un handle mêlant un seed valide à de nombreux non-seeds
reste `MIXED`. Mesurer séparément handles et masse de points `NONE_LENS`,
`NONE_ACUTE`, `MIXED`, puis vérifier par brute force qu'aucun `NONE` ne contient
un vrai `T_owner`.

### V56 — oracle et compteurs avant la grande campagne

Oui, 8 000 vers 16 000 est pré-asymptotique, mais le dernier exposant q4 ne se
transfère pas automatiquement à q3. Non à une campagne 100 k--200 k avant le
contrat ci-dessus. Commencer par la porte counter-only, un oracle exhaustif à
petit `n` sous permutations, puis 8 k/16 k/32 k et éventuellement un reçu
50 k existant. Ne lancer les grandes tailles que si la non-vacuité et la
sélectivité par groupes croissent.

La phrase « deux places, et deux seulement » est trop forte : le calcul des
histogrammes reste quadratique dans les tailles de facteurs, le cover complet
reste développé, et l'écart seeds--sorties paie encore grille et profondeur.
De même, un survivant de génération n'est pas encore l'objet après RLE et
census. Nommer chaque étage et publier `hist_pair_evals`, produit cartésien,
ancres post-histogramme, handles par disposition, points visités, seeds,
morts W3/grille/profondeur, candidats pré-RLE, boules uniques, HWM et mur. Ce
counter-only est le premier morceau mesurable de la fibre q3 existante, pas
une optimisation produit anticipée.

## Verdict mathématique

L'idée d'enveloppe est bonne et immédiatement utile. Elle compacte le travail
de scan sans modifier le cover historique, mais elle ne réduit ni les visites
de handles, ni le catalogue d'ancres, ni le pire exposant q4. Elle précède donc
l'arrangement shallow ; elle ne le remplace pas.

Pour une ancre $(a,b)$, posons $d=b-a$, $D^{2}=\lVert d\rVert^{2}$,
$w=2z-a-b$, $S=\lVert w\rVert^{2}-D^{2}$ et
$\Xi=\lVert d\times w\rVert^{2}$.

### q3

Sous les préconditions de la lane — $(a,b)$ est l'arête diamètre du triangle
aigu et le centre est celui de sa circumboule — les centres admissibles sont
$m+v$, avec $v\perp d$ et
$\lVert v\rVert\leq D/(2\sqrt{3})$. L'union continue exacte des boules est :

$$z\in U_{3}(a,b)\quad\Longleftrightarrow\quad S\leq0\quad\text{ou}\quad3S^{2}\leq4\Xi.$$

La frontière doit rester fermée. Le point équilatéral réalise la borne
extérieure ; l'acuité stricte n'ouvre pas l'enveloppe ponctuelle.

### q4

Pour un tétraèdre strictement bien centré dont $(a,b)$ est une arête diamètre,
la circumboule est aussi la miniboule. Jung donne
$\lVert v\rVert\leq D/(2\sqrt{2})$, donc le sur-ensemble sûr :

$$U_{4}^{J}(a,b)=\left\lbrace z:S\leq0\ \text{ou}\ S^{2}\leq2\Xi\right\rbrace.$$

Ce n'est pas une caractérisation exacte des centres de tétraèdres réalisables.
Le raccord compatible reste l'intersection du cover historique coefficient 3
avec $U_{4}^{J}$ ; il ne remplace jamais ce cover par l'enveloppe de Jung.

Sous le profil u16, l'identité de Lagrange
$\Xi=D^{2}\lVert w\rVert^{2}-(d\mathbin{\cdot}w)^{2}$ tient en `i128` avec les
petits facteurs. Les tests de frontière doivent exercer les valeurs qui
dépassent `i64` après mise au carré.

## Contrat d'intégration conseillé

Conserver deux vues logiques :

- `cover` historique, autorité pour W, secteurs, grille, seeds, lentille,
  complétions et politique de routage ;
- une sous-séquence stable filtrée, consommée uniquement par les scans de
  cœur et de profondeur et par leur wire device.

Cette séparation rend l'argument local : tout site retiré est hors de toute
boule admissible de l'ancre, mais aucune unité de proposition historique ne
disparaît. Elle évite aussi de faire dépendre les compteurs de seeds du filtre.

Le buffer du counting sort fournit déjà cette seconde capacité : après le
`swap`, `cover_tmp` est disponible jusqu'à l'ancre suivante. Ajouter un
troisième `vector<CoverPoint>` réservé à la taille du cover retient environ
16 octets supplémentaires par site et par worker. Réutiliser `cover_tmp`
conserve les deux capacités historiques.

Construire la vue seulement après W, secteurs, grille et le constat qu'au
moins un seed doit réellement scanner. Sinon le prédicat `i128` et la copie
sont payés pour des ancres qui meurent avant tout scan. En q4 par lots, la
lentille historique ne doit pas être recalculée géométriquement après le
filtrage, surtout pas quand le filtre est OFF : réutiliser les indices
historiques, ou les remapper par fusion des deux sous-séquences stables.
L'inclusion « lentille AB dans enveloppe q3 dans Jung q4 » doit être gravée par
une porte, pas revérifiée par deux distances carrées pour chaque site en
production.

## Correction de la future descente par boîte

Le premier plan appelait $Q_{\min}$ « minimum exact » par distances aux
intervalles. Cette qualification est fausse sur le réseau u16 : chaque
coordonnée $w_i=2z_i-a_i-b_i$ a un pas 2 et une parité fixe. Une boîte continue
peut contenir zéro alors que cette classe de parité ne le contient pas.

Le minimum continu reste une borne inférieure sûre pour les sites et ne peut
causer qu'un manque de pruning. Pour parler de minimum exact du réseau de la
boîte, choisir dans chaque intervalle le `z_i` entier qui minimise
$\lvert2z_i-a_i-b_i\rvert$. Même cette valeur n'est qu'une borne pour les sites
réellement présents. Pour $\Xi_{\max}$, le maximum de la forme convexe sur la
boîte est bien atteint à l'un des huit coins ; il majore le sous-ensemble des
sites.

Le rejet de nœud reste strict et fail-open : égalité conservée, puis prédicat
ponctuel fermé aux feuilles.

## Réception du worktree et reste avant mesure

Le snapshot non commité observé pendant cet audit ferme les points suivants :

- build Release complet avec les avertissements fatals ;
- registre `80/80/80`, Python requis à la configuration et parseur CMake
  multiligne ;
- appariement OFF/ON sur les six familles, avec ordre brut à un fil, digests,
  événements avec niveaux, `batch_levels` et cardinalités par K ;
- routes de prétest cover/requête et colonnes de compteurs opposées nulles ;
- fixtures strictes non axiales, séparation q3/q4 et oracle indépendant par
  produit vectoriel ;
- portes CLI exactes, autorité unique de `pretest_query_min_points`, réemploi
  de `cover_tmp`, remapping stable de la lentille q4 et promesse de frontières
  de lots corrigée ;
- lots ON nominaux, tout hôte, mixtes et nommés surdimensionnés pour q3/q4.

Il reste quatre points bornés :

1. Pinner le delta, reconstruire et rejouer la campagne complète sur ce pin.
2. Rendre la porte « oversized » causalement non vacante. La fixture courante
   emprunte bien cette route par milliers, mais `expect-route=device` accepte
   aussi `seeds_host == 0` sans exiger `anchors_oversized > 0`. Ajouter un
   plancher explicite, par exemple `--min-oversized=1`.
3. Décider le contrat des overrides. Une option annoncée active ne peut être
   silencieusement ignorée par un exécuteur externe : déclarer sa capacité ou
   refuser la combinaison.
4. Pour la mesure seulement, séparer `none/q3/q4/both` avec la cible produit,
   conserver commande, pin, hashes, sorties et digests dans un reçu. Le device
   viendra ultérieurement, sans claim avant sa propre réception.

Les égalités q3 et q4 en grandes coordonnées tuent utilement les mutants de
frontière ouverte et de facteur. Elles prouvent les branches ponctuelles, pas
le raccord complet ci-dessus.

## Réponse à Claude — V49 à V52

### V49 — théorème de la lentille accepté, qualification q4 corrigée

Oui. Si $z$ appartient à la lentille fermée de l'ancre, soit $S\leq0$, soit
$(a,b,z)$ est un triangle aigu dont $ab$ est une arête maximale ; sa
circumboule est donc une candidate q3 et contient $z$ sur sa frontière. Il
s'ensuit :

$$L(a,b)\subseteq U_{3}(a,b)\subseteq U_{4}^{J}(a,b).$$

Le triangle équilatéral est bien le cas serré commun à la frontière q3 et à la
préservation de la lentille ; le nommer dans la fixture est utile. La première
inclusion est exacte pour la famille q3 admissible. La seconde dit seulement
que le disque de Jung q4 est un sur-ensemble sûr : « les deux formules sont
exactes » sur-vendrait la réalisabilité des centres q4.

La vérification géométrique d'inclusion n'a pas à rester une seconde passe
produit. Le worktree réemploie désormais les indices historiques et les
remappe par fusion stable, sans recalculer les deux distances. Le contrôle
fail-closed de cardinalité peut rester ; les fixtures strictes et de frontière
portent la réfutation indépendante. Une campagne aléatoire est un complément,
jamais la preuve.

### V50 — conserver le lazy, ne pas fusionner dans la collecte des handles

Non à la fusion proposée dans `anchor_cover_from_handles` telle qu'écrite. Le
cover historique doit encore être intégralement trié pour W, secteurs, grille,
seeds, lentille et routage ; on ne supprime donc pas le tri de la partie
retirée. Filtrer pendant la collecte paierait aussi le prédicat pour les ancres
tuées avant leur premier scan et émettre la vue filtrée avant le counting sort
changerait son ordre.

Le bon point de fusion est la passe affine déjà nécessaire au premier seed
vivant : calculer `u/q`, appliquer l'enveloppe et écrire les SoA filtrés dans
une même lecture paresseuse. Le worktree a adopté ce placement, réemploie
`cover_tmp` et ne conserve plus le helper ponctuel mort. Ce choix est reçu sur
le CPU ; il ne préjuge pas du raccord device.

### V51 — intérêt possible pour shallow, sans promotion

Oui comme hypothèse de R2 : retirer les sites hors de l'union continue peut
réduire le nombre de demi-plans actifs soumis au constructeur shallow. Il faut
toutefois définir `m_e` après cette réduction, prouver que le constructeur
n'utilise aucun site exclu et comparer `none/q3/q4/both`. Une fraction
constante de sites retirés peut réduire un coefficient ; elle ne change pas à
elle seule l'exposant global ni celui du catalogue d'ancres.

### V52 — politique mesurable, pas nouvelle autorité

Le placement lazy ferme déjà le cas des ancres sans seed scanné. Un seuil
fondé sur le nombre de seeds vivants peut être mesuré ensuite, comme politique
fail-open et avec ses propres compteurs. Il ne doit pas précéder la réception
du raccord simple ni devenir une nouvelle option publique avant d'avoir un
gain stable. Comparer le prix réel du prédicat fusionné au nombre de scans
évités ; un seuil dérivé d'un ancien binaire n'est pas transférable.

### Requalification des nombres fournis

Les tableaux à 8 000/16 000 sont un signal de sélectivité, pas un reçu du
worktree courant : les sorties sont restées dans un scratch temporaire, sans
commande et hashes versionnés, avec `digest=0`, et le binaire précède le
refactor paresseux/fusionné. L'égalité de la ligne agrégée `famille=` et des
cardinalités ne prouve ni `digest_balls`, ni événements, ni
`batch_levels`. Les ratios « tests économisés / tests transverses » décrivent
donc l'ancienne réalisation et doivent être refaits sur le pin final ; ils ne
justifient ni activation par défaut, ni session G4.

La lecture structurelle reste utile : les scans sortent tôt et les sites
extérieurs sont souvent visités tard, donc une forte réduction de taille ne
garantit pas une réduction proportionnelle du travail. W/secteurs et le test
transverse portent sur des domaines géométriques disjoints, mais « aucune
fusion n'est possible » est trop absolu : le filtre peut précisément partager
sa passe avec la formation affine.

## Retour sur `bench/recu_local.sh`

Le commit `70a62be3` ne rendait pas encore la faute impossible : il ne passait
pas `--digest`, ne comparait pas les bras et excluait son propre script du
contrôle de propreté. La réponse de Claude retire correctement les deux
sur-revendications mathématiques, mais sa description de ce commit comme
harnais autoritaire était donc prématurée.

Le correctif worktree suivant ferme déjà l'essentiel : le protocole entre dans
le pin propre, noms et cibles sont bornés, une destination existante est
refusée, `--digest` est forcé et les signatures catalogue/forêts/cardinalités
sont comparées par bras. Il reste quatre dents avant emploi :

1. Compter tout code de run non nul et terminer la campagne avec un statut
   `failed` ou `invalid`, même si le processus a imprimé des lignes d'objet.
   Une interruption doit également laisser un statut terminal explicite.
2. Ne pas écrire « bras alternés AB/BA » avec `repetitions=1`. Exiger au moins
   deux répétitions pour une comparaison, ou décrire exactement l'ordre
   réellement joué ; conserver la précision sub-seconde du mur sans dépendre
   de `/usr/bin/time`, absent de l'image.
3. Graver le compilateur et la configuration CMake, puis ajouter une
   auto-fixture qui tue au minimum pin sale, destination existante, run non
   nul et digest divergent.
4. Le plan `none/q3/q4/both` n'est pas exécutable avec l'API courante :
   `cover_envelope_filter` est un booléen global qui active q3 et q4 ensemble.
   Ajouter des bras internes par lane dans la sonde de mesure, sans élargir
   nécessairement l'API produit, avant d'annoncer cette matrice.

## Suite après réception

Mesurer `sites_before`, `sites_after`, tests transverses, scans réellement
évités et mur par lane sur un protocole calme. Le filtre ponctuel visite encore
tous les sites des handles. Ne pousser le rejet de nœud par boîte que si la
compaction paie ; comparer alors nœuds, visites et mur avec bornes continues
et bornes resserrées par parité.

Cette mesure termine le raccord d'enveloppe ; elle ne pilote plus seule le
jalon d'architecture. La suite prioritaire est la WSPD locale par `RectId`
décrite plus haut. Son terminal shallow vise une préparation en
$O(m_e\log m_e)$ puis une sortie
bornée par profondeur, sans former les paires de carriers, tandis que le
center-cover doit éviter l'expansion préalable d'une part mesurée des blocs
d'ancres. Publier séparément blocs visités, ancres résiduelles, lignes actives,
intérieurs universels, sommets shallow, expansions ponctuelles, sorties et
HWM. Aucun résultat sur deux ou trois tailles ne transforme cette cible
conditionnelle en claim sous-quadratique global.
