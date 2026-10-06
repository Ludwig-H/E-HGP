# Mathématiques du moteur v11

Contrat mathématique de développement, 2 octobre 2026. Ce document fixe les objets à calculer ; il ne
qualifie pas leur implémentation. Il condense L01–L03 et le brouillon privé, avec les corrections
[Q1–Q5 acceptées](../audits/AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md).
Les décisions d'implantation et leurs portes sont dans [CONCEPTION_MOTEUR.md](CONCEPTION_MOTEUR.md) ; le § 10 (4 octobre 2026) fixe la sortie `supports`.
Cadre : `phase=exploration_v11_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u18_input_only`, `public_status=not_claimed`.
Les preuves ci-dessous concernent des sites distincts, de poids un, dans l'espace euclidien de dimension
trois. Les formules rationnelles supposent des coordonnées entières. Les profils numériques sont ceux
d'[ARCHITECTURE.md](ARCHITECTURE.md), sans qualification implicite d'un profil supplémentaire.

## 1. Objets et conventions

Soit $X$ un ensemble fini de $n$ sites distincts. Pour une partie non vide $F$, $B(F)$ est sa plus petite
boule englobante, $c(F)$ son centre et $\beta(F)$ son **rayon au carré**. Les coupes utilisent toujours
ce niveau carré $a$. L'ordre public de la tour vérifie $1 \leq k \leq K \leq n$.

Une boule critique positive $b=(c,\lambda)$ est la plus petite boule d'une partie d'au moins deux sites.
On note

$$ I_b=\{x\in X:\|x-c\|^2<\lambda\},\quad U_b=\{x\in X:\|x-c\|^2=\lambda\},\quad P_b=I_b\cup U_b,\quad p=|I_b|,\quad m=|U_b|. $$

Un support est une partie affinement indépendante de $U_b$ dont le centre appartient à l'intérieur
relatif de l'enveloppe convexe. Son cardinal minimal est $q=q_{\min}(b)\in\{2,3,4\}$.
Le support canonique $S^*(b)$ est le premier, pour l'ordre lexicographique déclaré sur les `SiteIdx`,
parmi les supports de cardinal $q$. L'ordre sert l'encodage déterministe ; il ne crée pas une invariance
géométrique par rotation ou échange d'axes. Les singletons de niveau zéro sont traités séparément.

**M1 — plus petite boule.** $B(F)$ existe et est unique. Une boule contenant $F$ est $B(F)$ si et
seulement si son centre appartient à l'enveloppe convexe des sites de $F$ sur sa frontière.

*Preuve.* La fonction $y\mapsto\max_{x\in F}\|y-x\|^2$ est continue et coercive. Deux centres
distincts optimaux sont impossibles : l'identité du milieu diminue strictement toutes les distances
carrées. Si $c=\sum_i t_i x_i$, $t_i\geq0$, $\sum_i t_i=1$, avec tous les $x_i$ sur la frontière,
alors $\sum_i t_i\|x_i-y\|^2=\lambda+\|c-y\|^2$ ; aucun autre centre ne réduit le maximum.
Inversement, si $c$ est hors de cette enveloppe convexe, une séparation stricte donne une direction qui
réduit simultanément les distances aux sites de frontière ; un déplacement assez petit conserve les
inégalités strictes des autres sites. Enfin, un sous-ensemble minimal portant cette combinaison est
affinement indépendant, donc a au plus quatre sites. Un seul site ne porte qu'un rayon nul.

**M2 — certificat combinatoire.** Si $S$ est un support certifié de $b$ et
$S\subseteq F\subseteq P_b$, alors $B(F)=b$, par M1. Le seul test $F\subseteq P_b$ ne suffit pas.
Ce certificat autorise une proposition rapide de boule, à condition de vérifier les deux inclusions
et de disposer d'un recensement exact de $P_b$.

## 2. Primitives exactes

Pour un support ancré en $a$, écrire $c=a+N/D$, avec $D>0$. Les formules sont :

| Cardinal | Vecteurs | $N$ | $D$ |
| --- | --- | --- | --- |
| 2 | $u=b-a$ | $u$ | $2$ |
| 3 | $u=b-a$, $v=d-a$, $w=u\times v$ | $(\|u\|^2v-\|v\|^2u)\times w$ | $2\|w\|^2$ |
| 4 | $u=b-a$, $v=d-a$, $z=e-a$ | $\|u\|^2(v\times z)+\|v\|^2(z\times u)+\|z\|^2(u\times v)$ | $2\det(u,v,z)$ |

Un déterminant nul refuse ce support ; pour le cardinal quatre, changer les signes de $N,D$ si
nécessaire. Le cardinal trois exige un triangle strictement aigu ; le cardinal quatre exige quatre
coordonnées barycentriques strictement positives. Un poids nul appartient à une présentation de
cardinal inférieur, pas à un support minimal de cardinal quatre. Un triplet utilisé pour **chercher**
un tétraèdre n'a, lui, aucune obligation d'être aigu.

Le signe de

$$ H_b(y)=D\|y-a\|^2-2N\mathbin{\cdot}(y-a) $$

est négatif à l'intérieur, nul sur la frontière et positif à l'extérieur. Le rayon carré vaut
$\|N\|^2/D^2$ ; pour un triangle, la forme réduite en degré
$\|u\|^2\|v\|^2\|u-v\|^2/(4\|u\times v\|^2)$ évite de grossir inutilement les produits.
Des rationnels non réduits conviennent : dénominateur positif et comparaison par produits croisés
exacts. L'égalité de niveaux est mathématique, jamais celle de deux approximations flottantes.

Chaque expression effectivement implantée doit avoir son propre budget de bits, y compris ses sommes,
ses coefficients et ses produits croisés. Une preuve pour une expression algébriquement simplifiée
ne borne pas nécessairement les intermédiaires de l'expression évaluée. Les filtres flottants F2–F6
restent soumis aux preuves de domaine d'ARCHITECTURE ; ils ne remplacent aucune des décisions ci-dessus.

## 3. Catalogue et boîtes de centres

Le catalogue utile à la connectivité jusqu'à l'ordre $K$ est

$$ \mathrm{Cat}_K=\{b\text{ critique positive}:p+q\leq K+1\}. $$

Sa fenêtre possible d'événements est $[p+q-1,p+m]\cap[1,K]$. L'admission ne se décide pas sur un
support quelconque lorsque la même boule possède un support plus petit : le recensement de coquille
doit permettre de retrouver $q$ et $S^*$.

**G1 — invariant de liste.** Une boîte demi-ouverte $Q=\prod_i[l_i,h_i)$ porte une liste $L\subseteq X$
contenant tous les $K$ plus proches voisins **avec tous les ex æquo** de tout centre de $\overline{Q}$.
Pour un ordre de diagnostic $K>n$, cette liste de voisins est $X$.

Pour deux sites distincts, $z\prec_Q x$ signifie que $z$ est strictement plus proche que $x$ en tout
point de $\overline{Q}$. Avec $s_i=h_i-l_i$, le critère exact est

$$ \|x-l\|^2-\|z-l\|^2>\sum_i\max(0,2s_i(x_i-z_i)). $$

Il résulte du minimum sur la boîte de la différence affine des distances carrées. L'égalité conserve
le candidat. Retirer $x$ lorsqu'il a au moins $K$ dominateurs distincts préserve G1 : ces $K$ sites
précèdent strictement $x$ à chaque centre. Les témoins peuvent provenir de n'importe quelle partie de
$X$, même s'ils ne sont plus candidats ; leur identité et leur distinction doivent rester vérifiables.

**G2 — recensement local exact dans le domaine utile.** Si $c\in Q$ et le vrai nombre d'intérieurs de
sa boule est $p<K$, alors $P_b\subseteq L$. Si $p\geq K$, $L$ contient au moins $K$ intérieurs.
En effet, dans le premier cas toute la boule fermée précède ou égale le $K$-ième voisin ; dans le second,
les $K$ premiers voisins sont strictement intérieurs. Donc un recensement achevé sur $L$ qui trouve
moins de $K$ intérieurs est globalement exact, coquille entière comprise.

**G3 — élagage d'un support partiel.** Poser $\theta_r=K+1-r$ pour $r=2,3,4$. Tout site dominant au
moins un sommet d'un support au centre de sa boule est strictement intérieur à cette boule ; les
sommets du support ne se dominent pas entre eux. L'union des dominateurs fournit donc un minorant de
$p$, sans compter deux fois un témoin. Un support partiel de cardinal $r$ peut être rejeté si ce minorant
est **strictement supérieur** à $\theta_r$. Aucun préfixe de $S^*$ d'une boule admise ne l'est :
$p\leq\theta_q\leq\theta_r$ pour $r\leq q$.
Si $\theta_r<0$, aucun support de cardinal $r$ ou supérieur ne peut être admis ; ne pas calculer ce
seuil par une soustraction non protégée dans un type non signé.

**G4 — propriété et complétude conditionnelle.** Partir d'une boîte contenant $\mathrm{conv}(X)$,
avec borne supérieure strictement au-delà des coordonnées maximales. Une subdivision en boîtes
demi-ouvertes disjointes donne un propriétaire unique à chaque centre. On peut intersecter une boîte
avec $\prod_i[\min L_i,\max L_i+\delta)$, pour $\delta>0$ exactement représentable dans la grille
de boîtes : M1 place tout centre critique encore pertinent dans $\mathrm{conv}(U_b)\subseteq\mathrm{bbox}(L)$.
Il faut conserver les égalités aux maxima, les largeurs positives et la progression de la subdivision.

Sur une partition finale **finie et entièrement traitée**, si chaque feuille énumère tous les supports
restants, applique les prédicats exacts, recense la coquille entière et ne publie que le support canonique
dans sa boîte propriétaire, la sortie est exactement $\mathrm{Cat}_K$. G1–G3 préservent le support
canonique de chaque boule admise ; son centre a un unique propriétaire ; la canonicalisation retire
ses autres présentations. Cette preuve ne démontre ni la terminaison d'une politique arbitraire de
subdivision, ni une borne sous-quadratique du nombre de candidats ou de boules.

Un plafond de mémoire ou de travail produit un refus explicite, jamais une sortie dite complète.
Tout élagage supplémentaire doit citer un lemme couvrant son domaine réel ; une bonne mesure n'est
pas un argument de complétude.

## 4. Définition de FULL

Pour $|F|=k$, soit $W_F(a)=\bigcap_{x\in F}\overline{B}(x,\sqrt{a})$ et
$L_k(a)=\bigcup_{|F|=k}W_F(a)$. Le graphe $\Gamma_k(a)$ a pour sommets les $k$-parties avec
$\beta(F)\leq a$. Chaque $(k+1)$-partie $G$ avec $\beta(G)\leq a$ relie toutes ses faces de
cardinal $k$. Le graphe strict $\Gamma_k^{<}(a)$ remplace ces deux tests par $<a$.
Pour la région stricte, utiliser les boules **ouvertes** :
$W_F^{<}(a)=\bigcap_{x\in F}B^\circ(x,\sqrt{a})$ et
$L_k^{<}(a)=\bigcup_{|F|=k}W_F^{<}(a)=\bigcup_{0\leq a'<a}L_k(a')$ pour $a>0$ ;
à $a=0$, elle est vide. Sélectionner les $W_F(a)$ fermés par le seul test $\beta(F)<a$ ne donnerait
pas cette région : des contacts tangentiels au niveau $a$ pourraient relier des composantes strictes.

