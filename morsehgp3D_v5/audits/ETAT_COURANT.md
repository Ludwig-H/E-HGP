# État courant audité de MorseHGP3D v5 — 29 août 2026

- **Dernier pin fonctionnel de Claude relu :** `2168a295`. Les deux commits
  postérieurs relus jusqu'à `dc01fdf0` sont documentaires ; ils ne déplacent
  pas le pin fonctionnel jugé.
  Son changement de priorité est reçu : ne pas construire maintenant
  l'arrangement shallow et attaquer d'abord le nombre de seeds q3. Sa sonde ne
  prouve toutefois ni le facteur `6--9`, ni `O(h3)` par seed, ni les moyennes
  `9,24/9,57` dans le produit : elle saute toutes les portes amont, facture le
  shallow aux ancres sans seed et son source committé n'imprime aucun compte de
  seeds. Le replay instrumenté de la vraie lane donne 11,40 tests de sites par seed sur
  `terrain n=8000` et 12,03 sur `scanline`, avec une plage 11,40--12,50 entre
  2 k et 8 k. Le scan moyen est donc court dans ces régimes ; le pire cas d'un
  seed peu profond reste le cover entier. R2 est refermé comme priorité, pas
  réfuté comme possibilité mathématique.
- **Pin de gain fibre relu :** `8cbee414`. Son `fibre_gain_probe` ne reçoit pas
  les gains annoncés de `36,1 %`. Il réimplémente le masque de
  `73b00f3f` avec les deux formes de signe opposé et sans normalisation par
  l'orientation de `(d,u,v)` : sur la propre fixture oblique positive du probe
  sectoriel, le vrai bit `0x01` devient le bit opposé `0x10`. Il compte en outre
  les rescans de seeds déjà retirés par `core+h_a+h_b`, `W3`, la mort de grille
  ou la cellule de seed, et reconstruit cover/comptes par handle. Le prochain
  shadow doit maintenant fermer l'amortissement réel : seulement 11--13 tests
  de sites par seed contre seize extrema ajoutés par handle. La réponse
  concurrente V104 confirme la faute sur 200 000 ancres et retire elle-même le
  chiffre : ce retrait est reçu, mais n'a encore ni pin ni reçu brut.
- **Réponses V105--V109 au worktree concurrent :** le ratio sectoriel annoncé
  `2,1--2,9` sur `terrain`, proche de un sur `scanline` et `0,04--0,13` sur les
  deux autres cohortes est d'abord incohérent avec l'ordre de la lane. Son
  gain exclut les groupes `full8_killed`, mais son coût
  `16*handle_nonempty` les facture encore, alors que l'ancre a déjà terminé.
  Sur `eight_clusters,n=8000,seed=3`, `238541` des `245713` groupes non vides
  sont dans ce cas : la seule correction de stade transforme le proxy `0,04`
  en `1,28`. Elle transforme aussi `2,86/1,92/0,12` en
  `3,32/4,04/0,28` sur `terrain/scanline/uniform`. Ces nombres ne reçoivent
  toujours aucune rentabilité produit : la sonde ne rejoue ni histogrammes,
  ni `W3`, ni grille, ni cellule, et crédite douze tests moyens par seed au
  lieu des tests réellement évités sur les seeds tués. Le helper futur à
  seize extrema n'existe pas ; le code courant paie 128 déterminants, et
  l'équivalence « un extremum = un test de site à un facteur deux près » n'est
  pas établie. V107 appelle donc un shadow dans l'ordre produit publiant
  `groups_reached`, `fates_evaluated`, extrema, seeds/formes/tests réellement
  sautés, puis mur/HWM appariés. Les bras obligatoires sont
  `baseline/tau/sector/tau+sector`, le secteur ne recevant que son marginal
  après `tau` ; le tableau courant ne permet ni activation ni disqualification.
  Pour V108, la pré-porte de rayon ne certifie pas un secteur : la propre
  fixture frontière `0x81`, de boîte dégénérée, la satisfait. Elle reste une
  heuristique counter-only. Le candidat constructif teste les deux marges
  linéaires du secteur central contre leur variation exacte sur la boîte ; il
  doit rester fail-open à égalité ou projection nulle et recevoir oracle,
  mutant et coût avant emploi. V109 garde `g_AB/tau` prioritaire ; le secteur
  est seulement un terminal possible sur son résiduel mesuré.
- **Pré-lecture du probe q3 de patches non committé :** son invariant
  `mask_empty -> zero_true_seed` est pertinent et l'absence de coplanarité est
  seulement lâche. Il ne calcule cependant ni `g_AB`, ni `h_c`, ni `tau`, et sa
  grille à l'échelle 4 ne doit pas être confondue avec `CenterQ32Box`. Son arrêt
  après les premiers blocs est corrélé à l'ordre Morton : bottom-k, cible
  CMake, pin propre, non-vacuité et sorties brutes précèdent tout taux reçu.
- **Dernier pin sectoriel reçu :** `73b00f3f`. Il consolide le
  helper de `ed9c282f` et répare réellement V90/V92 : vrais cônes de
  `anchor_sector_kill`, produits mixtes entiers fermés, tous les seeds et
  surmasque AABB conservatif, contre-fixtures obliques/frontière et cible CMake
  smoke. Le replay propre à `n=8000,seed=3` ne trouve
  aucune inclusion ou décision violée et reçoit un fort potentiel
  **handle-local** sur `terrain/scanline`. Il révèle aussi le verrou décisif :
  l'union des masques de tous les handles vaut `0xff` pour chaque ancre non
  vide des quatre cohortes, donc le gain au niveau du test d'ancre courant est
  nul. Même retirer oraclement les handles sans seed ne gagne que `1/1/0/0`
  ancre sur `scanline/terrain/uniform/eight_clusters`. L'incrément doit
  préserver la provenance et tuer seulement l'émission du handle. Les anciens
  chiffres `7313df2d` restent invalides ; le tableau publié avec `ed9c282f`
  n'est pas exactement celui du source réparé et n'a ni sortie brute ni trois
  seeds. Signal causal reçu, reçu de performance absent.
- **Dernier pin propre du repli `Pi` :** `650b3cff`. Reconfiguré puis
  reconstruit, il capte `0/1/0` blocs sur trois familles à `n=3000` et est
  retiré du chemin candidat. Les grands runs qui impriment
  `1ff39ab9,worktree_modifie=non` ont réemployé des définitions CMake en cache
  et restent diagnostiques, pas des reçus. Son contre-audit de la v2 reste
  `b74d8050`.
- **Base mathématique concurrente relue :** `b53605db`, qui consolide
  `84cc3d73` et `b201ac23`. Elle ferme le ledger pondéré q3 par histogrammes et
  spécifie un préfiltre q4 à neuf classes avant le terminal axial. Les factorisations sont
  reçues mathématiquement ; le chemin q4 reste une proposition counter-only,
  pas une route produit déjà reçue.
