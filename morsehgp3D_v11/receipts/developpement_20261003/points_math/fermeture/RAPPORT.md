# La fermeture qualifiée de l'auditeur : jugement mathématique

3 octobre 2026, 21 h 45 UTC (heure lue par `date -u`). Agent « fermeture » du workflow « meilleure méthode
mathématique » ; écrit seulement sous `build/v11-points-math/fermeture/`.

```text
phase=exploration_v11_hors_registre
backend=cpu_reference
profile=quantized_u18_input_only
public_status=not_claimed
GCP non utilisé. Aucune commande git, aucune construction native, worktree lu seulement (aucun .pyc écrit).
```

Sources jugées : `qualified_proof/README.md` de l'auditeur, la section « FULL → points » de
`AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md`, `docs/HIERARCHIE_POINTS.md` du développeur (version relue
après sa modification de 21 h), `docs/MATHEMATIQUES.md` § 7, les chapitres 5 à 7 et 9 de la thèse (texte extrait),
le verdict v10 et les réponses de l'utilisateur. Empreintes des fichiers lus ou importés : `SOURCES_LUES.sha256`.
L'oracle exact utilisé est l'étage A de `reference/hgp11_ref` (graphe $\Gamma_k$ par force brute, fractions) et la
route oracle des règles fidèles de `bench/points_reference.py` ; tout le reste est écrit ici
(`lib_fermeture.py`). Calcul total : environ 9 min 30 s de CPU local.

## 0. En bref

1. **Les cinq garanties de l'auditeur sont exactes**, preuves complètes au § 2 : laminarité, équivariance,
   stabilité $1\varepsilon$ en rayon, optimalité minmax, identité $(k,m=k+1)=(k+1,m=k+1)$. Contrôle exact sur
   450 nuages (7 858 triplets nuage–ordre–seuil) : 0 écart, sept mutants tués. Stabilité mesurée : pire rapport
   $0{,}964\,\varepsilon$, 0 violation exacte sur 147 062 comparaisons ; H_m atteint $2{,}68\,\varepsilon$.
2. **Ce que la fermeture est vraiment (nouveau, prouvé).** Pour $k\geq 2$ et $m\leq k+1$, elle ne lit pas la
   connexité de FULL : c'est la liaison simple de $w_{k'}(i,j)=\min\lbrace\beta(F):|F|=k',\ i,j\in F\rbrace$ avec
   $k'=\max(k,m)$ (T1). Elle est encadrée par HDBSCAN : $\sqrt{u}\leq u_{mr}\leq 2\sqrt{u}$ (T2), et par FULL :
   chaque bloc au rayon $r$ tient dans **une** couverture FULL au rayon $2r$ (T3). Les règles fidèles vivent de
   l'autre côté des échéances : $w\leq u_F\leq 4\max(e_i,e_j,w)$ (T4).
3. **La critique du développeur est juste sur le fond, mais bornée et en partie excessive.** La fermeture réunit
   bien deux amas entiers avant FULL : vallée (rayon 50 contre 100), contact, et deux cibles de l'utilisateur
   (Q3 : 700,5 contre 796,1 ; Q4 : 1 078,2 contre 1 334,2). Sur nuages aléatoires « deux amas + vallée », c'est
   le cas de 300 nuages sur 300 à $m=k+1$. Mais l'avance ne dépasse **jamais un facteur 2 en rayon**, et ce
   facteur est atteint (T3). « La fraction récupérable ne peut que baisser » est **faux à $n$ fini** : une fixture
   exacte donne 1 à la fermeture contre 1/2 à FULL. L'énoncé asymptotique reste une conjecture conditionnelle (§ 4.5).
4. **Il n'existe pas de variante extérieure fidèle au sens strict** (T5). On peut restreindre la fermeture aux
   parts exclusives (EC) ou aux intérieurs (core). Ces deux règles sont fidèles, mais EC est discontinue
   (1 591 $\varepsilon$) et core tardive. Datée à $2r$, la fermeture devient faiblement fidèle, mais c'est la
   même hiérarchie. H_m, de son côté, a un défaut que la note du développeur ne relève pas (elle signale seulement
   que la constante de H3 dépend de l'échelle) : un rival lointain retarde une entrée sans borne (×10 puis ×13,9
   en rayon, fixtures exactes, y compris pour la règle retenue H_3).
5. **Ce qui décide vraiment des cibles de l'utilisateur, c'est $m$, pas fermeture contre pendaison.** Avec
   $m=k+1$, la fermeture et H_{k+1} rendent Q1 (les deux triangles) et échouent Q2, Q3 et Q4. H_1 rend Q2, Q3
   et Q4 et échoue Q1.
6. **Verdict.** La fermeture qualifiée n'est pas la bonne projection principale de la tour : pour $m\leq k+1$,
   c'est un HDBSCAN géométrique à facteur 2 près. Elle n'utilise pas la connexité d'ordre supérieur, qui fait
   la valeur de FULL. Elle mérite en revanche une place précise : **enveloppe extérieure canonique et certificat**
   ($u\leq w\leq 4u$, tout candidat fidèle au-dessus de $w$), témoin de stabilité et détecteur exact
   d'ambiguïté. Il faut aussi l'apparier à H_{k+1} sur les mêmes scènes, ce qui n'a pas été fait.

## 1. Objet et notations

$X=\lbrace x_1,\dots,x_n\rbrace$ sites distincts, niveaux $a=r^2$ (rayon carré), coupes fermées. Pour une
composante $C$ de $L_k(a)$, l'amas discret (déf. 8 de la thèse) est $E_C(a)=\lbrace i : d(x_i,C)^2\leq a\rbrace$.
Le théorème 2 l'identifie à la réunion des $k$-parties de la composante de $\Gamma_k(a)$ ; c'est le masque
`coverage` des coupes de l'oracle. Une couverture est **qualifiée** si $|E_C(a)|\geq m$.

- **Fermeture** $\Pi_{k,m}(a)$ : partition la plus fine de l'ensemble actif (sites d'au moins une couverture
  qualifiée) dont chaque bloc contient chaque couverture qualifiée qu'il touche. Hauteur de réunion $u(i,j)$ ;
  entrée $e_i=u(i,i)$.
- **Échéances** $w(i,j)$ : premier niveau où une même couverture qualifiée contient $i$ et $j$ ($w(i,i)=e_i$).
- **Pendaison fidèle** (développeur, F1) : chaque site choisit un point $(o_i,e_i)$ de sa région de couverture
  qualifiée $R_i^{(m)}$, puis suit les ancêtres de $o_i$. Règles de cette famille : core, cover, first, H_1 =
  `margin1`, H_m = `margin`, et EC (§ 5).
