# Résultat E — Récupération condensée de la tour HGP par certificats de coupes

> **Objet.** Donner une notion de récupération sparse qui porte directement sur la hiérarchie $H_0$, et non sur une erreur de champ, un poids total d'arbre ou une partition finale.
>
> **Conclusion.** Un atlas incomplet peut restituer exactement les branches de persistance supérieure à un seuil $\pi$ s'il certifie leurs événements, leurs attaches, les coupes qui fixent leurs niveaux minimax et leurs porteurs verticaux, puis exclut par blocs toute naissance absente susceptible de vivre plus de $\pi$. Tant que ce dernier oracle global n'est pas fermé, le résultat reste conditionnel ; ni un fort rappel ANN, ni la stabilité empirique de l'atlas ne le remplacent.

## 1. Ce que signifie « condensé »

Une sortie condensée n'est pas :

- une sélection de minima sans leurs niveaux de fusion ;
- une collection de paires de persistance sans relation d'ascendance ;
- dix arbres simplifiés indépendamment ;
- une partition obtenue en coupant un dendrogramme.

Elle doit conserver quatre informations :

1. les branches longues et leurs niveaux de naissance et de mort ;
2. l'emboîtement de ces branches dans chaque ordre ;
3. les événements simultanés comme multifurcations ;
4. les morphismes verticaux entre ordres adjacents.

Nous partons donc de la forêt de fusion graduée du résultat C,

$$\mathsf{GMF}=\left(\left(T_k,h_k\right)_{1\leq k\leq K_{\max}},\left(v_k:T_{k+1}\to T_k\right)_{1\leq k<K_{\max}}\right).$$

## 2. Niveaux de connexion minimax

### 2.1 Présentation par un graphe pondéré

À un ordre $k$ fixé, soit $G_k=(V_k,E_k)$ une présentation exacte de la filtration de composantes :

- chaque sommet $u\in V_k$ est un atome ou une facette, de niveau de naissance $\beta_k(u)$ ;
- chaque arête $e=uv\in E_k$ représente une rencontre, de niveau $w_k(e)$ ;
- un sommet ou une arête est actif au seuil $t$ lorsque son niveau ne dépasse pas $t$.

Le poids pertinent pour la filtration doit inclure l'activation de ses extrémités. Nous utiliserons donc partout le poids effectif

$$\omega_k(uv)=\max\left\lbrace w_k(uv),\beta_k(u),\beta_k(v)\right\rbrace.$$

Dans les présentations géométriques exactes, on a normalement déjà $w_k(uv)\geq\max\left\lbrace\beta_k(u),\beta_k(v)\right\rbrace$, donc $\omega_k=w_k$. Cette normalisation est indispensable pour une présentation abstraite : une forêt minimum calculée avec les seuls $w_k$ peut préférer un chemin passant par un sommet qui n'est pas encore né et ne pas préserver la filtration.

Dans la présentation complète de la thèse, les sommets sont les facettes de cardinal $k$ et les adjacences élémentaires sont portées par les cofaces de cardinal $k+1$. Dans un atlas de blocs, $V_k$ est seulement la famille de faces déclarée et $E_k$ peut être implicite.

Une rencontre multiway est représentée soit par un sommet d'événement relié à tous ses bras, soit par des arêtes de poids égal contractées dans un même lot. Les arguments de forêt et de coupe ci-dessous s'appliquent à ce développement graphique ; ils ne doivent pas être appliqués directement à un hypergraphe sans préciser son développement.

Pour un chemin $P=(u_0,e_1,u_1,\ldots,e_m,u_m)$, définissons son niveau d'activation

$$\Lambda_k(P)=\max_{1\leq j\leq m}\omega_k(e_j),$$

avec $\Lambda_k((u))=\beta_k(u)$ pour le chemin trivial réduit à un sommet.

Le niveau auquel deux sommets deviennent connectés est

$$\lambda_k(u,v)=\min_{P:u\leadsto v}\Lambda_k(P).$$

Plus généralement, pour deux ensembles de sommets $A,B\subseteq V_k$,

$$\lambda_k(A,B)=\min_{u\in A,\,v\in B}\lambda_k(u,v).$$

Ces nombres décrivent exactement la hiérarchie : $u$ et $v$ appartiennent à la même composante au seuil $t$ si et seulement si $\lambda_k(u,v)\leq t$. Ils sont donc plus pertinents que le poids total d'un arbre couvrant.

