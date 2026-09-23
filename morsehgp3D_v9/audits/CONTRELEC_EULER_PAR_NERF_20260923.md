# Contrelecture de l'invariant d'Euler par le nerf des boules

23 septembre 2026. Base relue : note C et oracles du commit `cab281d8`.
Cadre : `phase=exploration_v9_hors_registre`, `backend=reference_cpu`,
`profile=quantized_u18_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`. GCP non utilisé. Aucune source moteur ni aucun
registre formel modifié.

## Verdict

L'égalité $E_K=1$ est une **condition nécessaire** de complétude du catalogue.
Elle admet une preuve finie sans hypothèse de position générale ni extension
de la théorie de Morse de Reani–Bobrowski aux coquilles dégénérées. La preuve
ci-dessous fournit aussi une seconde façon exacte de calculer chaque
contribution, indépendante de l'arrangement de grands cercles de l'auditeur C.
Les deux calculs concordent sur les cas explicites et les diagnostics indiqués
plus bas. Cette égalité seule ne certifie ni chaque clé ni la tour FULL.

## Identité globale

Soient $P$ un ensemble fini de $n$ sites distincts de $\mathbb{R}^{3}$ et
$D_x(r)$ la boule fermée de rayon $r$ autour du site $x$. Le sous-niveau de la
distance au $K$-ième voisin est $L_K(r)=\left\lbrace y:\#\left\lbrace x\in P:y\in D_x(r)\right\rbrace\geq K\right\rbrace$.
Pour $j\geq K$, posons $w_{j,K}=(-1)^{j-K}\binom{j-1}{K-1}$, et $w_{j,K}=0$
pour $j<K$. L'identité binomiale $\mathbf{1}_{t\geq K}=\sum_{j=K}^{t}w_{j,K}\binom{t}{j}$ vaut pour tout entier $t\geq0$ : elle vaut 1 à $t=K$ et sa
différence entre $t$ et $t+1$ est nulle par Pascal et $(1-1)^{t-K+1}=0$.

La valuation d'Euler appliquée aux intersections finies de boules donne donc
$\chi(L_K(r))=\sum_{J\subseteq P}w_{|J|,K}\,\mathbf{1}_{\rho(J)\leq r}$,
où $\rho(J)$ est le rayon de la boule minimale de $J$. En effet, chaque
intersection $\bigcap_{x\in J}D_x(r)$ est convexe : son Euler vaut 1 si elle
est non vide, 0 sinon, et elle devient non vide exactement à $\rho(J)$.
Cette formule est aussi l'inclusion-exclusion de l'union des intersections
de $K$ boules, après regroupement des mêmes $J$.

Regroupons les sous-ensembles $J$ par leur **boule minimale unique** $B(J)$.
Des boules différentes peuvent avoir le même rayon ; leurs contributions
s'additionnent au même niveau, sans supposer des valeurs critiques distinctes.
À $r=0$, seuls les singletons contribuent : $\chi(L_1(0))=n$ et
$\chi(L_K(0))=0$ pour $K\geq2$. Pour $r$ assez grand, toutes les boules
$D_x(r)$ possèdent un point commun ; l'union de leurs intersections de $K$
boules est étoilée en ce point, donc d'Euler 1. La somme des sauts aux rayons
strictement positifs vérifie ainsi $n\mathbf{1}_{K=1}+\sum_B e_K(B)=1$ pour $1\leq K\leq n$.

## Contribution d'une boule, coquille quelconque

Fixons une boule $B$ de centre $c$ et de rayon positif. Soient $I$ ses $p$
sites strictement intérieurs et $U$ sa coquille de $u$ sites. Un sous-ensemble
$J$ a précisément $B$ pour boule minimale si et seulement si
$J=R\cup T$, avec $R\subseteq I$, $T\subseteq U$ et $c\in\mathrm{conv}(T)$.
Le sens direct vient de la caractérisation de la boule minimale par les
points actifs de sa frontière ; le sens réciproque suit de la même
caractérisation, puisque les sites de $R$ sont strictement intérieurs.
L'appartenance à l'enveloppe est **faible** : $c$ peut être sur sa frontière
pour un $T$ non minimal.

