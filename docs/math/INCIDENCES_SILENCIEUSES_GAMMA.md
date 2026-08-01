# Incidences silencieuses et correction du flot de Gabriel

> **Statut.** La réduction de Gamma aux seules composantes non triviales est exacte sans hypothèse de position générale. Le lemme plus fin qui remplace une coface non-Gabriel par des attaches silencieuses est démontré sous support essentiel unique et absence d'égalité extérieure. Sous la même porte régulière globale, les cofaces directes et tous les co-minimiseurs de première incidence des facettes du cœur préservent désormais la forêt horizontale $H_0$ normalisée. La certification algorithmique de cette porte, la production complète de ces deux familles et la verticalité restent des obligations distinctes.

La [tour globale de boules saturées](TOUR_BOULES_SATUREES.md) donne désormais une seconde résolution exacte de cette perte : elle représente implicitement toute coface par le saturé de sa miniball, éventuellement de rang beaucoup plus élevé. Cette résolution globale ne supprime pas l'intérêt du locator parcimonieux ci-dessous dans le régime $K_{\max}=10$, car son énumération exhaustive brute conserve des saturés de tous les rangs jusqu'à $n$, même si leurs supports ont toujours une taille au plus quatre.

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

Cette observation suggère de ne calculer $\lambda(F)$ que pour les facettes réutilisées par un futur lot Gabriel. Elle ne fournit toujours pas une source facettée complète de Gamma. Les sections 5.2 et 5.3 ferment en revanche une conclusion conditionnelle pour la seule sémantique horizontale $H_0$ normalisée : sous une porte de régularité globale, les co-minimiseurs, les plateaux silencieux réguliers et les lots partageant des attaches se rétractent sur le cœur direct, puis toute coface tardive de l'étoile devient un no-op. Cette conclusion ne publie ni toutes les facettes, ni le `coverage_log` v2 exhaustif.

### 5.1 Falsification exacte de l'étoile une-étape

La fixture permanente [`moment_curve_k3_one_step_star_incompleteness_n9`](../../morsehgp3d/tests/unit/fixtures/moment_curve_k3_one_step_star_incompleteness_n9.hpp) prend neuf points de la courbe des moments,

$$p_i=(10\,000i,i^2,i^3),\qquad 0\leq i\leq8.$$

À l'ordre $k=3$, l'oracle exact borné certifie seulement les six familles directes $0123$, $1234$, $2345$, $3456$, $4567$ et $5678$. Leurs dix-neuf suppressions distinctes sont exactement les triplets dont la portée des indices est au plus trois. La facette $F=048$ n'appartient pas à cette frontière. Sa miniball possède le support essentiel unique $08$ et le niveau exact

$$\beta(048)=1\,600\,066\,560.$$

Les points $1,2,3,5,6,7$ sont tous strictement intérieurs à cette boule et aucun point extérieur n'est sur son shell. Les six cofaces $048x$, pour $x\in\lbrace1,2,3,5,6,7\rbrace$, conservent le support $08$, sont non-Gabriel et attachent silencieusement $048$ dans $\Gamma_3$. Pourtant aucune de leurs quatre facettes n'est une suppression directe : chaque facette candidate contient deux indices parmi $0,4,8$ dont l'écart est au moins quatre. Le compte exact est donc six cofaces cibles et zéro coface visible depuis l'étoile une-étape.

Tous les quadruplets de la fixture sont affinement indépendants, avec un déterminant absolu minimal de $120\,000$. L'échec n'est donc ni un plateau, ni une cosphéricité, ni une autre dégénérescence exclue par le lemme 2 : il provient d'un saturé de rang neuf invisible depuis la frontière directe de rang quatre.

Cette fixture réfute toute assertion `product_sparse_silent_source_complete` fondée sur la seule fermeture une-étape des suppressions directes. Elle ne prouve pas, à elle seule, une différence des seules unions de points publiques : ses six cofaces sont des continuations $q=1$ à delta de points nul. Une source Gamma facettée exacte doit soit porter implicitement le générateur saturé et son bloc de Johnson, soit découvrir les facettes requises par une worklist à la demande munie de certificats de terminaison et d'exclusion. Récursivement énumérer toutes les suppressions nouvelles reconstruirait Gamma et reste interdit comme architecture produit.

### 5.2 Théorème conditionnel de rétraction sur le cœur direct

