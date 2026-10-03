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
[`bench/points_gate.py`](../bench/points_gate.py) (porte natif contre oracle). Reçu :
[points_g4](../receipts/developpement_20261003/points_g4/README.md).

## Décision

À ordre $k$ fixé, la hiérarchie de points retenue est $H^{r}_{k+1}$ : **l'ancrage persistant $P_1$ de la v10
(marge mesurée en rayon), appliqué à la couverture qualifiée $\Pi_{k+1}$** (une composante ne reçoit des points
que si elle contient au moins deux $(k-1)$-simplexes). Chaîne complète : FULL$_k$ → $H^{r}_{k+1}$ (sans mcs) →
condensation à mcs → sélection, cette dernière hors de cette note.

Raisons, toutes prouvées (§ 3) : elle n'ajoute aucune réunion que FULL ne fait pas ; dates et hauteurs sont
stables en $3\varepsilon$, constante minimale de toute règle continue de sa classe, et elle est la plus précoce
à cette constante ; elle rend les deux triangles de la thèse (§ 6.1) et la liaison simple à $k=1$. Prix : un
point partagé entre deux composantes entre en retard (au plus $d_k/2$ après sa première couverture) ; une
structure de $k$ sites n'est pas un amas ; les cibles Q2, Q3, Q4 et Q-Π2 du catalogue v10 échouent.

**Provenance.** La règle $P_\kappa$ et ses théorèmes de laminarité, de stabilité et de retard viennent de la v10
(mémo « ancrage à marge » du 30 septembre, `build/v10-verrou-points/ancrage_marges/`, hors dépôt ; preuve
algorithmique de l'auditeur v10,
[persistent_anchor_stream](../../morsehgp3D_v10/receipts/audit_continu_20260929/persistent_anchor_stream_20260930/README.md)).
La v11 apporte la qualification $\Pi_{k+1}$, le front d'optimalité (théorème C), l'anticipation obligatoire
(théorème B) et la borne $3\varepsilon$ des hauteurs, établis par le workflow `wf_92a63749-ee7`
([rapports](../receipts/developpement_20261003/points_math/README.md)), l'implantation exacte sur le moteur v11
et les mesures contre HDBSCAN. Une première version mesurait la marge en rayon **carré** : c'est la règle $Q_1$
de la v10, une régression (§ 5).

## 1. L'objet n'est pas une partition

Pour un ordre $k$ et un rayon $r$, une composante $C$ de $L_k(r^{2})$ définit l'amas discret de la définition 8
de la thèse, $E_C(r)=\lbrace x\in X : d(x,C)\leq r\rbrace$ ; le théorème 2 l'identifie au $k$-polyèdre de
$\Gamma_k$. Pour $k\geq 2$ ces amas se recouvrent (thèse, § 6.1) et la thèse l'assume : « Nous ne chercherons
pas ici à forcer une partition à chaque niveau » (§ 6) ; « l'objet naturel n'est pas une partition de X, mais un
recouvrement de X (ou bien une partition des $(K-1)$-simplexes) » (§ 9.1). Toute hiérarchie laminaire de points
est une **approximation** de cette hiérarchie de recouvrements, intérieure (blocs inclus dans des amas discrets,
réunions aux seules fusions FULL) ou extérieure (fermeture de la co-couverture, proposition de l'auditeur).

Au chapitre 7, la thèse mesure une méthode persistante par la fraction d'un amas dense récupérée **avant la
fusion parasite** (théorème 3), en sémantique de couverture $\Theta^{poly}$. Une approximation intérieure ne crée
aucune fusion parasite que FULL ne crée pas ; c'est la direction retenue.

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
niveau de fusion FULL des propriétaires ; chaque bloc est inclus dans l'amas discret de son nœud. (P4, et
$p_i\in R_i$ avec la stabilité de $R_i$ par remontée.)

