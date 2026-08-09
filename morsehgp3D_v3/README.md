# MorseHGP3D v3

État : **M1 (le juge) et M2.1 (un falsificateur borné)**. Il n'y a pas de v3, et
il ne doit pas y en avoir avant que le §2 ait été tranché.

L'autorité mathématique reste `docs/SPECIFICATION_MORSEHGP3D.md` et
`docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md`. Les audits de
[`audits/`](audits/) motivent les corrections ; ils ne certifient rien. Aucun
statut public, aucun SLO n'est ouvert.

---

## 1. Ce qui est établi, et par quelle mesure

**Le générateur de la v2 est condamné**, pas lent. Son voisinage est dimensionné
par une borne *a priori* (relaxation conique du théorème 4) qui vaut $+\infty$
dès qu'un cône est trop pauvre : $\lvert W_p\rvert = n-1$ à tout $K$, mesuré, et
un coût en $\Theta(n^5)$. Même le théorème 4 exact ($\theta=0$) demanderait encore
175 voisins à $n=50\,000$, soit $4{,}4\cdot10^{10}$ quadruples.

**Le rang est une profondeur d'arrangement.** C'est l'invariant qui a survécu à
tous les audits, sous ses deux formes : ancré par arête, dans le plan médiateur,
$\mathrm{rang}=4+c_e+\delta_e(t)$ avec $Z_e\leq m_e(\kappa_e+1)$ ; ancré par
point, dans le dual inversif, $\mathrm{rang}=(4-j)+\mathrm{profondeur}$ sur une
face de dimension $j$. C'est ce qui permet de calculer le rang **pendant** la
génération au lieu de l'interroger après.

**Il n'existe aucune règle d'arrêt valide fondée sur les seules distances.** Un
certificat de localité a été proposé, puis **réfuté par le juge** : un support
inconnu employant un point exclu vérifie précisément $2r\geq d_{M+1}$, donc le
maximum des rayons déjà trouvés ne le borne pas. La complétude d'un générateur
ancré exige soit l'**exhaustivité**, soit un majorant de $R(p)$ — fini
($R\leq\mathrm{diam}(X)$) mais qu'il reste à calculer.

**Mesure honnête de la fenêtre qui aurait suffi** (régime exhaustif,
$s_{\max}=11$, calculée *a posteriori* depuis le vrai $r_{\max}$) :

| $n$ | p50 | p95 | max | sphères/point |
| ---: | ---: | ---: | ---: | ---: |
| 100 | 53 | 77 | 96 | 167,7 |
| 150 | 64 | 97 | 124 | 190,9 |
| 200 | 73 | 125 | 141 | 210,3 |

Elle **croît encore** : rien n'en est extrapolé.

**Un majorant du travail d'un parcours, et rien de plus.** En comptant les
**incidences support–ancre** — candidats affinement indépendants de rang fermé
$\leq s_{\max}$, produits par la cascade exhaustive — contre ce qui est émis
(régime exhaustif, $s_{\max}=11$) :

| $n$ | incidences | bien centrées | émises | **incidences / émise** | candidats / émise |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 60 | 108 960 | 22,2 % | 7 520 | **14,49** | 273 |
| 100 | 232 544 | 23,5 % | 16 767 | **13,87** | 965 |
| 150 | 398 999 | 23,5 % | 28 637 | **13,93** | 2 889 |

Un parcours toucherait $\approx14$ incidences par sphère émise, **constant en
$n$**,
là où la cascade en visite 273, 965 puis 2 889 — **croissant linéairement**. Le
gain attendu est donc $\approx19\times$ à $n=60$, $70\times$ à 100 et
$208\times$ à 150. C'est l'argument chiffré pour construire le constructeur de
strates, et c'est la cible que PEL-2 doit atteindre.

**Le dictionnaire de profondeur est vérifié.** C'est l'énoncé central de
l'architecture, et il ne l'avait jamais été. Le sujet `edge_shallow` calcule le
rang fermé des supports de taille quatre **exclusivement** par

$$\mathrm{rang} = 4 + c_e + \delta_e(t),$$

