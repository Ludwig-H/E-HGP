# Réponses à Claude — fenêtre locale, résiduel, forêt et tuilage

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Note auditée :
[`NOTE_CLAUDE_ROUTE_G4_50K_PUIS_10M_20260813.md`](NOTE_CLAUDE_ROUTE_G4_50K_PUIS_10M_20260813.md),
blob Git `fcc9dec50173878673f9497e0f654e15f0de436e`, SHA-256
`c603be69c7b5801b98894bebcee6fd98c5e0de55ba3f167e028ce8c313092c42`,
au `HEAD=ea1acc65c3947640389eb971a45c6799feffe727`.

## Verdict exécutif

La fenêtre k-NN ouvre un **fast path exact a posteriori**, mais pas encore une
source complète. Le certificat doit être reformulé avec la distance au premier
site omis et avec le census fermé entier. Il prouve qu'un support local déjà
énuméré et certifié est global ; il ne prouve ni la complétude de
l'énumérateur local, ni celle d'un résiduel composé des seuls candidats
refusés.

Deux affirmations de la note sont fausses dans Source S et doivent être
retirées avant implémentation : `|I_B|+|S|<=11` ne borne ni `|U_B|` ni le rang
fermé `|I_B|+|U_B|`, et un halo dimensionné par une coupure k-NN ne rend pas
les tuiles indépendantes. Une coquille peut avoir `Theta(n)` labels ; un
support transfrontière ou un événement de même niveau peut coupler plusieurs
tuiles.

La route admissible devient donc : fenêtre locale certifiée pour le noyau,
**couverture complète distincte** pour le résiduel, flux global d'événements
triés par niveau, lots égaux atomiques et fold global. Elle évite toujours le
catalogue de paires, de cofaces et de cellules de Delaunay d'ordre supérieur.
Elle ne possède encore ni preuve `O(n)`, ni producteur, ni payload officiel,
ni mesure G4.

| question | réponse courte |
| --- | --- |
| Q1 — `4R^2<d_M(a)^2` | oui comme certificat suffisant, et comme équivalence **conditionnelle** dans un sous-domaine complètement énuméré ; non comme équivalence de la route globale |
| Q2 — facteur arrangement/Source S | oui, sur cette famille précise tous les q4 comptés sont non positifs ; non, tester la positivité avant émission ne supprime pas automatiquement leur coût de proposition |
| Q3 — analogue Yao à l'ordre `k` | aucun théorème applicable trouvé dans le dépôt ou la recherche ciblée ; les graphes d'ordre `k` du plan ne sont ni l'objet Morse 3D ni une preuve pour son H0 normalisé |
| Q4 — supports longs H0-inutiles | non en général ; un argument de coupe ne les omet qu'après un oracle exact du minimum sortant sur le domaine résiduel complet |
| Q5 — résidence et tuilage | points/index, état du fold et fronts bornés peuvent rester résidents ; les supports se streament, mais les événements inter-tuiles et les lots de même niveau exigent une fusion globale |

## 1. Q1 — théorème exact de fenêtre

### 1.1 Bonne définition de la coupure

Soit `W_M(a)` l'ensemble exact des `M` **autres** sites les plus proches de
`a`, ordonnés canoniquement par `(distance_carree,PointId)`. Le site `a` est
réinjecté séparément. Poser

$$\delta_{\mathrm{out}}(a)^2=\min_{x\in X\setminus(\lbrace a\rbrace\cup W_M(a))}\left\lVert x-a\right\rVert^2,$$

avec `+inf` si aucun site n'est omis. Pour une boule fermée `B(c,R)` dont `a`
est un point de support, tout `y` dans `B` vérifie

$$\left\lVert y-a\right\rVert\leq\left\lVert y-c\right\rVert+\left\lVert c-a\right\rVert\leq2R.$$

On obtient donc l'implication exacte

$$4R^2<\delta_{\mathrm{out}}(a)^2\Longrightarrow X\cap B\subseteq\lbrace a\rbrace\cup W_M(a).$$