Fixons $2\leq k<n$. Soit $D_k$ l'ensemble global des facettes des cofaces Gabriel de cardinal $k+1$, et soit $\mathrm{Star}(D_k)$ la famille de toutes les cofaces incidentes à au moins une facette de $D_k$. Le mot *global* est essentiel : une facette silencieuse peut n'être reconnue comme utile que parce qu'une coface Gabriel strictement future la réutilise.

La conclusion suivante suppose simultanément :

- un support minimal unique et essentiel pour chaque facette et coface visitée ;
- tous les sommets considérés hors support strictement intérieurs et aucun point extérieur du nuage sur la miniball correspondante ;
- les catalogues Gabriel complets aux cardinalités $k$ et $k+1$ ;
- des requêtes top-$k$ exactes, complètes, sans arrêt budgétaire, et la correspondance usuelle entre composantes des sous-niveaux de $D_k$ et composantes de Gamma ;
- l'énumération exhaustive de $\mathrm{Star}(D_k)$, tous les co-minimiseurs conservés, puis un quotient atomique de chaque niveau exact sur un snapshot strictement pré-lot ;
- des carriers latents qui conservent la facette complète et sa couverture, même en l'absence de racine réduite publique.

**Théorème 4 — rétraction $H_0$ relative.** Sous ces hypothèses, l'intersection avec le cœur direct induit, à toute coupe ouverte ou fermée, une bijection entre les composantes non triviales de Gamma et les composantes de carriers du flot formé par les cofaces directes, $\mathrm{Star}(D_k)$ et les descentes strictes demandées. Cette bijection préserve l'union des points. À chaque lot, elle préserve $q_R$, `added_point_ids` et les parents sous la bijection inductive des racines. Elle ne préserve pas la famille exhaustive de facettes comme payload publié.

La preuve se décompose sans hypothèse de fidélité des carriers prise comme prémisse.

1. **Apex silencieux.** Le lemme 2 associe à chaque coface non-Gabriel régulière $Q$ une unique composante antérieure non triviale $P(Q)$. Elle contient toutes les facettes strictes de $Q$ et couvre déjà tous ses points.
2. **Confluence d'un plateau silencieux.** Soient $Q=E\cup\lbrace x\rbrace$ et $Q'=E\cup\lbrace y\rbrace$ deux cofaces non-Gabriel de même niveau $a$. Si $\beta(E)<a$, leur facette commune appartient déjà aux deux composantes antérieures. Si $\beta(E)=a$, les trois miniballs coïncident, $x$ et $y$ sont strictement intérieurs, et les supports uniques coïncident en un support $U\subset E$. Pour tout $u\in U$, la coface $R=(E\setminus\lbrace u\rbrace)\cup\lbrace x,y\rbrace$ vérifie $\beta(R)<a$ et relie les facettes strictes $Q\setminus\lbrace u\rbrace$ et $Q'\setminus\lbrace u\rbrace$. Ainsi $P(Q)=P(Q')$.
3. **Terminal direct.** Une facette régulière non terminale pour le top-$k$ remplace au moins un point essentiel par des points strictement intérieurs ; son niveau décroît strictement. La descente finie termine donc sur une facette Gabriel $M$ de la même composante Gamma sous le niveau source. Comme $k<n$, choisir une extension $M\cup\lbrace x\rbrace$ de niveau minimal montre que $M\in D_k$ : si cette extension était non-Gabriel, $x$ serait essentiel et son remplacement par un intrus strict produirait une extension de niveau plus petit.
4. **Élimination des chaînes hors étoile.** Après contraction de la coupe stricte, toute composante connexe de cofaces non-Gabriel de même niveau est un cône sur son unique apex $P$. Un contact par une facette stricte passe déjà par $P$ ; un contact par une facette égale avec une autre coface non-Gabriel conserve le même $P$ par l'étape 2 ; un contact par une facette d'une coface directe place cette coface dans $\mathrm{Star}(D_k)$. Une chaîne sans facette directe interne et extérieure à l'étoile ne modifie donc aucune équivalence entre les hyperarêtes retenues. Si tout son groupe est omis, il s'agit d'une continuation $q_R=1$, à delta de points nul et sans nœud.
5. **Induction par lots.** Une descente stricte associe chaque bras retenu à son carrier direct dans la même composante Gamma stricte. L'hypothèse d'induction classe ce carrier comme enraciné si et seulement si cette composante est non triviale, sinon comme latent. L'étape 4 donne alors une bijection des groupes qui contiennent une coface directe et l'identité de leurs racines antérieures. Une coface non-Gabriel n'ajoute aucun point. Pour une coface directe $Q$ de support $U$, l'identité $\bigcup_{u\in U}(Q\setminus\lbrace u\rbrace)=Q$, avec $\lvert U\rvert\geq2$, montre que ses bras stricts couvrent tous ses points. Les groupes ont donc les mêmes $q_R$, parents et deltas de points, puis l'invariant est préservé au lot suivant.