sans jamais compter les points de la boule : dans le plan médiateur de l'arête
d'ancrage, chaque point devient une forme **affine à coefficients entiers**
($a_x = b_1\cdot X$, $b_x = b_2\cdot X$, $c_x = \lVert X\rVert^2 - D^2$ avec
$X = 2x-p-q$), et la profondeur d'un sommet est un comptage de signes. Tout tient
dans un `i128` sans allocation — les largeurs sont bornées au §fichier.

Le vert du juge exhaustif **est** la vérification — mais seulement si le test
exerce autre chose que le cas trivial, et la première version ne le faisait pas.
`--max-order 3` tire l'ordre **uniformément**, donc la plupart des nuages avaient
$s_{\max}\leq3$, où tout support d'arité 4 accepté force $c_e=0$ et
$\delta_e=0$ : le vert ne prouvait que $\mathrm{rang}=\text{arité}$.

La porte impose désormais un ordre élevé **et** un plancher d'émissions à
profondeur strictement positive. **[mesuré]** 12 nuages, ordres 5–6, grille
déclarée :

```text
rangs emis par profondeur : rang2=500 rang3=756 rang4=965 rang5=1053
                            rang6=983 rang7=522
profondeur>0 : 3619 | constante interieure>0 : 0 | DICTIONNAIRE REFUTE=0
```

Une réfutation est comptée et fait échouer la campagne, plutôt que d'omettre le
support en silence. La colonne « constante intérieure » reste nulle : elle exige
un point **colinéaire** à l'arête d'ancrage et intérieur à la boule diamétrale,
de mesure nulle sur une grille aléatoire — il faudra une fixture dédiée, et
jusque-là cette branche n'est **pas** exercée.

**Les quatre arités**, désormais, et donc **tout le catalogue** :

| arité | point canonique dans le plan médiateur | rang |
| ---: | --- | --- |
| 1 | sans objet, sphère de rayon nul en $p$ | $1$ |
| 2 | $s=0$, la boule diamétrale ; $\delta_e$ compte les $c<0$ | $2+c_e+\delta_e$ |
| 3 | $s = c\,\mathrm{adj}(G)\,n / Q$, $Q=n^{\mathsf{T}}\mathrm{adj}(G)\,n>0$ | $3+c_e+\delta_e$ |
| 4 | intersection de deux droites actives | $4+c_e+\delta_e$ |

Le chemin exhaustif a entièrement disparu du sujet `edge_shallow` : **aucun rang
n'y est obtenu en comptant une boule.**

Deux obstacles de largeur ont été réglés en chemin, sans quitter les entiers
natifs. La base orthogonale naturelle $b_2=d\times b_1$ porte un facteur
$\lVert d\rVert$ de trop et faisait monter $Q$ à $2^{136{,}4}$ ; on prend donc
$b_1=d\times e_1$, $b_2=d\times e_2$ avec $e_1,e_2$ les deux axes **autres** que
la composante dominante de $d$ — indépendants car
$d\cdot(e_1\times e_2)=\pm d_{e_3}\neq0$, tous deux de taille $\lVert d\rVert$,
au prix d'une matrice de GRAM non diagonale. $Q$ retombe alors sous $2^{104}$,
et seul le test de profondeur de l'arité 3, à $2^{140{,}4}$, demande 256 bits —
fournis par `mul128`, sans allocation.

**Fixtures permanentes** (23 tests, dont 4 hérités de la v2) : la non-régression du faux certificat — sous
une fenêtre supposée trop étroite, le générateur doit se **déclarer incomplet**,
sans quoi un certificat erroné aurait été réintroduit ; le refus des campagnes
négatives vacues ; les bornes sémantiques du CLI, `--max-order 2147483647`
débordant dans `maximum_order + 1` ; et une **cocyclicité portée uniquement par
des triangles non bien centrés**, qui ne met pas le domaine hors `RelevantGP` —
le prototype la comptait à tort comme dégénérescence et censurait le nuage.

---

## 2. La voie la plus pertinente pour la v3

> **Construire le sous-complexe shallow stratifié, et y lire le rang comme une
> profondeur.** Tout le reste en découle, et rien d'autre n'a survécu aux audits.

### Ce que cela veut dire précisément

