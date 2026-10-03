# De la tour FULL à une hiérarchie laminaire de points

3 octobre 2026 (Claude, développeur). Cadre :
`exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`.
Demande de l'utilisateur : « comment passer de la tour full à une hiérarchie laminaire sur les points ? Il faut à
la fois une solution satisfaisante mathématiquement, mais aussi que cela marche sur au moins un des exemples de
Zoltan/ où la hiérarchie HDBSCAN échoue », puis « privilégie l'aspect mathématique », relire à titre indicatif les
parties I–II du manuscrit, et « sois critique vis à vis de la thèse comme vis à vis de l'auditeur ».

Ce document fixe une règle, ses preuves et ses limites. Il complète le § 7 de [MATHEMATIQUES.md](MATHEMATIQUES.md)
(P1–P5) sans le modifier. Code : [`bench/points_hierarchy.py`](../bench/points_hierarchy.py) (règle et
évaluation), [`bench/points_export.cpp`](../bench/points_export.cpp) (export natif des incidences fortes de P3),
[`bench/points_reference.py`](../bench/points_reference.py) (oracle de la définition),
[`bench/points_gate.py`](../bench/points_gate.py) (porte natif contre oracle).

## 1. L'objet n'est pas une partition

Pour un ordre $k$ et un niveau $a$ (rayon carré), une composante $C$ de $L_k(a)$ définit l'amas discret de la
définition 8 de la thèse, $E_C(a)=\lbrace x\in X : d(x,C)^{2}\leq a\rbrace$ ; le théorème 2 l'identifie au
$k$-polyèdre de $\Gamma_k$. Pour $k\geq 2$ ces amas se recouvrent (thèse, § 6.1, deux triangles) et la thèse
l'assume : « Nous ne chercherons pas ici à forcer une partition à chaque niveau » (§ 6, introduction) ;
« l'objet naturel n'est pas une partition de X, mais un recouvrement de X (ou bien une partition des
$(K-1)$-simplexes) » (§ 9.1). Toute hiérarchie laminaire de points est donc une **approximation** de cette
hiérarchie de recouvrements. Deux directions sont canoniques :

- **intérieure** : chaque bloc de points est inclus dans un amas discret et deux blocs ne se réunissent qu'à la
  fusion FULL de leurs composantes ;
- **extérieure** : on réunit tout ce qu'un même amas couvre, quitte à réunir avant FULL (fermeture de la
  co-couverture, proposition de l'auditeur).

Le critère de la thèse les départage. Au chapitre 7, la performance d'une méthode persistante est la fraction
d'un amas dense récupérée **avant la fusion parasite** (théorème 3), avec la sémantique de couverture
$\Theta^{poly}$ : le point compte s'il est à distance au plus $r$ de la composante géante. Une approximation
intérieure ne crée aucune fusion parasite que FULL ne crée pas ; une approximation extérieure peut en créer.

## 2. Pendaisons fidèles

Notons $T_k$ l'arbre de fusion de FULL à l'ordre $k$, vu comme espace : un point est un couple $(\nu, a)$ avec
$h(\nu)\leq a< h(\mathrm{parent}(\nu))$. La remontée $\mathrm{Up}(\nu,a)$ suit les ancêtres. Pour deux points,
$m(p,q)$ est le premier niveau où leurs remontées se rejoignent ; c'est une ultramétrique de rencontre :
$m(p,r)\leq\max(m(p,q),m(q,r))$. La **région de couverture** du site $x_i$ est
$R_i=\lbrace(\nu,a) : x_i\in E_\nu(a)\rbrace$ ; elle est stable par remontée (une composante couvre au moins ce
que couvrent ses descendants). Par P3, $R_i$ est la réunion des remontées issues des incidences fortes
$(\lambda_b,\nu_b)$ des boules qui contiennent $x_i$ : c'est ce qu'exporte `points_export.cpp`.

Une **pendaison fidèle** choisit pour chaque site un point $p_i=(o_i,e_i)\in R_i$ : le site entre à $e_i$ dans le
nœud $o_i$, puis suit ses ancêtres. Le bloc de $\nu$ au niveau $a$ est l'ensemble des sites entrés au plus tard à
$a$ et pendus dans le sous-arbre de $\nu$.

**F1 — laminarité, fidélité.** Les blocs forment une hiérarchie laminaire ; la hauteur de réunion de deux sites
est $u(i,j)=m(p_i,p_j)$, au moins le niveau de fusion FULL de leurs propriétaires ; chaque bloc est inclus dans
l'amas discret de son nœud. *Preuve.* P4 pour la laminarité ; $p_i\in R_i$ et la stabilité de $R_i$ par remontée
pour l'inclusion. □

