# Incidences silencieuses et correction du flot de Gabriel

> **Statut.** La réduction de Gamma aux seules composantes non triviales est exacte sans hypothèse de position générale. Le lemme plus fin qui remplace une coface non-Gabriel par des attaches silencieuses est démontré sous support essentiel unique et absence d'égalité extérieure. La découverte parcimonieuse de toutes les attaches utiles reste une obligation algorithmique distincte.

## 1. Référence exacte par Gamma

Soit un ensemble fini de sites $X$, un ordre $k$ et la valeur $\beta(F)$ du rayon carré de la miniball d'un label $F$. Pour $\prec\in\lbrace <,\leq\rbrace$, posons

$$E_k^{\prec}(a)=\left\lbrace Q\subseteq X:\lvert Q\rvert=k+1,\ \beta(Q)\prec a\right\rbrace.$$

L'ensemble des facettes incidentes à ces cofaces est

$$U_k^{\prec}(a)=\bigcup_{Q\in E_k^{\prec}(a)}\left\lbrace Q\setminus\lbrace x\rbrace:x\in Q\right\rbrace.$$

Pour chaque $Q\in E_k^{\prec}(a)$, notons

$$e_Q=\left\lbrace Q\setminus\lbrace x\rbrace:x\in Q\right\rbrace.$$

La monotonie de la miniball donne $\beta(F)\leq\beta(Q)$ pour toute facette $F\subset Q$. L'hypergraphe ayant $U_k^{\prec}(a)$ pour sommets et la famille $\left\lbrace e_Q:Q\in E_k^{\prec}(a)\right\rbrace$ pour hyperarêtes est donc un sous-hypergraphe bien défini de $\Gamma_k^{\prec}(a)$.

**Théorème 1 — réduction exacte des isolés.** Les composantes de cet hypergraphe sont exactement les composantes non triviales de $\Gamma_k^{\prec}(a)$, facette par facette, aux coupes ouvertes comme fermées.

**Preuve.** Une facette appartient à $U_k^{\prec}(a)$ si et seulement si elle est incidente à une coface active. C'est exactement la condition pour ne pas être isolée dans $\Gamma_k^{\prec}(a)$. Les deux hypergraphes ont ensuite les mêmes cofaces sur ce même ensemble de sommets. Ils induisent donc la même relation de connexité. Cette preuve ne demande ni généricité ni argument de Morse. $\square$

Le profil exact `hgp_reduced` de référence traite ainsi toutes les cofaces Gamma, mais ne crée aucune racine publique pour une facette isolée lorsque $k\geq2$. À l'ordre un, les singletons restent des racines normatives; traiter toutes les paires Gamma ou seulement un sous-graphe de Gabriel contenant un EMST produit les mêmes nœuds publics de fusion. Le certificat v2 conserve néanmoins toutes les cofaces Gamma. Pour $k=n>1$, aucune coface n'existe et la forêt réduite est vide.

À un niveau exact, les racines strictement antérieures sont figées, puis toutes les cofaces du niveau sont contractées dans un même hypergraphe temporaire. Si une composante temporaire rencontre $q$ racines antérieures, la règle est : naissance pour $q=0$, prolongement sans nœud pour $q=1$, multifusion unique pour $q\geq2$.

## 2. Lemme des attaches silencieuses

Le théorème précédent donne une référence exacte, mais il n'explique pas ce que le flot de Gabriel a oublié. Considérons une coface $Q$, avec $\lvert Q\rvert=k+1$ et $a=\beta(Q)$, qui n'est pas de Gabriel. Supposons que sa miniball possède un support minimal unique et essentiel $U$, que tous les points de $Q\setminus U$ soient strictement intérieurs, qu'aucun point de $X\setminus Q$ ne soit exactement sur sa frontière et qu'il existe un intrus $z\in X\setminus Q$ strictement intérieur. Posons $I=Q\setminus U$.

