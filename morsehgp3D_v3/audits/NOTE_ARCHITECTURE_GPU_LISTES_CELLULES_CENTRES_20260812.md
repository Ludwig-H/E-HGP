# Architecture proposée — listes imbriquées de cellules de centres

Date : 12 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Cette note propose une route constructive q3/q4 exacte sous ledger terminal et
adaptée au GPU. Sa parcimonie reste à démontrer par les compteurs : les listes
et les cliques peuvent être combinatoires. Elle ne modifie pas le code de Claude
et n'ouvre aucune session G4. Elle est distincte de P1a : P1a certifie seulement
une masse q4 H0-inerte; la machine ci-dessous localise des centres, engendre des
supports et produit leur census.

## 1. Théorème de localité hiérarchique

Soit `X` le nuage, `C` une cellule de centres bornée non vide et `K_C` sa
fermeture. Pour tout site `x`, poser

$$l_C(x)=\min_{c\in K_C}\left\Vert x-c\right\Vert^2,\qquad u_C(x)=\max_{c\in K_C}\left\Vert x-c\right\Vert^2.$$

Pour une arité `q`, poser `t_q=smax-q+1`. Si `t_q<=n`, soit `R_q(C)` la
`t_q`-ième statistique d'ordre du multiensemble des `u_C(x)`, puis

$$A_q(C)=\left\lbrace x\in X:l_C(x)\leq R_q(C)\right\rbrace.$$

Si `t_q>n`, la branche petit nuage prend `A_q(C)=X` et n'applique aucun prune
fondé sur la statistique manquante.

### Monotonie sous subdivision, cas global

Si la fermeture d'un enfant `D` est incluse dans celle de `C`, alors
`l_D(x)>=l_C(x)` et `u_D(x)<=u_C(x)`. Par monotonie de la statistique d'ordre,
`R_q(D)<=R_q(C)`, donc `A_q(D)` est inclus dans `A_q(C)`.

Lorsque le domaine actif enfant est réellement inclus dans le domaine actif
parent, le calcul enfant est local et donne la même liste que le calcul depuis
`X`. Tout `x` hors de `A_q(C)` vérifie
`u_D(x)>=l_D(x)>=l_C(x)>R_q(C)>=R_q(D)`. La `t_q`-ième valeur de `u_D` se
calcule donc uniquement sur `A_q(C)`, puis le filtre enfant uniquement sur
cette même liste. Aucun rescan global n'est requis après la racine.

Cette conclusion ne s'applique pas littéralement si l'on calcule les bornes
sur `tight=C intersection bbox(P_C)`, puis si l'on transmet la liste obtenue à
un enfant dyadique `D` qui n'est pas inclus dans `tight`. Dans ce cas, ni le
seuil calculé depuis le pool, ni la liste ne doivent être appelés le
`R_q(D)` ou le `A_q(D)` **global**. L'invariant exact utile est plus faible.

### Conservation relative au pool hérité

Soit `P` un pool qui contient déjà `I_B union U_B` pour toute boule pertinente
encore possédable par le domaine compact `K`. Définir `R_(p,P)(K)` comme la
`(p+1)`-ième plus petite borne supérieure calculée seulement sur `P`, puis
`D_(p,P)(K)={x in P:l_K(x)<=R_(p,P)(K)}`. Si le centre `c` de `B` appartient à
`K` et si `B` a exactement `p` intérieurs globaux, alors :

$$\beta_B\leq R_{p,P}(K)\quad\text{et}\quad I_B\cup U_B\subseteq D_{p,P}(K).$$

En effet, l'inégalité opposée fournirait `p+1` membres de `P` strictement
intérieurs, ce qui contredirait le census global. Le filtre conserve donc à
nouveau `I_B union U_B`, et l'induction recommence dans chaque enfant. Le
resserrement par `bbox(P)` ou par un k-DOP est sûr pour cette propriété, car la
positivité impose `c in conv(U_B)`, avec `U_B` inclus dans `P`.