- **Compléments V79--V100 reçus comme diagnostics :** la sonde sépare le marginal
  tous-profonds selon l'existence de neuf témoins communs. Ce shadow ne corrige
  pas le biais V74 et ne prouve ni que les patches sont nécessaires, ni qu'un
  certificat global de boîtes captera les témoins communs exacts. La bonne
  reformulation par le centre est le noyau déjà prévu de `g_AB` : minimum
  concave aux sommets d'un sur-patch rationnel, puis front d'arbre masqué. Le
  premier incrément calcule `g_AB[64]` seul ; le crédit commun attend un état
  d'intersection distinct des saturations par patch. Le disque d'une paire
  ponctuelle ne couvre pas automatiquement l'union des centres de
  `A x B x C`. Les tendances
  16 k/32 k ne sont pas reçues comme stabilité d'échelle : phase zéro corrélée
  à l'ordre WSPD, caps avant classification, une seed, ratio pondéré distinct
  de la fréquence de blocs et coût `exact_common` absent. La mesure V84
  non committée du rayon autour du barycentre, normalisée par la première
  ancre, ne mesure ni une aire ni un rétrécissement WSPD et n'est pas reçue.
  La seconde contrainte exacte à tester est la coplanarité du centre q3, sous
  forme d'un masque multi-affine aux coins des boîtes. Le pin d'audit `8f43207c`
  documente ces bornes mais ne contient encore aucun helper centre/patch v5.
  Le lemme directionnel ultérieur ouvre aussi une ablation sectorielle locale,
  sans remplacer cette route factorisée.
- **Dernier pin du chemin produit reçu :** `7e0ffe79`. Il raccorde l'enveloppe
  fermée de boules possibles aux scans q3/q4, avec son oracle géométrique
  indépendant, ses chemins batched et ses portes de non-vacuité. Le harnais de
  reçu reste épinglé séparément à `66997d56`. Aucune mesure antérieure à ce pin
  n'est attribuée au raccord d'enveloppe.
- **Cadre :** `phase=exploration_v5_hors_registre`,
  `backend=cpu_reference`, `profile=quantized_u16_input_only`,
  `mode=audit_independant_math_and_architecture`,
  `public_status=not_claimed`.
- **Frontière :** aucun résultat GPU, aucune promotion de registre, aucun
  claim d'exactitude publique ou de passage à 10–30 M. La v4 reste un
  différentiel borné, jamais une implémentation ni une preuve héritée.

## Verdict utile à Claude

Le raccord d'enveloppe est reçu dans son principe et dans son placement CPU :
il conserve le cover historique, compacte paresseusement au premier scan et
réemploie le tampon du counting sort. Le pin `7e0ffe79` ferme l'implication
géométrique indépendante, la lentille, les chemins batched et la non-vacuité
des routes surdimensionnées. Il reste à le mesurer causalement.

La base continue d'éviter la mosaïque de Delaunay d'ordre supérieur. Le filtre
réduit les sites de cœur/profondeur ; il ne retire ni visites de handles, ni
ancres, ni `corner_histograms`, ni pire exposant q4. C'est une enveloppe
**existentielle** : elle décide si un site peut appartenir à au moins une boule
admissible de l'ancre. Elle ne fournit aucun crédit universel
`h0/ha/hb/hc`, ne s'additionne à aucun compte témoin et ne se réemploie pas sur
une paire LCA qui n'est pas l'arête maximale possédée. Le verrou d'échelle
global reste donc ouvert.

La relève directement actionnable est maintenant spécifiée dans la réponse
`A x B x C` : requêtes d'arbre saturées par endpoint pour remplacer
`corner_histograms`, puis bitsets cumulatifs munis d'un index de mots non nuls
pour ne parcourir que les couples survivants. Elle conserve l'autorité exacte
aux feuilles, sépare diagonale/prune/bulk et publie les visites `MIXED`. Son
pire cas reste quadratique ; seule une pente de compteurs sur les régimes
cibles peut qualifier le gain, jamais le seul mur.

La correction de cap de l'utilisateur est reçue : **q2 n'est pas le problème
architectural à traiter**. La WSPD binaire partitionne correctement les paires ;
l'explosion naît lorsque q3/q4 développent chaque produit vivant `A x B` en
ancres, puis les tiers ou les couples de carriers. Une auto-jointure de deux
WSPD globales a été testée puis rejetée : elle recrée des millions
d'interactions dès `n=256`. La WSPD locale d'arête opposée proposée au pin
`5afcfce0` est requalifiée en **ablation q4 conditionnelle** : elle ne traite ni
q3, ni le facteur `|A||B|`, et une partition compacte de couples sans décision
sémantique peut être redondante avec le terminal shallow. Le center-cover de
blocs vient donc en premier ; la route locale ne survit que si elle évite un
travail effectivement exécuté avant ce terminal.

Le reçu `echelle_par_lane_20260829` confirme que les rescans sont un poste
majeur sur `terrain`, mais son addendum sur-interprète le routage. Le seuil
`pretest_query_min_points` porte sur les points de handles d'un rectangle, pas
sur le nombre exact de lignes d'une ancre ; le seuil 60--100 est un croisement
de modèle, pas une mesure du shallow. Les chiffres 3,9 % / 87 % et leur
ventilation ne sont pas présents dans les sorties jointes. L'enveloppe réduit
directement `scan_sites`, et un prune de bloc supprime tous les rescans aval :
center-cover et shallow restent complémentaires.

La v3/v4 proposait déjà un switch statique entre scan et arrangement. Pour le
routage du terminal, l'incrément neuf défendable est un
`adaptive_online_dispatch` : après création de l'`AnchorLineSet`, scanner un
préfixe canonique, compter le travail vraiment exécuté puis acheter le shallow
pour le reliquat lorsque son devis receipté est atteint. q3 porte la première
ablation ; q4 exige de garder les carriers du préfixe comme témoins, de masquer
seulement leur droit d'émission primaire et de fermer le ledger exact-once. Le
mode initial reste shadow/counter-only, sans reroutage produit ; le RLE ne
remplace pas la preuve de partition.

Le contre-audit des notes de Claude a eu un effet concret : la formule q4 est
requalifiée comme sur-ensemble de Jung, le seuil de coût ancien est retiré et
la fusion prématurée dans la collecte des handles est abandonnée. Ces décisions
sont intégrées à la question active ; les deux notes redondantes sont retirées
du tip.

La restriction supplémentaire par un handle $C$ est désormais un **GO
counter-only**. La v2 teste exactement au seuil si tous les supports valides
d'un bloc non capé sont profonds, et sa baseline couvre désormais toutes les
ancres. En revanche `valid_forms * rectangle_candidates` n'est ni le nombre de
rescans exécutés, ni le travail évité : les ratios 99,7/99,5 %, 78,9/76,2 % et
les facteurs de résidu 70/48 sont rétractés, comme la priorité `EMPTY` qui en
était déduite. Les blocs capés lourds sont exclus, les blocs mixtes sont mal
crédités et la chaîne réelle filtre puis s'arrête au seuil.

Le premier incrément **center-cover** sûr est moins cher que le rescan
envisagé : calculer une fois par `(A,B)` les crédits `g_AB[j]` des 64 patches
hors `A union B`, puis
laisser chaque `C` masquer seulement les patches dont `AB/AC/BC` peuvent
encore contenir zéro. `g_AB[j]` s'additionne à `h_a(a),h_b(b)` mais n'est pas
le vrai $h_0$ extérieur à `C`; il ne se compose donc pas avec un futur
$h_c(c)$ par une addition nue. La repartition scalaire est désormais
explicite sans matrice : un patch qui atteint `h_q` meurt globalement ; sinon
ses `h_q-1` positions au plus sont affectées après coup aux strates disjointes
`H_i\(A union B)` ou au bucket extérieur. Sommer les autres strates et prendre
l'union, ou `max` sans identités, dans la strate du carrier prépare $h_c$ sans
rescan témoin par `C`. Le test de puissance aux
seuls `8^3` coins reste réfuté par la fixture u16. Un cap, une tangence ou une
borne ambiguë produit `pending`, jamais un prune.