Pour chaque $u\in U$, définissons la facette stricte $F_u=Q\setminus\lbrace u\rbrace$ et la coface de remplacement $Q_u=F_u\cup\lbrace z\rbrace$. L'essentialité de $u$ et la marge intérieure de $z$ impliquent

$$\beta(Q_u)<\beta(Q)=a.$$

Puisque $F_u\subset Q_u$, on obtient aussi $\beta(F_u)<a$. En revanche, pour $x\in I$, la facette $Q\setminus\lbrace x\rbrace$ contient toujours $U$; les inégalités $\beta(U)\leq\beta(Q\setminus\lbrace x\rbrace)\leq\beta(Q)$ donnent donc $\beta(Q\setminus\lbrace x\rbrace)=a$. Les premières facettes sont strictement antérieures, les secondes naissent simultanément avec $Q$.

Pour $u\neq v$, les cofaces $Q_u$ et $Q_v$ partagent la facette

$$H_{u,v}=(Q\setminus\lbrace u,v\rbrace)\cup\lbrace z\rbrace.$$

Le chemin de facettes $F_u-H_{u,v}-F_v$ appartient donc à $\Gamma_k(<a)$, ses deux adjacences étant attestées respectivement par les cofaces $Q_u$ et $Q_v$. Toutes les facettes strictes $F_u$, $u\in U$, sont dans une même composante antérieure $C^{-}(Q)$. Comme un support essentiel contient au moins deux points distincts,

$$\bigcup_{u\in U}F_u=Q.$$

La composante $C^{-}(Q)$ couvre donc déjà tous les points de $Q$. Comme $\lvert U\rvert\geq2$, elle contient au moins deux facettes distinctes reliées avant $a$; elle est non triviale et possède déjà une unique racine publique réduite.

**Lemme 2 — coface non-Gabriel.** Sous les hypothèses précédentes, l'insertion de $Q$ rencontre exactement une racine antérieure, n'ajoute aucun point couvert et ne crée ni naissance ni fusion. Elle peut toutefois rattacher les facettes simultanées $Q\setminus\lbrace x\rbrace$, $x\in I$, à cette racine.

Autrement dit, une coface non-Gabriel est silencieuse pour la seule union de points, mais pas nécessairement pour l'ensemble des facettes. C'est l'étape manquante dans l'induction qui ne mémorisait que les unions de points.

## 3. Gabriel complété en incidences

Définissons $G_k^{+}$ avec les mêmes facettes que $\Gamma_k$, chacune née au niveau $\beta(F)$. Conservons chaque coface Gabriel comme hyperarête complète de poids $\beta(Q)$. Pour une coface non-Gabriel $Q$, choisissons canoniquement $u_0\in U$ et ajoutons au niveau $\beta(Q)$ seulement les attaches

$$Q\setminus\lbrace u_0\rbrace\longleftrightarrow Q\setminus\lbrace x\rbrace\qquad\text{pour tout }x\in I.$$

Les facettes obtenues en retirant un élément de $U$ sont déjà reliées strictement avant $a$ par le lemme 2. L'étoile précédente rattache exactement les facettes qui naissent simultanément avec $Q$.

**Théorème 3 — complétion silencieuse.** Sous les hypothèses du lemme 2 pour chaque coface non-Gabriel, $G_k^{+}$ et $\Gamma_k$ ont les mêmes composantes de facettes à toute coupe ouverte ou fermée.

**Preuve.** Procédons par induction sur les niveaux exacts. Les deux filtrations possèdent les mêmes facettes pondérées, et les cofaces Gabriel sont identiques. Pour une coface non-Gabriel de niveau $a$, le lemme 2 place toutes ses facettes strictes dans une même composante Gamma avant le lot; l'hypothèse d'induction transfère cette connexion pré-lot à $G_k^{+}$. L'étoile silencieuse rattache alors à cette composante toutes les facettes simultanées et induit exactement la même relation d'équivalence que l'hyperarête Gamma complète. La contraction atomique de toutes les cofaces de niveau $a$ rend l'argument indépendant de leur ordre d'énumération. $\square$