Core (P1) et la première couverture (P2, avec LCA des ex æquo) sont des pendaisons fidèles. La fermeture
qualifiée n'en est pas une : sur les cinq points de l'auditeur à $k=2$, elle réunit tout à $100/9$ alors que FULL
ne fusionne qu'à $36$.

**F2 — trilemme.** Aucune pendaison fidèle ne peut à la fois faire entrer chaque site à sa première couverture et
dépendre continûment des positions. *Preuve.* Sur $\lbrace 0,2,4\rbrace$ à $k=2$, le site médian est couvert au
niveau $1$ par deux composantes qui ne fusionnent qu'à $4$ ; une entrée à $1$ doit choisir l'une d'elles, et le
déplacement $4\mapsto 4+\delta$ (ou $0\mapsto -\delta$) impose l'une ou l'autre : $u$ saute de $3$ (P5). □

Il faut donc retarder. La question devient : de combien, au minimum, pour rester continu.

## 3. La règle $H_m$ : pendaison à marge

**Qualification.** Une composante est *qualifiée* au niveau $a$ si son amas discret compte au moins $m$ sites.
On note $R_i^{(m)}$ les points qualifiés de $R_i$ (encore stable par remontée).

**Définition.** Soit $t_i$ le niveau le plus bas de $R_i^{(m)}$ et $p_i$ un point de $R_i^{(m)}$ à ce niveau.
On pose

$$ D_i=\sup_{q\in R_i^{(m)}}\left(m(p_i,q)-h(q)\right),\qquad e_i=t_i+D_i,\qquad o_i=\text{ancêtre de }p_i\text{ vivant à }e_i. $$

$D_i$ est la plus longue durée pendant laquelle une composante qualifiée **rivale** couvre $x_i$ avant de
rejoindre la lignée de sa première couverture. Sans rival, $D_i=0$ et le site entre à sa première couverture
qualifiée. Avec un rival ex æquo, $D_i$ va jusqu'à la fusion : le site attend que l'ambiguïté soit levée par
l'arbre lui-même.

**H1 — bonne définition.** $e_i$ et $o_i$ ne dépendent pas du choix de $p_i$. *Preuve.* Si $p_i'$ est un autre
point le plus bas, le terme $q=p_i'$ donne $m(p_i,p_i')\leq e_i$ ; par l'inégalité ultramétrique, pour tout $q$,
$m(p_i',q)-h(q)\leq\max(m(p_i',p_i),m(p_i,q))-h(q)\leq\max(e_i-t_i,D_i)$ car $h(q)\geq t_i$ ; les deux remontées
sont confondues à $e_i$. □

**H2 — propriétés.** $H_m$ est une pendaison fidèle (F1), équivariante (aucun départage par identifiant ni par
repère : renuméroter ou appliquer une isométrie transporte tout), et sans retard sur les amas non ambigus : si
aucun site de $E_C(a)$ n'est couvert, avant de rejoindre $C$, par une composante qualifiée hors du sous-arbre de
$C$, alors le bloc de $C$ à $a$ est exactement l'ensemble des sites de $E_C(a)$ qualifiés. À $k=1$ et $m=1$,
$H_m$ est la liaison simple (chaque site est sa propre composante, aucun rival). *Preuve.* Fidélité et
équivariance par construction ; pour l'exactitude, chaque terme de $D_i$ provient d'un point de la lignée de $C$,
donc $e_i\leq a$ ; à $k=1$, $E_C$ est le cœur et $R_i$ est la remontée du singleton. □

**H3 — stabilité.** Soient deux nuages appariés dont les arbres FULL sont $\delta$-entrelacés en niveau, avec
transport des couvertures (P5). Alors $|e_i^{X}-e_i^{Y}|\leq 3\delta$ et $|u^{X}(i,j)-u^{Y}(i,j)|\leq 5\delta$.
Avec P5 et des niveaux au plus $\Lambda$ (niveau de la racine), $\delta=2\varepsilon\sqrt{\Lambda}+\varepsilon^{2}$.