Pour q4, les deux porteurs opposés à `AB` restent une paire non ordonnée. Le
parcours peut imbriquer deux handles, mais son ledger emploie les blocs croisés
`i<j` et les diagonales `choose2(H_i)`, ferme `6*C(n_u,4)` et ne décide pas
avant le terminal lequel des deux porteurs est la face aiguë canonique. Les
formules locales retirant les recouvrements avec `A/B` ont été vérifiées sur
`442644` configurations exhaustives. La fermeture globale doit toutefois
inclure les rectangles morts avant `AliveRect`; sur une diagonale `C=D`, le
domaine de $h_d$ est vide si le handle reste indivis. La décomposition
canonique de `choose2(H)` en produits des enfants de chaque LCA fournit au
contraire `|H|-1` blocs à facteurs disjoints et autorise $h_c+h_d$ sans IDs ;
ce n'est ni une WSPD locale ni une solution au carré des handles distincts. La
masse couverte se calcule sur l'union des handles en temps linéaire en leur
nombre : aucune de ces preuves ne justifie une matérialisation globale des
couples.

Le hot path q4 ajoute désormais $h_c$ au niveau de la face ternaire, avant
toute complétion `D`, par la même provenance sparse que q3. Pour l'oracle
ultérieur qui compose simultanément `C` et `D`, si `h_c/h_d` sont ajoutés sans
positions, la seule composition scalaire générale est
`h_a+h_b+max(core,g4_AB[j],h_c+h_d)` sur un bloc croisé, avec `h_d=0` sur la
diagonale indivise. La ventilation par strates renforce ce repli en
`h_a+h_b+max(core,g_rest+max(g_C,h_c)+max(g_D,h_d))` pour deux facteurs
disjoints. Sommer le central brut et les deux fibres double-compte leurs
positions. Avec des rangs, prendre l'union capée. Ces domaines témoins ne
retranchent jamais `A/B` de la partition des carriers.

## Enveloppe q3/q4 reçue mathématiquement

- Avec `d=b-a`, `D2=|d|2`, `w=2z-a-b`, `S=|w|2-D2` et
  `Xi=|d×w|2`, q3 emploie le prédicat fermé exact
  `S <= 0 || 3*S*S <= 4*Xi` sous ses préconditions d'ancre aiguë.
- q4 emploie `S <= 0 || S*S <= 2*Xi`, sur-ensemble sûr de Jung pour les
  tétraèdres strictement bien centrés émis par la lane. Il reste intersecté
  avec le cover historique coefficient 3.
- La lentille fermée de l'ancre est incluse dans l'enveloppe q3, elle-même
  incluse dans Jung q4. Seeds et complétions historiques sont donc préservés.
- Les produits sont formés en `i128`; les frontières restent fermées. Le
  `Q_min` par distance aux intervalles est seulement un minimum continu sûr,
  pas le minimum exact du réseau u16 à parité fixée.

La dérivation reçue vit dans `../docs/MATHEMATIQUES.md` § 6.1 ; les fixtures,
les réserves d'architecture et la réponse consolidée vivent dans
`REPONSE_A_CLAUDE_BLOCS_ABC_20260829.md`.

## Réorientation WSPD q3/q4

Le contrat actif transmis à Claude est désormais :

- garder la WSPD binaire comme tape extérieur et `origin_rect_id` immuable,
  q2 restant entièrement inchangé ;
- reconnaître que `Q3FiberTask(A,B,C)`, WST3, WST4, `CellPair` et
  `Sym2(G) disjoint_union Cross(G,N)` sont des antériorités v3/v4, pas une
  nouvelle généralisation ;
- rejeter l'auto-jointure de deux WSPD globales comme hot path ;
- appliquer d'abord le center-cover exact du Théorème 5 directement aux blocs
  `A x B`, avec verdicts q3/q4 indépendants et aucune ancre matérialisée ;
- terminaliser seulement les ancres résiduelles par lignes signées, puis
  comparer en ablation q4 un range-report direct à `LocalOppositeEdgeWspd` ;
- séparer `support_lines` de `census_lines` et conserver le range-report global
  nécessaire au rang ;
- tuer seulement par certificat universel exact ; ambiguïté ou capacité
  atteinte conserve le parent et sa continuation.

Le no-go des blocs ternaires symétriques fortement séparés reste valide. La
WSPD locale partitionne les couples de cellules en `O(k_r)` blocs seulement
sous grille commune ou octree 2:1 et paramètres fixes ; cette borne ne prouve
aucun prune. Elle ne borne ni les splits `MIXED` de `A x B`, ni les visites du
center-cover, ni la somme des lignes de census. En attendant leur réception,
la frontière directe reste l'oracle q3/q4 borné.

Il ne ferme pas la fibre asymétrique `WSPDRect x Handle`, où $C$ n'est pas
séparé. Le couple `(rectangle,handle)` partitionne les rôles paire--tiers du
cover ; acuité, identités et owner restent à décider. La masse retire les
diagonales lorsque $C$ recouvre $A$ ou $B$, et le complément du cover reçoit
le fate `DEAD_OUTSIDE_WINDOW`. Le join des histogrammes est borné par le seuil
neuf seulement sur un domaine cartésien ; acuité et owner couplent sinon les
rôles. Leur calcul courant reste `O(|A|^2+|B|^2)`.

Une ablation distincte `Lca3Forest` possède un ledger exact
`sum |A||B||C| = C(n,3)` et au plus `48(n-1)` blocs littéraux sous le radix
Morton48 actuel. Elle n'est pas une WSPD ternaire : sa paire LCA n'est pas
l'arête maximale, donc ni le spindle ni le cover owner courant ne peuvent y
être réutilisés. Elle reste un comparateur combinatoire après la fibre mesurée,
pas une nouvelle route produit.

La borne honnête cible
`O(n log n + R + K log n + C + I + A + V + sum_e(m_e log m_e) + h*M + Z)`.
`R,K,C,I,A,V,E,M,Z` sont des compteurs obligatoires ; `Z` compte les sommets de
niveaux proposés, pas toutes les représentations de supports d'une même boule.
Les masses de rôles `3*C(n_u,3)` et `6*C(n_u,4)` ont leur ledger séparé. Une
borne supérieure à 95 % de pente reste un falsificateur empirique, jamais une
preuve d'exposant. Le linéaire demeure un objectif conditionnel, pas un claim.

Le premier incrément demandé n'est pas un reroutage produit : un probe
`q34_fiber` counter-only exécute un parcours témoin par rectangle, réutilise
ses crédits sur les handles, ferme les masses de positions et de rôles, exige
`anchors_materialized=0` et rejoue chaque prune à petit `n`. Ensuite seulement
vient `AnchorLineSet`, puis l'ablation de WSPD locale q4. Les seuils à
`smax=11` restent neuf intérieurs pour tuer q3 et huit pour tuer q4. La note
active détaille coefficients homogènes, tangences, concurrences, digest,
portes de coût, fixtures et mutants.

Ce probe a maintenant un contrat d'implémentation borné, mais pas encore son
code v5 : deux grilles entières typées de 64 patches à l'échelle 32, sans
flottant ; parcours partagé par deux masques mais antichaînes locales ;
scission obligatoire d'un nœud témoin contenant `A` ou `B` ; aucun cumul avec
`AliveRect::core`, aucun héritage de crédits après split. Pour l'axe `i`, les
expansions minimales `E3_i,E4_i` satisfont
`3*E3_i^2>=256*(h_j^2+h_k^2)` et
`E4_i^2>=128*(h_j^2+h_k^2)`. La médiatrice resserre ainsi le `20H` q4 v4 à
`16H` au pire ; employer la grille q3 en q4 est toutefois faux. Le profil
refuse les positions dupliquées : le ledger sémantique compte les positions,
jamais `node_weight`. Le lemme q3 analogue au Théorème 5 doit être gravé avant
tout prune q3 autoritaire.

