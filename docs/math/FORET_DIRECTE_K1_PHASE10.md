# Forêt directe d'ordre un — contrat Phase 10.3

## Périmètre

Ce contrat concerne la Phase 10, le backend `reference_cpu`, le profil `hgp_reduced` et le mode `certified`. Sa porte d'entrée est satisfaite par la façade terminale de Phase 9, le journal 10.1, les graines 10.2 et le jalon local `compact_k1_forest_certified/local_k1_compact_forest_only` de Phase 5. Le résultat visé est exact pour la seule tranche d'ordre un, relativement à la façade terminale fournie comme autorité géométrique externe et aux deux journaux fraîchement rejoués; il ne revendique aucun `public_status` et ne ferme ni les ordres supérieurs, ni M.1.

Soit un nuage canonique de $n$ points distincts. Pour une paire $e=\lbrace u,v\rbrace$, posons son niveau HGP carré

$$\beta(e)=\frac{\left\Vert u-v\right\Vert^2}{4}.$$

À l'ordre un, les naissances sont les $n$ singletons de niveau zéro. Un rôle selle provient d'un événement direct de rang fermé deux. Son support positif minimal contient au moins deux points; le rang deux impose donc $I=\varnothing$, $U=\lbrace u,v\rbrace$ et exactement deux bras $F_u=\lbrace v\rbrace$ et $F_v=\lbrace u\rbrace$. Aucun calcul de miniball, aucune facette d'ordre supérieur et aucune incidence Gamma ne sont nécessaires pour les localiser.

## Lemme de descente de la boule diamétrale fermée

Soient $u\neq v$, $m=(u+v)/2$, $d^2=\left\Vert u-v\right\Vert^2$ et $z\notin\lbrace u,v\rbrace$ dans la boule fermée de centre $m$ et de rayon $\sqrt{d^2}/2$. L'identité du parallélogramme donne

$$\left\Vert z-u\right\Vert^2+\left\Vert z-v\right\Vert^2=2\left\Vert z-m\right\Vert^2+\frac{d^2}{2}\leq d^2.$$

Les deux termes de gauche sont strictement positifs. Chacun est donc strictement inférieur à $d^2$, d'où

$$\beta(\lbrace u,z\rbrace)<\beta(\lbrace u,v\rbrace),\qquad\beta(\lbrace z,v\rbrace)<\beta(\lbrace u,v\rbrace).$$

Ainsi, toute paire dont la boule diamétrale fermée contient un troisième site est remplaçable par un chemin de deux paires de niveaux strictement inférieurs. En répétant ce remplacement, les niveaux décroissent dans l'ensemble fini des niveaux de paires; la descente termine sur un chemin de paires directes certifiées, c'est-à-dire de paires dont la boule diamétrale fermée ne contient aucun site extérieur. Cette preuve accepte un troisième site sur la sphère : la décroissance des deux cordes reste stricte puisque les sites sont distincts.

## Équivalence de filtration à l'ordre un

Pour $a\geq0$, notons $G_{\mathrm{all}}^{\prec a}$ le graphe complet filtré par $\beta(e)\prec a$ et $G_{\mathrm{dir}}^{\prec a}$ son sous-graphe de paires directes, avec $\prec$ égal à $<$ ou $\leq$. Les sommets ont eux-mêmes le niveau zéro : la coupe ouverte en zéro est donc vide. Le lemme précédent remplace chaque arête absente de $G_{\mathrm{dir}}^{\prec a}$ par un chemin direct de niveau strictement inférieur au sien. Les deux graphes ont donc exactement les mêmes composantes aux coupes ouvertes et fermées :

$$\pi_0\bigl(G_{\mathrm{dir}}^{\prec a}\bigr)=\pi_0\bigl(G_{\mathrm{all}}^{\prec a}\bigr).$$

Posons $D_1(y)=\min_{x\in X}\left\Vert y-x\right\Vert^2$ et $L_1^{\prec}(a)=\lbrace y:D_1(y)\prec a\rbrace$. Le sous-niveau fermé d'ordre un est l'union des boules centrées sur les sites,

$$L_1(a)=\bigcup_{x\in X}B\bigl(x,\sqrt{a}\bigr).$$

Deux de ces boules se rencontrent exactement lorsque la paire de leurs centres vérifie $\beta(e)\leq a$; le même argument avec des boules ouvertes donne la condition stricte. Comme chaque boule est connexe, les composantes de leur union sont celles du graphe d'intersection. Le fait classique de préservation des composantes par une forêt minimum couvrante donne donc, pour les deux conventions de coupure,

$$\pi_0\bigl(L_1^{\prec}(a)\bigr)=\pi_0\bigl(G_{\mathrm{dir}}^{\prec a}\bigr)=\pi_0\bigl(G_{\mathrm{all}}^{\prec a}\bigr)=\pi_0\bigl(T_1^{\prec a}\bigr),$$

où $T_1$ est la forêt compacte certifiée de Phase 5. Cette égalité porte sur les partitions, pas sur les ensembles d'arêtes : le flux direct peut légitimement contenir davantage de paires que l'EMST.

## Snapshot strict et quotient d'un niveau égal

Fixons un niveau direct exact $a>0$. Le snapshot strict pré-lot contient les $n$ naissances et seulement les unions de niveaux strictement inférieurs à $a$. Pour chaque point $x$, notons $\rho_a^-(x)$ l'identifiant de sa racine dans ce snapshot. Une union créée exactement au niveau $a$ ne peut jamais être consultée par $\rho_a^-$.