Cet invariant reçoit la **complétude des supports pertinents**, pas l'égalité
des listes avec un rescan global ni la monotonie numérique des seuils entre
deux domaines actifs non emboîtés. Les reçus doivent publier `pool_digest` et
`active_domain_digest` séparément. Une fixture doit exercer un enfant dyadique
qui déborde du `tight` parent tout en gardant un centre pertinent dans leur
intersection.

La même monotonie existe entre arités dans une cellule :
`t_{q+1}=t_q-1`, donc `R_{q+1}(C)<=R_q(C)` et
`A_{q+1}(C)` est inclus dans `A_q(C)`. Les listes q3 puis q4 peuvent être
compactées depuis q2. Elles ne se déduisent jamais des seuls `t_q` témoins.

### Complétude et census local

Soit `U` un support minimal positif de taille `q`, de centre `c` owner de `C`,
de rayon carré `beta` et avec `p` points strictement intérieurs. Si
`p+q<=smax`, alors `beta<=R_q(C)`. Sinon, les `t_q=smax-q+1` sites aux plus
petits `u_C` seraient tous strictement intérieurs à la boule, donnant
`p>=t_q`, contradiction.

Pour tout site `x` dans la boule fermée,
`l_C(x)<=||x-c||^2<=beta<=R_q(C)`. Par conséquent

$$I_B\cup U_B\subseteq A_q(C).$$

Réciproquement, tout site hors de `A_q(C)` est strictement extérieur. Un scan
exact de `A_q(C)` donne donc le census **global** complet de la boule sans
interroger le reste du nuage. C'est la propriété décisive pour un GPU : une
liste de cellule devient simultanément domaine de génération et certificat de
census.

Enfin, la positivité donne `c` dans `relint conv(U)`, donc dans
`conv(A_q(C))`. Une séparation stricte entre `K_C` et `conv(A_q(C))` exclut
tout support positif possédé par la cellule. Un contact ou une égalité reste
toujours dans la branche conservée.

### Stratification par budget d'intérieurs

Une version plus sélective indexe la liste par un budget `h` d'intérieurs. Pour
`h>=0`, définir `R_h(C)` par la `(h+1)`-ième plus petite valeur de `u_C`, puis
`D_h(C)={x:l_C(x)<=R_h(C)}`. La notation `D_h` distingue cette enveloppe de
`A_q`, indexée par l'arité; `A_q=D_(smax-q)`. Toute boule owner
ayant exactement `p` intérieurs vérifie `beta<=R_p(C)` et
`I_B union U_B` inclus dans `D_p(C)`; sinon les `p+1` témoins seraient tous
strictement intérieurs.

Les `D_h` croissent avec `h` et restent imbriqués sous subdivision. Pour q3,
`h=0..8`; pour q4, `h=0..7`. Une seule CSR `D_8` suffit, avec un rang d'entrée
`tau_C(x)=min{h:x in D_h(C)}`. Pour un support proposé `U`, définir l'entrée
immuable `e0=max(tau_C(x):x in U)`, puis un curseur `h=e0`. Après le scan
complet de `D_h`, soit `r_h` le compte intérieur total. L'invariant est
`h<=r_h<=p`. Si `r_h<=h`, le census est global et `r_h=p=h`; si `r_h>h`, poser
`h=r_h` et scanner seulement les nouveaux buckets. Un dépassement de
`smax-q` prouve `above_support_window` sans publier de shell partiel. Les IDs
de puissance nulle sont accumulés à chaque bucket, pas seulement les
intérieurs.

`e0`, la cellule, le cloud/epoch, `R_h` et `tau` restent figés pendant la
promotion. Une partition terminale **commune** à tous les budgets d'une arité
est obligatoire : aucun parent ne peut émettre pour un budget pendant qu'un
autre descend. Dans une feuille commune, énumérer chaque q-sous-ensemble de
`D_(h_max)`, où `h_max=smax-q`, une fois, ou imposer
`max tau(U)=e0` avec un anchor canonique. Alors les strates partitionnent les
candidats et leur somme vaut `C(|D_(h_max)|,q)`; elles ne rendent pas cette
masse sparse. Si `p+1>n`, prendre
`D_p=X` et rester fail-open.

