# Question Claude — la sous-quadraticité **par régime**, et ce qu'elle exige (28 août 2026)

Ancrage : reçus de génération épinglés depuis `839cf1ec`, campagne directe
`scanline` 200 k et instruments exploratoires jusqu'à `819cac3c` ; la fraîcheur
du pin jugé est tenue dans [`ETAT_COURANT.md`](ETAT_COURANT.md).
Cadre : `phase=exploration_v5_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

## Réponse à Louis — généraliser la WSPD, mais par les centres

### Verdict

**Oui, il faut généraliser, mais pas en une WSPD symétrique de triplets ou de
quadruplets.** La bonne cible est une décomposition à deux étages : la WSPD de
paires reste le squelette exact et possède chaque ancre diamètre une fois ;
derrière chaque ancre survivante, un arrangement local unique traite les
complétions q3 et q4 sans développer leur produit cartésien.

La généralisation nommée existe dans la littérature : la
[WSSD de Kerber--Sharathkumar](https://arxiv.org/abs/1307.3272) couvre chaque
simplexe par un tuple de cellules bien séparées et donne une approximation de
Čech de taille linéaire à dimension fixée. Elle est utile comme broad phase ou
comme vocabulaire, mais elle n'est ni une partition exacte des supports, ni une
autorité de rang, de shell ou d'exact-once. La rendre exacte en développant
`A x B x X` ou `A x B x X x Y` recrée précisément le coût recherché. La
frontière directe de produits déjà testée confirme ce no-go pratique : ses
boîtes mélangent presque toujours supports acceptés et refusés, et un certificat
ne couvrait qu'environ 1,1 tuple dans
[`RAPPORT_SESSION_20260808.md`](../../docs/research/RAPPORT_SESSION_20260808.md).

Le nom de travail utile est donc **décomposition simpliciale bien séparée
fibrée par l'ancre**, pas WSSD standard :

1. `PairBlock(A,B)` partitionne les paires par la WSPD actuelle ; un certificat
   universel peut tuer tout le bloc, sinon il est raffiné ou émet des ancres ;
2. `AnchorCenterArrangement(a,b)` traite ensemble tous les tiers d'une ancre,
   en scratch borné puis en flux vers le RLE existant.

### L'objet commun q3/q4

Fixons l'ancre possédée `e=(a,b)`, son milieu $M$, sa longueur $D$ et son plan
médiateur. Chaque site $x$ induit dans ce plan la droite orientée
$h_x(v)=0$, avec une identité exacte déjà démontrée :

$$h_x(v)=2v\mathbin{\cdot}(x-M)-\left(\left\Vert x-M\right\Vert^{2}-\frac{D^{2}}{4}\right)=r^{2}-\left\Vert x-(M+v)\right\Vert^{2}.$$

Ainsi `h_x(v)>0`, `=0`, `<0` signifie respectivement intérieur strict, shell,
extérieur pour la sphère de centre `M+v`. Ce même arrangement donne les deux
lanes :

- **q3 :** `x` ne demande pas une nouvelle recherche géométrique. Le centre du
  triangle `(a,b,x)` est le point marqué $v_x$ de norme minimale sur sa propre
  droite `h_x=0` — son intersection avec le plan affine du triangle.
  On conserve ce point si sa profondeur stricte est au plus
  $\kappa_3=h_3-1=s_{\max}-3$, soit **8** pour `smax=11`. Le scan actuel
  `x x cover` devient une localisation dans le préfixe peu profond commun ;
- **q4 :** le centre de `(a,b,x,y)` est le sommet commun à `h_x=0` et `h_y=0`.
  On énumère directement les sommets de profondeur au plus
  $\kappa_4=h_4-1=s_{\max}-4$, soit **7**, puis seulement les filtres exacts de
  diamètre, owner, bon centrage, shell et `BallKey`. On ne forme jamais les
  $\binom{m_e}{2}$ paires de tiers.

Les points intérieurs sur tout le disque sont comptés une fois dans
`c_{e,q}` ; le budget résiduel devient `kappa_q-c_{e,q}`. Le disque q3 est
contenu dans celui de q4 : rayons carrés respectifs `D2/12` et `D2/8`. Une
préparation des droites du disque extérieur peut donc servir les deux lanes
pour toute ancre commune, q3 n'interrogeant que son disque intérieur. Les
certificats W/secteurs, grille et morceaux de corde restent des pré-prunes
facultatifs : ils ne deviennent pas la source des sorties. Pour préserver
d'abord le contrat v5 et `digest_balls`, l'intégration doit garder le cover
coefficient 3 du chemin courant. Élargir le range-report q4 au confinement
complet de Jung peut tuer davantage de candidats profonds et constitue un
changement de contrat séparé, déjà averti par
[`PISTES_FERMEES.md`](../docs/PISTES_FERMEES.md).

L'abstraction se généralise en dimension $d$, sans généraliser naïvement les
produits : une paire diamètre laisse un espace de centres de dimension $d-1$ ;
chacun des $q-2$ porteurs supplémentaires y ajoute un hyperplan orienté ; le
centre du support est le point de norme minimale de leur intersection, et son
budget de profondeur stricte vaut $s_{\max}-q$. En 3D, cela donne `q2 = v=0`,
`q3 = point marqué sur une droite`, `q4 = intersection de deux droites`. Cette
formulation unifie les preuves et les données, mais ne promet pas la même borne
combinatoire en dimension supérieure.

### Gain obtenu, et limite honnête

Pour une ancre `e`, notons `m_e` le nombre de droites actives. En position
générale, le nombre de sommets q4 admissibles par profondeur vérifie
$Z_e\leq m_e(\kappa_e+1)$ ; à `smax=11`, cela donne au plus `8*m_e`, contre
`m_e*(m_e-1)/2` complétions. Les constructions de niveaux peu profonds donnent
comme cible théorique locale
$O(m_e\log m_e+m_e(\kappa_e+1))`. Le document
[`RNG_JUNG_CLIQUES_ET_NIVEAUX_PEUPROFONDS.md`](../../docs/math/RNG_JUNG_CLIQUES_ET_NIVEAUX_PEUPROFONDS.md)
porte la preuve géométrique, les bornes et leurs limites.

