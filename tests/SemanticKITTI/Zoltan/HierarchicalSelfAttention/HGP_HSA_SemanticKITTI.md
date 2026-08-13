# Hypothèse HGP-HSA révisée

Cette note remplace l'esquisse initiale. Elle conserve l'intuition — exploiter toute la hiérarchie HGP sans choisir prématurément une coupe — mais sépare les hypothèses qui doivent être testées.

## Objet étudié

Soit un scan LiDAR $X=\left\lbrace x_i \in \mathbb{R}^{3}\right\rbrace_{i=1}^{N}$ muni de la rémission par point et d'une hiérarchie $K$-NN/HGP fournie. Ici $K$ désigne l'ordre HGP, $d_{\mathrm{geo}}$ la distance employée par le reconstructeur et $k_{\mathrm{local}}$ un éventuel voisinage du backbone. Pour la présente étude, le producteur de hiérarchie est supposé disponible, déterministe et indépendant des labels.

HSA requiert une forêt enracinée et laminaire. Le contrat d'entrée doit donc livrer une relation parent–enfant sans ambiguïté et un mapping stable des points vers les feuilles. Si une construction HGP d'ordre supérieur produit des appartenances ponctuelles qui se chevauchent, une projection laminaire ou un modèle DAG devra être déclaré comme tel ; elle ne pourra pas être appelée silencieusement « arbre HGP exact ».

## Couple de représentations central

L'hypothèse proposée est le couple $\mathcal{R}_v=(s_v,\mathcal{P}_v)$. Le premier canal $s_v$ est la fonction support normalisée des observations du cluster, résumé global de leur enveloppe convexe. Le second canal $\mathcal{P}_v$ est un **objet HGP marqué complet relativement au contrat**, fini et de taille variable : points, facettes de cardinal $K$, cofaces élémentaires de cardinal $K+1$, incidences, géométrie et niveaux de filtration. Le payload n'est pas lui-même qualifié de non convexe ; son `carrier_kind` déclare une réalisation qui peut l'être. Il est traité comme un hypergraphe typé avant toute réduction et, sous l'autorité requise, permet de reconstruire le carrier déclaré.

L'objet complet contient ses observations et détermine donc leur support. Le couple n'est pas plus injectif que $\mathcal{P}_v$ seul ; le rôle possible du support est computationnel, en exposant immédiatement les directions extrêmes dans un vecteur fixe. Cela ne signifie pas que le support du porteur continu HGP soit celui des observations. L'ablation centrale est ainsi `objet HGP complet seul` contre `support + objet HGP complet`, et non `support` contre `support + rayon`.

## Descripteur de support

Pour un nœud non dégénéré $v$, de centre $c_v$, de rayon $R_v>0$ et pour une direction $u_j$, le canal de support maximal est défini par $s_v(j)=\max_{x\in C_v}\left\langle u_j,\frac{x-c_v}{R_v}\right\rangle$. Si $R_v=0$, le contrat fixe le vecteur à zéro, active un masque `degenerate` et encode le log-rayon par une valeur plancher masquée ; aucune invariance exacte n'est revendiquée pour ce cas numérique particulier.

Ce canal est invariant par translation et homothétie positive si le centre et le rayon suivent les mêmes transformations. Il n'est pas invariant à une rotation arbitraire sur une grille fixe de directions. À centre et rayon fixés, la **fonction continue sur toutes les directions** détermine exactement l'enveloppe convexe normalisée, mais pas les trous, concavités, composantes intérieures ni densités. Un vecteur fini de directions ne détermine même pas toute cette enveloppe : il n'en est qu'un sketch. Si le centre ou le rayon dépend de tous les points, l'intérieur peut modifier indirectement le sketch par ces deux statistiques, sans rendre la représentation injective.

La normalisation indépendante des nœuds détruit également la composition hiérarchique directe. Dans un repère commun, $h_{A\cup B}(u)=\max\left(h_A(u),h_B(u)\right)$ ; dans des repères normalisés séparément, il faut au moins conserver le déplacement relatif des centres et le rapport d'échelle.

L'hypothèse crédible n'est donc pas « le support suffit », mais :

> Le support normalisé fournit un raccourci convexe global, tandis que l'encodeur de l'objet HGP marqué conserve les incidences et les données nécessaires au carrier déclaré, potentiellement non convexe, pour représenter structures minces, recollements, cavités et frontières.

