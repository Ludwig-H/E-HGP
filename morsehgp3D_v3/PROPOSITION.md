# MorseHGP3D v3 — proposition d'architecture

> **Statut : proposition, pas une conception arrêtée.** Écrit le 8 août 2026, à la
> demande de Louis, pendant qu'une investigation instrumentée de `morsehgp3D_v2`
> était encore en cours. Aucun code n'existe dans ce dossier. Toute mesure citée
> ci-dessous porte sa provenance ; ce qui est conjecturé est marqué comme tel.
>
> Ce document doit être relu quand les mesures en attente (§8) seront tombées :
> elles peuvent invalider le §5.

## 1. Pourquoi une v3, et ce qui n'est pas en cause

La v2 n'a pas une conception fausse. Elle a un **substitut de force brute posé à
la place de sa conception**, qui n'a jamais été écrite.

`morsehgp3D_v2/DESIGN.md` §7 budgète deux étages dominants :

| étage prévu | travail annoncé | cible |
| --- | --- | --- |
| peeling local par point | $\Theta\left(\sum_{p} \left(\lvert W_p\rvert\log\lvert W_p\rvert+Z_p\right)\right)$ | 20–60 ms |
| descente des bras | $\Theta\left(\text{fusions}\times\lvert U\rvert\times d_T\right)$ | 10–30 ms |

Le premier est **sensible à la sortie** — $Z_p$ compte les sphères réellement
émises en $p$. Or `src/catalogue.cpp` énumère exhaustivement tous les quadruples
du voisinage, et `src/forest.cpp::descend` **balaie le nuage entier à chaque
bras**. Les deux étages qui font le budget sont absents.

Mesure du 8 août (codespace, 2 vCPU, Release) :

| $n$ | $K$ | quadruples candidats | temps |
| ---: | ---: | ---: | ---: |
| 200 | 10 | $258\,739\,800 = 200\cdot\binom{199}{3}$ | 26,3 s |
| 500 | 2 | — | > 300 s |

L'égalité $258\,739\,800 = 200\cdot\binom{199}{3}$ dit tout : $W_p$ vaut le nuage
entier, à tout $K$. Le coût est en $\Theta(n^5)$, conformément à
`WARNING_AUDIT_IMPLEMENTATION_2.md` §5, qui l'annonçait.

### 1.1 La cause racine, lue dans le code

`src/catalogue.cpp:380` : `radius_bound` rend `+infini` dès que
`vals.size() < s_max`. La relaxation conique n'admet que les points à moins de
$49{,}7^\circ$ de l'axe du cône — $17{,}7\ \%$ de la sphère ; il suffit qu'**un**
cône sur 42 contienne moins de $s_{\max}$ points admissibles. Alors
`diam_cut = +infini` (`:290`), `std::isfinite` est faux (`:296`), **toutes** les
paires sont marquées vivantes, et la triple boucle visite
$\binom{\lvert W\rvert}{3}$ triplets. C'est le $\Theta(n^4)$, à l'unité près :
`candidate_quads` $= n\binom{n-1}{3}$ pour tout $K$ mesuré.

### 1.2 Même le théorème 4 exact ne sauve pas l'énumération locale

Mesure directe à $n=50\,000$, $K=10$, du plus petit $\rho$ vérifiant
$2\tau\leq\rho$ (bissection ; le prédicat est monotone) :

| configuration | $\theta$ | $\lvert W\rvert_{\text{cert}}$ mesuré (moy) |
| --- | ---: | ---: |
| dépôt, 42 cônes | 0,5765 | 17 578 |
| golden angle corrigé | 0,5028 | 5 764 |
| 512 cônes | 0,1553 | 355 |
| **borne exacte $\theta=0$ (théorème 4 lui-même)** | 0 | **175** |

Modèle fermé vérifié par la mesure :
$$\lvert W\rvert_{\text{cert}}\;=\;\frac{8\,s_{\max}\cos\theta}{\left(\cos\theta-\sin\theta\right)^4}.$$

Le facteur $8=2^3$ est intrinsèque : le critère borne le **diamètre** $2r$ d'une
boule tangente qui contient déjà $s_{\max}$ points, donc le voisinage certifié
contient au moins huit fois le contenu de la plus grosse boule critique.

