# Résultat D — cascade de puissance certifiée pour l’énumération 3D

> - Annexe mathématique à l'[architecture E-HGP](../ARCHITECTURE_MATHEMATIQUE.md)
> - Cible : $X\subset\mathbb{R}^{3}$, $K_{\max}=10$, catalogue critique exact lorsque le certificat de fermeture est obtenu ; hiérarchies $H_0$ exactes après certification séparée des attaches et des lots
> - Statut : noyau exact sous position générale ; contrat de plateaux dégénérés explicité ; complexité strictement sensible à la sortie $H_0$ encore ouverte

> [!IMPORTANT]
> **Décision.** Le backend 3D ne doit matérialiser ni *rhomboid tiling*, ni mosaïque de Delaunay d’ordre supérieur. La voie retenue est une **cascade de diagrammes de puissance de premier ordre**, consommés cellule par cellule. L’incrément d’Edelsbrunner–Osang fournit des colonnes structurées pour l’ordre suivant, mais il ne constitue pas à lui seul le certificat final. La complétude est certifiée par un test de séparation aux sommets de chaque cellule, fondé sur un oracle top-$k$ exact.

## 1. Verdict et frontière de la proposition

La bonne primitive n’est pas une triangulation régulière globale. C’est une opération bornée et parallélisable :

> étant donné un site pondéré et un domaine convexe borné, construire sa cellule par intersections successives de demi-espaces, puis prouver qu’aucun site implicite ne peut encore la couper.

Cette primitive suffit à bâtir une `PowerCascade` pour $k=1,\ldots,K_{\max}$. À chaque ordre, elle ne conserve que :

- les labels actifs $Q$ de cardinal $k$ ;
- leurs cellules de puissance ordinaires pendant leur consommation ;
- les incidences de codimension $1$, $2$ et $3$ nécessaires aux supports de taille $2$, $3$ et $4$ ;
- les événements critiques classés acceptés ;
- les colonnes proposées pour les ordres futurs ;
- un certificat local de fermeture et une file de frontières non encore fermées.

Le théorème générique ci-dessous utilise l’hypothèse $(\mathrm{GP}_{3})$ : les observations sont distinctes, elles engendrent $\mathbb{R}^{3}$, aucun shell sphérique pertinent ne contient plus de quatre observations, et chaque support critique est affine indépendant avec des coordonnées barycentriques non nulles. Cette hypothèse n’interdit pas qu’un sommet d’un diagramme d’ordre $k$ ait plus de quatre cellules incidentes : les choix de plusieurs points parmi un même shell créent précisément ces ex æquo structurels entre **labels** barycentriques. Ils font partie de l’objet à reconstruire.

Le résultat exact que l’on peut raisonnablement viser est le suivant.

> **Théorème-cible, forme certifiante.** Sous $(\mathrm{GP}_{3})$, si les requêtes top-$k$, les découpages polyédriques et les prédicats de rang sont exacts, et si toutes les cellules de puissance restreintes satisfont le certificat de fermeture défini à la section 5, alors le catalogue émis contient exactement toutes les sphères critiques de rang fermé au plus $K_{\max}+1$. Il fournit donc tous les porteurs locaux des naissances et selles requises par les forêts $H_0$ d’ordres $1$ à $K_{\max}$. La reconstruction des forêts exige en plus les certificats d’attache globale et le traitement par lots du résultat C.

Ce théorème ne promet pas que la fermeture est petite. Une cascade fermée peut encore rencontrer une sortie intermédiaire quadratique. En particulier, la simple exécution de dix diagrammes de puissance, même très rapide sur GPU, n’est pas une preuve de sparsité hiérarchique.

## 2. Identité fondamentale : l’ordre $k$ comme diagramme de puissance ordinaire

Soit $Q\subseteq X$ un sous-ensemble de cardinal $k$. Posons son barycentre, sa variance et son poids :

$$b_Q=\frac{1}{k}\sum_{x\in Q}x,\qquad v_Q=\frac{1}{k}\sum_{x\in Q}\left\Vert x-b_Q\right\Vert^2,\qquad \omega_Q=-v_Q=\left\Vert b_Q\right\Vert^2-\frac{1}{k}\sum_{x\in Q}\left\Vert x\right\Vert^2.$$

La puissance du site pondéré $(b_Q,\omega_Q)$ est

$$\pi_Q(y)=\left\Vert y-b_Q\right\Vert^2-\omega_Q=\frac{1}{k}\sum_{x\in Q}\left\Vert y-x\right\Vert^2.$$

Si $a_1(y)\leq\cdots\leq a_n(y)$ sont les distances carrées ordonnées, alors

$$\min_{\lvert Q\rvert=k}\pi_Q(y)=\frac{1}{k}\sum_{j=1}^{k}a_j(y).$$

Les minimiseurs sont exactement les ensembles des $k$ plus proches voisins de $y$. La cellule

$$C_k(Q)=\left\lbrace y:\pi_Q(y)\leq\pi_R(y)\text{ pour tout }R\subseteq X,\ \lvert R\rvert=k\right\rbrace$$

est donc le domaine de Voronoï d’ordre $k$ étiqueté par $Q$.

Deux précisions évitent un contresens.

