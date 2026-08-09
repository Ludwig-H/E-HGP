# Audit mathématique de la connectivité du parcours d'ordre $k$

Date : 9 août 2026 UTC.

## Verdict

> [!IMPORTANT]
> **La question de connectivité possède une réponse positive, mais sous un énoncé plus précis que celui du prototype.** Pour tout arrangement fini d'hyperplans affines non verticaux ayant des sommets, le 1-squelette réel induit par les sommets de niveau au plus $k$ est connexe. En arrangement simple dans $\mathbb{R}^{4}$, ses arêtes finies sont exactement les adjacences entre sommets consécutifs qui partagent trois hyperplans. Un seul sommet de niveau zéro suffit donc à parcourir tous les sommets d'arité quatre de niveau au plus $k$, si le germe, les voisins et les niveaux sont exacts.

Cette preuve **ne qualifie pas le prototype comme chemin produit** :

- RelevantGP n'implique pas la simplicité de l'arrangement global traversé ;
- le code représente un sommet par quatre identifiants et ne traite pas les sommets de multiplicité supérieure ;
- les témoins constants sur un pinceau restent un défaut d'implémentation indépendant de la connectivité ;
- les arités un à trois sont des minima métriques sur des strates de dimension positive, pas des sommets de l'arrangement ;
- le coût courant reste en $\Theta(nV)$ et matérialise globalement les sommets visités.

Le finding « connectivité non démontrée » de l'audit général peut donc être fermé comme **théorème conditionnel de navigation**, jamais comme fermeture de M3 produit.

## 1. Snapshots et méthode

Le théorème ci-dessous audite la logique de order_k_vertices, inchangée entre les deux contenus observés :

| objet | empreinte |
| --- | --- |
| commit audité | $7fa39b1d8c9d3b566bcd098bb4bdd2dbc107d7af$ |
| header commité | SHA-256 $a8111f02f76e458912e2a2e1e1ff2d4ee0b71bba31af7993975f49fa6c792a3c$ |
| header live, ajout du harvest après le parcours | SHA-256 $cf9374b64fdc6428625a1e8f72ecb6e19e6d66a80d3249361c694ea064c6d256$ |
| énumérateur indépendant sous /tmp | SHA-256 $fdd05b208342cca9dfd7d68345f908031bb657d37444fe8c17913d40cf9bbcd3$ |

Le contenu live ajoute order_k_catalogue après order_k_vertices ; il ne modifie ni le graphe parcouru, ni sa coupe par niveau. Les deux sujets sont donc distingués dans les sections 2 et 5.

GCP non utilisé.

## 2. Théorème de connectivité

### 2.1 Convention exacte du relevé

Pour un centre $c\in\mathbb{R}^{3}$, posons $t=\lVert c\rVert^{2}-r^{2}$ et, pour chaque point $p_i$,

$$H_i(c)=2c\mathbin{\cdot}p_i-\lVert p_i\rVert^{2}.$$

Le point $p_i$ est strictement intérieur à la sphère $(c,r)$ exactement lorsque $H_i(c)>t$. Le niveau d'un point $x=(c,t)$ de l'espace des paramètres est donc

$$\ell(x)=\#\lbrace i:H_i(c)>t\rbrace.$$

Tous les hyperplans $t=H_i(c)$ sont non verticaux. À un sommet $v$, notons

$$B(v)=\lbrace i:H_i(c_v)>t_v\rbrace,\qquad \ell(v)=\lvert B(v)\rvert.$$

Un déplacement vertical infinitésimal **au-dessus** de $v$ conserve exactement les hyperplans de $B(v)$ au-dessus du point et place tous les hyperplans porteurs de $v$ en dessous. Il entre donc dans une chambre pleine de même niveau.

### 2.2 Polyèdre de la chambre associée à un sommet

Pour tout ensemble de signes réalisable $B$, la fermeture de la chambre correspondante est

$$P_B=\lbrace(c,t):t\leq H_i(c)\ \text{pour}\ i\in B,\quad t\geq H_j(c)\ \text{pour}\ j\notin B\rbrace.$$

Soit $v$ un sommet de niveau $\ell>0$, et prenons $B=B(v)$. Alors :

1. $P_B$ est de dimension quatre, car le déplacement vertical infinitésimal au-dessus de $v$ satisfait toutes ses inégalités strictement.
2. $v$ est un sommet de $P_B$. En particulier, $P_B$ est pointé : un polyèdre contenant une droite ne possède aucun point extrême.
3. Au moins une contrainte $t\leq H_i(c)$, avec $i\in B$, porte une facette non vide de $P_B$.