Conséquence chiffrée : à $\lvert W\rvert=175$, l'énumération exhaustive donne
$n\binom{175}{3}=4{,}4\cdot10^{10}$ quadruples, soit **environ 92 s sur
48 cœurs**. Le générateur de la v2 est condamné **même dans son meilleur cas
théorique**.

### 1.3 L'obstruction est mathématique : les points peu profonds

$\tau_e(p)=+\infty$ **si et seulement si** le demi-espace ouvert
$\lbrace x:\langle x-p,e\rangle>0\rbrace$ contient moins de $s_{\max}$ points —
la boule tangente tend vers ce demi-espace quand $r\to\infty$. Donc

$$\tau(p)=+\infty \iff p \text{ est de profondeur de \textsc{Tukey} } \leq K,$$

et cela **reste vrai avec $W=X$**. Ce n'est pas un défaut d'implémentation : il
existe alors réellement des sphères critiques de rang $\leq K+1$ tangentes en
$p$, de rayon comparable au diamètre du nuage.

Minorant mesuré (4096 directions, témoin exact), $n=50\,000$ uniforme :

| $K$ | points non certifiables | part | quadruples qu'ils imposent seuls |
| ---: | ---: | ---: | ---: |
| 2 | $\geq 229$ | 0,46 % | $4{,}77\cdot10^{15}$ |
| 4 | $\geq 340$ | 0,68 % | $7{,}08\cdot10^{15}$ |
| 10 | $\geq 615$ | 1,23 % | $\mathbf{1{,}28\cdot10^{16}}$ |

Un seul point de la coque force $\binom{49\,999}{3}=2{,}08\cdot10^{13}$
quadruples, soit $\approx 12$ h$\cdot$cœur.

**C'est le fait qui décide de l'architecture.** Aucun générateur fondé sur un
voisinage $W_p$ ne peut survivre aux points peu profonds. Remplacer le
générateur *est* la seule route — c'est-à-dire faire une v3.

### 1.4 Deux trouvailles annexes, à ne pas perdre

- **`certified` est un faux positif de diagnostic.** `catalogue.cpp:475-486` :
  sortir par épuisement ($W=X$ ou $\rho\geq$ diamètre) met `ok = true`, et
  `:500` écrit `certified[i] = 1`. Le reçu annonce donc 100 % de certification
  pendant que $\lvert W_p\rvert = n-1$. Correctif minimal : deux drapeaux
  distincts, `certified_by_bound` et `certified_by_exhaustion`, et publier la
  moyenne de $\lvert W_p\rvert/n$.
- **Trou de correction latent, non gardé.** Si la boucle sortait par la borne,
  `classify` (`:202-227`) compte le rang **dans $W$ seulement**, en s'arrêtant à
  $\text{cut}=(2r)^2$. Pour une boule candidate de rayon $r>\rho/2$, les membres
  hors de $W_\rho$ sont invisibles : rang sous-compté, **sphère émise à tort**.
  Mesuré inactif (0 sur 8 770 au point de fonctionnement $\theta=0$), mais non
  gardé. Une ligne dans `enumerate_point` : rejeter toute sphère telle que
  $4\,\beta(s)>\rho^2$.

**Ce qui n'est pas en cause**, et se réutilise tel quel : l'arithmétique exacte
entière, la sémantique de la forêt, et l'oracle structurel (§6).

## 2. Le contrat, et le seul chiffre qui en découle

Hiérarchie de HARTIGAN **exacte** jusqu'à $K=10$ sur $n=50\,000$ points réels,
en moins d'une seconde, cible 100 ms.

Cible mesurée du dépôt (`morsehgp3D_v2/DESIGN.md` §7) : $\approx1{,}8\cdot10^{7}$
objets utiles à $n=50\,000$, $K=10$, tous rangs $\leq 11$ confondus.

$$\frac{100\ \text{ms}\times 48\ \text{cœurs}}{1{,}8\cdot10^{7}} \approx 267\ \text{ns par objet émis},$$

tout compris. À une seconde : $2{,}7\ \mu\text{s}$. Toute décision d'architecture
ci-dessous se juge contre ces deux nombres.

