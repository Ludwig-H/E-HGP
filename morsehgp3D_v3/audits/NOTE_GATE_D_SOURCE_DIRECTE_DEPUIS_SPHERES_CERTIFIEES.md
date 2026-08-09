# Gate D — source directe locale depuis un stream de sphères certifiées

Date : 9 août 2026 UTC.

Cadre : `backend=architecture_math`, `profile=hgp_reduced_quantized_u16`,
`mode=certified_sphere_stream_to_open_gabriel_source`,
`public_status=not_claimed`.

> [!IMPORTANT]
> Une fois une sphère exacte $B$, son intérieur strict $I(B)$, sa coquille
> $S(B)$ et ses supports minimaux certifiés, toutes ses cofaces de Gabriel
> ouvertes se décident **localement**. Sous la porte régulière forte
> $S(B)=U(B)$ avec support unique, il y en a au plus une :
> $Q=I(B)\cup S(B)$. Hors de cette porte, l'expansion en sous-ensembles de la
> coquille est parfois une masse de sortie intrinsèque. Ce théorème ne construit
> pas le stream terminal de sphères; il isole exactement le verrou qui lui reste.

Cette note complète les notes sur les
[`premières incidences`](NOTE_GATE_D_PREMIERES_INCIDENCES_DU_COEUR.md),
l'[attache unique](NOTE_GATE_D_UNE_ATTACHE_PAR_FACETTE_COEUR.md) et la
[`descente locale du carrier`](NOTE_GATE_D_DESCENTE_LOCALE_CARRIER_ET_FRONTIERE_GLOBALE.md).

## 1. Objet certifié et convention ouverte

Soit $X$ un nuage fini de points distincts et $B$ une boule euclidienne exacte.
On note

$$I(B)=X\cap B^{\circ},\qquad S(B)=X\cap\partial B.$$

Un **support minimal** de $B$ est un sous-ensemble $U\subseteq S(B)$ dont la
miniboule est $B$ et dont aucun sous-ensemble strict n'a cette propriété. En
dimension trois, sa cardinalité est comprise entre un et quatre.

Une coface $Q\subseteq X$ est de **Gabriel ouverte** lorsque sa miniboule
$B_Q$ ne contient aucun point de $X\setminus Q$ strictement à l'intérieur. Les
points extérieurs à $Q$ sur la coquille sont permis; c'est précisément la
politique d'extra-shell.

La présente note suppose que le record de $B$ authentifie :

- la sphère et son niveau exact;
- les ensembles complets $I(B)$ et $S(B)$, ou un range-report rejouable qui les
  reconstruit sans omission;
- tous les supports minimaux requis par la politique de domaine;
- l'identité du nuage et de l'index qui ont produit le census.

Une sphère plausible, un support seul ou une requête interrompue ne suffisent
pas.

## 2. Théorème exact de décomposition de la source

Fixons un ordre $k$ et posons $r=k+1-\lvert I(B)\rvert$.

**Théorème.** Les cofaces de Gabriel ouvertes $Q$ de cardinal $k+1$ dont la
miniboule est exactement $B$ sont en bijection avec les sous-ensembles
$T\subseteq S(B)$ de cardinal $r$ qui contiennent au moins un support minimal de
$B$. La bijection est

$$T\longmapsto Q=I(B)\cup T.$$

Si $r<0$, $r>\lvert S(B)\rvert$ ou si aucun support minimal n'a une cardinalité
au plus $r$, cette famille est vide.

### 2.1 Nécessité

Soit $Q$ une coface ouverte de miniboule $B$.

1. Tout point de $I(B)$ appartient à $Q$. Sinon ce point serait un outsider
   strict de $B_Q$, contrairement à la vacuité ouverte.
2. Aucun point de $Q$ n'est extérieur à $B$, puisque $B$ couvre $Q$.
3. Par conséquent $Q=I(B)\cup T$ avec $T\subseteq S(B)$ et
   $\lvert T\rvert=k+1-\lvert I(B)\rvert$.
4. Toute miniboule possède un support minimal contenu dans l'ensemble qu'elle
   couvre. Comme les points de $I(B)$ sont strictement intérieurs, ce support
   est contenu dans $T$.

### 2.2 Suffisance

