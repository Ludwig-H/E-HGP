# Pas exact de descente de facette sparse — Phase 10.5b

## Portée et statut

Phase 10, backend `reference_cpu`, profil `hgp_reduced`, mode `certified`, sémantique interne `partial_refinement`. La porte d'entrée de la phase est satisfaite. Le `public_status` reste `not_claimed`.

L'incrément 10.5b livre un seul pas géométrique exact depuis une facette vers un choix top-k canonique, avec statut logiciel `validated_host_software`. Il ne ferme pas une chaîne de descente et ne publie une résolution positive que si la facette source ou la facette cible est déjà liée dans le locator positif `const` fourni par l'appelant. Le sens HGP de cette liaison reste conditionnel au rejeu de l'autorité externe de l'appelant.

Le chemin ne construit ni coupe Gamma, ni catalogue global de facettes, ni cellule, ni coface, ni incidence globale, ni mosaïque de Delaunay d'ordre supérieur. L'oracle Gamma exhaustif reste réservé aux falsifications bornées.

## Données exactes d'un pas

Soit une facette source $F$ de cardinal $k$, avec $1\leq k\leq K\leq10$. Sa miniball locale exacte fournit un centre $c_F$ et un niveau $\beta(F)$. Une requête LBVH top-k exacte et complète au centre $c_F$ fournit :

- le cutoff exact $d=d_k(c_F)$;
- tous les points strictement sous $d$;
- tout le shell d'égalité à $d$;
- un choix canonique $G$ de cardinal $k$.

Une seconde miniball locale exacte fournit $c_G$ et $\beta(G)$. Le pas strict n'est publié que si les comparaisons fraîches établissent $d\leq\beta(F)$, $\beta(G)\leq d$ et $\beta(G)<\beta(F)$. Une observation contraire aux deux premières relations est une contradiction fermée, jamais une approximation réparée silencieusement.

Le niveau du lot est fermé : l'entrée géométrique admissible est $\beta(F)\leq a$. L'égalité $\beta(F)=a$ ne doit donc pas être rejetée.

## Théorème du segment source-ouvert

### Énoncé

Supposons la miniball de $F$, la partition top-k complète et la miniball de $G$ exactes. Supposons aussi $\beta(G)<\beta(F)\leq a$. Pour $z(t)=(1-t)c_F+tc_G$, tout point de $G$ est strictement sous le niveau source, donc sous le niveau fermé du lot, pour tout $t\in(0,1]$. Le segment fermé entier est strictement sous $a$ si et seulement si $d<a$.

### Preuve

Puisque $F$ contient $k$ points à distance carrée au plus $\beta(F)$ de $c_F$, le k-ième niveau vérifie $d\leq\beta(F)$. Le choix canonique $G$ contient au moins un point du shell, donc

$$\max_{x\in G}\left\Vert c_F-x\right\Vert^2=d.$$

La boule centrée en $c_F$ de rayon carré $d$ contient $G$, d'où $\beta(G)\leq d$. Pour tout $x\in G$ et tout $t\in[0,1]$, l'identité exacte de la corde donne

$$\left\Vert z(t)-x\right\Vert^2=(1-t)\left\Vert c_F-x\right\Vert^2+t\left\Vert c_G-x\right\Vert^2-t(1-t)\left\Vert c_G-c_F\right\Vert^2.$$

Par les bornes aux deux extrémités,

$$\left\Vert z(t)-x\right\Vert^2\leq(1-t)d+t\beta(G)-t(1-t)\left\Vert c_G-c_F\right\Vert^2.$$

Pour $t>0$, la relation $d\leq\beta(F)$ et l'inégalité stricte $\beta(G)<\beta(F)$ rendent le membre de droite strictement inférieur à $\beta(F)$, donc inférieur à $a$. À $t=0$, le maximum vaut exactement $d$. Le segment fermé est ainsi strictement sous $a$ exactement lorsque $d<a$. $\square$

Cette distinction est conservée dans deux booléens séparés : le certificat source-ouvert est toujours vrai sur une branche stricte, tandis que le certificat du segment fermé peut être faux lorsque $d=a$.

## Algorithme livré

Un appel exécute au plus les opérations suivantes, dans cet ordre :

1. sonder la clé source dans le locator positif sans mutation;
2. en cas de hit, rendre immédiatement la résolution relative;
3. en cas de miss, construire et rejouer la miniball locale de $F$;
4. refuser seulement si $\beta(F)>a$;
5. exécuter une tentative LBVH top-k complète avec sept plafonds distincts;
6. consommer transitoirement la partition complète, puis construire le choix canonique $G$;
7. si $G=F$, rendre `unresolved` sans conclure à l'activité;
8. construire et rejouer la miniball locale de $G$, puis exiger $\beta(G)<\beta(F)$;
9. certifier le témoin de segment et sonder $G$ dans le même locator `const` pré-appel;
10. rendre une résolution relative seulement sur hit; tout miss ou épuisement reste sans isolation, singleton ou attache inventée.

