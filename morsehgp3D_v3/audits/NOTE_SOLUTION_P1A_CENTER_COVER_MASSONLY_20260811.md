# Note auditée — falsificateur de masse P15-HOCUDA-P1a

Date : 11 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Portée et verdict indépendant

`P15-HOCUDA-P1a` est exclusivement la tranche **q4 mass-only au seuil
huit**. Elle partitionne implicitement les paires, compte la masse supprimée
et la masse terminale, mais n'émet ni paire, ni ancre, ni support. q3 et son
seuil neuf appartiennent au futur P1 complet, hors de cette porte.

Le théorème de prune corrigé ci-dessous est admissible comme certificat
conditionnel exact : si chacun de ses tests entiers stricts et chacun de ses
reçus sont satisfaits, le bloc ne peut porter aucune activation q4 non inerte
dont il possède l'arête maximale canonique. Ce verdict reçoit la preuve du
certificat, pas une implémentation, une complexité, un résultat à 50 k, la
complétude de P1 ou un SLO. Le statut logiciel et les octets effectivement
testés appartiennent exclusivement à
[AUDIT_ETAT_COURANT.md](AUDIT_ETAT_COURANT.md).

Aucune implémentation v3 de ce contrat n'est reçue. Un prior art q4/binary64
existe au commit `95dd8036a2fcb36c8a7b6aeb7c44197d9c9f7e03`, mais sa cible
CUDA native n'a jamais été compilée ni exécutée et aucun reçu G4 ne lui est
attaché. Il sert de comparateur d'architecture, selon
[AUDIT_REEMPLOI_YAO48_P1A_LIGNE_ENREGISTREE_20260811.md](AUDIT_REEMPLOI_YAO48_P1A_LIGNE_ENREGISTREE_20260811.md), jamais d'autorité v3.

Avant arrondi entier, le rayon fondé sur la distance maximale ci-dessous est
plus serré que le rayon `5H/8` de la roadmap. Après les arrondis extérieurs, sa
boîte entière reste sûre mais n'est pas nécessairement incluse dans l'ancienne
boîte réelle : pour `A=(0,0,0)` et `B=(1,0,0)`, elle donne `[-1,2]` sur l'axe
actif, contre `[-1/8,9/8]` avant arrondi dans l'ancien schéma. Le seul claim
universel est donc celui d'un majorant exact, souvent plus sélectif à grande
échelle. Il conserve le théorème de Jung et ne dispense ni du parcours
collectif de nœuds témoins, ni du ledger de la partition.

## 1. Univers implicite et propriété à prouver

Une disposition unique `(MortonKey, PointId)` et son LBVH fournissent une
partition triangulaire canonique :

$$\mathcal{T}(N)=\mathcal{T}(L)\mathbin{\dot\cup}(L\times R)\mathbin{\dot\cup}\mathcal{T}(R).$$

Chaque paire non ordonnée possède donc exactement un bloc croisé au cours du
raffinement. Pour un support q4 propre positif, le juge choisit d'abord les
arêtes de distance carrée maximale, puis la plus petite paire ordonnée de
`PointId` parmi les ex æquo. Seule cette arête canonique engage le sort du
support dans P1a.

Pour un bloc croisé de plages disjointes `(A,B)`, le prune exige que chaque
patch non certifié infaisable possède huit `PointId` distincts, hors de
`A∪B`, strictement intérieurs pour tout centre du patch et toute paire du
bloc. Un échec de preuve conserve et partage le bloc. Une microtuile terminale
est seulement comptée; P1a ne la résout pas.

## 2. Domaine de centres q4 et 64 patchs

Soient une paire maximale `(a,b)`, `d=b-a`, `D^2=\lVert d\rVert^2`,
`M=(a+b)/2` et un centre `c=M+t`. Le plan médiateur donne
`t\mathbin{\cdot}d=0` et `r^2=D^2/4+\lVert t\rVert^2`. Jung en dimension
trois donne `r^2\leq3D^2/8`, donc :