## Fonction radiale : ablation lossy, pas second canal

La quantité obtenue en cherchant, sur le rayon $c_v+ru$, la plus grande norme admissible est la fonction radiale extérieure $\rho_{P,c_v}(u)=\sup\left(\left\lbrace r\geq0:c_v+ru\in P\right\rbrace\cup\left\lbrace0\right\rbrace\right)$, et non une fonction support. Un masque distingue les directions sans intersection. Elle dépend du centre et reconstruit exactement $P$ seulement si $P$ est étoilé par rapport à ce centre. Dans le cas général, relier $c_v$ à chaque point radial extrême remplit les intervalles absents et perd précisément les trous, les composantes et certaines concavités recherchées.

Conserver toutes les intersections le long de chaque rayon est plus informatif, mais donne une sortie variable et de nombreux rayons vides pour certaines réalisations. Ces limites concernent une compression de $\mathcal{P}_v$ ou de son porteur, pas l'objet marqué complet lui-même. Rayon, ECT/WECT, distributions d'attributs et CDF sont des contrôles de readout à comparer à l'encodeur d'incidence complet. Les définitions et contre-exemples des compressions sont centralisés dans [GEOMETRIC_DESCRIPTOR_AUDIT.md](GEOMETRIC_DESCRIPTOR_AUDIT.md).

## Sémantique d'un cluster

Un cluster ne reçoit jamais un label unique. En excluant les labels ignorés, sa cible est l'histogramme normalisé $\pi_v\in\Delta^{18}$. À l'inférence, les feuilles prédisent $p_i\in\Delta^{18}$ puis le nœud déduit sans masque GT $\widehat\pi_v^{\mathrm{all}}=n_v^{-1}\sum_{i\in C_v}p_i$. Dans une hiérarchie laminaire, cette distribution est exactement la moyenne des distributions enfants pondérée par leurs masses ; aucune tête indépendante n'est requise. Une version restreinte aux labels valides sert seulement à la loss et au diagnostic, jamais au forward de validation/test.

Les proportions ne localisent pas les classes au sein d'un cluster mixte. La sortie officielle demeure point-wise, à partir des features de feuille et du contexte top-down. Le vecteur du cluster est un résumé sémantique multiscale et une cible auxiliaire, pas une prédiction uniforme diffusée aux points.

## Deux graphes et quatre carriers à ne pas confondre

À un niveau $a=r^2$, les sommets de $\Gamma_K^{\mathrm{full}}(a)$ sont les facettes actives $F$ de cardinal $K$, avec une arête $F\sim F'$ si $\beta(F\cup F')\leq a$. Le sous-graphe $\Gamma_K^{\mathrm{elem}}(a)$ impose en plus $|F\cup F'|=K+1$ ; ses arêtes sont encodées par les cofaces élémentaires $\mathcal{Q}^{\mathrm{elem}}$. Une arête complète se remplace par une chaîne d'échanges d'un sommet parmi les $K$-sous-ensembles du simplexe actif $F\cup F'$. Ainsi $\Gamma_K^{\mathrm{full}}$ et $\Gamma_K^{\mathrm{elem}}$ ont les mêmes composantes $H_0$, mais ni les mêmes arêtes ni la même géométrie. Les seules cofaces élémentaires ne reconstruisent pas l'adjacence complète.

Soit $\mathcal{A}_v(a)$ une de ces composantes communes et $V_v=C_v=\bigcup_{F\in\mathcal{A}_v(a)}F$. Quatre carriers distincts en découlent.

