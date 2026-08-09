# Verrous mathématiques prioritaires — F0, rayons owner et census terminal

Date : 9 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=architecture_math_and_cpu_oracles`,
`profile=quantized_u16_input_only`,
`mode=constructive_math_locks_for_claude`,
`public_status=not_claimed`.

> [!IMPORTANT]
> Cette note aide Claude à implémenter les prochaines fermetures; elle
> n'implémente rien à sa place. Elle sépare trois résultats : la décision F0
> source-agnostique, un réducteur exact des rayons owner et une voie exacte de
> census global. Aucun de ces résultats ne promeut le prototype ni le SLO.

## 1. Décisions immédiates

| verrou | décision mathématique | résiduel d'implémentation et de preuve |
| --- | --- | --- |
| naissance F0 sans carrier strict | une composante `q_R=0` portant une `DirectHyperedge` est une naissance; la garde par composante doit disparaître du fold général | oracle indépendant depuis le `RawBatch`, fixture géométrique d'arité quatre et mutations |
| capability régulière | chaque hyperarête directe régulière possède exactement $\lvert U\rvert\geq2$ facettes strictes | validation **par record avant projection**, distincte du fold |
| owner d'arité deux | les rayons extrêmes du cône signé se calculent en $O(\lvert S(v)\rvert+\lvert B_U\rvert)$ et mémoire $O(1)$ | intégrer le réducteur, comparer ses rayons à un oracle et mesurer le harvest total |
| owner d'arité trois | les deux orientations se décident par un seul scan signé | garder une garde de rang et tester les deux orientations |
| census d'une sphère | c'est un report exact de demi-espace en dimension quatre après relèvement paraboloïde | structure de report certifiée, statuts cappés fail-closed, terminalité et reçus |

L'ordre utile à Claude est : corriger d'abord la sémantique F0 et sa vérité,
intégrer ensuite le réducteur de rayons, puis construire le census terminal. F1,
F2, couverture et verticales ne peuvent pas réparer une source incomplète.

## 2. F0 : naissance générale et invariant régulier sont deux théorèmes différents

### 2.1 Lemme exact de suppression d'une facette

Soit $Q$ un ensemble fini, $B$ son unique miniboule et $a=\beta(Q)$. Pour
$x\in Q$ :

$$\beta(Q\setminus\lbrace x\rbrace)=a\quad\Longleftrightarrow\quad Q\setminus\lbrace x\rbrace\ \text{contient un support minimal de }B.$$

Dans le sens direct, $B$ couvre encore $Q\setminus\lbrace x\rbrace$ avec le
rayon minimal $a$. L'unicité de la miniboule impose que sa miniboule soit encore
$B$, dont elle contient donc un support minimal. Le sens réciproque est
immédiat : ce support force le rayon de $B$, tandis que $B$ couvre l'ensemble.

Ce lemme donne la caractérisation utile au fold : toutes les facettes de $Q$
sont activées exactement à $a$ si et seulement si chaque point de $Q$ est
évitable par au moins un support minimal de $B$.

### 2.2 Corollaire sous la porte régulière forte

Supposons maintenant $Q=I(B)\mathbin{\dot\cup}U(B)$, avec support unique
essentiel $U(B)$. Pour chaque $u\in U(B)$, aucun support de $B$ ne subsiste dans
$Q\setminus\lbrace u\rbrace$, donc $\beta(Q\setminus\lbrace u\rbrace)<a$. Pour
chaque $x\in I(B)$, le support $U(B)$ subsiste, donc
$\beta(Q\setminus\lbrace x\rbrace)=a$.

Une `DirectHyperedge` régulière possède ainsi exactement $\lvert U(B)\rvert$
facettes strictes. Pour des points distincts et une coface non triviale,
$\lvert U(B)\rvert\geq2$. Cet invariant est réel, mais il appartient au
**validateur de source régulière**, pas au classificateur F0 général.

La fixture régulière qui atteint la borne est :

```text
A=(0,0,0) B=(4,0,0) z=(2,1,0) D=(1,2,10)
```

Pour `Q=ABz` à l'ordre deux, $U=\lbrace A,B\rbrace$. Les facettes `Az` et
`Bz` sont strictes; `AB` est égale. Le record porte exactement deux endpoints
stricts et un $N_a$.

### 2.3 Contre-exemple géométrique hors porte régulière

Le fold annoncé accepte aussi la source Gabriel ouverte générale. Considérons :

```text
A=(0,0,0) B=(2,0,0) C=(2,2,0) D=(0,2,0) E=(0,0,10)
```

À l'ordre trois, `Q=ABCD` a pour miniboule le cercle de centre `(1,1,0)`
et de rayon carré `2`; `E` est extérieur et rend le nuage affine trois. Chacun
des quatre triangles de `Q` contient encore une diagonale du carré, donc possède
la même miniboule et le même niveau. Les quatre facettes sont des $N_a$.

La coface possède deux supports diagonaux et échoue précisément la porte
régulière. Elle reste une coface directe valide dans le contrat général et doit
créer une naissance `q_R=0`. La fixture F0 actuelle à deux endpoints est utile
comme abstraction, mais elle n'est pas géométrique pour le domaine annoncé
$k\geq2$, où une coface produit au moins trois facettes. Le carré d'arité quatre
retire cette ambiguïté.

### 2.4 Table de décision du fold général

Après fermeture de tout le lot exact, pour chaque composante portant un record
logique :

| racines strictes distinctes | record direct présent | décision |
| ---: | --- | --- |
| $0$ | oui | une naissance |
| $0$ | non | erreur : attaches sans cible stricte |
| $1$ | oui ou non | continuation de l'unique racine |
| $\geq2$ | oui | une multifusion |
| $\geq2$ | non | erreur de contrat des attaches |

Aucune ligne n'exige un $R^{-}$ ou un $L^{-}$ pour une naissance directe. Une
garde de carrier ajoutée à cette table change la sémantique.

### 2.5 Pourquoi la garde actuelle ne valide même pas la régularité

La propriété régulière est **locale à chaque `DirectHyperedge` brute**. Une
garde après fermeture, au niveau de la composante, laisse passer le lot suivant :

```text
bad_all_new      = DirectHyperedge(N0,N1)
good_with_strict = DirectHyperedge(L0,N1)
```

Les deux records deviennent connexes par `N1`; le `L0` du second masque le
premier record malformé. Un validateur régulier doit donc vérifier séparément,
avant projection, les identités source, les niveaux de toutes les facettes et
le reçu support/miniboule de chaque coface. Il ne doit jamais inférer la
régularité depuis la composante quotientée.

### 2.6 Oracle F0 indépendant et fixtures permanentes

Pour le petit domaine borné, une vérité indépendante peut éviter à la fois
Warshall et le DSU :

1. relire le `RawBatch` avec son propre validateur d'identités, de niveaux,
   d'arités et d'attaches;
2. construire ses propres sommets typés depuis le snapshot strict, sans appeler
   les helpers du sujet;
3. énumérer toutes les partitions des au plus cinq sommets projetés;
4. retenir l'unique partition la plus fine dans laquelle les endpoints de chaque
   record logique appartiennent à un même bloc;
5. appliquer littéralement la table ci-dessus et comparer les signatures
   récursives ainsi que le ledger complet.

Le domaine est assez petit pour que l'énumération des partitions de Bell reste
un oracle simple. Les assertions du gate doivent être des contrôles explicites,
afin que `python3 -O` ne puisse jamais rendre un faux `PASS`.

Fixtures et mutations minimales :

| fixture | attente | mutant tué |
| --- | --- | --- |
| carré `ABCD` tout $N_a$ | une naissance, quatre facettes, un record direct complet | `reject_carrierless` |
| `ABz` régulier | deux strictes et une égale avant projection | suppression d'une facette stricte |
| `regular_smuggling` ci-dessus | rejet du premier record par le validateur régulier | validation par composante |
| même composante avec un $R^{-}$ | continuation | naissance décidée record par record |
| même composante avec deux racines distinctes | multifusion unique | fermeture non atomique du lot |
| attaches seules avec $q_R=0$ | erreur | naissance sans record direct |

## 3. Owner : réducteur exact des rayons sans énumérer les triplets

### 3.1 Réduction intrinsèque de l'arité deux

Soit $U=\lbrace p_0,p_1\rbrace$, $e=p_1-p_0\neq0$. Toute direction de $F_U$
s'écrit

$$d(w)=(w,2p_0\mathbin{\cdot}w),\qquad w\mathbin{\cdot}e=0.$$

Pour $s\in S(v)\setminus U$, posons $q_s=p_s-p_0$,
$u_s=e\mathbin{\times}q_s$ et $b_s=\varepsilon_su_s$, avec
$\varepsilon_s=-1$ si $s\in B_U\cap S(v)$ et $+1$ sinon. Sur le plan
$e^{\perp}$, définissons $D_e(x,y)=e\mathbin{\cdot}(x\mathbin{\times}y)$. Alors :

$$\varepsilon_sa_s\mathbin{\cdot}d(w)=\frac{2}{\left\Vert e\right\Vert^2}D_e(b_s,w).$$

Le cône tangent signé est donc exactement l'intersection des demi-plans
centraux $H(b_s)=\lbrace w:D_e(b_s,w)\geq0\rbrace$. Si $u_s=0$, le point $s$
est collinéaire à $U$ et sa restriction est identiquement nulle; il faut
l'ignorer.

Cette écriture évite de fabriquer une base rationnelle du plan. Tous les rayons
stockés sont des $\pm b_s$, les signes relatifs se lisent par `orient3d`, et la
taille arithmétique ne croît pas au fil du scan.

### 3.2 Automate de cône à mémoire constante

Maintenir un état parmi :

- `FULL`, le plan entier;
- `HALF(a)=H(a)`;
- `LINE(r)`, la droite engendrée par $r$;
- `RAY(r)=\mathbb{R}_{+}r`;
- `WEDGE(l,r)`, le cône engendré par $l,r$ avec $D_e(l,r)>0$;
- `ZERO`.

Pour une nouvelle contrainte $H(b)$, les transitions exactes sont :

| état | tests | nouvel état |
| --- | --- | --- |
| `FULL` | — | `HALF(b)` |
| `ZERO` | — | `ZERO` |
| `HALF(a)` | $D_e(a,b)>0$ | `WEDGE(b,-a)` |
| `HALF(a)` | $D_e(a,b)<0$ | `WEDGE(a,-b)` |
| `HALF(a)` | $D_e(a,b)=0$, $a\mathbin{\cdot}b>0$ | inchangé |
| `HALF(a)` | $D_e(a,b)=0$, $a\mathbin{\cdot}b<0$ | `LINE(a)` |
| `LINE(r)` | $D_e(b,r)>0$ | `RAY(r)` |
| `LINE(r)` | $D_e(b,r)<0$ | `RAY(-r)` |
| `LINE(r)` | $D_e(b,r)=0$ | inchangé |
| `RAY(r)` | $D_e(b,r)\geq0$ | inchangé |
| `RAY(r)` | $D_e(b,r)<0$ | `ZERO` |

Pour `WEDGE(l,r)`, poser $t_l=D_e(b,l)$ et $t_r=D_e(b,r)$ :

| condition | nouvel état |
| --- | --- |
| $t_l\geq0$ et $t_r\geq0$ | inchangé |
| $t_l<0$ et $t_r<0$ | `ZERO` |
| $t_l=0$ et $t_r<0$ | `RAY(l)` |
| $t_l<0$ et $t_r=0$ | `RAY(r)` |
| $t_l<0<t_r$ | `WEDGE(b,r)` |
| $t_l>0>t_r$ | `WEDGE(l,-b)` |

La preuve tient en une ligne : tout $w$ du wedge s'écrit
$w=\alpha l+\beta r$, $\alpha,\beta\geq0$, et la nouvelle inégalité vaut
$\alpha t_l+\beta t_r\geq0$. Les transitions des autres états sont les mêmes
intersections en dimension un ou deux. Par induction sur le scan, l'état final
est exactement $K_v^U$, indépendamment de l'ordre des contraintes.

À un vrai sommet, les normales de $S(v)$ ont rang quatre. Après quotient par
les deux normales indépendantes de $U$, les restrictions ont rang deux; la
linéalité finale est donc nulle. Les états terminaux `FULL`, `HALF` ou `LINE`
signalent une contradiction de rang et doivent échouer fermés. Les seuls états
valides sont `ZERO`, `RAY` et `WEDGE`, qui donnent respectivement zéro, un ou
deux rayons extrêmes.

### 3.3 Prédicats entiers et objectif owner

Pour $u_s=e\mathbin{\times}(p_s-p_0)$, l'identité

$$D_e(u_s,u_t)=\left\Vert e\right\Vert^2\mathrm{orient3d}(p_0,p_1,p_s,p_t)$$

permet de décider chaque signe par le prédicat exact existant. Lorsque le
déterminant est nul, le produit scalaire de deux cross distingue des directions
égales et opposées. Une normalisation primitive par `gcd` rend la représentation
des rayons déterministe pour l'oracle.

Il n'est pas nécessaire de rescanner $B_U$ pour chaque rayon. Précalculer :

$$h_U=2\left(np_0-\sum_{i\in X}p_i\right)+4\sum_{i\in B_U}(p_i-p_0).$$

Alors $g_U\mathbin{\cdot}d(w)=h_U\mathbin{\cdot}w$. Pour chacun des au plus deux
rayons, le verdict teste donc :

$$\left(h_U\mathbin{\cdot}w,w_x,w_y,w_z,2p_0\mathbin{\cdot}w\right)<_{\mathrm{lex}}0.$$

Sous le profil u16 et les bornes de taille déclarées, une borne conservatrice
reste sous $2^{120}$; `i128` convient si les conversions et multiplications
restent gardées. La fusion de `S(v)` et de `B_U`, tous deux triés, doit produire
les $\varepsilon_s$ à deux pointeurs. Conserver un `binary_search` par point
transformerait la borne annoncée en $O(m\log\lvert B_U\rvert)$.

### 3.4 Arités trois et quatre

Pour $U=\lbrace p_0,p_1,p_2\rbrace$, poser
$u=(p_1-p_0)\mathbin{\times}(p_2-p_0)$. Les deux directions possibles sont
$d_{\pm}=\pm(u,2p_0\mathbin{\cdot}u)$. Un unique scan maintient :

```text
allow_plus  &= epsilon_s * (-sign(orient3d(p0,p1,p2,s))) >= 0
allow_minus &= epsilon_s * (-sign(orient3d(p0,p1,p2,s))) <= 0
```

Une seule orientation admissible donne un rayon, aucune donne `ZERO`. Deux
orientations admissibles signifient que toutes les restrictions sont nulles;
c'est une contradiction de rang à refuser. À l'arité quatre, le quotient est de
dimension zéro et il n'existe aucun rayon.

### 3.5 Coût réellement fermé et coût encore ouvert

Le réducteur coûte $O(m+\lvert B_U\rvert)$ par décision owner et $O(1)$ mémoire,
contre $O(m(m+\lvert B_U\rvert))$ aujourd'hui pour une paire. Sur les
$\Theta(m^2)$ paires, le terme owner passe de
$O(m^4+m^3\lvert B_U\rvert)$ à $O(m^3+m^2\lvert B_U\rvert)$.

Cela ne ferme pas tout le harvest. Si $\Theta(m^3)$ bases de triangles sont
encore matérialisées et chacune rescannée, l'arité trois conserve un terme
$O(m^4)$. Le réducteur ferme le verrou owner de chaque candidat; le quotient
des flats et la masse de candidats restent une porte séparée.

### 3.6 Porte permanente et défaut de la fixture live

L'oracle borné doit énumérer les directions $\pm u_s$, tester exhaustivement
toutes les contraintes et comparer l'ensemble primitif des rayons à l'automate.
Il faut permuter l'ordre des contraintes et exercer `FULL`, `HALF`, `LINE`,
`RAY`, `WEDGE`, `ZERO`, les $u_s=0$, les normales répétées et opposées. Deux
compteurs contractuels suffisent : `constraints_scanned=m` et `rays_tested<=2`.

Mutants : inverser $D_e$; ne pas ignorer $u_s=0$; confondre $b$ et $-b$ lorsque
$D_e=0$; supprimer un endpoint; ne tester qu'une orientation à l'arité trois;
accepter un état avec linéalité; supprimer le tie-break lorsque
$g_U\mathbin{\cdot}d=0$; remplacer $\varepsilon=-1$ par $+1$.

La nouvelle fixture catalogue `owner_signed_cone` ne tue pas ce dernier mutant.
Sur ses deux sommets candidats :

| sommet | owner signé | owner non signé |
| --- | --- | --- |
| coquille `{0,1,2,4}`, intérieur `{3}` | oui | non |
| coquille `{0,1,2,3}`, intérieur `{4}` | non | oui |

Le mutant déplace seulement le propriétaire de l'extrémité $z=0$ vers
$z=4$; le catalogue final et la table owner restent identiques. La porte doit
donc comparer directement l'identité du sommet propriétaire, ou exécuter une
mutation du signe attendue rouge. Comparer seulement les catalogues ne protège
pas le théorème local annoncé.

## 4. Census terminal : une requête exacte de demi-espace en dimension quatre

### 4.1 Identité de relèvement

Pour une sphère rationnelle `Sphere{base,n,d}` avec $d>0$, poser
$C=d\,\mathrm{base}+n$ et $N=\left\Vert n\right\Vert^2$. Pour tout point entier
$p$ :

$$F_B(p)=d^2\left\Vert p\right\Vert^2-2dC\mathbin{\cdot}p+\left\Vert C\right\Vert^2-N.$$

Le signe négatif, nul ou positif signifie exactement intérieur, coquille ou
extérieur. Avec $\varphi(p)=(p_x,p_y,p_z,\left\Vert p\right\Vert^2)$,
`closed_ball(B)` est donc un report de demi-espace affine dans
$\mathbb{R}^{4}$.

Une seule structure immuable sur $\varphi(X)$ suffit; elle ne matérialise ni
mosaïque de Delaunay d'ordre supérieur, ni cellules, ni cofaces. Une partition
tree générale donne espace $O(n)$ et requête
$O(n^{3/4}\log^{O(1)}n+r)$. Pour le halfspace reporting spécialisé en dimension
quatre, Matoušek donne un espace $O(n\log\log n)$ et une requête
$O(\sqrt{n}\log^c n+r)$. Ces bornes sont des constructions disponibles, pas des
mesures du prototype : voir
[Efficient partition trees](https://doi.org/10.1145/109648.109649) et
[Reporting points in halfspaces](https://doi.org/10.1016/0925-7721(92)90006-E).

### 4.2 API cappée et fail-closed

Pour un rang produit maximal $s_{\max}=K+1$, demander au plus $K+2$ témoins
fermés distingue les statuts suivants :

| statut | sens autorisé |
| --- | --- |
| `complete(I,S)` | le report est terminal et tous les points rendus sont reclassés exactement |
| `outside_regular_cap` | au moins $K+2$ points fermés distincts; sortie de la branche régulière |
| `incomplete` | budget, interruption ou watermark absent; aucune décision scientifique |
| `invalid` | sphère, largeur, nuage ou index non authentifié |

`outside_regular_cap` ne signifie jamais « source absente » hors de la porte
régulière : une grande extra-coquille peut porter une masse réelle de cofaces.
Le report doit être fermé, puis séparer $I(B)$ et $S(B)$ par le prédicat entier;
une requête seulement ouverte perd les extra-shells.

Les occurrences candidates doivent être triées par support, puis les sphères
par clef rationnelle normalisée. Une même sphère reçoit un seul census, même si
plusieurs masques ou supports l'ont retrouvée. Le pipeline compatible avec la
mémoire cible est :

```text
occurrences de masques -> runs tries par U -> miniboule
                       -> runs tries par clef exacte de sphere
                       -> census global -> support canonique -> owner -> emission