### 2.2 Propriété minimax d'une forêt minimum

Si $M_k$ est une forêt minimum couvrante de $G_k$ pour les poids effectifs $\omega_k$, alors, pour deux sommets d'une même composante du graphe non filtré, le chemin unique $P_{M_k}(u,v)$ vérifie

$$\Lambda_k\bigl(P_{M_k}(u,v)\bigr)=\lambda_k(u,v).$$

Ainsi, conserver dans $M_k$ les sommets tels que $\beta_k\leq t$ et les arêtes telles que $\omega_k\leq t$ restitue toutes les composantes de $G_k$ au même seuil. Plusieurs forêts minimum peuvent exister ; elles donnent néanmoins le même foncteur de composantes si les arêtes de niveaux égaux sont contractées par lots.

## 3. Atlas incomplet et exactitude relative

Soit $\mathcal{A}$ un atlas, fermé par les faces nécessaires aux ordres inférieurs, et notons $G_k^{\mathcal{A}}$ sa présentation à l'ordre $k$. Nous supposons ici qu'il s'agit d'une sous-présentation de $G_k$ à poids hérités : l'atlas supprime des sommets ou des rencontres, mais n'en invente pas et ne modifie pas leurs niveaux. Pour deux sommets présents dans l'atlas,

$$\lambda_k(u,v)\leq\lambda_k^{\mathcal{A}}(u,v),$$

car supprimer des sommets ou des rencontres ne peut qu'ajourner une connexion. L'atlas peut donc :

- manquer une naissance globale ;
- créer une branche artificiellement longue ;
- retarder une fusion réelle ;
- envoyer une branche verticale vers une composante trop petite.

Le calcul est **exact sur atlas** s'il retrouve toutes les valeurs $\lambda_k^{\mathcal{A}}$ et tous les morphismes de la fermeture descendante déclarée. Cette propriété ne dit rien sur l'égalité $\lambda_k^{\mathcal{A}}=\lambda_k$.

Un atlas doit être identifié par son contenu sémantique : famille de blocs, fermeture par faces, règle des rencontres implicites et version des poids. Le statut `atlas_exact` n'est pas transférable à un atlas plus grand ou au champ global.

## 4. Condensation hiérarchique

### 4.1 Branches et persistance

Fixons une politique de l'aîné $\mathfrak{e}$ sur l'univers global des événements de naissance, puis restreignons-la à chaque atlas. Elle compare d'abord les niveaux de naissance. En cas d'égalité, on peut soit conserver toute la classe d'aînés, soit employer une priorité globale déclarée ; cette convention n'ordonne jamais les opérations à l'intérieur du lot et ne binarise pas ses multifurcations. Une branche $b$ de $T_k$ naît au niveau $\mathrm{birth}(b)$ et meurt au premier lot où sa composante rejoint une classe plus ancienne selon $\mathfrak{e}$.

Cette compatibilité globale est nécessaire pour comparer la persistance d'une branche entre un atlas et la présentation complète. La topologie de la forêt et les lots ne dépendent pas de $\mathfrak{e}$ ; seules la décomposition en branches et la sélection au seuil $\pi$ en dépendent. La politique choisie fait donc partie du périmètre publié.

Une branche essentielle qui ne meurt pas reçoit $\mathrm{death}(b)=+\infty$ et est toujours retenue.

Dans la variable de filtration choisie ici, la persistance est

$$\mathrm{pers}(b)=\mathrm{death}(b)-\mathrm{birth}(b).$$

Elle est exprimée en distance au carré. Une version relative ou logarithmique peut être ajoutée comme autre politique, mais le seuil $\pi$ et tous les certificats doivent alors utiliser cette même variable.

### 4.2 Fermeture verticale du squelette persistant

Soit $\mathcal{R}_{\pi}$ l'ensemble des branches telles que

$$\mathrm{pers}(b)>\pi.$$

La **condensation hiérarchique** $\mathsf{C}_{\pi}(\mathsf{GMF})$ est obtenue ainsi :

1. marquer les extrémités de naissance et de mort de toutes les branches de $\mathcal{R}_{\pi}$ ;
2. ajouter les multifurcations et ancêtres communs nécessaires à leur emboîtement ;
3. ajouter, pour chaque sommet marqué de $T_{k+1}$, son image dans $T_k$ à la même hauteur ;
4. répéter la fermeture par images verticales et ancêtres jusqu'à stabilisation ;
5. conserver la sous-forêt minimale reliant ces marques aux racines, puis supprimer les sommets de degré $2$ qui ne portent ni hauteur critique, ni lot, ni flèche verticale.