L'objet **n'est pas** $V_k(p)$ vu comme un sous-ensemble de $\mathbb{R}^3$ :
pris comme ensemble, il efface les hyperplans intérieurs séparant deux cellules
toutes deux de profondeur $\leq k$. Contre-exemple minimal : $X=\lbrace p,u\rbrace$
et $k=1$ donnent $V_1(p)=\mathbb{R}^3$, dont la frontière n'a aucune 2-face —
alors que le plan médiateur $H_u$ existe et que son milieu porte la sphère
critique de support $\lbrace p,u\rbrace$.

L'objet est le **sous-complexe stratifié** : les faces de l'**arrangement** dont
la profondeur est $\leq k$, chacune avec sa dimension, et les quatre arités
traitées **séparément** (une preuve d'arité quatre ne se propage jamais à
l'arité trois).

### Les deux ancrages, et ce qui les sépare

| | ancré par **arête** (A2e) | ancré par **point** (A2p) |
| --- | --- | --- |
| dimension de l'arrangement | 2 | 3 |
| complétude de l'ancrage | **conditionnelle** à une source complète de paires diamétrales | **par construction** |
| borne de sortie | $Z_e\leq m_e(\kappa_e+1)$, classique | $O(m_p K^2)$ par ancre ; sans certificat local $m_p=n-1$ |
| coût du prédicat exact | plus faible | plus élevé |

A2e est le cœur algorithmique ; sa complétude est otage de **A1-source**, que le
RNG d'ordre borné ne peut pas fournir (théorème négatif du dépôt). A2p n'a pas ce
problème et pourrait fournir ces ancres — c'est l'hypothèse **A2pe** — mais elle
n'est pas démontrée.

### Ce qui tranche, et rien d'autre

| obligation | ce qu'elle décide |
| --- | --- |
| **PEL-1** | les 2-faces de l'arrangement **contiennent-elles** toutes les arêtes utiles ? L'inclusion suffit — des plans superflus sont permis. Si oui, A1-source disparaît. |
| **PEL-2** | le parcours est-il en $O(\text{entrée} + \text{sortie})$, et non $O(\text{sortie}\times m)$ ? Le terme d'entrée est obligatoire. |
| **PEL-3** | traitement exact des strates non bornées. L'énoncé « non bornée $\Rightarrow$ pas de sphère finie » est **réfuté** par la fixture à deux points ; l'obligation est le traitement, pas l'énoncé. |
| **PEL-4** | que coûte le prédicat exact en 3D contre 2D ? C'est l'arbitrage A2pe / A2e. |

Le prochain artefact décisif est donc **un constructeur exact du sous-complexe
stratifié, comparé exhaustivement à petit $n$** — pas un pipeline, pas de CUDA,
pas de réducteur. Écrire un pipeline avant de savoir si le parcours est sensible
à la sortie, ce serait refaire exactement l'erreur de la v2 : construire un
substitut, puis mesurer.

### Ce que la v3 ne fera pas

Aucune mosaïque de Delaunay d'ordre supérieur, aucun $\Gamma$ global, aucune
matrice paire–point, aucun catalogue géométrique matérialisé. En revanche le tri
global exact et le groupement des niveaux égaux sont **inévitables** : des ancres
indépendantes n'émettent pas en ordre monotone, et le réducteur exige un lot
atomique par niveau rationnel.

Le détail, les budgets et le journal des affirmations retirées sont dans
[`PROPOSITION.md`](PROPOSITION.md) ; le plan est à son §13.

---

## 2 bis. La question ouverte : 50 000 points, $K=10$, une seconde

**Question posée aux audits.** Le contrat est-il atteignable, et sous quelles
conditions ? Je ne peux pas encore répondre, et c'est un progrès : la question
est enfin bien posée.

### Ce qui est acquis

L'énoncé central n'est plus une conjecture : **le rang est une profondeur**,
vérifié contre la vérité exhaustive sur la grille déclarée, en arithmétique
entière tenant dans un `i128`. C'est la brique sur laquelle tout repose.
$Z_e\leq m_e(\kappa_e+1)$ est une **borne classique**, pas une hypothèse.

### Ce qui manque, et ce n'est pas un détail

Le prototype forme encore **toutes les paires de droites**, $O(m_e^2)$ par arête.
Il vérifie le dictionnaire ; il ne réalise pas le parcours. Or c'est le parcours
qui décide du contrat, et il n'existe pas.

### Les trois inconnues qui trancheraient

| inconnue | pourquoi elle décide | statut |
| --- | --- | --- |
| $\sum_e m_e$ et $\sum_e Z_e$ sur un vrai nuage à l'échelle | c'est le travail réel du parcours | **non mesuré** |
| coût du prédicat de profondeur par candidat | convertit le travail en secondes | non mesuré |
| volume **aval** : incidences silencieuses, tri global, lots, verticales | jamais chiffré, et il peut dominer | non mesuré |

À quoi s'ajoutent deux verrous indépendants du parcours : **A1-source**, dont
aucune version complète et sparse n'est démontrée — le RNG d'ordre borné en est
exclu par théorème — et le **tri global exact** par niveau rationnel, inévitable
parce que des ancres indépendantes n'émettent pas en ordre monotone.

### Mon estimation, donnée comme telle

Une seconde sur 48 cœurs me paraît **plausible mais non acquise** ; 100 ms
exigerait le GPU de bout en bout. Ce n'est pas une mesure, et aucune décision ne
doit s'y appuyer.

### Un obstacle concret trouvé en route, pour les arités 2 et 3

Le dictionnaire n'est vérifié qu'à l'**arité 4**, et il ne s'y propage pas
automatiquement (les arités ont leurs propres régions et seuils). En dérivant
l'arité 3, un obstacle précis apparaît. Le circumcentre du triangle est le point
de la droite $h_z=0$ situé dans le plan du triangle ; comme
$b_1\cdot b_2 = b_1\cdot(d\times b_1) = 0$, la matrice de \textsc{Gram} est
**diagonale**, ce qui donne une direction entière
$v=(a\lVert b_2\rVert^2,\ b\lVert b_1\rVert^2)$ et le paramètre $c/Q$ avec
$Q=a^2\lVert b_2\rVert^2+b^2\lVert b_1\rVert^2$.

Mais $Q<2^{136{,}4}$ sur la grille déclarée : **l'arité 3 ne tient pas dans un
`i128`**, là où l'arité 4 y tient ($<2^{123{,}6}$). Il faudra soit un grand
entier borné, soit une base $b_2$ mieux échelonnée. C'est un fait de largeur, pas
une difficulté de principe — et c'est exactement le genre de chose qu'il vaut
mieux savoir avant d'écrire le code.

---

## 2 ter. Question mathématique ouverte : construire le préfixe shallow

**Adressée aux audits.** Le dictionnaire est acquis ; le **parcours** ne l'est
pas, et c'est lui qui décide du contrat. Voici la question exactement, avec sa
réduction.

### La réduction au dual

Pour une arête d'ancrage, chaque point donne une droite $a s_1 + b s_2 = c$ à
coefficients entiers, et « strictement intérieur » s'écrit
$(s_1,s_2,-1)\cdot(a,b,c) > 0$. En envoyant la droite sur le point dual
$(a,b,c)\in\mathbb{Z}^3$ :

$$\delta_e(s) \;=\; \#\lbrace\, \text{points duaux dans le demi-espace ouvert de normale } (s_1,s_2,-1)\,\rbrace,$$

et un **sommet** de l'arrangement est un plan par l'origine contenant exactement
deux points duaux. Chercher le préfixe $\delta_e\leq\kappa$ est donc un problème
de **$k$-ensembles** dans $\mathbb{R}^3$, avec $\kappa\leq s_{\max}-4\leq 7$.

Le cas $\kappa=0$ est clair : ce sont les arêtes de l'enveloppe convexe conique
des points duaux, en $O(m\log m)$.

### Q1 — l'épluchage en couches est-il exact ?

Est-il vrai que les sommets de profondeur $\leq\kappa$ correspondent exactement
aux arêtes des $\kappa+1$ premières **couches convexes** du nuage dual ? Je
soupçonne que **non** — couches convexes et $k$-niveaux ne coïncident pas en
général — mais je ne veux pas construire dessus sans le savoir. Si c'est faux, un
contre-exemple minimal serait précieux ; si c'est vrai sous une hypothèse
supplémentaire, laquelle ?

### Q2 — quel algorithme de $k$-niveau, à $\kappa$ petit et en exact ?

À $\kappa\leq 7$ et $m$ de l'ordre de la centaine, quel constructeur atteint
$O(m\log m + m\kappa)$ **sans quitter l'arithmétique entière** ? Le tri des
croisements le long d'une droite demande de comparer des rationnels de
$\approx2^{105}$, donc des produits de $\approx2^{210}$ : un `BigInt<4>` suffit,
mais y a-t-il une formulation qui reste dans l'`i128` ?

### Q3 — un critère de rejet d'une droite, avant tout tri ?

Existe-t-il un test **exact et en $O(1)$** disant qu'une droite ne porte aucun
sommet de profondeur $\leq\kappa$, donc qu'on peut l'écarter avant de trier ses
croisements ? Dans le dual, cela revient à écarter un point dual dont aucun plan
support à $\leq\kappa$ points au-dessus ne passe par lui.

### Ce que coûte le prototype, et ce que le $k$-niveau économiserait

**Le balayage par droite est implémenté.** Le long d'une droite la profondeur est
une fonction en escalier qui ne varie que de $\pm1$ à chaque croisement : un tri
puis un balayage donnent tous ses sommets avec leur profondeur, au lieu de
recompter $O(m_e)$ droites par sommet. Le coût par arête passe de $O(m_e^3)$ à
$O(m_e^2\log m_e)$, et le juge est resté vert avec des compteurs **identiques**.

**[mesuré]** $s_{\max}=11$, un nuage par taille :

| $n$ | $m_e$ moyen | sommets examinés | dont peu profonds | part | tests / $m_e^2$ | temps |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 40 | 38 | 548 340 | 65 760 | 12 % | 1,75 | 0,28 s |
| 80 | 78 | 9 489 480 | 203 904 | **2,1 %** | 1,48 | 6,1 s |
| 160 | 158 | 157 766 160 | 521 034 | **0,33 %** | 1,29 | 165 s |

Deux lectures. Les tests de profondeur sont bien en $\Theta(m_e^2)$ et non
$\Theta(m_e^3)$ : le balayage tient sa promesse. Mais **la part des sommets
réellement peu profonds s'effondre en $\approx 1/m_e$** — à $n=160$ on énumère
300 fois plus de sommets qu'on n'en garde. C'est exactement ce que Q1–Q3
économiseraient, et cela chiffre l'enjeu : le balayage gagne un facteur $m_e$, un
vrai $k$-niveau en gagnerait un second.

---

## 3. M1 — le juge

Indépendant du chemin jugé sur les trois couches qui comptent :

| couche | choix | pourquoi |
| --- | --- | --- |
| arithmétique | signe-magnitude, chiffres de 32 bits, précision arbitraire | représentation *différente* du complément à deux de largeur fixe de la production |
| géométrie | élimination de **Gauss** | jamais les formules de Cramer du chemin jugé |
| structure | forêt reconstruite **depuis $\Gamma_k$** | tous les $k$- et $(k+1)$-sous-ensembles, jamais le catalogue jugé |

Ni division entière ni PGCD : les rationnels ne sont pas normalisés, la division
est une multiplication croisée et la comparaison un produit croisé — la partie
risquée d'un grand entier n'existe pas ici. Validé contre `__int128` **et** GMP ;
sans témoin large, le selftest **échoue** au lieu de rendre `OK`.

**Ce qu'il compare** : cardinal, doublons de support, rang, membres, tranche
triée, **niveau et centre rationnels exacts** ; par ordre, genre, arité, racines,
nombre canonique de nœuds, généalogie, les deux représentations d'adjacence
confrontées l'une à l'autre, tous les compteurs publics — et la **participation
effective** de la sphère source d'une multifusion à son lot.

**Fermeture** : `attempted = decided + rejected_domain`, planchers strictement
positifs, arguments absurdes refusés (code 2), censure inattendue = échec, garde
de domaine symétrique, lecture **hostile** et **atomique** du sujet.

**Campagne négative, fail-closed** : six fautes injectées sur une **copie d'un
sujet déjà vert**, avec une comptabilité distincte de la campagne positive — un
plancher ou un rejet de domaine ne peut donc pas tenir lieu de preuve. Chaque
faute doit être **appliquée exactement une fois** et déclencher **exactement une
fois son garde**, sans aucun diagnostic étranger : membres non triés, numérateur
tourné (même norme, donc même niveau), sentinelle invalide, `n_children` nul,
racine supprimée, et source de fusion étrangère **de même rang et de même niveau
exact** — sans cette égalité, c'est le garde de niveau qui rougirait et le garde
de contribution ne serait pas exercé.

Résultat du 8 août, grille déclarée $[0,65535]$ : `attempted=40 decided=40
rejected_domain=0 | spheres=1850 forets=82 noeuds=1909 | largeur max=158 bits`.
Reçus dans [`receipts/`](receipts/).

Deux faits produits par ce juge : il a trouvé que les tranches `I ∪ U` n'étaient
pas triées alors que le contrat l'exige (corrigé en v2), et les niveaux exacts
atteignent **158 bits** sur cette grille — donc au-delà de `__int128`, ce qui est
la raison pour laquelle la porte précédente y décidait *zéro nuage sur quarante*
en annonçant `OK`.

**Ouvert** : différentiel `Rational` contre `mpq_class`, compteurs de
vérifications réellement exécutées, provenance complète du reçu (digests des
nuages — la graine seule n'est pas un format portable), et le profil
`exact_dyadic_input`.

---

## 4. M2.1 — falsificateur borné, pas prototype

Générateur ancré par point qui énumère tous les supports de taille $\leq4$ dans
sa fenêtre. **C'est la cascade locale que le §1 condamne** : il sert de sujet
différentiel et de mesure du travail payé, jamais de voie produit, et il ne
construit ni arrangement, ni complexe stratifié, ni peeling.

Deux régimes nommés, jamais confondus : `exhaustive` — la seule complétude
disponible — et `assumed_window` — hypothèse **déclarée**, jugée séparément et
non qualifiante.

---

## 5. Construire et exécuter

```sh
cmake -S morsehgp3D_v3 -B build/v3 -DCMAKE_BUILD_TYPE=Release
cmake --build build/v3 -j
cd build/v3 && ctest --output-on-failure     # 14 tests

./mhgp3v_arith_selftest 20000                        # __int128 et GMP
./mhgp3v_oracle --clouds 40 --seed 4242 --min-points 8 --max-points 11 \
                --max-order 3 --min-decided 30 --min-nodes 500 \
                --receipt receipts/oracle_campaign.json
./mhgp3v_oracle --subject anchored --regime exhaustive --clouds 8 --seed 90210 \
                --min-points 9 --max-points 12 --max-order 3 \
                --min-decided 6 --min-nodes 60
./mhgp3v_oracle --subject edge_shallow --clouds 20 --seed 4242 --min-points 8 \
                --max-points 12 --max-order 3 --min-decided 15 --min-nodes 200
# Le probe hostile exige la fixture de meme niveau : sur un nuage generique il
# n'existe souvent aucune source alternative de niveau exactement egal, et le
# run echouerait faute d'injection applicable, pas parce qu'un garde a rougi.
./mhgp3v_oracle --inject merge_source_foreign --fixture foreign_source_same_level \
                --subject v2 --clouds 1 --seed 4242 --min-points 4 --max-points 4 \
                --max-order 1 --min-decided 1 --min-nodes 1
```

GMP n'est pas une dépendance de l'oracle : il n'intervient que comme second
témoin de la validation arithmétique.

`census_tukey_shallow.py` produit un reçu complet (provenance, digests, jeu de
directions, convention de demi-espace, identité de campagne). Il mesure un
minorant de l'ensemble où la borne tangente **non contrainte** de la v2 vaut
$+\infty$, et **rien d'autre** : l'ensemble où la borne à centre convexe échoue
est vide, puisque $R\leq\mathrm{diam}(X)$. Nuages : Stanford bunny, reconstruction
fusionnée et **dix captations brutes recalées** — le cas multi-captation que la
proposition doit traiter. Les données ne sont pas versionnées ; le reçu porte
leur origine et leur digest.