Elle couvre le support `S`, les intérieurs stricts `I_B` **et tout le shell
global `U_B`**. L'égalité reste fail-open et part au résiduel.

Si `d_M(a)` désigne la distance au M-ième voisin **inclus**, la comparaison
`4R^2<d_M(a)^2` demeure sûre, car tout site omis est au moins aussi loin. Elle
est toutefois conservatrice : le M-ième site inclus ne peut lui-même appartenir
à une boule ainsi certifiée. La primitive utile lit `M+1` voisins, conserve les
`M` premiers et emploie le premier omis comme coupure. Un scan total publie
explicitement `delta_out=inf`.

Les égalités de distance ne posent pas de difficulté avec `<` : un point omis
à la même distance que la coupure ne peut pas être dans `B`. Le mutant `<=`
doit mourir sur `a=(0,0,0)`, `b=(2,0,0)`, `c=(0,2,0)`, `M=1` : selon le
tie-break, le support `{a,b}` est omis alors que `4R^2=d_M^2=4`.

La comparaison n'est pas en général une comparaison de deux entiers de 64
bits. Pour un rayon rationnel canonique, il faut croiser numérateurs et
dénominateurs en largeur prouvée, avec repli multiprécision avant tout overflow.
Le format de clé de boule et le comparateur doivent être les mêmes dans count,
fill, RLE et oracle.

### 1.2 Ce que le théorème reçoit, et ce qu'il ne reçoit pas

La réciproque annoncée par Claude est vraie seulement sous les prémisses
suivantes :

- la requête top-M et sa coupure sont exactes ;
- l'énumérateur produit **tous** les supports q2, q3 et q4 contenant `a` dans
  `{a} union W_M(a)`, indépendamment par arité et sans cap silencieux ;
- indépendance affine, positivité, `|I_B|+|S|<=smax`, niveau et identités sont
  exacts ;
- le census local reconstruit l'ensemble fermé entier `I_B union U_B` ;
- rayon nul, positions colocalisées et scan terminal suivent leur politique
  explicite.

Sous ces prémisses, tout support global contenant `a` et vérifiant l'inégalité
a tous ses sommets et son census dans la fenêtre, donc il est retrouvé. Ce
n'est pas une preuve que l'étape 2 encore non écrite satisfait ces prémisses.
En particulier, « tester la positivité avant émission » ne prouve pas que les
quadruplets non positifs n'ont pas été formés.

Le bon statut est donc `exact_window_certified_subsource`, pas
`complete_global_source`.

### 1.3 Owner après découverte

Le propriétaire ne doit pas filtrer l'ancre avant certification. Avec
`a=(0,0,0)`, `b=(10,0,0)`, `c=(0,1,0)` et `M=2`, la paire `{a,b}` a
`D^2=100`. La coupure par M-ième inclus vaut `100` vue de `a`, donc l'owner
minimal échoue l'inégalité, mais elle vaut `101` vue de `b`, qui certifie le
support. Un chemin où seul l'owner propose perd ce record.

Chaque endpoint certifiant peut donc émettre une occurrence. Le RLE attribue
ensuite l'owner canonique et agrège tous les `SupportKey` d'une même `BallKey` ;
il ne choisit jamais un support unique par boule avant d'avoir reçu la politique
Gamma ou H0 normalisée.

### 1.4 Le résiduel doit couvrir les objets jamais proposés

Mettre dans une file les candidats locaux qui échouent
`4R^2<delta_out^2` ne couvre pas les supports globaux dont aucun tuple n'a été
formé dans une fenêtre. La phrase « les supports manqués sont exactement ceux
dont la miniboule déborde la fenêtre et ils vivent dans les directions
ouvertes » combine trois objets sans théorème de raccord :

1. une boule globale non encore connue ;
2. l'échec d'un certificat sur un candidat connu ;
3. une cellule directionnelle flottante échantillonnée par le diagnostic.