Le schéma abstrait qui montre pourquoi chaque hypothèse de régularité est indispensable possède deux racines strictes $R_1,R_2$, une facette égale nouvelle $F$ et deux hyperarêtes $e_1=\lbrace R_1,F\rbrace$, $e_2=\lbrace F,R_2\rbrace$. Chacune paraît isolément être une continuation $q=1$, mais leur quotient atomique donne une multifusion $q_R=2$. Le lemme de confluence interdit cette géométrie dans le domaine régulier. Un support multiple, un extra-shell ou une transition top-$k$ non stricte rouvre exactement ce schéma ; ces branches exigent un quotient de plateau démontré ou `unsupported_degeneracy`.

### 5.3 Réduction conditionnelle aux premières incidences du cœur

Notons $\mathcal{G}_k$ la famille des cofaces directes de cardinal $k+1$. Pour chaque $F\in D_k$, notons $M(F)$ la famille complète de ses cofaces minimisant $\lambda(F)$, puis posons le sous-flot candidat

$$\mathcal{A}_k=\mathcal{G}_k\cup\bigcup_{F\in D_k}M(F).$$

La porte régulière est exactement celle du théorème 4. Elle s'applique à toutes les facettes et cofaces nécessaires à l'étoile, pas seulement aux objets effectivement publiés par $\mathcal{A}_k$. Le remplacement porte uniquement sur la famille de cofaces : les descentes strictes et le resolver de carriers du théorème 4 restent disponibles dans les deux flots, sans exiger la publication de leurs facettes non-cœur. Pour une facette $F$, il faut distinguer ses intrus globaux étrangers

$$J_F=\bigl(B_F^{\circ}\cap X\bigr)\setminus F,$$

et les points de $F$ qui n'appartiennent pas à son support. Ces deux ensembles n'ont pas la même fonction et ne doivent pas partager le même compteur logiciel.

**Corollaire 4.1 — suffisance horizontale des premières incidences.** Sous les hypothèses du théorème 4, remplacer $\mathrm{Star}(D_k)$ par $\mathcal{A}_k$ conserve, à toute coupe ouverte ou fermée, les composantes ancrées dans $D_k$, leurs carriers latents et leurs unions de points. Après normalisation des continuations omises, les lots conservent $q_R$, `added_point_ids`, les parents et les nœuds de la forêt réduite $H_0$. Dans chaque composante du quotient exhaustif d'un lot exact, les cofaces de $\mathrm{Star}(D_k)\setminus\mathcal{A}_k$ ont l'une des deux formes suivantes : une composante purement omise est une continuation $q_R=1$ sans nouvelle facette du cœur, sans nouveau point, sans parent supplémentaire et sans nœud; une partie omise incidente à une composante retenue ne change ni son $q_R$, ni ses deltas, ni ses parents, ni son nœud.

**Preuve.** Commençons par la famille $M(F)$. Si $\lvert J_F\rvert=0$, l'absence d'extra-shell implique que la boule minimale $B_F$ ne contient aucun point étranger, donc $\lambda(F)>\beta(F)$. Dans un minimiseur non-Gabriel $Q=F\cup\lbrace x\rbrace$, le point $x$ doit appartenir au support essentiel : sinon le support serait contenu dans $F$ et donnerait $\beta(Q)=\beta(F)$, contradiction. Le remplacement strict de $x$ par un intrus de $Q$ conserverait alors $F$ et produirait une coface incidente de niveau inférieur, en contradiction avec la définition de $\lambda(F)$. Tous les éléments de $M(F)$ sont donc directs et déjà dans $\mathcal{G}_k$. Si $\lvert J_F\rvert=1$, avec $J_F=\lbrace z\rbrace$, toute extension de rayon $\beta(F)$ a pour boule minimale l'unique boule $B_F$ de $F$; l'absence d'extra-shell donne $B_F\cap X=F\cup\lbrace z\rbrace$. Ainsi $\lambda(F)=\beta(F)$ et $M(F)=\lbrace F\cup\lbrace z\rbrace\rbrace$; cette unique coface est directe. Si $\lvert J_F\rvert\geq2$, on a encore $\lambda(F)=\beta(F)$ et $M(F)=\lbrace F\cup\lbrace z\rbrace:z\in J_F\rbrace$. Chaque coface est non-Gabriel parce qu'un autre élément de $J_F$ demeure strictement intérieur. Elles partagent la facette égale $F$ et confluent vers un unique apex antérieur par l'étape 2 du théorème 4. Cette sous-famille installe $F$ et n'introduit aucune racine au-delà de cet apex ni aucun point nouveau; sa classification se fait toutefois avec toute la composante atomique du lot, qui peut aussi contenir une coface directe. Les trois cas excluent notamment un mélange direct--non-direct dans un même $M(F)$.

