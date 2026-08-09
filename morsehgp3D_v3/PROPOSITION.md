# MorseHGP3D v3 — proposition d'architecture

> **Ce document est une proposition, pas une autorité.** L'autorité mathématique
> reste [`SPECIFICATION_MORSEHGP3D.md`](../docs/SPECIFICATION_MORSEHGP3D.md) et
> [`STATUT_PREUVES_ET_HEURISTIQUES.md`](../docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md).
> L'état live est contrôlé par
> [`AUDIT_ETAT_COURANT.md`](audits/AUDIT_ETAT_COURANT.md); les autres audits sont
> indexés comme lectures courantes ou archives dans
> [`audits/README.md`](audits/README.md). Aucun audit ne certifie à lui seul une
> porte, un statut public ou un SLO.

## 0. Convention de statut

Chaque énoncé chiffré porte l'une de ces étiquettes, et rien n'est promu sans
reçu :

| étiquette | sens |
| --- | --- |
| **[théorème]** | démontré ici ou dans une note du dépôt, avec sa preuve |
| **[mesuré]** | produit par une exécution, avec sa commande |
| **[extrapolé]** | calculé depuis une mesure sous un modèle explicite |
| **[diagnostic]** | observation utile, sans reçu reproductible — ne qualifie rien |
| **[obligation]** | à prouver ou à mesurer avant toute décision qui s'y appuie |

**[obligation] générale** : aucune des mesures citées ci-dessous ne possède
aujourd'hui de sidecar complet (schéma, commit, binaire, compilateur, machine,
digest d'entrée, graine, compteurs de campagne fermés, sortie brute). Tant que
ce n'est pas fait, elles sont des diagnostics. C'est la première dette à payer.

## État courant du prototype audité

Le worktree reste `exploration_v3_hors_registre`, avec CPU de référence et
microkernel GPU candidat sous audit, sur le profil
`quantized_u16_input_only`. Il ne réalise pas encore l'architecture cible.

Résultats positifs scellés sur leurs entrées : les cinq portes flats Release à
petites coordonnées passent sur 4 990 cas, deux campagnes ASan/UBSan passent
sans diagnostic, et les gardes de filiation distinguent maintenant les
directions invalides et les cibles incohérentes. `kSinkStopped` sépare aussi
l'arrêt volontaire d'une erreur scientifique. Ces campagnes ne couvrent pas la
frontière entière u16 décrite ci-dessous. Les quatre portes du microkernel
passent sur CPU; une session G4 rapporte aussi zéro écart du payload borné
hôte/device. Aucun reçu brut n'est versionné et les refus ne sont toujours pas
rejoués.

Quatre verrous priment sur tout travail d'échelle; leurs constructions sont
réunies dans la
[`note mathématique courante`](audits/NOTE_VERROUS_MATHEMATIQUES_PRIORITAIRES.md) :

1. Le chemin owner tronque actuellement un déterminant `i128` en le transmettant
   à `tangent_sign(int, ...)`. Un tétraèdre d'échelle 1291 rend 7 sphères dans
   le catalogue normal et seulement 4 en owner; une seconde frontière déclenche
   `-INT_MIN` sous UBSan. Il faut réduire par `sign_of` avant toute optimisation.
2. La sémantique F0 est maintenant décidée : le fold général doit accepter une
   naissance directe tout $N_a$. L'invariant régulier d'au moins deux facettes
   strictes est conditionnel et se valide par record avant projection; Warshall
   et DSU partagent encore le garde contraire.
3. Hors de cette frontière entière, la correction `use_owner` concorde sur
   174 444 exécutions externes et sa
   porte permanente compare désormais le catalogue sémantique. Elle ne protège
   cependant pas l'identité du propriétaire signé : le mutant non signé échange
   les deux extrémités sans changer la sortie. La table est nulle seulement pour
   owner+index+navigable et le high-water publié compte des entrées, pas des
   octets.
4. Le census terminal de la source est une requête exacte de demi-espace après
   relèvement en dimension quatre. Cette existence mathématique est fermée;
   l'index certifié, les statuts cappés, la terminalité et les constantes restent
   à implémenter et recevoir.

Conséquence d'architecture : le propriétaire exact et le fold externe restent
les directions proposées. Le propriétaire est NO-GO sur le profil u16 tant que
la troncature et sa fixture permanente ne sont pas fermées; il ne possède pas
davantage de garantie globale sans table. La porte owner et la sémantique de
naissance F0 se ferment avant les optimisations, le GPU ou une revendication
mémoire.

La voie GPU suit un contrat distinct, détaillé dans la
[`note mathématique GPU`](audits/NOTE_VERROUS_MATHEMATIQUES_GPU.md). Le verdict
parent u16 tient exactement en 64 bits; voisin, owner et census demandent 128
bits; l'ordre exact des niveaux demande 384 bits ou un merge CPU. La
reverse-search se partitionne en sous-arbres disjoints, mais chaque tâche doit
commit ou rollback entièrement et tout refus doit rejouer le sous-arbre. Le live
possède un `.cu`; son exécution G4 déclarée ne calcule que les masques
d'admissibilité sur des sommets déjà produits par CPU. `neighbour_along`, replay
et reçu qualifiant restent absents.

Le profileur à densité fixe de `f851374` corrige une faute du protocole cube
antérieur et fournit un diagnostic CPU reproductible. Il ne borne pas 50 k : une
densité et une graine synthétiques, une moyenne qui ignore les statuts non `kOk`
et deux étages partiels ne décident ni les 100 ms primaires ni la seconde
secondaire. La source critique directe est une hypothèse prioritaire; sa
complétude, son census terminal et son coût output-sensitive restent à prouver.
Le delta `70ead99` ajoute quatre tailles utiles, mais trois incréments décroissants
ne prouvent ni convergence ni asymptote : les projections `1 430/390` par point
et les temps qui en découlent restent des scénarios conditionnels. Le tableau
mélange en outre deux répétitions à `n=100` et une à `n=200`; ses incréments ne
forment pas encore une série statistique homogène.

Le probe de sélectivité committé dans `40ad152` contient un P0 indépendant : il cherche
le minimum d'un demi-plan fermé seulement sur les directions des points et
compte les points de frontière. Trois projections sur un même rayon suffisent à
faire rendre 5 au lieu de 2 et à réfuter une paire critique `RelevantGP`. Le
lemme de demi-boule n'est pas réfuté; son oracle doit devenir un sweep circulaire
exact du demi-plan ouvert complémentaire avant toute mesure de sélectivité.
Les quatre tailles ne prouvent donc ni `O(n log n)` ni 7,5 millions de
candidates à 50 k. Le commit k-NN `5d9159a` (probe `130e316e...`) mesure encore
l'ensemble fautif, avec une matrice et $n$ tris hors chrono; son maximum reste
seulement une borne inférieure ponctuelle et son histogramme ne ferme pas cette
porte. Une construction u16 de 50 000 points donne même une paire Gabriel vide
avec `A=2` et rang croisé 25 000 : aucun petit `k` ne découle de
l'admissibilité, et tout filtre k-NN exige un complément exact certifié.

Une voie constructive distincte est maintenant formulée sous la capability
`center-cover + degree` : banque de témoins stricte par cellule, preuve de rayon
avant census local, voisinages complets de rayon `4Q`, groupement cross-lane par
`SphereKey` et replay de tout cap. Elle est mathématiquement exacte et ne
construit aucun arrangement; son SLO reste conditionné aux degrés, masses
combinadiques, locator, construction, tri et sortie reçus. Le terminal
`AboveInteriorWindow` doit être versionné explicitement, car la norme active
demande encore un shell complet sur une fenêtre uniforme.

## 0 ter. Ce que M3 a tranché, et ce qu'il a déplacé

**Le plan en cinq lignes ci-dessous a été exécuté jusqu'à sa branche de
décision, et la décision est prise.** Elle n'est pas celle qui était anticipée :
ni A1-source, ni A2e, ni A2p. Le générateur retenu ne s'ancre nulle part — il
**navigue** dans le $\leq k$-niveau de l'arrangement relevé, où le rang fermé
vaut $\ell+\lvert S\rvert$ et où le niveau se transporte le long d'un pinceau au
lieu de se recalculer. Le détail est au `README.md`.

**Deux énoncés de ce paragraphe ont été réfutés depuis, et corrigés là-bas** :
le transport n'est pas en $\pm1$, et pas davantage en $-1$, $0$ ou $+1$ : il vaut $\lvert D_-\rvert-\lvert A_{\text{int}}\rvert$ et n'est borne par rien des que plusieurs hyperplans coincident, le lot pouvant
laisser le niveau inchangé ; et la coupe du graphe ne porte pas sur le rang
fermé mais sur le **niveau strict**, plafonné à $s_{\max}-2$ — couper sur le
rang fermé supprime des sommets de niveau zéro et rompt la connectivité.

