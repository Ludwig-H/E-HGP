# MorseHGP3D v3 — proposition d'architecture

> **Statut : proposition, pas une conception arrêtée.** Révision du 8 août 2026
> au soir, après l'audit indépendant [`AUDIT_PROPOSITION.md`](AUDIT_PROPOSITION.md),
> qui a rendu un **NO-GO** sur la recommandation précédente (cascade A1) et
> proposé une meilleure architecture. Cette révision l'accepte, et y ajoute les
> conclusions de l'investigation instrumentée close le même soir.
>
> Les erreurs de la version précédente sont conservées et nommées au §13 : une
> proposition qui efface ses erreurs ne s'audite plus.

## 1. Pourquoi une v3, et ce qui n'est pas en cause

La v2 n'a pas une conception fausse. Elle a un **substitut de force brute posé à
la place de sa conception**, qui n'a jamais été écrite : `DESIGN.md` §7 budgète un
peeling local sensible à la sortie et une descente indexée ; `src/catalogue.cpp`
énumère exhaustivement tous les quadruples du voisinage et
`src/forest.cpp::descend` balaie le nuage entier à chaque bras.

Mesure du 8 août (2 vCPU, Release) :

| $n$ | $K$ | quadruples candidats | temps |
| ---: | ---: | ---: | ---: |
| 200 | 10 | $258\,739\,800 = 200\cdot\binom{199}{3}$ | 26,3 s |
| 500 | 2 | — | > 300 s |

$W_p$ vaut le nuage entier, à tout $K$.

### 1.1 La cause racine, lue dans le code

`src/catalogue.cpp:380` : `radius_bound` rend `+infini` dès que
`vals.size() < s_max`. La relaxation conique n'admet que les points à moins de
$49{,}7^\circ$ de l'axe — $17{,}7\ \%$ de la sphère ; il suffit qu'**un** cône sur
42 contienne moins de $s_{\max}$ points admissibles. Alors `diam_cut = +infini`
(`:290`), `std::isfinite` est faux (`:296`), toutes les paires sont vivantes, et
la triple boucle visite $\binom{\lvert W\rvert}{3}$ triplets.

### 1.2 Même le théorème 4 exact ne sauve pas l'énumération locale

Escalier mesuré à $n=50\,000$, $K=10$, 48 cœurs (débit mesuré
$3{,}2$–$8{,}9\cdot10^{6}$ quadruples/s/cœur) :

| configuration | $\lvert W\rvert$ | quadruples | temps |
| --- | ---: | ---: | ---: |
| code actuel | $n-1$ | $1{,}04\cdot10^{18}$ | 69 ans |
| croissance réparée, $\theta$ du dépôt | 17 578 | $4{,}53\cdot10^{16}$ | 3,0 ans |
| + golden angle corrigé | 5 764 | $1{,}59\cdot10^{15}$ | 38 jours |
| + 2 000 cônes | 252 | $1{,}32\cdot10^{11}$ | 275 s |
| **théorème 4 exact, $\theta=0$** | **175** | $4{,}39\cdot10^{10}$ | **91 s** |
| oracle de voisinage parfait | 130 | $1{,}79\cdot10^{10}$ | 37 s |
| *ce qu'il faudrait* | — | $4{,}08\cdot10^{7}$ | 0,085 s |

avec le modèle fermé
$\lvert W\rvert_{\text{cert}} = 8\,s_{\max}\cos\theta/(\cos\theta-\sin\theta)^4$.

Le budget de 100 ms impose $\binom{\lvert W\rvert}{3}\leq960$, soit
$\lvert W\rvert\leq18$ ; une seconde impose $\lvert W\rvert\leq39$. Or
$\lvert W\rvert\geq130$ est **géométriquement nécessaire** à $K=10$ : le support
le plus lointain d'une sphère critique émise est le 89ᵉ à 165ᵉ voisin, et il faut
le 189ᵉ pour compter les rangs. Rendement mesuré :
$1{,}5$–$2{,}4\cdot10^{-3}$ sphère par quadruple testé, soit **400 à 650
candidats jetés par sphère gardée**.

**L'énumération locale exhaustive des quadruples est condamnée, quels que soient
$\theta$, le nombre de cônes et la qualité de la borne.**

### 1.3 Le second mur, dans la forêt

