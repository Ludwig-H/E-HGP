# Hiérarchie laminaire de points issue de la tour multi-ordres

> **Phase :** 15. **Backend de référence visé :** `reference_cpu`, puis projection streamée/GPU après certification différentielle. **Profile source :** `hgp_reduced`. **Mode proposé :** `exact_relative_multi_order_laminar_point_projection_v1`. **Statut public :** `not_claimed`.
>
> **Statut mathématique.** Ce document spécifie une réduction aval déterministe de la tour horizontale et verticale des composantes exactes. Lorsque la forêt source, ses couvertures et ses applications verticales sont certifiées, la réduction peut être certifiée exacte relativement à cette source. Elle ne prouve jamais la complétude géométrique de la source, ne remplace pas la hiérarchie HGP sur les facettes et ne peut promouvoir une source partielle, conditionnelle ou surrogate.

## 1. Objet et séparation des sémantiques

Soit un nuage fini indexé

$$X=\left\lbrace x_0,\ldots,x_{n-1}\right\rbrace\subset\mathbb{R}^{3},\qquad n\geq1,$$

et soit $K_{\mathrm{eff}}=\min(K,n)$. Pour chaque ordre $k\in\left\lbrace1,\ldots,K_{\mathrm{eff}}\right\rbrace$, la source HGP fournit une forêt horizontale $T_k$. Pour $k\geq2$, ses composantes sont des composantes de facettes de cardinal $k$ et leurs unions de points peuvent se recouvrir. La famille $T_1,\ldots,T_{K_{\mathrm{eff}}}$, munie des applications verticales entre ordres adjacents, est appelée **tour multi-ordres**.

Trois objets doivent rester distincts dans le code, les certificats et les rapports :

1. la tour HGP source sur les facettes, avec ses recouvrements de points légitimes;
2. la réduction laminaire définie ici, qui force une appartenance unique des points;
3. un rendu plat obtenu par coupe de niveau ou par sélection d'excès de masse.

La réduction de points n'est pas une nouvelle preuve de Gamma, de la source normalisée, de `full_pi0`, de M.1, de la verticalité ou de la naturalité. Elle consomme ces autorités lorsqu'elles existent et enregistre leur identité. Si l'une d'elles manque, la construction échoue ou hérite explicitement de ce statut; elle ne complète jamais la source au moyen d'un MST de points, d'un graphe de voisinage, d'un Rips, d'un surrogate ou d'une plausibilité numérique.

## 2. Données source minimales

### 2.1 Forêts horizontales

À l'ordre $k$, un nœud horizontal représente une naissance ou une multifusion HGP au niveau exact $a\geq0$, exprimé en rayon carré. Les enfants d'une multifusion sont résolus simultanément. Une continuation $q_R=1$ n'ajoute pas de nœud topologique, mais elle peut porter un delta de facettes ou de points indispensable aux coupes et aux poids.

La projection exige, pour chaque ordre :

- les nœuds, parents et niveaux exacts de $T_k$;
- les continuations qui portent un delta de couverture non vide;
- l'affectation unique de chaque facette projectable à son premier carrier certifié;
- les identifiants canoniques et les digests de chaîne permettant un rejeu frais;
- les racines terminales, y compris les composantes distinctes qui ne fusionnent jamais à niveau fini.

Un lot structurel vide qui ne modifie ni topologie, ni couverture, ni verticale ne devient pas un faux nœud.

### 2.2 Applications verticales

À toute coupe fermée certifiée de niveau $a$, une composante source d'ordre $k+1$ possède une image unique d'ordre $k$ :

$$v_{k,a}:\pi_0\bigl(L_{k+1}(a)\bigr)\longrightarrow\pi_0\bigl(L_k(a)\bigr).$$

Pour $a\leq b$, les carrés de naturalité requis par la source sont

$$h_{k,a,b}\circ v_{k,a}=v_{k,b}\circ h_{k+1,a,b}.$$

La réduction n'infère pas une verticale absente depuis les unions de points. Elle consomme seulement les verticales certifiées. Deux records verticaux qui décrivent la même couture pendant un intervalle de naturalité peuvent être compactés en un record canonique, à condition que le reçu de compaction permette de reconstruire exactement les deux extrémités et le niveau d'activation maximal.

### 2.3 Flux de contributions projectables

Un manifeste qui ne contient que les niveaux Hartigan et les nombres de groupes ne suffit pas à calculer les poids de la section 6. La source doit aussi exposer un flux authentifié équivalent à

```text
(order, facet_or_carrier_id, point_ids, exact_squared_radius,
 source_event_id, source_chain_digest)
```

Ce flux peut être trié et réduit sans conserver un catalogue global. Si `exp_z` doit pouvoir changer après la construction de la forêt, les rayons atomiques exacts doivent rester rejouables. Si seuls des scores déjà agrégés sont conservés, leur exposant fait partie immuable du contrat de la forêt.

## 3. Échelle de densité commune aux ordres

### 3.1 Exposant exact