Une image verticale peut donc forcer la conservation d'un sommet de Steiner appartenant à une branche de persistance inférieure à $\pi$. Ce sommet ne devient pas une branche significative ; il est conservé parce qu'il porte la structure de la tour.

La sortie condensée contient encore des applications

$$v_k^{(\pi)}:T_{k+1}^{(\pi)}\longrightarrow T_k^{(\pi)}$$

qui commutent avec l'ascendance et les hauteurs. Elle est une sous-présentation comprimée du foncteur hiérarchique, pas un ensemble de caractéristiques indépendantes.

## 5. Certificats de coupes fondamentales

### 5.1 Coupe d'une arête retenue

Soit $M_k^{\mathcal{A}}$ une forêt couvrante candidate de l'atlas. Pour une arête $e$ de cette forêt, supprimer $e$ sépare son arbre en deux ensembles $S_e$ et $V_k^{\mathcal{A}}\setminus S_e$. Pour tester cette décision contre le graphe global, il faut exhiber une extension $\overline{S}_e\subseteq V_k$ telle que

$$\overline{S}_e\cap V_k^{\mathcal{A}}=S_e.$$

Tous les sommets globaux absents de l'atlas doivent donc être placés d'un côté de la coupe ou couverts implicitement par l'oracle. La coupe globale à certifier est

$$\partial\overline{S}_e=\left\lbrace f=uv\in E_k:u\in\overline{S}_e,\ v\notin\overline{S}_e\right\rbrace.$$

Son niveau optimal dans le graphe global vaut

$$\gamma_k(\overline{S}_e)=\min_{f\in\partial\overline{S}_e}\omega_k(f).$$

### Proposition E.1 — certificat exact d'une coupe

Si $e$ traverse une coupe globale et

$$\omega_k(e)=\gamma_k(\overline{S}_e),$$

alors $e$ est sûre pour au moins une forêt minimum globale. Si une forêt couvre le graphe global et si l'égalité est certifiée sur la coupe fondamentale de chacune de ses arêtes, cette forêt est minimum et ses chemins réalisent tous les niveaux minimax globaux.

Pour une forêt seulement définie sur l'atlas, des certificats arête par arête ne suffisent pas à affirmer la seconde conclusion. Les extensions $\overline{S}_e$ des coupes retenues doivent être compatibles avec les partitions de composantes du squelette global, ou bien les niveaux minimax correspondants doivent être certifiés directement. Cette compatibilité fait partie de la $\pi$-complétude de la section 7.

Pour une sortie condensée, il n'est pas nécessaire de certifier le poids total de toutes les arêtes. Il faut certifier au minimum :

- chaque arête ou hyperarête qui tue une branche retenue ;
- les arêtes qui fixent son parent et son emboîtement ;
- les coupes ajoutées par fermeture verticale ;
- les arêtes susceptibles de créer une branche de persistance supérieure à $\pi$ dans l'atlas mais pas globalement.

### 5.2 Forme calculable d'un certificat

Supposons qu'un oracle fournisse, pour une extension globale $\overline{S}_e$ :

- un intervalle $[L(e),U(e)]$ contenant le poids effectif $\omega_k(e)$ du candidat ;
- pour toute rencontre explicite concurrente et toute famille non parcourue qui traverse la coupe, un minorant de ses poids ;
- un minimum $L_{\mathrm{alt}}(\overline{S}_e)$ de tous ces minorants hors du lot candidat.

La décision est certifiée lorsque

$$U(e)<L_{\mathrm{alt}}(\overline{S}_e).$$

Si plusieurs arêtes exactes ont le même niveau, elles doivent être réunies dans un même lot ; on certifie alors que le prochain niveau strictement supérieur est séparé du lot. Le choix d'un arbre à l'intérieur du lot n'a aucune signification hiérarchique.

Un voisinage ANN ne fournit que des majorants candidats. Sans minorant sur les rencontres absentes qui traversent la coupe, il ne constitue pas un certificat.

## 6. Les quatre marges qui rendent la conclusion robuste

Les marges suivantes portent sur un squelette cible fini $\Sigma_{\pi}$ : événements des branches longues, lots qui les tuent, attaches, coupes et porteurs verticaux ajoutés par la fermeture de la section 4.

