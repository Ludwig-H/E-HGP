# Certificat groupé exact pour les paires ancrées — Phase 14Q P8g

## Statut et portée

Ce document fixe un lemme de prune exact pour un groupe borné d'ancres et un nœud certifié du LBVH. Il s'agit d'une primitive `reference_cpu / hgp_reduced / grouped_exact_certificate`, sous `deployment_status=architecture_only` et `public_status=not_claimed`. Elle ne classe aucun support supérieur, ne réduit aucune hiérarchie et ne qualifie ni `warm_e2e`, ni 50 k, ni 10 M+.

Pour un couple groupe--nœud, la primitive mutualise les évaluations de $G$ ancres contre $W$ témoins en au plus $W$ prédicats universels sur la boîte du groupe. Elle conserve encore le produit nœuds × témoins et refait sa préparation à chaque appel. Le pool commun n'a aucune obligation de rappel : sa seule fonction est de présenter au prédicat exact des candidats susceptibles de fournir une preuve commune.

## Identité diamétrale

Pour une ancre $p$, un second support $q$ et un témoin $x$, on pose :

$$\Phi_x(p,q)=(x-p)\cdot(x-q).$$

Le point $x$ est strictement intérieur à la boule fermée de diamètre $[p,q]$ si et seulement si $\Phi_x(p,q)<0$. Son lien exact avec la marge ancrée $H_x$ de P8e/P8f est :

$$\Phi_x(p,q)=\left\Vert x-p\right\Vert^2-(x-p)\cdot(q-p)=-H_x(p,q).$$

## Théorème de groupe

Soit $P$ un ensemble fini non vide d'ancres, $A$ sa boîte englobante exacte, $Q$ la boîte exacte d'un nœud LBVH et $W$ un pool borné de `PointId` distincts, disjoint de $P$. Pour chaque $x\in W$, on définit :

$$M_x(A,Q)=\max_{a\in A,\;q\in Q}(x-a)\cdot(x-q).$$

Soit $s_{\max}\in\lbrace 2,\ldots,11\rbrace$ et $m=s_{\max}-1$. S'il existe $m$ témoins distincts $x_1,\ldots,x_m\in W$ tels que $M_{x_j}(A,Q)<0$ pour tout $j$, alors, pour toute ancre réelle $p\in P$ et tout point réel $y$ couvert par le nœud $Q$, les $m$ témoins sont strictement intérieurs à la boule de diamètre $[p,y]$.

En effet, $p\in A$ et $y\in Q$, donc, pour chaque témoin certifié :

$$\Phi_{x_j}(p,y)\leq M_{x_j}(A,Q)<0.$$

La stricte négativité exclut automatiquement $x_j=p$, $x_j=y$ et $p=y$. Avec les deux supports, la boule fermée contient donc au moins :

$$m+2=(s_{\max}-1)+2=s_{\max}+1$$

`PointId` distincts. La seule cardinalité strictement intérieure vaut au moins $m=s_{\max}-1$; c'est son union avec les deux supports, de cardinalité au moins $s_{\max}+1$, qui dépasse la fenêtre fermée. La conclusion utilisée par le produit est ainsi plus forte qu'un simple rang observé élevé et exclut aussi le cas diagnostique extra-shell. Le prune ne doit pas être justifié en aval par le seul libellé « rang fermé supérieur ».

## Calcul exact de la borne

Le témoin $x$ est ponctuel. Pour chaque axe, $(x_i-a_i)(x_i-q_i)$ est bilinéaire sur le rectangle indépendant formé par les deux intervalles de $A$ et $Q$; son maximum est atteint sur l'une des quatre paires d'extrémités. La somme des trois maxima axiaux donne donc exactement $M_x(A,Q)$.

L'implémentation réutilise `exact_diametral_phi_aabb_maximum_sign(A, Q, {x})`. Le chemin dyadique borné décide la plage binary64 ordinaire; si cette enveloppe est insuffisante, le fallback `BigInt` exact intervient avant toute décision. La primitive n'alloue aucune arène persistante ou proportionnelle à $n$, mais elle ne revendique pas une absence littérale d'allocation transitoire.