Le paramètre `exp_z` est un rationnel positif réduit

$$z=\frac{p}{q},\qquad p\in\mathbb{N}^{\star},\qquad q\in\mathbb{N}^{\star},\qquad \gcd(p,q)=1.$$

Il ne doit pas être reçu comme un `double` non accompagné. Une entrée décimale est d'abord interprétée comme un rationnel décimal exact. Une entrée binaire64 peut être acceptée seulement si le contrat indique explicitement que sa valeur dyadique exacte, et non sa représentation imprimée, est l'exposant demandé.

Pour un niveau exact $a=r^2>0$ de l'ordre $k$, la densité multi-ordres est

$$\lambda_z(k,a)=\frac{k}{n\omega_3a^{z/2}}=\frac{k}{n\omega_3r^z},\qquad \omega_3=\frac{4\pi}{3}.$$

Le facteur commun $1/(n\omega_3)$ peut être omis pour trier les événements, mais il est conservé dans les métadonnées et dans le rendu numérique de lambda. Le facteur $k$ ne peut pas être omis lors de la combinaison des ordres : il est celui de l'estimateur K-NN et empêche la tour d'être ramenée instantanément à $T_1$.

Le niveau $a=0$ donne formellement $\lambda_z(k,0)=+\infty$. Les événements de rayon nul sont traités comme un lot symbolique unique. Aucun epsilon flottant ni cap arbitraire ne remplace cet infini.

En dimension trois, le choix statistiquement dimensionné est $z=3$. Un autre exposant positif définit une autre paramétrisation aval, enregistrée dans le certificat; il ne change pas les niveaux exacts $a$ de la forêt HGP source.

### 3.2 Comparaison exacte

Pour $a_i,a_j>0$ et le même exposant $z=p/q$, l'ordre des densités se décide sans évaluer de racine :

$$\lambda_z(k_i,a_i)\geq\lambda_z(k_j,a_j)\Longleftrightarrow k_i^{2q}a_j^p\geq k_j^{2q}a_i^p.$$

Les $a_i$ étant rationnels exacts, la comparaison finale est une comparaison d'entiers après produit croisé. Elle fournit aussi l'égalité exacte des niveaux multi-ordres. Le coût binaire des grands entiers doit être mesuré séparément du nombre de comparaisons.

### 3.3 Pourquoi le seul rayon ne suffit pas

Si toutes les verticales étaient activées au niveau $r^{-z}$ indépendamment de $k$, une composante supérieure et son image dans $T_1$ seraient raccordées au même niveau. La fermeture par connexité de tous les supports serait alors dominée par les composantes de $T_1$ et perdrait précisément l'information recherchée dans les ordres supérieurs.

Avec $\lambda_z(k,a)$, une composante d'ordre $k+1$ au rayon $r$ est visible au niveau $(k+1)/(n\omega_3r^z)$ et se raccorde à son image d'ordre $k$ au niveau $k/(n\omega_3r^z)$. L'intervalle entre ces deux valeurs conserve l'information propre à l'ordre supérieur sans nier l'inclusion verticale à plus faible densité.

## 4. Graphe compact de la tour

### 4.1 Sommets et marqueurs

Le graphe compact $\mathcal{G}$ possède :

- un sommet pour chaque naissance ou multifusion normalisée de chaque $T_k$;
- un marqueur de degré deux lorsqu'une continuation ajoute une couverture, modifie une couture verticale ou change une quantité nécessaire au rendu;
- les singletons certifiés de $T_1$ selon le contrat source;
- une racine virtuelle de niveau zéro, utilisée seulement pour arbitrer globalement les racines finies et représenter le bruit.

Une facette projectable n'a pas besoin de devenir un sommet global de $\mathcal{G}$. Elle est un atome de poids attaché à son premier carrier ou marqueur. Ce choix évite de réintroduire sous un autre nom un catalogue global de facettes.

### 4.2 Arêtes horizontales et verticales

Une arête horizontale relie un carrier enfant à son carrier parent au niveau exact de l'absorption. À l'ordre $k$, pour un événement de rayon carré $a$, son niveau est $\lambda_z(k,a)$.

Une arête verticale relie un carrier d'ordre $k+1$ à son image d'ordre $k$ à la même coupe exacte. Son activation est le niveau de l'extrémité inférieure :

$$\lambda^{\mathrm{vert}}_z(k+1\rightarrow k,a)=\lambda_z(k,a).$$

Cette règle est aussi le minimum des deux densités K-NN au même rayon. Si un futur profil autorise des facteurs d'ordre non monotones dans la topologie, la règle générale devient le minimum certifié des niveaux des deux extrémités. Les poids d'ordre utilisés pour distribuer les masses, définis plus loin, ne modifient pas les niveaux topologiques.

Toutes les racines restantes sont raccordées à la racine virtuelle au niveau zéro. Cette couture ne crée aucun événement géométrique et la racine virtuelle est exclue par défaut de la sélection EOM.

### 4.3 Filtration du graphe