1. Le **$K$-polyèdre source** est $V_v$, l'union finie des observations apparaissant dans la composante.
2. Le **carrier PL des facettes** est $C_v^{F}(a)=\bigcup_{F\in\mathcal{A}_v(a)}\mathrm{conv}(F)$. Il peut être non convexe, mais sa fonction support est exactement celle de $V_v$ ; cette identité réfute seulement l'emploi du support de $C_v^{F}$ comme second canal.
3. Le **carrier PL des cofaces** est $C_v^{Q}(a)=\bigcup_{Q\in\mathcal{Q}_v^{\mathrm{elem}}(a)}\mathrm{conv}(Q)$, avec ajout des facettes isolées uniquement si le profil le déclare. Lorsqu'il n'est pas vide, son support exact est $h_{C_v^{Q}}(u)=\max_{Q\in\mathcal{Q}_v^{\mathrm{elem}}}\max_{x\in Q}\langle u,x\rangle$ ; il égale $h_{V_v}(u)$ lorsque les cofaces conservées couvrent $V_v$. Le cas vide est encodé séparément.
4. L'**union témoin** est $W_v(a)=\bigcup_{F\in\mathcal{A}_v(a)}\bigcap_{x\in F}\overline{B}(x,\sqrt{a})$. Le graphe d'intersection des témoins est exactement $\Gamma_K^{\mathrm{full}}(a)$ ; les composantes égales de $\Gamma_K^{\mathrm{elem}}(a)$ donnent donc le même découpage $H_0$. Sous le contrat de toutes les facettes actives de la composante, $W_v(a)$ est la composante canonique de $L_K(a)$ associée à $V_v$. Il est généralement curviligne plutôt que polyédral, et son support n'est pas en général celui de $V_v$.

L'« équivalent polyédral complet » voulu doit donc préciser le carrier. Le schéma commun fixe `payload_kind=marked_incidence`, `carrier_kind=source_points|facet_pl|coface_pl|witness_union` et `authority=incidence_complete|pl_complete|witness_exact|witness_approx|h0_only`. `witness_exact` certifie toutes les facettes, coordonnées et la coupe nécessaires ; `witness_approx` versionne la méthode et $\varepsilon_W$ ; `h0_only` ne satisfait pas le loader du canal marqué. Le certificat fini conserve incidences point–facette, cofaces élémentaires, coordonnées, niveaux, naissances et plateaux. Il suffit à reconstruire $W_v(a)$ seulement si l'autorité `witness_exact` est établie ; un simple squelette de fusion ne l'implique pas.

La géométrie d'une composante persistante évolue entre naissance et mort. La baseline définit donc le readout par arête $p\leftarrow v$ avec `cut_policy=pre_parent`, `cut_level=a_p` et `cut_side=strict`, soit les événements $\beta<a_p$ juste avant le lot de fusion du parent. Une racine utilise `cut_policy=explicit` au dernier niveau fini sérialisé et `cut_side=closed`. Naissance, mort, deltas ordonnés, traitement des égalités et version de politique entrent dans le hash ; un état strict vide ou de persistance nulle est représenté explicitement ou rejeté.

Pour $K\geq2$, des composantes distinctes peuvent partager des points. Cette multiplicité est une information d'ordre supérieur à conserver dans $\mathcal{P}_v$. HSA reste d'abord sur $K=1$ ou une laminarisation documentée. Une projection recouvrante ne devient admissible qu'après définition d'un ownership $w_{iv}$ sur un domaine explicite, avec $w_{iv}\geq0$, $\sum_{v\in\mathcal{A}(i)}w_{iv}=1$, conservation de masse, absence de double comptage et invariance à l'ordre ; une partition de l'unité annoncée sans ces tests n'est pas une implémentation.

Le contrat amont n'est pas encore satisfait par MorseHGP3D v3. Le chemin réduit documenté produit des composantes horizontales, niveaux et unions de `PointId`, sans payload facetté de Gamma ; la sortie exhaustive facettes/cofaces/incidences demeure un oracle borné et non une route produit. Le prototype doit donc soit attendre une sérialisation sparse certifiée, soit employer un reconstructeur de recherche séparé en déclarant son coût et son statut. Aucun de ces choix ne change `public_status=not_claimed`.

L'encodeur initial est un hypergraphe typé point–facette–coface élémentaire. Il construit les tokens à partir des features de leurs points, de leur géométrie normalisée, de leur filtration et, pour `carrier_kind=witness_union`, de résumés des régions témoins. Ce payload saute des rangs pour certains $K$ : aucune variante Hodge/cochaîne n'est valide sans exporter tous les rangs de $0$ à $K$ et vérifier $\partial_{r-1}\partial_r=0$. Les tokens ne sont pas remplacés dès l'entrée par leur moyenne. Un readout invariant au réindexage fournit le contexte de branche, tandis que les incidences permettent de le renvoyer aux points. Toute condensation doit déclarer les cellules et incidences supprimées et mesurer les collisions induites. Les coûts rapportent $N_W$, nombre de requêtes ou éléments témoins évalués, et $\varepsilon_W$, erreur contre l'oracle ou le carrier selon une métrique versionnée.