*Preuve.* Notons $\varphi,\psi$ les cartes d'entrelacement : elles décalent les niveaux de $\delta$, commutent aux
remontées, envoient $R_i^{(m),X}$ dans $R_i^{(m),Y}$ et réciproquement (P5 transporte les mêmes identifiants
couverts : un cardinal de couverture ne peut que croître, la qualification est donc transportée exactement), et
$m^{Y}(p,q)\leq m^{X}(\psi p,\psi q)+\delta$. Pour $q'\in R_i^{(m),Y}$, l'inégalité ultramétrique via $p_i^{X}$
et la définition de $D_i^{X}$ appliquée à $\psi p_i^{Y}$ et $\psi q'$ donnent
$m^{X}(\psi p_i^{Y},\psi q')\leq D_i^{X}+h(q')+\delta$, d'où $m^{Y}(p_i^{Y},q')-h(q')\leq D_i^{X}+2\delta$ :
$D_i^{Y}\leq D_i^{X}+2\delta$. Comme $t_i^{Y}\leq t_i^{X}+\delta$, $e_i^{Y}\leq e_i^{X}+3\delta$. Pour les
paires : $m^{Y}(p_i^{Y},\varphi p_i^{X})\leq e_i^{Y}+2\delta$, donc le propriétaire de $Y$ et l'image de celui de
$X$ se rejoignent avant $e_i^{X}+5\delta$ ; l'inégalité ultramétrique conclut. Symétrie. □

**H4 — pourquoi cette marge.** La famille $e_i=\sup_{q}\left(m(p_i,q)-\kappa(h(q)-t_i)\right)$ contient la
première couverture ($\kappa=\infty$) et $H_m$ ($\kappa=1$). Pour $\kappa<1$, les points de la propre remontée de
$p_i$ rendent $e_i$ infini : $\kappa\geq 1$ est nécessaire. La même preuve donne
$|\Delta e|\leq(1+2\kappa)\delta$ et $|\Delta u|\leq(1+4\kappa)\delta$ : **$\kappa=1$ est la marge admissible la plus
stable**, $\kappa=\infty$ la plus précoce et discontinue. Aucun autre paramètre n'entre dans la règle.

**H5 — le seuil $m$.** $m\leq k$ ne change rien : toute composante couvre au moins $k$ sites. $m=k+1$ a un sens
simple : la composante contient au moins **deux** $(k-1)$-simplexes distincts (deux $k$-parties distinctes
couvrent au moins $k+1$ sites, et une seule en couvre $k$). Une boule de $k$ points n'est pas un amas. Toute
fusion est qualifiée dès sa naissance pour $m=k+1$ : deux composantes distinctes ne couvrent jamais les mêmes
$k$ sites, sinon elles partageraient ce sommet de $\Gamma_k$. Le cardinal couvert est transporté exactement par P5 (entier et monotone) ; une masse
fractionnaire ne l'est pas (§ 5). Choix retenu :
$m=\max(k+1,\mathrm{mcs})$, où mcs est la taille minimale de cluster de la condensation.

## 4. Fixtures exactes

Toutes vérifiées par deux routes sans code commun de règle (oracle de la définition sur les coupes de
$\Gamma_k$, consommateur du banc sur les incidences) ; accord exact des ultramétriques sur 1 178 puis 905 nuages
aléatoires (66 440 et 51 260 comparaisons), et sur l'export natif dans la porte G4.

| Fixture | HDBSCAN | core | première couverture | $H_{k+1}$ |
| --- | --- | --- | --- | --- |
| Deux triangles équilatéraux exacts (plan $x-y-z=0$), $k=2$ | les six points d'un coup | rien avant la racine | AB, EF ; C et D à la racine | **ABC, DEF** à $2/3$, réunis à $3/2$ |
| Cinq points de l'auditeur, $k=2$ (sa fermeture $m=3$ réunit tout à $100/9$) | — | {1,2}, {3,4} dès $16$ ; le point 0 à $40$ | {1,2}, {3,4} dès $4$ ; le point 0 à $36$ (LCA) | {1,2}, {3,4} dès $100/9$, séparés jusqu'à $36$ ; le point 0 entre à $36$ |
| $\lbrace 0,2s,4s\rbrace$ puis $4s+1$, $s=1000$ | — | — | médian : $4\,000\,000$ puis $1\,000\,000$ (saut) | médian : $4\,000\,000$ puis $4\,001\,000$ (continu) |
| $k=1$, 200 nuages | liaison simple | — | — | liaison simple exacte |

Stabilité mesurée (oracle, 2 458 040 paires perturbées de $\pm1$ par coordonnée) : rapport
$\vert\Delta u\vert/\delta$ maximal $1{,}46$ pour $H_1$ et $H_{k+1}$, aucune violation de H3 ; $59{,}3$ et 10 794
violations pour la première couverture ; $3{,}86$ et 97 dépassements pour core.

## 5. Lecture critique : thèse, auditeur, et cette règle

**Thèse, § 9.1.** La construction (masses de faces $S_\tau=\sum_{\sigma\supset\tau}\psi(\rho(\sigma))$, partition de
l'unité $S_\tau/T_x$, condensation par masse puis vote $\mathrm{argmax}_c V_x(c)$, proposition 7) est cohérente
pour une **partition plate** après sélection, et elle a le mérite de nommer l'objet (recouvrement, partition des
$(K-1)$-simplexes). Mais :

1. elle ne fournit pas de hiérarchie laminaire de points : appliqué à chaque niveau, le vote peut changer de
   propriétaire avant la fusion des deux candidats ;
2. $S_\tau$ dépend de $F_K$, l'ensemble des faces *construites* (Gabriel dans la version standard) : une propriété
   de l'algorithme, pas de l'objet K-NN ; l'exposant $p$ de $\psi(t)=t^{-p}$ est un paramètre libre ;
3. l'appartenance de Gabriel bascule sous perturbation : masses, condensation et vote ne sont pas continus ;
4. le départage des égalités de l'argmax est une convention de repère (non équivariante, cf. P4).

**Auditeur, fermeture qualifiée.** Laminaire, équivariante, stable en $1\varepsilon$, avec une optimalité exacte
(plus grande ultramétrique dominée par les échéances de co-couverture). Mais :

1. c'est une liaison simple sur l'hypergraphe de co-couverture : elle réunit des blocs que FULL sépare (cinq
   points). Dans le cadre du chapitre 7 de la thèse, une co-couverture dans la vallée entre deux amas crée une
   fusion parasite **avant** celle de FULL : la fraction récupérable ne peut que baisser. C'est le défaut même que
   la thèse reproche à la liaison simple et à HDBSCAN ;
2. l'optimalité est relative aux échéances $w$, qui posent déjà « co-couvert implique réuni » : c'est un choix,
   pas une conséquence ;
3. son identité $(k,m=k+1)=(k+1,m=k+1)$ montre qu'au seuil $k+1$ la fermeture d'ordre $k$ n'apporte rien de plus
   que l'ordre $k+1$ ;
4. sur les douze synthétiques de sa campagne, elle reste sous la première couverture à tout $K$ (0,816–0,831
   contre 0,827–0,852).

L'auditeur a raison sur deux points que cette note reprend : core et une première attache irréversible ne sont
pas l'objet de la définition 8, et la première couverture est instable (sa propre mesure : branches changées
32 fois sur 32 sous perturbation).

