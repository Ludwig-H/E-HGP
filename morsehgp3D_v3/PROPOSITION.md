# MorseHGP3D v3 — proposition d'architecture

> **Ce document est une proposition, pas une autorité.** L'autorité mathématique
> reste [`SPECIFICATION_MORSEHGP3D.md`](../docs/SPECIFICATION_MORSEHGP3D.md) et
> [`STATUT_PREUVES_ET_HEURISTIQUES.md`](../docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md).
> Les audits [`AUDIT_PROPOSITION.md`](AUDIT_PROPOSITION.md) et
> [`AUDIT_PROPOSITION_2.md`](AUDIT_PROPOSITION_2.md) **motivent** cette révision ;
> ils ne la certifient pas. Aucune porte, aucun statut public, aucun SLO n'est
> ouvert ici.
>
> Réécriture complète du 8 août 2026 au soir, après le second audit et son
> contre-audit. Le journal des affirmations retirées est au §14.

## 0. Convention de statut

Chaque énoncé chiffré porte l'une de ces étiquettes, et rien n'est promu sans
reçu :

| étiquette | sens |
| --- | --- |
| **[théorème]** | démontré ici ou dans une note du dépôt, avec sa preuve |
| **[mesuré]** | produit par une exécution, avec sa commande |
| **[extrapolé]** | calculé depuis une mesure sous un modèle explicite |
| **[diagnostic]** | observation utile, sans reçu reproductible — ne qualifie rien |
| **[obligation]** | à prouver ou à mesurer avant toute décision qui s'y appuie |

**[obligation] générale** : aucune des mesures citées ci-dessous ne possède
aujourd'hui de sidecar complet (schéma, commit, binaire, compilateur, machine,
digest d'entrée, graine, compteurs de campagne fermés, sortie brute). Tant que
ce n'est pas fait, elles sont des diagnostics. C'est la première dette à payer.

## 0 bis. Le plan, en cinq lignes

1. **M1, le juge** — un oracle indépendant en précision arbitraire, à campagne
   fermée, sur le domaine déclaré ; et les reçus des mesures déjà faites.
2. **M2, le prototype qui décide** — un seul programme CPU exact qui construit
   $V_k(p)$ pour une ancre, et qui **règle à lui seul PEL-1 à PEL-4** et mesure
   A1-source gratuitement.
3. **M3, le census** — les compteurs de M2 à l'échelle, sur de vrais nuages, y
   compris multi-captation, avec le ledger complet.
4. **Décision** à deux branches, puis seulement : descente indexée, tri externe
   et réducteur, GPU, reçu produit.
5. Rien de ce qui suit n'est un produit tant que M2 n'a pas parlé.

Le détail et la justification de l'ordre sont au **§13**.

## 1. Pourquoi une v3

La v2 n'a pas une conception fausse. Elle a un **substitut de force brute posé à
la place de sa conception**, qui n'a jamais été écrite : `DESIGN.md` §7 budgète un
peeling local sensible à la sortie et une descente indexée ; `src/catalogue.cpp`
énumère exhaustivement tous les quadruples du voisinage et
`src/forest.cpp::descend` balaie le nuage à chaque bras.

**[mesuré]** (2 vCPU, Release) : $n=200$, $K=10$ →
$258\,739\,800=200\cdot\binom{199}{3}$ quadruples en 26,3 s ; $n=500$, $K=2$ →
plus de 300 s. $W_p$ vaut le nuage entier, à tout $K$.

### 1.1 La cause racine, lue dans le code

`src/catalogue.cpp:380` : `radius_bound` rend `+infini` dès que
`vals.size() < s_max`. La relaxation conique n'admet que les points à moins de
$49{,}7^\circ$ de l'axe — $17{,}7\ \%$ de la sphère ; il suffit qu'**un** cône sur
42 contienne moins de $s_{\max}$ points admissibles. Alors `diam_cut = +infini`
(`:290`), `std::isfinite` est faux (`:296`), toutes les paires restent vivantes,
et la triple boucle visite $\binom{\lvert W\rvert}{3}$ triplets.

### 1.2 Même le théorème 4 exact ne sauve pas l'énumération locale

**[mesuré]** $\lvert W\rvert_{\text{cert}}$ à $n=50\,000$, $K=10$, par bissection
sur le plus petit $\rho$ vérifiant $2\tau\leq\rho$ ; **[extrapolé]** les temps, à
partir du débit **[mesuré]** de $3{,}2$ à $8{,}9\cdot10^{6}$ quadruples/s/cœur
sur 48 cœurs — l'intervalle est donné, pas un point.

| configuration | $\lvert W\rvert$ | quadruples | temps (48 cœurs) |
| --- | ---: | ---: | ---: |
| code actuel | $n-1$ | $1{,}04\cdot10^{18}$ | 77 à 214 ans |
| croissance réparée, $\theta$ du dépôt | 17 578 | $4{,}53\cdot10^{16}$ | 3,4 à 9,3 ans |
| + golden angle corrigé | 5 764 | $1{,}59\cdot10^{15}$ | 43 à 120 jours |
| + 2 000 cônes | 252 | $1{,}32\cdot10^{11}$ | 309 à 859 s |
| **théorème 4 exact, $\theta=0$** | **175** | $4{,}39\cdot10^{10}$ | **103 à 286 s** |
| voisinage parfait observé | 130 | $1{,}79\cdot10^{10}$ | 42 à 117 s |
| *ce qu'il faudrait pour 100 ms* | — | $1{,}5$ à $4{,}3\cdot10^{7}$ | 0,1 s |