Pour $\ell\geq0$, soit $\mathcal{G}^{\geq\ell}$ le sous-graphe contenant exactement les sommets déjà activés et les arêtes dont le niveau est au moins $\ell$. Si $\ell_1\geq\ell_2$, alors

$$\mathcal{G}^{\geq\ell_1}\subseteq\mathcal{G}^{\geq\ell_2}.$$

Les composantes connexes ne peuvent donc que fusionner lorsque $\ell$ décroît. Leur arbre de fusion est bien défini même si $\mathcal{G}$ contient des cycles.

## 5. Construction DSU atomique

Les activations sont parcourues par lambda décroissant. Pour chaque niveau exact commun $\ell$ :

1. prendre un snapshot logique des racines DSU strictement au-dessus de $\ell$;
2. activer tous les nouveaux sommets du lot;
3. considérer simultanément toutes les arêtes horizontales et verticales du lot;
4. calculer les composantes temporaires du quotient du lot;
5. publier une seule naissance ou multifusion multifurquée par composante temporaire;
6. appliquer seulement ensuite les mutations DSU du lot.

Une séquence arbitraire d'unions binaires au sein du lot n'est jamais exposée comme topologie. Le résultat ne dépend donc ni de l'ordre des arêtes dans le flux, ni de l'ordonnancement des threads, ni du représentant DSU choisi.

On note $\mathcal{M}$ l'arbre ou la forêt de fusion ainsi obtenu après ajout de la racine virtuelle. $\mathcal{M}$ est encore un arbre de carriers et d'atomes de la tour; ce n'est pas encore une hiérarchie de points.

### Proposition 5.1 — Exactitude de la réduction DSU relativement au graphe de tour

À tout niveau critique $\ell$, les racines DSU après le lot sont en bijection avec les composantes connexes de $\mathcal{G}^{\geq\ell}$.

**Preuve.** Avant le lot, l'invariant vaut par induction. Le quotient temporaire ajoute exactement les sommets et arêtes de niveau $\ell$, sans autre incidence. Ses composantes sont donc celles de $\mathcal{G}^{\geq\ell}$ obtenues depuis les composantes de $\mathcal{G}^{>\ell}$. La publication multifurquée ne change pas la relation de connexité, puis les unions DSU réalisent exactement ce quotient. Le cas initial est vide et le cas terminal est fermé par la racine virtuelle. Cette preuve porte sur $\mathcal{G}$ fourni; elle ne certifie pas que $\mathcal{G}$ contient toutes les données HGP exigées.

## 6. Distribution conservative des poids des simplexes vers les points

### 6.1 Scores de facettes

Pour $k\geq2$, soit $\mathcal{F}_k$ l'ensemble streamé des facettes projectables de cardinal $k$. Pour $\tau\in\mathcal{F}_k$, le score local généralisant le chapitre 9 du manuscrit est

$$S_{k,\tau}(z)=\sum_{\substack{\sigma\supset\tau\\\lvert\sigma\rvert=k+1}}\rho(\sigma)^{-z}.$$

La somme porte sur les contributions certifiées par le flux source, y compris les continuations nécessaires. Elle ne doit pas être reconstruite depuis les seuls nœuds de multifusion. Une coface silencieuse peut contribuer au score même lorsqu'elle n'ajoute aucun nœud topologique.

Pour $k=1$, la face est le singleton $\left\lbrace x\right\rbrace$ et le vote HGP est trivial. Le contrat attribue au singleton certifié une contribution directe positive conventionnelle, normalisée ensuite comme les autres ordres. Cette convention ne déplace aucun niveau topologique.

### 6.2 Normalisation dans chaque ordre

Pour un point $x$ et un ordre $k$, on pose

$$T_{k,x}(z)=\sum_{\substack{\tau\in\mathcal{F}_k\\x\in\tau}}S_{k,\tau}(z).$$

Soit l'ensemble des ordres actifs pour ce point

$$A_x=\left\lbrace k:T_{k,x}(z)>0\right\rbrace.$$

Le profil reçoit des poids d'ordre rationnels strictement positifs $\eta_1,\ldots,\eta_{K_{\mathrm{eff}}}$. Le défaut neutre est $\eta_k=1$. La part totale du point réservée à l'ordre $k$ est

$$\alpha_{k,x}=\frac{\eta_k}{\sum_{j\in A_x}\eta_j},\qquad k\in A_x.$$

La version publique v1 fixe $m_x=1$ pour chaque point. La formule générale suivante décrit une extension future à des masses rationnelles positives, qui n'est pas encore exposée par l'API :

$$\pi_{k,x,\tau}=m_x\alpha_{k,x}\frac{S_{k,\tau}(z)}{T_{k,x}(z)}.$$

La masse de la facette devient

$$m_{k,\tau}=\sum_{x\in\tau}\pi_{k,x,\tau}.$$

### Proposition 6.1 — Conservation de la masse

Pour tout point possédant au moins un ordre actif,

