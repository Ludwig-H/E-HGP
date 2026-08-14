# Branche HGP marquée et carriers non convexes

## Correction de l'interprétation

La proposition étudiée comporte bien deux canaux distincts :

- un canal de support normalisé $\widetilde{h}_v$, qui expose rapidement l'enveloppe convexe et les directions extrêmes ;
- un canal structurel $\mathcal{P}_v$, qui conserve la présentation HGP marquée, la géométrie du carrier choisi, ses incidences et ses marques de filtration.

Le payload marqué est un objet combinatoire et n'est lui-même ni convexe ni non convexe. Ce qualificatif appartient au carrier géométrique qu'il reconstruit. Le second canal n'est ni une seconde fonction support, ni une fonction radiale. La critique du rayon extérieur ne réfute donc pas cette proposition ; elle ne concerne qu'une ablation qui compresserait immédiatement $\mathcal{P}_v$ en un scalaire par direction.

Le terme `polytope` désigne usuellement un objet convexe. Le dossier emploie plutôt **composante HGP marquée**, **carrier polyédral** ou **union témoin**, selon l'objet exact.

## Objet source et incidence HGP

Soient $X\subset\mathbb{R}^{3}$ fini non vide, $1\leq K\leq|X|$ et un niveau carré $a\geq0$. Pour tout $S\subseteq X$ non vide, poser $\beta(S)=\min_{y\in\mathbb{R}^{3}}\max_{x\in S}\left\Vert y-x\right\Vert^{2}$. Une facette active est un ensemble $F\subseteq X$ de cardinal $K$ tel que $\beta(F)\leq a$. Une coface élémentaire active est un ensemble $Q\subseteq X$ de cardinal $K+1$ tel que $\beta(Q)\leq a$.

Le graphe source $\Gamma_{K}^{\mathrm{full}}(a)$ relie deux facettes actives distinctes $F,F'$ lorsque leur union entière est un simplexe de Čech actif, soit $\beta(F\cup F')\leq a$. Le sous-graphe $\Gamma_{K}^{\mathrm{elem}}(a)$ ne garde que les arêtes telles que $|F\cup F'|=K+1$ ; chacune est alors témoignée par la coface élémentaire $Q=F\cup F'$.

On a $\Gamma_{K}^{\mathrm{elem}}(a)\subseteq\Gamma_{K}^{\mathrm{full}}(a)$ et seulement l'égalité de leurs composantes connexes. En effet, pour toute arête full, une suite de remplacements d'un sommet transforme $F$ en $F'$ ; tous les $K$-sous-ensembles intermédiaires et leurs unions consécutives sont actifs par hérédité de l'activité de Čech. Cette chaîne est un chemin dans $\Gamma_{K}^{\mathrm{elem}}(a)$. Elle ne reconstruit pas les arêtes full : deux facettes reliées par un chemin élémentaire n'ont pas nécessairement une union active.

Pour une composante commune $v$, noter $\mathcal{F}_v(a)$ ses facettes, $\mathcal{Q}_v^{\mathrm{elem}}(a)$ ses cofaces élémentaires de connexion et $V_v(a)=\bigcup_{F\in\mathcal{F}_v(a)}F$. Le **$K$-polyèdre source** du manuscrit est l'ensemble discret $V_v(a)$, pas un solide.

La présentation minimale marquée pour conserver la connectivité d'ordre supérieur est

$$G_v^{\mathrm{inc}}(a)=\left(V_v\sqcup\mathcal{F}_v\sqcup\mathcal{Q}_v^{\mathrm{elem}},E_{V\!F},E_{FQ}^{\mathrm{elem}},m_v\right),$$

