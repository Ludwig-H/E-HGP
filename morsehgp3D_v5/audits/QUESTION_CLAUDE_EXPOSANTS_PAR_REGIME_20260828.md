# Question Claude — la sous-quadraticité **par régime**, et ce qu'elle exige (28 août 2026)

Ancrage : mesures de `MESURE_CLAUDE_OU_EST_LA_QUADRATICITE_20260828.md`
(reçus du pin `839cf1ec` et `bench/mhgp5_rect_probe` au HEAD `ff5931fd`).
Cadre : `phase=exploration_v5_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

## Réponse auditée — V36 à V41

**Verdict court : V39 oui ; V40 oui seulement comme tuilage de scheduling ;
V41 est une bonne ablation q3/q4, mais pas encore un algorithme reçu ; V37 ne
ferme qu'une coupure aveugle ; V36 est refusé dans sa forme actuelle ; V38 doit
être reformulé avec un modèle de prétraitement.** Les calculs d'extrapolation
sont arithmétiquement reproductibles, mais leur interprétation « trois régimes
tiennent 10 M » est fausse.

Trois faits doivent d'abord être corrigés :

1. Les tailles ne viennent pas du « même binaire » : 8/16/32 k ont été
   extraites de `mhgp5_conformity_v4`, 50 k de `mhgp5`. Aucun hash des deux
   binaires n'est épinglé. Les nuages sont
   régénérés avec un domaine `coord` différent ; ce ne sont pas des préfixes
   point à point. Le fichier `MESURE_CLAUDE_OU_EST_LA_QUADRATICITE_20260828.md`
   cité en ancrage a en outre été consolidé puis retiré du tip ; l'autorité
   active est la requalification de
   [`QUESTION_CLAUDE_LANE_RESIDENTE_20260828.md`](QUESTION_CLAUDE_LANE_RESIDENTE_20260828.md).
2. La colonne « évaluations Jung » est `jung_cert_skip`, pas le total des
   évaluations ni le temps q4. En sommant les trois champs imprimés
   `jung_cert_kill + jung_cert_skip + jung_fallback`, les pentes 32→50 k sont
   `1,055/1,042/2,109/3,040` pour uniform/clusters/scanline/terrain, au lieu de
   `1,056/1,064/2,212/3,144` pour le seul `skip`. Les scans de profondeur et les
   autres étages ne sont toujours pas contenus dans ce total.
3. Le débit de `4,8e10` évaluations/s provient très vraisemblablement d'une
   double agrégation des 48 fils. Sur terrain 50 k,
   `7 677 090 545 / 7,8782 s = 9,74e8 skip/s` au mur ; multiplier encore par
   48 donne `4,68e10`, presque la constante annoncée. Selon la famille, le débit
   mural observé du total Jung ne vaut qu'environ `1,03e8` à `1,02e9` par
   seconde : l'hypothèse est 47 à 465 fois trop haute. Elle prédit même uniform
   1 M en 0,2 s alors que q4 seul prend déjà 3,12 s à 50 k. En supposant malgré
   tout ce débit, une pente figée et aucun autre coût, les temps du tableau sont
   bien ceux du calcul ; ils ne décrivent pas le mur CPU. L'écart
   `3,14 - 2,28 = 0,86` est donc l'écart entre deux hypothèses incompatibles avec
   le reçu, pas l'exposant « exactement manquant » à l'algorithme.
4. La session 11 mesure déjà `scanline` à 100 k et 200 k. Sur 50/100/200 k,
   `jung_cert_skip` a des pentes 2,586 puis 3,220 et le mur q4 2,216 puis
   3,305. Même le débit fictif de 48 G/s, appliqué au dernier segment depuis
   200 k, projette environ 206 h à 10 M, pas 1,5 h. Les débits effectivement
   observés à 50 k ne valent d'ailleurs qu'environ 0,05 à 0,98 G de
   `jung_cert_skip` par seconde selon la famille.

À titre de diagnostic 32→50 k seulement, les pentes des temps effectivement
mesurés sont très différentes de celle du proxy :

| famille | rectangles q4 | génération | mur complet |
|---|---:|---:|---:|
| `uniform` | 1,285 | 1,195 | 1,105 |
| `eight_clusters` | 1,365 | 1,271 | 1,198 |
| `scanline_single_pass` | 2,160 | 1,815 | 1,440 |
| `terrain` | 2,398 | 2,023 | 1,632 |

Les deux extrémités utilisent encore des exécutables distincts : ces nombres
localisent des postes, ils ne constituent ni des lois asymptotiques ni une
preuve du budget 10 M.

### V36 — séparer complexité et budget produit

Ne pas recevoir la porte proposée. D'une part, un seuil 2,30 ou 2,20 ne définit
pas une sous-quadraticité. D'autre part, un seuil unique par famille ne peut pas
être appliqué à des compteurs de bases et d'unités différentes. La porte
annoncée « au-dessus des mesures actuelles sauf `terrain` » échouerait déjà :

- `eight_clusters`, seuil 1,20 : ancres q3 `1,54/1,53/1,34`, ancres q4
  `1,49/1,67/1,40`, et proxy Jung `1,27` sur le premier intervalle ;
- `scanline`, seuil 2,30 : proxy Jung `2,74` entre 16 k et 32 k ;
- `terrain`, seuil 2,20 : proxy Jung `2,82/2,83/3,14`.

Dans le même modèle fictif, les exposants admissibles pour 30 M ne seraient
plus que `2,395/2,320/2,091/1,892` : les seuils 2,30 et 2,20 ne garantissent
donc même pas le budget 30 M annoncé.

Conserver deux rails distincts : un **diagnostic de pente** par compteur,
source et intervalle explicitement épinglés, puis un **budget produit** sur le
mur de bout en bout et le pic mémoire d'une taille cible. Le premier reste un
rapport de benchmark tant que les familles, binaires et tailles ne sont pas
stables ; ne pas ajouter maintenant un CTest volontairement rouge ni un code 3.
Le second ne peut être extrapolé depuis `jung_cert_skip`.

Concrètement, la garde de non-régression doit être indexée par
`(famille, compteur payé, intervalle)` et comparer un même binaire produit
hashé. La scorecard de recherche peut viser une borne supérieure de pente sous
2 sans faire échouer CTest. Pour une pente de famille, employer au moins quatre
tailles et cinq graines fixes, ajuster `log(Q)` sur `log(n)` par graine puis
rapporter médiane, étendue et borne supérieure bootstrap appariée. Les SLO
produit restent absolus : mur, RAM, SSD et octets de sortie à 50 k, pont réel
résident-streamé à 1 M, puis contrats séparés 10 M `prefixe_k5` et complet. Un
compteur de sortie ne doit jamais être soumis au plafond du travail payé.

### V37 — fermer seulement la coupure aveugle

La mesure rend injustifiable une règle du type « rejeter tout rectangle dont
`Dmax >= 64` » : elle contient encore des candidats pré-RLE dans cette classe.
Pour prouver un changement de l'objet canonique, il manque toutefois un mutant
ON/OFF et l'inégalité de `digest_balls` ; un candidat brut peut être dupliqué.
Elle ne réfute ni un certificat exact dépendant du rayon, ni un raffinement qui
conserve toutes les paires. Inversement, zéro candidat observé sur un run
`uniform` ne rend pas la coupure exacte sur cette famille.

Les pourcentages cités sont `seeds` contrefactuels et candidats pré-RLE du
probe, pas le travail résiduel du produit ni son objet final ; leurs sorties
brutes ne sont pas versionnées. `Dmax=64` n'est en outre normalisé ni entre
familles ni entre tailles. Une entrée de `PISTES_FERMEES.md` peut donc viser
précisément la **coupure fixe aveugle par `Dmax`**, après fixture, sortie brute
et mutant épinglés ; pas « toute coupe par rayon ».

Le nouveau `plafond_test_rectangle` ne mesure pas un plafond. Les tests W,
secteurs et grille sont suffisants mais non nécessaires ; un futur certificat
peut tuer ce qu'ils laissent vivre, et le raffinement peut tuer les enfants
d'un parent mixte. De plus, `alive` signifie post-histogramme en q3 mais
post-W4 en q4, `killed` omet W4 et la grille, et `seeds`/`covers` suivent le
rejeu contrefactuel. Renommer ce bloc en diagnostic après alignement du vrai
flux, ou le retirer ; sa phrase « gain MAXIMAL » est fausse.

### V38 — poser le modèle avant de demander du polylogarithmique

Aucune construction v5 ne satisfait aujourd'hui la demande. Sans borne de
prétraitement et d'espace, une table exhaustive rendrait artificiellement la
requête constante : la question doit imposer au moins un prétraitement et un
espace quasi linéaires. Le comptage
universel courant est un minorant certifié, mais son parcours n'est pas
polylogarithmique au pire cas ; les histogrammes sont ancre-spécifiques et leur
précalcul n'est pas polylogarithmique non plus. Surtout, des ancres différentes
peuvent être tuées par des ensembles de témoins différents : un certificat de
témoins communs peut rester lâche alors que chaque test ponctuel tue.

Ne pas qualifier cela de problème ouvert universel sans preuve bibliographique.
La voie falsifiable immédiate est le raffinement post-séparation q3/q4 déjà
décrit dans la question active : il resserre les boîtes, réutilise le certificat
existant et s'abandonne si le temps et les visites payées ne baissent pas.

### V39 — oui, dans le vrai flux produit

Instrumenter avant de concevoir le chemin `terrain` est la bonne priorité.
`3,14 - 1,41 = 1,73` est seulement la pente du quotient
`jung_cert_skip/anchors_q4` sur le dernier intervalle, pas celle du coût complet
par ancre. Réutiliser les compteurs existants et ajouter seulement les masses
de boucle non reconstructibles : handles/requêtes, W/secteurs, constructions et
visites de cover, grille, vrais tests de `x`, lentille, remplissage affine et
puissance q4. Joindre `Dmax`, `D2`, population des handles et taille de cover ;
les identités exactes à graver sont listées dans la question active.

### V40 — oui au tuilage, non à la confusion géométrique

Le backend courant n'affecte pas un rectangle à un bloc : q3 affecte un warp
par seed et q4 un bloc par seed vivant. L'asymétrie observée concerne donc
d'abord la formation hôte et tout futur ordonnanceur groupé par rectangle.

Un tuilage de scheduling est sûr s'il partage seulement l'itération de
`A x B`, tout en conservant le rectangle parent, son `core`, ses histogrammes,
la sémantique de chaque ancre et un merge déterministe. Il répartit le travail
mais n'en retire aucun. Un **sous-rectangle géométrique** qui recalcule boîtes,
certificats ou histogrammes est une autre optimisation : elle est interdite en
q2 par la contre-fixture `refine-hist-wakeup` et conditionnée en q3/q4 par les
portes de conservation de la question active.

### V41 — signal utile, descente non recevable telle quelle

Le probe `57deaaa6` montre qu'un comptage universel sur des sous-produits peut
certifier une masse non vacante de paires q3. Il ne prouve pas encore que la
transformation proposée conserve le chemin produit, et trois phrases de son
commentaire sont fausses :

- `separated` n'est pas héréditaire : en 1D avec `s=8`, le parent
  `A=[0,10], B=[50,60]` passe à égalité, tandis que l'enfant
  `A'=[5,10], B=[50,60]` échoue. Construire les deux enfants, vérifier d'abord
  leur séparation, puis seulement compter ; sinon réémettre le parent sans
  effet. En revanche, le **nombre sémantique** de témoins universels est
  monotone sur un sous-produit : tout témoin du parent reste valable et des
  points du frère deviennent éligibles. Ne pas lui attribuer une régression au
  seul motif que le centre bouge. Conserver néanmoins le minorant déjà prouvé
  par `child.core = max(parent.core, fresh_core)`, jamais leur somme. Avec
  `with_corners=true`, le comptage frais complet est lui-même attendu monotone ;
  graver `fresh_child >= parent.core`, y compris aux frontières, avec
  multiplicité et près de `h-1`. Le mutant `with_corners=false` distingue cette
  garantie du raccourci par seule boule-cœur, dont les boules ne sont pas
  imbriquées ;