Le résultat ne conserve ni `TopKPartition`, ni shell, ni vecteur global des points extérieurs. Il conserve uniquement des clés de largeur au plus dix, des centres et niveaux exacts, des compteurs, le témoin constant du segment et, le cas échéant, un handle déjà présent.

## Plafonds et échec fermé

La tentative top-k possède des plafonds séparés pour :

- les visites de nœuds;
- les expansions de nœuds internes;
- les évaluations exactes de bornes AABB;
- les évaluations exactes de distances aux points;
- la taille de la frontière;
- la taille du tas des meilleurs voisins;
- le nombre de `PointId` retenus dans le shell du cutoff.

Chaque plafond est vérifié avant l'opération ou l'allocation correspondante. Une capacité de meilleurs voisins inférieure à $k$ échoue avant toute allocation proportionnelle à $k$. Une expansion interne vérifie simultanément la place de ses deux enfants et les deux évaluations AABB avant de modifier la frontière. Un shell provisoire peut déborder puis redevenir admissible si le cutoff diminue; si le shell final déborde, le parcours peut être complet mais aucune partition scientifique n'est publiée.

Les sondes source et cible budgètent séparément les slots du hash et les sauts de parents DSU. Aucun épuisement ne transforme une absence de résultat en composante isolée.

Cette API est une tentative bornée et recommençable depuis ses entrées. Elle ne fournit encore ni checkpoint de parcours, ni reprise incrémentale, ni annulation coopérative.

## Fixtures permanentes d'égalité au lot

### Gateway silencieuse `AC` vers `DE`

Avec les identifiants canoniques `D=0, A=1, B=2, C=3, E=4`, la source est $F=AC=\{1,3\}$ et la cible canonique est $G=DE=\{0,4\}$. Les valeurs exactes sont :

| quantité | valeur |
|---|---:|
| niveau fermé $a$ | $33/2$ |
| $c_F$ | $(1/2,2,7/2)$ |
| $\beta(F)$ | $33/2$ |
| cutoff $d$ | $31/2$ |
| $c_G$ | $(2,1/2,3/2)$ |
| $\beta(G)$ | $9/2$ |
| $\left\Vert c_G-c_F\right\Vert^2$ | $17/2$ |

Cette fixture interdit définitivement le garde erroné $\beta(F)<a$ : l'égalité au lot est admissible et le segment fermé est ici strict parce que $d<a$.

### Frontière source-ouverte `LR` vers `LP`

Avec `Q=0, L=1, P=2, R=3`, la source est $F=LR=\{1,3\}$ et le choix canonique est $G=LP=\{1,2\}$. On a $a=\beta(F)=d=1$, $c_F=(0,0,0)$, $c_G=(-1/2,1/4,0)$ et $\beta(G)=5/16$.

Le segment source-ouvert est strictement sous le lot, mais son extrémité source ne l'est pas. Le certificat `closed_segment_strict_below_closed_batch_level` doit donc être faux. Cette fixture empêche de confondre le théorème source-ouvert avec un théorème sur tout le segment fermé.

## Bornes d'architecture

Une passe de miniball locale énumère au plus $\sum_{j=1}^{4}\binom{10}{j}=385$ supports, borne indépendante de $n$. Le builder certifié calcule puis rejoue cette passe; un pas qui construit les deux miniballs exécute donc au plus quatre passes et examine au plus 1 540 supports candidats. La seule opération dépendant globalement du nuage est la requête LBVH top-k, bornée par l'appelant et sans sortie partielle en cas d'épuisement.

Le pire cas d'une tentative complète reste linéaire en $n$; la frontière et un shell d'égalité peuvent aussi devenir linéaires. Le code ne revendique donc ni le SLO sous la seconde à 50 000 points, ni la qualification 10 M+. En revanche, il ne contient aucun terme $\binom{n}{k}$ et n'alloue pas `requested_rank` avant d'avoir vérifié le plafond correspondant, ce qui préserve l'orientation vers les très grands nuages.

## Limites scientifiques et prochain incrément

Le pas 10.5b ne prouve pas :

- qu'un choix canonique identique à la source est actif;
- qu'une cible absente du locator n'appartient à aucune composante;
- que toutes les gateways silencieuses utiles seront découvertes;
- qu'une suite de pas stricts atteint toujours une liaison positive;
- la première incidence, les attaches de forêt supérieure ou la propriété M.1;
- un statut public `exact`.

Le prochain incrément 10.5c doit fermer une suite bornée de pas stricts, partager les suffixes déjà certifiés sans dupliquer les partitions top-k et conserver les facettes latentes ou gateways nécessaires. Cette fermeture devra rester relative au même snapshot d'autorité, échouer fermée sur plateau, cycle ou budget, et être confrontée à l'oracle Gamma borné sans l'introduire dans le chemin produit.

La validation ciblée GCC Release exécute six CTests en 4,16 secondes : LBVH budgété, miniball historique après extraction, locator lié à la clé, pas composé et deux contrôles statiques/symboliques. Elle rejette aussi l'ordre de parcours invalide avant un hit source qui court-circuite la géométrie. Aucun benchmark long ni GCP n'a été utilisé pour cette validation.
