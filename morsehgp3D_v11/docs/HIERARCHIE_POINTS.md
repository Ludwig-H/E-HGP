# De la tour FULL à une hiérarchie laminaire de points

3 octobre 2026 (Claude, développeur). Cadre :
`exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`.
Demande de l'utilisateur : « comment passer de la tour full à une hiérarchie laminaire sur les points ? Il faut à
la fois une solution satisfaisante mathématiquement, mais aussi que cela marche sur au moins un des exemples de
Zoltan/ où la hiérarchie HDBSCAN échoue » ; puis « privilégie l'aspect mathématique », relire à titre indicatif
les parties I–II du manuscrit, « sois critique vis à vis de la thèse comme vis à vis de l'auditeur », et
« Q2 ou Q3 ne sont que de peu d'importance par rapport au modèle mathématique ».

Code : [`bench/points_export.cpp`](../bench/points_export.cpp) (export natif des incidences fortes de P3),
[`bench/points_radius.py`](../bench/points_radius.py) (règle retenue, décisions exactes),
[`bench/points_hierarchy.py`](../bench/points_hierarchy.py) (règles témoins, évaluation commune),
[`bench/points_reference.py`](../bench/points_reference.py) (oracle de la définition),
[`bench/points_gate.py`](../bench/points_gate.py) (porte natif contre oracle). Reçus :
[points_g4](../receipts/developpement_20261003/points_g4/README.md) (sessions G4) et
[points_math](../receipts/developpement_20261003/points_math/README.md) (workflow `wf_92a63749-ee7` : quatre
propositions, quatre vérifications adverses, synthèse du juge).

## Décision

À ordre $k\geq 2$ fixé, la hiérarchie de points retenue est $H^{r}_{k+1}=P_1\circ\Pi_{k+1}$ : **l'ancrage
persistant $P_1$ de la v10, marge mesurée en rayon, appliqué à la couverture qualifiée $\Pi_{k+1}$** (une
composante ne reçoit des points que si sa composante de $\Gamma_k$ a au moins deux sommets). Chaîne complète :
FULL$_k$ → $H^{r}_{k+1}$ (sans mcs) → condensation à mcs (masses entières) → sélection → complétion facultative.
C'est aussi la recommandation du juge du workflow, et l'auditeur la juge « une réponse pertinente au problème de
frontière ».

Raisons, prouvées (§ 3) : aucune réunion de points avant la fusion FULL ; dates et hauteurs stables en
$3\varepsilon$ ; dans le cadre intrinsèque des profils de couverture, $3$ est la constante minimale et $P_1$ la
règle la plus précoce à cette constante ; deux triangles de la thèse (§ 6.1) et Q1/Q1bis rendus ; indépendance à
mcs ; liaison simple à $k=1$ (règle non qualifiée). Prix, prouvés aussi : toute entrée est comprise entre
$\alpha_{k+1}$ et $\alpha_{k+1}+d_k/2$ ; le respect du cœur est perdu à $k=2$ ; une structure isolée de $k$ sites
n'est jamais un bloc ; les cellules ancrées Q2, Q3, Q4 et Q-Π2 de l'utilisateur échouent (70 jugements sur 125),
ce que l'utilisateur juge secondaire. Au sens du chapitre 7, la qualification a probablement un coût : l'auditeur
exhibe une obstruction de Palm qui, sous des hypothèses de prolongement à l'infini, rend la fraction récupérée
strictement inférieure à celle de FULL (§ 8).

**Ce n'est pas une solution complète du verrou.** Sur le profil brut, les règles stables connues perdent T0 et
Q1 ; sur le profil qualifié, elles perdent Q2–Q4 ; la règle ER0h de la v10 passe les 125 cellules mais n'a aucune
constante uniforme (prouvé, § 6). Fidélité, équivariance et stabilité uniforme ne suffisent pourtant pas à exclure
les cibles : l'auditeur construit une règle qui choisit sa variante selon le nombre de sites, stable en
$5\varepsilon$ à nombre de sites fixé, et qui passe T0, Q1bis et Q2. Ce n'est pas un modèle ; cela montre qu'une
impossibilité doit nommer d'autres axiomes (stabilité par insertion, localité au profil, entrée immédiate sans
rival). Sous ces axiomes, la question reste ouverte (§ 8).

**Provenance.** $P_\kappa$, ses théorèmes de laminarité, de stabilité ($(1+2\kappa)\varepsilon$ pour les dates et
les hauteurs) et de retard viennent de la v10 (mémo « ancrage à marge » du 30 septembre,
`build/v10-verrou-points/ancrage_marges/`, hors dépôt ; preuve algorithmique de l'auditeur v10,
[persistent_anchor_stream](../../morsehgp3D_v10/receipts/audit_continu_20260929/persistent_anchor_stream_20260930/README.md)).
La v11 apporte la qualification $\Pi_{k+1}$, les théorèmes B, C (cadre intrinsèque), E et F du workflow,
l'implantation exacte sur le moteur v11 et les mesures contre HDBSCAN. Une première version mesurait la marge en
rayon **carré** : c'est la règle $Q_1$ de la v10, une régression (§ 6).

