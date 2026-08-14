# Note — les deux (ou trois) canaux directionnels : support, dernière sortie, première entrée

Cette note répond à une proposition précise : décrire chaque nœud HGP par **deux vecteurs**, sa fonction support normalisée et sa « dernière sortie » du polyèdre normalisée, éventuellement complétées par sa « première entrée ». Elle ne remplace pas [GEOMETRIC_DESCRIPTOR_AUDIT.md](GEOMETRIC_DESCRIPTOR_AUDIT.md), qui traite déjà le rayon extérieur comme ablation ; elle ajoute ce que cet audit ne contient pas : un théorème de caractérisation du canal support, le statut exact du canal d'entrée, et une échelle de complétude mesurable.

## Résumé du verdict

1. Le canal support n'est pas *un* choix parmi d'autres : sous des hypothèses faibles et naturelles, c'est **le seul** descripteur exactement agrégeable le long de l'arbre de fusion et exactement recentrable. Cela mérite d'être énoncé comme lemme justificatif — mais **pas** comme contribution : le résultat est voisin de la théorie des mesures maxitives, et le canal lui-même est un PointNet à première couche linéaire (§ Antériorité).
2. Ce même théorème implique que le canal support **ne porte aucune information propre aux nœuds internes** : il est entièrement déterminé par les feuilles. C'est un raccourci de calcul exact, jamais un signal supplémentaire. Un reviewer le remarquera ; il vaut mieux l'écrire.
3. Le canal « dernière sortie » $\rho_{\mathrm{out}}$ et le canal « première entrée » $\rho_{\mathrm{in}}$ échappent au théorème parce qu'ils sont **discontinus** en la position du point. C'est précisément ce qui leur permet de voir la non-convexité, et précisément ce qui leur coûte le recentrage en forme close.
4. Autour d'un centre propre au nœud, $\rho_{\mathrm{in}}$ est **vacu ou instable** : identiquement nul dès que le centre appartient au carrier, et non continu en distance de Hausdorff sinon. Tel quel, ce troisième canal n'est pas défendable.
5. Autour d'un **centre global unique — l'origine capteur** — les trois canaux redeviennent exactement fusionnables ($\max$, $\max$, $\min$), et $\rho_{\mathrm{in}}$ devient une quantité physique interprétable : la surface visible. C'est la version de la proposition qu'il faut tester.
6. Aucun de ces points ne rend l'état de l'art probable. Le descripteur n'est pas le facteur limitant du mIoU SemanticKITTI ; voir [REVIEWER_VERDICT.md](REVIEWER_VERDICT.md) et le § Où le descripteur ne peut pas aider.

## Théorème — le canal support est forcé

### Énoncé

Soit un *canal de descripteur* une application $D$ qui associe un réel à une partie finie non vide $A\subset\mathbb{R}^{3}$ et à un centre $c\in\mathbb{R}^{3}$. On suppose :