**Cette règle, $H_m$.** Elle choisit la direction intérieure et paie la stabilité par des retards : un site
couvert longtemps par deux composantes n'entre qu'à leur fusion. Limites :

1. perte de rappel sur les sites partagés (bords d'objets au contact) ; la complétion par le vote du § 9.1 n'est
   légitime qu'après sélection, pour une sortie plate ;
2. la marge est définie sur le niveau carré : la constante de H3 dépend de l'échelle ($\delta$ contient
   $\sqrt{\Lambda}$) ; la même règle en rayon donnerait $3\varepsilon$ et $5\varepsilon$ exacts mais demanderait
   des sommes de racines exactes ;
3. elle ignore les poids de densité : deux rivaux pèsent pareil quelle que soit la masse qu'ils portent ;
4. elle vit à $k$ fixé : les pendaisons de deux ordres ne sont pas emboîtées en général (P4) ;
5. aucune optimalité globale n'est démontrée ; H4 ne fait que classer les marges admissibles.

## 6. Mesures

Reçu : [points_g4](../receipts/developpement_20261003/points_g4/README.md). Même évaluateur pour HDBSCAN
(`sklearn` 1.7.2, `min_samples=k`, arbre du lien simple de l'atteignabilité mutuelle) et pour la tour : meilleur IoU
de chaque groupe vrai parmi les blocs publiés aux **plateaux fermés** (une fusion ex æquo n'est lue qu'entière),
points void exclus du bloc. Ce n'est pas une partition choisie : c'est la présence de l'objet dans la hiérarchie,
critère des démos de `Zoltan/`. Règles : `core`, `cover` (première couverture, LCA des ex æquo), `first`
(première couverture qualifiée, $m=k+1$, sans marge), `margin1` ($H_1$), `margin` ($H_{k+1}$, la règle retenue).

**Porte native (G4, `claudepts1`).** L'export natif lu par le banc rend exactement les ultramétriques de l'oracle
de la définition : 4 602 nuages, 261 335 comparaisons exactes, 0 désaccord, 90 379 sites retardés, 4 fixtures sur 4
(deux triangles $m=3$ et $m=1$, cinq points, continuité).

**Synthétique** (128 scènes : huit familles, niveaux medium et hard, 3 et 8 groupes, $n=2000$ et $8000$, deux
graines, 5 % de bruit ; plan écrit avant lecture). IoU moyen du meilleur bloc et différence appariée avec HDBSCAN
(intervalle bootstrap à 95 % par scène) :

| $k$ | HDBSCAN | core | cover | first | $H_1$ | $H_{k+1}$ | $H_{k+1}$ − HDBSCAN |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 2 | 0,822 | 0,773 | 0,831 | 0,834 | 0,821 | 0,832 | +0,010 [+0,006 ; +0,015] |
| 3 | 0,806 | 0,759 | 0,837 | 0,841 | 0,820 | 0,837 | +0,029 [+0,024 ; +0,035] |
| 5 | 0,785 | 0,753 | 0,844 | 0,847 | 0,820 | 0,841 | +0,053 [+0,045 ; +0,062] |
| 10 | 0,769 | 0,752 | 0,856 | 0,858 | 0,830 | 0,850 | +0,079 [+0,067 ; +0,091] |

La qualification $m=k+1$ fait l'essentiel du gain sur $H_1$ ; la marge coûte 0,003 à 0,008 par rapport à `first`
(prix de la stabilité H3). HDBSCAN garde un léger avantage dans `hierarchical` et `shells` (0,995 contre 0,996 à
0,998 ; 0,987 à 0,992 contre 0,992 à 0,993) ; $H_{k+1}$ gagne dans les six autres familles, jusqu'à +0,13 en
`spherical` et +0,14 en `unbalanced` à $k=10$.

**LiDAR** (44 trames : cinq démos de `Zoltan/demos`, 39 trames du criblage `Zoltan/demos/recherche` où HDBSCAN
échoue ; 524 instances d'au moins 50 points). IoU moyen : à $k=2,3,5,10$, HDBSCAN 0,899 / 0,897 / 0,891 / 0,873,
$H_{k+1}$ 0,899 / 0,901 / 0,899 / 0,889. **57 sauvetages** (objet, $k$) où HDBSCAN reste à 1/2 ou moins et
$H_{k+1}$ dépasse 1/2 au même $k$, contre 11 pertes ; **8 sauvetages forts** où HDBSCAN reste à 1/2 ou moins à
**tous** les $k$ testés, dont le vélo 39 de 08/000932 (0,983 à $k=2$), l'autre véhicule 18 de 08/001240 (0,922 à
$k=3$, HDBSCAN 0,464), le vélo 55 de la démo 01 et les vélos 55 et 56 de la démo 04.

Objets des démos (HDBSCAN recalculé, égal aux valeurs publiées des démos ; $H_{k+1}$) :

| Démo, objet | HDBSCAN $k=2/3/5/10$ | $H_{k+1}$ $k=2/3/5/10$ |
| --- | --- | --- |
| 01 A (vélo 43) | 0,66 / 0,65 / 0,67 / 0,62 | 0,60 / 0,63 / 0,70 / 0,68 |
| 01 B (vélo 57) | 0,41 / 0,41 / 0,41 / 0,40 | 0,41 / 0,48 / 0,42 / 0,42 |
| 01 C (vélo 56) | 0,65 / 0,64 / 0,75 / 0,64 | 0,66 / **0,82** / **0,84** / 0,67 |
| 01, vélo 55 (« échoue lui aussi ») | 0,49 / 0,47 / 0,50 / 0,30 | 0,48 / **0,55** / **0,54** / 0,48 |
| 02 A / B (vélos contre façade) | ≤ 0,36 | ≤ 0,40 |
| 02 C (vélo 59) | 0,71 / 0,67 / 0,60 / 0,52 | 0,69 / 0,44 / 0,67 / 0,47 |
| 03 A (piéton contre façade) | 0,44 partout | 0,44 à 0,46 |
| 04 C (vélo 56, avec sol) | 0,36 / 0,35 / 0,33 / 0,31 | 0,39 / **0,56** / 0,45 / 0,37 |
| 05 A–C (témoin voitures) | 0,83 à 0,99 | 0,85 à 1,00 |

Lecture : là où FULL contient l'objet, la hiérarchie de points le rend et HDBSCAN peut le perdre. Là où FULL ne le
contient pas (01 B, 02 A–B, 03 A : plafond de la définition 8 sous 1/2), aucune projection ne le sauve — la limite
est dans l'objet K-NN (objets au contact), pas dans la projection.
