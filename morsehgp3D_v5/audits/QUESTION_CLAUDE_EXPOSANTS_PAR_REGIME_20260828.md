# Note active à Claude — WSPD fibrée q3/q4, enveloppe et exposants

- **Base documentaire relue :** `66997d56`.
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
fenêtre de rang est fixée et la sortie sparse. Une garantie universelle reste
impossible, mais la multiplicité des supports n'est pas automatiquement une
borne de sortie : le pipeline quotient sémantiquement par `BallKey`, recense la
coquille, puis développe les événements requis. Un `SupportRecord` par support
n'est pas une obligation produit. Les vraies bornes de sortie portent sur les
boules, événements et facettes distincts ; les rôles de supports restent un
ledger de complétude séparé, jamais un volume que l'architecture doit réémettre
avant RLE.

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

## Piste q4 conditionnelle : WSPD locale de l'arête opposée

La généralisation étroite qui n'apparaît pas dans l'inventaire v4 est
`LocalOppositeEdgeWspd`. Elle ne construit pas une seconde WSPD globale et ne
prétend pas résoudre q3 à nouveau. Elle n'est cependant **pas encore la
construction retenue** : elle ne traite que le couple de carriers q4, après
le facteur d'ancres `A x B`, alors que le verrou demandé porte sur q3 **et**
q4. De plus, un bloc de cellules séparé ne certifie à lui seul ni owner, ni
bien-centrage, ni rang. Si tous ses blocs `MIXED` finissent dans le terminal
par lignes, ce terminal sait déjà traiter l'ensemble des lignes sans recevoir
une partition préalable des couples ; la WSPD locale devient alors un coût
intermédiaire sans décision sémantique.

Sa porte doit donc mesurer non seulement `I << CellPair`, mais le nombre et la
masse de blocs réellement résolus **avant** une arête ponctuelle. Tant qu'aucun
tel certificat n'est exposé, cette piste reste une ablation q4 à comparer au
chemin direct `center-cover -> ancres résiduelles -> lignes`, pas le premier
incrément de la généralisation.

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
ponctuelle, les cellules lourdes, diagonales et couples voisins doivent passer
au terminal par lignes dans le plan médiateur. Le contrat actuellement prouvé
s'arrête toutefois aux coefficients et prédicats locaux exacts :

- q3 interroge le centre distingué de chaque carrier ; le scan plat reste la
  baseline, car l'index par couches convexes est une piste fermée tant qu'une
  nouvelle assiette mesurée ne la rouvre pas ;
- q4 vise les intersections de profondeur au plus
  `kappa_e=smax-4-c_e`, sans former d'abord les couples ;
- owner, bien-centrage, `BallKey`, shell et niveau sont toujours rejoués par les
  prédicats exacts existants.

La borne `O(m_e log m_e + m_e*(kappa_e+1) + z_e)` n'est reçue qu'en position
générale. Parallèles, lignes coïncidentes, concurrences multiples et profondeur
strictement ouverte ne disposent pas encore d'un constructeur shallow exact
qualifié. Une perturbation symbolique ne peut pas être supposée préserver ces
sémantiques. Le premier terminal emploie donc une baseline quadratique bornée
comme oracle ; un futur fast path traite la position générale, et toute
dégénérescence au-delà de son fallback conserve une continuation `pending`.
À `smax=11`, `kappa_e<=7` pour q4. q3 et q4 partagent les coefficients de
lignes, jamais leurs verdicts ni, avant preuve, une structure de niveaux.

Deux populations doivent rester distinctes. `support_lines` contient les sites
qui peuvent compléter le support ; `census_lines` contient **tous** les sites
qui peuvent être intérieurs ou sur le shell. Une ligne q4 de support n'est pas
nécessairement un seed q3 : un tétraèdre bien centré garantit au moins une face
incidente aiguë, pas les deux. Le cover historique coefficient 3 peut proposer
q4, mais ne certifie pas son rang. Restreindre le niveau aux seuls carriers
donne un rang faux ; prendre naïvement `n` lignes pour chaque arête recrée un
coût dense. La quantité à réduire est donc la somme des lignes de census
actives après classification exacte des lignes constantes et range-report,
pas seulement `|G_r|`.