### 6.1 Marge d'événements

Les événements exactement égaux sont groupés dans un même lot. La marge entre lots distincts est

$$\Delta_{\mathrm{evt}}=\min_{e,f\in\Sigma_{\pi},\,[e]\neq[f]}\left\lvert t_e-t_f\right\rvert,$$

où $[e]$ désigne le lot de $e$. Cette marge garantit que des intervalles numériques assez fins ne renversent pas l'ordre de deux lots pertinents.

La distance au seuil de condensation est mesurée séparément :

$$\Delta_{\pi}=\min_{b\in\Sigma_{\pi},\,\mathrm{pers}(b)<+\infty}\left\lvert\mathrm{pers}(b)-\pi\right\rvert.$$

Si $\Delta_{\pi}=0$, l'appartenance au squelette condensé doit être déclarée indécidable à la précision courante ou résolue par une convention exacte.

Le minimum d'une famille vide est pris égal à $+\infty$ dans cette section.

### 6.2 Marge de coupe

Pour une coupe fondamentale dont le lot retenu a niveau $t_e$, retirons toutes les rencontres du même lot et posons

$$\Delta_{\mathrm{cut}}(e)=\min_{f\in\partial\overline{S}_e,\,[f]\neq[e]}\omega_k(f)-t_e.$$

Une marge positive sépare la décision hiérarchique du prochain concurrent. Elle n'exige pas l'unicité d'une arête à l'intérieur du lot.

### 6.3 Marge d'attache

Soit $e$ une selle de niveau $t_e$, de bras $A_1,\ldots,A_m$, et soit $\sim_e$ la partition correcte de ces bras dans le sous-niveau strict. Retirons du graphe toutes les rencontres du lot $[e]$. Une marge d'attache $\Delta_{\mathrm{att}}(e)>0$ est certifiée si :

$$A_i\sim_e A_j\Longrightarrow\lambda_k^{\neg[e]}(A_i,A_j)\leq t_e-\Delta_{\mathrm{att}}(e),$$

et

$$A_i\not\sim_e A_j\Longrightarrow\lambda_k^{\neg[e]}(A_i,A_j)\geq t_e+\Delta_{\mathrm{att}}(e).$$

La première inégalité demande un chemin témoin sous le niveau ; la seconde demande un certificat de séparation, typiquement par une coupe. Si un autre événement au même niveau relie les bras, il appartient au même lot et doit être remis avant de calculer ses composantes connexes.

### 6.4 Marge d'ordre

Soit $z$ un sommet marqué de $T_{k+1}$ à la hauteur $t$, et soit $C=v_k(z)$ sa composante porteuse post-lot dans $T_k$. Une preuve d'ordre contient :

- un témoin d'inclusion plaçant la composante source dans $C$ ;
- la fermeture par faces qui rend ce témoin présent à l'ordre $k$ ;
- un certificat que $C$ ne rejoint aucune autre composante post-lot avant $t+\Delta_{\mathrm{ord}}(z)$.

En notation minimax, la dernière condition est

$$\min_{C'\neq C}\lambda_k(C,C')\geq t+\Delta_{\mathrm{ord}}(z),$$

où les $C'$ parcourent les autres composantes immédiatement après le lot de niveau $t$. Les événements appartenant au lot ont déjà été contractés. Cette marge localise l'image verticale sur une arête cible non ambiguë ; elle ne remplace pas la preuve d'inclusion.

Dans la suite, $\Delta_{\mathrm{cut}}$, $\Delta_{\mathrm{att}}$ et $\Delta_{\mathrm{ord}}$ désignent les minima des marges correspondantes sur le squelette fini $\Sigma_{\pi}$.

## 7. Complétude au-dessus de $\pi$

Les certificats précédents valident des décisions proposées. Ils ne prouvent pas qu'aucune branche longue n'a été entièrement oubliée. Il faut une hypothèse ou un oracle supplémentaire.

### 7.1 Monotonie qui permet un certificat sparse

Supposons que $G_k^{\mathcal{A}}$ soit une sous-présentation du graphe global, avec les mêmes niveaux sur les sommets et rencontres conservés. Pour une naissance globale $b$ représentée dans l'atlas, notons $O_G(b)$ et $O_{\mathcal{A}}(b)$ les naissances plus anciennes selon la politique $\mathfrak{e}$ dans les deux présentations. Avec la convention que le minimum vide vaut $+\infty$, son niveau de mort global est