$$\sum_{k\in A_x}\sum_{\substack{\tau\in\mathcal{F}_k\\x\in\tau}}\pi_{k,x,\tau}=m_x.$$

Par conséquent,

$$\sum_{k=1}^{K_{\mathrm{eff}}}\sum_{\tau\in\mathcal{F}_k}m_{k,\tau}=\sum_{x:A_x\neq\varnothing}m_x.$$

**Preuve.** Pour un ordre actif fixé, la normalisation par $T_{k,x}$ donne une somme égale à $m_x\alpha_{k,x}$. La somme des $\alpha_{k,x}$ sur $A_x$ vaut un. L'échange des deux sommes donne la seconde identité.

Cette normalisation empêche un point incident à beaucoup de facettes d'être surpondéré et empêche un ordre possédant combinatoirement plus de facettes de recevoir automatiquement plus de masse. Les $\eta_k$ expriment un choix scientifique explicite, pas un effet accidentel de la représentation.

## 7. Scores de sous-arbres et canal `stay`

Chaque facette projectable est attachée une seule fois à un nœud ou marqueur $b(k,\tau)$ de $\mathcal{M}$. Pour un point $x$ et un nœud $u$, la contribution directe est

$$D_x(u)=\sum_{\substack{k,\tau\\b(k,\tau)=u\\x\in\tau}}\pi_{k,x,\tau}.$$

La contribution totale du sous-arbre enraciné en $u$ est

$$Q_x(u)=\sum_{v\preceq u}D_x(v),$$

où $v\preceq u$ signifie que $v$ appartient au sous-arbre de $u$. Si $c_1,\ldots,c_s$ sont les enfants de $u$, alors

$$Q_x(u)=D_x(u)+\sum_{i=1}^{s}Q_x(c_i).$$

À l'arrivée d'un point dans $u$, les candidats sont :

- chaque enfant $c_i$, avec score $Q_x(c_i)$;
- le pseudo-enfant $\mathrm{stay}(u)$, avec score $D_x(u)$.

Le canal `stay` est normatif. Sans lui, toute contribution positive d'un enfant forcerait le point à descendre même si l'essentiel de son évidence appartient directement au parent. Un point envoyé vers `stay` reste membre de $u$ pendant la vie de ce cluster, puis devient bruit au-dessus de son niveau de sortie au lieu d'être artificiellement attribué à un descendant. La version v1 ne matérialise pas de pseudo-terminal à un niveau distinct : elle exige que le niveau de sortie déclaré soit exactement celui du carrier. Une durée prolongée sera rejetée jusqu'à ce qu'un futur schéma la représente comme un vrai nœud visible par les coupes et la condensation.

## 8. Routage descendant irréversible

### 8.1 Règle

Tous les points commencent à la racine virtuelle. À un nœud $u$ :

1. calculer les scores de tous les enfants et de `stay`;
2. choisir l'unique score strictement maximal;
3. en cas d'égalité numérique certifiée, choisir la clé canonique minimale parmi les candidats à égalité; en v1, `stay` possède la clé réservée minimale, puis les enfants sont ordonnés par `PointHierarchyNodeId`;
4. si `stay` gagne, terminer le trajet dans le pseudo-enfant $\mathrm{stay}(u)$;
5. si un enfant gagne, continuer uniquement dans cet enfant;
6. si tous les scores sont nuls, envoyer le point vers le terminal de bruit de $u$.

Une décision prise n'est jamais révisée à une coupe ultérieure. Un vote indépendant à chaque rayon est interdit, car un point pourrait passer d'une branche à une autre et produire des partitions non emboîtées.

La racine virtuelle fait concourir toutes les racines de tous les ordres. Elle corrige le défaut d'un routage séparé par composante connexe, dans lequel le même point pourrait être retenu par deux racines distinctes.

### 8.2 Terminaux et clusters de points

Après ajout des pseudo-enfants `stay` et bruit, chaque point possède un terminal unique $t(x)$. Pour tout nœud $u$ de l'arbre augmenté, on définit

$$P(u)=\left\lbrace x\in X:u\text{ est un ancêtre de }t(x)\right\rbrace.$$

### Théorème 8.1 — Partition locale

Pour tout nœud $u$ de fils $c_1,\ldots,c_s$, les ensembles routés vers les enfants, vers `stay` et vers le bruit sont deux à deux disjoints et leur union vaut $P(u)$.

**Preuve.** Chaque point de $P(u)$ évalue la même famille finie de candidats et la règle de décision retourne une seule clé. Il appartient donc à une seule sortie. Toute sortie est un enfant ou un pseudo-enfant de $u$, donc reste contenue dans $P(u)$. Réciproquement, la règle traite chaque point arrivé dans $u$, y compris le cas de score nul.

### Théorème 8.2 — Laminarité globale

Pour deux nœuds $u$ et $v$ de l'arbre augmenté, au moins une des situations suivantes vaut :

$$P(u)\subseteq P(v),\qquad P(v)\subseteq P(u),\qquad P(u)\cap P(v)=\varnothing.$$