Ce théorème autorise une compression combinatoire lorsqu'on connaît déjà toutes les cofaces. Il ne prouve pas qu'un générateur sparse les découvre toutes. Le nom contractuel futur `incidence_complete_reduction_proved` reste donc réservé jusqu'à ce que la génération, les égalités de niveaux et les cas dégénérés possèdent leurs propres certificats.

## 4. Résolution exacte du cas à cinq points

La fixture permanente utilise

$$A=(0,0,7),\quad B=(0,9,6),\quad C=(1,4,0),\quad D=(0,0,1),\quad E=(4,1,2).$$

Elle est de dimension affine trois, satisfait `RelevantGP`, ne contient aucune dégénérescence exacte dans le domaine vérifié et possède une marge non nulle minimale égale à un.

| coface | nature | niveau carré | effet exact à l'ordre deux |
|---|---:|---:|---|
| $CDE$ | Gabriel | $162/25$ | crée la racine de facettes $CD,CE,DE$ |
| $ADE$ | Gabriel | $189/17$ | partage $DE$ et ajoute $AD,AE$ ainsi que le point $A$ |
| $ACD$, $ACE$ | non-Gabriel | $33/2$ | attachent silencieusement $AC$; aucun point ajouté |
| $ABC$ | Gabriel | $83886/3563$ | réutilise $AC$, ajoute $AB,BC$ et le seul point $B$ |
| $BCE$ | Gabriel | $24$ | lot $q=1$ dans Gamma : ajoute $BE$ sans point ni nœud; fusion tardive artificielle dans le flot brut |

Les deux cofaces silencieuses ont le même support $AC$, de centre $(1/2,2,7/2)$ et de rayon carré $33/2$. Dans la miniball de $ACD$, l'intrus $E$ est à la distance carrée $31/2$, soit une marge intérieure de un. Dans celle de $ACE$, l'intrus $D$ est à la distance carrée $21/2$, soit une marge intérieure de six.

Pour $ACD$, retirer $A$ et ajouter $E$ donne $CDE$, tandis que retirer $C$ et ajouter $E$ donne $ADE$ :

$$\beta(CDE)=\frac{162}{25}<\frac{33}{2},\qquad\beta(ADE)=\frac{189}{17}<\frac{33}{2}.$$

Les centres correspondants sont $(9/5,9/5,1)$ et $(24/17,6/17,4)$. Le chemin par leurs facettes communes prouve que $CD$ et $AD$ appartiennent déjà à la même racine. Le raisonnement symétrique pour $ACE$, avec l'intrus $D$, localise la même racine. Juste avant le lot $33/2$, cette composante possède les facettes

$$\lbrace AD,AE,CD,CE,DE\rbrace.$$

Le lot fermé lui ajoute la seule facette $AC$ et produit donc l'état post-lot $\lbrace AC,AD,AE,CD,CE,DE\rbrace$, sans modifier son union de points $\lbrace A,C,D,E\rbrace$.

Lorsque $ABC$ arrive, $AC$ est déjà dans cette racine; le lot a donc $q=1$. Il étend la composante avec $AB$ et $BC$ et ajoute seulement $B$. À la coupe fermée $83886/3563$, Gamma possède ainsi l'unique composante de facettes

$$\lbrace AB,AC,AD,AE,BC,CD,CE,DE\rbrace,$$

qui couvre les cinq points. Le flot Gabriel brut, privé de l'attache de $AC$, crée au contraire une seconde racine $ABC$ et ne la fusionne avec $ACDE$ qu'au niveau $24$ par $BCE$.

## 5. Localisation parcimonieuse à la demande

Pour une facette $F$, son premier niveau d'incidence est