Pour référence, à 50 k, la triangulation de DELAUNAY d'ordre 0 mesurée sur le
même nuage compte $334\,979$ tétraèdres
(`docs/validation/phase14_geogram_low_order_g4_16d8308.json`) : la sortie visée
est environ 54 fois cet objet, pour onze ordres.

## 3. Hypothèses de domaine, explicites

- **Position générale pour la filtration de ČECH** : aucune coquille
  cosphérique. Accordée par l'utilisateur le 8 août pour les nuages réels visés.
  Elle reste **détectée et déclarée**, jamais supposée en silence — la v2 a
  montré ce que coûte le silence (une hiérarchie incomplète publiée comme
  autoritaire).
- **Grille entière** $[0,2^{16})^3$. Toutes les largeurs sont bornées
  statiquement à partir de là, et la borne doit être *vérifiée*, pas supposée :
  `WARNING_AUDIT_PUBLICATION_3.md` §2.1 exhibe un triangle entier dont le
  numérateur carré ne tient pas dans un `i128`.
- **L'égalité de deux longueurs d'arête n'est PAS écartée.** Contrairement à la
  cosphéricité, elle est fréquente sur une grille entière. C'est une contrainte
  de conception, pas une hypothèse (§4.2).

## 4. Le générateur : quatre possibilités

### 4.1 Comparaison

| | A1 germination sur l'arête diamétrale | A2 peeling local | A3 Delaunay d'ordre $k$ | A4 reprendre la v1 |
| --- | --- | --- | --- | --- |
| complétude | **théorème** (JUNG + cascade) | conjecturée | classique | héritée |
| **points peu profonds** | **survit** (deux bornes, §4.2) | ancré sur un point : **meurt** | global, sans objet | survit |
| travail mesuré à 50 k | $\approx4{,}4\cdot10^{8}$ candidats | — | — | 110 µs/support |
| écrit ? | oui (v1), portes en binary64 | **non** | non | oui, mais lourd |
| localité mémoire | **bornée explicitement** (J10) | à établir | globale | tuilée |
| propriétaire canonique | **par construction** | à définir | à définir | filtre |
| risque | faible | élevé, durée inconnue | élevé | dette |

### 4.2 A1 — germination sur l'arête diamétrale

Deux énoncés prouvés (`docs/math/OPTIMISATIONS_JUNG_SUPPORTS_3_4.md`) :

- **JUNG.** Un support minimal bien centré a sa circumboule *pour* miniboule,
  donc $r\leq\gamma_m D$ avec $\gamma_3=1/\sqrt{3}$ et $\gamma_4=\sqrt{3/8}$ ;
  avec $D\leq 2r$, le circumrayon est confiné à $\left[D/2,\ \gamma_m D\right]$
  dès que l'arête diamétrale est fixée.
- **La cascade.** L'arête fixée, le lieu des circumcentres compatibles est un
  **disque** de rayon $\sqrt{\gamma_m^2-1/4}\,D$ dans le plan médiateur ; un
  troisième sommet le réduit à un **segment** de demi-longueur
  $\sqrt{\gamma_m^2D^2-r_\triangle^2}$.

Mesuré à $n=50\,000$, $s_{\max}=11$ (§3.4 du même document) :

| filtre au troisième sommet | retenus | part |
| --- | ---: | ---: |
| aucun | $2{,}95\cdot10^{8}$ | 100 % |
| J4′ libre sur $r_\triangle$ | $2{,}66\cdot10^{8}$ | 90,2 % |
| J8, segment $N=16$ | $\mathbf{4{,}70\cdot10^{7}}$ | **15,9 %** |

et le travail des quadruples passe de $3{,}31\cdot10^{9}$ à
$\mathbf{3{,}93\cdot10^{8}}$, soit un facteur 8,4.

**J10** borne la localité : toute la géométrie utile à l'arête $(p,q)$ tient dans
$\bar B\left(M,\ \left(\gamma_m+\sqrt{\gamma_m^2-1/4}\right)D\right)$, de rayon
$0{,}866\,D$ pour $m=3$ et $0{,}966\,D$ pour $m=4$. C'est directement une **tuile
GPU** : une arête par bloc, sa boule en mémoire partagée, rien au-delà.