La complexité totale revendicable reste conditionnelle :

```text
T = T_pair_blocks + T_range
  + sum_e O(m_e log(m_e) + m_e*(kappa_e+1))
  + T_exact + T_sink
```

Cette architecture supprime le carré **local** de q4 et le rescan q3 par `x`.
Elle ne prouve pas que le nombre d'ancres `a`, les blocs visités ou
`M=sum_e m_e` sont sous-quadratiques. C'est la WSPD extérieure et ses
certificats center-cover qui doivent contenir ces quantités. Le pire cas reste
dense ; le contrat réaliste est de mesurer `a`, `M` et `sum Z_e` par régime.
En dimension trois, il n'existe pas de support minimal de miniboule au-delà de
quatre points : il faut généraliser l'abstraction, pas ouvrir q5.

### Plan qui aide Claude sans engager une refonte aveugle

1. **R0, sonde sans décision :** sur les ancres courantes, publier
   `m_e`, `c_e`, `kappa_e`, `Z_e`, `sum binom(m_e,2)`, `sum m_e*(kappa_e+1)`,
   quantiles/maxima, temps de range, arrangement, exact et sink, plus le pic
   scratch. La baseline quadratique est analytique, jamais exécutée aux grandes
   tailles.
2. **R1, oracle borné :** pour petits nuages seulement, développer toutes les
   intersections, grouper les concurrences exactement et comparer le
   multiensemble pré-RLE, les `BallKey` post-RLE, les compteurs sémantiques et
   les digests au chemin courant.
3. **R2, vrai constructeur shallow CPU :** scan plat pour les ancres légères,
   arrangement q4 pour les moyennes/lourdes, puis q3 lourd si la préparation
   commune paie. Un prototype qui calcule d'abord les
   `binom(m_e,2)` intersections échoue la porte même s'il filtre ensuite.
4. **R3 seulement si l'amont domine encore :** ajouter le center-cover exact
   par blocs d'extrémités avant l'émission des ancres. Ne pas revenir au produit
   symétrique de quatre nœuds.

Les cas dégénérés sont contractuels : droites parallèles, identiques ou
concourantes, centre sur la frontière du disque, profondeurs 8/9 en q3 et 7/8
en q4, shell cosphérique jusqu'au cap, ties d'owner et `det=0`. Les concurrences
sont groupées par centre/`BallKey` et passées au census complet ; aucun jitter.
Au-delà du cap de shell, le statut reste `resource_exhausted`, jamais une
omission. Les mutants minimaux retirent ou dupliquent une droite, changent
`<` en `<=`, décalent `kappa` de un et éclatent une concurrence.

### Correction du dernier contre-proxy

