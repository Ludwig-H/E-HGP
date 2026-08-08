# Vers un *Relative Neighborhood Graph* d’ordre supérieur pour HGP-Clusterer

> [!NOTE]
> **Statut :** note de recherche fondée sur les deux premières parties du manuscrit de thèse, en particulier les chapitres 2, 6, 8 et 9.  
> Elle distingue explicitement :
> 1. les résultats déjà établis dans le manuscrit ;
> 2. les résultats nouveaux démontrés dans cette note ;
> 3. les propositions algorithmiques qui demandent encore une validation expérimentale ou un complément de preuve.

## Résumé exécutif

Le critère de Gabriel utilisé dans le manuscrit est une **condition locale nécessaire** pour qu’un $K$-simplexe provoque une fusion dans la hiérarchie des $K$-polyèdres. Il n’est toutefois pas optimal : certains simplexes de Gabriel sont déjà contournables, avant leur rayon de naissance, par des simplexes de plus petit rayon.

La généralisation naturelle du *Relative Neighborhood Graph* consiste à travailler non plus sur les points de $\mathcal X$, mais sur le **graphe pondéré des $(K-1)$-facettes** $\Gamma_K(\mathcal X)$. On obtient alors deux objets complémentaires :

1. **Une réduction RNG exacte du graphe pondéré des facettes**, qui conserve les composantes connexes à tous les niveaux de filtration.
2. **Un critère géométrique local de voisinage relatif pour les $K$-simplexes**, fondé sur l’existence de chemins de contournement entre leurs facettes actives.

Sous l’hypothèse de position générale du manuscrit, ce second critère vérifie

$$
\boxed{
\operatorname{Sep}_K(\mathcal X)
\subseteq
\operatorname{RNG}^{\mathrm{HGP}}_K(\mathcal X)
\subsetneq
\operatorname{Gab}_K(\mathcal X)
}
$$

La seconde inclusion est stricte en général, déjà pour $K=2$.

Ici :

- $\operatorname{Sep}_K$ est l’ensemble des $K$-simplexes séparants ;
- $\operatorname{RNG}^{\mathrm{HGP}}_K$ est l’ensemble proposé des $K$-simplexes de voisinage relatif HGP ;
- $\operatorname{Gab}_K$ est l’ensemble des $K$-simplexes de Gabriel du manuscrit.

Pour $K=1$, cette chaîne redonne exactement

$$
\mathrm{MST}\subseteq\mathrm{RNG}\subseteq\mathrm{Gabriel}.
$$

Le gain attendu n’est pas statistique : si la construction est exacte, la hiérarchie de Hartigan $K$-NN reste inchangée. Le gain potentiel porte sur la **parcimonie**, la **mémoire** et la **parallélisation GPU**.

> [!IMPORTANT]
> Le critère RNG-HGP est immédiatement sûr comme **préfiltre des événements de fusion** : il ne supprime aucun $K$-simplexe séparant. En revanche, remplacer sans autre précaution tout le $K$-graphe de Gabriel par ce seul ensemble de simplexes exige de traiter explicitement la naissance et le rattachement des facettes. La réduction RNG au niveau des arêtes du graphe des facettes, elle, conserve exactement toute la filtration de connexité.

---

## Table des matières