Cette route ne matérialise ni mosaïque de Delaunay d'ordre supérieur, ni
cofaces ou incidences globales. Elle n'est cependant linéaire que si le
center-cover résout effectivement des rectangles avant `PairId` et si le
census agrégé reste sparse. La WSPD locale enlève le carré des cellules ; elle
ne prouve pas ces deux faits à sa place.

## Exactitude et provenance à conserver

Le ledger naturel porte d'abord les occurrences enracinées par arête. Il faut
ici écrire `n_u=ix.unique_count()` : la génération q3/q4 parcourt les
positions uniques et `run_pipeline` refuse les coordonnées dupliquées avant
toute sémantique HGP. La WSPD basse possède encore un ledger pondéré de
`PointId` pour tester l'index ; sous entrée acceptée il coïncide avec la masse
de positions, mais il ne définit ni profondeur pondérée ni census distinct.

- q3 : `Omega3={(e,x): x notin e}`, de masse `3*C(n_u,3)` ;
- q4 : `Omega4={(e,{x,y}): x,y notin e}`, de masse `6*C(n_u,4)`.

Pour un bloc d'ancres de masse-position `p=|A||B|`, cela donne analytiquement
`p*(n_u-2)` rôles q3 et `p*C(n_u-2,2)` rôles q4. Un futur
`Q3FiberTask(A,B;C)` porte exactement
`|A||B||C|-|A∩C||B|-|B∩C||A|`, puisque `A` et `B` sont
disjoints. Ces masses tiennent en `u128` sous la limite i32 et sont additives
sous les splits.

Le tape extérieur, toute partition locale et ses continuations doivent fermer
ces ledgers de **rôles**. `EdgeKey` choisit ensuite l'unique arête maximale
canonique. La masse juste ne prouve ni `C(n_u,q)` supports acceptés, ni le rang,
ni la complétude du constructeur shallow. Les cellules hors de la fenêtre
reçoivent un reçu exclusif ; elles ne disparaissent jamais du ledger.

Un split de `A` ou `B` conserve obligatoirement `origin_rect_id`. La grille
peut rester immuable, ou être raffinée, mais une reconstruction n'est valide
que si une transition disjointe et exhaustive relie chaque cellule du parent
aux cellules enfants. L'immuabilité est une réalisation simple, pas un
théorème. Une capacité atteinte conserve le parent et une continuation, jamais
un prune.

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
- `Z`, sommets de niveaux proposés par ancre, puis `B`, `BallKey` distinctes ;
- `O3/O4`, masses logiques de rôles résolues, sans les réémettre comme sortie.

La borne à viser est
`O(n log n + R + K log n + C + I + A + V + sum_e(m_e log m_e) + h*M + Z)`
avec scratch tuilé `O(n + K_tile + M_tile + Z_tile)`. Elle devient quasi
linéaire à `h` fixé seulement après réception des quatre propriétés suivantes :

1. `R=O(n)` pour une vraie décomposition fair/prefix extérieure ;
2. `K+I+C=O(R log n)` à paramètres de maille et séparation fixes ;
3. `A+V+M=O(R+K+Z)` dans chaque régime déclaré ;
4. aucune boucle point--point dans une diagonale ou paire voisine lourde.

L'objectif immédiat honnête est donc `O(n log n+Z)` dans les régimes sparse,
où `Z` ne compte pas artificiellement toutes les représentations d'une même
boule concurrente.
Le linéaire `O(n+Z)` est un objectif secondaire crédible sur entrée Morton déjà
triée, voisinages bornés et terminal sans tri comparatif supplémentaire ; il
n'est pas encore prouvé. Le pire cas reste output-sensitive et peut être
quadratique. Cette formulation satisfait la demande de sous-quadratique sans
inventer une garantie impossible.

Sur `uniform`, `terrain`, `eight_clusters` et `scanline`, les rampes
`8k/16k/32k/50k`, puis éventuellement `100k/200k`, publient tous les compteurs
ci-dessus et leur HWM. Une borne supérieure à 95 % de pente est un falsificateur
empirique de régime, jamais une preuve d'exposant ; les seuils `1,8` et `1,2`
restent des budgets d'ingénierie, pas des constantes mathématiques. Une pente
de `I` seule ne qualifie rien. Aucun test à 100 k, 200 k ou 10 M ne commence
avant fermeture exacte, non-vacuité du mécanisme et mémoire bornée à 50 k.

