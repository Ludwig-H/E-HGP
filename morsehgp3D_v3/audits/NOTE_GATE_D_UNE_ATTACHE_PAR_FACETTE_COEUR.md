# Gate D — une attache résolue par facette cœur

Date : 9 août 2026 UTC.

Cadre : `backend=architecture_math`, `profile=hgp_reduced_quantized_u16`,
`mode=one_resolved_attachment_per_core_facet`, `public_status=not_claimed`.

> [!IMPORTANT]
> Sous la porte régulière du théorème 4, il n'est pas nécessaire de publier tous
> les co-minimiseurs silencieux $M(F)$. Une unique attache canonique par facette
> cœur ayant au moins deux intrus suffit à la forêt horizontale $H_0$
> normalisée. Cette attache doit viser le **carrier strict déjà résolu**, pas une
> simple facette stricte : celle-ci peut être absente du cœur direct. La masse
> silencieuse publiée tombe ainsi à au plus $lvert D_kvert$ attaches, mais
> l'information globale de partition pré-lot ne disparaît pas.

Cette note renforce la factorisation de
[`NOTE_GATE_D_PREMIERES_INCIDENCES_DU_COEUR.md`](NOTE_GATE_D_PREMIERES_INCIDENCES_DU_COEUR.md).
Elle concerne uniquement le quotient horizontal $H_0$ normalisé. Elle ne
préserve ni les facettes non-cœur, ni les identités Gamma du contrat v2, ni les
verticales.

## 1. Hypothèses et objet

Fixons un ordre $2\leq k<n$. Soient $\mathcal{G}_k$ la famille complète des
cofaces de Gabriel ouvertes de cardinal $k+1$ et $D_k$ l'ensemble de leurs
facettes. On suppose toutes les hypothèses de la porte régulière du théorème 4 :

- support minimal unique et essentiel pour chaque objet requis;
- tout point hors support strictement intérieur et aucune égalité extérieure
  pertinente;
- catalogues directs et requêtes top-$k$ exacts et terminaux;
- descentes strictes et resolver de carriers complets;
- niveaux rationnels exacts et lots égaux contractés atomiquement.

Pour $F\in D_k$, notons $B_F$ sa miniboule, $a_F=\beta(F)$, $U_F$ son support
essentiel et

$$J_F=\bigl(B_F^{\circ}\cap X\bigr)\setminus F.$$

La source déjà démontrée est

$$\mathcal{A}_k=\mathcal{G}_k\cup\bigcup_{F\in D_k}M(F).$$

Le but est de remplacer chaque famille silencieuse potentiellement grande
$M(F)$ par zéro ou une attache interne.

## 2. Construction canonique

Si $\lvert J_F\rvert\leq1$, aucune attache supplémentaire n'est émise : le
théorème de première incidence montre que $M(F)\subseteq\mathcal{G}_k$.

Si $\lvert J_F\rvert\geq2$, choisissons canoniquement
$z_F=\min J_F$ et $u_F=\min U_F$, selon les `PointId` canoniques, puis posons

$$T_F=\bigl(F\setminus\lbrace u_F\rbrace\bigr)\cup\lbrace z_F\rbrace.$$

On a $\beta(T_F)<a_F$. En effet, $T_F\subset B_F$, le point $z_F$ est strictement
intérieur et $u_F$ est essentiel. Si $T_F$ avait encore pour rayon minimal
$a_F$, l'unicité de la miniboule imposerait $B_{T_F}=B_F$; ses points de
frontière seraient contenus dans $U_F\setminus\lbrace u_F\rbrace$, ce qui
contredirait l'essentialité de $u_F$.

Notons

$$P_F=\mathrm{Resolve}_{<a_F}(T_F)$$

le token de composante du carrier de $T_F$ dans l'état strictement antérieur au
lot $a_F$. L'attache $\alpha_F$ porte le propriétaire `facet_key=F`, le niveau
$a_F$ et l'arête logique $F\leftrightarrow P_F$.

Le mot `Resolve` est essentiel. Il peut être implémenté par une descente locale
certifiée suivie d'une jointure vers le locator externe, ou par un handle de
carrier déjà authentifié. Une recherche dans le seul ensemble $D_k$ ne suffit
pas en général.

## 3. Théorème de l'attache unique

