# Catalogue critique 3D par raffinement de Voronoï restreint

> **Résultat principal.** Pour $K_{\max}=10$, le calcul de $H_0$ ne requiert que les sphères bien centrées de rang fermé au plus onze. Elles peuvent être énumérées par une suite de raffinements de cellules top-$m$ au moyen d'une primitive de diagramme de puissance GPU, puis certifiées par séparation exacte aux sommets. Aucune mosaïque de Delaunay d'ordre supérieur ni aucun rhomboid tiling n'est matérialisé.

Cette borne porte sur les événements de Morse qui peuvent changer immédiatement $H_0$. Elle ne borne pas les générateurs de la [tour globale de boules saturées](TOUR_BOULES_SATUREES.md), laquelle encode aussi les incidences latentes de Čech et peut exiger des saturés de rang jusqu'à $n$. Les deux constructions sont complémentaires et aucune ne réfute la borne de l'autre.

## 1. Caractérisation locale

Soit une sphère $(c,a)$ de centre $c$ et de rayon carré $a$. Notons

$$I=X\cap B^{\circ}(c,\sqrt{a}),\qquad U=X\cap\partial B(c,\sqrt{a}),\qquad s=\lvert I\rvert+\lvert U\rvert.$$

Sous position générale, Reani–Bobrowski caractérisent les points critiques de $D_k$ par

$$c\in\mathrm{relint}\,\mathrm{conv}(U),\qquad \lvert I\rvert<k\leq s,$$

et, dans cette fenêtre, leur indice par

$$\mu=s-k.$$

La condition d'ordre équivaut à $0\leq\mu\leq\lvert U\rvert-1$. Hors de cette fenêtre, la boule peut encore porter des incidences descendantes de Čech, mais elle n'est pas un événement critique de $D_k$ à ce niveau.

Leur multiplicité locale d'indice $\mu$ vaut

$$\Delta_{\mu}(c)=\binom{\lvert U\rvert-1}{\mu}.$$

À l'indice un, les $\lvert U\rvert$ germes stricts sont portés par $F_u=(I\cup U)\setminus\lbrace u\rbrace$, $u\in U$. Un même centre peut donc provoquer une multifusion globale et tuer jusqu'à $\lvert U\rvert-1$ classes de $H_0$; le nombre effectif dépend des attaches globales des bras. Le catalogue ne binarise jamais cet événement.

En dimension trois, $U$ est affinement indépendant et contient au plus quatre points. Le centre est alors déterminé par $U$ dans son enveloppe affine :

| taille de $U$ | centre candidat | test de bon centrage |
|---:|---|---|
| 1 | l'observation | cas de rayon nul ou multiplicité |
| 2 | milieu du segment | toujours dans l'intérieur relatif si les points diffèrent |
| 3 | centre du cercle circonscrit dans le plan | trois coordonnées barycentriques strictement positives |
| 4 | centre de la sphère circonscrite | quatre coordonnées barycentriques strictement positives |

Un triangle obtus et un tétraèdre dont le circumcentre sort de l'enveloppe convexe ne définissent pas un nouveau support critique de cette taille; leur miniball possède un support strictement plus petit.

Pour des points distincts, les $n$ supports de taille un sont injectés directement : centre $x_i$, rayon nul et rang fermé un. Ils forment les feuilles de $T_1$ et ne passent pas par le raffinement. Avec multiplicités, ce cas est remplacé par la règle de rang pondéré.

## 2. Partage des événements entre ordres

La sphère $(c,a)$ n'est utile à $H_0$ que pour :

- $k=s$, où $\mu=0$ et une composante naît;
- $k=s-1$, où $\mu=1$ et les $\lvert U\rvert$ bras locaux peuvent fusionner globalement.

Posons $K_{\mathrm{eff}}=\min(K_{\max},n)$ et $s_{\max}=\min(K_{\mathrm{eff}}+1,n)$. Pour $1\leq k\leq K_{\mathrm{eff}}$, il suffit donc de produire

$$1\leq s\leq s_{\max}.$$

Avec $K_{\max}=10$, le compteur intérieur vérifie

$$\lvert I\rvert\leq11-\lvert U\rvert\leq9$$

pour les supports non triviaux de taille au moins deux. Un seul catalogue alimente les dix tranches; il ne faut pas lancer dix recherches indépendantes.

## 3. Équivalence exacte avec Gabriel

