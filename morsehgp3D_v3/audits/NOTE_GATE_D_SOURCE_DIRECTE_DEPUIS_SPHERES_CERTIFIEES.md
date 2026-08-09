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

Fixture minimale d'arité deux, rendue affine trois par deux points extérieurs :

```text
A=(0,0,0) B=(2,0,0) z=(1,0,0) C=(0,10,0) D=(0,0,10)
```

La boule de diamètre `AB` a $I=\lbrace z\rbrace$, $S=\lbrace A,B\rbrace$ et
porte l'unique coface directe `ABz` à l'ordre deux. Elle n'autorise pas à réduire
la source aux seuls sommets pleins de l'arrangement.

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
- niveau exact, $I(B)$, $S(B)$ et supports minimaux, avec leurs autorités;
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
- réduction du producteur aux seuls sommets d'arrangement : **réfutée par les
  arités basses**;
- suppression de la masse extra-shell hors porte : **réfutée si les identités
  de cofaces restent contractuelles**;
- stream terminal de toutes les sphères, fold et contrat 50 k : **ouverts**.

Ce résultat est CPU/GPU agnostique. Il ne justifie aucun benchmark et aucune G4.
GCP non utilisé.
