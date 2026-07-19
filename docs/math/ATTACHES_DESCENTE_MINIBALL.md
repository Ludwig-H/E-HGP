# Attaches par descente K-NN–miniball

> **Statut.** La descente est un théorème sous supports essentiels, K-NN exacts et absence de shell extérieur. Elle attache correctement les bras d'un événement déjà découvert. Elle ne prouve ni la complétude du catalogue, ni la correction sur les plateaux dégénérés.

## 1. Rôle dans MorseHGP3D

Le flot de Gabriel brut ne suffit pas au profil exact `hgp_reduced`. La [preuve par incidences silencieuses](INCIDENCES_SILENCIEUSES_GAMMA.md) montre qu'une facette peut être attachée par une coface non-Gabriel sans changement immédiat de l'union de points. La descente remplit trois fonctions :

1. attacher les bras du profil `full_pi0` à des minima globaux;
2. comprimer un grand graphe de facettes en un DAG de successeurs;
3. localiser une racine pour une future réduction complétée en incidences et fournir un contrôle croisé indépendant de l'hyper-Kruskal Gamma.

Elle n'autorise pas à elle seule une promotion du flot brut : la complétude de toutes les facettes-portes et de leurs premiers niveaux d'incidence doit aussi être certifiée.

## 2. Enveloppe des facettes

Pour $F\subseteq X$, $\lvert F\rvert=k$, définissons

$$g_F(y)=\max_{x\in F}\left\Vert y-x\right\Vert^2.$$

Le champ K-NN dur est l'enveloppe inférieure

$$D_k(y)=\min_{\lvert F\rvert=k}g_F(y).$$

La valeur minimale de l'atome est

$$\beta(F)=\min_y g_F(y)=g_F(c_F),$$

où $c_F$ est le centre de la miniball de $F$. Pour $k\leq10$ en dimension trois, cette miniball peut être obtenue en testant tous les supports de taille au plus quatre; il y en a au plus

$$\sum_{j=1}^{4}\binom{10}{j}=385.$$

Cette énumération fixe est plus déterministe qu'une récursion de Welzl dans un noyau GPU.

## 3. Facettes top-$k$ exactes

Au point $y$, soit $r_k^2=D_k(y)$ et posons

$$E_{<}(y)=\left\lbrace x:\left\Vert y-x\right\Vert^2<r_k^2\right\rbrace,$$

$$E_{=}(y)=\left\lbrace x:\left\Vert y-x\right\Vert^2=r_k^2\right\rbrace.$$

La famille exacte des choix top-$k$ est

$$\mathcal{N}_k(y)=\left\lbrace E_{<}(y)\cup A:A\subseteq E_{=}(y),\ \lvert A\rvert=k-\lvert E_{<}(y)\rvert\right\rbrace.$$

Cette définition conserve les ex aequo. Une règle par identifiant peut choisir un représentant pour un chemin non dégénéré, mais elle ne doit pas supprimer le shell du certificat.

## 4. Hypothèse locale de descente

Pour toute facette visitée $F$, soit

$$U(F)=F\cap\partial B\bigl(c_F,\sqrt{\beta(F)}\bigr).$$

On suppose :

- $U(F)$ affinement indépendant;
- $c_F\in\mathrm{relint}\,\mathrm{conv}(U(F))$;
- $U(F)$ unique support minimal;
- aucun point de $X\setminus F$ sur la frontière de la miniball.

Tous les points de $U(F)$ sont alors essentiels : retirer l'un d'eux et n'ajouter que des points strictement intérieurs diminue strictement le rayon minimal.

## 5. Successeur et décroissance

Choisissons

$$\mathrm{succ}(F)\in\mathcal{N}_k(c_F).$$

Si $F\notin\mathcal{N}_k(c_F)$, un point extérieur est strictement intérieur à la miniball de $F$. Tout choix top-$k$ remplace au moins un support essentiel par des points intérieurs. Par essentialité,

$$\beta\bigl(\mathrm{succ}(F)\bigr)<\beta(F).$$

La valeur $\beta$ décroît donc strictement à chaque transition non stationnaire. La suite ne cycle pas et termine sur une facette $F_{\star}$ active à son propre centre.

Comme aucun point extérieur ne se trouve dans la miniball terminale fermée, le centre $c_{F_{\star}}$ est un minimum local de $D_k$ de rang fermé $k$, donc d'indice zéro.

## 6. Certificat de chemin sous-niveau

Soit $G=\mathrm{succ}(F)$ l'arc strict déjà certifié et le segment

$$\gamma(t)=(1-t)c_F+t c_G,\qquad0\leq t\leq1.$$

Posons $R=\beta(F)$, $a=g_G(c_F)=D_k(c_F)$, $b=g_G(c_G)=\beta(G)$ et $\delta=\left\Vert c_G-c_F\right\Vert^2$. Le certificat d'arc donne exactement $a\leq R$ et $b<R$; le calcul exact du déplacement donne $\delta\geq0$. Pour chaque $x\in G$, l'identité quadratique est

$$\left\Vert\gamma(t)-x\right\Vert^2=(1-t)\left\Vert c_F-x\right\Vert^2+t\left\Vert c_G-x\right\Vert^2-t(1-t)\delta.$$