### Stabilisation et bornes terminales conditionnelles

Pour des domaines compacts emboîtés contenant un même centre `c` et dont le
diamètre tend vers zéro, la liste globale `D_p` se stabilise exactement sur le
voisinage fermé au cutoff : les points strictement plus proches que le
`(p+1)`-ième rayon, puis tous les ex æquo à ce rayon. La finitude de `X` donne
un écart positif jusqu'au rayon suivant; les bornes inférieures et supérieures
convergent donc et finissent par exclure tout point plus lointain.

Sous l'hypothèse globale supplémentaire « aucun cinq sites ne sont
cosphériques », la multiplicité du cutoff est au plus quatre. Avec `smax=11`,
cela donne les conditions terminales sûres `|A_2|<=13`, `|A_3|<=12` et
`|A_4|<=11`, donc au plus 78 paires, 220 triplets et 330 quadruplets. Ce ne sont
pas des garanties du profil `RelevantGP` : une cosphère non critique au cutoff
peut avoir une taille arbitraire. Une liste plus grande appelle un quotient
saturé, un producteur exact alternatif ou `resource_exhausted`, jamais une
troncature.

### Scores affines à jauge fixe

Le terme quadratique commun peut être retiré des memberships. Fixer une seule
jauge dyadique `c_0` pour tout l'arbre et poser

$$s_x(c)=\left\Vert x-c_0\right\Vert^2-2\langle x-c_0,c-c_0\rangle,\qquad \left\Vert x-c\right\Vert^2=s_x(c)+\left\Vert c-c_0\right\Vert^2.$$

Les rangs et les égalités entre sites dépendent seulement du score affine. En
remplaçant `l/u` par les extrema exacts `L/U` de `s_x` sur le domaine, le même
argument de témoins prouve les listes et le census, mais le niveau doit lui
aussi être translaté. Pour une boule de centre `c_B`, poser
`theta_B=beta_B-||c_B-c_0||^2`. Alors `theta_B>R_h` implique `h+1` témoins
strictement intérieurs, tandis que `theta_B<=R_h` implique
`I_B union U_B` inclus dans `{x:L_C(x)<=R_h}`. Comparer `L/U` directement à
`beta_B` est faux. Sur une AABB, les extrema se calculent par signes des trois
coefficients, sans parabole. Un k-DOP reste ici un prune de séparation
distinct; s'il devenait le domaine des seuils, ses extrema affines exigeraient
ses sommets ou un programme linéaire exact. La jauge reste commune à tout
l'arbre; recentrer chaque enfant invaliderait la preuve de nesting sans un
nouveau reçu.

Le graphe d'ambiguïté possède l'arête `xy` exactement lorsque zéro appartient
à l'intervalle de `s_x-s_y` sur la cellule, c'est-à-dire lorsque le plan
bissecteur rencontre le domaine actif. Tout support de taille `q` induit une
q-clique. Une absence de q-clique est donc un prune exact; sinon les bitsets
warp évitent seulement les retests.

## 2. Égalités et contre-pièges

- Dans le layout de distances, `beta=R_q(C)` est admissible. Avec une cellule singleton en
  `(10,10,10)`, le support q2 `(9,10,10),(11,10,10)`, `smax=3`, donne
  `R=beta=1`. Remplacer `beta>R` par `beta>=R`, ou filtrer par `l<R`, perd le
  support.
- Les témoins top-`t_q` fixent seulement le rayon. Ils ne contiennent pas
  nécessairement le support. Il faut compacter **tous** les sites satisfaisant
  `l<=R`, égalités comprises.
- Un tie-break `(u,PointId)` rend la sélection des témoins reproductible, mais
  le filtre `A_q` conserve tous les ex æquo à la valeur `R`.
