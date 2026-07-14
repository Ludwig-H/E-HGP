# Addendum — DTM, hiérarchie sparse et régularisation GPU

- **Date :** 13 juillet 2026
- **Dépôt :** [`Ludwig-H/E-HGP`](https://github.com/Ludwig-H/E-HGP)
- **Révision vérifiée :** [`70423fa354e4f0252f8a8eddcf5e311bf1a4ac42`](https://github.com/Ludwig-H/E-HGP/tree/70423fa354e4f0252f8a8eddcf5e311bf1a4ac42)
- **Complément à :** [`RAPPORT_PERG_HGP_GRANDE_DIMENSION_2026-07-12.md`](https://github.com/Ludwig-H/E-HGP/blob/70423fa354e4f0252f8a8eddcf5e311bf1a4ac42/perg_hgp/RAPPORT_PERG_HGP_GRANDE_DIMENSION_2026-07-12.md)
- **Cible :** ordre $K\leq 10$, dimension ambiante quelconque ; cas prioritaire $n\simeq 50\,000$ en 3D, latence chaude visée inférieure à une seconde

---

## Résumé exécutif

La conclusion de cet addendum est plus précise que « utiliser de l'entropie » :

> **La bonne relaxation locale est la DTM, c'est-à-dire un transport partiel sur un simplexe capé. La bonne régularisation entropique est un KL sur ce simplexe capé. La bonne sparsification est un coreset de mesures locales transformées en sites de puissance. La bonne structure de hiérarchie $H_0$ est ensuite un MST de puissance dur.**

Le pipeline recommandé est donc

$$
\boxed{ \text{transport partiel capé, dur ou KL} \longrightarrow \text{sites de puissance témoins} \longrightarrow \text{graphe sparse certifiable} \longrightarrow \text{power-MST GPU} }.
$$

Il faut retenir six points.

1. **La DTM est déjà une régularisation du rayon $K$-NN.** Elle remplace le maximum des $K$ premières distances par leur moyenne quadratique. Pour l'objectif de vitesse pure, la witnessed DTM dure, sans entropie supplémentaire, est le premier prototype à construire.
2. **La contrainte essentielle est $q_j\leq 1/K$.** Le softmax actuellement utilisé par `PowerCover3D` vit sur un simplexe non capé ; il ne définit donc pas une DTM. À `kappa=1`, il donne une masse de Dirac, pas la mesure uniforme sur les $K$ voisins.
3. **Le KL capé n'est pas un Sinkhorn complet.** Le transport part d'un seul Dirac : la solution est un *capped softmax* obtenu par un seuil scalaire, sans matrice de transport ni alternance lignes/colonnes.
4. **L'entropie ne crée pas la sparsité topologique.** À température positive, Shannon densifie même la masse locale. La hiérarchie devient sparse grâce aux témoins ou au coreset de sites, au graphe candidat et au MST. Une pénalisation quadratique de type sparsemax/Tsallis est une ablation utile si des zéros exacts sont recherchés.
5. **Pour $K=1$, la limite dure est exactement l'EMST.** À température positive, le bon objet discret est le MST de puissance des barycentres entropiques locaux. Pour tout $K>1$, le même MST subsiste parce que $K$ a été déplacé *dans* la mesure locale.
6. **La DTM change l'estimateur HGP.** Elle ne calcule pas exactement la multicouverture de rayon $r_K$. Pour conserver HGP en faible dimension, le résultat théorique le plus intéressant est désormais la sparse multicover d'Alonso, linéaire en $n$ pour $d$ et $\varepsilon$ fixés. En grande dimension intrinsèque, aucun résultat cité ici ne garantit encore un backend HGP-$H_0$ à la fois fidèle, certifié et GPU-friendly ; cela ne constitue pas une preuve d'impossibilité.

La proposition pratique n'est donc pas de remplacer la grille actuelle par une autre discrétisation. Elle est de créer un backend distinct, par exemple `dtm_power_mst`, avec trois modes d'atomes :

- `hard` : witnessed DTM dure, référence scientifique et vitesse maximale ;
- `kl_free_energy` : DTM-KL au sens variationnel exact sur le support choisi ;
- `kl_transport_only` : l'entropie choisit l'atome, mais le terme KL n'est pas incorporé dans le poids final.

Le backend HGP fidèle doit rester séparé, par exemple sous le nom `sparse_multicover_h0`.

---

## 1. Ce que signifie réellement « hiérarchie sparse »

Quatre sparsités différentes sont souvent confondues.

| Couche | Objet à réduire | Mécanisme pertinent | Rôle de l'entropie |
|---|---|---|---|
| Masse locale | nombre de voisins portant $q$ | top-$L$, cap $1/K$, éventuellement sparsemax | KL lisse mais ne crée pas de zéros |
| Sites | nombre de fonctions de puissance | témoins, $\delta$-réseau, $M$-PDTM, génération de colonnes | sélectionne/stabilise chaque atome |
| Arêtes | graphe sur les sites | ANN, bottleneck spanner, Borůvka à raffinement | aucun |
| Sortie | nombre d'événements du merge tree | MST puis simplification par persistance | aucun |

Cette séparation est structurante : **régulariser davantage ne compense pas un nombre excessif de sites ou d'arêtes**. Réciproquement, une hiérarchie $H_0$ sur $M$ sites possède déjà une représentation de taille $O(M)$ par un MST, même si le champ sous-jacent est défini sur tout $\mathbb{R}^p$.

La difficulté restante n'est donc plus une mosaïque de Delaunay d'ordre supérieur. Elle devient :

1. construire assez peu de sites pour approcher le champ voulu ;
2. trouver assez d'arêtes pour contenir ou approcher le MST complet ;
3. rendre explicite l'erreur due aux supports ANN et au coreset.

---

## 2. DTM : la relaxation convexe naturelle du $K$-NN

### 2.1 Du rayon d'ordre à la moyenne quadratique

Soit

$$
X=\{x_1,\ldots,x_n\}\subset\mathbb{R}^p,
$$

et soient

$$
r_1(y)\leq\cdots\leq r_n(y)
$$

les distances ordonnées de $y$ aux observations. Le champ HGP dur utilise

$$
r_K(y).
$$

La DTM empirique de masse $m=K/n$ est

$$
D_K^2(y)=\frac{1}{K}\sum_{j=1}^{K}r_j^2(y).
$$

Elle remplace donc l'agrégateur $L^\infty$ sur les $K$ voisins par un agrégateur $L^2$. Cette modification est volontaire : elle est précisément celle qui donne une identité barycentrique quadratique et, par suite, des sites de puissance.

La DTM a été introduite comme un champ robuste et stable en Wasserstein par Chazal, Cohen-Steiner et Mérigot dans [*Geometric Inference for Probability Measures*](https://doi.org/10.1007/s10208-011-9098-0). Pour une masse $m$ fixée, elle est 1-lipschitzienne en $y$ et satisfait notamment

$$
\lVert D_{\mu,m}-D_{\nu,m}\rVert_\infty \leq m^{-1/2}W_2(\mu,\nu).
$$

Attention au régime statistique : si $K=10$ reste fixé tandis que $n\to\infty$, alors $m=K/n\to0$. Ce n'est pas le régime classique de consistance d'un estimateur de densité. Pour une théorie asymptotique, on demande généralement $K_n\to\infty$ et $K_n/n\to0$. Le choix $K=10$ reste parfaitement légitime comme objet géométrique à taille finie, mais il ne faut pas lui attribuer une garantie asymptotique qu'il n'a pas.

### 2.2 Formulation comme transport partiel capé

Définissons

$$
\mathcal C_{K,n} = \left\lbrace  q\in\mathbb{R}_+^n: \sum_{j=1}^nq_j=1, \quad q_j\leq\frac{1}{K} \right\rbrace .
$$

Alors

$$
\boxed{ D_K^2(y) = \min_{q\in\mathcal C_{K,n}} \sum_{j=1}^nq_j\lVert y-x_j\rVert^2 }.
$$

Le polytope $\mathcal C_{K,n}$ est un hypersimplexe redimensionné. Ses sommets sont exactement les mesures uniformes sur les sous-ensembles de cardinal $K$ :

$$
q^S_j=\frac{1}{K}\mathbf{1}_{\{j\in S\}}, \qquad |S|=K.
$$

Le minimum linéaire choisit donc les $K$ plus petites distances sans avoir à énumérer les $\binom nK$ sous-ensembles.

Ce point répond en partie à la question des interactions d'ordre supérieur :

> **la contrainte de cardinalité $K$ est représentée exactement au niveau des marginales par la géométrie du simplexe capé, sans hypergraphe explicite.**

Cette relaxation marginale ne conserve pas les corrélations conjointes entre inclusions de points. La Gibbs sur les $K$-sous-ensembles de la section 3.7 est la variante véritablement higher-order ; elle est plus coûteuse et ses corrélations disparaissent dès que l'on compresse la loi en un seul barycentre et une variance.

Une simple contrainte de perplexité ou un softmax non capé ne conserve pas cette interprétation. Le cap $1/K$ garantit qu'aucune observation ne peut porter plus d'un $K$-ième de la masse : une mesure admissible doit donc utiliser au moins $K$ observations.

### 2.3 L'identité qui supprime la combinatoire

Pour toute mesure admissible $q$, posons

$$
z(q)=\sum_jq_jx_j,
$$

et

$$
v(q)=\sum_jq_j\lVert x_j-z(q)\rVert^2.
$$

L'identité barycentrique donne, pour tout $y$,

$$
\sum_jq_j\lVert y-x_j\rVert^2 = \lVert y-z(q)\rVert^2+v(q).
$$

Une mesure locale complète est donc résumée exactement par :

- un centre $z(q)\in\mathbb{R}^p$ ;
- un scalaire additif $v(q)\geq0$.

La DTM exacte est l'enveloppe inférieure de ces fonctions de puissance pour tous les $q\in\mathcal C_{K,n}$. Dans le cas dur, il suffirait en principe de considérer tous les barycentres de $K$-sous-ensembles, ce qui est à nouveau combinatoire. La sparsification consiste à ne conserver qu'un ensemble bien choisi de mesures $q$.

### 2.4 Relation honnête avec la hiérarchie HGP

On a toujours

$$
\frac{r_K(y)}{\sqrt{K}} \leq D_K(y) \leq r_K(y),
$$

d'où

$$
\{r_K\leq r\} \subseteq \{D_K\leq r\} \subseteq \{r_K\leq\sqrt{K}\,r\}.
$$

Pour $K=10$, le facteur uniforme $\sqrt{10}\simeq3{,}16$ est trop lâche pour annoncer une équivalence des deux arbres.

Il explique cependant mal le régime usuel. Sous une densité localement régulière de dimension intrinsèque $d_\star$, les quantiles de distance vérifient heuristiquement

$$
r_u(y) \simeq \left(\frac{u}{f(y)\omega_{d_\star}}\right)^{1/d_\star}.
$$

En intégrant les carrés de ces quantiles,

$$
D_m^2(y) \simeq \frac{d_\star}{d_\star+2} r_m^2(y).
$$

Ainsi, après une reparamétrisation presque constante,

$$
D_m(y) \simeq \sqrt{\frac{d_\star}{d_\star+2}}\,r_m(y).
$$

Cette relation est une approximation locale, pas un théorème uniforme. Elle donne néanmoins deux indications utiles :

- en 3D, le facteur limite vaut $\sqrt{3/5}\simeq0{,}775$ ; sous un modèle de Poisson conditionné par $r_{10}$, une requête indépendante donne une correction de taille finie proche de $0{,}80$, tandis qu'une ancre observée avec auto-voisin inclus donne plutôt $\sqrt{0{,}58}\simeq0{,}762$ ;
- lorsque $d_\star$ augmente, le facteur tend vers 1 et les premières distances se concentrent : la DTM devient alors numériquement plus proche du rayon $K$-NN, même si l'estimation de densité devient statistiquement plus difficile.

La DTM est donc un surrogate particulièrement plausible en grande dimension, mais sa fidélité doit être mesurée sur les arbres et les coupes, pas déduite de la seule borne $\sqrt{K}$.

---

## 3. La régularisation entropique correcte

### 3.1 KL sur le simplexe capé

Pour un support de $L$ candidats et la mesure uniforme $u_L=(1/L,\ldots,1/L)$, définissons

$$
D_{K,\tau,L}^2(y) = \min_{q\in\mathcal C_{K,L}} \left[ \sum_{j=1}^Lq_jc_j(y) + \tau\,\mathrm{KL}(q\Vert u_L) \right],
$$

avec

$$
c_j(y)=\lVert y-x_j\rVert^2.
$$

Les conditions KKT donnent

$$
\boxed{ q_j^\tau(y) = \min\left\lbrace  \frac{1}{K}, A(y)e^{-c_j(y)/\tau} \right\rbrace , \qquad \sum_jq_j^\tau(y)=1 }.
$$

Le facteur $1/L$ de la référence uniforme est absorbé dans $A$. Lorsque $\tau\downarrow0$, et hors ex æquo,

$$
q_j^\tau(y) \longrightarrow \frac{1}{K}\mathbf{1}_{\{j\in N_K(y)\}}.
$$

Ce calcul n'est pas un Sinkhorn matriciel. Si $L=K$, la contrainte impose directement $q_j=1/K$. Si $L>K$ et si les coûts sont déjà triés par la recherche de voisins, on peut tester le nombre $s\in\{0,\ldots,K-1\}$ de coordonnées saturées, puis écrire

$$
A_s = \frac{1-s/K}{\sum_{j>s}e^{-c_j/\tau}}.
$$

Il suffit de choisir le $s$ compatible avec les inégalités de saturation. Pour $K=10$ et $L=32$, $64$ ou $128$, cela tient dans un bloc CUDA par ancre, avec un log-sum-exp stable et sans stocker la matrice globale des $q$.

### 3.2 Borne de biais et choix de la température

Soit $q^0$ la mesure uniforme sur les $K$ voisins durs. Comme

$$
\mathrm{KL}(q^0\Vert u_L)=\log(L/K),
$$

on obtient la borne élémentaire

$$
0 \leq D_{K,\tau,L}^2(y)-D_{K,L}^2(y) \leq \tau\log(L/K).
$$

Par conséquent,

$$
0 \leq D_{K,\tau,L}(y)-D_{K,L}(y) \leq \sqrt{\tau\log(L/K)}.
$$

Ici, $D_{K,L}$ désigne l'optimum dur sur le même support de $L$ candidats. Il coïncide avec la DTM globale dès que ce support contient les vrais $K$ plus proches voisins.

Cette inégalité suggère une API fondée sur un **budget de biais en rayon** $\eta_{\mathrm{ent}}$ plutôt que sur une température opaque :

$$
\tau \leq \frac{\eta_{\mathrm{ent}}^2}{\log(L/K)}.
$$

Cette règle suppose $L>K$. Si $L=K$, l'ensemble admissible est réduit à $q_j=1/K$ : le biais est nul pour toute $\tau$ et la division par $\log(L/K)=0$ n'a pas lieu d'être.

Pour le fonctionnel global sur les $n$ observations, $L$ est remplacé par $n$. La dépendance $\log(n/K)$ rappelle qu'une température numériquement identique ne produit pas le même biais lorsque la taille du support change.

Le choix scientifique par défaut devrait être :

1. `hard`, soit $\tau=0$ ;
2. une petite grille de budgets $\eta_{\mathrm{ent}}$ ;
3. sélection par stabilité/qualité de l'arbre, et non parce qu'une température positive serait intrinsèquement plus GPU-friendly.

### 3.3 Fonctionnel global, support local et certificat de queue

Une entropie de Shannon à température positive a une queue dense. Remplacer silencieusement les $n$ observations par un atlas local de $L$ voisins change donc le fonctionnel.

Deux contrats sont possibles.

#### Contrat local assumé

On définit explicitement le problème sur les $L$ voisins de chaque ancre, avec référence $u_L$. Il s'agit d'un estimateur localisé ; la borne de biais utilise $\log(L/K)$. C'est le mode le plus simple et probablement le plus rapide.

#### Contrat global avec encadrement

On conserve la référence globale $u_n$. Pour une ancre donnée, soit $S_L$ l'ensemble des $L$ plus petits coûts exacts et soit

$$
c_g=c_{(L+1)}
$$

le coût garde.

- Forcer $q_j=0$ hors de $S_L$ donne un **majorant** de l'optimum global.
- Remplacer optimistement les $n-L$ coûts omis par $c_g$ donne un **minorant**. Par convexité de l'entropie, le groupe omis peut être agrégé, mais pas traité comme une coordonnée ordinaire capée à $1/K$.

Si $t$ est la masse totale omise, le problème minorant exact est

$$
\begin{aligned} \min_{q,t}\quad &\sum_{j\in S_L}q_jc_j+t c_g\\ &+\tau\left[ \sum_{j\in S_L}q_j\log(nq_j) +t\log\frac{nt}{n-L} \right], \end{aligned}
$$

sous les contraintes

$$
\sum_{j\in S_L}q_j+t=1, \qquad 0\leq q_j\leq1/K, \qquad 0\leq t\leq\min\left(1,\frac{n-L}{K}\right).
$$

Le terme agrégé vient du partage entropiquement optimal $t/(n-L)$ entre les coordonnées omises. Le problème garde un dual scalaire et une multiplicité connue.

L'écart entre ces deux valeurs est un certificat de troncature par ancre. Si l'écart est trop grand, on augmente $L$ localement. Ce certificat est rigoureux si la liste et le garde sont exacts ; avec CAGRA ou RBC non certifié, il devient conditionnel à la qualité de l'ANN.

Cette stratégie est plus honnête que de présenter un Shannon tronqué comme le fonctionnel global exact. Elle reste très GPU-friendly : le solveur est toujours une réduction par ligne avec quelques scalaires supplémentaires.

### 3.4 Deux conventions de poids à ne pas mélanger

À une ancre $a$, calculons une mesure $q_a^\tau$ et posons

$$
z_a=\sum_jq_{a,j}^\tau x_j, \qquad v_a=\sum_jq_{a,j}^\tau\lVert x_j-z_a\rVert^2.
$$

Deux champs différents peuvent ensuite être construits.

#### Libre énergie KL littérale

On prend

$$
b_a^{\mathrm{FE}} = v_a + \tau\,\mathrm{KL}(q_a^\tau\Vert u),
$$

et

$$
W_{A,K,\tau}^{\mathrm{FE}}(y) = \min_{a\in A} \sqrt{\lVert y-z_a\rVert^2+b_a^{\mathrm{FE}}}.
$$

Si les $q_a^\tau$ sont calculés pour un même problème global — même ensemble admissible, même référence $u$ et même $\tau$ — ce champ est une restriction par témoins du fonctionnel DTM-KL. C'est la convention à employer si le backend annonce « entropic DTM » au sens variationnel. Des supports/références propres à chaque ancre définissent au contraire un estimateur localisé et ne bénéficient pas automatiquement de cette interprétation globale.

#### Atomes sélectionnés par entropie, coût de transport seul

On prend seulement

$$
b_a^{\mathrm{TO}}=v_a,
$$

puis

$$
W_{A,K,\tau}^{\mathrm{TO}}(y) = \min_{a\in A} \sqrt{\lVert y-z_a\rVert^2+b_a^{\mathrm{TO}}}.
$$

L'entropie sert ici à choisir une mesure locale stable, mais le champ final reste un coût de transport. Cette version est naturelle lorsque la température varie par ancre. Il faut la nommer `entropy_selected_witnessed_dtm` ou `kl_transport_only`, pas « DTM-KL exacte ».

La seconde convention reste un majorant de la DTM dure, puisque chaque $q_a^\tau$ est admissible dans $\mathcal C_K$. À l'ancre, son surcoût quadratique par rapport au problème dur sur le **même support** est borné par $\tau\log(L/K)$. La comparaison à la DTM globale exige que le support contienne les vrais $K$-NN ; avec la référence globale $u_n$, la borne brute devient $\tau\log(n/K)$ sauf certificat de queue plus fin.

### 3.5 Faut-il calibrer la température comme UMAP ?

Une idée de type UMAP consiste à résoudre, pour chaque ancre,

$$
\exp H(q_a)=\rho K, \qquad 1\leq\rho\leq L/K.
$$

L'entropie est non décroissante avec $\tau_a$. En présence d'ex æquo, elle n'est pas nécessairement strictement croissante et sa limite quand $\tau\downarrow0$ peut dépasser $\log K$. Une bissection batched ne doit donc être lancée qu'après avoir vérifié que la cible appartient à l'intervalle d'entropie réellement atteignable ; les bornes $\log K$ et $\log L$ sont en général des limites, non des valeurs finies garanties. Cette calibration a deux intérêts :

- une largeur effective de support interprétable ;
- un calcul parallèle presque identique à la résolution du paramètre local de UMAP.

Elle a aussi un risque majeur : normaliser toutes les ancres à la même perplexité peut effacer une partie du contraste de densité que l'on cherche à hiérarchiser. En outre, des $\tau_a$ variables ne correspondent plus à un unique fonctionnel global DTM-KL.

Recommandation :

- température globale ou budget de biais global pour le backend principal ;
- perplexité locale uniquement comme ablation `adaptive_temperature` ;
- ne jamais utiliser le graphe fuzzy UMAP comme topologie finale.

Les idées utiles d'UMAP sont la recherche ANN et la résolution locale d'un scalaire. Sa symétrisation fuzzy et son embedding ne préservent pas la multicouverture ni le merge tree DTM.

### 3.6 Alternative exactement sparse : quadratic/Tsallis

Si l'objectif prioritaire est une masse locale avec des zéros exacts, on peut remplacer KL par

$$
\frac{\tau}{2}\lVert q-u_L\rVert_2^2.
$$

Le problème

$$
\min_{q\in\mathcal C_{K,L}} \left[ \langle q,c\rangle + \frac{\tau}{2}\lVert q-u_L\rVert_2^2 \right]
$$

a pour solution

$$
q_j = \min\left\lbrace  \frac{1}{K}, \left[A-\frac{c_j}{\tau}\right]_+ \right\rbrace .
$$

Il s'agit d'un sparsemax capé, relié aux entropies de Tsallis. Son support est fini même à température positive. Une évaluation sur la mesure dure donne

$$
0 \leq D_{K,\tau,\mathrm{quad}}^2-D_K^2 \leq \frac{\tau}{2} \left(\frac{1}{K}-\frac{1}{L}\right).
$$

Ce solveur demande un scan ou un petit tri, ce qui reste excellent sur GPU. Il doit être comparé au KL, mais non substitué sans le dire : l'analogie avec le transport entropique/Sinkhorn devient moins directe.

Dans le pipeline recommandé, la différence de sparsité locale est moins décisive qu'il n'y paraît : les $q_a$ ne sont pas conservés après le calcul de $(z_a,b_a)$. Le principal avantage de Tsallis est donc de couper exactement la queue et de réduire le nombre de voisins effectivement relus, pas de réduire le nombre final de sites.

### 3.7 Gibbs sur les $K$-sous-ensembles : vraie interaction, mauvais défaut production

Une autre régularisation porte une loi de Gibbs sur les sous-ensembles $S$ de cardinal exactement $K$ :

$$
\Pi_\tau(S) \propto \exp\left[ -\frac{1}{K\tau} \sum_{j\in S}c_j \right].
$$

La fonction de partition est un polynôme symétrique élémentaire. Elle se calcule en $O(LK)$ et les marginales en $O(LK)$ par une passe préfixe/suffixe, sans énumérer $\binom LK$ sous-ensembles. Cette piste encode explicitement le tirage sans remise et ses corrélations négatives.

Cependant, après réduction à un site de puissance, seuls les premiers moments marginaux

$$
q_j=\frac{1}{K}\mathbb{P}(j\in S)
$$

subsistent. Les corrélations d'ordre supérieur ne sont plus visibles dans $(z,v)$. Pour construire une hiérarchie $H_0$ de puissance, le simplexe capé KL fournit donc une formulation plus directe et moins chère.

Cette Gibbs garde un intérêt comme :

- champ de rang différentiable ;
- générateur de candidats ;
- ablation scientifique « higher-order explicite ».

Elle ne devrait pas être le noyau de production par défaut.

L'audit du dépôt renforce cette conclusion. Dans [`rank_field.py`](https://github.com/Ludwig-H/E-HGP/blob/70423fa354e4f0252f8a8eddcf5e311bf1a4ac42/perg_hgp/perg_hgp/rank_field.py), la libre énergie actuelle n'est pas normalisée par $\binom m k$. À température finie, le shell

$$
Q_k=F_k-F_{k-1}
$$

porte donc le décalage combinatoire

$$
-\varepsilon\log\frac{m-k+1}{k}.
$$

Le calcul n'est pas intégralement log-domain, puis certaines différences de marginales sont clampées et renormalisées. Ce module convient à l'exploration de candidats ; il ne constitue pas encore une définition numérique de référence.

---

## 4. Des mesures locales aux sites de puissance

### 4.1 Witnessed DTM dure

La [*Witnessed $k$-Distance*](https://arxiv.org/abs/1102.4972) conserve une mesure par observation. Pour chaque témoin $x_i$, on prend

$$
q_i^0 = \frac{1}{K} \sum_{j\in N_K(x_i)}e_j,
$$

puis

$$
z_i^0=\sum_jq_{i,j}^0x_j, \qquad v_i^0=\sum_jq_{i,j}^0\lVert x_j-z_i^0\rVert^2.
$$

Le champ

$$
D_K^{W}(y) = \min_i \sqrt{\lVert y-z_i^0\rVert^2+v_i^0}
$$

n'utilise que $n$ sites, au lieu de $\binom nK$. Buchet, Chazal, Oudot et Sheehy donnent notamment la comparaison générale améliorée

$$
D_K\leq D_K^W\leq\sqrt{6}\,D_K
$$

dans [*Efficient and Robust Persistent Homology for Measures*](https://arxiv.org/abs/1306.0039).

Cette borne multiplicative est indépendante de l'échantillonnage mais reste large. Une borne additive beaucoup plus informative apparaît sur un domaine couvert par les ancres.

### 4.2 Coreset de témoins et borne $2\delta$

Soit $A$ un ensemble d'ancres qui forme un $\delta$-réseau du domaine pertinent $\Omega$. À chaque $a\in A$, supposons que $q_a^\star$ soit l'optimum exact d'un **même fonctionnel global** au point $a$. Définissons

$$
W_A(y) = \min_{a\in A} \sqrt{ \sum_jq_{a,j}^\star\lVert y-x_j\rVert^2 +R(q_a^\star) },
$$

où $R=0$ pour la DTM dure et $R=\tau\mathrm{KL}$ pour la libre énergie KL.

La même famille admissible et le même $R$ doivent être utilisés pour toutes les positions. Une référence $u_L$ ou une température qui change avec l'ancre n'entre pas directement dans cette preuve ; il faut alors mesurer le résidu par rapport à un oracle commun.

Alors, sur $\Omega$,

$$
\boxed{ 0\leq W_A(y)-D(y)\leq2\delta }.
$$

En effet :

1. chaque atome est admissible, donc $W_A\geq D$ ;
2. $W_A(a)=D(a)$ aux ancres ;
3. $D$ et chaque racine de fonction de puissance sont 1-lipschitziennes ;
4. pour une ancre $a$ à distance au plus $\delta$ de $y$,

   $$
   W_A(y) \leq W_A(a)+\delta =D(a)+\delta \leq D(y)+2\delta.
   $$

Si l'atome n'est qu'approché et si son résidu en valeur à l'ancre est au plus $\rho$, la borne devient

$$
0\leq W_A-D\leq2\delta+\rho.
$$

Cette forme résiduelle est celle qui convient au mode `transport_only` : l'atome entropique n'est pas l'optimum de la DTM dure, donc l'égalité $W_A(a)=D_K(a)$ n'est généralement pas vraie. Il faut calculer ou borner explicitement $W_A(a)-D_K(a)$.

Pour la DTM dure, combiner cette erreur de coreset avec la comparaison à $r_K$ donne un surrogate explicite. Pour la DTM-KL, on ajoute le budget entropique $\sqrt{\tau\log(L/K)}$ lorsqu'on la compare à la DTM dure.

### 4.3 $M$-PDTM : compression apprise

La [k-PDTM](https://arxiv.org/abs/1801.10346) de Brécheteau et Levrard — notée ici **$M$-PDTM** pour ne pas confondre le nombre de sites $M$ avec l'ordre $K$ — apprend directement $M\ll n$ sites de puissance :

$$
D_{K,M}^{\mathrm{PDTM}}(y) = \min_{1\leq\ell\leq M} \sqrt{\lVert y-z_\ell\rVert^2+b_\ell}.
$$

Sous des hypothèses de régularité et de dimension intrinsèque $d_\star$, la littérature obtient une erreur intégrée en carré de l'ordre de $M^{-2/d_\star}$ ; le compromis statistique suggère dans certains régimes

$$
M\asymp n^{d_\star/(d_\star+4)}.
$$

Cette méthode est attractive parce que ses étapes de type Lloyd sont vectorisables. Ses limites sont importantes pour le clustering hiérarchique :

- objectif non convexe ;
- garantie principalement intégrée plutôt qu'uniforme ;
- un petit cluster rare peut coûter peu dans l'objectif moyen et être supprimé ;
- lorsque $d_\star$ est élevé, l'exposant se rapproche de 1 et la compression disparaît.

La $M$-PDTM est donc une option de compression, pas le seul garde-fou topologique. Une sélection par résidu maximal ou par persistance doit compléter son objectif moyen.

### 4.4 Proposition nouvelle : génération de colonnes guidée par les selles

La représentation par mesures locales suggère un algorithme itératif simple. Fixons d'abord un oracle commun $D$ : DTM dure pour les atomes `hard`/`transport_only`, ou DTM-KL globale pour les atomes `free_energy`. Soit $\mathcal Q_t$ l'ensemble courant d'atomes admissibles pour cet oracle. Le champ

$$
W_t(y) = \min_{q\in\mathcal Q_t} \sqrt{\lVert y-z(q)\rVert^2+b(q)}
$$

est toujours un majorant du fonctionnel DTM ciblé. Ajouter un atome ne peut que le faire décroître :

$$
W_{t+1}\leq W_t, \qquad W_t\geq D.
$$

On obtient ainsi une approximation **anytime, monotone et d'un seul côté**.

#### Candidats critiques naturels

Construisons le power-MST du champ courant. Pour chaque arête $(i,j)$ de ce MST, le premier point de contact de ses deux boules est un candidat de selle du surrogate. Notons-le $y_{ij}$. On évalue alors :

$$
\rho_{ij} = W_t(y_{ij})-D(y_{ij}).
$$

Pour la DTM dure, calculer $D(y_{ij})$ et son atome optimal revient simplement à chercher les $K$ voisins de $y_{ij}$ et à leur donner la masse $1/K$. Pour la DTM-KL, on applique le capped softmax, avec éventuellement le certificat de queue.

Si $\rho_{ij}$ est grand, l'arête de MST prétend fusionner deux composantes à un niveau que la vraie DTM peut abaisser. Avec un oracle exact, ajouter l'optimiseur $q^\star(y_{ij})$ du **fonctionnel cible** corrige exactement le champ au point critique interrogé.

Dans le mode `transport_only`, le fonctionnel cible de certification est la DTM dure : l'atome ajouté à cette étape doit donc être la mesure dure uniforme sur les $K$-NN. Ajouter seulement un nouvel atome entropique transport-only fait décroître le majorant, mais ne garantit pas un résidu nul au point interrogé.

Cette heuristique est topologiquement beaucoup plus ciblée qu'un échantillonnage uniforme : elle interroge précisément les cols qui déterminent l'arbre courant.

#### Algorithme proposé

1. Construire un ensemble initial d'ancres par farthest-point sampling, net approximatif sur le graphe ANN ou simple sous-échantillonnage stratifié.
2. Calculer les atomes DTM durs ou capés-KL aux ancres.
3. Construire un graphe candidat, le repondérer par les temps de collision, puis calculer son MST.
4. Former un batch de requêtes comprenant :
   - les points de contact des arêtes du MST ;
   - les ancres ou points d'un réseau de contrôle ;
   - les zones où le support ANN ou l'arête MST n'est pas certifié.
5. Calculer $D(y)$, l'atome optimal $q^\star(y)$ du fonctionnel cible et le résidu $W_t(y)-D(y)$ pour tout le batch.
6. Ajouter les $B$ atomes de plus grand résidu au-dessus d'une tolérance, après déduplication de $(z,b)$.
7. Recalculer ou mettre à jour le graphe et le MST ; répéter jusqu'au budget ou au certificat.

Les requêtes d'une itération sont indépendantes. Elles se prêtent à un batch ANN, un kernel capped-KL et des réductions entièrement GPU.

#### Heuristique et certificat

Deux arrêts doivent être distingués.

- **Arrêt heuristique :** tous les résidus aux selles courantes sont petits et l'arbre ne change plus pendant deux itérations. C'est un excellent critère pratique, mais il peut manquer une région jamais visitée par le surrogate courant.
- **Arrêt certifié sur $\Omega$ :** un ensemble de contrôle $Y$ est un $\delta$-réseau de $\Omega$ et

  $$
  \max_{y\in Y}(W_t(y)-D(y))\leq\rho.
  $$

  Comme les deux champs sont 1-lipschitziens,

  $$
  0\leq W_t-D\leq\rho+2\delta
  $$

  sur tout $\Omega$. Les merge trees sont alors entrelacés avec la même erreur additive, sous les hypothèses usuelles de finitude.

Un farthest-point sampling effectué seulement sur $X$ fournit un rayon de couverture de $X$, pas un $\delta$-réseau du domaine continu $\Omega$. En grande dimension, certifier un domaine continu peut être impraticable ; la borne doit alors être annoncée comme théorique, ou limitée à un domaine de contrôle discrétisé explicitement. L'API ne doit jamais déduire `certified=True` du seul rayon de couverture des observations.

Cette génération de colonnes fournit la « très bonne heuristique de points critiques » recherchée, tout en indiquant exactement ce qui manque pour en faire un résultat : un réseau de contrôle et un oracle de support certifié.

---

## 5. Le power-MST représente toute la hiérarchie $H_0$

### 5.1 Naissances et collisions

Considérons un ensemble fini de sites

$$
(z_i,b_i), \qquad b_i\geq0,
$$

et le champ

$$
W(y)=\min_i\phi_i(y), \qquad \phi_i(y)=\sqrt{\lVert y-z_i\rVert^2+b_i}.
$$

Le site $i$ naît au niveau

$$
\alpha_i=\sqrt{b_i}.
$$

À l'échelle $r$, son sous-niveau est la boule

$$
B\left(z_i,\sqrt{r^2-b_i}\right), \qquad r^2\geq b_i.
$$

Pour deux sites, posons

$$
d_{ij}=\lVert z_i-z_j\rVert.
$$

Si $d_{ij}>0$, définissons

$$
u_{ij} = \mathrm{clip} \left( \frac{d_{ij}^2+b_j-b_i}{2d_{ij}}, 0, d_{ij} \right).
$$

Leur premier niveau de collision est

$$
\boxed{ \beta_{ij}^2 = \max\left\lbrace  u_{ij}^2+b_i, (d_{ij}-u_{ij})^2+b_j \right\rbrace  }.
$$

Si $d_{ij}=0$, on prend

$$
\beta_{ij}^2=\max(b_i,b_j).
$$

Le point de contact utilisé par la génération de colonnes est

$$
y_{ij} = z_i+\frac{u_{ij}}{d_{ij}}(z_j-z_i)
$$

dans le cas non dégénéré.

### 5.2 Pourquoi un MST suffit exactement

À tout rayon $r$, les composantes de $\{W\leq r\}$ sont celles du graphe complet contenant :

- les sommets $i$ tels que $\alpha_i\leq r$ ;
- les arêtes $(i,j)$ telles que $\beta_{ij}\leq r$.

La propriété minimax des MST implique que le sous-graphe d'un MST formé des arêtes de poids au plus $r$ possède exactement les mêmes composantes que le graphe complet au même seuil. Par conséquent :

> **un seul MST du graphe complet pondéré par $\beta_{ij}$ représente exactement toutes les composantes de tous les sous-niveaux du champ de puissance fini.**

Aucun triangle, aucune cellule de Delaunay et aucun simplexe d'ordre supérieur ne sont nécessaires pour $H_0$.

Le merge tree final contient au plus $M-1$ fusions pour $M$ sites. Les ex æquo doivent être regroupés en événements multiway. Une simplification par persistance peut ensuite contracter les branches sous une tolérance utilisateur, mais cette compression de sortie doit rester distincte de l'approximation du champ.

### 5.3 Le cas fondamental $K=1$

Dans tout ce rapport, la convention aux ancres est que la requête elle-même appartient à sa liste de voisins. Les duplicats et les ex æquo sont conservés avec un ordre déterministe par identifiant ; la valeur dure est unique même si l'atome optimal peut ne pas l'être. Une bibliothèque ANN qui exclut automatiquement la requête doit donc réinsérer l'auto-voisin avant le top-$K$.

Pour la DTM dure avec $K=1$,

$$
q_i=\delta_{x_i}, \qquad z_i=x_i, \qquad b_i=0.
$$

Alors

$$
\beta_{ij}=\frac{1}{2}\lVert x_i-x_j\rVert,
$$

et le power-MST est exactement l'EMST ordinaire, avec le facteur $1/2$ dû à la convention de rayon des boules.

Pour $K=1$ et $\tau>0$, le fonctionnel KL global exact vaut

$$
D_{1,\tau}^2(y) = -\tau\log\left[ \frac{1}{n}\sum_{j=1}^n e^{-\lVert y-x_j\rVert^2/\tau} \right].
$$

C'est la libre énergie d'une KDE gaussienne. Son merge tree continu n'est pas, en général, un MST des observations. La structure sparse proposée est sa **restriction témoignée par fonctions de puissance** :

1. calculer le softmax local ou global à des ancres ;
2. convertir chaque loi en $(z_i,b_i)$ ;
3. calculer le power-MST.

Elle est exacte pour le champ fini témoigné et converge vers l'EMST lorsque $\tau\downarrow0$ et que les ancres sont les observations.

C'est la définition la plus propre du « 1-NN régularisé » compatible avec une hiérarchie sparse :

$$
\boxed{ \text{1-NN régularisé} = \text{power-MST des atomes entropiques témoins} }.
$$

### 5.4 Généralisation à $K>1$

Rien ne change dans la structure finale. Seul le polytope local passe de

$$
\mathcal C_{1,L}=\Delta_L
$$

à

$$
\mathcal C_{K,L} = \{q\in\Delta_L:q_j\leq1/K\}.
$$

L'interaction de cardinal $K$ modifie le barycentre et le poids du site, mais le minimum extérieur reste un minimum de fonctions de puissance. Le même power-MST calcule donc $H_0$ pour tout $K$.

C'est précisément le gain par rapport à `PowerCover3D` :

- le backend actuel régularise d'abord chaque observation, puis prend la **$K$-ième** fonction de puissance ;
- le backend DTM place $K$ dans la mesure locale, puis prend le **minimum** des fonctions de puissance.

Schématiquement,

$$
\underbrace{\mathrm{ord}_K \{\phi_i(y)\}_{i=1}^n}_{\text{multicouverture, combinatoire pour }K>1} \quad\longrightarrow\quad \underbrace{\min_{a\in A} \phi_a^{(K)}(y)}_{\text{DTM témoignée, power-MST}}.
$$

Le prix mathématique de cette simplification est le changement d'estimateur déjà explicité en section 2.4.

### 5.5 Faut-il itérer de $k=1$ à $K$ ?

Pour calculer seulement l'arbre au rang final $K=10$, non. Une seule table de $L\geq10$ voisins suffit, et le solveur capé travaille directement avec le cap $1/10$.

Pour étudier toute la bifiltration en ordre, les problèmes exacts possèdent néanmoins une structure utile :

$$
\mathcal C_{K+1,L}\subset\mathcal C_{K,L},
$$

donc

$$
D_{K+1,\tau,L}(y)\geq D_{K,\tau,L}(y)
$$

pour un même $\tau$ et une même référence. Les dix valeurs du problème dur sont encore plus simples : ce sont les moyennes préfixes des distances triées.

On peut donc calculer $K=1,\ldots,10$ en parallèle ou par un petit axe supplémentaire dans le kernel, en partageant :

- l'index ANN ;
- les listes de voisins ;
- les distances ;
- une partie du graphe candidat.

En revanche, les enveloppes witnessed apprises séparément pour chaque $K$ ne sont pas automatiquement emboîtées hors des ancres. Une vraie bifiltration en $(K,r)$ demande soit un contrôle de monotonie explicite, soit la construction sparse multicover décrite plus loin. L'itération la plus utile pour le backend DTM est donc la **génération de colonnes et le raffinement des arêtes**, pas une obligation séquentielle de passer par tous les rangs.

---

## 6. Comment rendre aussi le graphe sparse

### 6.1 Le verrou restant

Le power-MST complet porte sur $\binom M2$ arêtes. Construire seulement un graphe $L$-NN sur les centres $z_i$, puis le repondérer par $\beta_{ij}$, est très rapide ; ce n'est pas automatiquement exact. Les poids additifs peuvent faire préférer une arête dont les centres ne sont pas parmi les tout premiers voisins euclidiens.

Il faut distinguer trois modes.

| Mode | Graphe | Contrat |
|---|---|---|
| `complete_oracle` | toutes les paires, par tuiles | exact, petits tests uniquement |
| `graph_approx` | CAGRA/RBC $L$-NN symétrisé | exact relativement au graphe, heuristique pour le champ complet |
| `cut_certified` | Borůvka avec listes élargies à la demande | exact si les gardes de distance sont exacts |

### 6.2 Condition abstraite : bottleneck spanner

Le contrat pertinent pour une hiérarchie $H_0$ n'est pas la seule `recall@L`. Un graphe candidat $G$ est un $(1+\varepsilon)$-*bottleneck spanner* si, pour toute paire $(i,j)$, il contient un chemin $P_{ij}$ tel que

$$
\max_{e\in P_{ij}}\beta_e \leq (1+\varepsilon)\beta_{ij}.
$$

Cette propriété donne directement un entrelacement multiplicatif des composantes à tous les seuils. Pour $\varepsilon=0$, le graphe préserve exactement la hiérarchie du graphe complet.

Il est plus pertinent de mesurer ou certifier cette propriété sur des petits cas que de rapporter uniquement le rappel des voisins.

### 6.3 Borůvka avec garde euclidienne

Les poids additifs sont non négatifs, donc

$$
\beta_{ij}\geq\frac{1}{2}\lVert z_i-z_j\rVert.
$$

Supposons que, pour un sommet $i$, **toutes** les arêtes de centre de longueur strictement inférieure à une distance garde $D_{g,i}$ aient été exhaustivement énumérées. Toute arête omise issue de $i$ vérifie alors

$$
\beta_{ij}\geq D_{g,i}/2.
$$

Dans une itération de Borůvka, si la meilleure arête sortante trouvée pour une composante $C$ a un poids $U$ et si, pour chaque sommet $i\in C$, la garde satisfait

$$
D_{g,i}/2\geq U,
$$

cette arête est certifiée minimale pour la coupe. Sinon, on élargit seulement les listes des sommets ou composantes non certifiés : $16\to32\to64\to128$, puis fallback ciblé.

Le reranking direct des distances d'une liste ANN non exhaustive ne fournit pas à lui seul cette garde : il certifie les poids des candidats retournés, pas l'absence d'un candidat omis plus proche. Un contrat exact doit en outre inclure tous les ex æquo à la frontière de garde et utiliser des bornes flottantes conservatrices. En 3D, une BVH/radius-search exhaustive ou un top-$L$ exact peut fermer ce contrat ; la RBC actuelle, même auditée par échantillonnage, et CAGRA restent conditionnels.

Cette procédure déplace la difficulté vers une recherche de voisins de centres classique, sans Delaunay d'ordre $K$. Elle est particulièrement plausible en 3D avec BVH/RBC et vérification exacte des distances. En grande dimension, son exactitude reste conditionnelle à la qualité de la recherche ou peut exiger trop d'élargissements lorsque la dimension intrinsèque est élevée.

---

## 7. Architecture ultra GPU-friendly proposée

### 7.1 Pipeline résident sur GPU

Pour $K=10$, $L=64$ et $M$ ancres :

1. **Voisinage**
   - un index ANN partagé pour toutes les valeurs de $K$ ;
   - identifiants `int32`, carrés de distances `float32` ;
   - distances des candidats recalculées directement depuis les coordonnées stockées ; l'erreur d'arrondi et l'exhaustivité du support restent deux diagnostics distincts.
2. **Atomes**
   - mode dur : moyenne préfixe et barycentre des $K$ voisins ;
   - mode KL : capped softmax, KL et dual scalaire ;
   - mode quadratic : scan sparsemax capé ;
   - aucune matrice $Q\in\mathbb{R}^{M\times L}$ persistante.
3. **Moments**
   - calcul de $z_a$, $v_a$ et éventuellement du terme KL ;
   - accumulation des offsets autour de l'ancre pour limiter la cancellation ;
   - conservation de $z\in\mathbb{R}^{M\times p}$ et $b\in\mathbb{R}^M$ seulement.
4. **Graphe candidat**
   - 3D : RBC/BVH ou recherche exacte tiled ;
   - grande dimension : [CAGRA](https://arxiv.org/abs/2308.15136) ou IVF, avec reranking exact des candidats ;
   - symétrisation CSR sur GPU.
5. **Poids de collision**
   - une arête par thread ou warp ;
   - formule fermée de $\beta_{ij}^2$ ;
   - calcul simultané du point de contact si la génération de colonnes est active.
6. **MST/MSF**
   - Borůvka résident ou [`raft::sparse::solver::mst`](https://docs.rapids.ai/api/raft/stable/cpp_api/solver/) sur CSR ; la primitive vérifiée est un en-tête C++ de la documentation RAFT 26.06, pas une API Python déjà branchable dans le dépôt RAPIDS 26.02 ; prévoir un spike de compatibilité et une extension C++/CUDA, ou conserver un Borůvka CuPy/CUDA dédié ;
   - élargissement des listes pour les composantes non certifiées.
7. **Hiérarchie**
   - tri GPU des $M-1$ arêtes ;
   - Kruskal reconstruction tree ou union-find GPU ;
   - regroupement des ex æquo ;
   - copie CPU finale limitée à l'arbre compact, si l'API Python l'exige.
8. **Affectation à une coupe**
   - top-1 de puissance des observations vers les sites ;
   - une observation reste bruit/non affectée si sa puissance minimale vérifie $\min_i\phi_i(x)>r$ ; sinon, composante du site minimisant dans le MST au rayon choisi ;
   - aucune relation boule–AABB ni grille de cellules.

Toutes les primitives lourdes sont des top-$L$, exponentielles/clips, réductions par ligne, moments pondérés, calculs pairwise sparse et MST. C'est beaucoup plus naturel pour un GPU qu'une grille adaptative ou un complexe dynamique.

### 7.2 Complexité

Avec dimension $p$, $M$ ancres, support d'atome $L_a$, degré de graphe $L_g$ et $J$ itérations scalaires pour une éventuelle bissection,

$$
T = T_{\mathrm{atom\text{-}NN}} +T_{\mathrm{site\text{-}graph}} +O(ML_aJ) +O(ML_ap) +O(ML_gp) +T_{\mathrm{MST}}(ML_g).
$$

Les deux premiers coûts correspondent en général à deux structures différentes : voisinage des ancres vers $X$, puis graphe des nouveaux centres $z$. Un seul index ne se réutilise pas automatiquement. Le solveur dur remplace $O(ML_aJ)$ par une simple réduction ; le scan explicite des coordonnées saturées du KL peut également éviter $J$.

La mémoire persistante est

$$
O(np+ML_a+Mp+ML_g).
$$

Pour $n=M=50\,000$ et $L_a=L_g=64$ :

| Tableau principal | 3D | $p=512$ |
|---|---:|---:|
| observations $X$, float32 | 0,6 Mo | 102,4 Mo |
| support : ids + $d^2$ | 25,6 Mo | 25,6 Mo |
| sites $z$ + poids $b$ | 0,8 Mo | 102,6 Mo |
| graphe : ids + poids | environ 25,8 Mo | environ 25,8 Mo |
| arbre et union-find | moins de 2 Mo | moins de 2 Mo |
| **base persistante** | **environ 55 Mo** | **environ 259 Mo** |

Ce tableau n'est pas une estimation du pic VRAM. Il exclut les index ANN, une éventuelle duplication de $X$ et $z$, la symétrisation pouvant approcher $2|E|$, les tris/uniques, les buffers MST/RAFT et le pool RMM.

La mémoire n'est pas le verrou du cas 50 k. Les risques de latence sont :

- la construction de l'index ou le top-$L$ exact ;
- les synchronisations et copies CPU ;
- les élargissements nécessaires pour certifier le MST ;
- en grande dimension, les $O(MLp)$ opérations de moments et de repondération.

### 7.3 Cible 50 k en 3D

La latence chaude **inférieure à une seconde** doit être traitée comme une cible d'ingénierie à falsifier par un benchmark de bout en bout, pas comme une prévision. Un budget interne de 0,3 à 0,8 seconde laisserait de la marge, mais aucun enchaînement complet n'est encore mesuré. La cible suppose :

- processus persistant et kernels précompilés ;
- $M=n=50\,000$ dans un premier prototype, donc aucune erreur de **sous-échantillonnage des ancres** ; l'enveloppe witnessed reste distincte de la DTM exacte hors des ancres et doit encore être évaluée par résidu ou par une borne de couverture ;
- $L_a=L_g\simeq64$ ;
- pas de transfert CPU avant l'arbre final ;
- MST de graphe sparse ou Borůvka avec peu d'élargissements.

Le résultat de Prokopenko, Sao et Lebrun-Grandié — [EMST 3D de 37 millions de points sous 0,5 s sur A100](https://arxiv.org/abs/2207.00514) — montre que les primitives géométriques et Borůvka sont compatibles avec cette échelle. Ce n'est pas un benchmark direct du power-MST pondéré : les poids additifs et la certification de coupe demandent du travail supplémentaire.

Le [rapport G4 actuel](https://github.com/Ludwig-H/E-HGP/blob/70423fa354e4f0252f8a8eddcf5e311bf1a4ac42/perg_hgp/benchmarks/results/G4_20260712_REPORT.md) mesure environ 1,13 s en mode dur et 1,38 s en mode local sur 100 k après compilation, mais sur une grille $48^3$. Il montre aussi que le MSF cubique ne prend qu'une fraction du temps et que la régularisation/ANN domine. La proposition DTM supprime le champ cubique et l'affectation boule–cellule, mais elle ajoute un graphe sur les sites : seul un benchmark de bout en bout permettra de conclure.

### 7.4 Haute dimension

En haute dimension, trois adaptations deviennent centrales.

1. **Réduire $M$ plutôt que $L$ seul.** Les moments coûtent $O(MLp)$ ; un coreset par net ou génération de colonnes est plus important qu'un gain marginal sur le solveur entropique.
2. **Séparer proposition et vérification.** CAGRA, projection de Johnson–Lindenstrauss ou quantification produit peuvent proposer les voisins ; les distances et poids $\beta$ des candidats sont toujours recalculés dans l'espace original.
3. **Rapporter la dimension intrinsèque.** Un bon comportement d'ANN et de net-tree dépend de la dimension de doublement, pas seulement du nombre de coordonnées.

Un graphe ANN de degré fixe ne garantit pas le PMST exact sur toutes les données de grande dimension. Le backend doit donc annoncer l'un des statuts :

- `exact_complete` ;
- `cut_certified` ;
- `bottleneck_empirical_sampled` avec facteur mesuré sur un échantillon/oracle réduit, sans extrapolation de garantie au grand jeu ;
- `graph_heuristic`.

---

## 8. Autres hiérarchies sparse à conserver comme branches ou baselines

### 8.1 DTM-filtration $p=1$ : le MST de cônes

Les [DTM-based filtrations](https://arxiv.org/abs/1811.04757) considèrent une famille de boules pondérées. La variante $p=1$ associée à des ancres $a_i$ et aux poids

$$
w_i=D_K(a_i)
$$

définit

$$
C_A(y)=\min_i\{w_i+\lVert y-a_i\rVert\}.
$$

Comme la DTM est 1-lipschitzienne, si toutes les positions du domaine pouvaient servir d'ancre, on aurait exactement $C=D_K$. Pour un $\delta$-réseau fini,

$$
D_K\leq C_A\leq D_K+2\delta.
$$

Les boules ont ici le rayon $r-w_i$, et le temps de collision est

$$
\beta_{ij}^{(1)} = \max\left\lbrace  w_i, w_j, \frac{\lVert a_i-a_j\rVert+w_i+w_j}{2} \right\rbrace .
$$

Cette construction est probablement le baseline continu le plus simple et le plus rapide : centres inchangés, un scalaire DTM par point, puis MST pondéré. Elle abandonne cependant l'identité quadratique du transport $W_2$ et le modèle de sites de puissance $p=2$ déjà présent dans le dépôt.

### 8.2 Approximation nodale $p=2$

Une autre approximation très simple est

$$
P_K(y) = \min_i \sqrt{\lVert y-x_i\rVert^2+D_K^2(x_i)}.
$$

Elle conserve les observations comme centres et utilise une DTM scalaire comme poids. Buchet et al. établissent, pour les nuages euclidiens finis, une comparaison multiplicative

$$
\frac{1}{\sqrt{2}}D_K \leq P_K \leq\sqrt{3}\,D_K.
$$

Cette variante est moins adaptative que la barycentrique witnessed DTM, mais constitue un excellent premier oracle GPU : top-$K$ aux observations, puis PMST.

### 8.3 Robust Single Linkage / HDBSCAN avec core DTM

Avec un core radius $c_i$, la mutual reachability est

$$
d_{\mathrm{mr}}(i,j) = \max\{c_i,c_j,\lVert x_i-x_j\rVert\}.
$$

Son MST donne une hiérarchie de type HDBSCAN. On peut choisir

$$
c_i=D_K(x_i)
$$

ou sa version entropique. Le résultat est extrêmement GPU-friendly. La robust single linkage standard possède des résultats de consistance pour les cluster trees ; voir [Chaudhuri, Dasgupta, Kpotufe et von Luxburg](https://arxiv.org/abs/1406.1546). Ces résultats ne couvrent pas automatiquement le remplacement de la core distance par une DTM/eDTM : cette extension demanderait une preuve séparée.

Il s'agit toutefois d'une hiérarchie sur le graphe des observations, pas des composantes d'un champ continu dans $\mathbb{R}^p$. Elle doit rester une baseline forte, non être renommée HGP ou DTM witnessed.

### 8.4 Sparse Rips et net-trees

Les [approximations linéaires de Rips de Sheehy](https://arxiv.org/abs/1203.6786) et les filtrations Rips pondérées DTM donnent des tailles linéaires sous hypothèse de dimension de doublement bornée, avec des constantes dépendant de cette dimension et de $\varepsilon$.

Pour $H_0$ seul, un PMST est plus direct qu'un complexe de Rips. Ces résultats restent utiles pour :

- construire un graphe bottleneck-spanner ;
- traiter aussi l'homologie supérieure ;
- obtenir une théorie lorsque la dimension intrinsèque est faible.

### 8.5 KDE, diffusion, Mapper et hypergraphes

Ces pistes ne ferment pas le problème principal.

- **KDE / log-sum-exp :** l'évaluation et les [coresets KDE](https://arxiv.org/abs/1710.04325) peuvent être très rapides, mais l'extraction globale des modes et selles d'un champ lisse n'est pas fournie par un MST exact.
- **[Diffusion condensation](https://arxiv.org/abs/1907.04463) :** les SpMM sont GPU-friendly, mais le paramètre est le temps de diffusion, pas le niveau d'un estimateur $K$-NN/DTM.
- **[Mapper](https://arxiv.org/abs/1511.05823) :** utile pour visualiser un filtre déjà choisi ; il ajoute une couverture et un clustering alors qu'un merge tree scalaire est plus direct.
- **[Sparsification spectrale d'hypergraphes](https://arxiv.org/abs/1807.04974) :** préserve des énergies ou des coupes, pas nécessairement les temps de fusion de tous les sous-niveaux ; elle suppose en outre que les hyperarêtes aient déjà été produites.
- **[UMAP](https://arxiv.org/abs/1802.03426) :** son écosystème ANN/GPU et sa calibration locale sont utiles ; sa topologie fuzzy et son embedding ne sont pas la topologie de densité finale recherchée.

### 8.6 Tableau de décision

La colonne « sortie » décrit la représentation finale. Elle ne prétend pas que sa construction est linéaire : avec un graphe candidat de degré $L_g$, le travail stocké est $O(M+ML_g)$, tandis que le graphe complet possède $O(M^2)$ arêtes.

| Méthode | Objet calculé | Fidélité HGP | Sortie après MST/complexe | GPU | Verdict |
|---|---|---:|---:|---:|---|
| Witnessed DTM dure + PMST | DTM approchée | estimateur différent | $O(M)$ | excellente | **premier prototype** |
| Witnessed DTM-KL + PMST | DTM entropique approchée | estimateur différent | $O(M)$ | excellente | **backend principal candidat** |
| $M$-PDTM + PMST | coreset DTM appris | estimateur différent | $O(M)$, $M\ll n$ possible | bonne | compression optionnelle |
| DTM $p=1$ + MST | enveloppe de cônes | estimateur différent | $O(M)$ | excellente | baseline continue la plus simple |
| DTM-RSL/HDBSCAN | arbre du graphe des points | faible à moyenne | $O(n)$ | excellente | baseline production |
| Sparse multicover Alonso | multicouverture HGP | très forte | $O(n)$ pour $d,\varepsilon$ fixés | à développer | voie théorique 3D |
| Sparse Rips/net-tree | Rips/DTM pondérée | dépend de l'objet | linéaire en faible doubling dimension | moyenne | utile au-delà de $H_0$ |
| KDE/free energy globale | champ lisse | faible à moyenne | évaluation sparse possible | bonne | pas de structure critique complète |
| UMAP/diffusion | autre graphe/temps | faible | sparse | excellente | briques, pas solution |

---

## 9. La voie fidèle à HGP en faible dimension

### 9.1 Sparse multicover d'Alonso

Le résultat théorique le plus directement pertinent est le préprint d'Ángel Javier Alonso, [*A Sparse Multicover Bifiltration of Linear Size*](https://arxiv.org/abs/2411.06986), révisé en juin 2025.

Pour la multicouverture

$$
\mathrm{Cov}(r,K) = \{y:r_K(y)\leq r\},
$$

il construit une approximation multiplicative $(1+\varepsilon)$ de toute la bifiltration $(r,K)$, la dilation portant sur le paramètre métrique $r$ et non sur l'ordre $K$, avec :

- taille $O(n)$ pour $d$ et $\varepsilon$ fixés ;
- taille indépendante de l'ordre maximal $K$ ;
- temps espéré annoncé $O(n\log\Delta)$ dans le modèle du papier, pour $d$ et $\varepsilon$ fixés, où $\Delta$ est le spread ;
- entrelacement homotopique multiplicatif avec la multicover.

La construction utilise :

- des persistent nets ;
- des boules sparse qui croissent, ralentissent puis disparaissent ;
- une application de couverture vers des boules survivantes ;
- des poids de multiplicité transférés aux survivantes.

Elle améliore conceptuellement les dépendances exponentielles explicites en l'ordre maximal des Sparse Higher Order Čech filtrations antérieures.

### 9.2 Ce que ce résultat permet — et ne permet pas — d'annoncer

Pour la 3D et $K=10$, cette méthode doit devenir :

- un backend HGP $(1+\varepsilon)$-approché ;
- une référence à erreur contrôlée pour valider la DTM sur des tailles accessibles ;
- une piste pour extraire seulement $H_0$ de l'équivalent sparse, sans matérialiser les dimensions homologiques inutiles.

Elle n'est pas encore une réponse prête pour la cible sub-seconde :

- les constantes dépendent sévèrement du packing, de $d$ et de $\varepsilon$ et peuvent être prohibitives, même lorsque la taille reste asymptotiquement linéaire en $n$ ;
- les persistent nets et les prédicats d'intersection sont moins SIMD que les moments DTM ;
- l'intersection des boules sparse mobilise des problèmes de type LP ;
- aucune implémentation GPU massive ni benchmark 50 k/3D n'est fourni par le papier.

Une version $H_0$-only peut probablement éviter une partie du complexe, mais cela demande une preuve spécifique. Il ne faut pas supposer qu'une simple paire de boules suffit à représenter un niveau de multicouverture pondérée.

### 9.3 Une limite pour la topologie complète — pas une obstruction pour $H_0$

Un préprint très récent de Kenneth McCabe, [*Lower Bounds for Approximating the Vietoris–Rips Filtration*](https://arxiv.org/abs/2607.06524), daté du 7 juillet 2026, montre sur des familles de métriques finies générales :

- une taille exponentielle peut être nécessaire pour un facteur fixe $c<\sqrt{2}$ ;
- une taille superlinéaire peut être nécessaire pour tout facteur constant ;
- les résultats s'étendent à l'intrinsic Čech et à des bifiltrations contenant une tranche Rips, notamment function-, degree- et subdivision-Rips.

Ces bornes concernent la taille d'approximations finiment présentées de la filtration complète et utilisent de l'homologie supérieure. Elles **ne donnent pas une obstruction à une représentation sparse de $H_0$** : le $H_0$ d'une filtration de Rips à un paramètre est lui-même représenté exactement par un MST de taille $O(n)$. Elles ne portent pas non plus directement sur les nuages euclidiens de dimension fixée, la DTM witnessed ou le $H_0$ de la multicouverture.

Ce papier ne peut donc pas servir de preuve que le passage HGP $\to$ DTM est nécessaire. Il délimite plutôt une extension future du projet : si l'objectif devient la préservation de toute l'homotopie, de l'homologie supérieure ou d'un modèle bifiltré complet, une approximation linéaire indépendante de toute hypothèse géométrique est impossible en général.

Pour le problème présent, limité à $H_0$, l'obstacle est différent : la sortie MST est déjà linéaire, mais il faut découvrir les bonnes arêtes et, dans le cas HGP, représenter fidèlement le champ continu de multicouverture. Aucune borne citée ici n'exclut une meilleure solution $H_0$-only en haute dimension.

La stratégie à deux voies reste recommandée, pour des raisons d'objet et d'algorithme plutôt qu'en vertu d'une borne d'impossibilité $H_0$ :

1. **faible dimension euclidienne :** sparse multicover HGP $(1+\varepsilon)$-approchée ; pour une métrique de faible dimension de doublement, l'extension d'Alonso porte sur l'analogue subdivision-Rips ;
2. **dimension quelconque :** changer explicitement d'estimateur vers une DTM coreset, avec bornes additives et certification conditionnelle de l'ANN/MST.

---

## 10. Comparaison avec l'implémentation actuelle de `perg_hgp`

### 10.1 État du dépôt au 13 juillet 2026

Le HEAD vérifié est [`70423fa`](https://github.com/Ludwig-H/E-HGP/commit/70423fa354e4f0252f8a8eddcf5e311bf1a4ac42). Par rapport à la révision auditée dans le rapport précédent, il ajoute uniquement ce rapport au dépôt ; le code n'a pas changé.

Il n'existe donc encore :

- ni backend DTM ;
- ni witnessed $K$-distance ;
- ni PMST générique sans grille ;
- ni test comparant DTM dure, DTM-KL et HGP.

### 10.2 Différence mathématique centrale

[`PowerCover3D`](https://github.com/Ludwig-H/E-HGP/tree/70423fa354e4f0252f8a8eddcf5e311bf1a4ac42/perg_hgp/perg_hgp/backends/power_cover_3d_cuda) construit des atomes locaux $(z_i,a_i)$, puis évalue sur une grille la $K$-ième valeur de puissance :

$$
R_K^{\mathrm{PC}}(y) = \mathrm{ord}_K \left\lbrace  \sqrt{\lVert y-z_i\rVert^2+a_i} \right\rbrace _{i=1}^n.
$$

Cette construction reste une multicouverture de boules de puissance. Elle préserve donc mieux l'objet HGP régularisé, mais conserve la combinatoire d'ordre $K$ et la résout actuellement par une approximation cubique 3D.

Le backend proposé calcule à chaque ancre une mesure de masse $K/n$, puis prend un minimum :

$$
W_K(y) = \min_{a\in A} \sqrt{\lVert y-z_a^{(K)}\rVert^2+b_a^{(K)}}.
$$

Il change l'estimateur en DTM, mais toute sa hiérarchie $H_0$ devient un PMST, sans grille et dans toute dimension.

### 10.3 Ce qui manque à la régularisation locale actuelle

[`local_gibbs.py`](https://github.com/Ludwig-H/E-HGP/blob/70423fa354e4f0252f8a8eddcf5e311bf1a4ac42/perg_hgp/perg_hgp/local_gibbs.py) et le noyau fusionné de [`spatial_core.py`](https://github.com/Ludwig-H/E-HGP/blob/70423fa354e4f0252f8a8eddcf5e311bf1a4ac42/perg_hgp/perg_hgp/backends/power_cover_3d_cuda/spatial_core.py) travaillent sur un simplexe non capé. Le mode dur `kappa=1` produit analytiquement

$$
q_i=\delta_{x_i},
$$

et non

$$
q_i=\frac{1}{K}\sum_{j\in N_K(i)}\delta_{x_j}.
$$

Pour obtenir une DTM, il faut donc :

1. ajouter la contrainte $q_j\leq1/K$ ;
2. déplacer $K$ à l'intérieur de l'atome ;
3. prendre le minimum extérieur, non la $K$-ième puissance ;
4. inclure $\tau\mathrm{KL}$ dans $b_i$ si le champ revendique le fonctionnel KL exact.

Ajouter seulement le cap au backend actuel tout en conservant l'opérateur d'ordre extérieur produirait encore un autre objet : une $K$-multicouverture d'atomes eux-mêmes de masse $K/n$.

### 10.4 Briques réutilisables

| Brique actuelle | Décision pour `dtm_power_mst` |
|---|---|
| normalisation, quantification et diagnostics | réutiliser |
| gestion RMM/CUDA, manifestes et reprises atomiques | réutiliser |
| recalcul des distances associées aux ids ANN | réutiliser |
| accumulation des offsets et variance stable | réutiliser comme squelette |
| solveur Gibbs sans cap | remplacer par capped-KL / hard prefix / sparsemax |
| `rank_field.py` | garder comme générateur expérimental, après normalisation/log-domain |
| formule de collision du mode historique `soft_only` | extraire, généraliser à $p$, tester |
| `SpatialGrid3D` et champs cubiques | ne pas utiliser dans le nouveau backend |
| [`cubical_msf.py`](https://github.com/Ludwig-H/E-HGP/blob/70423fa354e4f0252f8a8eddcf5e311bf1a4ac42/perg_hgp/perg_hgp/backends/power_cover_3d_cuda/cubical_msf.py) | réutiliser les idées Borůvka, pas les arêtes de grille |
| `dual_graph.py` / unions NumPy | remplacer par CSR + MST GPU |
| `coverage.py` boule–AABB | remplacer par top-1 de puissance |
| harnais G4 et rapports de certification | étendre aux erreurs DTM/site/graphe |

Le mode historique `soft_only` possède déjà une formule de collision pondérée, mais ne garde qu'un plus proche voisin de puissance par site. Ce graphe peut être déconnecté et n'est pas un PMST complet. Il fournit un test unitaire utile, pas le backend final.

### 10.5 Ce que la proposition améliore en 3D

Par rapport à l'implémentation la plus récente :

- aucune erreur de grille ;
- aucun budget $N_xN_yN_z$ ;
- aucune relation cellules–boules pour les labels ;
- complexité indépendante d'une résolution spatiale globale ;
- même code pour 3D et haute dimension ;
- nombre d'événements de hiérarchie linéaire dans le nombre de sites.

Ce qu'elle abandonne ou rend conditionnel :

- l'objet exact de multicouverture HGP ;
- le certificat inner/outer cubique actuel ;
- l'exactitude du PMST complet si le graphe candidat n'est pas certifié.

La comparaison expérimentale doit donc porter à la fois sur la vitesse et sur la distance entre arbres, pas seulement sur l'ARI d'une meilleure coupe.

---

## 11. Contrat logiciel recommandé

### 11.1 API conceptuelle

Un backend unique peut exposer :

```python
DTMPowerMST(
    K=10,
    atom_regularizer="hard",          # hard | kl | quadratic
    atom_energy="free_energy",       # free_energy | transport_only
    atom_support=64,
    entropy_radius_bias=0.0,
    temperature_mode="global",       # global | local_perplexity
    anchor_mode="all",               # all | net | pdtm | column_generation
    max_sites=None,
    graph_mode="cut_certified",       # complete | graph_approx | cut_certified
    graph_degree=64,
    target_graph_factor=1.0,
)
```

Le paramètre public prioritaire est `entropy_radius_bias`, qui détermine $\tau$ par la borne de section 3.2. Un utilisateur expert peut éventuellement fournir directement $\tau$.

### 11.2 Sorties et registre d'erreur

Le résultat doit conserver :

- centres $z_i$ et poids $b_i$ ;
- ids/supports des ancres seulement si demandés pour l'audit ;
- naissances $\alpha_i$ ;
- arêtes MST et niveaux $\beta_e$ ;
- Kruskal reconstruction tree ;
- diagnostics de troncature des supports ;
- diagnostics de coreset ;
- statut du graphe/MST ;
- convention exacte de rayon, carré de rayon et densité.

Un registre d'erreur explicite devrait séparer :

| Terme | Exemple de champ |
|---|---|
| changement HGP $\to$ DTM | borne $\sqrt{K}$ + métriques empiriques d'arbre |
| entropie | `entropy_radius_bias_bound` |
| support local | `tail_lower_upper_gap` |
| coreset | `anchor_cover_radius`, `max_anchor_residual` |
| ANN | exact / recall auditée / conditionnelle |
| graphe | complete / cut-certified / bottleneck empirique |
| flottant | enveloppe float32/float64 auditée |

La sortie ne doit annoncer `certified=True` que si chaque couche nécessaire au contrat choisi est certifiée. Un champ exact relativement à ses sites n'est pas automatiquement une approximation certifiée de la DTM, et un MST exact du graphe candidat n'est pas automatiquement le PMST complet.

### 11.3 Noms d'objets

Noms recommandés :

- `hard_witnessed_dtm` ;
- `kl_witnessed_dtm_free_energy` ;
- `entropy_selected_witnessed_dtm` ;
- `dtm_power_mst` ;
- `dtm_rsl` ;
- `sparse_multicover_h0`.

Noms à éviter :

- `sinkhorn_hgp`, puisqu'il n'y a pas deux marginales normalisées ;
- `exact_hgp`, pour la DTM ;
- `exact_mst`, lorsque seul le graphe ANN est traité exactement ;
- `entropic_dtm`, si le terme KL est omis du poids de puissance sans précision.

---

## 12. Programme expérimental falsifiable

### 12.1 Oracles de petite taille

Avant toute optimisation GPU :

1. distances complètes float64 ;
2. DTM dure exacte par tri ;
3. capped-KL global exact ;
4. sites témoins à toutes les observations ;
5. graphe complet de collisions ;
6. PMST complet ;
7. évaluation dense du champ uniquement pour visualisation/validation en 2D et 3D.

Ces oracles doivent vérifier :

- limite $\tau\to0$ ;
- cap $q_j\leq1/K$ et somme unité ;
- borne $\tau\log(L/K)$ ;
- identité barycentrique ;
- formule de collision ;
- égalité des composantes graphe complet/MST à tous les seuils ;
- borne $2\delta+\rho$ sur des domaines contrôlés.

### 12.2 Ablations minimales

Sur le même voisinage et le même graphe candidat :

1. DTM witnessed dure ;
2. KL `free_energy` ;
3. KL `transport_only` ;
4. quadratic/sparsemax ;
5. Gibbs sur $K$-sous-ensembles ;
6. approximation nodale $p=2$ ;
7. DTM-RSL/HDBSCAN ;
8. `PowerCover3D` dur et local ;
9. HGP-CGAL ou oracle multicover sur les tailles accessibles ;
10. sparse multicover Alonso lorsqu'une implémentation existe.

### 12.3 Mesures scientifiques

Mesurer séparément :

- erreur uniforme du champ sur un jeu de requêtes ;
- distance/interleaving ou bottleneck entre merge trees ;
- erreur des niveaux de fusion appariés ;
- préservation des petites branches persistantes ;
- ARI/NMI de coupes choisies sans utiliser la vérité terrain ;
- stabilité au bruit, aux outliers et au sous-échantillonnage ;
- taux de violation de monotonie entre $K$ pour les envelopes witnessed ;
- distribution des gaps $r_{K+1}^2-r_K^2$ ;
- résidus de génération de colonnes aux selles ;
- facteur bottleneck du graphe candidat sur les petits oracles.

### 12.4 Mesures système

Chronométrer séparément :

- construction/réutilisation de l'index ;
- requête ANN ;
- solveur d'atomes et moments ;
- construction/symétrisation du graphe ;
- poids de collision ;
- MST ;
- arbre et labels ;
- raffinement/certification.

Rapporter :

- temps froid et chaud ;
- pic VRAM ;
- copies et synchronisations CPU ;
- nombre d'ancres $M$ ;
- nombre d'arêtes ;
- nombre d'élargissements Borůvka ;
- statut exact/conditionnel/heuristique.

Le test d'acceptation 3D doit inclure le pipeline complet pour 50 k points et non uniquement le MST. La barre d'une seconde ne doit être annoncée qu'après plusieurs runs chauds, avec compilation exclue mais construction de la hiérarchie incluse.

### 12.5 Critères de décision

La version entropique ne doit remplacer la version dure que si elle améliore au moins l'un des axes suivants sans dégrader excessivement les autres :

- stabilité de l'arbre au bootstrap/bruit ;
- rappel des branches significatives ;
- qualité non supervisée des coupes ;
- robustesse aux ex æquo et aux petits gaps ;
- facilité de compression en nombre de sites.

Si elle n'apporte pas ce gain, la conclusion correcte sera que **la DTM dure suffit comme régularisation et que l'entropie supplémentaire n'est pas utile**.

---

## 13. Ordre d'implémentation recommandé

### Phase A — objet mathématique minimal

1. Oracle CPU `hard_witnessed_dtm`.
2. Graphe complet et PMST.
3. Tests $K=1$ contre EMST.
4. Tests $K=2,5,10$ contre DTM dense sur petits nuages.

### Phase B — régularisation

1. Capped-KL stable en log-domain.
2. Deux conventions `free_energy` et `transport_only`.
3. Borne automatique de biais.
4. Support global tronqué avec garde et encadrement.
5. Quadratic/sparsemax en ablation.

### Phase C — GPU

1. Kernel atome + moments générique en dimension $p$.
2. CAGRA/RBC, reranking exact.
3. Graphe CSR et poids $\beta^2$.
4. RAFT MST ou Borůvka CuPy/CUDA résident.
5. Kruskal reconstruction tree et labels top-1.

### Phase D — sparsification certifiable

1. `graph_approx` à degré fixe.
2. Borůvka à garde de coupe.
3. Ancres par net.
4. Génération de colonnes guidée par les selles.
5. Réseau de contrôle et borne $2\delta+\rho$.

### Phase E — voie HGP fidèle

1. Prototype sparse multicover Alonso en 3D.
2. Extraction $H_0$-only prouvée.
3. Comparaison à HGP-CGAL et à la DTM.
4. Étude d'une construction parallèle de persistent nets.

---

## 14. Verdict final

La réponse la plus satisfaisante mathématiquement et pratiquement est :

$$
\boxed{ \begin{aligned} &\text{\textbf{régularisation :}} &&\text{DTM sur simplexe capé, puis KL optionnel à biais contrôlé},\\ &\text{\textbf{sparsification :}} &&\text{témoins/coreset et génération de colonnes aux selles},\\ &\text{\textbf{topologie :}} &&\text{minimum extérieur dur et power-MST},\\ &\text{\textbf{GPU :}} &&\text{ANN + réductions par ligne + CSR + Borůvka},\\ &\text{\textbf{certification :}} &&\text{queue, réseau d'ancres, résidu et gardes de coupe séparés}. \end{aligned} }
$$

La formule fondamentale est

$$
\min_{q\geq0,\ \mathbf{1}^\top q=1,\ q\leq1/K} \left[ \langle q,c(y)\rangle + \tau\mathrm{KL}(q\Vert u) \right].
$$

Elle représente exactement la contrainte de cardinalité $K$ au niveau des marginales, sans hypergraphe ni Delaunay d'ordre supérieur, et se réduit sur GPU à un capped softmax par ligne. Elle ne conserve pas les corrélations d'inclusion de la Gibbs higher-order. Après barycentration, chaque ligne devient un site de puissance. Le MST représente exactement $H_0$ du surrogate fini si le graphe est complet, `cut_certified` ou un 0-bottleneck spanner ; sinon, il représente exactement seulement la hiérarchie du graphe candidat.

Deux réserves doivent rester écrites dans l'API et les articles :

1. ce backend approxime une DTM, pas la multicouverture HGP exacte ;
2. en grande dimension intrinsèque, l'exactitude du PMST complet ne vient pas gratuitement d'un graphe ANN de degré fixe.

Pour le cas 3D à 50 k points, cette architecture est beaucoup plus crédible sous la seconde que la grille actuelle, parce que son travail est proportionnel aux points, aux supports et aux arêtes utiles, non au nombre de cellules d'une discrétisation globale. Pour une voie fidèle à la thèse, la sparse multicover d'Alonso est désormais la référence théorique à explorer en parallèle.

---

## Annexe A — preuves courtes des trois bornes utilisées

### A.1 Biais KL

Pour tout $q\in\mathcal C_{K,L}$,

$$
\langle q,c\rangle\geq D_{K,L}^2.
$$

Comme $\mathrm{KL}\geq0$,

$$
D_{K,\tau,L}^2\geq D_{K,L}^2.
$$

La mesure $q^0$ uniforme sur les $K$ plus petits coûts est admissible et vérifie

$$
\mathrm{KL}(q^0\Vert u_L) = K\frac{1}{K}\log\frac{1/K}{1/L} = \log(L/K).
$$

En évaluant l'objectif en $q^0$,

$$
D_{K,\tau,L}^2 \leq D_{K,L}^2+\tau\log(L/K).
$$

Si le support contient les vrais $K$ plus petits coûts, $D_{K,L}=D_K$ global. Si $L=K$, le polytope est réduit à une seule mesure uniforme et le biais est identiquement nul, malgré une température quelconque.

Enfin, pour $a,b\geq0$,

$$
\sqrt{a+b}-\sqrt{a}\leq\sqrt{b},
$$

ce qui donne la borne en rayon.

### A.2 Coreset $2\delta+\rho$

Supposons $W\geq D$ et, sur un $\delta$-réseau $A$,

$$
W(a)-D(a)\leq\rho.
$$

Pour tout $y$, choisissons $a\in A$ tel que $\lVert y-a\rVert\leq\delta$. Les deux champs étant 1-lipschitziens,

$$
W(y) \leq W(a)+\delta \leq D(a)+\rho+\delta \leq D(y)+\rho+2\delta.
$$

### A.3 Préservation des seuils par un MST

Soit $G$ le graphe complet pondéré par $\beta$. Pour deux sommets $u,v$, le poids maximal sur leur chemin dans un MST $T$ est égal au plus petit bottleneck possible parmi tous les chemins de $G$. Ainsi, $u$ et $v$ sont reliés dans $G_{\leq r}$ si et seulement s'ils le sont dans $T_{\leq r}$. Cette équivalence vaut pour tout $r$ simultanément ; elle préserve donc toute la hiérarchie $H_0$.

---

## Annexe B — pseudocode de la génération de colonnes

```text
input: X, K, regularizer, initial anchors A0, tolerance eta
Q <- { optimal_local_measure(a, K) : a in A0 }

repeat
    S <- power_sites(Q)                 # (z, b)
    G <- candidate_graph(S)
    T, graph_status <- power_mst(G)

    Y_saddle <- contact_points(T)
    Y_control <- control_net_or_uncertain_regions()
    Y <- deduplicate(Y_saddle union Y_control)

    for y in Y, in parallel:
        q_star[y], D[y], support_status[y] <- dtm_oracle(y, K)
        residual[y] <- witnessed_field(S, y) - D[y]

    B <- top residual queries with residual > eta
    Q <- Q union {q_star[y] : y in B}

until B is empty and requested graph/support certificates pass

return power_sites(Q), T, error_ledger
```

---

## Références principales

1. F. Chazal, D. Cohen-Steiner, Q. Mérigot, [*Geometric Inference for Probability Measures*](https://doi.org/10.1007/s10208-011-9098-0), 2011.
2. L. J. Guibas, Q. Mérigot, D. Morozov, [*Witnessed $k$-Distance*](https://arxiv.org/abs/1102.4972), 2011/2013.
3. M. Buchet, F. Chazal, S. Y. Oudot, D. R. Sheehy, [*Efficient and Robust Persistent Homology for Measures*](https://arxiv.org/abs/1306.0039), 2013/2016.
4. C. Brécheteau, C. Levrard, [*The $k$-PDTM: a coreset for robust geometric inference*](https://arxiv.org/abs/1801.10346), 2018/2020.
5. H. Anai et al., [*DTM-based Filtrations*](https://arxiv.org/abs/1811.04757), 2018/2020.
6. Á. J. Alonso, [*A Sparse Multicover Bifiltration of Linear Size*](https://arxiv.org/abs/2411.06986), 2024, version révisée 2025.
7. K. McCabe, [*Lower Bounds for Approximating the Vietoris–Rips Filtration*](https://arxiv.org/abs/2607.06524), 7 juillet 2026.
8. D. R. Sheehy, [*Linear-Size Approximations to the Vietoris–Rips Filtration*](https://arxiv.org/abs/1203.6786), 2012/2013.
9. K. Chaudhuri, S. Dasgupta, S. Kpotufe, U. von Luxburg, [*Consistent Procedures for Cluster Tree Estimation and Pruning*](https://arxiv.org/abs/1406.1546), 2014.
10. H. Ootomo et al., [*CAGRA: Highly Parallel Graph Construction and Approximate Nearest Neighbor Search for GPUs*](https://arxiv.org/abs/2308.15136), 2023.
11. A. Prokopenko, P. Sao, D. Lebrun-Grandié, [*A single-tree algorithm to compute the Euclidean minimum spanning tree on GPUs*](https://arxiv.org/abs/2207.00514), 2022.
12. NVIDIA RAPIDS RAFT, [documentation de `raft::sparse::solver::mst`](https://docs.rapids.ai/api/raft/stable/cpp_api/solver/), version stable 26.06 consultée le 13 juillet 2026.
13. L. McInnes, J. Healy, J. Melville, [*UMAP: Uniform Manifold Approximation and Projection for Dimension Reduction*](https://arxiv.org/abs/1802.03426), 2018.
14. N. Brugnone et al., [*Coarse Graining of Data via Inhomogeneous Diffusion Condensation*](https://arxiv.org/abs/1907.04463), 2019.
15. M. Carrière, S. Oudot, [*Structure and Stability of the 1-Dimensional Mapper*](https://arxiv.org/abs/1511.05823), 2015.
16. T. Soma, Y. Yoshida, [*Spectral Sparsification of Hypergraphs*](https://arxiv.org/abs/1807.04974), 2018.
17. J. M. Phillips, W. M. Tai, [*Near-Optimal Coresets of Kernel Density Estimates*](https://arxiv.org/abs/1710.04325), 2017.