Définissons $\mathcal{C}_k$ comme la source directe $\mathcal{G}_k$ augmentée,
pour chaque $F\in D_k$ tel que $\lvert J_F\rvert\geq2$, de l'unique attache
$\alpha_F$ précédente.

**Théorème.** Sous les hypothèses du §1, $\mathcal{C}_k$ et
$\mathcal{A}_k$ induisent, à toute coupe ouverte ou fermée, la même partition
sur les facettes de cœur, les mêmes unions de points et, après normalisation des
continuations omises, les mêmes $q_R$, parents et nœuds de la forêt horizontale
$H_0$.

### 3.1 Un co-minimiseur silencieux possède un apex strict unique

Pour $Q=F\cup\lbrace z\rbrace\in M(F)$ avec $\lvert J_F\rvert\geq2$, un autre
point de $J_F$ reste strictement intérieur : $Q$ n'est pas direct. Le lemme des
attaches silencieuses place toutes ses facettes strictes dans une unique
composante antérieure $P(Q)$, laquelle couvre déjà tous les points de $Q$.

La facette $T_F$ est l'une de ces facettes strictes. Par conséquent,
$P_F=P(Q)$ et $P_F$ couvre déjà $F\cup\lbrace z_F\rbrace$. L'attache
$\alpha_F$ installe donc $F$ dans le bon apex sans ajouter de point et sans
créer de racine ou de fusion.

### 3.2 Les autres facettes égales ne transportent pas une autre racine

Une facette égale de $Q$ distincte de $F$ s'obtient en remplaçant dans $F$ un
point intérieur par $z$. Elle conserve $U_F$, la même boule et le même nombre
d'intrus. Si cette facette $H$ appartient à $D_k$, sa propre attache
$\alpha_H$ l'installe, et la confluence du plateau silencieux donne
$P_H=P_F$. Si $H\notin D_k$, elle n'est réutilisée par aucune coface directe et
peut disparaître du quotient normalisé.

Deux cofaces non directes partageant une telle facette égale ont le même apex
par le même lemme de confluence. Une facette égale ne peut donc pas cacher une
liaison entre deux composantes strictes distinctes. Sous la porte régulière, un
contact égal direct--non-direct est impossible; tout contact strict est déjà
contracté avant le lot.

### 3.3 Quotient du lot complet

Après contraction de l'état strict, chaque composante du lot formé par tous les
$M(F)$ est une étoile dont le centre est un unique apex antérieur et dont les
feuilles pertinentes sont les facettes de cœur nouvelles. La famille des
$\alpha_F$ conserve exactement une arête de ce centre vers chacune de ces
feuilles. Elle induit donc la même relation d'équivalence après projection sur
$D_k$.

L'argument vaut pour le **lot complet**. Appliquer les attaches une par une sur
un état déjà muté réintroduirait le défaut de quotient que la fermeture des ex
æquo interdit. L'induction par niveaux, puis le corollaire qui élimine les
cofaces tardives, donnent la conclusion à toutes les coupes. $\square$

## 4. Pourquoi la cible brute est fausse

La règle tentante « attacher $F$ directement à $T_F$ dans le cœur » est
réfutée, même sous régularité. Une fixture entière exacte d'ordre trois prend
les huit points, dans l'ordre des identifiants :

```text
(0,23,6) (0,26,17) (9,23,10) (17,32,18)
(18,28,26) (28,2,20) (30,25,11) (30,25,21)
```

Il s'agit de la translation de $(16,16,16)$ de la fixture entière d'origine;
elle est donc dans la grille u16 et conserve toutes les miniboules et tous les
niveaux.

Toutes les miniboules de cardinal trois et quatre requises ont une coquille
globale égale à leur support. Pour $F=235$,
$a_F=96615475/373338$, $U_F=235$ et $J_F=\lbrace4,6,7\rbrace$. Le choix canonique
donne $z_F=4$, $u_F=2$ et $T_F=345$, avec
$\beta(T_F)=1025/4<a_F$.

Mais $T_F\notin D_3$. Une attache brute `235 -> 345` laisse au niveau $a_F$ une
composante cœur isolée couvrant seulement $\lbrace2,3,4,5\rbrace$, alors que le
quotient par tous les co-minimiseurs place `235` dans l'apex antérieur qui couvre
les huit points et contient vingt-quatre facettes cœur. Le chemin strict

```text
345 --[3456, niveau 1025/4]-- 346
```