avec le modèle fermé
$\lvert W\rvert_{\text{cert}}=8\,s_{\max}\cos\theta/(\cos\theta-\sin\theta)^4$
**[extrapolé, vérifié contre la mesure]**.

Le budget de 100 ms impose $\binom{\lvert W\rvert}{3}\lesssim10^{3}$, soit
$\lvert W\rvert\leq18$ ; une seconde impose $\lvert W\rvert\leq38$ au meilleur
débit publié. **[obligation]** ces lignes remplacent le coût réel
$\sum_p\binom{\lvert W_p\rvert}{3}$ par $n\binom{\overline{\lvert W\rvert}}{3}$ :
par convexité c'est une **sous-estimation**, et les queues peuvent gouverner le
temps. La distribution complète de $\lvert W_p\rvert$ doit accompagner le tableau,
et chaque ligne doit dire si son $\lvert W\rvert$ est une moyenne, un quantile ou
un maximum. **[mesuré,
sur le corpus testé]** le support le plus lointain d'une sphère critique émise
est le 89ᵉ à 165ᵉ voisin, et il faut le 189ᵉ pour compter les rangs — donc
$\lvert W\rvert\geq130$ **sur ce corpus**, ce qui n'est pas une borne universelle
de la géométrie à $K=10$. **[mesuré]** rendement : $1{,}5$ à
$2{,}4\cdot10^{-3}$ sphère par quadruple testé, soit 400 à 650 candidats jetés
par sphère gardée.

**Conclusion, robuste à tout l'intervalle : l'énumération locale exhaustive des
quadruples est condamnée**, quels que soient $\theta$, le nombre de cônes et la
qualité de la borne.

### 1.3 Le second mur : la descente de la forêt

`build_forest::descend` (`forest.cpp:69-89`) cherche un intrus en parcourant les
points du nuage, avec **sortie au premier intrus** (`:80`, `break`). Son coût
réel est donc l'**indice du premier intrus**, pas $n$ — un balayage complet n'a
lieu que lorsque les intrus se raréfient, c'est-à-dire près des minima et sur les
bras qui échouent.

**[diagnostic, pire cas, non qualifiant]** $\approx10^{12}$ prédicats par ordre.
Ce chiffre est en outre trop agrégé : les événements de rang $k+1$ se
**répartissent** entre les ordres, donc appliquer « 21 % du catalogue » à chacun
des dix ordres dépasserait le catalogue total. **[obligation]** publier
l'histogramme $S_k$, le maximum de la carte des minima, le nombre de bras, de
pas, de visites d'index, de plateaux et de censures **par ordre**, ainsi que la
distribution de l'indice du premier intrus — pas un produit de cardinaux.

Le remède, lui, est un énoncé propre :

> **[théorème] Lemme de localité de la descente.** Si $F'\subseteq F$ et si
> $(c,r)$ est la sphère de l'événement avec $F\subseteq\bar B(c,r)$, alors le
> centre $c'$ de la miniboule de $F'$ vérifie
> $c'\in\mathrm{conv}(F')\subseteq\bar B(c,r)$ et son rayon $r'\leq r$ ; donc tout
> $x\in\bar B(c',r')$ vérifie
> $\lVert x-c\rVert\leq r'+r\leq 2r$.

**Mais ce lemme ne vaut que pour le PREMIER pas.** Après un remplacement,
$F_{i+1}$ contient l'intrus et n'est plus inclus dans $F$. On a seulement
$F_{i+1}\subseteq\bar B(c_i,r_i)$, donc $c_{i+1}\in\bar B(c_i,r_i)$ et
$\bar B(c_{i+1},r_{i+1})\subseteq\bar B(c_i,2r_i)$ : l'enveloppe centrée en $c_0$
croît comme $R_{i+1}\leq R_i+r_0$, soit $R_i\leq(i+1)\,r_0$ — **linéaire en
nombre de pas, jamais bornée par $2r$**. Employer $\bar B(c,2r)$ pour toute la
chaîne serait une troncature non prouvée.

**La référence est donc la requête dans la miniboule COURANTE
$\bar B(c_i,r_i)$ à chaque pas** — une requête d'existence à sortie anticipée,
qui ne demande aucun lemme. Le lemme reste un bon accélérateur du premier pas.

**[obligation]** ni le lemme ni la requête ne prouvent la terminaison, l'unicité
du minimum atteint, le traitement des plateaux, le caractère canonique du choix
de la victime (`forest.cpp:84` prend `mb.support[0]`, un choix arbitraire), ni la
complétude des incidences silencieuses. L'identité byte-à-byte avec le balayage
exhaustif reste à établir.

### 1.4 Le troisième mur : la mémoire

**[diagnostic]** $\approx450$ à $510$ sphères critiques distinctes par point en
régime intérieur, stable de $n=5\,000$ à $200\,000$ — donc
$\approx2{,}3\cdot10^{7}$ sphères à $n=50\,000$, soit $\approx4{,}4$ Go à
$160+32$ octets. Ces chiffres viennent d'une source v2 dont la complétude, la
canonicité et le domaine ne sont pas certifiés : ils ne qualifient rien.

**Ce qui doit rester quel que soit le chiffre** : l'architecture v3 **ne doit
exiger aucune matérialisation globale du catalogue**, le sink est consommé en
flux par le réducteur, et le high-water mémoire est publié.

### 1.5 Trois quantités à ne jamais confondre

1. la **boule tangente non contrainte** — centre libre, c'est ce que calcule
   `radius_bound` de la v2, et c'est elle qui vaut $+\infty$ sur les points de
   faible profondeur ;