Le troisième point mérite le détail qui manquait au commentaire du prototype. Sans aucune contrainte indexée par $B$, le système restant $t\geq H_j(c)$ est invariant par toute translation verticale positive. À $c$ fixé et pour $t$ assez grand, il viole toutes les contraintes $t\leq H_i(c)$, $i\in B$. Les contraintes de $B$ ne peuvent donc pas être collectivement redondantes. Une sous-famille minimale en contient au moins une ; dans un polyèdre plein, cette inégalité non redondante porte une facette.

Toute face non vide d'un polyèdre pointé contient un sommet. La facette précédente contient donc un sommet $w$ de $P_B$ tel que $t_w=H_i(c_w)$ pour au moins un $i\in B$. À chaque sommet $u$ de $P_B$, les hyperplans strictement au-dessus appartiennent à $B$. Par conséquent,

$$\ell(u)\leq\ell,\qquad \ell(w)\leq\ell-1.$$

Le graphe des sommets et arêtes d'un polyèdre pointé est connexe. Il existe donc, dans le 1-squelette de $P_B$, un chemin de $v$ à $w$ dont tous les sommets et toutes les arêtes ouvertes ont un niveau au plus $\ell$.

### 2.3 Descente jusqu'au niveau zéro

L'étape précédente diminue strictement le niveau terminal sans jamais dépasser le niveau initial. En l'itérant, tout sommet de niveau $\ell\leq k$ atteint un sommet de niveau zéro par un chemin entièrement shallow.

Tous les sommets de niveau zéro appartiennent au même polyèdre

$$P_{\varnothing}=\lbrace(c,t):t\geq H_j(c)\ \text{pour tout}\ j\rbrace.$$

Réciproquement, chaque sommet de $P_{\varnothing}$ est un sommet de niveau zéro de l'arrangement. Dès qu'il possède un sommet, $P_{\varnothing}$ est pointé : un polyèdre avec une direction de linéalité ne possède aucun point extrême. Son graphe est donc connexe. Tous les sommets de niveau zéro sont reliés entre eux.

En renversant les chemins de descente, on obtient le résultat annoncé :

> **Théorème.** Si l'arrangement possède au moins un sommet, le graphe réel induit par ses sommets de niveau au plus $k$ est connexe. Sous simplicité en dimension quatre, un parcours depuis n'importe quel sommet de niveau zéro par les voisins consécutifs partageant trois hyperplans visite tous ces sommets sans traverser un sommet de niveau supérieur à $k$.

La preuve vaut pour tout arrangement de graphes affines non verticaux ; la contrainte paraboloïde n'est pas nécessaire.

### 2.4 Région, complexe et 1-squelette

La région obtenue en réunissant les chambres de niveau au plus $k$ est facilement connexe : monter verticalement ne peut que retirer des hyperplans du compte, puis $P_{\varnothing}$ est convexe. Cela ne prouve pas à lui seul la connectivité du 1-squelette.

La preuve des sections 2.2 et 2.3 apporte précisément l'étape manquante : elle construit ses chemins sur les arêtes de fermetures de chambres shallow. Elle prouve donc plus que la seule connectivité du graphe induit sur les étiquettes de sommets. En revanche, elle ne prouve pas que toute la région est étoilée depuis un germe fixe ; ce mot doit être retiré tant qu'un argument séparé ne l'établit pas.

## 3. Niveau du sommet et niveau de l'arête ouverte

Sur une arête simple portée par trois hyperplans, soit $e$ le nombre d'autres hyperplans strictement au-dessus d'un point de son intérieur relatif. À une extrémité, le quatrième hyperplan devient shell et cesse d'être compté. Ainsi,

$$\ell(v)\in\lbrace e-1,e\rbrace,\qquad \ell(w)\in\lbrace e-1,e\rbrace.$$

Deux conséquences doivent être conservées dans le contrat :

- deux voisins peuvent avoir des niveaux qui diffèrent de $-1$, $0$ ou $+1$ ; la variation nulle est réelle ;
- une arête dont les deux sommets ont le niveau $k$ peut avoir le niveau ouvert $k+1$.

Le graphe simplement induit par les niveaux de ses extrémités peut donc contenir une arête qui n'appartient pas au sous-complexe géométrique de niveau au plus $k$. Cela ne casse pas le théorème : les chemins construits dans $P_B$ possèdent, eux, des ensembles strictement au-dessus inclus dans $B$, y compris à l'intérieur relatif de leurs arêtes.