Ce que cela **retire** de la présente proposition :

- `A1-source` cesse d'être un verrou : il n'y a plus d'ancres.
- `PEL-1` à `PEL-4` cessent d'être des obligations *bloquantes* : elles portaient
  sur le constructeur ancré, qui n'est plus le chemin produit. Le dictionnaire de
  profondeur reste vrai, et reste utile comme second oracle.
- La question de la cosphéricité change de statut : elle n'est plus un rejet de
  domaine mais un **cas traité**, la coquille entière étant l'objet porté.

Ce que cela **déplace**. Le facteur **100** entre travail et sortie, annoncé ici
comme la seule question restante, était un **artefact** : il comparait les
sommets visités à une récolte défaillante qui perdait l'essentiel des arités
deux et trois. Mesuré sur le catalogue complet et vérifié contre la force brute,
le rapport réel est d'environ **17:1**. La sortie est six fois plus grosse
qu'annoncé ; le travail total, lui, n'a pas bougé.

Le mur restant est double, et il est désormais chiffré au §5 du `README.md` :
la requête de pinceau balaie exactement $8(n-4)$ candidats faute d'index, et la
récolte paie un census en $O(n)$ par candidat dont 43 % sont des doublons.
Aucune des deux n'est une propriété du problème.

## 0 quater. Trois réserves qui n'étaient écrites nulle part

**(a) Le terrain parcouru et la mosaïque d'ordre supérieur sont reliés par une
dualité et une tranche, pas identiques.** Sous position générale, le pavage de
rhomboïdes est dual de l'arrangement relevé et sa tranche de profondeur $k$ est
la mosaïque de Delaunay d'ordre $k$. En dimension trois, un sommet
d'arrangement est dual d'un rhomboïde de dimension quatre, dont les tranches
non triviales donnent des cellules Delaunay tridimensionnelles à plusieurs
ordres; il n'est pas en bijection avec un sommet de mosaïque. Les quatre sommets
u16 d'un tétraèdre donnent déjà un sommet d'arrangement de niveau zéro contre
quatre sommets dans la mosaïque d'ordre un. Le parcours économise réellement
les cellules, incidences, étiquettes par ordre et le dual matérialisé, mais
aucun facteur mémoire cinq à dix ni aucune identité de nombres de sommets n'en
découle. Le gain qui compterait, ne pas tout retenir, n'est pas obtenu
aujourd'hui.

**(b) La borne connue ne suffit pas au contrat.** Clarkson--Shor borne le
$\leq k$-niveau de $n$ hyperplans de $\mathbb{R}^4$ par $O(n^2k^2)$ sous ses
hypothèses asymptotiques et de position générale; pour $K=10$ fixé, elle reste
quadratique en $n$. Les mesures publiées sont plusieurs ordres de grandeur en
dessous parce qu'un relevé est localement une surface. C'est un **régime**, pas
un théorème — cohérent avec le retrait déjà acté au §14 de « surface
$\Rightarrow$ faible profondeur presque partout ». Le census multi-captation
du §1.5, qui donne le nuage à dix captations recalées *moins* peu profond que la
reconstruction fusionnée, est le contre-exemple attendu et il est déjà mesuré.
C'est la branche no-go de **Gate D**.

Références primaires : [Edelsbrunner--Osang, proposition 1](https://pub.ista.ac.at/~edels/Papers/2020-J-07-SimpleAlgorithm.pdf) pour la dualité et les tranches; [Clarkson--Shor, corollaire 3.3](https://link.springer.com/content/pdf/10.1007/BF02187740.pdf) pour la borne shallow.

**(c) La mémoire de navigation et le GPU ont un verrou commun.** La table `seen`
des coquilles visitées est à la fois ce qui coûte la mémoire du parcours et ce
qui interdit une expansion device sans communication — table de hachage globale
à clefs de longueur variable, écrite par tous les fils. La *reverse search*
d'Avis et Fukuda supprime ce verrou de visitation. Le parent est désormais
prouvé sur le vrai graphe multiplicitaire sous coquille, intérieur et oracle
`next` exacts; son premier juge live ne remplace pas encore le BFS. Le
déterminisme de sortie, le tri en $\beta$, la partition horizontale, la couverture
et les verticales restent des globalités séparées. Un GPU multiplie le débit sans
réduire le nombre de sommets visités : il ne répond pas à la question de fond.

## 0 quinquies. Séquencement : le cas simple d'abord, les dégénérescences en dernier

**Décision.** Le premier algorithme complet est écrit sous hypothèse
d'arrangement **simple** — aucune cosphéricité, aucune coplanarité portante — et
le traitement des multiplicités est reporté à l'une des **dernières phases** du
projet.

Ce séquencement n'est plus imposé par l'existence du parent : son extension
multiplicitaire est prouvée. Il reste justifié comme ordre d'implémentation tant
que l'énumération des flats multiples, leur propriétaire canonique et l'oracle
rationnel ne sont pas qualifiés. On règle d'abord l'architecture — index,
propriétaire, streaming, tri global, forme device — sur le terrain simple, avant
de rouvrir ces coûts multiplicataires.

Trois conditions, faute de quoi la décision serait une régression : le domaine
est **déclaré et gardé fail-closed**, un nuage cosphérique étant refusé avec sa
raison nommée et jamais traité comme simple ; le travail multiplicitaire déjà
fait est **conservé et reste vert**, et devient la porte d'entrée de la phase
finale ; et **aucune mesure à l'échelle sur donnée réelle** n'est revendiquée
depuis cette version, la cible u16 quantifiée n'étant pas dans ce domaine.

## 0 bis. Le plan, en cinq lignes

1. **V3-O, le juge** — un oracle indépendant en précision arbitraire, à campagne
   fermée, sur le domaine déclaré ; et les reçus des mesures déjà faites.
2. **V3-P, le falsificateur CPU** — un programme exact qui construit, pour une
   ancre, le sous-complexe shallow **stratifié** de l'arrangement. Il peut réfuter
   PEL-1 à PEL-4 et mesurer leurs coûts sur corpus ; une campagne finie ne peut ni
   démontrer leur portée universelle, ni rendre A1-source « gratuit ».
3. **V3-C, le census** — les compteurs de V3-P à l'échelle, sur de vrais nuages,
   y compris multi-captation, avec le ledger complet.
4. **Décision** à deux branches, puis seulement : descente indexée, tri externe
   et réducteur, GPU, reçu produit.
5. Rien de ce qui suit n'est un produit tant que M2 n'a pas parlé.

Le détail et la justification de l'ordre sont au **§13**.

## 1. Pourquoi une v3

La v2 n'a pas une conception fausse. Elle a un **substitut de force brute posé à
la place de sa conception**, qui n'a jamais été écrite : `DESIGN.md` §7 budgète un
peeling local sensible à la sortie et une descente indexée ; `src/catalogue.cpp`
énumère exhaustivement tous les quadruples du voisinage et
`src/forest.cpp::descend` balaie le nuage à chaque bras.

**[mesuré]** (2 vCPU, Release) : $n=200$, $K=10$ →
$258\,739\,800=200\cdot\binom{199}{3}$ quadruples en 26,3 s ; $n=500$, $K=2$ →
plus de 300 s. $W_p$ vaut le nuage entier, à tout $K$.

### 1.1 La cause racine, lue dans le code

`src/catalogue.cpp:380` : `radius_bound` rend `+infini` dès que
`vals.size() < s_max`. La relaxation conique n'admet que les points à moins de
$49{,}7^\circ$ de l'axe — $17{,}7\ \%$ de la sphère ; il suffit qu'**un** cône sur
42 contienne moins de $s_{\max}$ points admissibles. Alors `diam_cut = +infini`
(`:290`), `std::isfinite` est faux (`:296`), toutes les paires restent vivantes,
et la triple boucle visite $\binom{\lvert W\rvert}{3}$ triplets.

### 1.2 Même le théorème 4 exact ne sauve pas l'énumération locale

**[mesuré]** $\lvert W\rvert_{\text{cert}}$ à $n=50\,000$, $K=10$, par bissection
sur le plus petit $\rho$ vérifiant $2\tau\leq\rho$ ; **[extrapolé]** les temps, à
partir du débit **[mesuré]** de $3{,}2$ à $8{,}9\cdot10^{6}$ quadruples/s/cœur
sur 48 cœurs — l'intervalle est donné, pas un point.

| configuration | $\lvert W\rvert$ | quadruples | temps (48 cœurs) |
| --- | ---: | ---: | ---: |
| code actuel | $n-1$ | $1{,}04\cdot10^{18}$ | 77 à 215 ans |
| croissance réparée, $\theta$ du dépôt | 17 578 | $4{,}53\cdot10^{16}$ | 3,4 à 9,3 ans |
| + golden angle corrigé | 5 764 | $1{,}59\cdot10^{15}$ | 43 à 120 jours |
| + 2 000 cônes | 252 | $1{,}32\cdot10^{11}$ | 309 à 859 s |
| **théorème 4 exact, $\theta=0$** | **175** | $4{,}39\cdot10^{10}$ | **103 à 286 s** |
| voisinage parfait observé | 130 | $1{,}79\cdot10^{10}$ | 42 à 117 s |
| *ce qu'il faudrait pour 100 ms* | — | $1{,}5$ à $4{,}3\cdot10^{7}$ | 0,1 s |

avec le modèle fermé
$\lvert W\rvert_{\text{cert}}=8\,s_{\max}\cos\theta/(\cos\theta-\sin\theta)^4$
**[extrapolé, vérifié contre la mesure]**.

Le budget de 100 ms impose $\binom{\lvert W\rvert}{3}\lesssim10^{3}$, soit
$\lvert W\rvert\leq18$ ; une seconde impose $\lvert W\rvert\leq38$ au meilleur
débit publié. **[obligation]** ces lignes remplacent le coût réel
$\sum_p\binom{\lvert W_p\rvert}{3}$ par $n\binom{\overline{\lvert W\rvert}}{3}$ :
par convexité c'est une **sous-estimation**, et les queues peuvent gouverner le
temps. La distribution complète de $\lvert W_p\rvert$ doit accompagner le tableau,
et chaque ligne doit dire si son $\lvert W\rvert$ est une moyenne, un quantile ou
un maximum. **[mesuré,
sur le corpus testé]** le support le plus lointain d'une sphère critique émise
est le 89ᵉ à 165ᵉ voisin, et il faut le 189ᵉ pour compter les rangs — donc
$\lvert W\rvert\geq130$ **sur ce corpus**, ce qui n'est pas une borne universelle
de la géométrie à $K=10$. **[mesuré]** rendement : $1{,}5$ à
$2{,}4\cdot10^{-3}$ sphère par quadruple testé, soit 400 à 650 candidats jetés
par sphère gardée.

**Conclusion, robuste à tout l'intervalle : l'énumération locale exhaustive des
quadruples est condamnée**, quels que soient $\theta$, le nombre de cônes et la
qualité de la borne.

### 1.3 Le second mur : la descente de la forêt

`build_forest::descend` (`forest.cpp:69-89`) cherche un intrus en parcourant les
points du nuage, avec **sortie au premier intrus** (`:80`, `break`). Son coût
réel est donc l'**indice du premier intrus**, pas $n$ — un balayage complet n'a
lieu que lorsque les intrus se raréfient, c'est-à-dire près des minima et sur les
bras qui échouent.

**[diagnostic, pire cas, non qualifiant]** $\approx10^{12}$ prédicats par ordre.
Ce chiffre est en outre trop agrégé : les événements de rang $k+1$ se
**répartissent** entre les ordres, donc appliquer « 21 % du catalogue » à chacun
des dix ordres dépasserait le catalogue total. **[obligation]** publier
l'histogramme $S_k$, le maximum de la carte des minima, le nombre de bras, de
pas, de visites d'index, de plateaux et de censures **par ordre**, ainsi que la
distribution de l'indice du premier intrus — pas un produit de cardinaux.

Le remède, lui, est un énoncé propre :

> **[théorème] Lemme de localité de la descente.** Si $F'\subseteq F$ et si
> $(c,r)$ est la sphère de l'événement avec $F\subseteq\bar B(c,r)$, alors le
> centre $c'$ de la miniboule de $F'$ vérifie
> $c'\in\mathrm{conv}(F')\subseteq\bar B(c,r)$ et son rayon $r'\leq r$ ; donc tout
> $x\in\bar B(c',r')$ vérifie
> $\lVert x-c\rVert\leq r'+r\leq 2r$.

