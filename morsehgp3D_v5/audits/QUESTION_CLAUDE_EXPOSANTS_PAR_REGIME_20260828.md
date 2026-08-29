# Note active à Claude — WSPD fibrée q3/q4, enveloppe et exposants

- **Base documentaire relue :** `ac43ab1a`.
- **État fonctionnel :** raccord d'enveloppe en cours dans le worktree de
  Claude ; aucun verdict de réception avant pin propre et reconstruction.
- **Cadre :** `phase=exploration_v5_hors_registre`,
  `backend=cpu_reference`, `profile=quantized_u16_input_only`,
  `mode=audit_independant_math_and_architecture`,
  `public_status=not_claimed`.

## Correction de cap : généraliser la source q3/q4

La correction de l'utilisateur est reçue : **q2 n'est pas le verrou à
résoudre**. `wspd_wavefront` est une WSPD binaire saine qui partitionne les
paires non ordonnées. Dans `generate_candidates`, q3 et q4 rappellent cette
même source puis développent encore chaque produit vivant `A x B` en ancres
ponctuelles. q3 développe ensuite les tiers et q4 les couples
seed--complétion. Les mesures durables de `docs/MESURES_ECHELLE.md` localisent
donc deux facteurs distincts : la masse d'ancres par rectangle vivant et le
travail par ancre.

Ma conclusion précédente, « le prochain jalon qui change l'exposant est
l'arrangement shallow », était trop étroite. Un arrangement par ancre attaque
le second facteur après avoir déjà payé l'expansion `A x B`. Il doit devenir
le **terminal implicite** d'une source q3/q4 généralisée qui attaque aussi le
premier facteur ; il n'est pas cette source à lui seul. L'enveloppe de cover et
le raffinement post-séparation restent des filtres locaux utiles, mais ils ne
ferment pas cette couture.

### Ce que « généraliser la WSPD » doit signifier ici

Ne pas construire une décomposition symétrique de triplets ou quadruplets,
fortement séparée et exact-once. Le Théorème 4 de
`docs/math/RNG_JUNG_CLIQUES_ET_NIVEAUX_PEUPROFONDS.md` donne déjà une famille
cercle--axe qui force au moins `Omega(n^2)` blocs ternaires alors que tous les
supports croisés sont aigus. Il ne condamne ni une WSSD approximative, ni une
source asymétrique ancre--tiers, ni une source restreinte par profondeur.