## 1. L'objet n'est pas une partition

Pour un ordre $k$ et un rayon $r$, une composante $C$ de $L_k(r^{2})$ définit l'amas discret de la définition 8
de la thèse, $E_C(r)=\lbrace x\in X : d(x,C)\leq r\rbrace$ ; le théorème 2 l'identifie au $k$-polyèdre de
$\Gamma_k$. Pour $k\geq 2$ ces amas se recouvrent (thèse, § 6.1) et la thèse l'assume : « Nous ne chercherons
pas ici à forcer une partition à chaque niveau » (§ 6) ; « l'objet naturel n'est pas une partition de X, mais un
recouvrement de X (ou bien une partition des $(K-1)$-simplexes) » (§ 9.1). Toute hiérarchie laminaire de points
est une **approximation** de cette hiérarchie de recouvrements, intérieure (blocs inclus dans des amas discrets,
réunions aux seules fusions FULL) ou extérieure (fermeture de la co-couverture, proposition de l'auditeur).

Au chapitre 7, la thèse mesure une méthode persistante par la fraction d'un amas dense récupérée **avant la
fusion parasite** (théorème 3), en sémantique de couverture $\Theta^{poly}$ et sous hypothèse de continuité. Une
approximation intérieure ne crée aucune fusion parasite que FULL ne crée pas ; c'est la direction retenue.

## 2. Pendaisons fidèles

Notons $T_k$ l'arbre de fusion de FULL à l'ordre $k$, hauteurs en **rayon**, vu comme espace : un point est un
couple $(\nu, r)$ avec $h(\nu)\leq r< h(\mathrm{parent}(\nu))$. La remontée $\mathrm{Up}(\nu,r)$ suit les
ancêtres ; $m(p,q)$ est le premier rayon où deux remontées se rejoignent, une ultramétrique de rencontre :
$m(p,s)\leq\max(m(p,q),m(q,s))$. La **région de couverture** du site $x_i$ est
$R_i=\lbrace(\nu,r) : x_i\in E_\nu(r)\rbrace$, stable par remontée ; par P3 elle est la réunion des remontées
issues des incidences fortes des boules qui contiennent $x_i$, ce qu'exporte `points_export.cpp`. Une
**pendaison fidèle** choisit un point $p_i=(o_i,e_i)\in R_i$ : le site entre à $e_i$ dans $o_i$, puis suit ses
ancêtres.

**F1 — laminarité, fidélité.** Les blocs forment une hiérarchie laminaire ; $u(i,j)=m(p_i,p_j)$ est au moins le
niveau de fusion FULL des propriétaires ; chaque bloc est inclus dans l'amas discret de son nœud (P4 ; v10 T1–T2).

**F2 — trilemme.** Une pendaison fidèle ne peut pas à la fois faire entrer chaque site à sa première couverture
et dépendre continûment des positions : sur $\lbrace 0,2,4\rbrace$ à $k=2$, le site médian est couvert au rayon
$1$ par deux composantes qui ne fusionnent qu'à $2$, et la réflexion échange les deux choix (P5 ; v10). Il faut
donc retarder ; la question est de combien.

## 3. La règle $H^{r}_{k+1}$ et ses garanties

**Qualification (postulat de modèle).** Pour $k\geq 2$, une composante reçoit des points si et seulement si sa
composante de $\Gamma_k$ a au moins deux sommets, c'est-à-dire si son amas discret compte au moins $k+1$ sites :
un $(k-1)$-simplexe isolé n'est pas un amas. C'est un choix, pas une conséquence : il préserve les deux triangles
(le pont CD n'est jamais qualifié avant la racine) mais s'écarte de la thèse, dont la figure 6.5 compte {C, D}
comme 2-polyèdre, et il fait échouer Q2–Q4. La première couverture qualifiée à l'ordre $k$ est exactement la
première couverture d'ordre $k+1$, $\alpha_{k+1}$ (théorème T1 du rapport `fermeture`). À $k=1$, pas de
qualification ($m=1$, convention figée pour le port natif, § 9 de ce document).

**Définition.** Soit $t_i$ le rayon le plus bas de $R_i^{(k+1)}$ (points qualifiés de $R_i$) et $p_i$ un point de
$R_i^{(k+1)}$ à ce rayon :

$$ D_i=\sup_{q\in R_i^{(k+1)}}\left(m(p_i,q)-h(q)\right),\qquad e_i=t_i+D_i,\qquad o_i=\text{ancêtre de }p_i\text{ vivant à }e_i. $$

$D_i$ est la plus longue persistance d'une branche qualifiée rivale avant qu'elle rejoigne la lignée de première
couverture. C'est $P_\kappa$ de la v10 pour $\kappa=1$, $t(x)=\max(\alpha,\max_j(m_j-\kappa(c_j-\alpha)))$ sur les
barres finies $(c_j,m_j)$ du code-barres couvrant.

**Le paramètre $\kappa$.** La famille $P_\kappa$, $\kappa\geq 1$, va de la plus stable ($\kappa=1$, constante 3,
retards maximaux) à la première couverture ($\kappa=\infty$, discontinue) ; $\kappa<1$ rend les dates infinies.
$\kappa=1$ est le seul choix sans paramètre libre ; $\kappa=2$ (constante 5, horizon d'inspection borné, moins de
retards) est l'alternative déclarée, à départager par l'expérience E1 (§ 8) ; le port natif fige $\kappa=1$ (§ 9 de ce document).

**Arithmétique exacte.** Une date est $\sqrt{t}+\sqrt{m}-\sqrt{q}$ pour trois niveaux carrés rationnels ; ce
n'est pas un niveau du catalogue. Le rival maximal, le propriétaire et le rang plancher comparent deux sommes de
deux racines, tranchées exactement par élévations au carré contrôlées. L'ordre de deux dates (trois racines
contre trois) regroupe d'abord les radicaux par **classes de carrés rationnelles** (le rapport de deux radicandes
est un carré si numérateur et dénominateur réduits sont des carrés parfaits, testé par racine entière, sans
factorisation) ; des classes distinctes étant linéairement indépendantes sur $\mathbb{Q}$, l'égalité est certifiée
si et seulement si tous les coefficients s'annulent. Une somme non nulle est ensuite séparée par encadrements
entiers de précision croissante ; au-delà du budget, la comparaison est **refusée**, jamais déclarée égale. Les
filtres flottants bornent l'erreur absolue par la somme des racines en jeu et retombent sur le calcul exact sinon.
Ces points corrigent la première implantation, sur les contre-gardes de l'auditeur
([hm_review](../receipts/hm_review_20261003/README.md), [hm_followup](../receipts/hm_followup_20261003/README.md)) :
les dates $(2,162,50)$ et $(8,98,32)$ valent toutes deux $5\sqrt{2}$, et le témoin $t=1/4$, $m=14\,000\,000^{2}$,
$q=(14\,000\,000-6/997)^{2}$ faisait donner au filtre un signe faux ; tous deux sont des fixtures de la porte.

