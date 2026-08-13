# Hypothèse HGP-HSA révisée

Cette note remplace l'esquisse initiale. Elle conserve l'intuition — exploiter toute la hiérarchie HGP sans choisir prématurément une coupe — mais sépare les hypothèses qui doivent être testées.

## Objet étudié

Soit un scan LiDAR $X=\left\lbrace x_i \in \mathbb{R}^{3}\right\rbrace_{i=1}^{N}$ muni de la rémission par point et d'une hiérarchie $K$-NN/HGP fournie. Ici $K$ désigne l'ordre HGP, $d_{\mathrm{geo}}$ la distance employée par le reconstructeur et $k_{\mathrm{local}}$ un éventuel voisinage du backbone. Pour la présente étude, le producteur de hiérarchie est supposé disponible, déterministe et indépendant des labels.

HSA requiert une forêt enracinée et laminaire. Le contrat d'entrée doit donc livrer une relation parent–enfant sans ambiguïté et un mapping stable des points vers les feuilles. Si une construction HGP d'ordre supérieur produit des appartenances ponctuelles qui se chevauchent, une projection laminaire ou un modèle DAG devra être déclaré comme tel ; elle ne pourra pas être appelée silencieusement « arbre HGP exact ».

## Descripteur de support

Pour un nœud non dégénéré $v$, de centre $c_v$, de rayon $R_v>0$ et pour une direction $u_j$, le canal de support maximal est défini par $s_v(j)=\max_{x\in C_v}\left\langle u_j,\frac{x-c_v}{R_v}\right\rangle$. Si $R_v=0$, le contrat fixe le vecteur à zéro, active un masque `degenerate` et encode le log-rayon par une valeur plancher masquée ; aucune invariance exacte n'est revendiquée pour ce cas numérique particulier.

Ce canal est invariant par translation et homothétie positive si le centre et le rayon suivent les mêmes transformations. Il n'est pas invariant à une rotation arbitraire sur une grille fixe de directions. À centre et rayon fixés, la **fonction continue sur toutes les directions** détermine exactement l'enveloppe convexe normalisée, mais pas les trous, concavités, composantes intérieures ni densités. Un vecteur fini de directions ne détermine même pas toute cette enveloppe : il n'en est qu'un sketch. Si le centre ou le rayon dépend de tous les points, l'intérieur peut modifier indirectement le sketch par ces deux statistiques, sans rendre la représentation injective.

La normalisation indépendante des nœuds détruit également la composition hiérarchique directe. Dans un repère commun, $h_{A\cup B}(u)=\max\left(h_A(u),h_B(u)\right)$ ; dans des repères normalisés séparément, il faut au moins conserver le déplacement relatif des centres et le rapport d'échelle.

L'hypothèse crédible n'est donc pas « le support suffit », mais :

> Un sketch directionnel de masse, combinant CDF/histogrammes projetés, quantiles robustes et canal de support maximal, apporte une information de forme complémentaire lorsque le modèle conserve explicitement l'échelle, la densité, la portée, la persistance HGP et la géométrie relative des branches.

## Sémantique d'un cluster

Un cluster ne reçoit jamais un label unique. En excluant les labels ignorés, sa cible est l'histogramme normalisé $\pi_v\in\Delta^{18}$. À l'inférence, les feuilles prédisent $p_i\in\Delta^{18}$ puis le nœud déduit sans masque GT $\widehat\pi_v^{\mathrm{all}}=n_v^{-1}\sum_{i\in C_v}p_i$. Dans une hiérarchie laminaire, cette distribution est exactement la moyenne des distributions enfants pondérée par leurs masses ; aucune tête indépendante n'est requise. Une version restreinte aux labels valides sert seulement à la loss et au diagnostic, jamais au forward de validation/test.

Les proportions ne localisent pas les classes au sein d'un cluster mixte. La sortie officielle demeure point-wise, à partir des features de feuille et du contexte top-down. Le vecteur du cluster est un résumé sémantique multiscale et une cible auxiliaire, pas une prédiction uniforme diffusée aux points.

## Complément géométrique prioritaire au support

La réalisation géométrique $|P_v|$ du $K$-polyèdre est l'union des réalisations de ses simplexes. Si $X_v$ est l'union de leurs sommets, alors $h_{|P_v|}(u)=h_{X_v}(u)$ dans toute direction : une forme linéaire atteint son maximum sur un sommet de chaque simplexe. Le support de la réalisation est donc exactement le descripteur initial du nuage du cluster. Il est HGP-friendly et fusionnable par maximum, mais il efface précisément l'ordre $K$, les incidences, les multiplicités et les niveaux de filtration ; seul, il est insuffisant pour viser le SOTA.

Pour exploiter réellement la réalisation HGP, définir plutôt une mesure sur ses simplexes et agréger leurs centres, aires/volumes ou longueurs, formes, niveaux $\beta(\sigma)$ et multiplicités. Les CDF projetées ou moments de ces attributs sont le candidat HGP-spécifique ; le support reste leur canal d'extrêmes.

Le support maximal ne conserve que l'extrémité de la distribution projetée. Le complément naturel est un sketch des CDF $F_v(u,t)$ des projections normalisées, échantillonné sur les mêmes directions et sur des seuils fixes. La collection continue de toutes les distributions projetées détermine la mesure du cluster par Cramér–Wold, alors que le support continu ne détermine que son enveloppe convexe. Une grille finie de directions et de bins n'est évidemment qu'une approximation.

Le premier descripteur à tester est donc `max support + projected CDF/histograms + radial occupancy + covariance`, complété par échelle, centre relatif, cardinalité, rémission et attributs HGP de naissance/mort/persistance. Les histogrammes à bins fixes sont préférables comme état fusionnable ; les quantiles sont robustes mais ne se composent pas exactement sans conserver un sketch plus riche.