Chaque selle $e=\lbrace u,v\rbrace$ du lot produit les deux liaisons exactes de bras vers $\rho_a^-(u)$ et $\rho_a^-(v)$. Le quotient $Q_a$ a pour sommets les racines strictes touchées et pour arêtes les paires de racines ainsi obtenues. Toutes les arêtes du niveau sont insérées dans le même quotient avant la moindre mutation. Les composantes connexes de $Q_a$ sont indépendantes de l'ordre des selles, de leur orientation et du pivot choisi par le DSU.

Pour une composante du quotient, soit $q$ le nombre de racines strictes distinctes qu'elle contient.

- Le cas $q=0$ est impossible : chaque extrémité est un singleton né au niveau zéro et $a>0$.
- Le cas $q=1$ est une continuation redondante. Il conserve l'identifiant de racine et ne crée ni nœud, ni enfant, même si plusieurs selles internes sont présentes.
- Le cas $q\geq2$ crée une unique multifusion canonique de niveau $a$ dont les $q$ enfants sont les racines strictes triées. Aucune binarisation auxiliaire ne fait partie de la sémantique.

Tous les groupes, enfants et identifiants résultants sont préparés sur le snapshot immuable. Le commit des unions ne commence qu'après résolution complète du lot. L'état post-lot devient alors, et alors seulement, le snapshot du niveau suivant. Le quotient direct et le lot correspondant de `K1CompactForest` peuvent être comparés comme différentiel indépendant par leurs partitions canoniques de racines; l'égalité de leurs arêtes n'est ni exigée ni attendue.

## Bornes de stockage et de travail

Soit $J$ le nombre total de selles directes de rang deux, $G$ le nombre de composantes de quotient non vides, $M$ le nombre de groupes créateurs et $L$ le nombre de lots exacts non vides. Les `RootBirth` ont les identifiants implicites $0,\ldots,n-1$ et n'occupent aucune arène. Les arènes persistantes contiennent exactement deux `ArmRootBinding` par selle, un `SaddleRecord` par selle, au plus un `AtomicGroup` par selle, les enfants des seuls groupes créateurs et au plus un `BatchRecord` par selle.

Chaque groupe créateur de cardinal $q\geq2$ diminue le nombre de racines de $q-1$. Par conséquent,

$$G\leq J,\qquad L\leq J,\qquad\sum_{g=1}^{M}(q_g-1)\leq n-1,\qquad M\leq n-1.$$

Le nombre total $C$ de `child_node_ids` des groupes créateurs vérifie alors

$$C=\sum_{g=1}^{M}q_g=\sum_{g=1}^{M}(q_g-1)+M\leq2n-2.$$

Les continuations $q=1$ ne contribuent ni à $C$, ni à une arène de nœuds. Le stockage logique ajouté par la tranche directe d'ordre un est donc borné par

$$2J+J+G+C+L\leq2n+5J-2.$$

Le journal étant déjà trié par niveau exact, un parcours direct de ses lots évite tout `cut` matérialisé à chaque niveau. Un DSU sur les points et racines occupe $O(n)$, tandis que le quotient du lot courant occupe $O(B_{\max})$, où $B_{\max}$ est le nombre maximal de selles partageant un niveau exact. Le scratch total est ainsi $O(n+B_{\max})$ et, avec les tris et recherches binaires locaux actuels, le travail séquentiel propre à cette couche est $O((n+J)\alpha(n)+\sum_{b}B_b\log(B_b+1))$, hors rejeu des autorités. Ces bornes sont structurellement compatibles avec les chemins 50 000 et 10 000 000+ points; elles ne constituent pas un benchmark ni une preuve de SLO.

## Autorité et limites

Le constructeur valide localement la façade terminale fournie comme autorité externe, rejoue fraîchement et sans seconde arène les journaux 10.1 et 10.2, puis construit la tranche directe sans consommer la forêt compacte K1. Cette dernière reste un témoin indépendant de Phase 5 pouvant servir à comparer les partitions canoniques. Le facteur $1/4$ entre longueur carrée et niveau HGP, la frontière stricte $<a$, la totalité des deux liaisons par selle, l'invariance sous permutation et l'atomicité du commit sont des invariants de rejet fermé.

Ce contrat est propre à $k=1$. Pour $k\geq2$, un bras $F_u$ est une facette de $k$ points et n'est pas nécessairement une racine réduite : il peut être isolé, appartenir à une composante non triviale ou avoir été rattaché silencieusement par une coface non-Gabriel. L'absence d'une facette dans un dictionnaire ne prouve donc jamais son isolation. De plus, pour $S=I\cup U$, les suppressions $S\setminus\lbrace x\rbrace$, $x\in I$, naissent au niveau égal et doivent participer au quotient avec les `GatewayAttach` du même lot.

La généralisation exige un locator total certifié, la génération complète des gateways silencieux et leur traitement simultané. La fixture permanente à cinq points, où la facette `AC` est attachée silencieusement avant d'être réutilisée, interdit de promouvoir le flot direct brut. M.1 reste nécessaire pour interpréter les attaches complètes comme reconstruction de `full_pi0`. Aucun de ces résultats ne découle de la preuve d'ordre un, qui ne construit ni Gamma, ni cellule, ni coface d'ordre supérieur, ni mosaïque de Delaunay.