**Mais ce lemme ne vaut que pour le PREMIER pas.** Après un remplacement,
$F_{i+1}$ contient l'intrus et n'est plus inclus dans $F$. On a seulement
$F_{i+1}\subseteq\bar B(c_i,r_i)$, donc $c_{i+1}\in\bar B(c_i,r_i)$ et
$\bar B(c_{i+1},r_{i+1})\subseteq\bar B(c_i,2r_i)$ : l'enveloppe centrée en $c_0$
croît comme $R_{i+1}\leq R_i+r_0$, soit $R_i\leq(i+1)\,r_0$ — **linéaire en
nombre de pas, jamais bornée par $2r$**. Employer $\bar B(c,2r)$ pour toute la
chaîne serait une troncature non prouvée.

**La référence est donc la requête dans la miniboule COURANTE
$\bar B(c_i,r_i)$ à chaque pas** — une requête d'existence à sortie anticipée,
qui ne demande aucun lemme. Le lemme reste un bon accélérateur du premier pas.

**[obligation]** ni le lemme ni la requête ne prouvent la terminaison, l'unicité
du minimum atteint, le traitement des plateaux, le caractère canonique du choix
de la victime (`forest.cpp:84` prend `mb.support[0]`, un choix arbitraire), ni la
complétude des incidences silencieuses. L'identité byte-à-byte avec le balayage
exhaustif reste à établir.

### 1.4 Le troisième mur : la mémoire

**[diagnostic]** $\approx450$ à $510$ sphères critiques distinctes par point en
régime intérieur, stable de $n=5\,000$ à $200\,000$ — donc
$\approx2{,}3\cdot10^{7}$ sphères à $n=50\,000$, soit $\approx4{,}4$ Go à
$160+32$ octets. Ces chiffres viennent d'une source v2 dont la complétude, la
canonicité et le domaine ne sont pas certifiés : ils ne qualifient rien.

**Ce qui doit rester quel que soit le chiffre** : l'architecture v3 **ne doit
exiger aucune matérialisation globale du catalogue**. Le sink alimente des runs
bornés; le réducteur consomme leur fusion après fermeture de chaque groupe de
$\beta$ exactement égal. Les high-water du producteur, du merge, du lot, du
locator horizontal et de la couture verticale sont publiés séparément.

### 1.5 Trois quantités à ne jamais confondre

1. la **boule tangente non contrainte** — centre libre, c'est ce que calcule
   `radius_bound` de la v2, et c'est elle qui vaut $+\infty$ sur les points de
   faible profondeur ;
2. $R(p)$, **supremum** des rayons de boules passant par $p$, **de centre dans
   $\mathrm{conv}(X)$**, de contenu $\leq s_{\max}$ — celle de la germination ;
3. la **sphère critique**, qui exige de plus
   $c\in\mathrm{relint}\,\mathrm{conv}(U)$.

**[théorème]** $R(p)\leq\mathrm{diam}(X)$ **pour tout $p$** : si
$c\in\mathrm{conv}(X)$ et $p\in X$, alors $r=\lVert c-p\rVert\leq\mathrm{diam}(X)$.
**$R$ n'est donc jamais infinie.**

Deux conséquences, et elles sont importantes :

- une énorme boule vide posée sur une surface a tous ses appuis du même côté :
  son centre n'est pas dans leur enveloppe, **ce n'est pas une sphère critique**.
  L'inférence « $\tau=+\infty$ donc grandes sphères critiques » de la version
  précédente est fausse ;
- la restriction $D\leq2R(p)$ **ne peut jamais mordre par finitude**, seulement
  par la *valeur* de $R(p)$. Tout le cadrage « points peu profonds » est donc
  hors sujet pour la germination ; **la seule mesure qui compte est la
  distribution de $R(p)$** — **[obligation]**, elle n'a pas été faite.

Ce qui subsiste, et qui suffit à condamner la v2 : **sa** borne, non contrainte,
est infinie sur ces points, donc son voisinage ne peut pas être dimensionné.

**[diagnostic]** census du 8 août sur le Stanford bunny (dix captations brutes
recalées par `bun.conf`, 362 272 points ; et sa reconstruction fusionnée
`bun_zipper`, 35 947 points), critère **non contraint**, 4 096 directions,
`census_tukey_shallow_20260808.json` :

