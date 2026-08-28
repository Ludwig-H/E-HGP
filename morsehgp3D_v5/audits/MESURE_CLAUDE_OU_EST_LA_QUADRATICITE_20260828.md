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

### `scanline_single_pass` — c'est là que ça casse

| grandeur | 8 000 | 16 000 | 32 000 | 50 000 | exposants |
|---|---|---|---|---|---|
| rectangles vivants q3 | 173 190 | 343 373 | 687 997 | 1 075 716 | **0,99 → 1,00 → 1,00** |
| ancres q3 | 626 015 | 1 591 516 | 4 526 213 | 9 872 535 | **1,35 → 1,51 → 1,75** |
| ancres q4 | 804 786 | 2 114 704 | 6 951 708 | 14 543 505 | 1,39 → 1,72 → 1,65 |
| seeds q3 | 4 826 424 | 13 609 086 | 43 679 735 | 89 477 813 | 1,50 → 1,68 → 1,61 |
| complétions q4 | 17 139 132 | 46 483 440 | 152 913 065 | 335 781 778 | 1,44 → 1,72 → 1,76 |
| évaluations Jung q4 | 33 560 242 | 119 704 289 | 801 555 291 | 2 151 583 810 | **1,83 → 2,74 → 2,21** |

Coût **par point** en explosion : 101 → 291 ancres q4, et **4 195 → 43 032**
évaluations Jung par point, soit × 10,3 quand $n$ est multiplié par 6,25.

## 3. Les trois faits qui en découlent

1. **La WSPD n'est pas en cause.** Le nombre de rectangles vivants croît en
   $n^{1{,}00}$ sur `scanline` comme en $n^{1{,}05}$ sur `uniform`. Toute
   proposition qui remplace la WSPD attaque un poste sain.
2. **Le coupable est le rapport ancres / rectangle**, c'est-à-dire le
   $\left\vert A \right\vert \left\vert B \right\vert$ moyen des rectangles
   *vivants* : constant sur `uniform` (1,65 → 1,68) et **croissant sur
   `scanline`** (3,61 → 4,63 → 6,58 → 9,18, soit $\approx n^{0{,}5}$). Les
   rectangles vivants d'un nuage de balayage deviennent de plus en plus gros ;
   la double boucle les paie intégralement.
3. **Le travail par ancre croît lui aussi** sur `scanline` : les évaluations
   Jung montent plus vite que les ancres ($n^{2{,}2}$ contre $n^{1{,}7}$),
   donc le cover et le nombre de seeds par ancre grossissent également.
   Le produit des deux est ce qui donne le $n^{2{,}2}$–$n^{2{,}7}$ observé.

Autrement dit, la question n'est **pas** « rendre l'algorithme
sous-quadratique » en général — il est déjà en $n^{1{,}06}$ sur `uniform`.
Elle est : **pourquoi les nuages de balayage produisent-ils des rectangles
vivants dont la population croît, et comment énumérer leurs ancres de manière
sensible à la sortie ?** C'est la géométrie quasi-coplanaire d'un passage
LiDAR unique qui est en cause, et c'est exactement la famille qui compte pour
`tests/SemanticKITTI/`.

## 4. Ce qui n'est pas mesuré, et que je ne prétends pas savoir

- Aucune mesure au-delà de $n = 50\,000$ : les exposants ci-dessus sont des
  pentes locales sur moins d'une décade, pas des asymptotes.
- `terrain` et `eight_clusters` ne sont pas dans ce tableau (les reçus les
  portent, mais je n'ai pas encore calculé leurs exposants) ; `two_lines` et
  `collinear_seven` sont des contre-familles de réfutation, pas des régimes.
- Le $\left\vert A \right\vert \left\vert B \right\vert$ moyen est déduit du
  quotient ancres/rectangles, pas mesuré par un histogramme : la distribution
  (quelques rectangles énormes, ou un grossissement général ?) reste à établir
  — `bench/rect_probe.cpp` la donne, elle n'a pas encore été relancée aux
  quatre tailles.