Le support maximal reste un chemin global court. CDF, moments, distributions d'attributs cellulaires, ECT/WECT, rayon et mini-PointNet/Deep Sets sont des contrôles de compression à budget égal. Les comparaisons prioritaires sont `points seuls`, $\Gamma_K^{\mathrm{elem}}$ seul, $\Gamma_K^{\mathrm{full}}$ lorsqu'il est disponible, `objet d'incidence complet`, puis `support + objet complet`.

## Trois hypothèses causales

### H1 — valeur de la hiérarchie

À opérateur et descripteur constants, HGP d'ordre $K=2,3$ améliore le mIoU et la robustesse à longue portée par rapport à RSL/HDBSCAN, octree, hiérarchie de voxels, superpoints et arbres aléatoires contrôlés. Le cas $K=1$, égal au single-linkage, sert de fixture de cohérence et non de baseline indépendante.

### H2 — valeur de la représentation

À arbre et budget constants, l'encodeur de $\mathcal{P}_v$ et du carrier déclaré apporte une information utile au-delà des mêmes points, de $\Gamma_K^{\mathrm{elem}}$ seul, de $\Gamma_K^{\mathrm{full}}$ lorsqu'il est disponible, d'un mini-PointNet et de contrôles de complexes appariés. Il doit distinguer des objets ayant mêmes points et même support mais des incidences ou régions témoins différentes. Le support n'est conservé en complément que si son raccourci améliore le Pareto qualité–coût.

### H3 — valeur de l'opérateur hiérarchique

À arbre et features constants, HSA ou un raffinement conditionné par la requête exploite mieux les interactions entre échelles qu'un passage bottom-up/top-down par MLP ou qu'un message passing parent–enfant/frères. `QC-HSA` doit en outre battre HSA aux frontières à coût réel documenté ; sinon elle reste un diagnostic technique.

Une expérience finale ne permet pas d'identifier ces trois effets. Chacun exige son ablation orthogonale.

## Deux mappings à ne pas confondre

### Variante A — HSA fidèle, recommandée en premier

- points ou micro-voxels comme feuilles ;
- features locales fortes Q/K/V aux feuilles ;
- support et readout du complexe HGP marqué transformés en un embedding pour chaque enfant dans le domaine de son parent ;
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
2. un encodeur d'hypergraphe typé point–facette–coface élémentaire construit les états de l'objet HGP, puis une agrégation bottom-up construit les statistiques des nœuds ;
3. un bloc hiérarchique initial, éventuellement deux, propage le contexte ; un sweep séparé teste 0/1/2/4 blocs ;
4. une passe top-down renvoie le contexte aux feuilles ;
5. un décodeur point-fin combine ce contexte avec un skip local ;
6. une tête produit 19 logits par point.

L'objectif d'entraînement principal est **exactement celui de la baseline reproduite** pour toutes les variantes appariées, y compris CE+Lovász si cette recette en contient déjà. Une loss auxiliaire de proportions peut ensuite comparer $\pi_v$ à l'agrégat restreint aux points valides et imposer la cohérence massique parent–enfants, avec une pondération empêchant le surcomptage des mêmes points aux différentes profondeurs. Toute loss additionnelle ne sera ouverte qu'après l'ablation de la structure, afin de ne pas confondre gain architectural et recette d'entraînement.

## Extension instance différée

L'instance n'est pas un objectif de la phase actuelle. L'interface conserve toutefois les logits/features par point, la topologie, les appartenances et les attributs HGP. Après validation sémantique seulement, une tête pourra scorer des nœuds et sélectionner une antichaîne avec corrections split/merge. Elle sera comparée à ALPINE sur les mêmes logits sémantiques gelés.

## Critère de réussite scientifique

Le succès n'est pas seulement un mIoU élevé. Il faut montrer un gain apparié et reproductible du complexe et de l'arbre HGP, une explication des classes/distances qui en bénéficient, une robustesse à l'amincissement LiDAR, et un coût complet compatible avec l'usage. Si le complexe n'apporte rien au-delà des points ou d'un graphe de contrôle, si le support est redondant en pratique, si un agrégateur plus simple égale HSA, ou si une hiérarchie géométrique conventionnelle égale HGP, le claim doit être réduit en conséquence.