1. Le potentiel $\pi_Q$ est la moyenne quadratique utilisée par la DTM, mais il sert ici de **représentation affine exacte de l’ordre des voisins**. On ne remplace pas le champ HGP dur par la DTM.
2. Le diagramme utilisé par le backend est toujours un diagramme de puissance **de premier ordre** sur des sites pondérés $(b_Q,\omega_Q)$. L’ordre supérieur reste seulement dans les labels $Q$.

En développant les carrés, la différence de deux puissances ne contient plus le terme $\left\Vert y\right\Vert^2$ :

$$\pi_R(y)-\pi_Q(y)=2\langle y,b_Q-b_R\rangle+\left\Vert b_R\right\Vert^2-\omega_R-\left\Vert b_Q\right\Vert^2+\omega_Q.$$

Elle est affine en $y$. Cette observation très simple est la clef du certificat de fermeture.

## 3. Domaine borné et état actif

Tous les centres critiques considérés sont dans l’enveloppe convexe de leur support frontière, donc dans $\mathrm{conv}(X)$. On fixe un polytope convexe borné $\Omega$ tel que

$$\mathrm{conv}(X)\subseteq\mathrm{int}(\Omega).$$

Une boîte englobante avec marge suffit mathématiquement. Ses faces sont artificielles et ne produisent aucun événement : tout candidat touchant seulement $\partial\Omega$ est ignoré.

À l’ordre $k$, un **état actif** est un label $Q\in\binom{X}{k}$ dont la cellule restreinte

$$C_k^{\Omega}(Q)=C_k(Q)\cap\Omega$$

est non vide. Sous position générale, on peut demander qu’elle soit de dimension $3$ ; les cellules de dimension plus faible sont alors des incidences, pas de nouveaux états. En présence de dégénérescences, elles sont conservées dans une classe de plateau.

Un état minimal contient conceptuellement :

| champ | sens mathématique |
|---|---|
| $Q$ | label trié de $k$ observations |
| $(b_Q,\omega_Q)$ | site de puissance exact ou filtré |
| $C$ | cellule convexe courante, éventuellement encore trop grande |
| `binding` | contraintes qui portent réellement une face de $C$ |
| `closure` | sommets déjà séparés contre l’oracle global |
| `incidence` | signatures de faces, arêtes et sommets à réduire |

Le polyèdre $C$ est un objet transitoire. Une fois ses incidences émises et appariées, il peut être libéré. La topologie persistante réside dans le catalogue critique et dans les forêts, pas dans un maillage volumique.

## 4. De l’incidence de puissance à une sphère critique

### 4.1 Faces de première génération

Soit $F$ une face de codimension $r-1$ du diagramme fermé à l’ordre $k$, avec $2\leq r\leq4$. Dans le cas générique utile, ses $r$ cellules incidentes ont des labels

$$Q_u=I\cup\left\lbrace u\right\rbrace,\qquad u\in U,$$

avec

$$\lvert I\rvert=k-1,\qquad U\cap I=\varnothing,\qquad \lvert U\rvert=r.$$

On appellera cette incidence **face de première génération**, sans construire l’objet supérieur dont cette terminologie est issue. Le test combinatoire est

$$I=\bigcap_{Q\in\mathcal{A}(F)}Q,\qquad U=\left(\bigcup_{Q\in\mathcal{A}(F)}Q\right)\setminus I,\qquad \lvert I\rvert=k-1,$$

où $\mathcal{A}(F)$ est la classe complète des cellules incidentes à $F$.

Pour $u,u'\in U$, l’égalité $\pi_{I\cup\{u\}}(y)=\pi_{I\cup\{u'\}}(y)$ équivaut à

$$\left\Vert y-u\right\Vert^2=\left\Vert y-u'\right\Vert^2.$$

Le lieu affine portant $F$ est donc le lieu d’équidistance aux points de $U$. Si $U$ est affine indépendant, ce lieu est orthogonal à $\mathrm{aff}(U)$ et leur intersection est un point unique, noté $c(U)$. C’est le centre de la sphère circonscrite à $U$ dans son enveloppe affine.

### 4.2 Critère exact d’acceptation

Une incidence de première génération donne un événement critique si et seulement si les tests suivants réussissent :

1. $c(U)$ appartient à la face fermée $F$ ;
2. $c(U)\in\mathrm{relint}\,\mathrm{conv}(U)$ ;
3. les points de $I$ sont strictement à l’intérieur de la sphère $(c(U),\rho)$ ;
4. les points de $U$ sont sur sa frontière ;
5. le prédicat de rang global ne trouve aucun point de $X\setminus(I\cup U)$ intérieur ou frontal ;
6. le rang fermé $s$ vérifie $s\leq K_{\max}+1$.

Ici

$$\rho^2=\left\Vert c(U)-u\right\Vert^2\qquad\text{pour tout }u\in U.$$

Sous position générale, les conditions 3 à 5 redonnent exactement $I$ et $U$, et

$$s=\lvert I\rvert+\lvert U\rvert=k-1+r.$$

La condition de relative-intériorité est un test de signes barycentriques. C’est elle qui élimine les supports obtus : une incidence de puissance n’est pas automatiquement un point critique de la fonction K-NN.

### 4.3 Les trois codimensions sont indispensables

| support $U$ | objet de puissance inspecté | dimension dans $\mathbb{R}^{3}$ | centre à tester |
|---:|---|---:|---|
| $2$ | face entre deux cellules | $2$ | milieu du segment |
| $3$ | arête commune à trois cellules | $1$ | centre circonscrit dans le plan du triangle |
| $4$ | sommet commun à quatre cellules | $0$ | centre de la sphère circonscrite |

