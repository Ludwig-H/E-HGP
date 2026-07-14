# Définition de la hiérarchie HGP en dimension trois

> **Statut.** La correspondance entre multicovertures et composantes de Gamma reste la définition normative. La proposition 6 et le théorème 5 du manuscrit sont contredits, dans leur formulation élaguée, par la fixture exacte `gabriel-point-set-counterexample-5-points-v1`; ils ne fondent plus la voie exacte v2.

## 1. Des multicovertures aux K-polyèdres

Soit $X\subset\mathbb{R}^{3}$ fini non vide, $n=\lvert X\rvert$ et $K_{\mathrm{eff}}=\min(K_{\max},n)$. Pour $a\geq0$, la multicoverture d'ordre $k$ est

$$L_k(a)=\left\lbrace y\in\mathbb{R}^{3}:\#\left(X\cap B(y,\sqrt{a})\right)\geq k\right\rbrace.$$

Pour tout ensemble fini non vide $A\subseteq X$, son niveau de naissance de Čech, exprimé en rayon carré, est

$$\beta(A)=\min_y\max_{x\in A}\left\Vert y-x\right\Vert^2.$$

Le graphe de facettes $\Gamma_k(a)$ possède :

- un sommet par $F\subseteq X$, $\lvert F\rvert=k$, avec $\beta(F)\leq a$;
- une arête entre $F$ et $F'$ lorsque $\lvert F\cup F'\rvert=k+1$ et $\beta(F\cup F')\leq a$.

Cette définition vaut jusqu'à $k=n$. Dans ce cas, $\Gamma_n(a)$ est vide avant $\beta(X)$, puis contient l'unique sommet $X$ et aucune arête. Cette composante terminale appartient à `full_pi0`; pour $n\geq2$, elle reste isolée et n'appartient pas au profil réduit défini ci-dessous.

Le théorème 2 du manuscrit identifie naturellement les composantes de $L_k(a)$ aux K-polyèdres, c'est-à-dire aux composantes de ce graphe vues comme unions des observations de leurs facettes. La proposition 5 garantit que les cofaces de cardinal supérieur à $k+1$ sont inutiles à la connexité.

Cette correspondance est exacte niveau par niveau. Elle explique deux propriétés parfois perdues dans les algorithmes de clustering usuels :

1. un K-polyèdre est une composante géométrique de haute densité, pas un simple groupe de points défini par voisinage direct;
2. pour $k\geq2$, deux K-polyèdres distincts peuvent partager des observations.

## 2. Hiérarchie horizontale

Lorsque $a$ croît, les sommets et arêtes de $\Gamma_k(a)$ ne font que s'ajouter. Ses composantes naissent puis fusionnent. La forêt de fusion $T_k$ est la compression finie de ce foncteur de composantes.

Un nœud de $T_k$ représente soit une naissance, soit une multifusion. Son niveau exact est un carré de rayon. Une arête de la forêt relie un composant antérieur au premier événement qui l'absorbe. Les composantes qui ne fusionnent jamais sont reliées à une racine à niveau infini uniquement dans les formats qui l'exigent; l'infini n'est pas un événement géométrique.

La forêt ne doit pas être artificiellement binaire. Si plusieurs composantes se rencontrent au même niveau exact, elles ont un parent commun dans un seul lot.

## 3. Hiérarchie verticale

Pour tout niveau,

$$L_{k+1}(a)\subseteq L_k(a).$$

Chaque composante de $L_{k+1}(a)$ est donc contenue dans une composante unique de $L_k(a)$. On note cette application

$$v_{k,a}:\pi_0\bigl(L_{k+1}(a)\bigr)\longrightarrow\pi_0\bigl(L_k(a)\bigr).$$

Si $a\leq b$, les applications horizontales $h_{k,a,b}$ et verticales doivent satisfaire

$$h_{k,a,b}\circ v_{k,a}=v_{k,b}\circ h_{k+1,a,b}.$$

Cette naturalité est une partie de la sortie. Dix forêts calculées indépendamment ne constituent pas une hiérarchie HGP complète si ces flèches manquent ou ne commutent pas.

## 4. Simplexes séparants

Un ensemble $S\subseteq X$ de cardinal $k+1$ apparaît au niveau $\beta(S)$. Ses facettes actives strictes sont

$$\mathcal{F}^{-}(S)=\left\lbrace S\setminus\lbrace x\rbrace:\beta\bigl(S\setminus\lbrace x\rbrace\bigr)<\beta(S)\right\rbrace.$$

Le simplexe $S$ est $k$-séparant si au moins deux éléments de $\mathcal{F}^{-}(S)$ appartiennent à des composantes distinctes de $\Gamma_k$ juste avant $\beta(S)$. Il provoque alors une fusion réelle. Cette définition est globale : la géométrie de $S$ seule ne décide pas si les bras étaient déjà reliés ailleurs.

Si $S=I\cup U$ provient d'une sphère critique de rang $k+1$, les bras locaux stricts sont précisément

$$F_u=S\setminus\lbrace u\rbrace,\qquad u\in U.$$

La multiplicité locale d'indice un de Reani–Bobrowski vaut $\Delta_1=\binom{\lvert U\rvert-1}{1}=\lvert U\rvert-1$. Si ces bras rencontrent $q$ composantes globales distinctes avant le lot, le centre en tue $q-1$, avec $q-1\leq\lvert U\rvert-1$; il peut aussi être globalement redondant. Ainsi une sphère avec $\lvert U\rvert=3$ peut produire une triple fusion. Le modèle correct est une hyperarête multifurquée, jamais une suite arbitraire de selles binaires.

## 5. Sous-graphe de Gabriel et obstruction à la réduction brute

Un $k$-simplexe $S$ est de Gabriel lorsque l'intérieur de sa miniball ne contient aucun point extérieur :

$$B^{\circ}\bigl(c_S,\sqrt{\beta(S)}\bigr)\cap(X\setminus S)=\varnothing.$$

Sous la position générale du manuscrit, tout simplexe $k$-séparant est de Gabriel. Le K-graphe de Gabriel relie en clique les facettes de chaque tel $S$, au poids $\beta(S)$.

Pour la seule connexité, la clique peut être remplacée par l'hyperarête

$$e_S=\left\lbrace S\setminus\lbrace x\rbrace:x\in S\right\rbrace$$

ou par une étoile ayant un pivot canonique. Une clique, une hyperarête et une étoile ont les mêmes composantes à tous les seuils, à condition que leurs éléments soient insérés dans le même lot.

Chaque hyperarête de Gabriel est une coface de Gamma et ne peut donc inventer une connexion. En revanche, la fixture exacte `gabriel-point-set-counterexample-5-points-v1` montre que le sous-graphe élagué peut manquer une incidence silencieuse créée par une coface non-Gabriel, puis retarder une fusion future. L'égalité des collections d'unions de points de la proposition 6 et la conclusion élaguée du théorème 5 sont donc `false_in_general` sous cette définition. Le flot brut certifie seulement une connectivité positive partielle.

## 6. Hyper-Kruskal par lots

Pour un ordre fixé, la voie exacte de référence trie par niveau toutes les naissances de facettes et toutes les cofaces de Gamma. À l'ordre un, le DSU possède dès le niveau zéro les $n$ racines singleton. Aux ordres $k\geq2$, une facette isolée ne crée aucune racine publique dans `hgp_reduced`, mais elle reste active avec toutes ses incidences internes; une racine réduite apparaît seulement lorsqu'une composante devient non triviale. À un niveau $a$, soit $R^{-}$ l'ensemble des racines réduites publiques de niveau strictement inférieur. On construit un hypergraphe temporaire dont les sommets sont :

- les racines de $R^{-}$ touchées par le lot;
- les facettes Gamma nouvellement vues et qui n'ont pas encore de racine publique, quelle que soit leur valeur propre $\beta(F)$;

et dont les hyperarêtes sont toutes les cofaces Gamma de niveau $a$.

Pour chaque composante temporaire qui contient au moins une hyperarête, notons $q$ le nombre de racines distinctes de niveau strictement inférieur :

- si $q=0$, créer un nœud de naissance `hgp_reduced` au niveau $a$;
- si $q=1$, prolonger cette racine sans nœud topologique nouveau;
- si $q\geq2$, créer un nœud de multifusion des $q$ racines.

Dans les trois cas, toutes les facettes nouvelles sont activées et rattachées à la racine obtenue. Les facettes simultanées ne deviennent pas des branches de persistance positive.

Un cas $q=1$ peut ajouter des facettes et des observations au K-polyèdre sans changer sa composante. La forêt seule ne suffit donc pas à restituer les coupes : le `coverage_log` obligatoire conserve, pour chaque lot, un `coverage_delta` de facettes et d'identifiants nouvellement couverts. Une hyperarête n'est pleinement redondante et supprimable que si toutes ses facettes sont déjà actives dans la même racine et si son delta de couverture est vide.

Après résolution complète du lot seulement, les unions DSU sont appliquées. Le pivot d'une étoile peut être choisi par le plus petit label lexicographique; il n'affecte pas la forêt sémantique.

Cette construction exhaustive porte `proof_basis=gamma_exhaustive_reference` et ne peut publier `exact` que sur `reference_cpu`. Remplacer les cofaces Gamma par les seules hyperarêtes Gabriel donne le même mécanisme algorithmique mais seulement `proof_basis=gabriel_positive_connectivity` et `forest_semantics=partial_refinement`.

## 7. Cas fondamental $k=1$

Les facettes sont les singletons $\lbrace x\rbrace$, tous nés au niveau zéro. Par convention normative, `hgp_reduced` et `full_pi0` coïncident à cet ordre : les feuilles singleton ne sont pas supprimées par la réduction. Un simplexe de Gabriel est une paire $\lbrace u,v\rbrace$ dont la boule diamétrale est vide, et son niveau vaut

$$\beta(\lbrace u,v\rbrace)=\frac{\left\Vert u-v\right\Vert^2}{4}.$$

Le graphe de Gabriel contient l'arbre minimum couvrant euclidien. La forêt $T_1$ produite par le flot de Gabriel doit donc être exactement la forêt de fusion obtenue en triant les arêtes de l'EMST avec les niveaux ci-dessus, lots de longueurs égales compris.

Cette égalité est un oracle structurel puissant. Elle ne suffit pas à calculer les ordres supérieurs, mais toute implémentation qui l'échoue est incorrecte avant même d'aborder $k=2$.

## 8. Facettes isolées et incidences silencieuses

Une facette isolée de $\Gamma_k(a)$ peut ne pas être un sommet du K-graphe de Gabriel à ce niveau. L'ajouter à sa naissance dans un DSU élagué ne suffit pas à préserver sa future attache : un événement non-Gabriel peut l'absorber sans changer immédiatement l'ensemble de points du K-polyèdre non trivial déjà présent, puis cette incidence devient décisive plus tard.

Il faut donc distinguer :

- `hgp_reduced`, qui suit toutes les composantes à $k=1$, puis les composantes non triviales calculées depuis Gamma exhaustif pour $k\geq2$;
- `full_pi0`, qui doit suivre aussi la naissance et l'absorption de chaque composante isolée au moyen du catalogue de Morse et d'attaches globales.

La voie exacte conserve les incidences de toutes les facettes, même lorsque leur généalogie publique est omise. Le flot Gabriel brut ne conserve qu'une sous-relation positive et reste conditionnel.

Le [lemme des attaches silencieuses](INCIDENCES_SILENCIEUSES_GAMMA.md) précise l'obstruction. Sous support essentiel unique, une coface non-Gabriel rencontre une seule racine antérieure et n'ajoute aucun point, mais elle peut ajouter une facette. Une étoile d'attaches vers ces facettes simultanées restitue exactement les composantes Gamma lorsque toutes les cofaces sont connues. La génération sparse certifiée de toutes les attaches utiles reste séparément ouverte.

## 9. Lemme utile sur les naissances isolées

Soit $F$, $\lvert F\rvert=k$, un sommet isolé de $\Gamma_k(a)$ au niveau exact de sa naissance $a=\beta(F)$. Alors la miniball de $F$ ne contient aucun point extérieur dans sa boule fermée.

En effet, si $z\in X\setminus F$ appartenait à cette boule, alors $F\cup\lbrace z\rbrace$ serait contenu dans une boule de rayon carré $a$. Toutes ses facettes seraient actives à ce niveau et $F$ posséderait une adjacency élémentaire, contradiction.

Ainsi, toute vraie naissance isolée est une sphère critique de rang fermé $k$. Le catalogue de Morse contient donc les générateurs nécessaires à `full_pi0`. Le verrou restant n'est pas leur découverte, mais la certification de leur attache ultérieure.

## 10. Ancre naissance–selle entre ordres

Pour $2\leq s\leq K_{\mathrm{eff}}$, une sphère critique de rang $s$ est un minimum de $D_s$ et un événement d'indice un de $D_{s-1}$. À son niveau $a$, son centre appartient à $L_s(a)$ et à $L_{s-1}(a)$. Dans `full_pi0`, la composante nouvellement née dans $T_s$ doit être envoyée vers la composante de $T_{s-1}$ contenant ce centre après traitement du lot fermé de niveau $a$. Le rang un n'a pas de tranche inférieure; un événement de rang $K_{\mathrm{eff}}+1$ ne possède pas de tranche source dans la tour calculée.

Dans la base exacte v2, la cible est l'unique composante de Gamma exhaustif qui contient la composante source au même niveau fermé. Le backend de référence la calcule directement et vérifie l'inclusion et l'unicité.

Une voie expérimentale tente de localiser une cible dans le seul DSU de Gabriel par une descente constructive issue de la preuve du théorème 4. Soit $Q$, $\lvert Q\rvert=k+1$, une coface active de l'ordre $k$. Si $Q$ n'est pas de Gabriel, choisir exactement un intrus $z$ dans l'intérieur de sa miniball et un point $u$ de son support, puis poser

$$Q'=\bigl(Q\setminus\lbrace u\rbrace\bigr)\cup\lbrace z\rbrace.$$

La preuve du théorème 4 donne $\beta(Q')<\beta(Q)$, et $Q,Q'$ partagent la facette $Q\setminus\lbrace u\rbrace$. La transition reste donc dans la même composante de $\Gamma_k$ au niveau courant. La décroissance stricte et la finitude conduisent à un $k$-simplexe de Gabriel $G$. Cela ne prouve pas que la racine du DSU Gabriel brut représente toute la composante Gamma : les choix et témoins certifient le chemin positif, pas l'exhaustivité des incidences.

Une coface non-Gabriel peut contenir bien plus de points strictement intérieurs que la profondeur du catalogue. Le locator ne dépend donc pas de `RelevantGP` pour ces étapes : il certifie séparément le support minimal unique et essentiel, l'intrus strict et l'inégalité de décroissance. Une égalité extérieure ou une ambiguïté de support arrête ce diagnostic; même sans ambiguïté, il ne promeut pas le flot brut au statut exact.

Toute composante source réduite d'ordre $k+1$ contient un label $Q$ de taille $k+1$. Vu à l'ordre $k$, ce label est une coface qui relie ses facettes, donc sa cible Gamma est non triviale. Deux labels adjacents de la source partagent une facette, ce qui garantit l'indépendance de la cible dans Gamma. L'oracle `locate_reduced_root(k,Q,a)` reste un candidat de réduction; ses flèches sont partielles sauf vérification indépendante contre Gamma.

Pour $2\leq s\leq K_{\mathrm{eff}}$, le minimum de rang $s$ est initialement une facette isolée et n'est donc pas un nœud public de `hgp_reduced`. L'ancre naissance–selle ne doit pas être promise dans ce profil avant qu'une composante source non triviale soit effectivement représentée; elle devient alors un contrôle croisé de la cible Gamma. Cette asymétrie n'existe pas dans `full_pi0`.

Cette règle reste correcte lorsque plusieurs événements partagent $a$, à condition de poser l'image après la contraction simultanée.

## 11. K-MST de sortie ou forêt de fusion

Le K-MST élagué du manuscrit n'est plus un certificat d'exactitude autorisé. Une future structure sparse complétée en incidences pourra devenir un certificat seulement après preuve. La sortie normative conserve :

- la forêt de fusion multifurquée;
- le `coverage_log`, formé des `coverage_delta` de chaque lot;
- les labels de facettes nécessaires aux coupes;
- les informations suffisantes pour rejouer les unions de points à la demande.

Les hyperarêtes Gamma acceptées par Kruskal de référence peuvent être conservées comme certificat; les hyperarêtes Gabriel seules ne certifient qu'une connectivité positive. La matérialisation immédiate des unions de points reste facultative.

Une hyperarête de taille $k+1$ coûte $k$ unions au lieu de $\binom{k+1}{2}$ arêtes. Pour $k\leq10$, cet encodage est borné et directement GPU-friendly.

## 12. Restitution d'une coupe

À l'ordre un, une coupe de `hgp_reduced` contient aussi les composantes singleton. Pour $k\geq2$, elle rejoue la forêt Gamma réduite et le `coverage_log` jusqu'à $a$, puis retourne les nœuds actifs non triviaux et l'union des observations de leurs facettes. Une coupe de `full_pi0` ajoute les composantes isolées à tous les ordres. Deux sorties peuvent partager des identifiants d'observations.

La condensation par taille, le choix d'un représentant unique pour une observation ou la conversion en partition sont des transformations aval. Elles doivent publier leurs propres paramètres et ne modifient pas la forêt HGP source.

### Sortie budgétée

Si le catalogue ne contient qu'un sous-flot certifié $E'$ du flot complet $E$, la structure obtenue est une `partial_forest`, pas une forêt HGP. Pour tout niveau $a$ et toute paire de facettes émises,

$$F\sim_{E'_{\leq a}}F'\Longrightarrow F\sim_{E_{\leq a}}F'.$$

La partition partielle est donc un raffinement de la partition exacte. Elle peut manquer une connexion, jamais en inventer une; son niveau de connexion fini est une borne supérieure du vrai niveau. En revanche, ses nœuds de naissance et de fusion ne sont pas des événements HGP certifiés : une hyperarête antérieure absente peut les rendre tardifs ou redondants. Les morphismes verticaux restent absents ou explicitement partiels tant que leurs cibles ne sont pas certifiées.

## 13. Résultats acquis et obligations

| énoncé | statut |
|---|---|
| $\pi_0(L_k(a))$ correspond aux K-polyèdres | théorème 2 du manuscrit |
| les adjacences de cardinal $k+1$ suffisent | proposition 5 du manuscrit |
| tout simplexe séparant est de Gabriel | théorème 4 du manuscrit, sous position générale |
| Gabriel préserve les K-polyèdres non triviaux | `false_in_general` pour le graphe élagué; contre-exemple exact permanent |
| un K-MST élagué préserve ces composantes | `false_in_general` dans cette formulation, par héritage du contre-exemple |
| Gamma exhaustif puis omission de la généalogie publique des facettes isolées définit `hgp_reduced` | fait exact définitionnel, base `gamma_exhaustive_reference` |
| une coface non-Gabriel stricte ne produit qu'une attache silencieuse $q=1$ | `proved_here` sous support essentiel unique et marge extérieure stricte |
| Gabriel complété par toutes les attaches silencieuses restitue Gamma | `proved_here` sous les mêmes hypothèses; la génération sparse complète reste ouverte |
| une étoile remplace une clique pour $H_0$ | fait élémentaire exact |
| la multiplicité locale d'indice $\mu$ vaut $\binom{\lvert U\rvert-1}{\mu}$ | théorème de Reani–Bobrowski; le nombre de classes globalement tuées dépend des attaches |
| les naissances isolées ont rang fermé $k$ | lemme ci-dessus |
| le flot de Gabriel seul reconstruit leur généalogie | `false_in_general`; fixture permanente à cinq points |
| catalogue complet et attaches exactes reconstruisent `full_pi0` | `proof_obligation` M.1; aucune revendication exacte avant preuve |
| le locator non-Gabriel définit la cible verticale réduite exacte dans le DSU brut | `proof_obligation`; le chemin positif ne prouve pas la complétude des incidences |
| un sous-flot certifié produit un raffinement de connectivité | fait exact unilatéral; ses nœuds ne sont pas une forêt HGP exacte |

## Références

- L. Hauseux, [*Manuscrit de thèse*](../references/MANUSCRIT_THESE_HAUSEUX.pdf), chapitres 6 et 8.
- L. Hauseux, [*Generalization of single-linkage with higher-order interactions*](../references/pdfs/hauseux-et-al-hgp-applied-network-science-2026.pdf), *Applied Network Science*, 2026.
- Y. Reani et O. Bobrowski, [*Morse Theory for the k-NN Distance Function*](https://arxiv.org/abs/2403.12792), 2024.