Considérons un événement de rang $s=k+1$ et posons $S=I\cup U$. La boule $(c,a)$ contient $S$, aucun point extérieur à $S$ dans son intérieur et, sous position générale, aucun point extérieur sur sa frontière. Comme $c$ appartient à l'intérieur relatif de $\mathrm{conv}(U)$, $U$ est le support minimal de la miniball de $S$. Ainsi

$$c=c_S,\qquad a=\beta(S),\qquad B^{\circ}(c_S,\sqrt{\beta(S)})\cap(X\setminus S)=\varnothing.$$

Donc $S$ est un $k$-simplexe de Gabriel.

Réciproquement, soit $S$ un $k$-simplexe de Gabriel. Le centre de sa miniball appartient à l'enveloppe convexe de son support minimal. Sous position générale, ce support est affinement indépendant, aucun point extérieur n'est sur la frontière et l'intérieur ne contient aucun point extérieur par définition. La sphère a donc le rang fermé $\lvert S\rvert=k+1$ et satisfait le critère critique.

Cette équivalence montre que le catalogue de rang $k+1$ est exactement le catalogue des hyperarêtes nécessaires au K-graphe de Gabriel.

## 4. Pourquoi les voisinages locaux fixes échouent

Il n'existe pas de constante $L$ telle que toutes les paires critiques de rang deux soient présentes dans les listes $L$-NN de leurs extrémités.

Prenons deux points $u,v$ dont la boule diamétrale est vide. Ajoutons arbitrairement beaucoup de points très près de $u$, mais juste à l'extérieur de cette boule. La paire reste de Gabriel et son milieu reste critique, tandis que $v$ quitte toute liste de longueur fixée autour de $u$.

Une stratégie ANN, UMAP, CAGRA ou liste locale peut donc proposer des supports, mais jamais certifier seule la complétude. Le mécanisme exact doit posséder un oracle global de violation.

## 5. Cellules top-$m$

Pour $I\subseteq X$, $\lvert I\rvert=m$, la cellule top-$m$ est

$$C_m(I)=\left\lbrace y:\max_{i\in I}\left\Vert y-i\right\Vert^2\leq\min_{x\in X\setminus I}\left\Vert y-x\right\Vert^2\right\rbrace.$$

Elle est l'ensemble des points dont $I$ est un choix des $m$ plus proches voisins. C'est un polyèdre convexe, car chaque comparaison de deux distances carrées est affine en $y$.

Le niveau $m=0$ possède l'unique cellule $C_0(\varnothing)=\mathbb{R}^{3}$. Pour le calcul, on la coupe par une boîte dyadique tridimensionnelle $\Omega$ strictement paddée, de sorte que

$$c\in\mathrm{conv}(U)\subseteq\mathrm{conv}(X)\subset\mathrm{int}(\Omega).$$

Le padding est positif sur les trois axes, même si $X$ est colinéaire ou coplanaire, et sa valeur exacte est enregistrée. Si le padding ne peut pas être représenté par le chemin numérique rapide, il est conservé par le chemin exact ou la construction échoue explicitement. Les faces héritées de $\partial\Omega$ sont marquées artificielles et ne créent jamais d'événement. Utiliser l'AABB non paddée et rejeter ses incidences perdrait des événements légitimes situés sur le bord de l'enveloppe des données.

## 6. Raffinement ancré

Fixons une cellule parente $C_m(I)$ et un point $u\notin I$. Le morceau ancré de $u$ est

$$P(I,u)=C_m(I)\cap\bigcap_{v\in X\setminus I}\left\lbrace y:\left\Vert y-u\right\Vert^2\leq\left\Vert y-v\right\Vert^2\right\rbrace.$$

Il s'agit de la cellule de Voronoï ordinaire de $u$ parmi $X\setminus I$, restreinte au parent. Un diagramme de Voronoï ordinaire est un diagramme de puissance dont tous les poids sont nuls; la même primitive GPU de clipping sert donc aux diagrammes ordinaires et pondérés.

> **Proposition D.1 — identité de raffinement.** Pour tout $Q\subseteq X$, $\lvert Q\rvert=m+1$,
>
> $$C_{m+1}(Q)=\bigcup_{u\in Q}P(Q\setminus\lbrace u\rbrace,u).$$