**Preuve.** Si $u$ est ancêtre de $v$, tout terminal descendant de $v$ est descendant de $u$, donc $P(v)\subseteq P(u)$. Le cas symétrique est identique. Sinon, $u$ et $v$ sont situés dans deux sous-arbres issus de sorties distinctes de leur plus proche ancêtre commun; le Théorème 8.1 impose leur disjonction.

### Corollaire 8.3 — Unicité des étiquettes à toute coupe

Toute antichaîne de l'arbre augmenté produit des clusters de points deux à deux disjoints. Si l'antichaîne rencontre chaque trajet terminal, ces clusters et le bruit forment une partition de $X$.

La propriété ne dépend pas d'un test a posteriori sur les étiquettes : elle est imposée par la représentation et le routage.

## 9. Rendu par coupe de lambda

Pour un seuil $\lambda_{\mathrm{cut}}\geq0$, la coupe conserve les composantes de $\mathcal{G}^{\geq\lambda_{\mathrm{cut}}}$, puis lit les terminaux routés dans les nœuds maximaux correspondants. Un point dont le terminal n'est pas encore actif à ce seuil est bruit.

Après filtrage éventuel par masse minimale, les nœuds retenus forment une antichaîne. Le rendu renvoie donc directement un tableau d'une étiquette au plus par `PointId`; il ne reconstruit pas des listes de points indépendantes susceptibles de se recouvrir.

Le seuil lambda est comparé au moyen de la même représentation exacte que les niveaux de la tour. Une valeur décimale doit être parsée exactement ou accompagnée de sa tolérance et de son statut de simple proposition.

## 10. Rendu de type DBSCAN par rayon

Une coupe de la tour multi-ordres ne possède pas un rayon physique commun à tous les ordres, puisque le même lambda correspond à des rayons différents selon $k$. Le rendu par rayon reçoit donc un ordre de référence $k_{\mathrm{ref}}$, égal à $K_{\mathrm{eff}}$ par défaut, et définit

$$\lambda_{\mathrm{cut}}=\frac{k_{\mathrm{ref}}}{n\omega_3\varepsilon^z}.$$

Le couple $(\varepsilon,k_{\mathrm{ref}})$ joue le rôle des paramètres `(eps, min_samples)` de DBSCAN, mais la connectivité sous-jacente reste celle de la réduction multi-ordres, pas celle d'un graphe DBSCAN ajouté en parallèle.

Si $b=\varepsilon^2$ est exact, un événement $(k,a)$ se trouve au-dessus de la coupe si et seulement si

$$k^{2q}b^p\geq k_{\mathrm{ref}}^{2q}a^p.$$

Cette comparaison ne calcule ni racine, ni puissance flottante. L'API scientifique doit exposer clairement `reference_order`; un `cut_radius(epsilon)` qui cacherait ce choix serait ambigu.

## 11. Condensation par masse de points

La condensation se fait après le routage, sur des points distincts. La masse d'un nœud est

$$M(u)=\sum_{x\in P(u)}m_x.$$

Elle ne compte ni facettes, ni cofaces, ni incidences, ni occurrences répétées d'un point. La version v1 utilise uniquement `min_cluster_size` avec $m_x=1$. Un futur `min_cluster_mass` exigerait l'extension de masses rationnelles annoncée en section 6; dans les deux cas, le seuil masque une branche de rendu sans supprimer son état de la tour source.

Lorsqu'une branche enfant tombe sous le seuil, ses points quittent le cluster parent au niveau de séparation correspondant et deviennent du bruit condensé. Les nœuds de degré un qui ne portent ni sortie de point, ni changement de masse, ni niveau nécessaire au rejeu peuvent être contractés avec un reçu d'identité.

La racine virtuelle n'est pas un cluster sélectionnable. Elle sert seulement à garantir l'arbitrage global et une représentation totale. Par défaut, `allow_single_cluster=false` interdit aussi de retenir l'unique racine naturelle lorsqu'elle est le seul cluster condensé; l'option explicite `true` autorise cette sélection et appartient au reçu EOM.

## 12. Excès de masse et sélection EOM

Pour un cluster condensé $u$, soit $\lambda_{\mathrm{in}}(u)$ son niveau d'entrée. Pour $x\in P(u)$, soit $\lambda_{\mathrm{exit}}(x,u)$ le niveau auquel $x$ descend dans un enfant retenu ou quitte $u$ vers `stay` ou le bruit. La stabilité est

$$E(u)=\sum_{x\in P(u)}m_x\bigl(\lambda_{\mathrm{exit}}(x,u)-\lambda_{\mathrm{in}}(u)\bigr).$$

La condensation garantit que chaque différence est non négative. Pour les feuilles, le niveau de sortie doit être fini ou porté par une règle symbolique certifiée. Une contribution infinie issue de doublons à rayon nul n'est jamais remplacée silencieusement par `1e12` ou par un epsilon.