| nuage | $n$ | $s_{\max}=3$ | $s_{\max}=11$ |
| --- | ---: | ---: | ---: |
| bunny, 10 captations recalées | 50 000 | 1,06 % | 2,21 % |
| cube uniforme volumétrique | 50 000 | 0,48 % | 1,32 % |
| bunny, 10 captations recalées | 20 000 | 1,80 % | 3,88 % |
| bunny, reconstruction fusionnée | 20 000 | 5,86 % | 10,43 % |
| cube uniforme volumétrique | 20 000 | 0,95 % | 2,83 % |

Ce census mesure la quantité **non contrainte**, donc il diagnostique la v2 et
**rien d'autre** : la version précédente en tirait un majorant de l'ensemble où
la borne convexe échoue, ce qui est **vacue** puisque cet ensemble est vide. La
première observation sur un vrai nuage multi-captation est intéressante — le
nuage multi-captation est *moins* peu profond que la reconstruction fusionnée,
ce qui est cohérent avec l'épaisseur donnée par l'erreur de recalage — mais le
transfert à SemanticKITTI et aux familles sanctionnées reste **ouvert**, et la
causalité n'est pas démontrée.

### 1.6 Deux défauts v2 à ne pas perdre

- **`certified` est un faux positif** : `catalogue.cpp:471`, `:480`, `:484`
  mettent tous `ok = true`, y compris les deux sorties par épuisement, et `:500`
  écrit `certified[i] = 1`. Le reçu annonce 100 % de certification pendant que
  $\lvert W_p\rvert=n-1$. Trois drapeaux distincts sont nécessaires.
- **Trou de correction latent, non gardé** : `classify` compte le rang dans $W$
  seulement ; une boule de rayon $r>\rho/2$ verrait ses membres hors de $W_\rho$
  invisibles. **[mesuré]** inactif (0 sur 8 770), non gardé. Une ligne : rejeter
  toute sphère telle que $4\beta(s)>\rho^2$.

## 2. Le contrat, et son ledger

Hiérarchie de HARTIGAN **exacte** jusqu'à $K=10$ sur $n=50\,000$ points réels, en
moins d'une seconde, cible 100 ms.

Le budget ne se compte **pas** en nanosecondes par objet émis. Il se compte en
**ledger de bout en bout** : candidats rejetés, requêtes d'index, octets écrits,
tris, replis exacts, sink, réduction, synchronisations, high-water mémoire. Un
quotient sur la seule sortie masque exactement les postes qui tuent.

## 3. Domaine

### 3.1 Deux profils, jamais confondus

La spécification interprète chaque binary64 d'entrée comme un **dyadique exact**.
Quantifier sur une grille $2^{16}$ est un **autre problème** : la quantification
peut changer l'ordre des distances, le rang fermé, les égalités d'arêtes, les
cosphéricités et la topologie des lots simultanés. **Zéro collision ne prouve pas
l'équivalence géométrique.**

1. **`exact_dyadic_input`** — autorité sur les binary64 originaux, filtres
   d'intervalles dirigés, repli multiprécision ;
2. **`quantized_u16_input`** — autorité sur le nuage entier produit *seulement*,
   avec transformation, origine, échelle, écrêtages, collisions, multiplicités et
   digest source→cible au reçu.

Une sortie du second ne porte **jamais** `public_status=exact` relativement au
premier.

### 3.2 `RelevantGP` par référence, pas par raccourci

`RelevantGP` **n'est pas** « absence de coquille cosphérique ». Sa définition
normative (spécification §12) exige au moins : points distincts, support unique
et affinement indépendant, centre dans l'intérieur relatif, barycentriques non
nulles, shell complet sans point extérieur, prédicats et égalités exacts. Cette
proposition l'emploie **par référence** ; un balayage des événements acceptés ne
suffit pas à établir `relevant_gp_complete`.

### 3.3 Ce qu'on n'a pas le droit de supposer

Ni surface, ni volume, ni densité, ni forme étoilée, ni origine capteur, ni image
de distance. Certains nuages sont **recomposés de plusieurs captations** : il n'y
a ni paramétrisation commune, ni visibilité mono-retour. Ces structures peuvent
**proposer** du travail ; le complément exact doit rendre la même sortie sans
elles.

Corollaire de conception : **la correction est inconditionnelle, la sélectivité
est data-dépendante et doit être publiée par exécution.** Un nuage sur lequel les
coupes ne mordent pas doit produire un résultat *lent*, pas faux.

## 4. Le générateur : quatre objets

| | rôle | statut |
| --- | --- | --- |
| **A1-source** | source complète d'ancres diamétrales | **pièce ouverte** |
| **A2e** | peeling **2D** ancré par une **arête** | cœur algorithmique, complet *si* A1-source l'est |
| **A2p** | arrangement shallow **3D stratifié** ancré par un **point** | complet comme oracle exhaustif si projections et shells sont rejoués ; coût produit ouvert |
| **A2pe** | extraire des plans porteurs de A2p, puis exécuter A2e une fois par paire canonique | **candidat sous quatre obligations (§6)** |

### 4.1 A2e — la réduction de dimension

Ancre diamétrale $e=pq$, $d=q-p$, $D^2=d\cdot d$, $M=(p+q)/2$, deux vecteurs
entiers indépendants $b_1,b_2\perp d$, $B=[b_1\ b_2]$, centre $c=M+Bt$.

JUNG donne l'ellipse exacte
$J_e^{(4)}=\lbrace t: t^{\mathsf{T}}(B^{\mathsf{T}}B)t\leq D^2/8\rbrace$, et
chaque point $x\notin\lbrace p,q\rbrace$ la forme **affine**

$$h_x(t)=2(Bt)\cdot(x-M)-\left(\lVert x-M\rVert^2-\frac{D^2}{4}\right)=r^2-\lVert x-c\rVert^2.$$

Intérieur strict, shell et extérieur sont donc les **signes d'une droite** :
aucune base orthonormale, aucune racine carrée, tous les signes entiers ou
rationnels exacts.

Sur l'ellipse, chaque point est intérieur constant (compté dans $c_e$), extérieur
constant (éliminé), ou **droite active** ($m_e$). Si $c_e>s_{\max}-4$, l'ancre ne
porte aucun support quatre. Sinon, avec $\kappa_e=s_{\max}-4-c_e$ :

$$\text{\textbf{[théorème conditionnel]}}\qquad \mathrm{rang}_{\text{ferme}}(p,q,z,w)=4+c_e+\delta_e(t).$$

$\delta_e(t)$ est le nombre de demi-plans actifs strictement positifs au sommet
$t$. La formule exige `RelevantGP` et les deux seules droites porteuses du support
quatre ; une concurrence ou un extra-shell reconstruit d'abord le shell complet.
**Le rang est alors une profondeur d'arrangement 2D.** La preuve élémentaire de
[`RNG_JUNG_CLIQUES_ET_NIVEAUX_PEUPROFONDS.md`](../docs/math/RNG_JUNG_CLIQUES_ET_NIVEAUX_PEUPROFONDS.md)
donne, en position générale, la borne exacte

$$Z_e\leq m_e(\kappa_e+1),\qquad Z_e\leq 8m_e \text{ pour } s_{\max}=11,\ c_e=0 .$$

C'est le saut : la cascade développe des tuples *puis* interroge leur rang ; A2e
**calcule le rang pendant la génération**, ce qui retire la requête de boule
fermée du chemin chaud.

**[obligation] une borne de sortie n'est pas un constructeur.** Un prototype qui
forme toutes les intersections puis filtre reste en $\Theta(m_e^2)$ même s'il
respecte $Z_e$. Restent à fermer : coefficients rationnels larges, clipping
elliptique exact, droites parallèles, concurrences, ordre total et
reproductibilité, graphe de conflits, scratch borné, ordonnancement des ancres
lourdes, mapping GPU sans phase sérielle dominante.

**Les quatre arités sont quatre problèmes.** Support un : rayon nul,
multiplicités. Support deux : centre $M$, profondeur en $t=0$, shell complet
malgré tout. Support trois : circumcentre = point de $h_z=0$ minimisant
$t^{\mathsf{T}}(B^{\mathsf{T}}B)t$, rang $3+c_e+\delta_e$, **ellipse et seuils
différents**. Support quatre : sommets shallow. Au rang fermé 11, réfuter tout
support trois demande **neuf** témoins stricts, tout support quatre **huit** :
une preuve d'arité quatre ne se propage jamais à l'arité trois.

### 4.2 A1-source — la pièce ouverte, et pourquoi elle est dure

JUNG confine les centres **une fois la paire connue**. Il ne fournit pas
l'énumération sparse et complète des paires utiles. Trois voies :

- balayer $\binom{n}{2}$ — complet, incompatible avec le produit
  (**[mesuré]** $1{,}92\ \mu$s/paire, soit 2 400 s à 50 k) ;
