# Résultat C — Échafaudage de Morse bifiltré en ordre et en échelle

> **Objet.** Définir la sortie mathématique commune aux hiérarchies HGP d'ordres $1\leq k\leq K_{\max}$, préciser ce que porte une sphère critique classée, et isoler les hypothèses sous lesquelles un catalogue fini d'événements reconstruit exactement $H_0$.
>
> **Conclusion.** L'objet minimal n'est ni une mosaïque, ni dix dendrogrammes indépendants. C'est une **tour de forêts de fusion graduées** reliées par les morphismes canoniques induits par $L(k+1,t)\subseteq L(k,t)$. En position générale, un catalogue commun de sphères de rang fermé au plus $K_{\max}+1$, muni des attaches globales de leurs bras et de l'incidence relative complète des niveaux égaux, présente toute cette tour en degré zéro.

## 1. Cadre et convention d'échelle

Soit $X=\left\lbrace x_1,\ldots,x_n\right\rbrace\subset\mathbb{R}^{d}$ un nuage fini. Pour $y\in\mathbb{R}^{d}$, notons

$$a_1(y)\leq\cdots\leq a_n(y)$$

les distances euclidiennes **au carré** de $y$ aux observations, comptées avec multiplicité. Pour $1\leq k\leq n$, le champ HGP d'ordre $k$ et son sous-niveau sont

$$D_k(y)=a_k(y),\qquad L(k,t)=\left\lbrace y\in\mathbb{R}^{d}:D_k(y)\leq t\right\rbrace,\qquad t\geq0.$$

La variable $t$ est donc un rayon au carré. Cette convention évite toute conversion silencieuse entre les rayons de la thèse et les poids internes.

Définissons le poset ordre–échelle

$$\mathcal{P}_{K_{\max}}=\left\lbrace1,\ldots,K_{\max}\right\rbrace^{\mathrm{op}}\times\mathbb{R}_{\geq0},$$

avec

$$(k,t)\preceq(\ell,u)\quad\Longleftrightarrow\quad k\geq\ell\ \text{ et }\ t\leq u.$$

### Proposition C.1 — bifiltration HGP

L'application $(k,t)\mapsto L(k,t)$ est un foncteur de $\mathcal{P}_{K_{\max}}$ vers les espaces topologiques et inclusions. En effet,

$$t\leq u\Longrightarrow L(k,t)\subseteq L(k,u),\qquad k\leq k+1\Longrightarrow L(k+1,t)\subseteq L(k,t).$$

Il en résulte deux foncteurs canoniques : le foncteur ensembliste des composantes $\Pi_0(k,t)=\pi_0(L(k,t))$ et, pour tout corps $\mathbb{F}$, le module bifiltré $\mathcal{H}_0(k,t)=H_0(L(k,t);\mathbb{F})$. Aucune régularisation et aucun choix algorithmique n'interviennent dans ces flèches.

À rayon $r=\sqrt{t}$, le théorème 2 du manuscrit identifie $\Pi_0(k,t)$ aux $k$-polyèdres de Čech, donc aux amas discrets de forte densité $k$-NN après application de la relation de couverture du manuscrit. Ainsi, travailler avec $L(k,t)$ ne change pas l'objet hiérarchique de la thèse.

## 2. Les tranches horizontales : forêts de fusion $T_k$

Fixons $k$. Pour $t\leq u$, l'inclusion induit une application

$$h^k_{t,u}:\Pi_0(k,t)\longrightarrow\Pi_0(k,u).$$

Une composante ne se scinde jamais lorsque $t$ croît : elle naît, persiste, puis fusionne avec d'autres composantes. Le système $\left(\Pi_0(k,t),h^k_{t,u}\right)$ possède donc une représentation par une forêt de fusion graduée $T_k$.