- $A_k(x)$ : première couverture (P2). $D_k(x)$ : carré de la distance au $k$-ième voisin, le site compté (P1).
  $\rho_k(z)$ : distance d'un point quelconque $z$ à son $k$-ième site.

**Fait 0 (monotonie).** Pour $a\leq b$, toute composante $C$ de $L_k(a)$ est incluse dans une unique composante
$C'$ de $L_k(b)$, et $E_C(a)\subseteq E_{C'}(b)$ ; la qualification passe donc de $a$ à $b$.
*Preuve.* $L_k(a)\subseteq L_k(b)$ et $C$ est connexe ; si $d(x,C)^2\leq a$ alors $d(x,C')^2\leq a\leq b$. □

## 2. Les garanties de l'auditeur : vérification

### 2.1 Laminarité

**Énoncé.** Pour $a\leq b$, $\Pi_{k,m}(a)$ raffine $\Pi_{k,m}(b)$ et l'ensemble actif croît ; $u$ est une
ultramétrique de diagonale $e$. *Preuve.* Par le fait 0, chaque hyperarête de niveau $a$ est incluse dans une
hyperarête de niveau $b$ : les composantes de l'hypergraphe ne peuvent que se réunir. Si $i,l$ et $l,j$ sont
réunis au niveau $\max(u(i,l),u(l,j))$, alors $i$ et $j$ le sont aussi. □

Contrôle G1 : la partition recalculée coupe par coupe, sans mémoire, raffine la suivante et égale la partition
cumulée. 7 858 triplets, 0 écart.

### 2.2 Équivariance

Une permutation des identifiants permute les couvertures ; une isométrie $g$ envoie $L_k^X(a)$ sur $L_k^{gX}(a)$
avec les mêmes couvertures ; une homothétie de rapport $s$ multiplie les rayons par $|s|$. Aucun ordre, aucun
support, aucun axe n'intervient. □ Contrôle G9 (permutation aléatoire et isométrie entière, rotation, réflexion,
translation) : 2 643 contrôles, 0 écart.

### 2.3 Stabilité $1\varepsilon$ en rayon

**Énoncé.** Si $\Vert x_i-y_i\Vert\leq\varepsilon$ pour tout $i$, alors
$|\sqrt{u^X(i,j)}-\sqrt{u^Y(i,j)}|\leq\varepsilon$ pour tous $i,j$, diagonale comprise.
*Preuve.* D'après P5, $\rho_k^Y\leq\rho_k^X+\varepsilon$, donc une composante $C$ de $L_k^X(r^2)$ est incluse
dans une composante $C'$ de $L_k^Y((r+\varepsilon)^2)$. Si $z\in C$ et $\Vert x_i-z\Vert\leq r$, alors $z\in C'$ et
$\Vert y_i-z\Vert\leq r+\varepsilon$ ; d'où $E_C^X(r^2)\subseteq E_{C'}^Y((r+\varepsilon)^2)$, et la qualification
est conservée (mêmes identifiants, cardinal croissant). Toute hyperarête de $X$ au rayon $r$ est donc incluse
dans une hyperarête de $Y$ au rayon $r+\varepsilon$ : $\Pi^X(r^2)$ raffine $\Pi^Y((r+\varepsilon)^2)$. On
conclut par symétrie. □

**Mesure** (`verif_stabilite.py`, 600 nuages perturbés de $\lbrace -1,0,1\rbrace^3$ par site, $k\leq 3$,
$m\in\lbrace 1,k+1,k+2\rbrace$, 147 062 comparaisons par règle ; $\delta=2\varepsilon\sqrt{\Lambda}+\varepsilon^2$
comme dans H3) :

| Règle | max $\vert\Delta\sqrt{u}\vert/\varepsilon$ | max $\vert\Delta u\vert/\delta$ | paires $>3\varepsilon$ |
| --- | --- | --- | --- |
| fermeture | **0,964** (0 violation exacte de $1\varepsilon$) | 0,859 | 0 |
| H_m (`margin`) | 2,679 | 1,483 | 0 |
| H_1 (`margin1`) | 2,679 | 1,376 | 0 |
| core | 1,928 | 2,824 | 0 |
| first, cover, EC | 1 591 | 1 177 | 452, 1 250, 517 |

Le développeur annonçait 1,46 pour H_m ; je trouve 1,48 sur un autre échantillon, sous sa borne $5\delta$.

### 2.4 Optimalité minmax