```

### 4.3 Contre-exemple à un census limité à la coquille du sommet

Soit le sommet de centre `(10,10,10)` et rayon carré `25`, porté par :

```text
A=(13,14,10) B=(13,6,10) C=(5,10,10) D=(10,10,15)
```

Ces quatre points sont affinement indépendants. La boule diamètre de `AB` a
centre `(13,10,10)` et rayon carré `16`. Ajouter `z=(16,10,10)` donne une
puissance `11` dans la sphère du sommet, mais `-7` dans la boule `AB` : `z` est
invisible depuis $S(v)$ et pourtant strictement intérieur à la candidate. Le
census global est indispensable à la seconde inclusion owner.

Variantes exactes :

| point | puissance au sommet | puissance dans `AB` | obligation |
| --- | ---: | ---: | --- |
| `(17,10,10)` | 24 | 0 | report fermé et branche extra-shell |
| `(7,10,10)` | -16 | 20 | vérifier tout $B(v)$ contre la candidate |
| `(9,10,10)` | -24 | 0 | ne pas remplacer l'inclusion stricte par une inclusion fermée |
| `(12,10,10)` | -21 | -15 | intérieur positif transporté |
| `(15,10,10)` | 0 | -12 | intérieur positif issu de $S(v)$ |

### 4.4 Porte CPU bornée

Une première porte indépendante peut parcourir les 4 096 sous-nuages de
$\lbrace8,10,12\rbrace\mathbin{\times}\lbrace8,12\rbrace\mathbin{\times}\lbrace8,12\rbrace$.
Pour chaque support de taille deux à quatre, elle compare le reporter au scan
`cpp_int` indépendant : statut, ensemble fermé, partition intérieur/coquille et
équivariance. Le domaine comporte au plus 306 944 requêtes et moins de
3,7 millions de classifications ponctuelles.

Ajouter les cinq variantes ci-dessus, une sphère u16 à grand centre rationnel,
des représentations non réduites d'une même sphère, le cube multi-support, les
fixtures d'arités deux et trois, un cap dépassé, une interruption et un index
étranger.

Mutants : boule ouverte; cap décalé; `complete` après interruption; pruning
flottant; absence du census extérieur; oubli de $B(v)$; égalité extérieure
ignorée; mauvaise fusion des clefs rationnelles; `outside_regular_cap` traité
comme absence.

## 5. Ce que ces résultats ferment — et ne ferment pas

- La sémantique de naissance F0 est décidée. Reste à corriger le sujet et surtout
  sa vérité corrélée.
- L'invariant « au moins deux facettes strictes » est prouvé seulement sous la
  porte régulière et doit être validé record par record.
- Le coût des rayons owner d'un candidat est ramené à un scan linéaire exact;
  le nombre total de flats candidats reste ouvert.
- Le census global n'est plus une primitive mathématique inconnue : c'est un
  halfspace-report exact en dimension quatre. Son implémentation certifiée, ses
  constantes, la terminalité du stream et le contrat 50 k restent ouverts.
- F1 et F2 externalisent un fold correct; ils ne compensent jamais un census ou
  une source incomplets.

GCP non utilisé.