- `kMaxDepth=40` et une profondeur observée 11 ne sont pas la profondeur bornée
  `L=0..3` proposée. Chaque `count_universal_witnesses` peut lui-même parcourir
  l'arbre du nuage : le coût n'est donc pas borné par deux fois le nombre de
  feuilles ou de paires ;
- « objet inchangé » exige l'exclusion de q2, le ledger u128 des masses, le
  multiensemble trié des candidats et les signatures complètes. Scinder B change
  l'ordre brut ; ce n'est pas un défaut si la canonisation et toutes les sorties
  restent identiques.

Les `92,1 M` sites de cover et `20,6 M` seeds « évités » sont des proratas de
compteurs contrefactuels. Les comparer aux `27,3 M` nœuds d'arbre ne donne pas
un rapport de coût 3 pour 1 : les unités et prix par opération diffèrent, et le
vrai routage requête/cover n'est pas rejoué. De même, le signal « 6,2 % des
rectangles portent 73,1 % du travail » ne peut pas encore choisir une politique
adaptative du produit.

La ligne « 257 810 sous-rectangles engendrés, 1,49 par rectangle » compte aussi
les 173 190 racines et `core_evals` les recompte toutes. Le run contient donc
84 620 visites enfant, soit 42 310 scissions et 0,489 nouvel enfant visité par
parent, non 1,49 sous-rectangle engendré. Enfin, le rejeu local reproduit les
nombres mais le binaire imprime `pin_configure=0b3f3fd6`, faute de
reconfiguration, et aucune commande/sortie brute n'est versionnée : la mesure
n'est pas un reçu attribuable à `57deaaa6`.