Considérons maintenant une coface tardive $Q\in\mathrm{Star}(D_k)\setminus\mathcal{A}_k$ de niveau $a$. Pour toute facette de cœur $F\in D_k$ contenue dans $Q$, l'exclusion de $M(F)$ donne $\lambda(F)<a$; toutes les facettes de cœur de $Q$, et pas seulement une facette témoin, sont donc déjà présentes strictement. La coface $Q$ n'est pas directe. Le lemme 2 place toutes ses facettes strictes dans un unique apex $P(Q)$, qui contient en particulier chaque facette de cœur de $Q$ et couvre déjà tous les points de $Q$.

Il reste à fermer le quotient de niveau égal, car une analyse coface par coface ne suffirait pas. Deux cofaces non directes qui partagent une facette $E$ avec $\beta(E)=a$ ont le même apex par la confluence du théorème 4. Une coface directe $R$ ne peut pas partager une telle facette égale avec $Q$ : l'unicité de la boule minimale de $E$ et les égalités $\beta(E)=\beta(Q)=\beta(R)$ imposent $B_E=B_Q=B_R$; la régularité de $E$ exclut un sommet étranger sur sa frontière, donc le sommet de $Q\setminus R$ est strictement intérieur à $B_R$, en contradiction avec le caractère direct de $R$ et son rang fermé $k+1$. Tout contact d'une composante omise avec une coface directe passe donc par une facette stricte déjà dans l'apex.

Raisonnons maintenant par composante du quotient atomique exhaustif au niveau $a$. Les contacts égaux non-directs et les contacts stricts imposent le même apex à toute partie omise connexe. Si cette partie ne touche aucune coface retenue, son unique racine antérieure est cet apex; toutes ses facettes du cœur et tous ses points sont déjà couverts strictement. Elle est donc une continuation pure $q_R=1$, sans delta de cœur ou de points et sans nœud. Si elle touche une ou plusieurs cofaces retenues, chaque contact apporte le même apex. Deux morceaux retenus ainsi touchés partagent donc déjà ce jeton enraciné et appartiennent à la même composante retenue avant ajout des cofaces omises. Celles-ci n'ajoutent alors ni racine, ni facette du cœur, ni point : leur retrait conserve le $q_R$, l'ensemble des parents, les deltas et le nœud éventuel de la composante retenue. Cette dichotomie couvre en particulier les lots mixtes direct--résiduel, qui ne sont pas nécessairement des continuations.

L'induction porte simultanément sur les composantes ancrées dans $D_k$, les carriers latents et les unions de points, et compare chaque lot exact complet plutôt que ses cofaces isolément. Les familles $M(F)$ installent les attaches éventuellement nécessaires; la dichotomie précédente retire simultanément les cofaces tardives sans changer aucune composante retenue; les cofaces directes sont présentes dans les deux flots. Les racines publiques, $q_R$, les parents, les deltas de points et les nœuds suivent par induction de la coupe stricte à la coupe fermée, puis au niveau exact suivant. $\square$

Ce corollaire ferme l'obligation mathématique horizontale `Star` vers `directes + premières incidences` sous la porte régulière globale. Il ne certifie pas que le producteur a énuméré toutes les cofaces directes, toutes les facettes du cœur et tous les $M(F)$, ni que la porte régulière vaut pour les cofaces non matérialisées. Il ne suffit donc pas à positionner `incidence_complete_reduction_proved=true`. La verticalité demande en outre un certificat croisé entre ordres : elle s'obtient conditionnellement en conjuguant la verticale Gamma par les bijections de cœur et en prouvant l'indépendance du représentant source au moyen de la facette commune d'ordre $k$; le différentiel horizontal actuel ne rejoue pas ce carré.