2. $R(p)$, **supremum** des rayons de boules passant par $p$, **de centre dans
   $\mathrm{conv}(X)$**, de contenu $\leq s_{\max}$ — celle de la germination ;
3. la **sphère critique**, qui exige de plus
   $c\in\mathrm{relint}\,\mathrm{conv}(U)$.

**[théorème]** $R(p)\leq\mathrm{diam}(X)$ **pour tout $p$** : si
$c\in\mathrm{conv}(X)$ et $p\in X$, alors $r=\lVert c-p\rVert\leq\mathrm{diam}(X)$.
**$R$ n'est donc jamais infinie.**

Deux conséquences, et elles sont importantes :

- une énorme boule vide posée sur une surface a tous ses appuis du même côté :
  son centre n'est pas dans leur enveloppe, **ce n'est pas une sphère critique**.
  L'inférence « $\tau=+\infty$ donc grandes sphères critiques » de la version
  précédente est fausse ;
- la restriction $D\leq2R(p)$ **ne peut jamais mordre par finitude**, seulement
  par la *valeur* de $R(p)$. Tout le cadrage « points peu profonds » est donc
  hors sujet pour la germination ; **la seule mesure qui compte est la
  distribution de $R(p)$** — **[obligation]**, elle n'a pas été faite.

Ce qui subsiste, et qui suffit à condamner la v2 : **sa** borne, non contrainte,
est infinie sur ces points, donc son voisinage ne peut pas être dimensionné.

**[diagnostic]** census du 8 août sur le Stanford bunny (dix captations brutes
recalées par `bun.conf`, 362 272 points ; et sa reconstruction fusionnée
`bun_zipper`, 35 947 points), critère **non contraint**, 4 096 directions,
`census_tukey_shallow_20260808.json` :

| nuage | $n$ | $s_{\max}=3$ | $s_{\max}=11$ |
| --- | ---: | ---: | ---: |
| bunny, 10 captations recalées | 50 000 | 1,06 % | 2,21 % |
| cube uniforme volumétrique | 50 000 | 0,48 % | 1,32 % |
| bunny, 10 captations recalées | 20 000 | 1,80 % | 3,88 % |
| bunny, reconstruction fusionnée | 20 000 | 5,86 % | 10,43 % |
| cube uniforme volumétrique | 20 000 | 0,95 % | 2,83 % |

Ce census mesure la quantité **non contrainte**, donc il diagnostique la v2 et
**rien d'autre** : la version précédente en tirait un majorant de l'ensemble où
la borne convexe échoue, ce qui est **vacue** puisque cet ensemble est vide. La
première observation sur un vrai nuage multi-captation est intéressante — le
nuage multi-captation est *moins* peu profond que la reconstruction fusionnée,
ce qui est cohérent avec l'épaisseur donnée par l'erreur de recalage — mais le
transfert à SemanticKITTI et aux familles sanctionnées reste **ouvert**, et la
causalité n'est pas démontrée.

### 1.6 Deux défauts v2 à ne pas perdre

- **`certified` est un faux positif** : `catalogue.cpp:471`, `:480`, `:484`
  mettent tous `ok = true`, y compris les deux sorties par épuisement, et `:500`
  écrit `certified[i] = 1`. Le reçu annonce 100 % de certification pendant que
  $\lvert W_p\rvert=n-1$. Trois drapeaux distincts sont nécessaires.
- **Trou de correction latent, non gardé** : `classify` compte le rang dans $W$
  seulement ; une boule de rayon $r>\rho/2$ verrait ses membres hors de $W_\rho$
  invisibles. **[mesuré]** inactif (0 sur 8 770), non gardé. Une ligne : rejeter
  toute sphère telle que $4\beta(s)>\rho^2$.

## 2. Le contrat, et son ledger

Hiérarchie de HARTIGAN **exacte** jusqu'à $K=10$ sur $n=50\,000$ points réels, en
moins d'une seconde, cible 100 ms.

Le budget ne se compte **pas** en nanosecondes par objet émis. Il se compte en
**ledger de bout en bout** : candidats rejetés, requêtes d'index, octets écrits,
tris, replis exacts, sink, réduction, synchronisations, high-water mémoire. Un
quotient sur la seule sortie masque exactement les postes qui tuent.

## 3. Domaine

### 3.1 Deux profils, jamais confondus

La spécification interprète chaque binary64 d'entrée comme un **dyadique exact**.
Quantifier sur une grille $2^{16}$ est un **autre problème** : la quantification
peut changer l'ordre des distances, le rang fermé, les égalités d'arêtes, les
cosphéricités et la topologie des lots simultanés. **Zéro collision ne prouve pas
l'équivalence géométrique.**

1. **`exact_dyadic_input`** — autorité sur les binary64 originaux, filtres
   d'intervalles dirigés, repli multiprécision ;
2. **`quantized_u16_input`** — autorité sur le nuage entier produit *seulement*,
   avec transformation, origine, échelle, écrêtages, collisions, multiplicités et
   digest source→cible au reçu.

Une sortie du second ne porte **jamais** `public_status=exact` relativement au
premier.

### 3.2 `RelevantGP` par référence, pas par raccourci

`RelevantGP` **n'est pas** « absence de coquille cosphérique ». Sa définition
normative (spécification §12) exige au moins : points distincts, support unique
et affinement indépendant, centre dans l'intérieur relatif, barycentriques non
nulles, shell complet sans point extérieur, prédicats et égalités exacts. Cette
proposition l'emploie **par référence** ; un balayage des événements acceptés ne
suffit pas à établir `relevant_gp_complete`.

### 3.3 Ce qu'on n'a pas le droit de supposer