## Premier incrément utile à Claude

Ne pas rerouter le produit pendant que le raccord d'enveloppe est non commité.
L'ordre du pin `5afcfce0` est inversé : construire d'abord une WSPD d'arête
opposée n'attaque ni q3, ni le facteur `|A||B|`. Le premier probe est un
**counter-only de blocs d'ancres** fondé sur le Théorème 5 de
`docs/math/RNG_JUNG_CLIQUES_ET_NIVEAUX_PEUPROFONDS.md` :

1. dérouler une seule WSPD binaire pour q3/q4, q2 restant inchangé ;
2. pour chaque `RectId`, appliquer deux grilles exactes de 64 patches, une par
   lane, avec des antichaînes témoins séparées ; une grille q4 commune serait
   un sur-ensemble q3 plus faible, jamais la réciproque ;
3. classer la masse-position en `pruned`, `microtile` ou `pending`, fermer
   `C(n_u,2)`, `Omega3` et `Omega4`, et exiger
   `anchors_materialized=0` ;
4. rejouer chaque prune par oracle à `n<=14`, sous permutations, égalités et
   `PointId` non Morton ;
5. publier patches infaisables/morts/ambigus, visites patch--nœud, splits,
   masse arrivée aux microtiles, HWM et continuations.

Ce probe falsifie directement le premier facteur. Si la majorité de la masse
atteint les microtiles, ne pas ajouter la WSPD locale. Si le center-cover est
non vacant, ouvrir ensuite `AnchorLineSet`, puis comparer en **ablation q4** le
range-report direct aux cellules locales. `LocalOppositeEdgeWspd` n'est retenue
que si elle évite un travail réellement exécuté avant le shallow, pas parce
que `I` est inférieur au nombre analytique de `CellPair`.

### C1 — le center-cover possède une ABI entière courte

Le vague « arrondi extérieur » du Théorème 5 n'a pas besoin de binary64 sous
le profil u16. Poser `e3=4`, `e4=5` et, pour chaque axe :

$$\ell_{q,i}^{(8)}=4(A_i^-+B_i^-)-e_qH,\qquad u_{q,i}^{(8)}=4(A_i^++B_i^+)+e_qH.$$

Les cinq frontières des quatre sous-intervalles, représentées directement en
coordonnées multipliées par 32, sont exactement :

$$\beta_{q,i,j}^{(32)}=4\ell_{q,i}^{(8)}+j\left(u_{q,i}^{(8)}-\ell_{q,i}^{(8)}\right),\qquad j\in\left\lbrace0,1,2,3,4\right\rbrace.$$

On obtient ainsi deux pavages distincts : `q3` couvre
`M_AB + [-H/2,H/2]^3`, `q4` couvre
`M_AB + [-5H/8,5H/8]^3`. Les patches sont fermés et leurs frontières communes
sont représentées par le même entier. La boîte des milieux vaut, à cette
échelle, `[16*(Alo+Blo),16*(Ahi+Bhi)]` axe par axe.

Noter `d32_2` la distance carrée entre un patch et cette boîte des milieux,
et `Dmax_2` la distance maximale carrée exacte entre `A` et `B`. Les rejets de
rayon, stricts, deviennent :

- q3 : `3*d32_2 > 256*Dmax_2` ;
- q4 : `d32_2 > 128*Dmax_2`.

L'égalité conserve le patch. Pour q3, le lemme manquant dans l'énoncé actuel
est court : si `pq` est une arête maximale d'un triangle aigu, son angle
opposé est au moins 60 degrés, donc son circumrayon vérifie `R^2 <= D^2/3` et
`|c-m_pq|^2 = R^2-D^2/4 <= D^2/12 <= H^2/4`. Avec `smax-2` témoins stricts
et trois points de coquille, la même preuve d'antichaîne que le Théorème 5
exclut le rang fermé au plus `smax`. Ce lemme doit être gravé comme proposition
q3 avant qu'un bit q3 ne devienne autoritaire ; le document racine ne fait
actuellement qu'annoncer la constante et le seuil.

