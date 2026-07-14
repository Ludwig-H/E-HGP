# PERG-HGP sans grille : hiérarchie entropique de forte densité en grande dimension

- **Date :** 12 juillet 2026
- **Dépôt audité :** [`Ludwig-H/E-HGP`](https://github.com/Ludwig-H/E-HGP)
- **Révision auditée :** [`d67647f5bc6ad7ce76e259541a5b77d116359555`](https://github.com/Ludwig-H/E-HGP/tree/d67647f5bc6ad7ce76e259541a5b77d116359555)
- **Manuscrit :** `Manuscrit_de_thèse(7).pdf`, version du 14 juin 2026
- **Cible principale :** ordre final $K=10$, dimension ambiante potentiellement grande
- **Cible secondaire :** environ $50\,000$ points dans $\mathbb R^3$, réponse en moins d'une seconde sur une machine puissante

---

## Résumé exécutif

La conclusion principale est double.

1. **Oui, l'entropie permet de remplacer une sélection combinatoire de $K$ voisins par un calcul régulier et GPU-friendly.** La formulation canonique est une loi de Gibbs sur les sous-ensembles de cardinal $k$. Sa fonction de partition est un polynôme symétrique élémentaire ; tous les ordres $1,\ldots,K$ se calculent sans énumérer les $\binom nk$ sous-ensembles.
2. **Non, cela ne transforme pas automatiquement la hiérarchie HGP exacte en un “Sinkhorn de Delaunay”.** L'entropie lisse le rang ou les atomes, mais la recherche exhaustive des cols, des lentilles et des cofaces critiques reste un problème géométrique. En dimension intrinsèque élevée, aucune construction exacte et de taille raisonnable n'est connue pour $K=10$.

Trois objets mathématiques doivent être distingués.

| Objet | Fidélité à HGP-Clusterer | Structure de la hiérarchie | Grande dimension |
|---|---:|---|---:|
| Libre énergie du rang $K$ | Régularisation littérale du rang, mais niveaux modifiés | Merge tree d'un champ lisse ; pas de MST exact | Évaluation possible ; extraction globale des cols difficile |
| Atomes entropiques + $K$-ième distance de puissance | Préserve exactement le modèle HGP régularisé | Čech de puissance, faces de taille $K$, cofaces de taille $K+1$ | Exactitude globale encore combinatoire |
| Distance-$K$ témoignée entropique + minimum de puissance | Change l'estimateur en une approximation de la DTM | **MST de puissance pour tout $K$** | **Voie pratique recommandée** |

### Recommandation

Le dépôt devrait proposer deux backends explicitement différents.

- **`power_multicover`** : cible scientifique fidèle à la thèse. On conserve le champ $K$-ième distance de puissance, on traite $K=1$ par un vrai MST de puissance, puis on complète progressivement un atlas de rang jusqu'à $K=10$. La libre énergie, le rank-shell mean-shift et UMAP servent uniquement à proposer des candidats ; les décisions topologiques finales restent dures. Ce backend peut être exact lorsque des certificats de couverture et de coupe passent, sinon son rapport doit dire que la complétude est inconnue.
- **`entropic_witnessed_dtm`** : cible de production en grande dimension. On déplace l'interaction d'ordre $K$ à l'intérieur d'atomes locaux de masse $K/n$, puis on prend un minimum externe. Le champ devient une distance de puissance dont toute la hiérarchie $H_0$ est représentée par un MST de collisions de boules de puissance. Cette voie est stable, sans grille, valable dans tout $\mathbb R^p$ et compatible avec un pipeline GPU clairsemé. En contrepartie, elle n'est pas l'exacte $K$-couverture HGP du manuscrit.

Pour le cas $50\,000$ points en 3D, le second backend a une chance crédible de passer sous la seconde après préchauffage. Le premier peut être rapide sur des données favorables, mais aucune garantie de temps ne peut être donnée sans hypothèse de faible dimension intrinsèque et de marges de rang.

L'implémentation actuelle `PowerCover3D` est mathématiquement plus sérieuse que le prototype historique et a réellement tourné sur GPU à 30 millions de points. Elle reste néanmoins une approximation cubique 3D. Les expériences du dépôt montrent elles-mêmes que l'erreur de grille domine actuellement la qualité des clusters ; augmenter l'entropie ne la corrige pas.

---

## 1. Objet exact hérité des deux premières parties du manuscrit

### 1.1 Hiérarchie continue

Soit

$$
X=\{x_1,\ldots,x_n\}\subset\mathbb R^p.
$$

Le rayon $K$-NN continu est

$$
r_K(y)
=\inf\bigl\{r\ge 0:\lvert B(y,r)\cap X\rvert\ge K\bigr\}.
$$

Il est la $K$-ième valeur ordonnée de $\{\lVert y-x_i\rVert\}_{i=1}^n$. Pour l'estimateur usuel

$$
\widehat f_K(y)=\frac{K}{n\omega_p r_K(y)^p},
$$

les amas de forte densité de Hartigan sont, à reparamétrisation près, les composantes des sous-niveaux

$$
L_K(r)=\{y:r_K(y)\le r\}.
$$

La cible première de HGP-Clusterer est donc la filtration

$$
r\longmapsto \pi_0\bigl(L_K(r)\bigr),
$$

et non un graphe $K$-NN construit seulement sur les observations.

Repères du manuscrit : estimateur $K$-NN, définition 7, p. 23--24 (PDF 45--46) ; amas discrets, définition 8, p. 25 (PDF 47).

### 1.2 Higher-order interactions

On a l'identité

$$
L_K(r)
=\left\lbrace y:\#\{i:\lVert y-x_i\rVert\le r\}\ge K\right\rbrace 
=\bigcup_{\lvert\tau\rvert=K}
\underbrace{\bigcap_{i\in\tau}B(x_i,r)}_{T_\tau(r)}.
$$

Les régions $T_\tau(r)$ sont convexes. Les composantes de leur union sont donc les composantes de leur graphe d'intersection.

La convention du manuscrit est essentielle :

- une interaction entre $K$ points est un $(K-1)$-simplexe $\tau$ de cardinal $K$ ;
- un événement élémentaire de fusion est porté par une coface $\sigma$ de cardinal $K+1$, donc un $K$-simplexe ;
- pour $K=10$, les sommets du graphe HGP sont donc des 9-simplexes et les événements de fusion des 10-simplexes.

Le théorème de correspondance Čech--amas $K$-NN est donné p. 62--63 (PDF 84--85). Les rayons de naissance, le graphe de Gabriel et le $K$-MST sont étudiés p. 86--93 (PDF 108--115).

### 1.3 La sortie discrète est un recouvrement

Pour une composante continue $C\subset L_K(r)$, le cluster discret naturel est

$$
C^{\mathrm{disc}}
=\left\lbrace x_i:\exists y\in C,\ \lVert x_i-y\rVert\le r\right\rbrace .
$$

Pour $K\ge2$, plusieurs composantes continues peuvent impliquer une même observation : la sortie naturelle est un recouvrement, pas nécessairement une partition. Toute fonction `labels_at_cut` impose donc une politique supplémentaire qui doit rester séparée de l'objet mathématique.

### 1.4 Convention de rayon

Le paramètre $r$ est le rayon des boules. Pour $K=1$, deux boules fusionnent lorsque

$$
\lVert x_i-x_j\rVert\le 2r.
$$

Une arête euclidienne de longueur $\ell$ provoque donc une fusion au rayon $r=\ell/2$. Le code doit distinguer :

- longueur euclidienne $\ell$ ;
- rayon de filtration $r=\ell/2$ ;
- énergie $t=r^2$ ;
- niveau de densité $\lambda=K/(n\omega_p r^p)$.

Les ex æquo au même rayon doivent former un événement multiway. Un ordre arbitraire de fusions binaires ne doit pas changer la sortie.

---

## 2. Ce que l'analogie avec Sinkhorn permet réellement

### 2.1 Pourquoi Sinkhorn simplifie le transport optimal

Dans le transport optimal discret, l'objet combinatoire est un programme linéaire sur un polytope de couplages dont les marginales sont fixées. L'ajout d'un terme KL rend le problème strictement convexe ; les deux familles de contraintes marginales conduisent aux normalisations alternées de Sinkhorn.

La hiérarchie HGP n'est pas un programme de transport de cette forme. Son coût vient de deux sources distinctes :

1. le rang $K$ parmi $n$ distances ;
2. la topologie globale de toutes les régions où ce rang change.

Le premier point se régularise très bien. Le second ne disparaît pas.

### 2.2 Un softmax ligne par ligne n'est pas Sinkhorn

L'implémentation actuelle construit chaque loi $q_i$ indépendamment. Elle impose une contrainte de ligne, mais aucune marginale de colonne. Il s'agit d'une loi de Gibbs locale, pas d'un plan bistochastique.

Un véritable Sinkhorn sparse pourrait être introduit si l'on imposait aussi une contrainte sur $Q^\top\mathbf 1$. Cela contrôlerait le hubness en grande dimension. Il faut toutefois éviter une marginale de colonne uniforme rigide : elle tendrait à effacer les variations de densité que l'algorithme cherche précisément à mesurer. Si cette voie est testée, une pénalisation **non équilibrée** des colonnes est plus cohérente qu'un équilibrage exact.

### 2.3 L'entropie ne supprime pas la taille de sortie

Même après régularisation :

- un champ global à température positive a un support dense ;
- évaluer exactement sa queue peut demander de considérer tous les points ;
- extraire tous ses cols reste un problème continu ;
- la $K$-couverture exacte peut posséder un grand nombre de lentilles et de cofaces.

Il faut donc séparer trois statuts :

1. **définition régularisée exacte** ;
2. **génération rapide de candidats** ;
3. **certification topologique dure**.

Le dépôt mélange encore partiellement les deux derniers dans le prototype Rank-Gabriel.

### 2.4 Notations à ne pas confondre

| Symbole | Rôle |
|---|---|
| $K$ | ordre final demandé, par exemple 10 |
| $k$ | ordre courant dans la progression $1,\ldots,K$ |
| $m_{\rm reg}$ | largeur du support ANN local utilisé pour régulariser |
| $\tau_{\rm rank}$ | température de la libre énergie servant à proposer les changements de rang |
| $\tau_{\rm atom}$ | température de la Gibbs à cardinal fixé dans un atome local |
| $\kappa$ | perplexité effective du softmax indépendant utilisé par le code actuel |
| $L$ | degré du graphe candidat du MST |

Ces paramètres ne sont pas interchangeables. En particulier, augmenter $m_{\rm reg}$ améliore un support de calcul ; cela ne change ni $K$ ni, à lui seul, le niveau de lissage.

---

## 3. Trois définitions possibles de la régularisation

### 3.1 Régularisation littérale du rang : libre énergie fermionique

Posons

$$
q_i(y)=\lVert y-x_i\rVert^2,
\qquad
q_{(1)}(y)\le\cdots\le q_{(n)}(y),
$$

et

$$
S_k(y)=\sum_{j=1}^k q_{(j)}(y).
$$

Pour $\tau>0$, définissons la fonction de partition normalisée

$$
Z_{k,\tau}(y)
=\frac{1}{\binom nk}
\sum_{\substack{A\subset[n]\\|A|=k}}
\exp\!\left(-\frac{1}{\tau}\sum_{i\in A}q_i(y)\right),
$$

la libre énergie

$$
F_{k,\tau}(y)=-\tau\log Z_{k,\tau}(y),
\qquad F_{0,\tau}=0,
$$

et le shell de rang régularisé

$$
R_{k,\tau}^2(y)=F_{k,\tau}(y)-F_{k-1,\tau}(y).
$$

Cette construction encode réellement une interaction d'ordre $k$ : la loi de Gibbs porte sur les sous-ensembles de cardinal exactement $k$, sans remise.

#### Bornes de biais

Le log-sum-exp donne

$$
S_k(y)
\le F_{k,\tau}(y)
\le S_k(y)+\tau\log\binom nk.
$$

Comme $q_{(k)}=S_k-S_{k-1}$,

$$
-\tau\log\binom n{k-1}
\le R_{k,\tau}^2(y)-q_{(k)}(y)
\le\tau\log\binom nk.
$$

La régularisation converge donc uniformément vers la statistique d'ordre dure lorsque $\tau\log\binom nk\to0$. Pour $K=10$ et grand $n$, cette condition impose une température assez faible ; le facteur combinatoire ne peut pas être ignoré.

#### Calcul sans explosion combinatoire

Avec

$$
z_i(y)=e^{-q_i(y)/\tau},
$$

la somme non normalisée est le polynôme symétrique élémentaire $e_k(z_1,\ldots,z_n)$. Tous les $e_0,\ldots,e_K$ se calculent par la récurrence

$$
E_0\leftarrow1,\qquad
E_r\leftarrow E_r+z_iE_{r-1}\quad(r=K,\ldots,1),
$$

en $O(nK)$ opérations et $O(K)$ mémoire par requête. Sur un support local de taille $m$, le coût devient $O(mK)$. Il faut une implémentation log-domain ou renormalisée pour éviter sous-flux et annulations.

#### Le cas $K=1$

On obtient

$$
F_{1,\tau}(y)
=-\tau\log\left(\frac1n\sum_i e^{-\lVert y-x_i\rVert^2/\tau}\right),
$$

c'est-à-dire, à une constante près, le logarithme négatif d'une KDE gaussienne. Sa structure hiérarchique est le merge tree d'un champ lisse, **pas** un MST des observations.

Tout point critique satisfait une équation de mean-shift. Si

$$
p_i^{(k)}(y)=\mathbb P_{k,y}(i\in A),
$$

et

$$
a_i^{(k)}(y)=p_i^{(k)}(y)-p_i^{(k-1)}(y),
$$

alors

$$
\nabla R_{k,\tau}^2(y)
=2\left(y-\sum_i a_i^{(k)}(y)x_i\right).
$$

Les candidats critiques vérifient donc

$$
y=\sum_i a_i^{(k)}(y)x_i.
$$

Cette identité fournit un **rank-shell mean-shift** naturel pour suivre les points critiques de $k=1$ à $K$. Pour $k>1$, l'itération brute n'est pas garantie contractante : il faut un amortissement, une région de confiance et une vérification du gradient et de la Hessienne.

#### Verdict

Cette libre énergie est la meilleure réponse à la question « quelle est la régularisation KL canonique de la $K$-ième statistique d'ordre ? ». Elle est excellente comme champ de proposition et comme objet théorique. Elle n'est pas la bonne structure finale si l'on veut conserver le théorème Čech et un squelette de type MST.

### 3.2 Atomes entropiques puis rang dur : la cible HGP fidèle

Pour chaque observation, choisissons un support local $A_i$ et une loi

$$
\nu_i=\sum_{j\in A_i}q_{ij}\delta_{x_j}.
$$

Posons

$$
z_i=\sum_j q_{ij}x_j,
\qquad
a_i=\sum_jq_{ij}\lVert x_j-z_i\rVert^2.
$$

On a l'identité exacte

$$
d_i(y)^2
:=W_2^2(\delta_y,\nu_i)
=\sum_jq_{ij}\lVert y-x_j\rVert^2
=\lVert y-z_i\rVert^2+a_i.
$$

Chaque atome devient donc un site de puissance. On garde ensuite l'ordre statistique dur :

$$
r_{K,q}(y)=K\text{-ième plus petite valeur de }\{d_i(y)\}_{i=1}^n.
$$

Alors

$$
\{r_{K,q}\le r\}
=\left\lbrace y:\#\left\lbrace i:\lVert y-z_i\rVert^2+a_i\le r^2\right\rbrace \ge K\right\rbrace .
$$

Il s'agit exactement d'une $K$-couverture de boules de puissance. Le théorème Čech du manuscrit s'étend immédiatement, car les intersections restent convexes.

#### Stabilité

Définissons

$$
s_i=W_2(\delta_{x_i},\nu_i),
\qquad s=\max_i s_i.
$$

Par l'inégalité triangulaire de $W_2$,

$$
|d_i(y)-\lVert y-x_i\rVert|\le s_i.
$$

La statistique d'ordre est 1-lipschitzienne pour la norme uniforme, donc

$$
\boxed{\ \lVert r_{K,q}-r_K\rVert_\infty\le s.\ }
$$

On en déduit l'entrelacement

$$
L_K(r-s)
\subseteq L_{K,q}(r)
\subseteq L_K(r+s),
$$

et une borne $s$ sur la distance d'interleaving des merge trees, sous les hypothèses usuelles de finitude.

Le mode `local_distortion` de `PowerCover3D` va plus loin : si

$$
s_i\le\gamma r_K(x_i),\qquad \gamma<\tfrac12,
$$

il obtient correctement

$$
(1-2\gamma)r_K(y)
\le r_{K,q}(y)
\le(1+2\gamma)r_K(y).
$$

#### Verdict

C'est la cible exacte recommandée pour le backend scientifique `power_multicover`. Elle préserve HGP et donne un paramètre entropique interprétable par une borne de déformation. Elle ne supprime pas la combinatoire pour $K>1$.

### 3.3 Déplacer $K$ dans l'atome : distance-$K$ témoignée entropique

La troisième construction change volontairement d'estimateur afin de retrouver un MST à tout ordre.

Pour la mesure empirique $\mu_n=n^{-1}\sum_i\delta_{x_i}$ et $m=K/n$, la distance à la mesure empirique vérifie

$$
d_{\mu_n,m}^2(y)
=\frac1K\sum_{j=1}^{K}\lVert y-x_{(j)}(y)\rVert^2.
$$

Elle admet la formulation de transport partiel

$$
d_{\mu_n,m}^2(y)
=\min_{q\in\mathcal H_K}
\sum_iq_i\lVert y-x_i\rVert^2,
$$

où

$$
\mathcal H_K
=\left\lbrace q\ge0:\sum_iq_i=1,\ q_i\le\frac1K\right\rbrace .
$$

$\mathcal H_K$ est un hypersimplexe redimensionné. Ses sommets sont exactement les lois uniformes sur les sous-ensembles de cardinal $K$. La contrainte $q_i\le1/K$ encode donc une interaction d'ordre $K$ plus fidèlement qu'une simple contrainte de perplexité.

L'exacte DTM est une distance de puissance sur tous les barycentres de $K$-sous-ensembles, donc à nouveau combinatoire. La **witnessed $K$-distance** de Guibas, Mérigot et Morozov ne conserve que le barycentre des $K$ voisins de chaque observation : elle utilise $O(n)$ sites de puissance.

#### Version entropique proposée

Pour chaque ancre $x_i$, prenons un support $A_i$ de taille $m_{\rm reg}\ge K$ et une Gibbs sur les sous-ensembles $S\subset A_i$, $|S|=K$ :

$$ \Pi_i^{K,\tau}(S) \propto \exp\!\left[ -\frac{1}{K\tau_i} \sum_{j\in S}\lVert x_i-x_j\rVert^2 \right]. $$

Définissons les marginales

$$
q_{ij}^{K,\tau}
=\frac1K\,\mathbb P_{\Pi_i^{K,\tau}}(j\in S).
$$

Alors

$$
\sum_jq_{ij}^{K,\tau}=1,
\qquad
0\le q_{ij}^{K,\tau}\le\frac1K.
$$

Quand $\tau_i\downarrow0$ et en l'absence d'ex æquo, on retrouve la loi uniforme sur les $K$ plus proches voisins de $x_i$.

Les marginales se calculent par préfixes/suffixes de polynômes symétriques élémentaires en $O(m_{\rm reg}K)$, sans énumération des sous-ensembles.

Posons

$$
z_i^{(K)}=\sum_jq_{ij}^{K,\tau}x_j,
\qquad
a_i^{(K)}=\sum_jq_{ij}^{K,\tau}\lVert x_j-z_i^{(K)}\rVert^2.
$$

Le champ recommandé est ensuite tropical :

$$
D_{K,\tau}^{W}(y)
=\min_i
\sqrt{\lVert y-z_i^{(K)}\rVert^2+a_i^{(K)}}.
$$

Il est une distance de puissance. Ses sous-niveaux sont des unions de $n$ boules de puissance et leur hiérarchie $H_0$ est entièrement représentée par un MST de puissance.

#### Garanties

Soit $D_K$ l'exacte DTM empirique et $D_K^W$ sa version témoignée dure. Sur un domaine $\Omega$ où les ancres forment un $\delta$-net,

$$
0\le D_K^W(y)-D_K(y)\le2\delta.
$$

En effet, pour une ancre $w$ telle que $\lVert y-w\rVert\le\delta$, l'atome $K$-NN de $w$ est optimal en $w$ ; la 1-lipschitzianité donne

$$
D_K^W(y)
\le \lVert y-w\rVert+D_K(w)
\le D_K(y)+2\delta.
$$

Si $\nu_i^0$ désigne l'atome dur et si la régularisation vérifie

$$
\max_iW_2(\nu_i^{\tau},\nu_i^0)\le B,
$$

alors

$$
\lVert D_{K,\tau}^{W}-D_K^W\rVert_\infty\le B.
$$

La hiérarchie entropique témoignée est donc à distance d'interleaving au plus $2\delta+B$ de la hiérarchie DTM sur $\Omega$.

Une borne GPU bon marché de $W_2(\nu_i^{\tau},\nu_i^0)$ peut être obtenue en faisant correspondre la masse commune sur place puis en majorant le déplacement restant par le diamètre du support :

$$
W_2^2(\nu_i^{\tau},\nu_i^0)
\le \operatorname{diam}(A_i)^2
\operatorname{TV}(q_i^{\tau},q_i^0).
$$

#### Rapport avec le rayon $K$-NN dur

On a toujours

$$
\frac{r_K(y)}{\sqrt K}
\le D_K(y)
\le r_K(y).
$$

Par conséquent,

$$
\{r_K\le r\}
\subseteq\{D_K\le r\}
\subseteq\{r_K\le\sqrt K\,r\}.
$$

Cette borne est honnête mais lâche : pour $K=10$, le facteur uniforme vaut $\sqrt{10}$. Sous régularité locale de la densité, les rayons successifs sont beaucoup plus proches et l'écart réel est généralement moindre. Il faut néanmoins reconnaître que la hiérarchie DTM n'est pas la hiérarchie HGP exacte.

#### Verdict

C'est le meilleur compromis connu entre :

- interaction d'ordre $K$ ;
- stabilité Wasserstein ;
- représentation linéaire en $n$ ;
- absence de grille ;
- hiérarchie codée par un MST ;
- calcul en dimension ambiante quelconque.

Il doit avoir un nom d'API distinct et ne pas être présenté comme HGP-Clusterer exact.

---

## 4. Le cas $K=1$ : MST de collisions de boules de puissance

Le cas $K=1$ fournit le squelette fondamental des deux backends à sites de puissance.

Considérons

$$
d_i(y)^2=\lVert y-z_i\rVert^2+a_i.
$$

Le site $i$ naît au rayon

$$
b_i=\sqrt{a_i}.
$$

Pour deux sites $i,j$, le premier rayon d'intersection est

$$
\beta_{ij}
=\min_y\max\{d_i(y),d_j(y)\}.
$$

Soit $D_{ij}=\lVert z_i-z_j\rVert$. Si $D_{ij}>0$, posons

$$
u_{ij} =\mathrm{clip}\!\left( \frac{D_{ij}^2+a_j-a_i}{2D_{ij}}, 0,D_{ij} \right).
$$

Alors

$$
\boxed{ \beta_{ij}^2 =\max\left\lbrace  u_{ij}^2+a_i, (D_{ij}-u_{ij})^2+a_j \right\rbrace . }
$$

Si $D_{ij}=0$,

$$
\beta_{ij}^2=\max(a_i,a_j).
$$

Lorsque $a_i=a_j=0$,

$$
\beta_{ij}=\frac{\lVert x_i-x_j\rVert}{2},
$$

ce qui retrouve exactement la convention du manuscrit.

Le point critique associé est également explicite : dans le cas équilibré,

$$
c_{ij}
=z_i+\frac{u_{ij}}{D_{ij}}(z_j-z_i).
$$

Dans le cas dominé, il s'agit de $z_i$ ou $z_j$.

### Théorème du MST de puissance

Considérons le graphe complet dont :

- le sommet $i$ apparaît au rayon $b_i$ ;
- l'arête $ij$ apparaît au rayon $\beta_{ij}$.

À tout rayon $r$, ce graphe est exactement le graphe d'intersection des boules

$$
B\!\left(z_i,\sqrt{r^2-a_i}\right).
$$

Ses composantes sont donc celles de leur union. Un MST du graphe complet pondéré par $\beta_{ij}$, complété par les naissances $b_i$, préserve ces composantes à tous les seuils.

La bonne structure du 1-NN régularisé compatible avec HGP est ainsi :

$$
\boxed{
\text{un MST de puissance avec poids sur sommets et arêtes.}
}
$$

Le mode `soft_only` actuel de `PERGHGPClusterer` ne calcule pas cet objet. Il relie chaque site à son meilleur voisin au point $Z_i$, puis calcule un MSF du graphe obtenu. Un graphe 1-NN ne contient pas en général toutes les arêtes du MST et peut rester déconnecté.

### Calcul GPU

Le noyau $\beta_{ij}$ est élémentaire et vectorisable. Le vrai verrou est de trouver les arêtes candidates sans matérialiser les $n(n-1)/2$ paires.

Un pipeline pratique doit réunir :

1. voisins euclidiens des centres $z_i$ ;
2. voisins de puissance, obtenus par le relèvement $(z_i,\sqrt{a_i})$ ;
3. arêtes du graphe $K$-NN original ;
4. arêtes du MST à l'ordre précédent ;
5. expansion adaptative des composantes Borůvka non certifiées.

La borne

$$
\beta_{ij}\ge
\max\left\lbrace \frac{D_{ij}}2,\sqrt{a_i},\sqrt{a_j}\right\rbrace 
$$

donne un certificat utile. Si une composante Borůvka possède un meilleur candidat de rayon $U$, toute meilleure arête omise doit satisfaire

$$
D_{ij}<2U,
\qquad a_i<U^2,
\qquad a_j<U^2.
$$

Une range-search exhaustive dans cette boule certifie la coupe. En haute dimension, cette recherche peut dégénérer en scan quadratique ; le rapport doit alors passer de `exact` à `conditional` plutôt que masquer le fallback.

En 3D, une triangulation régulière des sites pondérés $(z_i,w_i=-a_i)$ fournit également un oracle exact d'arêtes de complexe alpha pondéré. Elle est appropriée comme oracle CPU pour $n\simeq50\,000$ et comme référence de tests, même si le chemin GPU Borůvka sera vraisemblablement plus rapide.

---

## 5. Extension HGP fidèle à $K>1$ sans mosaïque complète

### 5.1 Objet dur

Pour une face $\tau\subset[n]$, définissons son rayon de naissance

$$ \rho_q(\tau)^2 =\min_y\max_{i\in\tau} \left(\lVert y-z_i\rVert^2+a_i\right). $$

Pour $m=|\tau|$, cette miniballe additive admet le dual concave

$$ \rho_q(\tau)^2 =\max_{\lambda\in\Delta_m} \left[ \sum_{i\in\tau}\lambda_i \bigl(\lVert z_i\rVert^2+a_i\bigr) -\left\lVert\sum_{i\in\tau}\lambda_i z_i\right\rVert^2 \right], $$

et son centre vaut $c_\tau=\sum_i\lambda_i z_i$. Pour $K=10$, une coface ne contient que 11 sites : une fois le candidat découvert, son poids et son centre se calculent donc par un petit solveur actif, facilement batchable sur GPU. La difficulté n'est pas la validation d'une coface, mais sa découverte exhaustive.

À l'ordre $k$ :

- les sommets du graphe dual sont les faces $\tau$ de cardinal $k$ ;
- une coface $\sigma$ de cardinal $k+1$ relie simultanément ses $k+1$ facettes ;
- son poids est $\rho_q(\sigma)$ ;
- une étoile de $k$ arêtes au même poids représente l'événement multiway ;
- les naissances $\rho_q(\tau)$ des faces doivent être conservées en plus des arêtes du $k$-MSF.

Une simple liste d'arêtes sans naissances de faces ne décrit pas la hiérarchie complète.

### 5.2 Itérer $k=1,\ldots,K$ est utile, mais les MSF ne sont pas imbriqués

On a bien

$$
L_{k+1}(r)\subseteq L_k(r),
$$

mais les cofaces critiques ne se déduisent pas uniquement du MSF précédent. Une coface critique à l'ordre $k+1$ peut avoir des facettes qui n'étaient ni séparantes ni retenues à l'ordre $k$.

La progression doit donc réutiliser :

- les points critiques précédents ;
- une bande de challengers près des frontières de rang ;
- toutes les facettes des cofaces acceptées ;
- de nouveaux seeds locaux indépendants à chaque ordre.

Elle ne doit jamais n'utiliser que les arêtes ou cofaces du MSF précédent.

### 5.3 Générateur de candidats higher-order

À un témoin $y$, la Gibbs sur les $k$-sous-ensembles de sites de puissance est

$$ \mathbb P_{k,\tau}(A\mid y) \propto \exp\!\left[ -\frac1\tau\sum_{i\in A}\phi_i(y) \right], \qquad |A|=k, $$

avec

$$
\phi_i(y)=\lVert y-z_i\rVert^2+a_i.
$$

Sur un support local de $m$ candidats, le top-$k$ entropique se calcule en $O(mk)$ par polynômes symétriques. Si l'ensemble optimal dur est unique et séparé du deuxième par un gap énergétique $\Delta$, alors

$$
\mathbb P(A\neq A_*)
\le\left(\binom mk-1\right)e^{-\Delta/\tau}.
$$

Cette borne permet de choisir la température à partir d'une probabilité de proposition manquée, au lieu d'un paramètre arbitraire.

Pour l'ingénierie, la relaxation Fermi

$$
u_i(y)=\frac{1}{1+\exp((\phi_i(y)-\mu(y))/\tau)},
\qquad \sum_i u_i(y)=k,
$$

est encore moins coûteuse : seul $\mu(y)$ est trouvé par bisection. Les sites où $u_i(1-u_i)$ est grand sont les challengers proches d'un changement de rang.

### 5.4 Candidats de points critiques

Le `rank_field.py` historique possède déjà la bonne libre énergie $F_k$ et les shells $F_k-F_{k-1}$. Il faut remplacer le pool actuel par une continuation de points critiques :

1. seeds : centres du MST de puissance, observations à forte entropie de shell, centres des cofaces acceptées, barycentres de voisinages communs ;
2. continuation de $k-1$ vers $k$ et d'une température haute vers une température basse ;
3. itération rank-shell mean-shift amortie ;
4. Newton tronqué ou trust-region avec produits Hessienne--vecteur ;
5. conservation séparée des minima, selles d'indice 1 et dégénérescences ;
6. réinjection des centres certifiés à l'ordre suivant.

Le pool actuel (`witnesses.py`) ne fait qu'une itération barycentrique et quelques moyennes des premiers voisins. Il n'a ni classification d'indice, ni analyse de bassin, ni garantie de couverture ; en cas de filtre vide, il conserve arbitrairement les mille premiers candidats. Cette partie doit être réécrite, pas simplement optimisée.

### 5.5 Certification dure

Chaque coface $\sigma$ proposée doit subir :

1. calcul robuste de la miniballe additive $\rho_q(\sigma)$ ;
2. vérification que toutes ses facettes sont correctement représentées ;
3. test Gabriel de puissance global, ou garde ANN avec borne d'omission ;
4. test de coupe Borůvka dans le graphe dual ;
5. enregistrement des gaps numériques et géométriques.

Le test naturel est

$$
\min_{j\notin\sigma}\phi_j(c_\sigma)
\ge \rho_q(\sigma)^2.
$$

Il s'écrit comme une recherche de produit scalaire maximal :

$$ \min_j\phi_j(c) =\lVert c\rVert^2 -\max_j\left\langle (2c,-1),\, (z_j,\lVert z_j\rVert^2+a_j) \right\rangle. $$

Le prédicat Gabriel global est donc compatible avec un top-1 MIPS GPU, suivi d'une garde exacte sur les quasi-ex æquo. L'ANN accélère le test ; seule une borne d'omission peut le certifier.

L'extension pondérée du théorème « toute coface séparante est Gabriel » est plausible sous position générale de puissance, mais elle n'est pas démontrée dans le manuscrit. Elle doit faire l'objet d'un théorème séparé avant d'être utilisée comme certificat de complétude.

### 5.6 Résultat conditionnel de complétude des témoins

Soit $\sigma$ une coface pertinente, de centre $c_\sigma$, et supposons un gap strict

$$
\gamma_\sigma
=\min_{j\notin\sigma}\phi_j(c_\sigma)
-\max_{i\in\sigma}\phi_i(c_\sigma)>0.
$$

Les différences $\phi_j-\phi_i$ sont affines et

$$
\operatorname{Lip}(\phi_j-\phi_i)
=2\lVert z_j-z_i\rVert.
$$

Tout témoin $y$ tel que

$$
\lVert y-c_\sigma\rVert
<
\frac{\gamma_\sigma}
{2\max_{i\in\sigma,j\notin\sigma}\lVert z_j-z_i\rVert}
$$

retrouve donc exactement $\sigma$ dans son top-$(k+1)$ dur.

Ainsi, **marge de rang + couverture des centres critiques** implique la complétude du générateur. C'est une vraie garantie exploitable lorsque les centres critiques vivent sur une structure de faible dimension intrinsèque. Construire cette couverture sans hypothèse géométrique reste exponentiel en pire cas.

### 5.7 Ce que certifie réellement Lazy Cut-Borůvka

Un certificat de coupe sur les faces déjà découvertes prouve seulement que le MSF est exact **dans le graphe découvert**. Il ne prouve pas :

- qu'une face de cardinal $K$ n'a pas été omise ;
- qu'une composante entière de l'atlas n'est pas absente ;
- qu'une coface dont aucune facette n'a été découverte n'existe pas.

Les listes $\beta$ bornent les extensions d'une face connue, pas les faces initialement absentes. Le rapport d'exactitude doit donc distinguer :

```yaml
accepted_cofaces_valid: true | false
candidate_graph_msf_exact: true | false
critical_center_cover_complete: true | false | unknown
power_hgp_hierarchy_complete: true | false | unknown
```

---

## 6. Résultat théorique existant sans Delaunay d'ordre supérieur

Buchet, Dornelas et Kerber construisent une **sparse higher-order Čech filtration**. Pour $n$ sites, ordre fixé $k$, dimension de doublement $\delta$ et $\varepsilon>0$, leur filtration est $(1+\varepsilon)$-homotopy-entrelacée avec la $k$-couverture. Sa taille est linéaire en $n$ lorsque $k$, $\delta$ et $\varepsilon$ sont considérés constants.

C'est la réponse théorique la plus proche du problème posé. Elle évite la mosaïque de Delaunay d'ordre supérieur et traite aussi la multicoverture $k=1,\ldots,\mu$ de façon compatible.

Deux réserves sont décisives.

1. Les constantes contiennent un facteur exponentiel en $k\delta$ ; les auteurs signalent eux-mêmes que l'utilisabilité est limitée aux petits ordres et faibles dimensions de doublement.
2. Le théorème est formulé pour des boules non pondérées. Son extension aux boules de puissance à naissances différentes requiert une nouvelle preuve des rayons de gel, des propriétés de packing et des prédicats d'intersection.

Il faut donc utiliser ce travail comme modèle théorique du backend `power_multicover`, pas comme justification immédiate d'un algorithme pratique pour $K=10$ en vraie haute dimension.

Point important pour l'itération : leur construction multicover n'empile pas naïvement dix complexes indépendants. Elle utilise une permutation à la $\mu$-distance et, au niveau $k$, des lentilles de toutes cardinalités $\ell$ avec $k\le\ell\le\mu$ afin de rendre les niveaux compatibles. Cela confirme qu'une simple réutilisation des seuls MSF successifs est mathématiquement insuffisante.

Les mosaïques de Delaunay d'ordre supérieur restent un excellent oracle exact en basse dimension. L'algorithme progressif d'Edelsbrunner et Osang construit l'ordre $k$ depuis les ordres inférieurs au moyen de mosaïques de Delaunay pondérées du premier ordre. Il ne résout toutefois pas le problème de taille de sortie en grande dimension, et sa progression combinatoire n'est pas équivalente à la progression entropique proposée ici. Le bon usage dans `E-HGP` est celui d'un oracle de validation 3D, pas d'une architecture générale.

---

## 7. UMAP : quoi reprendre, quoi refuser

UMAP apporte des briques pratiques utiles :

- ANN/NN-descent ;
- échelle locale $\rho_i$ ;
- température $\sigma_i$ trouvée par bisection ;
- représentation floue d'une frontière de voisinage ;
- kernels GPU éprouvés.

Sa calibration résout approximativement

$$ \sum_j \exp\!\left[ -\frac{\max(0,d_{ij}-\rho_i)}{\sigma_i} \right] =\log_2 K. $$

Cette normalisation vise un voisinage flou de cardinalité comparable en tout point. Pour du clustering de densité, elle peut effacer une partie des variations de densité physique. Elle doit donc seulement initialiser la température ; le réglage final doit respecter un budget de déformation $s$, $B$ ou un gap de rang.

Le graphe UMAP est un 1-squelette. Ajouter des triangles d'un flag complex sans modifier les activations n'affecte pas $H_0$. Pour que les interactions d'ordre supérieur changent les fusions, elles doivent agir sur les poids des cofaces ou sur la loi des $K$-sous-ensembles, pas seulement remplir des simplexes au-dessus du même graphe.

À retenir :

- utiliser UMAP/cuVS pour proposer des voisins et calibrer des températures ;
- ne pas utiliser la symétrisation floue comme définition du cluster tree ;
- ne pas réduire d'abord les données par UMAP en prétendant préserver la hiérarchie HGP ;
- mesurer le **cut recall** des minima sortants Borůvka, pas seulement `recall@10`.

---

## 8. Architecture recommandée

### 8.1 Backend fidèle : `power_multicover`

```text
X
 └─> support ANN sur-échantillonné
      └─> lois locales q_i sous budget de déformation
           └─> sites de puissance (z_i,a_i)
                ├─> K=1 : vrai MST de puissance
                └─> k=2,...,K :
                     libre énergie locale + rank-shell mean-shift
                     └─> cofaces candidates
                          └─> miniballe + Gabriel + audit de coupe
                               └─> k-MSF + naissances de faces
```

#### Étapes

1. Construire une seule liste ANN de largeur $m_{\rm reg}\gg K$ ; $64$ est un point de départ pour $K=10$, pas une constante théorique.
2. Résoudre la régularisation locale sous budget de déformation. Conserver `fixed_kappa` seulement pour les expériences contrôlées.
3. Construire le MST de puissance exact ou certifié à $k=1$.
4. Calculer les shells entropiques jusqu'à $K+1$ sur les supports locaux.
5. Continuer les points critiques, avec seeds supplémentaires à chaque ordre.
6. Proposer seulement les challengers de shell, sans énumérer les cliques.
7. Certifier les cofaces en float64 robuste.
8. Construire le $K$-MSF dual sur GPU.
9. Produire un rapport de complétude gradué.

#### Mode rapide et mode certifié

```yaml
mode: fast
ann_width: 64
candidate_expansion: bounded
critical_cover: heuristic
cut_audit: sampled
```

```yaml
mode: certified_when_possible
ann_width: adaptive
candidate_expansion: until_cut_certificate
critical_cover: margin_and_net_report
cut_audit: exhaustive_on_unresolved_components
fail_on_unknown: true
```

L'option `fail_on_unknown` est indispensable pour ne pas publier une hiérarchie incomplète comme exacte.

### 8.2 Backend pratique : `entropic_witnessed_dtm`

```text
X
 └─> m-NN GPU
      └─> Gibbs sur les K-sous-ensembles locaux
           └─> marginales q_i, centres z_i, variances a_i
                └─> graphe candidat de collisions de puissance
                     └─> Borůvka / MST GPU
                          └─> merge tree + affectation de puissance
```

#### Progression $k=1,\ldots,K$

```text
Compute one m-NN support A_i for every i
E_prev <- empty

for k = 1,...,K:
    if k == 1:
        q_i <- delta_i
    else:
        q_i <- fixed-cardinality Gibbs marginals on A_i

    (z_i,a_i) <- moments(q_i)

    E <- original_mNN
         union center_mNN(z)
         union lifted_power_mNN(z,sqrt(a))
         union E_prev

    repeat:
        beta <- exact_power_collision_weights(E)
        T <- GPU_MSF(E,beta)
        unresolved <- boruvka_cut_audit(T,z,a)
        expand E only around unresolved cuts
    until certified or budget exhausted

    emit merge tree T and diagnostics
    E_prev <- edges(T) plus near-ties
```

Les MST successifs ne sont pas imbriqués ; `E_prev` est un warm start, jamais une preuve de complétude.

Il ne faut pas pour autant lancer dix relaxations indépendantes. Sur chaque support ou témoin, un seul DP calcule $e_0,\ldots,e_{K+1}$ et les marginales de tous les rangs en $O(m_{\rm reg}K)$ ; la relaxation Fermi peut de même warm-starter $\mu_k$ par $\mu_{k-1}$. Ce qui progresse réellement de $1$ à $K$ est l'atlas géométrique — faces, cofaces et centres critiques réinjectés — et non le coût du soft top-$k$.

#### Affectation des observations

À une coupe $r$, l'appartenance continue est déterminée par les composantes du MST de puissance. Une partition plate peut affecter chaque observation $x_i$ au site de puissance minimisant

$$
\lVert x_i-z_j\rVert^2+a_j,
$$

si cette valeur est au plus $r^2$. Les ex æquo et les observations couvertes par plusieurs composantes doivent être signalés avant toute politique de tie-break.

Cette affectation est directe et ne passe pas par une relation cellule--composante.

---

## 9. Complexité et mémoire

Soient :

- $n$ le nombre de points ;
- $p$ la dimension ambiante ;
- $m$ la largeur du support local ;
- $L$ le degré du graphe candidat de MST ;
- $K\le10$.

### Atomes entropiques

- construction ANN : dépend de l'index ;
- distances du support : $O(nmp)$ ;
- Gibbs à cardinal fixé : $O(nmK)$ ;
- moments : $O(nmp)$ ;
- mémoire persistante : $O(n(p+1))$ pour $(z_i,a_i)$, plus $O(nm)$ si les voisins sont conservés.

Le support peut être traité en streaming pour éviter $O(nm)$ persistant.

### MST de puissance

- graphe candidat : $O(nL)$ arêtes ;
- calcul des $\beta$ : $O(nL)$ ;
- MSF GPU : quasi linéaire dans le nombre d'arêtes en pratique ;
- audits : faibles si les coupes sont bien séparées, jusqu'à $O(n^2p)$ dans le pire cas.

### Backend HGP fidèle

La complexité dépend du nombre $M_k$ de faces et cofaces proposées. Le pipeline est utilisable si $M_k=O(nL)$ ou proche. Aucune borne polynomiale indépendante de la dimension intrinsèque ne peut être annoncée pour l'atlas complet.

### Queue entropique

Une Gibbs globale a un support dense. Si l'on conserve seulement $L$ voisins, une borne grossière de masse oubliée est

$$
(n-L)
\exp\!\left[-\frac{q_{(L+1)}-q_{(K)}}{\tau}\right].
$$

En haute dimension, la concentration des distances réduit souvent ce gap. Une température assez grande pour lisser peut donc rendre la troncature locale incorrecte. Le code doit choisir entre :

- un **modèle localisé**, clairement défini sur le support ANN ;
- une approximation globale par sommes de noyaux/RFF, avec ses propres bornes ;
- une largeur adaptative jusqu'à un certificat de queue.

Il ne faut pas appeler « exact global » un calcul sur 64 voisins.

---

## 10. Audit de l'implémentation actuelle au commit `d67647f5`

### 10.1 Deux algorithmes distincts

Le package expose :

- `PowerCover3D`, nouvel algorithme de hiérarchie cubique du champ $K$-ième distance de puissance ;
- `PERGHGPClusterer`, prototype historique Rank-Gabriel.

Les fichiers mathématiques essentiels du prototype historique sont inchangés depuis le commit `150c1ce` :

- `estimator.py` ;
- `grid.py` ;
- `local_gibbs.py` ;
- `rank_field.py` ;
- `witnesses.py` ;
- `gabriel.py` ;
- `cofaces.py` ;
- `dual_graph.py` ;
- `hierarchy.py` ;
- `tests/test_perg_hgp.py`.

Les progrès récents portent principalement sur `PowerCover3D`, le backend CUDA, les audits numériques et les benchmarks G4.

### 10.2 Ce que `PowerCover3D` fait correctement

Le cœur mathématique est valide :

$$
\lVert y-z_i\rVert^2+a_i
=\sum_jq_{ij}\lVert y-x_j\rVert^2,
$$

puis

$$
g_K(y)=K\text{-ième plus petite valeur de }
\sqrt{\lVert y-z_i\rVert^2+a_i}.
$$

Le mode `local_distortion` et ses bornes additives/multiplicatives sont solides, sous réserve de la validité des voisinages et des enveloppes flottantes déclarées.

Le champ cubique est évalué aux centres des cellules. La 1-lipschitzianité donne, pour une demi-diagonale $H$,

$$
g(c_Q)-H-\delta_{\rm num}
\le g(y)
\le g(c_Q)+H+\delta_{\rm num}.
$$

Le MSF du graphe 26-connexe conserve exactement $H_0$ du champ cubique stocké. Les deux enveloppes fournissent donc un entrelacement spatial sérieux.

Le nouveau backend a aussi reçu un travail d'ingénierie important :

- kernels CUDA fusionnés ;
- recomputation des distances associées aux identifiants RBC ;
- overfetch $\times4$ ;
- audit direct float64 ;
- projection fail-closed vers le budget de distorsion ;
- schéma de sauvegarde versionné ;
- harnais de benchmark reproductible ;
- exécution réelle sur RTX PRO 6000 Blackwell.

Il faut réutiliser ces briques, notamment la régularisation locale, les diagnostics, la normalisation numérique et le harnais de benchmark.

### 10.3 Ce que `PowerCover3D` ne fait pas

`PowerCover3D` n'utilise pas `rank_field`, `witnesses`, `cofaces`, `gabriel` ou `dual_graph`. Il ne calcule donc :

- ni atlas de rang ;
- ni faces HGP ;
- ni cofaces ;
- ni mosaïque de Delaunay d'ordre $K$ ;
- ni $K$-MSF du manuscrit.

Il calcule une approximation de $H_0$ du champ continu choisi, ce qui est un objectif légitime mais différent.

Toute la géométrie est codée en dimension 3 : formes `(N,3)`, cellules à trois indices, 26-connexité, prédicats boule--AABB 3D et relèvement CPU en dimension 4. Cette architecture ne se généralise pas à la grande dimension.

### 10.4 La grille est le verrou de qualité

Le [rapport G4 du dépôt](https://github.com/Ludwig-H/E-HGP/blob/d67647f5bc6ad7ce76e259541a5b77d116359555/perg_hgp/benchmarks/results/G4_20260712_REPORT.md) donne un résultat négatif mais clair.

Sur trois nuages synthétiques de $n=1000$, grille $24^3$ :

- beaucoup d'ARI oracle `PowerCover` valent 0 ;
- HDBSCAN se situe approximativement entre 0,62 et 0,95 ;
- HGP-CGAL entre 0,57 et 0,95 ;
- passer de $24^3$ à $48^3$ sur les blobs anisotropes, $K=10$, améliore l'ARI de 0,271 à 0,586, mais le fit CPU passe de 1,238 s à 6,961 s ;
- la régularisation locale change peu l'ARI à résolution fixée.

La conclusion du benchmark est correcte : augmenter l'entropie ne répare pas une grille sous-résolue.

### 10.5 Le prototype Rank-Gabriel reste incomplet

Points principaux :

- `rank_field.py` implémente une libre énergie et des shells mathématiquement intéressants ;
- l'implémentation repose encore sur des boucles/tableaux PyTorch et n'est pas un kernel industriel ;
- `witnesses.py` ne garantit pas la couverture des points critiques ;
- `extract_top_cofaces` vérifie la cohérence de candidats proposés, pas leur complétude ;
- le paramètre `eta` de l'extraction de cofaces est inutilisé ;
- le test top-$(K+1)$ float32 ne certifie pas la stricte condition Gabriel en présence de quasi-ex æquo ;
- le graphe dual et Borůvka font encore des déduplications, rescans et unions largement CPU ;
- aucun benchmark actuel du `PERGHGPClusterer` ne démontre une utilisation à $50\,000$ points.

### 10.6 Certification CUDA conditionnelle

Dans `PowerCover3D`, `certified_fraction=1` signifie que toutes les requêtes passent la garde **conditionnellement** à la validité de l'ordre RBC renvoyé. L'audit de quelques requêtes ne constitue pas une preuve exhaustive.

Le dépôt le documente correctement. Le nouveau backend est donc mieux décrit par « conditionnellement certifié et empiriquement audité » que par « exact ».

---

## 11. Cas 3D : $50\,000$ points, $K=10$, moins d'une seconde

### 11.1 État du backend actuel

Sur RTX PRO 6000 Blackwell, les résultats archivés sont :

| Cas | Temps de fit |
|---|---:|
| 100 k, $K=10$, grille $48^3$, $\kappa=1$ | 1,139 s |
| 100 k, $K=10$, grille $48^3$, régularisation locale | 1,385 s |
| 10 M, $K=10$, régularisation locale | 159,2 s |
| 30 M, $K=10$, régularisation locale | 962,9 s |
| 30 M, $K=10$, $\kappa=1$ identité | 33,6 s |

Le passage de 100 k à 50 k ne divisera pas nécessairement le temps par deux : l'initialisation, l'index et les coûts fixes dominent déjà une part importante du petit run.

Un défaut de routage est particulièrement important. Pour $n=50\,000$,

$$
\lfloor\sqrt n\rfloor=223.
$$

Le défaut `candidate_k_max=1023` exige un garde au rang 1024 et force donc le chemin cuVS brute force au lieu de RBC. Pour utiliser RBC, il faut actuellement `candidate_k_max <= 222`, sans fallback seulement pour les requêtes incertaines. Un routage hybride manque : RBC d'abord, brute force uniquement sur les requêtes non certifiées.

L'objectif sous la seconde n'est donc pas démontré avec la configuration actuelle, et il exclut encore l'affectation complète des observations.

### 11.2 Pourquoi le MST de puissance est plus adapté

Pour $50\,000$ points :

- une liste de 64 voisins représente environ 3,2 millions d'arêtes orientées ;
- le calcul de $q_i$, $(z_i,a_i)$ et $\beta_{ij}$ est entièrement fusionnable ;
- un MSF GPU sur quelques millions d'arêtes est une opération standard ;
- aucun tableau de cellules ni scan de centres de grille n'est nécessaire ;
- les labels à une coupe se calculent directement à partir des composantes de l'arbre et d'une requête de puissance.

Des travaux GPU calculent un EMST exact de 37 millions de points 3D en moins de 0,5 s sur A100. Cela ne prouve pas le même débit pour le MST de puissance pondéré, mais démontre que le cas $K=1$, 50 k/3D, ne justifie absolument pas une grille.

#### Budget temporel proposé après préchauffage

Le prototype doit viser :

| Étape | Budget indicatif |
|---|---:|
| m-NN 3D, $m=64$ | 100--200 ms |
| Gibbs/moments pour $K=10$ | 50--100 ms |
| graphe candidat de puissance | 100--200 ms |
| $\beta$ + MSF GPU | 100--200 ms |
| affectation et coupe | 100--200 ms |
| marge d'orchestration | 100 ms |

Ces valeurs sont des objectifs d'ingénierie, pas des mesures. Le benchmark doit inclure transferts, affectation et synchronisation. La compilation JIT doit être exclue du temps chaud mais publiée séparément.

#### Choix recommandé

- Pour une réponse stricte HGP : `power_multicover` en mode rapide, avec complétude probablement inconnue sur certains nuages.
- Pour garantir une réponse pratique sous la seconde : `entropic_witnessed_dtm` + MST de puissance.
- Pour un oracle 3D : triangulation régulière CPU sur les sites pondérés, utilisée dans les tests de petite/moyenne taille.

---

## 12. Cas de la vraie grande dimension

Le terme « grande dimension » doit être séparé en deux régimes.

### 12.1 Grande dimension ambiante, faible dimension intrinsèque

Si les données vivent sur une variété ou un ensemble de faible dimension de doublement :

- ANN et farthest-point sampling restent efficaces ;
- une couverture des centres critiques peut être réaliste ;
- les résultats de sparse higher-order Čech donnent un modèle théorique ;
- le backend HGP progressif peut fonctionner avec des certificats de marge.

### 12.2 Grande dimension intrinsèque

Si les distances se concentrent et la dimension de doublement est élevée :

- les gaps de rang diminuent ;
- une température entropique utile rend la queue plus dense ;
- un degré ANN fixe n'assure ni connexité ni présence du MST ;
- les constantes des sparsifications higher-order deviennent exponentielles ;
- les témoins nécessaires à une couverture globale deviennent trop nombreux.

Dans ce régime, aucune méthode ne peut promettre simultanément : exactitude HGP continue, $K=10$, coût quasi linéaire et absence d'hypothèse. Le backend DTM témoigné est alors le compromis le plus défendable.

Une étape optionnelle de réduction de dimension ne doit être utilisée que si elle fournit une garantie sur les distances pertinentes, par exemple une projection de Johnson--Lindenstrauss sur l'ensemble fini des sites et candidats. Une réduction UMAP n'offre pas une telle garantie topologique.

---

## 13. Plan d'implémentation proposé

### PR 1 — Oracle mathématique du MST de puissance

- nouvelle structure `PowerSite(center, variance)` ;
- formule robuste de $\beta_{ij}$ ;
- naissances de sommets ;
- graphe complet CPU pour petit $n$ ;
- merge tree avec événements multiway ;
- test $a_i=0\Rightarrow\beta_{ij}=\lVert x_i-x_j\rVert/2$ ;
- comparaison exacte au Single-Linkage du manuscrit pour $K=1$.

### PR 2 — Atomes $K$ entropiques à cardinal fixé

- support $m$-NN incluant canoniquement l'observation ;
- DP préfixe/suffixe pour $e_k$ et marginales ;
- cap implicite $q_{ij}\le1/K$ ;
- limite $\tau\downarrow0$ vers l'uniforme sur les $K$-NN ;
- température choisie par budget de distorsion ;
- borne de transport/TV archivée ;
- kernel CUDA fusionné pour $K\le10$.

### PR 3 — MST de puissance GPU

- graphe candidat union de plusieurs oracles ;
- kernel $\beta$ ;
- RAFT/cuGraph MST ou Borůvka dédié ;
- expansion adaptative par coupe ;
- fallback exhaustif seulement sur les composantes non certifiées ;
- oracle triangulation régulière en 3D ;
- exactness report par phase.

### PR 4 — Backend `entropic_witnessed_dtm`

- construction pour $k=1,\ldots,K$ ;
- warm starts sans hypothèse d'imbrication ;
- merge tree et sauvegarde ;
- affectation de puissance GPU ;
- extraction plate séparée ;
- comparaison DTM exacte / witnessed / entropique sur petit $n$.

### PR 5 — Réécriture du générateur Rank-Gabriel

- noyau log-domain des libres énergies $F_0,\ldots,F_{K+1}$ ;
- marginales de shell ;
- rank-shell mean-shift amorti ;
- Hessian-vector products ;
- classification des critiques ;
- atlas avec gaps et rayons de couverture ;
- aucune règle arbitraire « conserver les 1000 premiers ».

### PR 6 — Cofaces et $K$-MSF indépendants de la dimension

- miniballe additive active-set pour $p$ quelconque ;
- filtres par bornes pairwise $\beta$ ;
- test Gabriel global avec garde d'omission ;
- événements multiway ;
- naissances de faces ;
- rapport séparant validité, MSF candidat et complétude globale.

### PR 7 — Benchmarks

- 3D : 10 k, 50 k, 100 k ;
- haute dimension : $p\in\{32,128,512,2048\}$ ;
- faible dimension intrinsèque plongée dans grande dimension ;
- chaînes de bruit, corridors de densité et ex æquo ;
- SemanticKITTI, sous-nuages de 50 k ;
- HDBSCAN, HGP-CGAL, PowerCover3D et les deux nouveaux backends ;
- temps froid/chaud, VRAM, RSS, cut recall, merge distortion et qualité de coupe non supervisée.

---

## 14. Critères d'acceptation

### Exactitude mathématique

1. À $K=1$, $\tau=0$, égalité exacte avec l'EMST, rayon divisé par deux.
2. Avec sites pondérés, égalité du MST candidat certifié avec le graphe complet sur petits nuages.
3. Limite entropique vers l'atome dur lorsque $\tau\downarrow0$.
4. Respect de $q_j\le1/K$ et $\sum q_j=1$.
5. Événements ex æquo groupés au même rayon.
6. Invariance par permutation et isométrie ; équivariance par homothétie.
7. Aucune complétude HGP annoncée sans certificat de couverture.

### Exactitude numérique

1. Génération float32, décisions critiques float64 ou intervalles dirigés.
2. Gaps de rang, de Gabriel et de coupe enregistrés.
3. Audit ANN portant sur les minima sortants Borůvka, pas seulement sur `recall@K`.
4. Fallback ciblé, jamais global et silencieux.

### Performance 3D

1. $n=50\,000$, $K=10$, temps chaud médian inférieur à 1 s.
2. Le temps inclut la hiérarchie et l'affectation à une coupe prédéfinie.
3. p95 publié sur au moins 100 répétitions sans recompilation.
4. Aucun scan de grille.

### Qualité

1. Pas de « meilleure coupe oracle » comme score de production.
2. Courbes de persistance et merge distortion sur petits sous-échantillons exacts.
3. Comparaison à HDBSCAN et HGP-CGAL à paramètres et métriques identiques.
4. Ablation : sans entropie, entropie indépendante, cardinalité fixée, éventuel Sinkhorn non équilibré.

### Statistique

$K=10$ fixé est un choix de compromis finite-sample, pas une garantie de consistance universelle. Toute affirmation asymptotique doit spécifier des suites $K_n$, $\tau_n$, $m_n$ telles que

$$
K_n\to\infty,
\qquad
\frac{K_n}{n}\to0,
\qquad
\text{et}\qquad
\text{biais entropique}\to0.
$$

Pour le backend HGP régularisé, une condition suffisante naturelle est en plus

$$
s_n=\max_iW_2(\delta_{x_i},\nu_i)\to0,
\qquad
s_n=o(r_{K_n}),
$$

de sorte que la perturbation des niveaux soit asymptotiquement négligeable. Les théorèmes précis doivent encore fixer les hypothèses de densité, de séparation et de dimension intrinsèque.

---

## 15. Comparaison finale avec le `PERG-HGP` actuel

| Critère | `PowerCover3D` actuel | `PERGHGPClusterer` historique | Proposition `power_multicover` | Proposition `entropic_witnessed_dtm` |
|---|---|---|---|---|
| Champ | $K$-ième distance de puissance | Rang mou + cofaces témoins | $K$-ième distance de puissance | Minimum de puissance d'atomes $K$ |
| Fidélité HGP | Oui pour le champ, pas pour le complexe | Visée HGP, atlas incomplet | Oui, conditionnelle à la complétude | Non, surrogate DTM explicite |
| Higher-order | Implicite par $K$-couverture | Explicite mais heuristique | Explicite, candidat mou/décision dure | À l'intérieur des atomes |
| $K=1$ | Grille du champ | Graphe 1-NN insuffisant | Vrai MST de puissance | Vrai MST de puissance |
| Topologie | MSF cubique inner/outer | $K$-MSF du graphe découvert | $K$-MSF + naissances de faces | MST de puissance |
| Dimension | 3 seulement | 3 en pratique | $p$ quelconque, coût dépendant de l'atlas | $p$ quelconque |
| Grille | Oui | Oui dans plusieurs prédicats | Non | Non |
| Garantie | Entrelacement cubique conditionnel | Validité partielle, complétude inconnue | Graduée, exacte si certificats complets | Exacte pour le champ témoin ; borne vers DTM |
| 50 k / <1 s | Non démontré | Non crédible actuellement | Possible sur données favorables | Objectif crédible |
| Qualité démontrée | Actuellement faible sur petits synthétiques | Pas de benchmark actuel | À démontrer | À démontrer |

---

## 16. Conclusion

L'idée « entropie = passage d'un objet combinatoire à un calcul GPU » est correcte pour la **sélection de rang**, mais incomplète pour la **topologie globale**.

La stratégie mathématiquement la plus fidèle est :

$$
\boxed{
\text{atomes entropiques}
\to
\text{sites de puissance}
\to
\text{rang dur}
\to
\text{atlas progressif}
\to
\text{cofaces certifiées}
\to
K\text{-MSF}.
}
$$

Son cas $K=1$ doit être corrigé immédiatement : la structure n'est ni une grille ni un graphe 1-NN, mais un MST de collisions de boules de puissance avec naissances de sommets.

Pour $K>1$, la libre énergie sur les $K$-sous-ensembles et le rank-shell mean-shift constituent un meilleur générateur de points critiques que le pool actuel. Ils donnent des bornes de biais, des marges de rang et une continuation naturelle $1\to K$. Les décisions finales doivent toutefois rester dures et accompagnées de certificats de couverture et de coupe.

Si la priorité est une méthode qui fonctionne réellement en grande dimension avec $K=10$, la meilleure décision est de déplacer $K$ dans un atome de distance-à-la-mesure témoigné, puis de revenir à un minimum externe. On perd l'identité exacte avec HGP-Clusterer, mais on gagne un champ stable, une taille linéaire, un vrai MST et un pipeline GPU réaliste.

Le backend actuel `PowerCover3D` doit donc être conservé comme référence 3D à entrelacement contrôlé et comme source de kernels/diagnostics. Il ne doit plus être l'architecture principale. Les propres benchmarks du dépôt montrent que la grille est désormais l'erreur dominante.

---

## Références

### Manuscrit et dépôt

- Louis Hauseux, *Structuration automatique de données : clustering hiérarchique, percolation et interactions d'ordre supérieur*, parties I et II, version du 14 juin 2026.
- [`PERG-HGP/README.md`](https://github.com/Ludwig-H/E-HGP/blob/d67647f5bc6ad7ce76e259541a5b77d116359555/perg_hgp/README.md).
- [`POWER_COVER_3D.md`](https://github.com/Ludwig-H/E-HGP/blob/d67647f5bc6ad7ce76e259541a5b77d116359555/perg_hgp/POWER_COVER_3D.md).
- [`G4_20260712_REPORT.md`](https://github.com/Ludwig-H/E-HGP/blob/d67647f5bc6ad7ce76e259541a5b77d116359555/perg_hgp/benchmarks/results/G4_20260712_REPORT.md).
- [`algorithme_reg_knn_hgp_certifie_v2.md`](https://github.com/Ludwig-H/E-HGP/blob/d67647f5bc6ad7ce76e259541a5b77d116359555/algorithme_reg_knn_hgp_certifie_v2.md).

### Distance à une mesure et distances de puissance

- F. Chazal, D. Cohen-Steiner, Q. Mérigot, [*Geometric Inference for Probability Measures*](https://doi.org/10.1007/s10208-011-9098-0), *Foundations of Computational Mathematics*, 2011. Formulation OT de la DTM, 1-lipschitzianité et stabilité $m^{-1/2}W_2$.
- L. Guibas, Q. Mérigot, D. Morozov, [*Witnessed k-Distance*](https://doi.org/10.1007/s00454-012-9465-x), *Discrete & Computational Geometry*, 2013. Représentation linéaire par barycentres témoins et distance de puissance.
- C. Brécheteau, C. Levrard, [*A k-points-based distance for robust geometric inference*](https://doi.org/10.3150/20-BEJ1214), *Bernoulli*, 2020. Coreset de distance de puissance et stabilité.

### Čech d'ordre supérieur et multicouvertures

- M. Buchet, B. B. Dornelas, M. Kerber, [*Sparse Higher Order Čech Filtrations*](https://doi.org/10.1145/3666085), *Journal of the ACM*, 2024 ; [version SoCG/arXiv](https://arxiv.org/abs/2303.06666).
- R. Corbet et al., [*Computing the Multicover Bifiltration*](https://doi.org/10.1007/s00454-022-00476-8), *Discrete & Computational Geometry*, 2023.
- H. Edelsbrunner, G. Osang, [*A Simple Algorithm for Higher-Order Delaunay Mosaics and Alpha Shapes*](https://doi.org/10.1007/s00453-022-01027-6), *Algorithmica*, 2023.

### Rang entropique et transport optimal

- M. Cuturi, [*Sinkhorn Distances*](https://arxiv.org/abs/1306.0895), NeurIPS 2013.
- M. Cuturi, O. Teboul, J.-P. Vert, [*Differentiable Ranking and Sorting using Optimal Transport*](https://proceedings.neurips.cc/paper/2019/hash/d8c24ca8f23c562a5600876ca2a550ce-Abstract.html), NeurIPS 2019.
- Y. Xie et al., [*Differentiable Top-k with Optimal Transport*](https://proceedings.neurips.cc/paper/2020/hash/ec24a54d62ce57ba93a531b460fa8d18-Abstract.html), NeurIPS 2020.
- M. Blondel et al., [*Fast Differentiable Sorting and Ranking*](https://proceedings.mlr.press/v119/blondel20a.html), ICML 2020.
- B. Schmitzer, [*Stabilized Sparse Scaling Algorithms for Entropy Regularized Transport Problems*](https://doi.org/10.1137/16M1106018), *SIAM Journal on Scientific Computing*, 2019.

### Merge trees, clustering et UMAP

- D. Morozov, K. Beketayev, G. Weber, [*Interleaving Distance between Merge Trees*](https://www.mrzv.org/publications/interleaving-distance-merge-trees/), stabilité par perturbation uniforme.
- J. Eldridge, M. Belkin, Y. Wang, [*Beyond Hartigan Consistency: Merge Distortion Metric for Hierarchical Clustering*](https://proceedings.mlr.press/v40/Eldridge15.html), COLT 2015.
- L. McInnes, J. Healy, J. Melville, [*UMAP: Uniform Manifold Approximation and Projection for Dimension Reduction*](https://arxiv.org/abs/1802.03426), 2018 ; logiciel [JOSS](https://doi.org/10.21105/joss.00861).

### GPU, voisinage et MST

- A. Prokopenko, P. Sao, D. Lebrun-Grandié, [*A Single-Tree Algorithm to Compute the Euclidean Minimum Spanning Tree on GPUs*](https://doi.org/10.1145/3545008.3546185), ICPP 2022 ; [arXiv](https://arxiv.org/abs/2207.00514).
- J. Johnson, M. Douze, H. Jégou, [*Billion-scale similarity search with GPUs*](https://arxiv.org/abs/1702.08734), 2017.
- H. Ootomo et al., [*CAGRA: Highly Parallel Graph Construction and Approximate Nearest Neighbor Search for GPUs*](https://doi.org/10.1109/ICDE60146.2024.00323), ICDE 2024.
- [Documentation RAFT : Minimum Spanning Tree](https://docs.rapids.ai/api/raft/stable/cpp_api/).