**Énoncé.** Pour $i\neq j$, $u(i,j)=\min_{\pi:i\leadsto j}\max_{(l,l')\in\pi}w(l,l')$ ; $u$ est la plus grande
ultramétrique majorée par $w$ hors diagonale ; pour $m\geq 2$, $e_i=\min_{j\neq i}w(i,j)$.
*Preuve.* Si un chemin a ses échéances au plus $a$, chaque couple consécutif est couvert par une hyperarête de
niveau au plus $a$, incluse dans une hyperarête de niveau $a$ (fait 0) : $u\leq a$. Réciproquement, une chaîne
d'hyperarêtes de niveau $u(i,j)$ fournit un chemin d'échéances au plus $u(i,j)$. Toute ultramétrique
$v\leq w$ vérifie $v(i,j)\leq\max_\pi v\leq\max_\pi w$ sur tout chemin, donc $v\leq u$, et l'arête directe
donne $u\leq w$. Pour l'entrée : une hyperarête qualifiée ($m\geq 2$) qui contient $i$ contient un autre site. □
Contrôle G2 (Floyd–Warshall minmax en fractions) : 0 écart.

### 2.5 Identité $(k,k+1)=(k+1,k+1)$, et ses limites

**Énoncé.** Pour $k+1\leq n$ et tout $a$ : $\Pi_{k,k+1}(a)=\Pi_{k+1,k+1}(a)$, ensembles actifs compris.
*Preuve.* (⊆) Soit $C$ une composante de $\Gamma_k(a)$ avec $|E_C|\geq k+1$. Elle a au moins deux sommets, car
une seule $k$-partie couvre $k$ sites. Prenons un arbre couvrant de $C$ dans $\Gamma_k(a)$. Chaque arête relie
$F$ et $F'$ par $G=F\cup F'$, avec $|G|=k+1$ et $\beta(G)\leq a$ : c'est un sommet de $\Gamma_{k+1}(a)$, contenu
dans la couverture de sa composante, qualifiée à $m=k+1$. Chaque sommet de $C$ touche une arête de l'arbre, donc
$E_C$ est la réunion de ces $G$. Deux arêtes de l'arbre qui partagent un sommet $F$ partagent les $k\geq 1$ sites
de $F$. L'arbre étant connexe, $E_C$ tient dans un bloc de $\Pi_{k+1,k+1}(a)$.
(⊇) Pour une composante $D$ de $\Gamma_{k+1}(a)$, deux sommets adjacents $G,G'$ partagent une $k$-face commune.
Toutes les $k$-faces des sommets de $D$ sont donc reliées dans $\Gamma_k(a)$, à travers $G$ et $G'$. Elles
tombent dans une même composante $C$, avec $E_D\subseteq E_C$ et $|E_C|\geq k+1$. □

De même $m\leq k$ ne change rien (toute composante couvre au moins $k$ sites). Contrôles : G4, 1 772 triplets,
0 écart ; G3, 4 388, 0 écart. Le mutant M5, l'identité $(k,k+2)=(k+1,k+2)$, échoue sur 1 698 triplets sur 1 698 :
l'auditeur avait raison de la refuser.

### 2.6 Fixtures de l'auditeur, rejouées par une route indépendante

`verif_fixtures_auditeur.py` (oracle de la définition, le `check.py` de l'auditeur n'est pas importé) :
`resultats/fixtures_auditeur.txt`, verdict `conforme`.

- Triangles, ponts 2000, 1998 et 1700, $k=2$ : à $m=3$, ABC | DEF dès $\beta=249978000484/187489$, puis réunion
  à 3 731 956, 3 728 225 et 3 194 656, égale à la fusion FULL ; à $m=2$, racine à $10^6$, 999 956 et 999 956.
- $\lbrace 0,2,100,102\rbrace$ : à $m=3$, les quatre entrées sont à 2 500 ; à $m=2$, réunion à 2 401 contre une
  fusion FULL à 2 500.
- Cinq points : à $m=3$, réunion à $100/9$ contre une fusion FULL à 36 ; core du point 0 à 40 ; avec H_3, au
  niveau 35, les blocs sont {1,2} et {3,4}, et l'entrée du point 0 est à 36.

### 2.7 Tableau des contrôles exacts

`verif_garanties.py --clouds 450 --seed 20261003` : 150 nuages `gate` (ex æquo fréquents), 150 `generic`, 150
`two_blobs` ; $n\leq 9$, $k\leq 4$, $m$ de 1 à $k+2$. Durée 138,7 s ; verdict `conforme`
(`resultats/garanties.txt`).

| Contrôle | Objet | Contrôles | Écarts |
| --- | --- | --- | --- |
| G1–G4, G9 | § 2.1–2.5 | 7 858 / 4 388 / 1 772 / 2 643 | 0 |
| G5 | T1, forme directe | 5 710 | 0 |
| G6 | T2, encadrement HDBSCAN | 5 710 | 0 |
| G10, G11 | T3 : $w\leq 4u$ ; bloc au niveau $a$ dans une couverture au niveau $4a$ | 7 858 + 7 858 | 0 |
| G7, G12 | T4, pour core, cover, first, H_1, H_m, EC | 6 × 7 858 + 6 × 7 858 | 0 |
| G8 | EC : chaque bloc dans une couverture vivante | 7 858 | 0 |
| M1 | T1 avec le mauvais ordre $k'=k$ | 1 322 | 1 322 (tué) |
| M2 | fermeture $\geq w$ (soit $w$ ultramétrique) | 7 858 | 5 014 (tué) |
| M3 | encadrement HDBSCAN avec facteur 1 | 5 710 | 5 710 (tué) |
| M4 | fermeture fidèle (bloc dans une couverture) | 7 858 | 5 018 (tué) |
| M5 | identité $(k,k+2)=(k+1,k+2)$ | 1 698 | 1 698 (tué) |
| M6 | T3 avec 1,9 au lieu de 2 | 7 858 | 58 (tué : facteur 2 presque atteint) |
| M7 | T4 avec 3 au lieu de 4, pour first | 7 858 | 351 (tué) |

Diagnostic de T4, $\max u_F/\max(e_i,e_j,w)$ : first et cover 3,887 ; core 2,25 ; H_1 et H_m 1,838 (borne 4).
Lecture de M2 et M4 : sur ces petits nuages, $w$ n'est pas une ultramétrique dans 5 014 triplets sur 7 858. Dans
5 018, un bloc de la fermeture ne tient dans aucune couverture vivante. Comme la fermeture est la plus fine
partition extérieure, **aucune** partition extérieure n'est alors faiblement fidèle (§ 3.5). L'ambiguïté est la
règle, pas l'exception (environ 64 %).

## 3. Ce que la fermeture est vraiment

### 3.1 T1 — Pour $m\leq k+1$, la fermeture ignore la connexité de FULL

**Théorème T1.** Soit $k\geq 2$ et $m\leq k$. Les blocs de $\Pi_{k,m}(a)$ sont les composantes de l'hypergraphe
des $k$-parties $F$ telles que $\beta(F)\leq a$, deux hyperarêtes étant reliées si elles partagent un site. Donc
$u_{k,m}$ est la liaison simple de $w_k(i,j)=\min\lbrace\beta(F):|F|=k,\ i,j\in F\rbrace$, et $e_i=A_k(x_i)$. Avec
2.5, $\Pi_{k,k+1}=\Pi_{k+1,1}$ : pour tout $m\leq k+1$ tel que $k'=\max(k,m)\geq 2$, la fermeture est la
liaison simple de $w_{k'}$ (cas $k=1$, $m=2$ compris).
*Preuve.* $E_C(a)$ est la réunion des $k$-parties de la composante (théorème 2). Deux sommets adjacents de
$\Gamma_k(a)$ partagent $k-1\geq 1$ sites : chaque $E_C$ est une réunion connexe dans l'hypergraphe des
$k$-parties. Inversement, chaque $k$-partie de niveau au plus $a$ est un sommet d'une composante, donc incluse dans
son $E_C$. Les deux familles engendrent la même fermeture. Deux sites sont reliés au niveau $a$ si et seulement si
une chaîne de $k$-parties de niveau au plus $a$ les relie, c'est-à-dire si et seulement si un chemin a toutes ses
valeurs $w_k$ au plus $a$. Un site est actif si et seulement si une $k$-partie de niveau au plus $a$ le contient,
soit $A_k(x)\leq a$. □

Conséquence. Pour $m\leq k+1$, la fermeture ne lit **que** les rayons des plus petites boules des $k'$-parties,
et les recolle par un seul site commun. FULL_k, lui, recolle deux $k$-parties par une $(k-1)$-face commune portée
par une $(k+1)$-partie de niveau assez bas. À $m=k+1$, la fermeture lit les mêmes nombres que les arêtes de
$\Gamma_k$, mais pas la manière dont elles se recollent. La connexité d'ordre supérieur (déf. 22, théorème 2) n'y
joue aucun rôle. Le « K2/m3 » de l'auditeur est l'objet d'ordre 3 sans sa connexité, c'est-à-dire $HL_3$
(liaison simple de $w_3$). Le développeur avait vu
que $(k,k+1)$ n'apporte rien de plus que l'ordre $k+1$ ; T1 montre plus : la fermeture n'apporte rien de la
**connexité** de FULL, à aucun ordre, tant que $m\leq k+1$. C'est exactement l'asymétrie que le chapitre 6
reproche au Robust Single-Linkage et à HDBSCAN : une contrainte forte sur les « points » (ici des $k'$-parties
de petite boule) et une connexion lâche (un seul site partagé).

### 3.2 T2 — Encadrement par HDBSCAN

**Théorème T2.** Soit $k'\geq 2$ et $d_{mr}(i,j)=\max(\rho_{k'}(x_i),\rho_{k'}(x_j),\Vert x_i-x_j\Vert)$,
l'atteignabilité mutuelle avec la convention de P1 (le site compte parmi ses voisins). Alors
$\sqrt{w_{k'}}\leq d_{mr}\leq 2\sqrt{w_{k'}}$ et $\sqrt{A_{k'}}\leq\rho_{k'}\leq 2\sqrt{A_{k'}}$. Si
$u_{mr}$ désigne la liaison simple de $d_{mr}$, on a donc $\sqrt{u}\leq u_{mr}\leq 2\sqrt{u}$ coefficient par
coefficient, et les blocs vérifient $\mathrm{SL}_{mr}(r)\subseteq\Pi(r^2)\subseteq\mathrm{SL}_{mr}(2r)$.
*Preuve.* $\sqrt{w(i,j)}=\min_z\max(\Vert z-x_i\Vert,\Vert z-x_j\Vert,\rho(z))$ : une boule $B(z,s)$ qui contient
$x_i$, $x_j$ et au moins $k'$ sites contient une $k'$-partie contenant $i$ et $j$ ; réciproquement, le centre de
la plus petite boule d'une telle partie convient. En prenant $z=x_i$, on obtient
$\sqrt{w}\leq\max(\Vert x_i-x_j\Vert,\rho(x_i))\leq d_{mr}$. Au point optimal $z$, de valeur $s$, on a
$\Vert x_i-x_j\Vert\leq 2s$, et $\rho(x_i)\leq\Vert x_i-z\Vert+\rho(z)\leq 2s$, car les $k'$ sites de
$B(z,\rho(z))$ sont à moins de $\Vert x_i-z\Vert+\rho(z)$ de $x_i$. Les entrées suivent de P2. Enfin, la liaison
simple est croissante et homogène. □ Contrôle G6 : 5 710, 0 écart ; le mutant à facteur 1 est tué 5 710 fois.

T2 est écrit avec le site compté parmi ses $k'$ voisins (convention de P1). Avec une convention qui l'exclut,
remplacer $k'$ par $k'-1$ côté HDBSCAN. Avec la convention de la thèse (niveau HDBSCAN = distance de liaison / 2),
la partition de la fermeture au rayon $r$ est plus fine que celle de HDBSCAN au rayon $r$ et plus grossière que
celle de HDBSCAN au rayon $r/2$.

### 3.3 Lemme S et T3 — Encadrement par FULL, facteur 2 exact

**Lemme S.** Si deux composantes $C,C'$ de $L_k(r^2)$ sont toutes deux à distance au plus $r$ d'un même site
$x$, elles sont dans la même composante de $L_k((2r)^2)$.
*Preuve.* Prenons $z\in C$ et $z'\in C'$ à distance au plus $r$ de $x$. Tout $p\in[z,x]$ vérifie
$\Vert p-z\Vert\leq r$, donc $B(p,2r)\supseteq B(z,r)$ contient $k$ sites ; de même sur $[x,z']$. La ligne
brisée $z\to x\to z'$ est dans $L_k((2r)^2)$. □

**Théorème T3.** Pour tous $k$, $m$ et $r$, chaque couverture qualifiée au rayon $r$ tient dans un bloc de
$\Pi_{k,m}(r^2)$. Inversement, chaque bloc de $\Pi_{k,m}(r^2)$ tient dans la couverture d'**une seule**
composante de $L_k((2r)^2)$. En particulier $w\leq 4u$ hors diagonale, et si la fermeture réunit au rayon $r$ les
couvertures de deux composantes, FULL fusionne ces composantes au plus tard au rayon $2r$. La constante est
atteinte.
*Preuve.* Par le fait 0, un bloc au niveau $r^2$ est la réunion d'une chaîne de couvertures qualifiées du même
niveau, deux consécutives partageant un site. Le lemme S place deux composantes consécutives dans une même
composante au rayon $2r$, donc toutes dans une composante $C^{\ast}$. On a $E_{C_t}(r^2)\subseteq E_{C^{\ast}}((2r)^2)$,
et $C^{\ast}$ est qualifiée. Si $i$ et $j$ sont dans un même bloc au niveau $u(i,j)$, ils sont donc co-couverts
au niveau $4u(i,j)$. □ La finesse vient de la vallée à $m\leq 2$ (§ 4.2) : la fermeture réunit à $r=50$, FULL
fusionne à $r=100$ exactement. Contrôles G10 et G11 : 0 écart ; le mutant à 1,9 est tué 58 fois.

**Lecture.** Au même rayon, la fermeture est plus grossière que FULL. Datée au double du rayon, elle devient plus
fine que FULL : c'est un **entrelacement de facteur 2**.

### 3.4 T4 — Dualité : la fermeture sous $w$, les pendaisons fidèles au-dessus

**Théorème T4.** Pour toute pendaison fidèle de seuil $m$ (avec $w$ non qualifié pour core, cover et H_1) :
$w(i,j)\leq u_F(i,j)\leq 4\max(e_i,e_j,w(i,j))$.
*Preuve.* (≥) Au niveau $u_F(i,j)$, l'ancêtre vivant commun de $o_i$ et $o_j$ couvre $i$ et $j$, car $R_i$ est
stable par remontée, et il est qualifié. (≤) Posons $a=\max(e_i,e_j,w(i,j))$. Au niveau $a$, l'ancêtre vivant
$A_i$ de $o_i$ couvre $x_i$, l'ancêtre vivant $A_j$ de $o_j$ couvre $x_j$, et une composante vivante $C$ couvre
les deux. Le lemme S, appliqué en $x_i$ puis en $x_j$, réunit $A_i$, $C$ et $A_j$ au niveau $4a$. □

On obtient ainsi une chaîne exacte, en rayon et pour $m\leq k$ :
$u_{HDB}^{\text{thèse}}\leq\sqrt{u_{CL}}\leq\sqrt{w}\leq\sqrt{u_F}$, avec chaque fois au plus un facteur 2, sauf pour
les retards d'entrée des règles fidèles. La fermeture est l'ultramétrique **sous-dominante** de $w$ : unique, car
le maximum de deux ultramétriques en est une. En général, il n'existe pas de plus petite ultramétrique au-dessus
de $w$ : le minimum de deux ultramétriques n'en est pas une. C'est pour cela que la direction intérieure n'a pas
d'objet canonique. Sur $\lbrace 0,2,4\rbrace$ à $k=2$, les deux ultramétriques minimales au-dessus de $w$ sont échangées
par la réflexion. Une règle équivariante doit passer au-dessus des deux, donc retarder le site médian (P4, F2).

### 3.5 T5 — Pas d'extérieur fidèle

**Proposition T5.** Supposons qu'à un niveau $a$, deux composantes vivantes qualifiées distinctes $C$ et $C'$
aient des couvertures qui se rencontrent, et qu'aucune composante vivante ne couvre $E_C(a)\cup E_{C'}(a)$. Alors
aucune partition des sites actifs n'est à la fois **extérieure** (chaque couverture qualifiée dans un bloc) et
**faiblement fidèle** (chaque bloc inclus dans une couverture vivante). A fortiori, aucune pendaison fidèle n'est
extérieure à ce niveau, puisque ses blocs sont dans les couvertures de leurs nœuds (F1).
*Preuve.* Le site commun force $E_C\cup E_{C'}$ dans un seul bloc ; la fidélité faible exigerait une couverture
vivante qui le contienne. □ Exemple : les cinq points au niveau $100/9$. Plus généralement, une partition
extérieure est moins fine que la fermeture. Il n'existe donc une partition extérieure faiblement fidèle à un niveau
que si les blocs de la fermeture tiennent chacun dans une couverture : c'est faux dans 5 018 triplets aléatoires sur
7 858 (M4).

## 4. La critique du développeur, mise à l'épreuve (chapitre 7)

### 4.1 Définitions opérationnelles

Pour chaque fusion FULL $\nu$ (niveau $h$) et chaque paire d'enfants $s$, $t$, l'**anticipation** est le premier
niveau de coupe $U<h$ où la fermeture place dans un même bloc des couvertures qualifiées des deux lignées. Les
couvertures de lignée $S(a)$ et $T(a)$ sont les réunions des couvertures des nœuds vivants des sous-arbres. On pose
$\mu=\max_{a\in[U,h)}\min(|S\setminus T|,|T\setminus S|)$ et $\theta=\max(m,k+1)$.

- **absorption** si $\mu=0$ : une lignée reste incluse dans l'autre, et aucune paire de sites exclusifs n'est
  réunie ;
- **réunion d'un site partagé** si $1\leq\mu<\theta$ : la réunion passe par des sites partagés et joint des sites
  exclusifs, mais l'un des côtés n'a jamais $\theta$ sites propres avant la fusion FULL (franges de contact,
  cinq points) ;
- **réunion de deux amas entiers** si $\mu\geq\theta$ : les deux lignées ont chacune au moins $\theta$ sites
  propres à un même niveau avant la fusion FULL, et la fermeture les a déjà réunies.

On distingue aussi « direct » (couvertures sécantes en $U$) et « par chaîne » (à travers une troisième lignée).
Code : `lib_fermeture.parasitic_events`.

### 4.2 Familles exactes

`familles.py` produit `resultats/familles.txt`. Rayons ; rapport = $r_U/r_h$ pour la réunion des deux amas
désignés.

| Famille ($k$) | $m$ | Fermeture : réunion A–B | FULL | Rapport | Type |
| --- | --- | --- | --- | --- | --- |
| vallée, triangles d=100, t=10 (2) | 1, 2 | 50,00 | 100,00 | **0,500** | deux amas entiers, 1 site partagé, exclusifs (3, 3) |
| idem | 3 | 55,23 | 100,00 | 0,552 | deux amas entiers |
| idem t=30 / t=50 | 3 | 66,71 / 79,06 | 100,00 | 0,667 / 0,791 | deux amas entiers |
| vallée 3D, amas de 4 (2) | 3, 4 | 60,83 | 100,00 | 0,608 | deux amas entiers (4, 4) |
| vallée 3D, amas de 4 (3) | 3 / 4 | 60,83 / 61,24 | 110,45 | 0,551 / 0,554 | deux amas entiers (4, 4) |
| contact en un site, L=12, z=0/2/5 (2) | 2 | 6,08 / 6,16 / 6,58 | 12,00 / 12,04 / 12,26 | 0,507 / 0,512 / 0,537 | site partagé (2, 2) |
| idem | 3 | 6,17 / 6,25 / 6,65 | idem | 0,514 / 0,519 / 0,543 | site partagé |
| cinq points de l'auditeur (2) | 2 / 3 | 3,16 / 3,33 | 6,00 | 0,527 / 0,556 | site partagé (2, 2) |
| Q3 filament contre amas (2) | 3 | 700,50 | 796,12 | 0,880 | deux amas entiers, exclusifs (8, 5) |
| Q4 deux tétraèdres (3) | 4 | 1 078,17 | 1 334,17 | 0,808 | deux amas entiers, exclusifs (4, 4) |

H_m, H_1, EC, first et core ne réunissent jamais avant FULL (fidélité, T4). Quand les deux amas ne deviennent
qualifiés qu'au niveau même de leur réunion, la fermeture ne montre jamais A seul (fraction 0). C'est le cas des
triangles à $m=4$, et des cinq points et du contact symétrique à $m=3$.

### 4.3 Fréquences sur nuages aléatoires

`frequences.py --clouds 900 --seed 11` (30,5 s) : `resultats/frequences.txt`. Nombre de nuages présentant au
moins un événement, sur 300 par générateur.

| Générateur, k, m | anticipation | site partagé | deux amas entiers | rapport rayon des « deux amas » (min / médiane) |
| --- | --- | --- | --- | --- |
| gate, 2, 1 | 300 | 300 | 30 | 0,500 / 0,765 |
| gate, 2, 3 | 100 | 79 | 25 | 0,694 / 0,904 |
| gate, 3, 4 | 66 | 65 | 1 | 0,908 |
| generic, 2, 1 | 300 | 300 | 45 | 0,570 / 0,774 |
| generic, 2, 3 | 171 | 142 | 40 | 0,589 / 0,906 |
| generic, 3, 4 | 120 | 119 | 0 | — |
| two_blobs, 2, 1 | 300 | 300 | 295 | 0,502 / 0,748 |
| two_blobs, 2, 3 | 300 | 10 | **295** | 0,530 / 0,814 |
| two_blobs, 2, 4 | 295 | 176 | 119 | 0,566 / 0,847 |
| two_blobs, 3, 1 | 300 | 300 | 118 | 0,517 / 0,697 |
| two_blobs, 3, 4 | 298 | 184 | 115 | 0,525 / 0,724 |
| two_blobs, 3, 5 | 225 | 225 | 0 | — |

Sur two_blobs, à $k=2$ et $m=3$, la fermeture réunit les germes de A et B **avant** FULL dans 300 nuages sur 300
(rapport médian 0,817). C'est encore le cas dans 300 sur 300 à $(3,4)$ et 241 sur 300 à $(3,5)$. Le minimum
observé, 0,5015, est conforme à T3. Ce sont des fréquences d'oracle sur $n\leq 9$ : elles illustrent un
mécanisme, elles n'établissent aucune fréquence ni pente à $n=8000$.

### 4.4 Ampleur : une avance bornée et atteinte

La critique est juste dans sa direction. La fermeture crée bien des fusions parasites avant FULL, et l'événement
« deux amas entiers » est générique dès qu'une vallée ou un contact porte un site couvert par les deux amas.
Le seuil $m$ n'y change rien quand les amas ont eux-mêmes au moins $m$ sites. Mais l'avance est **bornée** par T3 :
jamais plus d'un facteur 2 en rayon (4 en niveau carré). La borne est atteinte. Ni le développeur ni l'auditeur
ne l'avaient établie : elle donne la mesure exacte du défaut.

### 4.5 Fraction récupérable : la phrase « ne peut que baisser » est fausse à $n$ fini

- **Fermeture moins bonne** (vallée + frange, $k=2$, $m=3$) : réunion à 55,23 avec 3/4 de A, contre FULL à 100
  avec 1 (H_3, EC et first : 1 ; core : 3/4).
- **Fermeture meilleure** (amas scindé par un pont mince interne, au contact épais de B ; $d=200$, $t=20$,
  $g=180$) : la fermeture réunit A et B à 91,24 avec **1** de A, contre FULL à 110,11 avec **1/2** (H_3, EC et
  first : 1/2 ; H_1 et core : 1/3). Avec $g=140$ : 71,59 et 1, contre 110,11 et 1/2. La fermeture réunit plus tôt
  mais a déjà recousu A à travers le pont mince, ce que FULL ne fait qu'après avoir absorbé B.

À $n$ fini, il n'y a donc pas de monotonie. Côté asymptotique, dans le cadre du théorème 3 de la thèse :

- **Prouvé.** $\Theta^{CL}(\lambda)\geq\Theta^{poly}(\lambda)$, car les couvertures FULL sont dans les blocs ; et
  $\lambda_c^{CL}\leq\lambda_c^{poly}\leq 2^p\lambda_c^{CL}$. La première inégalité vient de la même inclusion. La
  seconde vient de T3 : un bloc infini au rayon $1/2$ tient dans une composante de $L_K$ au rayon 1, qui couvre une
  infinité de sites et est donc non bornée ; on applique ensuite le changement d'échelle
  $\Theta_{K,p,r}(\lambda)=\Theta_{K,p,1/2}(\lambda(2r)^p)$ du chapitre 7.
- **Conséquence.** La fraction du théorème 3, $\Theta^{cc}(\rho\lambda_c^{cc})$, n'est **pas** ordonnée par ces
  inégalités. La fermeture a un seuil plus bas, mais une $\Theta$ plus haute.
- **Conjecture C1.** $\lambda_c^{CL}<\lambda_c^{poly}$ pour $K\geq 2$ et $p\geq 2$. Relier deux composantes de
  $L_K$ par un site couvert par les deux est un renforcement essentiel au sens d'Aizenman–Grimmett. Pour $K=1$,
  les couvertures sont disjointes et il y a égalité.
- **Conjecture C2** (heuristique, sous C1). À fort contraste $\rho=\rho_1/\rho_0$, les deux fractions non
  récupérées sont dominées par le même événement local : « 0 n'est couvert par aucune boule de rayon $1/2$
  contenant $K$ sites ». Leur rapport croît alors comme $\exp(c\rho(\lambda_c^{poly}-\lambda_c^{CL}))$, et la
  fermeture perd exponentiellement en $\rho$. La thèse a mesuré des vitesses de percolation pour poly, core et
  dbscan, pas pour cette famille : c'est la mesure qui trancherait.

### 4.6 Bilan sur la critique

| Point du développeur | Jugement |
| --- | --- |
| 1. réunit ce que FULL sépare ; fusion parasite avant FULL | **juste**, fréquent, borné par un facteur 2 en rayon (T3) |
| 1 bis. « la fraction récupérable ne peut que baisser » | **réfuté** à $n$ fini (fixture « amas scindé ») ; asymptotiquement, conjecture conditionnelle C1–C2 |
| 1 ter. « défaut même de la liaison simple et de HDBSCAN » | **juste et précisé** : T1 et T2 (liaison simple d'une dissimilarité à 2 près de l'atteignabilité mutuelle) |
| 2. optimalité relative à $w$ | juste ; T4 donne le dual exact (fidèle ⟺ au-dessus de $w$) |
| 3. identité $(k,k+1)=(k+1,k+1)$ | juste ; T1 la renforce (aucune connexité de FULL pour $m\leq k+1$) |
| 4. sous la première couverture dans la campagne de l'auditeur | chiffres de l'auditeur, non rejoués ici ; la comparaison porte sur first, pas sur H_{k+1} ; il manque toujours l'appariement fermeture contre H_{k+1} sur les mêmes scènes |

## 5. Existe-t-il une variante extérieure fidèle ?

### 5.1 Non au sens strict (T5)

Dès que des couvertures qualifiées vivantes se chevauchent sans qu'une même composante les couvre toutes,
extérieur et fidèle s'excluent. Le trilemme F2 du développeur devient un quadrilemme sur {laminaire, fidèle,
continu, entrée à la première couverture} : chaque règle connue abandonne exactement un terme.

| Règle | Fidèle | Continue | Entrée à la 1re couverture | Ce qu'elle abandonne |
| --- | --- | --- | --- | --- |
| fermeture | non (T5) | oui, $1\varepsilon$ | oui | la fidélité, avec une avance d'au plus 2× (T3) |
| EC (ci-dessous), first | oui | **non** (1 591 $\varepsilon$) | presque (ex æquo seulement) | la continuité |
| H_m | oui | oui, $2{,}68\varepsilon$ mesuré | non (retard non borné, § 5.4) | les entrées précoces |
| core | oui | oui, $1{,}93\varepsilon$ mesuré (borne $2\varepsilon$ énoncée par l'auditeur) | non, mais $D_k\leq 4A_k$ (facteur 2) | l'objet de la déf. 8 (cœur au lieu de couverture) |

### 5.2 La fermeture restreinte à l'intérieur de chaque composante

Deux lectures naturelles :

- **Restreinte aux intérieurs** $C\cap X$ : les intérieurs de composantes distinctes sont disjoints, et la fermeture
  rend exactement **core** (P1).
- **Restreinte aux parts exclusives (EC)** : un site entre au premier niveau où **une seule** composante vivante
  qualifiée le couvre, dans cette composante, puis suit ses ancêtres. C'est une pendaison fidèle sans aucun
  choix : laminaire (F1), équivariante, avec $u\geq w$ (T4, G7, G8 : 0 écart). On peut aussi la lire comme une
  fermeture. Si $i\in X_C(a)$ et $i\in X_{C'}(a')$ avec $a\leq a'$, la lignée de $C$ couvre encore $i$ en $a'$,
  donc $C'$ est l'ancêtre de $C$ : des hyperarêtes exclusives de lignées distinctes ne partagent jamais de site.
  EC est **discontinue** : sur $(0,2s,4s)$ puis $(0,2s,4s+1)$, le site médian entre à $4s^2$, puis à $s^2$
  (`rival_lointain.py`, $D=0$ puis $D=1$). Rapport mesuré : 1 591 $\varepsilon$, comme first.

Mesures EC contre H_m (`ec_vs_hm.py`, 600 nuages, 4 394 sites par ligne) :

| $k$, $m$ | sites retardés H_m | sites retardés EC | EC <, =, > H_m | $\sqrt{e/t}$ moyen / max H_m | idem EC |
| --- | --- | --- | --- | --- | --- |
| 2, 1 | 3 872 (88 %) | 232 (5 %) | 3 663, 700, 31 | 1,578 / 10,51 | 1,026 / 2,24 |
| 2, 3 | 468 (10,7 %) | 13 (0,3 %) | 455, 3 939, 0 | 1,047 / 3,32 | 1,001 / 1,41 |
| 3, 1 | 4 047 (92 %) | 166 (3,8 %) | 3 900, 460, 34 | 1,467 / 5,67 | 1,011 / 1,91 |
| 3, 4 | 764 (17,4 %) | 24 (0,5 %) | 741, 3 653, 0 | 1,086 / 3,92 | 1,001 / 1,29 |

EC retarde aussi peu que first, avec le même nombre de sites retardés à chaque ligne, mais paie la continuité.
H_m paie la continuité en retardant 10 à 17 % des sites à $m=k+1$, et presque tous à $m=1$.

### 5.3 La fermeture datée à $2r$ : fidélité faible, même hiérarchie

Si l'on publie au rayon $2r$ le bloc de la fermeture au rayon $r$, chaque bloc publié tient dans **une**
couverture vivante au rayon de publication (T3), et chaque couverture prise à la moitié de ce rayon tient dans un
bloc publié. C'est la seule variante
extérieure canonique et « fidèle à un facteur 2 près ». Elle est $2\varepsilon$-stable et équivariante. Mais
c'est la **même** hiérarchie combinatoire : mêmes blocs, même ordre de fusion. Une sélection EOM est inchangée
par l'homothétie des $\lambda$, et le meilleur IoU l'est aussi. Elle ne répond donc pas à la critique du
chapitre 7.

### 5.4 Comparaison avec H_m : un défaut non relevé

H2 (« sans retard sur les amas non ambigus ») porte sur tout l'avenir de la lignée : un rival **lointain dans le
temps** suffit à retarder un site. Fixtures exactes :

- H_1 sur $(0,2s,4s+D)$, $k=2$. Le site médian entre au niveau $4s^2+sD$ ; la formule est vérifiée exactement.
  Le rival n'apparaît qu'au rayon $s+D/2$. Pour $s=1$ et $D=96$, l'entrée est au rayon **10** contre une première
  couverture à 1 ; rival à 49, fusion à 50. Pour $s=1000$ et $D=96000$, entrée au rayon 10 000 contre 1 000. EC,
  LCA et la fermeture entrent à $s$ (`resultats/rival_lointain.txt`).
- H_3, la règle retenue à $k=2$ : le site $x$ forme un triangle aigu avec deux voisins (qualifié à $r=1{,}25$), et
  un triangle lointain se trouve à l'abscisse $D$. $x$ entre à 2,459, 3,473, 5,662, 10,103 et 17,380 pour
  $D$ = 4, 10, 30, 100 et 300, soit jusqu'à **×13,9**. EC, first et la fermeture entrent à 1,25
  (`resultats/rival_lointain_m3.txt`). Le triangle n'existe comme bloc de 3 qu'à partir de cette entrée tardive.

Le retard d'entrée de H_m n'est donc borné par rien de local. Une marge mesurée en rayon le ramènerait à la durée
de rivalité en rayon (non mesuré ici). Une marge qui décroît avec l'écart $h(q)-t_i$ (le $\kappa>1$ de H4)
supprimerait l'effet, au prix des constantes $(1+2\kappa)\delta$ et $(1+4\kappa)\delta$. La fermeture n'a pas ce défaut : elle
entre toujours à la première couverture qualifiée.

## 6. Cibles de l'utilisateur (Q1–Q4)

`cibles_utilisateur.py` produit `resultats/cibles_utilisateur.txt`. Q3 a 14 sites à $k=2$ : 2 835 essais de
sphères candidates, contre 6 375 pour 9 sites à $k=4$, donc dans le budget de l'oracle borné. Réponses de
l'utilisateur : Q1 (b), Q2 (a), Q3 (a), Q4 (a). Ce sont des préférences, pas un oracle.

| Règle | Q1 ABC \| DEF | Q2 x avec a | Q3 x dans le filament | Q4 attendre, puis CmD |
| --- | --- | --- | --- | --- |
| fermeture $m=1$ | non (CD à 850, tout à 999,98) | partiel (xa \| bc sur [50 ; 60,21[, puis tout) | partiel (x1 dès 350, tout à 450) | non (CmD, puis tout à 816,5) |
| fermeture $m=k+1$ | **oui** (1 154,68 → 1 787,36) | non (xbc dès 60,36) | non (x avec l'amas dès 453,47 ; tout à 700,50 < 796,12) | non (CPQR \| DSTU, tout à 1 078,17 < 1 334,17) |
| H_1 | non (AB \| EF, puis AB \| CD \| EF) | **oui** (xa \| bc dès 67,37) | **oui** (x12345 dès 744,18) | **oui** (PQR \| STU dès 899,5 ; CmD dès 1 262,27) |
| H_{k+1} | **oui** | non (xbc) | non (x avec l'amas dès 590,54) | non (CPQR \| DSTU, réponse b) |
| EC_{k+1}, first_{k+1} | **oui** | non | non (453,47) | non (b) |
| core | non (CD à 1 700) | — | **oui** (x1 dès 700,006) | non |

Le levier décisif est la qualification $m=k+1$ : elle donne Q1 à la fermeture comme à H_{k+1}, et leur fait
perdre ensemble Q2, Q3 et Q4. Le succès de l'auditeur sur les triangles ne tient pas à la fermeture. Sur Q3 et
Q4, la fermeture qualifiée réunit en plus deux amas entiers avant la racine FULL, contre la réponse de
l'utilisateur. Le verdict v10 annonçait 125 sur 125 cellules ancrées pour ER0h ; aucune des règles jugées ici
n'atteint les quatre réponses.

## 7. Verdict : la place de l'approximation extérieure

1. **Pas la projection principale de la tour.** Pour $m\leq k+1$, la fermeture est la liaison simple de $w_{k'}$
   (T1), à un facteur 2 de l'atteignabilité mutuelle (T2). Elle n'emploie pas la connexité d'ordre supérieur qui
   distingue FULL, et elle réintroduit le chaînage par sites que le chapitre 6 retire. Elle réunit des amas
   entiers avant FULL (§ 4) ; en percolation, son seuil est au plus celui de FULL (C1 : strictement).
2. **Mais un objet mathématique de premier rang.** C'est l'unique approximation extérieure canonique (sous-dominante
   de $w$), la plus stable mesurée (pire rapport $0{,}964\varepsilon$), et elle est entrelacée à FULL avec le
   facteur 2 exact (T3). Ses défauts sont **bornés**. Ceux des règles fidèles ne le sont pas toujours : retard
   non local de H_m, discontinuité de EC et de first.
3. **Places recommandées.**
   - **Certificat d'encadrement** : pour tout candidat fidèle, $u_{CL}\leq w\leq u_F\leq 4\max(e,w)$ et
     $w\leq 4u_{CL}$. Une porte peut vérifier ces inégalités globales à l'échelle par échantillonnage de paires,
     sans juge $O(n^3)$.
   - **Détecteur exact d'ambiguïté** : $w(i,j)/u_{CL}(i,j)\in[1,4]$. Le rapport vaut 1 pour toutes les paires si
     et seulement si $w$ est ultramétrique. Les paires où il dépasse 1 sont exactement celles où toute projection
     fidèle et la fermeture divergent ($u_F\geq w>u_{CL}$, F2, T5) : c'est là que se paie la laminarité.
   - **Témoin dans les campagnes** : à apparier à H_{k+1} sur les mêmes scènes, aux mêmes $k$ et $m$ (non fait :
     les chiffres disponibles viennent de deux campagnes distinctes). Pour $m\leq k+1$, elle ne requiert pas la
     connexité de FULL, ce qui en fait aussi un témoin négatif naturel de la tour.
4. **Pour la projection retenue**, le débat utile n'est pas « extérieur contre intérieur » mais le choix de $m$ et
   la forme de la marge. $m=k+1$ fait perdre Q2–Q4. La marge $\kappa=1$ en niveau carré crée des retards non
   locaux. Ce sont deux défauts de H_{k+1} à traiter avant de la consacrer. S'y ajoute une tension : le choix
   $m=\max(k+1,\mathrm{mcs})$ (H5) rend la projection dépendante de mcs dès que $\mathrm{mcs}>k+1$. Or le verdict
   v10 concluait, d'après les réponses Q1bis et Q-Π2, à une projection **indépendante** de mcs suivie d'une
   condensation. La fermeture à $m$ fixe, elle, respecte cette séparation.

## 8. Limites

- Tous les calculs exacts portent sur $n\leq 9$ (14 pour la seule fixture Q3, à $k=2$). Ce sont des oracles de
  correction et des fixtures, jamais une fréquence ou une pente à $n=8000$. Aucune mesure G4 ni LiDAR n'est jouée
  ici.
- C1 et C2 sont des conjectures. Le théorème 3 de la thèse suppose $\Theta$ continue : je ne l'ai pas vérifié pour
  la fermeture.
- La classification « site partagé / deux amas entiers » dépend du seuil $\theta=\max(m,k+1)$. Les nombres de
  sites partagés et exclusifs sont publiés pour permettre un autre seuil.
- Convention de `min_samples` : T2 est écrit avec le site compté (P1).
- Les sites retardés de EC et de first coïncident en nombre ; je n'ai pas testé l'égalité des règles.
- Le vote et les masses du § 9.1 de la thèse, ainsi que ER0h (v10), ne sont pas rejoués ici ; seules les cibles
  Q1–Q4 sont confrontées.

## 9. Reproduction

Depuis `build/v11-points-math/fermeture/`, avec `PYTHONDONTWRITEBYTECODE=1 python3` (Python 3.12.1) :

```text
verif_fixtures_auditeur.py                                   -> resultats/fixtures_auditeur.txt   (1 s)
verif_garanties.py --clouds 450 --seed 20261003 --seconds 400 --out resultats/garanties.json -> garanties.txt (139 s)
verif_stabilite.py --clouds 600 --seed 7 --seconds 200 --out resultats/stabilite.json -> stabilite.txt (61 s)
familles.py > resultats/familles.txt                                                              (1 s)
frequences.py --clouds 900 --seed 11 --seconds 240 --out resultats/frequences.json -> frequences.txt (31 s)
ec_vs_hm.py --clouds 600 --seed 23 --seconds 100 --out resultats/ec_vs_hm.json -> ec_vs_hm.txt    (23 s)
cibles_utilisateur.py > resultats/cibles_utilisateur.txt                                          (1 s)
rival_lointain.py > resultats/rival_lointain.txt ; rival_lointain_m3.py > resultats/rival_lointain_m3.txt
```

`lib_fermeture.py` contient la fermeture par les coupes, la forme directe $HL_{k'}$, l'atteignabilité mutuelle,
EC, les anticipations classées, les fractions du chapitre 7 et les générateurs. Il importe sans les modifier
`reference/hgp11_ref` (étage A) et `bench/points_reference.py`.