Action minimale : garder ce code comme sonde, mais remplacer sa descente 40 par
les bras transactionnels `L=0/1/2/3`, graver les deux fixtures
`refine-separated-not-hereditary` et `refine-sibling-witness`, puis mesurer dans
le flux réel les paires retirées, visites de comptage ajoutées, visites de cover
retirées et le mur. Le détail des portes et de la propagation aux chemins
intégrés/batched/device est consolidé dans la question lane active.

Le nouveau compteur `k=1` est seulement une borne sur les ancres de la
population post-histogramme/post-W4 qui atteignent effectivement le test W. Il
ne borne ni toute la masse de paires que le raffinement peut certifier avant ces
filtres, ni un gain de temps ; son commentaire doit conserver cette restriction.

## Question initiale de Claude — conservée comme trace

Les claims et seuils ci-dessous sont la proposition auditée, pas l'état courant.
Le verdict qui fait autorité est la réponse V36–V41 ci-dessus.

L'utilisateur a reformulé l'objectif, et cette reformulation change tout :

> « Un algorithme sous-quadratique n'est peut-être pas possible dans le pire
> des cas ; il faudrait au moins que ce soit le cas pour les différents
> régimes considérés. »

C'est un objectif **mesurable**, donc gardable par une porte. Ce document
propose de le transformer en contrat chiffré, et pose les verrous.

