# Gate D — descendre localement jusqu'au cœur, puis résoudre globalement la composante

Date : 9 août 2026 UTC.

Cadre : `backend=architecture_math`, `profile=hgp_reduced_quantized_u16`,
`mode=local_strict_carrier_descent_then_external_fold`,
`public_status=not_claimed`.

> [!IMPORTANT]
> `ResolveStrictCarrier` n'a pas besoin d'être une boîte noire géométrique
> globale. Sous la porte régulière, une descente canonique, strictement
> décroissante en $\beta$, transforme toute facette stricte munie d'un témoin en
> une facette du cœur $D_k$. Dans la **résolution de ce carrier**, ce qui reste
> global est exactement le dernier `find` de cette facette dans la partition
> horizontale antérieure au lot. On supprime ainsi le locator des facettes
> non-cœur, jamais l'information de composante du cœur; la source terminale, les
> lots, la couverture et les verticales conservent leurs propres globalités.

Cette note affine la cible `ResolveStrictCarrier` de
[`NOTE_GATE_D_UNE_ATTACHE_PAR_FACETTE_COEUR.md`](NOTE_GATE_D_UNE_ATTACHE_PAR_FACETTE_COEUR.md)
et répond à la question : une fois le parcours d'arrangement local, quelle
information ne peut toujours pas être décidée localement ?

## 1. Hypothèses et facette témoin

Fixons un ordre $2\leq k<n$, un niveau de lot $a$ et la source directe ouverte
$\mathcal{G}_k$. Son cœur $D_k$ est l'ensemble des facettes de ses cofaces. Pour
un $k$-ensemble $H$, notons $B_H$ sa miniboule, $\beta(H)$ son niveau, $U_H$ son
support minimal essentiel et $J_H=(B_H^{\circ}\cap X)\setminus H$.

La porte est la porte régulière forte, sur **chaque** objet rencontré :

- support minimal unique, positif et essentiel;
- tout point de l'objet hors support strictement intérieur à sa miniboule;
- aucune égalité extérieure pertinente;
- census strict et fermé exacts, source $\mathcal{G}_k$ terminale et niveaux
  rationnels exacts;
- toute erreur, tout budget épuisé et toute sortie hors domaine échouent sans
  résultat partiel.

Ces hypothèses locales suffisent au théorème de descente du §3. Le corollaire
d'attache et l'assertion `added_points=empty` héritent en plus de la porte
globale du théorème d'attache unique : elle doit authentifier tous les objets
nécessaires du plateau silencieux, y compris ceux que le quotient omet. Une
certification limitée à la seule chaîne parcourue ne promeut donc pas le quotient
horizontal.

Une *facette témoin sous $a$* est une paire $(H,W)$ telle que $H\subset W$,
$\lvert H\rvert=k$, $\lvert W\rvert=k+1$ et $\beta(W)<a$. Le témoin ne doit pas
être direct : il certifie seulement que $H$ vit déjà dans une composante de
$\Gamma_k(<a)$.

Dans l'attache de la note précédente, le départ est
$T_F=(F\setminus\lbrace u_F\rbrace)\cup\lbrace z_F\rbrace$. Si $w_F$ est le
second intrus strict canonique, alors $W_0=T_F\cup\lbrace w_F\rbrace$ vérifie
$\beta(W_0)<a_F$. L'essentialité de $u_F$ donne la stricte inégalité : après
avoir retiré $u_F$, tous les points restants de $W_0$ sont sur les autres
supports ou strictement à l'intérieur de $B_F$.

## 2. Descente canonique vers le cœur

Considérons une facette témoin $(H,W)$ avec $\beta(H)<a$.

### 2.1 Branche avec au moins deux intrus

Si $\lvert J_H\rvert\geq2$, choisir $z_H=\min J_H$ et $u_H=\min U_H$ selon les
`PointId` canoniques, puis poser

$$H'=(H\setminus\lbrace u_H\rbrace)\cup\lbrace z_H\rbrace.$$