$$\lVert t\rVert^2\leq\frac{D^2}{8}.$$

La positivité propre est indispensable : le centre circonscrit appartient à
l'intérieur du tétraèdre, donc sa sphère est la miniboule des quatre sommets et
Jung s'applique. Cette borne ne doit jamais être étendue à un quadruplet non
positif seulement parce qu'il est cosphérique.

Poser `X=\mathrm{maxdist}^2(A,B)`, calculé exactement sur les AABB. Alors
`D^2\leq X`. Le rayon entier extérieur est défini sans division tronquée :

$$R_t=\min\left\lbrace r\in\mathbb{Z}_{\geq0}:8r^2\geq X\right\rbrace.$$

Avec les arrondis dirigés vers l'extérieur, le domaine

$$T_0=\prod_{d=1}^{3}\left\lbrack\left\lfloor\frac{A_{\mathrm{lo},d}+B_{\mathrm{lo},d}}{2}\right\rfloor-R_t,\ \left\lceil\frac{A_{\mathrm{hi},d}+B_{\mathrm{hi},d}}{2}\right\rceil+R_t\right\rbrack$$

contient tous les centres pertinents. Le calcul pratique prend
`r=\mathrm{isqrt}(\lfloor X/8\rfloor)` puis incrémente si et seulement si
`8r^2<X`.

Sur chaque axe entier `[l,h]`, les bornes
`b_j=l+j(h-l)/4` pour `j=0,\ldots,4` définissent quatre intervalles fermés
indexés. Leurs produits donnent exactement 64 patchs indexés. Les coins ont
des coordonnées `c=C/4`; les chevauchements de frontière sont volontaires.
Chaque index reçoit un statut, même lorsque deux patchs dégénérés coïncident.
L'ordinal scellé est `patch_id=j_x|(j_y<<2)|(j_z<<4)`. Toute égalité conserve
le patch.

Deux fixtures verrouillent les arrondis :

- `(0,0,0),(0,0,1),(0,2,0),(2,1,1)` forme un q4 propre positif de centre
  `(3/4,1,1/2)` et de diamètre carré six. Pour l'arête
  `(0,0,0)--(2,1,1)`, une racine fautive `\mathrm{isqrt}(6/8)=0` omet le
  centre; la valeur correcte est `R_t=1`. L'autre diamètre carré six est
  `(0,2,0)--(2,1,1)` : l'ex æquo exerce donc aussi le tie-break lexicographique;
- `(11,11,11),(11,9,9),(9,11,9),(9,9,11)` est le cas d'égalité de Jung :
  `D^2=8`, `r^2=3` et `\lVert t\rVert^2=1`. Un test large à la place d'un
  test strict supprimerait un vrai centre.

## 3. Certificats d'infaisabilité d'un patch

Pour un patch rationnel `T`, poser les numérateurs entiers
`N_{\min}(T,S)=16\,\mathrm{dist}^2(T,S)` et
`N_{\max}(T,S)=16\,\mathrm{maxdist}^2(T,S)`. Un patch peut être déclaré
infaisable seulement si au moins une preuve stricte est vraie :

- `N_{\min}(T,A)>N_{\max}(T,B)` ou la relation symétrique;
- `N_{\min}(T,A)>6X` ou `N_{\min}(T,B)>6X`, par Jung q4;
- `N_{\mathrm{mid}}(T)>2X`, où
  `N_{\mathrm{mid}}(T)=16\,\mathrm{dist}^2(T,M_{AB})` et `M_{AB}` est la
  boîte de tous les milieux.

Les bornes dirigées `L/U` plus sélectives de la roadmap complètent ces trois
tests dans le profil P1a sanctionné et publient leur gain marginal. Une
implémentation qui les omet reste un diagnostic volontairement plus faible et
ne peut pas réfuter la route P1a complète. Zéro, overflow, intervalle indécis
ou preuve absente laisse un **patch survivant**; ce terme ne prétend pas qu'un
centre y existe réellement.