| # | Garantie | Statut, source |
| --- | --- | --- |
| H1 | $e_i$ et $o_i$ ne dépendent pas du choix de $p_i$ (inégalité ultramétrique) | prouvé (v10, ce document) |
| H2 | fidèle, équivariante, indépendante de mcs ; un site qu'aucun rival qualifié ne couvre, à aucun rayon avant la rencontre des lignées, entre à sa première couverture qualifiée (énoncé non local) ; à $k=1$ : liaison simple, entrées à $0$ comprises | prouvé ; liaison simple contrôlée sur 200 nuages |
| H3 | sous perturbation appariée de pas $\varepsilon$ (identifiants appariés, sites distincts, poids unitaires ou positifs fixes, $k$ et $m$ fixés) : dates **et** hauteurs bougent d'au plus $3\varepsilon$. La constante 3 est atteinte sur des profils abstraits, sans réalisation géométrique connue. Aucune stabilité par insertion : sur $\lbrace 0,2,4\rbrace$ à $k=2$, $m=3$, ajouter un site à $\eta$ de 0 fait passer l'entrée de 0 de 2 à 1 | prouvé : $(1+2\kappa)\varepsilon$ (v10 T5 et synthèse v10 § 5.2), proposition D du workflow ; chaîne complète depuis la géométrie par l'auditeur (Q1 : $\psi\varphi=\mathrm{Up}(2\varepsilon)$, qualification transportée dans les deux sens) |
| H4 | dans le cadre **intrinsèque** (profils de couverture abstraits, qualifiés ou non, règle locale au profil, équivariante, entrée immédiate sans rival) : aucune constante inférieure à 3 ; $P_\kappa$ atteint $1+2\kappa$ ; $P_1$ est la plus précoce de constante 3 parmi les règles monotones. Sur la seule image des nuages, la borne inférieure ne se transfère pas d'elle-même (à $n=k+1$, $m=k+1$, la règle est $1\varepsilon$-stable) ; en géométrie, seule la borne $\kappa/2$ de la v10 (T6) est connue : la minimalité géométrique de 3 est **ouverte** | prouvé dans ce cadre (théorème C, confirmé par l'auditeur, Q2 et Q7) |
| H5 | $\alpha_{k+1}(x_i)\leq e_i\leq\alpha_{k+1}(x_i)+d_k(x_i)/2$ et $\alpha_{k+1}\leq d_{k+1}$ ; un site entre après son temps de cœur si et seulement si $\alpha_{k+1}+D_i>d_k$. Un point de cœur est dans le bloc de sa composante à la coupe fermée $F$ dès que $\alpha_{k+1}+d_k/2\leq F$, et strictement avant la fusion parasite $F$ si l'inégalité est stricte (chapitre 7). Pas mieux en général : un rival qualifié né **après** $F$ peut retarder un point de cœur au-delà de $F$ (témoin exact R1, 8 sites, entrée $\sqrt{250}-5/2$ pour $F=12$) | prouvé (v10 L6 sur le profil qualifié ; synthèse du workflow ; auditeur, Q3) ; une première version de H5 était fausse (R1) |
| H6 | toute fusion est qualifiée à sa naissance ; la qualification est transportée exactement par P5 (cardinal entier et monotone) | prouvé |
| H7 | toute règle fidèle, équivariante, continue, à entrée immédiate sans rival, regarde des événements postérieurs à la date qu'elle publie ; pour $\kappa=1$ l'horizon n'est pas borné | prouvé (théorème B du workflow) |

