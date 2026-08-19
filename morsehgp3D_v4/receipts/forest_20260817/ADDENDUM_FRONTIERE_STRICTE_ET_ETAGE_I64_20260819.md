# Addendum — la puissance de face était déjà dans le moteur, la frontière stricte n'était pas gardée, et le banc mesurait la mauvaise chose

Date : 19 août 2026 UTC. Exécute le contre-audit `7420355` (« le premier
filtre équatorial existe déjà dans le moteur ») et l'addendum `6a2bc96`
(« deux filtres q4 purement i64 avant même `q3_power` »).

Trois choses sortent de cette session, et la troisième est une
**rétractation méthodologique** sur le reçu de la veille.

---

## 1. La primitive n'était pas à écrire : c'est `q3_power`

Le contre-audit a raison, et je l'ai redéroulé avant de le coder plutôt
que de le recopier.

Soit le tétraèdre $abxy$, de circumcentre $o$. Soit $F$ la face $abx$, de
circumcentre en plan $o_F$, de rayon $R_F$, de normale unitaire $n$. On
a $o = o_F + t n$ et $y = \pi_F(y) + h n$ avec $h \neq 0$. Le reçu de la
veille établit

$$2 t h = \left\Vert y - o_F \right\Vert^2 - R_F^2 = \mathrm{Pow}_F(y).$$

Ce qui restait implicite, et que le contre-audit rend explicite : en
prenant $o_F$ pour origine de l'axe normal, les trois sommets $a, b, x$
ont une coordonnée normale nulle, $y$ vaut $h$, et $o$ vaut $t$. Comme
$o = \lambda_a a + \lambda_b b + \lambda_x x + \lambda_y y$ avec
$\sum_i \lambda_i = 1$, la projection sur $n$ donne $t = \lambda_y h$,
donc

$$\mathrm{Pow}_F(y) = 2 t h = 2 \lambda_y h^2 .$$

Comme $h^2 > 0$, le signe de $\mathrm{Pow}_{abx}(y)$ **est** celui de
$\lambda_y$. Ce n'est donc pas « une condition nécessaire parmi
d'autres » : c'est **exactement l'un des quatre signes barycentriques**
que `q4_center_strictly_inside` vérifie déjà.

Et cette puissance est déjà une primitive du moteur. Avec $d = b-a$,
$u = x-a$, $D = \left\Vert d \right\Vert^2$,
$E = \left\Vert u \right\Vert^2$, $F_{du} = d \cdot u$, la forme
`q3_form(a,b,x)` porte $G = DE - F_{du}^2$ et
$W = E(D-F_{du}) d + D(E-F_{du}) u$, et

$$\texttt{q3\_power}(f, z) = G \left\Vert z-a \right\Vert^2 - (z-a) \cdot W = G \cdot \mathrm{Pow}_{abx}(z),$$

puisque $W = 2G (o_F - a)$ — c'est la définition même des coefficients
$\alpha = E(D-F_{du})/(2G)$ et $\beta = D(E-F_{du})/(2G)$. Avec $G > 0$
garanti par l'acuité stricte du seed, le signe est identique.

**Conséquence pratique.** La forme `f3s` du seed est déjà construite une
fois par seed dans la boucle des complétions : le préfiltre coûte
désormais **un appel à une primitive existante**, sans coefficient
supplémentaire à amortir, et non plus la formule en six longueurs que
j'avais écrite la veille. Cette formule (`equatorial_power4`) est
**conservée**, mais rétrogradée à son seul rôle légitime : un
**oracle croisé** dans `mhgp4_q4_oracle`, écrit dans une représentation
volontairement différente de celle de la production — exactement la
discipline du selftest arithmétique.

## 2. L'étage zéro purement i64 de `6a2bc96`

Les deux inégalités de l'addendum sont vérifiées et branchées **avant**
la puissance de face.

**Filtre du sommet `y`.** Si $c$ est strictement intérieur, il existe des
barycentriques $\lambda_i > 0$ avec $\sum_i \lambda_i u_i = 0$ où
$u_i = p_i - c$ ; le produit scalaire avec $u_i$ interdit que tous les
$u_i \cdot u_j$ soient positifs ou nuls, donc chaque sommet possède une
arête incidente avec $d_{ij}^2 = 2R^2 - 2 u_i \cdot u_j > 2R^2 \geq D^2/2$.
Pour $a, b$ l'arête $ab$ le donne, pour $x$ l'acuité stricte du seed
($l_{ax} + l_{bx} > D^2$) le donne ; **seul `y` reste à vérifier** :