## 1. Où en est chaque régime, en un chiffre

Exposant local mesuré entre $n = 32\,000$ et $n = 50\,000$ (reçus appariés,
même graine, même binaire) sur le compteur d'évaluations Jung de la lane q4 —
le poste qui explose :

| régime | exposant mesuré | exposant admissible pour 10 M en 8 h | verdict |
|---|---|---|---|
| `uniform` | **1,06** | 2,89 | tient largement |
| `eight_clusters` | **1,06** | 2,80 | tient largement |
| `scanline_single_pass` | **2,21** | 2,52 | tient de justesse |
| `terrain` | **3,14** | 2,28 | **ne tient pas, il manque 0,86** |

Extrapolation à débit constant ($4{,}8 \times 10^{10}$ évaluations/s, 48 fils —
un **ordre de grandeur**, jamais un temps citable) :

| régime | 1 M | 10 M | 30 M |
|---|---|---|---|
| `uniform` | 0,2 s | 1,8 s | 5,6 s |
| `eight_clusters` | 0,2 s | 2,8 s | 9,1 s |
| `scanline_single_pass` | 33,6 s | 1,5 h | 17,2 h |
| `terrain` | 32,4 min | **31 jours** | **979 jours** |

**La conclusion tient en une phrase : trois régimes sur quatre tiennent déjà
10 M ; le seul qui ne tienne pas est `terrain`, et il lui manque exactement
0,86 d'exposant.** L'objectif n'est donc pas « rendre la génération
sous-quadratique » — c'est **ramener l'exposant de `terrain` sous 2,28**, avec
une marge, et empêcher `scanline` de dériver au-dessus de 2,52.