En regroupant les $\binom{p}{i}$ choix de $R$, on obtient la formule exacte
$e_K(B)=\sum_{T\subseteq U:\,c\in\mathrm{conv}(T)}\sum_{i=0}^{p}\binom{p}{i}w_{|T|+i,K}$.
La forme polynomiale est plus compacte :
$\sum_{K\geq1}e_K(B)t^{K-1}=t^p\sum_{T\subseteq U:\,c\in\mathrm{conv}(T)}(t-1)^{|T|-1}$.
Elle découle de $\sum_{K=1}^{j}w_{j,K}t^{K-1}=(t-1)^{j-1}$.
Elle montre immédiatement que $e_K(B)=0$ pour $K\leq p$ et pour $K>p+u$.

Si la coquille est un support minimal affinement indépendant de taille $q$,
son seul sous-ensemble $T$ contenant $c$ dans son enveloppe est $U$ lui-même.
On retrouve $t^p(t-1)^{q-1}$, donc
$e_K(B)=(-1)^{p+q-K}\binom{q-1}{p+q-K}$ pour $p<K\leq p+q$.
La multiplicité binomiale figure bien dans l'implémentation de
`euler_check.cpp` ; son commentaire d'en-tête ne mentionne que le signe et
doit être corrigé avant d'être cité comme spécification. Une simple égalité
$u=q$ ne suffit pas à employer cette formule si la minimalité du support
n'est pas certifiée.

### Égalité avec le lien inférieur dégénéré