#### 5.3.1 Inertie $H_0$ exacte des blocs saturés au-dessus de la fenêtre de rang

La régularité géométrique globale du corollaire 4.1 est plus forte que ce qui est nécessaire pour exclure les boules déjà au-dessus de tous les ordres demandés. Le lemme suivant remplace, pour ces seules boules, une fermeture du shell par une preuve combinatoire directe. Il accepte des supports multiples et des extra-shells arbitraires; il ne les rebaptise jamais « réguliers ».

Soit $B$ une boule fermée de centre $c$, de niveau carré $a>0$, et soit $U$ un support minimal positif de $B$, affinement indépendant, de cardinal $2\leq s\leq4$. Notons

$$I=X\cap B^{\circ},\qquad E=X\cap\partial B,\qquad S=I\cup E,\qquad p=\lvert I\rvert,\qquad s=\lvert U\rvert.$$

Pour un ordre $k$, notons $G_k^{<a}(S)$ le graphe dont les sommets sont les $k$-sous-ensembles $Q\subseteq S$ tels que $\beta(Q)<a$, deux sommets étant adjacents lorsque leur union possède $k+1$ points et reste de niveau strictement inférieur à $a$.

**Théorème 4.2 — inertie saturée de haut rang.** Si

$$1\leq k\leq p+s-2,$$

alors $G_k^{<a}(S)$ est non vide, connexe et l'union de ses sommets vaut $S$. L'activation fermée du bloc de Johnson $J_k(S)$ au niveau $a$ est donc une continuation $H_0$ avec une unique composante antérieure, sans fusion et sans delta de couverture.

**Preuve.** Pour $Q\subseteq S$, posons $A=Q\cap E$. La boule $B$ est admissible pour $Q$. Si $\beta(Q)=a$, l'unicité de la miniball impose que sa boule minimale soit $B$, donc sa condition d'optimalité donne $c\in\mathrm{conv}(A)$. Réciproquement, si $c\in\mathrm{conv}(A)$, la condition d'optimalité des points de $A$ situés sur la sphère montre que $B$ est leur miniball, puis celle de $Q$. Ainsi

$$\beta(Q)=a\Longleftrightarrow c\in\mathrm{conv}(A),\qquad \beta(Q)<a\Longleftrightarrow c\notin\mathrm{conv}(A).$$

Appelons **acyclique** un sous-ensemble $A\subseteq E$ tel que $c\notin\mathrm{conv}(A)$. Ajouter un point de $I$ à un ensemble strict ne change pas sa partie frontière et conserve donc la stricte inégalité. Si $k\leq p$, tout sommet strict se relie par échanges successifs à un $k$-sous-ensemble fixé de $I$; chaque union intermédiaire reste stricte. Un point de $E$ appartient à un sommet strict en le complétant par $k-1$ points de $I$, tandis que tout point intérieur appartient au sommet fixé ou à un échange de celui-ci. Connexité et couverture en découlent dans ce premier cas.

Supposons désormais $k>p$ et posons $q=k-p$. L'hypothèse donne $1\leq q\leq s-2$. Tout sommet strict se relie d'abord, par ajout des points intérieurs manquants, à un sommet $I\cup A$ où $A\subseteq E$ est acyclique et $\lvert A\rvert=q$. Il suffit donc d'étudier le graphe des $q$-sous-ensembles acycliques de $E$, avec une arête lorsque leur union acyclique possède $q+1$ points.

Si $A$ n'est pas inclus dans $U$, il existe $u\in U\setminus A$ tel que $A\cup\lbrace u\rbrace$ soit acyclique. Sinon, pour chaque $u\in U\setminus A$, une dépendance positive de $A\cup\lbrace u\rbrace$ placerait $u-c$ dans l'espace engendré par $A-c$. Les éléments de $U\cap A$ y sont déjà, donc tout $U-c$ serait contenu dans cet espace, en contradiction avec

$$\dim\mathrm{span}(U-c)=s-1>q\geq\dim\mathrm{span}(A-c).$$

Retirer ensuite un élément de $A\setminus U$ donne un voisin acyclique qui possède un élément de plus dans $U$. L'itération atteint un $q$-sous-ensemble de $U$. Tous les $q$-sous-ensembles de $U$ sont reliés entre eux, car l'union de deux voisins dans le graphe de Johnson est un sous-ensemble propre de $U$, donc reste acyclique par minimalité positive de $U$. Le graphe strict est connexe.