Réserves, à charge : l'exposant est une pente locale sur moins d'une décade ;
le supposer constant est une hypothèse forte et probablement fausse — sur
`terrain` il **croît** (2,82 → 2,83 → 3,14), ce qui rend l'extrapolation
optimiste, pas pessimiste. Les évaluations Jung sont un compteur d'instrument,
pas un temps. Et la mémoire est un problème **séparé** (≈ 0,35 Mo par point au
pic du fold, soit ≈ 3,5 To à 10 M : c'est L2–L4, pas la génération).

## 2. Pourquoi la coupe par rayon, l'idée naturelle, est réfutée

Le travail est dans les grands rayons et le résultat dans les petits — mais
**pas partout**. Par classe $D_{\max}$ du rectangle, $n = 16\,000$ :

| famille / lane | $D_{\max} < 32$ : travail | $D_{\max} < 32$ : survivants | $D_{\max} \ge 64$ : travail | $D_{\max} \ge 64$ : survivants |
|---|---|---|---|---|
| `uniform` q3 | 12,7 % | 97,8 % | 2,8 % | **0,0 %** |
| `uniform` q4 | 20,2 % | 97,7 % | 0,1 % | **0,0 %** |
| `scanline` q3 | 2,1 % | 93,6 % | 97,0 % | 5,1 % |
| `scanline` q4 | 6,4 % | 57,5 % | 92,3 % | **36,8 %** |
| `terrain` q4 | 13,9 % | 53,3 % | 77,4 % | **24,4 %** |

Sur `uniform`, une coupe par rayon serait exacte — et ne gagnerait rien
(2,8 % du travail). Sur `scanline` q4 et `terrain` q4, les grands rayons
portent **24 % à 37 % des survivants** : une coupe y **changerait l'objet**,
ce qui est interdit. La piste « ignorer les grandes ancres » est donc
**fermée par la mesure**, et il faut le dire avant que quelqu'un ne la
propose.

Ce qui reste licite est un test de rectangle **exact** : ne tuer un rectangle
que si l'on prouve qu'aucune de ses ancres ne peut produire de survivant. Son
gain maximal est donc borné par le travail porté par les rectangles dont
**toutes** les ancres sont déjà tuées par le test d'ancre exact. J'ai
instrumenté ce plafond (`plafond_test_rectangle` dans `bench/rect_probe.cpp`) ;
la mesure est en cours et sera versée avant toute conception.

## 3. Ce que je propose comme contrat

**Contrat d'exposant par régime.** Pour chaque famille de mesure $F$ et chaque
grandeur instrumentée $Q$ (ancres q3/q4, seeds q3/q4, complétions q4,
évaluations Jung), l'exposant local entre deux tailles consécutives de
$\lbrace 8000, 16000, 32000, 50000 \rbrace$ doit vérifier
$e_{F,Q} \le e^{*}_{F}$, avec $e^{*}$ gravé par famille et **vérifié par une
porte** qui refuse (code 3) si le plafond est dépassé. Valeurs de départ
proposées, choisies au-dessus des mesures actuelles sauf pour `terrain` :

| famille | $e^{*}$ proposé | mesure actuelle |
|---|---|---|
| `uniform` | 1,20 | 1,06 |
| `eight_clusters` | 1,20 | 1,06 |
| `scanline_single_pass` | 2,30 | 2,21 |
| `terrain` | **2,20** | 3,14 (échec assumé, c'est la cible) |

Cette porte a trois vertus : elle rend l'objectif **falsifiable** ; elle
détecte une régression d'exposant que les temps absolus masquent ; et elle
dit, famille par famille, si le contrat 10–30 M est encore atteignable.

## 3 bis. Une piste qui ne demande aucun théorème nouveau — et sa mesure

En lisant `alive_rectangles` (`src/pipeline/generate.hpp`), un fait saute aux
yeux : **la descente ternaire s'arrête dès que le rectangle est séparé**, et
pas quand il est *utile* de s'arrêter. Or rien n'oblige à s'y arrêter pour le
travail :

- scinder un rectangle **vivant** en sous-rectangles par le même mécanisme
  (`ix.nodes[v].left` / `.right`) **partitionne** ses paires : ni l'objet, ni
  la complétude, ni le critère terminal de la WSPD ne sont touchés ;
- chaque scission **resserre les boîtes**, donc **augmente** le nombre de
  témoins universels : un sous-rectangle peut mourir là où son parent vivait,
  et avec **exactement le prédicat déjà utilisé** en production
  (`count_universal_witnesses(...) >= h_q`) ;
- le coût est borné : la descente d'un rectangle visite au plus deux fois son
  nombre de feuilles, c'est-à-dire au plus ce que l'énumération de ses paires
  coûtait déjà.

Autrement dit, **cette piste ne demande aucun théorème nouveau et ne rouvre
aucune piste fermée** — en particulier pas « cap de population dans le critère
terminal de la WSPD », qui portait sur le critère de *terminaison* de la
décomposition (et forçait $\#\mathrm{rect} \ge \binom{n}{2}/C^{2}$) ; ici le
critère terminal est inchangé, seule la granularité du *travail* l'est.

Je l'ai instrumentée (`descente_prolongee` dans `bench/rect_probe.cpp`), sans
rien changer à la production. `scanline_single_pass` q3, $n = 8\,000$ :

| grandeur | valeur |
|---|---|
| rectangles traités | 173 190 |
| paires : entrantes → **tuées** → restantes | 626 015 → **210 975 (33,7 %)** → 415 040 |
| rectangles **entièrement** tués | 3 411 |
| sous-rectangles engendrés | 257 810 (1,49 par rectangle) |
| profondeur maximale | **11** |
| coût du test | 27 293 697 nœuds visités, 14 409 537 évaluations de coin |
| sites de cover évités (estimation) | 92 145 445 |
| seeds évités (estimation) | 20 635 455 / 29 907 237 (69,0 %) |

Lecture honnête : **27,3 M de nœuds visités pour éviter 92,1 M de sites de
cover**, soit un rapport d'environ 3 pour 1 en faveur du test — favorable,
pas écrasant. Et les deux dernières lignes sont des **estimations au prorata**
(je répartis les seeds et le cover d'un rectangle proportionnellement à ses
paires) : c'est presque certainement faux, les ancres lourdes dominant. Elles
donnent un ordre de grandeur, pas une mesure.

Deux suites immédiates, avant toute implémentation :
1. **Rendre la descente adaptative** — n'y entrer que sur les rectangles où
   elle paie (6,2 % des rectangles portent 73,1 % du travail) : le coût du
   test devrait chuter d'un ordre de grandeur à gain presque constant.
2. **Vérifier que le gain croît avec $n$**, sans quoi il baisse la constante
   sans toucher l'exposant — et c'est l'exposant qui décide des contrats
   10–30 M. La mesure à 16 000 est en cours.

## 4. Verrous

- **V36** — acceptez-vous le **contrat d'exposant par régime** comme critère
  d'avancement (porte à code 3 sur les exposants locaux des compteurs de
  génération, seuils gravés par famille), plutôt qu'un objectif de
  sous-quadraticité au pire cas ?
- **V37** — la coupe par rayon est réfutée par la mesure (24 à 37 % des
  survivants q4 vivent à $D_{\max} \ge 64$ sur `terrain` et `scanline`).
  Confirmez-vous qu'elle doit entrer dans `PISTES_FERMEES.md` **avec cette
  mesure comme cause**, avant que quelqu'un ne la repropose ?
- **V38** — existe-t-il un **minorant du nombre de points strictement
  intérieurs valable pour toutes les ancres d'un rectangle** $A \times B$, et
  serré ? Formellement : une fonction $\underline{I}(A, B)$ calculable en
  $O(\text{polylog})$ telle que $\underline{I}(A, B) \le \min_{(a,b) \in A \times B} \lvert I_{B(a,b)} \rvert$
  et qui atteigne le seuil $h_q$ aussi souvent que le test d'ancre. Les
  histogrammes de coins actuels en sont un, mais lâche (ils ne tuent **rien**
  dans les classes $c \le 3$ et 75 % en $c = 6$ là où le test d'ancre en tue
  96 %). Est-ce un problème ouvert, ou connaissez-vous la construction ?
- **V39** — sur `terrain` q4, l'exposant est de **3,14** alors que les ancres
  ne croissent qu'en $n^{1{,}41}$ : le coût par ancre croît donc en
  $n^{1{,}73}$. Avant de concevoir, faut-il d'abord instrumenter **ce qui
  grossit dans une ancre `terrain`** (taille du cover ? nombre de seeds par
  ancre ? profondeur de la complétion q4 ?) — je penche pour oui, et je ne
  proposerai rien avant cette mesure.
- **V41** — la **descente prolongée** du § 3 bis vous paraît-elle correcte
  telle que je l'énonce (partition des paires, prédicat de mort inchangé,
  critère terminal de la WSPD intact, donc objet inchangé et aucune piste
  fermée rouverte) ? Si oui, elle est implémentable sans nouveau théorème et
  je la propose comme premier commit ; si non, où est la faute ?
- **V40** — l'asymétrie (médiane 5 seeds par rectangle contre un maximum de
  849 452) rend toute affectation « un rectangle par bloc GPU » inutilisable.
  Le découpage d'un rectangle **vivant** en sous-rectangles, à seule fin de
  répartir le travail, ne change ni les paires énumérées ni l'objet — il ne
  touche pas au critère terminal de la WSPD, donc pas à la piste fermée « cap
  de population dans le critère terminal ». Confirmez-vous cette lecture ?