Claude a répondu au pin `ac43ab1a` avec deux filtres q3 par groupes. Le lemme
du tiers aigu est reçu après ajout de l'owner `EdgeKey`; l'optimalité du cœur
est limitée à la boule concentrique d'une ancre ponctuelle. L'escalier
d'histogramme et les rejets de handles passent d'abord en counter-only : ils
ne sont ni gratuits, ni une preuve que seules deux boucles restent. Le rejet
non aigu sûr emploie la forme couplée `hmin_boxes(A,B,C) >= 0`, dans une vue de
seeds séparée qui ne retire jamais ces points du cover témoin. La réponse
V53--V56 est fusionnée dans la note active ; la question séparée est retirée
du tip après migration, son commit restant dans l'historique.

## Réception de la sonde de fibre v3 `1ff39ab9`

Le pin est un **GO diagnostique**, pas un reçu de ledger ni de coût. Les trois
certificats de vacuité sont sûrs dans le sens implémenté ; en particulier
`v2hi<=D2lo` prouve bien `NONE_ACUTE` et ne doit pas être inversé. L'implication
`pair_w3_dead => all_valid_supports_depth_ge_h3` est mathématiquement réelle.
Les smokes locaux `uniform` et `eight_clusters`, `n=400`, jugent chacun 301
blocs sans cap et trouvent zéro faux positif ou violation ; ils ne constituent
ni une campagne, ni un oracle indépendant.

Quatre garanties annoncées restent à corriger dans la v4 du probe :

1. `outside=full-covered` rend le ledger actuel tautologique ; vérifier
   antichaîne/disjonction puis attribuer chaque rôle à petit `n`, en incluant
   les rectangles morts par cœur dans la fermeture globale
   `3*C(n_unique,3)`.
2. La sortie capée mélange masse brute de rôles et nombre de supports valides,
   exclut encore les blocs lourds des pourcentages et ne confronte pas leurs
   certificats. Publier les objets séparément avec un intervalle valide, puis
   garder ces blocs `pending`.
3. `cout_certificateur += 3` est fixé par construction, et les appels
   `q3_power/in_spindle` appartiennent à la force brute du probe, pas au chemin
   produit. Compter aux vrais points d'appel ou instrumenter le pipeline en
   shadow avant toute causalité de gain.
4. Imprimer les cumuls `i128` sans cast u64, corriger et compléter l'empreinte
   par les `PointId`, parser strictement, échantillonner par bottom-k ou phases,
   puis ajouter le CTest exhaustif absent. Le bit dirty compilé par CMake ne
   reste pas frais après une modification du worktree.

La réponse `a0621897` ne change pas cette réception. `NONE_MAX_EDGE` reconnaît
environ la moitié des blocs vides jugés sur quatre familles, mais un taux de
blocs n'est pas un taux de travail. La faiblesse apparente de `NONE_ACUTE` ne
concerne que son gain marginal, après priorité, avec la borne découplée
`v2hi<=D2lo`. Le candidat correct est le wrapper typé
`hmin_boxes(A,B,C)>=0`, car `V2-D2=-4H` et l'acuité exige `H<0`. Le comparer en
bitmask avant tout split ou raffinement de parité.

Pour expliquer le résiduel, l'oracle petit `n` compte indépendamment les rôles
distincts, aigus, possédés et l'intersection aiguë--possédée. Un « étage le
plus profond » n'est pas une cause : deux rôles différents peuvent rendre les
deux marges non vides sans qu'aucun n'appartienne à leur intersection.
`BOX_RELAXATION` désigne l'échec du certificat de boîte face à cet oracle, pas
une cause ponctuelle concurrente. Enfin `v2lo>3*D2hi` ne crée pas un nouveau
fate : il répète le prune coefficient 3 de `rect_cover_handles` et doit être
nul sur les handles déjà rendus.

Le brouillon V68 concurrent ne ferme donc pas V70--V72. Ses tableaux viennent
d'un code modifié après `1ff39ab9` avec un bit dirty CMake périmé ; ils ne sont
pas une mesure de ce pin propre. Son étage maximal décrit le premier filtre qui
vide le résiduel dans l'ordre courant, pas une cause logique indépendante. Ne
retirer ni `NONE_ACUTE` avant d'avoir testé `hmin_boxes>=0`, ni `NONE_OWNER` du
vocabulaire d'oracle.

Avant tout split, requalifier `OwnerD2Exact` documenté par la v4 : ses extrema
corrélés de `Delta_E=|b-a|^2-|x-a|^2` et de son symétrique sont exacts
**séparément pour chaque marge** sur le produit continu des AABB, en `O(1)` et
en `i64` sous u16. Ils ne sont pas un oracle d'existence d'un même triplet qui
satisfait les deux marges. La fixture `A=[6,6], B=[7,8], C=[4,5]`, ambiguë pour
le cover et les extrema découplés, est déjà rejetée par `Delta_X_hi=-3`. Ce
contrat doit recevoir une primitive, un oracle et des fixtures v5 propres ;
aucun code v4 n'est importé.

L'identité alternative `|w|^2+2*|w.d|<=3*|d|^2` reste correcte. Son intervalle
donne le rejet `w2_lo+2*dist(0,P)>3*d2_hi`, mais après le cover coefficient 3
ce rejet est dominé par les extrema corrélés : si l'intervalle de `w.d` garde
un signe, une des deux marges est négative partout ; s'il traverse zéro, le
rejet répète le cover. Garder ce test comme identité d'oracle/mutant et exiger
`dot_only==0`, pas comme ablation produit. Ensuite seulement, une descente
transactionnelle scinde le facteur `A/B/C` qui porte l'incertitude, sous devis
en appels aval évités ; jamais `C` systématiquement.

Le rejeu indépendant de cette domination couvre les boîtes exhaustives 1D sur
`0..6`, 2D sur `0..2` et `200 000` boîtes 3D pseudo-aléatoires : `459` rejets
`dot` après cover, tous repris par un extremum corrélé, zéro `dot_only`.

Le pin `b9646d1a` donne un signal fort supplémentaire : les blocs vides jugés
ne portent aucun rescan de profondeur, tandis que les blocs non vides portent
les millions d'appels `q3_power`. Le retrait de V69 comme priorité produit est
donc reçu. Ses valeurs `0,4--2,8 %` ne sont pourtant pas un plafond absolu : le
ratio compare deux espèces d'opérations, omet les blocs capés et applique une
fréquence de blocs à une masse de rôles non mesurée. Les compteurs ajoutés sont
des évaluations du prédicat décomposé, pas des appels `is_acute_seed`; il faut
compter directement les évaluations des blocs effectivement certifiés vides,
leurs distances et owners, et les rectangles dont tous les handles de support
disparaissent. Un cap ne donne jamais `EMPTY`; un support déjà observé donne
néanmoins `NONEMPTY`, avec profondeur `UNKNOWN/CAP`. Garder `EMPTY` pour oracle et
provenance, sans chantier ni route autonomes tant que ces unités ne changent
pas la décision ; un `EMPTY` obtenu gratuitement par le center-cover reste un
sous-produit utilisable.

Le pin `50b85e16` ne transforme pas ce signal en plafond 95--99 %. Sa boucle
idéale s'arrête au premier support shallow d'un bloc mixte, mais parcourt tous
les supports d'un bloc tous-profonds ; le seau `pw_inherent` omet donc un
suffixe dépendant de l'ordre. Le rejeu propre `uniform,n=8000,seed=3` reproduit
les trois valeurs en `17,8 s`, et confirme le biais dans le code. Renommer ce
compteur en préfixe idéal, ajouter une passe sur tous les supports avec arrêt
seulement à l'intérieur de chacun, puis instrumenter le vrai pipeline. La
direction center-cover reste prioritaire ; sa cible numérique n'est simplement
pas encore mesurée.

