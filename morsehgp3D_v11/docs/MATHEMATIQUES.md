# Mathématiques du moteur v11

Contrat mathématique de développement, 2 octobre 2026. Ce document fixe les objets à calculer ; il ne
qualifie pas leur implémentation. Il condense L01–L03 et le brouillon privé, avec les corrections
[Q1–Q5 acceptées](../audits/AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md).
Les décisions d'implantation et leurs portes sont dans [CONCEPTION_MOTEUR.md](CONCEPTION_MOTEUR.md).
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

**P4 — projection laminaire.** Pour une règle qui attache chaque site une seule fois à un nœud vivant
à sa date d'entrée, les ensembles de sites attachés aux sous-arbres sont laminaires : deux sous-arbres
sont inclus ou disjoints. Cela prouve la laminarité, pas la qualité statistique de la règle. Les sites
inactifs restent explicitement absents, ou deviennent des singletons selon une convention séparée.

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
Cette porte est ajoutée pour la campagne G4 ; son ajout ne vaut pas exécution ni qualification native.

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