## Trois hypothèses causales

### H1 — valeur de la hiérarchie

À opérateur et descripteur constants, HGP d'ordre $K=2,3$ améliore le mIoU et la robustesse à longue portée par rapport à RSL/HDBSCAN, octree, hiérarchie de voxels, superpoints et arbres aléatoires contrôlés. Le cas $K=1$, égal au single-linkage, sert de fixture de cohérence et non de baseline indépendante.

### H2 — valeur du support

À arbre, dimension et budget constants, le sketch directionnel de masse — CDF/histogrammes projetés, quantiles et canal de support maximal — ajoute une information utile au-delà des moments, de la covariance, d'un mini-PointNet ou d'un simple pooling des features de feuilles.

### H3 — valeur de l'opérateur hiérarchique

À arbre et features constants, HSA ou sa relaxation `QC-HSA` exploite mieux les interactions entre échelles qu'un passage bottom-up/top-down par MLP ou qu'un message passing parent–enfant/frères. `QC-HSA` doit en outre battre HSA aux frontières à coût réel documenté.

Une expérience finale ne permet pas d'identifier ces trois effets. Chacun exige son ablation orthogonale.

## Deux mappings à ne pas confondre

### Variante A — HSA fidèle, recommandée en premier

- points ou micro-voxels comme feuilles ;
- features locales fortes Q/K/V aux feuilles ;
- descripteurs HGP/support transformés en un embedding pour chaque enfant dans le domaine de son parent ;
- proportions aux nœuds déduites récursivement des distributions de feuilles ;
- sorties point par point ;
- blocs HSA dans les couches moyennes ou tardives seulement.

Cette variante reste proche du formalisme du papier NeurIPS 2025 si l'énergie entre frères utilise le produit scalaire de leurs embeddings par enfant, comme dans le papier. Pour un arbre donné, avec les projections Q/K LayerNormées, l'énergie quadratique, la température et le rescaling spécifiés, le théorème minimise la somme par ligne $\sum_i D_{\mathrm{KL}}\left(\theta^{\mathrm{HSA}}_i\,\Vert\,\theta^{\mathrm{flat}}_i\right)$ sur les matrices stochastiques satisfaisant la contrainte de blocs. La cible plate est celle construite avec la même énergie et les mêmes positions, pas une attention Softmax arbitraire de PTv3. Le résultat porte sur les poids d'attention ; il ne couvre pas automatiquement V, gate, MLP, tokens internes ou scores pairwise libres, et ne prouve ni que l'arbre HGP est le bon arbre ni que la segmentation sera meilleure.

### Variante A2 — QC-HSA, résultat technique candidat

Pour chaque feuille requête, les sous-arbres frères rencontrés jusqu'à la racine partitionnent les autres feuilles. `QC-HSA` impose des poids constants dans chacun de ces groupes, mais ne lie pas les lignes de deux requêtes différentes. Sa projection reverse-KL fermée optimise une famille contenant la famille HSA et ne peut donc pas être plus éloignée de la même attention plate. Elle est exacte lorsque le score d'une paire est constant sur le couple de branches de fusion pertinent. Cette propriété générique à tout arbre est achetée par davantage d'interactions et ne constitue pas encore un théorème central ; les détails et le pont conditionnel vers HGP sont dans [THEOREM_PROGRAM.md](THEOREM_PROGRAM.md).

### Variante B — tokens pour tous les nœuds

- chaque nœud reçoit un état appris ;
- attention ou message passing entre parent, enfants, frères et éventuellement voisins de frontière ;
- fusion top-down vers les points.

Cette variante est un nouveau Tree Transformer. Elle peut être plus expressive, mais le résultat théorique de HSA ne s'y applique pas automatiquement. Elle ne sera ouverte que si la variante fidèle établit que la structure HGP est utile.

## Architecture sémantique minimale

1. un encodeur local point/voxel calcule des features haute résolution ;
2. une agrégation bottom-up construit les statistiques des nœuds ;
3. un bloc hiérarchique initial, éventuellement deux, propage le contexte ; un sweep séparé teste 0/1/2/4 blocs ;
4. une passe top-down renvoie le contexte aux feuilles ;
5. un décodeur point-fin combine ce contexte avec un skip local ;
6. une tête produit 19 logits par point.

L'objectif d'entraînement principal est **exactement celui de la baseline reproduite** pour toutes les variantes appariées, y compris CE+Lovász si cette recette en contient déjà. Une loss auxiliaire de proportions peut ensuite comparer $\pi_v$ à l'agrégat restreint aux points valides et imposer la cohérence massique parent–enfants, avec une pondération empêchant le surcomptage des mêmes points aux différentes profondeurs. Toute loss additionnelle ne sera ouverte qu'après l'ablation de la structure, afin de ne pas confondre gain architectural et recette d'entraînement.

## Extension instance différée

L'instance n'est pas un objectif de la phase actuelle. L'interface conserve toutefois les logits/features par point, la topologie, les appartenances et les attributs HGP. Après validation sémantique seulement, une tête pourra scorer des nœuds et sélectionner une antichaîne avec corrections split/merge. Elle sera comparée à ALPINE sur les mêmes logits sémantiques gelés.

## Critère de réussite scientifique

Le succès n'est pas seulement un mIoU élevé. Il faut montrer un gain apparié et reproductible de l'arbre HGP, une explication des classes/distances qui en bénéficient, une robustesse à l'amincissement LiDAR, et un coût complet compatible avec l'usage. Si un agrégateur plus simple égale HSA, ou si une hiérarchie géométrique conventionnelle égale HGP, le claim doit être réduit en conséquence.