$$\mathrm{death}_{G}(b)=\min_{a\in O_G(b)}\lambda_k(b,a).$$

L'ensemble ancien de l'atlas est inclus dans l'ensemble ancien global. Comme l'atlas supprime des candidats et que $\lambda_k\leq\lambda_k^{\mathcal{A}}$, on a

$$\mathrm{death}_{\mathrm{global}}(b)\leq\mathrm{death}_{\mathcal{A}}(b),\qquad\mathrm{pers}_{\mathrm{global}}(b)\leq\mathrm{pers}_{\mathcal{A}}(b).$$

Cette inégalité est la raison pour laquelle il suffit d'exclure les **naissances absentes** et de certifier les **branches longues proposées par l'atlas**. Elle échoue si l'atlas ajoute des rencontres qui ne sont pas des rencontres globales ou si la politique d'aîné change entre les deux calculs.

### 7.2 Certificat d'exclusion des naissances absentes

Soit $\mathcal{U}^{\mathrm{birth}}$ l'univers fini des familles de naissances globales admissibles dans le périmètre déclaré. Les naissances non représentées dans l'atlas sont couvertes par des blocs $B$. Un certificat d'exclusion associe à chaque bloc un majorant global $P^{+}(B)$ tel que

$$b\in B\Longrightarrow\mathrm{pers}_{\mathrm{global}}(b)\leq P^{+}(B).$$

Le certificat peut combiner un minorant de naissance et un chemin global de connexion vers une classe plus ancienne ; il peut aussi provenir d'une borne géométrique uniforme sur tout le bloc. L'assertion $P^{+}(B)\leq\pi$ est contrôlable bloc par bloc. Elle ne reformule pas « toute branche longue est présente » : elle fournit un témoin quantitatif vérifiable pour chaque famille omise.

### Définition E.2 — atlas $\pi$-complet par certificats pour $H_0$

Notons $\Sigma_{\pi}^{\mathcal{A}}$ la fermeture par ancêtres et images verticales des branches de l'atlas dont la persistance d'atlas est supérieure à $\pi$. L'atlas $\mathcal{A}$ est **$\pi$-complet par certificats** si :

1. **couverture des naissances** : chaque naissance globale est soit représentée par un témoin exact dans $\mathcal{A}$, soit couverte par un bloc exclu avec $P^{+}(B)\leq\pi$ ;
2. **événements retenus** : chaque naissance, fusion candidate et événement d'ascendance apparaissant dans $\Sigma_{\pi}^{\mathcal{A}}$ possède un témoin géométrique global valide, un niveau certifié et un identifiant de lot exact ;
3. **attaches et lots** : les partitions globales des bras et l'incidence relative complète de chaque lot rencontré par $\Sigma_{\pi}^{\mathcal{A}}$ sont certifiées ;
4. **coupes** : chaque décision de fusion de $\Sigma_{\pi}^{\mathcal{A}}$ possède une coupe globale compatible certifiée contre tous les blocs et toutes les rencontres, y compris ceux absents de l'atlas ; le certificat donne un témoin traversant au niveau du lot, exclut tout franchissement strictement antérieur et identifie le côté qui porte la classe aînée ; pour une arête non comprimée, il s'agit de la coupe fondamentale de la proposition E.1 ;
5. **ordre** : l'atlas est fermé par les faces porteuses des images verticales de $\Sigma_{\pi}^{\mathcal{A}}$ ; chaque image possède un témoin d'inclusion global et un certificat de séparation jusqu'au prochain lot pertinent, ainsi que les ancêtres nécessaires.

La clause 1 empêche les faux négatifs ; les clauses 2 à 4 empêchent les branches artificiellement longues et déterminent leurs parents à partir de témoins locaux et de coupes, sans supposer ces parents connus d'avance ; la clause 5 empêche une simplification indépendante des ordres. Ces clauses portent sur des événements, des blocs, des coupes et des inclusions, et non sur l'isomorphisme final que l'on veut conclure.

La $\pi$-complétude peut être beaucoup plus faible que la complétude de toute la mosaïque, mais elle n'est pas automatique. Dans le pire cas de grande dimension, obtenir les majorants $P^{+}(B)$ ou fermer une coupe globale peut exiger d'examiner un nombre non sparse de candidats.

## 8. Théorème conditionnel de récupération

### Théorème E.3 — récupération de la tour condensée