**Le propriétaire canonique devient un théorème.** La v2 émet chaque sphère
depuis *chacun* de ses points de support puis déduplique — d'où le filtre de
propriétaire vide, les membres non triés et le payload dépendant du nombre de
fils (`WARNING_AUDIT_PUBLICATION_3.md` §4). Germer sur l'arête diamétrale émet
chaque support **une fois**. Réserve obligatoire : quand plusieurs arêtes
atteignent la longueur maximale, le propriétaire est *la plus petite paire
lexicographique parmi elles* — règle totale, à écrire explicitement, et non
optionnelle sur une grille entière.

**Pourquoi A1 survit aux points peu profonds, et pas A2 ni la v2.** C'est
l'argument structurel décisif du §1.3. Le germe est une **arête**, dont les deux
extrémités sont sur la sphère : la restriction certifiée $D\leq 2R(\cdot)$
s'applique donc **aux deux** — et la v1 le fait déjà
(`local_germination.cpp:968-972`). Un point peu profond apparié à un point profond est
rejeté par la borne du profond ; seules survivent les paires peu-profond –
peu-profond, soit $\binom{615}{2}\approx1{,}9\cdot10^{5}$ à 50 k, $K=10$. Un
générateur ancré sur **un** point n'a qu'une borne et n'a pas cette protection —
c'est exactement pourquoi la v2 meurt sur $1{,}23\ \%$ de ses points. Ce que la
v1 mesure confirme le mécanisme : 4,5 M paires retenues à 50 k, soit
$\Theta(n)$.

### 4.2 bis La coupe manquante : $R(z)\geq D/2$ à **tous** les sommets

> **Proposition.** Soit $U$ un support minimal bien centré, de circumboule
> $\bar B(c,r)$ avec $\lvert X\cap\bar B\rvert\leq s_{\max}$, et $D=\mathrm{diam}(U)$.
> Alors pour **tout** $z\in U$ :
> $$R(z)\;\geq\;r\;\geq\;\frac{D}{2},$$
> où $R(z)$ est le plus grand rayon d'une boule passant par $z$, centrée dans
> l'enveloppe convexe, de contenu $\leq s_{\max}$.

*Démonstration.* $z\in U\subseteq\partial B$, donc $\bar B$ **est** une boule
passant par $z$, de rayon $r$, de contenu $\leq s_{\max}$ ; son centre est dans
$\mathrm{relint}\,\mathrm{conv}(U)\subseteq\mathrm{conv}(X)$ puisque $U$ est bien
centré. D'où $R(z)\geq r$. Et $U\subseteq\bar B$ donne
$D\leq\mathrm{diam}(\bar B)=2r$. $\square$

Les trois hypothèses employées — sommets sur la sphère, rang fermé
$\leq s_{\max}$, bon centrage — sont exactement la définition de la cible du
générateur : la proposition ne suppose rien de plus.

**Audit de la proposition, cinq points.**

1. *Sens de l'inégalité.* Rejeter demande $R_{\text{utilisé}}(z)<D/2$ ; la sûreté
   exige donc que $R_{\text{utilisé}}$ **majore** $R$. C'est bien le sens de la
   borne tangente (elle majore le rayon de toute sphère critique de rang
   $\leq s_{\max}$ passant par $z$). Sens correct.
2. *Constante serrée.* Pour un triangle bien centré de plus grand côté $D$
   opposé à $A<90^\circ$, $r=D/(2\sin A)>D/2$, et $r\to D/2$ quand
   $A\to 90^\circ$ : l'infimum est $D/2$, non atteint. On ne peut pas améliorer
   la constante sans hypothèse supplémentaire.