Enfin, tout point de $E$ appartient à un $q$-sous-ensemble acyclique : à partir de ce point, on ajoute successivement un élément de $U$ hors de l'espace déjà engendré, ce qui est possible tant que la taille reste au plus $s-2$. Les points de $I$ appartiennent à tous les sommets $I\cup A$. La couverture vaut donc $S$.

Le graphe fermé $J_k(S)$ est connexe et contient le graphe strict précédent. Tous ses nouveaux sommets s'attachent ainsi à une seule composante déjà présente, dont la couverture contient déjà $S$. Deux boules distinctes activées au même niveau ne peuvent contourner cet argument : un $k$-sous-ensemble commun nouvellement actif aurait les deux boules comme miniball, que l'unicité rendrait égales; tout autre contact commun est strictement antérieur. Le lot simultané ne crée donc aucune fusion cachée entre deux blocs ainsi exclus. $\square$

**Corollaire 4.3 — autorité de fenêtre de rang.** Posons

$$r_{\max}=\min(K_{\mathrm{eff}}+1,n).$$

Si $p+s>r_{\max}$, alors soit $r_{\max}=n$ et ce cas est impossible, soit $p+s\geq K_{\mathrm{eff}}+2$ et le théorème 4.2 rend le bloc saturé $H_0$-inerte pour tous les ordres demandés. Les seuils d'intérieurs stricts sont donc $K_{\mathrm{eff}}$ pour un support de taille deux, $\max(K_{\mathrm{eff}}-1,0)$ pour une taille trois et $\max(K_{\mathrm{eff}}-2,0)$ pour une taille quatre.

Cette conclusion se compose exactement avec la façade terminale des supports sous les conditions suivantes : les univers de supports minimaux de tailles deux à quatre sont fermés; toute boule vérifiant $p+s\leq r_{\max}$ a subi une requête fermée complète; tout shell strictement plus grand que son support a produit un diagnostic; et le nombre de diagnostics pertinents est nul. Les candidats non minimaux sont couverts par leur support minimal d'arité inférieure. Les boules de la fenêtre ont alors un shell égal à leur support, tandis que les boules omises sont exclues par le corollaire 4.3. Un reçu de prune isolé, une absence locale de diagnostic ou un flux non terminal ne satisfait pas cette composition.

La fixture permanente [`higher_rank_prune_does_not_certify_star_regularity.json`](../../tests/fixtures/regressions/higher_rank_prune_does_not_certify_star_regularity.json) distingue précisément les deux affirmations. Pour la boule de support `AB`, on a $s=2$, $p=10$, $K_{\mathrm{eff}}=10$ et $r_{\max}=11$, donc $p+s=12=K_{\mathrm{eff}}+2$. Le point `Y` sur le shell prouve que cette boule est **géométriquement irrégulière** et que le prune ne ferme pas sa coque; le théorème 4.2 prouve néanmoins que son bloc saturé est **$H_0$-inerte** aux ordres un à dix. Ce verdict concerne ce bloc de haut rang : toute autre extra-shell de la même entrée qui resterait dans la fenêtre doit encore produire `unsupported_rank_relevant_extra_shell_degeneracy`.

L'autorité correspondante porte donc explicitement `hidden_above_window_extra_shells_explicitly_permitted=true` et `geometric_global_regularity_claimed=false`. Une extra-shell pertinente, $p+s\leq r_{\max}$, échoue ouverte avec `unsupported_rank_relevant_extra_shell_degeneracy`; elle n'est ni supprimée ni envoyée vers le lemme de haut rang. Cette autorité ne certifie que les composantes horizontales $H_0$ jusqu'à $K_{\mathrm{eff}}$. Elle ne prouve ni la complétude des cofaces directes et des premières incidences, ni le quotient de tout plateau pertinent, ni la verticalité, ni la naturalité, ni les identités v2 ou v3, ni `incidence_complete_reduction_proved`, ni `forest_semantics=exact`, ni `public_status=exact`. Elle ne construit aucun `Star`, Gamma global ou Delaunay d'ordre supérieur et n'établit aucune borne de temps pour terminer les flux de supports.

### 5.4 Validation bornée et obstruction du contrat v2