**Preuve.** Soit $y\in C_{m+1}(Q)$. Choisissons dans $Q$ un point $u$ dont la distance à $y$ est maximale, avec toutes les égalités conservées. Les $m$ points de $I=Q\setminus\lbrace u\rbrace$ ne sont pas plus éloignés que $u$, et aucun point extérieur à $Q$ n'est plus proche que $u$. Ainsi $y\in C_m(I)$ et $u$ est un plus proche voisin parmi $X\setminus I$, donc $y\in P(I,u)$.

Réciproquement, si $y\in P(I,u)$, les points de $I$ sont un choix top-$m$ et $u$ est le meilleur point restant. Par conséquent $I\cup\lbrace u\rbrace$ est un choix top-$(m+1)$ et $y\in C_{m+1}(I\cup\lbrace u\rbrace)$. $\square$

Cette identité est l'adaptation recherchée de l'algorithme incrémental des mosaïques : les cellules de l'ordre suivant sont engendrées par des diagrammes de puissance restreints indépendants, puis réunies par label. La structure globale de la mosaïque n'est jamais stockée.

> **Proposition D.3 — reconstruction canonique d'un enfant.** Après découverte et déduplication d'un label $Q$, $p=\lvert Q\rvert$, la cellule certifiée est reconstruite comme
>
> $$C_p(Q)\cap\Omega=\Omega\cap\bigcap_{q\in Q,\ v\notin Q}\left\lbrace y:\left\Vert y-q\right\Vert^2\leq\left\Vert y-v\right\Vert^2\right\rbrace.$$

Le clipping provisoire ne reçoit que des contraintes croisées $Q\times(X\setminus Q)$ sûres héritées des fragments. Toute comparaison interne à $Q$ et toute face de raccord entre fragments est exclue. À un sommet provisoire $z$, calculons exactement les ensembles complets

$$Q_{\max}(z)=\arg\max_{q\in Q}\left\Vert z-q\right\Vert^2,\qquad V_{\min}(z)=\arg\min_{v\notin Q}\left\Vert z-v\right\Vert^2.$$

Notons $d_{\max}(z)$ et $d_{\min}(z)$ les valeurs correspondantes. Si $d_{\max}(z)<d_{\min}(z)$, toutes les contraintes croisées sont strictes. Si $d_{\max}(z)=d_{\min}(z)$, toutes les paires de $Q_{\max}(z)\times V_{\min}(z)$ sont ajoutées ou enregistrées comme incidences actives, même si elles ne changent pas le polytope. Si $d_{\max}(z)>d_{\min}(z)$, toutes ces paires sont violatrices, elles sont insérées et le clipping reprend. Chaque différence de distances carrées est affine; le lemme de séparation aux sommets prouve donc la cellule lorsque tous ses sommets passent le test, que toutes les égalités actives sont réconciliées et que la file est vide.

La v1 certifiée applique D.3 à chaque enfant avant de l'utiliser comme parent. Les morceaux $P(I,u)$ servent à découvrir les labels non vides et à extraire les strates naturelles dans le diagramme restreint du parent canonique; leurs raccords ne sont jamais propagés comme faces de l'enfant. Conserver à la place un complexe de fragments avec coutures est une optimisation future : une strate naturelle peut croiser une couture, et la preuve d'absence de perte ou de double compte reste une `proof_obligation`.

> **Corollaire D.4 — induction de complétude des labels et cellules.** La base est l'unique parent canonique $C_0(\varnothing)\cap\Omega=\Omega$. Supposons qu'à une profondeur $m<m_{\star}$ toutes les cellules non vides $C_m(I)\cap\Omega$ aient été énumérées et fermées, et que chaque diagramme de Voronoï restreint à ces parents, initialisé par au moins un site de $X\setminus I$, ait été fermé par D.2 avec tous ses co-minimiseurs. Alors D.1 et D.2 émettent exactement les labels $Q$, $\lvert Q\rvert=m+1$, tels que $C_{m+1}(Q)\cap\Omega\neq\varnothing$; D.3 reconstruit exactement chacune de ces cellules. Par induction, tous les parents canoniques sont complets jusqu'à $m_{\star}$.