$$\lambda(F)=\min_{x\in X\setminus F}\beta(F\cup\lbrace x\rbrace),$$

avec $\lambda(F)=+\infty$ si aucune coface n'existe. La facette est isolée avant $\lambda(F)$ et devient incidente dans la coupe fermée à ce niveau.

Une coface minimisante $Q=F\cup\lbrace x\rbrace$ peut localiser la racine de $F$. Si $Q$ est Gabriel, sa racine est directement connue. Sinon, tant que chaque descendant possède un support essentiel unique, un intrus strict et aucune transition de plateau, un remplacement intrus–support diminue strictement $\beta$ tout en conservant un chemin dans la même composante Gamma; l'itération finie termine alors sur une coface Gabriel. De plus, si $x$ appartenait au support essentiel d'un minimiseur non-Gabriel, le remplacement qui retire $x$ conserverait $F$ et produirait une coface de niveau plus petit, contradiction. Une première attache non-Gabriel est donc simultanée à la naissance de $F$ sous ces hypothèses.

Cette observation suggère de ne calculer $\lambda(F)$ que pour les facettes réutilisées par un futur lot Gabriel. Il s'agit d'une piste parcimonieuse, pas d'un théorème établi : il reste à prouver l'induction en présence de plusieurs minimiseurs au même niveau et de lots partageant des attaches silencieuses. Même une version suffisante pour la seule forêt des unions de points ne publierait pas toutes les facettes de Gamma sans un oracle total de rejeu.

## 6. Niveaux égaux, verticales et dégénérescences

Une descente stricte termine avant le lot cible; les facettes de même niveau restent dans l'hypergraphe temporaire du lot. Toute séquentialisation pourrait créer des fusions binaires artificielles et est interdite.

Pour une composante source d'ordre $k+1$, chacun de ses labels devient une coface à l'ordre $k$ et relie ses facettes au même niveau. Deux labels source adjacents partagent une facette; ils possèdent donc une cible Gamma unique. Les applications verticales du profil réduit sont les restrictions des applications Gamma, et leur naturalité découle de l'unicité de cette cible et des inclusions horizontales.

Si le support n'est pas essentiel, si un autre point est exactement sur la frontière ou si une transition conserve le niveau, le lemme 2 ne s'applique pas. La référence Gamma exhaustive reste exacte, mais la compression silencieuse doit soit contracter les plateaux par un quotient multivalué démontré, soit retourner `unsupported_degeneracy`.

## 7. Portée algorithmique

La complétion $G_k^{+}$ exhaustive inspecte jusqu'à $\binom{n}{k+1}$ cofaces. Une coface non-Gabriel émet au plus $\lvert I\rvert\leq k-1$ attaches silencieuses. Pour une version à la demande qui examine $p$ facettes-portes, une recherche naïve de $\lambda$ coûte $O(p(n-k))$ calculs de miniball; aucune borne pratique n'est encore démontrée pour la longueur maximale des descentes.

La priorité de l'implémentation scalable est donc un oracle spatial qui propose les minimiseurs de $\lambda$, suivi d'un branch-and-bound certifiant qu'aucune coface moins chère n'a été omise. Un bon résultat moyen ne remplace pas ce certificat.

## 8. Tests de falsification obligatoires

- rejouer le cas à cinq points et vérifier l'ajout de $AC$ avec `added_point_ids=[]` au niveau $33/2$;
- comparer les facettes, et pas seulement leurs unions de points, à toutes les coupes ouvertes et fermées;
- vérifier une facette née avant sa première coface et une facette née au même niveau;
- permuter toutes les cofaces d'un lot et conserver une multifusion canonique;
- tester un lot $q=1$ qui ajoute effectivement un point;
- vérifier l'ordre terminal vide et toutes les applications verticales;
- conserver Gamma exhaustif comme oracle différentiel tant que la génération parcimonieuse n'est pas prouvée.