- `R+1` n'est pas équivalent à `beta<=R`, car `beta` est généralement
  rationnel. La formulation exacte compare directement les rationnels ou leurs
  produits croisés. Dans le layout affine, les mêmes phrases emploient
  `theta_B`, jamais `beta_B`.
- `smax` ne borne ni `|A_q|` ni le shell. Une cosphère massive conserve une
  liste `Theta(n)`; ce cas est diagnostiqué, quotienté par un générateur saturé
  reçu ou refusé, jamais tronqué.
- La pertinence ne s'hérite pas entre arités. Il existe un q3 pertinent dont
  chacune des trois arêtes a dix intérieurs, et un q4 pertinent dont chacune des
  quatre faces en a neuf, à `smax=11`. Les coordonnées u16, preuves exactes et
  réponses complètes sont dans
  [`AUDIT_REPONSES_CELLULES_CENTRES_20260812.md`](AUDIT_REPONSES_CELLULES_CENTRES_20260812.md).

## 3. Machine GPU `count / scan / fill`

### Données résidentes

- points u16 en SoA `x[]/y[]/z[]/PointId[]`;
- cellules en ordre Morton, bornes dyadiques fermées et chemin owner half-open;
- CSR par niveau : `cell_offsets`, `candidate_ids` de `D_8`, rangs d'entrée
  `tau_C`, puis `D_7` comme préfixe/buckets; ajouter `D_9` seulement si q2
  emprunte cette machine;
- seuils de budget `R_h` exacts, compteurs, sorts et digests par cellule;
- arènes séparées pour candidats support, census et `BallRecord`.

Les frères peuvent partager des sites. L'exact-once ne vient pas de la liste,
mais du centre rationnel owner de l'unique cellule half-open. La face haute de
la racine doit appartenir explicitement au dernier enfant; un axe bbox dégénéré
est une cellule singleton, jamais un intervalle vide.

### Un niveau de subdivision

Le pseudocode suivant choisit le layout de distances `l/u`. Le layout affine
est une alternative exclusive : il remplace partout `l/u` par `L/U`, compare
au niveau `theta_B` et exige sa propre preuve de largeur. Un kernel ne mélange
jamais les deux conventions.

1. `bounds_kernel` calcule `l_D/u_D` pour chaque couple
   `(child,candidate_parent)` en arithmétique exacte.
2. Une réduction warp/block conserve les neuf plus petites clés
   `(u_D,PointId)` pour q3/q4 (`h=0..8`); q2 séparé demanderait top-10.
3. `count_kernel` compte les rangs d'entrée `tau_D` et tous les
   `l_D<=R_8(D)`.
4. Un scan exclusif réserve la CSR.
5. `fill_kernel` compacte les identifiants par bucket `tau_D`, puis publie
   digest et invariants : inclusion parent et `R_h(child)<=R_h(parent)`.
6. Une classification décide `prune`, `terminal`, `split` ou `resource_exhausted`.

Le noyau q3 utilise `D_8`; q4 utilise son sous-ensemble `D_7`.
Sous coordonnées u16 et numérateurs dyadiques à profondeur `d`, la borne est
`l/u<=3*(65535*2^d)^2<2^(34+2d)`. Elle tient en i128 pour `d<=26`, mais pas en
entier device 64 bits. Les centres, rayons et prédicats des supports exigent
leurs propres bornes 128/256 bits. Les cellules étendues ou rationnelles
imposent une nouvelle analyse de bits.

## 4. Génération candidate des supports

Une énumération aveugle de `C(m,3)` ou `C(m,4)` dans chaque terminal annule le
gain. Les arités sont générativement indépendantes : un q3 pertinent peut ne
posséder aucune arête q2 pertinente, et un q4 pertinent aucune facette q3
pertinente. La stratégie combine donc des petites listes de profondeur et des
filtres fail-open :

1. **séparation convexe** : si `K_C` est strictement séparé de
   `conv(A_q(C))`, la cellule est vide de supports positifs;