**T1 — modèle fini et verticales.** Les composantes de $\Gamma_k(a)$ et de $L_k(a)$ sont en bijection,
compatible avec les inclusions de niveaux. Chaque $W_F$ non vide est convexe. Deux de ces ensembles
s'intersectent exactement lorsque $\beta(F\cup F')\leq a$ ; les échanges d'un élément entre
$k$-parties de cette union donnent un chemin utilisant seulement des $(k+1)$-parties au même seuil.
Le graphe d'intersection et $\Gamma_k$ ont donc les mêmes composantes. Pour les ouverts,
$W_F^{<}(a)\cap W_{F'}^{<}(a)\ne\varnothing$ équivaut à $\beta(F\cup F')<a$, d'où la même
preuve aux coupes strictes. Une réunion finie de ces convexes compacts, ou de ces convexes ouverts,
a précisément les composantes de son graphe d'intersection.

L'inclusion $L_k(a)\subseteq L_{k-1}(a)$ donne l'application verticale. Dans le graphe, choisir une
face quelconque de cardinal $k-1$ d'un sommet $F$ donne la même composante : toutes ces faces sont
reliées par $F$. Cette application respecte les arêtes et commute aux augmentations de niveau.

FULL contient, pour chaque ordre, la forêt des composantes par niveaux, ainsi que ces applications
verticales. Une fusion simultanée est un nœud N-aire ; aucun parent n'a le même niveau que son enfant.
Pour $k\leq n$, la forêt a une racine unique à grand niveau. À poids un, seules les naissances d'ordre
un sont au niveau zéro.

## 5. Cellules locales et passage au global

Pour une boule critique positive, appeler $A\subseteq U_b$ **séparable** lorsque
$c\notin\mathrm{conv}(A)$ ; l'ensemble vide est séparable. Par séparation stricte, cela équivaut à
l'existence d'une direction ayant un produit scalaire strictement positif avec chaque $x-c$, $x\in A$.

**T2 — trace stricte.** Pour $F\subseteq P_b$, $\beta(F)<\lambda$ si et seulement si $F\cap U_b$
est séparable. M1 prouve le sens non séparable. Dans l'autre sens, déplacer légèrement le centre selon
un séparateur diminue toutes les distances de frontière et conserve celles des intérieurs.

Si $p\geq k$, toutes les $k$-parties strictes de $P_b$ sont reliées strictement : ajouter des sites
intérieurs par échanges les ramène à des $k$-parties de $I_b$. Si $p<k$, poser $t=k-p$. On comprime
toute partie stricte en $I_b\cup A$, où $A$ est une $t$-partie de sa trace. Les échanges gardent une
trace séparable. Les **morceaux locaux** sont alors les composantes du graphe des $t$-parties séparables,
deux sommets étant reliés si leur union est séparable. Une union peut aussi être parcourue par échanges
Johnson ; cette définition donne les composantes du graphe strict **induit sur les parties de $P_b$**.

L'inclusion dans le graphe global donne une **surjection** des morceaux locaux vers les composantes
strictes globales qu'ils rencontrent. Des chemins extérieurs à $P_b$ peuvent identifier plusieurs
morceaux. Un représentant par morceau suffit ; une partition exhaustive de chaque morceau, avec un
représentant par sous-bloc non vide, suffit aussi. Un sous-échantillon sans preuve de couverture ne
suffit pas. Les racines globales doivent être dédupliquées avant de compter les enfants d'une fusion.

**T3 — événement d'une boule à l'ordre $k$.** Si $|P_b|<k$, rien n'apparaît. Si $|P_b|=k$, l'unique
partie $P_b$ naît isolément. Si $|P_b|\geq k+1$, toutes les $k$-parties de $P_b$ sont connectées au
seuil fermé $\lambda$ ; les composantes strictes rencontrées sont réunies, et les nouveaux sommets
s'y attachent. Sans partie stricte, c'est une naissance. Avec une seule composante stricte, aucun nœud
de fusion n'est créé, même si la population couverte augmente.

Toute partie non séparable contient un support et a au moins $q$ sites. Pour $1\leq t\leq q-2$,
toutes les $t$-parties et leurs échanges sont donc séparables : un seul morceau, aucun événement de
connectivité. Cela justifie la fenêtre $[p+q-1,p+m]$ et le catalogue du § 3. Si $m=q$, les seules
cellules avec événement sont la naissance à $k=p+q$ et la jonction des $q$ morceaux à $k=p+q-1$.
Une coquille étendue ne doit pas être traitée comme une coquille régulière.

**T4 — plateau atomique.** À niveau fixé, construire le graphe biparti entre composantes strictes
globales et événements qui les touchent. Chaque composante connexe réunissant au moins deux anciennes
composantes donne une multifusion avec exactement ces enfants. Les naissances du niveau restent
isolées à ce niveau : une arête incidente à un sommet nouveau $F$ d'une naissance $b$ est une partie
contenant $F$ de même rayon minimal ; l'unicité de la MEB impose encore $b$. Ses autres faces restent
donc dans $P_b$. De proche en proche, aucun chemin du plateau ne quitte cette population ni n'atteint
un sommet strict, puisqu'une naissance n'en contient pas.
On peut alternativement contracter tous les événements binaires adjacents de même niveau, à condition
de prouver qu'ils représentent exactement ce même plateau avant publication.

## 6. Descente, mémo et verticales

**T5 — descente valide.** Partir d'une $k$-partie $F$ et calculer $b=B(F)$. Si $p\geq k$, choisir
une $k$-partie de $I_b$. Sinon, si une $t$-partie séparable $A\subseteq U_b$ existe, choisir
$I_b\cup A$. Sinon arrêter à la naissance. Chaque pas diminue strictement $\beta$ par T2 et reste
dans la composante de $F$ à la coupe fermée $\beta(F)$ par les échanges dans $P_b$. La finitude des
$k$-parties prouve la terminaison, avec au plus $\binom{n}{k}-1$ pas : cette borne n'est pas un résultat
de performance exploitable.

Le terminal peut dépendre du choix. Sur $X=\{0,2,4\}$, $k=2$, la paire extrême peut descendre vers
l'une ou l'autre paire voisine. Les deux terminaux représentent la même classe à tout niveau
$a\geq\beta(F)$, sans être identiques avant ce niveau. Une descente est une fonction pure seulement
après fixation d'une politique déterministe.

Un mémo de cellule $(b,k)$ est valable à la coupe fermée $a\geq\lambda_b$, ou à la coupe ouverte
$a>\lambda_b$. Il ne peut identifier les morceaux distincts avant $\lambda_b$. Pour résoudre un
représentant strict $R$ d'un événement de niveau $\lambda$, un raccourci doit justifier
$\lambda_b\leq\beta(R)<\lambda$. Des pointeurs de cellules vers des cellules de niveau strictement
inférieur donnent un autre schéma possible ; leur acyclicité et leur date d'utilisation font partie
de son contrat.

**T6 — suffisance constructive, conditionnelle.** Un catalogue complet, des populations exactes,
des représentants couvrant chaque composante stricte rencontrée, des descentes valides et T4 suffisent
à reconstruire FULL. Par récurrence sur les niveaux, T3 donne toutes les modifications, T5 retrouve
leurs anciennes composantes et T4 effectue exactement les unions simultanées. Ce théorème n'accorde
aucune qualification à un générateur, une heuristique de représentants ou un cache non vérifiés.

Pour une naissance d'ordre $k$, toutes les faces de cardinal $k-1$ de sa population sont connectées
à son niveau. Résoudre une face puis remonter au nœud vivant de la **coupe fermée** donne son image
verticale. Pour une fusion, remonter les images de tous ses enfants à son niveau doit donner un même
nœud ; cette égalité est un invariant contrôlable. Un nœud vivant à $a$ vérifie
$a_v\leq a<a_{\mathrm{parent}(v)}$, ou n'a pas de parent.

## 7. De FULL aux points : deux entrées différentes

La composante des centres n'est pas d'emblée un groupe exclusif de sites. Séparer la filtration
géométrique, sa projection sur les sites, puis la condensation et la sélection.

**P1 — core.** $D_k(x)$ est la distance carrée au $k$-ième voisin de $x$, lui-même compris.
Le site $x$ entre à $D_k(x)$ dans l'unique composante de $L_k(D_k(x))$ contenant le centre $x$.
Toutes les $k$-parties de la boule fermée de centre $x$ au seuil $D_k(x)$ sont connectées et donnent
cette même composante ; les ex æquo ne peuvent modifier l'attache.

**P2 — cover ensembliste.** Définir

$$ A_k(x)=\min_{F\ni x,\ |F|=k}\beta(F),\qquad E_k(x)=\{C\in\pi_0(L_k(A_k(x))):C\text{ couvre }x\}. $$

Ici « couvre » signifie qu'il existe $y\in C$ avec $\|x-y\|^2\leq A_k(x)$.
$E_k(x)$ est un ensemble non vide de composantes à la coupe fermée, pas nécessairement un singleton.
Pour tout site, $D_k(x)/4\leq A_k(x)\leq D_k(x)$ : une partie de rayon carré $A$ contenant $x$
a tous ses sites à distance au plus $2\sqrt{A}$ de $x$ ; inversement la boule centrée en $x$ au
$k$-ième voisin contient une $k$-partie incluant $x$.

**P3 — témoins suffisants pour cover, $k\geq2$.** Appeler forte une cellule vérifiant
$p+q\leq k\leq p+m$. Toute première entrée $A_k(x)$ possède un témoin fort $b$ contenant $x$ ;
$E_k(x)$ est exactement l'ensemble des composantes fermées des témoins de ce niveau contenant $x$.

*Preuve.* Une partie $F\ni x$ de niveau minimal donne une boule critique $b$ contenant au moins $k$
sites. Si $k<p+q$, on peut choisir une autre $k$-partie contenant $x$, avec tous les intérieurs
nécessaires et moins de $q$ sites de coquille ; sa trace est séparable, donc son niveau est strictement
plus petit, contradiction. Réciproquement toute boule contenant $x$ et $k$ sites fournit une telle
partie, donc a un niveau au moins $A_k(x)$. Si une composante couvre $x$ à cette date, choisir une
$k$-partie contenant $x$ dans la boule centrée en un témoin de couverture donne une MEB de même niveau,
dans cette composante. Le raisonnement fonctionne aussi en minimisant parmi les parties d'une
composante donnée à une coupe ultérieure : la couverture complète est l'union des populations des
cellules fortes qu'elle contient. En coquille régulière, une cellule forte est une naissance.

Les continuations étendues peuvent augmenter la couverture sans créer de nœud. Conserver une relation
**boule forte → nœud vivant** ; l'union des seules populations de naissance ne suffit pas en général.

**P4 — projection laminaire à ordre fixé.** Fixer $k$. Pour une règle qui attache chaque site une seule fois à un nœud vivant
à sa date d'entrée, les ensembles de sites attachés aux sous-arbres sont laminaires : deux sous-arbres
sont inclus ou disjoints. Cela prouve la laminarité, pas la qualité statistique de la règle. Les sites
inactifs restent explicitement absents, ou deviennent des singletons selon une convention séparée.