Un différentiel exact en arithmétique rationnelle, avec supports énumérés jusqu'à quatre points, a comparé Gamma exhaustif au cœur direct et à son étoile pour $k=2,3$. Les familles déterministes sont les courbes des moments d'échelles $10\,000$ pour $5\leq n\leq9$ et $1$ pour $5\leq n\leq8$, E5, la fixture silencieuse à cinq points et deux familles zigzag de sept et huit points. À chaque coupe stricte et fermée, l'audit a vérifié : cœur direct non vide pour chaque composante non triviale, connexion de ce cœur dans l'étoile, même couverture de points et même nombre de composantes. Aucun contre-exemple n'a été trouvé ; il s'agit d'une falsification bornée, pas d'un remplacement de la preuve conditionnelle.

Sur `moment10000`, $n=9$, $k=3$, l'oracle contient $\binom{9}{4}=126$ cofaces, dont six directes. Le cœur possède dix-neuf facettes, son étoile soixante-dix-sept cofaces et aucune égalité extérieure pertinente. Les quatre invariants précédents restent vrais malgré les six cofaces `048x` invisibles depuis l'étoile une-étape. Les tests ciblés `ExactDirectMorseGammaCarrierConformance` et `ExactDirectSparseFacetDescentClosure` passent séparément ; ils restent des validations logicielles relatives.

Le différentiel borné `ExactDirectStarH0RetractionDifferential` ajoute un second objet de falsification : les six cofaces directes réunies à tous les co-minimiseurs de première incidence des dix-neuf facettes du cœur. Sur cette fixture, ces co-minimiseurs ne rajoutent aucune coface aux six directes, tandis que soixante-et-onze cofaces de l'étoile deviennent tardives. Le premier lot physiquement absent du sous-flot se trouve au niveau exact $400001088=1600004352/4$ et contient `0124`, `0134`, `0234`, de support diamétral `04`. Gamma lui attribue $q_R=1$, une continuation, aucun delta de facette du cœur, aucun `added_point_ids` et aucun nœud. Exiger un lot candidat matériel produit donc un faux négatif : l'état strict et l'état fermé sont déjà identiques, et l'absence du lot est un no-op certifié pour la forêt $H_0$ abstraite.

Ces fixtures servent de falsificateurs du corollaire 4.1 sans le remplacer. La preuve conditionnelle établit désormais $\mathrm{Star}(D_k)$ vers `directes + co-minimiseurs`, mais le différentiel reste borné à $4\leq n\leq9$ et $k\in\lbrace2,3\rbrace$, avec Gamma high-cut transitoire comme oracle. Ni la complétude algorithmique du producteur, ni sa porte régulière globale, ni le certificat vertical croisé, ni `incidence_complete_reduction_proved`, ni une porte 50 k ou massive n'en découlent. Même si les partitions et la généalogie réduite coïncident sur une fixture acceptée, `EqualLevelBatch.batch_id` engage toujours le payload exhaustif : `contract_v2_batch_identity_equivalence_claimed` et `contract_v2_forest_identity_equivalence_claimed` restent faux.

La fixture permanente [`higher_rank_prune_does_not_certify_star_regularity.json`](../../tests/fixtures/regressions/higher_rank_prune_does_not_certify_star_regularity.json) ferme en outre une fausse composition de certificats. Le triangle direct `ACD` est régulier, sa facette de cœur `AC` a pour première incidence unique `D` au niveau carré 25, puis la coface tardive `ACB` appartient à son étoile et possède le support essentiel `AB` au niveau 1024. Dix points sont strictement intérieurs avant que `Y` ne soit rencontré exactement sur le shell. Un cutoff de rang fermé maximal onze peut donc certifier correctement que cette branche est hors fenêtre après les douze premiers points fermés, sans jamais observer `Y`. L'absence de diagnostic extra-shell dans un produit ainsi pruné ne certifie ni la fermeture de son shell, ni la régularité globale exigée par le corollaire 4.1. Le checker rationnel recalcule la miniball directe, tous les candidats de première incidence de `AC`, la miniball tardive, ses dix intérieurs et l'égalité extérieure; cette contradiction interdit de recycler un reçu de prune de rang comme autorité de régularité. Elle ne contredit pas le théorème 4.2 : celui-ci conserve explicitement l'irrégularité de la boule `AB` et certifie seulement l'inertie $H_0$ de son bloc saturé au-dessus de la fenêtre.