Le terme de déplacement est commun à tous les points, mais le maximiseur peut changer entre les deux extrémités. En prenant le maximum, on obtient donc une inégalité, pas une identité pour l'enveloppe :

$$g_G(\gamma(t))\leq q(t)=(1-t)a+tb-t(1-t)\delta.$$

Pour $0\leq t\leq1$,

$$q(t)-R=(1-t)(a-R)+t(b-R)-t(1-t)\delta.$$

Chaque terme est non positif sur le segment fermé, donc $D_k(\gamma(t))\leq g_G(\gamma(t))\leq R$ pour $0\leq t\leq1$. Pour $0<t\leq1$, le terme $t(b-R)$ est strictement négatif, donc

$$D_k(\gamma(t))\leq g_G(\gamma(t))\leq q(t)<R.$$

Le point initial peut satisfaire $a=R$ : le certificat universel strict porte alors sur le demi-segment $\gamma((0,1])$, tandis que le segment fermé $\gamma([0,1])$ n'est certifié que dans le sous-niveau fermé. Si $a<R$, le point initial est lui aussi strict, mais ce renforcement est enregistré séparément. Aucun échantillonnage de $t$ n'est nécessaire.

Le cas $\delta=0$ est valide. L'exactitude rationnelle impose alors $c_F=c_G$ et donc $a=b$; le segment dégénère en un point déjà strict sous $R$. Rejeter systématiquement des centres égaux serait incorrect. À l'inverse, $\delta=0$ avec des centres différents ou avec $a\neq b$ est une contradiction fail-closed.

Le certificat machine `exact_squared_distance_chord_identity_max_envelope_half_open_segment_v1` porte seulement sur cet arc et la portée `canonical_strict_arc_half_open_sublevel_segment_only`. Les branches active ou non prise en charge ne contiennent aucun témoin de segment.

### 6.1 Chaîne canonique stricte

Le jalon 6.5 itère exactement ce constructeur sur une seule source. Écrivons les facettes engagées $F_0,\ldots,F_L$. Chaque transition conserve leur cardinal $k$ et rejoue la comparaison exacte $\beta(F_{i+1})<\beta(F_i)$. Deux facettes ne peuvent donc pas se répéter. Comme il existe $\binom{n}{k}$ facettes de cardinal $k$, toute orbite stricte contient au plus $\binom{n}{k}-1$ segments.

La couture entre deux segments est une égalité de témoins complets : la facette, le centre rationnel et le niveau $\beta(F_{i+1})$ de la cible précédente doivent être identiques à ceux de la source fraîchement reconstruite. Cette égalité ne force pas le niveau atomique suivant $D_k(c_{F_{i+1}})$ à être égal à $\beta(F_{i+1})$; il peut lui être strictement inférieur. Le rejeu conserve donc séparément les nœuds de miniball et les coefficients analytiques de chaque segment.

Posons $R_0=\beta(F_0)$. Le premier segment privé de son point source est strictement sous $R_0$. Pour $i\geq1$, le segment fermé issu de $F_i$ est contenu dans le sous-niveau de $\beta(F_i)$, lui-même strictement inférieur à $R_0$. La polyligne entière appartient donc à $\left\lbrace D_k\leq R_0\right\rbrace$ et la polyligne privée de son premier point appartient à $\left\lbrace D_k<R_0\right\rbrace$.

`build_exact_facet_descent_chain` exige une politique externe explicite, dont le budget zéro est valide et dont le backend de référence borne la valeur à 4096 segments engagés. Un arrêt sur une facette régulière active certifie la fin de cette orbite. Un arrêt sur une dégénérescence non prise en charge ou sur la frontière du budget ne certifie que le préfixe strict effectivement engagé; le `stopping_probe` complet conserve la raison exacte et, dans le cas budgétaire, le prochain segment strict non engagé. Le booléen `finite_strict_facet_orbit_theorem_certified` certifie seulement l'existence de la borne combinatoire précédente; il n'affirme pas que le budget effectif couvre toutes les facettes. La base `exact_replayed_half_open_segments_exact_seams_strict_facet_potential_finite_orbit_v1` et la portée `single_source_canonical_strict_descent_chain_only` ne certifient encore ni germe initial depuis un centre critique, ni fermeture de plusieurs sources en DAG, ni attache globale, forêt ou statut public.

## 7. Bras d'un point critique d'indice un

Soit un événement critique de rang fermé $k+1$ :

$$S=I\cup U,\qquad\lvert S\rvert=k+1,\qquad c\in\mathrm{relint}\,\mathrm{conv}(U).$$

Pour chaque $u\in U$, la facette active du bras est

$$F_u=S\setminus\lbrace u\rbrace.$$

Reani–Bobrowski donnent, pour l'indice général $\mu$, la multiplicité locale

$$\Delta_{\mu}(c)=\binom{\lvert U\rvert-1}{\mu}.$$