Pour les puissances, mettre aussi `A`, `B` et `W` à l'échelle 32. Dans une
coordonnée, avec `C=[c0,c1]`, le minimum de
`dist(t,P)^2-max((t-X0)^2,(t-X1)^2)` est atteint parmi `c0`, `c1` et les
bornes de `P` appartenant à `C`. Le milieu de `X` est un saut descendant de
dérivée, donc jamais un minimum intérieur. La somme des trois minima donne
`L32(C,P,X)` et `U32(C,P,X)=-L32(C,X,P)`. Les décisions exactes de boîte sont :

- patch médiateur impossible si `L32(C,A,B)>0` ou `U32(C,A,B)<0` ;
- nœud témoin universel si
  `max(L32(C,A,W),L32(C,B,W))>0` ;
- aucun témoin strict dans le nœud si
  `min(U32(C,A,W),U32(C,B,W))<=0`.

`L32/U32` est exact sur les produits de boîtes, mais reste une borne sur les
ensembles discrets qu'elles contiennent. Sous u16, les patches q4 restent
dans `[-1310700,3407820]`, toute différence est inférieure à `2^22`, et tous
ces carrés, sommes et membres multipliés restent sous `2^47`. `i64` suffit
avec promotion avant les opérations ; `i128` peut rester défensif, mais ne
doit pas masquer un cast tardif.

### C2 — parcours partagé, mais crédits jamais partagés

Le falsificateur doit partir d'une seule WSPD brute et intervenir avant
`corner_histograms`, `rect_cover_handles`, `AnchorScratch` et les boucles
d'ancres. `alive_rectangles()` est aujourd'hui rejoué séparément par lane et
entremêle déjà un autre prune ; il ne mesure donc pas isolément le premier
facteur. Après un terminal WSPD, un descendant
`PairProductBlock{a,b,origin_rect_id,lane_mask}` partitionne seulement le
produit. Il n'a pas à repasser `separated` et n'est plus appelé WSPD.

Le parcours témoin peut partager ses lectures avec une pile
`{NodeRef,mask3,mask4}`, mais il conserve deux tableaux de 64 compteurs. Un
nœud accepté pour un patch retire ce bit de ses descendants et construit
implicitement l'antichaîne locale ; il peut rester ouvert pour les autres
bits. Compter séparément les `witness_node_pops` physiques et les
`patch_node_tests` logiques empêche de faire paraître le coût 64 fois plus
petit. Un patch qui recevrait simultanément `L_W>0` et `U_W<=0` incrémente
`bound_conflict` et reste `pending` : cette combinaison doit être impossible
pour un centre médiateur faisable et constitue d'abord un invariant à tester.

Avant toute borne de puissance, la plage de `W` doit être entièrement
disjointe de celles de `A` et `B`. Un sous-arbre inclus dans une extrémité est
jeté ; un ancêtre qui la contient est scindé. Réutiliser
`witness_detail::credit_weight` puis soustraire l'overlap certifierait une
boîte contaminée et ne prouverait pas l'antichaîne du Théorème 5.

Deux autres compositions sont interdites :

- ne pas initialiser les patches avec `AliveRect::core` ni additionner le
  résultat de `count_universal_witnesses`, car les identités déjà créditées ne
  sont pas conservées et peuvent être recomptées ;
- après un split de `A` ou `B`, ne pas hériter les compteurs des patches du
  parent : la boîte de centres, le pavage et les antichaînes changent. Chaque
  enfant repart de zéro, sauf futur certificat explicite de transport.

Le profil courant refuse normativement les positions dupliquées avant la
génération. Le helper bas niveau vérifie donc cette précondition et passe
`pending` sans prune sinon ; sa masse témoin est le nombre de positions de la
plage, pas `node_weight`. La phrase du document racine qui attribue une
sémantique aux `PointId` co-positionnels n'instancie pas la v5 actuelle. Le
ledger pondéré exposé par la WSPD reste diagnostique ; sous entrée acceptée il
doit coïncider avec le nouveau `pair_position_mass`.

Par lane, `pruned + microtile + pending = C(n_u,2)`. Les rôles ferment ensuite
`pair_mass*(n_u-2)` en q3 et `pair_mass*C(n_u-2,2)` en q4, en `u128`. Une lane
avec `smax<q` est inactive avant toute soustraction non signée. Un budget
atteint conserve le bloc entier dans `pending`; le premier probe exige
`anchors_materialized=0` et `pair_records_materialized=0`.

