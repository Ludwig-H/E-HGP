# Descripteurs de nœud — support, canaux radiaux, canal de masse

Ce document traite la question : **comment résumer un nœud de la hiérarchie en un vecteur de taille fixe ?** Il part d'une proposition précise — décrire chaque nœud par sa fonction support normalisée et sa « dernière sortie » du polyèdre, éventuellement complétées par sa « première entrée » — et en établit le statut exact.

Pour une entrée progressive, lire d'abord le [chapitre 4 du guide](GUIDE.md). Ce document-ci est le niveau de détail avec les démonstrations. Il complète [CONTRAT_HGP.md](CONTRAT_HGP.md), qui porte sur les carriers et leur sérialisation.

## La géométrie, avant tout le reste

Une confusion coûte cher si on ne la lève pas d'emblée : **$u$ est un vecteur unitaire de $S^{2}$, pas un plan.**

Mais les fibres de $x\mapsto\left\langle u,x\right\rangle$ **sont** les plans orthogonaux à $u$ : tous les points d'un même plan $\perp u$ ont la même projection. Donc $u$ seul désigne une **famille de plans parallèles**, et le couple $(u,t)$ désigne **un** plan précis, celui à la cote $t$. Le demi-espace $\left\lbrace x:\left\langle u,x\right\rangle\leq t\right\rbrace$ est ce qui se trouve dessous.

C'est de là que vient le nom : $h(u)=\max_x\left\langle u,x\right\rangle$ donne la position du dernier plan $\perp u$ qui touche encore l'ensemble — **le plan d'appui**. Le plan est la sortie, pas l'entrée.

Vérification par les degrés de liberté : $u\in S^{2}$ en apporte $2$, $t\in\mathbb{R}$ en apporte $1$, et $(u,t)\in S^{2}\times\mathbb{R}$ en apporte $3$ — exactement la dimension de l'espace des plans orientés de $\mathbb{R}^{3}$. D'où, directement, le coût des canaux : le support vit sur $S^{2}$ et coûte $D$ ; la CDF vit sur $S^{2}\times\mathbb{R}$ et coûte $D\times B$. La fonction support est littéralement **le bord du domaine de la CDF**, le lieu où celle-ci atteint $1$.

## Résumé du verdict

1. Le canal support n'est pas *un* choix parmi d'autres : sous des hypothèses faibles et naturelles, c'est **le seul** descripteur exactement agrégeable le long de l'arbre de fusion et exactement recentrable. Cela mérite d'être énoncé comme lemme justificatif — mais **pas** comme contribution : le résultat est voisin de la théorie des mesures maxitives, et le canal lui-même est un PointNet à première couche linéaire (§ Antériorité).
2. Ce même théorème implique que le canal support **ne porte aucune information propre aux nœuds internes** : il est entièrement déterminé par les feuilles. C'est un raccourci de calcul exact, jamais un signal supplémentaire. Un reviewer le remarquera ; il vaut mieux l'écrire.
3. Le canal « dernière sortie » $\rho_{\mathrm{out}}$ et le canal « première entrée » $\rho_{\mathrm{in}}$ échappent au théorème parce qu'ils **ne se recentrent pas en forme close** — et non, comme je l'avais d'abord écrit, parce qu'ils sont discontinus : le rayon extérieur non binné est parfaitement continu et échoue déjà. Le fenêtrage angulaire leur coûte la continuité **en plus**. C'est ce renoncement au recentrage qui achète la sensibilité à la non-convexité.
4. Autour d'un centre propre au nœud, $\rho_{\mathrm{in}}$ est **vacu ou instable** : identiquement nul dès que le centre appartient au carrier, et non continu en distance de Hausdorff sinon. Tel quel, ce troisième canal n'est pas défendable.
5. Autour d'un **centre global unique — l'origine capteur** — les trois canaux redeviennent exactement fusionnables ($\max$, $\max$, $\min$), et $\rho_{\mathrm{in}}$ devient une quantité physique interprétable : la surface visible. C'est la version de la proposition qu'il faut tester.
6. Aucun de ces points ne rend l'état de l'art probable. Le descripteur n'est pas le facteur limitant du mIoU SemanticKITTI ; voir [STRATEGIE_PUBLICATION.md](STRATEGIE_PUBLICATION.md) et le § Où le descripteur ne peut pas aider.

## Théorème — le canal support est forcé

### Énoncé

Soit un *canal de descripteur* une application $D$ qui associe un réel à une partie finie non vide $A\subset\mathbb{R}^{3}$ et à un centre $c\in\mathbb{R}^{3}$. On suppose :