## 4. Pourquoi RelevantGP ne suffit pas au parcours codé

Le théorème abstrait parle du **vrai 1-squelette polyédrique**. Pour l'identifier au graphe manipulé par order_k_vertices, il faut au minimum :

1. chaque sommet visité est représentable canoniquement, ou le domaine global garantit exactement quatre hyperplans incidents ;
2. une arête simple est portée par trois hyperplans indépendants et ses deux voisins finis sont les événements consécutifs du pinceau ;
3. les hyperplans qui ne coupent pas un pinceau sont classés comme constamment intérieurs ou extérieurs ;
4. le germe annoncé de niveau zéro est réellement certifié.

RelevantGP ne quantifie que les supports propres, bien centrés, utiles et sans intrus strict. Il autorise donc des égalités portées par des supports non bien centrés que la navigation globale rencontre malgré tout. La fixture noncritical_shell_tie du juge est explicite :

$$p_0=(1065,1000,100),\quad p_1=(1063,1016,100),\quad p_2=(1060,1025,100),\quad p_3=(1056,1033,100).$$

Ces quatre points sont cocycliques sur un arc court et leurs triples porteurs ne sont pas bien centrés. Le dépôt les déclare non bloquants pour RelevantGP, alors que l'arrangement possède une intersection de multiplicité supérieure à celle du modèle simple.

À l'inverse, la fixture coplanaire de [l'audit général du BFS](AUDIT_ORDER_K_BFS_A8111F0.md) montre qu'un hyperplan constant sur un pinceau peut être strictement intérieur. Le hash $a8111f0$ le saute, attribue deux faux niveaux zéro et dépend de la permutation. Il s'agit d'un défaut du germe et du transport des offsets constants, pas d'un contre-exemple au théorème de connectivité.

Deux voies seulement sont cohérentes :

- déclarer, vérifier et publier une hypothèse ArrangementSimple4D globale, strictement plus forte que RelevantGP ;
- ou représenter les sommets et arêtes de multiplicité quelconque, quotienter les supports équivalents et traiter les témoins constants.

Un rejet de toute égalité rencontrée ne peut pas être présenté comme RelevantGP.

## 5. Arité quatre contre strates d'arités basses

### 5.1 Ce que la connectivité ferme

En arrangement simple, les sommets sont des intersections de quatre hyperplans. Le théorème ferme donc uniquement la découverte exhaustive des **sommets d'arité quatre**, avant filtre de bon centrage et de shell, sous les préconditions d'implémentation de la section 4.

Un sommet d'arrangement quelconque peut être non bien centré. La connectivité autorise sa traversée, mais elle ne permet ni de l'émettre comme événement Morse, ni de borner $V$ par la taille du catalogue final.

### 5.2 Où vivent les arités un à trois

La fonction rayon vaut $r^{2}=\lVert c\rVert^{2}-t$. Un support indépendant de cardinalité $q$ impose $q$ hyperplans et laisse une strate affine de dimension $4-q$. Son événement Morse est le minimum métrique de $r^{2}$ sur cette strate :

| arité $q$ | strate dans l'arrangement relevé | événement |
| ---: | --- | --- |
| 1 | dimension 3 | rayon nul au point |
| 2 | dimension 2 | milieu de la paire |
| 3 | dimension 1 | circumcentre du triangle |
| 4 | dimension 0 | sommet bien centré |

La connectivité des seuls sommets ne transforme pas ces minima en sommets.

### 5.3 Lemme d'attachement conditionnel utile au delta live

Le hash live $cf9374b6$ récolte chaque paire et chaque triple apparaissant dans un sommet visité, puis parcourt jusqu'au plafond $s_{\max}+2$. Cette idée peut être justifiée, mais pas par les phrases « rang $r+1$, puis rang $r+2$ » actuellement écrites.

Soit $U$ un support indépendant de cardinalité $q<4$, dont la sphère minimale se trouve dans une face relativement ouverte de profondeur $d$. Restreignons les autres hyperplans au flat de dimension $4-q$ défini par $U$. Si cet arrangement restreint est essentiel et l'arrangement global simple, la fermeture de la cellule contenant le minimum est pointée et possède un sommet. Tout tel sommet :

- contient les $q$ hyperplans de $U$ ;
- a un niveau au plus $d$ ;
- a donc un rang d'arrangement au plus $4+d=(q+d)+(4-q)$.

Sous dimension affine trois du nuage et simplicité globale, l'arrangement restreint est essentiel pour toute paire distincte et tout triangle non collinéaire. Il en résulte les plafonds suffisants