- [1. Point de départ dans le manuscrit](#1-point-de-départ-dans-le-manuscrit)
- [2. Pourquoi le critère de Gabriel n’est pas optimal](#2-pourquoi-le-critère-de-gabriel-nest-pas-optimal)
- [3. Généralisation canonique du RNG au graphe des facettes](#3-généralisation-canonique-du-rng-au-graphe-des-facettes)
- [4. Critère géométrique RNG-HGP au niveau des simplexes](#4-critère-géométrique-rng-hgp-au-niveau-des-simplexes)
- [5. Résultat principal](#5-résultat-principal)
- [6. Inclusion stricte dans les simplexes de Gabriel](#6-inclusion-stricte-dans-les-simplexes-de-gabriel)
- [7. Ce que le critère améliore réellement](#7-ce-que-le-critère-améliore-réellement)
- [8. Génération sans mosaïque de Delaunay d’ordre supérieur](#8-génération-sans-mosaïque-de-delaunay-dordre-supérieur)
- [9. Pipeline GPU proposé en dimension 3](#9-pipeline-gpu-proposé-en-dimension-3)
- [10. Exactitude, approximations et limites](#10-exactitude-approximations-et-limites)
- [11. Comparaison des trois niveaux de filtrage](#11-comparaison-des-trois-niveaux-de-filtrage)
- [12. Recommandation](#12-recommandation)
- [13. Travaux à mener](#13-travaux-à-mener)
- [14. Références internes au manuscrit](#14-références-internes-au-manuscrit)

---

## 1. Point de départ dans le manuscrit

### 1.1 Cas classique : $K=1$

Le manuscrit rappelle la chaîne euclidienne classique

$$
\mathrm{MST}
\subseteq
\mathrm{RNG}
\subseteq
\mathrm{Gabriel}
\subseteq
\mathrm{Delaunay}.
$$

Pour une arête $\{x,x'\}$, le critère RNG élimine l’arête lorsqu’il existe un point $z$ tel que

$$
\max\bigl(\lVert x-z\rVert,\lVert x'-z\rVert\bigr)
<
\lVert x-x'\rVert.
$$

Le point $z$ fournit alors un chemin de deux arêtes strictement plus courtes entre $x$ et $x'$.

Le critère de Gabriel est plus faible : il impose seulement la vacuité de la boule diamétrale de $[x,x']$.

### 1.2 Cas HGP : $K\geq 2$

Dans HGP-Clusterer :

- les sommets du graphe auxiliaire $\Gamma_K(\mathcal X,r)$ sont les $(K-1)$-simplexes du complexe de Čech, donc les sous-ensembles de cardinal $K$ ;
- deux facettes $\tau$ et $\tau'$ sont adjacentes lorsqu’elles diffèrent d’un point et que leur union est un $K$-simplexe de Čech ;
- le poids de cette adjacence est le rayon de naissance

$$
w(\tau,\tau')
=
\rho(\tau\cup\tau').
$$

Les composantes connexes de $\Gamma_K(\mathcal X,r)$ correspondent exactement aux $K$-polyèdres, eux-mêmes en correspondance avec les amas discrets de forte densité de l’estimateur $K$-NN.

Le chapitre 8 introduit ensuite :

- les facettes actives d’un $K$-simplexe $\sigma$ ;
- les $K$-simplexes séparants, qui provoquent effectivement une fusion ;
- les $K$-simplexes de Gabriel, dont la miniball ne contient aucun point extérieur dans son intérieur.

Le Théorème 4 du manuscrit établit

$$
\operatorname{Sep}_K(\mathcal X)
\subseteq
\operatorname{Gab}_K(\mathcal X).
$$

La preuve contient déjà le mécanisme du RNG : un point intrus dans la miniball permet de contourner le simplexe par des simplexes de plus petit rayon.

---

## 2. Pourquoi le critère de Gabriel n’est pas optimal

Le critère de Gabriel ne détecte qu’un type particulier de contournement : celui produit par un point

$$
z\in\mathring B_\sigma\setminus\sigma.
$$

Or un point situé **hors** de la miniball $B_\sigma$ peut lui aussi fournir des simplexes de remplacement de rayon strictement inférieur à $\rho(\sigma)$.

Le critère pertinent n’est donc pas seulement :

> « La miniball de $\sigma$ contient-elle un point intrus ? »

mais plutôt :

> « Les facettes actives de $\sigma$ sont-elles déjà reliées, avant $\rho(\sigma)$, par des chemins de simplexes plus légers ? »

C’est exactement l’idée du *Relative Neighborhood Graph* classique, transposée au graphe pondéré des facettes.

---

## 3. Généralisation canonique du RNG au graphe des facettes

### 3.1 Graphe pondéré statique

Considérons le graphe pondéré élémentaire

$$
\Gamma_K(\mathcal X)=(V_K,E_K,w),
$$

avec

$$
V_K=\{\tau\subseteq\mathcal X:|\tau|=K\},
$$

et

$$
\{\tau,\tau'\}\in E_K
\quad\Longleftrightarrow\quad
|\tau\cap\tau'|=K-1.
$$

Le poids est

$$
w(\tau,\tau')=\rho(\tau\cup\tau').
$$

Chaque sommet $\tau$ possède par ailleurs son propre rayon de naissance $\rho(\tau)$. Par convention, $\Gamma_K(\mathcal X)_{\leq r}$ contient les sommets tels que $\rho(\tau)\leq r$ et les arêtes telles que $w(\tau,\tau')\leq r$.

### 3.2 Réduction de voisinage relatif des facettes

On conserve une arête $e=\{\tau,\tau'\}$ si elle ne possède aucun contournement en deux pas strictement plus léger :

$$
\boxed{
\{\tau,\tau'\}\in E_K^{\mathrm{RN}}
\iff
\nexists\eta\in V_K:
\begin{cases}
\{\tau,\eta\}\in E_K,\\
\{\eta,\tau'\}\in E_K,\\
\max\bigl(w(\tau,\eta),w(\eta,\tau')\bigr)<w(\tau,\tau').
\end{cases}
}
$$

On note le graphe obtenu

$$
\operatorname{RNG}^{\mathrm{fac}}_K(\mathcal X).
$$

Pour $K=1$, les sommets sont les points de $\mathcal X$, le graphe sous-jacent est complet et

$$
w(\{x\},\{x'\})=\frac12\lVert x-x'\rVert.
$$

La définition redonne donc exactement le RNG classique.

### 3.3 Préservation exacte des composantes

**Proposition.** Pour tout $r\geq 0$,

$$
\boxed{
\pi_0\!\left(\operatorname{RNG}^{\mathrm{fac}}_K(\mathcal X)_{\leq r}\right)
=
\pi_0\!\left(\Gamma_K(\mathcal X)_{\leq r}\right).
}
$$

#### Preuve

Soit une arête $e=\{\tau,\tau'\}$ supprimée, de poids $a$. Par définition, il existe une facette $\eta$ telle que les deux arêtes

$$
\{\tau,\eta\},\qquad \{\eta,\tau'\}
$$

ont des poids strictement inférieurs à $a$.

Si ces deux arêtes sont conservées, elles remplacent directement $e$. Si l’une d’elles est elle-même supprimée, elle possède à son tour un contournement constitué d’arêtes de poids strictement inférieur.

En raisonnant par induction sur les niveaux de poids distincts, toute arête supprimée est remplaçable par un chemin d’arêtes conservées de poids strictement inférieur. Par conséquent, toute connexion présente au niveau $r$ dans $\Gamma_K$ est encore présente dans le graphe réduit.

L’inclusion réciproque est immédiate puisque le graphe réduit est un sous-graphe de $\Gamma_K$.

### 3.4 Relation avec une forêt minimum couvrante

Toute arête supprimée est strictement la plus lourde d’un cycle de longueur trois. Elle ne peut donc appartenir à aucune forêt minimum couvrante. Ainsi,

$$
\mathrm{MSF}(\Gamma_K)
\subseteq
\operatorname{RNG}^{\mathrm{fac}}_K(\mathcal X)
\subseteq
\Gamma_K(\mathcal X).
$$

Cette construction est mathématiquement exacte, mais ne résout pas encore le problème algorithmique : construire naïvement $\Gamma_K$ reste combinatoire.

---

## 4. Critère géométrique RNG-HGP au niveau des simplexes

La réduction précédente porte sur les arêtes du graphe des facettes. Pour obtenir un critère géométrique directement applicable à un $K$-simplexe candidat, on exploite ses facettes actives.

### 4.1 Ensemble de support et facettes actives

Soit $\sigma\subseteq\mathcal X$ un $K$-simplexe, donc $|\sigma|=K+1$, et posons

$$
r=\rho(\sigma),
\qquad
B_\sigma=\overline B(c_\sigma,r).
$$

Sous l’hypothèse de position générale du manuscrit, l’ensemble de support est

$$
S_\sigma=\sigma\cap\partial B_\sigma,
$$

avec

$$
2\leq |S_\sigma|\leq p+1.
$$

Pour $s\in S_\sigma$, la facette

$$
\tau_s=\sigma\setminus\{s\}
$$

est active puisque

$$
\rho(\tau_s)<\rho(\sigma).
$$

Les facettes obtenues en retirant un point intérieur à $B_\sigma$ gardent le même rayon et ne sont donc pas actives.

### 4.2 Simplexes de remplacement

Pour $z\in\mathcal X\setminus\sigma$ et $s\in S_\sigma$, définissons

$$
\sigma_s^z
=
(\sigma\setminus\{s\})\cup\{z\}.
$$

Pour deux points distincts $s,t\in S_\sigma$, définissons la **lune de remplacement de Čech**

$$
\boxed{
\Lambda_{s,t}^{\check C}(\sigma)
=
\left\{
 z\in\mathcal X\setminus\sigma:
 \rho(\sigma_s^z)<r
 \text{ et }
 \rho(\sigma_t^z)<r
\right\}.
}
$$

Un point $z\in\Lambda_{s,t}^{\check C}(\sigma)$ fournit le chemin

$$
\tau_s
\longleftrightarrow
(\sigma\setminus\{s,t\})\cup\{z\}
\longleftrightarrow
\tau_t
$$

dans $\Gamma_K(\mathcal X)_{<r}$.

### 4.3 Graphe de contournement

On construit un graphe

$$
\mathcal B_\sigma
$$

sur les sommets $S_\sigma$, avec

$$
\{s,t\}\in E(\mathcal B_\sigma)
\quad\Longleftrightarrow\quad
\Lambda_{s,t}^{\check C}(\sigma)\neq\varnothing.
$$

Chaque arête de $\mathcal B_\sigma$ certifie que les deux facettes actives correspondantes sont déjà reliées avant la naissance de $\sigma$.

### 4.4 Définition proposée

On appelle $\sigma$ un **$K$-simplexe de voisinage relatif HGP** lorsque son graphe de contournement n’est pas connexe :

$$
\boxed{
\sigma\in\operatorname{RNG}^{\mathrm{HGP}}_K(\mathcal X)
\iff
\mathcal B_\sigma\text{ est non connexe}.
}
$$

Cette définition est plus forte que la simple absence d’un témoin commun à toutes les facettes. Des témoins différents peuvent connecter successivement les facettes actives ; dès que leur graphe de contournement est connexe, le simplexe ne peut plus provoquer de fusion.

### 4.5 Retour au RNG classique

Pour $K=1$, un $1$-simplexe est une arête $\sigma=\{x,x'\}$. Son support est $S_\sigma=\{x,x'\}$ et

$$
\rho(\{x,z\})<\rho(\{x,x'\})
\quad\Longleftrightarrow\quad
\lVert x-z\rVert<\lVert x-x'\rVert.
$$

Ainsi,

$$
\Lambda_{x,x'}^{\check C}(\sigma)\neq\varnothing
$$

si et seulement s’il existe $z$ dans la lune classique

$$
B^\circ(x,\lVert x-x'\rVert)
\cap
B^\circ(x',\lVert x-x'\rVert).
$$

La définition redonne exactement le RNG usuel.

---

## 5. Résultat principal

### Théorème

Sous l’hypothèse de position générale pour la filtration de Čech,

$$
\boxed{
\operatorname{Sep}_K(\mathcal X)
\subseteq
\operatorname{RNG}^{\mathrm{HGP}}_K(\mathcal X)
\subseteq
\operatorname{Gab}_K(\mathcal X).
}
$$

### 5.1 Première inclusion

Supposons que $\mathcal B_\sigma$ soit connexe. Pour deux facettes actives quelconques $\tau_s$ et $\tau_t$, choisissons un chemin

$$
s=s_0,s_1,\ldots,s_m=t
$$

dans $\mathcal B_\sigma$.

Chaque arête $\{s_i,s_{i+1}\}$ fournit un chemin dans $\Gamma_K(\mathcal X)_{<r}$ entre $\tau_{s_i}$ et $\tau_{s_{i+1}}$. En concaténant ces chemins, toutes les facettes actives de $\sigma$ appartiennent déjà à la même composante avant $r$.

Le simplexe $\sigma$ n’est donc pas $K$-séparant. Par contraposée,

$$
\operatorname{Sep}_K(\mathcal X)
\subseteq
\operatorname{RNG}^{\mathrm{HGP}}_K(\mathcal X).
$$

### 5.2 Seconde inclusion

Supposons que $\sigma$ ne soit pas de Gabriel. Il existe alors

$$
z\in\mathring B_\sigma\cap(\mathcal X\setminus\sigma).
$$

La preuve du Théorème 4 du manuscrit montre que, pour tout $s\in S_\sigma$,

$$
\rho(\sigma_s^z)<\rho(\sigma).
$$

Le même point $z$ appartient donc à toutes les lunes

$$
\Lambda_{s,t}^{\check C}(\sigma),
\qquad s\neq t.
$$

Le graphe $\mathcal B_\sigma$ est complet, donc connexe. Par contraposée,

$$
\operatorname{RNG}^{\mathrm{HGP}}_K(\mathcal X)
\subseteq
\operatorname{Gab}_K(\mathcal X).
$$

---

## 6. Inclusion stricte dans les simplexes de Gabriel

Considérons dans $\mathbb R^2$

$$
A=(-1,0),
\qquad
B=(1,0),
\qquad
C=(0,0.1),
$$

et le triangle

$$
\sigma=\{A,B,C\}.
$$

Sa miniball est le disque unité centré en l’origine, soutenu par $A$ et $B$ :

$$
\rho(\sigma)=1,
\qquad
S_\sigma=\{A,B\}.
$$

Ajoutons

$$
z=(0,1.1).
$$

Le point $z$ est extérieur à la miniball de $\sigma$, donc $\sigma$ est de Gabriel dans

$$
\mathcal X=\{A,B,C,z\}.
$$

En revanche, les triangles

$$
\{B,C,z\}
\qquad\text{et}\qquad
\{A,C,z\}
$$

sont obtus et ont pour rayon de miniball

$$
\frac{\sqrt{2.21}}{2}
\approx 0.743
<1.
$$

Le point $z$ relie donc les deux facettes actives $\{A,C\}$ et $\{B,C\}$ strictement avant la naissance de $\sigma$.

Ainsi,

$$
\sigma\in\operatorname{Gab}_2(\mathcal X),
\qquad
\sigma\notin\operatorname{RNG}^{\mathrm{HGP}}_2(\mathcal X).
$$

Par conséquent,

$$
\boxed{
\operatorname{RNG}^{\mathrm{HGP}}_K(\mathcal X)
\subsetneq
\operatorname{Gab}_K(\mathcal X)
}
$$

en général.

---

## 7. Ce que le critère améliore réellement

### 7.1 Ce qui est amélioré

Le critère RNG-HGP est :

- strictement plus sélectif que Gabriel ;
- plus proche de la notion exacte de $K$-simplexe séparant ;
- local, puisqu’il ne demande que des tests de miniball et des recherches de témoins ;
- naturellement parallélisable sur GPU ;
- indépendant de la construction explicite de la mosaïque de Delaunay d’ordre supérieur.

### 7.2 Ce qui n’est pas amélioré

Si l’algorithme reste exact, il ne produit pas une nouvelle hiérarchie statistique :

$$
\theta_K^{\mathrm{HGP}}
$$

reste la hiérarchie exacte des amas discrets de forte densité $K$-NN.

Le bénéfice attendu concerne :

- le nombre de simplexes candidats ;
- le nombre d’arêtes du graphe dual ;
- le volume mémoire ;
- le coût du tri et des unions ;
- la possibilité de traiter les événements par lots sur GPU.

### 7.3 Critère local versus critère exact

Le meilleur critère possible est la définition même d’un simplexe $K$-séparant : tester si ses facettes actives appartiennent à des composantes distinctes de $\Gamma_K(\mathcal X)_{<r}$.

Ce test est exact, mais global et dynamique. Le critère RNG-HGP est un préfiltre local qui détecte les contournements de longueur deux, éventuellement concaténés entre les facettes actives.

On obtient donc la hiérarchie de tests suivante :

$$
\text{Gabriel}
\quad\Longrightarrow\quad
\text{RNG-HGP}
\quad\Longrightarrow\quad
\text{test exact de séparation par Union-Find/Borůvka}.
$$

---

## 8. Génération sans mosaïque de Delaunay d’ordre supérieur

Le fait géométrique décisif est que la miniball d’un ensemble fini de $\mathbb R^p$ est soutenue par au plus $p+1$ points.

### 8.1 Caractérisation par les supports

Soit

$$
S\subseteq\mathcal X,
\qquad
2\leq |S|\leq p+1,
$$

et notons

$$
B_S=\overline B(c_S,r_S)
$$

sa miniball. Supposons que $S$ soit **exactement l’ensemble de support** de $B_S$ : les points de $S$ sont sur $\partial B_S$, ils sont affinement indépendants et $c_S$ appartient à l’intérieur relatif de $\operatorname{conv}(S)$.

**Proposition.** Sous l’hypothèse de position générale, les deux assertions suivantes sont équivalentes :

1. $|\mathcal X\cap B_S|=K+1$ ;
2. $\sigma_S=\mathcal X\cap B_S$ est un $K$-simplexe de Gabriel dont l’ensemble de support est $S$.

#### Preuve

Si $|\mathcal X\cap B_S|=K+1$, posons

$$
\sigma_S=\mathcal X\cap B_S.
$$

Toute boule contenant $\sigma_S$ contient en particulier $S$, donc son rayon est au moins $r_S$. Comme $B_S$ contient $\sigma_S$, il s’agit de la miniball de $\sigma_S$.

Aucun point extérieur à $\sigma_S$ n’appartient à l’intérieur de $B_S$, puisque tous les points de $\mathcal X\cap B_S$ ont été inclus dans $\sigma_S$. Le simplexe est donc de Gabriel.

Réciproquement, si $\sigma$ est un $K$-simplexe de Gabriel, sa miniball contient ses $K+1$ sommets et aucun point extérieur dans son intérieur. La position générale exclut aussi les points extérieurs sur la frontière. Ainsi,

$$
\mathcal X\cap B_\sigma=\sigma,
$$

et l’ensemble de support $S_\sigma$ possède un cardinal au plus $p+1$.

### 8.2 Conséquence en dimension 3

Pour $p=3$, tout candidat de Gabriel est déterminé par un support de taille :

- 2 points : boule diamétrale ;
- 3 points : cercle circonscrit dans le plan affine du triangle, avec centre dans son intérieur relatif ;
- 4 points : sphère circonscrite, avec centre dans l’intérieur du tétraèdre.

Pour $K=10$, il n’est donc pas nécessaire d’énumérer directement des sous-ensembles de cardinal 11. On peut :

1. générer une petite configuration support de cardinal au plus 4 ;
2. calculer sa miniball ;
3. compter les points contenus dans cette boule ;
4. conserver la boule uniquement si elle contient exactement 11 points.

> [!WARNING]
> Cette réduction de la taille des supports ne rend pas magiquement l’énumération exhaustive bon marché. Énumérer tous les couples, triplets et quadruplets parmi $n$ points reste prohibitif. Il faut impérativement une génération locale, par rayon maximal, index spatial ou certificat adaptatif.

---

## 9. Pipeline GPU proposé en dimension 3

### 9.1 Vue générale

```text
Nuage 3D
  -> index spatial GPU
  -> génération locale de supports de taille 2, 3 ou 4
  -> calcul parallèle des miniballs
  -> comptage borné des points dans chaque boule
  -> candidats de Gabriel
  -> test local RNG-HGP
  -> tri par rayon
  -> test exact des fusions par Union-Find/Borůvka
  -> K-arbre couvrant / dendrogramme
```

### 9.2 Index spatial

Les structures les plus adaptées sont :

- une grille hachée multi-résolution ;
- un octree linéaire ;
- un LBVH ;
- un tri par code de Morton suivi de recherches par intervalles de cellules.

Le tri de Morton est particulièrement utile pour :

- regrouper spatialement les points ;
- répartir les requêtes par blocs ;
- limiter les accès mémoire irréguliers ;
- traiter des nuages dépassant la mémoire GPU par tuiles avec halo.

### 9.3 Génération locale des supports

Pour une hiérarchie exacte jusqu’à un rayon maximal $R_{\max}$, tout support pertinent possède un diamètre au plus $2R_{\max}$.

On peut donc :

1. construire le graphe de voisinage de rayon $2R_{\max}$ ;
2. énumérer localement les supports de taille 2, 3 et 4 ;
3. calculer chaque miniball indépendamment.

Cette étape est massivement parallèle. Son coût dépend cependant du nombre d’occupants par cellule et du nombre de petites cliques locales.

### 9.4 Comptage dans les miniballs

Pour chaque boule candidate $B_S$ :

1. interroger l’index spatial ;
2. arrêter le comptage dès que $K+2$ points sont trouvés ;
3. rejeter si le nombre de points est différent de $K+1$ ;
4. retourner les $K+1$ indices lorsque le comptage est exact.

Pour $K=10$, le compteur peut s’arrêter à 12, ce qui limite fortement le travail dans les régions trop denses.

### 9.5 Test RNG-HGP

En dimension 3,

$$
|S_\sigma|\leq 4,
$$

et le graphe de contournement possède donc au plus

$$
\binom 42=6
$$

paires à tester.

Pour une paire $s,t\in S_\sigma$, un témoin $z$ doit satisfaire nécessairement

$$
\lVert z-x\rVert<2r
\qquad
\text{pour tout }x\in\sigma.
$$

On peut donc :

1. effectuer une requête de rayon $2r$ autour d’un sommet d’ancrage ;
2. filtrer les points par les autres contraintes de distance ;
3. calculer les deux miniballs de remplacement ;
4. arrêter la recherche dès qu’un témoin est trouvé ;
5. arrêter tout le traitement dès que $\mathcal B_\sigma$ devient connexe.

Le graphe $\mathcal B_\sigma$ tient dans quelques bits et sa connexité se teste sans structure dynamique générale.

### 9.6 Pseudocode

```text
for each local support candidate S in parallel:
    (c, r) <- miniball(S)

    if S is not the exact support of its miniball:
        continue

    sigma <- range_query(X, center=c, radius=r, stop_after=K+2)

    if |sigma| != K+1:
        continue

    Bypass <- graph with vertex set S

    for each unordered pair {s, t} subset S:
        for z in witness_candidates(sigma, r):
            if z in sigma:
                continue

            if rho((sigma \ {s}) union {z}) < r
               and rho((sigma \ {t}) union {z}) < r:
                add edge {s, t} to Bypass
                break

        if Bypass is connected:
            break

    if Bypass is disconnected:
        emit RNG-HGP candidate (sigma, S, r)
```

### 9.7 Traitement des événements

Les événements conservés sont triés par rayon croissant, puis traités par lots de rayons égaux.

Deux possibilités :

- **Union-Find par lots**, avec lecture des composantes avant les unions du lot ;
- **Borůvka parallèle**, où chaque composante sélectionne ses événements sortants minimaux.

Le test dynamique final permet de ne conserver que les événements qui fusionnent réellement des composantes.

### 9.8 Représentation mémoire

Il est déconseillé de matérialiser toutes les cliques de facettes d’un $K$-simplexe.

Pour un simplexe $\sigma$, il suffit de stocker :

- ses $K+1$ indices de points ;
- son rayon ;
- son support $S_\sigma$ de cardinal au plus 4 en dimension 3 ;
- quelques arêtes entre facettes actives ou, mieux, les unions minimales retenues par le test dynamique.

Pour $K=10$, matérialiser la clique complète des 11 facettes demanderait jusqu’à 55 arêtes. Le support actif n’en comporte au plus que 4, et une fusion de ces facettes exige au plus 3 unions.

---

## 10. Exactitude, approximations et limites

### 10.1 Version exacte jusqu’à un rayon $R_{\max}$

La construction est exacte jusqu’à $R_{\max}$ si :

1. tous les supports de rayon au plus $R_{\max}$ sont énumérés ;
2. les comptages dans les boules sont exacts ;
3. les recherches de témoins sont exhaustives ;
4. les comparaisons de rayons sont robustes ;
5. les facettes et les événements de même rayon sont traités sans dépendre de l’ordre d’exécution.

### 10.2 Version exacte sur toute la hiérarchie

On peut augmenter progressivement $R_{\max}$ jusqu’à obtenir la connexion globale. Cela évite de construire explicitement la mosaïque de Delaunay d’ordre supérieur, mais ne supprime pas le pire cas combinatoire.

La mosaïque d’ordre $K$ organise les événements critiques du champ $K$-NN. Une méthode exacte peut éviter de la **matérialiser**, mais elle doit tout de même découvrir une quantité suffisante de ces événements.

### 10.3 Listes $k$-NN

Une liste de voisins de taille fixe constitue un bon générateur heuristique de supports, mais pas une garantie d’exactitude générale.

Un support géométriquement pertinent peut avoir un rang de voisinage arbitrairement élevé à cause de points situés hors de sa miniball mais proches de l’un de ses sommets.

Une version certifiée doit agrandir adaptativement la région explorée jusqu’à ce que les bornes de distance aux cellules non visitées excluent tout support ou témoin manquant.

### 10.4 Préfiltre RNG-HGP et filtration complète

Le résultat

$$
\operatorname{Sep}_K
\subseteq
\operatorname{RNG}^{\mathrm{HGP}}_K
$$

garantit que le préfiltre ne supprime aucun événement de fusion.

Il ne suffit cependant pas, à lui seul, à définir sans ambiguïté la filtration complète des facettes : une facette peut naître au même rayon qu’un simplexe sans provoquer immédiatement de fusion.

Deux stratégies sûres sont possibles :

1. conserver explicitement les naissances et rattachements de facettes, puis utiliser RNG-HGP uniquement pour filtrer les fusions ;
2. appliquer la réduction exacte $\operatorname{RNG}^{\mathrm{fac}}_K$ à un graphe de facettes déjà généré, éventuellement après le filtrage de Gabriel.

### 10.5 Masses et sortie en partition stricte

Le chapitre 9 associe aux facettes des scores dépendant de tous les cofaces considérés, par exemple

$$
S_\tau
=
\sum_{\sigma\supset\tau}
\psi(\rho(\sigma)).
$$

Supprimer un simplexe redondant pour la connexité ne signifie pas que sa contribution statistique doit être supprimée.

Il faut distinguer :

- le graphe utilisé pour les fusions ;
- les contributions utilisées pour les masses, l’excès de masse et le vote pondéré.

Une implémentation peut accumuler la contribution d’un simplexe, puis décider de ne pas matérialiser ses arêtes.

### 10.6 Robustesse numérique

Les tests reposent sur des inégalités strictes. Sur des données LiDAR quantifiées, les égalités et quasi-cosphéricités seront fréquentes.

Une implémentation robuste doit :

- traiter ensemble les événements de même rayon ;
- calculer les miniballs au moins en double précision ;
- conserver les candidats proches de l’égalité plutôt que de les supprimer ;
- utiliser un prédicat robuste ou une perturbation symbolique en repli ;
- privilégier les faux positifs, qui coûtent du temps, aux faux négatifs, qui détruisent l’exactitude.

---

## 11. Comparaison des trois niveaux de filtrage

| Critère | Nature | Garantie | Sélectivité | Coût local | Besoin de connexité globale |
|---|---|---:|---:|---:|---:|
| Gabriel | Vacuité de la miniball | Nécessaire pour être séparant | Faible | Faible | Non |
| RNG-HGP | Absence de réseau local de contournement | Nécessaire pour être séparant | Strictement meilleure | Moyen | Non |
| $K$-séparant | Facettes actives dans des composantes distinctes | Exact | Maximale | Élevé | Oui |

Une architecture naturelle est donc :

$$
\boxed{
\text{support local}
\rightarrow
\text{Gabriel}
\rightarrow
\text{RNG-HGP}
\rightarrow
\text{test exact de séparation}
}
$$

---

## 12. Recommandation

La généralisation du RNG est mathématiquement plus pertinente que le seul critère de Gabriel pour identifier les simplexes utiles à la hiérarchie.

Le bon objet conceptuel est le **RNG du graphe pondéré des facettes**. Le bon critère géométrique local est le **graphe de contournement des facettes actives**.

La chaîne recommandée est :

$$
\boxed{
\operatorname{Sep}_K
\subseteq
\operatorname{RNG}^{\mathrm{HGP}}_K
\subsetneq
\operatorname{Gab}_K.
}
$$

Pour une implémentation GPU en petite dimension, notamment $p=3$ et $K=10$, la direction la plus crédible est :

1. ne pas construire la mosaïque de Delaunay d’ordre supérieur ;
2. générer localement les miniballs à partir de supports de taille au plus 4 ;
3. récupérer les candidats de Gabriel par comptage exact de $K+1$ points ;
4. éliminer les candidats contournables par le critère RNG-HGP ;
5. traiter les événements restants par Union-Find ou Borůvka en lots ;
6. conserver séparément les contributions statistiques des simplexes supprimés du graphe de fusion.

Cette approche remplace une structure globale, irrégulière et difficile à paralléliser par une suite de kernels locaux :

- génération de petits supports ;
- calcul de miniballs ;
- requêtes spatiales ;
- tests de remplacement ;
- tri ;
- unions parallèles.

Elle est donc mieux alignée avec une architecture GPU. Elle ne garantit toutefois un passage à l’échelle massif que si le nombre de supports locaux reste contrôlé ou si l’on fixe un rayon maximal avec un mécanisme adaptatif de certification.

---

## 13. Travaux à mener

### 13.1 Complément théorique prioritaire

Formaliser un théorème complet de remplacement du $K$-graphe de Gabriel par un graphe RNG-HGP qui traite explicitement :

- la naissance des facettes ;
- les événements de même rayon ;
- les composantes réduites à une facette ;
- le passage des composantes de facettes aux ensembles de points des $K$-polyèdres.

### 13.2 Analyse de complexité probabiliste

Sur un processus ponctuel homogène ou une densité bornée en dimension 3, estimer :

- le nombre moyen de supports candidats par point ;
- le nombre de boules contenant exactement $K+1$ points ;
- le rapport

$$
\frac{|\operatorname{RNG}^{\mathrm{HGP}}_K|}
{|\operatorname{Gab}_K|};
$$

- la distribution du nombre de témoins testés avant arrêt.

### 13.3 Prototype expérimental

Comparer sur des nuages synthétiques et LiDAR :

1. nombre de candidats Gabriel ;
2. nombre de candidats RNG-HGP ;
3. nombre de vrais simplexes séparants ;
4. mémoire maximale ;
5. temps des requêtes spatiales ;
6. temps du tri et de l’Union-Find ;
7. fidélité exacte de la hiérarchie ;
8. comportement sur SemanticKITTI.

### 13.4 Variantes approximatives

Étudier une version rapide où :

- les supports sont générés depuis un graphe $m$-NN ;
- les témoins sont cherchés dans une liste locale bornée ;
- un mode certifié réexamine uniquement les candidats proches de la décision.

---

## 14. Références internes au manuscrit

Cette note s’appuie principalement sur les éléments suivants :

- **Chapitre 2** : définition du RNG, du graphe de Gabriel et chaîne
  $\mathrm{MST}\subseteq\mathrm{RNG}\subseteq\mathrm{Gabriel}\subseteq\mathrm{Delaunay}$ ;
- **Chapitre 6** : graphe $\Gamma_K$, $K$-polyèdres et correspondance exacte avec les amas discrets de forte densité $K$-NN ;
- **Chapitre 8** : rayon de naissance $\rho$, miniball $B_\sigma$, ensemble de support, facettes actives, simplexes $K$-séparants, simplexes de Gabriel et Théorème 4 ;
- **Chapitre 8** : $K$-graphe de Gabriel, $K$-arbre minimum couvrant et préservation des $K$-polyèdres non triviaux ;
- **Chapitre 9** : construction pratique, masses portées par les facettes et limites de la mosaïque de Delaunay d’ordre supérieur.