L'oracle `n<=14` développe les paires seulement côté test, vérifie leur
partition littérale, rejoue chaque patch et chaque nœud accepté, puis cherche
indépendamment tout triangle ou tétraèdre possédé de rang interdit dans un
bloc pruné. Les mutants prioritaires sont : union de crédits entre patches,
cumul `core+cover`, overlap soustrait après acceptation, héritage après split,
seuil `h-1`, infaisabilité sur égalité, confusion des deux grilles et perte ou
duplication d'un `pending`.

## Réponse au commit `fd7b6c70` — le signal rescan est reçu, pas son routage

### R57 — rétrogradation partiellement acceptée

Oui : l'escalier d'histogramme et les rejets de handles ne sont pas la réponse
générale au coût observé sur `terrain`. Les compteurs du pin propre
`a3c15d84`, à 8 k, 16 k et 32 k, montrent bien que les seeds et complétions
croissent plus vite que les rectangles et que les scans méritent leur propre
étage. Cela renforce `AnchorLineSet` après le center-cover.

Non à la conclusion que post-séparation, enveloppe et filtres de handles ont
le même défaut. Le code les place sur trois facteurs différents :

- un prune de produit supprime des ancres **et tous leurs rescans** ;
- l'enveloppe paresseuse construit `scan_sites()` et réduit directement les
  itérations de cœur et de profondeur de chaque seed ou complétion survivante ;
- le filtre de handles seul réduit essentiellement les propositions de seeds.

Le center-cover reste donc premier parce qu'il peut enlever le produit
`A x B` avant sa matérialisation ; le shallow reste indispensable pour les
ancres résiduelles denses. L'un ne réfute pas l'autre. De même, trois tailles
compatibles avec une pente proche de un ne prouvent pas « exactement
linéaire ». `rect_alive` est un sous-flux déjà filtré, et
`GenerateStats::candidates` est un catalogue pré-RLE, pas « l'objet » ; les
boules distinctes, événements et forêts sont les sorties pertinentes.

### R58 — route dense plausible, seuil actuel non reçu

Le seuil `m environ 60--100` est le croisement d'un modèle à coût unitaire
`0,24*m^2` contre `m*log2(m)+8*m`, pas une rentabilité mesurée d'un
constructeur. Il ignore ses constantes, les sorties anticipées, les
dégénérescences et la production exacte des lignes. Il peut dimensionner un
prototype, jamais ouvrir une route produit.

Les nombres « 3,9 % des ancres / 87 % du travail », les quatre moyennes de
`m` et leur ventilation cover/requête ne figurent pas dans le reçu joint. Ils
ne sont donc pas reproductibles depuis `RECU.txt` et ses six sorties. Même
avec ces moyennes, remplacer `sum(m_e^2)` par `nombre_ancres*moyenne(m_e)^2`
perd la variance et ne prouve pas la concentration du travail réel ; les
sorties anticipées peuvent aussi différer entre routes.

Surtout, `pretest_query_min_points` teste une fois par rectangle si
`handle_points >= 512`. Il ne teste ni le `m_e` exact d'une ancre, ni le seuil
60--100, et toutes les ancres du rectangle héritent du même choix. La
coïncidence des moyennes en fait un hint de scheduling, pas « exactement » le
routeur shallow. Pour q4, le cover historique de coefficient 3 ne contient
pas nécessairement toutes les lignes qui influencent le rang ; le `m_e`
autoritaire vient du `census_lines` global classifié, pas de cette taille de
cover.

La porte utile publie donc par ancre ou par buckets de `m_e` : lignes de
support, lignes de census, seeds, complétions, tests de puissance réellement
exécutés, early exits, fallbacks et temps. Elle compare ensuite scan exact et
shallow sur le **même** `AnchorLineSet`. Une route dense seulement est une
bonne ablation ; elle se décide sur `m_e` après construction de la source et
reste fail-open ailleurs, sans réutiliser silencieusement le seuil des
prétests.

### R59 — profondeur bornée ne signifie pas population bornée