La réunion des familles de différents ordres n'est pas nécessairement laminaire. Sur l'axe,
$X=\{0,10,11,26,27,45,46\}$ donne en core un groupe statique $\{0,10,11\}$ à K1
(naissance 25, parent 225/4) et $\{10,11,26,27\}$ à K2 (naissance 64, parent 361/4).
Le site 0 n'entre core à K2 qu'au niveau 100, après ce parent : les descendants statiques se croisent
même après toutes les attaches. Une hiérarchie commune ne peut conserver les deux groupes exacts.
Le [témoin de référence](../reference/test_projection_contracts.py) vérifie groupes et parents dans les
deux étages ; il reprend explicitement le [contre-exemple indépendant](../receipts/audit_independant_20261002/cross_order_contract_review_3/README.md).
Cela ne contredit pas les inclusions verticales à une coupe fixée.

`core` fournit une telle règle. Pour `cover`, publier d'abord $A_k(x)$ et tout $E_k(x)$.
Choisir un membre par ordre lexicographique est une convention de repère. Il n'existe pas de choix
singleton équivariant sous toutes les isométries : dans $X=\{0,2,4\}$ à l'ordre deux, la réflexion
fixe le site médian et échange ses deux propriétaires possibles. Le premier ancêtre commun de
$E_k(x)$ est une autre projection, datée du maximum entre $A_k(x)$ et la naissance de cet ancêtre ;
elle peut retarder l'entrée et ne se confond pas avec P2.

**P5 — stabilité de FULL en rayon, portée de la projection.** Apparier tous les sites de
$X=(x_i)$ et $Y=(y_i)$ dans un même repère, avec $\max_i\|x_i-y_i\|\leq\varepsilon$.
Pour un centre quelconque $z$, soit $\rho_k^X(z)$ sa distance au $k$-ième site de $X$.
Chaque distance bouge d'au plus $\varepsilon$, donc leur $k$-ième ordre aussi. Ainsi
$L_k^X(r^2)\subseteq L_k^Y((r+\varepsilon)^2)$, et réciproquement. Une composante s'envoie dans
l'unique composante contenant son image ; les compositions sont les inclusions à $r+2\varepsilon$,
et ces cartes commutent avec les verticales. C'est une stabilité par entrelacement en **rayon**,
sans bijection promise entre boules critiques, supports ou nœuds.

La couverture dynamique à tous les rayons se transporte aussi : si $z\in C\cap\overline{B}(x_i,r)$,
le même témoin appartient à l'image de $C$ et à $\overline{B}(y_i,r+\varepsilon)$. De plus,
$\alpha_i^X=\sqrt{A_k^X(x_i)}=\min_z\max(\|z-x_i\|,\rho_k^X(z))$ ; continuité et coercivité
assurent le minimum. Les deux termes, leur maximum puis leur minimum bougent d'au plus
$\varepsilon$, donc $|\alpha_i^X-\alpha_i^Y|\leq\varepsilon$. Pour les niveaux carrés, la borne
est $|A_k^X(x_i)-A_k^Y(y_i)|\leq\varepsilon(\alpha_i^X+\alpha_i^Y)$, sans erreur additive
uniforme $\varepsilon$. L'argument continu compte aussi des retours répétés appariés, sans
qualifier le port d'une tour pondérée. Une grille de pas $h$, arrondie au plus proche sans clipping,
donne $\varepsilon\leq\sqrt{3}h/2$ après restitution de l'origine et des multiplicités.

**L'ensemble figé au premier instant et sa projection LCA n'héritent pas de cette stabilité.**
Sur $X=\{0,2,4\}$ à l'ordre deux, le site médian entre à rayon 1 dans deux composantes ; son LCA
est la racine, de rayon 2. Sur $Y=\{0,2,4+\delta\}$ pour tout $\delta>0$, il entre encore à 1,
mais dans la composante gauche seule : son LCA est daté 1. Les naissances et la fusion de FULL se
déplacent d'au plus $\delta/2$ en rayon, tandis que la date projetée saute de 1. Laminarité et
équivariance ne suffisent donc pas à une stabilité sous perturbation, ni à celle des masses ou labels.

Ce témoin, établi par l'audit `2e5ca6e12`, devient la quatrième fixture de
[test_projection_contracts.py](../reference/test_projection_contracts.py) : les deux routes de l'oracle
doivent rendre les ensembles du site médian $\{0,1\}$ puis $\{0\}$, sur les entrées entières
$(0,2s,4s)$ et $(0,2s,4s+1)$ pour $s=1,1000$, et les niveaux LCA $4s^2$ puis $s^2$.
Avec pas physique $1/s$, le déplacement est $1/s$ et le saut de rayon projeté reste 1.
Les portes de référence normal/−O passent sur G4 à `a97180667` ; leur portée reste celle
de ces deux étages et de ces fixtures, sans qualification d'un futur moteur FULL natif.

Ni P4 ni une maturité fondée sur une masse recouvrante ne garantissent un nombre minimal de membres
**exclusifs** après projection. La cible « deux triangles distincts avant fusion » reste une porte
de choix de projection, pas une conséquence automatique de FULL. Aucune équivalence générale avec
une hiérarchie de mutual reachability, ni optimalité du clustering final, n'est démontrée ici.

## 8. Juges et limites

**J1 — restriction.** Pour $K'\geq K$, filtrer $\mathrm{Cat}_{K'}$ par $p+q\leq K+1$ redonne
$\mathrm{Cat}_K$, après recalcul des rangs. C'est une porte utile entre deux exécutions différentes.

**J2 — ordre un.** Les composantes de $\Gamma_1(a)$ sont celles d'un arbre couvrant euclidien minimal
coupé aux arêtes de longueur carrée au plus $4a$. Pour toute arête du graphe complet, le chemin de
l'arbre entre ses extrémités ne contient aucune arête plus lourde, sinon un échange réduit son poids.
Comparer la structure N-aire aux plateaux, pas seulement le multiensemble des longueurs.

**J3 — Euler, diagnostic nécessaire.** Pour une boule critique, poser $t=k-p$ et, si $1\leq t\leq m$,

$$ e_k(b)=\sum_{A\subseteq U_b,\ c\in\mathrm{conv}(A),\ |A|\geq t} (-1)^{|A|-t}\binom{|A|-1}{t-1}; $$

sinon poser $e_k(b)=0$. Alors, en sommant sur toutes les boules critiques,
$n[k=1]+\sum_b e_k(b)=1$. C'est l'identité
$\sum_{s=k}^n(-1)^{s-k}\binom{n}{s}\binom{s-1}{k-1}=1$, regroupée par MEB des parties :
la somme sur les sous-parties intérieures est une différence finie d'ordre $p$, laissant le terme
ci-dessus. Pour une coquille régulière, $e_k=(-1)^{q-t}\binom{q-1}{t-1}$.

Pour tester tous les ordres jusqu'à $K$, $\mathrm{Cat}_{K+2}$ suffit puisque $q\leq4$ ;
$\mathrm{Cat}_K$ ne suffit pas en général aux deux derniers ordres. Deux omissions peuvent se
compenser, même niveau par niveau : à $k=1$, les sites $(0,5,0),(8,9,0),(8,1,0),(35,5,0),(45,5,0)$
portent au niveau 25 un triangle de contribution $+1$ et une paire de contribution $-1$.
Les omettre ensemble conserve Euler et change une fusion. Ce juge n'est jamais un certificat de
complétude du catalogue.

Les invariants racine unique, parents strictement plus hauts, attaches vivantes, verticales cohérentes,
neutralité des politiques de descente et déterminisme complètent les oracles bornés ; aucun isolément
ne démontre l'exactitude générale d'une implantation.

## 9. Domaine restant ouvert

- **Multiplicités.** `cloud` conserve les retours et leurs poids ; le présent contrat moteur reste à
  poids un. Le brouillon privé propose un modèle par copies, mais son transfert complet, les naissances
  nulles pondérées, les fenêtres non contiguës et le quotient local compact demandent une relecture et
  une implantation distinctes. Tant que cela manque, refus explicite d'entrée pondérée au plus tôt ;
  aucun dédoublonnage silencieux ne change la population de référence.
- **Coquilles étendues.** L'énumération exhaustive est une définition bornée de référence. Les quotients
  par arrangements de directions sont une optimisation proposée ; leurs cas dégénérés et leur coût
  demandent leurs propres preuves et portes avant de remplacer cette référence.
- **Coût.** Les preuves de couverture, de terminaison de descente et de laminarité ne donnent ni borne
  globale sous-quadratique ni temps cible sur une trame. Compter candidats, populations, incidences,
  visites d'index, sorties et mémoire coexistante ; certaines familles ont déjà une sortie quadratique.
- **Tête et qualité.** Condensation, stabilité, règle EOM/leaf, traitement des égalités, poids et racine
  constituent un contrat supplémentaire. Aucune « meilleure » hiérarchie ou sélection universelle
  n'est définie par les seuls théorèmes précédents.