Inspecter seulement les sommets du diagramme manquerait donc les supports de tailles $2$ et $3$. Réciproquement, énumérer toutes les paires, tous les triangles et tous les tétraèdres de $X$ détruirait le bénéfice de la structure de puissance.

Le cas $\lvert U\rvert=1$ est ajouté séparément : sous généricité et sans duplications, ce sont les observations elles-mêmes, de rayon nul et de rang $1$.

### 4.4 Un seul passage jusqu’à $K_{\max}$

Une sphère non triviale avec $m$ points strictement intérieurs et $r\geq2$ points de support apparaît comme face de première génération dans le diagramme d’ordre

$$k=m+1.$$

Son rang fermé vaut $s=m+r$. Si $s\leq K_{\max}+1$, alors

$$k=m+1=s-r+1\leq K_{\max}.$$

Il n’est donc pas nécessaire de construire le diagramme d’ordre $K_{\max}+1$ pour obtenir les selles de rang $K_{\max}+1$. Les diagrammes $1$ à $K_{\max}$, plus les naissances triviales de rang $1$, suffisent.

Une sphère acceptée alimente ensuite l’échafaudage classé du rapport principal : naissance à l’ordre $s$ si $s\leq K_{\max}$, selle d’indice $1$ à l’ordre $s-1$, et ancre verticale entre ces deux ordres. L’ordre $m+1=s-r+1$ est seulement l’**ordre de détection** de la sphère par une face de première génération ; sauf lorsque $r=2$, il diffère de l’ordre de selle $s-1$.

## 5. Noyau exact : fermeture par séparation aux sommets

La difficulté véritable n’est pas de valider une sphère proposée. Elle est de prouver qu’aucune cellule active, donc aucune face critique, n’a été omise. La fermeture suivante résout ce problème sans énumérer au préalable les $\binom{n}{k}$ sites.

### 5.1 Cellule relative à un ensemble de colonnes

Soit $\mathcal{S}\subseteq\binom{X}{k}$ un ensemble courant de labels et $Q\in\mathcal{S}$. Posons

$$C_{\mathcal{S}}(Q)=\Omega\cap\bigcap_{R\in\mathcal{S}}\left\lbrace y:\pi_Q(y)\leq\pi_R(y)\right\rbrace.$$

Ce polytope contient la vraie cellule $C_k^{\Omega}(Q)$ et peut être trop grand. Pour un sommet $v$ de $C_{\mathcal{S}}(Q)$, une requête top-$k$ exacte fournit un minimiseur global

$$R_v\in\mathop{\mathrm{argmin}}_{\lvert R\rvert=k}\pi_R(v).$$

Si $\pi_{R_v}(v)<\pi_Q(v)$, le label $R_v$ est une **colonne violatrice**. On l’insère et on découpe par le demi-espace correspondant. Si $\pi_{R_v}(v)=\pi_Q(v)$, la géométrie de la cellule de $Q$ est déjà correcte vis-à-vis de $R_v$, mais le co-minimiseur doit encore être inséré pour reconstruire l’incidence non perturbée.

### 5.2 Lemme de séparation polyédrique

> **Lemme — certificat aux sommets.** Supposons $C_{\mathcal{S}}(Q)$ non vide et borné. Alors
>
> $$C_{\mathcal{S}}(Q)=C_k^{\Omega}(Q)$$
>
> si et seulement si $Q$ est un minimiseur global de $\pi_R(v)$ à chaque sommet $v$ de $C_{\mathcal{S}}(Q)$.

**Preuve.** La nécessité est immédiate. Réciproquement, fixons un label implicite $R\in\binom{X}{k}$. La fonction $h_R=\pi_R-\pi_Q$ est affine. Par hypothèse, $h_R(v)\geq0$ à tous les sommets du polytope. Toute valeur de $h_R$ sur le polytope est une combinaison convexe de ses valeurs aux sommets ; elle est donc positive ou nulle. Ainsi $\pi_Q\leq\pi_R$ sur tout $C_{\mathcal{S}}(Q)$, simultanément pour tout $R$. $\square$

La contraposée est algorithmique : si une fonction implicite coupe encore le polytope, une violation existe à au moins un de ses sommets. Une requête top-$k$ à ce sommet produit immédiatement une contrainte valide.

### 5.3 Terminaison et découverte des sites manquants

À chaque ronde non fermée, au moins un nouveau label de $\binom{X}{k}$ est ajouté. Pour un ensemble initial $\mathcal{S}_0$ non vide, la boucle de colonnes termine donc après au plus $\binom{n}{k}-\lvert\mathcal{S}_0\rvert$ rondes d’insertion. Cette borne finie est seulement combinatoire et n’a aucune valeur pratique pour $k=10$.

À la fermeture de toutes les cellules courantes, aucun site actif de dimension pleine ne peut manquer. En effet, les cellules courantes couvrent $\Omega$. Si un label absent $R$ minimisait strictement la puissance sur un ouvert, cet ouvert rencontrerait une cellule courante certifiée appartenant à un autre label $Q$, ce qui contredirait le lemme.