*Preuve de H3 (dates, $\kappa=1$).* Les cartes d'entrelacement de P5 décalent les rayons de $\varepsilon$,
commutent aux remontées, envoient $R_i^{(k+1),X}$ dans $R_i^{(k+1),Y}$ et réciproquement (mêmes identifiants
couverts, donc qualification transportée), et $m^{Y}(p,q)\leq m^{X}(\psi p,\psi q)+\varepsilon$. Pour
$q'\in R_i^{(k+1),Y}$, l'inégalité ultramétrique via $p_i^{X}$ et la définition de $D_i^{X}$ donnent
$m^{X}(\psi p_i^{Y},\psi q')\leq D_i^{X}+h(q')+\varepsilon$, d'où $D_i^{Y}\leq D_i^{X}+2\varepsilon$ ; avec
$t_i^{Y}\leq t_i^{X}+\varepsilon$, $e_i^{Y}\leq e_i^{X}+3\varepsilon$. Symétrie. □

## 4. Fixtures exactes

Deux routes sans code de règle commun : oracle de la définition (force brute sur les coupes de $\Gamma_k$, toute
décisions de rival et de propriétaire en rationnels exacts par une routine propre, le décimal ne servant qu'à la
lecture) et banc (incidences, décisions exactes). La porte compare **exactement**, site par site, la date (somme de
radicaux, égalité certifiée) et le propriétaire (niveau de naissance et sites couverts à la naissance), sur des nuages
aléatoires et sur les nuages à égalités exactes ci-dessous ; elle grave douze fixtures et tue quatre mutants causaux
(qualification décalée, marge en niveau carré, absence de marge, coupe ouverte). Une première version comparait des
ultramétriques en flottant à $10^{-9}$ près, avec un oracle décimal à 28 puis 120 chiffres : insuffisant aux
plateaux exacts (auditeur, dont le témoin des quatre sites), remplacé.

| Fixture | HDBSCAN | première couverture | $H^{r}_{k+1}$ |
| --- | --- | --- | --- |
| Deux triangles équilatéraux exacts, plan $x-y-z=0$, $k=2$ | les six points d'un coup | AB, EF ; C et D à la racine | **ABC, DEF**, réunis à la racine |
| Cinq points de l'auditeur, $k=2$ (sa fermeture $m=3$ réunit tout à $\sqrt{100/9}$) | — | {1,2}, {3,4} ; le point 0 à la racine | {1,2}, {3,4}, séparés jusqu'à la fusion FULL ; le point 0 à la racine |
| Point commun $(6-\delta,2)$ des cinq points, rayon d'entrée (×1000) | — | $6$ puis $3{,}333$ dès $\delta=0{,}001$ (saut de $8/3$ relevé par l'auditeur) | $6$ ; $5{,}9991$ ; $5{,}9911$ ; $5{,}9111$ pour $\delta=0$ ; $0{,}001$ ; $0{,}01$ ; $0{,}1$ |
| R1 : huit sites, $k=2$, point de cœur $x$, fusion parasite $F=12$ | — | — | $x$ entre à $\sqrt{250}-5/2\approx 13{,}31$ (rival qualifié né à $12{,}5$) |
| Quatre sites collinéaires de l'auditeur, $k=2$, $m=1$ : date sur une fusion | — | — | $\sqrt{2}+\sqrt{18}-\sqrt{8}=\sqrt{8}$ ; propriétaire = parent né à 8, sites $\lbrace 0,1,3\rbrace$ (coupe fermée) ; l'ancien oracle décimal gardait l'enfant mort |
| $k=1$, 200 nuages | liaison simple | — | liaison simple exacte ($m=1$) |

Stabilité mesurée en rayon (oracle, 502 488 paires perturbées de $\pm1$ par coordonnée, échelles 10 à 1 000) :
rapport maximal $2{,}28\,\varepsilon$ pour les dates comme pour les hauteurs, aucune violation de H3.

## 5. Sortie plate

**Factorisation** (synthèse du workflow, § 5.1, prouvée et contrôlée sur 29 692 et 13 133 blocs). Pour toute
pendaison fidèle $H$ et tout mcs, la règle $H^{\mathrm{mcs}}$ de même lignée et de date $\max(e_x,a_x)$, où $a_x$ est
le premier rayon où la lignée de $x$ couvre au moins mcs sites, a, à tout rayon, les mêmes blocs d'au moins mcs
points que $H$. « Projeter sans mcs, puis condenser » réalise donc exactement la proposition de l'utilisateur du
1er octobre (propriétaire sur la tour condensée), lue en « absorption » ; la lecture « admission stricte » est
réfutée par Q1bis et Q-Π2.

**Condensation.** Masses entières après engagement (un point compte pour 1) ; clusters = blocs d'au moins mcs
points. Les masses fractionnaires du § 9.1 de la thèse ($m_\tau=S_\tau\sum_{x\in\tau}1/T_x$) donnent à chaque
triangle de T0 une masse $8/3<3$ : ils ne seraient plus des clusters à mcs 3. Le critère d'**existence** d'un
cluster (Q-Π2 : un point de bord ne fait pas exister un cluster) relève de cet étage et reste ouvert.

