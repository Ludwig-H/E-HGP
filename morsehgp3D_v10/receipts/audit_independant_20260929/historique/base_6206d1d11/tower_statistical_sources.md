# Sources primaires et voie statistique pour la projection de points

Recherche du 29 septembre 2026, complément à [TOUR_ET_POINTS.md](TOUR_ET_POINTS.md). Références vérifiées sur les
articles des auteurs ou les proceedings ; aucune source secondaire utilisée pour les résultats ci-dessous.
Cette note propose une route de preuve et de comparaison. Elle ne qualifie pas statistiquement la v10 actuelle.

## Quatre résultats pertinents, avec leurs hypothèses

**1. Consistance uniforme de la densité K-NN.** Devroye et Wagner, *The strong uniform consistency of nearest
neighbor density estimates*, Annals of Statistics 5(3), 536–540, 1977. Pour un échantillon i.i.d. dans
$\mathbb{R}^{d}$ de densité uniformément continue f, l'estimateur $\widehat f_n(y)=k/(nV_k(y))$ converge
uniformément presque sûrement si $k/n\to0$ et $k/\log n\to\infty$. Le facteur k−1 au lieu de k ne modifie pas
ce résultat asymptotique. Ce théorème est plus exigeant que le seul $k\to\infty$.
[Article, PDF hébergé par Devroye](https://luc.devroye.org/devroye_wagner_1977_the_strong_uniform_consistency_of_nearest_neighbor_density_estimates.pdf), p. 536–537.

**2. Un comparateur de connexité avec garanties finies.** Chaudhuri et Dasgupta, *Rates of convergence for the
cluster tree*, NeurIPS 2010. Leur robust single linkage admet les sites dont le rayon k-NN est ≤r et relie deux
sites distants d'au plus αr. Le théorème 6 sépare et connecte correctement des régions $(\sigma,\epsilon)$-
séparées si les effectifs, la densité minimale et la largeur σ suffisent. Le régime présenté utilise α proche de
$\sqrt{2}$ à 2 et k d'ordre $d\log n/\epsilon^2$, avec facteur de confiance. L'article rappelle aussi qu'un
cluster tree calculé depuis un estimateur uniformément consistant hérite de la consistance de Hartigan. Il ne
prouve pas la même chose pour un K fini constant.
[Article dans les proceedings](https://papers.nips.cc/paper_files/paper/2010/file/b534ba68236ba543ae44b22bd110a1d6-Paper.pdf), § 2.2, fig. 3, théorème 6.

**3. La suppression des faux embranchements est une obligation distincte.** Chaudhuri, Dasgupta, Kpotufe et von
Luxburg, *Consistent procedures for cluster tree estimation and pruning*, IEEE TIT 60(12), 7900–7912, 2014.
Le théorème VII.5 qualifie un élagage par reconnexion à un niveau de densité légèrement inférieur. Les garanties
demandent une marge de séparation, un effectif suffisant et un paramètre d'élagage contrôlant l'oscillation locale
de f ; la consistance après élagage demande aussi que ce paramètre tende vers zéro. La taille d'une branche seule
n'est pas le critère justifié. Ce résultat concerne leurs arbres RSL/k-NN et leur élagage ; il ne se transfère pas
automatiquement à EOM, à z ou à la première couverture.
[Version publiée, PDF des auteurs](https://cseweb.ucsd.edu/~dasgupta/papers/cluster-ieee.pdf), § VII, fig. 9 et théorème VII.5.

**4. Mesurer les dates de fusion, pas seulement une partition réussie.** Eldridge, Belkin et Wang, *Beyond Hartigan
Consistency: Merge Distortion Metric for Hierarchical Clustering*, COLT 2015. Leur distance compare les hauteurs
de fusion des paires. La consistance de Hartigan permet encore sursegmentation et mauvais emboîtements ; les
théorèmes 15–16 relient convergence en merge distortion, minimalité uniforme et séparation uniforme. Le théorème
17 borne la variation entre les cluster trees de f et g par $\lVert f-g\rVert_\infty$. Leur théorème 20 qualifie
RSL sous f Lipschitz, support compact et nombre fini de composantes à chaque niveau, avec le régime de paramètres
de leur analyse. Le théorème de stabilité ne signifie pas invariance du nombre de branches après perturbation.
[Article COLT](https://proceedings.mlr.press/v40/Eldridge15.pdf), définitions 4, 6, 13 ; théorèmes 15–17, 20.

## Ce que le FULL et core permettent vraiment de déduire

Voici une **déduction propre à l'objet v10**, pas une nouvelle garantie publiée pour son moteur. Si le nuage est
interprété dans un modèle euclidien de dimension d et si $v_d$ est le volume de la boule unité, poser
$\widehat f_{K,n}(y)=K/(nv_dD_K(y)^{d/2})$. Pour τ>0, le superniveau de cet estimateur est exactement
$L_K((K/(nv_d\tau))^{2/d})$. La tour FULL à K fixé est donc le cluster tree de cet estimateur avec ce
changement de niveau. Core en est la restriction aux sites observés : le point x apparaît précisément quand sa
valeur estimée atteint le niveau demandé.

Pour le FULL natif sur tout $\mathbb{R}^{3}$, le modèle de densité ambiante correspondant prend d=3. Changer
l'exposant en une dimension intrinsèque supposée ne prouve pas que les composantes calculées dans l'espace
ambiant sont celles d'un superniveau restreint à une surface ; des chemins hors de cette surface peuvent relier
des régions distinctes sur elle. Ce transfert réclamerait une preuve géométrique supplémentaire.

Cette correspondance est plus forte qu'une ressemblance avec une méthode de densité. Elle indique où placer la
preuve statistique manquante : sur l'estimateur et le modèle d'entrée, puis sur l'effet d'une sélection de branches.
Elle ne réclame pas de remplacer la géométrie FULL par une densité minimale calculée sur les seuls points attachés.
Les dates de fusion natives doivent rester marquées ; le minimum des densités des seules feuilles peut être plus
haut que le col géométrique qui a réellement fusionné leurs composantes.

Une borne directe suffit pour le premier transfert. Supposer $\lVert\widehat f-f\rVert_\infty\leq\eta$.
Alors $\lbrace f\geq\tau+\eta\rbrace\subseteq\lbrace\widehat f\geq\tau\rbrace\subseteq\lbrace f\geq\tau-\eta\rbrace$.
Pour deux points, définir $m_f(x,y)=\sup\lbrace\tau:x,y\text{ sont dans la même composante de }\lbrace f\geq\tau\rbrace\rbrace$.
Ces inclusions donnent $|m_{\widehat f}(x,y)-m_f(x,y)|\leq\eta$, puis la même borne sur toutes les paires de
sites. On conserve ici les niveaux de fusion du continuum ; on ne rééquipe pas arbitrairement l'arbre fini avec
un minimum de valeurs aux feuilles. Un plateau peut engendrer de petits embranchements sous perturbation tout en
respectant cette borne ; extraire exactement le même nombre de modes exige une marge ou un élagage supplémentaire.

**Conditions non acquises dans le produit actuel.** K≤10 ne satisfait pas ce régime asymptotique. Une grille
fixe de 1 mm n'est pas un échantillon exact d'une densité continue lorsque n tend vers l'infini : les doublons et
l'échelle de quantification doivent entrer dans le modèle. Le modèle i.i.d., la dimension d et la mesure de
référence d'une trame LiDAR ne sont pas établis par l'emploi de coordonnées en trois dimensions. Enfin, la tête
double, EOM, cover et le remplissage ajoutent des transformations auxquelles la borne ci-dessus ne s'applique pas
sans contrôle distinct. Ce sont des obligations de modèle et de transfert, pas des défauts du calcul exact actuel.

## Pourquoi cover ne reçoit pas immédiatement ce transfert

Core prend l'appartenance de x à une composante du superniveau. Cover choisit une composante au premier rayon
couvrant α, puis la prolonge horizontalement. Le point peut alors être affecté à une composante dont il n'est pas
un point du superniveau. Les amas discrets couverts se recouvrent ; la prolongation choisit une seule branche.
Leur bonne laminarité comme projection ne les identifie donc pas au cluster tree plug-in ci-dessus.

Pour transférer un résultat statistique à cover, il faudrait borner l'écart de cette affectation et de ses masses
par rapport à l'appartenance ciblée, puis établir que la sélection reste stable sous cet écart. Les bornes
$d_K(x)/2\leq\alpha_K(x)\leq d_K(x)$, utiles géométriquement, ne donnent pas un écart tendant vers zéro en
elles-mêmes. Garder cover comme bras empirique et core comme projection de référence rend cette comparaison claire.

## Proposition bornée et réaliste pour la suite

Conserver core à K fixé comme référence exacte de points. Comparer sa connexité à RSL ou au témoin MR du dépôt
avec **la même convention k-NN incluant le site lui-même, la même entrée et la même tête**. Pour RSL, publier
$w_{\alpha,K}(x,y)=\max(r_K(x),r_K(y),\lVert x-y\rVert/\alpha)$ : son balayage reproduit le graphe à seuil r.
Le facteur α de cette définition doit être fixé explicitement ; il ne faut pas transférer un théorème RSL à une
variante dont l'API utilise un autre facteur ou l'ignore. Les garanties des articles servent à choisir une famille
de comparateurs et des diagnostics, elles ne désignent pas un α ou K optimal pour ces trames.

Si plusieurs K sont utilisés, une règle d'ensemble peut produire un seul arbre sans prétendre conserver toutes
les branches de la tour. Proposition mathématique à qualifier : pour chaque K, conserver la dissimilarité de
fusion core $a_K(x,y)$, qui comprend les entrées des deux points et la date de leur première composante commune.
À dimension d fixée dans le modèle, une échelle comparable est
$u_K(x,y)=(nv_d/K)^{1/d}\sqrt{a_K(x,y)}$. C'est une transformation monotone des niveaux ; elle ne reconstitue
pas la densité minimale d'un nœud depuis sa population. Pour les dissimilarités entre sites distincts, on fixe la
diagonale à zéro ; les entrées individuelles restent enregistrées séparément.

La médiane des $u_K(x,y)$ n'est pas en général ultramétrique. Une fermeture par les chemins,
$U(x,y)=\min_{x=x_0,\ldots,x_m=y}\max_i M(x_i,x_{i+1})$, où M est cette médiane, produit une ultramétrique et
donc des partitions emboîtées. C'est un nouveau modèle de consensus, avec le risque de chaînes d'arêtes, pas un
résultat automatique du FULL. La médiane seule ne suffit pas à conserver la laminarité.

**Stabilité géométrique déduite, sans hypothèse statistique de majorité.** Pour la restriction core exacte,
les mêmes observations étiquetées déplacées d'au plus ε, la même grille finie de K et les constantes
$c_K=(nv_d/K)^{1/d}$ communes et figées, la borne en rayon $|\Delta\sqrt{a_K(x,y)}|\leq2\varepsilon$
donne $\lVert M_X-M_Y\rVert_\infty\leq2\varepsilon\max_K c_K$. La médiane coordonnée et la fermeture
min–max sont chacune 1-Lipschitz ; ainsi $\lVert U_X-U_Y\rVert_\infty\leq2\varepsilon\max_K c_K$.
La diagonale reste nulle. Les entrées core, conservées séparément, changent elles aussi d'au plus
$2\varepsilon c_K$ : le site requête se déplace avec les observations. Cette dernière constante est atteinte
sur deux points à K2. Réestimer dimension ou échelle après perturbation ajoute un terme que cette preuve
ne contrôle pas ; les petits K peuvent fortement amplifier ε par leur normalisation.

Cette déduction contrôle des hauteurs dans le modèle de consensus déclaré. Elle ne garantit ni leur fidélité
à la densité vraie, ni l'identité de toutes les branches, ni la sélection EOM. Aucun prototype de consensus,
gain de score ou chemin produit sans matrice n² n'est qualifié ici.

Une garantie conditionnelle élémentaire est disponible : si une majorité stricte des arbres entiers est à distance
uniforme au plus η d'une même ultramétrique cible U*, alors M reste à distance au plus η de U*. La fermeture
ci-dessus fixe U* et est 1-Lipschitz pour la norme uniforme (max le long d'un chemin, puis min sur les chemins).
Donc $\lVert U-U^*\rVert_\infty\leq\eta$. La condition de majorité autour d'une cible n'est établie ni par
le simple changement de K, ni par un rééchantillonnage ; elle est l'obligation à tester ou prouver. Cette proposition
donne un objet laminaire vérifiable, sans promettre une robustesse statistique avant sa qualification.

Les premiers diagnostics utiles sont les erreurs de dates de fusion sur modèles à densité connue, la stabilité
des cols sous perturbations/rééchantillonnage, le maintien des modes avec une marge et la masse attribuée sous le
col. La fixture cross-K de [tower_point_crossing.py](../../probes/tower_point_crossing.py) vérifie le choix de laminarisation ;
elle ne teste aucune de ces garanties statistiques.