La règle v1 emploie un unique symbole formel $\Lambda_{\infty}$ pour tout le lot de rayon carré nul, indépendamment de l'ordre $k$, conformément à la section 3. Une expression de stabilité est conservée sous la forme exacte suivante, sans évaluer une soustraction indéfinie d'infinis :

$$A=c_{\infty}\Lambda_{\infty}+\sum_j c_j\lambda_j,\qquad \lambda_j<\Lambda_{\infty}.$$

Deux masses symboliques égales s'annulent exactement. Si $c_{\infty}\neq0$, son signe décide avant toute partie finie; si $c_{\infty}=0$, seule la somme finie restante est comparée par intervalles rationnels certifiés. Cette règle est une sémantique symbolique explicite du lot commun, pas une limite flottante : elle n'introduit ni epsilon, ni plafond numérique, ni ordre artificiel entre $k/0$ et $k'/0$. Une feuille née au rayon nul mais absorbée à un rayon strictement positif quitte simplement sa branche au niveau fini de cette absorption; aucun pseudo-terminal n'est ajouté.

La programmation dynamique EOM est

$$R(u)=\max\left(E(u),\sum_{c\in\mathrm{children}(u)}R(c)\right).$$

Si $E(u)$ est strictement supérieur, $u$ est retenu et aucun descendant ne l'est. Si la somme des enfants est strictement supérieure, la sélection descend. En cas d'égalité certifiée, le contrat doit fixer une règle, par exemple préférer le parent pour obtenir la sélection la plus grossière. Cette règle fait partie du digest de projection.

### Proposition 12.1 — Disjonction de la sélection EOM

Les nœuds sélectionnés par la récurrence EOM forment une antichaîne.

**Preuve.** Lorsqu'un parent est retenu, la récurrence interdit tous ses descendants. Lorsqu'il ne l'est pas, seules les sélections calculées dans ses sous-arbres enfants sont réunies; ces sous-arbres sont disjoints. L'induction part des feuilles.

Le rendu EOM hérite donc immédiatement du Corollaire 8.3.

## 13. Calcul certifié des poids et décisions fail-closed

Pour $z$ pair entier, les valeurs $\rho^{-z}$ sont rationnelles lorsque les niveaux carrés le sont. Pour un exposant rationnel général, les scores peuvent être des sommes de nombres algébriques. Leur ordre ne doit pas être décidé par un simple `double` si la sortie revendique une projection certifiée.

Le pipeline sépare quatre couches :

1. **proposition flottante** : tri approximatif, scores binary64 et candidats gagnants;
2. **borne certifiée** : intervalles dirigés contenant chaque score et chaque stabilité;
3. **décision certifiée** : intervalles disjoints pour le gagnant, ou égalité formelle démontrée;
4. **réduction hiérarchique** : publication du terminal et des nœuds sélectionnés seulement après la décision.

Si les deux meilleurs intervalles se recouvrent, la précision augmente. Si le budget de précision est atteint sans séparation et sans preuve d'égalité, le point, le lot ou la sélection EOM échoue fermé selon l'atomicité annoncée. La clé canonique ne départage qu'une égalité certifiée; elle ne masque pas une comparaison indécidable.

Chaque sortie doit enregistrer au minimum :

- le digest et le statut de la source;
- $K_{\mathrm{eff}}$, $z=p/q$ et les poids $\eta_k$;
- la convention des singletons d'ordre un;
- la règle d'activation verticale;
- la politique d'égalité du routage et de l'EOM;
- les budgets de précision et le nombre de rejeux certifiés;
- le nombre de décisions ambiguës et le motif de tout échec fermé;
- le digest de l'arbre de carriers, du routage terminal et de la sélection.

Un résultat n'est publié comme `exact_relative_to_certified_source` que si toutes les décisions topologiques, tous les routages et toutes les comparaisons EOM nécessaires sont certifiés. Cette chaîne n'autorise toujours pas `public_status=exact` pour la bibliothèque si les portes source restent ouvertes.

## 14. Architecture streamée

Notons :

- $H$ le nombre total de nœuds et marqueurs horizontaux;
- $V$ le nombre de coutures verticales compactes;
- $F$ le nombre de facettes projectables réellement émises;
- $C$ le nombre de contributions coface--facette réellement émises;
- $I=\sum_{k,\tau}\lvert\tau\rvert$ le nombre d'incidences point--facette;
- $d_x$ le nombre de carriers incidents au point $x$ et $d_{\max}=\max_x d_x$;
- $B$ la taille maximale d'un chunk résident.

### 14.1 Topologie

À ordre fixé, les niveaux source sont déjà triés par rayon et donc par lambda inverse. Si les coutures verticales sont elles aussi émises dans l'ordre certifié de leur niveau cible, une fusion à $K_{\mathrm{eff}}$ voies des flux d'ordres donne le calendrier global en

$$O\bigl((H+V)\log K_{\mathrm{eff}}\bigr)$$

