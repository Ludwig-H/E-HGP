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
> silencieuse publiée tombe ainsi à au plus $\lvert D_k\rvert$ attaches, mais
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
- tout sommet de l'objet considéré hors support strictement intérieur à sa
  miniboule, et aucune égalité extérieure pertinente;
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

Si $\lvert J_F\rvert\geq2$, choisissons canoniquement $z_F$ et $w_F$ comme les
deux plus petits éléments distincts de $J_F$, et $u_F=\min U_F$, selon les
`PointId` canoniques, puis posons

$$T_F=\bigl(F\setminus\lbrace u_F\rbrace\bigr)\cup\lbrace z_F\rbrace.$$

On a $\beta(T_F)<a_F$. En effet, $T_F\subset B_F$, le point $z_F$ est strictement
intérieur et $u_F$ est essentiel. Si $T_F$ avait encore pour rayon minimal
$a_F$, l'unicité de la miniboule imposerait $B_{T_F}=B_F$; ses points de
frontière seraient contenus dans $U_F\setminus\lbrace u_F\rbrace$, ce qui
contredirait l'essentialité de $u_F$.

Notons

$$P_F=\mathrm{Resolve}_{<a_F}(T_F)$$

le token de composante du carrier de $T_F$ dans l'état de $\mathcal{C}_k$ déjà
fermé aux niveaux strictement inférieurs à $a_F$. L'attache $\alpha_F$ porte le
propriétaire `facet_key=F`, le niveau $a_F$ et l'arête logique
$F\leftrightarrow P_F$. Cette définition n'est pas circulaire : par induction,
l'état strict de $\mathcal{C}_k$ coïncide avec celui de $\mathcal{A}_k$, et
$\beta(T_F)<a_F$ rend le resolver disponible avant le lot courant.

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

Posons $Q_F=F\cup\lbrace z_F\rbrace$. Le point $w_F$ reste strictement
intérieur à $B_{Q_F}=B_F$, donc $Q_F$ n'est pas direct. Pour chaque $u\in U_F$,
la facette $A_u=Q_F\setminus\lbrace u\rbrace$ et la coface de remplacement
$A_u\cup\lbrace w_F\rbrace$ ont un niveau strictement inférieur à $a_F$. Pour
$u\neq v$, ces remplacements relient $A_u$ et $A_v$ par leur facette commune
$(Q_F\setminus\lbrace u,v\rbrace)\cup\lbrace w_F\rbrace$. Toutes les facettes
strictes $A_u$ appartiennent donc à une unique composante antérieure $P(Q_F)$.

Comme $\lvert U_F\rvert\geq2$, on a
$\bigcup_{u\in U_F}A_u=Q_F$. La composante $P(Q_F)$ couvre ainsi
$F\cup\lbrace z_F\rbrace$. La facette $T_F=A_{u_F}$ est l'un de ses bras;
le resolver donne donc $P_F=P(Q_F)$. Pour tout autre
$z\in J_F$, la coface $F\cup\lbrace z\rbrace$ conflue avec $Q_F$ par leur
facette égale commune $F$, donc possède le même apex. L'attache $\alpha_F$
installe finalement $F$ dans le bon apex, sans ajouter de point et sans créer de
racine ou de fusion.

### 3.2 Les autres facettes égales ne transportent pas une autre racine

Pour $z\in J_F$, posons $Q_z=F\cup\lbrace z\rbrace$. Une facette égale
$H=Q_z\setminus\lbrace x\rbrace$ distincte de $F$ remplace nécessairement un
point intérieur $x\in F\setminus U_F$ par $z$. Elle conserve $U_F$, la même
boule et vérifie
$J_H=(J_F\setminus\lbrace z\rbrace)\cup\lbrace x\rbrace$. On a donc
$\lvert J_H\rvert=\lvert J_F\rvert\geq2$. Si $H$ appartient à $D_k$, sa propre attache
$\alpha_H$ l'installe, et la confluence du plateau silencieux donne
$P_H=P_F$. Si $H\notin D_k$, elle n'est réutilisée par aucune coface directe et
peut disparaître du quotient normalisé.

Deux cofaces non directes partageant une telle facette égale ont le même apex
par le même lemme de confluence. Une facette égale ne peut donc pas cacher une
liaison entre deux composantes strictes distinctes. Sous la porte régulière, un
contact égal direct--non-direct est impossible; tout contact strict est déjà
contracté avant le lot.