Les preuves algébriques et combinatoires résumées ici sont séparées des tests de leurs futurs ports.
La [synthèse d'audit](AUDIT_V10_SYNTHESE.md) date les sources lues, les résultats hérités et les lacunes.

## 10. Hiérarchie des supports d'ordre K

Ajout du 4 octobre 2026 (Claude, développeur) : contrat S0 de la sortie `supports`. Cadre :
`phase=exploration_v11_hors_registre`, `backend=cpu_reference`, `profile=quantized_u21_input_only`,
`public_status=not_claimed`. Ce paragraphe fixe l'objet publié ; il ne qualifie aucune implantation. Il suit les
décisions de l'utilisateur du 4 octobre 2026 (squelette $\mathcal{Q}_b$ sans populations, compte `kparties_reliees`)
et les gardes de l'[audit de conception](../receipts/audit_supports_20261004/README.md) du commit `1bf4be68f`. Le
format est décrit dans [SORTIES.md](SORTIES.md). Hypothèses du § 1 : sites distincts, de poids un ; aucune position
générale n'est supposée. La thèse est citée par ses pages imprimées. Le code n'est cité que comme réalisation, jamais
dans une preuve.

### 10.1 Vocabulaire

- Une **$K$-partie** est une partie de $K$ sites : un $(K-1)$-simplexe de la thèse, un sommet de $\Gamma_K$.
- Une **liaison**, au sens de la thèse, est une $(K+1)$-partie $G$ : un $K$-simplexe, arête élémentaire de
  $\Gamma_K$ au niveau $\beta(G)$, qui relie toutes ses $K$-faces (Prop. 5, p. 86). Les comptes dérivés la nomment
  `cofaces`.
- Ce que l'utilisateur appelle les « liaisons » d'un support sont les **$K$-parties que sa boule relie** : toutes les
  $K$-parties de $P_b$. Ce sont exactement les $K$-parties $F$ dont la région témoin $W_F(\lambda_b)$ contient le
  centre $c_b$, puisque $c_b\in W_F(\lambda_b)$ si et seulement si $F\subseteq P_b$ (lemme A). Leur nombre s'appelle
  `kparties_reliees` ; c'est le compte mis en avant (§ 10.7).
- Un **support** de $b$ est un élément de $\mathcal{Q}_b$ (§ 10.6). Une boule peut en porter plusieurs.

On écrit $\lambda_b$ le niveau d'une boule $b$, $t=K-p$ et, pour $F\subseteq P_b$, « $F$ stricte » si
$\beta(F)<\lambda_b$, c'est-à-dire si $F\cap U_b$ est séparable (T2).

### 10.2 L'arbre $T_K$ et sa numérotation canonique

Fixer $1\leq K\leq n$. L'arbre $T_K$ est la forêt d'ordre $K$ de FULL (§ 4), arbre de fusion des composantes de
$\Gamma_K(a)$ quand $a$ croît. À un niveau $a$, une composante de $\Gamma_K(a)$ sans sommet de $\Gamma_K^{<}(a)$ est
une **naissance** ; une composante qui contient au moins deux composantes de $\Gamma_K^{<}(a)$ est une **fusion**
N-aire de ce niveau ; une composante qui en contient exactement une ne crée aucun nœud. Un nœud $v$ a un niveau
$a_v$ ; son parent est la fusion qui absorbe sa composante. Pour $K\leq n$, la racine est unique.

- $v$ est **vivant à la coupe fermée** $a$ si $a_v\leq a<a_{\mathrm{parent}(v)}$, et **vivant à la coupe ouverte**
  $a$ si $a_v<a\leq a_{\mathrm{parent}(v)}$ (sans borne supérieure pour la racine). Les nœuds vivants sont en
  bijection avec les composantes de $\Gamma_K(a)$, respectivement de $\Gamma_K^{<}(a)$. On note $C_v(a)$ la composante
  de $v$.
- $w\preceq v$ signifie que $w$ est dans le sous-arbre de $v$. Pour $a\geq a_w$, $\mathrm{anc}_a(w)$ est l'ancêtre
  de $w$, lui-même compris, vivant à la coupe fermée $a$ ; pour $a>a_w$, $\mathrm{anc}^{<}_a(w)$ est celui de la coupe
  ouverte. Les composantes croissent : si $w$ est vivant à $a$ et $a\leq a'$, alors
  $C_w(a)\subseteq C_{\mathrm{anc}_{a'}(w)}(a')$.
- À $K=1$, les naissances sont les sites, au niveau 0. À $K\geq 2$, ce sont exactement les boules de naissance de
  $W_K$ (lemme B).

**Numérotation canonique.**
1. Les naissances d'abord, par (niveau, centre de la boule de naissance), les centres exacts étant comparés dans
   l'ordre lexicographique des coordonnées. À $K=1$, le centre est le site.
2. Les fusions ensuite, par (niveau, plus petit numéro de naissance de leur sous-arbre).
3. Les enfants d'une fusion par numéro croissant.

L'ordre est total : deux naissances n'ont jamais la même boule (lemme B), et deux fusions de même niveau ont des
sous-arbres disjoints. Un enfant a un numéro plus petit et un niveau strictement plus petit que son parent ; la
racine est le dernier nœud. Cette numérotation ne dépend que de l'arbre, des niveaux et des centres. Réalisation :
`OrderForest` (`src/tower/forest_build.cpp`, `src/tower/forest_plateau.cpp`) ; même convention dans
`reference/hgp11_ref/model.py`.

**Rangs.** Les niveaux sont repérés par les rangs denses `LevelRank` : le rang 0 est le niveau nul, les boules de
$\mathrm{Cat}_K$ ont un rang $r_b\geq 1$, et l'égalité des rangs équivaut à celle des niveaux. On note $\ell(r)$ le
niveau de rang $r$. Tout niveau de nœud est 0 (feuille à $K=1$) ou le niveau d'une boule de $W_K$ (lemmes B et C). Les
nœuds vivants à la coupe ouverte $\lambda_b$ sont donc ceux de la coupe fermée de rang $r_b-1$.

Seuls les **ensembles de nœuds** coïncident, pas les composantes. Entre $\ell(r_b-1)$ et $\lambda_b$ peuvent naître
des sommets de $\Gamma_K$ dont la boule minimale est hors de $\mathrm{Cat}_K$ ; ils rejoignent des composantes
existantes sans créer de nœud (lemme W, point 1). Une $K$-partie stricte de $b$ peut ainsi naître après $\ell(r_b-1)$ : sur le
témoin D2 (§ 10.11), la trace stricte $AB$ de la boule $ABC$ naît au niveau 64, avec $41=\ell(r_b-1)<64<\lambda_b=1681/25$.
Aucune garde ne doit donc exiger $\beta(F)\leq\ell(r_b-1)$ d'une trace stricte $F$, et aucun juge ne doit comparer
les composantes de la coupe ouverte à celles de la coupe fermée de rang $r_b-1$.

### 10.3 Boules d'événement et périmètre

$$ W_K=\lbrace b\in\mathrm{Cat}_K : p+q-1\leq K\leq p+m\rbrace=\lbrace b\in\mathrm{Cat}_K : p+m\geq K\rbrace. $$

La seconde égalité vient de $\mathrm{Cat}_K=\lbrace p+q\leq K+1\rbrace$ (§ 3). Pour $b\in W_K$, on a $p\leq K-1$,
donc $1\leq t\leq m$. On appelle **naissance** une boule de $W_K$ sans $K$-partie stricte : toutes les $t$-parties de
$U_b$ sont non séparables, ce qui force $p+q\leq K$. Les autres boules de $W_K$ sont des **cellules** ; elles ont
$\lvert P_b\rvert\geq K+1$. Une boule est **forte** si $p+q\leq K\leq p+m$ (P3), **faible** si $K=p+q-1$. Une
naissance est toujours forte ; une boule faible est toujours une cellule, puisque toute partie de $q-1$ sites de
coquille est séparable. Les boules faibles portent des fusions : le triangle équilatéral
$(0,0,0),(2,2,0),(2,0,2)$ réunit à $K=2$ trois paires nées au niveau 2, au niveau $8/3$ (audit, revue `qb`).

**Lemme A (rattachement).** Soit $b$ une boule critique positive avec $p+m\geq K$. Toutes les $K$-parties de $P_b$
sont des sommets de $\Gamma_K(\lambda_b)$ et elles sont dans une même composante de $\Gamma_K(\lambda_b)$. Pour une
$K$-partie $F$ quelconque, $c_b\in W_F(\lambda_b)$ si et seulement si $F\subseteq P_b$ : les $K$-parties de $P_b$ sont
exactement celles dont la région témoin contient $c_b$ au niveau $\lambda_b$. Pour $b\in W_K$, on note $C_b$ cette
composante et $\mathrm{att}(b)$ l'unique nœud vivant à la coupe **fermée** $\lambda_b$ qui la représente : il est lu
après tous les événements du niveau $\lambda_b$, pas après la seule boule $b$.

*Preuve.* $c_b\in W_F(\lambda_b)$ équivaut à $\|x-c_b\|^2\leq\lambda_b$ pour tout $x\in F$, c'est-à-dire à
$F\subseteq P_b$. Une $K$-partie $F\subseteq P_b$ est donc dans la boule fermée $b$ : $\beta(F)\leq\lambda_b$. Si
$p+m=K$, il n'y a qu'une $K$-partie. Sinon, deux $K$-parties de $P_b$ se joignent par des
échanges d'un site dans $P_b$ ; l'union $G$ de deux parties consécutives a $K+1$ sites de $P_b$, donc
$\beta(G)\leq\lambda_b$ : c'est une arête de $\Gamma_K(\lambda_b)$ (T3). Composantes et nœuds vivants se correspondent
(§ 10.2). ∎

**Lemme P (plateau).** Fixer un niveau $\lambda>0$.
1. *Sommets neufs privés.* Si $F$ est un sommet de niveau $\beta(F)=\lambda$ et $b$ une boule de niveau $\lambda$
   avec $F\subseteq P_b$, alors $b=B(F)$. Toute arête $G\supseteq F$ de niveau $\lambda$ vérifie $B(G)=B(F)$, donc
   $G\subseteq P_{B(F)}$.
2. *Boules hors fenêtre.* Soit $b\notin W_K$ de niveau $\lambda$ avec $p+m\geq K$ ; alors $p+q\geq K+2$. Ses
   $K$-parties strictes existent, elles sont toutes dans une même composante de $\Gamma_K^{<}(\lambda)$, et chaque
   site de $P_b$ appartient à l'une d'elles.
3. *Décomposition.* Soit $H_\lambda$ le graphe biparti entre les composantes de $\Gamma_K^{<}(\lambda)$ et les
   cellules de $W_K$ de niveau $\lambda$, une cellule $b$ **touchant** une composante $D$ si une $K$-partie stricte de
   $P_b$ est dans $D$. Les composantes de $\Gamma_K(\lambda)$ qui contiennent un sommet strict correspondent aux
   composantes connexes de $H_\lambda$ : chacune contient exactement les composantes strictes de la sienne. Les autres
   composantes de $\Gamma_K(\lambda)$ sont les naissances de niveau $\lambda$ : pour chaque naissance $b\in W_K$ de ce
   niveau, l'ensemble des $K$-parties de $P_b$.

*Preuve.*
1. $F\subseteq P_b$ et $\beta(F)=\lambda_b$ : par T2, $F\cap U_b$ n'est pas séparable, donc contient un support de
   $b$ (lemme F, point 1), et $B(F)=b$ par M2. Si $G\supseteq F$ a le niveau $\lambda=\beta(F)$, la boule $B(G)$
   contient $F$ avec le rayon minimal de $F$ : c'est $B(F)$, par l'unicité de M1.
2. $b\notin\mathrm{Cat}_K$, donc $p+q\geq K+2$. Une partie d'une partie séparable est séparable. Toute $K$-partie
   stricte de $P_b$ se ramène, par des échanges stricts, à une partie qui contient $\min(p,K)$ intérieurs : ajouter
   un intérieur absent laisse la trace inchangée, puis retirer un site de coquille la rétrécit.
   - Si $p\geq K$, on aboutit aux $K$-parties de $I_b$, de trace vide, reliées entre elles par les $(K+1)$-parties
     de $I_b$, strictes elles aussi. Tout site $x\in P_b$ est dans une $K$-partie stricte : $x$ et $K-1$ intérieurs,
     de trace vide ou réduite à $\lbrace x\rbrace$, séparable puisque $\lambda>0$.
   - Si $p<K$, on a $1\leq t\leq q-2$ et $t\leq m$ : toute partie de $t$ ou $t+1$ sites de coquille a moins de $q$
     sites, donc est séparable (lemme F, point 1). On aboutit aux traces $I_b\cup A$, toutes strictes ; deux traces
     dont les $A$ diffèrent d'un site sont reliées par la $(K+1)$-partie stricte $I_b\cup A\cup A'$. Un site de
     coquille $x$ est dans la trace $I_b\cup A$ pour tout $A\ni x$.

   Dans les deux cas, les $K$-parties strictes forment une seule composante stricte.
3. Une arête de niveau $<\lambda$ relie deux sommets d'une même composante stricte. Sur un chemin de
   $\Gamma_K(\lambda)$ entre deux sommets stricts, couper aux sommets stricts. Soient $F_i$ et $F_j$ deux sommets
   stricts consécutifs du chemin, qui ne sont pas reliés par une seule arête de niveau $<\lambda$. Les sommets
   intermédiaires sont neufs, toutes les arêtes entre $F_i$ et $F_j$ sont de niveau $\lambda$ et, par le point 1,
   elles ont la même boule $b$ ; $F_i$ et $F_j$ sont des $K$-parties strictes de $P_b$. Si $b\notin W_K$, le point 2
   les place dans une même composante stricte ; sinon $b$ est une cellule qui touche les deux composantes.
   Réciproquement, une cellule relie toutes ses $K$-parties (lemme A). Une composante sans sommet strict n'a que des
   sommets neufs ; par le point 1, sommets et arêtes y ont une seule boule $b$, qui n'a aucune $K$-partie stricte, et
   la composante contient toutes les $K$-parties de $P_b$ (lemme A). Une telle $b$ vérifie $p<K$ (sinon $I_b$
   fournit des parties strictes) et $t\geq q$, donc $b\in W_K$ : c'est une naissance. Inversement, les $K$-parties
   d'une naissance sont neuves et ne sont reliées à rien d'autre à ce niveau (point 1). ∎

C'est T4 précisé : les « événements » d'un plateau sont les cellules de $W_K$ de ce niveau.

**Lemme W (périmètre).**
1. Une boule hors de $W_K$ ne crée aucun nœud de $T_K$, n'en réunit aucun et n'ajoute aucun site à un $K$-polyèdre.
   Si $p+m<K$, $P_b$ n'a aucune $K$-partie. Sinon, ses $K$-parties neuves se rattachent, au niveau $\lambda_b$, à
   l'unique composante de ses $K$-parties strictes, qui contiennent déjà tous ses sites (lemme P). Les naissances
   (lemme P), les fusions et leurs enfants (lemme C) et les ensembles de points (lemme H) sont donc portés par les
   seules boules de $W_K$.
2. Ces rattachements appartiennent pourtant à $\Gamma_K$ : ils décident plus tard quelle composante une trace stricte
   d'une cellule touche. Retirer de $\Gamma_K$ les liaisons des boules hors de $W_K$ change $T_K$ en général, que
   l'on garde leurs sommets ou non. Sur les cinq sites E5,
   $A=(0,0,7)$, $B=(0,9,6)$, $C=(1,4,0)$, $D=(0,0,1)$, $E=(4,1,2)$, à $K=2$, la boule de la paire $AC$ (niveau
   $33/2$, intérieurs $D$ et $E$, $p=2$, $q=m=2$) est hors fenêtre ; par ses liaisons $ACD$ et $ACE$, elle rattache
   le sommet $AC$ à la composante de $AD$, $AE$, $CD$, $CE$ et $DE$. Dans $T_2$, la fusion de niveau $83886/3563$ a
   trois enfants, $\lbrace AB\rbrace$, $\lbrace AC,AD,AE,CD,CE,DE\rbrace$ et $\lbrace BC\rbrace$
   ([fixture](../../tests/fixtures/regressions/gabriel_point_set_counterexample.json)
   `gabriel-point-set-counterexample-5-points-v1`).
   - *Sans ses liaisons, sommet $AC$ gardé*, comme dans le $K$-graphe de Gabriel de la thèse (Déf. 29, p. 89), où
     $AC$ est un sommet en tant que facette du simplexe de Gabriel $ABC$. $AC$ devient une naissance isolée au niveau
     $33/2$ (huit naissances au lieu de sept), et la fusion de niveau $83886/3563$ garde trois enfants, mais ce sont
     $\lbrace AB\rbrace$, $\lbrace AC\rbrace$ et $\lbrace BC\rbrace$.
   - *Sans ses sommets ni ses liaisons* ($AC$ n'entre qu'avec la liaison $ABC$) : cette fusion n'a plus que deux
     enfants.

   Dans les deux lectures, une fusion supplémentaire à deux enfants apparaît au niveau 24. Le témoin D2 (§ 10.11) en
   donne un second exemple plan : sans la boule de $AB$, hors fenêtre, une fusion supplémentaire apparaît au niveau
   $145/2$ et les enfants de la fusion de niveau $1681/25$ changent. La publication se limite à $W_K$ ; le calcul de
   $\mathrm{att}$ et de $\mathrm{ant}$ lit le vrai $\Gamma_K$, comme le font les descentes (T5) et le test strict (T2).
3. Toute liaison de Gabriel (Déf. 28, p. 87 : $(K+1)$-partie $G$ dont la boule minimale n'a aucun site intérieur
   hors de $G$, soit $I_{B(G)}\subseteq G$) a sa boule dans $W_K$, avec $p+m\geq K+1$.
4. *Théorème 4 sans position générale.* Une liaison qui n'est pas de Gabriel n'est jamais séparante (Déf. 27,
   p. 86) : ses $K$-faces strictes sont toutes dans une même composante de $\Gamma_K^{<}(\beta(G))$.

*Preuve.* Le point 1 suit du lemme P, points 2 et 3. Le point 2 est un calcul exact sur la fixture. Point 3 : avec
$b=B(G)$, on a $G\subseteq P_b$ et, par M1, $G\cap U_b$ non séparable ; elle contient un support, de $q$ sites au
moins. Comme $I_b\subseteq G$, on obtient $K+1\geq p+q$ et $p+m\geq K+1$. Point 4 : soit $z\in I_b\setminus G$. Une
face $G\setminus\lbrace s\rbrace$ est stricte si et seulement si sa trace est séparable (T2). Pour deux faces
strictes $G\setminus\lbrace s\rbrace$ et $G\setminus\lbrace s'\rbrace$, les $(K+1)$-parties
$(G\setminus\lbrace s\rbrace)\cup\lbrace z\rbrace$ et $(G\setminus\lbrace s'\rbrace)\cup\lbrace z\rbrace$ sont dans
$P_b$, avec ces mêmes traces séparables : elles sont strictes, et elles partagent la face
$(G\setminus\lbrace s,s'\rbrace)\cup\lbrace z\rbrace$. C'est la preuve de la thèse, où T2 remplace la position
générale. ∎

La Prop. 6 (p. 90) n'est ni utilisée ni revendiquée : elle est fausse comme égalité de collections d'ensembles de
points, sur cette même fixture E5 (registre des preuves). Les fusions se décrivent boule par boule (lemmes P et C),
pas par le seul graphe de Gabriel. Les boules de $p+q\in\lbrace K+2,K+3\rbrace$, déjà présentes dans
$\mathrm{Cat}_{K+2}$ pour le juge J3, n'apporteraient que des rattachements et des liaisons non Gabriel : leur
publication est une extension non livrée. Au-delà, elles exigeraient un catalogue d'ordre plus élevé et restent hors
du périmètre.

### 10.4 Rattachement, branches et rôles

**Lemme B (rôles).** Soit $b\in W_K$.
1. Si $b$ est une naissance, $\mathrm{att}(b)$ est la naissance de $T_K$ formée des $K$-parties de $P_b$, de niveau
   $\lambda_b$ : rôle **naissance**. À $K\geq 2$, c'est une bijection entre les naissances de $W_K$ et celles de
   $T_K$. À $K=1$, aucune boule de $W_1$ n'est une naissance.
2. Si $b$ est une cellule, deux cas seulement :
   - $a_{\mathrm{att}(b)}=\lambda_b$ et $\mathrm{att}(b)$ est la fusion créée au plateau $\lambda_b$ : rôle
     **fusion** ;
   - $a_{\mathrm{att}(b)}<\lambda_b<a_{\mathrm{parent}(\mathrm{att}(b))}$, ou $\mathrm{att}(b)$ est la racine avec
     $a_{\mathrm{att}(b)}<\lambda_b$ : rôle **interne** (continuation, qui ferme des cycles ou ajoute des sommets sans
     fusionner).

Le rôle se lit donc sur les rangs : naissance sans $K$-partie stricte ; sinon fusion si
$\mathrm{rang}(\mathrm{att}(b))=r_b$, interne si $\mathrm{rang}(\mathrm{att}(b))<r_b$.

*Preuve.* Point 1 : lemme P, point 3. À $K=1$, une boule de $W_1$ a $p=0$, $t=1$, et chaque site de coquille est
séparable. Point 2 : $C_b$ contient une $K$-partie stricte, donc n'est pas une naissance de niveau $\lambda_b$. Si la
composante de $b$ dans $H_{\lambda_b}$ contient au moins deux composantes strictes, $C_b$ est la fusion de ce niveau ;
sinon $C_b$ prolonge l'unique composante stricte, dont le nœud reste vivant à $\lambda_b$ (lemme P, point 3). ∎

**Définition (branches).** $\mathrm{ant}(b)$ est l'ensemble des nœuds vivants à la coupe **ouverte** $\lambda_b$
dont la composante contient une $K$-partie stricte de $P_b$, c'est-à-dire les composantes que $b$ touche. C'est un
ensemble : chaque branche y figure une fois, quel que soit le nombre de traces qui la touchent. Son cardinal est
`components` (nom de l'audit : `strict_global_components`). Il est vide pour une naissance.

**Lemme C (branches).**
1. $\mathrm{ant}(b)$ est aussi l'ensemble des nœuds de coupe ouverte des seules traces strictes $I_b\cup A$, avec
   $\lvert A\rvert=t$ et $A$ séparable.
2. Rôle interne : $\mathrm{ant}(b)=\lbrace\mathrm{att}(b)\rbrace$. Rôle fusion :
   $\mathrm{ant}(b)\subseteq\mathrm{enfants}(\mathrm{att}(b))$. Pour tout $u\in\mathrm{ant}(b)$, on a
   $\mathrm{att}(b)=\mathrm{parent}(u)$ si $a_{\mathrm{parent}(u)}=\lambda_b$, et $\mathrm{att}(b)=u$ sinon.
3. Pour toute fusion $v$, la réunion des $\mathrm{ant}(b)$ sur les boules de rôle fusion telles que
   $\mathrm{att}(b)=v$ est exactement $\mathrm{enfants}(v)$. En particulier, toute fusion porte au moins une boule de
   rôle fusion.

*Preuve.*
1. Une $K$-partie stricte $F\subseteq P_b$ se ramène à une trace stricte $I_b\cup A$ par des échanges stricts : on
   ajoute un intérieur absent, ce qui laisse la trace séparable inchangée, puis on retire un site de coquille. Ces
   échanges ont lieu dans $\Gamma_K^{<}(\lambda_b)$ ; ils sont possibles car $\lvert F\cap U_b\rvert>t\geq 1$ tant
   que $F$ ne contient pas $I_b$.
2. Soit $u$ le nœud de coupe ouverte d'une $K$-partie stricte $F$ : $a_u<\lambda_b\leq a_{\mathrm{parent}(u)}$. Si
   $a_{\mathrm{parent}(u)}=\lambda_b$, le parent est vivant à la coupe fermée $\lambda_b$ et contient $F$, donc
   $C_b$ (lemme A) : c'est $\mathrm{att}(b)$, de niveau $\lambda_b$. Sinon $u$ est vivant à $\lambda_b$ et contient
   $F$, donc $u=\mathrm{att}(b)$, de niveau $<\lambda_b$. Le premier cas est celui du rôle fusion, le second celui du
   rôle interne, pour tout $u\in\mathrm{ant}(b)$ à la fois.
3. Par le lemme P, point 3, les enfants de $v$ sont les composantes strictes de la composante de $H_{a_v}$ qui
   forme $v$. Cette composante est connexe et contient au moins deux composantes strictes : chacune est touchée par
   une cellule $b$, pour laquelle $C_b$ est la composante de $v$, donc $\mathrm{att}(b)=v$ avec le rôle fusion. ∎

**Conséquences.**
- Les $\mathrm{att}$ partitionnent $W_K$ sur les nœuds : chaque nœud a sa **liste propre** de boules.
- À $K\geq 2$, une naissance possède exactement une boule de rôle naissance, la sienne, plus d'éventuelles boules
  internes. À $K=1$, une feuille ne possède aucune boule.
- Une cellule de rôle fusion peut ne toucher qu'une branche (« passagère », `components` égal à 1) : une autre
  cellule du même plateau fusionne sa composante. Exemple à $K=1$ : $(0,0),(2,2),(4,0),(8,0)$. Les trois premiers
  sites fusionnent au niveau 2 ; au niveau 4, la boule de diamètre $(0,0),(4,0)$, qui porte $(2,2)$ sur son cercle,
  ne touche que cette composante, tandis que la boule de $(4,0),(8,0)$ la réunit à $(8,0)$.
- Le nombre d'unions effectuées par une cellule dans une structure d'union-recherche dépend de l'ordre de traitement :
  sur le tétraèdre de l'audit (revue `plateau`), chacune des quatre boules de face touche trois composantes, et les
  unions valent 2, 2, 1, 0 selon la place, dans les 24 ordres. Ce nombre n'est jamais publié.

### 10.5 Calcul sans descente supplémentaire, juge par descente

**Lemme D (graines).** Soit $b$ une cellule et $F$ une trace stricte de $b$. Soit $g$ une naissance et $a'<\lambda_b$
une coupe fermée avec $a_g\leq a'$ et $F\in C_{\mathrm{anc}_{a'}(g)}(a')$. Toute descente valide (T5) partie de $F$
fournit un tel couple, avec $a'=\beta(F)$ et $g$ la naissance de sa boule terminale ; à $K=1$, la descente s'arrête
sur le site lui-même (boule de rayon nul) et $g$ est sa feuille. L'hypothèse n'exige pas $a'\leq\ell(r_b-1)$ : sur le
témoin D2, la trace stricte $AB$ naît au niveau 64, après $\ell(r_b-1)=41$, et sa descente aboutit à la naissance
$ZW$, de niveau 1. Alors :
1. $a_g<\lambda_b$ ;
2. $u=\mathrm{anc}^{<}_{\lambda_b}(g)$ est le nœud de $F$ à la coupe ouverte, donc $u\in\mathrm{ant}(b)$ ;
3. $\mathrm{att}(b)=\mathrm{anc}_{\lambda_b}(g)$, soit $\mathrm{parent}(u)$ si $a_{\mathrm{parent}(u)}=\lambda_b$ et
   $u$ sinon.

Ce résultat ne dépend ni de la trace ni de la graine : son égalité pour toutes les traces d'une boule est un contrôle
de T3 (lemme A).

*Preuve.* Le point 1 est immédiat. Point 2 : $\Gamma_K(a')\subseteq\Gamma_K^{<}(\lambda_b)$, donc la composante de
$F$ à la coupe $a'$ est contenue dans sa composante à la coupe ouverte, que représente l'ancêtre de coupe ouverte de
$g$. Point 3 : lemme C, point 2. Pour la descente, à $K\geq 2$ : sa boule terminale a $p<K$, contient $K$ sites et
n'a aucune $t$-partie séparable, donc $t\geq q$ ; c'est une naissance de $W_K$ (§ 10.3), dont le nœud est $g$
(lemme B). À $K=1$, la boule terminale est celle d'un site, de rayon nul, hors de $W_1$ ; $g$ est la feuille de ce
site, de niveau 0. T5 place la $K$-partie terminale dans la composante de $F$ à la coupe $\beta(F)$. ∎

*Réalisation (E1).* Le constructeur consigne la graine $g$ de chaque trace stricte quand il applique une cellule
(`ForestBuilder::cell` et `ForestBuilder::regular_cell`, `src/tower/forest_plateau.cpp`). Un seul balayage des
ancêtres à la coupe fermée de rang $r_b-1$ donne $\mathrm{ant}(b)$ ; la règle du parent donne $\mathrm{att}(b)$.
Aucune descente supplémentaire n'est faite. Le balayage ne lit que des nœuds et leurs ancêtres : il reste juste
quand la trace naît après $\ell(r_b-1)$ (témoin D2), alors qu'un contrôle de la trace elle-même à ce rang, ou de sa
composante, serait faux (§ 10.2).

**Lemme E (juge).** Soit $b\in W_K$. Pour toute $K$-partie $F\subseteq P_b$, stricte ou non, et toute descente
valide partie de $F$, de naissance terminale $g_F$ (à $K=1$, la feuille du site de $F$), on a
$\mathrm{anc}_{\lambda_b}(g_F)=\mathrm{att}(b)$. Pour une naissance, la descente s'arrête dès le premier pas sur la
naissance de $b$.

*Preuve.* Par T5, $F$ est dans la composante de $\mathrm{anc}_{\beta(F)}(g_F)$ à la coupe $\beta(F)\leq\lambda_b$.
À la coupe $\lambda_b$, $F\in C_b$ (lemme A). Si $b$ est une naissance, $B(F)=b$ (lemme P, point 1), $p<K$ et aucune
$t$-partie n'est séparable : T5 s'arrête. ∎

Une $K$-partie quelconque de $P_b$ peut avoir le niveau $\lambda_b$ lui-même : sur la ligne $(0,0),(1,0),(2,0)$ à
$K=2$, la paire extrême a le niveau 1 de la boule faible, les deux autres le niveau $1/4$. Le juge part donc d'un
niveau initial $\beta(F)\leq\lambda_b$ et remonte à la coupe **fermée** $\lambda_b$. Le sélecteur comprimé
($I_b$, puis $t$ sites de $U_b$) rend, lui, une trace stricte pour une boule faible : tout $A$ de $t=q-1$ sites est
séparable. Le journal E1 ne consigne que des traces strictes, de niveau $<\lambda_b$.

*Réalisation (E2).* C'est la descente suivie de l'ancêtre fermé de `ball_nodes` (`bench/points_export.cpp`), avec la
fenêtre de $W_K$ au lieu du prédicat fort. Elle sert de juge de test et ne figure jamais dans le produit. Ses fixtures
comprennent le témoin D2, sur le domaine étroit $\mathrm{Cat}_K$ ($k_{\max}=K=2$) : dès $k_{\max}\geq 3$, la boule de
$AB$ entre au catalogue, $\ell(r_b-1)$ vaut 64 et D2 n'est plus un contre-cas. La porte `mhgp11_tower_attach_e1e2` juge
donc les deux domaines, $\mathrm{Cat}_{k_{\max}}$ et $\mathrm{Cat}_K$, et compte les traces nées après $\ell(r_b-1)$ ; le
différentiel `mhgp11_tower_attach_fraction` compare le rattachement natif à l'oracle borné sur le domaine étroit
(intégration L1 de S3).

### 10.6 Supports positifs minimaux

$$ \mathcal{Q}_b=\lbrace Q\subseteq U_b : Q\ \text{affinement indépendant},\ c_b\in\mathrm{relint}\,\mathrm{conv}\,Q\rbrace. $$

Comme $\lambda_b>0$ en dimension trois, $2\leq\lvert Q\rvert\leq 4$. L'ordre canonique des supports est (cardinal,
ordre lexicographique des `SiteIdx`).

**Lemme F (supports).**
1. *Carathéodory strict.* Pour $Q\subseteq U_b$, il y a équivalence entre : $Q$ est affinement indépendant et
   $c_b\in\mathrm{relint}\,\mathrm{conv}\,Q$ ; $c_b\in\mathrm{conv}\,Q$ et aucune partie propre de $Q$ ne vérifie
   cette inclusion. Donc $\mathcal{Q}_b$ est la famille des parties non séparables **minimales** de $U_b$, et une
   partie de $U_b$ est non séparable si et seulement si elle contient un élément de $\mathcal{Q}_b$.
2. *Prédicats exacts par arité.* Les sites de $Q$ étant sur la sphère de $b$ :
   - $\lvert Q\rvert=2$ : $c_b$ est le milieu des deux sites ;
   - $\lvert Q\rvert=3$ : le triangle est strictement aigu et coplanaire au centre ;
   - $\lvert Q\rvert=4$ : $c_b$ est strictement du côté du sommet opposé pour chacune des quatre faces, ce qui exclut
     les quatre sites coplanaires.

   Aucun test séparé d'indépendance ni de minimalité n'est nécessaire. Un triangle droit n'est pas un support ; son
   hypoténuse en est un.
3. *Structure.* $\mathcal{Q}_b\neq\varnothing$ ; son plus petit cardinal est $q$ et son premier élément est $S^*(b)$ ;
   si $m=q$, $\mathcal{Q}_b=\lbrace U_b\rbrace=\lbrace S^*(b)\rbrace$. Un support détermine sa boule : $B(Q)=b$. Les
   $\mathcal{Q}_b$ sont donc deux à deux disjoints, et chaque support appartient à la liste propre d'un seul nœud.
4. *Énumération sur toute la coquille.* $\mathcal{Q}_b$ peut contenir des supports de cardinal supérieur à $q$ ; il
   s'énumère sur toutes les parties de 2 à 4 sites de $U_b$. Le cube $\lbrace 0,2\rbrace^3$ ($q=2$) a quatre
   diamètres et deux tétraèdres, et aucun triangle (revue `qb` de l'audit). L'octaèdre
   $(0,1,1),(2,1,1),(1,0,1),(1,2,1),(1,1,0),(1,1,2)$ n'a que ses trois diamètres. On a
   $\lvert\mathcal{Q}_b\rvert\leq\binom{m}{2}+\binom{m}{3}+\binom{m}{4}$.

*Preuve.*
1. Si $Q$ est affinement indépendant, les coordonnées barycentriques de $c_b$ sont uniques, et l'intérieur relatif
   est l'ensemble des points à coordonnées strictement positives : une représentation sur une partie propre en
   donnerait une autre, avec un zéro. Réciproquement, la minimalité force des poids strictement positifs, et une
   dépendance affine permettrait d'en annuler un en restant dans $\mathrm{conv}\,Q$.
2. Deux sites de la sphère dont le milieu est le centre forment un diamètre. Trois sites distincts d'une sphère ne
   sont jamais alignés ; s'ils sont coplanaires au centre, celui-ci est leur centre circonscrit, intérieur au
   triangle ouvert si et seulement si le triangle est strictement aigu. Quatre sites : intérieur strict d'un
   tétraèdre non plat. L'indépendance est donc automatique, et la minimalité suit du point 1.
3. Par M1, $c_b\in\mathrm{conv}(U_b)$ ; une partie minimale non séparable est un support. Le reste suit des
   définitions de $q$ et de $S^*$. Si $m=q$, une partie propre de $U_b$ a moins de $q$ sites. Enfin, si
   $Q\in\mathcal{Q}_b$, alors $c_b\in\mathrm{conv}\,Q$ avec $Q$ sur la sphère de $b$, d'où $B(Q)=b$ par M1. ∎

*Réalisation.* Le canoniseur de $S^*$ et les seuils d'admission du générateur ne sont pas des énumérateurs de
$\mathcal{Q}_b$. Les prédicats stricts de `num` se composent dans le module `supports` ; le drapeau de présentation
q4 du générateur n'est pas un test de support. Le produit plafonne la coquille à 24 sites et refuse l'appel entier
au-delà (`support_shell_capacity`, [SORTIES.md](SORTIES.md)) : c'est une limite du produit, pas de l'objet. Les
84 sites entiers de la sphère $x^{2}+y^{2}+z^{2}=50$ forment une coquille qui dépasse ce plafond.

### 10.7 Comptes

On pose $N_j=\lvert\lbrace A\subseteq U_b : \lvert A\rvert=j,\ A\ \text{contient un élément de}\ \mathcal{Q}_b\rbrace\rvert$,
le nombre de parties non séparables de $j$ sites de coquille (lemme F, point 1).

**Lemme G (comptes).** Pour $b\in W_K$ :

| Nom | Portée | Sens exact | Valeur | Jonction régulière ($m=q$, $K=p+q-1$) | Naissance régulière ($m=q$, $K=p+q$) |
| --- | --- | --- | --- | --- | --- |
| `kparties_reliees` | boule | $K$-parties de $P_b$, toutes reliées à la coupe fermée $\lambda_b$ | $\binom{p+m}{K}$ | $K+1$ | 1 |
| `cofaces` | boule | liaisons $G\subseteq P_b$ de boule $B(G)=b$, distinctes | $\sum_{j}\binom{p}{K+1-j}N_j$ | 1 | 0 |
| `cofaces` | support $Q$ | liaisons de $P_b$ qui contiennent $Q$, toutes de boule $b$ : incidences $(Q,G)$ | $\binom{p+m-\lvert Q\rvert}{K+1-\lvert Q\rvert}$, nul si $K+1<\lvert Q\rvert$ | 1 | 0 |
| `strict_traces` | boule | traces $I_b\cup A$ strictes | $\binom{m}{t}-N_t$ | $q$ | 0 |
| `compressed_parts` | boule | $K$-parties qui contiennent tout $I_b$ | $\binom{m}{t}$ | $q$ | 1 |
| `components` | boule | branches $\lvert\mathrm{ant}(b)\rvert$ | lue sur $T_K$ | au moins 1 | 0 |
| `gabriel_cofaces` | boule | liaisons de boule $b$ qui contiennent $I_b$ (Déf. 28) | $N_{t+1}$ | 1 | 0 |
| `gabriel_cofaces` | support $Q$ | celles qui contiennent $Q\cup I_b$ | $\binom{m-\lvert Q\rvert}{t+1-\lvert Q\rvert}$, nul si $t+1<\lvert Q\rvert$ | 1 | 0 |

*Preuve.* `kparties_reliees` : lemme A. `strict_traces` : par T2, $I_b\cup A$ est stricte si et seulement si $A$ est
séparable, c'est-à-dire ne contient aucun support (lemme F). `cofaces` de la boule : $B(G)=b$ si et seulement si
$G\subseteq P_b$ et $G\cap U_b$ n'est pas séparable (M1 et M2) ; on compte selon $j=\lvert G\cap U_b\rvert$.
`cofaces` d'un support : si $Q\subseteq G\subseteq P_b$, alors $B(G)=b$ par M2 ; on choisit les autres sites.
`gabriel_cofaces` : on impose en plus $I_b\subseteq G$, soit $G=I_b\cup A'$ avec $\lvert A'\rvert=t+1$. Les valeurs
régulières viennent de $N_j=[j=q]$ pour $j\leq q=m$. ∎

**Remarques.**
- `kparties_reliees` ne dépend que de $(p,m,K)$, pas de $\mathcal{Q}_b$ : à $(p,m,K)$ fixés, il ne voit pas le
  choix des supports. Ce n'est pas une stabilité (§ 10.9). Il vaut $K+1$ à une jonction régulière, 1 à une naissance
  régulière, 4 au carré à $K=3$.
- La somme de `kparties_reliees` sur les boules compte des incidences $(b,F)$, pas des $K$-parties distinctes : une
  même $K$-partie appartient aux populations de plusieurs boules. Sur la ligne $(0,0),(1,0),(2,0)$ à $K=2$, les trois
  boules donnent $1+1+3=5$ pour 3 paires.
- La somme des `cofaces` des boules compte, elle, des liaisons distinctes, puisqu'une liaison n'a qu'une boule
  minimale ; elle se limite aux boules de $W_K$, et n'est donc pas le nombre de toutes les liaisons de $\Gamma_K$.
- La somme des `cofaces` des supports compte des incidences, pas des liaisons distinctes :
  $\max_Q\mathrm{cofaces}(Q)\leq\mathrm{cofaces}(b)\leq\sum_Q\mathrm{cofaces}(Q)$, avec égalité à droite si et
  seulement si aucune liaison ne contient deux supports. Au carré à $K=3$ : 1 contre 2.
- Sur une coquille régulière, un support porte une liaison (jonction) ou aucune (naissance). Il n'en porte plusieurs
  que sur une coquille étendue.
- `components` ne dépasse pas `strict_traces` (lemme C, point 1). La différence $N_t$ entre `compressed_parts` et
  `strict_traces` ne compte pas les sommets neufs de $\Gamma_K$ : sur le tétraèdre de l'audit, elle vaut 0 pour
  chaque boule de face, qui a pourtant trois sommets neufs.
- `strict_traces` se calcule par deux voies distinctes, le test strict T2 des traces et $\mathcal{Q}_b$ : leur égalité
  est un contrôle par boule, naissances comprises.
- Avec $K\leq 12$, $p\leq 11$ et $m\leq 24$ (plafond du produit, refus `support_shell_capacity` au-delà ; le format
  porte $m$ en `u8`), toutes ces valeurs sont inférieures à $2^{32}$ :
  $\binom{35}{13}<1{,}5\cdot 10^{9}$.

### 10.8 Polyèdres de supports, instantanés, lemme H

**Réalisation par supports.** Pour un nœud $v$ :

$$ P_v=\bigcup_{w\preceq v}\ \bigcup_{\mathrm{att}(b)=w}\ \bigcup_{Q\in\mathcal{Q}_b}\mathrm{conv}\,Q. $$

Donc $P_w\subseteq P_v$ pour $w\preceq v$. À une coupe fermée $a$ où $v$ est vivant, l'**instantané daté** retient
les boules $b\in W_K$ telles que $\mathrm{att}(b)\preceq v$ et $\lambda_b\leq a$. Ce sont :
- toutes les boules du sous-arbre strict, de niveau $\lambda_b<a_v$ ;
- le préfixe, par rang, de la liste propre de $v$ formé des boules de niveau au plus $a$.

Seules des boules internes de $v$ dépassent $a_v$ : un état est daté (fixture `growth_ABCZ` ci-dessous). Le masque
`has_support_geometry` n'est faux que pour une feuille de $K=1$, qui ne possède aucune boule ; tout autre nœud en
possède une au moins (lemmes B et C).

*Preuve de la description.* Si $\mathrm{att}(b)=w$ avec $w$ strictement sous $v$, $w$ est vivant à $\lambda_b$, donc
$\lambda_b<a_{\mathrm{parent}(w)}\leq a_v\leq a$. Une boule propre de $v$ vérifie $a_v\leq\lambda_b$, avec égalité
pour les rôles naissance et fusion (lemme B). ∎

**Lemme H (forme datée de P3).** On note $\mathrm{pts}(C)$ l'ensemble des sites des $K$-parties d'une composante
$C$ : c'est le $K$-polyèdre de la Déf. 21 (p. 58) au rayon $\sqrt{a}$, puisque le graphe de cette définition a les
composantes de $\Gamma_K(a)$ (Prop. 5, T1). Soit $K\geq 2$, $v$ un nœud et $a$ une coupe fermée où $v$ est vivant.
Alors

$$ \mathrm{pts}(C_v(a))=\bigcup\lbrace P_b : b\in W_K,\ \mathrm{att}(b)\preceq v,\ \lambda_b\leq a\rbrace, $$

et la réunion peut se restreindre aux boules fortes. À $K=1$, $\mathrm{pts}(C_v(a))$ est l'ensemble des feuilles du
sous-arbre de $v$ ; l'égalité vaut encore si $v$ n'est pas une feuille (une feuille ne possède aucune boule).

*Preuve.* C'est celle de P3, menée dans une composante donnée et datée.
- *Inclusion de droite à gauche.* Si $\mathrm{att}(b)\preceq v$ et $\lambda_b\leq a$, alors
  $v=\mathrm{anc}_a(\mathrm{att}(b))$, et les $K$-parties de $P_b$, qui sont dans $C_b$ (lemme A), sont dans
  $C_v(a)$. Comme $\lvert P_b\rvert\geq K$, chaque site de $P_b$ est dans l'une d'elles.
- *Inclusion de gauche à droite.* Soit $x\in\mathrm{pts}(C_v(a))$. Parmi les $K$-parties de $C_v(a)$ qui contiennent
  $x$, en choisir une, $F_0$, de niveau minimal, et poser $b=B(F_0)$, de niveau $\lambda_b=\beta(F_0)\leq a$. Alors
  $x\in P_b$ et $p+m\geq K$. Supposons $K<p+q$ : on construit une $K$-partie $F_1\ni x$ de $P_b$ dont la trace a au
  plus $\max(1,K-p)<q$ sites. Si $p\geq K$, ou si $p=K-1$ et $x\in U_b$, $F_1$ est formée de $x$ et de $K-1$
  intérieurs ; sinon, de $I_b$, de $x$ et de sites de coquille jusqu'à $K$ sites. La trace de $F_1$ est séparable,
  donc $\beta(F_1)<\lambda_b$ (T2). Or $F_0$ et $F_1$ sont dans une même composante à $\lambda_b$ (lemme A), donc
  dans $C_v(a)$ : c'est contraire à la minimalité. Donc $p+q\leq K\leq p+m$, $b$ est une boule forte de $W_K$, et
  $F_0\in C_b\cap C_v(a)$ donne $\mathrm{att}(b)\preceq v$.
- *Cas $K=1$.* $\Gamma_1$ a pour sommets les sites. Une feuille $x$ strictement sous $v$ est un enfant de la fusion
  $\mathrm{parent}(x)\preceq v$, de niveau au plus $a$. Par le lemme C, point 3, une boule de rôle fusion de ce nœud
  touche $\lbrace x\rbrace$, donc $x\in P_b$. ∎

Le lemme H n'est pas publié par le format : il n'y a pas de population (décision de l'utilisateur). Il dit comment
reconstruire le $K$-polyèdre hors format, à partir du catalogue ($P_b=I_b\cup U_b$, $\mathrm{att}(b)$, $\lambda_b$),
et l'oracle borné le contrôle à chaque nœud et à chaque niveau d'événement. La réunion des seules populations de
naissance ne suffit pas (P3) : sur `growth_ABCZ` à $K=3$, au niveau 25, le polyèdre de la naissance $ABC$ vaut
$\lbrace A,B,C,Z\rbrace$. Les boules faibles n'ajoutent aucun site, mais elles portent des fusions de $T_K$.

### 10.9 Ce que la réalisation par supports n'est pas

Ces déclarations sont obligatoires pour toute sortie `supports`.
- **Elle n'est pas stable aux cosphéricités.** Sur le cercle unité, avec $A=(1,0)$, $B=(0,1)$, $C=(-1,0)$,
  $D=(0,-1)$, $\mathcal{Q}_b=\lbrace AC,BD\rbrace$. Remplacer $B$ par
  $B_t=(2t/(1+t^{2}),(1-t^{2})/(1+t^{2}))$, $0<t<1/2$, laisse la boule inchangée, mais donne
  $\mathcal{Q}_b=\lbrace AC,B_tCD\rbrace$. Le point $(-1/4,1/4)$ est dans le triangle $B_tCD$, à distance $1/4$ des
  deux diamètres : le saut de Hausdorff est au moins $1/4$, alors que le déplacement $2t/\sqrt{1+t^{2}}$ tend vers 0
  ([revue `carrier`](../receipts/audit_supports_20261004/carrier/README.md)). Les immersions entières $t=1/n$, à
  l'échelle $n^{2}+1$, tiennent en u21 jusqu'à $n=1023$ ; le profil fini n'hérite pas de la limite analytique. La
  stabilité de FULL en rayon (P5) ne se transfère pas à cette réalisation ; aucune stabilité n'est revendiquée, et la
  robustesse d'un jeton qui l'emploierait est un contrat séparé, à mesurer.
- **Elle omet** les sites intérieurs et, sur une coquille étendue, les sites de coquille hors de tout support : dans
  le triangle droit $(0,0),(4,0),(0,3)$, l'origine est sur le cercle de l'hypoténuse sans appartenir à aucun
  support.
- **Elle n'est pas le $K$-polyèdre** de la Déf. 21, qui est un ensemble de sites (lemme H), ni
  $\mathrm{conv}(U_b)$, ni une population. Les réalisations de deux branches peuvent se recouvrir, et leurs
  intersections ne donnent pas la connectivité de FULL : au carré de côté 2, à $K=2$ et au niveau 1, quatre
  composantes de côtés partagent leurs sommets.
- `kparties_reliees`, fonction de $(p,m,K)$ seuls, échappe à ce saut : sur le cercle, il vaut 6 à $K=2$ avant et après
  la perturbation, alors que $\mathcal{Q}_b$ et les `cofaces` des supports changent. Il n'est pas stable pour autant,
  car sortir un site de la coquille change $m$ sans changer ni la boule ni $\mathcal{Q}_b$. Avec le diamètre $(0,0)$,
  $(20,0)$ et le site $(10,10)$ sur son cercle, à $K=2$, il vaut 3 ; déplacé en $(10,11)$, hors du cercle, ce site
  le fait passer à 1, alors que la boule et son unique support, le diamètre, restent les mêmes (auditeur, réponse
  D.1 de `aef7182b3`).

### 10.10 Invariance et déterminisme

L'objet publié ne dépend que de l'ensemble des positions et de $K$ : la numérotation de $T_K$ ne lit que l'arbre, des
niveaux et des centres ; $\mathrm{att}$ ne dépend ni de la trace ni de la politique de descente (lemmes D et E) ;
$\mathrm{ant}(b)$ et $\mathcal{Q}_b$ sont des ensembles ; les rôles se lisent sur les rangs ; les comptes sont des
fonctions de $(p,m,K,\mathcal{Q}_b)$ et de $T_K$. Conséquences :
- une permutation de l'entrée ne change rien, ordres compris, car les `SiteIdx` sont les rangs de Morton des
  positions ;
- un réétiquetage injectif des `PointId` ne change que les identifiants publiés ;
- une translation entière laisse la numérotation de $T_K$, les niveaux, les listes propres et les ensembles
  $\mathcal{Q}_b$ invariants, à la translation près. Les ordres qui passent par les `SiteIdx` peuvent changer : lignes
  de sites, ordre des boules d'un même rang, ordre des supports d'une boule ;
- une isométrie entière de la grille (permutation ou réflexion d'axes) transporte l'objet, mais peut changer la
  numérotation ;
- aucune invariance par rotation n'est revendiquée : la quantification casse l'équivariance.

Toute réalisation doit rendre cet objet quel que soit le nombre de fils.

### 10.11 Témoins exacts

Les valeurs suivantes ont été recalculées en `Fraction` exacte par le rédacteur. L'oracle borné S1 les recalcule et
les grave. Coordonnées entières, $z=0$ si rien n'est précisé.

| Nuage | $K$ | Faits |
| --- | ---: | --- |
| carré $(0,0),(2,0),(2,2),(0,2)$ | 1 | quatre côtés de rôle fusion au niveau 1, `components` 2 ; diagonale de niveau 2 interne sur la racine, `kparties_reliees` 4, `cofaces` 2, 1 par support |
| carré | 2 | quatre naissances au niveau 1 ; diagonale de rôle fusion, `components` 4, `kparties_reliees` 6, `strict_traces` 4, `cofaces` 4, 2 par support |
| carré | 3 | naissance étendue, `kparties_reliees` 4, `cofaces` 1, 1 par support : la somme des supports vaut 2 |
| carré | 4 | naissance, `kparties_reliees` 1, `cofaces` 0 |
| triangle droit $(0,0),(4,0),(0,3)$ | 2 | naissances aux niveaux $9/4$ et 4, fusion à $25/4$ ; $\mathcal{Q}_b$ réduit à l'hypoténuse ; `strict_traces` 2, `cofaces` 1 |
| `growth_ABCZ` $(1,8),(5,10),(9,8),(5,0)$ | 3 | naissance $ABC$ au niveau 16 ; boule interne au niveau 25 sur cette naissance, $\mathcal{Q}_b=\lbrace BZ,ACZ\rbrace$, `cofaces` 1 par support et 1 pour la boule |
| passagère $(0,0),(2,2),(4,0),(8,0)$ | 1 | fusion à trois enfants au niveau 2 ; au niveau 4, `components` 2 pour la boule de $(4,0),(8,0)$ et 1, rôle fusion, pour celle de diamètre $(0,0),(4,0)$ |
| triangle aigu $(0,0),(2,0),(1,2)$ | 2 | trois naissances, fusion à trois enfants au niveau $25/16$, `strict_traces` 3, `cofaces` 1 |
| ligne $(0,0),(1,0),(2,0)$ | 2 | boule de niveau 1, $I_b$ réduit au milieu ; `kparties_reliees` 3, `compressed_parts` 2, `strict_traces` 2, `components` 2, `cofaces` 1 ; somme des `kparties_reliees` des trois boules : 5, pour 3 paires |
| triangle équilatéral $(0,0,0),(2,2,0),(2,0,2)$ | 2 | trois naissances au niveau 2 ; fusion à trois enfants au niveau $8/3$ par une boule faible |
| tétraèdre $(20,20,20),(20,0,0),(0,20,0),(0,0,20)$, intérieurs $(10,10,10),(11,10,10),(10,11,10)$ | 5 | six naissances au niveau 200 ; quatre boules de face au niveau $800/3$, chacune `components` 3, `kparties_reliees` 6, `strict_traces` 3, `cofaces` 1 ; une fusion à six enfants |
| cube $\lbrace 0,2\rbrace^3$ ; octaèdre du § 10.6 | 2 | supports : quatre diamètres et deux tétraèdres ; trois diamètres |
| cube $\lbrace 0,2\rbrace^3$ | 1 | boule interne de niveau 3 : $\mathcal{Q}_b$ garde les deux tétraèdres, de `cofaces` nul, à côté des quatre diamètres, de `cofaces` 1 ; aucun filtre par arité ni par `cofaces` |
| cercle $A,B,C,D$ puis $A,B_t,C,D$, $t=1/n$, échelle $n^{2}+1$, $n\in\lbrace 3,5,1023\rbrace$ | 2 | $\mathcal{Q}_b$ passe de $\lbrace AC,BD\rbrace$ à $\lbrace AC,B_tCD\rbrace$ ; témoin à distance $(n^{2}+1)/4$ ; `kparties_reliees` 6 inchangé |
| E5 (§ 10.3) | 2 | boule de $AC$ hors fenêtre au niveau $33/2$ ; sans ses liaisons, sommet $AC$ gardé, la fusion de niveau $83886/3563$ garde trois enfants, mais pas les mêmes ; sans ses sommets ni ses liaisons, elle n'en a que deux ; dans les deux lectures, une fusion à deux enfants apparaît au niveau 24 |
| D2 : $A=(2,10)$, $B=(18,10)$, $C=(10,20)$, $Z=(9,3)$, $W=(11,3)$ (auditeur, `de4ab58a8`) | 2 | boule faible $ABC$ ($p=0$, $q=m=3$) au niveau $1681/25$, rôle fusion à trois branches ; sa trace stricte $AB$ naît au niveau 64 (boule de $p=2$, $q=2$, hors de $\mathrm{Cat}_2$), après le niveau 41 de rang $r_b-1$ ; la descente de $AB$ aboutit à la naissance $ZW$, de niveau 1 ; sans la boule de $AB$, une fusion apparaît au niveau $145/2$ |

### 10.12 Ce que ce paragraphe n'établit pas

- Aucune implantation n'est qualifiée, aucun coût n'est borné : le nombre de supports d'une coquille étendue et le
  nombre de boules de $W_K$ se mesurent.
- La complétude de $\mathcal{Q}_b$ ne se vérifie pas sur le fichier publié, qui ne contient pas la coquille : elle
  relève des juges bornés et natifs.
- La Prop. 6 n'est pas réparée : les fusions sont décrites boule par boule.
- Aucune stabilité de la réalisation par supports n'est revendiquée (§ 10.9).
- Les entrées pondérées restent hors contrat (§ 9).

### 10.10 Sortie publiée : l'arbre couvrant d'ordre K (6 octobre 2026)

Décision de l'utilisateur du 6 octobre 2026 : ne publier que les supports de l'arbre couvrant minimal d'ordre K, sous
la forme des arêtes de Kruskal avec $S^*$ seul. La sortie publiée (`MHGP11SP` version 2) garde donc les boules de
$W_K$ de rôle naissance ou fusion, chacune avec son seul support $S^*$.

**Proposition (suffisance).** Les boules de rôle naissance et fusion, avec leurs rangs, leur rattachement et leurs
branches, déterminent $T_K$. Les boules de rôle interne n'y ajoutent aucun nœud ni aucune arête.

*Preuve.* Lemme B, point 1 : à $K\geq 2$, les naissances de $W_K$ sont en bijection avec celles de $T_K$. Lemme C,
point 3 : pour toute fusion $v$, la réunion des $\mathrm{ant}(b)$ des boules de fusion rattachées à $v$ vaut
$\mathrm{enfants}(v)$. Lemme C, point 2 : une boule interne a $\mathrm{ant}(b)=\lbrace\mathrm{att}(b)\rbrace$, et
ne relie que des $K$-parties d'un nœud déjà formé. ∎

**Sélection de Kruskal au plateau** (correction du même jour, audit `be8085ec1`). Le rôle seul ne suffit pas. À
$K=1$, les trois sites équidistants $(0,0,0)$, $(1,1,0)$, $(1,0,1)$ donnent trois boules diamétrales de même niveau,
toutes de rôle fusion, rattachées à la même multifusion : les garder toutes ferme un cycle. La sortie garde donc
chaque naissance. Pour chaque fusion $v$, elle parcourt ses boules de fusion dans l'ordre des `BallIdx` (niveau, puis
$S^*$) et unit leurs branches dans un DSU des enfants de $v$ ; une boule est gardée si elle réalise au moins une union.
Par le lemme C, point 3, les unions finissent par relier tous les enfants. La famille gardée est ainsi un arbre
couvrant de l'hypergraphe : une hyperarête qui réunit $c\geq 3$ composantes est gardée une seule fois, avec ses
branches d'origine, même si certaines de ses liaisons sont déjà redondantes. Les comptes du § 10.7 restent définis
pour toute boule, mais ils ne sont plus publiés.