- RNG ou catalogue de paires de rang borné — **[théorème négatif]** réfuté :
  pour tout $q$ fini il existe un support de rang fermé 11 dont le RNG d'ordre
  $q$ n'est pas une clique (`RNG_JUNG_CLIQUES_ET_NIVEAUX_PEUPROFONDS.md` th. 1) ;
- complément **fail-open par self-join du LBVH et center-cover**, voie
  `P15-HOCUDA-P1` — parcimonie et débit **non qualifiés**.

Donc : **complétude de A2e conditionnelle à A1-source ; parcimonie non prouvée.**
L'observation de 4,5 M paires retenues à une seule taille n'établit pas
$\Theta(n)$.

Et l'ellipse compacte de A2e ne supprime pas le problème des grandes sphères,
**elle le déplace** : un grand $D$ donne une grande ellipse, donc un grand $m_e$
(**[mesuré]** jusqu'à 25 026 points de voisinage sur `eight_clusters`). Ce qui
serait une cellule non bornée chez A2p devient une charge chez A2e, et elle
atterrit intégralement dans A1-source.

## 5. A2p — le dual inversif stratifié

Ancrons en $p$ ; pour tout autre $u$, poser

$$H_u=\left\lbrace c:\ 2\langle c-p,\ u-p\rangle=\lVert u-p\rVert^2\right\rbrace,$$

à coefficients **entiers** sur la grille. Alors $u$ est strictement dans la
sphère passant par $p$ centrée en $c$ ssi $c$ est du côté positif, donc

$$\mathrm{niveau}(c)=\#\lbrace u:\lVert u-c\rVert<\lVert p-c\rVert\rbrace,$$

Pour une strate relativement ouverte $F$ de dimension $j$, soient
$H_{u_1},\ldots,H_{u_{3-j}}$ ses hyperplans porteurs et
$U_F=\lbrace p,u_1,\ldots,u_{3-j}\rbrace$. La strate ne constitue pas à elle
seule un événement. Elle fournit **au plus un** centre candidat :

$$c_F=\mathrm{proj}_{\mathrm{aff}(F)}(p).$$

Il faut ensuite vérifier, exactement et dans cet ordre : $c_F\in F$ ;
$c_F\in\mathrm{relint}\,\mathrm{conv}(U_F)$ ; indépendance affine ; shell global
complet ; profondeur stricte ; rang fermé ; propriétaire canonique. Sous
`RelevantGP` et arrangement simple seulement,

$$\mathrm{rang}_{\text{ferme}}(c_F)=(4-j)+\mathrm{profondeur}(F).$$

En dégénérescence, le membre $4-j$ est remplacé par la cardinalité du shell
complet. Les quatre dimensions donnent respectivement : singleton $p$, milieu
d'une paire, circumcentre d'un triangle et sommet support quatre.

Le bon objet est le **sous-complexe stratifié et étiqueté** de l'arrangement des
$H_u$, jamais l'ensemble sous-jacent d'un unique $V_k(p)$. Le budget de profondeur
dépend de l'arité $q=4-j$ :

$$\mathrm{profondeur}(F)\leq s_{\max}-q.$$

À $s_{\max}=11$, les maxima sont 9, 8 et 7 pour les supports deux, trois et
quatre ; le singleton est injecté séparément. Un complexe uniforme de profondeur
au plus 10 est un sur-ensemble de travail, pas la sortie utile exacte.

**Complétude théorique.** Si l'arrangement stratifié complet est construit pour
chaque ancre, tout support critique contenant $p$ apparaît à la strate portant
ses égalités, puis passe les tests ci-dessus. Cette exhaustivité fait d'A2p un bon
oracle de recherche ; elle ne fournit ni constructeur sparse ni coût produit.

**Complexité ouverte.** Pour $m_p$ plans effectivement insérés, la borne
worst-case du préfixe shallow est $O(m_pK^2)$ par ancre, à laquelle s'ajoutent
construction, projections, conflits et transcripts. Sans arrêt certifié,
$m_p=n-1$ et la somme peut atteindre $O(n^2K^2)$. Les 450 à 510 sphères v2 et le
voisinage A2e de 175 points ne mesurent ni $m_p$, ni les strates A2p.

**Aucun certificat local n'est acquis.** Un point exposé de $\mathrm{conv}(X)$
possède des directions extérieures de profondeur zéro ; le sous-complexe shallow
y est non borné. Une coupure de carrier est sûre seulement si un majorant certifié
$R_{\mathrm{hi}}(p)$ prouve $\lVert u-p\rVert>2R_{\mathrm{hi}}(p)$. Sans un tel
majorant sélectif, le repli exact insère les $n-1$ plans. Les témoins de profondeur
ne sont jamais élagués par cette coupure.

**Le prix** reste un arrangement 3D, des prédicats plus lourds, des strates non
bornées et une duplication de travail pouvant atteindre quatre ancres par support.
Le propriétaire canonique réduit seulement l'émission.

## 6. A2pe — l'unification, et ce qu'elle demande

**Correction de formulation, imposée par l'audit 2 §2.1.** L'objet n'est **pas**
$V_k(p)$ vu comme un sous-ensemble de $\mathbb{R}^3$ : pris comme ensemble, il
efface les hyperplans intérieurs séparant deux cellules toutes deux de profondeur
$\leq k$. Contre-exemple minimal : $X=\lbrace p,u\rbrace$, $k=1$ donne
$V_1(p)=\mathbb{R}^3$, dont la frontière n'a aucune 2-face — alors que $H_u$
existe et que son milieu porte la sphère critique de support $\lbrace p,u\rbrace$.

L'objet correct est le **sous-complexe stratifié** : les faces de
l'**arrangement** des $H_u$ dont la profondeur est $\leq k$, chacune avec sa
dimension. Tout ce qui suit s'entend ainsi.

**L'observation.** Une 2-face $F$ portée par $H_u$ est une région relativement
ouverte de ce plan ; elle n'est pas le plan lui-même. Seule l'identité
$\mathrm{aff}(F)=H_u$ est vraie. La restriction de l'arrangement entier à
$H_u$ produit exactement les droites $H_u\cap H_v$ du §4.1. **A2e doit donc être
exécuté une fois sur le plan porteur canonique de la paire $(p,u)$, jamais une
fois par 2-face.**

**La conséquence conditionnelle.** Le sous-complexe stratifié A2p peut proposer
les plans porteurs rencontrés à faible profondeur, puis A2e peut construire leurs
niveaux 2D. Ce raccord ne supprime A1-source qu'après preuve que toute paire
diamétrale canonique utile apparaît parmi ces plans. Les faces supplémentaires
sont permises : projection, diamètre, Jung, shell et rang les filtrent.

Le théorème négatif du §4.2 ne se transporte pas directement. Pour un centre
$c\in H_u$, on a $r^2=\lVert c-M\rVert^2+D^2/4$. La boule diamétrale est incluse
dans la circumboule seulement si $\lVert c-M\rVert+D/2\leq r$ ; après élévation
au carré, cette condition force $c=M$ et $r=D/2$. Hors de ce cas, le rang de la
boule diamétrale ne décide pas celui de la circumboule.

**L'architecture candidate**, encore conditionnelle, est donc :

> ancrer en chaque point ; construire transitoirement le sous-complexe shallow
> stratifié ou une source équivalente de plans porteurs ; dédupliquer les paires
> canoniques ; exécuter A2e une fois par plan ; appliquer projection, shell, rang
> et owner exacts ; produire des runs bornés, les trier et les fusionner par niveau
> rationnel exact ; grouper tous les niveaux égaux avant réduction ; descendre par
> requête dans la miniboule **courante** à chaque remplacement.

**[obligation] — quatre, et elles conditionnent tout le §6.** Elles sont
étiquetées `PEL-*` et non `M*`, pour ne pas entrer en collision avec
l'obligation normative M.1 du registre.

1. **PEL-1 — complétude des plans porteurs.** Prouver l'inclusion « toute paire
   diamétrale canonique utile apparaît parmi les plans proposés ». L'égalité avec
   les 2-faces est fausse ; les plans superflus sont acceptables s'ils sont filtrés
   sans omission.
2. **PEL-2 — sensibilité à la sortie du parcours.** Établir un coût comprenant
   au moins un terme d'entrée, par exemple $O(m_p\,\mathrm{polylog}(m_p)+Z_p)$,
   puis publier plans, strates, projections, rejets et sorties. `O(sortie)` seul
   est impossible lorsque la sortie est vide.
3. **PEL-3 — strates non bornées.** L'énoncé « non bornée implique aucune sphère
   critique finie » est **faux**, déjà pour deux points : le plan médiateur non
   borné contient leur milieu critique. Il faut prouver finalisation, projection
   canonique et terminaison, ou déclarer le repli exhaustif.

**[obligation] PEL-4 — le coût du prédicat exact en 3D contre 2D.** C'est
l'arbitrage central entre A2pe et A2e seule : A2pe supprime A1-source au prix
d'un arrangement de dimension supérieure. Il faut le mesurer avant de trancher.

## 7. La coupe $R(z)\geq D/2$

> **[théorème]** $U$ support minimal bien centré, circumboule $\bar B(c,r)$ de
> contenu $\leq s_{\max}$, $D=\mathrm{diam}(U)$. Alors pour tout $z\in U$ :
> $R(z)\geq r\geq D/2$, avec $R$ le **supremum** des rayons de boules passant par
> $z$, de centre dans $\mathrm{conv}(X)$, de contenu $\leq s_{\max}$.

*Preuve.* $z\in U\subseteq\partial B$ donc $\bar B$ est une telle boule, de centre
dans $\mathrm{relint}\,\mathrm{conv}(U)\subseteq\mathrm{conv}(X)$ : $R(z)\geq r$.
Et $U\subseteq\bar B$ donne $D\leq2r$. $\square$

Trois précisions :

1. $R$ est un **supremum** : avec des boules fermées la population saute quand un
   point atteint le shell, la borne peut ne pas être atteinte ;
2. ce $R$ n'est **pas** la quantité tangente non contrainte du §1.5, et il est
   **toujours fini** — la coupe ne mord que par sa valeur ;
3. **la coupe ne peut PAS filtrer la profondeur.** Le lemme est une condition
   nécessaire pour être **porteur**. Il ne dit rien d'un point $y\notin U$ situé à
   l'*intérieur* de la sphère : sa forme affine $h_y$ peut rester strictement
   positive au centre candidat et **doit** alors compter dans $c_e$, dans
   $\delta_e$ et dans le rang fermé. Élaguer $y$ du range-report sous-compterait
   le rang et publierait des sphères de rang $>K$.

Il faut donc **deux flux explicitement distincts** :

| flux | contenu | filtre autorisé |
| --- | --- | --- |
| **témoin / profondeur** | toutes les formes susceptibles de compter comme intérieures | **aucun** |
| **`carrier_eligible`** | tout point non exclu par une preuve sûre | retirer seulement si un majorant certifié prouve $2R_{\mathrm{hi}}(z)<D$ |

Les emplois **licites** de la coupe sont donc : (i) la sélection des **ancres**,
où les deux extrémités sont porteuses ; (ii) le masque `carrier_eligible` sur les
droites, pieds et intersections. L'emploi **interdit** est le flux témoin.

Si un agrégat d'index est employé pour (i) et (ii), il doit s'appeler
`max_two_R_upper_hi` — **pas** `max_tau_hi`, qui réintroduirait la confusion
$\tau$/$R$ du §1.5 — être défini comme intervalle, et rester **fail-open** sur
non fini, sous-normal, débordement ou intervalle traversant le seuil.

Dans A2pe, la coupe sert surtout à **séparer les régimes** (petites ancres en
flux, grandes ancres en file), pas à élaguer.

## 8. Canonicité, égalités, arithmétique

**Propriétaire canonique** : la plus petite paire lexicographique parmi les arêtes
de longueur **maximale** du support, sur des `PointId` stables, testée **avant**
le sink. Sur une grille entière l'égalité de deux longueurs d'arête n'est pas
rare : la règle de départage est obligatoire.

**Égalités** : une concurrence exacte de $t$ droites est **un lot unique**, pas
$\binom{t}{2}$ sommets perturbés. Shell complet compté avant émission ; hors
`RelevantGP`, l'autorité est retirée ou une voie dégénérée explicitement
certifiée est prise. Sortie exigée byte-à-byte identique sous permutation
d'entrée, nombre de fils et ordonnancement GPU. **[mesuré]** en v2, $55\ \%$ des
sphères sont conservées chez un propriétaire non canonique même en mono-thread,
et aucun digest reproductible n'est possible.

**Arithmétique de production** : entière, sans allocation, largeur auditée.
**[extrapolé, dérivation symbolique à reprendre au registre]** majorants pour
$M=65535$ : $\lvert\mathrm{num}_i\rvert<2^{84{,}96}$, $\mathrm{den}<2^{68{,}17}$,
$\mathrm{num2}<2^{169{,}93}$, $\mathrm{den}^2<2^{136{,}35}$, comparaison de deux
niveaux $<2^{306{,}28}$. `sphere.hpp` de la v2 y est compatible
(`BigInt<4>`/`BigInt<6>`) et **[mesuré]** son `sphere_cmp_beta` est d'accord avec
GMP sur 18 601 paires — c'est un **candidat à reprendre après audit prédicat par
prédicat**, pas un composant qualifié.

Ces majorants ne concernent que **`quantized_u16_input`**. Ils ne dimensionnent
pas `exact_dyadic_input`, dont les exposants binary64 et les produits
intermédiaires exigent leur propre dérivation, leur propre repli et un SLO
séparé. **[obligation]**

**Oracle** : précision **arbitraire**, représentation *différente* de la
production. **[mesuré]** elle décide la grille déclarée là où l'`i128` décide
$0/40$ nuages. Ce n'est pas une exclusivité mathématique — une largeur fixe
indépendante suffisamment grande déciderait aussi un domaine entier borné ; c'est
le choix **robuste**, parce qu'il supprime la question de la largeur au lieu de
la re-prouver à chaque site.

## 9. Ce que la v2 fournit, et ce qu'elle ne fournit pas

Elle fournit une **sémantique candidate** (lots exacts, multifusion contractée,
censure atomique, source exacte par nœud), des **fixtures** et des
**contre-exemples**. Elle ne fournit **aucune certification** : l'oracle qui a
produit « 1 462 nuages, 89 247 cas » déborde en arithmétique signée avant son
garde, saute des nuages en silence, accepte une campagne vide ou censurée, ne
compare ni arités, ni enfants, ni racines, ni sources, et tire ses coordonnées
dans $[0,120]$ — **[mesuré]** sur la grille déclarée il décide zéro nuage sur
quarante en annonçant `OK`.

## 10. Architecture cible, non encore implémentée

Le live possède un endpoint CPU de reverse search qui décide le parcours sans
`seen` ni `frontier`, reprend son curseur et rend le même ensemble que le BFS
dans les portes bornées. Après le voisin aller, le préfiltre puis `decide_child`
suppriment toute requête de voisin retour. L'API sink mesure les identifiants des
sommets du chemin et distingue l'arrêt demandé; le wrapper différentiel
matérialise encore la sortie et le catalogue continue d'appeler le BFS. Le
high-water mémoire complet, l'intégration produit, la transaction du préfixe et
la forme device ne sont donc pas acquis.

Le propriétaire local est branché à titre expérimental sur la récolte naviguée.
Les chemins sans sommet propriétaire utilisent un repli exact. Une sonde
indépendante de 174 444 exécutions valide statuts et payloads complets dans les
quatre quadrants; les cinq portes permanentes donnent aussi zéro désaccord et
une table owner-indexée vide sur leurs coordonnées basses. Elles ratent le P0
u16 où `orient3d_exact` est tronqué avant `tangent_sign`, avec pertes et doublon
de sphères. La fixture de domaine compare désormais statut,
support, arité, rang, niveau et membres; l'équivariance conserve membres et
multiplicités, le cône signé et le cube sont permanents. Mais le test du cône ne
compare pas l'identité du sommet owner et laisse passer le mutant non signé.
Sans index il reste $O(n)$ clefs singleton; sur la voie directe la table peut
rester en $\Theta(\text{sortie})$. Le high-water live compte les entrées de
`emitted`, pas les octets résidents. Le coût owner reste
$\Theta(m^4+m^3\lvert B_U\rvert)$ dans le code; le réducteur exact en un scan
est prouvé mais non intégré, et le harvest total d'arité trois reste ouvert.

Le relevé live mesure maintenant coquille et intérieur au point d'admission et
porte la capacité intérieure à 32, cohérente avec la borne contractuelle 30. La
campagne `smax=19` passe donc avec un intérieur 17/32. Le nouveau microkernel
exerce aussi 27 refus de flats. Mais le harness fait `continue` avant l'oracle
sur chacun d'eux : il prouve l'activation du statut, pas le replay ni la
conservation. Un mutant qui refuse tous les sommets reste vert. Les refus
d'admission sont, eux, omis du batch et du compteur.

Sept points cosphériques génériques portent déjà 35 flats contre la capacité
32. Une voie industrielle doit réduire les flats par pages ou rejouer le sommet
entier, avec un ledger par raison et l'égalité du parcours CPU avec l'union
device+replay. `neighbour_along` reste CPU; le `.cu` ne produit ni voisins ni
verdicts de parent. Le kernel cible doit fermer le minimum global du pinceau et
le lot complet des ex æquo, partitionner la reverse-search en tâches disjointes
et stager des runs exacts; aucune de ces opérations n'est reçue par le live.

Le noyau F0 matérialise volontairement Warshall et DSU pour juger un lot borné.
Son contrat de naissance est actuellement contredit par une garde commune aux
deux chemins; il reste un falsificateur en réparation, pas la réduction
horizontale de production. De même, `mhgp3v_first_incidence` emploie
délibérément `flat_catalogue(pts,n)`, des maps et une vérité combinatoire : c'est
un oracle borné, pas l'étage de production décrit ci-dessous.

**J10 borne un rayon spatial, pas une cardinalité** — **[mesuré]** voisinage
maximal 25 026 points sur `eight_clusters`. Aucune tuile ne peut donc supposer
« quelques dizaines de points en mémoire partagée ».

| étage résident | rôle | structure globale évitée |
| --- | --- | --- |
| canonicalisation | `PointId`, domaine, digest, `RelevantGP` | copie ambiguë de l'entrée |
| LBVH | range-report, self-join, `max_two_R_upper_hi` fail-open | matrice paire–point |
| reverse search | parent, enfants, propriétaire local, pile active sans borne produit démontrée | `seen/frontier/visited` globaux |
| source directe ouverte | cofaces Gabriel et facettes du cœur streamées | catalogue critique global; le prototype exhaustif n'est que l'oracle |
| première incidence | census fermé $E_F$, census strict $J_F$ saturé, attache propriétaire, descente locale vers $R_F\in D_k$, puis `find` pré-lot | $\mathrm{Star}(D_k)$, locator non-cœur et matérialisation de tous les $M(F)$ sous la porte régulière |
| **tri et lots** | ordre global par niveau exact, groupement des égaux | catalogue géométrique global |
| réduction horizontale | fold de partition, locator externalisable, forêt append-only | snapshots complets et mosaïque d'ordre supérieur |
| couverture | DAG ou journal exact de provenance | matérialisation répétée des unions de points |
| jointure verticale | requêtes vers l'ordre adjacent et sweep à coupe fermée | matrice tous ordres contre tous ordres |

**Le flux ne dispense pas du tri global exact.** Des producteurs indépendants
produisent les événements dans un ordre arbitraire, alors que le réducteur exige,
par ordre : un ordre global par **niveau rationnel exact**, le groupement de
**tous** les événements de niveau égal, un état pré-lot logiquement gelé, puis
une application atomique — et déduplication, propriétaire, incidences silencieuses et
`coverage_delta` doivent respecter ce même ordre. Aucun producteur monotone entre
sous-arbres indépendants n'est énoncé ni plausible.

La voie compatible avec l'invariant mémoire du §1.4 est un **tri externe** :
produire des **runs bornés** triés par clé exacte et identifiants canoniques, les
fusionner par un merge déterministe, **réunir les clés rationnellement égales
avant toute mutation**, puis alimenter le réducteur. À publier : taille des runs,
high-water RAM, octets écrits et lus, coût des comparaisons exactes, nombre et
taille maximale des lots. Une variante entièrement résidente est acceptable si
elle prouve les mêmes bornes. Le produit visé évite le catalogue géométrique
global; le falsificateur `first_incidence` le matérialise encore volontairement.
On ne peut pas éviter l'**ordonnancement global des événements utiles**.

**Les deux pistes sont parallèles** : oracle CPU multiprécision indépendant pour
l'autorité ; CUDA `proposal_only` très tôt pour falsifier masses, divergence et
mémoire ; aucune promotion avant parité.

## 11. Le contrat du générateur n'est pas le contrat HGP

Émettre les supports est un composant géométrique. La décision locale de
$\lambda(F)$ et $M(F)$ est maintenant démontrée sous une source Gabriel ouverte
terminale; sous la porte régulière, un census saturé et une attache résolue par
facette cœur suffisent même au quotient $H_0$. Restent des points durs distincts
du générateur : produire cette source sans catalogue global, authentifier la
régularité, fermer les lots et les déduplications, certifier les descentes vers
le cœur puis résoudre leurs handles dans la partition pré-lot, conserver
`coverage_delta` et `coverage_log`, puis construire les applications verticales
et leurs carrés de naturalité.

Le contrat v2 exige en outre des identités exhaustives de facettes, cofaces,
groupes et provenance. Elles peuvent être streamées, mais pas supprimées comme
une optimisation interne. Un quotient $H_0$ normalisé plus compact demanderait
un profil public versionné, sa propre preuve de complétude, ses verticales et ses
digests. Le streaming décrit ici n'est pas encore implémenté par le prototype en
mémoire. La factorisation précise est dans
[`NOTE_GATE_D_GLOBALITES_RESIDUELLES.md`](audits/NOTE_GATE_D_GLOBALITES_RESIDUELLES.md).

## 12. Portes

**Gate A — autorité documentaire.** Reprendre A2e, A2p/A2pe stratifiés, le
dictionnaire strate--projection--support, le lemme $2r$, la coupe
$R(z)\geq D/2$, $R\leq\mathrm{diam}(X)$, la fixture à deux points et les profils
d'entrée dans la spécification et le registre ; classer chaque énoncé
`proved_here`, `conditional_theorem`, `proof_obligation`, `experimental_target`
ou `false_in_general`.

**Gate B — reçus.** Sidecars complets pour chaque mesure de ce document ; digests
des entrées, binaires et sorties ; identité de campagne fermée ; distinction
stricte mesure / extrapolation / théorème. Le census bunny devient une fixture
reproductible, puis SemanticKITTI et les familles sanctionnées.

**Gate C — oracle indépendant.** Multiprécision, représentation distincte,
exhaustif à petit $n$, campagne fermée
(`attempted = decided + rejected_domain`), lecture hostile fail-fast et
comparaison structurelle complète : arités, enfants, racines, coface source
réellement contributrice et canonique, nombre canonique de nœuds, censure et
diagnostics publics.

**Gate D — census de charge.** Pour la voie retenue : sommets shallow, flats,
enfants et profondeur; événements propriétaires; incidences directes et
silencieuses; niveaux et lot égal maximal; carriers/racines actifs; locator,
partition et couverture; nœuds et enfants; requêtes verticales; runs, octets et
high-water par étage. Les anciens compteurs $R(p)$, A2e et A2p ne concernent que
les voies abandonnées et ne qualifient pas le parcours courant. **À $n$ de
plusieurs milliers** — ce n'est pas la même campagne que Gate C, et l'une ne
tient pas lieu de l'autre.
**Décision à deux branches** : sortie énorme ⇒ réviser le SLO ; sortie sparse
mais intermédiaires denses ⇒ **architecture no-go**.

**État live de Gate D : NO-GO.** Le census borné des flats et le nouveau
high-water d'entrées sont positifs sur leurs campagnes. Mais owner tronque un
déterminant exact sur le profil u16, sa porte ne protège pas l'identité signée,
et F0 contredit sa sémantique de naissance. Le census global possède maintenant
une construction 4D exacte, mais aucune capability terminale reçue. Les
compteurs verts ne compensent pas ces écarts.

**Gate E — shallow CPU exact.** Constructeur sans travail en $\sum_e m_e^2$ ;
comparaison à l'oracle exhaustif et au brute-force local ; permutations,
égalités, parallèles, concurrences, limites de l'ellipse ; transcript de conflits
et sortie canonique. Obligations propres à A2e (couverture des ancres,
classification exacte, préfixe shallow, clipping, profondeur et conflits,
concurrences groupées, arités deux à quatre, shell/centrage/owner, transcript) —
et, **séparément**, obligations propres à A2p (dictionnaire, atteignabilité, coût
global sur $n$ ancres, cellules non bornées, duplication). Les deux listes ne
s'impliquent pas.

**Gate F — descente indexée.** La boule $\bar B(c,2r)$ ne certifie que la première
recherche issue de la facette initiale. Chaque remplacement interroge la
miniboule **courante**, sauf nouvelle preuve d'enveloppe globale. Différentiel
contre balayage exhaustif, terminaison, décroissance stricte, victime canonique,
plateaux et compteurs **par ordre** sont obligatoires.

**Gate G — source HGP et réduction.** Fixer `hgp_reduced` ou `full_pi0`, produire
facettes, cofaces, incidences actives et silencieuses, attaches, M.1 si le profil
l'exige, `coverage_delta`, `coverage_log`, runs triés, lots atomiques et
verticales ; comparer le transcript complet à l'oracle.

**Gate H — publication, déterminisme et produit sans budget configuré.** Exiger
reçus complets, sorties byte-à-byte sous permutations et ordonnancements, aucune
censure silencieuse et aucune structure globale interdite ; seulement ensuite
qualifier 100 ms, **par famille sanctionnée**.

## 13. Ordre des travaux

1. **Largeur owner** : réduire explicitement chaque `orient3d_exact` par
   `sign_of` avant `tangent_sign`; rendre permanents 1023/1024/1025 et
   1290/1291, UBSan, `-Wconversion` et le mutant de troncature.
2. **F0 source-agnostique** : accepter le carré direct tout $N_a$, retirer la
   garde corrélée et construire la vérité par énumération de partitions depuis
   le `RawBatch`.
3. **Capability régulière séparée** : vérifier chaque `DirectHyperedge` brute,
   ses niveaux de facettes et son reçu; tuer le mutant de validation par
   composante.
4. **Identité owner** : tester directement les deux sommets du cône signé et
   tuer $\varepsilon=-1\mapsto+1$; graver aussi la vérité 19 du cas coplanaire.
5. **Coût owner** : intégrer le réducteur `FULL/HALF/LINE/RAY/WEDGE/ZERO`, le
   comparer à l'oracle exhaustif de rayons, puis mesurer les grandes coquilles.
6. **Census terminal** : implémenter le halfspace-report 4D exact avec statuts
   cappés et scan `cpp_int` borné comme vérité; trier les sphères rationnelles
   avant census.
7. **Flux transactionnel** : intégrer sink et propriétaire au catalogue,
   documenter le repli direct et publier les octets high-water de tous les
   conteneurs, pas seulement le nombre d'entrées de `emitted`.
8. **F1/F2 et source HGP** : runs scellés, versions externes, couverture,
   verticales et reçus complets contre l'oracle indépendant.
9. Puis seulement données réelles à grande échelle, GPU et publication.

**GO immédiat** : preuves, contre-exemples, fixtures CPU bornées et reçus des
points 1 à 4. **NO-GO immédiat** : optimisation GPU avant leur fermeture, toute
revendication `exact`, tout SLO, toute autorité publique, ou présenter une
campagne finie comme une preuve universelle.

## 14. Journal des affirmations retirées

| affirmation | sort |
| --- | --- |
| facteur **100** entre travail et sortie | **artefact** d'une récolte défaillante ; le rapport réel est $\approx17$ (§0 ter) |
| transport du niveau « en $\pm1$ » | faux, et « $-1$, $0$ ou $+1$ » l'est aussi : la variation vaut $\lvert D_-\rvert-\lvert A_{\text{int}}\rvert$, non bornée par un (§0 ter) |
| coupe du parcours sur le rang fermé | faux : supprime des sommets de niveau zéro ; couper sur le **niveau strict** (§0 ter) |
| « même objet » et nombre de sommets identique à la mosaïque d'ordre supérieur | **faux** : l'arrangement et la mosaïque sont duaux et leurs dimensions sont échangées; le tétraèdre donne un sommet d'arrangement contre quatre sommets Delaunay (§0 quater a) |
| taille du $\leq k$-niveau tenue pour linéaire | régime de surface mesuré, non garanti : Clarkson--Shor donne $O(n^2k^2)$ sous position générale (§0 quater b) |
| « complétude A1 = théorème » | conditionnelle à A1-source (§4.2) |
| $\tau=+\infty\Rightarrow$ grandes sphères critiques | faux (§1.5) |
| « surface ⇒ faible profondeur presque partout » | census, pas théorème (§1.5) |
| census = majorant de l'échec de $R$ convexe | **vacu** : cet ensemble est vide (§1.5) |
| A1 contre A2, dichotomie | quatre objets (§4) |
| gain $9{,}5\cdot10^9\to1{,}2\cdot10^8$ | invalide sans range-report indexé (§7) |
| « J10 rend la tuile GPU sûre » | borne spatiale, 25 026 points mesurés (§10) |
| « le point dur n'est ni le générateur ni la forêt » | retiré (§1.3, §11) |
| forêt v2 et O2 « certifiés » | sémantique candidate (§9) |
| $267$ ns par objet | ledger de bout en bout (§2) |
| grille $2^{16}$ comme domaine | deux profils disjoints (§3.1) |
| `RelevantGP` = absence de cosphéricité | définition normative par référence (§3.2) |
| 4,5 M paires $\Rightarrow\Theta(n)$ | retiré (§4.2) |
| « GPU dès la première ligne » vs « pas de CUDA avant » | deux pistes parallèles (§10) |
| catalogue matérialisable | flux obligatoire ; 4,4 Go en diagnostic (§1.4) |
| 69 ans / 91 s | intervalle de débit publié : 77–215 ans, 103–286 s (§1.2) |
| $\lvert W\rvert\geq130$ « géométriquement nécessaire » | observé sur le corpus mesuré (§1.2) |
| `std::map` sur $2{,}3\cdot10^{7}$ clés | carte **par ordre**, rang $=k$ seulement (§1.3) |
| $10^{12}$ prédicats de descente | pire cas ; `descend` sort au premier intrus (§1.3) |
| « `sphere.hpp` sain » | candidat, audit prédicat par prédicat à faire (§8) |
| M1–M5 imposées à A2e | deux listes disjointes (§12, Gate E) |
| « l'audit est l'autorité » | spécification + registre des preuves (en-tête) |
| « les 2-faces de $V_k(p)$ » | sous-complexe stratifié ; une face $F$ vérifie seulement $\mathrm{aff}(F)=H_u$, et A2e s'exécute une fois sur $H_u$ (§5–6) |
| certificat de localité M2.1 « $2r_{\max}\leq d_{M+1}$ » | **faux** : un support inconnu employant un point exclu vérifie $2r\geq d_{M+1}$ ; réfuté par le juge, support $\lbrace6,10\rbrace$ manquant (§12) |
| $\lvert W\rvert$ « certifié » 128/256 à $n=200$–$1000$ | retiré ; mesure honnête = **fenêtre suffisante** en régime exhaustif (§12) |
| élaguer le range-report par $2R<D$ | **incorrect** : perd les témoins de profondeur (§7) |
| `max_tau_hi` | `max_two_R_upper_hi` : $\tau\neq R$ (§7) |
| $\bar B(c,2r)$ pour toute la descente | premier pas seulement ; enveloppe $\leq(i+1)r_0$ (§1.3) |
| « consommé au fil de l'eau » | tri externe et groupement des niveaux égaux (§10) |
| $\mathrm{rang}=1+\mathrm{niveau}$ | $(4-j)+\mathrm{profondeur}$ (§5) |
| PEL-3 « non bornée ⇒ pas de sphère finie » | **faux** dès deux points ; reste la terminaison des strates non bornées (§5–6) |
| M1–M5 | `PEL-1` à `PEL-4`, pour ne pas heurter M.1 du registre (§6) |
| $\lvert W\rvert\leq39$ à une seconde | 38 au meilleur débit publié (§1.2) |
| $n\binom{\overline{\lvert W\rvert}}{3}$ | sous-estime $\sum_p\binom{\lvert W_p\rvert}{3}$ par convexité (§1.2) |
| « 21 % de 23 M par ordre » | les rangs $k+1$ se répartissent entre ordres (§1.3) |
| largeurs prouvées = arithmétique du produit | valent pour `quantized_u16_input` seul (§8) |
| multiprécision « seule option » | choix robuste, pas exclusivité (§8) |
| certificat de localité $V^{(M)}\subseteq B(p,d_{M+1}/2)$ **fermé** | faux à l'égalité ; il faut la marge STRICTE, témoin $u=2c-p$ |
| épluchage en couches convexes pour le préfixe shallow | **réfuté**, fixture permanente `convex_layer_refutation` |
| produits croisés de 210 bits pour trier le pinceau | évitables : chirotope $3\times3$ sous $2^{107,4}$ |
| A1-source comme verrou du générateur | **caduc** : le générateur retenu n'a pas d'ancres (§0 ter) |
| cosphéricité = rejet de domaine | **traitée** : le sommet porte sa coquille entière (§0 ter) |
| « le clipping de Jung débloque l'échelle » | grand facteur constant, pas un ordre ; $m_e\approx0{,}45\,n$ |