Ni surface, ni volume, ni densité, ni forme étoilée, ni origine capteur, ni image
de distance. Certains nuages sont **recomposés de plusieurs captations** : il n'y
a ni paramétrisation commune, ni visibilité mono-retour. Ces structures peuvent
**proposer** du travail ; le complément exact doit rendre la même sortie sans
elles.

Corollaire de conception : **la correction est inconditionnelle, la sélectivité
est data-dépendante et doit être publiée par exécution.** Un nuage sur lequel les
coupes ne mordent pas doit produire un résultat *lent*, pas faux.

## 4. Le générateur : quatre objets

| | rôle | statut |
| --- | --- | --- |
| **A1-source** | source complète d'ancres diamétrales | **pièce ouverte** |
| **A2e** | peeling **2D** ancré par une **arête** | cœur algorithmique, complet *si* A1-source l'est |
| **A2p** | peeling **3D** ancré par un **point** (dual inversif) | complet par construction ; oracle, et candidat |
| **A2pe** | **l'unification** : A2e est la 2-face de A2p | **recommandé, sous trois obligations (§6)** |

### 4.1 A2e — la réduction de dimension

Ancre diamétrale $e=pq$, $d=q-p$, $D^2=d\cdot d$, $M=(p+q)/2$, deux vecteurs
entiers indépendants $b_1,b_2\perp d$, $B=[b_1\ b_2]$, centre $c=M+Bt$.

JUNG donne l'ellipse exacte
$J_e^{(4)}=\lbrace t: t^{\mathsf{T}}(B^{\mathsf{T}}B)t\leq D^2/8\rbrace$, et
chaque point $x\notin\lbrace p,q\rbrace$ la forme **affine**

$$h_x(t)=2(Bt)\cdot(x-M)-\left(\lVert x-M\rVert^2-\frac{D^2}{4}\right)
= r^2-\lVert x-c\rVert^2 .$$

Intérieur strict, shell et extérieur sont donc les **signes d'une droite** :
aucune base orthonormale, aucune racine carrée, tous les signes entiers ou
rationnels exacts.

Sur l'ellipse, chaque point est intérieur constant (compté dans $c_e$), extérieur
constant (éliminé), ou **droite active** ($m_e$). Si $c_e>s_{\max}-4$, l'ancre ne
porte aucun support quatre. Sinon, avec $\kappa_e=s_{\max}-4-c_e$ :