**Preuve.** Soit $Q$ tel que $C_{m+1}(Q)\cap\Omega$ soit non vide. D.1 implique qu'au moins un morceau $P(Q\setminus\lbrace u\rbrace,u)\cap\Omega$ est non vide. Son parent $C_m(Q\setminus\lbrace u\rbrace)\cap\Omega$ est donc non vide et appartient à l'hypothèse d'induction. À partir du site initial, son diagramme provisoire possède au moins un gagnant partout dans le parent. Si le site $u$ manque encore, il bat ou co-minimise un gagnant provisoire sur un ensemble non vide. La différence affine atteint son maximum sur un sommet du polytope gagnant; D.2 révèle donc $u$ comme violateur ou co-minimiseur. Le label $Q$ est émis. Réciproquement, tout morceau émis est inclus dans $C_{m+1}(Q)\cap\Omega$ par D.1, donc aucun faux label non vide n'est créé. Après déduplication, D.3 remplace les morceaux par l'unique cellule canonique correspondante. $\square$

À la profondeur finale $m_{\star}$, D.4 ne demande aucun enfant $C_{m_{\star}+1}$ : les diagrammes restreints sont seulement fermés pour extraire toutes leurs strates naturelles. Comme tout événement de rayon positif et de rang au plus $s_{\max}$ possède au plus $m_{\star}$ points strictement intérieurs, l'induction D.4, l'extraction exhaustive des strates de supports deux à quatre et l'oracle global de shell/rang donnent alors le catalogue critique complet sous les hypothèses déclarées.

## 7. Où se trouvent les supports critiques

Soit $I$ l'ensemble des points strictement intérieurs à une sphère critique et $U$ son shell. Au centre $c$ :

- $c\in C_{\lvert I\rvert}(I)$;
- tous les points de $U$ sont co-minimiseurs parmi $X\setminus I$;
- leur intersection dans le diagramme de Voronoï restreint contient $c$;
- $c$ appartient à l'intérieur relatif de $\mathrm{conv}(U)$.

Ainsi les supports de tailles deux, trois et quatre apparaissent respectivement sur des faces de codimensions un, deux et trois du raffinement ancré. Toutes les codimensions sont nécessaires : ne regarder que les sommets perd les supports de deux ou trois points et donc des minima et selles réels.

Pour chaque strate candidate $U$ :

1. calculer le centre équidistant dans $\mathrm{aff}(U)$;
2. calculer ses coordonnées barycentriques exactes;
3. rejeter si un signe n'est pas strictement positif dans le domaine de position générale;
4. interroger l'oracle de rang global et terminer le shell complet avant tout filtre de rang;
5. si $U$ est un support minimal pertinent mais que le shell contient un point extérieur au simplexe $I\cup U$, mettre `relevant_gp_complete=false` et signaler `unsupported_degeneracy`;
6. vérifier que le shell complet est exactement $U$;
7. accepter l'événement si le rang fermé est au plus $s_{\max}$;
8. dédupliquer par $(I,U)$ et témoins exacts.

## 8. Fermeture exacte d'un diagramme restreint

Un moteur GPU ne doit pas commencer avec les $n-m$ contraintes. Il construit un diagramme provisoire sur un petit ensemble non vide $S$ de sites extérieurs, choisi par l'héritage du parent, un index spatial ou une heuristique. Si ces propositions sont vides, le plus petit identifiant de $X\setminus I$ fournit un germe canonique; cet ensemble est non vide aux profondeurs calculées. La fermeture révèle ensuite les sites manquants.

Pour deux sites $u,v$, posons

$$h_{v,u}(y)=\left\Vert y-u\right\Vert^2-\left\Vert y-v\right\Vert^2.$$

La fonction $h_{v,u}$ est affine. Une cellule provisoire bornée $P_S(I,u)$ est exacte relativement à tous les sites si $h_{v,u}(y)\leq0$ pour tout $v\notin I$ et tout $y\in P_S(I,u)$.

> **Lemme D.2 — séparation aux sommets.** Si chaque sommet $z$ de $P_S(I,u)$ vérifie que $u$ est un plus proche voisin global dans $X\setminus I$, avec le shell complet enregistré, alors $P_S(I,u)=P(I,u)$.

**Preuve.** Pour tout $v$, le maximum de l'affine $h_{v,u}$ sur le polytope borné $P_S(I,u)$ est atteint à un sommet. Les inégalités vérifiées sur tous les sommets valent donc partout. L'inclusion provisoire dans la cellule globale est acquise; l'inclusion inverse résulte du fait que le provisoire utilise un sous-ensemble des contraintes globales. $\square$