3. *Applicabilité.* La boucle du troisième sommet énumère bien des candidats
   **sommets de $U$**, et la condition $\lvert z-p\rvert\leq D$,
   $\lvert z-q\rvert\leq D$ (plus $\lvert w-z\rvert\leq D$ à l'arité 4) garantit
   $\mathrm{diam}(U)=D$. La proposition s'applique donc avec le bon $D$.
4. *Nouveauté, vérifiée dans le code.* `local_germination.cpp:968-972` applique
   déjà la restriction **aux deux extrémités du germe**
   (`diameter > tangent_bound[first] || diameter > tangent_bound[second]`), et
   `tangent_bound` est rempli **pour tous les points** (`:920-925`, il stocke
   $2R$). La boucle du troisième sommet (`:1007-1035`) ne le teste **jamais**.
   La proposition se réduit donc à *la clause existante, appliquée à un sommet
   auquel elle ne l'était pas* — une disjonction sur un tableau déjà calculé.
   Elle hérite exactement de la certification de la clause existante.
5. *Réserves.* La *force* de la coupe dépend de la finesse du jeu de directions :
   un majorant lâche laisse passer, sans jamais rejeter à tort. Et
   `tangent_bound` est en binary64 « proposal-only » — obligation de la clause
   existante, pas une obligation nouvelle.

**Pourquoi elle est décisive.** Sans elle, une paire germe peu profonde a un $D$
comparable au diamètre du nuage, donc une lentille qui contient presque tout :
$1{,}9\cdot10^{5}$ paires $\times\ 5\cdot10^{4}$ points $\approx9{,}5\cdot10^{9}$
troisièmes sommets examinés. Avec elle, les candidats doivent eux-mêmes vérifier
$2R(z)\geq D$, donc être peu profonds : $\approx1{,}2\cdot10^{8}$. **Ordre de
grandeur attendu, pas encore mesuré** — la mesure demande de reconstruire la v1.

C'est ce qui rend le régime des points peu profonds du §1.3 traitable au lieu
d'être seulement *rare*.

**Ce que JUNG ne donne pas** (§6 du document, à citer tel quel) : il ne borne ni
le rang, ni le nombre de supports par arête ; il ne réduit pas la porte de bon
centrage (28 % des triples, 10 % des quadruples, **constant en $n$**) ; il ne
dispense pas de la classification terminale exacte. La condition
$\lvert P\cap\bar B(c_U,r_U)\rvert\leq s_{\max}$ **reste la seule source de
sensibilité à la sortie**.

**Manques à combler** : les portes sont en binary64 sans intervalle extérieur
complet ni repli exact — l'en-tête de `local_germination.hpp` interdit
explicitement d'en revendiquer la complétude ; et la boucle de germes de la v1
balaie $\binom{n}{2}$ paires (1,92 µs/paire mesuré à 50 k, soit 2 400 s), alors
que les paires retenues sont $\Theta(n)$ et doivent être énumérées par l'index.

### 4.3 A2 — peeling local

**Il hérite de l'obstruction du §1.3.** Étant ancré sur un point, il n'a qu'une
borne de restriction, donc les $1{,}23\ \%$ de points de profondeur de \textsc{Tukey}
$\leq K$ lui imposent le même mur qu'à la v2 — à moins d'un traitement séparé,
qui reste entièrement à concevoir. C'est une objection nouvelle, qui ne figure
dans aucun des trois audits.

L'intention de `DESIGN.md` §7 : $\Theta\left(\lvert W_p\rvert\log\lvert W_p\rvert+Z_p\right)$
par point, soit $\approx4{,}7\cdot10^{7}$ opérations à 50 k — environ **dix fois
mieux que A1**. Mais `WARNING_AUDIT_IMPLEMENTATION_2.md` §5 est net : ce n'est
pas encore un algorithme sensible à la sortie démontré, et avant d'en faire le
chemin produit il faut borner **séparément en $n$ et en $K$** les plans, faces,
cellules et sorties visités, la mémoire, et la **duplication entre ancres**.
Rien n'est écrit.

### 4.4 A3 — Delaunay d'ordre supérieur

Par relèvement, les sphères critiques de rang $\leq k$ sont le $\leq k$-level
d'un arrangement de $n$ hyperplans de $\mathbb{R}^4$, de complexité
$\Theta\left(n^2k^2\right)$ au pire, soit $\approx3\cdot10^{11}$ ici. Le pire cas
n'est pas atteint (sortie mesurée $1{,}8\cdot10^{7}$), mais un algorithme
sensible à la sortie ramène à A2 ; et matérialiser une structure globale viole
l'invariant d'architecture du dépôt.

### 4.5 A4 — repartir de la v1

Elle a le LBVH, les prédicats exacts, le pipeline G4 qualifié. Mais son chemin
exact mesure $\approx110\ \mu\text{s}$ par support terminal (dont $\approx46\
\mu\text{s}$ de normalisation canonique finale) et sa cérémonie est
considérable. À **emprunter par morceaux** — la germination, l'index — pas à
reprendre.

### 4.6 Recommandation

**A1 pour le chemin produit ; A2 gardé comme piste de recherche.**

Et surtout : faire du générateur une **interface** au contrat certifié —

> émettre tous les supports minimaux bien centrés de rang fermé $\leq s_{\max}$,
> chaque rejet étant certifié, chaque support émis une seule fois par son
> propriétaire canonique

— afin qu'A2 puisse remplacer A1 **sans toucher à l'aval**, et que l'oracle teste
l'*interface*, pas l'implémentation. C'est la décision d'architecture la plus
importante de ce document.

## 5. Le point dur, nommé

Ce n'est ni le générateur ni la forêt : c'est la **classification terminale**.

Les portes bon marché sont peu coûteuses en entier : bon centrage (signes de
déterminants), construction de la circumsphère (quelques produits `i128`), de
l'ordre de la centaine de cycles. Le coût réel est la requête
$\lvert P\cap\bar B(c_U,r_U)\rvert\leq s_{\max}$, à sortie anticipée dès
$s_{\max}+1$.

J10 est ce qui la rend tenable : les points utiles à l'arête sont **déjà
chargés**, donc la requête est un balayage linéaire sur quelques dizaines de
points, pas une descente d'arbre. C'est ce qui rend 267 ns concevable.

Le chiffre manquant, et c'est **le** chiffre : le coût unitaire de cette
classification en arithmétique entière v2, contre les $110\ \mu\text{s}$ de la
multiprécision v1. Un facteur 400 sépare la mesure v1 du budget. Tant qu'il
n'est pas mesuré, aucune promesse de 100 ms n'est fondée.

## 6. Répartition CPU / GPU, honnête

Plancher amont mesuré côté v1 à 50 k : canonicalisation 3,1 ms + LBVH CPU
15,4 ms = **18,5 ms**, avant la première paire. Avec $\approx4{,}4\cdot10^{8}$
candidats à quelques dizaines de nanosecondes de portes, on est déjà à
$\approx0{,}3$ s sur 48 cœurs **avant** les requêtes.

- **$\approx1$ s sur 48 cœurs** : contrat réaliste du chemin CPU ;
- **100 ms : contrat GPU**, et il exige le pipeline GPU **de bout en bout**, LBVH
  compris. C'est d'ailleurs ce que dit `DESIGN.md` §7 (« un bloc CUDA traite un
  point »).

**Conséquence de conception : v3 doit être taillée GPU dès la première ligne**,
le CPU servant de référence exacte et non de cible. J10 est ce qui rend ce
découpage possible.

## 7. Ce qui se réutilise, ce qui s'écrit

**Repris de la v2, tel quel ou presque**

- `include/mhgp/exact.hpp` — `i128`, `BigInt<N>`, bornes statiques, aucune
  allocation. **À élargir** selon `WARNING_AUDIT_PUBLICATION_3.md` §2.1 avant
  toute réutilisation.
- `include/mhgp/sphere.hpp` — `sphere1..4`, `sphere_side`, `well_centered3/4`,
  `sphere_cmp_beta`, avec l'audit de largeur refait pour la grille 16 bits.
- La **sémantique** de `src/forest.cpp` : regroupement des lots par égalité
  rationnelle, contraction de l'hypergraphe du lot en une multifusion, censure
  atomique d'un événement à bras non résolu, source exacte par nœud. C'est la
  partie que l'oracle O2 a certifiée sur 1 462 nuages et 89 247 cas.
- `tests/oracle2.cpp` comme porte structurelle, **après** réparation de son
  arithmétique (§2.1 de l'audit 3 : ses `i128` signés sont testés *après* le
  débordement qu'ils prétendent détecter).
- La discipline fail-closed : domaine déclaré, retrait d'autorité, refus de
  publier.

**À écrire à neuf**

- Le générateur germé, avec portes **exactes** (intervalle flottant dirigé puis
  repli entier) et boucle de germes **indexée**.
- La requête de boule fermée à sortie anticipée, sur la tuile J10.
- La descente des bras **indexée** (la v2 balaie le nuage par bras).
- Le reçu **fail-closed** : statut non ambigu, sérialisation canonique de la
  forêt avec niveaux rationnels exacts, reçu de quantification (échelle,
  origine, collisions), codes de retour. Dès le premier jour, pas après.
- Une CI stricte, Release et ASan/UBSan. La v2 n'en a aucune.

## 8. Ce qui reste à mesurer avant d'arrêter cette proposition

1. **Le coût unitaire de la classification terminale exacte en arithmétique
   entière.** C'est le §5. Sans lui, le choix CPU/GPU du §6 n'est qu'une
   estimation.
2. **La largeur réellement nécessaire** pour les niveaux exacts sur la grille
   16 bits (audit 3 §2.1) : elle décide de la forme de `exact.hpp`.
3. **Le débit réel par candidat** des portes de la cascade, une fois exactes.
4. ~~La cause exacte du non-élagage du voisinage v2~~ — **fermée** le 8 août,
   voir §1.1 à §1.3. La borne conique reste utile pour dimensionner la tuile :
   elle vaut $\lvert W\rvert_{\text{cert}} = 8\,s_{\max}\cos\theta/(\cos\theta-\sin\theta)^4$,
   soit 175 en moyenne à $\theta=0$, $n=50\,000$, $K=10$.
5. **Le nombre de paires germes retenues sur un nuage réel**, et la part qui
   provient des points peu profonds : c'est ce qui dimensionne A1.
6. **Le gain réel de la coupe du §4.2 bis**, dont seul l'ordre de grandeur est
   estimé.
7. **La sélectivité sur un nuage-SURFACE**, et non volumétrique (§10). C'est la
   mesure la plus urgente : elle peut invalider le modèle de coût entier.

## 9. Obligations de preuve ouvertes

- **Terminaison et unicité de la descente** (`morsehgp3D_v2/DESIGN.md` §4) :
  aujourd'hui seulement empirique, sur 89 247 cas. C'est le cœur du modèle
  d'appariement ; il faut une preuve.
- **Exactitude des portes de germination** une fois le binary64 remplacé : les
  lemmes géométriques sont exacts, leur *implémentation* ne l'est pas encore.
- **Propriétaire canonique** avec départage des arêtes diamétrales de longueur
  égale : à énoncer et à prouver total.
- **Traitement complet des coquilles cosphériques** : hors modèle par hypothèse,
  mais l'obligation reste ouverte (`morsehgp3D_v2/DESIGN.md` §6.4).

## 10. Le risque qui domine tous les autres : le nuage réel est une surface

Tous les chiffres de ce document — 615 points peu profonds, paires retenues en
$\Theta(n)$, $1{,}8\cdot10^{7}$ objets — sont mesurés sur des nuages **uniformes
volumétriques**. Or le nuage réel de référence du projet est un relevé LiDAR
(`HGP-old/tests/SemanticKITTI`) : une **surface**.

Sur une surface, un demi-espace appuyé sur le plan tangent en $p$ ne contient
presque aucun point. Par le critère du §1.3, $\tau(p)=+\infty$ **presque
partout**, et non sur $1{,}23\ \%$ des points. La restriction $D\leq 2R$ — donc
aussi la coupe du §4.2 bis, qui est la même inégalité — cesserait alors de mordre.
La v1 l'a déjà observé sur `eight_clusters` : $79{,}55\ \%$ de l'univers retenu à
$n=256$, « le vide entre amas est intérieur à l'enveloppe, aucun majorant convexe
de $R(p)$ ne l'exclut ».

Cela ne casse pas la complétude de A1, qui reste un théorème, ni la validité de
la coupe, qui reste prouvée. Cela casserait sa **sélectivité**, donc le budget.

Et cela pose une question antérieure à toute implémentation : **sur un
nuage-surface, à quoi ressemble la sortie elle-même ?** Il existe réellement
d'énormes sphères critiques posées sur la surface par l'extérieur, et elles font
partie de la hiérarchie exacte. Le $1{,}8\cdot10^{7}$ du §2 n'a jamais été mesuré
là.

**Mesure à faire avant d'écrire une ligne de v3** — trois nombres sur un scan
SemanticKITTI décimé à 50 k : combien de points de profondeur de \textsc{Tukey}
$\leq K$, combien de paires germes retenues, combien d'objets en sortie. Si la
surface fait tomber la sélectivité, ce n'est pas l'architecture qu'il faut
changer, c'est le contrat de 100 ms qu'il faut réénoncer.