Cette conclusion suffit déjà pour la complétude critique sous $(\mathrm{GP}_{3})$. Pour une sphère de détection $(I,U)$ et chaque $u\in U$, l’indépendance affine de $U$ permet une petite perturbation du centre qui rend $u$ strictement plus proche que $U\setminus\{u\}$, tandis que l’écart aux points extérieurs persiste. Chaque label $I\cup\{u\}$ incident à la face critique possède donc une cellule de dimension pleine et ne peut manquer.

Pour obtenir non seulement la géométrie des cellules, mais toutes leurs incidences, l’oracle ne doit pas retourner un unique top-$k$ arbitraire lorsqu’il y a égalité. Il retourne la décomposition compacte du shell :

$$I(v)=\left\lbrace x:\left\Vert v-x\right\Vert^2<a_k(v)\right\rbrace,\qquad U(v)=\left\lbrace x:\left\Vert v-x\right\Vert^2=a_k(v)\right\rbrace.$$

Les minimiseurs sont tous les labels

$$I(v)\cup W,\qquad W\subseteq U(v),\qquad \lvert W\rvert=k-\lvert I(v)\rvert.$$

Les sommets suffisent aussi pour retrouver un co-minimiseur qui ne coupe pas strictement la cellule. Une fois la cellule de $Q$ certifiée, le lieu $\pi_R=\pi_Q$ y est soit vide, soit toute la cellule, soit une face exposée ; dans les deux derniers cas, il contient un sommet du polytope borné. L’énumération complète du shell à tous les sommets révèle donc $R$.

Sous position générale 3D, le shell pertinent contient au plus quatre points. En données dégénérées, il peut être stocké comme hyperévénement sans être développé aveuglément. Un ordre ne reçoit toutefois pas le statut `power_closed(k)` tant que cette représentation comprimée n’a pas permis de reconstruire toutes les cellules et incidences utiles, ou tant que les labels requis n’ont pas été explicitement développés.

### 5.4 Certificat global de fermeture

Un ordre $k$ reçoit le statut `power_closed(k)` lorsque les quatre conditions suivantes sont réunies :

1. chaque cellule non vide possède un certificat aux sommets contre le top-$k$ global ;
2. à chaque sommet de cellule, tous les minimiseurs donnés par le shell top-$k$ ont été enregistrés, puis les classes d’égalité portées par les faces et arêtes ont été reconstruites ;
3. chaque face interne émise est appariée à toutes ses cellules incidentes, tandis que les faces non appariées sont sur $\partial\Omega$ ;
4. la file des colonnes violatrices, des voisins de face et des classes d’incidence à reconstruire est vide.

La condition 3 sert de contrôle topologique et numérique ; la preuve de complétude géométrique repose sur 1, 2 et le fait que les cellules certifiées couvrent $\Omega$.

> **Corollaire — exactitude du catalogue.** Sous $(\mathrm{GP}_{3})$, si `power_closed(k)` vaut pour tout $1\leq k\leq K_{\max}$ et si les prédicats de la section 4 sont exacts, le catalogue contient toutes et seulement les sphères critiques de rang fermé au plus $K_{\max}+1$.

**Justification.** Toute sphère critique non triviale de rang admissible possède $m$ points intérieurs, un support affine indépendant $U$ de taille $2$ à $4$, et apparaît dans le diagramme fermé à l’ordre de détection $m+1\leq K_{\max}$ comme face de première génération. Les cellules $I\cup\{u\}$ incidentes sont de dimension pleine par l’argument de perturbation de la section 5.3 ; la réduction d’incidences émet donc cette face et le test retrouve son centre. Réciproquement, chaque événement accepté satisfait directement les conditions géométriques et le rang requis. $\square$

Ce corollaire est le noyau exact recommandé. Il ne dépend pas d’une supposition selon laquelle la récurrence entre ordres aurait proposé toutes les colonnes.

## 6. Cascade inter-ordres : accélération prouvée, pas certificat final

### 6.1 Traduction en flux de l’algorithme incrémental

Edelsbrunner et Osang montrent que les sites actifs des ordres successifs peuvent être engendrés à partir des cellules **pleines** de première génération, en appelant à chaque ordre un algorithme de Delaunay pondéré de premier ordre. Leur construction peut être traduite en flux sans conserver les mosaïques après consommation.

À un sommet du diagramme de puissance dual d’une cellule Delaunay pleine de première génération à l’ordre $k$, les quatre cellules de puissance incidentes portent, sous généricité,

$$I\cup\left\lbrace u_1\right\rbrace,\ I\cup\left\lbrace u_2\right\rbrace,\ I\cup\left\lbrace u_3\right\rbrace,\ I\cup\left\lbrace u_4\right\rbrace,$$

avec $\lvert I\rvert=k-1$ et $U=\{u_1,u_2,u_3,u_4\}$. La récurrence publiée considère les générations $g=2,3$. Elle émet, à l’ordre $k+g-1$, la cellule dont les labels sont

$$\mathcal{V}_{g}(I,U)=\left\lbrace I\cup W:W\in\binom{U}{g}\right\rbrace,\qquad g\in\{2,3\}.$$

Pour l’initialisation des sites pondérés, il suffit d’insérer chaque label de $\mathcal{V}_{g}(I,U)$ dans le seau correspondant. Les labels de génération $1$ sont ceux de l’ordre courant. La tranche extrême de génération $4$ n’entre pas dans la boucle publiée : le théorème d’Edelsbrunner–Osang garantit que tout site actif d’ordre ultérieur est engendré depuis une cellule pleine de première génération antérieure, comme sommet d’une tranche de génération $2$ ou $3$.