2. **cœur de Jung** : des témoins collectifs déjà prouvés éliminent les branches
   dont tout support serait au-dessus de la fenêtre;
3. **q3 indépendant** : lorsque `D_8` est petite, énumérer ses triplets, puis
   décider directement indépendance, positivité, owner et profondeur par
   promotion; un graphe local de bissecteurs peut filtrer mais ne dépend jamais
   des q2 retenus;
4. **q4 indépendant** : lorsque `D_7` est petite, décider directement les
   quadruplets, ou les produire depuis toutes les faces triangulaires aiguës
   géométriques et leurs apex compatibles. Ces faces sont des carriers, jamais
   des activations q3 retenues. Une réduction de pinceau ou les conditions de
   boule équatoriale/cylindre peuvent filtrer les apex sans tester la profondeur
   q3 de la face.

Au singleton sous `U_B=U`, `|D_p|=p+q<=11`, donc une feuille idéale paie au plus
`C(11,3)=165` ou `C(11,4)=330` candidats. Ce chiffre n'est pas un cap : si la
liste ne se contracte pas, la branche splitte, appelle un producteur exact
alternatif ou rend `resource_exhausted`.

Le propriétaire de centre élimine les doublons intercellules, à condition que
tous les budgets d'une arité partagent la même partition terminale. Il ne doit
toutefois pas être évalué après un lift dans chaque vue de cellule : ce choix
répète précisément la géométrie coûteuse que l'owner devait éviter. Une
fixture minimale impose cet invariant : pour
`A=(10,10,10),B=(20,10,10),C=(15,18,10),W=(15,12,10)`, le support `ABC` a
exactement `W` intérieur. Il entre à `e0=0` dans la racine, mais à `e0=1` dans
la cellule singleton de son centre; deux arbres de budgets indépendants
l'émettraient deux fois.

L'ordonnance GPU recommandée possède deux RLE. Chaque feuille émet d'abord,
après les seuls filtres intervalle--bissecteur--enveloppe, une occurrence
`(cloud_epoch,SupportKey,CensusContext)` **sans lift**. `SupportKey` contient
l'arité et les identifiants triés. Le contexte lie `cell_id`, digests du
pool/domaine, backend ou arène encore vivante, `e0` et `b_cert`, budget maximal
dont toute l'ascendance certifie l'invariant de pool. Un premier radix/RLE par
`SupportKey` calcule la géométrie une seule fois, puis choisit dans le run
le contexte dont la cellule half-open possède le centre. La complétude garantit
l'existence de cette occurrence pour tout support pertinent. Pour un tuple
arbitraire non positif ou hors fenêtre, zéro owner dans le run est normal et le
tuple est rejeté; l'oracle doit prouver qu'aucun support pertinent n'est ainsi
perdu. Plusieurs owners restent un échec d'invariant. Le contexte owner rejoue
`U subseteq D_(smax-q)` avant d'être engagé.

Une variante device plus compacte existe lorsque q2/q3/q4 partagent le même
arbre terminal et que sa table de feuilles/CSR reste résidente. La première
arène n'émet alors que `SupportKey` — avec `CellId` seulement comme diagnostic.
Après radix unique et calcul du centre, une descente half-open retrouve
directement la feuille owner et son CSR `(pool,tau,buckets,b_cert)`; elle rejoue
`U subseteq D_H(C_owner)` et recalcule `e0=max tau(U)`. Cela évite de transporter
tous les `CensusContext` du run. La complétude garantit la feuille et les
membres pour un support pertinent; leur absence rejette un tuple arbitraire et
plus d'une feuille est impossible par partition. Si les arités ont des arbres
ou durées de vie distincts, revenir aux contextes conservés ou au census
global. Comparer octets d'occurrences, lookup owner et durée de vie de la table
avant de choisir entre les deux layouts.

Le candidat positif owner émet ensuite
`(cloud_epoch,GeometricBallKey,SupportKey,CensusContext)`. Le second radix/RLE
par `(cloud_epoch,GeometricBallKey)` arrive **avant** la promotion et conserve
tous les supports et contextes. Pour
`H_run=smax-q_min`, il sélectionne atomiquement un contexte avec
`b_cert>=H_run`; ce contexte seul effectue la promotion et le census fermé :