Le calcul `8,4 -> 23,8` lorsque `n` est multiplié par quatre donne bien une
pente terminale proche de `0,75`, mais pour la **moyenne des seeds par ancre**,
pas pour `m_e`. Il mélange la taille du cover, le taux d'acuité, les deux routes
de prétest et leur distribution. Le reçu ne publie ni `sum_e m_e`, ni ses
quantiles, ni son maximum ; il ne permet donc pas encore d'affirmer
`m_e ~ n^0,75`. Cette pente prouve que le rescan est à mesurer, pas quelle
variable géométrique l'explique.

La croissance de `m_e` n'invalide pas la borne locale
`O(m_e log m_e + m_e*(kappa_e+1) + Z_e)` ; c'est précisément le régime où elle
peut remplacer un rescan quadratique. Elle invalide seulement toute phrase qui
déduirait `m_e=O(1)` de `kappa_e=O(1)`. Au rang fermé onze, q3 cherche au plus
huit intérieurs et q4 au plus sept avant crédits globaux exacts ; les crédits
locaux des patches ne réduisent jamais ces seuils terminaux.

La question globale reste `M=sum_e m_e`, pas le maximum d'une ancre. Un petit
nombre d'ancres denses peut être favorable au shallow ; une masse croissante
d'ancres portant chacune un grand `m_e` peut encore rendre la construction des
lignes superquadratique. Publier la pente de `M`, le travail de range-report
et `Z` est donc obligatoire. Enfin, la borne shallow citée reste une borne de
position générale : parallèles, coïncidences, concurrences et profondeur
stricte gardent l'oracle quadratique borné puis `pending` au-delà du cap.

Ordre inchangé mais mieux motivé : pinner l'enveloppe, exécuter le center-cover
counter-only, construire la source signée et mesurer directement `m_e`, puis
ouvrir le shallow sur la route dense. La WSPD locale q4 reste une ablation du
range-report, pas un préalable.

### R58 bis — remplacer le faux seuil statique par une bascule sur travail exécuté

L'idée d'un backend hybride n'est pas nouvelle. La v3 proposait déjà le switch
statique `carrier_count*cover_mass > switch_budget` entre scan axial et
`EdgeCenterShallowCut` dans
[`NOTE_AUDITEUR_LBVH_SPARSE_Q3_Q4_APRES_53815F_20260816.md`](../../morsehgp3D_v3/audits/NOTE_AUDITEUR_LBVH_SPARSE_Q3_Q4_APRES_53815F_20260816.md),
et la v4 demandait encore une branche adaptative dans
[`REPONSE_CLAUDE_E573888_FILTRES_CERTIFIES_ET_Q3_DEMI_PLANS_20260818.md`](../../morsehgp3D_v4/audits/REPONSE_CLAUDE_E573888_FILTRES_CERTIFIES_ET_Q3_DEMI_PLANS_20260818.md).
Réintroduire seulement un seuil sur `m_e` ne généraliserait donc rien.

L'incrément qui ne figure pas dans cet inventaire est une bascule **en ligne**
sur le préfixe de travail réellement consommé :

1. construire une fois `AnchorLineSet`, en temps linéaire dans les sites
   reportés, avec les trits exacts `INTERIOR | EXTERIOR | ACTIVE_LINE` ;
2. traiter les carriers dans leur ordre canonique par scan direct et compter
   les prédicats réellement exécutés, sorties anticipées et replis exacts
   compris ;
3. lorsque ce cumul atteint le devis receipté du constructeur shallow pour le
   bucket `(m_e,kappa_e,profil_degenerescence)`, construire l'index une fois et
   lui soumettre seulement les carriers restants ;
4. prouver le ledger exact-once
   `primary_carriers=prefixe_direct disjoint_union suffixe_shallow`, conserver
   le préfixe déjà produit et exiger le même objet que les routes `all-direct`
   et `all-shallow`. Le RLE canonicalise les doublons résiduels ; il ne prouve
   ni cette partition ni l'absence d'un support manquant.

Nommer d'abord cette politique `adaptive_online_dispatch`, pas `ski_rental` :
elle n'est deux-compétitive que si coût de location et prix d'achat sont connus
dans la même unité et si le prix de construction est indépendant du jeu de
données. Une construction exacte randomisée, ses listes de conflits et ses
dégénérescences ne satisfont pas encore ces hypothèses. Le devis doit donc être
calibré par compteurs pondérés, puis confronté au mur et à la HWM ; le reçu
publie aussi le regret contre `min(all-direct,all-shallow)`.

