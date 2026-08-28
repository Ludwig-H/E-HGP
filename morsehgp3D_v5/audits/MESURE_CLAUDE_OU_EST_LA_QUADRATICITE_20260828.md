# Mesure Claude — où est exactement la quadraticité (28 août 2026)

Ancrage : compteurs `generation` des reçus de la session G4 n° 14, pin
`839cf1ecafb8` (`receipts/campagne_g4_v5_20260828_g0_g1/out/`), quatre tailles
appariées, mêmes graines, même binaire. Cadre :
`phase=exploration_v5_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=mesure`, `public_status=not_claimed`.

Ce document ne propose rien. Il **localise** le coût, parce qu'une proposition
faite sans cette localisation viserait au mauvais endroit.

## 1. Ce que la boucle fait réellement

`src/pipeline/generate.hpp` (lanes q3 et q4) :

```
for (i32 ua = ra.first; ua <= ra.last; ++ua)
  for (i32 ub = rb.first; ub <= rb.last; ++ub) { ++anchors[q]; ... }
```

Chaque rectangle **vivant** de la WSPD voit **toutes** ses paires énumérées.
Comme la WSPD partitionne exactement les paires, le majorant structurel est
$\sum_{\text{rect}} \left\vert A \right\vert \left\vert B \right\vert = \binom{n}{2}$ :
la quadraticité est *dans l'énumération*, pas dans la WSPD. Ce qui la retient
aujourd'hui est `alive_rectangles` (élagage au niveau du rectangle) puis les
tests d'ancre en $O(1)$ (histogrammes de coins, $W_q$, secteurs, grille de
cellules) — mais ceux-ci s'appliquent **après** que la paire a été énumérée.

## 2. Les exposants mesurés

Exposant local $e$ tel que la grandeur croît en $n^{e}$, entre tailles
consécutives (8 000 → 16 000 → 32 000 → 50 000).

### `uniform` — déjà quasi-linéaire

| grandeur | 8 000 | 16 000 | 32 000 | 50 000 | exposants |
|---|---|---|---|---|---|
| rectangles vivants q3 | 665 954 | 1 408 859 | 2 908 394 | 4 655 860 | 1,08 → 1,05 → 1,05 |
| ancres q3 | 1 098 708 | 2 315 622 | 4 867 765 | 7 802 774 | 1,08 → 1,07 → 1,06 |
| ancres q4 | 1 282 964 | 2 719 520 | 5 733 463 | 9 216 156 | 1,08 → 1,08 → 1,06 |
| seeds q3 | 12 292 070 | 25 818 041 | 54 269 565 | 86 995 992 | 1,07 → 1,07 → 1,06 |
| complétions q4 | 52 198 607 | 110 279 498 | 229 726 923 | 366 947 557 | 1,08 → 1,06 → 1,05 |
| évaluations Jung q4 | 43 175 547 | 91 133 772 | 191 649 146 | 307 025 250 | 1,08 → 1,07 → 1,06 |

Coût **par point** stable : 160 → 184 ancres q4, 5 397 → 6 141 évaluations
Jung. Sur cette famille il n'y a **rien à rendre sous-quadratique** : c'est
déjà $n^{1{,}06}$ avec une constante élevée.

### `terrain` — le travail par ancre devient **cubique**

| grandeur | 8 000 | 16 000 | 32 000 | 50 000 | exposants |
|---|---|---|---|---|---|
| rectangles vivants q3 | 207 772 | 424 347 | 862 401 | 1 362 505 | 1,03 → 1,02 → 1,02 |
| ancres q3 | 436 323 | 1 004 603 | 2 355 773 | 4 292 160 | 1,20 → 1,23 → 1,34 |
| ancres q4 | 491 665 | 1 178 984 | 2 875 525 | 5 405 171 | 1,26 → 1,29 → 1,41 |
| évaluations Jung q4 | 37 434 927 | 265 061 893 | 1 887 474 598 | 7 677 090 545 | **2,82 → 2,83 → 3,14** |

7,68 milliards d'évaluations à 50 000 points, soit **153 542 par point**.

### `eight_clusters` — beaucoup d'ancres, mais bon marché