Réciproquement, soit $T\subseteq S(B)$ de cardinal $r$ contenant un support
minimal $U$ de $B$. La boule $B$ couvre $I(B)\cup T$ et le sous-ensemble
$U\subseteq T$ force toute boule qui couvre $I(B)\cup T$ à avoir un rayon au
moins égal à celui de $B$. Sa miniboule est donc exactement $B$.

Enfin tous les points strictement intérieurs à $B$ appartiennent à
$I(B)\subseteq Q$. Tout point omis est sur la coquille ou à l'extérieur; $Q$ est
donc de Gabriel ouverte. La construction est réciproque de celle du §2.1.

Une forme équivalente évite de matérialiser la famille entière des supports. Pour
$T\subseteq S(B)$, la miniboule de $T$ est $B$ si et seulement si le centre
$c_B$ appartient à l'enveloppe convexe de $T$. La caractérisation de la
miniboule donne le sens nécessaire; Carathéodory extrait dans l'autre sens un
support positif d'au plus quatre points. L'expansion peut donc certifier chaque
$T$ par un test exact $c_B\in\mathrm{conv}(T)$, ou authentifier la famille
complète des supports minimaux. Elle ne peut se contenter du premier support
rencontré.

## 3. Corollaire régulier : une coface au plus par sphère

Sous la porte régulière forte, le support minimal est unique et essentiel, et
la coquille complète coïncide avec lui : $S(B)=U(B)$. Le théorème donne alors :

- si $\lvert I(B)\rvert+\lvert S(B)\rvert=k+1$, la seule coface directe portée
  par $B$ est $Q=I(B)\cup S(B)$;
- sinon $B$ ne porte aucune coface directe à l'ordre $k$.

La décision coûte donc un test de cardinalité et une émission, sans
énumération de sous-ensembles de coquille et sans table globale de cofaces.

Ce corollaire concerne un **stream déjà complet de sphères certifiées**. Il ne
prouve pas que le seul parcours des sommets d'arrangement produit ce stream :
une miniboule de support d'arité deux ou trois n'est pas nécessairement un
sommet de l'arrangement relevé. Ces arités doivent provenir du producteur local
certifié et de son propriétaire, ou d'une autre source terminale. Les confondre
avec les seuls sommets de rang quatre perdrait des cofaces directes.

Fixture d'arité deux non collinéaire, rendue affine trois par un point extérieur :

```text
A=(0,0,0) B=(4,0,0) z=(2,1,0) D=(1,2,10)
```

La boule de diamètre `AB` a $I=\lbrace z\rbrace$, $S=U=\lbrace A,B\rbrace$ et
porte l'unique coface directe `ABz` à l'ordre deux. Le simplex `ABz` n'est pas
collinéaire, mais sa miniboule a toujours un support de rang deux.

L'arité trois est tout aussi réelle :

```text
A=(0,0,0) B=(4,0,0) C=(2,3,0) D=(0,0,10)
```

Le cercle de `ABC`, de centre `(2,5/6,0)` et rayon carré `169/36`, porte la
coface directe `ABC` à l'ordre deux. Son record vit sur un flat de rang trois,
pas sur un sommet de rang quatre.

Enfin, une sphère brute d'arrangement ne remplace pas la miniboule. Pour
`A=(0,0,0), B=(4,0,0), C=(1,1,0), D=(1,0,1), P=(2,1,1)`, la sphère passant par
`ABCD` a pour centre `(2,-1,-1)` et rayon carré 6, mais la miniboule de `ABCD`
est le diamètre `AB`, de rayon carré 4, qui contient `P` strictement. Interpréter
directement le sommet brut comme coface ouverte serait faux. La fixture positive
de rang quatre
`A=(0,0,0), B=(0,2,2), C=(2,0,2), D=(2,2,0), P=(1,1,1)` a au contraire
$S=U=\lbrace A,B,C,D\rbrace$, $I=\lbrace P\rbrace$, centre `P` et rayon carré 3;
elle porte l'unique coface `ABCDP` à l'ordre quatre.

### 3.1 Plafond shallow positif pour le producteur