q3 est le premier terrain sûr de cette ablation. Pour q4, l'unité du préfixe
est un seed carrier canonique avec toutes ses complétions. Les lignes des seeds
déjà traités restent dans l'index comme témoins de profondeur et comme apex
possibles ; seul leur droit d'émission comme carrier primaire est masqué.
Retirer physiquement le préfixe créerait des manques, le réémettre créerait des
doubles. Les paires croisant préfixe et suffixe suivent le carrier primaire
canonique, jamais la date de bascule. Une capacité ou une dégénérescence non
supportée conserve la continuation ou retourne son fate explicite ; elle ne
déclenche jamais un fallback direct non borné.

Le premier mode est `shadow_counter_only` : sur de petites et moyennes ancres,
les trois politiques s'exécutent, l'oracle compare profondeur stricte, shell,
`BallKey`, owner et digest, puis le routeur ne publie encore aucune décision.
Cette étape donne enfin un seuil causal sans attendre que le constructeur soit
assez mûr pour rerouter le produit.

### V57 — la vue signée commune q3/q4 est déjà disponible

Pour une ancre exacte, la future primitive ne doit pas conserver seulement le
booléen d'enveloppe. Avec les notations de la section mathématique ci-dessous,
poser :

$$Q_{3}=4\Xi-3S^{2},\qquad Q_{4}=2\Xi-S^{2}.$$

Pour chaque lane, le trit exact relatif à son disque continu est :

- `Q < 0 && S < 0` : `UNIVERSAL_INTERIOR`, à créditer dans `c_e` ;
- `Q < 0 && S > 0` : `UNIVERSAL_EXTERIOR`, ligne et carrier impossibles ;
- `Q >= 0` : `ACTIVE_LINE`, tangence `Q == 0` comprise.

Les deux extrémités de l'ancre sont exclues avant ce trit : elles donnent la
contrainte identiquement nulle `Q=S=0`, pas une ligne active.

q3 emploie son disque exact ; q4 emploie tout le disque de Jung, donc les deux
branches universelles restent sûres pour le sous-ensemble de centres
réalisables. À `smax=11`, `c_3>=9` tue q3 et `c_4>=8` tue q4. Le filtre du
worktree actuel n'exploite que la moitié extérieure `S>0` : cette extension
signée appartient au nouvel îlot après pin, pas au raccord d'enveloppe en cours.

Les crédits d'un center-cover sont locaux à un patch : neuf témoins différents
peuvent certifier chacun des 64 patches. Ils ne forment pas pour autant un
`c_e` global et ne se soustraient jamais de `kappa_e`. Au terminal exact,
`c_e` est recompté par la branche `UNIVERSAL_INTERIOR` sur **tout** le disque.

### V58 — coefficients homogènes exacts, sans division

Choisir le pivot `k` tel que `|d_k|` soit maximal, et noter `i,j` les deux
autres axes. Paramétrer le plan médiateur par :

$$t=\alpha(d_k\mathbf{e}_{i}-d_i\mathbf{e}_{k})+\beta(d_k\mathbf{e}_{j}-d_j\mathbf{e}_{k}).$$

La ligne d'un site `z` s'écrit exactement :

$$X_z\alpha+Y_z\beta=W_z,\qquad X_z=2(d_kw_{z,i}-d_iw_{z,k}),\qquad Y_z=2(d_kw_{z,j}-d_jw_{z,k}),\qquad W_z=S_z.$$

L'intérieur strict est `X_z*alpha + Y_z*beta > W_z`. Représenter la ligne par
`(X,Y,-W)` ; l'intersection de deux lignes est leur produit vectoriel
homogène, normalisé avec coordonnée de dénominateur positive. Sous u16,
`|X|,|Y|<2^35`, `|W|<2^36`, les coordonnées locales d'intersection restent
sous environ 72 bits et une incidence sous 109 bits : les prédicats locaux
tiennent en `i128`. Ce résultat ne prouve pas que le tri global choisi par un
constructeur shallow tient en `i128` ; certaines comparaisons de rationnels
peuvent exiger 192 ou 256 bits.