On a $\beta(H')<\beta(H)$. La preuve est la même que pour l'attache initiale :
$B_H$ couvre $H'$, mais si elle restait sa miniboule, sa frontière dans $H'$ ne
contiendrait plus l'essentiel $u_H$, contradiction.

Posons aussi $Q_H=H\cup\lbrace z_H\rbrace$. Comme $z_H$ est strictement dans
$B_H$, on a $\beta(Q_H)=\beta(H)<a$. Les deux ensembles $H$ et $H'$ sont des
facettes de $Q_H$; ils appartiennent donc à la même composante stricte. Enfin,
$Q_H$ devient le témoin de $H'$ pour l'itération suivante.

### 2.2 Branche avec un intrus

Si $J_H=\lbrace z\rbrace$, la coface $H\cup\lbrace z\rbrace$ a pour miniboule
$B_H$. Après inclusion du seul intrus et sous l'autorité zéro extra-shell, elle
est de Gabriel ouverte. Elle appartient donc à $\mathcal{G}_k$ et $H\in D_k$.
La descente termine sur $R=H$.

### 2.3 Branche sans intrus

Si $J_H=\varnothing$, l'autorité zéro extra-shell donne aussi
$(B_H\cap X)\setminus H=\varnothing$. Le témoin $W$ impose
$\lambda(H)\leq\beta(W)<a$. Le lemme de première incidence s'étend ici à tout
$k$-ensemble, sans supposer déjà $H\in D_k$. En effet, soit
$Q=H\cup\lbrace x\rbrace$ un minimiseur. On a
$\beta(Q)>\beta(H)$. Si un outsider $y$ était strictement intérieur à $B_Q$,
$B_Q$ couvrirait $H\cup\lbrace y\rbrace$. L'égalité
$\beta(H\cup\lbrace y\rbrace)=\beta(Q)$ ferait de $B_Q$ son unique miniboule;
comme $y$ n'est pas sur sa frontière, un support positif correspondant serait
contenu dans $H$ et ferait aussi de $B_Q$ une miniboule de $H$, contradiction.
Ainsi $\beta(H\cup\lbrace y\rbrace)<\beta(Q)$, ce qui contredit la minimalité de
$Q$. Le minimiseur est donc de Gabriel ouverte et appartient à la source
terminale $\mathcal{G}_k$.
Par conséquent $H\in D_k$ et la descente termine encore sur $R=H$.

Le témoin est indispensable dans cette dernière branche : $J_H=\varnothing$
seul ne prouve pas que la première coface de $H$ arrive avant le cutoff $a$.

## 3. Théorème de résolution locale du carrier

**Théorème.** Sous les hypothèses du §1, toute facette témoin $(T,W_0)$ sous $a$
admet une descente canonique finie vers une facette $R(T,a)\in D_k$. Toutes les
étapes restent dans une même composante de $\Gamma_k(<a)$ et
$\beta(R(T,a))<a$. Par conséquent, le carrier strict de $T$ est exactement la
composante pré-lot qui contient la clef cœur $R(T,a)$.

**Preuve.** Tant que $\lvert J_H\rvert\geq2$, le §2.1 produit une facette $H'$
de niveau strictement plus petit, reliée à $H$ par une coface de niveau
$\beta(H)<a$, et conserve un témoin. La suite ne peut pas cycler : les niveaux
décroissent strictement et il n'existe qu'un nombre fini de $k$-sous-ensembles.
Elle atteint donc une branche terminale. Les §§2.2 et 2.3 montrent que cette
branche appartient à $D_k$, toujours avant $a$. Les choix par `PointId` et le
support unique rendent la suite canonique. Toutes les arêtes de descente sont
strictes, donc la composante antérieure au lot est conservée. $\square$

Le seul majorant général immédiat sur le nombre d'étapes est le nombre de
$k$-sous-ensembles. Le théorème donne la finitude et la localité scientifique,
aucune borne compatible avec le SLO 50 k.

### 3.1 Corollaire pour l'attache et la couverture

Le terminal $R_F$ remplace le token opaque
$\mathrm{Resolve}_{<a_F}(T_F)$. Au lot $a_F$, le fold exécute seulement
`find_<a_F(R_F)>`, puis attache $F$ à cette composante.

La descente seule certifie que $R_F$ et $T_F$ appartiennent à la même composante
stricte. L'enracinement de cette composante et la couverture ci-dessous viennent
du certificat **combiné** de l'attache, notamment des ponts construits avec les
deux intrus canoniques.

Cette composante couvre déjà $F\cup\lbrace z_F\rbrace$. Posons
$Q_F=F\cup\lbrace z_F\rbrace$. Pour chaque $u\in U_F$, le bras
$A_u=Q_F\setminus\lbrace u\rbrace$ et la coface
$C_u=A_u\cup\lbrace w_F\rbrace$ ont un niveau strictement inférieur à $a_F$.
Pour $u\neq v$, les deux cofaces partagent la facette
$(Q_F\setminus\lbrace u,v\rbrace)\cup\lbrace w_F\rbrace$; tous les $A_u$
appartiennent donc à une même composante stricte. Comme $\lvert U_F\rvert\geq2$,
leur union vaut $Q_F$. Le bras canonique $T_F=A_{u_F}$ descend jusqu'à $R_F$
dans cette même composante. L'attache ajoute ainsi la clef cœur $F$, mais aucun
point : `added_points=empty`.

Les cofaces intermédiaires de la descente certifient une équivalence dans
$\Gamma_k(<a_F)$; elles n'ont pas à être publiées. L'induction d'équivalence du
quotient réduit aux niveaux stricts autorise ensuite le lookup du seul terminal
cœur. Publier $T_F$ comme carrier latent, ou supposer qu'il appartient à $D_k$,
réintroduirait précisément la règle réfutée.

## 4. Ce qui reste réellement global

Le théorème remplace le record opaque `(F,a,T,Resolve(T))` par le record
`(F,a,R,certificat_de_descente)`, où $R\in D_k$ est une clef canonique. Il ne dit
pas quelle racine horizontale contient $R$ juste avant $a$.

Cette dernière réponse dépend de tout l'historique strict. Une continuation
$q_R=1$ peut attacher une facette à une composante sans créer de nœud public et
sans ajouter de point; la forêt publique et la seule couverture ne suffisent
donc pas à reconstruire le locator. Abstraitement, $m$ handles de cœur peuvent
porter une partition parmi $B_m$ partitions possibles. La représenter ou la
reconstruire demande au pire au moins $\log_2 B_m=\Theta(m\log m)$ bits
d'information.
C'est une borne du modèle du fold, pas l'affirmation que toute partition de Bell
est réalisée par une famille u16 particulière.

Il en résulte une frontière nette.

| décision | nature | état nécessaire |
| --- | --- | --- |
| choisir le parent de reverse search | locale au sommet | coquille, intérieur, prédicats exacts |
| descendre $T$ vers une clef cœur $R$ | locale, avec lecture certifiée de $X$ | miniboules, census, source directe terminale |
| savoir quelle composante contient $R$ avant $a$ | globale historique | partition ou journal exact des unions strictes |
| contracter le lot $a$ | globale par lot | toutes les clefs égales et état pré-lot gelé |

Une seule passe séquentielle à mémoire bornée, sans stockage adressable ni
relecture, ne peut répondre à une suite arbitraire de `find` sur cette partition.
Trois familles d'implémentation restent exactes : DSU résident, locator externe
à accès aléatoire, ou fold multipasse par tris, jointures et pointer-jumping.
Le multipasse est une conséquence du choix d'un stockage externe séquentiel, pas
une obligation si un DSU ou un locator externe adressable conserve l'information.

## 5. Fold externe sans locator résident

La descente locale permet de ne donner des handles qu'aux facettes cœur.

1. Chaque producteur d'attache émet `(ordre,a,F,R,certificat)` dans un run trié,
   sans résoudre la racine de $R$; les activations et cofaces directes suivent
   leurs propres records.
2. Le merger ferme tous les records de même niveau exact et forme le snapshot
   strict du lot.
3. Une jointure résout chaque handle $R$ dans un journal de versions de
   composantes, en suivant seulement les liens dont le niveau est strictement
   inférieur à $a$.
4. Le quotient complet du lot calcule les unions et les valeurs $q_R$ depuis ce
   même snapshot; aucune mutation partielle n'est publiée.
5. Les nouvelles versions et attaches sont ajoutées au journal append-only,
   puis pointer-jumping ou weighted-ancestor prépare les requêtes suivantes.

Ce schéma supprime la résidence du locator, pas son information. Une attache
différée doit engager le cutoff strict : résoudre contre l'état fermé au niveau
$a$ au lieu de l'état strictement antérieur peut changer une multifusion en
continuation.

Si $A$ est le nombre de descentes, $\ell_F$ leur nombre de pas et
$L=\sum_F\ell_F$, chaque descente visite $\ell_F+1$ facettes. Un scan complet du
nuage à chaque census donne donc un coût $O(n(A+L))$ au pire. Le chemin ou son
reçu demande $O(k(A+L))$ identifiants, plus les niveaux rationnels; l'état
courant de la descente peut rester $O(n)+O(k)$ hors index et runs. Si
$N_{\mathrm{ver}}$ désigne le nombre total de handles et versions de composante,
le fold conserve $O(N_{\mathrm{ver}})$ informations en RAM ou sur disque.
Aucune borne utile sur $L$ n'est démontrée.

## 6. Certificat minimal et portes de mutation

Un reçu `LocalStrictCarrierDescent` doit engager :

- $F$, $a_F$, $z_F$, $w_F$, $u_F$, le bras initial $T_F$ et le témoin
  $W_0=T_F\cup\lbrace w_F\rbrace$;
- pour chaque étape, $H$, $B_H$, $U_H$, la classe saturée de $J_H$, les choix
  $z_H,u_H$, $H'$, les comparaisons exactes de niveaux et la coface témoin;
- au terminal, soit la coface directe $H\cup\lbrace z\rbrace$, soit le groupe
  direct minimal de la branche vide et son niveau strictement inférieur à
  $a_F$;
- la clef cœur finale $R$, le digest de la source directe terminale, les preuves
  de régularité et le nombre d'étapes;
- zéro sortie si un budget, une primitive, une autorité ou un watermark manque.

Les mutations minimales sont : oublier le témoin de la branche vide; accepter
une baisse non stricte; choisir un point de support non essentiel; tolérer une
égalité extérieure; arrêter sur une facette non cœur; résoudre $R$ dans l'état
fermé au lieu de l'état strict; perdre une continuation $q_R=1$ du journal; et
publier un préfixe après budget moins un.

La fixture u16 à dix points de la note d'attache, dont les trois bras immédiats
sont hors de $D_3$, doit forcer au moins une vraie descente et être acceptée avec
un budget suffisant. Budget moins un ou autorité absente doivent refuser sans
payload.
La fixture rationnelle à sept points, mise à l'échelle u16 ci-dessous, possède
`F=016`, `T=126` et le chemin strict `126--1246--124--0124--024`, où les deux
cofaces intermédiaires ne sont pas directes. Elle réfute le lookup immédiat et
illustre un chemin strict passant par des relais non directs; elle ne borne pas
la longueur de la descente canonique, qui atteint ici une autre clef cœur en une
étape.

```text
(0,400,275) (600,100,324) (0,200,294) (600,600,271)
(500,0,276) (200,0,301) (300,600,320)
```

La fixture `E5`, à l'ordre deux, prend
`A=(0,0,7), B=(0,9,6), C=(1,4,0), D=(0,0,1), E=(4,1,2)`. Pour `F=AC`, le bras
`T=CD` vérifie $\beta(T)=9/2$, $J_T=\varnothing$, et conserve la coface témoin
`CDE` de niveau $162/25<\beta(F)=33/2$. Elle verrouille l'usage du témoin dans
la branche vide.
Une coquille antipodale à supports multiples doit refuser la baisse non stricte.
Un différentiel permanent doit ensuite comparer, à chaque coupe ouverte et
fermée, `tous M(F)`, `une attache avec Resolve complet` et `une attache vers R
puis find externe`.

## 7. Décision

- recherche d'un représentant cœur du carrier : **locale et prouvée sous la
  porte régulière**;
- locator pour les facettes non-cœur : **supprimable**;
- partition des facettes cœur avant le lot : **information globale
  irréductible, mais externalisable**;
- tri exact, fermeture atomique des ex æquo, couverture et verticales :
  **toujours globaux logiquement**;
- borne 50 k, mémoire et débit : **ouverts**.

Le parcours est local. Le carrier peut être ramené localement au cœur. Dans la
**résolution de ce carrier**, le seul `find` qui reste global est celui du cœur
dans l'histoire horizontale — et c'est
précisément l'information que le fold doit conserver.

GCP non utilisé.