**F2 — trilemme.** Une pendaison fidèle ne peut pas à la fois faire entrer chaque site à sa première couverture
et dépendre continûment des positions : sur $\lbrace 0,2,4\rbrace$ à $k=2$, le site médian est couvert au rayon
$1$ par deux composantes qui ne fusionnent qu'à $2$, et la réflexion échange les deux choix (P5 ; théorème A du
workflow, rappel v10). Il faut donc retarder ; la question est de combien.

## 3. La règle $H^{r}_{m}$ et ses garanties

**Qualification.** Une composante est qualifiée au rayon $r$ si son amas discret compte au moins $m$ sites ;
$R_i^{(m)}$ désigne les points qualifiés de $R_i$, encore stable par remontée. On prend $m=k+1$.

**Définition.** Soit $t_i$ le rayon le plus bas de $R_i^{(m)}$ et $p_i$ un point de $R_i^{(m)}$ à ce rayon :

$$ D_i=\sup_{q\in R_i^{(m)}}\left(m(p_i,q)-h(q)\right),\qquad e_i=t_i+D_i,\qquad o_i=\text{ancêtre de }p_i\text{ vivant à }e_i. $$

$D_i$ est la plus longue persistance d'une branche qualifiée rivale avant qu'elle rejoigne la lignée de première
couverture. C'est $P_\kappa$ de la v10 pour $\kappa=1$ : $t(x)=\max(\alpha,\max_j(m_j-\kappa(c_j-\alpha)))$ sur les
barres finies $(c_j,m_j)$ du code-barres couvrant de $x$. Toutes les décisions se ramènent à comparer
$\sqrt{a}+\sqrt{b}$ et $\sqrt{c}+\sqrt{d}$ pour des niveaux carrés rationnels, ce que `points_radius.py` tranche
exactement par élévations au carré contrôlées.

| # | Garantie | Statut, source |
| --- | --- | --- |
| H1 | $e_i$ et $o_i$ ne dépendent pas du choix de $p_i$ (inégalité ultramétrique) | prouvé (ce document, v10) |
| H2 | fidèle, équivariante, indépendante de mcs ; à $k=1$, $m=1$ : liaison simple ; sans rival, entrée à la première couverture qualifiée | prouvé ; liaison simple contrôlée sur 200 nuages |
| H3 | sous perturbation appariée de pas $\varepsilon$ : dates **et** hauteurs bougent d'au plus $3\varepsilon$ | prouvé : $(1+2\kappa)\varepsilon$ pour les dates (v10 T5), $(1+2\kappa)\varepsilon$ pour les hauteurs (proposition D du workflow, qui corrige $(1+4\kappa)$) |
| H4 | aucune règle continue, équivariante, locale au profil, qui fait entrer à la première couverture les sites sans rival, n'a de constante inférieure à 3 ; $P_\kappa$ atteint $1+2\kappa$ ; $P_1$ est la plus précoce de constante 3 parmi les règles monotones | prouvé (théorème C du workflow) |
| H5 | sans qualification ($P_1$) : retard $e_i-\alpha_i\leq d_k(x_i)/2$, donc jamais après le temps de cœur à $k=2$ et au plus $1{,}5\,d_k$ à $k\geq 3$ ; tout point de cœur de la composante géante avec $\alpha+d_k/2\leq F$ entre avant la fusion parasite $F$ (chapitre 7). La qualification ne retarde au-delà que des sites dont toutes les composantes couvrantes ont au plus $k$ sites, hors de la composante géante | prouvé (v10 T3 ; propositions H et I du workflow) |
| H6 | $m=k+1$ : au moins deux faces ; toute fusion est qualifiée à sa naissance ; la qualification est transportée exactement par P5 (cardinal entier et monotone) | prouvé |
| H7 | toute règle fidèle, équivariante, continue, à entrée immédiate sans rival, regarde des événements postérieurs à la date qu'elle publie | prouvé (théorème B du workflow) |