À l'indice un, $\Delta_1=\lvert U\rvert-1$ et le sous-niveau strict possède exactement les $\lvert U\rvert$ germes $F_u$. Ce n'est donc pas une selle binaire : si les bras s'attachent à $q$ racines globales distinctes, le centre tue $q-1\leq\lvert U\rvert-1$ classes de $H_0$. Avec $\lvert U\rvert=3$, une triple fusion est possible en un seul événement.

Retirer le support essentiel $u$ donne

$$\beta(F_u)<\beta(S).$$

Le segment de $c$ vers $c_{F_u}$ entre immédiatement dans $\left\lbrace D_k<\beta(S)\right\rbrace$ et dans le germe local correspondant à $u$. La descente depuis $F_u$ reste dans ce sous-niveau et termine au minimum de la composante globale du bras.

Si plusieurs bras terminent dans la même racine antérieure, l'événement est partiellement ou totalement redondant. La multiplicité locale est une capacité de changement, pas le nombre de classes effectivement tuées globalement.

## 8. DAG fonctionnel et GPU

Pour toutes les facettes actives requises par le catalogue, on peut calculer les successeurs en parallèle. Sous les hypothèses strictes, les arcs non stationnaires diminuent $\beta$; le graphe est un DAG fonctionnel orienté vers les minima.

Le calcul GPU suit :

1. miniball exacte de chaque facette;
2. requête top-$k$ exacte au centre;
3. émission du successeur et du certificat de décroissance;
4. tri et déduplication des nouvelles facettes;
5. répétition jusqu'à fermeture;
6. pointer-jumping pour propager les racines;
7. comparaison des racines avec le DSU de Gabriel.

Le pointer-jumping accélère la propagation; il ne remplace pas la certification de chaque arc.

## 9. Longueur et stockage des chemins

La finitude donne seulement la borne combinatoire $\binom{n}{k}-1$, inutilisable en pratique. Le nombre moyen d'étapes doit être mesuré par famille de données. Les compteurs obligatoires sont : médiane, p95, maximum, nombre de facettes nouvelles et taux de réutilisation des successeurs.

Trois niveaux d'audit sont proposés :

| niveau | stockage |
|---|---|
| `roots_only` | racine terminale, nombre d'étapes et hash du chemin |
| `replay` | identifiants de toutes les facettes et témoins de centres |
| `full_geometry` | en plus, intervalles et fallbacks de chaque segment |

Le statut exact nécessite au moins un chemin rejouable ou un engagement cryptographique accompagné des données temporaires conservées jusqu'à validation.

## 10. Plateaux

Si un point extérieur se trouve exactement sur la sphère ou si le support n'est pas essentiel, plusieurs successeurs peuvent conserver la même valeur. Une règle arbitraire peut alors cycler.

Le modèle correct devient un graphe multivalué :

- un arc $F\to G$ existe pour $G\in\mathcal{N}_k(c_F)$;
- les arcs sont classés en constants ou strictement descendants;
- les composantes fortement connexes d'arcs constants sont contractées;
- le quotient doit être acyclique pour fournir des attaches.

Il reste à démontrer que cette contraction représente tous les germes de sous-niveau dans les cas cosphériques. Avant cela, un plateau rencontré bloque le profil exact; il n'est pas résolu par une perturbation cachée.

## 11. Vérifications différentielles

Pour chaque facette visitée sur les petits nuages :

- comparer la miniball à l'énumération exhaustive des supports;
- comparer le shell top-$k$ à la force brute;
- vérifier exactement $\beta(F')<\beta(F)$;
- vérifier la majoration exacte par $q(t)$ issue de l'identité quadratique; tout échantillonnage, notamment au milieu, reste seulement diagnostique;
- vérifier que la racine terminale est un événement de rang $k$ du catalogue;
- comparer l'attache à la composante de $\Gamma_k$ strictement antérieure;
- vérifier l'absence de cycle sous toute permutation d'identifiants.

Pour $k=1$, toutes les facettes sont déjà des minima au niveau zéro; la descente est stationnaire. Cela confirme qu'une régularisation locale ne remplace pas l'EMST pour construire la hiérarchie.

## 12. Place exacte dans les profils

| usage | nécessité | statut |
|---|---|---|
| `hgp_reduced` exact par Gamma exhaustif | facultative | contrôle croisé seulement |
| `hgp_reduced` sparse complété en incidences | nécessaire au locator proposé, mais non suffisante | complétude algorithmique encore ouverte |
| `full_pi0` sous position générale | nécessaire tant qu'aucun autre oracle global n'est prouvé | conditionnelle aux chemins certifiés et à la fermeture de M.1 |
| plateau ou shell dégénéré | insuffisante dans la forme stricte | programme de recherche |
| mode budgété | utile | attaches retournées vérifiées, complétude conditionnelle |

## Références

- Y. Reani et O. Bobrowski, [*Morse Theory for the k-NN Distance Function*](https://arxiv.org/abs/2403.12792).
- E. Welzl, [*Smallest Enclosing Disks (Balls and Ellipsoids)*](https://doi.org/10.1007/BFb0038202).
- L. Hauseux, [*Manuscrit de thèse*](../references/MANUSCRIT_THESE_HAUSEUX.pdf), chapitres 6 à 8.