La rétractation de « 31 jours » et de « il manque exactement 0,86 » est juste.
En revanche, les nouveaux temps `2 s / 42 s / 1,3 min` ne sont pas reçus : ils
appliquent encore le débit non mesuré `4,8e10/s`, cette fois à
`completions_q4`, une unité différente de celle qui avait inspiré ce débit.
`completions_q4` est un meilleur compteur de boucle que `jung_cert_skip`, mais
pas une horloge.

Le reçu direct `scanline` 200 k avec grille mesure déjà **214,544 s** dans le
corps q4 sur **267,701 s** de mur. Il interdit donc la conclusion générale
« la génération n'est pas le mur » et rend a fortiori le temps 10 M de 1,3 min
non interprétable. La mémoire reste un chantier prioritaire, mais elle ne
disqualifie pas la suppression architecturale de `x x y` : conserver deux rails
séparés, mur/RSS mesurés d'une part, compteurs de travail d'autre part. La note
autonome de rétractation est consolidée ici puis retirée du tip.

## Réponse auditée — V36 à V41

**Verdict court : V39 oui ; V40 oui seulement comme tuilage de scheduling ;
V41 est une bonne ablation q3/q4, mais pas encore un algorithme reçu ; V37 ne
ferme qu'une coupure aveugle ; V36 est refusé dans sa forme actuelle ; V38 doit
être reformulé avec un modèle de prétraitement.** Les calculs d'extrapolation
sont arithmétiquement reproductibles, mais leur interprétation « trois régimes
tiennent 10 M » n'est pas démontrée et est déjà contredite par la mesure
historique `scanline`.

Quatre faits doivent d'abord être corrigés :

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
3. La provenance du débit de `4,8e10` évaluations/s n'est pas établie. Il est
   compatible, sur le seul cas terrain 50 k, avec une double agrégation des 48
   fils :
   `7 677 090 545 / 7,8782 s = 9,74e8 skip/s` au mur ; multiplier encore par
   48 donne `4,68e10`, presque la constante annoncée. Cette coïncidence n'est
   pas une preuve causale, d'autant que le mur q4 contient d'autres coûts. Selon
   la famille, le débit
   mural observé du total Jung ne vaut qu'environ `1,03e8` à `1,02e9` par
   seconde : l'hypothèse est 47 à 465 fois trop haute. Elle prédit même uniform
   1 M en 0,2 s alors que q4 seul prend déjà 3,12 s à 50 k. En supposant malgré
   tout ce débit, une pente figée et aucun autre coût, les temps du tableau sont
   bien ceux du calcul ; ils ne décrivent pas le mur CPU. L'écart
   `3,14 - 2,28 = 0,86` est donc l'écart entre deux hypothèses incompatibles avec
   le reçu, pas l'exposant « exactement manquant » à l'algorithme.
4. La session 11, au pin historique `82f613d3`, mesure déjà `scanline` à 100 k
   et 200 k. Sur 50/100/200 k,
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

Concrètement, la garde déterministe de non-régression doit être indexée par
`(famille, compteur payé, intervalle)` et comparer un même binaire produit
hashé sur des tailles et graines fixes, sans claim statistique. La scorecard de
recherche peut viser une pente sous 2 sans faire échouer CTest, mais doit aussi
publier les pentes adjacentes et les résidus : un unique ajustement log-log peut
masquer la courbure déjà observée. Toute inférence de famille exige un protocole
préenregistré qui fixe tailles, graines, répétitions, niveau de confiance et
unité de rééchantillonnage ; cinq pentes ne justifient pas à elles seules une
borne bootstrap. Les temps demandent des répétitions appariées et un
environnement épinglé. Les SLO produit restent absolus : mur, RAM, SSD et octets
de sortie à 50 k, pont réel résident-streamé à 1 M, puis contrats séparés 10 M
`prefixe_k5` et complet. Un compteur de sortie ne doit jamais être soumis au
plafond du travail payé.

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

Le prototype GPU courant n'affecte pas un rectangle à un bloc : q3 affecte un warp
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
rectangle racine, non 1,49 sous-rectangle engendré. Enfin, le rejeu local reproduit les
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


## Réponse auditée à V42 et deux corrections de réception

**Oui : en faire une porte de correction bornée dès le prototype.** Deux
niveaux complémentaires évitent de transformer l'oracle en coût produit :

1. Sur de petits arbres, développer littéralement les couples de positions
   uniques. La porte compare après tri canonique la réunion disjointe
   `couples_emis union couples_branches_certifiees_mortes` au multiensemble du
   front vivant de base. Elle exige au moins une scission de A, une de B, une
   branche morte, une survivante et un rollback pour enfant non séparé. Des
   mutants retirent un enfant, le dupliquent et appliquent un effet avant le
   rollback ; `refine-hist-wakeup` tue séparément toute activation q2.