Cette rétraction $H_0$ ne satisfait pas le contrat public v2. Les identifiants denses de l'histoire Gamma et de la forêt directe sont locaux ; les identifiants publics sont des SHA-256 de contenu. En particulier, `EqualLevelBatch.batch_id` engage la liste exhaustive `gamma_coface_ids`, tandis que les snapshots pré-lot et post-lot ainsi que le `coverage_log` engagent toutes les facettes et tous les `added_facet_point_ids`. `MergeNode.node_id` engage à son tour `batch_id`, et l'identité de la forêt engage ses nœuds et son journal. Omettre les six cofaces `048x` conserve la sémantique $H_0$ conditionnelle et les points, mais omet le lot facetté et l'ajout de `048` ; les identifiants v2 ne peuvent donc pas coïncider.

Deux voies seulement restent compatibles avec une future publication exacte : diffuser exhaustivement Gamma sous forme streamée avec ses digests, snapshots et deltas, sans matérialiser une mosaïque de Delaunay d'ordre supérieur, ou adopter une nouvelle révision contractuelle qui définisse et certifie l'identité du quotient $H_0$. Le contrat normalisé v3 suit la seconde voie, mais sa porte reste fermée jusqu'aux certificats de complétude du producteur et des premières incidences, au traitement fail-open des extra-shells pertinentes, puis aux certificats de quotient, de verticalité et de naturalité. Le nom `incidence_complete_reduction_proved` reste réservé et absent du schéma actif. Le présent théorème de fenêtre ne ferme ni `forest_semantics=exact`, ni `public_status=exact`.

## 6. Niveaux égaux, verticales et dégénérescences

Une descente stricte termine avant le lot cible; les facettes de même niveau restent dans l'hypergraphe temporaire du lot. Toute séquentialisation pourrait créer des fusions binaires artificielles et est interdite.

Pour une composante source d'ordre $k+1$, chacun de ses labels devient une coface à l'ordre $k$ et relie ses facettes au même niveau. Deux labels source adjacents partagent une facette; ils possèdent donc une cible Gamma unique. Les applications verticales du profil réduit sont les restrictions des applications Gamma, et leur naturalité découle de l'unicité de cette cible et des inclusions horizontales.

Si le support n'est pas essentiel, si un autre point est exactement sur la frontière ou si une transition conserve le niveau, le lemme 2 ne s'applique pas. La référence Gamma exhaustive reste exacte, mais la compression silencieuse doit soit contracter les plateaux par un quotient multivalué démontré, soit retourner `unsupported_degeneracy`.

### 6.1 Carré vertical normalisé local

Une fois les composantes horizontales normalisées fournies par une autorité séparée, leur application verticale ne demande pas de reconstruire Gamma. Considérons une composante fermée $C$ d'ordre $k+1$ avec sa liste exhaustive de labels et un arbre couvrant d'adjacences. Pour chaque arête de l'arbre, les deux labels ont une union égale à une coface source active et une intersection $F$ de cardinal $k$. Si toutes ces facettes communes se résolvent vers la même racine fermée $R$ à l'ordre $k$ et à la même coupe exacte, la connexité de l'arbre prouve que le choix du label source ne change pas l'image verticale de $C$.

Pour une inclusion horizontale certifiée $C_a\to C_b$ entre deux coupes fermées, les labels de $C_a$ doivent tous persister dans $C_b$. La facette témoin canonique $F_a$ de l'arbre de $C_a$ est alors re-résolue à la coupe $b$, après composition attestée de tous les niveaux cibles intermédiaires. Si sa racine obtenue est l'image verticale indépendante de $C_b$, le carré commute. Le raisonnement couvre une continuation $q_R=1$ : aucune création de nœud n'est nécessaire, seulement l'unicité du successeur horizontal et la re-résolution du même carrier.

`ExactDirectNormalizedVerticalSquareReceipt` implémente ce certificat local avec budgets, échecs atomiques et vérificateur frais. Il est conditionnel aux reçus exhaustifs de membership source, de spanning tree, de résolution cible et d'inclusion horizontale que le plan candidat actuel ne produit pas encore. Il ne positionne donc ni `all_naturality_squares_replayed`, ni `vertical_maps_complete`, ni `incidence_complete_reduction_proved`, ni `global_regularity_authority_certified`. Ses arènes ne contiennent que les labels et arêtes d'une composante, les bindings de facettes demandés, deux images et un carré; elles ne matérialisent aucune étoile, Gamma ou mosaïque de Delaunay d'ordre supérieur.

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