Soient $\mathsf{GMF}$ la tour globale et $\mathsf{GMF}^{\mathcal{A}}$ la tour calculée exactement sur un atlas descendant $\mathcal{A}$. Fixons $\pi>0$. Supposons :

1. $G_k^{\mathcal{A}}$ est une sous-présentation de $G_k$ pour chaque ordre, avec niveaux hérités et politique d'aîné compatible ;
2. l'atlas est $\pi$-complet par certificats au sens de la définition E.2.

Alors il existe un isomorphisme de forêts graduées, compatible avec toutes les flèches verticales,

$$\mathsf{C}_{\pi}\bigl(\mathsf{GMF}^{\mathcal{A}}\bigr)\cong\mathsf{C}_{\pi}(\mathsf{GMF}).$$

Cet isomorphisme préserve les niveaux de naissance, les niveaux de mort, les lots multifurqués, l'ascendance des branches et les morphismes entre ordres.

**Preuve.** Une branche globale de persistance supérieure à $\pi$ ne peut appartenir à un bloc exclu par la clause 1. Sa naissance possède donc un témoin dans l'atlas. La monotonie de la section 7.1 implique que sa persistance d'atlas est encore supérieure à $\pi$ : elle appartient à $\Sigma_{\pi}^{\mathcal{A}}$. Il n'y a donc pas de faux négatif. Réciproquement, la clause 2 fournit les niveaux candidats globaux, la clause 3 détermine les composantes fusionnées simultanément, et la clause 4 exclut toute connexion globale plus précoce tout en identifiant le côté aîné. Chaque branche longue de l'atlas a donc le même niveau de mort et le même parent global ; elle est retenue globalement si et seulement si elle l'est dans l'atlas, sans faux positif. La clause 5 identifie les cibles des flèches verticales et impose la même fermeture par ancêtres. Les deux côtés conservent alors les mêmes marques et les mêmes sous-forêts minimales ; la suppression des mêmes sommets non marqués de degré $2$ donne l'isomorphisme annoncé. $\square$

### Version avec intervalles certifiés

Supposons que chaque niveau pertinent soit enfermé dans un intervalle de largeur au plus $\varepsilon$, que les égalités de lot soient décidées exactement, que les erreurs des niveaux minimax soient également au plus $\varepsilon$, et que chaque bloc de naissances omises possède un majorant certifié dont la borne supérieure ne dépasse pas $\pi$. Si

$$2\varepsilon<\min\left(\Delta_{\mathrm{evt}},\Delta_{\mathrm{cut}},\Delta_{\mathrm{att}},\Delta_{\mathrm{ord}}\right)$$

et

$$2\varepsilon<\Delta_{\pi},$$

alors la même conclusion combinatoire vaut : mêmes branches retenues, mêmes lots, mêmes parents et mêmes images verticales. Les hauteurs restent données par leurs intervalles certifiés. Cette assertion concerne directement les décisions hiérarchiques ; elle n'utilise aucune borne globale entre champs.

Le facteur $2$ couvre la comparaison de deux niveaux et l'erreur sur une persistance, différence d'un niveau de mort et d'un niveau de naissance chacun connu à $\varepsilon$ près. Il peut être resserré si les erreurs sont corrélées ou unilatérales.

## 9. Les trois statuts autorisés

Chaque résultat doit publier un statut, un périmètre et la liste des certificats effectivement fermés.

| statut | signification exacte | ce qu'il n'autorise pas à dire |
|---|---|---|
| `exact` | la tour globale est reconstruite dans le périmètre déclaré, complet ou condensé ; tous les événements, attaches, coupes et morphismes requis sont certifiés contre l'univers complet | rien n'est laissé à une hypothèse ANN ou de stabilisation |
| `atlas_exact` | la tour est exacte pour l'atlas descendant identifié et son graphe implicite complet | aucune conclusion automatique sur les branches du champ global |
| `conditional` | la conclusion globale, éventuellement condensée au seuil $\pi$, dépend d'au moins une hypothèse non certifiée telle que la $\pi$-complétude | ne doit pas être renommé « exact » parce que plusieurs rondes n'ajoutent plus de bloc |

Le périmètre est stocké séparément, par exemple `scope = full` ou `scope = condensed(pi)`. Un calcul peut être `atlas_exact` et simultanément fournir une **conjecture conditionnelle** sur le global ; le statut principal reste alors `conditional` pour cette conclusion globale.