Le test par les milieux est sûr car tout vrai centre vérifie
`\mathrm{dist}^2(c,M_{AB})\leq X/8`. Il élimine notamment des coins diagonaux
artificiels du pavé `T_0`.

## 4. Certificat témoin exact, échelle 16

Pour un point `z`, un côté d'extrémités `S` égal à `A` ou `B` et un centre
`c`, poser :

$$f(c,s,z)=\lVert s\rVert^2-2c\mathbin{\cdot}s-\lVert z\rVert^2+2c\mathbin{\cdot}z.$$

La condition `f>0` équivaut à `\lVert c-z\rVert^2<\lVert c-s\rVert^2`.
Pour `c` fixé, le minimum continu sur l'AABB de `S` est atteint par
`s_d=\mathrm{clip}(c_d,[S_{\mathrm{lo},d},S_{\mathrm{hi},d}])`. Après cette
minimisation, `h(c)=\mathrm{dist}^2(c,S)-\lVert c-z\rVert^2` est concave :
son minimum sur un patch boîte est donc atteint à un de ses huit coins.
L'emploi de l'AABB continue est exact pour la boîte et seulement suffisant,
donc conservateur, pour les points réels du nœud.

Pour un coin `c=C/4`, définir
`Q_d=\mathrm{clip}(C_d,[4S_{\mathrm{lo},d},4S_{\mathrm{hi},d}])`. Le test
entier exact est :

$$F(C,z,S)=\sum_d\left(Q_d^2-2C_dQ_d\right)-16\lVert z\rVert^2+8C\mathbin{\cdot}z=\sum_d\left((Q_d-C_d)^2-(4z_d-C_d)^2\right)>0.$$

Le facteur correct est seize, pas quatre. L'exemple unidimensionnel
`c=1/4`, `S=[0,1]` donne un minimum `-1/16` et réfute une échelle quatre.
Le témoin passe seulement si `F>0` aux huit coins; `F=0` échoue.

Pour `u16`, `X\leq3\cdot65535^2`, `R_t\leq40132` et
`\lvert Q_d-C_d\rvert,\lvert4z_d-C_d\rvert\leq422668`. Ainsi
`\lvert F\rvert\leq3\cdot422668^2<2^{39}` : `i64` suffit au sujet et le juge
emploie `i128`. Chaque opérande est élargi en `i64` avant soustraction,
multiplication ou carré, notamment pour `3X`, `6X` et `8r^2`; un résultat final
borné ne rend pas un intermédiaire `int32` acceptable. `F` est évalué sous la
forme différence de carrés ci-dessus.

Chaque crédit reçu est
`(patch_id,witness_range_or_PointIds,chosen_side)` avec `chosen_side` dans
`{A,B}`; des crédits du même patch peuvent choisir des côtés différents. Le
juge rejoue le côté engagé. Au vrai centre les deux rayons coïncident. Les huit
témoins sont dédupliqués par `PointId` et exclus des plages `A` et `B` entières.
Cette exclusion est une règle structurelle de reçu : un identifiant
d'extrémité ne peut pas fournir un crédit universel pour toutes les paires du
bloc.

## 5. Range-query collective obligatoire

Tout témoin strict d'un vrai centre vérifie
`\lVert z-c\rVert^2<r^2\leq3X/8`. La boîte de recherche sûre est donc
`T_0\mathbin{\oplus}[-R_w,R_w]^3` avec :

$$R_w=\min\left\lbrace r\in\mathbb{Z}_{\geq0}:8r^2\geq3X\right\rbrace.$$

Le calcul exact prend `r=isqrt(floor(3X/8))`, après élargissement signé, puis
incrémente si et seulement si `8r^2<3X`.

Ce crop est une garantie de complétude spatiale, pas une borne de complexité :
au pire u16, `R_w=69511` et il peut couvrir tout le nuage.