Chaque ligne porte séparément `rank_active`, `support_q3` et `support_q4`.
Pour q4, les deux carriers viennent de toute la lentille admissible, pas des
seuls seeds q3. Une intersection candidate repasse par `tetra_owned_by`, les
préfiltres exacts, `q4_form`, `q4_center_strictly_inside` et le niveau. Deux
lignes parallèles ne produisent rien ; deux lignes coïncidentes ne définissent
pas un tétraèdre, mais leur multiplicité de shell ne peut pas être jetée.

### V59 — dégénérescences et digest sont deux portes distinctes

La borne shallow citée suppose la position générale. Le premier gate compare
donc l'ensemble homogène à toutes les paires seulement pour de petits `m_e`,
groupe les concurrences par centre exact, puis recertifie profondeur et shell.
Un fallback quadratique est acceptable sous un plafond causal de petite ancre ;
au-delà, le prototype conserve `pending`. Une SoS unilatérale reste une piste,
pas une autorité avant preuve qu'elle préserve la profondeur stricte et remappe
toutes les concurrences.

Le RLE courant choisit, à `BallKey` égal et arité égale, la plus petite
**représentation non réduite** de `ExactLevel`. Émettre une seule paire
incidente arbitraire change donc potentiellement `digest_balls`. Sous le
`shell_cap`, le fallback local peut tester les paires incidentes valides et
choisir la même représentation minimale ; au-delà, il suit le statut de
ressource du census, jamais une troncature.

Enfin, un center-cover plus fort supprime volontairement des boules profondes
que le digest v4 des candidats conserve encore, notamment la contre-fixture du
cover q4. L'objet final peut rester identique alors que `digest_balls` diverge.
Claude doit choisir explicitement entre un mode de compatibilité qui réémet ces
candidats — et paie leur coût — ou un payload v5 versionné de candidats déjà
filtrés par rang, comparé à la v4 sur census, événements et forêts. Cette
divergence de contrat ne doit être ni masquée, ni utilisée pour bloquer par
accident l'architecture neuve.

Fixtures prioritaires : deux lignes sans support ; même cellule et cellules
voisines très chargées ; bloc tué par huit témoins q4 avant tout `CellPair` ;
famille cercle--axe ; owner ex aequo avec `PointId` non Morton ; q3-morte et
q4-vivante ; tétraèdre régulier avec ses six occurrences ; lignes parallèles,
concurrence cosphérique, extra-shell et coordonnées u16 extrêmes ; pivot
négatif sous permutations d'axes ; tangences `Q3==0` et `Q4==0` ; patches
individuellement morts avec ensembles témoins incompatibles. Les mutants
écartent une tangence, unissent les crédits de patches, comptent le shell comme
intérieur, restreignent q4 aux seeds q3 ou remplacent le shallow par
`choose2(m_e)`. Ce dernier doit échouer sur une porte de travail, même s'il
reproduit la bonne sortie.

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

Ne pas fabriquer le trit complémentaire avec `hmax4_boxes` : cette fonction
calcule un `min_(a,b) max_x`, pas le maximum de `H` sur le produit. En une
dimension, `A=[0,10]`, `B=[20,30]`, `C={9}` donne `hmax4_boxes=-84` alors que
`H(0,20,9)=99`. Un futur certificat signé peut employer un vrai maximum de
`H` et un majorant de `G=|(b-a) x (x-a)|^2`; le maximum continu exact de `G`
est atteint sur au plus `8^3=512` triplets de coins. Ces trits restent une
ablation counter-only **après** le center-cover : ils peuvent accélérer ses
64 patches, mais le pavage local est plus fort et doit d'abord servir de
baseline. La contre-fixture ci-dessus devient permanente avant toute branche
`UNIVERSAL_EXTERIOR` de bloc.

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
jalon d'architecture. La suite prioritaire est le center-cover counter-only de
blocs décrit plus haut. Son résultat décide si assez d'ancres q3/q4 peuvent
rester implicites. Le terminal shallow vise ensuite une préparation en
$O(m_e\log m_e)$ puis une sortie bornée par profondeur, sans former les paires
de carriers ; la WSPD locale par `RectId` n'entre qu'en ablation q4 de son
range-report. Publier séparément blocs visités, ancres résiduelles, lignes
actives, intérieurs universels, sommets shallow, expansions ponctuelles,
sorties et HWM. Aucun résultat sur deux ou trois tailles ne transforme cette
cible conditionnelle en claim sous-quadratique global.