$$2 \max(l_{ay}, l_{by}, l_{xy}) > D^2 .$$

**Filtre du couple `(x,y)`.** Avec $w = u_x + u_y$, la non-antipodalité
de $x$ et $y$ (sinon $c$ serait le milieu de l'arête $xy$, donc au bord)
donne $w \cdot u_x = w \cdot u_y = 2R^2 - d_{xy}^2/2 > 0$ ; comme
$\sum_i \lambda_i (w \cdot u_i) = 0$, l'un de $a, b$ vérifie
$w \cdot u_z < 0$, soit $d_{xz}^2 + d_{yz}^2 > 4R^2 \geq D^2$ :

$$\max(l_{ax}+l_{ay}, \; l_{bx}+l_{by}) > D^2 .$$

Deux `max`, deux additions, une comparaison, tout en `i64` — sous u16,
$2l < 2^{35}$.

**Rendement à n=8000**, famille `uniform`, `smax=11`, quatre fils :

```text
q4_entonnoir paires=173001161 centre=71014748
q4_etage_i64 sommet_y=10840388 couple_xy=2512872
q4_puissance_equatoriale soumises=74146499 rejets=43921721 faux_rejets=0
```

Sur les **87 499 759** paires qui entrent dans la cascade, l'étage i64
en retire **13 353 260** — **15,3 %** — avant qu'aucun `i128` ne soit
touché, et la puissance de face en retire 43 921 721 de plus. Le total,
**57 274 981**, est **exactement** celui qu'obtenait la formule en six
longueurs seule : **80,65 %** des 71 014 748 rejets du centre. L'étage
i64 ne change donc pas ce qui est rejeté ; il change **où** le rejet est
prononcé.

## 3. Ce que le compteur de faux rejets ne pouvait pas voir

C'est la faute que cette session corrige, et elle était structurelle.

Le contrat de la garde a **deux moitiés** :

- une garde **trop agressive** tue des paires que Cramer garde — le
  compteur `faux_rejets` la voit ;
- une garde **trop permissive** ne tue rien à tort : elle **omet** de
  tuer. Aucun compteur de faux rejets ne peut la voir, jamais.

Or le mutant que le contre-audit exige en premier —
`seed-face-power-nonstrict`, « $\geq 0$ accepté à tort » — est de la
seconde espèce. La frontière $\mathrm{Pow} = 0$ signifie
$\lambda_y = 0$, c'est-à-dire **le centre dans le plan de la face**,
donc pas strictement intérieur : le contrat strict doit la rejeter. Le
mutant la laisse passer, `q4_form` la rejette de toute façon en aval,
le flux ne bouge pas d'un bit, et la porte restait **verte**. Elle a
d'ailleurs été verte pendant plusieurs itérations de cette session, et
c'est le code de sortie 3 (« porte inefficace : mutant non discriminé »)
qui l'a dit — pas moi.

**Second couple de compteurs.** `q4_eq_boundary` compte les paires de
puissance nulle, `q4_eq_missed` celles que rien n'a rejetées. La
première est un **plancher** (sans frontière, la moitié du contrat n'est
pas exercée), la seconde doit rester nulle.

**Il a fallu un quatrième nuage.** Sur les trois emprises par défaut, la
frontière n'apparaît que **17 fois** en tout (8, 2, 7) : $\mathrm{Pow}=0$
est de codimension un sur des entiers à cinq chiffres, donc pratiquement
jamais atteint. Le quatrième nuage est **200 points dans $14^3 = 2744$
sites** — l'emprise serrée rend les quadruples cosphériques fréquents, ce
qui est précisément la raison pour laquelle l'oracle q4 travaille sur de
petits nuages. Il apporte **1 507** frontières à lui seul.

**Un mode d'exécution a dû être ajouté pour que la frontière soit
observable.** Dans la cascade de production, une paire tuée par l'étage
i64 ne voit jamais `q3_power` ; sa frontière serait invisible. Le mode
*instrumenté* déroule la cascade en entier, sans court-circuit, et c'est
le seul qui renseigne les deux nouveaux compteurs.

## 4. Quatre mutants, un par garantie dégradée

```text
sain                             RC=0  violations=0 rejets=1786290
                                       faux_rejets=0 frontieres=1524
                                       frontieres_manquees=0

seed-face-power-nonstrict        RC=4  frontieres_manquees=1524
seed-face-power-sign             RC=4  faux_rejets=657986
seed-i64-vertex-y-drop-factor    RC=4  faux_rejets=657986  rejets=0
seed-i64-pair-xy-min             RC=4  faux_rejets=379705  rejets=275944
```

Les deux mutants de l'étage i64 sont des **glissements crédibles**, pas
des sabotages : `drop-factor` oublie le facteur 2 — et comme la lentille
garantit déjà $l \leq D^2$, la garde devient universellement vraie et
**tout** est rejeté (`rejets=0` sur la puissance : plus rien ne lui
parvient) ; `pair-xy-min` prend le minimum au lieu du maximum, alors que
l'inégalité n'est prouvée que pour **l'un** des deux sommets $a, b$.

Le fait que `sign` et `drop-factor` donnent **le même** nombre de faux
rejets (657 986) n'est pas une coïncidence : les deux rejettent
l'intégralité des paires que Cramer garde, et 657 986 est donc le
cardinal exact des complétions bien centrées des quatre nuages.

Enfin, la porte saine rejette les 1 524 frontières **et** ne compte
aucun faux rejet : c'est une vérification indépendante, sur le flux réel,
que $\mathrm{Pow} = 0$ implique bien « non strictement intérieur ».

## 5. Rétractation : le banc apparié de la veille ne comparait pas « avec » à « sans »

Le reçu `ADDENDUM_PREFILTRE_Q4_EQUATORIAL_20260819.md` § 5 publie
**×1,042** pour le préfiltre, dix victoires sur dix. Le plan était bien
apparié, contrebalancé et signé — la statistique est correcte. **Ce qui
était mesuré ne l'était pas.**

Le bras témoin était appelé avec le préfiltre en mode « sans
court-circuit », c'est-à-dire qu'il **calculait l'étage i64 et la
puissance de face, puis jetait le résultat**. Le banc comparait donc
« court-circuité » à « calculé puis jeté », et non « avec » à « sans ».
Le chiffre ×1,042 est **retiré**.

La correction ajoute un mode *éteint* (le bras témoin ne paie plus rien)
et un mode *puissance seule* (l'étage i64 est sauté), ce qui permet enfin
de poser les deux questions séparément :

| comparaison | question |
|---|---|
| mode 1 contre mode 0 | le préfiltre entier vaut-il son coût ? |
| mode 1 contre mode 3 | l'étage i64 vaut-il son coût, la puissance étant déjà là ? |

C'est la seconde qui répond littéralement à la réception demandée par
`6a2bc96` § 3 : « conserver le filtre seulement s'il retire une masse
mesurable avant `q3_power` **sans ralentir le banc apparié** » — et elle
ne se tranche pas en comparant le préfiltre à rien.

Les deux mesures sont au § 6.

## 6. Bancs appariés contrebalancés (n=8000, taille d'intérêt)

Même discipline que les précédents : échauffement non chronométré, ordre
ABBA, plan refusé si `--bench-repeat` est impair ou `< 4`, signature du
flux vérifiée à chaque exécution, **médiane des rapports appariés** comme
estimateur.

### Banc A — cascade complète (mode 1) contre AUCUN préfiltre (mode 0)

```text
rapports appaires cascade/temoin, dix paires :
0,9898  0,9353  0,9469  0,8990  1,0055
0,9647  0,9489  0,9439  0,9228  0,9967

mediane_appariee    = 0,9479  ->  x1,055   (ESTIMATEUR)
mediane_log         = -0,0535 ->  x1,055   (coherent)
rapport_de_medianes = 0,9576
victoires_cascade   = 9/10    P(X >= 9 | Bin(10 ; 1/2)) = 11/1024 = 0,011
ordre_sans_premier  = 5/5     flux_identique = oui
```

**×1,055**, soit **plus** que le ×1,042 retiré : le témoin biaisé
sous-estimait le gain, ce qui est exactement le sens attendu de l'erreur.
Le préfiltre entier vaut donc son coût, et c'est maintenant établi contre
un chemin qui existerait sans lui.

### Banc B — cascade complète (mode 1) contre PUISSANCE SEULE (mode 3)

```text
dix paires   : mediane_appariee = 0,9874  victoires_cascade = 8/10
vingt paires : mediane_appariee = 1,0021  victoires_cascade = 8/20
               rapport_de_medianes = 1,0060
               P(X >= 12 | Bin(20 ; 1/2)) = 0,252, et DANS L'AUTRE SENS
               ordre_sans_premier = 10/10  flux_identique = oui
```

**L'étage i64 ne fait gagner aucun temps mesurable.** À dix paires il
affichait ×1,013 avec huit victoires sur dix — j'ai failli publier ce
chiffre ; à vingt paires la médiane appariée repasse au-dessus de 1 et
les victoires tombent à 8/20. Le premier résultat était du bruit, et rien
dans le plan ne le distinguait du second : c'est la taille d'échantillon,
seule, qui a tranché.

L'explication est cohérente avec l'entonnoir : dans les **deux** bras, la
puissance de face court-circuite déjà `q4_form`. L'étage i64 n'économise
donc pas `q4_form` — il économise **un appel à `q3_power`** sur 15,3 %
des paires, c'est-à-dire trois produits `i128` sur une part d'un poste
qui n'est lui-même qu'une fraction de `t_gen`. Un gain de cet ordre est
sous le bruit de ce conteneur, et le banc le dit.

**Décision, et sur quel argument.** L'étage est **conservé**, mais pas
sur le temps : le critère de `6a2bc96` § 3 (« retirer une masse mesurable
sans ralentir le banc apparié ») est rempli — 13 353 260 paires retirées,
aucun ralentissement — alors que le bénéfice « probablement excellent »
annoncé au § conclusion **n'est pas observé**. Ce qui le justifie est une
quantité *comptée*, non une durée : il retire **environ 40 millions de
multiplications `i128`** du chemin chaud, et l'`i128` est précisément la
ressource rare du port GPU. Cet argument est explicitement un pari sur un
chantier futur, pas une mesure CPU ; `--q4-i64-stage-bench` le rejugera,
et un seul mode le supprime.

## 7. État

`ctest --test-dir build/v4` : **144 tests**, tous verts (141 avant : le
mutant `q4-eq-wrong-length`, devenu sans objet avec l'abandon de la
formule en six longueurs sur le chemin de production, est remplacé par
les quatre ci-dessus).

`python tools/check_docs.py`, `python tools/check_implementation_status.py`
et `python tools/check_passation.py` : verts.

## 8. Ce qui reste ouvert

- Les **trois autres faces** : toujours non branchées. Le cinquième
  restant des rejets du centre est leur seul gisement, et il faudrait
  reconstruire une forme `q3_form` par paire (les faces `aby`, `axy`,
  `bxy` contiennent toutes `y`), donc sans l'amortissement qui rend
  `abx` gratuite. À rouvrir sur mesure, pas sur principe.
- Le poste amont `t_anchor_cover` (12,6 s CPU) reste intouché et devient
  le premier candidat.

## 9. Reproduction

```bash
cmake -S morsehgp3D_v4 -B build/v4 -DCMAKE_BUILD_TYPE=Release
cmake --build build/v4 -j
ctest --test-dir build/v4 --output-on-failure                  # 144 tests
./build/v4/mhgp4_forest_probe --q4-eq-gate                     # 0
./build/v4/mhgp4_forest_probe --q4-eq-gate --inject=seed-face-power-nonstrict      # 4
./build/v4/mhgp4_forest_probe --q4-eq-gate --inject=seed-face-power-sign           # 4
./build/v4/mhgp4_forest_probe --q4-eq-gate --inject=seed-i64-vertex-y-drop-factor  # 4
./build/v4/mhgp4_forest_probe --q4-eq-gate --inject=seed-i64-pair-xy-min           # 4
./build/v4/mhgp4_forest_probe --q4-prefilter-bench --family=uniform \
    --n=8000 --s=8 --smax=11 --seed=3 --threads=4 --bench-repeat=10
./build/v4/mhgp4_forest_probe --q4-i64-stage-bench --family=uniform \
    --n=8000 --s=8 --smax=11 --seed=3 --threads=4 --bench-repeat=10
```