`build_forest::descend` (`src/forest.cpp:69-89`) balaie **tout le nuage** à
chaque pas, pour chacun des $\leq4$ bras de chaque sphère de rang $k+1$. À
$K=10$, environ $2{,}3\cdot10^{7}$ sphères dont $\approx21\ \%$ de rang $k+1$ :
$\approx4\cdot10^{6}\times4\times5\cdot10^{4}\approx10^{12}$ prédicats
`sphere_side` **par ordre**, soit $\approx20$ s/ordre et $\approx200$ s pour la
tour — le même ordre de grandeur que le plancher du catalogue. S'y ajoute
l'indexation des minima par `std::map<std::vector<i32>,i32>` sur
$2{,}3\cdot10^{7}$ clés.

**Remplacer le générateur sans remplacer la forêt ne gagne rien.**

Et le lemme qui la répare : si $F'\subset F$ et $(c,r)$ est la sphère de
l'événement, le centre du miniball de $F'$ est dans
$\mathrm{conv}(F')\subseteq\bar B(c,r)$ et son rayon est $\leq r$, donc **tout
intrus est dans $\bar B(c,2r)$**. La recherche d'intrus est une **requête de
boule**, jamais un balayage du nuage.

### 1.4 Le mur de mémoire : ne jamais matérialiser le catalogue

Mesuré : $\approx450$ à $510$ sphères critiques distinctes par point en régime
intérieur, **stable de $n=5\,000$ à $200\,000$** — donc
$\approx2{,}3\cdot10^{7}$ sphères à $n=50\,000$. À
`sizeof(CriticalSphere) = 160` octets plus $\approx32$ octets de membres, cela
fait **4,4 Go**.

Conséquences : le catalogue complet ne doit **jamais** être matérialisé, la
sortie est consommée **en flux** par le réducteur, et le budget est
$\approx200$ ns par sphère produite (48 cœurs, 100 ms), atteignable seulement
pour un producteur amorti $O(1)$ par sortie.

### 1.5 Les points peu profonds — énoncé corrigé

La version précédente affirmait : « $\tau(p)=+\infty$ dès que $p$ est de
profondeur de \textsc{Tukey} $\leq K$, donc il existe de grandes sphères
**critiques** tangentes en $p$ ». **La seconde moitié est fausse**, et l'audit a
raison de la refuser (§3.2).

Trois quantités doivent être distinguées :

1. la **boule tangente non contrainte**, dont le centre peut sortir de
   $\mathrm{conv}(X)$ — c'est celle que calcule `radius_bound` de la v2 ;
2. $R(p)$, **supremum** des rayons de boules passant par $p$, **de centre dans
   $\mathrm{conv}(X)$**, de contenu $\leq s_{\max}$ — celle de la germination ;
3. la **sphère critique**, qui exige de surcroît
   $c\in\mathrm{relint}\,\mathrm{conv}(U)$.

Une énorme boule vide posée sur une surface a tous ses points d'appui du même
côté : son centre n'est pas dans leur enveloppe, elle n'est **pas** critique. Ce
qui subsiste — et qui suffit à condamner la v2 — est que **sa borne, non
contrainte, est infinie sur ces points**, donc son voisinage ne peut pas être
dimensionné. Le fait mesuré : $\geq615$ points sur 50 000 à $K=10$ (1,23 %) ne
sont certifiables par aucun $\theta$ ni aucun nombre de cônes, et imposent à eux
seuls $1{,}28\cdot10^{16}$ quadruples.

La convention de profondeur (leave-one-out ou non, points sur le plan frontière)
doit être fixée explicitement : à $K\leq10$, un décalage d'une unité compte.

### 1.6 Mesure sur nuages réels — l'alarme « surface » est levée

Minorant de $\lbrace p:\tau(p)=+\infty\rbrace$, critère **non contraint**, 4 096
directions, donc **majorant** de l'ensemble où la borne à centre convexe échoue :

| nuage | $n$ | $s_{\max}=3$ | $s_{\max}=11$ |
| --- | ---: | ---: | ---: |
| bunny, 10 captations recalées | 50 000 | 1,06 % | **2,21 %** |
| cube uniforme volumétrique | 50 000 | 0,48 % | 1,32 % |
| bunny, 10 captations recalées | 20 000 | 1,80 % | 3,88 % |
| bunny, reconstruction fusionnée | 20 000 | 5,86 % | 10,43 % |
| cube uniforme volumétrique | 20 000 | 0,95 % | 2,83 % |

Nuages : Stanford bunny, dix captations brutes recalées par `bun.conf`
(362 272 points), et sa reconstruction fusionnée `bun_zipper` (35 947 points).

Deux faits : un vrai nuage de surface multi-captation à 50 k donne $2{,}2\ \%$,
soit $1{,}7\times$ l'uniforme volumétrique et non « presque partout » ; et le
nuage **multi-captation est moins peu profond que la reconstruction fusionnée**
($3{,}88\ \%$ contre $10{,}43\ \%$ à $n$ égal) — l'erreur de recalage lui donne
une épaisseur, donc un caractère localement volumétrique. La crainte de la
version précédente est réfutée deux fois, par l'argument de centre convexe et par
la mesure. Ce n'est pas pour autant un théorème dans l'autre sens : ces chiffres
sont un **census**, pas une propriété du domaine.

### 1.7 Deux trouvailles annexes, à ne pas perdre

- **`certified` est un faux positif de diagnostic** : `catalogue.cpp:471`, `:480`,
  `:484` mettent tous `ok = true`, y compris les deux sorties par épuisement, et
  `:500` écrit `certified[i] = 1`. Le reçu annonce 100 % de certification pendant
  que $\lvert W_p\rvert=n-1$. Il faut trois drapeaux distincts :
  `certified_by_bound`, `certified_by_exhaustion_W_eq_X`,
  `certified_by_exhaustion_diameter`.
- **Trou de correction latent, non gardé** : `classify` compte le rang dans $W$
  seulement ; une boule candidate de rayon $r>\rho/2$ verrait ses membres hors de
  $W_\rho$ invisibles, donc un rang sous-compté. Mesuré inactif (0 sur 8 770),
  non gardé. Une ligne : rejeter toute sphère telle que $4\beta(s)>\rho^2$.

## 2. Le contrat, et comment il doit être compté

Hiérarchie de HARTIGAN **exacte** jusqu'à $K=10$ sur $n=50\,000$ points réels,
en moins d'une seconde, cible 100 ms.

La version précédente en tirait « $267$ ns par objet émis ». L'audit refuse ce
raccourci (§10.13), et il a raison : le budget doit être un **ledger de bout en
bout** — candidats rejetés, octets, tri, replis exacts, sink et réduction
comprises — pas un quotient sur la seule sortie. La cible chiffrée reste utile
comme ordre de grandeur ; elle ne remplace pas le ledger.

## 3. Domaine : deux profils, jamais confondus

La spécification principale interprète chaque binary64 d'entrée comme un
**dyadique exact**. Quantifier sur une grille $2^{16}$ est donc un **autre
problème** : la quantification peut changer l'ordre des distances, le rang fermé,
les égalités d'arêtes, les cosphéricités et la topologie des lots simultanés.
Compter zéro collision **ne prouve pas** l'équivalence géométrique.

D'où deux profils nommés et disjoints :

1. **`exact_dyadic_input`** — autorité sur les binary64 originaux, filtres
   d'intervalles dirigés et repli multiprécision ;
2. **`quantized_u16_input`** — autorité sur le nuage entier produit *seulement*,
   avec transformation, origine, échelle, écrêtages, collisions, multiplicités et
   digest source→cible au reçu.

Une sortie du second ne porte **jamais** `public_status=exact` relativement au
premier.

Hypothèses admises, liste close : ensemble fini de points ; position générale au
sens de l'absence de coquille cosphérique, **détectée et déclarée**. Et rien
d'autre — ni surface, ni volume, ni densité, ni forme étoilée, ni origine
capteur, ni image de distance. Ces structures peuvent **proposer** du travail ;
le complément exact doit rendre la même sortie sans elles.

## 4. Le générateur : trois objets, pas deux

L'alternative « A1 cascade contre A2 peeling » de la version précédente était une
fausse dichotomie (audit §3.3). Il faut séparer :

| | rôle | décision |
| --- | --- | --- |
| **A1-source** | source complète d'**ancres diamétrales** | **nécessaire**, et c'est la pièce ouverte |
| **A2e** | peeling **2D** ancré par une **arête** | **recommandé** pour le produit |
| **A2p** | peeling **3D** ancré par un **point** (dual inversif) | **oracle** indépendant ; second candidat |
| Delaunay d'ordre supérieur global | — | no-go produit ; oracle externe borné |

### 4.1 A1-source — la pièce difficile, et elle n'est pas fermée

JUNG et la cascade prouvent qu'un support accepté possède une paire diamétrale et
confinent ses centres **une fois cette paire connue**. Ils ne fournissent **pas**
une énumération sparse et complète des paires utiles. Trois voies seulement :

- balayer $\binom{n}{2}$ — complet, incompatible avec le produit
  (v1 mesure $1{,}92\ \mu$s/paire, soit 2 400 s à 50 k) ;
- un RNG ou un catalogue de paires de rang borné — **réfuté** comme autorité
  complète par `RNG_JUNG_CLIQUES_ET_NIVEAUX_PEUPROFONDS.md`, théorème 1 : pour
  tout $q$ fini il existe un support de rang fermé 11 dont le RNG d'ordre $q$
  n'est pas une clique ;
- le complément **fail-open par self-join du LBVH et center-cover**, voie
  `P15-HOCUDA-P1`, dont la parcimonie et le débit ne sont **pas** qualifiés.

Donc : **complétude conditionnelle à une source complète d'ancres ; parcimonie
non prouvée.** L'observation de 4,5 M paires retenues à une seule taille
n'établit pas $\Theta(n)$ — et plusieurs mesures historiques employaient des
restrictions non certifiées.

### 4.2 A2e — la réduction de dimension, et pourquoi c'est le vrai saut

Ancre diamétrale $e=pq$, $d=q-p$, $D^2=d\cdot d$, $M=(p+q)/2$. Deux vecteurs
entiers indépendants $b_1,b_2$ orthogonaux à $d$, $B=[b_1\ b_2]$, centre
$c=M+Bt$ avec $t\in\mathbb{R}^2$. JUNG donne l'ellipse exacte

$$J_e^{(4)}=\left\lbrace t: t^{\mathsf{T}}\left(B^{\mathsf{T}}B\right)t\leq\frac{D^2}{8}\right\rbrace,$$

et chaque point $x\notin\lbrace p,q\rbrace$ définit la forme **affine**

$$h_x(t)=2(Bt)\cdot(x-M)-\left(\lVert x-M\rVert^2-\frac{D^2}{4}\right),$$

dont l'identité de puissance donne exactement
$h_x(t)=r^2-\lVert x-c\rVert^2$. Intérieur strict, shell et extérieur sont donc
les **signes d'une droite**. Aucune base orthonormale, aucune racine carrée : les
signes sont entiers ou rationnels exacts.

Sur l'ellipse, chaque point est intérieur constant (compté dans $c_e$), extérieur
constant (éliminé), ou **droite active** (comptée dans $m_e$). Si
$c_e>s_{\max}-4$, l'ancre ne porte aucun support quatre utile. Sinon le budget de
profondeur est $\kappa_e=s_{\max}-4-c_e$, et

$$\mathrm{rang}_{\text{ferm\'e}}(p,q,z,w)=4+c_e+\delta_e(t)$$

où $\delta_e(t)$ est le nombre de demi-plans actifs strictement positifs au
sommet $t$. **Le rang est une profondeur d'arrangement 2D.** Il suffit d'énumérer
les sommets de profondeur $\leq\kappa_e$, pas les $\binom{m_e}{2}$
intersections, avec la borne locale du dépôt

$$Z_e\leq m_e\left(\kappa_e+1\right),$$

soit $Z_e\leq 8m_e$ pour $s_{\max}=11$ et $c_e=0$.

**C'est le saut.** La cascade développe des tuples *puis* interroge leur rang ;
A2e **calcule le rang pendant la génération**. C'est ce qui supprime la requête
de boule fermée comme coût dominant (§5).

Les quatre arités se traitent séparément, avec des ellipses, des $c_e$ et des
seuils différents : support un (rayon nul, multiplicités), support deux (centre
$M$, profondeur en $t=0$), support trois (circumcentre = point de la droite
$h_z=0$ minimisant $t^{\mathsf{T}}(B^{\mathsf{T}}B)t$, rang $3+c_e+\delta_e$),
support quatre (sommets shallow). Au rang fermé 11, réfuter tout support trois
demande **neuf** témoins stricts et tout support quatre **huit** : un profiler
qui ne couvre que le second ne peut pas servir de source complète.

Ce que « peeling » doit signifier ici : construire **directement** le préfixe de
profondeur $\leq\kappa_e$, avec un travail lié à $m_e$, $\kappa_e$ et $Z_e$. Ce
n'est *pas* former toutes les intersections puis filtrer, ni perturber
symboliquement les égalités, ni matérialiser l'arrangement. Le brute-force
$m_e^2$ reste un **oracle local borné** — jamais le prototype qu'on chronomètre.

### 4.3 A2p — le dual inversif, et les cinq propriétés à prouver

Variante ancrée par point, nommée par `DESIGN.md` §3 : pour chaque $p$, parcourir
le $\leq(s_{\max}-1)$-level de l'arrangement des $n-1$ **plans duaux**

$$H_u=\left\lbrace x: 2\langle x,\,u-p\rangle=\lVert u-p\rVert^2\right\rbrace$$

dont les coefficients sont **entiers** sur la grille 16 bits. Dictionnaire :
sommet $\leftrightarrow\lvert U\rvert=4$, arête $\leftrightarrow\lvert U\rvert=3$,
face $\leftrightarrow\lvert U\rvert=2$, chaque cellule portant son point le plus
proche de l'origine, donc $(c,r)$.

Cinq propriétés, aucune acquise, et **elles valent pour tout peeling, A2e
compris** :

- **M1 — dictionnaire exact.** Cellule de dimension $j$ du level
  $\leftrightarrow$ sphère critique de support $4-j$ et de rang fermé
  $1+\text{niveau}$. Sans M1, on produit un autre objet.
- **M2 — atteignabilité.** Toute cellule de niveau $\leq s_{\max}-1$ est atteinte
  depuis le niveau 0 par franchissement d'un plan. `DESIGN.md` §3.1 bis documente
  déjà un élagage **réfuté** faute exactement de cette preuve : sans M2, on
  remplace un algorithme trop cher par un algorithme **incomplet**.
- **M3 — sensibilité à la sortie.** Cellules visitées $=O(\text{sortie})$, et non
  $O(\text{sortie}\times\lvert W\rvert)$ : c'est le facteur 400–650 mesuré qu'on
  récupère.
- **M4 — localité.** Le peeling **ne supprime pas** le besoin de $\rho$ : il faut
  toujours charger les plans de $W_\rho$ avec $\rho$ certifié. Il supprime le
  $\binom{\lvert W\rvert}{3}$, pas le $\lvert W\rvert$.
- **M5 — points de faible profondeur.** Dans le dual, ces sphères sont exactement
  les **cellules non bornées** du level. À prouver : « cellule non bornée
  $\leftrightarrow$ pas de sphère critique finie ». Si elle tient, le peeling
  traite ces points **sans cas particulier** — c'est son second argument décisif
  après M3.

L'audit place A2p en **oracle** plutôt qu'en produit : sa complexité globale, sa
duplication entre les $n$ ancres et son mapping GPU ne sont pas compatibles avec
une décision produit, et l'ancrage par arête gagne une dimension. Cette révision
suit l'audit — mais A2p garde une vraie valeur différentielle, ancrage et
structure étant distincts du chemin produit, et M5 lui donne un argument que A2e
doit encore reproduire.

### 4.4 Le contrat du générateur n'est pas le contrat HGP

Émettre « tous les supports minimaux bien centrés de rang fermé utile » est un
composant géométrique, **pas** la frontière scientifique. La sortie normative
(`SPECIFICATION_MORSEHGP3D.md` §§5, 13, 17) exige aussi les facettes et cofaces
utiles, les incidences **actives et silencieuses**, les attachements et
remplacements de la descente, les lots de niveau exactement égal, les
`coverage_delta` et le `coverage_log`, et les applications verticales entre
ordres avec leurs carrés de naturalité.

La phrase « le point dur n'est ni le générateur ni la forêt » est **retirée** :
la source complète, les incidences silencieuses et les verticales sont des points
durs indépendants — et le §1.3 montre que la forêt en est un aussi.

## 5. Où passe le coût, une fois A2e retenu

La requête de boule fermée répétée était le point dur **de la cascade**. Avec
A2e, la profondeur du sommet d'arrangement *est* le nombre d'intérieurs stricts,
et les identifiants de conflit peu profonds se transportent avec le transcript :
le chemin produit ne refait pas une descente LBVH complète par support émis.

La décision terminale vérifie encore exactement : éligibilité de diamètre et
propriétaire, bon centrage et indépendance affine, shell complet ou statut
`RelevantGP`, cohérence profondeur/conflits/rang, niveau rationnel canonique. Une
requête de boule fermée indépendante reste excellente comme **vérificateur
différentiel ou repli d'ambiguïté** — plus comme coût imposé à chaque candidat.

## 6. La coupe $R(z)\geq D/2$ — conservée, corrigée

> **Proposition.** $U$ support minimal bien centré, circumboule $\bar B(c,r)$ de
> contenu $\leq s_{\max}$, $D=\mathrm{diam}(U)$. Alors pour tout $z\in U$ :
> $R(z)\geq r\geq D/2$, où $R(z)$ est le **supremum** des rayons de boules
> passant par $z$, de centre dans $\mathrm{conv}(X)$, de contenu $\leq s_{\max}$.

*Démonstration.* $z\in U\subseteq\partial B$ donc $\bar B$ est une telle boule ;
son centre est dans $\mathrm{relint}\,\mathrm{conv}(U)\subseteq\mathrm{conv}(X)$.
D'où $R(z)\geq r$. Et $U\subseteq\bar B$ donne $D\leq 2r$. $\square$

Trois précisions imposées par l'audit :

1. $R$ est un **supremum**, pas « le plus grand rayon » : avec des boules
   fermées la population saute quand un point atteint le shell, la borne peut ne
   pas être atteinte ;
2. ce $R$, contraint par $\mathrm{conv}(X)$, **n'est pas** la quantité tangente
   non contrainte du §1.5 ;
3. surtout — **le gain n'existe que si le seuil entre dans le range-report**.
   Ajouter `if (2R(z) < D) continue` après avoir balayé la lentille ne réduit pas
   les visites, seulement les candidats aval. Il faut augmenter chaque nœud LBVH
   d'un majorant extérieur `max_tau_hi` et n'élaguer un nœud que si
   `max_tau_hi < D` est certifié ; non fini, sous-normal, débordement ou
   intervalle traversant le seuil restent **fail-open**.

Le $1{,}2\cdot10^{8}$ annoncé précédemment était une hypothèse de mesure, pas une
conséquence du lemme. Il est retiré tant que le range-report indexé et les
distributions p95/p99/max n'existent pas.

## 7. Architecture GPU

**J10 borne un rayon spatial, pas une cardinalité.** Les mesures du dépôt
atteignent un voisinage maximal de **25 026 points** sur `eight_clusters` :
« une arête, sa boule en mémoire partagée, quelques dizaines de points » n'est
donc pas une architecture sûre, et cette phrase de la version précédente est
retirée.

| étage résident | rôle | structure globale évitée |
| --- | --- | --- |
| canonicalisation | `PointId`, domaine, digest, `RelevantGP` | copie ambiguë de l'entrée |
| LBVH | range-report et self-join | matrice paire–point |
| propositions | RNG, image de distance, heuristiques | aucune autorité au proposeur |
| center-cover | source complète d'ancres | tableau des $\binom{n}{2}$ paires |
| cordes | classification constante/active | tous les $W_e$ persistants |
| shallow 2D | niveaux peu profonds | $\sum_e\binom{m_e}{2}$ |
| décision exacte | diamètre, shell, bon centrage, owner | rescans globaux par candidat |
| source HGP | facettes, cofaces, silences, couverture | $\Gamma$ global |
| réduction | lots, attaches, forêts, verticales | mosaïque d'ordre supérieur |

Ancres traitées par classes de charge : warp pour les petites, CTA sous-tuilé
pour les moyennes, file persistante pour les lourdes. Le sink est consommé **au
fil de l'eau** par le réducteur — le §1.4 l'impose : 4,4 Go de catalogue ne
peuvent pas être accumulés.

**Sur la contradiction CPU/GPU** de la version précédente (« GPU dès la première
ligne » puis « pas de CUDA avant la référence CPU ») : les deux pistes sont
**parallèles**. CPU multiprécision indépendant pour l'autorité et les petits
différentiels ; CUDA `proposal_only` très tôt pour falsifier masses, divergence
et mémoire ; aucune promotion scientifique avant parité avec l'oracle.

## 8. Canonicité et égalités

Le propriétaire canonique est la plus petite paire lexicographique parmi les
arêtes de longueur **maximale** du support, sur des `PointId` stables — testé
**avant** le sink. Sur une grille entière l'égalité de deux longueurs d'arête
n'est pas rare : la règle de départage est obligatoire, pas cosmétique.

L'hypothèse de position générale n'autorise pas à ignorer les égalités
rencontrées : une concurrence exacte de $t$ droites est **un lot unique**, pas
$\binom{t}{2}$ sommets artificiels. Shell complet compté avant émission ; une
forme hors `RelevantGP` retire l'autorité ou prend une voie dégénérée
explicitement certifiée. Sortie exigée byte-à-byte identique sous permutation
d'entrée, nombre de fils et ordonnancement GPU.

Rappel de la mesure v2 : $55\ \%$ des sphères sont conservées chez un
propriétaire non canonique même en mono-thread, le filtre annoncé ne contient que
des commentaires, et aucun digest reproductible n'est possible.

## 9. Arithmétique

**Production** — entière, sans allocation, largeur auditée. Majorants prouvés
pour $M=65535$ : $\lvert\mathrm{num}_i\rvert<2^{84{,}96}$,
$\mathrm{den}<2^{68{,}17}$, $\mathrm{num2}<2^{169{,}93}$,
$\mathrm{den}^2<2^{136{,}35}$, comparaison de deux niveaux $<2^{306{,}28}$. Le
`sphere.hpp` de la v2 y est conforme et son `sphere_cmp_beta` est d'accord avec
GMP sur 18 601 paires : composant sain, à reprendre en resserrant ses constantes
d'en-tête, lâches d'un facteur $\approx2^{11}$.

**Oracle** — précision **arbitraire**, représentation *différente* de celle de
production (jamais `exact.hpp`, dont un défaut se compenserait des deux côtés).
Mesuré : c'est la seule option qui décide la grille déclarée, $40/40$ nuages
contre $0/40$ aujourd'hui, UBSan propre.

## 10. La v2 n'est pas une autorité réutilisable

La version précédente affirmait la sémantique de `forest.cpp` « certifiée sur
1 462 nuages et 89 247 cas ». **Retiré.** L'oracle qui a produit ces chiffres
déborde en arithmétique signée avant son garde, saute des nuages en silence,
accepte une campagne vide ou censurée, ne compare ni arités, ni enfants, ni
racines, ni sources, ni nombre canonique de nœuds, et sa campagne tire des
coordonnées dans $[0,120]$ — sur la grille déclarée elle décide **zéro nuage sur
quarante** en annonçant `OK`.

La v2 fournit une **sémantique candidate**, des fixtures et des contre-exemples.
Pas une certification. Et le §1.3 ajoute que sa descente est, en plus, un mur de
performance à part entière.

## 11. Recommandation et portes

> **V3 = source complète fail-open de paires diamétrales + arrangement shallow
> exact par paire + descente par requête de boule + émission canonique en flux
> + réduction HGP résidente.**

Ordre des travaux, aligné sur les portes falsifiables de l'audit :

0. **V3-0 — domaine et objet public.** Choisir `exact_dyadic_input` ou
   `quantized_u16_input` ; définir $K_{\mathrm{eff}}$, `RelevantGP`, profils,
   statuts, source HGP, couverture et verticales.
1. **V3-1 — oracle indépendant et largeurs.** Multiprécision indépendante,
   exhaustif jusqu'à $n\leq14$, campagne fermée
   (`attempted = decided + rejected_domain`, zéro skip, zéro débordement
   silencieux), comparaison structurelle complète, fixtures des trois warnings.
2. **V3-2 — census avant architecture.** Sur `uniform_latin`, `eight_clusters`,
   multiscale, filaments, déséquilibrés **et vrais scans mono/multi-captations** :
   collisions, écrêtages, multiplicités, `RelevantGP` ; $Q$, $V_W$, $a$,
   $C=\sum_e c_e$, $M=\sum_e m_e$, $Z=\sum_e Z_e$ ; p50/p95/p99/max de
   $\lvert W_e\rvert$, $m_e$, $Z_e$ ; rétention de chaque porte ; taille de la
   sortie canonique acceptée ; queues lourdes, replis, mémoire.
   **Décision à deux branches** : sortie énorme $\Rightarrow$ réviser le SLO ;
   sortie sparse mais intermédiaires denses $\Rightarrow$ **architecture no-go**.
3. **V3-3 — source center-cover complète** (§4.1), arités trois et quatre
   séparées, aucune matrice de paires.
4. **V3-4 — shallow CPU exact** (§4.2) avec M1 à M5 démontrées ou explicitement
   ouvertes ; comparé au brute-force local et au catalogue global ; no-go si le
   travail reste en $\sum_e m_e^2$.
5. **V3-5 — descente par requête de boule** (§1.3) : intrus cherchés dans
   $\bar B(c,2r)$, jamais par balayage ; index des minima sans `std::map` sur
   $2{,}3\cdot10^{7}$ clés.
6. **V3-6 — shallow GPU et décision exacte**, parité byte-à-byte, aucun scan
   global par support, sink consommé en flux.
7. **V3-7 — source HGP et réduction** : incidences silencieuses, attaches, lots
   égaux, `coverage_log`, forêt, verticales, naturalité.
8. **V3-8 — publication et déterminisme**, puis **produit sans budget
   configuré** : qualifier d'abord la seconde, puis seulement la cible 100 ms,
   **par famille sanctionnée**.

**GO immédiat** : preuve constructive et prototype CPU exact du peeling 2D ancré
par arête ; census LiDAR ; center-cover.
**NO-GO immédiat** : implémenter la cascade A1 comme architecture produit,
annoncer la forêt v2 certifiée, ouvrir un statut exact ou un SLO.

## 12. Ce qui reste à mesurer

1. le census du §11 V3-2, sur de vrais nuages, y compris multi-captation ;
2. le coût unitaire de la décision terminale exacte en arithmétique entière ;
3. $\sum_e m_e$ et $\sum_e Z_e$ réels — ce sont eux, et non la sortie, qui
   décident si A2e tient ;
4. la validité de M1 à M5.

## 13. Ce que cette révision corrige, nommément

| erreur de la version précédente | correction |
| --- | --- |
| « complétude A1 = théorème » | conditionnelle à une source complète d'ancres (§4.1) |
| $\tau=+\infty \Rightarrow$ grandes sphères critiques | faux : trois quantités distinctes (§1.5) |
| « surface $\Rightarrow$ faible profondeur presque partout » | census, pas théorème ; mesuré à 2,2 % (§1.6) |
| A1 contre A2, dichotomie | trois objets : A1-source, A2e, A2p (§4) |
| gain $9{,}5\cdot10^9\to1{,}2\cdot10^8$ | invalide sans range-report indexé (§6) |
| « J10 rend la tuile GPU sûre » | borne spatiale, pas cardinale ; max 25 026 points (§7) |
| « le point dur n'est ni le générateur ni la forêt » | retiré ; la forêt est un mur à part (§1.3) |
| forêt v2 et O2 « certifiés » | sémantique candidate seulement (§10) |
| $267$ ns par objet | ledger de bout en bout (§2) |
| grille $2^{16}$ comme domaine | deux profils disjoints (§3) |
| 4,5 M paires $\Rightarrow\Theta(n)$ | retiré (§4.1) |
| « si la sélectivité tombe, réénoncer le SLO » | décision à deux branches (§11, V3-2) |
| « GPU dès la première ligne » vs « pas de CUDA avant » | deux pistes parallèles (§7) |
| catalogue matérialisable | 4,4 Go : flux obligatoire (§1.4) |

L'audit [`AUDIT_PROPOSITION.md`](AUDIT_PROPOSITION.md) est l'autorité de cette
révision ; la réduction 2D vient de
`docs/math/RNG_JUNG_CLIQUES_ET_NIVEAUX_PEUPROFONDS.md` ; les murs de la forêt et
de la mémoire viennent de l'investigation instrumentée du 8 août.