Les diagnostics minimaux sont :

- identifiant et fermeture sémantique de l'atlas ;
- seuil $\pi$ et convention de persistance ;
- partition des familles de naissances omises et majorants $P^{+}(B)$ ;
- événements et lots retenus ou non résolus ;
- coupes certifiées, marges et familles de candidats couvertes ;
- bras dont l'attache est exacte ou seulement proposée ;
- porteurs verticaux certifiés ;
- bornes d'intervalles et branche grise éventuelle autour de $\pi$.

## 10. Oracles batchables et frontière GPU

La définition précédente ne suppose aucune architecture. Elle isole cependant des oracles qui se prêtent à un traitement par lots :

1. **oracle d'événements** : propose des naissances et selles, puis renvoie niveaux et prédicats certifiés ;
2. **oracle d'attaches** : cherche des chemins sous-niveau pour beaucoup de bras et produit, lorsque nécessaire, des coupes de séparation ;
3. **oracle de coupes** : minimise une borne sur toutes les rencontres traversant un ensemble de coupes fondamentales ;
4. **oracle d'ordre** : évalue les faces porteuses et localise en parallèle les composantes cibles des flèches verticales ;
5. **oracle d'exclusion** : borne la persistance maximale de familles d'événements ou de blocs encore absents.

Un index ANN, une continuation DTM–HGP ou une lentille entropique homogène peut alimenter l'oracle de proposition et fournir des bornes. La décision finale reste la comparaison de niveaux, de coupes et d'attaches du problème hiérarchique dur. L'entropie est utile si elle diminue le nombre de cas durs sans modifier le sens des certificats.

Le dernier oracle est le plus difficile. S'il ne peut pas exclure globalement des événements absents, le résultat reste `conditional`, même si tous les calculs **dans** l'atlas sont massivement parallèles et exacts.

## 11. Programme de preuve minimal

Une validation mathématique et expérimentale du résultat E doit procéder dans cet ordre :

1. vérifier les niveaux $\lambda_k$ contre le graphe exhaustif sur de petits jeux ;
2. vérifier chaque coupe fondamentale retenue, pas seulement le coût total de la forêt ;
3. comparer les partitions de bras avant chaque lot ;
4. vérifier les carrés de naturalité des morphismes verticaux ;
5. construire $\mathsf{C}_{\pi}$ des deux côtés et comparer branches, parents, lots et images ;
6. supprimer volontairement des blocs afin de vérifier que le statut passe à `conditional` dès qu'un certificat global manque.

Le cas perturbé de la base canonique doit rester un test négatif : si l'atlas nécessaire cesse d'être sparse, l'algorithme doit l'annoncer, non produire une fausse hiérarchie certifiée.

## 12. Statut mathématique de cette annexe

| composant | statut |
|---|---|
| niveaux minimax et propriété des coupes d'une forêt minimum | résultat classique exact |
| condensation par fermeture verticale et ancêtres | définition mathématique proposée ici |
| théorème E.3 | théorème conditionnel ; la preuve est formelle une fois les hypothèses certifiées |
| marges d'événements, de coupe, d'attache et d'ordre | contrats suffisants, non prétendus nécessaires |
| oracle global de $\pi$-complétude en grande dimension | ouvert ; peut être non sparse dans le pire cas |
| calcul exact dans un atlas descendant fini | réalisable en principe sans prétendre à l'exactitude globale |

La bonne ambition en grande dimension est donc précise : **maximiser la part du squelette condensé accompagnée de certificats globaux, et rendre visible la frontière exacte entre `atlas_exact` et `conditional`.**

## Références

- L. Hauseux, *Détection d'amas et interactions d'ordre supérieur*, chapitres 6 à 8 : équivalence avec les amas $k$-NN, adjacences élémentaires, simplexes séparants et $k$-MST.
- Y. Reani et O. Bobrowski, [*Morse Theory for the k-NN Distance Function*](https://arxiv.org/abs/2403.12792), 2024 : événements critiques et distinction entre effet local et effet homologique global.
- [Architecture mathématique E-HGP](../ARCHITECTURE_MATHEMATIQUE.md) : échafaudage classé, niveaux minimax et statuts d'atlas.
- [`RESULTAT_C_COMPLEXE_MORSE_BIFILTRE.md`](RESULTAT_C_COMPLEXE_MORSE_BIFILTRE.md) : définition de la tour de forêts et des morphismes verticaux.