où $E_{V\!F}$ code $x\in F$, $E_{FQ}^{\mathrm{elem}}$ code $F\subset Q$ et $m_v$ conserve les coordonnées, $K$, $a$, les niveaux exacts $\beta(F)$ et $\beta(Q)$, les identifiants canoniques et les marques de naissance, mort ou persistance disponibles. Les cofaces élémentaires reconstruisent $\Gamma_{K}^{\mathrm{elem}}$, jamais l'adjacence de $\Gamma_{K}^{\mathrm{full}}$. Si cette dernière est consommée, le contrat doit fournir ses arêtes et niveaux, ou un oracle certifié qui reteste $\beta(F\cup F')$ ; il est interdit de la déduire des seules cofaces de cardinal $K+1$.

Ce graphe d'incidence conserve un certificat élémentaire exact de $H_0$, car les deux graphes ont le même $\pi_0$. Il ne conserve ni toute l'adjacence full ni automatiquement le nerf complet des régions témoins : un claim sur leurs trous ou leur homotopie exige les intersections d'arité supérieure correspondantes, ou directement leur géométrie. Un graphe $\Gamma$ seul perd même l'incidence point--facette.

## Coupe versionnée d'une branche persistante

Un nœud persistant ne détermine pas un payload statique : ses facettes, cofaces et témoins évoluent entre naissance et fusion. Le payload est donc attaché à une arête hiérarchique et non au seul identifiant de nœud. Pour une arête $p\leftarrow v$, la baseline fixe `cut_policy_version=1`, `cut_policy=pre_parent`, `cut_level=a_p` et `cut_side=strict` : elle lit l'état de $v$ juste avant le lot de fusion de $p$, avec les cellules satisfaisant $\beta<a_p$. L'embedding enfant--parent est ainsi conditionné par cette coupe.

Une racine sans parent utilise `cut_policy=explicit`, le dernier niveau fini enregistré, `cut_side=closed` et $\beta\leq a_{\mathrm{terminal}}$. Le format sérialise aussi naissance, mort, liste ordonnée des niveaux d'événements et deltas de facettes, cofaces et incidences. Un nœud né et fusionné dans le même lot rend la coupe stricte vide : il est rejeté ou conservé sous une politique explicite de lot d'événements, jamais converti silencieusement en coupe fermée. Toute autre politique parmi `pre_parent|post_birth|explicit` reçoit les valeurs de niveau et de côté correspondantes et ne partage pas un hash de configuration avec la baseline.

## Trois carriers géométriques dérivés

### Carrier des facettes

Le carrier PL des facettes est

$$C_v^{F}(a)=\bigcup_{F\in\mathcal{F}_v(a)}\mathrm{conv}(F).$$

Il est généralement non convexe. Pour $K>4$ dans $\mathbb{R}^{3}$, $\mathrm{conv}(F)$ est un polytope potentiellement dégénéré, pas la réalisation injective d'un simplexe abstrait de dimension $K-1$. Deux cellules peuvent aussi s'intersecter autrement que par une face commune ; `complexe simplicial plongé` est donc interdit sans certificat de conformité ou subdivision.

Son support vérifie exactement

$$h_{C_v^{F}}(u)=\max_{x\in V_v}\langle u,x\rangle=h_{V_v}(u).$$

Cette identité ne rend pas $C_v^{F}$ inutile : le carrier complet conserve les recollements et l'intérieur de chaque cellule, tandis que le canal support fournit seulement un raccourci dérivable de ses sommets.

### Carrier des cofaces

Le carrier des cofaces élémentaires est

$$C_v^{Q}(a)=\bigcup_{Q\in\mathcal{Q}_v^{\mathrm{elem}}(a)}\mathrm{conv}(Q).$$