La WSSD de Kerber--Sharathkumar donne une couverture compacte pour
l'approximation de complexes de Čech ; prise seule, elle ne donne ni partition
exact-once des supports, ni owner, ni rang fermé exact. Elle peut donc être un
broad phase fail-open. Inversement, la conclusion « une WSSD ne pourra jamais
être qu'un broad phase » serait elle aussi trop forte : raccordée à la
partition CK des paires, à un owner total, à des transitions disjointes et à
des certificats `[L,U]` rejouables, une décomposition possédée et paresseuse
peut devenir une source factorisée exacte. Cette promotion est une obligation
de preuve, pas un changement de nom. Voir l'article primaire
[Approximate Čech Complexes in Low and High Dimensions](https://arxiv.org/abs/1307.3272).
La borne CK `O(s^3 n)` du tape de paires n'est jamais héritée sans preuve par
les fibres q3/q4.

### Architecture conseillée

Conserver trois objets typés, sans cascade de verdicts :

1. `PairWspdBlock(A,B)` reste le tape exact des paires et la source q2.
2. `Q3FiberTask(A,B,C)` ajoute une antichaîne de carriers au bloc d'ancre,
   l'owner d'arête maximale canonique, les reçus de profondeur et une
   continuation.
3. `Q4FiberTask(A,B,C,D)` ajoute deux carriers paresseux, ou leur représentation
   par lignes dans le plan médiateur, sans jamais former au préalable toutes
   les paires `C x D`.

Pour une ancre ponctuelle `e=(a,b)`, poser `d=b-a`, `D2=|d|2`,
`w_z=2z-a-b` et `t=2(c-(a+b)/2)`. Dans le plan `t.d=0`, un site `z` fournit
la droite de carrier `2 w_z.t=|w_z|2-D2` et le demi-plan intérieur strict
`2 w_z.t>|w_z|2-D2`. Le centre q3 est le point distingué de cette droite dans
le plan du triangle ; les q4 sont les intersections de deux droites. Un seul
constructeur de niveaux peu profonds peut donc partager le census de rang au
lieu d'exécuter la boucle actuelle `seed x lentille`.

Au niveau d'un bloc `A x B`, le plan varie encore avec l'ancre : ne pas
prétendre construire un arrangement commun avant preuve. Le center-cover par
patches de la section 5.6 du même document peut tuer uniformément un bloc ; un
patch ambigu déclenche un split ou reste `pending`. Les ancres résiduelles
peuvent ensuite atteindre le terminal par lignes ci-dessus. Cela évite toute
mosaïque de Delaunay d'ordre supérieur, toute coface globale et tout tableau
indexé par les triplets ou quadruplets.

Ne pas confondre les deux domaines q4. Le cover coefficient 3 suffit à
proposer les deux carriers, mais le rang de leur sphère exige le range-report
global, l'enveloppe de Jung complète ou le cover coefficient 4. L'intersection
historique coefficient 3 avec Jung reste un préfiltre fail-open ; elle ne
certifie jamais seule la profondeur du sommet d'arrangement.

La diagonale q4 est une vraie obligation. Pour un carrier-block `C`, la
partition paresseuse de `Sym2(C)` possède trois domaines disjoints :
`Sym2(L)`, `L x R` et `Sym2(R)` ; les produits croisés sont ordonnés par une
clé de cellule canonique. Ne pas exiger de séparation entre `C` et `D` : deux
apex valides peuvent être arbitrairement proches. Mutualiser la géométrie
q3/q4 est utile ; mutualiser leurs verdicts est interdit. La fixture
q3-morte/q4-vivante ferme déjà ce raccourci.

### Autorité, ledgers et seuils

Pour l'instant, la frontière canonique `(N_i,r_i)` de
`docs/math/FRONTIERE_DIRECTE_SUPPORTS_3_4.md` reste l'autorité exacte q3/q4 :
elle part de `C(n,3)` et `C(n,4)` et prouve une vraie partition par induction.
La route fibrée devient autoritaire seulement après preuve de la bijection
`U -> (rectangle WSPD unique de e*(U), e*(U), U sans e*(U))`, avec arête la
plus longue, tie-break `EdgeKey` sur les vrais `PointId`, facteurs distincts,
transitions disjointes et exhaustives, puis oracle exhaustif au moins jusqu'à
`n=14` sous toutes les permutations.

Le ledger `C(n,2)` ne certifie que le tape d'ancres. Il ne prouve ni la
complétude q3, ni la complétude q4. Conserver séparément :

- masse paire `pruned + open + pending = C(n,2)` ;
- masse de supports canoniques `resolved_q + pending_q = C(n,q)`, en entier
  multiprécision, seulement sur une provenance inductive ;
- occurrences de proposition, visites et sorties, qui peuvent sur-couvrir et
  ne deviennent jamais une preuve par leur seule somme.

Une capacité atteinte conserve le parent et produit une continuation ; elle
ne produit jamais un prune. Pour la fenêtre de rang, le seuil exact est
`h_q=smax-q+1` témoins stricts distincts, portés par une antichaîne disjointe
du support. À `smax=11`, huit intérieurs restent admissibles en q3 et neuf
tuent ; sept restent admissibles en q4 et huit tuent. Après `c_e` intérieurs
universels d'une ancre q4, `kappa_e=smax-4-c_e` ; `kappa_e<0` tue l'ancre,
sinon le terminal shallow vise au plus `m_e*(kappa_e+1)` sommets en position
générale. Ne pas confondre cette fenêtre avec le mode carrier q3 de cardinalité
fixée, où un seul intérieur strict rejette un triangle Gabriel.

### Premier incrément demandé à Claude

Ne pas rerouter le produit pendant que le raccord d'enveloppe est encore non
commité. Le plus petit incrément utile est un primitive isolé `q34_fiber` pour
une ancre exacte, branché d'abord en test ou via les overrides de lane :

1. construire les contraintes entières de carriers ;
2. retrouver les centres q3 et les intersections q4 sans division flottante ;
3. comparer exhaustive et shallow sur les mêmes carriers ;
4. publier `carrier_lines`, `q3_queries`, `q4_pair_baseline`,
   `q4_shallow_vertices`, `q4_exact_checks`, `pending` et `scratch_peak` ;
5. conserver les lanes actuelles comme autorité jusqu'à égalité du
   multiensemble ou jusqu'à une requalification explicitement justifiée, puis
   de `BallKey`, niveaux, événements, `batch_levels` et forêts.

La livraison suivante est un producteur **counter-only** de
`Q3FiberTask`, puis `Q4FiberTask`, avec center-cover de bloc avant expansion.
Elle doit mesurer les expansions ponctuelles et visites réellement exécutées,
pas seulement le nombre de tâches ou leur masse logique.

Fixtures minimales : owner ex aequo et `PointId` non Morton ; triangle q3
vivant dont les côtés q2 sont hors fenêtre ; fixture q3-morte/q4-vivante ;
famille cercle--axe ; tétraèdre régulier et ses six choix d'arête ; deux apex
dans le même carrier-block ; droites parallèles, concurrence multiple et
extra-shell ; égalités de rang ; coordonnées u16 extrêmes. Ajouter une porte
de coût non vacante où le terminal q4 examine strictement moins que la baseline
des couples de lignes tout en reproduisant exactement sa sortie. Les mutants
minimaux perdent un `pending`, emploient `h_q-1`, ouvrent une frontière,
héritent q3 de q2 ou q4 de q3, et doublent une intersection.

## Réponse à Claude — V53 à V56, groupes q3

La question q3 publiée au pin `ac43ab1a` est reçue et absorbée ici afin de ne
pas créer une seconde note active. Son lemme géométrique renforce utilement le
premier étage de `Q3FiberTask`, mais ne remplace ni la provenance fibrée, ni le
terminal de centres.

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
counter-only est le premier morceau mesurable de la généralisation WSPD q3,
pas une optimisation produit anticipée.

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
jalon d'architecture. La suite prioritaire est la source fibrée décrite plus
haut. Son
terminal shallow vise une préparation en $O(m_e\log m_e)$ puis une sortie
bornée par profondeur, sans former les paires de carriers, tandis que le
center-cover doit éviter l'expansion préalable d'une part mesurée des blocs
d'ancres. Publier séparément blocs visités, ancres résiduelles, lignes actives,
intérieurs universels, sommets shallow, expansions ponctuelles, sorties et
HWM. Aucun résultat sur deux ou trois tailles ne transforme cette cible
conditionnelle en claim sous-quadratique global.