Une définition indépendante de toute discrétisation est la suivante. Les éléments de $T_k$ sont les paires $(C,t)$, avec $C\in\Pi_0(k,t)$, identifiées le long des intervalles réguliers où elles décrivent la même branche. L'ordre est

$$(C,t)\leq_{T_k}(C',u)\quad\Longleftrightarrow\quad t\leq u\ \text{ et }\ h^k_{t,u}(C)=C',$$

et la fonction de hauteur est $h_k(C,t)=t$. Pour un nuage fini, la filtration est définissable et possède un nombre fini de valeurs critiques ; après contraction des intervalles réguliers, $T_k$ est une forêt finie dont les arêtes portent des intervalles de hauteur.

Trois types de sommets suffisent en degré zéro :

- une **naissance**, où une nouvelle composante apparaît ;
- une **fusion post-lot**, éventuellement multifurquée ;
- une **subdivision marquée**, qui ne change pas $H_0$ mais porte un morphisme vertical ou un certificat.

Une selle d'indice $1$ qui ne fusionne aucune composante ne doit pas devenir un faux nœud de branchement. Elle peut néanmoins imposer une subdivision marquée dans $T_k$.

## 3. Les flèches verticales et leur naturalité

Pour $1\leq k<K_{\max}$, l'inclusion $L(k+1,t)\hookrightarrow L(k,t)$ envoie chaque composante dans une unique composante. Notons

$$v_{k,t}:\Pi_0(k+1,t)\longrightarrow\Pi_0(k,t)$$

cette application. Les inclusions commutent, donc, pour $t\leq u$,

$$h^k_{t,u}\circ v_{k,t}=v_{k,u}\circ h^{k+1}_{t,u}.$$

Cette égalité est le contrat central de la sortie. Elle interdit de calculer dix arbres puis de les apparier a posteriori par une similarité de clusters.

Après subdivision des arêtes aux hauteurs nécessaires, les $v_{k,t}$ se recollent en une application monotone préservant la hauteur

$$v_k:T_{k+1}\longrightarrow T_k.$$

La tour recherchée est donc

$$T_{K_{\max}}\xrightarrow{v_{K_{\max}-1}}T_{K_{\max}-1}\longrightarrow\cdots\xrightarrow{v_2}T_2\xrightarrow{v_1}T_1.$$

Cette orientation traduit la décroissance des sous-niveaux avec l'ordre. Une implémentation peut afficher les flèches dans l'autre sens, mais elle doit conserver ce sens sémantique.

## 4. Sphères critiques classées

Soit $c\in\mathbb{R}^{d}$ et $r\geq0$. Posons

$$I(c,r)=X\cap B^{\circ}(c,r),\qquad U(c,r)=X\cap\partial B(c,r),$$

et notons

$$i=\lvert I(c,r)\rvert,\qquad m=\lvert U(c,r)\rvert,\qquad s=i+m.$$

Le nombre $s$ est le **rang fermé** de la sphère. Supposons que les points soient en position générale au sens de la théorie de Morse du $k$-NN : $U$ est affine indépendant, $m\leq d+1$, et

$$c\in\mathrm{relint}\,\mathrm{conv}(U).$$

La caractérisation de Reani–Bobrowski donne alors le fait suivant.

### Théorème C.2 — plage d'ordres d'une sphère critique

La même sphère est critique pour tous les ordres

$$i<k\leq s,$$

et son indice à l'ordre $k$ vaut

$$\mu_k(c)=s-k.$$

En particulier :

- à l'ordre $k=s$, elle est d'indice $0$ et fait naître une composante ;
- à l'ordre $k=s-1$, elle est d'indice $1$ dès que $m\geq2$ ;
- aux ordres $k\leq s-2$, elle est d'indice au moins $2$ et ne modifie pas $H_0$.

La seconde ligne exige bien $m\geq2$. Le cas $r=0$, $I=\varnothing$ et $U=\left\lbrace x\right\rbrace$ est la naissance d'ordre $1$ portée par l'observation $x$ ; il n'a pas de rôle de selle.

La caractérisation et la formule d'indice sont locales et restent valides lorsque plusieurs points critiques ont la même valeur. Le théorème homologique global de Reani–Bobrowski est, lui, énoncé sous l'hypothèse de valeurs critiques distinctes. Les sections 5 et 6 ne prétendent pas supprimer cette hypothèse sans donnée supplémentaire : elles remplacent l'ordre séquentiel par l'incidence relative complète du lot.

### Corollaire C.3 — catalogue commun jusqu'à $K_{\max}$

Pour reconstruire les changements de $H_0$ de tous les ordres $1\leq k\leq K_{\max}$, il suffit de connaître toutes les sphères critiques de rang fermé

$$s\leq K_{\max}+1.$$

Les rangs $s\leq K_{\max}$ portent les naissances ; les rangs $2\leq s\leq K_{\max}+1$ portent les selles d'indice $1$ de l'ordre $s-1$. En dimension $3$, leur support frontière contient au plus quatre points, même lorsque $K_{\max}=10$.

Ce corollaire réduit la **taille géométrique d'un support**, mais ne borne pas le nombre de sphères peu profondes. La complétude de leur énumération reste un problème distinct.

## 5. Une selle est un hyperévénement d'attache, pas nécessairement une fusion

Considérons une sphère critique de rang $s$ à l'ordre

$$k=s-1,$$

et au niveau carré $t=r^2$. Pour chaque $u\in U$, la facette

$$Q_u=I\cup\bigl(U\setminus\left\lbrace u\right\rbrace\bigr)$$

possède exactement $k$ points. Elle représente un germe du sous-niveau strict au voisinage de la selle. Sous les hypothèses non dégénérées de Reani–Bobrowski, le sous-niveau local strict possède $m=\lvert U\rvert$ bras et la modification homologique locale totale a rang $m-1$.

Le théorème homologique local fixe seulement le nombre total $m-1$ de changements : il ne détermine pas leurs signes globaux, c'est-à-dire combien tuent des classes de $H_0$ et combien créent des classes de $H_1$. Il ne dit pas non plus quels bras sont déjà reliés loin de $c$. Il faut donc une application d'attache globale

$$\alpha_e:U\longrightarrow\Pi_0(k,t^-),$$

où $t^-\in(t-\varepsilon,t)$ et l'intervalle $(t-\varepsilon,t)$ ne contient aucune valeur critique. L'image de $u$ est la composante atteinte depuis le bras $Q_u$ par un chemin restant dans $L(k,t^-)$. Le choix précis de $t^-$ est sans effet tant que l'intervalle est régulier.

Posons

$$q_e=\left\lvert\alpha_e(U)\right\rvert.$$

Si $e$ est le seul événement au niveau $t$, son passage fusionne exactement les $q_e$ composantes distinctes incidentes et diminue donc le rang de $H_0$ de $q_e-1$. Comme la multiplicité locale totale vaut $m-1$, les $m-q_e$ changements restants créent des classes de $H_1$. Dans un lot de plusieurs événements, l'hyperarête de $e$ identifie encore ses $q_e$ sommets, mais sa contribution au rang n'est pas additive : des hyperarêtes du même lot peuvent être redondantes. Le support frontière seul ne permet jamais de déduire $q_e$. Trois cas doivent être distingués pour un événement isolé :

- $q_e=m$ : tous les bras sont globalement distincts avant la selle ;
- $1<q_e<m$ : la selle produit une fusion et une partie de son effet local concerne un degré supérieur ;
- $q_e=1$ : la selle ne change pas $H_0$, mais reste un événement critique et peut porter une flèche verticale.

Ainsi, « indice $1$ » et « arête conservée par Kruskal » ne sont pas synonymes. C'est précisément la partition des bras par $\alpha_e$ qui détermine l'effet sur la forêt de fusion.

### Statut de l'oracle d'attache

L'annexe B propose la descente K-NN–miniball à partir de chaque $Q_u$. Sous l'hypothèse $(\mathrm{GP}_{\partial}^{\mathrm{desc}})$, avec top-$k$ exact, miniballs exactes, traitement cohérent des ex æquo et minimum terminal présent dans le catalogue, cette descente fournit un chemin sous-niveau et donc la valeur exacte de $\alpha_e(u)$. Sans ces conditions, elle est un oracle conditionnel ; la seule validation du rang de la sphère ne certifie pas son attache.

## 6. Lots de niveaux égaux

L'hypothèse « toutes les valeurs critiques sont distinctes » simplifie les preuves mais n'est pas un contrat acceptable pour des données numériques. Pour un ordre $k$ et un niveau $t$, notons $\mathcal{E}_{k,t}$ l'ensemble de tous les événements dont le niveau certifié est $t$.

Choisissons $\varepsilon>0$ sans autre niveau critique dans $(t-\varepsilon,t+\varepsilon)$, et gelons, avec la convention $L(k,u)=\varnothing$ pour $u<0$,

$$\mathcal{C}^-_{k,t}=\Pi_0(k,t-\varepsilon).$$

Le **graphe d'incidence du lot** $\mathscr{H}_{k,t}$ est un hypergraphe dont :

- les sommets sont les composantes de $\mathcal{C}^-_{k,t}$ et les générateurs d'indice $0$ nés dans le lot ;
- chaque selle d'indice $1$ fournit une hyperarête incidente aux classes distinctes de ses bras ;
- dès que des naissances et des selles coexistent au même niveau, toutes les incidences relatives entre leurs générateurs de dimensions $0$ et $1$ sont ajoutées avant toute contraction ; cette donnée ne se déduit pas des seules attaches dans le sous-niveau strict.

Les composantes post-lot sont les composantes connexes de $\mathscr{H}_{k,t}$, auxquelles s'ajoutent les composantes gelées non incidentes. Si $V_{k,t}$ est l'ensemble de ses sommets incidents et $c_{k,t}$ le nombre de ses composantes connexes, le lot impose $\lvert V_{k,t}\rvert-c_{k,t}$ identifications. Cette quantité, et non la somme des $q_e-1$, donne le nombre de suppressions de générateurs après ajout simultané des éventuelles naissances du lot. La construction ne dépend ni de l'ordre des événements dans le lot ni de l'ordre des opérations parallèles.

Deux règles sont impératives :

1. toutes les attaches sont évaluées relativement au sous-niveau strict ou au complexe relatif complet du plateau ;
2. une seule multifurcation est créée par composante connexe du lot, même si plusieurs hyperarêtes redondantes la décrivent.

L'égalité de valeurs entre points critiques isolés et une dégénérescence géométrique ne sont pas la même difficulté. La première se traite par le lot ci-dessus lorsque son incidence relative est connue ; une cosphéricité qui viole la position générale peut changer le nombre de bras et sort du théorème C.2. Une perturbation symbolique peut remplacer cette incidence seulement si l'on prouve que la contraction des événements infinitésimaux redonne le même lot de l'entrée non perturbée. Un tri arbitraire des identifiants n'est pas une telle preuve.

## 7. L'ancre verticale naissance–selle

Soit $e$ une sphère de rang fermé $s$ et de niveau $t$. Elle porte simultanément :

- une naissance $b_e$ dans $T_s$ ;
- une selle $a_e$ dans $T_{s-1}$ lorsque son support contient au moins deux points.

Pour $0<\eta\ll\varepsilon$, l'inclusion

$$L(s,t+\eta)\subseteq L(s-1,t+\eta)$$

envoie la composante née en $b_e$ dans la composante **post-lot** de $T_{s-1}$ contenant les bras de $a_e$. C'est l'ancre canonique de $v_{s-1}$ à cet événement.

Si la selle ne fusionne rien, cette composante post-lot existe tout de même. Il faut alors subdiviser l'arête correspondante de $T_{s-1}$ à la hauteur $t$ afin de représenter l'ancre, sans créer un faux branchement.

Les identifiants partagés des sphères ne suffisent pas à définir toutes les flèches verticales. Après avoir placé les ancres :

1. on subdivise les arêtes cibles aux hauteurs des sommets sources ;
2. on envoie chaque composante source vers son unique composante contenante ;
3. aux fusions, l'image est l'ancêtre commun imposé par l'inclusion ;
4. on propage le long des arêtes par la relation de naturalité de la section 3.

La commutation en ordre et en échelle est alors une propriété vérifiable de la sortie, et non une convention d'affichage.

## 8. Objet de sortie minimal fidèle à $H_0$

Définissons la **forêt de fusion graduée HGP**

$$\mathsf{GMF}_{\leq K_{\max}}(X)=\left(\left(T_k,h_k\right)_{1\leq k\leq K_{\max}},\left(v_k\right)_{1\leq k<K_{\max}}\right).$$

À isomorphisme près, cet objet est la représentation minimale du foncteur $\Pi_0$ si chaque $T_k$ conserve :

- les hauteurs exactes des naissances et des fusions post-lot ;
- les multifurcations, sans binarisation artificielle ;
- les subdivisions nécessaires aux images verticales ;
- les applications $v_k$ sur ces sommets, étendues monotonement aux arêtes.

Le catalogue de sphères, les bras et les certificats d'attache constituent une **présentation vérifiable** de $\mathsf{GMF}$, mais ne font pas partie de son quotient minimal en degré zéro. Ils doivent néanmoins être conservés si l'on veut auditer la complétude, reprendre le calcul ou reconstruire les relations de couverture.

La hiérarchie discrète complète de la thèse contient davantage que $H_0$ : un $k$-polyèdre est l'union des identifiants des facettes de sa composante et plusieurs polyèdres peuvent se recouvrir. Pour restituer cette sortie, il faut ajouter à $\mathsf{GMF}$ une relation d'incidence entre composantes et observations. Cette relation n'est pas requise pour représenter le seul foncteur $H_0$.

## 9. Théorème de reconstruction conditionnelle

### Théorème C.4 — exactitude du squelette classé en degré zéro

Fixons $K_{\max}$. Supposons :

1. le nuage satisfait les hypothèses de position générale et de non-dégénérescence de la théorie de Morse du $k$-NN ;
2. toutes les sphères critiques de rang fermé au plus $K_{\max}+1$ sont énumérées ;
3. leur rang, leur support, leur niveau et leurs égalités de niveau sont décidés exactement ;
4. toutes les applications d'attache $\alpha_e$ sont exactes ;
5. les événements simultanés sont traités par leur graphe d'incidence relatif complet, y compris les incidences naissance–selle au même niveau ;
6. les ancres verticales et les inclusions de composantes sont calculées exactement.

Alors la construction des sections 5 à 8 produit un objet $\mathsf{GMF}_{\leq K_{\max}}(X)$ naturellement isomorphe au foncteur $(k,t)\mapsto\pi_0(L(k,t))$. En particulier, pour tout $(k,t)$, les composantes lues dans $T_k$ à la hauteur $t$ sont exactement celles de $L(k,t)$, et toutes les flèches des carrés ordre–échelle commutent.

**Justification.** Entre deux valeurs critiques, la filtration ne change pas d'homotopie. Les événements d'indice $0$ engendrent toutes les naissances de $H_0$ ; ceux d'indice $1$, munis de leurs attaches globales, engendrent toutes ses fusions ; les indices supérieurs ne changent pas $H_0$. Le corollaire C.3 partage ces événements entre tous les ordres. À niveau distinct, ceci est la conséquence directe du théorème de Morse cité. À niveau égal, l'hypothèse 5 fournit précisément la donnée relative supplémentaire nécessaire : sa contraction simultanée donne les bonnes composantes post-lot sans attribuer artificiellement les morts événement par événement. Les inclusions $L(k+1,t)\subseteq L(k,t)$ imposent ensuite les morphismes verticaux et leur naturalité.

Ce théorème est exact **après** les six hypothèses. Il n'est pas une preuve que le catalogue peut être énuméré en temps quasi linéaire, ni que la descente d'attache termine sur des données dégénérées.

## 10. Ce qui est démontré et ce qui reste à construire

| énoncé | statut |
|---|---|
| $L(k,t)$ est une bifiltration et induit les $v_{k,t}$ | exact, élémentaire |
| les composantes de $L(k,t)$ coïncident avec les $k$-polyèdres de la thèse | démontré dans le manuscrit, théorème 2 |
| caractérisation des sphères critiques et formule $\mu_k=s-k$ | démontré sous position générale par Reani–Bobrowski |
| rang $s$ : naissance à l'ordre $s$, selle à l'ordre $s-1$ | corollaire direct, avec $m\geq2$ pour la selle |
| les rangs $s\leq K_{\max}+1$ suffisent à tous les changements de $H_0$ | démontré sous les hypothèses de Morse |
| une selle fusionne les classes distinctes de ses bras | exact une fois les attaches globales connues |
| descente K-NN–miniball pour calculer chaque attache | démontrée dans l'annexe B sous $(\mathrm{GP}_{\partial}^{\mathrm{desc}})$ et oracles exacts ; sinon conditionnelle |
| valeurs critiques égales sous position générale | exact si l'incidence relative complète du lot est fournie ; cette incidence reste à calculer |
| cosphéricités et autres violations de la position générale | hors du théorème C.2 ; traitement à construire et à prouver |
| fermeture complète du catalogue 3D par diagrammes de puissance ordinaires | démontrée sous $(\mathrm{GP}_{3})$ et oracles exacts dans le résultat D |
| énumération 3D sensible aux seuls événements $H_0$ | ouverte ; la fermeture complète peut avoir une sortie intermédiaire quadratique |
| vrai complexe de chaînes de Morse bifiltré dans tous les degrés | hors du besoin $H_0$ ; demanderait les indices supérieurs et leurs applications d'attache |

Le terme **échafaudage de Morse bifiltré** est donc préférable à « complexe de Morse » tant que seules les données de degré zéro et les attaches d'indice $1$ sont construites.

## 11. Oracles compatibles avec un traitement massivement parallèle

La structure mathématique ne dépend pas du GPU. Elle se réduit toutefois à quatre familles d'oracles batchables :

1. certification du centre, du rayon, du support et du rang fermé de sphères candidates ;
2. calcul indépendant des bras et de leurs attaches sous-niveau ;
3. tri certifié des niveaux, construction des lots et composantes d'hypergraphes ;
4. localisation des composantes contenantes pour les morphismes verticaux.

Ces oracles peuvent être parallélisés sans changer l'objet. En revanche, une exécution concurrente des unions ne doit jamais décider l'ordre mathématique des événements égaux.

## Références primaires

- L. Hauseux, *Détection d'amas et interactions d'ordre supérieur*, chapitres 6 à 8 : régions témoins, $k$-polyèdres, adjacences élémentaires, simplexes séparants et $k$-MST.
- Y. Reani et O. Bobrowski, [*Morse Theory for the k-NN Distance Function*](https://arxiv.org/abs/2403.12792), 2024 : caractérisation des points critiques, indice et effet homologique.
- E. Corbet, M. Kerber, M. Lesnick et G. Osang, [*The Multi-Cover Persistence of Euclidean Balls*](https://doi.org/10.1137/19M1264207), 2021 : contexte de la bifiltration de multicoverture.