| grandeur | 8 000 | 16 000 | 32 000 | 50 000 | exposants |
|---|---|---|---|---|---|
| rectangles vivants q3 | 564 502 | 1 231 555 | 2 610 700 | 4 202 134 | 1,13 → 1,08 → 1,07 |
| ancres q3 | 2 689 569 | 7 843 126 | 22 703 890 | 41 359 603 | 1,54 → 1,53 → 1,34 |
| ancres q4 | 3 810 453 | 10 681 086 | 33 884 685 | 63 319 157 | 1,49 → 1,67 → 1,40 |
| évaluations Jung q4 | 60 702 910 | 146 058 120 | 307 794 390 | 494 934 575 | **1,27 → 1,08 → 1,06** |

### `scanline_single_pass` — les deux mécanismes à la fois

| grandeur | 8 000 | 16 000 | 32 000 | 50 000 | exposants |
|---|---|---|---|---|---|
| rectangles vivants q3 | 173 190 | 343 373 | 687 997 | 1 075 716 | **0,99 → 1,00 → 1,00** |
| ancres q3 | 626 015 | 1 591 516 | 4 526 213 | 9 872 535 | **1,35 → 1,51 → 1,75** |
| ancres q4 | 804 786 | 2 114 704 | 6 951 708 | 14 543 505 | 1,39 → 1,72 → 1,65 |
| seeds q3 | 4 826 424 | 13 609 086 | 43 679 735 | 89 477 813 | 1,50 → 1,68 → 1,61 |
| complétions q4 | 17 139 132 | 46 483 440 | 152 913 065 | 335 781 778 | 1,44 → 1,72 → 1,76 |
| évaluations Jung q4 | 33 560 242 | 119 704 289 | 801 555 291 | 2 151 583 810 | **1,83 → 2,74 → 2,21** |

### Ancres q4 par rectangle vivant — la population des rectangles

| famille | 8 000 | 16 000 | 32 000 | 50 000 |
|---|---|---|---|---|
| `uniform` | 1,93 | 1,93 | 1,97 | **1,98 (constant)** |
| `terrain` | 2,37 | 2,78 | 3,33 | 3,97 |
| `scanline_single_pass` | 4,65 | 6,16 | 10,10 | 13,52 |
| `eight_clusters` | 6,75 | 8,67 | 12,98 | 15,07 |

## 3. Deux mécanismes distincts, et ils ne frappent pas les mêmes familles

Le coût s'écrit $\sum_{\text{rect vivant}} \left\vert A \right\vert \left\vert B \right\vert \times (\text{travail par ancre})$.
Les mesures séparent nettement les deux facteurs :

1. **La WSPD n'est en cause dans aucune famille.** Les rectangles vivants
   croissent en $n^{1{,}00}$ à $n^{1{,}07}$ partout. Toute proposition qui
   remplace la WSPD attaque un poste sain — et la remplacer par une
   décomposition d'ordre supérieur ne changerait pas ce constat.
2. **Facteur A — la population des rectangles vivants** croît sur trois
   familles sur quatre (× 1,7 sur `terrain`, × 2,2 sur `eight_clusters`,
   × 2,9 sur `scanline` entre 8 000 et 50 000) et reste **constante** sur
   `uniform`. C'est la double boucle `for ua × for ub` qui la paie
   intégralement, après quoi seulement les tests d'ancre en $O(1)$ s'appliquent.
3. **Facteur B — le travail par ancre** explose sur `terrain`
   ($n^{3{,}14}$, 153 542 évaluations Jung par point à 50 000) et sur
   `scanline` ($n^{2{,}21}$), mais reste **plat** sur `uniform` et
   `eight_clusters` ($n^{1{,}06}$). C'est le produit cover × seeds.