Si un sommet révèle un gagnant $v$ absent, ce site est ajouté à la primitive et la cellule est reclippée. Si plusieurs sites sont exactement co-minimiseurs, ils sont tous ajoutés afin de préserver les incidences. Comme il existe un nombre fini de sites, la génération de colonnes termine après un nombre fini d'insertions; cette borne combinatoire n'est pas une borne pratique faible.

La certification d'un ordre exige :

- fermeture de chaque morceau créé;
- au moins un site initial par diagramme restreint;
- déduplication complète des labels enfants;
- file globale de nouvelles colonnes vide;
- découverte complète des labels enfants non vides;
- reconstruction canonique D.3 de chaque enfant avant propagation;
- aucune comparaison interne au label ni face de raccord héritée dans la cellule canonique;
- incidences naturelles réconciliées entre cellules canoniques;
- aucun overflow tronqué;
- tous les prédicats ambigus résolus.

## 9. Deux formulations de puissance

La formulation ancrée est recommandée pour la production, mais une seconde formulation fournit une référence indépendante. Pour $Q$, $\lvert Q\rvert=k$, posons

$$\mu_Q=\frac{1}{k}\sum_{x\in Q}x,\qquad\omega_Q=\left\Vert\mu_Q\right\Vert^2-\frac{1}{k}\sum_{x\in Q}\left\Vert x\right\Vert^2.$$

Alors

$$\frac{1}{k}\sum_{x\in Q}\left\Vert y-x\right\Vert^2=\left\Vert y-\mu_Q\right\Vert^2-\omega_Q.$$

Minimiser cette quantité sur tous les labels $Q$ produit exactement les cellules top-$k$. Chaque ordre est donc aussi un diagramme de puissance sur les sites barycentriques pondérés $(\mu_Q,\omega_Q)$.

Cette forme est commode pour un oracle CPU et pour vérifier la génération de labels. Elle expose toutefois un nombre combinatoire de sites implicites et des barycentres susceptibles de compliquer les prédicats. La production privilégie donc les points originaux et les raffinements ancrés; la cascade barycentrique reste une implémentation de référence, pas le chemin critique.

## 10. Prédicats sans division

Pour comparer deux labels $Q,R$ de même cardinal, stockons

$$S_Q=\sum_{x\in Q}x,\qquad N_Q=\sum_{x\in Q}\left\Vert x\right\Vert^2.$$

Le signe de leur différence de coût est celui de la forme affine

$$H_{R,Q}(y)=-2\langle y,S_R-S_Q\rangle+(N_R-N_Q).$$

Cette écriture évite les divisions par $k$ et les coordonnées explicites des barycentres dans les décisions exactes.

Après élimination des puissances de deux communes aux entrées dyadiques, le centre d'un support de taille trois ou quatre est stocké par les déterminants homogènes

$$\mathrm{ExactCenter}=(C_x,C_y,C_z,D_c),\qquad D_c>0.$$

Il représente $(C_x/D_c,C_y/D_c,C_z/D_c)$ sans effectuer ces divisions. Son rayon carré est stocké sous la forme réduite

$$\mathrm{ExactLevel}=(N,D),\qquad D>0,$$

avec $a=N/D$. L'ordre de deux événements est le signe exact de $N_1D_2-N_2D_1$. Les incidences, distances et signes barycentriques sont évalués directement sur les formes homogènes après multiplication par les puissances positives nécessaires. Le signe du dénominateur et le plus grand diviseur commun rendent les deux témoins canoniques.

Un sommet provisoire est représenté par les trois plans liants qui le définissent, une approximation et une boîte d'intervalle :

```text
VertexWitness
    binding_plane_ids[3]
    approximate_xyz
    interval_box
    affine_dimension
    degeneracy_class
```

Le signe d'un quatrième plan au sommet des trois premiers est ramené à un déterminant. La cascade numérique recommandée est : estimation FP32, filtre FP64 avec borne d'erreur, expansion adaptative GPU, puis big-int ou rationnel multiprécision. Les expansions accélèrent la décision de signe, mais ne matérialisent jamais une division rationnelle exacte. Une coordonnée approchée seule ne constitue ni un témoin top-$k$, ni un centre, ni un niveau exact.

## 11. Oracle global de rang

Étant donné un centre certifié $c$ et un rayon carré certifié $a$, l'oracle doit retourner :

$$N_{<}(c,a)=\#\left\lbrace x:\left\Vert x-c\right\Vert^2<a\right\rbrace,$$