Le domaine global est seulement un majorant. Le masque initial d'un patch
emploie le crop plus serré `T_i\mathbin{\oplus}[-R_w,R_w]^3`; un nœud témoin
ne conserve que les bits dont ce crop intersecte son AABB. Le parcours
industriel porte des états
`(witness_node, active_patch_mask)`. Pour un nœud témoin `W` disjoint des
plages d'extrémités et un coin `C`, les deux quantités
`D_S(C)=\min_{s\in S_{\mathrm{box}}}\lVert C-4s\rVert^2` et
`E_W(C)=\max_{z\in W_{\mathrm{box}}}\lVert C-4z\rVert^2` sont séparables et
exactes. Si `E_W(C)<D_S(C)` aux huit coins d'un patch, tous les points de
`W` sont des témoins pour ce patch et sa plage peut créditer la banque jusqu'à
huit. Une borne supérieure dirigée `U_W\leq0` peut rejeter un bit; toute autre
situation partage uniquement les bits ambigus. À une feuille, le test ponctuel
`F` décide.

Les nœuds acceptés forment une antichaîne de plages disjointes **par patch**.
Le parcours s'arrête pour un patch dès huit `PointId` distincts. Un nœud qui
chevauche `A` ou `B` descend jusqu'à des sous-arbres canoniques réellement
disjoints; il n'est jamais crédité par la seule soustraction de la masse
chevauchante. Toute plage résiduelle reconstruite reçoit une nouvelle AABB et
un nouveau certificat. Le préfiltre
`n-\lvert A\rvert-\lvert B\rvert<8` s'applique seulement aux produits croisés
dont les deux plages sont disjointes, avec additions et soustractions vérifiées.

Un rescan de la racine par bloc n'est pas admis. Une antichaîne du parent peut
servir de hint après split, mais les 64 grilles enfants diffèrent : overlap
d'extrémités, crops et inégalité aux huit nouveaux coins sont tous recertifiés.
Aucun crédit ni ordinal de patch parent n'est hérité comme vérité. Une
wavefront collective peut fournir la même propriété sans vecteur de candidats
par bloc.

## 6. Machine, ledger et mémoire

La politique v3 est **microtuile avant cover**. Le seuil `microtile=mu` est
scellé et le preflight exige `mu>=leaf_size^2`, afin que toute tâche
insécable soit terminale. L'automate est exact :

1. une tâche de masse nulle disparaît; une tâche de masse au plus `mu` commet
   cette masse dans `P_microtile` sans évaluer les 64 patchs;
2. un self-bloc non terminal est remplacé par les deux self-enfants et leur
   produit croisé selon la partition triangulaire;
3. un produit croisé non terminal est pruné seulement si ses 64 patchs sont
   soit certifiés infaisables, soit couverts au seuil huit; sinon il est
   partagé.

Si un seul côté croisé est interne, ce côté est partagé. Si les deux le sont,
comparer `(span,cardinality)`; partager le plus grand couple lexicographique,
puis le côté de plus petit `NodeId` en cas d'égalité. Les enfants sont poussés
par `NodeId` canonique. `leaf_size`, `microtile` et ces tie-breaks sont scellés
dans la provenance. Le prior art `95dd803` tentait le cover avant le terminal :
ses profils de patchs ne sont donc pas comparables directement à cette
politique.

Le ledger transactionnel par shard vérifie en permanence :

$$P_{\mathrm{prune}}+P_{\mathrm{microtile}}+P_{\mathrm{pending}}=P_{\mathrm{seed}}.$$

Un split remplace la masse exacte du parent par celles de ses enfants :
`\binom{s}{2}=\binom{l}{2}+lr+\binom{r}{2}` pour un self-bloc et
`\lvert A\rvert\lvert B\rvert=\lvert A_L\rvert\lvert B\rvert+\lvert A_R\rvert\lvert B\rvert`
pour un produit. Un identifiant canonique de chemin interdit le double commit.
À la fermeture, `P_{\mathrm{pending}}=0` et
`P_{\mathrm{prune}}+P_{\mathrm{microtile}}=\binom{n}{2}`.