Le résiduel doit partitionner un **domaine de recherche** avant les tuples. Une
tâche peut porter `(anchor,cellule_directionnelle,intervalle_rayon,epoch)` ou
un bloc collectif `A times B times C`, mais ses cellules doivent couvrir la
sphère exactement, les frontières être half-open, et chaque split conserver la
masse. Sur petit `n`, l'oracle exhaustif compare alors les identités :

$$\mathcal{S}_{\mathrm{globale}}=\mathcal{S}_{\mathrm{fenetre\ certifiee}}\mathbin{\dot\cup}\mathcal{S}_{\mathrm{residuelle}}.$$

Le diagnostic à 512 directions est utile pour ordonner ce résiduel. Il ne le
certifie pas : il est flottant, à banque finie, sur quatre familles et une
seule taille.

## 2. Correction P0 — Source S ne borne pas le shell

Pour un support propre positif `S` de cardinal `q`, Source S utilise la
condition `p+q<=smax`, où `p=|I_B|`. Elle ne remplace pas `q` par `|U_B|`.
Ainsi

$$|I_B|+|S|\leq11\centernot\Longrightarrow|I_B|+|U_B|\leq11.$$

Le dépôt contient déjà un support q2 pertinent de rang fermé douze, une famille
à extra-shell `Theta(n)` et une fixture à shell 30. Les références sont
[`AUDIT_REPONSES_SOURCE_FRONT_INVERSE_20260812.md`](AUDIT_REPONSES_SOURCE_FRONT_INVERSE_20260812.md)
et
[`NOTE_SOLUTION_SOURCE_CELLULES_CENTRES_20260812.md`](NOTE_SOLUTION_SOURCE_CELLULES_CENTRES_20260812.md).

Conséquences directes pour la note de Claude :

- « une boule qui contient au plus onze points » est faux ;
- « leur nombre par ancre est petit » ne suit pas de la pertinence ;
- `M=128/256` est un choix diagnostique, pas une borne universelle ;
- si le certificat strict passe, tout le shell réel tient bien dans la fenêtre,
  mais sa taille est alors bornée par `M`, pas par `smax` ;
- un échec de fenêtre sur une entrée régulière est un résiduel normal, pas une
  `unsupported_degeneracy`. Seule une vraie violation du domaine mathématique
  porte ce statut ; l'épuisement d'une ressource physique porte
  `resource_exhausted` et reste atomique.

La baseline `480,340886` est une espérance Poisson continue. Elle ne prouve ni
une sortie linéaire au pire cas, ni un degré borné, ni un coût constant par
point dans le profil u16.

## 3. Q2 — la famille à deux droites

La preuve de
[`AUDIT_REPONSES_VOLUME_PINCEAU_PROJECTIONS_20260812.md`](AUDIT_REPONSES_VOLUME_PINCEAU_PROJECTIONS_20260812.md)
est admise. Chaque sommet q4 compté utilise deux `A_i` et deux `B_j`. Les
contraintes sur les poids barycentriques imposeraient simultanément une masse
strictement supérieure à `1/2` sur chacun des deux groupes ; le centre est donc
hors de l'enveloppe convexe. Les autres quadruplets sont affinement dépendants,
et tout triangle possède un angle obtus. Source S ne contient ici que les q2.

À `m=25 000`, le rapport exact est

$$\frac{34\,364\,000\,715}{499\,945}=68\,735{,}5623418576\ldots$$

soit environ `68 736`, et non `68 000` comme valeur exacte. Ce quotient compare
des sommets q4 à toutes les sorties q2--q4 ; il ne constitue pas un taux de
rejet homogène.

La positivité explique exactement la séparation **sémantique** sur cette
famille : aucun des sommets q4 n'est une sortie positive. Elle ne donne pas
l'exemption algorithmique par simple test avant émission. Une implémentation
qui forme les quatre points, calcule le centre puis rejette la positivité paie
toujours le facteur. L'étape 2 doit montrer qu'elle ne forme pas ces transits,
avec au minimum `q4_products_considered`, `lifts`, `positivity_tests`,
`positive_candidates` et `emitted_supports`.