Le théorème de propriétaire de la
[`note de parent local`](NOTE_PARENT_LOCAL_REVERSE_SEARCH_GATE_D.md) ferme une
partie importante du verrou de complétude. Soit $Q$ une coface ouverte d'ordre
$k$, $B=B_Q$, $d=\lvert I(B)\rvert$, et $U$ un support minimal de cardinal
$q$ contenu dans sa partie de coquille. Comme $Q$ contient $I(B)\cup U$ et a
cardinal $k+1$, on a $d+q\leq k+1$.

Sous dimension affine trois, le propriétaire canonique $o(U)$ est un vrai
sommet d'arrangement contenant $U$ et son intérieur strict est inclus dans
$I(B)$. Son niveau vérifie donc $\ell(o(U))\leq d\leq k+1-q$. Dans le profil à
coordonnées distinctes et pour $k\geq1$, une coface non triviale a $q\geq2$.
Par conséquent, un reverse search jusqu'au niveau strict $k-1$ rencontre un
propriétaire de **toute** sphère source pertinente : plafond $k-1$ pour les
paires, $k-2$ pour les flats de rang trois et $k-3$ pour les supports de rang
quatre. La taille d'une extra-coquille ne change pas ce plafond.

Ce corollaire ne dit pas que la sphère du propriétaire est la sphère critique.
Le flux exact est :

```text
sommet shallow -> support/flat canonique U -> miniboule x_U
                -> census global I,S -> expansion locale T -> coface Q
```

Il reste à énumérer les supports ou flats canoniques incidents sans retomber sur
tous les sous-ensembles d'une grande coquille, à appliquer le critère local de
propriété, puis à dédupliquer les supports multiples d'une même boule. Le plafond
de navigation est donc **prouvé**; la borne de travail du harvest et la
terminalité du stream restent ouvertes.

Il faut récolter **tous** les flats fermés incidents, pas seulement les rayons du
cône de parent ou les arêtes qui possèdent un voisin fini. La fixture u16

```text
A=(8,4,2) B=(0,8,2) C=(0,0,2) p=(7,5,0) y=(8,5,3)
```

le montre déjà en rang trois. Sur le pinceau `ABC`, paramétré par le centre
`(3,4,2+t)`, le rayon carré vaut $25+t^2$, et les puissances de `p` et `y`
valent respectivement $-4+4t$ et $2-2t$. À $t=0$, la sphère critique a
$S=U=\lbrace A,B,C\rbrace$, $I=\lbrace p\rbrace$ et porte la coface directe
`ABCp` à l'ordre trois. Son propriétaire est l'unique sommet $t=1$, de coquille
`ABCpy` et de niveau zéro. Les pentes opposées de `p` et `y` interdisent pourtant
toute orientation non nulle dans le cône de chambre, et les deux côtés du
pinceau sont sans voisin fini. Une récolte branchée après le filtre du parent
omettrait donc cette sphère exacte.

L'arité deux ne se récupère pas non plus depuis les seuls extrema de rang trois.
Avec

```text
A=(0,0,0) B=(4,0,0) z=(2,1,0) C=(0,10,0) D=(0,0,10)
```

le flat `AB` s'écrit par les centres `(2,y,w)`. La boule diamètre source est son
minimum `(y,w)=(0,0)` et porte `ABz`. Son propriétaire est le sommet
`ABCD`, en `(y,w)=(5,5)`, avec `z` strictement intérieur. Les deux flats de rang
trois contenant `AB` atteignent leurs propres minima en `(5,0)` et `(0,5)`;
aucun ne redonne la boule diamètre. La source doit donc traiter le flat de rang
deux lui-même, et non espérer reconstruire toutes les paires depuis les seuls
plans extrêmes.

### 3.2 Borne de sortie par propriétaire sous zéro-extra-shell

La difficulté précédente n'impose pas pour autant l'énumération brute de toutes
les paires ou de tous les triplets. Soit $v$ un propriétaire shallow,
$m=\lvert S(v)\rvert$, et fixons un plafond $h$ sur le nombre de points de
$S(v)$ strictement intérieurs à la sphère cible. Tous les points de $S(v)$ sont
sur une même sphère strictement convexe $V$.