Le profil 50 k conserve seulement le LBVH `O(n)`, les piles privées des
shards, les blocs actifs et, par bloc, un masque, 64 compteurs plafonnés à
huit et leurs antichaînes bornées. Il interdit tout tableau `Q\times64`,
liste globale de candidats, reçu par bloc, arène de paires ou frontière
persistante de tous les états. Les reçus développés sont limités au
différentiel borné.

Le chemin sanctionné est sans budget configurable. Un cap de diagnostic peut
faire retomber le prune fail-open ou refuser le probe, mais sa sortie porte
`slo_eligible=false` et ne qualifie ni P1a ni un SLO. Une capacité physique
réellement insuffisante échoue atomiquement; elle ne transforme jamais une
frontière en résultat complet.

## 7. Juge indépendant borné

La campagne sanctionnée à `n=32` matérialise les sorts uniquement dans le
juge. Celui-ci ne partage ni le découpage, ni `clip`, ni les bornes du sujet.
Il reconstruit :

1. `X`, `R_t`, `R_w`, `T_0` et les 64 indices, avec exactement un statut par
   patch;
2. chaque preuve stricte d'infaisabilité;
3. chaque côté témoin, les huit coins, les huit `PointId` effectivement
   crédités, leur appartenance aux plages résiduelles et les intervalles de
   nœuds acceptés;
4. chaque split, chaque masse et exactement un sort par paire.

Si un nœud accepté contient plus de points que le crédit restant d'un patch,
le reçu sélectionne canoniquement les premiers `PointId` de sa plage Morton
résiduelle. Un simple couple `(node,mass)` ne suffit pas après un overlap : le
juge reconstruit la décomposition disjointe et la sélection exacte. Pour un
témoin réel et un point réel `s`, le rejeu direct au coin `C/4` compare en
`i128` :

$$\sum_d(C_d-4z_d)^2<\sum_d(C_d-4s_d)^2.$$

Pour `s` fixé, la différence est affine en `c`; les huit coins suffisent sans
réutiliser `clip`.

En parallèle, un oracle rationnel ou déterminantal indépendant énumère les
q4 propres positifs, leur sphère, leurs barycentries, leur profondeur stricte
et leur arête maximale canonique. Tout support q4 propre positif avec moins de
huit intérieurs stricts doit avoir son arête propriétaire hors de la masse
prunée. Cette vérification ne ferme ni le shell ni la régularité d'une
activation. Les primitives v2 peuvent servir de différentiel supplémentaire,
jamais d'autorité unique.

Le juge développe les blocs prunés et les microtuiles, exige une multiplicité
un par paire et compare séparément nombres et masses. Cette bijection tue une
omission compensée par un doublon, qu'une égalité globale laisserait passer.

## 8. Fixtures, mutants et non-vacuité

Les fixtures permanentes couvrent :

- les deux cas exacts de racine et d'égalité de Jung de la section 2;
- un bloc réellement prunable et un support q4 non inerte dont l'arête
  canonique tombe dans un bloc candidat;
- un patch survivant sans huit témoins, les frontières de patch, les extrêmes
  u16, un chevauchement de plage témoin avec une extrémité et une permutation
  des `PointId`;
- le contraste entre l'ancienne boîte `5H/8` et le nouveau `T_0` entier, ainsi
  qu'une microtuile prouvant que les 64 patchs ne sont pas évalués avant le
  terminal;
- un bloc diagonal si le prune center-cover y est un jour activé; tant que la
  preuve et cette fixture manquent, un self-bloc est seulement partagé en ses
  deux enfants et leur produit croisé;
- le profil distinct exigé; des positions colocalisées sont rejetées au
  preflight, pas transformées en supports propres.

Les mutants suivants doivent mourir au code 4 :

- `clip-scale-4` et `ceil-root-truncated`;
- `patch-omitted` sur une fixture dont le vrai centre appartient à l'intérieur
  du patch omis;
- `feasible-declared-infeasible` et `strict-to-large`, qui remplace `>` par
  `>=`;