- partir de **son** `e0` immuable et de **ses** buckets avec un curseur `h`
  séparé;
- scanner chaque bucket ajouté une fois, en accumulant intérieurs stricts et
  contacts nuls;
- rejeter tôt au premier compte total supérieur au plus grand `smax-q` encore
  porté par le run, sans publier de shell partiel;
- sinon attacher le `p` commun à la boule et distinguer, pour **chaque**
  support, `relevant_by_min_support=(p+q<=smax)` de
  `accepted_closed_rank=(p+|U_B|<=smax)`;
- envoyer une extra-shell pertinente au quotient saturé ou au refus fermé.

Une arène q4 filtrée ancestralement à `D_7` ne certifie pas rétroactivement un
support q3 à `p=8`, même si elle matérialise un `D_8` terminal. Si q2 vient d'un
backend Yao/LBVH séparé ou si aucun contexte local ne certifie `H_run`, le run
est routé vers un census global exact. `cell_id` et les digests ne font pas
partie de la clé sphère; ils restent des contextes candidats du run.

Construire la géométrie avant le premier RLE la répéterait dans chaque cellule;
promouvoir avant le second répéterait le census pour chaque support incident à
la même boule. Inversement, le rejet d'un support de grande arité ne permet pas
de supprimer la boule entière si un support plus petit du même run reste
pertinent. Les deux arènes sont comptées et segmentées par clés entières sans
couper un run; un flot de tuples trop grand termine en `resource_exhausted`, pas
en troncature.

Cette génération est une proposition d'implémentation, pas encore un théorème
de complexité. Son admission dépend des masses mesurées de cellules, listes,
candidats q3/q4, requêtes de pinceau et supports uniques. Les cliques
d'intervalles restent un filtre potentiellement `Theta(m^4)`, jamais une preuve
de parcimonie.

### Axe de face q4 et spécialisation par carriers aigus