Pour une paire cible $U$ possédée par $v$, le plan radical de sa boule diamètre
et de $V$ contient exactement les deux points de $U$ sous zéro-extra-shell. Le
côté intérieur contient au plus $h$ points de $S(v)$. Si
chaque point de $S(v)$ est gardé indépendamment avec probabilité $p$, l'événement
« les deux supports sont gardés et tous ces intérieurs sont omis » a probabilité
au moins $p^2(1-p)^h$. Dans cet événement, $U$ est une arête exposée de
l'enveloppe convexe de l'échantillon. Or un polyèdre convexe à $r$ sommets a au
plus $3r$ arêtes. Si $N_2(v,h)$ est le nombre de telles paires, alors

$$N_2(v,h)p^2(1-p)^h\leq3pm.$$

Pour un support cible de rang trois, le même plan contient exactement son
triangle; après omission des intérieurs, il devient une face triangulaire
exposée. Un polyèdre convexe à $r$ sommets a au plus $2r$ faces, d'où

$$N_3(v,h)p^3(1-p)^h\leq2pm.$$

Pour $h\geq1$, prendre $p=1/(h+1)$ et utiliser
$(1-p)^h\geq1/4$ donne les bornes explicites
$N_2(v,h)\leq12m(h+1)$ et $N_3(v,h)\leq8m(h+1)^2$. Pour $h=0$, prendre
$p=1$ donne les bornes plus fortes $N_2(v,0)\leq3m$ et
$N_3(v,0)\leq2m$.

Au vrai propriétaire, les deux inclusions du certificat donnent l'identité
$I(B)=B(v)\mathbin{\dot\cup}(I(B)\cap S(v))$. Pour toutes les sources d'ordre au
plus $K$, un plafond local certifié est donc $h_q(v)=K+1-q-\ell(v)$; s'il est
négatif, $v$ ne possède aucune source de rang $q$. Les bornes utiles deviennent
$N_2(v)\leq12m(K-\ell(v))$ et
$N_3(v)\leq8m(K-1-\ell(v))^2$, avec la branche $h=0$ ci-dessus. Soustraire le
niveau sans avoir vérifié les deux inclusions du propriétaire serait en revanche
une censure. À $K\leq10$, la masse possédée par un sommet est donc linéaire en
$m$ sous la porte régulière, avec une constante dépendant de $K$, pas
quadratique ou cubique en $m$. Le rang quatre contribue au plus une sphère et
impose alors $m=4$. Si la sphère cible est $V$ elle-même, zéro-extra-shell donne
directement $m=q$ et le cas est trivial.

Ce résultat est une borne de sortie, pas encore un algorithme. Il suggère de
construire sur $S(v)$ le graphe de Gabriel sphérique $h$-shallow pour le rang
deux et les faces triangulaires bien centrées de calottes sphériques
$h$-shallow pour le rang trois, puis de recertifier miniboule, census, support
canonique et owner. Ces objets sont des sous-ensembles des arêtes et faces d'une
Delaunay sphérique d'ordre au plus $h$; les identifier sans convention sur les
ex aequo serait trop fort. La construction déterministe, son coût, ses
multiplicités et son oracle restent un verrou mathématique/algorithmique. Hors
zéro-extra-shell, d'autres points peuvent rester sur le plan exposé et
l'argument ne donne pas cette borne sans quotient supplémentaire.

Un univers couvrant certifié est la famille de squelettes de coques sphériques
jusqu'à $h$ suppressions

$$\mathcal{C}_h(S(v))=\bigcup_{D\subseteq S(v),\,\lvert D\rvert\leq h}\mathrm{skel}_{1,2}\!\left(\mathrm{conv}(S(v)\setminus D)\right).$$

Toute paire cible avec $t\leq h$ est une arête du convexe obtenu en supprimant
ses $t$ intérieurs, et tout triple cible est une face triangulaire du même
convexe. Ainsi $\mathcal{C}_h$ couvre toutes les candidates, mais ne les
caractérise pas : diamètre, bon centrage, census et owner restent à vérifier.
Les squelettes provenant de suppressions différentes peuvent même se croiser;
$\mathcal{C}_h$ n'est pas nécessairement un complexe géométrique. Énumérer cet
univers sans reconstruire tous les ensembles supprimés est le verrou
combinatoire précis. En dégénérescence, une face polygonale et ses vraies arêtes
doivent rester telles quelles; une triangulation arbitraire ne peut pas servir
d'autorité.