## 4. Q3 — aucun Yao d'ordre supérieur reçu

Il faut d'abord séparer trois objets souvent appelés « ordre k » :

- le Yao classique porte sur le graphe complet pondéré des **points** et
  contient un EMST euclidien ;
- le graphe de Delaunay d'ordre k de la littérature plane relie encore des
  points si un cercle incident contient au plus k autres points ;
- MorseHGP3D relie des facettes, cofaces, carriers et lots de plusieurs ordres,
  avec des incidences silencieuses. Son MSF horizontal n'est pas le MST du
  graphe précédent.

La recherche ciblée n'a trouvé aucun théorème disant qu'un graphe local de
taille `O(c(k)n)` contient la forêt Morse d'ordre k en dimension trois. Les
résultats proches ne ferment pas cette obligation :

- [Yao, 1982](https://doi.org/10.1137/0211059) traite l'EMST euclidien des
  points ;
- [Abellanas et al., 2009](https://doi.org/10.1142/S0218195909003143) donnent
  des bornes linéaires en `kn` pour le graphe de Delaunay d'ordre k **dans le
  plan**, sans la sémantique Morse ;
- [Chazelle et al., 1994](https://doi.org/10.1137/S0097539790179919) montrent
  déjà qu'un graphe de Gabriel ordinaire peut avoir `Omega(n^2)` arêtes en
  dimension trois ;
- [Edelsbrunner et Osang, 2023](https://doi.org/10.1007/s00453-022-01027-6)
  construisent les mosaïques d'ordre supérieur, mais leur complexité en
  dimension au moins trois dépend de sorties potentiellement denses et cette
  structure globale est précisément interdite dans le target v3.

Dans le dépôt, `G_tau` est un générateur complet seulement si une majoration
locale certifiée est fournie ; son degré n'est pas borné. Le K-graphe de
Gabriel brut est faux pour le H0 exact à cause des incidences silencieuses de
la fixture E5. Sa complétion `G_k+` est exacte sous porte régulière, mais sa
construction littérale reste exhaustive en cofaces.

La ligne enregistrée contient bien un Boruvka relatif :
`relative_morse_boruvka.hpp` sparsifie un hypergraphe Morse **déjà complet**
fourni par l'appelant. Il déclare justement non certifiées la complétude du
catalogue et la couverture des incidences silencieuses. C'est un aval utile,
pas un générateur géométrique `O(c(k)n)`. Les autres briques proches partent
elles aussi de streams ou facettes déjà certifiés, sont bornées à petit `n`, ou
ne portent que `k=1`.

Un corollaire d'audit du lemme 5.1 de Chazelle et al. ferme même la variante
**littérale** sur les facettes, dans le domaine réel. Leur construction fournit
deux familles `A={a_i}` et `B={b_j}` telles que les `m^2` boules diamétrales
`D_ij` soient strictement vides et possèdent un voisinage ouvert commun de
l'origine. Pour un ordre fixe `K>=2`, ajouter `K-1` points génériques `R` dans
ce voisinage. Chaque `Q_ij={a_i,b_j} union R` est alors une coface directe de
rang fermé `K+1`. L'union des facettes de ces `m^2` cofaces comprend exactement
`{a_i} union R`, `{b_j} union R` et,
pour chaque `r` dans `R`, `{a_i,b_j} union (R minus {r})`. Il y a donc
`V_0=(K-1)m^2+2m` facettes distinctes dans cette sous-famille connectée ; le
graphe complet peut en contenir davantage. Toute forêt qui conserve **ces
sommets-facettes littéraux** a au moins `V_0-1=Omega(m^2)` liens ordinaires, ou
au moins `ceil((V_0-1)/K)` hyperarêtes de taille `K+1`. Pour `K=1`, les facettes
quadratiques disparaissent, conformément au cas Yao/EMST.

Ce corollaire est dérivé de conditions strictes ouvertes dans `R^3`; aucune
famille asymptotique u16 n'est reçue ici. Surtout, il ne réfute pas un quotient
H0 de carriers qui ne matérialise pas les facettes. Il interdit seulement de
présenter le K-Gabriel littéral comme le graphe linéaire recherché.

Ce constat ne prouve pas qu'un quotient de carriers `O(c(k)n)` est impossible.
Il répond seulement : **aucun analogue utilisable n'est actuellement reçu**.
Un futur énoncé devrait préserver, pour chaque coupe stricte et fermée, les
composantes, les naissances et les lots égaux du H0 normalisé, pas seulement
être connecté ou contenir un arbre de poids total minimal.

## 5. Q4 — les directions ouvertes ne sont pas H0-inertes par nature

Un support long peut être la première liaison entre deux amas ou deux feuilles
de surface dont tous les événements plus courts sont internes. La direction
normale ouverte n'est donc pas un certificat d'absence de fusion.

La propriété de coupe de Boruvka s'applique après avoir défini un graphe pondéré
complet, ou si un oracle implicite renvoie exactement un minimum sortant pour
chaque composante. Elle ne permet pas d'ignorer la partie du domaine dans
laquelle ce minimum pourrait vivre. Deux réductions sûres restent possibles :

1. après génération, omettre un événement dont les carriers sont déjà reliés
   au bon préfixe ;
2. remplacer le résiduel par un oracle composante--domaine qui renvoie le
   minimum sortant exact, toutes les égalités requises et un reçu de couverture
   des directions non sélectionnées.

La seconde option serait un excellent remplacement de l'étape 3, mais elle est
une nouvelle implémentation du résiduel, pas sa suppression. Les rondes
Boruvka ne sont pas l'ordre de filtration : les événements de même niveau
restent calculés sur un snapshot strict commun puis commis atomiquement.

Il existe néanmoins un prune géométrique exact avant ce résiduel. Si un support
positif `S` contient `a`, son centre `c` possède des poids barycentriques
strictement positifs et

$$c-a=\sum_{p\in S\setminus\lbrace a\rbrace}\lambda_p(p-a).$$

La direction `c-a` appartient donc au cône tangent global
`T_a=cone(X-a)`. Toute cellule directionnelle qu'un demi-espace entier sépare
strictement de `T_a` ne contient aucun support positif, quelle que soit sa
longueur. Ce prune peut retirer les directions normales d'un nuage coplanaire
et doit être mesuré sur `terrain` avant de conclure qu'environ un dixième du
travail reste. Il ne borne aucun rayon dès que la cellule rencontre `T_a`.
Publier `open_cells`, `positive_feasible_open_cells` et le résiduel après leur
intersection, puis vérifier zéro support omis contre l'oracle borné.

Enfin, cette réduction vise seulement le H0 horizontal normalisé. Elle ne
reconstruit pas à elle seule les incidences Gamma, les applications verticales
ou le `BenchmarkOutputContract-v1`.

La fixture E5 rend la prudence concrète : une incidence silencieuse ne fusionne
pas immédiatement deux unions de `PointId`, mais installe la facette `AC` qui
porte une fusion ultérieure. Un DSU de points et le critère « aucune fusion
immédiate » la perdent. Pour omettre un événement après résolution H0, il faut
donc résoudre tous ses bras sur le snapshot pré-lot, prouver leur racine commune
et conserver encore l'ancre/provenance si le payload vertical ou de couverture
la demande.

Les deux fichiers cités comme primitive directionnelle ne ferment pas ce trou.
`exact_ray_sweep.hpp` part d'une paire déjà choisie et mesure la profondeur de
ses sphères ; `first_incidence_dichotomy.cpp` part d'une facette du cœur déjà
connue et emploie un univers oracle. Aucun ne paramètre exhaustivement les
directions d'une ancre pour générer son premier support. Il reste à prouver la
couverture des directions, l'owner, l'ordre des contacts, les tangences et tous
les ex aequo ; « faire croître jusqu'au premier contact » n'est pas encore une
primitive reçue.

## 6. Q5 — résidence exacte à dix millions

### 6.1 Le nuage ne coûte pas seulement 60 Mo

`3*u16*n` vaut bien `60 000 000` octets à `n=10^7`. Ce chiffre ne contient pas
les `PointId`, la permutation spatiale, l'index ni le fold. Quelques planchers
illustrent l'obligation de ledger :

| objet | taille si matérialisé à `n=10^7` |
| --- | ---: |
| coordonnées seules, `3*u16` | `60 MB` |
| un `PointId:u32` par point | `40 MB` |
| une coupure carrée `u64` par point | `80 MB` |
| listes explicites `M=128`, indices u32 | `5,12 GB` |
| listes explicites `M=256`, indices u32 | `10,24 GB` |
| dix forêts ayant chacune `n-1` arêtes, deux endpoints u32 seulement | environ `0,80 GB`, illustration conditionnelle |

La dernière ligne n'est pas une borne normative sur le nombre de records de la
forêt ; elle illustre seulement pourquoi les 60 Mo ne suffisent pas. Elle omet
niveaux rationnels, nœuds internes, lots, verticales et certificat. Un LBVH
binaire possède presque `2n` nœuds ; ses enfants, boîtes,
ranges, clés Morton et workspaces doivent être comptés selon le layout réel.
La note ne peut donc pas conclure « le nuage et le fold tiennent » depuis les
seuls 60 Mo.

### 6.2 Ensemble résident candidat

Peuvent rester résidents, sous préflight exact :

- coordonnées, `PointId`, permutation et index spatial global, ou un annuaire
  global authentifié de pages ;
- coupures k-NN exactes si leur matérialisation gagne contre leur recalcul ;
- état global du fold par ordre sur les handles de facettes/carriers, snapshot
  strict du lot courant et handles stables ; un DSU des seuls `PointId` est
  incorrect dès les ordres supérieurs ;
- deux buffers de tuiles pour requêtes, candidats, RLE, census et événements ;
- files résiduelles et inter-tuiles bornées, avec spill lossless et manifestes ;
- buffers d'émission, ancres verticales en attente, digests, compteurs,
  manifestes et high-water.

Ne doivent pas rester résidents : toutes les listes `W_M(a)`, tous les supports,
les `4,8` milliards de supports Poisson attendus, une matrice de paires, Gamma
ou une mosaïque de Delaunay. Les fenêtres se calculent par tuile et se
réutilisent en shared/registers uniquement pour les ancres actives.

### 6.3 Pourquoi les tuiles ne sont pas indépendantes

`d_M` est le résultat d'une requête globale, pas un halo connu à l'avance. Une
tuile ne peut déclarer son halo fermé que si l'index global ou un annuaire de
boîtes prouve qu'aucune page omise ne contient un site plus proche. Même alors,
le certificat ne couvre que les supports qui passent strictement ; un support
résiduel peut traverser plusieurs tuiles sans rayon local borné.

Le protocole exact minimal est :

1. halo en lecture seule et owner de support canonique, indépendant du
   scheduling ;
2. RLE global des occurrences par `SupportKey` puis `BallKey`, y compris entre
   tuiles ;
3. front global ou spillable pour les supports résiduels transfrontières ;
4. flux de chaque tuile trié par `(ordre,niveau_exact,cle_canonique)` ;
5. fusion multiway globale, gel de toutes les racines du niveau, construction
   de toutes les incidences, puis commit unique du lot égal ;
6. DSU global, ou contractions locales accompagnées d'un certificat de coupe
   prouvant qu'aucun événement inter-tuile de niveau inférieur ou égal n'a été
   omis ;
7. raccord global des verticales et du certificat final.

Réduire chaque tuile à sa composante finale puis fusionner ces composantes perd
les niveaux de connexion, les lots simultanés et potentiellement les verticales.
Le résumé d'interface exact peut, au pire, être aussi grand que le stream
d'événements : aucune constante d'interface n'est encore prouvée.

Le mot « tuile » ne doit pas emprunter une preuve existante par homonymie. Les
chunks reçus dans l'architecture actuelle sont des suites de **lots exacts
complets** ou des lanes de travail partageant le même snapshot locator. Leur
merge externe porte sur les identités canoniques des lots et actions, pas sur
des sous-nuages spatiaux indépendants. La couture spatiale avec halo est donc
une nouvelle obligation de preuve.

Le plan de tests autorise déjà le streaming à `10^7` seulement si le catalogue
reste sparse, avec un objectif distinct de `600 s`. Cette série ne change ni le
payload ni le contrat `50 000` : à 50 k, le `warm_e2e` se termine toujours
après les dix forêts, verticales, lots et certificat minimal en mémoire hôte
épinglée.

## 7. Lecture des nouveaux diagnostics

Le reçu
[`cone_scale_and_locality_20260813`](../receipts/cone_scale_and_locality_20260813/README.md)
pinçe correctement l'ELF `abbc57c5...` et les commandes de la rampe jusqu'à
`n=16 000`. Il reconnaît que sa colonne `target_visits` duplique les visites
k-NN et que les temps sont contaminés ; seuls les autres compteurs sont
retenus. Ils renforcent le NO-GO de la DFS par endpoint.

Le probe de cellules ouvertes est conservé hors dépôt avec ses limites :
flottant, banque 512, discrétisation finie, `n=2 000`, quatre familles. Le fait
qu'environ neuf dixièmes des cellules échantillonnées soient fermées justifie
de prioriser le fast path. Il ne justifie pas les phrases « le résiduel est
exactement le cône ouvert », « travail `O(n)M` » ou « tuiles indépendantes ».

## 8. Ordre de reprise transmis à Claude

1. Corriger dans le contrat de la route `U_B`, la coupure au premier omis, les
   ties, le scan total, l'owner tardif et la différence entre
   `unsupported_degeneracy`, `residual` et `resource_exhausted`.
2. Construire un sujet borné de fenêtre qui compare les identités complètes
   `(BallKey,SupportKey,I_B,U_B)` à l'oracle rationnel, avec la partition
   certifié/résiduel et des planchers non nuls.
3. Graver les fixtures `equality-tie`, endpoint non-owner seul certifiant,
   extra-shell 30, support jamais proposé, rayon nul, positions agrégées et
   overflow du produit croisé.
4. Séparer les compteurs de la requête k-NN, de la proposition, de la
   positivité, du census, des supports certifiés et des tâches résiduelles.
5. Recevoir ensuite un domaine résiduel complet, soit directionnel avec cellules
   exactes, soit collectif `A times B times C`, et prouver zéro reprise racine.
6. Raccorder seulement après cela owner global, RLE, flux trié, lots atomiques,
   fold, verticales et `BenchmarkOutputContract-v1`.
7. Exiger deux pentes `<=1,35` aux tailles contractuelles avant tout port CUDA ;
   le fast path et le résiduel publient leurs pentes séparées et conjointes.
8. Pour `10^7`, fermer d'abord le schéma de stream, le ledger mémoire complet,
   les événements inter-tuiles et le fold global ; ne pas extrapoler les 60 Mo
   de coordonnées en capacité produit.

Les portes minimales de cette dernière étape comparent une exécution à une,
deux puis `N` tuiles : mêmes `SupportKey/BallKey`, mêmes lots, mêmes forêts,
mêmes couvertures et mêmes verticales. Elles incluent un support trans-tuile,
une même boule proposée par plusieurs halos, un shell distant, E5, un lot égal
inter-tuile et une reprise après interruption juste avant le commit global.

Le delta logiciel live du juge spindle observé pendant cette rédaction est en
cours chez Claude. Il n'est pas inclus dans ce verdict et aucun fichier
d'implémentation n'a été modifié par l'auditeur.

GCP non utilisé.