comparaisons de niveaux. Une source verticale non triée exige au préalable un tri exact, éventuellement externe, en $O\bigl((H+V)\log(H+V)\bigr)$ comparaisons; ce coût doit être publié séparément. Le DSU ajoute

$$O\bigl((H+V)\alpha(H)\bigr)$$

opérations amorties, hors coût binaire des comparaisons exactes. Le stockage de la topologie et du DSU est $O(H+V)$ avant compaction, puis $O(H)$ si les coutures sont consommées et authentifiées en flux.

### 14.2 Poids

Une réalisation massive emploie plusieurs passes authentifiées :

1. trier ou recevoir groupées les contributions par clé exacte de facette et réduire $S_{k,\tau}$;
2. émettre les incidences $(k,x,\tau,S_{k,\tau})$ et réduire $T_{k,x}$ par point et ordre;
3. rejouer ou relire les incidences pour produire $\pi_{k,x,\tau}$;
4. trier les contributions de routage par `PointId`.

Ces passes peuvent utiliser un spool externe borné ou un tri radix GPU, mais chaque run, chunk et digest doit être relié au reçu source. Elles ne matérialisent ni matrice point--facette, ni matrice point--point.

### 14.3 Routage

Pour un point fixé, seules ses feuilles ou carriers incidents sont utiles. Leur ordre DFS et leurs plus proches ancêtres communs construisent l'arbre virtuel minimal du point. Une réalisation CPU générale coûte

$$O\bigl(d_x\log d_x\bigr)$$

par point, soit

$$O\bigl(I\log d_{\max}\bigr)$$

au total. Des incidences déjà triées et un traitement parallèle par lot peuvent approcher $O(I)$, sans changer la sémantique.

La sortie compacte conserve un terminal par point, l'ordre DFS des terminaux et les intervalles des nœuds. Les membres d'un cluster sont alors un intervalle logique, pas une copie de tous ses `PointId`. La mémoire résidente cible est

$$O(H+n+B),$$

hors spool authentifié. Le seul tableau de terminaux impose déjà un coût linéaire incompressible en $n$.

## 15. Structures globales explicitement évitées

La projection ne construit pas :

- la mosaïque de Delaunay d'ordre supérieur;
- Gamma global;
- Star global;
- un catalogue résident global de toutes les facettes, cofaces ou incidences;
- une matrice de paires ou une matrice point--cluster dense;
- un graphe de voisinage de points destiné à remplacer la tour;
- un MST de points présenté comme source HGP.

Le flux de contributions peut être exhaustif et trié extérieurement sans devenir l'architecture résidente par défaut. Un oracle exhaustif borné peut vérifier cette réduction sur de petites fixtures, mais ne devient pas le chemin produit.

## 16. Tests mathématiques et logiciels requis

### 16.1 Fixtures minimales permanentes

La validation doit inclure au minimum :

1. $K=1$, où le routage restitue la hiérarchie de points source;
2. deux composantes d'ordre supérieur partageant un point, pour vérifier qu'elles ne produisent jamais deux étiquettes;
3. deux racines distinctes partageant des incidences de points, pour exercer l'arbitrage de la racine virtuelle;
4. un nœud dont `stay` gagne contre tous les enfants;
5. une égalité exacte entre enfants, stable sous permutation des entrées;
6. une quasi-égalité où la proposition binary64 est fausse mais le rejeu certifié corrige le gagnant;
7. un budget de précision insuffisant qui échoue sans publier d'étiquette;
8. un lot multifurqué de niveau égal permuté dans tous les ordres;
9. une verticale supérieure qui persiste entre $\lambda_z(k+1,a)$ et $\lambda_z(k,a)$, afin de détecter toute absorption prématurée par $T_1$;
10. une continuation $q_R=1$ avec delta de couverture qui modifie les poids sans ajouter de branche;
11. une coupe lambda, une coupe rayon et une sélection EOM vérifiées sur chaque niveau critique;
12. un événement de rayon nul traité symboliquement;
13. une mutation de digest, d'ordre, de niveau, de verticale, de facette ou de poids rejetée par rejeu frais.

Toute contradiction mathématique doit devenir une fixture permanente et mettre à jour le registre des preuves avant toute optimisation supplémentaire.

### 16.2 Invariants vérifiés exhaustivement

Sur chaque fixture et chaque campagne qualifiante :

- chaque `PointId` possède exactement un terminal, bruit inclus;
- la conservation de masse de la Proposition 6.1 vaut;
- les enfants et `stay` partitionnent leur parent;
- toutes les paires de nœuds satisfont le Théorème 8.2 sur les petites fixtures;
- chaque coupe critique produit au plus une étiquette par point;
- la sélection EOM est une antichaîne;
- les lots égaux sont indépendants de leur permutation;
- les modes résident, streamé et restauré donnent des digests et terminaux identiques;
- aucune sortie certifiée n'est publiée après une ambiguïté non résolue.

## 17. Mesures de performance à publier

Un rapport de performance doit séparer :