### 3.3 Quotient du lot complet

Après contraction de l'état strict, chaque composante de la partie silencieuse
$(\bigcup_{F\in D_k}M(F))\setminus\mathcal{G}_k$ devient, par confluence des contacts
égaux, une étoile dont le centre est un unique apex antérieur et dont les
feuilles pertinentes sont les facettes de cœur nouvelles. La famille des
$\alpha_F$ conserve exactement une arête de ce centre vers chacune de ces
feuilles. Les cofaces directes sont identiques dans $\mathcal{A}_k$ et
$\mathcal{C}_k$ et participent au même quotient atomique. Les deux sources
induisent donc la même relation d'équivalence après projection sur $D_k$.

L'argument vaut pour le **lot complet**. Une implémentation peut effectuer ses
unions internes séquentiellement, mais elle ne doit calculer ni publier $q_R$,
parents, deltas ou nœuds depuis un état partiellement muté. L'induction par
niveaux, puis le corollaire qui élimine les cofaces tardives, donnent la
conclusion à toutes les coupes. $\square$

## 4. Pourquoi la cible brute est fausse

La règle tentante « attacher $F$ directement à $T_F$ dans le cœur » est
réfutée, même sous régularité. Plus fortement, aucun choix du support supprimé
ne garantit que le bras immédiat appartienne au cœur. Une fixture u16 exacte
d'ordre trois prend les dix points, dans l'ordre des identifiants :

```text
(0,1,4) (18,6,24) (38,4,17) (12,22,29) (20,40,11)
(22,5,24) (4,25,10) (17,6,21) (15,31,6) (8,21,14)
```

Les 120 triplets et 210 quadruplets ont un support positif minimal unique; tout
sommet hors support est strictement intérieur, aucun point extérieur n'est sur
la coquille, et les 210 quadruplets sont affinement indépendants avec
$\min\lvert\det\rvert=8$. La facette $F=289$ appartient à $D_3$ par la coface
directe `2489`. Elle vérifie
$a_F=893109/2588$, $U_F=289$ et
$J_F=\lbrace1,5,7\rbrace$, avec des marges strictes positives.

Le choix canonique $z_F=1$ donne trois bras possibles, tous stricts et tous hors
$D_3$ :

| support supprimé | bras | niveau exact |
| ---: | ---: | ---: |
| 2 | `189` | $479/2$ |
| 8 | `129` | $599/2$ |
| 9 | `128` | $299225073/867436$ |

Tous ces niveaux sont strictement inférieurs à $893109/2588$. Les trois
co-minimiseurs silencieux sont `1289`, `2589` et `2789`. Leurs bras rejoignent
la même composante stricte, laquelle rencontre trente-sept facettes de $D_3$;
mais aucun lookup immédiat dans $D_3$, ni aucun autre choix de $u_F$, ne peut
identifier cette composante. La cible résolue est correcte; la clef stricte
brute ne l'est pas.

Cette fixture montre précisément ce qui reste global : le choix de $T_F$ est
local, mais la question « dans quelle composante stricte vit $T_F$ ? » dépend de
l'histoire horizontale antérieure.

## 5. Contrôle borné transitoire

Un différentiel rationnel transitoire, non versionné et donc non qualifiant, a
comparé Gamma exhaustif, `directes + tous M(F)` et
`directes + une attache résolue` à chaque niveau exact.

- La fixture `E5`, $n=5$, $k=2$, passe avec une attache au lieu des deux
  cofaces silencieuses de `AC`.
- Avec le seed `9374`, 25 tirages par couple $(n,k)$, $5\leq n\leq8$,
  $2\leq k\leq\min(3,n-1)$, sur la grille entière $[-10,10]^3$, 152 nuages
  satisfont la porte régulière forte et 48 sont refusés. Les cas acceptés
  totalisent 2 317 facettes cœur, 1 175 cofaces directes et 1 194 cofaces
  logiques distinctes dans $\mathcal{G}_k\cup\bigcup_{F\in D_k}M(F)$, après
  déduplication séparée par nuage. Dix branches ont au moins deux intrus, pour
  vingt co-minimiseurs comptés avec leur propriétaire de facette.
- Aucune divergence n'est observée entre Gamma et la source complète, ni entre
  celle-ci et l'attache unique résolue.