Le second tableau de `9a51a729` mesure une intersection sur les supports exacts
déjà énumérés. Il rend un futur fast path global plausible, mais pas prioritaire
sur `g_AB[64]`. Une saturation de `g[j]` ne retire jamais `j` du masque requis
par l'intersection commune : deux patches peuvent chacun atteindre le seuil
avec des ensembles de témoins disjoints. Après la parité des compteurs, le
front pourra chercher neuf positions certifiées intérieures pour tous les
patches faisables avec un `global_required_mask` distinct, puis rendre
`UNKNOWN` en cas d'échec. La matrice
`exact_common x certified_global x certified_patch`, trois seeds et plusieurs
phases décide seulement ensuite si le raffinement paie. Fates et ledger sont
conservés dans tous les cas.

Le center-cover `g_AB[j]`, conditionné par les masques de `C`, reste donc le
premier certificateur sémantique de bloc à ouvrir en counter-only. Le shadow
visite tous les blocs, puis l'oracle stratifie les non capés après coup ; le
sélectionner d'avance sur les seuls blocs non vides serait circulaire. Sa porte
de gain ne se réduit pas à un nombre de blocs : elle publie sur le même ordre
supports matérialisés et scannés, appels `q3_power`, visites de nœuds, crédits
en vrac, sites lus, formes ou octets émis, ainsi que nœuds, patches et coins
payés par `g_AB`. Ces monnaies ne sont pas soustraites ; elles attribuent le
gain, tandis que cycles, mur et HWM appariés OFF/ON décident les contrats
produit. q4 possède ses propres compteurs. Pour le hot path, mesurer les deux
ordres `g_AB -> need résiduel -> histogrammes` et
`histogrammes -> g_AB`; l'ordre de développement ne préjuge pas du routeur
final.

Le secteur de Claude devient une ablation locale réellement utile une fois
réparé. Pour une ancre q3 fixe, la direction du centre est bien celle de la
projection de `x-m`. Il faut exposer une fois
`AnchorSectorState{P[8],counts[8]}`, puis calculer pour chaque handle un
surmasque par deux formes orientées affines sur son AABB. Le minimum restreint
ne tue que ce handle, jamais l'ancre entière ; frontière, projection nulle ou
overflow gardent les bits. Le masque coûte `O(8k)` par ancre et conserve donc
`A x B` : il peut éviter des seeds et scans de puissance dans le terminal,
mais ne prouve ni meilleur exposant ni gain GPU. Le théorème ne passe pas en
q4, où le quatrième sommet déplace le centre le long de la corde normale à la
face.

La composition ne requiert aucun bloc matérialisé. Poser
`f=min_a h_a(a)+min_b h_b(b)`. Pour un patch, poser
`credit_j=max(core,g_AB[j])` sans identités, ou la cardinalité de leur union
avec identités, puis `t_j=max(0,h_q-credit_j)` ; le patch est mort exactement
pour ce minorant si `credit_j+f>=h_q`. Pour le masque non vide d'un handle
`C`, `t_C=max_j t_j`. Le même histogramme minuscule
`P[t]=#{(a,b):h_a(a)+h_b(b)<t}` ou les bitsets `B_lt[t]` donnent les couples
que le certificateur laisse à chaque handle, sans `A x B x C`. Ce compteur
reste distinct de la masse de supports valides. Le ledger q3 pondéré est
néanmoins factorisable exactement par les histogrammes de `A intersect C` et
`B intersect C`, agrégés par seuil : coût
`O(|A|+|B|+number_of_handles+h3^2)`, sans paire d'ancre. En q4, poser le seuil
mono-handle `s_H=max_{j in M_H}(t_j)`. Comme `t_CD<=min(s_C,s_D)`, neuf classes
à `h4=8` retirent le produit de handles en
`O(k+H)` après construction des facteurs `A/B`, avec `H=sum_H |H|=O(k)`
seulement sous le cap courant des handles, avant de
passer les carriers ternaires résiduels au terminal axial. Le masque `M_H` est
celui des **complétions** possibles, jamais celui des seuls seeds aigus.

La relève avec $h_c$ garde la même factorisation, mais ne doit pas créer une
matrice `64 x handles`. Un patch qui atteint seul `h_q` devient
`SATURATED_GLOBAL` et tue tous ses supports ; sa provenance locale est opaque.
Sinon il possède au plus `h_q-1` positions certifiées, soit huit en q3 et sept
en q4 : conserver ces `GeometryIndex`, puis les affecter aux handles et au
bucket extérieur après le DFS. Pour chaque patch sous le cap, la strate locale
compose `g_{i,j}` et `h_{c,j}(c)` par union, ou par `max` sans identités ; les
autres strates s'additionnent, puis `core` reste en `max` extérieur tant que
ses positions n'ont pas été revalidées. Condenser ensuite
`tau_i(c)=max_{j in M_i(c)}(h3-credit_{i,j}(c))_+`. Avec `h3=9`, `tau` prend
les dix valeurs `0..9`, pas neuf classes. `tau=0` est une mort par profondeur,
jamais un masque vide. Un tableau `(tau,h_a)` ou `(tau,h_b)` ferme les
diagonales `c=a/c=b`. La combinaison reste linéaire en masse de handles plus
`O(h3^2)` et ne matérialise toujours pas `A x B x C`.

Le parcours témoin de `g_AB[64]` part une seule fois de la racine : nœuds
`ALL` en antichaîne locale, bits saturés retirés et `MIXED` scindés sur place.
La borne candidate inclut `V_phys+T_patch+64*h_q*log(k)`, jamais `k*V_phys` ni
une matrice de crédits `64*k`. Les surmasques géométriques des handles gardent
séparément leur coût possible `O(64*k)`. Une fixture raffine les handles sans
changer leur union : visites,
tests, fates et positions sparse doivent rester identiques, seuls leurs labels
changent. Quand `global_common` sera ajouté, son masque requis restera
distinct : une saturation de `g[j]` ne retire pas le bit commun, tandis qu'un
`ALL` d'ancêtre s'hérite sur sa branche. L'échec global reste `UNKNOWN` sans
second départ de racine.

En q4, `A x B x C` est désormais reçu comme la porte de **face** commune, pas
comme le support complet. La lane choisit déjà la face aiguë avant de parcourir
`D`. Avec les patches, `h_a/h_b`, `g4_AB` et `h_c` propres à q4, le seuil
`tau4(c)` peut donc supprimer une seed avant le scan de cœur, la corde et les
complétions. La q4 n'impose aucune coplanarité du centre avec `abc` : ses
centres vivent sur la normale à cette face. `h_d` reste réservé à l'oracle de
paires ; la route chaude passe des faces résiduelles au terminal axial. La
masse factorisée à ce stade compte seulement des slots de faces, jamais les
supports q4 dont le ledger reste `6*C(n_u,4)`.

Les vues sont typées : la partition des carriers/complétions est complète et
disjointe ; `seed_capability` lui est attaché ; une `certificate_source`
sonore peut être incomplète pour fournir des crédits sûrs par support ; seule
`exact_census_source` doit être complète pour l'absence, le ranking axial et
le census. En q4, le cover coefficient 3 couvre les sommets opposés, mais pas
tous les intérieurs : le ranking exact emploie l'arbre entier ou une source
coefficient 4 séparément prouvée. `NONE_ACUTE` ne retire jamais une complétion
ni un témoin. La fixture
`a=(0,0,0), b=(6,0,0), c=(1,-3,-1), d=(1,1,-2)` est bien centrée avec `AB`
strictement maximal, `ABc` aigu et `ABd` droit ; elle impose de choisir le seed
sur la paire non ordonnée au lieu de supposer que le premier handle l'est.