Un support q4 propre positif a son circumcentre strictement à l'intérieur du
tétraèdre. Un tel tétraèdre possède au moins deux faces aiguës, résultat établi
dans la solution du problème 3653 de
[`Crux Mathematicorum 38(8), pages 341--343`](https://cms.math.ca/wp-content/uploads/crux-pdfs/CRUXv38n8.pdf).
Pour **toute** face non dégénérée `F`, aiguë ou obtuse, le lieu des centres
équidistants de ses trois sommets est la droite rationnelle normale au plan de
`F` passant par son circumcentre. Le centre q4 appartient à cette droite.
L'intersection de la droite d'une face canonique quelconque avec le domaine
actif de la cellule est donc une condition nécessaire exacte, plus forte que
les trois seuls tests de bissecteurs; elle ne requiert pas le théorème d'acuité.

Le théorème d'acuité autorise une spécialisation différente : énumérer les
faces aiguës géométriques indépendamment de leur
admission q3, rejeter celles dont la droite ne rencontre pas la cellule, puis
chercher leurs apex. Chaque q4 survivant choisit sa plus petite face aiguë
canonique pour éviter les doublons intracellule. Ce théorème réduit les K4
tentés; il ne donne aucune borne sparse sur le nombre de faces carriers. Il est
incomplet de tester seulement si la face des trois plus petits `PointId` est
aiguë : cette face peut être obtuse lorsque d'autres faces sont aiguës.

La borne deux est optimale et fournit une fixture u16 : centre
`C=(10,10,10)`, rayon carré `30`, sommets
`P0=(5,8,9)`, `P1=(5,8,11)`, `P2=(9,12,5)`, `P3=(15,11,12)`. Les
barycentriques de `C` sont `(5,6,5,12)/28`, toutes strictement positives. Les
faces opposées à `P0` et `P1` sont aiguës; celles opposées à `P2` et `P3` sont
obtuses. Une génération par carriers doit donc couvrir exactement le cas où
seules deux faces sont admissibles et rester invariante sous permutation des
`PointId`.

Pour un carrier de circumcentre `N/G`, de normale `n`, et un apex `d`, poser
`v=d-a` et `Delta=||d||^2-||a||^2`. Si `n dot v` est non nul, l'intersection
exacte axe--bissecteur donne le centre q4

`[2(n dot v)N+n(G Delta-2<N,v>)]/[2G(n dot v)]`.

Le triangle paie ainsi son lift une fois et chaque apex seulement des
produits--sommes, l'owner et les barycentriques avant la clé de sphère.

On peut attaquer l'owner encore plus tôt. Intersecter une fois la droite
`c0+t*n` du triangle avec le domaine actif donne un intervalle rationnel fermé
`T`. Pour chaque apex non coplanaire `d`, l'équation de son bissecteur détermine
un unique paramètre `t_d`; tester `t_d in T` par produits croisés précède le
lift q4 complet. Une égalité reste conservée. Cette condition par apex est plus
sélective que le seul test « la droite rencontre la cellule » et mutualise
`c0/n/T` par triangle; elle exige i128 ou un filtre i64 gardé avec fallback
exact.

Dans la lane q3 seulement, la positivité d'un triangle propre équivaut à son
acuité stricte. Les trois produits scalaires aux sommets, calculables en i64
sous u16, rejettent donc les triangles droits ou obtus avant le lift. Ce filtre
ne conditionne jamais la lane q4 : une face canonique d'un q4 positif peut être
obtuse.

### Certificat local de liste bornée

Choisir `c0` dans le domaine actif `K`, supposer `diam(K)<=delta` et noter
`rho` la distance au `(H+1)`-ième voisin de `c0` dans le pool. Les `H+1`
voisins ont `u_K<=(rho+delta)^2`, donc
`R_(H,P)(K)<=(rho+delta)^2`. Tout site de `D_(H,P)(K)` appartient alors à
`B(c0,rho+2 delta)`.

Si `delta<=alpha rho` et si un census local certifie

`|P intersection B(c0,(1+2 alpha)rho)|<=Lambda(H+1)`,

la liste terminale contient au plus `Lambda(H+1)` sites. Cette implication
conditionnelle est la gate exacte pour choisir une warp et un bitset de taille
au plus 64. Sans certificat d'expansion, le code choisit CSR, subdivise ou
échoue sur la ressource physique; une profondeur maximale ne transforme jamais
une liste dense en liste sparse.

### Potentiel exact du graphe d'intervalles avant terminalisation

Pour des intervalles `[l_i,u_i]` triés par `l_i`, soit `a_i` le nombre
d'intervalles antérieurs `j<i` tels que `u_j>=l_i`. La propriété de Helly des
intervalles donne exactement le nombre de q-cliques :

$$N_q=\sum_i \binom{a_i}{q-1}.$$

Un sweep `O(m log m)` calcule donc exactement les cliques du **graphe
d'intervalles scalaires** sans les énumérer. Ce graphe est seulement un
préfiltre/surgraphe : le fait que `[L_x,U_x]` et `[L_y,U_y]` se croisent ne
garantit pas que la fonction corrélée `s_x-s_y` s'annule dans la cellule 3D.
Après construction du bitset de vrais plans bissecteurs, `E/T/Q` doivent être
comptés séparément. Le split compare une somme pondérée de ce majorant peu cher,
des `E/T/Q` exacts si disponibles, de la réplication dans les enfants, des
octets CSR et du coût maximal de census. `|A|`, le nombre de paires seul ou une
profondeur maximale ne sont pas des gates de travail. Si le potentiel reste
hors budget à la profondeur physique maximale, le statut est
`resource_exhausted`.

La coquille triée `U_B` identifie sémantiquement une boule munie d'un support
minimal positif dans un cloud/epoch fixé. Elle n'existe cependant qu'après le
census et peut avoir `Theta(n)` labels. Elle sert de certificat aval, pas de
clé chaude avant RLE. Une coquille de taille supérieure à `smax` est
quotientée ou refusée explicitement; elle n'est jamais tronquée dans un tampon
fixe.

Une clé chaude complète se déduit directement de la forme liftée. Si
`D||y-a||^2+C dot (y-a)=0`, développer donne le 5-uplet homogène
`(D,C-2Da,D||a||^2-C dot a)`. Imposer `D>0`, puis diviser les cinq coefficients
par leur pgcd, fournit une clé primitive indépendante du support et disponible
avant census. Un radix pré-hash peut router; le 5-uplet exact tranche les
collisions. Les bornes de largeur doivent être recertifiées pour q2/q3/q4 u16
avant le port device. `U_B` reste le certificat sémantique final.

Enfin, le besoin de conserver tous les supports dépend de la sortie. Une
coquille de taille `m` peut porter un nombre quartique de supports q4 positifs.
Le payload Gamma exhaustif peut exiger cette provenance; le quotient H0
normalisé ne doit pas l'énumérer sous un autre nom. Pour ce dernier, une boule
peut porter un support positif canonique rejouable et un certificat exact de
`q_min`; le saturé fermé est traité comme un bloc. Cette optimisation ne vaut
ni pour Gamma, ni pour les verticales.

## 5. Reçus et portes

Chaque cellule reçue porte au minimum : chemin et bornes fermées, budget `h`,
digest parent, témoins top-9 et leurs valeurs, `R_h`, cardinal/digest des
buckets enfants, décision commune `split/terminal` et sort. Chaque run de boule
ajoute centre/rayon rationnels, preuve owner, `e0` immuable, curseurs de
promotion, comptes totaux, signes complets, clé géométrique, `I_B/U_B`, tous
les supports et statut plateau.

Compteurs déterministes :

- `cells_created/pruned/split/terminal`, profondeurs et cellules vides;
- `parent_candidate_reads`, `bound_evals`, top-t opérations, ties;
- `candidate_ids_q3/q4`, buckets `tau`, octets CSR et high-water;
- séparation/Jung tentés, prunes et ambiguïtés;
- paires bitset, triplets/quadruplets tentés, prédicats exacts et requêtes de
  pinceau;
- census scans, interior/shell IDs, `BallKey`, supports, doublons;
- générateurs saturés, joins et octets du payload aval.

La gate CPU commence par un différentiel rationnel `n<=50`, identités
`(BallKey,support,I_B,U_B)` et mutants égalité, tie omis, enfant calculé hors
parent, owner frontière, shell tronqué, inter-arités, arbres de budgets
indépendants, confusion `e0/h`, compte promotionnel partiel, contact nul perdu,
oubli de `max tau(U)=e0` et cap-as-empty. Ensuite
seulement, rampe `12 500/25 000/50 000`; le Poisson uniforme et le mélange
équilibré de huit amas sont bloquants selon le plan officiel, tandis que les
familles terrain/scanline complètent le diagnostic. Deux pentes supérieures à
`1,35` d'un compteur dominant ou des octets ferment cette ordonnance avant G4.

## 6. Pourquoi cette piste est préférable au plein arrangement

Elle ne construit ni atlas, ni adjacences, ni table globale de sommets
d'arrangement. Elle ne rescane le nuage qu'à la racine; toutes les listes
enfants sont héritées avec une preuve d'inclusion. Le même objet local reçoit
la génération et le census. Elle se prête aux primitives GPU stables : réduction
top-k minuscule, prefix-sum, compactage CSR et files persistantes.

Elle peut néanmoins échouer industriellement si les listes restent larges ou si
la génération q3/q4 recrée un coût combinatoire. Le résultat attendu du premier
prototype n'est donc pas un chrono, mais une décision d'architecture par les
compteurs ci-dessus. P1a peut servir de prune amont indépendant; ses crédits de
patch ne deviennent jamais des listes de census et doivent être recertifiés
après chaque split.

GCP non utilisé.