*Preuve de H3 (dates, $\kappa=1$).* Les cartes d'entrelacement de P5 décalent les rayons de $\varepsilon$,
commutent aux remontées, envoient $R_i^{(m),X}$ dans $R_i^{(m),Y}$ et réciproquement (mêmes identifiants
couverts, donc qualification transportée), et $m^{Y}(p,q)\leq m^{X}(\psi p,\psi q)+\varepsilon$. Pour
$q'\in R_i^{(m),Y}$, l'inégalité ultramétrique via $p_i^{X}$ et la définition de $D_i^{X}$ donnent
$m^{X}(\psi p_i^{Y},\psi q')\leq D_i^{X}+h(q')+\varepsilon$, d'où $D_i^{Y}\leq D_i^{X}+2\varepsilon$ ; avec
$t_i^{Y}\leq t_i^{X}+\varepsilon$, $e_i^{Y}\leq e_i^{X}+3\varepsilon$. Symétrie. □

**Le seuil $m$.** $m\leq k$ ne change rien. $m=k+1$ rend les deux triangles : le pont CD ne couvre que deux sites
et ne qualifie jamais avant la racine. Il n'est pas imposé par les cibles de l'utilisateur, qui le réfutent même :
aucun seuil ne passe à la fois Q1bis et Q2 (théorème F du workflow). Il est retenu pour une raison de modèle :
une boule de $k$ points isolée n'est pas un amas. Conséquences : une structure de $k$ sites n'est jamais un bloc,
et un site dont la première couverture est une telle structure est rattaché à la première composante qualifiée qui
le couvre ; c'est exactement ce qui fait échouer Q2, Q3 et Q4 (paire, lentille de filament ou chaîne de $k$ sites
supplantées par un amas plus gros). Un seuil dépendant de mcs violerait
l'indépendance à mcs et fabrique le cluster refusé en Q-Π2 (proposition G).

## 4. Fixtures exactes

Deux routes sans code de règle commun : oracle de la définition (force brute sur les coupes de $\Gamma_k$,
`decimal` à 120 chiffres pour les rayons) et banc (incidences, décisions exactes) ; accord sur 1 211 nuages
aléatoires et 13 692 ultramétriques (écart relatif au plus $1{,}9\cdot 10^{-15}$, arrondi de lecture), et sur
l'export natif dans la porte G4.

| Fixture | HDBSCAN | première couverture | $H^{r}_{k+1}$ |
| --- | --- | --- | --- |
| Deux triangles équilatéraux exacts, plan $x-y-z=0$, $k=2$ | les six points d'un coup | AB, EF ; C et D à la racine | **ABC, DEF**, réunis à la racine |
| Cinq points de l'auditeur, $k=2$ (sa fermeture $m=3$ réunit tout à $\sqrt{100/9}$) | — | {1,2}, {3,4} ; le point 0 à la racine | {1,2}, {3,4}, séparés jusqu'à la fusion FULL ; le point 0 à la racine |
| Point commun $(6-\delta,2)$ des cinq points, rayon d'entrée (×1000) | — | $6$ puis $3{,}333$ dès $\delta=0{,}001$ (saut de $8/3$ relevé par l'auditeur) | $6$ ; $5{,}9991$ ; $5{,}9911$ ; $5{,}9111$ pour $\delta=0$ ; $0{,}001$ ; $0{,}01$ ; $0{,}1$ |
| $k=1$, 200 nuages | liaison simple | — | liaison simple exacte |

Stabilité mesurée en rayon (oracle, 502 488 paires perturbées de $\pm1$ par coordonnée, échelles 10 à 1 000) :
rapport maximal $2{,}28\,\varepsilon$ pour les dates comme pour les hauteurs, aucune violation de H3.

## 5. Lecture critique : thèse, auditeur, v10, et cette note