La `PowerCascade` effectue donc :

1. consommation du seau $\mathcal{S}_k$ ;
2. construction et fermeture des cellules ordinaires de ses sites ;
3. extraction des événements de supports $2$, $3$ et $4$ ;
4. émission des expansions publiées vers $\mathcal{S}_{k+1}$ et $\mathcal{S}_{k+2}$ ;
5. libération de la géométrie de l’ordre $k$ après appariement des incidences.

Si toutes les mosaïques complètes des ordres précédents sont consommées, cette récurrence produit le jeu complet de sites actifs pour des observations originales non pondérées sous les hypothèses d’Edelsbrunner–Osang. Elle ne se généralise pas automatiquement à des poids arbitraires sur les observations.

### 6.2 Pourquoi la séparation reste nécessaire

La preuve publiée de la complétude de cette récurrence s’appuie sur la structure combinatoire de la *rhomboid tiling*. On peut utiliser le théorème sans construire cette tiling, mais quatre raisons imposent un contrôle indépendant dans le backend :

- une incidence peut être perdue par un prédicat numérique ou par une reconstruction incomplète des ex æquo ;
- la cascade proposée tronque les cellules à $\Omega$ et les consomme en flux ; si elle ne traite pas toutes les cellules pleines globales des ordres antérieurs, le théorème publié ne suffit plus à justifier ses seaux restreints ;
- les duplications et cosphéricités sortent de la position générale utilisée par la preuve simple ;
- un futur mode H0-pruné ne traitera volontairement pas toutes les cellules dont la récurrence aurait besoin.

La cascade doit donc être comprise comme une initialisation chaude extrêmement structurée. Le test aux sommets transforme ensuite « aucune nouvelle proposition n’est apparue » en un véritable certificat mathématique.

> [!WARNING]
> **Ce qui n’est pas prouvé.** Répéter un diagramme de puissance à partir des seules unions de voisins visibles, ou arrêter après quelques ordres stables, ne garantit pas d’avoir trouvé tous les événements. Une telle variante ne peut porter que le statut `conditional` tant que le certificat global n’est pas fermé.

## 7. Forme GPU-friendly du calcul mathématique

Cette section décrit des contrats de primitives, pas une architecture CUDA figée.

### 7.1 Lots de cellules indépendantes

La construction par découpage est naturellement orientée cellule. Pour un lot d’états $Q$ :

1. initialiser ou reprendre leur polytope courant ;
2. appliquer en parallèle les demi-espaces de puissance proposés ;
3. compacter les cellules vides ;
4. émettre les sommets non certifiés ;
5. lancer les top-$k$ exacts en lot ;
6. trier et dédupliquer les colonnes violatrices ;
7. répéter jusqu’à fermeture du lot.

Le travail de Taveira et al. sur les diagrammes de Voronoï et de puissance 3D confirme la pertinence pratique de trois briques : découpage indépendant des cellules convexes, élimination directionnelle conservative et parcours prioritaire d’un BVH adapté aux poids. Il ne faut cependant pas confondre cette primitive publiée avec le certificat d’ordre supérieur ci-dessus : notre ensemble de sites est implicite et le top-$k$ sur les observations joue le rôle d’oracle de séparation.

### 7.2 Insertion et suppression locales

L’ajout d’une colonne $R$ ne fait qu’ajouter une inégalité affine. Les cellules concernées sont celles dont le polytope courant rencontre le demi-espace où $R$ bat leur site. Elles sont découpées ; les autres restent inchangées. Une cellule devenue vide est supprimée. Cette monotonie est bien adaptée à :

- des files persistantes de cellules violées ;
- des mises à jour par lots après tri des identifiants de colonnes ;
- une reconstruction locale des signatures d’incidence ;
- une comptabilité explicite de la mémoire de géométrie transitoire par lot actif.

Le recalcul global demeure une solution de référence plus simple. L’insertion locale est une optimisation qui doit préserver exactement le même certificat aux sommets.

### 7.3 Tri et réduction des incidences

Chaque cellule fermée émet des signatures canoniques :

- une face par paire de labels minimisants et son support géométrique ;
- une arête par classe de trois labels ou par intersection de deux faces ;
- un sommet par classe complète de minimiseurs ex æquo.

Un tri suivi d’une réduction regroupe les émissions redondantes, reconstruit $\mathcal{A}(F)$ et distingue une face interne d’une face de $\partial\Omega$. Les candidats critiques sont ensuite triés par clé combinatoire `(interior_ids, boundary_ids)` avant les prédicats précis.

Cette réduction est également le bon endroit pour ne pas trianguler artificiellement une cellule non simpliciale. À l’ordre supérieur, une incidence de six cellules peut être une conséquence structurelle des poids barycentriques, même lorsque $X$ est générique ; elle ne doit pas être prise pour une erreur flottante.

### 7.4 Deux index, deux responsabilités

La proposition utilise deux recherches différentes.

| index | sites indexés | responsabilité |
|---|---|---|
| BVH de puissance | colonnes $(b_Q,\omega_Q)$ déjà connues | proposer les plans susceptibles de couper une cellule |
| LBVH top-$k$/shell/rang exact | observations originales $X$ | séparer contre tous les sous-ensembles implicites et certifier le rang |

Le premier index accélère. Le second décide. Un voisinage approché peut alimenter le premier, mais ne peut remplacer le second dans un statut `critical_complete`.