Conséquence pour la conception : **`eight_clusters` demande de couper le
facteur A** (beaucoup d'ancres, chacune bon marché) ; **`terrain` demande de
couper le facteur B** (peu d'ancres, chacune ruineuse) ; **`scanline` demande
les deux**. Une solution qui n'attaque qu'un des deux facteurs laissera une
famille de mesure en régime super-linéaire — et `terrain` comme `scanline`
sont les deux familles réalistes pour un nuage LiDAR.

La question n'est donc pas « rendre l'algorithme sous-quadratique » en
général : il est déjà en $n^{1{,}06}$ sur `uniform`, à constante élevée. Elle
est **double** : énumérer les ancres des rectangles peuplés de façon sensible
à la sortie, et borner le travail par ancre sur les géométries quasi-planes.

## 3 bis. La sonde des rectangles : l'amplification du cover et l'asymétrie

`bench/mhgp5_rect_probe`, relancé le 28 août aux deux tailles disponibles
localement (1 fil, machine partagée : les **compteurs** sont citables, pas les
temps).

| famille / lane | grandeur | 8 000 | 16 000 | exposant |
|---|---|---|---|---|
| `uniform` q3 | somme des covers d'ancre | 124 675 293 | 270 502 326 | $n^{1{,}12}$ |
| `uniform` q3 | amplification cover / points de rectangle | 0,49 | 0,48 | — |
| `uniform` q3 | seeds par survivant | 24,8 | 25,8 | — |
| `uniform` q3 | seeds par rectangle : médiane / p99 / max | 24 / 461 / 4 066 | 26 / 476 / 3 242 | — |
| `scanline` q3 | somme des covers d'ancre | 135 518 839 | 695 200 627 | **$n^{2{,}36}$** |
| `scanline` q3 | amplification cover / points de rectangle | 6,21 | **15,32** | — |
| `scanline` q3 | seeds par survivant | 82,4 | **217,5** | — |
| `scanline` q3 | seeds par rectangle : médiane / p99 / max | 5 / 3 493 / 299 919 | **5 / 10 053 / 849 452** | — |
| `scanline` q4 | somme des covers d'ancre | 46 823 188 | 188 958 845 | $n^{2{,}01}$ |
| `scanline` q4 | amplification cover / points de rectangle | 1,84 | 3,49 | — |
| `scanline` q4 | seeds par survivant | 215,1 | **484,7** | — |

Trois lectures, et ce sont les plus utiles du document :

1. **L'amplification du cover est le cœur.** Le rapport
   « somme des covers d'ancre / points des rectangles » dit combien de fois
   chaque point d'un rectangle est balayé. Il vaut **0,48 et stable** sur
   `uniform` ; il vaut **15,32 et croissant** sur `scanline` q3. Le cover est
   calculé depuis les **boîtes du rectangle** (`rect_cover_handles(ix,
   box_of(a), box_of(b), coef, …)`) : quand la géométrie est quasi-plane, les
   rectangles vivants ont des boîtes énormes, donc un $D_{\max}$ énorme, donc
   un cover énorme — et **chaque ancre du rectangle le repaie**.
2. **L'asymétrie est extrême et c'est un fait de charge, pas d'asymptote.**
   Sur `scanline` q3 à 16 000, la **médiane** est de 5 seeds par rectangle et
   le **maximum de 849 452** : un rapport de 169 890 (contre 125 sur
   `uniform`). Un seul rectangle porte 0,53 % de tous les seeds. C'est
   rédhibitoire pour un GPU : le bloc qui hérite de ce rectangle décide seul du
   mur.
3. **L'élagage d'ancre est déjà presque saturé en q3, pas en q4.** Sur
   `scanline` q3 la production ($W_q$ exact + secteurs) tue **50,1 % des ancres
   et 91,4 % des seeds** — il ne reste que 8,6 % à gagner, et ils croissent
   quand même en $n^{2{,}4}$. Sur `scanline` q4 elle ne tue que **21,3 % des
   ancres et 56,9 % des seeds**, et $W_q$ n'y tue **rien** (0 ancre) : toute la
   marge d'élagage restante est en q4. Aucun faux positif nulle part
   (`FAUX POSITIFS = 0`) : les tests d'ancre sont sûrs.

## 3 ter. Le palmarès des rectangles lourds — et la correction d'une explication

La sonde possédait déjà le palmarès que je réclamais au § 4 ; mon extraction
l'avait tronqué. Sur `scanline_single_pass` q3, $n = 16\,000$ :

```
palmares : 1 % des rectangles les plus lourds (3434) = 80,3 % des seeds,
           0,2 % des survivants ; seeds dans les rectangles SANS survivant = 98,1 %
  rect#0  Dmax=298 |A||B|=1600 vivantes=960 tuees_prod=960 seeds=849452 survivants=0 covers=4041952
  rect#1  Dmax=291 |A||B|=1520 vivantes=798 tuees_prod=798 seeds=675964 survivants=0 covers=3179869
  rect#2  Dmax=322 |A||B|=2200 vivantes=592 tuees_prod=592 seeds=649352 survivants=0 covers=2532935
```

(la colonne `seeds` est **contrefactuelle** — corps de production sans les
tests d'ancre — c'est ce que les tests évitent, pas ce qui est produit.)

Quatre lectures, dont deux corrigent ce que j'avais avancé :

1. **La concentration est écrasante** : 1 % des rectangles portent 80,3 % du
   travail et 0,2 % du résultat ; **98,1 % des seeds vivent dans des
   rectangles qui ne produisent aucun survivant**. Le travail n'est pas
   « réparti et coûteux », il est **massivement inutile et concentré**.
2. **Mon explication du § 3 bis était trop rapide.** Les rectangles les plus
   lourds n'ont pas des $\left\vert A \right\vert \left\vert B \right\vert$
   démesurés (1 600, 1 520, 2 200 paires : deux à trois ordres en dessous de
   $n$) ; ce qui les distingue est leur $D_{\max}$ — 298, 291, 322 sur un nuage
   dont c'est une fraction notable du diamètre. Le coût vient donc du
   **rayon**, pas de la population : un grand $D$ donne un cover
   $\propto D^{3}$ et un nombre de seeds proportionnel.
3. **Et la production les tue déjà tous** : `vivantes=960 tuees_prod=960` sur
   le rectangle le plus lourd. Le vidage de son ancre la plus lourde
   ($a = (126, 249, 11)$, $b = (92, 1, 147)$, $D = 284{,}9$) montre pourquoi :
   sa boule diamétrale contient déjà **2 176 sites**, soit 218 fois le seuil
   $h_3 = 10$. Ces ancres sont mortes d'avance et le test de cœur le voit.
   Le gain restant n'est donc **pas** sur les rectangles lourds.
4. **Ce qui reste coûteux est le régime intermédiaire.** Après élagage, il
   reste 13,6 M seeds à $n = 16\,000$ sur `scanline` q3, et ils croissent en
   $n^{1{,}50}$. Ils viennent des rectangles que les tests d'ancre ne tuent
   pas — ni assez petits pour être triviaux, ni assez dégénérés pour être
   tués. C'est **là** qu'une idée nouvelle doit porter, et c'est beaucoup plus
   étroit que « rendre la génération sous-quadratique ».

## 4. Ce qui n'est pas mesuré, et que je ne prétends pas savoir

- Aucune mesure au-delà de $n = 50\,000$ : les exposants ci-dessus sont des
  pentes locales sur moins d'une décade, pas des asymptotes.
- `two_lines` et `collinear_seven` sont des contre-familles de réfutation, pas
  des régimes : elles n'ont pas d'exposant.
- Les évaluations Jung sont un **compteur d'instrument**, pas un temps : elles
  mesurent le travail de la lane q4, pas le mur. Le lien travail → temps n'est
  pas linéaire (vectorisation, cache).
- La sonde des rectangles n'a été relancée qu'à 8 000 et 16 000 (§ 3 bis) :
  les exposants qu'elle donne reposent sur **deux** points, pas quatre.
- Le § 3 ter mesure le $D_{\max}$ et **corrige** l'explication du § 3 bis : le
  coût des rectangles lourds vient de leur rayon, pas de leur population.
- Le palmarès n'a été lu que sur `scanline` q3 à $n = 16\,000$. Les autres
  familles et la lane q4 sont en cours ; sans elles, la phrase « le gain
  restant n'est pas sur les rectangles lourds » ne vaut que pour ce cas.
- Le régime intermédiaire (§ 3 ter, point 4) n'est **pas** caractérisé : je
  sais ce qu'il coûte, pas à quoi il ressemble. C'est la mesure suivante.