2. Sur le chemin de taille, ne matérialiser aucun couple : conserver seulement
   le ledger u128 d'ancres uniques
   `emitted_pair_mass + postsep_killed_pair_mass = base_alive_pair_mass`. Si la
   masse pondérée par multiplicité de PointId est revendiquée, lui donner un
   second ledger explicitement nommé. Les digests de candidats, sorties,
   forêts, événements et niveaux restent des portes aval distinctes.

Cette porte doit être un CTest dédié du prototype, avec planchers de non-vacuité
et codes de sortie exacts. Le benchmark garde seulement les compteurs ; il ne
devient jamais l'oracle quadratique.

Deux formulations de la réception `4ecb57d4` restent à corriger avant la série
suivante :

- le binaire 50 k est `mhgp5`, pas `mhgp5_probe`. Surtout, imposer une valeur
  numérique de `coord` identique à toutes les tailles changerait la densité et
  donc le régime. Épingler le générateur et sa règle `coord(n)`, ou déclarer une
  construction emboîtée différente ; enregistrer chaque valeur effectivement
  utilisée ;
- la masse `k=1` ne borne que les ancres post-histogramme/post-W4 effectivement
  soumises au test W. Elle ne borne ni toute la masse de paires supprimable avant
  ces filtres, ni le temps. Enfin, q4 n'est pas encore une « perte nette » : les
  69,8 M visites sont mesurées, mais les 4,0 M covers évités sont estimés au
  prorata et les unités n'ont pas le même prix. Ce signal ne classe donc pas la
  priorité q4 ; il refuse seulement ce raffinement tant qu'une mesure intégrée
  du mur et des visites payées ne le rend pas favorable.

En q3, `33,7 % -> 43,8 %` décrit donc une **opportunité de pruning sur deux
tailles**, pas encore un gain croissant ni une baisse d'exposant. Cet ordre
historique est remplacé par R0--R3 ci-dessus : partager le diagnostic puis
traiter le carré local q4 avec le constructeur shallow.

## Requalification de la mesure prédicteur `905c5361`

La ventilation `k=1/k=2` est utile, mais le coefficient `70,6 %` n'est pas
identifié par les totaux publiés. Notons, dans les rectangles vivants de base :

- `E` les paires éliminées avant le test W ponctuel — histogramme en q3,
  histogramme puis W4 explicite en q4 ;
- `P` la population restante qui atteint ce test, et `W1` les paires de `P`
  tuées par `k=1` ;
- `K` les paires certifiées mortes par la descente post-séparation.

La relation sûre est `K subset E union W1`. Elle ne donne ni `E subset K`, ni
une partition de `K` sans mesurer l'intersection. Pour `scanline q3` 16 k, les
valeurs annoncées `|K|=696 537`, `|E|=310 615` et `|W1|=546 779` impliquent
seulement :

```text
385922 <= |K inter P| <= 546779
70,581 % <= |K inter P| / |W1| <= 100 %
149758 <= |K inter E| <= 310615
```

Le `70,6 %` publié est donc la **borne inférieure** obtenue en supposant à tort
que toutes les morts histogramme appartiennent à `K`. Pour l'identifier, la
sonde doit imprimer la partition exacte
`K = K_pretests + K_k1`, avec intersections calculées paire par paire et les
conservations correspondantes. Sur uniform q3, `141 246 / 564 834 = 25,007 %`
n'est également qu'une borne supérieure avant retrait de `K inter E`, pas un
taux récupéré « inférieur à 25 % » déjà mesuré.

Le zéro `k=1` q4 signifie `K inter P = emptyset`, pas `K = emptyset`. Il est
donc compatible avec les 5,9 à 40,4 % de paires q4 supprimées par la sonde :
ces paires doivent appartenir aux prétests `E` et peuvent encore éviter des
coûts amont. Elles ne prouvent en revanche aucune économie du corps q4 aval.

Enfin, les rapports 0,06:1 à 6,3:1 divisent des covers estimés au prorata par
des visites d'arbre mesurées. Ils ne classent ni gain ni perte de temps. Les
phrases « paie sur un seul cas », « perd partout ailleurs » et « gain croissant »
restent donc rejetées jusqu'au prototype intégré. La note autonome de
`905c5361`, sans commande, sortie brute, hash de binaire ni pin de configuration
opposable, est consolidée ici puis retirée pour garder `audits/` propre.
