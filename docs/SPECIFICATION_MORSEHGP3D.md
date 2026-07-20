# Spécification mathématique de MorseHGP3D

> [!IMPORTANT]
> Ce document fixe l'objet à calculer. Une optimisation n'a le droit de modifier ni cet objet, ni les niveaux, ni les inclusions entre ordres. Toute exécution publie son profil, ses hypothèses satisfaites et son statut de certification.

## 1. Verdict d'architecture

La bonne voie 3D n'est pas de lisser la hiérarchie puis d'espérer retrouver ses fusions. Elle consiste à exploiter trois propriétés propres à la dimension fixée et à $K_{\max}=10$ :

1. les changements de $H_0$ sont portés par des sphères critiques de rang fermé au plus onze;
2. leur support minimal contient au plus quatre observations en position générale;
3. les événements d'indice un sont exactement les simplexes de Gabriel utiles au graphe de facettes du manuscrit.

MorseHGP3D exact est donc défini par la chaîne de référence

$$\text{raffinement restreint certifié}\longrightarrow\text{catalogue critique}\longrightarrow\Gamma\text{ exhaustif}\longrightarrow\text{hyper-Kruskal gradué}.$$

Le flot de Gabriel, les descentes par miniball, la DTM, l'entropie et les voisinages approchés restent des voies de proposition, de connectivité positive ou de compression. Aucun ne remplace Gamma exhaustif dans la base exacte v2 sans une nouvelle preuve couvrant les incidences silencieuses.

Une seconde représentation combinatoire est désormais démontrée sur le papier : la [tour globale de boules saturées](math/TOUR_BOULES_SATUREES.md) engendre exactement le complexe de Čech, puis les composantes de Gamma, au moyen de simplexes implicites et d'une forêt d'intersections seuillée. Elle constitue une voie candidate indépendante, non un backend ni une base de preuve du contrat v2. Son prototype, sa persistance et son éventuelle migration contractuelle suivent une piste de recherche séparée; la chaîne active ci-dessus et la cible $K_{\max}=10$ restent inchangées.

## 2. Entrée et sémantique numérique

L'entrée mathématique initiale est un ensemble fini non vide de points distincts

$$X=\left\lbrace x_1,\ldots,x_n\right\rbrace\subset\mathbb{R}^{3}$$

et un ordre maximal $1\leq K_{\max}\leq10$. L'ordre maximal effectivement calculable est

$$K_{\mathrm{eff}}=\min(K_{\max},n).$$

Toutes les tranches, bornes de rang et tailles de tableaux utilisent $K_{\mathrm{eff}}$, jamais $K_{\max}$ seul. Les coordonnées IEEE-754 fournies par l'utilisateur sont interprétées comme des nombres dyadiques exacts. Les calculs approchés peuvent guider le GPU, mais toute décision combinatoire certifiante doit avoir le même résultat que l'arithmétique exacte sur ces dyadiques.

La première version certifiée peut exiger la position générale précisée à la section 12. Les doublons seront ensuite agrégés en sites munis de multiplicités; ils ne seront jamais séparés par un bruit silencieux. Les valeurs non finies sont refusées. Toute normalisation qui change les distances est hors contrat; une transformation de similarité explicitement demandée doit être enregistrée dans le certificat.

Tous les niveaux publics sont des rayons **au carré**. Pour un ensemble fini non vide $A\subseteq X$, on note

$$\beta(A)=\min_{y\in\mathbb{R}^{3}}\max_{x\in A}\left\Vert y-x\right\Vert^2,$$

et $c_A$ le centre unique de sa plus petite boule englobante.

Même si les coordonnées d'entrée sont dyadiques, le centre et le rayon carré d'un support de trois ou quatre points sont en général rationnels sans être dyadiques. Une expansion flottante ne représente pas la division exacte. Le format normatif d'un centre est donc homogène,

$$\mathrm{ExactCenter}=(C_x,C_y,C_z,D_c),\qquad D_c>0,\qquad c=(C_x/D_c,C_y/D_c,C_z/D_c),$$

et celui d'un niveau est une fraction canonique

$$\mathrm{ExactLevel}=(N,D),\qquad D>0,\qquad a=N/D,$$

avec signe du dénominateur fixé et facteurs communs supprimés. Deux niveaux sont comparés sans quotient par le signe de $N_1D_2-N_2D_1$; les tests de distance, de barycentriques et d'incidence utilisent de même les déterminants homogènes. FP32, FP64 et expansions ne sont que des filtres de signe avec bornes d'erreur. Une ambiguïté passe au fallback big-int ou rationnel, qui matérialise le témoin canonique; aucune décision exacte ne dépend d'un quotient flottant.

## 3. Objet continu : une tour ordre–échelle

Pour $y\in\mathbb{R}^{3}$, soient

$$a_1(y)\leq\cdots\leq a_n(y)$$

les distances carrées aux observations, avec multiplicité. On pose $D_k(y)=a_k(y)$ et

$$L_k(a)=\left\lbrace y:D_k(y)\leq a\right\rbrace.$$

L'ensemble $L_k(a)$ est la région couverte par au moins $k$ boules fermées de rayon $\sqrt{a}$ centrées sur $X$. Deux monotonies définissent la bifiltration :

$$a\leq b\Longrightarrow L_k(a)\subseteq L_k(b),\qquad k<\ell\Longrightarrow L_{\ell}(a)\subseteq L_k(a).$$

La sortie complète est le foncteur de composantes, pour $1\leq k\leq K_{\mathrm{eff}}$,

$$\mathcal{H}_X(k,a)=\pi_0\bigl(L_k(a)\bigr),$$

avec les applications induites par les deux types d'inclusion. Une représentation finie doit conserver les naissances, les multifusions, les niveaux exacts et les applications verticales entre ordres.

## 4. Modèle discret exact du manuscrit

Pour $1\leq k<n$, définissons le graphe filtré $\Gamma_k(a)$ :