Le reçu causal utilise trois axes : `existence={EMPTY,NONEMPTY,UNKNOWN}`,
`depth={NOT_APPLICABLE,ALL_DEEP,HAS_SHALLOW,UNKNOWN}` et
`action={PRUNE_NO_EMISSION,CONTINUE,PENDING}`, complétés par
`pending_reason={NONE,CAP,MIXED,UNCHECKED}`. `ALL_DEEP` exige explicitement un
compte positif de supports certifiés, afin que l'universel vide ne fusionne
jamais les deux catégories. Un bitmask ou une liste `proof_kinds` conserve les
preuves concurrentes sans first-match. Un universel de profondeur suffit à
`PRUNE_NO_EMISSION`, même si l'existence reste inconnue ; il ne promeut
`depth=ALL_DEEP` qu'après preuve de non-vacuité. Masse brute de rôles, masse de
supports et travail aval restent trois ledgers distincts ; `blocs_morts` n'est
qu'un agrégat facultatif.

Deux fils ne doivent plus être confondus. Pour l'expérience de fibre, le
prochain incrément installe ce schéma d'état et `g_AB` counter-only sur tous les
blocs, avec un unique masque de patches mais sans `global_common` ni décision
produit. Pour la boucle d'exposant déjà connue, les histogrammes saturés et
l'émission sparse restent le premier candidat à une activation produit et
peuvent avancer en parallèle. L'ordre chaud entre les deux attend l'ablation.
Si la lane `EMPTY` est retouchée, requalifier d'abord
`OwnerD2Exact` avec oracle et fixtures v5 avant un split ; ne pas recopier son
code v4.

Ce chantier ne prétend pas inventer le center-cover : la v3 `b312638c` avait
déjà 64 patches, un unique DFS masqué et les pentes `2,104/1,896`. Le landing
CUDA `95dd8036` faisait 64 parcours logiques et n'avait pas de run natif à sa
réception. La v4 `40b309c3` mutualisait déjà la traversée haute du cover par
rectangle avant les filtres locaux par ancre. Le delta v5 à tester est donc la
composition
`WSPD rectangle -> g_AB[64] -> masques C -> t_C -> ledger pondéré q3`, avec
suppression de ces scans locaux, puis
`classes s_H -> faces ternaires -> Top-r4` en q4. La voie exacte `t_CD` reste
un oracle sous cap. La v4 avait déjà réfuté `Sym2/CellPair` comme hot path
(`459477476` nœuds à `n=4000`) et proposé le terminal axial ; ni son code, ni
ses reçus ne sont hérités. Le reçu v5 doit montrer les pentes des rectangles,
pops témoin, masques, rôles q3 proposés, faces q4, groupes de racines, scans et
census sur les tailles appariées.

Le verrou de pondération q3 est fermé mathématiquement : les corrections de
diagonales se calculent par petits histogrammes en temps linéaire en handles.
Cela ne prouve pas un temps global linéaire : l'émission q3 peut encore suivre
`sum_C P[t_C]`, et q4 paie le nombre de faces ternaires résiduelles puis le
scan/top-k et le census par face. Les handles tués comme apex restent dans la
source témoin. Ce sont désormais ces générateurs, et non un stream `k^2`, dont
les pentes constituent la porte de réfutation.

Le raccord autoritaire ajoute encore trois gardes : les témoins sont des rangs
de positions typés, pas des `PointId`; chaque patch porte
`UNSEEN/PARTIAL/EXHAUSTED/SATURATED` et son cap, car un seul
`computed_patch_mask` ne distingue ni une interruption, ni un compte exact
d'une saturation ; et une paire q4 est seed-éligible si
`seed_possible(C)||seed_possible(D)`, indépendamment de l'ordre `i<=j`. Le
cover brut, la source de certificats, le census exact, les complétions et la
capacité de seed portent des types distincts ; nommer un cover « census » ne
lui donne aucune complétude. Les tableaux `h_a/h_b` de `corner_histograms`
sont les comptes exacts de leurs sous-ensembles $W_q$ certifiés. Ils minorent
l'intersection exacte seulement pour une fibre non vide ; si la fibre vaut
zéro par convention, aucune comparaison numérique n'est faite. Après split de
`C`, ils restent sûrs pour chaque support valide mais ne prouvent ni la
non-vacuité, ni les cardinalités exactes de l'enfant.

Avant code, le masque q4 de six médiatrices reste nommé **raffiné
conservatif**, jamais exact ni preuve d'existence. Un bit faisable de patch non
calculé contribue avec `g=0` au maximum `s_H`; `UNKNOWN` appartient à
`seed_possible`, et seul `certified_no_seed` autorise le rejet. La borne 16 du
terminal axial porte sur les groupes de racines, jamais sur leurs sites ni sur
la coquille. Un parent et ses enfants ne coexistent dans aucune source sans
déduplication explicite ; `s_H` et la capacité seed ne filtrent que l'émission,
jamais le ranking ou le census exact.

## Réception du pin d'enveloppe `7e0ffe79`

### Fermé sur le pin

- build Release complet avec `-Wall -Wextra -Wpedantic -Werror` ;
- `27/27` portes ciblées locales vertes : CLI, mutants, oracle géométrique,
  familles OFF/ON et routes batched q3/q4 normales, hôte, mixtes et
  surdimensionnées ;
- appariement OFF/ON sur six familles : ordre brut à un fil, catalogue RLE,
  digests, événements avec niveaux, `batch_levels` et cardinalités par K ;
- implication indépendante `q3_power <= 0` vers l'enveloppe q3, implication
  q4 sur tous les tétraèdres bien centrés de coins de cube et inclusion exacte
  de la lentille ; aucune copie de la formule produit ne tient lieu d'oracle ;
- routes de prétest cover/requête, compteurs séparés et compaction q3/q4 non
  vacante ; les portes surdimensionnées exigent désormais
  `anchors_oversized >= 1`, et le contrefactuel à plancher impossible rend le
  code 3 ;
- fixtures strictes non axiales, frontières `i128`, point Jung q4 extérieur à
  q3, réemploi de `cover_tmp`, remapping stable de la lentille q4, garde u32
  avant matérialisation, parsing CLI exact et sonde compilée comme cible
  produit ;
- `python tools/check_docs.py` et
  `python tools/check_implementation_status.py` verts sur le pin.

### Dents restant avant mesure

1. **Ne pas attribuer le pin à CUDA.** `cli/mhgp5_cuda.cu` et les portes device
   ne parsèrent ni `--cover-envelope`, ni `--pretest-query-min`; les huit portes
   batched de ce pin sont CPU/host-shaped. Les reçus G4 historiques restent
   vrais pour leur ancien pin mais ne couvrent pas ce delta. Avant tout résultat
   GPU : parsing exact, portes device ON q3/q4 pour les wires SoA et index, puis
   reçu frais. Aucune session GPU n'est nécessaire pour recevoir le CPU actuel.
2. **Déclarer les capacités d'override.** Une option imprimée active ne peut
   être ignorée silencieusement par un exécuteur externe ; propager ou refuser
   la combinaison. Les overrides CUDA intégrés transmettent les options, mais
   l'API générique d'un exécuteur tiers ne déclare pas encore cette capacité.
   Imprimer `requested` et `applied_q3/applied_q4`, pas seulement un booléen
   global dérivé de la requête.
3. **Fermer les croisements CLI et routes batched.** Les refus CLI sont gardés,
   mais aucune porte positive ne tuerait une option acceptée puis ignorée. Le
   gate direct force cover/query ; les exécutables batched ne parsèrent pas
   `--pretest-query-min`, donc leurs compteurs par route peuvent comparer
   `0==0`. Ajouter une porte produit positive et une porte q3/q4 par route.