## 5. Ce que je ne propose pas encore, et pourquoi

Aucune conception. Trois mesures manquent, et chacune peut invalider une
famille entière de solutions : le plafond d'un test de rectangle exact (en
cours), la décomposition du coût par ancre sur `terrain` (V39), et le
comportement des exposants au-delà de 50 000 points — sans quoi toute
extrapolation à 10 M reste une pente sur moins d'une décade.


## Réception Claude des réponses V36–V40 (même jour)

**J'accepte les cinq corrections de fait, dont une qui invalide une phrase que
j'ai publiée.**

1. **« Gain MAXIMAL » était faux.** W, secteurs et grille sont des conditions
   *suffisantes non nécessaires*, et les populations de mon quotient n'étaient
   pas alignées (`alive` post-histogramme en q3, post-W4 en q4 ; `killed`
   omettant W4 et la grille). Le bloc `plafond_test_rectangle` est **retiré**
   de `bench/rect_probe.cpp`, pas renommé. Il est remplacé par la mesure que
   vous désignez comme la seule bornante : la **ventilation de la mortalité par
   cause**, `k=1` (W_q, certificat *universel*) contre `k=2` (secteurs,
   *ancre-spécifique*) — seule la masse `k=1` borne ce qu'un raffinement de
   boîtes peut retirer.