atteint `346`, qui appartient à $D_3$. La cible résolue est donc correcte; la
clef stricte brute ne l'est pas.

Cette fixture montre précisément ce qui reste global : le choix de $T_F$ est
local, mais la question « dans quelle composante stricte vit $T_F$ ? » dépend de
l'histoire horizontale antérieure.

## 5. Contrôle borné transitoire

Un différentiel rationnel transitoire, non versionné et donc non qualifiant, a
comparé Gamma exhaustif, `directes + tous M(F)` et
`directes + une attache résolue` à chaque niveau exact.

- La fixture `E5`, $n=5$, $k=2$, passe avec une attache au lieu des deux
  cofaces silencieuses de `AC`.
- Sur 200 nuages entiers de cinq à huit points aux ordres deux et trois, 152
  satisfont la porte régulière forte et 48 sont refusés. Les cas acceptés
  totalisent 2 317 facettes cœur, 1 175 cofaces directes, 1 194 cofaces dans la
  source avec tous les premiers incidents, dix branches à au moins deux intrus
  et vingt intrus correspondants.
- Aucune divergence n'est observée entre Gamma et la source complète, ni entre
  celle-ci et l'attache unique résolue.

Ce contrôle falsifie une erreur simple, mais ne remplace ni un harnais conservé,
ni les fixtures permanentes, ni un oracle indépendant du resolver sujet.

## 6. Capability et coût

Un reçu `ResolvedCoreFacetAttachment` doit engager au minimum :

- `facet_key=F`, sa provenance $F\in D_k$, $a_F$, $B_F$ et $U_F$;
- le census fermé complet et la classification terminale
  $\lvert J_F\rvert\in\lbrace0,1,\geq2\rbrace$;
- pour la troisième branche, $z_F$, $u_F$, $T_F$ et la preuve
  $\beta(T_F)<a_F$;
- un certificat `ResolveStrictCarrier` lié au watermark $<a_F$, son token
  $P_F$ et la preuve que $P_F$ couvre $F\cup\lbrace z_F\rbrace$;
- `added_core_facet=F`, `added_points=empty`, le lot exact complet et le digest
  de toutes les provenances agrégées.

Plusieurs attaches peuvent avoir le même apex ou la même arête logique. Le
stockage peut les agréger, mais le reçu doit conserver le digest de chaque
propriétaire de facette.

La sortie silencieuse passe de
$\Theta\bigl(\sum_{F\in D_k}\lvert M(F)\rvert\bigr)$ records potentiels à au
plus $\lvert D_k\rvert$ attaches. Les requêtes de boule fermée, la preuve de
régularité, le tri par niveau et la résolution stricte restent nécessaires. Le
range-report peut toucher $\Theta(n)$ points et le resolver reste une jointure
globale logique : aucune borne 50 k ou SLO ne découle de ce théorème.

## 7. Portes de falsification

Avant toute autorité produit, il faut graver :

- la fixture régulière du §4, qui doit refuser une cible brute hors $D_k$ et
  accepter sa cible résolue;
- `E5`, avec égalité du quotient complet et du quotient à une attache;
- deux facettes cœur égales du même plateau, qui doivent viser le même apex;
- une facette égale non-cœur, omise sans changement de partition ou de
  couverture;
- un lot mêlant attaches et cofaces directes, contracté atomiquement;
- les mutations `Resolve` absent, token d'un niveau fermé au lieu de strict,
  cible d'un autre apex, oubli d'un propriétaire et budget moins un, toutes sans
  payload partiel;
- un différentiel permanent contre Gamma exhaustif aux coupes ouvertes et
  fermées, avec comparaison de la partition sur $D_k$, des couvertures, $q_R$,
  parents et nœuds.

## 8. Décision

- tous les $M(F)$ comme sortie matérielle : **non nécessaires pour $H_0$
  normalisé sous la porte régulière**;
- une attache par facette cœur avec au moins deux intrus : **théorème prouvé ici**;
- cible strictement locale $T_F$ : **réfutée**;
- target `Resolve_{<a_F}(T_F)` : **nécessaire et suffisante sous les hypothèses**;
- état global résident : **non requis**;
- information globale de partition pré-lot : **toujours requise**, mais
  externalisable par handles, journaux et jointures;
- contrat Gamma/v2, verticalité et 50 k : **non qualifiés**.

GCP non utilisé.