**Thèse, § 9.1.** La construction (masses de faces $S_\tau$, partition de l'unité $S_\tau/T_x$, condensation par
masse, vote, proposition 7) est cohérente pour une **partition plate** après sélection et nomme bien l'objet.
Mais elle ne donne pas de hiérarchie laminaire de points (le vote peut changer de propriétaire avant la fusion
des deux candidats) ; $S_\tau$ dépend des faces *construites* (Gabriel), une propriété de l'algorithme ;
l'exposant $p$ est libre ; l'appartenance de Gabriel bascule sous perturbation ; le départage des égalités est
une convention de repère. Le vote reste le bon outil pour **compléter** une sortie plate avec les points
partagés que $H^{r}_{k+1}$ retarde.

**Auditeur, fermeture qualifiée.** Ses cinq garanties sont exactes (laminarité, équivariance, stabilité
$1\varepsilon$, optimalité minmax, identité $(k,k+1)=(k+1,k+1)$ ; contrôle indépendant du workflow, 450 nuages).
Pour $m\leq k+1$ et $k\geq 2$, c'est la liaison simple du poids $w_{k'}(i,j)=\min\lbrace\beta(F):|F|=k',\ i,j\in F\rbrace$ :
elle n'utilise pas la connexité d'ordre supérieur de FULL, et elle est encadrée par HDBSCAN à un facteur 2
près (théorèmes T1–T3 du workflow). Elle réunit deux amas entiers avant FULL (300 nuages sur 300 du type
« deux amas et une vallée »), mais jamais plus tôt que d'un facteur 2 en rayon ; ma formule « la fraction
récupérable ne peut que baisser » est **fausse à $n$ fini** (fixture : 1 contre 1/2) et reste une conjecture
asymptotique. Sa place : enveloppe extérieure canonique et certificat ($w\leq u_F\leq 4\max(e_i,e_j,w)$ pour
toute pendaison fidèle), pas la projection principale. Son propre test de la première couverture qualifiée a
trouvé le saut que $H^{r}_{k+1}$ supprime (§ 4).

**Verdict v10 (ER0h).** 125 cibles ancrées sur 125, par un vote non monotone ; aucune borne de stabilité
uniforme n'est prouvée (pentes locales jusqu'à 103). Les cibles T0 et Q1 exigent une règle sensible au nombre de
branches couvrantes, incompatible avec toute règle locale au profil et monotone (théorème E) ; la compatibilité
de ces cibles avec une stabilité uniforme reste **ouverte**. Selon la consigne de l'utilisateur, le modèle prime.

**Cette note, corrigée.** La première version mesurait la marge en rayon carré : c'est $Q_1$ de la v10, que la v10
gardait comme mutant ; elle est dominée par $P_1$ (entrées jamais plus précoces), n'a pas de constante uniforme
(rapport $31{,}8$ à $L=10\,000$) et peut retarder un site sans borne. Elle annonçait $\kappa=1$ comme retard
minimal : c'est le retard **maximal** de la famille, choisi pour sa constante minimale. Elle proposait
$m=\max(k+1,\mathrm{mcs})$ : retiré.

## 6. Mesures

Reçu : [points_g4](../receipts/developpement_20261003/points_g4/README.md). Même évaluateur pour HDBSCAN
(`sklearn` 1.7.2, `min_samples=k`, arbre du lien simple de l'atteignabilité mutuelle) et pour la tour : meilleur
IoU de chaque groupe vrai parmi les blocs publiés aux **plateaux fermés**, points void exclus. C'est la présence
de l'objet dans la hiérarchie, critère des démos de `Zoltan/`, pas une partition choisie. Règles témoins :
`core`, `cover` (première couverture, LCA des ex æquo), `first` (première couverture qualifiée, sans marge),
`margin` ($H_{k+1}$ en rayon carré, $Q_1\circ\Pi_{k+1}$), `margin_r` ($H^{r}_{k+1}$, la règle retenue).

MESURES_A_COMPLETER