$$N_{=}(c,a)=\left\lbrace x:\left\Vert x-c\right\Vert^2=a\right\rbrace.$$

Pour un ordre maximal effectif $K_{\mathrm{eff}}$, le comptage intérieur peut s'arrêter dès que $N_{<}>K_{\mathrm{eff}}$, mais l'égalité de shell doit être complète pour tout candidat encore admissible. Un LBVH global fournit les bornes de boîtes; les élagages utilisent des intervalles arrondis vers l'extérieur. Les feuilles ambiguës sont évaluées par le même prédicat exact que le centre.

L'événement est accepté seulement si :

$$N_{<}+\lvert N_{=}\rvert\leq s_{\max},\qquad N_{=}=U.$$

Le shell complet reste obligatoire pour certifier la position générale pertinente, même si la première inégalité échoue. Pour $A\subseteq X$, notons $B_A$ sa miniball et `Proper(A)` la propriété « support minimal unique, affinement indépendant et bien centré ». Plus précisément,

$$\mathrm{RelevantGP}(X,K_{\mathrm{eff}})\Longleftrightarrow\forall A\subseteq X,\ 2\leq\lvert A\rvert\leq s_{\max},\ \bigl[\mathrm{Proper}(A)\land B_A^{\circ}\cap(X\setminus A)=\varnothing\bigr]\Longrightarrow\partial B_A\cap(X\setminus A)=\varnothing.$$

La prémisse d'intérieur vide est essentielle. Si $A$ viole cette propriété, aucun point de $X\setminus A$ n'est strictement intérieur et $X\cap B_A^{\circ}=A\cap B_A^{\circ}$. Son support propre contient au moins deux points, donc $\lvert X\cap B_A^{\circ}\rvert\leq\lvert A\rvert-2\leq m_{\star}=s_{\max}-2$. Le centre est visible sur une strate issue d'un parent dans la profondeur calculée, et le shell global y révèle le point extérieur. Cette observation ne devient un certificat que si tous les parents canoniques, supports propres pertinents et co-minimiseurs ont été énumérés. Le booléen `relevant_gp_complete` vaut faux ou inconnu tant que cette obligation n'est pas satisfaite.

Les cofaces non-Gabriel parcourues par le candidat `locate_reduced_root` peuvent avoir des intrus stricts nombreux et ne sont pas couvertes par ce certificat shallow. Chaque étape certifie séparément son support unique et essentiel, son intrus strict et la baisse de miniball; toute égalité extérieure ou ambiguïté de support bloque le diagnostic. Cette certification locale ne prouve pas que le DSU Gabriel brut contient toutes les incidences de Gamma et ne peut donc pas autoriser `exact` dans le contrat v2.

En mode avec multiplicités, ces cardinalités deviennent des sommes de poids et le shell conserve les identifiants de sites avec leurs multiplicités.

## 12. Algorithme certifiant

Le lemme d'arrêt pour $H_0$ fixe la profondeur. Un événement utile vérifie $s\leq s_{\max}$ et, hors rayon nul, $\lvert U\rvert\geq2$; par conséquent

$$m=\lvert I\rvert=s-\lvert U\rvert\leq m_{\star}=s_{\max}-2\leq K_{\mathrm{eff}}-1.$$

Pour $s_{\max}\geq2$, il suffit de fermer les cellules parentes $C_0$ à $C_{m_{\star}}$. À la profondeur finale, les morceaux $P(I,u)$ sont construits et fermés pour extraire les strates, mais ils ne sont ni réunis en cellules enfants ni propagés. Si $n\geq K_{\mathrm{eff}}+1$, alors $m_{\star}=K_{\mathrm{eff}}-1$; pour $n>10$ et $K_{\mathrm{eff}}=10$, cela donne $m_{\star}=9$. Au bord $n=K_{\mathrm{eff}}\geq2$, on a plutôt $m_{\star}=n-2$, car aucun événement de rang $n+1$ ne peut exister. Les facettes de cardinal $K_{\mathrm{eff}}$ et les ensembles fermés de cardinal $s_{\max}$ restent des objets hiérarchiques : il est inutile de fermer une cellule d'intérieur de taille supérieure à $m_{\star}$. Si $s_{\max}=1$, l'événement de rang un et de rayon nul est injecté directement et aucune cascade n'est lancée.