- **(H0) localité de repère** : $D(A;c)=D(A-c;0)$ ;
- **(H1) agrégation exacte sur l'arbre de fusion** : il existe $F$ continue et non décroissante en chaque argument telle que $D(A\cup B;c)=F\left(D(A;c),D(B;c)\right)$ pour toutes parties finies non vides $A,B$ et tout $c$ ;
- **(H1b) monotonie d'extension** : $A\subseteq A'$ entraîne $D(A;c)\leq D(A';c)$, c'est-à-dire qu'ajouter des points ne diminue pas la mesure d'étendue ;
- **(H2) recentrage en forme close** : il existe $G$ telle que $D(A;c')=G\left(D(A;c),c-c'\right)$ pour tous $A,c,c'$ ;
- **(H3) régularité** : $g(y)\triangleq D\left(\left\lbrace y\right\rbrace;0\right)$ est continue et non constante.

Alors il existe $a\in\mathbb{R}^{3}\setminus\left\lbrace0\right\rbrace$ et $\varphi:\mathbb{R}\to\mathbb{R}$ continue strictement **croissante** tels que $D(A;c)=\varphi\left(\max_{x\in A}\left\langle a,x-c\right\rangle\right)=\varphi\left(h_{A-c}(a)\right)$. Le couple $(a,\varphi)$ est déterminé à $a\mapsto\lambda a$, $\varphi\mapsto\varphi(\cdot/\lambda)$ près, $\lambda>0$. Si de plus le recentrage est un **décalage**, $G(v,\delta)=v+\ell(\delta)$, alors $\varphi$ est affine et $D(A;c)=\alpha\,h_{A-c}(a)+\beta$ avec $\alpha\neq0$.

Autrement dit : **tout canal vérifiant (H0)–(H3) est une reparamétrisation croissante d'une valeur de la fonction support.** L'énoncé et la démonstration valent dans $\mathbb{R}^{n}$ pour tout $n\geq1$ ; rien n'est propre à la dimension $3$.

La lecture la plus utile est la forme négative : sous (H0)–(H3), $D(A;c)=D\left(\mathrm{conv}(A);c\right)$. Un canal scalaire continu, exactement fusionnable et exactement recentrable **ne voit rien de $A$ hormis un hyperplan d'appui de son enveloppe convexe**. Tout descripteur réellement sensible à la non-convexité doit donc renoncer à l'une des trois propriétés.

### Démonstration

*Étape 1 — (H1)+(H1b) forcent le max.* L'union étant commutative, associative et idempotente, $F$ l'est aussi sur l'image de $D$. Soient $x,y$ dans cette image. (H1b) appliquée aux inclusions $A\subseteq A\cup B$ et $B\subseteq A\cup B$ donne $F(x,y)\geq\max(x,y)$. L'idempotence et la croissance en chaque argument donnent $F(x,y)\leq F\left(\max(x,y),\max(x,y)\right)=\max(x,y)$. Donc $F=\max$ et, par récurrence sur $|A|$, $D(A;c)=\max_{x\in A}g(x-c)$.

**(H1b) n'est pas un confort de rédaction, c'est ce qui tue la branche médiane.** Sans elle, l'idempotence et la monotonie ne donnent que l'*internalité* $\min(x,y)\leq F(x,y)\leq\max(x,y)$, et la trichotomie classique de Fung--Fu (1975) laisse $F\in\left\lbrace\max,\min,\mathrm{med}(\cdot,\cdot,\alpha)\right\rbrace$. La médiane $\mathrm{med}(\cdot,\cdot,\alpha)$ est une opération continue, commutative, associative, idempotente et monotone parfaitement légitime, réalisée par le descripteur $D(A;c)=\mathrm{med}\left(\min_{x\in A}g(x-c),\max_{x\in A}g(x-c),\alpha\right)$, qui vérifie (H0), (H1) et (H3). Elle n'est éliminée que par (H2) : aucun support écrêté ne se recentre en forme close. Reste alors $F=\min$, exclue par (H1b) puisque ajouter des points ferait décroître $D$. Deux rédactions sont donc possibles — imposer (H1b) et conclure directement, ou omettre (H1b) et faire de l'équivariance exacte tirée de (H2) l'étape qui classe $F$ dans $\left\lbrace\max,\min\right\rbrace$. La seconde est plus autonome ; la première est plus courte. Ce qu'il ne faut **pas** faire, c'est prétendre que (H1) seule force le $\max$.

*Étape 2 — (H2) force des lignes de niveau affines.* Appliquée aux singletons, (H2) donne $g(y+\delta)=G\left(g(y),\delta\right)$ pour tous $y,\delta$. Donc $g(y_1)=g(y_2)$ entraîne $g(y_1+\delta)=g(y_2+\delta)$ : la partition de $\mathbb{R}^{3}$ en lignes de niveau de $g$ est invariante par translation, et l'application $\delta\mapsto$ classe de $\delta$ est surjective. Notons $H$ la classe de $0$. Les classes sont exactement les translatés $H+\delta$ ; comme elles partitionnent $\mathbb{R}^{3}$, $H$ est un sous-groupe de $\left(\mathbb{R}^{3},+\right)$ et les classes sont ses classes latérales. La continuité de $g$ rend $H$ fermé, et $g$ induit une injection continue de $\mathbb{R}^{3}/H$ dans $\mathbb{R}$.

*Étape 3 — classification.* Les sous-groupes fermés de $\mathbb{R}^{3}$ sont isomorphes à $\mathbb{R}^{k}\times\mathbb{Z}^{m}$ avec $k+m\leq3$, et $\mathbb{R}^{3}/H$ est alors homéomorphe à $\mathbb{R}^{3-k-m}\times\mathbb{T}^{m}$. Une injection continue dans $\mathbb{R}$ interdit toute composante de dimension $\geq2$ (invariance du domaine) et tout facteur torique de dimension $\geq1$ (un tore est compact et connexe, donc son image serait un segment, ce qui contredit l'injectivité par un argument de point non séparant). Il reste $\dim\left(\mathbb{R}^{3}/H\right)\leq1$ sans facteur compact, soit $H=\mathbb{R}^{3}$ — exclu par (H3) — soit $H$ hyperplan vectoriel.

*Étape 4 — conclusion.* $H=a^{\perp}$ pour un $a\neq0$, donc $g=\varphi\left(\left\langle a,\cdot\right\rangle\right)$ avec $\varphi$ continue injective sur $\mathbb{R}$, donc strictement monotone. Si $\varphi$ était décroissante, $D(A;c)=\max_{x\in A}\varphi\left(\left\langle a,x-c\right\rangle\right)=\varphi\left(\min_{x\in A}\left\langle a,x-c\right\rangle\right)$ décroîtrait par ajout de points, contredisant (H1b) ; donc $\varphi$ est croissante et $D(A;c)=\varphi\left(\max_{x\in A}\left\langle a,x-c\right\rangle\right)$. $\square$

*Une dichotomie à ne pas écrire.* Il est tentant de conclure « $\varphi$ croissante $\Rightarrow\max$, $\varphi$ décroissante $\Rightarrow\min$ ». C'est une fausse alternative : $\varphi$ décroissante composée avec $\min_a$ **est** une fonction croissante composée avec $\max_{-a}$, si bien que les deux branches décrivent le même ensemble de canaux, tandis que les canaux qui sont des fonctions *décroissantes* d'une valeur de support — par exemple $D=-\max_{x}\left\langle a,x-c\right\rangle$ — n'y figurent pas alors qu'ils vérifieraient (H0)–(H3) sans (H1b). Avec (H1b) le problème disparaît : $\varphi$ est croissante, un seul $a$, un seul $\max$.

*Deux hypothèses de portée, à déclarer.* **(i) Par canal.** (H2) est supposée canal par canal : chacun se recentre à partir de sa seule valeur. La version affaiblie où les $D$ canaux se recentrent conjointement, $D_j(A;c')=G_j\left(D_1(A;c),\ldots,D_D(A;c),c-c'\right)$, n'est pas couverte et sa classe admissible est *a priori* plus large ; c'est une question ouverte, pas un corollaire. **(ii) Aveugle au cardinal et aux répétitions.** (H1) quantifie sur tous les $A,B$, y compris $A=B$ : c'est l'idempotence, donc l'invariance par duplication d'un point, et elle n'est **pas** imposée par la structure d'arbre de fusion, qui ne fusionne jamais que des enfants disjoints. De même $F$ est supposée ne pas voir $\left|A\right|$. Ces deux clauses sont des choix de modélisation à assumer, et non des conséquences : dès que le nœud stocke sa cardinalité — ce que fait l'architecture — le canal barycentrique $\left\langle a,\mathrm{moy}(A)-c\right\rangle$ redevient exactement fusionnable et exactement recentrable, sans être une valeur de support.

*Ce que devient le théorème sans l'idempotence, et pourquoi c'est une bonne nouvelle.* Si l'on remplace (H1) par la seule agrégation sur enfants **disjoints** — l'hypothèse fidèle à l'arbre de fusion — la classe ne se réduit pas à $\left\lbrace\mathrm{somme},\mathrm{LSE},\max\right\rbrace$ : les opérations continues, commutatives et associatives d'un intervalle sont des sommes ordinales de sommes quasi-arithmétiques et de morceaux idempotents (Aczél ; Mostert--Shields). Mais en imposant (H0)+(H2)+(H3) par-dessus, la classe se referme exactement sur $D(A;c)=\psi\left(\beta^{-1}\log\sum_{x\in A}e^{\beta\left\langle a,x-c\right\rangle}\right)$, avec $a\neq0$, $\beta\in\left(0,+\infty\right]$ et $\psi$ continue strictement monotone : **les fonctions support adoucies par log-sum-exp**, dont la fonction support dure est le membre $\beta=+\infty$, et l'unique membre idempotent. C'est un meilleur résultat que l'unicité seule, et il a une conséquence pratique immédiate : la version à température finie est exactement fusionnable elle aussi, tout en distribuant le gradient sur plus d'un point par direction, là où le $\max$ dur n'en alimente qu'un seul. C'est la variante différentiable à tester en priorité contre le support dur.

### Corollaires opérationnels

- **Fusion exacte** : $h_{A\cup B}^{c}(u)=\max\left(h_{A}^{c}(u),h_{B}^{c}(u)\right)$.
- **Recentrage exact** : $h_{A}^{c'}(u)=h_{A}^{c}(u)+\left\langle u,c-c'\right\rangle$.
- **Identité normalisée** déjà écrite dans [ARCHITECTURE.md](ARCHITECTURE.md) : $s_p(u)=\max_{v\in\mathrm{children}(p)}\left[\left\langle u,\frac{c_v-c_p}{R_p}\right\rangle+\frac{R_v}{R_p}s_v(u)\right]$. Le théorème explique **pourquoi** cette identité existe et pourquoi aucun autre descripteur de forme comparable ne l'a.
- **Conséquence négative, à écrire dans le papier** : le canal support d'un nœud interne est une fonction déterministe de ses enfants, donc des feuilles. Il n'apporte aucune information au-delà des feuilles ; il évite seulement au réseau de réapprendre une agrégation qu'on sait calculer exactement. L'ablation `objet complet seul` contre `support + objet complet` mesure donc un effet d'optimisation, pas un effet d'information, et doit être présentée comme telle.

### Antériorité : ce lemme justifie, il ne contribue pas

Il faut calibrer honnêtement ce que vaut ce résultat, sous peine de le voir démonté en review.

- **La fonction support échantillonnée est un PointNet.** $h_{C_v}(u_k)=\max_{x\in C_v}\left\langle u_k,x\right\rangle$ est littéralement un PointNet dont la première couche est linéaire et l'agrégation un max-pooling ; le *critical point set* de PointNet est exactement l'ensemble des points extrémaux. Le canal support n'est donc pas une feature nouvelle : c'est le PointNet le plus simple possible, et il faut le présenter ainsi.
- **L'agrégation par $\max$ sur les enfants d'un arbre de partition est déjà publiée.** Superpoint Transformer (ICCV 2023) l'emploie explicitement dans son équation d'encodeur, $g_p^{i}=T^{i}\circ\varphi^{i}\left(x_p^{i},\max_{q\in\mathrm{children}(p)}g_q^{i-1}\right)$. La fonctorialité par union sur un arbre de clusters n'est pas un terrain vierge.
- **Chaque étape du théorème a un ancêtre publié ; seule leur conjonction ne s'apparie à rien.** Étape 1 — « canal idempotent exactement fusionnable $=$ sup d'une fonction ponctuelle » est le théorème de représentation des mesures idempotentes (Maslov) : Shilkret 1971, Akian, *Trans. AMS* 351 (1999) 4515–4543, Kolokoltsov et Maslov 1997, Litvinov--Maslov--Shpiz 2001. La trichotomie $\left\lbrace\min,\max,\mathrm{med}\right\rbrace$ que la première rédaction avait sautée est Fung--Fu 1975, forme de manuel dans Grabisch, Marichal, Mesiar et Pap, *Aggregation Functions*, CUP 2009. Étape 2 — « lignes de niveau invariantes par translation $+$ continuité $\Rightarrow$ fonction d'une forme linéaire » est l'équation de translation d'Aczél, plus le théorème de structure des sous-groupes fermés de $\mathbb{R}^{n}$. Les identités $h_{A\cup B}=\max(h_A,h_B)$ et le décalage de recentrage sont Schneider, *Convex Bodies*, § 1.7 ; le plongement de Hörmander--Rådström $A\mapsto h_A$ dit déjà que le support est le bon système de coordonnées. Côté ML : DeepSets 2017, PointNet 2017, Wagstaff *et al.* 2019, Janossy pooling. La démonstration ci-dessus est donc du **folklore recombiné, avec une seule étape non triviale** — l'injection continue de $\mathbb{R}^{n}/H$ dans $\mathbb{R}$.
- **Les descripteurs directionnels apprenables plus riches existent déjà** : ECT/DECT différentiable à directions apprises (Röell et Rieck, ICLR 2024), WECT, et leurs implémentations GPU. La fonction support n'est que l'extrémité du filtrage directionnel $\left\langle x,u\right\rangle$ que l'ECT résume entièrement.
- **Une borne d'expressivité s'applique.** Wagstaff *et al.*, ICML 2019 : un encodeur d'ensembles à latent de dimension $D$ ne représente fidèlement que des ensembles de cardinalité au plus $D$. Un descripteur à $D$ directions est donc **prouvablement lossy** dès que $n_v>D$, ce qui est le régime de tous les gros nœuds.

Conclusion de calibrage : ce lemme sert à **justifier** un choix d'architecture et à en dériver l'algorithme exact de remontée. Il ne doit jamais être annoncé comme la contribution, et une soumission qui reposerait dessus serait correctement rejetée.

### Ce que le théorème dit des deux autres canaux

Le témoin propre n'est pas le canal binné mais le **rayon extérieur non binné** $R(A;c)=\max_{x\in A}\left\Vert x-c\right\Vert$. Son $g(y)=\left\Vert y\right\Vert$ est continu, il vérifie (H0), (H1), (H1b) et (H3), et il viole **uniquement** (H2) : $\left\lbrace c+e_1\right\rbrace$ et $\left\lbrace c+e_2\right\rbrace$ donnent la même valeur $1$ au centre $c$, mais $2$ et $\sqrt{2}$ au centre $c-e_1$ — même valeur d'entrée, même décalage, sorties différentes, donc aucun $G$ n'existe.

C'est donc **le recentrage en forme close, et non la continuité, qui sépare les canaux radiaux du canal support.** Le fenêtrage angulaire vient ensuite : avec $g_u(y)=\left\Vert y\right\Vert$ si $y/\left\Vert y\right\Vert\in B_u$ et $-\infty$ sinon, $\rho_{\mathrm{out}}(u)=\max_{x\in A}g_u(x-c)$ perd **en plus** (H3), $g_u$ étant discontinue au bord de fenêtre et non réelle quand $A$ manque le bin — d'où le masque obligatoire. Symétriquement $\rho_{\mathrm{in}}$ est un $\min$-pooling, qui viole (H1b).

Le tableau exact est donc :

| Canal | (H1)+(H1b) fusion exacte | (H2) recentrage clos | (H3) continuité | voit la non-convexité |
|---|---|---|---|---|
| support $h(u)$ | oui, $\max$ | **oui** | oui | non |
| rayon extérieur non binné | oui, $\max$ | **non** | oui | un peu |
| $\rho_{\mathrm{out}}$ binné | oui, $\max$ | **non** | **non** | oui |
| $\rho_{\mathrm{in}}$ binné | $\min$, viole (H1b) | **non** | **non** | oui |

> Un canal directionnel exactement fusionnable est soit exactement recentrable — et c'est alors la fonction support, donc aveugle à tout sauf un hyperplan d'appui de l'enveloppe convexe — soit sensible à la non-convexité, et il faut alors fixer le centre une fois pour toutes.

C'est exactement l'argument qui impose le choix du § suivant : puisqu'on ne peut pas recentrer les canaux radiaux, il ne faut pas essayer — il faut leur donner un centre unique.

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

Le même argument frappe la hiérarchie elle-même, et plus durement : la connexité d'ordre $K$ exige que $K$ points soient simultanément proches, ce que les structures filiformes ne fournissent pas — le manuscrit documente ce mode d'échec sur `birch2`. Voir [ORDRE_DES_PREUVES.md](ORDRE_DES_PREUVES.md).

## Le canal de masse : la CDF projetée

Les trois canaux précédents sont **tous des extrema**. C'est leur limite commune, et elle est théorique : un $\max$-pooling ne peut pas approcher une moyenne — c'est la séparation classique PointNet / DeepSets, dont le centre de masse est le contre-exemple canonique.

### Définition

Prends un nœud et une direction $u$. Projette tous ses points : tu obtiens un nuage de nombres en dimension 1. Le support en garde **un seul**, le maximum. Le canal CDF garde **toute la distribution** :

$F_v(u_j,t_b)=n_v^{-1}\sum_{x\in C_v}\mathbf{1}\left\lbrace\left\langle u_j,(x-c_v)/R_v\right\rangle\leq t_b\right\rbrace$

Le support en est un cas particulier dégénéré : $h(u)=\sup\left\lbrace t:F(u,t)<1\right\rbrace$ et $-h(-u)=\inf\left\lbrace t:F(u,t)>0\right\rbrace$. **La CDF contient les deux valeurs de support et tout ce qu'elles jettent.**

### Une seule filtration, trois lectures

C'est la bonne façon de situer ce canal. Les trois descripteurs directionnels du dossier reposent sur la même construction — le sous-niveau $\left\langle x,u\right\rangle\leq t$ — et ne diffèrent que par ce qu'ils en résument.

| Lecture du sous-niveau | Descripteur | Agrégation | Ce qu'il voit |
|---|---|---|---|
| l'**extremum** | fonction support | $\max$ | l'enveloppe convexe |
| la **masse** | CDF projetée | $+$ | la distribution des points |
| la **topologie** | ECT / WECT | $\chi$ | trous et composantes |

### Elle se fusionne exactement, elle aussi

C'est ce qui la rend compatible avec l'arbre : **les comptes sont additifs**. En stockant les comptes non normalisés dans un repère commun,

$N_v(u_j,t_b)=\#\left\lbrace x\in C_v:\left\langle u_j,x-c_0\right\rangle\leq t_b\right\rbrace$

on a $N_{A\cup B}=N_A+N_B$ pour des enfants disjoints : une addition par case, exacte, une seule passe ascendante. Le support est le membre $\max$ du monoïde, la CDF en est le membre $+$. Et $n_v$ tombe gratuitement — c'est la dernière case — donc la densité aussi.

Deux réserves.

1. **La normalisation par nœud casse l'exactitude sur grille fixe.** Recentrer et redimensionner translate et dilate l'argument, $F^{c'}(u,t)=F^{c}\left(t+\left\langle u,c'-c\right\rangle\right)$ : exact **comme fonction**, mais sur une grille de seuils figée il faut rééchantillonner, donc interpoler. Même mécanique que pour les canaux radiaux : agréger dans un repère global, normaliser à la lecture. Une liste de **quantiles**, elle, ne se fusionne pas du tout.
2. **Pour $K\geq2$, l'additivité double-compterait — et le manuscrit fournit le correctif.** Les $K$-polyèdres se recouvrent, donc un point appartenant à deux composantes serait compté deux fois. Le § 9.1 définit la partition de l'unité $w_{x\tau}=S_\tau/T_x$, avec $S_\tau=\sum_{\sigma\supset\tau,\left|\sigma\right|=K+1}\psi\left(\rho(\sigma)\right)$, $\psi(t)=1/t^{p}$, et $T_x=\sum_{\tau\ni x}S_\tau$. En pondérant chaque point par $w_{x\to v}=\sum_{\tau\in v}w_{x\tau}$, l'additivité redevient **exacte** et la masse est conservée sur toute antichaîne, puisque $\sum_v w_{x\to v}=1$. Le comptage brut $n_v$ doit alors être remplacé partout par cette masse pondérée.

### Deux économies pratiques

- **Antipodes.** Pour le support, $u$ et $-u$ ne sont pas redondants, puisque $h(-u)=-\min_x\left\langle u,x\right\rangle$ : il faut la sphère entière. Pour la CDF, $\left\langle -u,x\right\rangle\leq t\iff\left\langle u,x\right\rangle\geq-t$, donc $F(-u,t)=1-F\left(u,(-t)^{-}\right)$ : la direction opposée est entièrement déterminée, et le plan projectif $\mathbb{RP}^{2}$ suffit — **deux fois moins de directions**. Vérification de cohérence : la CDF sur une demi-sphère fournit les deux extrémités de chaque axe, c'est-à-dire $h$ sur la sphère entière.
- Le facteur $B$ est donc en pratique un facteur $B/2$ à budget d'information égal.

### Ce que ça achète

- **Le cube plein contre sa frontière**, fixture permanente du dossier : mêmes support et rayon extérieur, profils de masse totalement différents. C'est le seul des canaux qui la distingue.
- **La haie contre la clôture** : mêmes $\rho_{\mathrm{in}}$, $\rho_{\mathrm{out}}$ et épaisseur ; l'une a des retours étalés en profondeur, l'autre deux surfaces et du vide entre.
- **La densité et le cardinal**, invisibles à tout $\max$-pooling.

Par **Cramér–Wold**, la collection de toutes les projections 1-D détermine la mesure : à résolution infinie la CDF projetée est un descripteur **complet** de la mesure ponctuelle normalisée, ce que la fonction support n'est prouvablement pas. À grille finie ce n'est qu'un sketch, mais strictement plus riche à budget apparié.

Le seul argument sérieux contre est la dimension, $D\times B$ contre $D$. Il se règle par ablation à budget apparié : `support seul`, `CDF seule`, `les deux`, à $D\times B$ constant.

## Points ou polyèdre reconstruit ?

Question distincte de la précédente, et la réponse dépend du canal.

| Canal | Points ou carrier reconstruit |
|---|---|
| support $h$ | **aucune différence** — c'est une identité, pas une approximation |
| support de $W_v(a)$ | **vraie différence**, la seule reconstruction qui paye |
| CDF / masse | **vrai choix**, et c'est exactement le débat de la portée |
| radial depuis le capteur | **points** ; le carrier n'ajoute que de l'interpolation |

### Pour le support, la question n'a pas d'objet

$h_{C_v^{F}}(u)=\max_{F\in\mathcal{F}_v}\max_{x\in F}\left\langle u,x\right\rangle=h_{V_v}(u)$.

Le support du carrier PL des facettes **est** celui de ses sommets, exactement, pour toute direction. Reconstruire $\bigcup_F\mathrm{conv}(F)$ pour en prendre ensuite le support, c'est payer un travail combinatoire pour retrouver un maximum sur les points. **Calcule $h$ sur les points, toujours.**

### La seule reconstruction qui apporte quelque chose

$W_v(a)$ est le seul des quatre carriers dont le support diffère de celui des points, et ce qu'il mesure est intéressant. Un point témoin $y\in W_v(a)$ doit avoir $K$ observations à distance $\leq\sqrt a$ ; près du bord du nuage il n'y en a pas assez, donc le niveau de densité **recule** par rapport à l'enveloppe des points. La grandeur nouvelle est

$\Delta_v(u)=h_{V_v}(u)-h_{W_v(a)}(u)\geq0$

soit une **mesure directionnelle de la densité au bord** : de combien faut-il rentrer, dans la direction $u$, avant que la condition d'ordre $K$ soit satisfaite. Les points seuls ne la donnent pas, et elle est propre à HGP — un octree ou des superpoints n'ont pas d'équivalent.

Elle est aussi le canal le plus directement lié au risque de portée : $\Delta_v$ grandit quand l'échantillonnage s'amincit. Soit c'est un descripteur utile, soit c'est un thermomètre de portée — et c'est mesurable.

### Comment la calculer sans énumérer les facettes

C'est ce qui rend l'affaire faisable, l'énumération de $\mathcal{F}_v$ étant précisément ce que l'invariant d'architecture interdit. On passe par la caractérisation en ensemble de niveau, $L_K(a)=\left\lbrace y:r_K(y)\leq\sqrt a\right\rbrace$ où $r_K$ est la distance au $K$-ième voisin. Par direction $u_j$ :

1. partir de $t\leftarrow h_{V_v}(u_j)$ ;
2. **dichotomie sur $t$** : tester si le plan $\left\lbrace\left\langle u_j,y\right\rangle=t\right\rbrace$ rencontre $W_v$ ;
3. test d'appartenance en $y$ par requête $K$-NN : $r_K(y)\leq\sqrt a$ **et** les $K$ voisins appartiennent à $C_v$. La seconde condition règle l'appartenance à la **composante** ; le $K$-ième voisin global ne suffit pas.

Coût : $\mathcal{O}\left(D\log(1/\varepsilon)\right)$ requêtes $K$-NN par nœud, sans mosaïque ni énumération. Deux clauses contractuelles : c'est une approximation, donc `authority=witness_approx` avec $\varepsilon_W$ sérialisé et jamais `witness_exact` ; et le balayage doit échantillonner la trace du plan, pas un point unique, sinon on rate les composantes fines.

**Le coût caché est la composabilité.** $W_v(a)$ dépend du **niveau**, donc contrairement à $h_{V_v}$ il ne se compose pas par $\max$ : on n'a que l'encadrement $h_{W_p}(u)\geq\max_{v\in\mathrm{enfants}}h_{W_v}(u)$, l'écart venant des facettes nées entre $a_p^{-}$ et $a_p$ et de la croissance des boules. À `cut_policy=pre_parent` tous les enfants sont lus au même niveau, donc la borne est serrée et corrigible par les deltas — mais c'est un recalcul par nœud, pas une passe ascendante gratuite.

### Pour la CDF, le choix est réel, et c'est le débat de la portée

Points et carrier mesurent deux grandeurs **différentes** :

- **sur les points**, la **masse d'échantillonnage** : combien de retours, et où. Le signal LiDAR le plus fort, et le plus contaminé par la portée ;
- **sur le carrier**, une grandeur **géométrique** : longueur, aire ou volume, invariante à la densité d'échantillonnage.

Leur différence **est** le diagnostic de portée. Calculer les deux et regarder leur écart en fonction de la distance est plus informatif que l'un ou l'autre isolément.

**Piège dimensionnel à connaître avant de coder.** Dans $\mathbb{R}^{3}$, $\mathrm{conv}(F)$ avec $\left|F\right|=K$ est de dimension $K-1$ : pour $K=2$ des **segments**, pour $K=3$ des **triangles**, donc de mesure de Lebesgue **nulle**. Une CDF de *volume* du carrier PL est identiquement dégénérée dans le régime $K=2,3$ prévu. Il faut pondérer par la mesure de Hausdorff de la bonne dimension — longueur pour $K=2$, aire pour $K=3$. C'est faisable en forme close : la projection d'un segment est une densité **uniforme** sur un intervalle, celle d'un triangle est **affine par morceaux**, et le tout reste additif sur les facettes.

### Pour les canaux radiaux, reste sur les points

Depuis l'origine capteur, un scan mono-retour donne au plus un point par faisceau : le nuage du cluster est déjà exactement étoilé, et $\rho_{\mathrm{out}}$ sur les points est déjà la description complète. Reconstruire le carrier n'ajoute que de l'**interpolation entre faisceaux** — un lissage, pas de l'information nouvelle.

### Ordre d'implémentation

1. $h$ sur les points — gratuit, exact, une passe ascendante ;
2. CDF sur les points — additive, exacte, une passe ascendante ;
3. $\rho_{\mathrm{in}}$, $\rho_{\mathrm{out}}$, épaisseur, masque, comptes, depuis le capteur — mêmes propriétés ;
4. **puis seulement** $\Delta_v(u)$, par dichotomie $K$-NN, en `witness_approx`, avec son coût mesuré ;
5. CDF géométrique du carrier PL, en contrôle, uniquement pour le diagnostic de portée.

Les trois premiers ne demandent aucune reconstruction et se remplissent en une seule remontée. Le quatrième est le seul qui justifie de reconstruire quoi que ce soit, et c'est aussi le seul à casser la composabilité — donc celui à ablater le plus sévèrement.

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

**Le tenseur de CDF projetées $F_v(u_j,t_b)$ doit figurer dans cette table dès le départ**, au même titre que les canaux directionnels : il est additif donc exactement fusionnable, strictement plus expressif que les trois extrema, et c'est le seul canal qui répond à la fixture « cube plein contre frontière ». Voir la section dédiée ci-dessus.

À cela s'ajoutent les canaux non normalisés déjà listés dans [ARCHITECTURE.md](ARCHITECTURE.md) — $\log R_v$, $\log(1+n_v)$, portée du centre, naissance/mort/persistance, valeurs propres de covariance, statistiques de rémission — qui portent l'échelle absolue que la normalisation détruit et qui, sur SemanticKITTI, sont probablement plus discriminants que la forme elle-même.

## Ce qu'il faut mesurer avant d'écrire une ligne de modèle

Deux diagnostics, sans aucun entraînement, tranchent l'essentiel.

1. **Oracle d'antichaîne.** Sur la séquence 08, construire la forêt HGP, sélectionner une antichaîne à budget de régions fixé, étiqueter chaque nœud par sa classe majoritaire et rapporter le mIoU obtenu, contre HDBSCAN, octree/voxel, partition superpoint et arbre aléatoire à compression égale. Le protocole exact — et pourquoi « la meilleure antichaîne au sens du mIoU » n'est pas un objectif bien posé — est dans [ORDRE_DES_PREUVES.md](ORDRE_DES_PREUVES.md). Si HGP ne domine aucun contrôle, le programme est réfuté en une semaine.
2. **Stabilité par portée.** Transporter le même objet à plusieurs portées, rééchantillonner selon un modèle capteur déclaré, et mesurer la dérive des niveaux de naissance/mort et de l'ancêtre commun. C'est le test de [R1](RISQUES.md) ; il conditionne tout le reste, car une hiérarchie qui encode la portée n'est pas une hiérarchie sémantique.

Ces deux mesures coûtent moins qu'une seule journée de GPU et valent plus que toute extension supplémentaire de la spécification.