### 7.5 Ordonnancement inter-ordres

Le balayage reste séquentiel en $k$, car les colonnes de la récurrence dépendent des ordres précédents. À l’intérieur d’un ordre, toutes les opérations lourdes sont massivement parallèles. Les pré-calculs suivants se partagent entre ordres :

- top-$k$ exact prolongé jusqu’à la fin du shell au seuil $a_k$, et top-$K_{\max}+2$ pour rejeter un rang trop grand ;
- sommes et barycentres lors de l’extension $Q\mapsto Q\cup\{x\}$ ;
- codes spatiaux et BVH sur $X$ ;
- caches de rang pour les centres critiques dédupliqués ;
- prédicats filtrés réévalués seulement près de zéro.

## 8. Prédicats, égalités et dégénérescences

### 8.1 Filtres exacts requis

La géométrie rapide peut être calculée en `float32` ou `float64`, mais les décisions suivantes exigent un signe certifié :

- côté d’un plan de puissance ;
- dimension et orientation d’un support ;
- signes des coordonnées barycentriques de $c(U)$ ;
- appartenance de $c(U)$ à une face ;
- comparaison $\left\Vert c-x\right\Vert^2$ avec $\rho^2$ ;
- égalité et ordre de deux niveaux critiques.

Le schéma recommandé est un filtre flottant avec borne d’erreur, suivi d’une expansion adaptative ou d’une arithmétique exacte lorsque l’intervalle contient zéro. Les travaux classiques de Shewchuk donnent le modèle approprié pour les prédicats d’orientation et de sphère ; les prédicats pondérés doivent recevoir des filtres analogues.

### 8.2 Les ex æquo structurels ne doivent pas être perturbés hors de la sortie

Une perturbation symbolique peut imposer un ordre de calcul déterministe, mais elle ne doit pas effacer l’événement non perturbé. La sortie doit regrouper :

- toutes les cellules minimisantes au même lieu ;
- tout le shell frontal ;
- toutes les selles de même niveau avant union ;
- les attaches comme hyperévénement simultané.

En particulier, trianguler une incidence à six cellules puis conserver les diagonales arbitraires créerait de faux événements et pourrait binariser une fusion multifurquée.

### 8.3 Plus de quatre points cosphériques

En données dégénérées, le shell frontal $U_{\mathrm{full}}$ peut contenir plus de quatre points. Le centre critique appartient à $\mathrm{relint}\,\mathrm{conv}(U_{\mathrm{full}})$, et le théorème de Carathéodory assure l’existence d’un certificat de convexité utilisant au plus quatre points en dimension $3$. Il faut donc distinguer :

- un support minimal de taille au plus $4$, utilisé pour certifier le centre ;
- le shell frontal complet, utilisé pour le rang et l’hyperévénement.

Comme seuls les rangs fermés au plus $K_{\max}+1=11$ sont acceptés, un shell utile complet reste de taille au plus onze. Au-delà, le parcours de rang peut rejeter tôt le candidat.

Le lemme de fermeture des cellules reste exact sans $(\mathrm{GP}_{3})$. En revanche, l’extension du **catalogue de Morse** aux plateaux exige une définition stratifiée des événements simultanés et une preuve que la reconstruction par shells conserve toutes les incidences $H_0$. Tant que cette preuve et ces tests ne sont pas présents, un jeu qui viole $(\mathrm{GP}_{3})$ ne doit pas recevoir automatiquement le statut `critical_complete`, même si ses cellules de puissance sont fermées.

### 8.4 Données dupliquées

Des observations identiques produisent des distances nulles et des multiplicités combinatoires. Deux contrats sont possibles :

1. agréger les duplications en sites munis d’une multiplicité, puis développer les identifiants dans la sortie ;
2. conserver les identifiants distincts et traiter le rayon nul comme un lot initial.

Le choix doit être global au dépôt. Une perturbation silencieuse des duplications changerait la hiérarchie en ordre et n’est pas admissible.

## 9. Complexité honnête et sensibilité à la sortie

Notons, pour l’ordre $k$ :

- $M_k$ le nombre de cellules actives rencontrant $\Omega$ ;
- $P_k$ le nombre total d’incidences de faces, arêtes et sommets effectivement émises ;
- $W_k$ le nombre de sommets soumis à l’oracle de séparation, répétitions comprises ;
- $J_k$ le nombre de nouvelles colonnes distinctes insérées, qu’elles soient strictement violatrices ou co-minimisantes ;
- $A_k$ le nombre de tests conservatifs « colonne contre cellule » effectués après culling ;
- $L_k$ le nombre de découpages polyédriques effectivement exécutés ;
- $Z_k$ le nombre de sphères critiques acceptées à cet ordre de détection.

Si $T_{\mathrm{top}}(n,k,q)$ est le coût réel d’un lot de $q$ requêtes top-$k$ exactes avec prolongement de shell, la comptabilité pertinente est de la forme

$$T\leq\sum_{k=1}^{K_{\max}}\left(T_{\mathrm{top}}(n,k,W_k)+T_{\mathrm{cull}}(A_k)+T_{\mathrm{clip}}(L_k,P_k)+T_{\mathrm{sort}}(P_k)+T_{\mathrm{rank}}(n,Z_k)\right).$$