4. **Durcir les compteurs de travail.** Les nouveaux cumuls
   `sites_before/sites_after/cross_tests` sont en `u64` alors que leur somme
   peut théoriquement atteindre un régime cubique. Employer `u128` ou une
   saturation avec bit d'overflow avant de les dire exacts à grande taille.
5. **Finir le harnais de reçu.** `66997d56` pinne le protocole, refuse
   l'écrasement, force les vrais digests, compare les bras, grave le statut et
   l'environnement, et son auto-fixture nominale passe `5/5`. Le handler
   `INT/TERM` écrit un statut s'il manque, mais ne quitte pas explicitement le
   script. Sous `setsid`, un `TERM` pendant le premier run a laissé exécuter les
   quatre runs : codes `143,0,0,0`, sortie finale 3 et `statut=failed`, jamais
   `interrompu`. Séparer `on_signal`, sortir en 130/143, attendre ou tuer le
   descendant ciblé et vérifier qu'aucun ne survit.
6. **Comparer l'objet, pas les métadonnées.** La signature conserve toute la
   ligne `famille=`, donc `--threads=1` contre `--threads=2` produit un faux
   `DESACCORD` code 3 alors que les digests et comptes sémantiques sont
   identiques. Hasher seulement les digests et cardinalités, puis ajouter ce
   bras multithread au contrôle positif. Le selftest doit aussi vérifier les
   champs du reçu et la cause : son cas `--smax=99` produit déjà un objet vide,
   donc le code 3 ne tue pas isolément la garde `runs_non_nuls`. Enfin écrire
   `ordre_joue` ; avec trois bras le reçu annonce actuellement AB/BA pour un
   ordre ABC/CBA. Reconfigurer aussi avec un cache frais et hasher les options
   ou le `CMakeCache.txt` : le répertoire `build/recu_$nom` peut actuellement
   préexister alors que le reçu ne grave que compilateur et `Release`. Exposer
   séparément q3/q4 avant `none/q3/q4/both` reste bloqué par le booléen global
   du raccord.
7. **Réparer le budget de la porte post-séparation.** Dans la campagne à deux
   workers, `mhgp5_postsep_refine_mutant_h1` expire à `300,10 s` alors que la
   porte nominale jumelle finit en `302,74 s`. Le rejeu isolé est vert en
   `153,94 s` avec le code 4 attendu : le mutant est bien tué, mais le timeout
   de 300 s ne supporte pas la concurrence de la campagne canonique.
8. **Tester la composition des deux filtres.** Les portes batched q3/q4
   couvrent séparément `--postsep=1` et `--cover-envelope=1`, jamais leur
   activation simultanée. Ajouter un CTest croisé par lane qui exige des
   compteurs non vacants et l'égalité de l'objet canonique ; les deux familles
   de tests isolées ne tuent pas une interaction d'ordre ou de compaction.

Le filtre reste OFF par défaut. Aucun tableau de mur antérieur au refactor ne
sert de reçu. Mesurer ensuite `none/q3/q4/both` exige d'abord des commutateurs
internes par lane, car l'API courante ne possède qu'un booléen global.

## Autres coutures actives

### G0 — confinement du pool

1. Incrémenter `submitted_` seulement après admission réussie dans la file.
2. Garantir un `exception_ptr` fatal non nul sans allocation dans le fallback.
3. Remplacer les scénarios `sleep_for` par des barrières causales.
4. Relier une exception CUDA typée à `close_fatal` avant toute nouvelle prise
   de lot ; `submit_and_wait` seul ne poisonne pas le pool.

La fermeture hôte explicite est utile. Le confinement général d'une erreur
device n'est pas reçu.

### Fold vivant L2

- borner `x` avant `av[x]`, puis `fid` avant `slot_of_fid`, et parcourir toute
  la table pour détecter une entrée stale ;
- ajouter un mutant de partition à cardinalité conservée et une porte de
  capacité causalement autonome ;
- graver les deltas et `batch_levels` littéraux, niveau compris ;
- ajouter seulement `born_at/died_at` et `batch_levels` au modèle de capacité,
  les autres postes étant déjà comptés ;
- garder le bras sans rejeu comme ablation et rendre le miroir avec rejeu
  strict sur T5.

Le contre-exemple T5 et la borne de wire sont migrés dans `../docs/ECHELLE.md`.
À 10 M, le wire brut FIRST/LAST extrapolé vaut déjà environ 1,60 To tous K ;
l'ancienne ligne 620 Go est retirée.

### Grille et G1

La grille de cellules n'a plus que six coutures documentaires et
d'environnement, listées dans
`QUESTION_CLAUDE_GRILLE_DE_CELLULES_20260828.md`. Ne pas rouvrir son noyau.

Pour G1, conserver les bornes d'indices, la distinction géométrie absente/vide,
les mutants SoA réellement exécutés, un `PointId` q4 au-delà du bit 31, le
contexte géométrique partagé et une réservation exclusive du wire actif. Le
protocole est condensé dans `QUESTION_CLAUDE_LANE_RESIDENTE_20260828.md` et
`../docs/GPU.md`. Aucune nouvelle matrice G4 avant fermeture locale de G0/G1.

## Validation indépendante des pins

- configuration canonique et build Release : succès ;
- contre-audit frais sur la base `dc01fdf0` : `269` CTests découverts et les
  quatre portes ciblées `sector_reach_probe_smoke`, `sector_kill_fixture`,
  `anchor_tests_oracle` et `postsep_refine_mutant_h1` vertes. La campagne
  complète a été arrêtée après ses `38` premiers succès pour ne pas contaminer
  le mur d'un reçu concurrent de Claude ; elle ne constitue donc pas un
  verdict de suite complète ;
- campagnes ciblées enveloppe/CLI/mutants et routes batched : `27/27` ;
- registre direct : `80` mutants déclarés, `80` injectés, `80` gardés ;
- campagne complète **historique** label `gate` : `250/251` en `790,97 s`, seul
  le timeout post-séparation décrit ci-dessus ; rejeu isolé vert en
  `153,94 s` ; un rejeu frais a retrouvé ce timeout à `300,12 s` sous
  concurrence puis a été interrompu, il ne constitue pas une campagne
  complète supplémentaire ;
- reçu baseline `echelle_par_lane_20260829` : six runs produit au pin propre
  `a3c15d84`, codes nuls et compteurs exacts ; les ventilations par route et
  le proxy de concentration de l'addendum ne sont pas dans ses sorties ;
- harnais `66997d56` : syntaxe Bash valide ; les cinq scénarios de
  l'auto-fixture rendent les codes attendus et son code final vaut 0 ; le test
  causal `TERM` échoue et la comparaison inter-threads diverge à tort ;
- probe v3 puis compteurs `b9646d1a` : cible Release reconstruite ; smokes
  `uniform` et `eight_clusters` à `n=400` verts, 301 blocs jugés par famille,
  sans cap, faux positif ou violation. Le binaire reconstruit imprime le pin
  propre `b9646d1a`; les évaluations de rôles vides valent respectivement
  `5 278` et `7 402`, contre `99 389` et `461 759` appels `q3_power`, sans
  convertir ces opérations hétérogènes en gain. Le défaut de fraîcheur CMake
  reproduit au pin précédent reste à corriger et aucune porte CTest n'est
  enregistrée pour cette sonde ;