**Sélection.** Hors de cette note ; même sélection pour toutes les hiérarchies comparées, HDBSCAN calculé sur la
même machine (l'ordre des ex æquo de `sklearn` en dépend).

**Complétion facultative**, après sélection, d'un point resté seul mais couvert par un cluster sélectionné : par
défaut le long de sa lignée (sans paramètre, sans réunion nouvelle) ; le vote de la thèse en bras déclaré, jamais
comme projection (par niveau il n'est pas laminaire ; figé, il est discontinu ; il dépend des faces construites et
de l'exposant $p$).

## 6. Lecture critique : thèse, auditeur, v10, et cette note

**Thèse, § 9.1.** La construction (masses de faces $S_\tau$, partition de l'unité $S_\tau/T_x$, condensation par
masse, vote, proposition 7) est cohérente pour une **partition plate** après sélection et nomme bien l'objet. Mais
elle ne donne pas de hiérarchie laminaire de points ; $S_\tau$ dépend des faces *construites* (Gabriel) et de
l'exposant $p$, qui retournent la décision sur les deux triangles (rien, AB | EF, ou ABC | DEF selon le choix) ;
l'appartenance de Gabriel bascule sous perturbation ; les masses fractionnaires perdent T0 à mcs 3 ; le théorème 3
suppose la continuité de $\Theta$.

**Auditeur, fermeture qualifiée.** Ses cinq garanties sont exactes (laminarité, équivariance, stabilité
$1\varepsilon$, optimalité minmax, identité $(k,k+1)=(k+1,k+1)$). Pour $m\leq k+1$ et $k\geq 2$, c'est la liaison
simple du poids $w_{k'}(i,j)=\min\lbrace\beta(F):|F|=k',\ i,j\in F\rbrace$ : elle n'utilise pas la connexité
d'ordre supérieur de FULL (théorème T1 du rapport `fermeture`). Elle réunit deux amas entiers avant FULL
(300 nuages sur 300 du type « deux amas et une vallée »), d'au plus un facteur 2 en rayon. À $m=k+1$ elle a les
mêmes entrées que $H^{r}_{k+1}$ ($\alpha_{k+1}$). Sa place : borne inférieure canonique et certificat,
$u_{\mathrm{fermeture}}\leq w\leq u_H$ pour toute pendaison fidèle qualifiée $H$. Ma formule « la fraction
récupérable ne peut que baisser » était **fausse à $n$ fini** (fixture : 1 contre 1/2) ; retirée.

**Verdict v10 (ER0h).** 125 cellules ancrées sur 125 par un vote non monotone, mais **aucune constante uniforme** :
sur une famille exacte de trois sites, $\vert\Delta u\vert/\delta\geq\frac{64\kappa}{129}\rho^{2}-1$ (proposition S
du rapport `majorite_vote`, confirmée par l'auditeur) ; en rayon aussi, un déplacement 1 fait sauter une date d'au
moins $\kappa\rho^{2}/4-1$ (auditeur, Q7). Les « pentes jusqu'à 103 » de la v10 ne sont pas bornées. ER0 et ER-hv,
discontinues, passent aussi les 125 cellules, et la région $(\eta,\kappa)$ d'ER0h a été réglée sur ces mêmes
cellules. La variante à cône relatif ER0hr passe aussi les 125 cellules, sans constante prouvée.

**Cette note, corrigée.** La première version mesurait la marge en rayon carré : c'est $Q_1$ de la v10, que la v10
gardait comme mutant ; dominée par $P_1$, sans constante uniforme (rapport $31{,}8$ à $L=10\,000$), elle peut
retarder un site sans borne. Elle annonçait $\kappa=1$ comme retard minimal (c'est le maximal), proposait
$m=\max(k+1,\mathrm{mcs})$ (retiré : dépend de mcs et fabrique le cluster refusé en Q-Π2), et énonçait un H5 faux
(R1). Ces erreurs sont aussi inscrites au registre des statuts.

**Plusieurs ordres.** Les blocs qualifiés de deux ordres peuvent se croiser au même rayon (six sites collinéaires,
ordres 2 et 3, rayon 7 ; auditeur, `check_vertical.py`) : $H^{r}_{k+1}$ reste une hiérarchie **à $k$ fixé**.

## 7. Mesures

Reçu : [points_g4](../receipts/developpement_20261003/points_g4/README.md). Même évaluateur pour HDBSCAN
(`sklearn` 1.7.2, `min_samples=k`, arbre du lien simple de l'atteignabilité mutuelle) et pour la tour : meilleur
IoU de chaque groupe vrai parmi les blocs publiés aux **plateaux fermés**, points void exclus. C'est la présence
de l'objet dans la hiérarchie (niveau B, oracle optimiste), critère des démos de `Zoltan/`, pas une partition
choisie. Règles : `core`, `cover` (première couverture, LCA des ex æquo), `first` (première couverture qualifiée,
sans marge), `margin1` et `margin` ($Q_1\circ\Pi_1$ et $Q_1\circ\Pi_{k+1}$, niveau carré), `margin_r`
($H^{r}_{k+1}$, la règle retenue).

**Sessions.** Six sessions G4 gardées, arrêts ciblés certifiés, VM `TERMINATED` relue après chacune. A à D sont
des instantanés de l'arbre de travail (`dev_snapshot` : essais de développement, jamais une preuve publiable) ;
E et F partent de commits poussés (`pushed_commit`) :

| Session | Règle mesurée | Données | Porte (ancienne version) | Remarque |
| --- | --- | --- | --- | --- |
| A `claudepts1` | marge en niveau carré `margin` (consommateur 5df63b60, relevé par l'auditeur) | 128 synthétiques ; 5 démos et 39 trames du criblage | conforme, 4 602 nuages | — |
| B `claudepts2` | idem | 72 trames voisines préparées sur la VM, 20 témoins | conforme | — |
| C `claudepts3` | `margin_r` f023f6d0, filtre flottant défectueux | 128 synthétiques, 64 scènes LiDAR, 72 voisines | conforme | voisines coupées par l'échéance : 66 persistées sur 72 |
| D `claudepts4` | `margin_r` 58952a8b, arithmétique corrigée | mêmes données que C | conforme, 2 478 nuages, 168 882 comparaisons | voisines coupées : 68 sur 72 ; **les 258 scènes communes à C et D sont identiques hors chronométrage** : le correctif ne change aucun observable publié |
| E `claudepts5` | commit 6c88fe0ed | porte stricte, puis démos, voisines, synthétique | arrêtée après sa boucle principale : le dossier d'export des mutants n'était pas créé ; campagnes refusées | défaut de la porte, corrigé en f02f91c7e |
| F `claudepts6` | commit f02f91c7e | mêmes données que C : démos, 72 voisines, synthétique | **porte stricte conforme** : 2 854 nuages, 194 520 comparaisons, 215 974 sites comparés exactement en rayon, fixtures 12/12, mutants 4/4 | toutes étapes complètes ; **les 201 scènes communes à D et F sont identiques hors chronométrage** |

« Identiques » porte sur les observables publiés par scène (meilleurs IoU à six décimales, nombres de blocs, de
nœuds et d'incidences, membres des blocs rattrapés), après retrait des seuls chronométrages ; aucun dump canonique
de toutes les dates et de tous les propriétaires internes ne l'établit (relecture de l'auditeur,
[pts4_review](../receipts/pts4_review_20261003/README.md)).

La version publiée de `points_radius.py` ne diffère de 58952a8b que par sa docstring. La porte stricte compare dates
et propriétaires exactement par site, sur des nuages aléatoires et à égalités exactes, contre un oracle entièrement
exact ; elle grave douze fixtures et tue quatre mutants causaux. Elle est qualifiée sur G4 par la session F, et F
reproduit D à l'identique : les mesures ci-dessous sont celles du code publié. Démos et trames voisines viennent de
F, échecs du criblage et témoins de D.

**Synthétique** (sessions D et F, identiques ; 8 familles × 2 difficultés × 3 ou 8 groupes × 2 graines, 5 % de bruit). Écart
apparié par scène du meilleur IoU moyen, `margin_r` − HDBSCAN, intervalle bootstrap à 95 % (graine fixe), et part
des scènes où `margin_r` fait au moins aussi bien :

| $n$ | $k=2$ | $k=3$ | $k=5$ | $k=10$ |
| --- | --- | --- | --- | --- |
| 8 000 (taille d'intérêt) | +0,008 [+0,004 ; +0,013], 41/64 | +0,026 [+0,020 ; +0,033], 47/64 | +0,050 [+0,039 ; +0,061], 48/64 | +0,078 [+0,062 ; +0,093], 48/64 |
| 2 000 | +0,013 [+0,005 ; +0,020], 47/64 | +0,033 [+0,024 ; +0,042], 47/64 | +0,059 [+0,046 ; +0,073], 48/64 | +0,084 [+0,067 ; +0,103], 48/64 |

Le gain vient de la tour, pas de la projection : à $n=8\,000$, `first` (première couverture qualifiée, sans marge)
fait +0,010, +0,030, +0,055, +0,084 et `cover` +0,007, +0,026, +0,052, +0,083 ; la marge coûte 0,002 à 0,006 au
meilleur bloc, c'est le prix de la stabilité. `core` (points de cœur seuls) perd 0,017 à 0,047. La marge en niveau
carré, rejouée dans la même session, reste de 0,0003 à 0,0023 sous `margin_r` : le choix entre les deux est
mathématique (§ 6), pas statistique. Tableaux complets, toutes règles et toutes tailles, recalculés par le lecteur
du reçu.

**LiDAR** (sessions D et F ; trames de la séquence 08 de SemanticKITTI, grille 1 mm, sol retiré sauf dans la démo
04 ; de 32 462 à 126 267 sites, voisines de 36 752 à 81 688 ; instances d'au moins 50 points ; scènes dédoublonnées
par empreinte des sites : la voisine 000882 est la démo 02 et compte comme démo, l'auditeur, qui dédoublonne aussi
par étiquettes, compte 68 voisines dans D). Cellule : meilleur IoU moyen `margin_r` / HDBSCAN,
puis sauvetages / pertes au même ordre (sauvetage : HDBSCAN à 1/2 ou moins, la règle strictement au-dessus ; perte :
l'inverse).

| Cohorte | Scènes | Instances | $k=2$ | $k=3$ | $k=5$ | $k=10$ |
| --- | --- | --- | --- | --- | --- | --- |
| démos de `Zoltan/` | 5 | 72 | 0,831 / 0,825, +3/−0 | 0,832 / 0,821, +6/−1 | 0,833 / 0,819, +3/−0 | 0,821 / 0,799, +0/−1 |
| échecs du criblage | 37 | 425 | 0,916 / 0,917, +7/−6 | 0,917 / 0,914, +8/−2 | 0,916 / 0,907, +8/−1 | 0,908 / 0,890, +18/−0 |
| trames voisines (F, complètes) | 71 | 859 | 0,884 / 0,879, +19/−8 | 0,882 / 0,875, +22/−7 | 0,881 / 0,870, +27/−4 | 0,878 / 0,852, +35/−1 |
| témoins | 20 | 204 | 0,973 / 0,977, 0/0 | 0,972 / 0,976, 0/0 | 0,969 / 0,970, 0/0 | 0,968 / 0,966, 0/0 |

**Démos de `Zoltan/` où HDBSCAN échoue** (01 et 04 sont la même trame, sans puis avec le sol) :

| Démo, objet | HDBSCAN, $k=2/3/5/10$ | `margin_r`, $k=2/3/5/10$ |
| --- | --- | --- |
| 04, vélo C (instance 56) | 0,361 / 0,348 / 0,335 / 0,314 | 0,387 / **0,557** / 0,451 / 0,374 |
| 04, vélo (instance 55) | 0,453 / 0,438 / 0,465 / 0,278 | 0,448 / **0,512** / **0,509** / 0,457 |
| 01, vélo (instance 55) | 0,487 / 0,471 / 0,500 / 0,299 | 0,481 / **0,550** / **0,543** / 0,491 |
| 04, voiture (instance 342) | 0,462 / 0,462 / 0,465 / 0,717 | **0,705** / **0,705** / **0,705** / 0,752 |
| 01, voiture (instance 352) | 0,447 / 0,432 / 0,552 / 0,588 | **0,583** / **0,576** / 0,574 / 0,613 |

Les trois vélos sont des sauvetages forts : HDBSCAN reste à 1/2 ou moins à tous les ordres testés. Une perte : démo
02, vélo (instance 59), 0,443 contre 0,667 à $k=3$ et 0,469 contre 0,524 à $k=10$. Les vélos contre la façade
(démo 02, instances 38 et 58) et le piéton (démo 03) ne sont rattrapés par aucune règle : le plafond de FULL y reste
sous 1/2 à $K=5$ et $10$ (audit L03). Hors démos, sauvetages forts nets : vélo 39 de c08_000932 (0,983 à $k=2$),
autre véhicule 18 de c08_001240 (0,922 à $k=3$), vélo 25 de c08_001873 (0,650 à $k=2$, HDBSCAN au plus 0,29),
piéton 9 des trames voisines c08_000041 à 47 (0,72 à 0,80 contre 0,45 au plus).

**Ce que ces mesures n'établissent pas.** C'est le niveau B (meilleur bloc, oracle optimiste), pas une partition
choisie : aucune condensation ni sélection n'est mesurée. Les sauvetages sont communs à toutes les règles de la tour
sauf `core`, et MR₂-bord, sans tour, rattrape aussi le vélo C (audit L03) : ils montrent ce que FULL contient, pas
la supériorité de $H^{r}_{k+1}$ sur une autre projection fidèle. Les trames voisines sont corrélées, pas des
observations indépendantes. L'expérience décisive reste E1 (§ 8).

## 8. Points ouverts et expériences décisives

Ouverts : (P1) une règle fidèle, équivariante, uniformément lipschitzienne, **stable par insertion et locale au
profil**, qui passe T0, Q1, Q2, Q3 et Q4 (sans ces deux axiomes, la construction par nombre de sites de l'auditeur
passe T0, Q1bis et Q2). La métrique d'insertion doit faire payer la masse : au sens de Hausdorff, anciens sites
fixes, aucune règle à entrée immédiate sans rival qualifié n'est stable ($\lbrace 0,2,4\rbrace$ contre
$\lbrace 0,\eta,2,4\rbrace$, entrée et réunion sautent de 2 à 1 ; auditeur) ; (P2) la continuité d'ER0h et d'ER0hr, sans constante uniforme ; (P3) la constante géométrique exacte, entre $\kappa/2$ et
$1+2\kappa$ ; (P4) le critère d'existence des clusters à l'étage condensé ; (P5) le chapitre 7 projeté : l'auditeur
prouve que $H^{r}_{k+1}$ et FULL récupèrent la même fraction limite si la masse perdue (frontière couverte hors de
la composante, sites où $\alpha_{k+1}+d_k/2$ dépasse le rayon) devient négligeable, ce qui n'est pas démontré à $K$
et contraste fixés, puisque retards et rayons ont la même échelle ; en sens inverse, il exhibe à $k=2$, $m=3$ une
**obstruction de Palm** : un motif local de probabilité positive retarde un point de cœur de la composante géante
au-delà de la fusion parasite, de sorte que, si la règle se prolonge en une règle mesurable et fidèle sur le
processus infini, $\Theta_{H}<\Theta^{poly}$ aux mêmes intensité et rayon
([palm_obstruction](../receipts/palm_obstruction_20261003/README.md)). Au sens du chapitre 7, la qualification a
donc probablement un coût asymptotique ; il reste à mesurer $\Theta_{H}$ pour lui-même (E4) ; (P6) la synthèse de plusieurs ordres (piste de
l'auditeur : transporter les attaches d'un ordre supérieur vers les ordres inférieurs). Le port natif suivra le
contrat de l'auditeur (Q8 : type de date distinct des niveaux, trois rangs, ordre commun avec FULL, refus
transactionnel ; budgets exacts par profil u18, u21, u24). Questions et réponses :
[QUESTION_CLAUDE_PREUVES_POINTS_20261003](../receipts/audit_dialogues_20261004/QUESTION_CLAUDE_PREUVES_POINTS_20261003.md.snapshot),
[réponses de l'auditeur](../audits/AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md).

Expériences, sessions G4 gardées, tailles 8 000 à 32 000 : **E1** comparaison appariée au niveau B et après
condensation à mcs $\in\lbrace k,10,20,\sqrt{n}\rbrace$ (même sélection) de $H^{r}_{k+1}$ ($\kappa=1$ et $2$),
$P_2\circ\Pi_1$, `first`, `cover`, `core`, fermeture $(k,k+1)$, MR₂-bord et HDBSCAN, avec prédictions écrites
d'avance (à mcs $=k$ sur LiDAR, la qualification doit réduire la sur-segmentation de `cover`) ; **E2** stabilité à
l'échelle sous requantification et gigue appariées ; **E3** prix du postulat (part des entrées après $d_k$,
groupes isolés de $k$ sites) ; **E4** modèle à deux densités du théorème 3 ; **E5** meilleur IoU par objet des
démos à $K=2$ à $5$ avec MR₂-bord à côté.

## 9. Conventions figées pour le port natif (4 octobre 2026)

Cadre : `exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`. Amendement de la
critique d'architecture de la sortie paramétrée, retenu avec les décisions de l'utilisateur du 4 octobre 2026. Le
contrat des points ci-dessus ne change pas : ce paragraphe fige deux conventions du futur port natif
(`--sortie=points`, format `MHGP11PT`, voir [SORTIES.md](SORTIES.md)), avant toute ligne native.

- **Qualification.** $m(K)=1$ si $K=1$, $m(K)=K+1$ sinon (§ 3). À $K=1$, aucune qualification : la règle est la
  liaison simple, entrées à 0 comprises (H2). L'en-tête de `MHGP11PT` publie $m$.
- **Marge.** $\kappa=1$, soit la règle $P_1$ de constante 3 (H3), seul choix sans paramètre libre. $\kappa=2$ reste un
  bras de l'expérience E1, en Python seulement ; aucune option native ne le choisit. L'en-tête publie `kappa=1`.
- **Divergence connue.** Les bancs Python passent $m=k+1$ à tous les ordres, donc $m=2$ à $k=1$ :
  - `bench/points_flat_gate.py:117` (`hang_margin_radius(orders[k], k + 1)`), qui parcourt toujours $k=1$ ;
  - `bench/points_flat_campaign.py:164` (`hang_margin_radius(..., k + 1, 'margin_r')`) ;
  - `bench/points_flat_dump.py:67`, qui écrit aussi $m=k+1$ dans les métadonnées `.npz` (`:71`) ;
  - `bench/points_campaign.py:135-138`, pour `first`, `margin` et `margin_r`.

  Les deux derniers n'emploient $m=2$ à $k=1$ que si `--orders` contient 1 ; leur défaut est `2,3,5,10`.

  **Alignés le 5 octobre 2026** (tranche S9, avant tout différentiel natif) : les quatre bancs appellent
  `points_radius.qualification(k)` ($1$ à $k=1$, $k+1$ sinon), et `points_flat_dump.py` publie ce $m$ dans ses
  métadonnées. La fixture `F13_identite_k1` de `points_flat_gate.py` a été rejouée sous $m(1)=1$ : conforme sur ses
  24 nuages, en ordre de référence Python comme sur l'export natif (rapport `impl_s9.md`). Les résultats antérieurs à
  $k=1$ portent sur $m=2$ et ne se comparent pas au port natif.
- **Port natif** (tranche S9, `src/points/`, `--sortie=points`, format `MHGP11PT` de [SORTIES.md](SORTIES.md), § 7) :
  identique site par site à `hang_margin_radius(order, m(K))` et à `tower_point_tree` sur le même `MHGP11PH` (porte
  `mhgp11_points_vs_python`), et à l'oracle de la définition (`mhgp11_points_oracle`). $K\geq n$ est refusé dès
  $K\geq 2$ (SORTIES.md, § 3, étape 6).
