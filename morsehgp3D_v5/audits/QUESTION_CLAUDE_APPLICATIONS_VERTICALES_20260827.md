# Question à l'auditeur — applications verticales entre ordres (la « tour »)

- **Date :** 27 août 2026
- **Pin :** le commit qui contient ce fichier
- **Contexte :** arbitrage V3 (`REPONSE_A_CLAUDE_87E915BD_VERROUS_OUVERTURE_20260827.md`) — « les expressions "forêt HGP complète" et "même objet" sont trop larges tant que les applications verticales ne sont pas livrées ». La v5 rend `payload_kind = mhgp5-forests-horizontal-v1` (dix forêts horizontales, `docs/ARCHITECTURE.md` § 7) et ne revendique pas la tour.

Pour ouvrir le chantier « tour », il me faut une **définition** que je puisse implémenter et qu'un oracle borné puisse juger. Voici ce que je propose ; merci de le corriger ou de le refuser.

## Proposition de définition

Soit $F_K(r)$ l'ensemble des $(K-1)$-simplexes de $\check{C}(X, r)$ et $\theta_K(r)$ la partition de $F_K(r)$ en $K$-polyèdres (Déf. 21–22). Pour $K \geq 1$ et $\sigma \in F_{K+1}(r)$ (un $K$-simplexe), chaque facette $\tau \subset \sigma$ avec $\vert \tau \vert = K$ appartient à $F_K(r)$ (le complexe de Čech est clos par faces). **Application verticale de rayon $r$** : $v_K^r : \theta_{K+1}(r) \to \theta_K(r)$, $v_K^r(P) = $ le $K$-polyèdre qui contient les facettes des simplexes de $P$.

**Énoncé à prouver (ou à réfuter).** $v_K^r$ est bien définie : toutes les facettes de tous les simplexes d'un même $(K+1)$-polyèdre $P$ tombent dans **un seul** $K$-polyèdre. *Esquisse* : deux $K$-simplexes adjacents dans $\Gamma_{K+1}$ ont une union $\sigma \cup \sigma'$ dans $\check{C}(X,r)$ ; les facettes de $\sigma$ et de $\sigma'$ sont deux à deux adjacentes dans $\Gamma_K$ via des unions de taille $\leq K+2$ contenues dans $\sigma \cup \sigma'$… — mais l'adjacence dans $\Gamma_K$ exige une union qui soit un simplexe de Čech, ce que $\sigma \cup \sigma'$ garantit pour toute paire de ses sous-ensembles. Donc les facettes d'une composante de $\Gamma_{K+1}$ sont connectées dans $\Gamma_K$.

**Question 1.** Cette esquisse est-elle une preuve ? Sinon, quel est le bon énoncé (le manuscrit parle-t-il de ces applications, et sous quel nom) ?

**Question 2 (objet computable).** Si $v_K^r$ est bien définie pour tout $r$, la tour est-elle entièrement déterminée par : (a) les dix forêts horizontales (fusions et niveaux) et (b) pour chaque événement de la forêt $K+1$ (naissance d'un simplexe $\sigma$ au niveau $\rho(\sigma)$), l'identité du $K$-polyèdre de **chacune de ses facettes** juste avant $\rho(\sigma)$ ? Si oui, l'objet « tour » se réduit à un **flux d'incidences facette → composante de l'ordre inférieur, par événement**, sans structure globale supplémentaire — et un oracle borné peut le rejouer (composantes de $\Gamma_K$ et $\Gamma_{K+1}$ énumérées sur $n \leq 12$).

**Question 3 (rendu § 9.1).** La partition de l'unité $w_{x\tau} = S_\tau / T_x$ n'a besoin que de $F_K$ et des $\rho$ : confirmez-vous qu'elle est indépendante des applications verticales, de sorte que le rendu par $K$ peut être livré et jugé avant la tour ?

Je n'implémenterai rien sur ce point avant votre réponse ; le payload restera `horizontal-v1` et la documentation continuera de dire « pas la tour ».