2. **Les tailles ne viennent pas du même binaire ni du même `coord`.** Mes
   exposants mélangent `mhgp5_conformity_v4` (8/16/32 k) et `mhgp5_probe`
   (50 k) sur des nuages régénérés : ce n'est pas une série appariée. Je les
   requalifie en **diagnostic de pente à source épinglée**, et je referai la
   série avec un binaire et un `coord` uniques avant d'en tirer quoi que ce
   soit.
3. **La colonne « Jung » est `jung_cert_skip`**, pas le coût q4. Je la nomme
   désormais ainsi.
4. **Le débit $4{,}8 \times 10^{10}$/s n'est pas mesuré.** L'écart
   $3{,}14 - 2{,}28 = 0{,}86$ est un écart entre deux hypothèses
   conditionnelles, pas « l'exposant manquant à l'algorithme ». Je retire cette
   formulation.
5. **V36 : votre refus est fondé.** Mes seuils échouaient déjà sur
   `eight_clusters` (ancres q3 à 1,54 pour un seuil de 1,20). Je retiens vos
   deux rails : diagnostic de pente par compteur, source et intervalle
   épinglés, d'un côté ; budget produit sur le **mur de bout en bout** et le
   **pic mémoire** d'une taille cible de l'autre. Aucun CTest rouge n'est
   ajouté.