La formule de l'auditeur C se déduit aussi de ce comptage, sans hypothèse de
position générale. Pour $m=K-p$ dans $1..u$, posons
$H_x=\{v\in S^2:\langle v,x-c\rangle>0\}$ et
$\Lambda_m=\{v:\#\{x\in U:v\in H_x\}\geq m\}$. Ici $\chi_c$ désigne
l'Euler **à supports compacts** de cet ouvert ; c'est le comptage alterné
des cellules ouvertes utilisé par `chi_cells`. L'identité binomiale de la
première section et l'additivité de $\chi_c$ donnent
$\chi_c(\Lambda_m)=\sum_{\varnothing\ne T\subseteq U}w_{|T|,m}\,
\chi_c(\bigcap_{x\in T}H_x)$.

L'intersection est soit vide, soit un ouvert géodésiquement convexe contenu
dans un hémisphère, donc homéomorphe à un disque ouvert et de $\chi_c=1$.
Par séparation stricte des convexes, elle est non vide exactement quand
$c\notin\mathrm{conv}(T)$. Comme
$\sum_{T\subseteq U}w_{|T|,m}=1$ pour $m\leq u$ (avec $w_{0,m}=0$), on obtient
$1-\chi_c(\Lambda_m)=\sum_{T:\,c\in\mathrm{conv}(T)}w_{|T|,m}$.
Le facteur polynomial $t^p$ ci-dessus décale simplement l'ordre de $m$
à $K=p+m$. Ainsi **la contribution du lien inférieur dégénéré et celle
des sous-ensembles sont égales**, et l'énoncé local de C peut être promu
de `conditional_theorem` à preuve finie, sous la convention explicite
$\chi=\chi_c$ pour les ouverts. Cette preuve ne change pas la limite
« nécessaire, non suffisant » du contrôle de catalogue.

## Admissibilité et contrôle fini

Une boule avec $e_K(B)\ne0$ apparaît comme boule minimale d'au moins un
$J$, donc $c\in\mathrm{conv}(U)$. Par Carathéodory, il existe dans $U$ un
support minimal d'au plus quatre sites : $q_{\min}\leq4$. Comme $p<K$,
$p+q_{\min}\leq K+3\leq K_{\max}+1$ pour $K\leq K_{\max}-2$. Une telle
boule satisfait donc le **critère d'admissibilité** du catalogue v9 ; cela
ne prouve pas que le générateur l'a effectivement émise. Le contrôle
$E_K=1$ est mathématiquement fondé pour ces ordres seulement.

Sur le domaine accepté par la chaîne ($u\leq12$), un juge indépendant peut
énumérer les $2^u-1\leq4095$ sous-ensembles non vides de la coquille. Il teste
exactement $c\in\mathrm{conv}(T)$ en cherchant dans $T$ un support affinement
indépendant de taille 2 à 4, puis en résolvant ses coordonnées barycentriques
rationnelles **non négatives**. Il peut prétester au plus
$\binom{12}{2}+\binom{12}{3}+\binom{12}{4}=781$ supports, marquer leurs
masques, propager l'appartenance à tous les sur-ensembles en $O(u2^u)$, compter
les masques valides par cardinal, puis calculer le polynôme ci-dessus en
entiers. Cette procédure contrôle aussi $q_{\min}$ et l'hypothèse du cas
générique. Une coquille au-delà de 12 reste un refus explicite du domaine
actuel, jamais une troncature.

## Contre-vérification de l'oracle C

Une implémentation de cette somme avec barycentres `Fraction` a été confrontée
à `chi_cells` du commit `cab281d8`, chargé sans source moteur v9. Toutes les
directions ci-dessous sont de même norme dans chaque ligne ; $p=0$, et les
colonnes donnent $e_1,\ldots,e_u$.

| Coquille $U$ autour de $c=0$ | Somme par sous-ensembles | Grands cercles C |
| --- | --- | --- |
| $(\pm1,0,0),(0,\pm1,0)$ | $(1,-3,1,1)$ | $(1,-3,1,1)$ |
| $(\pm1,0,0),(0,1,0)$ | $(0,-1,1)$ | $(0,-1,1)$ |
| $(\pm5,0,0),(0,5,0),(3,-4,0),(-3,-4,0)$ | $(1,0,-3,1,1)$ | $(1,0,-3,1,1)$ |
| $(\pm1,0,0),(0,\pm1,0),(0,0,1)$ | $(0,1,-3,1,1)$ | $(0,1,-3,1,1)$ |

Diagnostic supplémentaire non archivé comme porte : graine Python
`20260923`, 200 coquilles de 2 à 8 directions tirées parmi les 30 points
entiers de norme carrée 25, une paire antipodale imposée une fois sur deux.
Les 1 013 valeurs locales par ordre concordent, zéro écart. Ce contrôle
empirique ne démontre pas l'équivalence générale avec la formule du lien
inférieur $1-\chi(\Lambda_m)$ ; la somme par sous-ensembles fournit une voie
de calcul autonome qui n'en a pas besoin.

## Portée opérationnelle

Les 18 résultats LiDAR publiés par C à 8k/16k/32k passent leurs ordres
vérifiables, mais les fichiers versionnés ne contiennent pas les coquilles
recomptées ; cette contrelecture ne rejoue donc pas ces 18 sommes.
L'expression « gratuit » de la note C signifie au mieux « hors du chrono de
chaîne » : l'implémentation générique fait au plus quatre additions par boule,
mais son harnais balaie tous les $n$ sites pour chaque boule dégénérée, soit
121 824 000 évaluations exactes sur les 18 lignes publiées, puis exécute les
arrangements en Python. Aucun temps propre de ce juge n'est publié.
Un contrôle intégré peut réutiliser les coquilles déjà recensées par la
chaîne, en conservant un coût mesuré séparément. Une somme égale à 1 peut
masquer des omissions de signes opposés ou des clés à contribution nulle ; elle
ne remplace ni l'oracle T2 borné, ni la comparaison de catalogues $K_{\max}$
et $K_{\max}+2$, ni un juge de tour FULL.