```text
entrée: X, Keff = min(Kmax, n) <= 10, smax = min(Keff + 1, n)
injecter les n événements canoniques (x_i, rayon 0, rang 1)
si smax = 1:
    mstar = null
    retourner le catalogue de rang 1 et son certificat sans cascade
mstar = smax - 2
construire une boîte Omega strictement paddée
parents[0] = {(label vide, Omega)}

pour m = 0 .. mstar:
    fragments = vide
    pour chaque parent C_m(I), par lots GPU:
        proposer les sites u dont la cellule peut couper le parent
        si la proposition est vide: injecter le plus petit u hors I
        construire les morceaux P(I,u) par clipping de puissance
        répéter:
            interroger le 1-NN exact hors I à tous les sommets
            ajouter tous les violateurs et co-minimiseurs
            reclipping des morceaux affectés
        jusqu'à fermeture locale
        extraire toutes les strates de support 2, 3 et 4
        certifier centre, relint, shell complet et rang
        mettre à jour relevant_gp_complete avant le filtre de rang
        émettre les événements nouveaux
        si m < mstar: émettre les enfants Q = I union {u}
    si m < mstar:
        trier et dédupliquer les fragments par Q
        reconstruire C_{m+1}(Q) avec les seules contraintes croisées sûres
        fermer D.3 par tous les co-farthest(Q) et co-1-NN(X hors Q) aux sommets
        enregistrer toutes les paires actives à égalité
        réconcilier les incidences réelles et certifier la fermeture globale
    checkpoint de la profondeur m

trier le catalogue par niveau exact et rang
émettre les hyperarêtes de Gabriel
```

L'oracle spatial exclut donc au plus $m_{\star}\leq K_{\mathrm{eff}}-1$ identifiants, avec égalité seulement lorsque $n\geq K_{\mathrm{eff}}+1$; cela vaut neuf pour la cible avec $n>10$. Les facettes HGP stockées atteignent $K_{\mathrm{eff}}$ identifiants et le rang fermé maximal vaut $s_{\max}$; leurs arènes séparées empêchent un décalage de taille. Le certificat indique la valeur de $m_{\star}$, les profondeurs effectivement fermées et si les enfants canoniques D.3 ainsi que `RelevantGP` sont complets.

## 13. Politique de propositions

Les sources suivantes peuvent remplir la file initiale, sans portée certifiante :

- sites gagnants aux sommets du parent;
- adjacences héritées de l'ordre précédent;
- cellules voisines dans le LBVH ou dans un hash Morton;
- prédiction DTM ou entropique;
- listes CAGRA exact-rerankées;
- petit diagramme de puissance GPU calculé en flottants;
- cache des violateurs observés dans les parents voisins.

Chaque proposition publie son taux d'acceptation. Aucune source n'a le droit de déclarer une cellule vide; seule la fermeture globale le peut.

## 14. Dégénérescences

### 14.1 Niveaux égaux

Deux événements distincts de même rayon exact sont valides et seront mis dans le même lot. Ils ne nécessitent aucune perturbation.

### 14.2 Point extérieur sur une miniball pertinente

Si un support minimal bien centré $U$ avec $\lvert I\rvert+\lvert U\rvert\leq s_{\max}$ possède un co-minimiseur $z\notin I\cup U$, alors le simplexe $I\cup U$ viole `RelevantGP`. Le candidat n'est pas seulement rejeté : le certificat fixe `relevant_gp_complete=false` et le mode exact retourne `unsupported_degeneracy`. Ce contrôle précède le rejet par rang fermé, car l'ajout de $z$ peut pousser ce rang au-delà de $s_{\max}$.

### 14.3 Shell de plus de quatre points

Un tel shell viole la position générale. Le centre peut porter plusieurs supports minimaux et plus de bras locaux. Une extension possible étudie les signes des distances sur une petite sphère directionnelle autour de $c$; elle produit un arrangement de petits cercles sur $S^2$. Il reste à prouver que les cellules de cet arrangement correspondent exactement aux germes de sous-niveau. Avant cette preuve, le mode exact retourne `unsupported_degeneracy`.

### 14.4 Points coplanaires ou colinéaires

Ils ne sont pas automatiquement interdits. Un support de taille deux ou trois vit dans son enveloppe affine et peut être parfaitement non dégénéré. Seuls l'indépendance du support choisi, les égalités supplémentaires et les signes barycentriques décident le statut.

### 14.5 Doublons