Il épaissit les connexions qui ont effectivement soudé les facettes. Il peut différer fortement de $C_v^{F}$ et devient vide pour une composante réduite à une facette isolée. Une variante augmentée ajoute explicitement ces facettes isolées ; le contrat doit dire laquelle est utilisée. Poser $U_v^{Q}=\bigcup_{Q\in\mathcal{Q}_v^{\mathrm{elem}}}Q$. Lorsque $\mathcal{Q}_v^{\mathrm{elem}}\neq\varnothing$, on a exactement $h_{C_v^{Q}}=h_{U_v^{Q}}$, puis $h_{C_v^{Q}}=h_{V_v}$ si et seulement si $\mathrm{conv}(U_v^{Q})=\mathrm{conv}(V_v)$. La couverture $U_v^{Q}=V_v$ est suffisante, mais pas nécessaire. Le cas vide et toute politique d'augmentation possèdent une sémantique de support séparée.

### Union témoin canonique

Pour chaque facette, définir sa région témoin

$$T_a(F)=\bigcap_{x\in F}\overline{B}\left(x,\sqrt{a}\right).$$

L'objet continu canonique associé à la composante est

$$W_v(a)=\bigcup_{F\in\mathcal{F}_v(a)}T_a(F).$$

Les $T_a(F)$ sont compacts et convexes. On a $T_a(F)\cap T_a(F')=T_a(F\cup F')$ ; cette intersection est non vide exactement lorsque $\beta(F\cup F')\leq a$. Ainsi, $\Gamma_{K}^{\mathrm{full}}(a)$ est littéralement le graphe d'intersection des témoins. Une intersection dont l'union contient plus de $K+1$ sommets n'est pas une arête de $\Gamma_{K}^{\mathrm{elem}}(a)$, mais la chaîne de remplacements ci-dessus préserve $\pi_0$. Par conséquent, les deux graphes indexent les mêmes unions connexes $W_v(a)$.

En posant $L_K^{X}(a)=\left\lbrace y\in\mathbb{R}^{3}:\left|X\cap\overline{B}(y,\sqrt{a})\right|\geq K\right\rbrace$, on obtient $L_K^{X}(a)=\bigcup_{F\in\mathcal{F}_K(a)}T_a(F)$ et chaque $W_v(a)$ est exactement une de ses composantes connexes, pas un proxy PL. Il vit dans l'espace des centres possibles de boules couvrant au moins $K$ observations ; il décrit donc un niveau de densité, pas nécessairement la surface physique de l'objet LiDAR.

$W_v(a)$ est en général curviligne et non polyédral. Surtout, aucune identité $h_{W_v}=h_{V_v}$ n'est vraie en général. La fixture `support(carrier) == support(vertices)` s'applique à $C_v^{F}$, et sous sa condition de couverture à $C_v^{Q}$, jamais à $W_v$.

## Schéma contractuel du canal 2

Trois axes orthogonaux sont versionnés dans chaque expérience :

- `payload_kind=marked_incidence` désigne les tables finies de points, facettes, cofaces élémentaires, incidences et marques ; il ne désigne pas un carrier et ne porte aucun qualificatif de convexité ;
- `carrier_kind=source_points|facet_pl|coface_pl|witness_union` fixe l'objet géométrique effectivement interrogé ;
- `authority=incidence_complete|pl_complete|witness_exact|witness_approx|h0_only` borne exactement ce que le producteur certifie.

`coface_pl` sérialise en plus sa politique d'isolés. `witness_approx` enregistre la métrique, la tolérance $\varepsilon_W$ et l'oracle de comparaison ; il n'est jamais accepté sous `witness_exact`. La branche principale recommandée conserve le payload `marked_incidence` et compare `facet_pl` à `witness_union`. Le premier représente directement les observations et leurs interactions ; le second est l'objet continu exact de la théorie HGP. Les combiner est autorisé, mais leur gain et leur coût doivent être isolés.

## Complétude relative et état de v3

`Complet` signifie complet **relativement au carrier déclaré et à la composante demandée**, jamais matérialisation du complexe de Čech ambiant, de $\Gamma_K$ global ou d'une mosaïque de Delaunay d'ordre supérieur. Les cellules canoniques peuvent être stockées une seule fois par scan, puis référencées par les nœuds de la hiérarchie.

Une présentation sparse est recevable seulement avec l'une des autorités suivantes :

- `incidence_complete` : elle reconstruit le même $G_v^{\mathrm{inc}}$ marqué et son graphe élémentaire ; toute autorité sur l'adjacence full reste un champ explicite supplémentaire ;
- `pl_complete` : elle reconstruit exactement le même $C_v^{F}$ ou $C_v^{Q}$ ;
- `witness_exact` : elle reconstruit exactement le même $W_v$ ;
- `witness_approx` : elle fournit une borne $\varepsilon_W$ déclarée contre un oracle exact borné ;
- `h0_only` : elle certifie seulement la même composante et la même union d'observations.

Le dernier statut ne suffit pas à alimenter une branche géométrique complète. Deux sources ayant le même $H_0$ peuvent avoir des incidences, des carriers PL et des unions témoins différents.

Au `HEAD` de cette étude, le cadre v3 reste `public_status=not_claimed` et son audit live est antérieur au `HEAD`. Surtout, la voie v3 ne persiste pas un payload composante-local complet de facettes, cofaces et incidences ; ses certificats sparse actuels visent principalement la réduction hiérarchique. Le canal 2 exige donc un nouvel exporteur composante-local certifié. Il ne peut pas être reconstruit depuis la seule forêt de points et ne doit pas réintroduire sous un autre nom l'oracle Gamma exhaustif dans le chemin produit.

## Encodeur de référence

1. Le backbone produit une feature locale $f_x$ par observation et conserve un chemin de sortie point-wise.
2. Des tokens de facettes sont formés par une fonction symétrique de leurs sommets, avec géométrie normalisée, matrice de Gram, $\beta(F)$ et marques HGP.
3. Des tokens de cofaces représentent les connexions, leur niveau $\beta(Q)$, leurs facettes incidentes et leur géométrie.
4. Un encodeur de type hypergraphe invariant au réindexage échange des messages `point ↔ facette ↔ coface`. Il ne s'agit pas d'un complexe de cochaînes lorsque les rangs intermédiaires manquent. Une variante Hodge/cochaîne n'est admise que si tous les rangs de $0$ à $K$ sont exportés, les orientations sont contractuelles et les cobords vérifient $d_{j+1}d_j=0$.
5. Pour `witness_union`, poser $D_{K,v}(y)=\min_{F\in\mathcal{F}_v}\max_{x\in F}\left\Vert y-x\right\Vert$. Alors $y\in W_v(a)$ si et seulement si $D_{K,v}(y)\leq\sqrt{a}$. Le seul $K$-ième voisin global teste $y\in L_K^{X}(a)$, pas l'appartenance à la composante $v$. Les requêtes de distance ou patches de frontière conservent cette contrainte de composante ; toute discrétisation rapporte son erreur et son coût.
6. Les tokens ou un pooling injectif sur une classe bornée produisent le contexte $g_v$. Le support normalisé constitue une voie globale courte, puis $g_v$ décore les relations parent--enfant utilisées par HSA ou son contrôle par message passing.
7. Le contexte redescend vers les points et est fusionné avec le skip local ; aucune prédiction finale n'est constante sur tout le cluster.

La complexité de l'encodeur d'incidence dépend du payload composante-local effectivement parcouru, typiquement $|V_v|+|\mathcal{F}_v|+|\mathcal{Q}_v^{\mathrm{elem}}|+|E_{V\!F}|+|E_{FQ}^{\mathrm{elem}}|$, et non d'un catalogue ambiant supposé. Le stockage partagé doit éviter de recompter une même cellule à chaque ancêtre. Pour `witness_union`, rapporter en plus $N_W$, nombre de requêtes d'appartenance, distance, frontière ou patches effectivement évalués, ainsi que $\varepsilon_W$, le temps et la mémoire de leur validation contre l'oracle borné.

## Programme théorique T0--T6

Les sept énoncés T0 à T6 — sémantique des carriers, invariance aux certificats équivalents, conscience des incidences, équivariance et stabilité, composition sparse, couplage avec l'attention et recouvrements — sont énoncés une seule fois, dans [THEOREMES.md](THEOREMES.md). Ne pas les redupliquer ici : une seule version fait foi.

## Fixtures permanentes

- mêmes observations et même support, incidences différentes ;
- singleton $K=1$ à $a>0$ : $C_v^{F}=\left\lbrace x\right\rbrace$ mais $W_v(a)=\overline{B}(x,\sqrt{a})$, donc supports différents ;
- carrier des facettes contre carrier des cofaces ;
- carrier PL contre $W_v$, avec supports différents ;
- coface absente, facette isolée et politique d'augmentation ;
- même composante $H_0$ mais couvertures témoins différentes ;
- deux certificats sparse équivalents contre un certificat seulement `h0_only` ;
- replay `cut_policy=pre_parent`, `cut_side=strict` des deltas, racine explicite fermée et plateaux à égalité ;
- cube plein contre frontière pour l'ablation radiale ;
- rayon extérieur non binné comme témoin d'une violation du seul recentrage : $g$ continu, agrégation exacte par $\max$, et pourtant $\left\lbrace c+e_{1}\right\rbrace$ et $\left\lbrace c+e_{2}\right\rbrace$ valent tous deux $1$ au centre $c$ puis $2$ et $\sqrt{2}$ au centre $c-e_{1}$ ;
- carrier PL à $K=3$ dont la CDF de volume est identiquement nulle, contre la même CDF pondérée par l'aire des triangles ;
- recouvrements $K\geq2$ avant et après laminarisation ;
- même patch sous homothétie, puis sous transport et rééchantillonnage LiDAR.

Toute collision qui invalide un claim devient une fixture permanente ; elle n'est pas retirée lorsque l'encodeur change.

## Ablations et falsifications obligatoires

- `support seul`, `complexe seul` et `support + complexe` ;
- carrier des facettes, carrier des cofaces et union témoin, sans mélanger leurs claims ;
- points seuls, multiensemble de cellules sans incidence, $\Gamma_{K}^{\mathrm{elem}}$ seul, adjacence $\Gamma_{K}^{\mathrm{full}}$ contrôlée et incidence complète ;
- incidence correcte contre incidence permutée à degrés et marques appariés ;
- même support et mêmes observations, mais recollements différents ;
- singleton $K=1$ à $a>0$, dont le carrier de facette est un point et l'union témoin une boule ;
- suppression séparée des cofaces, niveaux, géométrie et marques de filtration ;
- présentation complète contre deux certificats sparse équivalents, puis contre un certificat seulement `h0_only` ;
- coupe `cut_policy=pre_parent`, `cut_side=strict` rejouée depuis les deltas, racine `cut_policy=explicit`, `cut_side=closed` et mutant qui change silencieusement le côté d'un plateau ;
- HGP contre objet aléatoire apparié en cellules, degrés et niveaux ;
- fonction radiale extérieure uniquement comme ablation lossy ;
- qualité, collisions, RAM, VRAM et latence selon cellules, incidences, portée et thinning.

## Verdict corrigé

Cette proposition est substantiellement plus forte que `support + rayon extérieur` : le payload conserve la structure qui permet de reconstruire le carrier non convexe déclaré, là où le premier canal convexifie. Elle est cohérente et peut soutenir un papier. Elle ne garantit pas le SOTA : le canal peut être redondant avec un backbone fort, sensible à l'échantillonnage LiDAR ou trop coûteux à exporter et encoder.

Pour CVPR, le gain causal face aux encodeurs de points, superpoints et complexes doit être net à coût complet. Pour ICML/NeurIPS, appliquer un réseau simplicial existant ne suffit pas ; le résultat central devrait combiner au moins T3, T4, T5 ou T6 avec une validation multi-capteurs.