- ses sommets sont les sous-ensembles $F\subseteq X$ de cardinal $k$ tels que $\beta(F)\leq a$;
- deux sommets $F$ et $F'$ sont adjacents lorsque $\lvert F\cup F'\rvert=k+1$ et $\beta(F\cup F')\leq a$.

La proposition 5 du manuscrit justifie la restriction aux adjacences élémentaires. Le théorème 2 établit une correspondance naturelle entre les composantes de $L_k(a)$ et les composantes de $\Gamma_k(a)$, vues comme unions d'identifiants d'observations. Pour $k\geq2$, ces unions peuvent se recouvrir : la sortie n'est pas une partition de $X$.

Ce modèle est un oracle conceptuel, pas une structure à matérialiser : il possède $\binom{n}{k}$ sommets potentiels.

Le cas terminal $k=n$ n'est pas supprimé du contrat : $\Gamma_n(a)$ est vide si $a<\beta(X)$, puis possède l'unique sommet $X$ et aucune arête si $a\geq\beta(X)$. Il représente l'unique composante éventuelle de $L_n(a)$. Il n'existe alors aucun événement d'indice un de rang $n+1$.

## 5. Catalogue critique partagé par les ordres

Pour une sphère de centre $c$ et de rayon carré $a$, posons

$$I(c,a)=X\cap B^{\circ}(c,\sqrt{a}),\qquad U(c,a)=X\cap\partial B(c,\sqrt{a}),$$

et son rang fermé

$$s(c,a)=\lvert I(c,a)\rvert+\lvert U(c,a)\rvert.$$

Sous les hypothèses de Reani–Bobrowski, $c$ est critique pour $D_k$ si et seulement si

$$c\in\mathrm{relint}\,\mathrm{conv}\bigl(U(c,a)\bigr),\qquad \lvert I(c,a)\rvert<k\leq s(c,a).$$

Pour ces ordres critiques, son indice vaut

$$\mu_k(c)=s(c,a)-k.$$

La fenêtre de rang est équivalente à $0\leq\mu_k(c)\leq\lvert U(c,a)\rvert-1$. Hors de cette fenêtre, la même boule peut encoder des faces de Čech, mais son centre n'est pas un point critique de $D_k$ au niveau $a$.

Le théorème local de Reani–Bobrowski associe à ce point critique la multiplicité

$$\Delta_{\mu}(c)=\binom{\lvert U(c,a)\rvert-1}{\mu}.$$

Cette quantité est un rang local, pas un nombre garanti de changements globaux. Pour $\mu=1$, le sous-niveau strict local possède les $\lvert U\rvert$ bras

$$F_u=(I\cup U)\setminus\lbrace u\rbrace,\qquad u\in U,$$

et un seul centre peut donc tuer jusqu'à $\lvert U\rvert-1$ classes de $H_0$, selon les composantes globales auxquelles ces bras s'attachent. En particulier, $\lvert U\rvert=3$ autorise une triple fusion en un événement. Une « selle » d'indice un n'est pas supposée binaire; l'hyperarête et les multifusions sont la représentation normative.

Une même sphère critique de rang $s$ donne donc, lorsque l'ordre appartient à $1,\ldots,K_{\mathrm{eff}}$ :

- une naissance de composante à l'ordre $k=s$, d'indice zéro, si $1\leq s\leq K_{\mathrm{eff}}$;
- un événement susceptible de fusionner des composantes à l'ordre $k=s-1$, d'indice un, si $2\leq s\leq\min(K_{\mathrm{eff}}+1,n)$.

Pour les tranches $1\leq k\leq K_{\mathrm{eff}}$, il suffit donc de cataloguer $s\leq s_{\max}=\min(K_{\mathrm{eff}}+1,n)$. En dimension trois et en position générale, le support frontal $U$ est affinement indépendant et $1\leq\lvert U\rvert\leq4$. Les événements non nuls ont donc une description de taille constante, même lorsque $k=10$.

Cette borne concerne les événements de Morse capables de modifier immédiatement $H_0$ aux ordres demandés. Elle ne borne pas une représentation globale de toutes les incidences de Čech : le saturé de la miniball d'une petite face peut contenir jusqu'à $n$ observations. La tour saturée doit donc distinguer le rang utile du catalogue critique, borné par $s_{\max}$, de la capacité d'un générateur saturé, potentiellement égale à $n$.

Un événement canonique stocke au minimum :

```text
CriticalEvent
    event_id
    closed_rank                 # 1..s_max
    interior_ids               # au plus 9 pour les événements utiles
    shell_ids                  # au plus 4 sous position générale
    minimal_support_ids         # canonique
    center_witness_homogeneous    # (Cx, Cy, Cz, Dc), Dc > 0
    squared_level_exact           # ExactLevel canonique (N, D), D > 0
    barycentric_signs
    degeneracy_class
    predicate_status
```

Le centre approché n'est jamais l'identité d'un événement. L'identité canonique est formée par le shell complet, l'ensemble intérieur, le support minimal et les témoins exacts.

## 6. Équivalence entre événements et simplexes de Gabriel

Soit une sphère critique de rang $s=k+1$ et posons $S=I\cup U$. Alors $\lvert S\rvert=k+1$, $c=c_S$, $\beta(S)=a$ et l'intérieur de la miniball de $S$ ne contient aucun point de $X\setminus S$. Ainsi, $S$ est un $k$-simplexe de Gabriel au sens du manuscrit.

Réciproquement, sous position générale, tout $k$-simplexe de Gabriel $S$ définit la sphère critique de sa miniball : son support minimal est bien centré et son rang fermé vaut $k+1$.

Cette bijection est le pont central entre Morse et HGP. Chaque événement de rang $k+1$ peut émettre :

- les $k+1$ facettes $S\setminus\lbrace x\rbrace$;
- une hyperarête de niveau $\beta(S)$ reliant ces facettes;
- l'union des observations de $S$;
- si $k+1\leq K_{\mathrm{eff}}$, pour `full_pi0` une ancre verticale de l'ordre $k+1$ vers l'ordre $k$; pour `hgp_reduced`, seulement un contrôle conditionnel lorsqu'un représentant source réduit existe déjà.

La clique de facettes définissant le graphe de Gabriel est remplacée par une étoile déterministe de $k$ unions. Les deux représentations ont exactement les mêmes composantes à tout seuil.

### 6.1 Voie candidate par boules saturées

Pour tout $Q\subseteq X$ non vide, définissons $\mathrm{Sat}(Q)=X\cap B_Q$, où $B_Q$ est sa miniball fermée. Les théorèmes S.1–S.6 de l'[audit dédié](math/TOUR_BOULES_SATUREES.md) établissent sans position générale :

1. $\beta(\mathrm{Sat}(Q))=\beta(Q)$;
2. les simplexes abstraits portés par tous les saturés actifs engendrent exactement le complexe de Čech;
3. à l'ordre $k$, les composantes de Gamma sont celles du graphe de générateurs dont les arêtes vérifient $\lvert S\cap T\rvert\geq k$;
4. une forêt couvrante de poids maximum, pondérée par $\lvert S\cap T\rvert$ et calculée par Kruskal décroissant, préserve simultanément toutes ces composantes après seuillage;
5. une sous-famille exactement validée produit une sous-filtration, avec une sémantique scientifique interne `partial_refinement` tant que son exhaustivité n'est pas certifiée; le schéma v2 ne sérialise pas encore cette voie.

Toute miniball en dimension trois possède un support minimal de taille au plus quatre; l'énumération exhaustive de ces supports suivie d'une classification fermée globale est donc mathématiquement complète, y compris lorsque le saturé a un rang élevé. Les shells cosphériques et supports multiples exigent néanmoins une canonicalisation distincte de celle du catalogue critique générique.

Cette voie encode des faces silencieuses à des ordres très inférieurs au rang du générateur. Ces activations descendantes sont des faits combinatoires de Čech/Gamma, pas automatiquement des événements de Morse : la multiplicité $\binom{\lvert U\rvert-1}{\lvert S\rvert-k}$ peut être nulle. De même, une forêt de générateurs à une coupe n'est pas le `MergeForest` normatif; la généalogie, les `coverage_delta`, les lots égaux et les morphismes verticaux doivent encore être construits et certifiés.

La voie candidate ne réutilise pas `CriticalEvent`. Un futur `SaturatedGenerator` conserverait une capacité jusqu'à $n$, sa boule exacte, son saturé et éventuellement plusieurs supports témoins. Aucun type public, aucune valeur de `backend` et aucune base de preuve nouvelle ne sont activés par la présente spécification v2.

## 7. Profil normatif `hgp_reduced`

Pour $k\geq2$, le profil `hgp_reduced` retourne la hiérarchie des K-polyèdres non réduits à une facette isolée. L'ordre un constitue une exception normative : `hgp_reduced` et `full_pi0` y coïncident et contiennent les $n$ feuilles singleton nées au niveau zéro. Cette convention est indispensable pour que $T_1$ soit exactement l'arbre de fusion du single-linkage et de l'EMST.

La base exacte v2 est définitionnelle : le backend `reference_cpu` énumère tous les sommets et toutes les cofaces de $\Gamma_k$ avec leurs niveaux $\beta$, traite chaque niveau exact en un lot, puis réduit la généalogie des composantes réduites à une facette isolée. À l'ordre un, le DSU est initialisé avec les $n$ racines ponctuelles de niveau zéro. Aux ordres $k\geq2$, une facette isolée est active dans Gamma mais ne devient pas encore une racine publique réduite. Pour chaque composante d'un lot, si $q$ est le nombre de racines publiques strictement antérieures, $q=0$ crée une naissance réduite dès que la composante devient non triviale, $q=1$ prolonge une racine et $q\geq2$ crée une multifusion. Toutes les facettes et incidences Gamma nouvellement vues sont activées dans les trois cas.

La forêt est accompagnée d'un journal `coverage_delta` : un lot avec $q=1$ peut ajouter des facettes et des observations sans changer la topologie de la forêt. Le DSU ne peut écarter une hyperarête comme pleinement redondante que si toutes ses facettes sont déjà actives dans la même racine et si son delta est vide.

Le contrat ne promet pas la généalogie publique d'une facette isolée avant son absorption, mais le calcul exact conserve son incidence interne dans Gamma. La proposition 6 et le théorème 5 du manuscrit ne sont plus une base autorisée : la fixture générique `gabriel-point-set-counterexample-5-points-v1` montre qu'une coface non-Gabriel peut attacher silencieusement une facette qui devient décisive lors d'une fusion ultérieure.

Le flot Gabriel brut ne contient que des cofaces de Gamma et fournit donc une relation positive de connectivité sans inventer de connexion. Il peut manquer ou retarder des connexions; il émet `proof_basis=gabriel_positive_connectivity`, `forest_semantics=partial_refinement`, `require_exact=false` et un statut `conditional` ou `budget_exhausted`. Une future réduction complétée en incidences devra recevoir une preuve, une base de preuve nouvelle et des tests dédiés avant toute promotion.

La [preuve des incidences silencieuses](math/INCIDENCES_SILENCIEUSES_GAMMA.md) ferme la partie combinatoire : sous support essentiel unique, une coface non-Gabriel ne peut produire qu'une attache $q=1$ sans nouveau point, et une étoile vers ses facettes simultanées restitue Gamma si toutes les cofaces sont connues. Elle ne ferme pas la partie algorithmique scalable, qui doit encore découvrir et certifier toutes les facettes-portes et leurs premiers niveaux d'incidence. La base contractuelle future reste donc désactivée.

## 8. Profil cible `full_pi0`

Le profil `full_pi0` retourne toutes les composantes de $L_k(a)$, y compris celles représentées temporairement par une seule facette. Il utilise le catalogue critique comme complexe de Morse comprimé :

1. chaque événement de rang $k$ crée un minimum de la tranche $k$;
2. chaque événement de rang $k+1$ fournit les germes locaux de ses bras strictement sous son niveau;
3. un oracle d'attache relie chaque germe à un minimum ou à une composante antérieure;
4. les événements de même niveau sont contractés simultanément;
5. seules les attaches entre composantes distinctes créent une fusion.

> [!WARNING]
> La caractérisation locale des événements ne décide pas seule si un point d'indice un tue une classe de $H_0$ : cette décision est globale. Des attaches accompagnées de chemins sous-niveau certifiés, ou déterminées par une structure de composantes déjà prouvée complète, sont nécessaires mais ne suffisent à publier `exact` qu'après fermeture de M.1.

L'énoncé de reconstruction que l'implémentation devra satisfaire est le suivant.

> **Obligation de preuve M.1 — reconstruction complète.** Sous position générale, si le catalogue contient tous les événements de rang au plus $s_{\max}$, si les $\lvert U\rvert$ bras de chaque point d'indice un sont tous attachés à leur composante de $\left\lbrace D_k<a\right\rbrace$, et si tous les centres d'un même niveau exact sont traités dans un seul lot respectant leur multiplicité locale, alors la tour de forêts produite représente $\mathcal{H}_X(k,a)$ pour $1\leq k\leq K_{\mathrm{eff}}$.

M.1 possède actuellement le statut `proof_obligation`. Les résultats de Reani–Bobrowski caractérisent les événements locaux, y compris la multiplicité $\binom{\lvert U\rvert-1}{\mu}$, mais la documentation active ne contient pas encore la preuve globale requise pour convertir simultanément toutes les attaches en morphismes de $\pi_0$, notamment lorsque plusieurs centres ont le même niveau. Un accord exhaustif avec l'oracle est nécessaire mais ne remplace pas cette preuve. Jusqu'à fermeture de M.1, `full_pi0` reste un profil de recherche qui ne peut pas publier le statut `exact`; `hgp_reduced` exact reste limité au Gamma exhaustif du backend de référence.

## 9. Attaches : flot de Gabriel et descente

Trois mécanismes distincts sont conservés.

### 9.1 Gamma exhaustif

Pour `hgp_reduced` exact, toutes les facettes et cofaces de Gamma sont énumérées. Les composantes pré-lot constituent l'oracle global, y compris lorsque l'ajout d'une coface ne change pas immédiatement leur union de points. Ces incidences silencieuses restent actives et peuvent participer à un lot futur.

### 9.2 Flot de Gabriel brut

Pour un événement Gabriel $S=I\cup U$, les bras pré-lot sont les facettes strictes $S\setminus\lbrace u\rbrace$, $u\in U$. Le lot active toutes les facettes $S\setminus\lbrace x\rbrace$, $x\in S$, mais il ne voit pas les cofaces non-Gabriel. Cette voie certifie seulement que les connexions émises appartiennent à Gamma; elle ne certifie ni leur exhaustivité, ni les temps de naissance ou de fusion de la hiérarchie HGP.

### 9.3 Descente K-NN–miniball

Pour une facette $F$ de cardinal $k$, soit $c_F$ le centre de sa miniball et posons

$$\mathrm{succ}(F)\in\mathcal{N}_k(c_F),$$

où $\mathcal{N}_k(c_F)$ est la famille exacte des choix top-$k$ au centre. Sous support essentiel et absence de point extérieur sur la sphère, si $F$ n'est pas actif à son propre centre, on peut choisir un successeur tel que

$$\beta\bigl(\mathrm{succ}(F)\bigr)<\beta(F).$$

La préclassification certifiée au centre conserve séparément la boule globale et toute la famille top-$k$. Si $I_X(F)$ et $U_X(F)$ désignent respectivement les observations strictement intérieures et sur la frontière de la miniball de $F$, on pose $I_{\mathrm{ext}}(F)=I_X(F)\setminus F$ et $U_{\mathrm{ext}}(F)=U_X(F)\setminus F$. Le mot « extérieur » dans $U_{\mathrm{ext}}$ signifie extérieur à la facette, pas extérieur à la boule. Une facette est active à son propre centre exactement lorsque $F\in\mathcal{N}_k(c_F)$, ce qui équivaut ici à $I_{\mathrm{ext}}(F)=\varnothing$. Cette décision porte sur la famille complète : elle ne se déduit ni de l'égalité de $F$ avec un choix canonique, ni de la seule égalité entre le cutoff top-$k$ et $\beta(F)$.

Dans le domaine strict, le shell interne de la facette doit coïncider avec son support positif unique et $U_{\mathrm{ext}}(F)$ doit être vide. La préclassification est alors fail-closed : une facette régulière active retourne `already_active_at_own_center`; une facette régulière inactive retourne `strict_descent_admissible`; un support non essentiel, plusieurs supports optimaux ou un shell extérieur non vide retournent `unsupported_degeneracy`, tout en conservant séparément le fait exact d'activité. Pour une facette régulière inactive, tout $G\in\mathcal{N}_k(c_F)$ omet au moins un point du support essentiel et n'ajoute que des points strictement intérieurs; l'unicité de la miniball et la relative-intériorité stricte du support donnent donc universellement $\beta(G)<\beta(F)$. Un shell top-$k$ multivalué n'est pas à lui seul un plateau lorsque cette preuve s'applique.

Cette préclassification ne construit aucun successeur. Lorsque les préconditions strictes échouent, elle certifie seulement que la décroissance n'est pas établie; l'existence d'un arc de plateau $\beta(G)=\beta(F)$ ne peut être décidée qu'après construction de $G$ et calcul exact de sa miniball. Elle ne certifie donc encore ni segment sous-niveau, ni DAG, ni pointer-jumping, ni attache, ni forêt, ni statut public.

Le jalon préparatoire d'arc choisi ne s'ouvre que pour `strict_descent_admissible`. Il pose alors $G$ égal à `canonical_choice_ids` de la partition top-$k$ déjà certifiée, vérifie $G\in\mathcal{N}_k(c_F)$ et $G\neq F$, puis recalcule intégralement la miniball de $G$. Le certificat exige $\beta(G)\leq D_k(c_F)\leq\beta(F)$ et, séparément, $\beta(G)<\beta(F)$. La dernière inégalité est une comparaison rationnelle fraîche : le logiciel ne se contente pas de l'implication conditionnelle issue des préconditions.

Les sources `already_active_at_own_center` et `unsupported_degeneracy` produisent respectivement `no_arc_already_active_at_own_center` et `no_arc_unsupported_degeneracy`; elles ne portent aucun identifiant de successeur, aucune miniball cible et aucune comparaison de niveau. Si une source `strict_descent_admissible` fraîchement rejouée produit $G=F$ ou $\beta(G)\geq\beta(F)$, l'exécution échoue fermée : elle ne requalifie pas cette contradiction en plateau pris en charge. La portée `canonical_top_k_selected_strict_level_arc_only`, fondée sur `exact_descent_preconditions_canonical_top_k_member_fresh_miniball_strict_level_v1`, ne certifie encore aucun segment, DAG, pointer-jumping, attache, forêt ou statut public.

Le certificat préparatoire de segment rejoue 6.3 et n'émet un témoin que pour un arc strict. Pour $R=\beta(F)$, il recalcule $a=g_G(c_F)=D_k(c_F)\leq R$, lit $b=\beta(G)<R$ depuis la miniball cible et calcule $\delta=\left\Vert c_G-c_F\right\Vert^2\geq0$. Pour chaque $x\in G$ et $\gamma(t)=(1-t)c_F+t c_G$, l'identité quadratique exacte est

$$\left\Vert\gamma(t)-x\right\Vert^2=(1-t)\left\Vert c_F-x\right\Vert^2+t\left\Vert c_G-x\right\Vert^2-t(1-t)\delta.$$

Son passage au maximum donne seulement la borne d'enveloppe

$$g_G(\gamma(t))\leq q(t)=(1-t)a+tb-t(1-t)\delta.$$

L'inégalité peut être stricte parce que le point maximisant peut changer; présenter $g_G(\gamma(t))=q(t)$ comme une identité générale est interdit. La décomposition $q(t)-R=(1-t)(a-R)+t(b-R)-t(1-t)\delta$ certifie le segment fermé dans $\left\lbrace D_k\leq R\right\rbrace$ et le demi-segment $\gamma((0,1])$ dans $\left\lbrace D_k<R\right\rbrace$. Le point source peut satisfaire $a=R$. Si $\delta=0$, les centres sont exactement égaux et $a=b$; ce segment dégénéré reste valide. Les branches sans arc n'émettent aucun témoin de segment.

La portée `canonical_strict_arc_half_open_sublevel_segment_only`, fondée sur `exact_squared_distance_chord_identity_max_envelope_half_open_segment_v1`, ne certifie qu'un segment isolé. Elle ne certifie ni concaténation, ni DAG, ni pointer-jumping, ni identification d'un germe, ni attache, ni forêt ou statut public.

Le jalon de chaîne 6.5 itère cette transition canonique depuis une seule facette. Si $F_0,\ldots,F_L$ sont les facettes engagées, chaque segment rejoue exactement $\beta(F_{i+1})<\beta(F_i)$ à cardinal fixé $k$. Aucune facette ne peut donc se répéter et $L\leq\binom{n}{k}-1$. Chaque couture exige l'égalité exacte de la facette, du centre rationnel et du niveau de miniball entre la cible précédente et la source fraîchement reconstruite; elle ne confond pas ce niveau avec la valeur atomique suivante $D_k(c_{F_{i+1}})$, qui peut être plus petite.

Pour $R_0=\beta(F_0)$, la concaténation est contenue dans $\left\lbrace D_k\leq R_0\right\rbrace$ et, après retrait de son premier point, dans $\left\lbrace D_k<R_0\right\rbrace$. Un arrêt régulier actif termine cette orbite canonique. Un arrêt sur dégénérescence non prise en charge ou sur budget explicite ne certifie que le préfixe engagé; le dernier probe complet conserve la raison d'arrêt et le prochain segment strict éventuel. Le booléen `finite_strict_facet_orbit_theorem_certified` porte sur l'existence de la borne combinatoire, pas sur une couverture exhaustive par le budget effectif. La portée `single_source_canonical_strict_descent_chain_only`, fondée sur `exact_replayed_half_open_segments_exact_seams_strict_facet_potential_finite_orbit_v1`, ne certifie encore ni segment initial d'un germe critique, ni fermeture de plusieurs sources en DAG, pointer-jumping, attache, forêt ou statut public.

Le segment initial d'un bras demande un contrat distinct. Pour un support critique complet $U$, sa partition globale $S=I\cup U$, l'ordre $k=\lvert S\rvert-1$ et $u\in U$, posons $F_u=S\setminus\lbrace u\rbrace$, $a=\beta(S)$, $b_u=\beta(F_u)$ et $d_u=c_{F_u}-c$. Si $2\leq\lvert U\rvert\leq4$, si $U$ est affinement indépendant, si $c\in\mathrm{relint}\,\mathrm{conv}(U)$ et si aucun point extérieur ne complète le shell, alors $b_u<a$, $\left\Vert d_u\right\Vert^2>0$ et le coefficient $2(c-u)\mathbin{\cdot}d_u$ est strictement positif. L'enveloppe quadratique certifie tout le segment privé de $c$ sous $a$.

Pour $p\in X\setminus S$, définissons $A_p=\left\Vert c-p\right\Vert^2-a>0$ et $B_p=2(c-p)\mathbin{\cdot}d_u$. La borne $\tau_u=\min\left(\left\lbrace1\right\rbrace\cup\left\lbrace\frac{A_p}{-2B_p}:p\in X\setminus S,\ B_p<0\right\rbrace\right)$ maintient chaque extérieur au-dessus de $a$ pour $0<t\leq\tau_u$. Sur ce préfixe, les points strictement sous $a$ sont exactement ceux de $F_u$ : le germe omettant $u$ est donc identifié, même si un point extérieur entre plus tard avant $c_{F_u}$. Le raccord à 6.5 exige l'égalité exacte de $F_u$, de $c_{F_u}$ et de $\beta(F_u)$; son budget ne compte jamais ce segment initial. Les portées `single_index_one_critical_arm_initial_germ_segment_only` et `single_index_one_critical_arm_plus_canonical_strict_chain_only` restent mono-bras et ne certifient ni l'ensemble des $\lvert U\rvert$ bras, ni racine pré-lot, attache globale, DAG, forêt ou statut public.

Le jalon préparatoire 6.7 ferme seulement la famille événement-locale. Il canonise le shell critique complet $U$, énumère une fois chaque $u\in U$ et rejoue indépendamment 6.6 pour $F_u$ avec le même budget de chaîne. Tous les bras doivent reconstruire exactement la même source critique. Lorsqu'un bras atteint une facette régulière active, son label terminal est la suite canonique de ses `PointId`; son témoin exact doit rester strictement sous le niveau critique. Les labels terminaux égaux sont regroupés en classes d'identité de facettes, ordonnées canoniquement, tout en conservant la provenance des points $u$ retirés.

Cette partition n'a une portée complète que si tous les bras finissent sur une facette régulière active. Une source non prise en charge ne produit aucune famille certifiée; toute dégénérescence terminale, tout épuisement du budget ou leur combinaison rend la partition terminale incomplète, même si les labels déjà observés restent rejouables. Une facette terminale active n'est pas une racine globale. Deux labels distincts peuvent appartenir à la même composante de Gamma, et une classe d'identité de labels n'est ni une composante de Gamma, ni une attache. La portée exacte `all_index_one_critical_arms_independent_canonical_strict_chains_terminal_labels_only` ne produit donc ni racine, ni DAG global, ni forêt, ni `public_status`.

Le jalon préparatoire 6.8 construit séparément la coupe exhaustive strictement ouverte de Gamma. Les types `ExactStrictGamma*` acceptent un nuage canonique borné par $n\leq14$, un ordre $1\leq k\leq10$ avec $k<n$, un niveau de coupe $a$ et une à quatre facettes sources canoniques de cardinal $k$. Une facette $F$ est active exactement lorsque $\beta(F)<a$ et une coface $Q$ exactement lorsque $\beta(Q)<a$. Ni $\beta\leq a$, ni tolérance, ni epsilon numérique ne sont admis dans cette décision.

Avant toute miniball, toute activation ou toute union, le préflight calcule exactement $\binom{n}{k}$ facettes candidates, $\binom{n}{k+1}$ cofaces candidates et $k\binom{n}{k+1}$ tentatives d'union. Le budget doit couvrir simultanément ces trois nombres; sinon la décision `no_cut_preflight_budget_insufficient` ne publie aucune coupe partielle, aucun composant et aucune classification de source. Le préflight est donc tout-ou-rien.

Les miniballs des facettes de taille au plus dix sont calculées une seule fois puis conservées dans le catalogue exhaustif. Pour $k<10$, chaque coface possède au plus dix points et reçoit sa miniball directe. Pour $k=10$, une coface $Q$ de taille onze utilise l'identité exacte $\beta(Q)=\max_{q\in Q}\beta(Q\setminus\lbrace q\rbrace)$. En dimension trois, un support minimal de $Q$ contient au plus quatre points; au moins une suppression évite ce support et atteint donc $\beta(Q)$, tandis que toute facette de suppression est incluse dans $Q$ et ne peut avoir un niveau supérieur. Les onze niveaux nécessaires sont ainsi déjà présents dans le cache de facettes taille dix. Le témoin conserve le premier maximiseur lexicographique et vérifie que sa boule couvre aussi le point omis.

Le DSU exhaustif contient chaque facette active, y compris une facette isolée conformément au profil `full_pi0`. Chaque coface active réunit ses $k+1$ facettes de suppression par exactement $k$ tentatives d'union. Les composantes sont des familles canoniques de facettes, pas leurs seules unions de points. Chacune des une à quatre sources est ensuite classée indépendamment : une source satisfaisant $\beta(F)<a$ reçoit l'indice de sa composante exhaustive, tandis qu'une source inactive est explicitement signalée sans indice. Une exécution complète peut donc retourner `complete_all_sources_active_and_classified` ou `complete_with_inactive_sources` sans confondre inactivité et échec du calcul.

La base `exact_bounded_exhaustive_strict_gamma_full_pi0_source_component_classification_v1` et la portée `bounded_exhaustive_strict_gamma_full_pi0_source_components_only` certifient uniquement cette coupe bornée et la localisation de ses sources. Le flot de Gabriel ne peut jamais remplacer l'énumération des cofaces, notamment sur les incidences silencieuses. Un indice de composante n'est ni une racine hiérarchique, ni une attache de Morse, ni un nœud de DAG ou de forêt; 6.8 ne produit aucun `public_status` et ne revendique aucune scalabilité au-delà des bornes annoncées.

Notons $\Gamma_k^{<a}$ le graphe dont les facettes $F$ et les cofaces $Q$ sont actives exactement sous $\beta(F)<a$ et $\beta(Q)<a$. Ses composantes correspondent naturellement à celles du sous-niveau strict $L_k^{<a}=\left\lbrace y:D_k(y)<a\right\rbrace$. En effet, tout chemin fini dans $\Gamma_k^{<a}$ possède un niveau maximal $b<a$; le théorème 2 fermé le place dans une composante de $L_k(b)\subseteq L_k^{<a}$. Réciproquement, les sous-niveaux stricts de $D_k$ sont semi-algébriques et localement connexes par arcs; pour tout arc continu $\gamma:[0,1]\to L_k^{<a}$, la compacité donne $b=\max_{t\in[0,1]}D_k(\gamma(t))<a$, puis le théorème 2 fermé relie ses représentants dans $\Gamma_k(b)\subseteq\Gamma_k^{<a}$. Cette correspondance ouverte justifie une classification pré-événement, pas une mutation hiérarchique.

Le raccord borné 6.9 compose cette correspondance avec la famille complète 6.7. `build_exact_critical_arm_gamma_component_classification` dérive l'ordre $k$ et le niveau critique $a$ de la source 6.7 fraîchement rejouée, transmet exactement une facette source par classe de label terminal à 6.8, puis projette chaque classe et chaque bras vers sa composante de $\Gamma_k^{<a}$. Les labels terminaux égaux sont dédupliqués avant 6.8; des labels distincts restent distincts même lorsqu'ils sont ensuite regroupés dans la même composante Gamma. Une famille incomplète n'engage aucun calcul Gamma, et un préflight Gamma insuffisant ne publie aucune projection partielle.

Le vérificateur 6.9 reconstruit 6.7 et 6.8 depuis le nuage, le shell et les deux budgets fiables, sans accepter d'ordre, de niveau ou d'indice de composante observé comme entrée. La base `exact_complete_critical_arm_family_strict_path_bounded_exhaustive_open_gamma_component_classification_v1` et la portée `bounded_complete_critical_arm_family_to_exhaustive_strict_gamma_components_only` certifient uniquement les composantes strictement antérieures incidentes à un événement isolé pris en charge. Elles ne créent ni racine, ni fusion, ni attache publique, ni DAG, ni forêt, ni `public_status`; tous les événements d'un même niveau doivent encore être contractés simultanément sur le même état pré-lot figé.

Le jalon 6.10 construit cette transition de coupe indépendamment des événements critiques. `build_exact_gamma_equal_level_transition` reprend les bornes $2\leq n\leq14$, $1\leq k\leq10$ avec $k<n$ et le préflight atomique de 6.8. Il rejoue exhaustivement $\Gamma_k^{<a}$, catalogue séparément chaque facette et chaque coface de niveau exactement $a$, puis reconstruit $\Gamma_k^{\leq a}$ avec toutes les incidences strictes et égales. Une coface $Q$ de niveau $a$ ne peut référencer qu'une facette de suppression de niveau inférieur ou égal à $a$, par monotonie de la miniball.

Soient $C_i$ les composantes de $\Gamma_k^{<a}$. Le système de jetons est formé des $C_i$ et de chaque facette $F$ telle que $\beta(F)=a$. Pour toute coface $Q$ telle que $\beta(Q)=a$, chacune de ses facettes de suppression est remplacée soit par son unique jeton $C_i$ si son niveau est strict, soit par son propre jeton $F$ si son niveau vaut $a$; les jetons ainsi obtenus forment une hyperincidence. Les composantes de cet hypergraphe sont exactement les composantes fermées affectées : tout chemin fermé se contracte en alternant sous-chemins stricts et hyperincidences égales, et tout chemin de jetons se relève en un chemin de $\Gamma_k^{\leq a}$. Une facette égale isolée forme donc un groupe avec $q=0$, une croissance dans une seule composante stricte donne $q=1$ et une coalescence de plusieurs composantes strictes donne $q\geq2$; une composante stricte non touchée reste inchangée et n'émet aucun groupe.

Le rejeu frais certifie les deux catalogues, chaque incidence sous la forme exclusive composante stricte ou facette nouvelle, la partition fermée, la projection stricte--fermée, les groupes et leurs compteurs exacts. La base `exact_bounded_exhaustive_gamma_strict_to_closed_equal_level_simultaneous_transition_v1` et la portée `bounded_exhaustive_gamma_equal_level_transition_only` ne construisent aucune racine persistante, naissance réduite, fusion publique, attache verticale, preuve M.1, forêt ou `public_status`. Les catégories $q$ sont des diagnostics exhaustifs de la coupe Gamma; leur conversion hiérarchique reste un jalon ultérieur.

Le jalon 6.11 superpose seulement une liste fournie d'événements critiques pris en charge aux groupes exhaustifs 6.10. `build_exact_supplied_critical_event_gamma_overlay` canonise les shells, rejoue indépendamment 6.9 pour chacun d'eux et exige que toutes les familles de bras soient complètes. Il dérive ensuite un unique couple commun $(k,a)$; si les ordres ou les niveaux exacts diffèrent, aucun couple commun, aucune transition et aucune projection ne sont publiés. Le préflight externe borne atomiquement le nombre d'événements fournis et le nombre total de bras avant tout calcul géométrique. La liste fournie n'est jamais présentée comme le catalogue critique complet du niveau.

Pour un événement accepté, soit $S=I\cup U$ sa partition critique globale, avec $\lvert S\rvert=k+1$ et $\beta(S)=a$. Pour chaque $u\in U$, la facette $S\setminus\left\lbrace u\right\rbrace$ est exactement la facette initiale du bras 6.9, strictement active avant $a$, et son jeton 6.10 doit être la même composante stricte. Pour chaque $i\in I$, la facette $S\setminus\left\lbrace i\right\rbrace$ contient toujours le support critique positif $U$ et reste dans sa boule fermée; sa miniball vaut donc exactement $a$ et elle doit apparaître comme facette nouvellement active. Ces deux familles partitionnent les $k+1$ suppressions de la coface égale $S$, qui appartient ainsi à un unique groupe simultané 6.10.

Chaque projection porte un indice dans l'ordre canonique des requêtes, jamais la position du demandeur. Chaque groupe 6.10 est conservé, même s'il ne reçoit aucun événement fourni; il expose séparément ses indices canoniques fournis et toutes ses cofaces égales sans provenance fournie. Le booléen de provenance est donc existentiel et ne certifie pas que le groupe, ni le niveau, possède un catalogue critique complet. La base `exact_supplied_complete_critical_arm_gamma_event_cofaces_reconciled_with_exhaustive_equal_level_gamma_transition_v1` et la portée `bounded_supplied_equal_order_level_complete_critical_events_to_exhaustive_gamma_transition_groups_only` ne créent ni racine persistante, ni naissance ou fusion publique, ni attache verticale, ni couverture M.1, ni DAG, ni forêt, ni `public_status`.

Le jalon 6.12 fournit cette première autorité exhaustive sur le domaine borné $1\leq n\leq14$, $1\leq K_{\max}\leq10$. `build_exact_critical_catalog` préflight atomiquement $C(n)=\sum_{j=1}^{\min(4,n)}\binom{n}{j}$ supports et $nC(n)$ classifications ponctuelles, soit au plus 1470 et 20580. Après succès seulement, il énumère chaque support canonique de cardinal un à quatre, classe exactement sa dépendance affine et la position de son circumcentre, puis appelle la partition globale fermée uniquement pour un support minimal strictement positif.

Pour un tel support $U$, la pertinence d'un shell supplémentaire est décidée par $\lvert I\rvert+\lvert U\rvert\leq s_{\max}$, même si le rang fermé observé $\lvert I\rvert+\lvert S\rvert$ dépasse $s_{\max}$. Toute égalité est conservée dans un enregistrement dédupliqué avec tous ses supports sources; seule une égalité pertinente invalide le certificat générique. Un événement est accepté exactement lorsque le shell global vaut $U$ et que son rang fermé ne dépasse pas $s_{\max}$. Les événements sont triés par niveau, rang, intérieur, shell, support et centre, puis regroupés en lots H0 canoniques $(k,a)$ avec rôle de naissance au rang $k$ et rôle de selle au rang $k+1$, lorsqu'ils appartiennent à la fenêtre effective.

Le rejeu frais reconstruit supports, classifications, partitions, dégénérescences, événements, indices et lots uniquement depuis le nuage, $K_{\max}$ et le budget fiable. La base `exhaustive_exact_supports_up_to_four_global_closed_ball_critical_catalog_h0_batches_v1` et la portée `bounded_n14_k10_exhaustive_supports_up_to_four_critical_catalog_h0_batches_only` ne raccordent encore aucun événement à 6.9 ou 6.10 et ne créent ni racine persistante, ni naissance ou fusion publique, ni attache verticale, ni M.1, ni DAG, ni forêt, ni `public_status`.

Le jalon 6.13 projette directement la transition Gamma exhaustive 6.10 vers la sémantique locale `hgp_reduced` d'un seul niveau exact. `build_exact_reduced_gamma_batch` accepte $2\leq k<n\leq14$ avec $k\leq10$, le niveau $a$ et les trois budgets fiables de Gamma. Le cas terminal $k=n$, dont le Gamma réduit est vide pour $n>1$, ne possède pas de transition égalitaire 6.10 et reste délibérément hors de ce contrat local.

Pour $k\geq2$, une composante de $\Gamma_k^{<a}$ porte une racine réduite antérieure si et seulement si elle est non triviale, c'est-à-dire si elle contient plus d'une facette. Cette propriété équivaut à l'incidence avec au moins une coface stricte : toute telle coface réunit plusieurs facettes, et toute composante de plusieurs facettes contient un pas porté par une coface. Les composantes réduites antérieures sont donc reconstruites sans identifiant persistant; les composantes réduites à une facette isolée restent actives dans Gamma, mais sont omises comme racines.

Chaque groupe 6.10 contenant une coface égale est classé par le nombre $q_R$ de composantes strictes non triviales qu'il absorbe : $q_R=0$ donne une naissance réduite, $q_R=1$ une continuation et $q_R\geq2$ une multifusion. Les composantes strictes isolées absorbées restent des témoins exhaustifs, jamais des parents. Une facette de niveau égal sans coface reste différée, sans racine ni delta. Pour tout groupe non différé, le delta de couverture est la famille des facettes de la composante fermée moins l'union des facettes des parents, et l'union de ses points moins l'union des points de ces mêmes parents; `fully_redundant` vaut vrai exactement lorsque ces deux différences sont vides. Ce drapeau qualifie seulement la couverture : il ne supprime jamais le groupe topologique, en particulier une multifusion éventuelle.

`verify_exact_reduced_gamma_batch` rejoue fraîchement 6.10 puis reconstruit classifications strictes, groupes, deltas, faits, compteurs, décision et portée. La base `exact_bounded_exhaustive_gamma_strict_nontrivial_component_reduction_and_equal_level_batch_semantics_v1` et la portée `bounded_exhaustive_gamma_single_equal_level_hgp_reduced_semantics_orders_two_to_ten_with_k_less_than_n_only` certifient seulement cette projection locale. Elles ne construisent ni identifiant de racine persistant, ni généalogie entre niveaux, ni DAG, ni forêt, ni attache verticale, ni certificat M.1, ni réduction de plateau, ni `public_status`.

Le jalon 6.14 déroule désormais ces lots sur tous les niveaux exacts d'un ordre fixé. `build_exact_persistent_reduced_gamma_order_history` accepte $2\leq k\leq\min(10,n)$ et $2\leq n\leq14$. Pour $k<n$, il calcule d'abord le diamètre exact $D^2$ du nuage et construit une unique coupe Gamma strictement ouverte au niveau $2D^2$. Toute facette ou coface $Q$ vérifie $\beta(Q)\leq D^2$, car une boule centrée en l'un de ses points et de rayon carré au plus $D^2$ la contient; la coupe haute catalogue donc strictement les $F=\binom{n}{k}$ facettes et les $C=\binom{n}{k+1}$ cofaces. Le tri exact et la déduplication de leurs niveaux donnent tout le sweep, y compris les niveaux où seules des facettes s'activent.

Le préflight est atomique avant toute distance ou miniball. Avec $U=kC$ et $L\leq F+C$, il borne les travaux logiques par $(L+1)F$, $(L+1)C$ et $(L+1)U$; ces nombres comptent les espaces candidats sémantiques, pas les parcours physiques internes des rejeux 6.8, 6.10 et 6.13. Sur le domaine annoncé, $F,C\leq3432$, $U\leq21021$, $L\leq6435$, les trois travaux sont respectivement bornés par 22088352, 22088352 et 135291156, et le diamètre demande au plus 91 paires. Le journal compact contient au plus $F+C$ groupes, $C$ nœuds, $C-1$ références d'enfants, $C-1$ références de racines antérieures, $F$ facettes nouvellement actives, $C$ cofaces égales, $F$ facettes de delta et $kF\leq24024$ références de points de delta. Les résultats 6.13 complets sont transitoires et ne sont jamais accumulés dans l'historique.

À chaque niveau, toutes les racines sont appariées aux composantes strictes non triviales par leur ensemble complet de facettes, puis tous les groupes sont résolus sur ce snapshot immuable. Pour un groupe non différé, l'état résultant est rejoué comme l'union canonique des états antérieurs et de son delta; facettes, points couverts et cofaces du niveau sont comparés au lot 6.13 transitoire avant une mutation simultanée. Une naissance ou une multifusion crée exactement un nœud dense; les racines antérieures d'une multifusion sont ses enfants, une continuation conserve exactement son identifiant et une facette isolée différée ne crée ni racine, ni nœud, ni delta. Chaque groupe non différé conserve exactement un delta, même vide, et `fully_redundant` ne supprime jamais son enregistrement. Chaque coface exhaustive est affectée une fois et chaque facette entre une fois dans les deltas; la branche normale finit en une racine couvrant les $F$ facettes et les $n$ points.

Le cas terminal $k=n\leq10$ est une sortie complète distincte : $F=1$ mais $C=U=L=0$, aucun calcul géométrique ne démarre et coupe haute, niveaux, groupes, nœuds, deltas et racines restent absents. La fixture permanente $n=k=2$ interdit donc d'appliquer abusivement l'identité normale « somme des facettes ajoutées = $F$ » ou de former $C-1$. La persistance certifiée signifie seulement que les identifiants locaux survivent entre les lots de ce sweep; elle ne signifie ni stockage durable, ni checkpoint. La base `exact_bounded_exhaustive_gamma_all_exact_levels_persistent_reduced_root_genealogy_v1` et la portée `bounded_n14_k10_single_order_persistent_hgp_reduced_gamma_history_including_empty_terminal_only` excluent toujours le raccord de provenance au catalogue 6.12, les identifiants SHA publics, `full_pi0`, l'attache verticale, M.1, le DAG global, le pointer-jumping, les plateaux, CUDA, G4, la scalabilité et tout `public_status`.

Le jalon 6.15 extrait une coupe réduite exacte d'un historique 6.14 déjà certifié, sans reprendre le nuage ni reconstruire Gamma. Pour les niveaux d'activation strictement croissants $a_0<\cdots<a_{L-1}$ et un seuil exact $a$, la frontière ouverte sélectionne le préfixe $p=\lvert\left\lbrace i:a_i<a\right\rbrace\rvert$ par borne inférieure; la frontière fermée sélectionne $p=\lvert\left\lbrace i:a_i\leq a\right\rbrace\rvert$ par borne supérieure. Seuls des lots entiers sont rejoués. Un lot différé ou entièrement redondant peut donc ne changer aucune racine tout en avançant exactement le curseur.

Avant la sélection, un audit global borné de la source vérifie les $L$ niveaux, métadonnées, nœuds, groupes, labels, références et l'arithmétique scalaire de la forêt complète. Cet audit de défense en profondeur est explicitement séparé du budget de rejeu du préfixe : ses compteurs et son scratch borné restent visibles même pour une coupe vide. Le préflight propre à la coupe parcourt ensuite le seul préfixe sans construire de payload de facettes et calcule exactement lots, groupes, nœuds, références antérieures et d'enfants, activations, deltas, pic de racines, tailles de sortie et travail de rejeu. Si $Q_p$ est le nombre de groupes non différés, $A_p$ le nombre final de racines actives, $N_p$ le nombre de nœuds, $R_p$ le nombre de références de racines antérieures et $H_p$ le nombre de références d'enfants, la forêt compacte vérifie $R_p=Q_p-A_p$ et $H_p=N_p-A_p$. Le nombre de références de facettes en sortie est exactement le nombre cumulé $D_{F,p}$ de facettes de delta; les références ponctuelles de sortie sont au plus $\min(nA_p,kD_{F,p})$.

La taille finale ne borne pas seule le coût d'une suite de continuations. Le budget compte donc aussi $W_{F,p}$, somme, pour chaque groupe non différé inclus, des facettes de ses racines antérieures et de son delta. Les racines antérieures d'un même lot étant distinctes par facette, $W_{F,p}\leq(F+C)F\leq22084920$ sur le domaine borné; le scan ponctuel correspondant vérifie $W_{P,p}\leq kW_{F,p}\leq154594440$. Deux capacités supplémentaires comptent les vérifications d'incidence : chaque facette nouvellement active doit appartenir au delta et à la racine résultante, et les $k+1$ suppressions de chaque coface égale doivent appartenir à cette racine; elles sont bornées par 27456 labels de facettes et 192192 références de `PointId`. Après succès seulement, chaque lot est résolu sur un snapshot immuable, toutes ses mutations sont préparées, puis déplacées ensemble dans la table active; aucune copie complète de la table ni des racines préparées n'est nécessaire. Les créations de nœuds suivent exactement les plages denses des lots, toute racine appartient au préfixe déjà créé et le représentant canonique de chaque groupe est la première facette de son état.

Cette certification est explicitement conditionnelle. Le type d'entrée attendu est l'historique en mémoire retourné par le builder 6.14 et séparément accepté par son vérificateur; les plafonds de cardinalité ne bornent pas le nombre de limbs d'un `ExactLevel` forgé par désérialisation hostile. Sans nuage, ni appel du vérificateur 6.14, ni rejeu Gamma, un journal structurellement cohérent mais forgé reste indiscernable d'un journal géométriquement certifié. La base `exact_certified_persistent_reduced_gamma_journal_prefix_cut_replay_v1` et la portée `bounded_n14_k10_single_order_strict_or_closed_hgp_reduced_cut_from_certified_6_14_journal_only` certifient donc seulement le rejeu structurel du préfixe. Le cas terminal reste vide à toute frontière; aucun statut public, raccord catalogue--Gamma, `full_pi0`, attache verticale, M.1, DAG global, forêt multi-ordre, CUDA, G4 ou résultat scalable n'en découle.

Le jalon 6.16 raccorde enfin les lots H0 exhaustifs de 6.12 aux slots d'égalité exhaustifs de l'histoire 6.14, sans transférer au catalogue l'autorité d'incidence de Gamma. `build_exact_critical_catalog_reduced_gamma_overlay` accepte directement le nuage, un ordre $2\leq k<n\leq14$, $k\leq10$, et un budget composite fiable. Il construit le catalogue avec $K_{\max}=k$ et l'histoire au même ordre, exige leurs vérificateurs frais, puis conserve les deux résultats certifiés avec une couche compacte d'indices. Une dégénérescence extra-shell pertinente bloque tout raccord; le terminal $k=n$ reste hors de cette identité, car son histoire réduite est vide alors que le catalogue peut porter une naissance de rang $n$.

Pour un événement accepté, écrivons $I$ pour les points intérieurs, $U$ pour son support positif, $S=I\cup U$ pour `closed_point_ids`, $s=\lvert S\rvert$ et $a$ pour le niveau carré. Si $c=\sum_{u\in U}\lambda_u u$, avec $\lambda_u>0$ et $\sum_{u\in U}\lambda_u=1$, alors toute boule de centre $z$ contenant $U$ vérifie l'identité suivante, tandis que la boule critique contient déjà $S$ :

$$\sum_{u\in U}\lambda_u\left\Vert u-z\right\Vert^2=a+\left\Vert c-z\right\Vert^2\geq a,\qquad \beta(S)=a.$$

La clé de raccord est donc exclusivement le couple formé du niveau exact et de `closed_point_ids`, jamais le support ou le shell seuls. Si $s=k$, le rôle de naissance référence l'unique slot de facette nouvellement active portant $S$; si $s=k+1$, le rôle de selle référence l'unique slot de coface égale portant $S$. Tous les slots sont indexés depuis l'histoire avant la lecture des rôles du catalogue. Chaque rôle se projette exactement une fois et chaque slot est soit projeté, soit conservé comme résidu sans provenance H0 acceptée; aucune incidence résiduelle n'est qualifiée arbitrairement de silencieuse ou non critique.

Sous l'absence certifiée d'extra-shell pertinent, les naissances du catalogue sont en bijection avec les facettes différées de Gamma. En effet, une coface de même niveau imposerait, par unicité de la miniball de la facette, un point fermé supplémentaire; réciproquement, une facette différée ne possède aucun tel point et son support minimal positif produit l'événement de rang $k$. Une selle vise toujours un groupe non différé, mais son rôle ne décide jamais si l'histoire classe ce groupe comme naissance réduite, continuation ou multifusion. Le groupe, son `kind`, ses racines, son nœud et son delta restent exclusivement ceux de 6.14; plusieurs selles simultanées peuvent viser des slots distincts d'un même groupe sans séquentialisation.

Le préflight de la couche de raccord précède toute géométrie subordonnée. Avec $F=\binom{n}{k}$, $C=\binom{n}{k+1}$ et $L=F+C$, l'histoire fournit exactement $L\leq6435$ slots, au plus $L$ groupes et exactement $kF+(k+1)C\leq48048$ lectures de `PointId`. Les rôles utiles proviennent de supports de tailles deux à quatre, donc leur capacité sûre est au plus 1456; le scan fermé conservateur est borné par 16016 références et les références de groupes par 1456. Un budget de raccord insuffisant ne lance ni 6.12 ni 6.14; un budget subordonné insuffisant conserve seulement le résultat d'échec correspondant, et aucun chemin ne publie un overlay partiel.

Le vérificateur 6.16 reconstruit le catalogue, l'histoire, les projections, tous les slots, tous les groupes, les résidus, les faits et les compteurs depuis les seules entrées externes. Les identités finales sont $P_b+R_f=F$ et $P_s+R_c=C$, où $P_b,P_s$ comptent les projections de naissance et de selle et $R_f,R_c$ les slots résiduels de facettes et cofaces. La base `exact_critical_closed_label_h0_references_reconciled_with_exhaustive_persistent_reduced_gamma_equality_slots_v1` et la portée `bounded_n14_k10_single_order_freshly_verified_critical_catalog_h0_provenance_to_exhaustive_persistent_reduced_gamma_history_groups_only` n'ajoutent ni identifiant durable ou public, ni attache verticale, ni preuve M.1, ni DAG global, ni forêt multi-ordre, ni CUDA, ni G4, ni scalabilité, ni `public_status`.

Le jalon 6.17 ferme ensuite le raccord exhaustif des bras de selles pour un ordre borné sans convertir ces témoins locaux en objets publics `Attachment`. `build_exact_critical_catalog_arm_gamma_overlay` reconstruit et vérifie fraîchement 6.12 avec $K_{\max}=k$, sélectionne toutes les références `saddle_order=k`, puis appelle une fois 6.7 pour chacune. La miniball, le support et la partition fermée reconstruits par chaque famille doivent être exactement ceux de l'événement catalogué. Une extra-shell pertinente bloque avant les familles; si une seule famille s'arrête sur budget ou dégénérescence, aucune composante-cible Gamma n'est publiée.

Lorsque toutes les familles sont complètes, une unique instance transitoire de 6.13 est construite et vérifiée pour chaque lot H0 contenant au moins une selle. Pour chaque bras $u$, le raccord cherche séparément sa facette initiale $F_u$ et sa facette terminale $T_u$ dans la coupe `strict_gamma` exhaustive de ce lot. Elles doivent appartenir à la même unique composante `full_pi0`, et cet indice doit figurer dans l'unique groupe non différé contenant `closed_point_ids` de la selle. La cible conservée est le témoin complet de cette composante; son éventuel statut `prior_nontrivial_reduced_root` ou `omitted_isolated_facet`, ainsi que le `kind` réduit du groupe, sont copiés dans des champs distincts et ne pilotent jamais le choix de cible.

Avec $E=\sum_{s=2}^{\min(4,k+1,n)}\binom{n}{s}$, $A=4E$ et $F=\binom{n}{k}$, le préflight propre précède toute géométrie et exige des capacités pour au plus $E\leq1456$ familles et, séparément, au plus autant de lots, puis au plus $A\leq5824$ bras et, séparément, au plus autant de composantes ciblées, $EF\leq4996992$ références de facettes, $k(EF+A)=kE(F+4)\leq35025536$ références ponctuelles incluant le représentant canonique de chaque témoin, et $AL\leq23855104$ segments engagés sous $L\leq4096$. Les résultats 6.13 restent transitoires : le payload compact conserve le catalogue, les familles et leurs chemins, les métadonnées de lots, les seules composantes effectivement ciblées et exactement une incidence par triple `(catalog_event_index, order, removed_shell_point_id)`.

Le vérificateur 6.17 reconstruit toutes ces couches depuis le nuage, l'ordre et le budget composite; aucun indice, groupe, terminal ou fait observé, aucune composante observée et aucune décision observée ne choisit une branche du rejeu. La base `exact_exhaustive_critical_catalog_index_one_arm_families_reconciled_with_strict_gamma_full_pi0_components_and_separate_reduced_annotations_v1` et la portée `bounded_n14_k10_single_order_fresh_catalog_all_index_one_saddle_arm_families_to_exhaustive_strict_gamma_full_pi0_components_with_reduced_annotations_only` restent mono-ordre, bornées et locales. Elles ne ferment ni H5 au-delà du domaine et des familles complètes, ni M.1, ni la transaction de forêt simultanée, ni les identifiants v2, ni la verticalité, ni les plateaux, ni CUDA, ni G4, ni la scalabilité, ni `public_status`.

Le jalon 6.18 compose ces deux raccords dans un journal mono-ordre compact et explicitement typé. Les types `ExactCriticalCatalogTypedGamma*` et `ExactCriticalCatalogTypedGammaJournal*` figent ce contrat; `build_exact_critical_catalog_typed_gamma_journal` accepte le nuage canonique, un ordre $2\leq k<n\leq14$, $k\leq10$, les budgets composites de 6.16 et 6.17 et neuf capacités propres. Avant toute géométrie, le constructeur exige l'égalité exacte des deux budgets 6.12 embarqués et celle du budget Gamma de l'histoire 6.14 avec le budget du lot 6.13 employé par 6.17. Il construit et vérifie fraîchement 6.16 et 6.17, exige leurs décisions complètes et l'égalité exacte de leurs catalogues reconstruits. Ces deux overlays, les deux catalogues, les familles 6.7 et leurs chemins sont des sources transitoires : le résultat ne retient qu'une seule histoire 6.14, celle certifiée par 6.16, puis les arènes compactes du nouveau raccord.

Les $L=F+C$ labels d'égalité de cette histoire sont conservés une fois comme `ExactCriticalCatalogTypedGammaLabelEntry` et partitionnés sans perte par quatre sémantiques disjointes : `catalog_birth`, `catalog_saddle`, `residual_newly_active_facet` et `residual_equal_level_coface`. Il n'existe pas d'arène parallèle d'événements H0. Une entrée cataloguée conserve les indices locaux de l'événement, du lot H0, du lot historique, du groupe historique et du slot; une entrée résiduelle conserve explicitement l'absence de provenance acceptée. Tous les groupes, y compris différés, entièrement redondants ou sans provenance H0, restent dans l'unique histoire 6.14. Une naissance H0 rejoint exactement un groupe `deferred_isolated_facet`, tandis qu'une selle H0 rejoint un groupe non différé; cela ne permet jamais de renommer une selle en naissance réduite ni de déduire le `kind` réduit depuis le rôle H0.

Chaque entrée `catalog_saddle` référence un `ExactCriticalCatalogTypedGammaSaddleRecord` distinct. La jointure primaire retrouve l'unique famille 6.17 par `(catalog_event_index, catalog_h0_batch_index)`; le niveau exact et `closed_point_ids` servent de défense sémantique indépendante. Si $g$ est le groupe 6.14 désigné par 6.16 et $j$ le groupe local 6.13 désigné par 6.17, le raccord exige `g.batch_group_index == j`, l'égalité des niveaux et l'égalité de `ExactReducedGammaBatchGroupKind`. Les `ExactCriticalCatalogTypedGammaTerminalClassRecord` de la famille sont ensuite conservés séparément des `ExactCriticalCatalogTypedGammaArmRecord` : chaque classe stocke son terminal canonique, les points de shell retirés qui la réalisent et sa cible stricte. Chaque bras apparaît exactement une fois sous la clé `(catalog_event_index, order, removed_shell_point_id)` et référence sa classe terminale ainsi que la même cible.

Les composantes-cibles déjà dédupliquées par 6.17 sous la clé locale `(batch_record_index, strict_component_index)` sont copiées et remappées bijectivement comme `ExactCriticalCatalogTypedGammaStrictTargetRecord`, puis chacune est rattachée au groupe historique commun à tous ses bras. Le représentant canonique reste une défense interne du témoin, jamais une nouvelle clé de déduplication 6.18. Chaque enregistrement conserve le témoin complet `ExactStrictGammaComponentWitness` de `full_pi0`; plusieurs classes terminales, bras ou selles simultanées peuvent légitimement le partager. `ExactReducedGammaStrictComponentKind` reste une annotation de composante distincte de `ExactReducedGammaBatchGroupKind`, lui-même distinct du rôle H0. En particulier, `prior_nontrivial_reduced_root` certifie seulement l'existence d'une racine réduite antérieure : 6.18 ne lui attribue aucun `root_node_id` sans un nouveau raccord par ensembles complets de facettes.

Pour $F=\binom{n}{k}$, $C=\binom{n}{k+1}$, $L=F+C$, $E=\sum_{s=2}^{\min(4,k+1,n)}\binom{n}{s}$ et $A=4E$, le préflight propre ferme exactement neuf capacités avant les deux sources : $L\leq6435$ entrées typées, $E\leq1456$ selles, puis séparément $A\leq5824$ classes terminales, $A\leq5824$ bras et $A\leq5824$ cibles strictes, $(k+1)A\leq64064$ références de `PointId` stockées par les classes terminales, $2A\leq11648$ indices dans les deux listes classes--bras des selles, $EF\leq4996992$ références de facettes des témoins cibles et $k(EF+A)\leq35025536$ références de `PointId` de ces témoins, représentants compris. Toute addition ou multiplication est vérifiée avant réservation. Une capacité insuffisante, un désaccord des budgets partagés, une source 6.16 ou 6.17 non complète, une divergence de catalogue ou une jointure incohérente publie zéro histoire, entrée typée, selle, classe, bras et cible.

`verify_exact_critical_catalog_typed_gamma_journal` reconstruit les deux sources transitoires et le journal attendu depuis le nuage, l'ordre et le budget fiable; aucun indice, type, groupe, terminal, témoin ou diagnostic observé ne choisit une branche. La base `exact_fresh_catalog_h0_provenance_and_strict_full_pi0_arm_targets_reconciled_through_one_typed_single_order_reduced_gamma_journal_v1` et la portée `bounded_n14_k10_single_order_exhaustive_gamma_groups_typed_catalog_h0_roles_and_strict_full_pi0_arm_targets_with_separate_hgp_reduced_effect_annotations_only` certifient uniquement cette factorisation locale. Elles ne créent aucun `Attachment` public, identifiant durable ou public, certificat M.1, cible verticale, transaction de forêt `full_pi0`, forêt multi-ordre, DAG global, pointer-jumping, quotient de plateau, résultat CUDA/G4, revendication de scalabilité ou `public_status`.

Le jalon 6.19 ajoute le raccord local explicite que 6.18 laissait ouvert. `build_exact_critical_catalog_typed_gamma_root_overlay` reçoit le nuage canonique, l'ordre, un budget fiable et un journal 6.18 externe. Il dérive et valide ses dix capacités propres ainsi que le budget imbriqué avant de recertifier géométriquement ce journal avec `verify_exact_critical_catalog_typed_gamma_journal`. Une source rejetée ou incomplète ne produit aucune liaison. La sortie complète ne recopie ni le journal ni l'histoire : ses indices de cible et ses `root_node_id` restent locaux à cette source externe fraîchement recertifiée.

Fixons une cible stricte $T$ rattachée au lot historique $i$, et notons $\mathcal{F}(T)$ sa famille complète de facettes. Le rejeu 6.14 maintient avant ce lot un snapshot immuable des racines réduites actives $\mathcal{R}_{i^-}$, dont les familles de facettes sont exactement les composantes non triviales de $\Gamma_k^{<a_i}$. Les composantes strictes partitionnent les facettes actives. Comme 6.18 conserve pour $T$ le témoin `full_pi0` complet du même lot, on obtient l'alternative exhaustive suivante :

$$\lvert\mathcal{F}(T)\rvert>1\Longrightarrow\exists!\,r\in\mathcal{R}_{i^-}:\mathcal{F}(r)=\mathcal{F}(T),\qquad \lvert\mathcal{F}(T)\rvert=1\Longrightarrow\nexists\,r\in\mathcal{R}_{i^-}:\mathcal{F}(T)\cap\mathcal{F}(r)\neq\varnothing.$$

Pour chaque lot portant une cible, le raccord indexe temporairement toutes les facettes de toutes les racines du snapshot. Un candidat non trivial n'est accepté qu'après égalité de sa famille complète avec celle de la cible et appartenance de son identifiant aux `prior_root_node_ids` du groupe historique visé. L'absence d'un singleton est certifiée contre cet index exhaustif, puis enregistrée séparément comme `omitted_isolated_singleton`. Le champ `reduced_component_kind` de 6.18 n'est comparé qu'après cette décision géométrique; il ne choisit ni la branche ni la racine. Toutes les cibles du lot sont résolues avant de préparer les mutations, tous les groupes sont ensuite reconstruits sur le même snapshot, et les effacements et insertions ne sont appliqués qu'après la résolution complète du lot. Une racine créée au niveau critique ne peut donc jamais devenir artificiellement la cible pré-lot d'une selle simultanée.

Avec $F=\binom{n}{k}$, $C=\binom{n}{k+1}$, $L=F+C$, $E=\sum_{s=2}^{\min(4,k+1,n)}\binom{n}{s}$, $A=4E$ et $R=\left\lfloor\frac{F}{k+1}\right\rfloor$, les dix capacités propres sont, dans l'ordre de l'API, $A\leq5824$, $2R\leq858$, $2F\leq6864$, $2kF\leq48048$, $LF\leq22084920$, $kLF\leq154594440$, $EF\leq4996992$, $kEF\leq34978944$, $EF\leq4996992$ et $kEF\leq34978944$. Elles bornent respectivement les liaisons, les états de racines actifs ou préparés, leurs références de facettes et les identifiants contenus dans ces facettes, puis des unités logiques de payload pour le rejeu, la comparaison des familles cibles et l'indexation des snapshots. Une unité logique compte une référence sémantique une fois; elle ne compte pas chaque instruction, recherche arborescente, étape de tri ou rescan défensif du conteneur CPU. Chaque racine non triviale contient au moins les $k+1$ facettes d'une coface active, d'où $R\leq\left\lfloor\frac{F}{k+1}\right\rfloor$. Les lots portant une cible sont au plus $E$ et leurs composantes cibles sont disjointes dans chaque snapshot, d'où $EF$ plutôt que $AF$ pour les quatre scans de familles. L'index temporaire possède sa capacité séparée et est détruit avant la préparation; les mutations sont réservées sous $R$, leurs facettes sous $F$, et l'état final est comparé en flux sans copie complète.

La base `exact_fresh_typed_full_pi0_target_families_reconciled_with_frozen_pre_batch_local_reduced_gamma_roots_v1` et la portée `bounded_n14_k10_single_order_full_pi0_target_families_to_frozen_pre_batch_local_hgp_reduced_roots_with_explicit_isolated_singletons_only` certifient uniquement ce raccord pré-lot mono-ordre borné. Elles ne rendent pas les identifiants locaux durables ou publics, ne publient aucun `Attachment`, ne ferment ni M.1, ni la verticalité, ni une transaction de forêt `full_pi0`, ni le DAG global, le pointer-jumping, les plateaux ou la forêt multi-ordre, et ne produisent aucun résultat CUDA/G4, aucune revendication de scalabilité ni aucun `public_status`.

Le jalon 6.20 compose enfin, sans nouvelle géométrie, chaque bras typé du journal externe 6.18 avec la liaison de sa cible dans l'overlay externe 6.19. `build_exact_critical_catalog_typed_gamma_arm_root_composition` reçoit ces deux sources, le nuage, l'ordre et un budget qui embarque exactement celui de 6.19. Toute l'arborescence des capacités imbriquées est validée avant le préflight propre et avant le rejeu frais de l'overlay, lequel recertifie transitivement le journal. Une couture discordante, une source rejetée ou une source incomplète ne publie aucun candidat.

Notons $\tau$ l'application totale bras--cible certifiée par 6.18 et $\rho$ l'application totale cible--racine locale ou singleton omis certifiée par 6.19. La nouvelle couche matérialise seulement leur composition :

$$\tau:\mathcal{A}\to\mathcal{T},\qquad \rho:\mathcal{T}\to\mathcal{R}_{\mathrm{local}}\sqcup\lbrace \bot_{\mathrm{singleton}}\rbrace,\qquad \chi=\rho\circ\tau.$$

Il existe exactement un candidat dense par bras, sous la clé événement-locale `(catalog_event_index, order, removed_shell_point_id)`. Plusieurs bras peuvent avoir la même cible, la même liaison et la même racine; ils restent néanmoins des candidats distincts. Le témoin `full_pi0` demeure dans le journal externe, tandis que la disposition et l'éventuel `root_node_id` local `hgp_reduced` sont copiés de la liaison externe sans reclassification. Avec $E=\sum_{s=2}^{\min(4,k+1,n)}\binom{n}{s}$ et $A=4E$, une seule capacité propre $A\leq5824$ suffit. Les candidats et compteurs sont préparés hors résultat puis committés ensemble.

La base `exact_fresh_typed_critical_arm_target_indices_composed_with_recertified_target_root_bindings_v1` et la portée `bounded_n14_k10_single_order_event_local_typed_critical_arms_to_strict_full_pi0_targets_and_frozen_pre_batch_local_hgp_reduced_root_or_explicit_omitted_singleton_candidates_only` certifient uniquement cette composition relationnelle interne. Aucun chemin de descente n'est recopié ou rendu rejouable; aucun objet `Attachment`, identifiant durable ou public, certificat M.1, morphisme vertical, transaction de forêt `full_pi0`, DAG global, pointer-jumping, plateau, forêt multi-ordre ou `public_status` n'en découle.

Le contrat 6.21 introduit `ExactCriticalCatalogTypedGammaArmRootPathOverlay*` sur `reference_cpu` dans le même domaine borné. `build_exact_critical_catalog_typed_gamma_arm_root_path_overlay` reçoit le journal 6.18, l'overlay 6.19 et la composition 6.20 comme sources externes non possédées. Son budget embarque exactement celui de 6.20; tous les plafonds imbriqués et les trois coutures sont contrôlés avant les cinq capacités propres et avant toute géométrie. Une capacité insuffisante, une composition rejetée ou une composition certifiée mais incomplète produit un diagnostic sans chemin.

Après recertification complète de 6.20, le constructeur reconstruit une fois le catalogue 6.12 depuis le budget fiable imbriqué, puis une famille 6.7 par selle depuis le shell critique frais. Chaque candidat dense doit retrouver exactement un bras par son point retiré, la même classe terminale 6.18, la facette initiale redérivée de `closed_point_ids`, le même terminal canonique et la double appartenance de ces facettes à la cible externe `full_pi0`. Les indices de cible et de liaison, la disposition et l'éventuelle racine locale `hgp_reduced` sont recopiés sans reclassification et sans déduplication des chemins partageant une cible ou une racine.

Le record compact conserve la clé événement-locale, le centre et le niveau critiques, le témoin analytique du germe, la distance cible et le coefficient sortant du point retiré, la borne locale, les contraintes extérieures négatives, les nœuds exacts et les témoins engagés de la chaîne 6.5. Il ne recopie ni catalogue, ni partition globale, ni miniball exhaustive, ni `stopping_probe`. Avec $E=\sum_{s=2}^{\min(4,k+1,n)}\binom{n}{s}$, $A=4E$ et $B\leq4096$, les cinq capacités sont $A\leq5824$, $AB\leq23855104$, $A(B+1)\leq23860928$, $kA(B+1)\leq238609280$ et $A(n-k-1)\leq64064$ pour les chemins, segments de chaîne, segments composites, références ponctuelles des nœuds et contraintes extérieures.

La base `exact_fresh_event_local_typed_critical_arm_strict_descent_paths_replayed_and_linked_to_full_pi0_targets_with_separate_local_reduced_dispositions_v1` et la portée `bounded_n14_k10_single_order_event_local_typed_critical_arms_with_replayable_strict_descent_paths_linked_to_external_full_pi0_targets_and_separate_frozen_pre_batch_local_hgp_reduced_root_or_omitted_singleton_dispositions_only` certifient uniquement des chemins internes rejouables. La racine réduite locale n'est pas une extrémité géométrique. Aucun `Attachment`, identifiant durable ou public, fermeture de H5, O3 ou M.1, morphisme vertical, transaction de forêt `full_pi0`, DAG global, pointer-jumping, plateau, forêt multi-ordre ou `public_status` n'en découle.

Le contrat 6.22 ajoute `morsehgp3d::contract::CanonicalId` et `ExactCriticalCatalogTypedGammaDurableArmKeyCatalog*` sur `reference_cpu`. Le budget embarque exactement 6.21, valide récursivement tous ses plafonds, contrôle les quatre coutures externes puis les quatre bornes propres avant toute recertification. Une source 6.21 rejetée ou incomplète ne publie aucune clé. La branche complète recertifie 6.21, reconstruit une seule fois le catalogue 6.12 et conserve pour chaque selle la projection scientifique v2 complète, sans `schema_version` imbriqué : intérieur, shell, support minimal, centre homogène exact et niveau carré exact. `event_id` est le SHA-256 de domaine `MorseHGP3D/v2/CriticalEvent/` concaténé au JSON canonique de cette projection. Deux digests égaux sont toujours comparés par leur projection complète; une collision ou une duplication échoue fermée.

Sur la branche générique complète, soit $\mathcal{E}_k$ l'ensemble des selles d'ordre $k$. Pour $e\in\mathcal{E}_k$, notons $d_e$ son `event_id` et $U_e$ son shell complet. Les clés de bras attendues sont exactement $\mathcal{K}_k=\lbrace(d_e,k,u):e\in\mathcal{E}_k,\ u\in U_e\rbrace$. Le tri lexicographique de ces tuples, leur unicité et l'égalité événement par événement entre les points retirés observés et le shell frais établissent une bijection avec les chemins 6.21. Les indices de catalogue et de chemins restent de simples jointures locales et n'entrent dans aucune identité. Avec $E=\sum_{s=2}^{\min(4,k+1,n)}\binom{n}{s}$, les bornes conservatives sont $E\leq1456$ événements, $A=4E\leq5824$ tuples et références événement--bras, puis $(k+5)E\leq21840$ références de `PointId`, car intérieur et shell comptent $k+1$ points et le support minimal au plus quatre.

Les `event_id` sont compatibles avec le contrat public v2 et les tuples sont exactement les projections d'identité de futures attaches. Le jalon ne calcule pourtant aucun `attachment_id`, ne sérialise aucun `Attachment`, ne fabrique aucun `target_node_id` public et ne forme aucun `EqualLevelBatch`. L'invariance validée porte sur la permutation d'arrivée avant canonicalisation, pas sur un relabelling arbitraire des `PointId`. L'indépendance de tous les choix admissibles, `attachments_complete_by_order`, H5, O3, M.1, l'attache verticale, la forêt globale et tout `public_status` restent ouverts. La base est `v2_domain_separated_sha256_critical_event_keys_with_full_projection_collision_checks_and_exhaustive_single_order_arm_identity_tuple_catalog_v1` et la portée `bounded_n14_k10_single_order_v2_critical_event_ids_and_canonical_arm_identity_tuples_from_recertified_internal_replayable_paths_only`.

Le contrat expérimental 6.23 introduit `ExactMorseGammaPartitionSweep*` comme falsificateur interne mono-ordre, sans prolonger la chaîne d'identifiants 6.22. Sur $3\leq n\leq14$, $2\leq k<n$ et $k\leq10$, il reconstruit d'abord 6.12 et 6.7 sans consulter Gamma. Chaque événement `birth_order=k` crée une naissance locale; chaque terminal actif d'un événement `saddle_order=k` doit retrouver l'unique naissance de même facette, centre et niveau, strictement antérieure à la selle. Toute absence, ambiguïté ou famille incomplète arrête cette voie sans généalogie partielle.

À niveau égal, les racines strictes sont figées et chaque selle fournit l'hyperarête de ses racines terminales. Les composantes connexes du quotient sont indépendantes de toute permutation des hyperarêtes, car elles sont la clôture transitive de la même relation symétrique; le rejeu en ordre inverse est un témoin logiciel de non-séquentialisation, pas le fondement de ce fait. Une composante à une racine est une continuation et une composante à au moins deux racines crée une seule multifusion dont les enfants précèdent le parent. Toutes les mutations du lot suivent la résolution complète de ces composantes.

Gamma ne démarre qu'après la fermeture de cette généalogie candidate. Si $F=\binom{n}{k}$, $C=\binom{n}{k+1}$ et $L$ est le nombre de niveaux d'activation distincts, alors $L\leq F+C$. Une transition exhaustive 6.10 est reconstruite à chacun de ces $L$ niveaux. Aux coupes stricte et fermée, la projection des facettes de naissance actives doit établir une bijection entre racines Morse et composantes Gamma; une composante sans naissance, une racine divisée ou une fusion Gamma non expliquée produit le premier témoin de contradiction et aucun payload de généalogie. Ce témoin conserve les labels canoniques de la facette de naissance et de la facette représentante Gamma lorsqu'ils existent; ses indices locaux restent de simples aides de rejeu. La base `exact_catalog_minima_and_strict_arm_terminal_hyperkruskal_partition_sweep_with_posterior_exhaustive_gamma_oracle_v1` et la portée `bounded_n14_k10_single_order_morse_minimum_saddle_partition_sweep_compared_to_exhaustive_gamma_at_every_activation_level_only` ne prouvent ni O1, O3, O4, H5 ou M.1, ne publient aucune forêt `full_pi0` et ne changent aucun `public_status`.

Les plateaux et shells dégénérés exigent un graphe de successeurs multivalué traité par composantes fortement connexes. Tant que ce quotient n'est pas prouvé et implémenté, ils déclenchent `unsupported_degeneracy` en mode exact.

### 9.4 Premier niveau d'incidence

Pour une facette $F$, posons $\lambda(F)=\min_{x\in X\setminus F}\beta(F\cup\lbrace x\rbrace)$. Une facette réutilisée par un futur événement Gabriel doit être rattachée à sa racine dès ce premier niveau, même si le delta d'observations est vide. Une coface minimisante et une descente intrus–support peuvent localiser cette racine; un oracle spatial scalable devra en plus certifier qu'aucun minimiseur de niveau inférieur n'a été omis.

## 10. Événements simultanés

Des centres distincts peuvent avoir exactement le même niveau. Une exécution séquentielle créerait des bifurcations binaires artificielles et pourrait changer les morphismes verticaux.

Pour chaque ordre $k$ et niveau exact $a$ :

1. figer les composantes de niveau strictement inférieur;
2. distinguer les racines des bras stricts et toutes les facettes à activer dans le lot;
3. remplacer chaque hyperarête par une étoile sans effectuer immédiatement les unions;
4. calculer les composantes de l'hypergraphe du lot sur les racines figées;
5. créer une naissance si $q=0$, aucun nœud si $q=1$, ou une multifusion si $q\geq2$;
6. effectuer ensuite les unions;
7. enregistrer les `coverage_delta` et, pour les rangs $2\leq s\leq K_{\mathrm{eff}}$, poser les ancres verticales après l'état fermé du lot.

Le regroupement des niveaux utilise d'abord des intervalles certifiés, puis un comparateur exact dans chaque groupe ambigu. Une tolérance flottante n'est pas une relation d'égalité.

Pour un centre isolé d'indice un, le nombre $q$ de racines globales distinctes touchées par ses $\lvert U\rvert$ bras vérifie $0\leq q\leq\lvert U\rvert$; s'il fusionne réellement, il tue $q-1\leq\lvert U\rvert-1$ classes. Un lot contenant plusieurs centres de même niveau contracte l'hypergraphe entier avant de compter les multifusions. Il ne décompose jamais ces événements en selles binaires.

## 11. Morphismes verticaux

L'inclusion $L_{k+1}(a)\subseteq L_k(a)$ envoie chaque composante de l'ordre $k+1$ vers l'unique composante de l'ordre $k$ qui la contient. Pour une composante source, choisir un label $Q$ de cardinal $k+1$. Vu à l'ordre $k$, $Q$ est une coface qui relie toutes ses facettes, donc sa cible est non triviale.

Dans la base exacte v2, le backend de référence détermine la cible directement dans les composantes exhaustives de $\Gamma_k(a)$. L'inclusion des labels de la composante source dans cette cible et son unicité sont vérifiées sur l'état fermé exact.

L'oracle expérimental `locate_reduced_root(k,Q,a)` peut transformer $Q$ par remplacements intrus–support et certifier chaque baisse de niveau. Il reste un candidat de localisation pour une future réduction complétée en incidences. Le contrat v2 ne permet pas de conclure qu'une racine du DSU Gabriel brut est la cible HGP exacte; une cible verticale de cette voie est partielle sauf si elle est vérifiée indépendamment contre Gamma.

Pour $2\leq s\leq K_{\mathrm{eff}}$, une sphère de rang $s$ fournit dans `full_pi0` une ancre naturelle : sa naissance dans $T_s$ est envoyée vers la composante post-lot contenant son centre dans $T_{s-1}$. Le rang $s=1$ n'a pas de tranche inférieure, et le rang $s=K_{\mathrm{eff}}+1$ éventuellement catalogué ne possède pas de tranche source dans la tour. Dans `hgp_reduced`, pour $s\geq2$, cette naissance est une facette isolée omise et ne fournit donc aucun nœud source à ancrer. Les morphismes réduits utilisent `locate_reduced_root`; l'ancre Morse ne sert de contrôle que lorsqu'un représentant source réduit existe, éventuellement après une activation non triviale.

Les autres images sont obtenues par propagation le long des forêts. Le certificat vérifie, pour chaque événement horizontal et chaque ordre adjacent, que les deux chemins ordre–échelle donnent la même composante. Ces carrés de naturalité sont des invariants de correction, pas des métadonnées facultatives.

## 12. Domaine exact et dégénérescences

La première cible exacte suppose :

- points distincts;
- tout shell critique utile de cardinal au plus quatre;
- support minimal affinement indépendant;
- centre dans l'intérieur relatif du support avec coordonnées barycentriques non nulles;
- aucun point extérieur sur la frontière de la miniball d'un simplexe traité;
- prédicats et égalités de niveaux décidés exactement.

La position générale requise n'est certifiée que sur le périmètre utile. Pour $A\subseteq X$, notons $B_A$ sa miniball fermée et disons que $A$ a un support propre si son support minimal $U_A$ est unique, affinement indépendant et contient le centre dans son intérieur relatif. On pose

$$\mathrm{RelevantGP}(X,K_{\mathrm{eff}})\Longleftrightarrow\forall A\subseteq X,\ 2\leq\lvert A\rvert\leq s_{\max},\ \bigl[\mathrm{Proper}(A)\land B_A^{\circ}\cap(X\setminus A)=\varnothing\bigr]\Longrightarrow\partial B_A\cap(X\setminus A)=\varnothing.$$

Autrement dit, seuls les ensembles de taille utile qui sont Gabriel dans leur intérieur sont tenus de ne pas posséder de point extérieur supplémentaire sur leur frontière. Le champ `relevant_gp_complete` ne vaut vrai que si cette propriété est établie. Une simple absence de dégénérescence parmi les événements acceptés ne suffit pas : pour tout support propre rencontré avec au plus $m_{\star}=s_{\max}-2$ points strictement intérieurs lorsque $s_{\max}\geq2$, l'oracle doit terminer le shell complet avant de filtrer par rang. Si un point extérieur à un candidat Gabriel utile se trouve exactement sur sa miniball, l'exécution certifiée retourne `unsupported_degeneracy`, même lorsque le rang fermé total dépasse $s_{\max}$.

Cette propriété `RelevantGP` ne certifie pas les générateurs saturés de rang arbitraire de la section 6.1. Leur théorème combinatoire tolère un grand shell fermé, mais leur future implémentation devra agréger les supports multiples au lieu d'interpréter `relevant_gp_complete=true` comme une généricité globale.

Cette profondeur est suffisante pour détecter toute violation de la propriété ainsi restreinte. En effet, si $A$ satisfait l'antécédent et possède un point extérieur sur $\partial B_A$, l'absence d'intrus strict donne $X\cap B_A^{\circ}=A\cap B_A^{\circ}$. Comme le support propre contient au moins deux points, $\lvert X\cap B_A^{\circ}\rvert\leq\lvert A\rvert-2\leq m_{\star}$. Le centre appartient donc à une strate issue d'un parent fermé, où le shell global révèle le point extérieur. La promotion de ce raisonnement en certificat logiciel exige que toutes les strates naturelles et tous les co-minimiseurs concernés aient été réconciliés.

Les égalités de niveaux entre événements distincts sont prises en charge par lots et ne violent pas ces hypothèses. Les cas suivants demandent une extension :

- doublons : sites à multiplicité et rangs pondérés;
- plus de quatre points cosphériques : arrangement local des directions sur $S^2$ et preuve d'isotopie;
- supports affinement dépendants : support minimal canonique et cellules de dimension réduite;
- plateaux de descente : contraction exacte du graphe multivalué;
- coordonnées provoquant un dépassement des filtres : repli multiprécision, jamais troncature.

Le mode exact échoue explicitement lorsqu'un cas non couvert est détecté, lorsque `relevant_gp_complete` est faux ou inconnu, ou lorsqu'un shell pertinent n'est pas complet. Une perturbation symbolique peut servir de diagnostic, mais son résultat porte le statut `perturbed`, pas `exact` pour l'entrée originale.

## 13. Énumération sans mosaïque matérialisée

Pour $I\subseteq X$, $\lvert I\rvert=m$, définissons la cellule top-$m$

$$C_m(I)=\left\lbrace y:\max_{i\in I}\left\Vert y-i\right\Vert^2\leq\min_{x\notin I}\left\Vert y-x\right\Vert^2\right\rbrace.$$

Pour $u\notin I$, définissons son morceau de Voronoï restreint

$$P(I,u)=C_m(I)\cap\bigcap_{v\notin I}\left\lbrace y:\left\Vert y-u\right\Vert^2\leq\left\Vert y-v\right\Vert^2\right\rbrace.$$

Pour tout $Q$ de cardinal $m+1$, l'identité exacte

$$C_{m+1}(Q)=\bigcup_{u\in Q}P(Q\setminus\lbrace u\rbrace,u)$$

adapte l'algorithme incrémental des ordres à une primitive de diagramme de puissance GPU. Les intersections de deux, trois ou quatre cellules extérieures donnent les supports critiques de même cardinal; le rang est ensuite compté globalement.

Les morceaux issus de parents différents servent à découvrir les labels non vides, mais leur union n'est pas la représentation certifiée de la cellule enfant. Après tri et déduplication d'un label $Q$, $p=\lvert Q\rvert$, la v2 reconstruira obligatoirement le polytope canonique

$$C_p(Q)\cap\Omega=\Omega\cap\bigcap_{q\in Q,\ v\notin Q}\left\lbrace y:\left\Vert y-q\right\Vert^2\leq\left\Vert y-v\right\Vert^2\right\rbrace.$$

Le clipping provisoire est initialisé uniquement avec des contraintes sûres croisées $Q\times(X\setminus Q)$ héritées des fragments; les comparaisons internes à $Q$ et les faces de raccord entre fragments sont exclues. À chaque sommet provisoire $z$, calculer exactement les ensembles complets

$$Q_{\max}(z)=\arg\max_{q\in Q}\left\Vert z-q\right\Vert^2,\qquad V_{\min}(z)=\arg\min_{v\notin Q}\left\Vert z-v\right\Vert^2.$$

Notons $d_{\max}(z)$ et $d_{\min}(z)$ leurs valeurs communes. Si $d_{\max}(z)<d_{\min}(z)$, toutes les contraintes croisées sont strictes au sommet. Si $d_{\max}(z)=d_{\min}(z)$, toutes les paires de $Q_{\max}(z)\times V_{\min}(z)$ sont insérées ou enregistrées comme incidences actives, même sans violation. Si $d_{\max}(z)>d_{\min}(z)$, toutes ces paires violatrices sont ajoutées, puis la cellule est reclippée. Chaque différence étant affine, la validation de tous les sommets certifie toutes les contraintes sur le polytope; la fermeture exige en plus que toutes les paires actives à égalité aient été réconciliées. La déduplication du seul label $Q$ ne suffit pas; `canonical_children_complete` exige cette reconstruction fermée pour chaque enfant.

Un complexe de fragments qui conserverait des coutures internes est une optimisation future. Une vraie strate naturelle pourrait croiser une couture; prouver sa propagation sans perte ni double compte reste une `proof_obligation`. Ce format n'est pas autorisé dans le chemin exact de la v2.

Le calcul est restreint à une boîte tridimensionnelle exacte $\Omega$ telle que

$$\mathrm{conv}(X)\subset\mathrm{int}(\Omega).$$

Cette inclusion doit être stricte, y compris pour des données colinéaires ou coplanaires. La boîte est obtenue par un padding dyadique positif enregistré; un dépassement lors de sa construction provoque un repli exact ou un échec, jamais un clipping silencieux. Tout centre critique appartient à $\mathrm{conv}(U)\subseteq\mathrm{conv}(X)$ et reste donc strictement intérieur à $\Omega$. Les faces de boîte sont marquées artificielles et ne peuvent pas servir de support à un événement; aucun événement légitime n'est rejeté du seul fait d'une incidence numérique avec la boîte.

La complétude ne repose pas sur les seuls voisins proposés. Chaque diagramme restreint est initialisé par au moins un site de $X\setminus I$; si toutes les heuristiques sont muettes, le plus petit identifiant extérieur sert de germe canonique. Dans une cellule convexe provisoire, la différence de deux distances carrées est affine; si une contrainte manque et viole l'intérieur, elle viole au moins un sommet. Un oracle exact du plus proche voisin extérieur à $I$, exécuté à tous les sommets et sur les co-minimiseurs, révèle donc les colonnes manquantes. La fermeture termine lorsque chaque sommet est certifié et que la file globale est vide.

La complétude globale suit alors par induction. La base est $C_0(\varnothing)\cap\Omega=\Omega$. Si tous les parents canoniques de profondeur $m$ et leurs diagrammes restreints sont fermés, l'identité de raffinement émet exactement tous les labels non vides de profondeur $m+1$; la reconstruction canonique les transforme en parents exacts. Cette induction est le corollaire D.4 du [catalogue critique](math/CATALOGUE_CRITIQUE_3D.md) et s'arrête aux parents $C_{m_{\star}}$; à la dernière profondeur, seuls les morceaux nécessaires aux strates sont fermés.

Cette procédure garde le pire cas combinatoire de la géométrie 3D. Elle remplace une construction monolithique par des cellules indépendantes, fermables, diffusables et adaptées au GPU; elle ne transforme pas une sortie quadratique en sortie linéaire.

Pour $n\geq2$, la profondeur géométrique maximale utile est

$$m_{\star}=s_{\max}-2\leq K_{\mathrm{eff}}-1.$$

En effet, un événement utile de rayon strictement positif possède $\lvert U\rvert\geq2$ et $s\leq s_{\max}$. L'énumérateur ferme les parents $C_0$ à $C_{m_{\star}}$. À la profondeur finale, il construit et ferme encore les morceaux $P(I,u)$ nécessaires aux intersections de sites extérieurs, puis extrait leurs strates; il ne les réunit pas en cellules $C_{m_{\star}+1}$ et ne les propage pas. Si $n\geq K_{\mathrm{eff}}+1$, alors $m_{\star}=K_{\mathrm{eff}}-1$; en particulier, pour $n>10$ et $K_{\mathrm{eff}}=10$, $m_{\star}=9$ et aucun $C_{10}$ ou $C_{11}$ n'est construit. Au bord $n=K_{\mathrm{eff}}\geq2$, on a au contraire $m_{\star}=n-2$, car le rang $n+1$ est impossible. Si $s_{\max}=1$, le rang un de rayon nul est injecté directement et aucune cascade n'est lancée.

### 13.1 Contrat borné d'audit d'une primitive pondérée

La Phase 7 compare les primitives de proposition avec la convention $\delta_i(y)=\left\Vert y-p_i\right\Vert^2-w_i$. Pour deux sites pondérés, la contrainte de la cellule de $i$ face à $j$ est $\Phi_{ij}(y)=\delta_i(y)-\delta_j(y)=2(p_j-p_i)\mathbin{\cdot}y+\left\Vert p_i\right\Vert^2-\left\Vert p_j\right\Vert^2-w_i+w_j\leq0$. Cette orientation impose qu'augmenter $w_i$ agrandit la cellule de $i$ et qu'une translation commune des poids ne change pas le diagramme.

L'oracle d'audit initial reçoit une boîte dyadique explicite $\Omega$ et se limite à $n\leq8$. Une cellule possède alors au plus $P=6+(n-1)\leq13$ plans. L'énumération exhaustive des triplets borne le travail à $\binom{13}{3}=286$ intersections candidates, 286 sommets rationnels après filtrage au maximum et $13\times286=3718$ tests d'incidence. Ces plafonds dimensionnent un oracle local de falsification; ils ne modifient ni le backend exact v2 ni le domaine de publication.

L'implémentation `build_exact_bounded_power_cell_reference` trie les concurrents par identifiant, classifie exactement les sites confondus, énumère les intersections de rang trois avec les primitives rationnelles existantes, filtre toutes les inégalités puis reconstruit la liste complète des plans actifs de chaque sommet. Les décisions `complete_nonempty` et `complete_empty` décrivent uniquement le polyèdre local fermé, y compris ses cas de dimension inférieure; elles ne certifient ni position générale, ni complétude d'une proposition externe, ni diagramme global.

Une primitive GPU externe reste une source de propositions flottantes tant que tous ses plans actifs, sommets, incidences, overflows et abandons de parcours ne sont pas exposés puis recertifiés. Une adjacency plausible ou un statut local de succès ne ferme pas une cellule et ne peut produire aucun `public_status=exact`.

Pour le candidat Paragram épinglé, un débordement de pile de parcours est une erreur de cellule distincte et ne constitue jamais un pruning. Les codes historiques 0 à 5 restent stables et le code 6 désigne `traversal_stack_overflow`. Toute branche dont la borne ne prouve pas une exclusion stricte, y compris une marge exactement nulle, doit rester dans le parcours ou produire ce statut si la pile est pleine. Toute cellule de statut non nul a une ligne CSR vide et aucune arête publiée ne peut la prendre pour extrémité. Ce contrat ne couvre pas encore les fautes CUDA asynchrones, le choix du stream, la boîte de l'appelant ou l'export géométrique; il ne renforce donc pas le sens de `success` au-delà d'une proposition locale.

## 14. Axes d'exécution

Le backend, le profil, le mode et le statut sont orthogonaux :

| axe | valeurs initiales |
|---|---|
| `backend` | `reference_cpu`, `cuda_g4` |
| `profile` | `hgp_reduced`, `full_pi0` |
| `mode` | `certified`, `budgeted` |
| `public_status` | `exact`, `conditional`, `budget_exhausted`, `unsupported_degeneracy`, `numeric_failure` |

```text
certified
    require_exact = true
    aucun budget ne peut convertir silencieusement la sortie
    succès seulement si catalogue, cellules enfants canoniques, prédicats, lots, RelevantGP et profil sont complets

budgeted
    time_budget, memory_budget ou output_budget explicite
    chaque événement retourné est vérifié
    le catalogue peut être incomplet
    forest_semantics = partial_refinement
    statut public = conditional ou budget_exhausted
```

`reference_cpu` est un backend exhaustif pour petits $n$, utilisé pour les preuves par test et les différentiels; ce n'est pas un troisième mode.

Dans le mode budgété, soit $E'$ le sous-flot certifié effectivement traité et $E$ le flot exact complet. À tout niveau, la connectivité produite par $E'$ est une sous-relation de celle produite par $E$ : sur les facettes émises, chaque composante partielle est contenue dans une composante exacte. La partition partielle est donc un raffinement de la partition exacte; elle peut manquer des connexions mais n'en invente aucune. Lorsqu'un niveau de connexion partiel existe entre deux facettes, il est une borne supérieure du vrai niveau de connexion.

Cette garantie unilatérale ne certifie pas les nœuds de la forêt comme événements HGP : une « fusion » du sous-flot peut être redondante dans le flot complet à cause d'une hyperarête antérieure manquante, et une naissance réduite peut être retardée. La sortie porte donc le nom `partial_forest`; ses applications verticales sont absentes ou explicitement partielles tant que leurs cibles ne sont pas certifiées. Seuls les événements géométriques, les chemins émis et la relation positive de connectivité portent un certificat individuel.

Le certificat de run expose au minimum la fermeture de chaque ordre, les dégénérescences, les fallbacks numériques, les budgets, les compteurs de sortie et le profil réellement calculé.

## 15. Limites de complexité

Une liste fixe de voisins par observation n'est pas complète : une paire de Gabriel peut avoir une boule diamétrale vide tout en étant absente d'une liste $L$-NN arbitrairement longue, en plaçant de nombreux points juste à l'extérieur de cette boule près d'une extrémité.

La triangulation de Delaunay ordinaire en 3D peut déjà avoir une taille quadratique. Il est donc impossible de garantir simultanément, pour tout nuage : sortie exacte complète, temps quasi linéaire, mémoire bornée et latence inférieure à une seconde. Les performances visées concernent le régime volumique sparse et doivent publier les tailles intermédiaires.

Les résultats moyens pour des processus de Poisson à ordre fixé rendent crédible un catalogue de taille linéaire en moyenne dans ce régime. Ils ne constituent ni une borne déterministe, ni une promesse sur les nuages surfaciques, LiDAR ou adversariaux.

## 16. Place de la DTM et de l'entropie

La DTM empirique quadratique est la moyenne des $k$ premières distances carrées. Elle lisse le maximum, mais ses sous-niveaux ne sont pas ceux de $D_k$. Une continuation depuis la DTM peut créer, supprimer ou permuter des événements avant la limite dure.

Une sélection entropique sur le polytope $0\leq p_i\leq1$, $\sum_i p_i=k$ rend les poids différentiables, mais les rend généralement denses. Dans la limite froide, elle retrouve les mêmes régions top-$k$; elle ne réduit donc pas leur atlas. Pour $k=1$, elle ne fournit pas une structure hiérarchique remplaçant l'arbre minimum couvrant.

Usages autorisés :

- classer les cellules à fermer;
- choisir les premières colonnes d'un diagramme restreint;
- préchauffer le passage de $m$ à $m+1$;
- proposer des événements en mode budgété;
- fournir une baseline de stabilité.

Usages interdits pour le statut `exact` : exclure une région sans borne globale, conclure à l'absence d'un événement, décider une attache ou remplacer le traitement d'un lot égal.

## 17. Objet de sortie

```text
MorseHGP3DResult
    profile                       # hgp_reduced | full_pi0
    forest_semantics              # exact | partial_refinement
    forests[1..Keff]
    vertical_maps[1..Keff-1]
    equal_level_batches
    critical_catalog
    gamma_cofaces                  # toutes les cofaces exigées par gamma_complete_by_order
    gabriel_hyperedges             # sous-flot positif, insuffisant seul aux ordres supérieurs
    coverage_log                  # obligatoire pour rejouer les coupes
    optional_materialized_point_sets
    run_certificate

RunCertificate
    public_status                 # exact | conditional | ...
    input_semantics
    general_position_status
    relevant_gp_complete
    canonical_children_complete
    active_cross_incidences_complete
    catalog_complete_by_rank[1..s_max]
    attachments_complete_by_order[1..Keff]
    gamma_complete_by_order[1..Keff]
    batches_complete_by_order[1..Keff]
    vertical_maps_complete
    partial_guarantees
    exact_predicate_counts
    fallback_counts
    work_and_memory_counters
    budgets_and_stop_reason
```

Le `coverage_log` contient les `coverage_delta` de chaque lot et fait partie de la sortie normative. Une matérialisation immédiate des ensembles de points peut être omise, mais une coupe exacte doit pouvoir retourner les composantes et, pour $k\geq2$, leurs ensembles d'observations recouvrants. Toute option de condensation ou de partition est une opération aval, distincte de la sortie HGP normative.

Les objets `SaturatedGenerator`, forêt d'intersection et certificats de complétude associés sont réservés à une future migration. Ils ne doivent pas être sérialisés sous `critical_catalog`, `gamma_cofaces` ou une base de preuve v2 existante, car leur rang, leur provenance et leurs obligations de complétude diffèrent.

## 18. Critères de réception mathématiques

La spécification est satisfaite lorsque :

1. l'oracle exhaustif vérifie toutes les tranches et flèches pour les petits nuages;
2. la tranche $k=1$ coïncide avec l'arbre de fusion de l'EMST, niveaux divisés par quatre en unités carrées;
3. le profil réduit exact est obtenu depuis les composantes du Gamma exhaustif, et le flot Gabriel brut ne revendique qu'une `partial_refinement`;
4. le profil complet coïncide avec $\Gamma_k$ à chaque intervalle entre valeurs critiques;
5. toute permutation des observations ne change que les identifiants canoniques;
6. tout lot égal produit la même multifurcation quel que soit l'ordonnancement GPU;
7. chaque carré ordre–échelle commute;
8. aucun statut `exact` n'est émis lorsqu'une fermeture, une cellule enfant canonique, une attache, `relevant_gp_complete` ou une dégénérescence reste indécise;
9. une sortie budgétée vérifie la propriété de raffinement unilatéral et n'expose jamais sa `partial_forest` comme une forêt HGP exacte.

La [feuille de route](ROADMAP_IMPLEMENTATION_MORSEHGP3D.md) transforme ces conditions en phases d'implémentation; le [plan de tests](TEST_PLAN_MORSEHGP3D.md) les rend falsifiables.
