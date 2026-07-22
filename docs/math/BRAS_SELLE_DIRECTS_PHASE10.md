# Bras factorisés des selles directes — contrat Phase 10.2

## Périmètre

Ce contrat concerne le backend `reference_cpu`, le profil `hgp_reduced`, le mode `certified` et la sémantique interne `partial_refinement`. Il consomme une façade terminale de Phase 9 comme autorité géométrique externe et son journal de rôles Phase 10.1 fraîchement rejoué. Il ne construit ni composante, ni racine, ni forêt, ni `GatewayAttach`.

Soit un événement direct régulier de boule fermée exacte $B(c,a)$, avec ensemble strictement intérieur $I$, shell complet $U$ et saturé $S=I\cup U$. La certification de Phase 9 impose que $U$ soit un support positif minimal, que $2\leq\lvert U\rvert\leq4$, que $s=\lvert S\rvert\leq K_{\mathrm{eff}}+1\leq11$ et que le rôle selle appartienne à l'ordre $k=s-1$.

## Théorème du bras supprimé

Pour chaque $u\in U$, on définit la facette initiale $F_u=S\setminus\lbrace u\rbrace$.

**Proposition 10.2.1.** La famille contient exactement $\lvert U\rvert$ bras deux à deux distincts, chaque $F_u$ possède exactement $k$ points et sa miniball vérifie $\beta(F_u)<a$.

**Preuve.** La cardinalité découle de $\lvert S\rvert=s$ et de la suppression d'un unique point. Si $u\neq v$, alors $u\in F_v$ mais $u\notin F_u$, donc les deux facettes sont distinctes. La boule $B(c,a)$ contient $F_u$, d'où $\beta(F_u)\leq a$. Supposons $\beta(F_u)=a$. L'unicité de la plus petite boule englobante impose alors que $B(c,a)$ soit aussi la miniball de $F_u$. Ses points actifs dans $F_u$ sont exactement $U\setminus\lbrace u\rbrace$, car tous les points de $I$ sont strictement intérieurs. La condition d'optimalité d'une miniball placerait donc $c$ dans $\mathrm{conv}(U\setminus\lbrace u\rbrace)$. Or la positivité minimale de $U$ place $c$ dans l'intérieur relatif de $\mathrm{conv}(U)$ et rend chaque sommet essentiel; en particulier $c\notin\mathrm{conv}(U\setminus\lbrace u\rbrace)$. Contradiction, donc $\beta(F_u)<a$.

Le théorème identifie des germes locaux, pas leurs composantes globales. Deux événements distincts peuvent reconstruire la même facette; leurs graines restent distinctes parce que leur provenance `(événement, point retiré)` est différente. Le nombre de composantes strictes incidentes et le nombre de fusions ne se déduisent pas de $\lvert U\rvert$.

## Représentation factorisée

`ExactDirectSaddleArmSeedJournalResult` conserve une famille par rôle selle et une graine par point de support retiré. Il ne recopie ni $I$, ni $U$, ni $F_u$, ni centre, ni miniball. La facette est reconstruite à la demande depuis la façade de Phase 9 dans un tableau fixe de dix `PointId`, puis triée canoniquement. Les deux digests de nuage, les deux digests sémantiques et un digest local des champs utiles de chaque événement interdisent de reconstruire une graine avec une autre façade.

Si $E$ est le nombre d'événements directs, $J$ le nombre de rôles selle et $A=\sum_e\lvert U_e\rvert$, alors les bornes sont les suivantes :

$$J\leq E,\qquad A\leq4J\leq4E,\qquad J+A\leq5E.$$

Avec la borne $3n+5E$ du journal Phase 10.1, la coexistence des deux étages vérifie donc :

$$\text{stockage}_{\mathrm{logique}}\leq3n+10E.$$

Le préflight calcule ces besoins avant le rejeu du journal source et avant tout `reserve`. Le rejeu streaming de 10.1 vérifie sur place les projections, l'ensemble trié des rôles et les lots : il n'alloue pas un second journal, n'effectue aucun nouveau tri et utilise un nombre constant de records auxiliaires. Il recalcule les deux digests du nuage en deux passes séquentielles $O(n)$, puis le travail propre aux rôles et graines est $O(E+A)$. La reconstruction d'un bras utilise dix identifiants au plus. Aucun scan géométrique du nuage par bras, calcul miniball, partition globale, catalogue de facettes, cellule, coface, incidence Gamma ou mosaïque de Delaunay d'ordre supérieur n'est exécuté.

## Autorité et limites

La stricte décroissance est prouvée relativement au certificat terminal de Phase 9 fourni comme autorité externe. Le constructeur rejoue fraîchement et en streaming les projections, rôles et lots de Phase 10.1, puis vérifie leur jointure exacte avec les indices, supports, rangs, ordres et niveaux de la façade. Il ne réexécute pas la recherche géométrique complète de Phase 9.

L'ancienne famille `ExactCriticalArmFamilyResult` reste un oracle borné : son chemin courant refait des partitions globales et peut retenir des vecteurs d'extérieurs en $\Theta(n)$ pour chaque bras. Elle ne doit pas être appelée par le target produit de 10.2. Le contrôle statique vérifie aussi l'absence de symboles miniball, `critical_arm` et Gamma dans le binaire ciblé.

Restent ouverts : l'attachement aux racines du snapshot strict pré-lot, la résolution simultanée de l'hypergraphe quotient, les gateways silencieux, la preuve M.1, la réduction hiérarchique et tout `public_status`.