Cette écriture paramètre le coût par le travail réellement produit plutôt que par les $\binom{n}{k}$ labels possibles. Elle n’est pas, à elle seule, une borne output-sensitive : $A_k$, $L_k$, $P_k$ et $W_k$ peuvent être grands, et $J_k$ ne suffit pas à borner le nombre de couples colonne–cellule testés. Elle ne donne donc pas la borne souhaitée en fonction de $Z=\sum_k Z_k$ seulement.

### 9.1 Pourquoi un diagramme complet ne peut être facturé à la seule forêt $H_0$

En dimension $3$, une triangulation de Delaunay ordinaire peut avoir une complexité quadratique, tandis qu’une forêt de fusion $H_0$ ne retient qu’un nombre linéaire d’arêtes. Par conséquent, tout algorithme qui ferme et émet l’intégralité des incidences du diagramme de puissance à $k=1$ peut effectuer un travail quadratique alors que sa forêt finale est linéaire.

Cela ne prouve pas qu’un autre algorithme direct, fondé sur des coupes ou du branch-and-bound, ne puisse être sensible aux seuls événements utiles. Cela prouve que **la fermeture du diagramme complet ne possède pas cette propriété en général**.

### 9.2 Deux modes scientifiquement honnêtes

Le backend de recherche devrait exposer deux modes.

| mode | arrêt | garantie |
|---|---|---|
| `power_closed` | toutes les cellules et incidences ferment | catalogue critique complet |
| `power_budgeted` | budget de cellules, de sommets ou de temps atteint | événements retournés individuellement certifiés, complétude inconnue |

Le second mode peut prioriser les cellules produites par la cascade, les faces à petit rang et les supports presque bien centrés. Cette priorité est une heuristique ; elle n’autorise jamais le statut `critical_complete`.

Ces noms sont des certificats **internes** au backend 3D. Dans la taxonomie publique du dépôt, `power_budgeted` implique toujours `conditional`. `power_closed` autorise `critical_complete` sous $(\mathrm{GP}_{3})$, mais la hiérarchie ne devient `exact` qu’après fermeture des attaches globales et des incidences de lots ; sinon elle reste `conditional`. Le statut `atlas_exact` est réservé au backend d’atlas et n’est pas produit ici.

### 9.3 Hypothèse pratique à tester, pas théorème

Pour rendre falsifiable le scénario de faible latence sur des nuages volumétriques, $K_{\max}$ fixé et densité locale régulière, on peut poser l’hypothèse de travail suivante :

$$\mathbb{E}\left[\sum_{k=1}^{K_{\max}}(M_k+P_k+W_k+J_k)\right]=O(nK_{\max}).$$

Cette relation doit être traitée comme une **hypothèse expérimentale de sparsité de certificat**. Elle n’est ni démontrée ici, ni valable pour les nuages surfaciques ou les configurations de type courbe des moments. Elle ne suffit pas non plus à borner le temps sans hypothèse sur $A_k$ et $L_k$. La cible de moins d’une seconde pour $50\,000$ points ne devient crédible que si les mesures confirment ces deux régimes et si la fermeture, pas seulement le rappel des candidats, est effectivement atteinte.

## 10. Algorithme certifiant proposé

Le pseudo-code mathématique suivant sépare proposition, fermeture et extraction.

```text
PowerCascadeH0(X, Kmax, Ω)
    buckets[1] ← tous les singletons
    catalog ← événements de rang 1

    pour k = 1, ..., Kmax
        S ← dédupliquer(buckets[k])
        ajouter un label top-k témoin si S est vide

        répéter
            construire ou mettre à jour les cellules de puissance de S dans Ω
            V ← tous les sommets de cellules non encore certifiés
            shells ← exact_top_k_shell(X, V, k)
            missing ← tous les minimiseurs révélés qui ne sont pas dans S
            S ← S ∪ missing
        jusqu’à missing = ∅ et toutes les cellules certifiées

        apparier faces, arêtes et sommets par tri-réduction
        vérifier la fermeture des incidences et de la frontière de Ω

        pour chaque face de première génération de support U, 2 ≤ |U| ≤ 4
            calculer c(U), les barycentriques et ρ²
            certifier face, relative-intériorité, shell et rang
            si le rang fermé est ≤ Kmax+1
                insérer l’événement dédupliqué dans catalog

        pour chaque sommet de première génération (I, U), |U| = 4
            pour g dans {2, 3}
                pour chaque W dans C(U, g)
                    si |I| + g ≤ Kmax
                        buckets[|I| + g] ← buckets[|I| + g] ∪ {I ∪ W}

        libérer les polyèdres de l’ordre k

    retourner catalog et les certificats power_closed(1..Kmax)
```

Deux détails sont normatifs.

- La condition d’arrêt porte sur **tous les sommets des cellules mises à jour**, pas seulement sur les nouveaux sommets issus de la dernière coupe.
- `exact_top_k_shell` renvoie une classe complète d’ex æquo. Un seul label top-$k$ choisi par identifiant suffit à séparer une violation stricte, mais pas à reconstruire les événements.

## 11. Ce qui est démontré, conditionnel et ouvert