- lecture et vérification de la source;
- fusion multi-ordres des événements;
- construction DSU atomique;
- réduction des scores de facettes;
- réduction des normalisateurs par point;
- tri des incidences;
- routage flottant proposé;
- rejeu certifié et précision maximale atteinte;
- construction des intervalles DFS;
- condensation;
- rendu par rayon, lambda et EOM.

Pour chaque étape, publier au minimum le temps mur, le temps GPU éventuel, le débit d'événements ou d'incidences, le RSS HWM, le pic de mémoire device, les octets de spool, $H$, $V$, $F$, $C$, $I$, la distribution des $d_x$, le nombre de décisions rejouées et le nombre d'échecs fermés. Les temps de la source géométrique et ceux de la projection aval restent séparés.

## 18. Cibles 50 k et dizaines de millions : non-promesses actuelles

La complexité de la projection est linéaire ou quasi linéaire dans la taille du flux effectivement produit. Elle ne donne aucune borne a priori sur cette taille. En dimension trois, une structure de Delaunay peut avoir une complexité quadratique dans le pire cas, et la complétude d'une source HGP sparse reste une obligation séparée.

### 18.1 Profil d'environ 50 k points

Une qualification 50 k exige au préalable :

- une source exacte complète et scellée pour tous les ordres demandés;
- toutes les verticales et naturalités requises;
- un flux de contributions authentifié;
- un budget préflight sur $H$, $V$, $C$ et $I$;
- l'identité byte-à-byte des chemins résident et streamé;
- zéro décision de routage ou EOM non certifiée;
- les mesures détaillées de la section 17.

Aucun temps interactif, aucun SLO inférieur à une seconde et aucune exactitude publique 50 k ne découle de ce document.

### 18.2 Profil de dizaines de millions de points

Le profil massif exige en plus :

- un calendrier borné et reprenable de tous les chunks;
- un spool ou une réduction GPU dont les digests sont restaurables;
- une mémoire $O(H+n+B)$ effectivement mesurée;
- aucune table globale dimensionnée par toutes les facettes possibles;
- une reprise après interruption donnant le même terminal par point;
- une qualification sur la distribution réelle de $d_x$ et non sur sa seule moyenne;
- l'arrêt fermé avant allocation lorsque les budgets source ou projection sont dépassés.

La mention « dizaines de millions » reste une cible produit conditionnelle. Un benchmark de composant, une forêt structurelle, un accord moyen ou un run surrogate ne la qualifie pas.

## 19. Alternatives rejetées

### 19.1 Vote indépendant à chaque coupe

Il produit une partition à une coupe fixée, mais un point peut changer de gagnant lorsque le niveau varie. Les partitions ne sont alors pas nécessairement emboîtées. Le routage irréversible est requis.

### 19.2 Fermeture par union des supports

Fusionner deux composantes dès qu'elles partagent un point transforme le recouvrement en connexité de points. Avec toutes les verticales au même rayon, cette fermeture est dominée par $T_1$ et détruit l'information d'ordre supérieur. Elle n'est pas retenue.

### 19.3 Utiliser seulement $T_1$ comme squelette

Cette option garantit la laminarité, mais les ordres supérieurs ne peuvent plus créer de branche propre; ils ne font que décorer le Single-Linkage. Elle ne répond pas à l'objectif multi-ordres.

### 19.4 Construire un MST de points pondéré par les votes

Un tel MST définit une autre hiérarchie, mais il remplace la tour par un graphe de points et réintroduit une fermeture de type Single-Linkage. Il peut être étudié comme transformation distincte, jamais comme chemin exact de cette spécification.

## 20. Absence d'argument `splitting` dans le cœur

La construction normative ne possède pas d'argument `splitting`. La topologie, le routage et les rendus standards restent reproductibles sans callback utilisateur.

Si une fonctionnalité de raffinement est ultérieurement demandée, elle doit être une transformation séparée de la sélection déjà obtenue. Elle peut remplacer récursivement un nœud sélectionné par une antichaîne de descendants lorsque le prédicat accepte les ensembles de points enfants déjà disjoints. Elle ne recalcule jamais les votes et ne modifie ni la tour source, ni le terminal des points, ni le certificat de projection.

## 21. Portée exacte de la conclusion

Sous les hypothèses suivantes :

1. la tour $T_1,\ldots,T_{K_{\mathrm{eff}}}$ et ses verticales sont complètes et certifiées;
2. le flux des contributions projectables est complet et lié au même digest source;
3. toutes les comparaisons de niveaux, de routage et de sélection sont certifiées;
4. les budgets sont respectés sans fallback;

la construction retourne une hiérarchie de points déterministe, laminaire et exactement conforme aux définitions de ce document. À toute coupe et pour toute sélection EOM, aucun point ne peut appartenir à deux clusters distincts.

Cette conclusion est **relative à la source fournie**. Elle ne démontre ni la complétude scientifique de la forêt HGP, ni M.1, ni la verticalité absente, ni un SLO 50 k, ni le passage à des dizaines de millions de points, ni `public_status=exact`.