- `corner-skipped`, `witness-duplicated` et `witness-strict-to-large`;
- `witness-subtrees-overlap` et `endpoint-overlap-subtracted`, qui créditent
  deux fois une feuille ou retirent seulement une masse sans reconstruire la
  plage résiduelle;
- `stale-inherited-credit`, qui réutilise un bit de patch parent sans
  recertifier les coins de l'enfant;
- `q4-radius-replaced-by-q3`, qui sous-majore dangereusement le domaine;
- `microtile-truncated` et `terminal-compensated`.

La présence d'un témoin dans `A∪B` est vérifiée comme règle structurelle du
reçu. Ce n'est pas un mutant mathématique autonome : sous le prédicat strict,
une vraie extrémité produit déjà une égalité et ne peut passer.

Les planchers de prunes, masse prunée, splits, microtuiles, patchs survivants,
nœuds acceptés, feuilles ambiguës et témoins ponctuels échouent au code 3
lorsqu'ils ne sont pas exercés. Une permutation ne doit préserver que
l'exactitude et les sorts canoniques après renommage; un filtre suffisant et
incomplet n'est pas tenu de produire la même partition prune/microtuile.

## 9. Profil 50 k et conditions no-go

Après Release stricte, ASan/UBSan hôte et le différentiel sanctionné à `n=32`,
une unique session G4 gardée construit le cubin AOT `sm_120`, ferme la parité
fake/native, puis exécute les reçus natifs `n=32`, leur rejeu exact et
Compute Sanitizer. Seulement alors elle passe directement aux deux profils
50 k `uniform_latin` et `eight_clusters`, sans palier intermédiaire ni retry
automatique de performance. P1a ne demande donc pas deux exposants; les gates
`12 500/25 000/50 000` concernent les autres routes de source.

Avant la session, chaque famille reçoit une définition u16 exécutable, une
graine, une règle de quantification, une politique de rejet des doublons et un
digest de nuage attendus. Le reçu scelle aussi commit, sources, options, cubin
et binaire. Pour le seuil chaud, un warmup complet de configuration identique
est explicitement hors mesure, puis un seul run sanctionné est mesuré; un échec
ne déclenche aucun retry de performance.

Le profil publie au minimum :

- `Q`, blocs tentés, splits, microtuiles et l'identité logique
  `patch_slots=64Q`;
- patchs infaisables par motif, survivants, couverts et ambigus;
- visites patch--nœud `V_W`, nœuds range, points candidats, tests
  point--patch, évaluations coin--clip, reprises racine, hints parent réutilisés
  et crédits enfants recertifiés;
- masse prunée, masse terminale, masse moyenne par prune, distributions
  p50/p95/p99/max des visites et des antichaînes;
- piles, queue, blocs actifs, continuations, déséquilibre CTA, durée de queue
  finale, octets physiques, allocations, memsets, kernels, synchronisations,
  H2D et D2H;
- temps LBVH et source--cover séparés, avec une seule synchronisation
  terminale;
- digests des entrées et des artefacts, options exactes, temps de warmup exclu
  et identité du run sanctionné.

La route est no-go avant extension si la majorité de la masse atteint les
microtuiles, si les 64 patchs survivent mais se couvrent rarement, si un
rescan racine par bloc subsiste, si `Q` ou `V_W` révèle le régime quadratique
ou cubique, si la p99/max sérialise la fin, si une allocation ou un memset a
lieu par bloc, ou si source--cover seul dépasse son enveloppe secondaire de
200 ms. Le futur P1 refuse aussi source--cover plus cordes au-dessus de
400 ms.

Même favorable, ce profil n'émet aucune ancre et ne mesure ni cordes, shallow,
décision exacte, reducer, dix forêts, verticales, lots, certificat minimal ou
retour hôte. Il ne qualifie donc ni P1, ni les 100 ms principaux, ni la seconde
secondaire de `BenchmarkOutputContract-v1`.

GCP non utilisé pour cette note.