## Ce que ma mesure apporte à votre priorité q3 — et ce qu'elle réfute

Vous écrivez : « Le zéro `W4` déjà observé sur le run q4 fait de ce raccord une
priorité **q3** ». J'ai instrumenté le raffinement post-séparation
(`descente_prolongee`) avant d'avoir lu votre réponse, et **la mesure va dans
votre sens, y compris contre moi** :

| cas ($n = 16\,000$ sauf mention) | paires tuées | coût (nœuds visités) | cover évité (estimation) | rapport |
|---|---|---|---|---|
| `scanline` q3, $n = 8\,000$ | 33,7 % | 27,3 M | 92,1 M | 3,4 : 1 |
| `scanline` q3 | **43,8 %** | 84,4 M | 534,8 M | **6,3 : 1** |
| `scanline` q4 | 21,2 % | 109,2 M | 29,6 M | **0,27 : 1** |
| `terrain` q4 | **5,9 %** | 69,8 M | 4,0 M | **0,06 : 1** |

Trois lectures :

- **En q3 sur `scanline`, le gain croît avec $n$** (33,7 → 43,8 % des paires ;
  rapport 3,4 → 6,3) : c'est le seul endroit où ce raccord touche peut-être
  l'exposant et pas seulement la constante.
- **En q4 il est une perte nette**, et d'autant plus que la famille est celle
  qui échoue : sur `terrain` q4 il paie 69,8 M de visites pour éviter 4,0 M de
  sites — dix-sept fois défavorable. Votre prédiction (« en q4 il ne pourrait
  d'abord gagner que par des candidats de handles/requête plus petits ») est
  confirmée par la mesure, pas seulement plausible.
- Donc **ce raccord ne résout pas `terrain`**, qui est précisément le régime
  qui ne tient pas. Je ne le proposerai pas comme réponse au problème d'échelle,
  seulement comme une optimisation q3 conditionnelle.

Réserves que je maintiens sur mes propres chiffres : « cover évité » et
« seeds évités » sont des **estimations au prorata** des paires, presque
certainement fausses puisque les ancres lourdes dominent ; et le succès, comme
vous l'exigez, se juge sur le **temps et les visites payées**, pas sur
`seeds_cf`. Ma colonne « coût » est en visites de nœuds réellement comptées,
elle ; c'est la seule des trois qui soit une mesure.

## Ordre de travail que j'adopte, dans le vôtre

1. **Ventilation `k=1` / `k=2` par classe** (livrée ci-dessus dans le code,
   mesure en cours) — c'est elle qui décide si le raffinement q3 a une masse
   accessible, avant tout prototype.
2. **V39 : instrumenter le vrai flux `terrain`** (handles/requêtes,
   W/secteurs, constructions et visites de cover, grille, tests de `x`,
   lentille, remplissage affine, puissance q4 ; avec `Dmax`, `D2`, population
   des handles et taille de cover). C'est le régime qui échoue, et je n'ai
   aucun modèle de son coût.
3. **Série d'exposants appariée** — binaire unique, `coord` unique, mêmes
   nuages — avant toute reprise de l'argument d'échelle.
4. Seulement ensuite, et seulement si 1 donne une masse : prototype de
   raffinement q3 avec `L = 0..3`, ledger de masse de paires
   (`emitted + postsep_killed = base_alive`), **route q2 interdite**, fixture
   `refine-hist-wakeup` gravée sur vos quatre positions, et abandon si le temps
   et les visites payées ne baissent pas.

Une question de méthode, V42 : vous demandez de comparer « le multiensemble
littéral des couples d'indices avant/après sur de petits arbres ». Faut-il en
faire une **porte** (avec ses planchers et son mutant), ou un diagnostic du
prototype ? Je penche pour la porte dès le prototype, parce qu'une perte de
paire est exactement le genre de faute qu'un digest global masque.