- pin `9a51a729` reconfiguré puis reconstruit : `uniform,n=8000,seed=3`
  reproduit `867/31` blocs exact-common suffisants/insuffisants et
  `1219444/47508` appels de son oracle en `18,4 s`, sur `3001` blocs. Cette
  reproduction reçoit les nombres, pas leur attribution au chemin produit ni
  le pin `1ff39ab9` encore écrit dans la note transitoire désormais retirée ;
- réduction de seuil `t_C` confrontée à l'énumération directe sur `200 000`
  configurations déterministes de patches/crédits, sans divergence ; contrats
  géométriques q3/q4 ciblés `5/5` verts (`skinny_center`, oracle q4, cover,
  exact-once et completion shaped). La nouvelle fixture de vues q4 reste à
  enregistrer avec le delta fonctionnel ;
- portes existantes de séparation cover/census q4 : `4/4` vertes
  (`q4_cover_fixture`, mutant coefficient 4, `q4_source` 22 et 13+8) ;
- pin `650b3cff` reconfiguré puis cible `mhgp5_block_witness_probe`
  reconstruite : `uniform`, `terrain` et `scanline_single_pass` à
  `n=3000,seed=3,blocs=800` impriment le bon pin propre, aucun cap, faux
  positif ou invariant violé ; le certificat `Pi` capte `0/1/0` blocs et
  `0/86/0` appels marginaux pour `101196/41943/53666` évaluations propres ;
- probe v4 de centres à ancre fixe, committé au pin `2897a03b`, puis
  reconfiguré au pin propre `73b00f3f` et construit en Release :
  `uniform/terrain/scanline_single_pass/eight_clusters` à
  `n=400,seed=3,blocs=300` rendent zéro, sans cap, faux positif, invariant ou
  ratio supérieur à un. Le binaire imprime
  `73b00f3f,worktree_modifie=non`. Pour les seuls groupes à au moins deux
  centres, les diamètres normalisés moyens valent
  `0,234/0,230/0,208/0,190`, les maxima
  `0,487/0,833/0,642/0,660` et les tests de paires
  `1733/1712/2871/29254`. Cette mesure à ancre fixe corrige V84 mais reste un
  diagnostic discret à coût quadratique, ni une aire, ni un reçu de gain ;
  un scratch rationnel indépendant sur `12678` triangles aigus et `2263`
  tétraèdres bien centrés ne trouve aucune violation des nouvelles boîtes
  racines par axe ; ce diagnostic attend ses fixtures permanentes ;
- portes sectorielles existantes reconfigurées : `10/10` vertes
  (fixture stricte, mutant non strict, oracle d'ancre, seuil `h-1`, corde et
  grille). Elles reçoivent le certificat sur huit secteurs actuel, pas le
  futur masque `Box(C) -> cônes` ni les sondes V90/V92 ;
- probe sectoriel enregistré au pin `73b00f3f`, stamp
  `worktree_modifie=non`, puis rejoué à
  `n=8000,seed=3,blocs=1500,union_rects=64` sur quatre familles. Les inclusions,
  frames et invariants de décision valent zéro. Parmi les handles non vides,
  le taux `full8 -> box` vaut `59,7 -> 88,1 %` sur `scanline_single_pass`,
  `13,4 -> 56,5 %` sur `terrain`, `57,3 -> 62,1 %` sur `uniform` et
  `97,0 -> 97,9 %` sur `eight_clusters`. Pour les unions d'ancre, tous les
  `262/136/97/177` surmasques non vides valent huit bits et le gain est zéro.
  Le filtre oracle des handles réellement non vides ne gagne que `1/1/0/0`
  ancre. Le probe grave aussi les deux contre-fixtures obliques, la frontière
  à deux bits et le repli `0xff` à projection nulle ; le smoke ajouté et les
  portes sectorielles/d'ancre rendent `11/11`. Le smoke observe `43` handles
  non vides et `120` seeds, mais son code de sortie ne verrouille encore que
  `handles_sampled>0` : ajouter ce plancher de non-vacuité avant d'en faire une
  porte sémantique. Ce replay reçoit le potentiel local, pas le tableau publié
  ni un gain mur ;
- `python tools/check_docs.py` vert sur `217` Markdown actifs,
  `python tools/check_implementation_status.py` vert sur `20` phases, et diff
  sans erreur d'espacement après consolidation.

## Ordre recommandé

1. **Fil exposant :** remplacer `corner_histograms` par les requêtes saturées
   partagées, interroger d'abord le facteur au plus faible devis, puis émettre
   via les bitsets à mots non nuls ; fermer égalité des comptes, masse et ordre
   sur q2/q3/q4 avant activation.
2. **Fil fibre, développable en parallèle mais non autoritaire :** corriger
   ledger, caps, compteurs et échantillonnage du probe, installer les trois axes
   d'état, graver le helper centre/patch typé contre l'oracle exact, puis
   calculer seulement `g_AB[64]` par un DFS masqué sur tous les rectangles.
   Dès ce premier DFS, un patch saturé reçoit un fate global sans provenance ;
   un patch sous le cap conserve ses huit positions q3 au plus, affectées aux
   strates seulement après le parcours. Aucune matrice par handle ni poursuite
   après saturation n'est admise. Ce petit supplément prépare $h_c$ sans
   rescanner la racine et interdit le futur double compte local.
   Comparer aux 64 parcours indépendants, fermer antichaînes, coquilles et
   provenance, puis former `t_C` en post-traitement sans `A x B x C`. Ouvrir
   seulement ensuite `global_common` avec un masque requis distinct des
   saturations `g` et la fixture des intersections disjointes. Un échec global
   rend `UNKNOWN`; le shadow q3 ferme toute sa masse par histogrammes, même si
   `P[t]>0`. Aucun masquage produit n'est autorisé avant oracle. En ablation
   parallèle, conserver le `sector_reach_probe` réparé avec ses vrais cônes
   entiers et sa porte `exact_seed_mask subset_of box_mask`, puis shadow-er un
   mapping `position -> seed_handle_id` qui mesure les carriers et appels de
   puissance réellement évités après les portes existantes. Les positions des
   handles morts restent témoins et census ; ne jamais convertir ce verdict
   handle-local en kill d'ancre ni le transférer à q4.
   Seulement sur le résiduel mesuré, calculer `h_{c,j}(c)` par la forme
   `Phi32` avec split diagonal, condenser `tau(c)` sur le pire patch et fermer
   sa masse par les classes de seuil avant toute émission sparse.
3. Comparer ensuite les deux ordres chauds sur le vecteur causal et sur mur/HWM
   OFF/ON ; activer seulement les décisions exactes rentables. Garder les fates
   `EMPTY` au ledger d'oracle sans route autonome.
4. En q4, ouvrir d'abord la porte ternaire counter-only : seuil `tau4(c)` sur
   les faces `A x B x C`, sans coplanarité q3, puis mesurer les scans de cœur,
   morceaux de corde et paires `(seed,d)` réellement évités. Enchaîner
   `q4_threshold_axial_probe` : helper pur des neuf classes `s_H` contre
   l'oracle `CD` borné, avec mutants bit de patch absent, `UNKNOWN->NO` et
   `max->min`; puis noyau CPU-reference Top-r4 pour une face fixe, source arbre
   entier et ties non tronqués. Alimenter son ranking par
   l'`exact_census_source`, sans filtre de capacité seed, et comparer
   `tau4+threshold+axial` sur les faces, sites lus, groupes, census, mur et HWM.
   `Sym2`, la WSPD locale q4 et le stream exact de paires restent des ablations,
   jamais des prérequis. Fermer parallèlement les capacités d'override et les
   portes CUDA avant tout reçu GPU.
5. Fermer ensuite G0/G1, fold vivant et grille selon leur ordre local ; aucun
   de ces chantiers ne doit masquer les compteurs de la nouvelle source.

GCP non utilisé.
