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

## 4. Hors régularité : l'extra-shell est une masse réelle

Supposons pour commencer que $B$ possède un support minimal unique $U$, mais une
coquille de taille $s>\lvert U\rvert$. Si $r\geq\lvert U\rvert$, le nombre de
cofaces directes portées par ce record vaut exactement

$$\binom{s-\lvert U\rvert}{r-\lvert U\rvert}.$$

Avec plusieurs supports minimaux, la famille est l'union des sous-ensembles de
taille $r$ qui contiennent au moins l'un d'eux; elle doit être dédupliquée par
l'identité canonique de $T$, pas par le premier support rencontré.

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
cofaces ouvertes porte-t-elle ? » Cette décision n'a besoin ni de $Gamma_k$, ni
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
- identification de la sphère critique à la sphère brute du sommet propriétaire :
  **réfutée par les arités basses et la fixture mal centrée**;
- suppression de la masse extra-shell hors porte : **réfutée si les identités
  de cofaces restent contractuelles**;
- harvest borné des flats/supports, stream terminal de toutes les sphères, fold
  et contrat 50 k : **ouverts**.

Ce résultat est CPU/GPU agnostique. Il ne justifie aucun benchmark et aucune G4.
GCP non utilisé.