$$q=2:\ 4+d\leq s_{\max}+2,\qquad q=3:\ 4+d\leq s_{\max}+1,\qquad q=4:\ 4+d\leq s_{\max}.$$

Le plafond uniforme $s_{\max}+2$ du delta live est donc **conditionnellement suffisant** pour retrouver les paires et triangles utiles comme sous-supports de sommets visités.

Ce lemme donne des inégalités, pas deux augmentations exactement égales à un : atteindre la frontière d'une cellule peut aussi transformer un ancien intérieur en shell et diminuer le niveau. Il exige en outre dimension affine trois, support indépendant, face ouverte sans extra-shell et arrangement restreint essentiel. RelevantGP seul ne fournit pas toutes ces hypothèses.

Une fixture entière minimale réfute déjà l'égalité annoncée au premier pas :

$$a=(0,2,0),\qquad b=(4,2,0),\qquad w=(2,3,0).$$

La boule diamétrale de $(a,b)$ a pour centre $(2,2,0)$ et rayon carré $4$ ; elle contient strictement $w$, donc son rang fermé vaut $r=3$. Sur la famille de centres $c_s=(2,2+s,0)$ passant par $a,b$, le rayon carré vaut $4+s^{2}$ et la puissance de $w$ vaut $-3-2s$. En partant de $s=0$ vers les valeurs négatives, le premier et seul troisième point est atteint à $s=-3/2$. Il devient shell, l'intérieur disparaît et la sphère porte les trois points avec un rang fermé encore égal à $3=r$, **pas** à $r+1$. Cette fixture ne réfute pas la borne supérieure d'attachement ; elle réfute seulement la narration par incréments exactement unitaires.

Enfin, pour $n=2$ ou $n=3$, order_k_vertices renvoie vide et le harvest live n'émet respectivement ni la paire ni le triangle, alors que ces événements existent. Les dimensions affines basses doivent avoir une voie directe ou un contrat de rejet explicite ; elles ne peuvent pas être couvertes par la connectivité des sommets.

L'[audit dynamique du catalogue live](AUDIT_ORDER_K_CATALOGUE_CF9374.md) reproduit séparément ces deux cas de base, quatre supports manquants sur la fixture coplanaire RelevantGP et un extra-shell utile omis silencieusement. Ces échecs d'implémentation sont compatibles avec le lemme conditionnel : chacune de ses hypothèses doit être vérifiée avant de l'utiliser comme certificat.

## 6. Falsification exacte indépendante

Un énumérateur rationnel créé uniquement sous /tmp a :

1. rejeté toute collinéarité de triple, coplanarité de quadruplet et cosphéricité de quintuplet ;
2. calculé chaque sphère de quadruplet par élimination de Gauss sur Fraction ;
3. compté son niveau par distances rationnelles exactes ;
4. ordonné exactement tous les événements de chaque pinceau de triangle ;
5. relié seulement les quadruplets consécutifs ;
6. vérifié la connectivité pour chaque seuil $0\leq k\leq\max\ell$.

Résultat :

| $n$ | nuages génériques décidés | contre-exemples |
| ---: | ---: | ---: |
| 5 | 2 935 | 0 |
| 6 | 2 839 | 0 |
| 7 | 2 670 | 0 |
| 8 | 2 356 | 0 |
| **total** | **10 800** | **0** |

Cette campagne est cohérente avec le théorème. Elle n'en est pas la preuve et ne couvre volontairement ni les témoins constants, ni les multiplicités, ni RelevantGP au sens public.

## 7. Décision d'audit

La voie correcte consiste à enregistrer deux résultats séparés :

1. **Connectivité du vrai 1-squelette shallow : prouvée.** Un seul germe de niveau zéro suffit pour les sommets, et aucune excursion au-dessus de $k$ n'est nécessaire.
2. **Complétude de order_k_catalogue dans le domaine public : non prouvée et actuellement fausse.** Le germe coplanaire, les constantes de pinceau, les multiplicités permises par RelevantGP, les petites dimensions, le shell, le bon centrage et le coût restent des portes indépendantes.

La preuve retire un risque mathématique réel du parcours. Elle ne retire ni le NO-GO produit de [AUDIT_ORDER_K_BFS_A8111F0.md](AUDIT_ORDER_K_BFS_A8111F0.md), ni le facteur $\Theta(nV)$, ni l'obligation architecturale de ne pas matérialiser une mosaïque d'ordre supérieur.