- Une seconde exploration avec le seed `381990`, 7 000 tirages plans rationnels
  de cinq à sept points aux ordres deux et trois, ne trouve pas non plus de
  divergence entre tous les $M(F)$ et l'attache résolue.

Ces deux explorations étaient des heredocs transitoires sans fichier, hash ni
sortie durable. Elles falsifient une erreur simple, mais ne remplacent ni un
harnais conservé, ni les fixtures permanentes, ni un oracle indépendant du
resolver sujet.

## 6. Un census saturé à deux suffit

La branche régulière n'a pas besoin de matérialiser $J_F$ ni tous les $M(F)$.
Une requête certifiée peut rendre seulement
`count_class in {0,1,at_least_2}`, $z_F$ et, dans la troisième classe, $w_F$.
La classe zéro choisit le minimum direct; la classe un désigne une coface déjà
présente dans $\mathcal{G}_k$; la troisième produit l'unique attache.

Pour $k\leq10$, un nœud AABB entièrement et strictement intérieur peut être
agrégé par un compteur saturé à $k+2$ et ses $k+2$ plus petits `PointId`. Cette
marge permet d'exclure les au plus $k$ identifiants de $F$ tout en conservant les
deux plus petits intrus. Un nœud strictement extérieur est élagué; toute
intersection ou égalité descend jusqu'aux points et doit soit confirmer
l'autorité zéro extra-shell, soit échouer fermée.

Ce census borne le payload, pas le temps. Deux intrus stricts distincts prouvent
déjà la classe `at_least_2`; sous une autorité régulière acquise, il n'est pas
nécessaire de visiter le reste pour cette seule décision. Il faut néanmoins
prouver que $z_F,w_F$ sont les deux plus petits identifiants. Un parcours
best-first muni du plus petit `PointId` de chaque sous-arbre peut s'arrêter dès
que toute borne restante dépasse $w_F$. Les classes zéro et un exigent, elles,
une partition complète des nœuds. Sans autorité séparée sur les égalités, deux
témoins stricts ne certifient pas la porte régulière. Le pire cas reste
$\Theta(n)$.

## 7. Capability et coût

Un reçu `ResolvedCoreFacetAttachment` doit engager au minimum :

- `facet_key=F`, sa provenance $F\in D_k$, $a_F$, $B_F$ et $U_F$;
- un census terminal authentifié, saturé en
  $\lvert J_F\rvert\in\lbrace0,1,\geq2\rbrace$, et l'autorité séparée qui exclut
  les extra-shells pertinents;
- pour la troisième branche, $z_F$, $w_F$, $u_F$, $T_F$, la preuve
  $\beta(T_F)<a_F$ et le certificat de couverture du §3.1;
- un certificat `ResolveStrictCarrier` lié au watermark $<a_F$, son token
  $P_F$ et la preuve que $P_F$ couvre $F\cup\lbrace z_F\rbrace$;
- `added_core_facet=F`, `added_points=empty`, le lot exact complet et le digest
  de toutes les provenances agrégées.

Plusieurs attaches de facettes différentes peuvent partager le même apex, mais
leurs arêtes logiques restent distinctes parce que leur propriétaire $F$ est
distinct. Une même facette peut avoir plusieurs provenances directes : elles
doivent être groupées avant de produire un unique $\alpha_F$, avec liste ou
digest de toutes les provenances.

La sortie silencieuse passe de
$\Theta\bigl(\sum_{F\in D_k}\lvert M(F)\rvert\bigr)$ records potentiels à au
plus $\lvert D_k\rvert$ attaches. Les requêtes de boule fermée, la preuve de
régularité, le tri par niveau et la résolution stricte restent nécessaires. Le
range-report peut toucher $\Theta(n)$ points et le resolver reste une jointure
globale logique : aucune borne 50 k ou SLO ne découle de ce théorème.

## 8. Portes de falsification

Avant toute autorité produit, il faut graver :

- la fixture régulière du §4, dont les trois bras immédiats sont hors $D_k$ :
  elle doit refuser tout lookup brut et accepter la cible résolue;
- `E5`, avec égalité du quotient complet et du quotient à une attache;
- le cas $\lvert J_F\rvert=1$, dont la coface est déjà directe et ne doit pas
  produire d'attache;
- une coquille à supports multiples où retirer le support choisi laisse
  $\beta(T_F)=\beta(F)$, qui doit échouer hors de la porte régulière;
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

## 9. Décision

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