Une porte de mutation doit notamment refuser : le rang fermé à la place des
intérieurs stricts; l'oubli du cas $h=0$; un point extérieur supplémentaire sur
le plan radical; un support triangulaire qui n'est pas bien centré; la confusion
entre les intérieurs globaux de $B$ et ceux de $S(v)$; le cas cible $B=V$; et
une récolte limitée aux rayons du cône de parent ou aux voisins finis.

La fixture entière reproductible
[`check_gate_d_shallow_source_bound.py`](check_gate_d_shallow_source_bound.py)
place 36 points sur le cercle équatorial et deux pôles d'une sphère u16 de centre
`(65,65,65)` et rayon 65. Son unique sommet de niveau zéro possède une coquille
de 38 points : tous les sites sont sur cette sphère et leur dimension affine
trois rend le système de normales relevées de rang quatre. Le script compte
exactement **432** sphères régulières de rang deux
avec au plus neuf intérieurs et **648** sphères régulières de rang trois avec au
plus huit intérieurs, exclusivement par signes entiers. Il décompose aussi les
comptes par profondeur : 108 puis neuf fois 36 pour le rang deux, et neuf fois
72 pour le rang trois; les dix-neuf diamètres porteurs d'extra-shell sont
explicitement exclus. Cette fixture exerce une masse linéaire non triviale; elle
ne prétend pas saturer les constantes des bornes.

## 4. Hors régularité : l'extra-shell est une masse réelle

Supposons pour commencer que $B$ possède un support minimal unique $U$, mais une
coquille de taille $s>\lvert U\rvert$. Si $r\geq\lvert U\rvert$, le nombre de
cofaces directes portées par ce record vaut exactement

$$\binom{s-\lvert U\rvert}{r-\lvert U\rvert}.$$

Avec plusieurs supports minimaux, la famille est l'union des sous-ensembles de
taille $r$ qui contiennent au moins l'un d'eux; elle doit être dédupliquée par
l'identité canonique de $T$, pas par le premier support rencontré.

Si $\mathcal{U}(B)$ est la famille complète des supports, son cardinal exact est
donné par l'inclusion-exclusion

$$N_B(r)=\sum_{\varnothing\neq A\subseteq\mathcal{U}(B)}(-1)^{\lvert A\rvert+1}\binom{\lvert S(B)\rvert-\left\lvert\bigcup_{U\in A}U\right\rvert}{r-\left\lvert\bigcup_{U\in A}U\right\rvert}.$$

On adopte la convention que le binomial vaut zéro hors domaine. Cette identité
est un oracle de comptage, pas une stratégie produit lorsque la famille des
supports est elle-même grande.

Le cube u16 $\lbrace0,2\rbrace^3$ est une fixture compacte. Sa sphère
circonscrite a huit points de coquille, aucun point intérieur et **six** supports
minimaux : quatre paires diamétrales antipodales et les deux tétraèdres de parité.
À l'ordre trois, une coface a quatre points. Parmi les
$\binom{8}{4}=70$ sous-ensembles, 54 contiennent un diamètre; parmi les 16 qui
choisissent un point dans chaque paire antipodale, les deux tétraèdres de parité
forcent encore la même sphère. Il y a donc exactement **56** cofaces de Gabriel
ouvertes portées par cette sphère, et 14 sous-ensembles dont la miniboule est
strictement plus petite.

Cette fixture réfute trois raccourcis :

1. émettre une seule coface par sphère hors porte régulière;
2. choisir seulement les supports diamétraux ou un seul support minimal lorsque
   plusieurs arités portent la même boule;
3. appeler l'expansion extra-shell un simple doublon interne.

Si le contrat expose les identités de ces cofaces, leurs 56 records sont une
sortie scientifique. Un quotient horizontal plus petit demanderait un théorème
et un contrat versionné distincts; il ne peut pas les supprimer implicitement.

La masse devient déjà grande sur une petite fixture u16 à support unique. Prendre
le centre `c=(20,20,20)`, le rayon 13, la paire antipodale
`U={c+(-13,0,0),c+(13,0,0)}` et ajouter à la coquille les dix-neuf vecteurs
relatifs suivants :