| énoncé | statut |
|---|---|
| représentation des domaines top-$k$ par les sites $(b_Q,\omega_Q)$ | identité exacte |
| certificat d’une cellule par top-$k$ exact à tous ses sommets | lemme exact |
| terminaison de la génération de colonnes | exacte, borne finie combinatoire |
| extraction d’un support $2$, $3$ ou $4$ depuis une face de première génération | exacte sous prédicats certifiés |
| suffisance des ordres $1$ à $K_{\max}$ pour les rangs au plus $K_{\max}+1$ | exacte |
| récurrence inter-ordres par cellules pleines de première génération | théorème connu si les mosaïques antérieures complètes sont consommées, sous hypothèses génériques non pondérées |
| complétude du catalogue quand tous les ordres sont fermés | exacte sous $(\mathrm{GP}_{3})$ et les contrats annoncés |
| reconstruction d’un plateau arbitrairement dégénéré | spécification donnée, preuve complète encore à rédiger |
| complétude après simple stabilisation heuristique | fausse comme règle générale |
| coût proportionnel au seul nombre d’événements $H_0$ | ouvert |
| temps inférieur à une seconde pour $n\simeq50\,000$ | objectif expérimental conditionné à la sparsité du certificat |

Le résultat mathématique à viser après cette annexe n’est donc pas « les diagrammes de puissance sont rapides ». Il est l’un des deux énoncés suivants :

1. une borne moyenne sur la taille du certificat $(M_k,P_k,W_k,J_k)$ et sur le travail de culling/clipping $(A_k,L_k)$ pour un modèle de nuage 3D explicitement défini ;
2. un oracle d’exclusion qui couvre les régions non visitées et prouve qu’elles ne contiennent aucun centre bien centré de rang au plus $K_{\max}+1$, sans fermer toutes leurs cellules.

Le second serait la véritable avancée sensible à la sortie $H_0$. Tant qu’il n’est pas obtenu, `PowerCascade` constitue la référence exacte GPU-friendly proposée ici, et `power_budgeted` sa variante faible latence au statut honnêtement conditionnel.

## 12. Comparaison aux pistes abandonnées

| piste | décision | raison |
|---|---|---|
| grille volumique | abandon pour la hiérarchie exacte | événements et coût dépendent de la résolution |
| *rhomboid tiling* explicite | abandon | structure 4D plus riche que la sortie $H_0$ |
| mosaïques d’ordre supérieur matérialisées | abandon | cellules non simpliciales et sortie intermédiaire potentiellement massive |
| diagramme de puissance ordinaire par ordre | retenu comme primitive | cellules convexes indépendantes, clipping et culling GPU |
| récurrence Edelsbrunner–Osang seule | retenue comme proposition | exacte sous ses hypothèses, mais insuffisante comme diagnostic numérique et pour un mode pruné |
| fermeture par top-$k$ aux sommets | retenue comme certificat | sépare contre tous les $k$-sous-ensembles implicites |
| listes locales de voisins de taille fixe | proposition seulement | aucune valeur universelle ne couvre les grandes sphères peu profondes |

## 13. Références vérifiées

- H. Edelsbrunner et G. Osang, [*A Simple Algorithm for Higher-order Delaunay Mosaics and Alpha Shapes*](https://arxiv.org/abs/2011.03617), 2020. L’article établit la représentation par Delaunay pondérée de premier ordre, la génération des sites des ordres futurs depuis les cellules de première génération et la complétude de cette récurrence sous ses hypothèses.
- B. Taveira, C. Lindström, M. Fatemi, L. Hammarstrand et F. Kahl, [*Scalable GPU Construction of 3D Voronoi and Power Diagrams*](https://arxiv.org/abs/2605.06408), SIGGRAPH 2026, [DOI 10.1145/3799902.3811229](https://doi.org/10.1145/3799902.3811229). Le calcul est orienté cellule, par clipping convexe et élimination directionnelle avec hiérarchie de volumes englobants.
- Y. Reani et O. Bobrowski, [*Morse Theory for the k-NN Distance Function*](https://arxiv.org/abs/2403.12792), 2024. Cette source donne le cadre de Morse et la caractérisation combinatoire-géométrique des points critiques de la distance K-NN.
- J. R. Shewchuk, [*Adaptive Precision Floating-Point Arithmetic and Fast Robust Geometric Predicates*](https://www.cs.cmu.edu/~quake/robust.html), *Discrete & Computational Geometry* 18, 1997. Cette référence justifie l’architecture filtre rapide puis précision adaptative pour les signes géométriques.

## 14. Conclusion normative

La voie 3D recommandée est `PowerCascadeH0` : une cascade $k=1,\ldots,10$ de cellules de puissance ordinaires, engendrées en flux, fermées par séparation top-$k$ exacte, puis immédiatement réduites à des sphères critiques classées. La cascade incrémentale vise à réduire le nombre de colonnes laissées à l’oracle ; seul le test aux sommets transforme cette initialisation en algorithme certifiant.

Le contrat de sortie doit rester explicite :

- `critical_complete` seulement si `power_closed(k)` vaut pour tous les ordres, si $(\mathrm{GP}_{3})$ s’applique et si les prédicats sont exacts ;
- `conditional` dès qu’un budget interrompt la fermeture ;
- événements individuellement certifiés dans les deux modes ;
- hiérarchie publique `exact` seulement après certification supplémentaire des attaches et des lots ;
- aucune prétention de complexité universelle proportionnelle à la forêt finale.

Cette proposition renonce effectivement aux structures d’ordre supérieur comme objets de calcul. Elle en conserve uniquement l’information mathématique indispensable, sous la forme la plus compatible identifiée à ce jour avec des primitives GPU massivement parallèles.