Les doublons doivent être agrégés avant l'index spatial. Le site garde une multiplicité et le rang cumule cette multiplicité. La combinatoire des choix de facettes devient implicite; la sortie doit préciser si elle restitue les occurrences ou les sites agrégés. Cette extension doit être comparée à l'oracle multiensemble avant activation du statut exact.

## 15. Complexité honnête

Notons $M_m$ le nombre de parents non vides, $P_m$ le nombre de morceaux, $V_m$ le nombre de sommets interrogés, $J_m$ le nombre de colonnes ajoutées et $C_s$ le nombre d'événements de rang $s$. Le travail pertinent est sensible à

$$\sum_m(M_m+P_m+V_m+J_m)+\sum_s C_s,$$

pas seulement à $n$ ou à la taille de la forêt finale.

Même à l'ordre un, une configuration 3D peut avoir une complexité de Delaunay quadratique. Le mode certifié doit donc posséder des garde-fous de mémoire et de volume de sortie. Lorsque l'un d'eux est atteint, le résultat devient `budget_exhausted`; les événements déjà produits restent individuellement vérifiés, mais le catalogue n'est pas complet.

Dans le régime de Poisson volumique à ordre fixé, les résultats connus prédisent un nombre moyen d'événements et de cellules linéaire en l'intensité sur un domaine compact. C'est le régime où l'objectif de débit GPU est plausible. Les surfaces, filaments, courbes des moments et cosphères doivent être mesurés séparément.

## 16. Certificat de catalogue

```text
CatalogCertificate
    k_max
    k_eff
    s_max
    m_star                       # null si s_max = 1
    bounding_domain_and_proof
    strict_padding_proved
    closed_parent_orders
    parent_count_by_order
    fragment_count_by_order
    vertex_query_count_by_order
    inserted_column_count_by_order
    overflow_count
    unresolved_predicate_count
    exact_fallback_count
    critical_count_by_rank_and_support
    child_labels_complete
    canonical_children_complete
    active_cross_incidences_complete
    relevant_gp_complete
    duplicate_policy
    degeneracy_status
    complete
```

Le booléen `complete` vaut vrai seulement si toutes les files sont vides, la boîte contient strictement $\mathrm{conv}(X)$, tous les labels enfants non vides ont été découverts, chaque enfant propagé satisfait la reconstruction canonique D.3, toutes les égalités croisées actives et incidences naturelles sont réconciliées, `relevant_gp_complete` vaut vrai, aucun overflow n'a tronqué un objet et aucun prédicat ne reste ambigu.

## 17. Comparaison avec les alternatives

| voie | avantage | raison de ne pas en faire le cœur exact |
|---|---|---|
| grille régulière | kernels simples | dépend de la résolution, limitée par la dimension et modifie la hiérarchie |
| mosaïque d'ordre supérieur complète | structure duale naturelle | explosion combinatoire et stockage global |
| rhomboid tiling | cohérence multi-ordre | structure plus riche que $H_0$ et difficile à diffuser sur GPU |
| sites barycentriques globaux | identité de puissance élégante | atlas implicite combinatoire |
| listes $L$-NN | très rapides | incomplètes sans hypothèse de croissance locale |
| DTM ou entropie | candidats stables | autre filtration, pas de certificat d'exclusion |
| raffinement ancré fermé | exact, local, parallélisable, streaming | pire cas toujours combinatoire; primitive et prédicats à implémenter |

## 18. Références

- Y. Reani et O. Bobrowski, [*Morse Theory for the k-NN Distance Function*](https://arxiv.org/abs/2403.12792), caractérisation et indice.
- L. Hauseux, [*Manuscrit de thèse*](../references/MANUSCRIT_THESE_HAUSEUX.pdf), chapitres 6 à 8.
- H. Edelsbrunner et G. Osang, [*A Simple Algorithm for Higher-order Delaunay Mosaics and Alpha Shapes*](../references/pdfs/edelsbrunner-osang-higher-order-delaunay-arxiv-2011.03617v1.pdf), construction incrémentale.
- B. Taveira et al., [*Scalable GPU Construction of 3D Voronoi and Power Diagrams*](../references/pdfs/taveira-et-al-paragram-arxiv-2605.06408v1.pdf), primitive GPU et stratégie de clipping.
- J. R. Shewchuk, [*Adaptive Precision Floating-Point Arithmetic and Fast Robust Geometric Predicates*](https://www.cs.cmu.edu/~quake/robust.html), prédicats filtrés.