```text
(-5,12,0) (5,12,0) (0,12,-5) (0,12,5)
(-12,5,0) (12,5,0) (0,5,-12) (0,5,12)
(4,12,3) (4,12,-3) (-4,12,3) (-4,12,-3)
(3,12,4) (3,12,-4) (-3,12,4) (-3,12,-4)
(12,4,3) (-12,4,3) (12,4,-3)
```

Tous ont norme carrée 169 et une coordonnée relative `y` positive. Toute
combinaison convexe donnant le centre met donc un poids nul sur ces dix-neuf
points : la paire $U$ est l'unique support minimal. À $k=10$, chaque choix de
neuf points parmi les dix-neuf extras complète $U$ en une coface ouverte
distincte. Une seule sphère porte ainsi
$\binom{19}{9}=92\,378$ records de sortie. Aucune localité du parent ne peut
supprimer cette masse si leurs identités restent contractuelles.

## 5. Ce que le résultat retire, et ce qu'il laisse global

Le théorème retire la recherche suivante : « pour une sphère certifiée, quelles
cofaces ouvertes porte-t-elle ? » Cette décision n'a besoin ni de $\Gamma_k$, ni
d'un locator de composantes, ni d'une mosaïque d'ordre supérieur.

Il laisse quatre obligations séparées.

1. **Complétude du stream de sphères.** Chaque miniboule pertinente, y compris
   les arités deux et trois, doit être produite exactement une fois par un
   propriétaire certifié.
2. **Census terminal.** $I(B)$ et $S(B)$ doivent être complets. Un index peut
   éviter le scan systématique, jamais transformer une omission en preuve de
   vacuité.
3. **Masse extra-shell.** Hors porte régulière, l'expansion peut être
   combinatoire et doit être streamée, compressée sous un théorème explicite, ou
   refusée par un profil qui l'annonce.
4. **Fold aval.** Les cofaces émises doivent encore être triées par niveau exact,
   groupées en lots, puis réduites contre la partition horizontale pré-lot.

Ainsi la source directe n'est plus une boîte noire unique. Elle se factorise en

```text
stream terminal de sphères -> census exact -> expansion locale -> runs triés
```

Le premier étage reste le verrou mathématique/producteur; le dernier reste une
globalité de fold.

## 6. Reçu minimal et portes de mutation

Un record `OpenGabrielCofaceFromCertifiedSphere` doit engager :

- digest du nuage, de l'index et du record de sphère;
- niveau exact, $I(B)$, $S(B)$ et soit la famille des supports minimaux, soit le
  certificat exact $c_B\in\mathrm{conv}(T)$, avec leurs autorités;
- ordre $k$, valeur $r$ et décision vide/non vide;
- identité canonique de $T$ et preuve qu'il contient un support minimal;
- identité $Q=I(B)\cup T$, cardinal $k+1$ et politique d'extra-shell;
- compteur demandé/émis, déduplication et watermark terminal du record de
  sphère;
- zéro publication si census, support, budget ou terminalité manque.

Les mutations permanentes minimales sont : omettre un point de $I(B)$; utiliser
la boule ouverte au lieu de la coquille pour $S(B)$; accepter un $T$ sans
support; ne garder que les quatre diamètres ou le premier support du cube;
émettre `ABz` sans la source d'arité deux; arrêter l'expansion avant les 56
cofaces du cube; et committer un préfixe après budget.

## 7. Décision

- caractérisation locale des cofaces ouvertes d'une sphère certifiée :
  **prouvée sans régularité**;
- émission unique sous $S(B)=U(B)$ : **prouvée**;
- plafond de navigation strict $k-1$ pour rencontrer un propriétaire de toute
  sphère source : **prouvé sous dimension affine trois et coordonnées
  distinctes**;
- nombre de sources de rang deux/trois possédées par un sommet : **borné par
  $O(mK)$ et $O(mK^2)$ sous zéro-extra-shell, avec constantes explicites**;
- identification de la sphère critique à la sphère brute du sommet propriétaire :
  **réfutée par les arités basses et la fixture mal centrée**;
- suppression de la masse extra-shell hors porte : **réfutée si les identités
  de cofaces restent contractuelles**;
- algorithme output-sensitive de harvest des flats/supports, stream terminal de
  toutes les sphères, fold et contrat 50 k : **ouverts**.

Ce résultat est CPU/GPU agnostique. Il ne justifie aucun benchmark et aucune G4.
GCP non utilisé.