## Le pool est propositionnel

Un témoin n'a pas besoin d'appartenir à la banque de chaque ancre. Dès que $M_x(A,Q)<0$, la quantification universelle sur $A\times Q$ fournit toute l'autorité nécessaire. Le pool peut donc provenir d'une banque graine, d'un voisin représentatif ou d'un producteur device borné. Une omission ne crée qu'un résultat inconclusif et ne peut supprimer une paire pertinente.

Le contrat courant impose seulement un pool strictement croissant, sans doublon, de taille au plus 64 et disjoint du groupe d'au plus 32 ancres. Aucun tableau $64n$ n'est construit.

## Perte de rappel de la relaxation AABB

La boîte du groupe contient des coins hybrides qui ne sont pas nécessairement des ancres réelles. Cette relaxation est donc suffisante, jamais nécessaire. Considérons :

$$P=\lbrace (1,-2,0),(-2,1,0)\rbrace,\qquad q=(1,1,0),\qquad x=(0,0,0).$$

Pour chacune des deux ancres réelles, $\Phi_x(p,q)=-1$. Pourtant, la boîte $A=[-2,1]\times[-2,1]\times\lbrace 0\rbrace$ contient le coin hybride $a=(1,1,0)$ et :

$$\Phi_x(a,q)=2.$$

Le certificat groupé répond donc correctement « inconclusif ». Les groupes devront être spatialement cohérents et un échec devra se diviser ou retomber sur le parcours individuel; il ne pourra jamais être interprété comme une absence de témoin.

## Provenance et atomicité

Un reçu positif est lié simultanément à l'identité process-local du nuage et du LBVH, à l'indice du nœud, à sa plage de feuilles, à sa boîte exacte, au rang fermé maximal demandé et à la liste canonique des ancres. Tout ce payload scientifique est privé après construction. Son rejeu doit appeler `certifies(...)` avec le nœud, le rang et les ancres attendus; un simple test de `certified()` ne suffit pas à l'appliquer à un autre contexte.

Les caps d'ancres, de témoins et de prédicats exacts sont vérifiés avant l'opération correspondante. Si un cap manque après quelques succès stricts, aucun masque ni témoin partiel n'est publié. Un résultat inconclusif ou épuisé n'a aucune autorité de prune.

## Bornes et structures évitées

Pour $G\leq32$ ancres et $W\leq64$ témoins, la construction de la boîte coûte $O(G)$ et un nœud coûte au plus $O(W)$ prédicats exacts, avec arrêt dès $m\leq10$ succès. Le payload fixe d'identifiants occupe au plus $8(32+64+10)=848$ octets, hors métadonnées et jetons process-local.

Cette primitive ne construit ni corde par nœud, ni allocator global de records, ni tableau par ancre du nuage complet, ni catalogue de paires, facettes, cofaces ou incidences, ni cellule, Gamma ou mosaïque de Delaunay d'ordre supérieur. Le LBVH certifié existant reste la seule structure globale géométrique.

## Limite de l'incrément hôte

P8g ouvre seulement le certificat exact d'un nœud authentifié. Il ne prépare pas encore $A$ et le pool une seule fois pour tout un parcours et ne transporte pas encore vers les enfants un masque typé de témoins déjà stricts. Or la monotonie $Q'\subseteq Q$ permet de conserver tout succès strict du parent. Le prochain incrément d'implémentation devra encapsuler cette préparation et cet héritage dans un curseur non forgeable; accepter un masque brut fourni par l'appelant serait interdit.

Aucun benchmark GCP n'est autorisé sur ce seul oracle hôte. Le premier gate futur restera 4 096 points, puis le même 50 k seulement si l'autorité recertifiée reste identique et si le parcours groupé réduit effectivement le nombre de prédicats.