- **(H0) localité de repère** : $D(A;c)=D(A-c;0)$ ;
- **(H1) agrégation exacte sur l'arbre de fusion** : il existe $F$ continue et non décroissante en chaque argument telle que $D(A\cup B;c)=F\left(D(A;c),D(B;c)\right)$ pour toutes parties finies non vides $A,B$ et tout $c$ ;
- **(H1b) monotonie d'extension** : $A\subseteq A'$ entraîne $D(A;c)\leq D(A';c)$, c'est-à-dire qu'ajouter des points ne diminue pas la mesure d'étendue ;
- **(H2) recentrage en forme close** : il existe $G$ telle que $D(A;c')=G\left(D(A;c),c-c'\right)$ pour tous $A,c,c'$ ;
- **(H3) régularité** : $g(y)\triangleq D\left(\left\lbrace y\right\rbrace;0\right)$ est continue et non constante.

Alors il existe $a\in\mathbb{R}^{3}\setminus\left\lbrace0\right\rbrace$ et $\varphi:\mathbb{R}\to\mathbb{R}$ continue strictement monotone tels que $D(A;c)=\varphi\left(\max_{x\in A}\left\langle a,x-c\right\rangle\right)$ si $\varphi$ est croissante, et $D(A;c)=\varphi\left(\min_{x\in A}\left\langle a,x-c\right\rangle\right)$ si $\varphi$ est décroissante.

Autrement dit : **tout canal vérifiant (H0)–(H3) est une reparamétrisation monotone d'une valeur de la fonction support.**

### Démonstration

*Étape 1 — (H1)+(H1b) forcent le max.* L'union étant commutative, associative et idempotente, $F$ l'est aussi sur l'image de $D$. Soient $x,y$ dans cette image. (H1b) appliquée aux inclusions $A\subseteq A\cup B$ et $B\subseteq A\cup B$ donne $F(x,y)\geq\max(x,y)$. L'idempotence et la croissance en chaque argument donnent $F(x,y)\leq F\left(\max(x,y),\max(x,y)\right)=\max(x,y)$. Donc $F=\max$ et, par récurrence sur $|A|$, $D(A;c)=\max_{x\in A}g(x-c)$.

Sans (H1b), les opérations continues, commutatives, associatives et idempotentes d'un intervalle réel sont exactement les médianes $F(x,y)=\mathrm{med}(x,y,\alpha)$, $\alpha$ constant, dont $\max$ et $\min$ sont les cas extrêmes. On obtient alors une fonction support **écrêtée**, ce qui ne change rien à la conclusion géométrique.

*Étape 2 — (H2) force des lignes de niveau affines.* Appliquée aux singletons, (H2) donne $g(y+\delta)=G\left(g(y),\delta\right)$ pour tous $y,\delta$. Donc $g(y_1)=g(y_2)$ entraîne $g(y_1+\delta)=g(y_2+\delta)$ : la partition de $\mathbb{R}^{3}$ en lignes de niveau de $g$ est invariante par translation, et l'application $\delta\mapsto$ classe de $\delta$ est surjective. Notons $H$ la classe de $0$. Les classes sont exactement les translatés $H+\delta$ ; comme elles partitionnent $\mathbb{R}^{3}$, $H$ est un sous-groupe de $\left(\mathbb{R}^{3},+\right)$ et les classes sont ses classes latérales. La continuité de $g$ rend $H$ fermé, et $g$ induit une injection continue de $\mathbb{R}^{3}/H$ dans $\mathbb{R}$.

*Étape 3 — classification.* Les sous-groupes fermés de $\mathbb{R}^{3}$ sont isomorphes à $\mathbb{R}^{k}\times\mathbb{Z}^{m}$ avec $k+m\leq3$, et $\mathbb{R}^{3}/H$ est alors homéomorphe à $\mathbb{R}^{3-k-m}\times\mathbb{T}^{m}$. Une injection continue dans $\mathbb{R}$ interdit toute composante de dimension $\geq2$ (invariance du domaine) et tout facteur torique de dimension $\geq1$ (un tore est compact et connexe, donc son image serait un segment, ce qui contredit l'injectivité par un argument de point non séparant). Il reste $\dim\left(\mathbb{R}^{3}/H\right)\leq1$ sans facteur compact, soit $H=\mathbb{R}^{3}$ — exclu par (H3) — soit $H$ hyperplan vectoriel.

*Étape 4 — conclusion.* $H=a^{\perp}$ pour un $a\neq0$, donc $g=\varphi\left(\left\langle a,\cdot\right\rangle\right)$ avec $\varphi$ continue injective sur $\mathbb{R}$, donc strictement monotone. Si $\varphi$ est croissante, $\max_{x\in A}\varphi\left(\left\langle a,x-c\right\rangle\right)=\varphi\left(\max_{x\in A}\left\langle a,x-c\right\rangle\right)$ ; sinon le $\max$ externe devient un $\min$ interne. $\square$

*Portée exacte de (H2), à ne pas surestimer.* L'hypothèse est **par canal** : chaque canal se recentre à partir de sa seule valeur. Un reviewer proposera la version affaiblie où le descripteur complet à $D$ canaux se recentre conjointement, $D_j(A;c')=G_j\left(D_1(A;c),\ldots,D_D(A;c),c-c'\right)$. Le théorème ci-dessus ne couvre pas ce cas et la classe admissible y est *a priori* plus large. Il faut donc énoncer l'hypothèse par canal explicitement, et traiter la version conjointe comme une question ouverte plutôt que comme un corollaire.

### Corollaires opérationnels

- **Fusion exacte** : $h_{A\cup B}^{c}(u)=\max\left(h_{A}^{c}(u),h_{B}^{c}(u)\right)$.
- **Recentrage exact** : $h_{A}^{c'}(u)=h_{A}^{c}(u)+\left\langle u,c-c'\right\rangle$.
- **Identité normalisée** déjà écrite dans [ARCHITECTURE.md](ARCHITECTURE.md) : $s_p(u)=\max_{v\in\mathrm{children}(p)}\left[\left\langle u,\frac{c_v-c_p}{R_p}\right\rangle+\frac{R_v}{R_p}s_v(u)\right]$. Le théorème explique **pourquoi** cette identité existe et pourquoi aucun autre descripteur de forme comparable ne l'a.
- **Conséquence négative, à écrire dans le papier** : le canal support d'un nœud interne est une fonction déterministe de ses enfants, donc des feuilles. Il n'apporte aucune information au-delà des feuilles ; il évite seulement au réseau de réapprendre une agrégation qu'on sait calculer exactement. L'ablation `objet complet seul` contre `support + objet complet` mesure donc un effet d'optimisation, pas un effet d'information, et doit être présentée comme telle.

### Antériorité : ce lemme justifie, il ne contribue pas

Il faut calibrer honnêtement ce que vaut ce résultat, sous peine de le voir démonté en review.

- **La fonction support échantillonnée est un PointNet.** $h_{C_v}(u_k)=\max_{x\in C_v}\left\langle u_k,x\right\rangle$ est littéralement un PointNet dont la première couche est linéaire et l'agrégation un max-pooling ; le *critical point set* de PointNet est exactement l'ensemble des points extrémaux. Le canal support n'est donc pas une feature nouvelle : c'est le PointNet le plus simple possible, et il faut le présenter ainsi.
- **L'agrégation par $\max$ sur les enfants d'un arbre de partition est déjà publiée.** Superpoint Transformer (ICCV 2023) l'emploie explicitement dans son équation d'encodeur, $g_p^{i}=T^{i}\circ\varphi^{i}\left(x_p^{i},\max_{q\in\mathrm{children}(p)}g_q^{i-1}\right)$. La fonctorialité par union sur un arbre de clusters n'est pas un terrain vierge.
- **Le théorème lui-même est voisin d'un corpus établi.** Aucun énoncé nommé n'affirme l'unicité sous cette forme exacte, mais la théorie des **sup-mesures et mesures maxitives** — Vervaat 1988, Norberg--Vervaat 1989, Akian, *Trans. AMS* 1999 — établit qu'une mesure idempotente possède une densité, c'est-à-dire $T(A)=\sup_{x\in A}d(x)$ : c'est l'étape 1 de la démonstration. Voisinent aussi les endomorphismes de Minkowski de Schneider (1974) et, du côté additif, le théorème de Hadwiger. La démonstration ci-dessus est donc une **dérivation propre dans un cadre connu**, pas une découverte.
- **Les descripteurs directionnels apprenables plus riches existent déjà** : ECT/DECT différentiable à directions apprises (Röell et Rieck, ICLR 2024), WECT, et leurs implémentations GPU. La fonction support n'est que l'extrémité du filtrage directionnel $\left\langle x,u\right\rangle$ que l'ECT résume entièrement.
- **Une borne d'expressivité s'applique.** Wagstaff *et al.*, ICML 2019 : un encodeur d'ensembles à latent de dimension $D$ ne représente fidèlement que des ensembles de cardinalité au plus $D$. Un descripteur à $D$ directions est donc **prouvablement lossy** dès que $n_v>D$, ce qui est le régime de tous les gros nœuds.

Conclusion de calibrage : ce lemme sert à **justifier** un choix d'architecture et à en dériver l'algorithme exact de remontée. Il ne doit jamais être annoncé comme la contribution, et une soumission qui reposerait dessus serait correctement rejetée.

### Ce que le théorème dit des deux autres canaux

Avec un centre $c$ et une fenêtre angulaire $B_u\subset\mathbb{S}^{2}$, poser $g_u(y)=\left\Vert y\right\Vert$ si $y/\left\Vert y\right\Vert\in B_u$ et $-\infty$ sinon. Alors $\rho_{\mathrm{out}}(u)=\max_{x\in A}g_u(x-c)$ vérifie (H0) et (H1) mais **pas** (H3), car $g_u$ est discontinue au bord de la fenêtre. Symétriquement $\rho_{\mathrm{in}}$ est un $\min$-pooling. Le théorème n'est donc pas contredit ; il localise exactement le compromis :

> un canal directionnel est soit continu, exactement fusionnable et exactement recentrable — et c'est alors la fonction support, donc aveugle à la non-convexité — soit discontinu, exactement fusionnable à centre fixé, sensible à la non-convexité, et non recentrable.

Il n'existe pas de canal qui ait les trois propriétés. Cette dichotomie est la vraie justification de la proposition à deux canaux, et elle est plus forte que l'argument « l'un est convexe, l'autre non ».

### Redondance exacte en résolution infinie, complémentarité en résolution finie

Il faut aussi savoir répondre à l'objection « vos deux canaux n'en font qu'un ». Pour un même ensemble et un même centre, en résolution **infinie**, le support est une fonctionnelle du rayon extérieur : tout $x\in A$ s'écrit $\left\Vert x\right\Vert v$ avec $\left\Vert x\right\Vert\leq\rho_{\mathrm{out}}(v)$, d'où $h_A(u)=\sup\left\lbrace\rho_{\mathrm{out}}(v)\left\langle v,u\right\rangle:\left\langle v,u\right\rangle>0\right\rbrace$ dès que $0\in A$. Le premier canal n'ajoute alors rien au second.

En résolution **finie**, la conclusion s'inverse. $\rho_{\mathrm{out}}$ échantillonné sur $D$ fenêtres ne permet plus de reconstruire $h$ : le maximum d'une projection linéaire est atteint par un point qui peut tomber dans n'importe quelle fenêtre. Le canal support fournit donc, pour le même budget de $D$ nombres, la valeur **exacte** d'une grandeur que le canal radial n'approche que par bin, et cette valeur est celle qui se fusionne et se recentre exactement. La complémentarité des deux canaux est un fait de résolution, pas d'information ; c'est ainsi qu'il faut l'écrire, et c'est ce que l'ablation `support seul` / `radial seul` / `les deux` doit mesurer à $D$ apparié.

## Statut du canal « première entrée »

Pour un carrier $A$, un centre $c$ et une direction $u$, poser $I_{A,c}(u)=\left\lbrace r\geq0:c+ru\in A\right\rbrace$, puis $\rho_{\mathrm{out}}(u)=\sup I_{A,c}(u)$ et $\rho_{\mathrm{in}}(u)=\inf I_{A,c}(u)$.

Trois obstructions, à traiter avant toute implémentation.

1. **Vacuité.** Si $c\in A$, alors $\rho_{\mathrm{in}}\equiv0$ et le canal ne transporte rien. Le canal n'est donc informatif que pour les nœuds dont le centre tombe dans un vide du carrier — pour une coque de surface LiDAR autour de son barycentre, c'est fréquemment le cas ; pour un carrier volumique comme un buisson dense, ce ne l'est pas. Vacuité et informativité s'excluent, et le régime dépend du nœud : il faut un drapeau `center_in_carrier` par nœud et une mesure de sa fréquence par classe, avant de décider si le canal existe.
2. **Instabilité.** $h$ est $1$-lipschitzienne en distance de Hausdorff : $\left|h_A(u)-h_B(u)\right|\leq d_H(A,B)$ pour tout $u$. Ni $\rho_{\mathrm{in}}$ ni $\rho_{\mathrm{out}}$ ne le sont. Contre-exemple : soit $A$ une boule de rayon $1$ centrée en $c+5u$, de sorte que $\rho_{\mathrm{in}}(u)=4$ et $\rho_{\mathrm{out}}(u)=6$. Retirer de $A$ un cylindre mince de rayon $\delta$ autour du rayon, sur $r\in\left[4,4{,}5\right]$, modifie $A$ de moins de $\delta$ en distance de Hausdorff mais fait passer $\rho_{\mathrm{in}}(u)$ de $4$ à $4{,}5$ ; le même perçage à l'arrière fait chuter $\rho_{\mathrm{out}}$. L'écart n'est borné par aucun multiple de $\delta$. Ce n'est pas une pathologie théorique : l'amincissement angulaire d'un LiDAR à longue portée retire exactement les retours qui définissent le premier contact dans un bin.
3. **Discrétisation.** Sur un nuage fini, $I_{A,c}(u)$ est vide pour presque tout $u$ : les trois canaux n'existent qu'après fenêtrage angulaire ou épaississement, et leur valeur dépend alors de la largeur de fenêtre autant que de la forme. La largeur devient un hyperparamètre contractuel, à ablater et à consigner, exactement comme la grille de directions.

## Correction proposée : le centre est le capteur, pas le barycentre

La proposition devient nettement plus solide si les canaux radiaux sont calculés autour d'un **centre global unique par scan, l'origine du LiDAR**, et non autour du centre propre de chaque nœud.

Conséquences.

- **Fusion exacte retrouvée pour les trois canaux.** À centre commun, $h$ et $\rho_{\mathrm{out}}$ se fusionnent par $\max$, $\rho_{\mathrm{in}}$ par $\min$, et le masque « rayon touché » par disjonction. Toute la hiérarchie se remplit en une seule passe ascendante, en $\mathcal{O}(D)$ par arête, sans recalcul par nœud et sans approximation.
- **Recette complète qui en découle.** Tout calculer une seule fois à l'origine capteur, remonter l'arbre par $\max/\max/\min$, puis appliquer à la fin, et seulement au canal support, le recentrage exact $h_v(u)\leftarrow\left[h_v^{c_0}(u)+\left\langle u,c_0-c_v\right\rangle\right]/R_v$ vers le repère propre du nœud. On obtient un canal invariant par translation et par échelle **et** un canal capteur, tous deux exacts, pour une seule passe ascendante. C'est l'argument de coût le plus fort en faveur de cette famille de descripteurs, et il ne vaut que grâce au théorème ci-dessus.
- **Sens physique.** Dans le repère capteur, $\rho_{\mathrm{in}}$ est la **surface visible** du nœud dans la direction $u$, $\rho_{\mathrm{out}}$ sa surface la plus lointaine, et $\rho_{\mathrm{out}}-\rho_{\mathrm{in}}$ son **épaisseur en profondeur** — la quantité qui sépare un mur d'un buisson, ou une façade d'un feuillage, et que la fonction support ne voit pas.
- **Vacuité résolue.** L'origine capteur n'appartient jamais au carrier d'un nœud, donc $\rho_{\mathrm{in}}$ n'est jamais identiquement nul.
- **Équivariance.** Avec des bins uniformes en azimut, une rotation de lacet permute cycliquement les bins : un encodeur convolutif circulaire en azimut est exactement équivariant à l'augmentation de lacet standard de SemanticKITTI. En élévation il n'y a pas d'équivariance à exiger, la gravité étant sémantiquement pertinente.
- **Prix à payer, à déclarer.** L'invariance par translation du descripteur est perdue : deux voitures identiques à deux endroits n'ont plus le même canal radial. C'est un choix, pas un oubli, et c'est le même choix que font les méthodes à image de portée. Il faut donc conserver **en parallèle** le canal support normalisé par nœud, qui reste invariant par translation et par échelle. La proposition à deux vecteurs devient alors : un canal invariant exactement fusionnable dans le repère du nœud, un canal capteur exactement fusionnable dans le repère global.
- **Antériorité à citer.** Dans le repère capteur, $\left(\rho_{\mathrm{in}},\rho_{\mathrm{out}}\right)$ par bin est une **image de portée min/max restreinte au nœud**. La littérature d'images de portée est une antériorité directe, pas un détail ; la nouveauté éventuelle porte sur le couplage à la hiérarchie, pas sur la représentation.

## Échelle de complétude radiale

Le triplet proposé est la troncature à deux termes d'un invariant complet.

Pour $c$ fixé, les rayons issus de $c$ partitionnent $\mathbb{R}^{3}\setminus\left\lbrace c\right\rbrace$. L'application $A\mapsto\left(I_{A,c}(u)\right)_{u\in\mathbb{S}^{2}}$ est donc **injective** sur les compacts : la famille complète des cordes radiales, depuis un centre unique, détermine exactement $A$. On obtient une échelle strictement croissante d'information, entièrement mesurable :

$h\prec\rho_{\mathrm{out}}\prec\left(\rho_{\mathrm{in}},\rho_{\mathrm{out}}\right)\prec$ troncature à $k$ intervalles $\prec$ occupation binaire le long du rayon $\prec I_{A,c}$ complet.

Deux avertissements.

- Sur un **nuage fini**, l'injectivité est triviale et l'objet complet n'est qu'un ré-encodage polaire du nuage : l'échelle ne mesure alors qu'une résolution angulaire et radiale, pas une inductive bias. Elle n'a de contenu géométrique que sur un carrier continu déclaré, $C_v^{F}$, $C_v^{Q}$ ou $W_v(a)$.
- L'échelle n'établit rien sur la **stabilité** : plus on monte, plus on gagne en information et plus on perd en robustesse au rééchantillonnage. La courbe à tracer est donc à deux axes, information reconstruite contre stabilité sous thinning, et non un simple classement.

Cette échelle a néanmoins une vertu que l'audit actuel n'exploite pas : elle transforme « le rayon est une ablation lossy » en une **famille d'ablations paramétrée avec un point terminal exact**, donc en une expérience qui répond à la question « combien coûte la compression » au lieu de la trancher a priori.

## Où le descripteur ne peut pas aider

Le mIoU SemanticKITTI est dominé par les classes rares et fines : `bicycle`, `motorcycle`, `person`, `bicyclist`, `motorcyclist`, `pole`, `traffic-sign`. Ce sont exactement les nœuds de faible cardinalité, où un descripteur sphérique échantillonné sur $D$ directions est essentiellement du bruit : le nombre de directions non vides est inférieur au nombre de points. À l'inverse, les classes où un descripteur de forme est fiable — `road`, `building`, `vegetation`, `terrain` — plafonnent déjà au-delà de $90$ d'IoU.

Il faut donc **stratifier par cardinalité de nœud dès la première mesure** et accepter le résultat : si le gain se concentre sur les grandes classes, il ne se transformera pas en mIoU. Ce point ne réfute pas la hiérarchie ; il réfute l'idée que le descripteur de nœud soit le facteur limitant.

Le même argument frappe la hiérarchie elle-même, et plus durement : la connexité d'ordre $K$ exige que $K$ points soient simultanément proches, ce que les structures filiformes ne fournissent pas — le manuscrit documente ce mode d'échec sur `birch2`. Voir [NOTE_CLAUDE_ORDRE_DES_PREUVES.md](NOTE_CLAUDE_ORDRE_DES_PREUVES.md).

## Spécification minimale implémentable

Si la proposition doit être codée telle quelle, voici sa version défendable, à $D$ directions et $B$ bins azimut $\times$ élévation.

| Canal | Définition | Centre | Fusion | Recentrage | Dimension |
|---|---|---|---|---|---|
| support | $h_v(u_j)=\max_{x\in C_v}\left\langle u_j,x-c_0\right\rangle$ | capteur, puis recentré | $\max$ exact | exact, additif | $D$ |
| dernière sortie | $\rho_{\mathrm{out}}(b)=\max\left\lbrace\left\Vert x\right\Vert:x\in C_v\cap\text{bin }b\right\rbrace$ | capteur | $\max$ exact | sans objet | $B$ |
| première entrée | $\rho_{\mathrm{in}}(b)=\min\left\lbrace\left\Vert x\right\Vert:x\in C_v\cap\text{bin }b\right\rbrace$ | capteur | $\min$ exact | sans objet | $B$ |
| épaisseur | $\rho_{\mathrm{out}}-\rho_{\mathrm{in}}$ | capteur | dérivé | sans objet | $B$ |
| masque de bin | $\mathbf{1}\left\lbrace C_v\cap\text{bin }b\neq\emptyset\right\rbrace$ | capteur | disjonction | sans objet | $B$ |
| comptage par bin | $\left|C_v\cap\text{bin }b\right|$ | capteur | somme exacte | sans objet | $B$ |

Les six canaux se remplissent en **une seule passe ascendante** sur l'arbre, chacun par un monoïde exact ($\max$, $\min$, $\vee$, $+$). Aucun n'exige de recalcul par nœud. Le masque et le comptage sont obligatoires : sans eux, un bin vide et un bin à un seul point sont indiscernables, et c'est la source d'erreur la plus probable de cette famille de descripteurs.

**Le canal qu'il faut ajouter en priorité n'est pas un canal de forme, c'est un canal de masse — et il y a un théorème pour le dire.** La fusion par $\max$ et la fusion par $\sum$ sont toutes deux exactes sur l'arbre, mais elles ne sont pas d'expressivité comparable : un agrégateur $\max$ à la PointNet ne peut pas approcher des moyennes de fonctions continues — le centre de masse en est le contre-exemple canonique — alors qu'un agrégateur additif à la DeepSets le peut, et lui est strictement supérieur à cardinalité fixée. Les canaux $h$, $\rho_{\mathrm{out}}$ et $\rho_{\mathrm{in}}$ sont **tous les trois** des extrema : ils ne voient aucune masse intérieure. Le tenseur de CDF projetées $F_v(u_j,t_b)=n_v^{-1}\sum_{x\in C_v}\mathbf{1}\left\lbrace\left\langle u_j,(x-c_v)/R_v\right\rangle\leq t_b\right\rbrace$, déjà signalé comme « ajout prioritaire » dans [ARCHITECTURE.md](ARCHITECTURE.md), est additif donc exactement fusionnable dans un repère commun, et strictement plus expressif. Il devrait faire partie de la proposition dès le départ, au même titre que les deux vecteurs directionnels — c'est le canal qui répond à la fixture « cube plein contre frontière ».

À cela s'ajoutent les canaux non normalisés déjà listés dans [ARCHITECTURE.md](ARCHITECTURE.md) — $\log R_v$, $\log(1+n_v)$, portée du centre, naissance/mort/persistance, valeurs propres de covariance, statistiques de rémission — qui portent l'échelle absolue que la normalisation détruit et qui, sur SemanticKITTI, sont probablement plus discriminants que la forme elle-même.

## Ce qu'il faut mesurer avant d'écrire une ligne de modèle

Deux diagnostics, sans aucun entraînement, tranchent l'essentiel.

1. **Oracle d'antichaîne.** Sur la séquence 08, construire la forêt HGP, sélectionner une antichaîne à budget de régions fixé, étiqueter chaque nœud par sa classe majoritaire et rapporter le mIoU obtenu, contre HDBSCAN, octree/voxel, partition superpoint et arbre aléatoire à compression égale. Le protocole exact — et pourquoi « la meilleure antichaîne au sens du mIoU » n'est pas un objectif bien posé — est dans [NOTE_CLAUDE_ORDRE_DES_PREUVES.md](NOTE_CLAUDE_ORDRE_DES_PREUVES.md). Si HGP ne domine aucun contrôle, le programme est réfuté en une semaine.
2. **Stabilité par portée.** Transporter le même objet à plusieurs portées, rééchantillonner selon un modèle capteur déclaré, et mesurer la dérive des niveaux de naissance/mort et de l'ancêtre commun. C'est le test de [R1](RISKS_AND_GO_NO_GO.md) ; il conditionne tout le reste, car une hiérarchie qui encode la portée n'est pas une hiérarchie sémantique.

Ces deux mesures coûtent moins qu'une seule journée de GPU et valent plus que toute extension supplémentaire de la spécification.