$$\textbf{[théorème]}\qquad \mathrm{rang}_{\text{ferm\'e}}(p,q,z,w)=4+c_e+\delta_e(t),$$

$\delta_e(t)$ étant le nombre de demi-plans actifs strictement positifs au sommet
$t$. **Le rang est une profondeur d'arrangement 2D.** Et **[théorème classique]**
le $\leq\kappa$-level de $m$ droites a $\Theta(m(\kappa+1))$ sommets, d'où

$$Z_e\leq m_e(\kappa_e+1),\qquad Z_e\leq 8m_e \text{ pour } s_{\max}=11,\ c_e=0 .$$

C'est le saut : la cascade développe des tuples *puis* interroge leur rang ; A2e
**calcule le rang pendant la génération**, ce qui retire la requête de boule
fermée du chemin chaud.

**[obligation] une borne de sortie n'est pas un constructeur.** Un prototype qui
forme toutes les intersections puis filtre reste en $\Theta(m_e^2)$ même s'il
respecte $Z_e$. Restent à fermer : coefficients rationnels larges, clipping
elliptique exact, droites parallèles, concurrences, ordre total et
reproductibilité, graphe de conflits, scratch borné, ordonnancement des ancres
lourdes, mapping GPU sans phase sérielle dominante.

**Les quatre arités sont quatre problèmes.** Support un : rayon nul,
multiplicités. Support deux : centre $M$, profondeur en $t=0$, shell complet
malgré tout. Support trois : circumcentre = point de $h_z=0$ minimisant
$t^{\mathsf{T}}(B^{\mathsf{T}}B)t$, rang $3+c_e+\delta_e$, **ellipse et seuils
différents**. Support quatre : sommets shallow. Au rang fermé 11, réfuter tout
support trois demande **neuf** témoins stricts, tout support quatre **huit** :
une preuve d'arité quatre ne se propage jamais à l'arité trois.

### 4.2 A1-source — la pièce ouverte, et pourquoi elle est dure

JUNG confine les centres **une fois la paire connue**. Il ne fournit pas
l'énumération sparse et complète des paires utiles. Trois voies :

- balayer $\binom{n}{2}$ — complet, incompatible avec le produit
  (**[mesuré]** $1{,}92\ \mu$s/paire, soit 2 400 s à 50 k) ;
- RNG ou catalogue de paires de rang borné — **[théorème négatif]** réfuté :
  pour tout $q$ fini il existe un support de rang fermé 11 dont le RNG d'ordre
  $q$ n'est pas une clique (`RNG_JUNG_CLIQUES_ET_NIVEAUX_PEUPROFONDS.md` th. 1) ;
- complément **fail-open par self-join du LBVH et center-cover**, voie
  `P15-HOCUDA-P1` — parcimonie et débit **non qualifiés**.

Donc : **complétude de A2e conditionnelle à A1-source ; parcimonie non prouvée.**
L'observation de 4,5 M paires retenues à une seule taille n'établit pas
$\Theta(n)$.

Et l'ellipse compacte de A2e ne supprime pas le problème des grandes sphères,
**elle le déplace** : un grand $D$ donne une grande ellipse, donc un grand $m_e$
(**[mesuré]** jusqu'à 25 026 points de voisinage sur `eight_clusters`). Ce qui
serait une cellule non bornée chez A2p devient une charge chez A2e, et elle
atterrit intégralement dans A1-source.

## 5. A2p — le dual inversif, complet par construction

Ancrons en $p$ ; pour tout autre $u$, poser

$$H_u=\left\lbrace c:\ 2\langle c-p,\ u-p\rangle=\lVert u-p\rVert^2\right\rbrace,$$

à coefficients **entiers** sur la grille. Alors $u$ est strictement dans la
sphère passant par $p$ centrée en $c$ ssi $c$ est du côté positif, donc

$$\mathrm{niveau}(c)=\#\lbrace u:\lVert u-c\rVert<\lVert p-c\rVert\rbrace,$$

et **sur une face de dimension $j$** de l'arrangement, les porteurs sont au
nombre de $4-j$, d'où la formule générale

$$\mathrm{rang}_{\text{ferm\'e}}=(4-j)+\mathrm{profondeur}.$$

L'écriture « $1+\mathrm{niveau}$ » n'est que le cas $j=3$ (intérieur d'une
cellule) et ne doit pas être employée sans sa convention de porteurs.

La région utile est
$V_k(p)=\lbrace c:\ p\ \text{est parmi les}\ k+1\ \text{plus proches de}\ c\rbrace$,
dont le treillis de faces donne **toutes** les sphères critiques contenant $p$ :
sommet $\leftrightarrow\lvert U\rvert=4$, arête $\leftrightarrow3$, face
$\leftrightarrow2$.

C'est la cellule de VORONOÏ d'ordre $\leq k$ de $p$ — objet classique.
**[théorème classique]** le $\leq k$-level de $m$ plans de $\mathbb{R}^3$ a une
complexité $\Theta(mk^2)$ ; **[diagnostic]** la mesure donne 450 à 510 cellules
par point, soit un comportement constant en $n$ et compatible avec $\Theta(k^2)$
par point — donc une sortie totale $\Theta(nk^2)$.

**Deux propriétés qu'A2e n'a pas.**

1. **L'ancrage est complet sans rien à prouver** : tout support contient au moins
   un point, donc ancrer en chaque point est exhaustif. Il n'y a pas de A1-source.
2. **La localité devient un certificat *a posteriori*.** Comme
   $\mathrm{dist}(p,H_u)=\tfrac12\lVert u-p\rVert$, en insérant les points par
   distance croissante on peut s'arrêter dès que la demi-distance du prochain
   point dépasse le rayon de la région courante : **aucun point restant ne peut
   plus la couper**. C'est exact, entier, sans relaxation conique — donc sans le
   $+\infty$ qui a tué la v2. C'est une recherche par file de priorité sur le
   LBVH, avec condition d'arrêt certifiée.

**Le prix** : arrangement 3D au lieu de 2D, prédicats plus lourds, cellules non
bornées à traiter, et duplication $\leq4$ (chaque support est vu depuis chacun de
ses $\leq4$ sommets), réduite à l'émission par le propriétaire canonique mais pas
en travail.

## 6. A2pe — l'unification, et ce qu'elle demande

**L'observation.** Une 2-face de $V_k(p)$ portée par $H_u$ est exactement le plan
médiateur de $(p,u)$, et les traces $H_u\cap H_v$ sont exactement les droites
$h_v$ du §4.1. **A2e n'est pas une alternative à A2p : c'est sa restriction à une
face.**

**La conséquence.** Les 2-faces de $V_k(p)$ **sont** les ancres de A2e, complètes
et par construction — A1-source disparaît. Et le théorème négatif du §4.2 ne s'y
oppose pas : il réfute « paire de rang fermé $\leq s_{\max}$ » (boule diamétrale),
alors que la 2-face signifie « il existe une boule par $p$ et $u$ de contenu
$\leq s_{\max}$ ». **[théorème]** ces deux conditions diffèrent : pour un support,
$c$ est sur le plan médiateur donc $r^2=\lVert M-c\rVert^2+D^2/4$, et l'inclusion
de la boule diamétrale dans la circumboule exigerait $r\leq D/2$, c'est-à-dire
l'égalité. La réfutation ne transporte donc pas.

**L'architecture recommandée** en découle :

> ancrer en chaque point ; construire $V_k(p)$ par insertion en distance
> croissante avec arrêt certifié ; lire les supports par dimension de cellule, la
> 2D de A2e servant de sous-routine sur chaque face ; n'émettre qu'au
> propriétaire canonique ; descendre la forêt par requête de boule
> $\bar B(c,2r)$ ; consommer en flux.

**[obligation] — quatre, et elles conditionnent tout le §6.** Elles sont
étiquetées `PEL-*` et non `M*`, pour ne pas entrer en collision avec
l'obligation normative M.1 du registre.

1. **PEL-1 — les 2-faces donnent exactement les arêtes utiles.** L'argument
   ci-dessus est une double inclusion informelle ; il faut une preuve propre, en
   particulier sur les faces non bornées et les égalités.
2. **PEL-2 — sensibilité à la sortie du parcours.** Cellules visitées
   $=O(\text{sortie})$ et non $O(\text{sortie}\times m)$. **[à noter]** l'écart
   entre la borne classique $\Theta(mk^2)$ (soit $1{,}75\cdot10^4$ par point à
   $m=175$, $k=10$) et la mesure (450 à 510) est d'un facteur $\approx38$ : le
   pire cas n'est pas atteint, mais rien ne le garantit.
3. **PEL-3 — cellules non bornées.** L'énoncé « cellule non bornée $\Rightarrow$
   pas de sphère critique finie » est **probablement faux tel quel** : une
   cellule polyédrique fermée non vide, même non bornée, possède une projection
   finie de l'origine. Il faut une condition supplémentaire portant sur le bon
   centrage, le support et le rang — et la terminaison du parcours dans ces
   directions.

**[obligation] PEL-4 — le coût du prédicat exact en 3D contre 2D.** C'est
l'arbitrage central entre A2pe et A2e seule : A2pe supprime A1-source au prix
d'un arrangement de dimension supérieure. Il faut le mesurer avant de trancher.

## 7. La coupe $R(z)\geq D/2$

> **[théorème]** $U$ support minimal bien centré, circumboule $\bar B(c,r)$ de
> contenu $\leq s_{\max}$, $D=\mathrm{diam}(U)$. Alors pour tout $z\in U$ :
> $R(z)\geq r\geq D/2$, avec $R$ le **supremum** des rayons de boules passant par
> $z$, de centre dans $\mathrm{conv}(X)$, de contenu $\leq s_{\max}$.

*Preuve.* $z\in U\subseteq\partial B$ donc $\bar B$ est une telle boule, de centre
dans $\mathrm{relint}\,\mathrm{conv}(U)\subseteq\mathrm{conv}(X)$ : $R(z)\geq r$.
Et $U\subseteq\bar B$ donne $D\leq2r$. $\square$

Trois précisions :

1. $R$ est un **supremum** : avec des boules fermées la population saute quand un
   point atteint le shell, la borne peut ne pas être atteinte ;
2. ce $R$ n'est **pas** la quantité tangente non contrainte du §1.5, et il est
   **toujours fini** — la coupe ne mord que par sa valeur ;
3. **la coupe ne peut PAS filtrer la profondeur.** Le lemme est une condition
   nécessaire pour être **porteur**. Il ne dit rien d'un point $y\notin U$ situé à
   l'*intérieur* de la sphère : sa forme affine $h_y$ peut rester strictement
   positive au centre candidat et **doit** alors compter dans $c_e$, dans
   $\delta_e$ et dans le rang fermé. Élaguer $y$ du range-report sous-compterait
   le rang et publierait des sphères de rang $>K$.

Il faut donc **deux flux explicitement distincts** :

| flux | contenu | filtre autorisé |
| --- | --- | --- |
| **témoin / profondeur** | toutes les formes susceptibles de compter comme intérieures | **aucun** |
| **`carrier_eligible`** | ce qui a le droit d'engendrer un support | $2R(z)\geq D$, certifié |

Les emplois **licites** de la coupe sont donc : (i) la sélection des **ancres**,
où les deux extrémités sont porteuses ; (ii) le masque `carrier_eligible` sur les
droites, pieds et intersections. L'emploi **interdit** est le flux témoin.

Si un agrégat d'index est employé pour (i) et (ii), il doit s'appeler
`max_two_R_upper_hi` — **pas** `max_tau_hi`, qui réintroduirait la confusion
$\tau$/$R$ du §1.5 — être défini comme intervalle, et rester **fail-open** sur
non fini, sous-normal, débordement ou intervalle traversant le seuil.

Dans A2pe, la coupe sert surtout à **séparer les régimes** (petites ancres en
flux, grandes ancres en file), pas à élaguer.

## 8. Canonicité, égalités, arithmétique

**Propriétaire canonique** : la plus petite paire lexicographique parmi les arêtes
de longueur **maximale** du support, sur des `PointId` stables, testée **avant**
le sink. Sur une grille entière l'égalité de deux longueurs d'arête n'est pas
rare : la règle de départage est obligatoire.

**Égalités** : une concurrence exacte de $t$ droites est **un lot unique**, pas
$\binom{t}{2}$ sommets perturbés. Shell complet compté avant émission ; hors
`RelevantGP`, l'autorité est retirée ou une voie dégénérée explicitement
certifiée est prise. Sortie exigée byte-à-byte identique sous permutation
d'entrée, nombre de fils et ordonnancement GPU. **[mesuré]** en v2, $55\ \%$ des
sphères sont conservées chez un propriétaire non canonique même en mono-thread,
et aucun digest reproductible n'est possible.

**Arithmétique de production** : entière, sans allocation, largeur auditée.
**[extrapolé, dérivation symbolique à reprendre au registre]** majorants pour
$M=65535$ : $\lvert\mathrm{num}_i\rvert<2^{84{,}96}$, $\mathrm{den}<2^{68{,}17}$,
$\mathrm{num2}<2^{169{,}93}$, $\mathrm{den}^2<2^{136{,}35}$, comparaison de deux
niveaux $<2^{306{,}28}$. `sphere.hpp` de la v2 y est compatible
(`BigInt<4>`/`BigInt<6>`) et **[mesuré]** son `sphere_cmp_beta` est d'accord avec
GMP sur 18 601 paires — c'est un **candidat à reprendre après audit prédicat par
prédicat**, pas un composant qualifié.

Ces majorants ne concernent que **`quantized_u16_input`**. Ils ne dimensionnent
pas `exact_dyadic_input`, dont les exposants binary64 et les produits
intermédiaires exigent leur propre dérivation, leur propre repli et un SLO
séparé. **[obligation]**

**Oracle** : précision **arbitraire**, représentation *différente* de la
production. **[mesuré]** elle décide la grille déclarée là où l'`i128` décide
$0/40$ nuages. Ce n'est pas une exclusivité mathématique — une largeur fixe
indépendante suffisamment grande déciderait aussi un domaine entier borné ; c'est
le choix **robuste**, parce qu'il supprime la question de la largeur au lieu de
la re-prouver à chaque site.

## 9. Ce que la v2 fournit, et ce qu'elle ne fournit pas

Elle fournit une **sémantique candidate** (lots exacts, multifusion contractée,
censure atomique, source exacte par nœud), des **fixtures** et des
**contre-exemples**. Elle ne fournit **aucune certification** : l'oracle qui a
produit « 1 462 nuages, 89 247 cas » déborde en arithmétique signée avant son
garde, saute des nuages en silence, accepte une campagne vide ou censurée, ne
compare ni arités, ni enfants, ni racines, ni sources, et tire ses coordonnées
dans $[0,120]$ — **[mesuré]** sur la grille déclarée il décide zéro nuage sur
quarante en annonçant `OK`.

## 10. Architecture GPU

**J10 borne un rayon spatial, pas une cardinalité** — **[mesuré]** voisinage
maximal 25 026 points sur `eight_clusters`. Aucune tuile ne peut donc supposer
« quelques dizaines de points en mémoire partagée ».

| étage résident | rôle | structure globale évitée |
| --- | --- | --- |
| canonicalisation | `PointId`, domaine, digest, `RelevantGP` | copie ambiguë de l'entrée |
| LBVH | range-report, self-join, `max_tau_hi` | matrice paire–point |
| propositions | RNG, image de distance, heuristiques | aucune autorité au proposeur |
| ancres | 2-faces de $V_k(p)$, ou center-cover | tableau des $\binom{n}{2}$ paires |
| shallow | niveaux peu profonds, rang = profondeur | $\sum_e\binom{m_e}{2}$ |
| décision exacte | diamètre, shell, bon centrage, owner | rescans globaux par candidat |
| source HGP | facettes, cofaces, silences, couverture | $\Gamma$ global |
| **tri et lots** | ordre global par niveau exact, groupement des égaux | catalogue géométrique global |
| réduction | lots, attaches, forêts, verticales | mosaïque d'ordre supérieur |

Ancres par classes de charge : warp, CTA sous-tuilé, file persistante.

**Le flux ne dispense pas du tri global exact.** Des ancres indépendantes
produisent les événements dans un ordre arbitraire, alors que le réducteur exige,
par ordre : un ordre global par **niveau rationnel exact**, le groupement de
**tous** les événements de niveau égal, un instantané pré-lot, puis une
application atomique — et déduplication, propriétaire, incidences silencieuses et
`coverage_delta` doivent respecter ce même ordre. Aucun producteur monotone entre
ancres indépendantes n'est énoncé ni plausible.

La voie compatible avec l'invariant mémoire du §1.4 est un **tri externe** :
produire des **runs bornés** triés par clé exacte et identifiants canoniques, les
fusionner par un merge déterministe, **réunir les clés rationnellement égales
avant toute mutation**, puis alimenter le réducteur. À publier : taille des runs,
high-water RAM, octets écrits et lus, coût des comparaisons exactes, nombre et
taille maximale des lots. Une variante entièrement résidente est acceptable si
elle prouve les mêmes bornes. On évite le catalogue géométrique global ; on ne
peut pas éviter l'**ordonnancement global de ses événements utiles**.

**Les deux pistes sont parallèles** : oracle CPU multiprécision indépendant pour
l'autorité ; CUDA `proposal_only` très tôt pour falsifier masses, divergence et
mémoire ; aucune promotion avant parité.

## 11. Le contrat du générateur n'est pas le contrat HGP

Émettre les supports est un composant géométrique. La sortie normative exige
aussi les facettes et cofaces utiles, les incidences **actives et silencieuses**,
les attachements et remplacements de la descente, les lots de niveau exactement
égal, `coverage_delta` et `coverage_log`, et les applications verticales entre
ordres avec leurs carrés de naturalité. **La source complète, les incidences
silencieuses, les verticales et la descente sont des points durs indépendants du
générateur.**

## 12. Portes

**Gate A — autorité documentaire.** Reprendre A2e, le lemme $2r$, la coupe
$R(z)\geq D/2$, $R\leq\mathrm{diam}(X)$ et les profils d'entrée dans la
spécification et le registre ; classer chaque énoncé `proved_here`,
`conditional_theorem`, `proof_obligation` ou `experimental_target`.

**Gate B — reçus.** Sidecars complets pour chaque mesure de ce document ; digests
des entrées, binaires et sorties ; identité de campagne fermée ; distinction
stricte mesure / extrapolation / théorème. Le census bunny devient une fixture
reproductible, puis SemanticKITTI et les familles sanctionnées.

**Gate C — oracle indépendant.** Multiprécision, représentation distincte,
exhaustif à petit $n$, campagne fermée
(`attempted = decided + rejected_domain`), comparaison structurelle complète
(arités, enfants, racines, sources, nombre canonique de nœuds).

**Gate D — census de charge.** Distribution de $R(p)$ ; $\Sigma_e m_e$,
$\Sigma_e Z_e$, $c_e$ ; p50/p95/p99/max de $\lvert W_e\rvert$, $m_e$, $Z_e$ ;
rétention de chaque porte ; taille de la sortie canonique acceptée ; queues,
replis, high-water. **À $n$ de plusieurs milliers** — ce n'est pas la même
campagne que Gate C, et l'une ne tient pas lieu de l'autre.
**Décision à deux branches** : sortie énorme ⇒ réviser le SLO ; sortie sparse
mais intermédiaires denses ⇒ **architecture no-go**.

**Gate E — shallow CPU exact.** Constructeur sans travail en $\sum_e m_e^2$ ;
comparaison à l'oracle exhaustif et au brute-force local ; permutations,
égalités, parallèles, concurrences, limites de l'ellipse ; transcript de conflits
et sortie canonique. Obligations propres à A2e (couverture des ancres,
classification exacte, préfixe shallow, clipping, profondeur et conflits,
concurrences groupées, arités deux à quatre, shell/centrage/owner, transcript) —
et, **séparément**, obligations propres à A2p (dictionnaire, atteignabilité, coût
global sur $n$ ancres, cellules non bornées, duplication). Les deux listes ne
s'impliquent pas.

**Gate F — descente indexée.** Fixture permanente du lemme $\bar B(c,2r)$ ;
différentiel balayage global contre requête ; terminaison, décroissance stricte,
victime canonique, plateaux ; compteurs **par ordre**, jamais extrapolés du
total.

**Gate G — source HGP et réduction**, puis **Gate H — publication,
déterminisme, et produit sans budget configuré** : qualifier d'abord la seconde,
puis seulement 100 ms, **par famille sanctionnée**.

## 13. Ordre des travaux

1. **Gate B d'abord sur ce qui existe déjà** — rendre reproductibles les mesures
   citées ici. C'est peu de travail et cela change leur statut.
2. **Gate C**, l'oracle : rien ne doit se construire au-dessus d'une porte qui
   décide zéro nuage en annonçant `OK`.
3. **PEL-1 et PEL-3** (§6) : la preuve de l'unification. C'est court, c'est
   mathématique, et cela décide entre A2pe et A2e + A1-source.
4. **Gate E** sur la voie retenue, avec son oracle brute-force local.
5. **Gate D** à l'échelle, sur de vrais nuages, y compris multi-captation.
6. **Gate F**, la descente.
7. Puis seulement source HGP, GPU, publication.

**GO immédiat** : preuves PEL-1/PEL-3, prototype CPU exact du shallow, reçus.
**NO-GO immédiat** : produit v3 avant PEL-1 à PEL-4 ; toute revendication `exact`, tout
SLO, toute autorité publique ; et présenter une mesure de ce document comme une
qualification.

## 14. Journal des affirmations retirées

| affirmation | sort |
| --- | --- |
| « complétude A1 = théorème » | conditionnelle à A1-source (§4.2) |
| $\tau=+\infty\Rightarrow$ grandes sphères critiques | faux (§1.5) |
| « surface ⇒ faible profondeur presque partout » | census, pas théorème (§1.5) |
| census = majorant de l'échec de $R$ convexe | **vacu** : cet ensemble est vide (§1.5) |
| A1 contre A2, dichotomie | quatre objets (§4) |
| gain $9{,}5\cdot10^9\to1{,}2\cdot10^8$ | invalide sans range-report indexé (§7) |
| « J10 rend la tuile GPU sûre » | borne spatiale, 25 026 points mesurés (§10) |
| « le point dur n'est ni le générateur ni la forêt » | retiré (§1.3, §11) |
| forêt v2 et O2 « certifiés » | sémantique candidate (§9) |
| $267$ ns par objet | ledger de bout en bout (§2) |
| grille $2^{16}$ comme domaine | deux profils disjoints (§3.1) |
| `RelevantGP` = absence de cosphéricité | définition normative par référence (§3.2) |
| 4,5 M paires $\Rightarrow\Theta(n)$ | retiré (§4.2) |
| « GPU dès la première ligne » vs « pas de CUDA avant » | deux pistes parallèles (§10) |
| catalogue matérialisable | flux obligatoire ; 4,4 Go en diagnostic (§1.4) |
| 69 ans / 91 s | intervalle de débit publié : 77–214 ans, 103–286 s (§1.2) |
| $\lvert W\rvert\geq130$ « géométriquement nécessaire » | observé sur le corpus mesuré (§1.2) |
| `std::map` sur $2{,}3\cdot10^{7}$ clés | carte **par ordre**, rang $=k$ seulement (§1.3) |
| $10^{12}$ prédicats de descente | pire cas ; `descend` sort au premier intrus (§1.3) |
| « `sphere.hpp` sain » | candidat, audit prédicat par prédicat à faire (§8) |
| M1–M5 imposées à A2e | deux listes disjointes (§12, Gate E) |
| « l'audit est l'autorité » | spécification + registre des preuves (en-tête) |
| élaguer le range-report par $2R<D$ | **incorrect** : perd les témoins de profondeur (§7) |
| `max_tau_hi` | `max_two_R_upper_hi` : $\tau\neq R$ (§7) |
| $\bar B(c,2r)$ pour toute la descente | premier pas seulement ; enveloppe $\leq(i+1)r_0$ (§1.3) |
| « consommé au fil de l'eau » | tri externe et groupement des niveaux égaux (§10) |
| $\mathrm{rang}=1+\mathrm{niveau}$ | $(4-j)+\mathrm{profondeur}$ (§5) |
| PEL-3 « non bornée ⇒ pas de sphère finie » | probablement faux tel quel (§6) |
| M1–M5 | `PEL-1` à `PEL-4`, pour ne pas heurter M.1 du registre (§6) |
| $\lvert W\rvert\leq39$ à une seconde | 38 au meilleur débit publié (§1.2) |
| $n\binom{\overline{\lvert W\rvert}}{3}$ | sous-estime $\sum_p\binom{\lvert W_p\rvert}{3}$ par convexité (§1.2) |
| « 21 % de 23 M par ordre » | les rangs $k+1$ se répartissent entre ordres (§1.3) |
| largeurs prouvées = arithmétique du produit | valent pour `quantized_u16_input` seul (§8) |
| multiprécision « seule option » | choix robuste, pas exclusivité (§8) |
