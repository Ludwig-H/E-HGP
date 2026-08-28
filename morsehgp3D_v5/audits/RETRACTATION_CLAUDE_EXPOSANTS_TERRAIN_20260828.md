# Rétractation Claude — « terrain ne tient pas 10 M » était faux (28 août 2026)

Ancrage : compteurs `generation` des reçus de production, pin `839cf1ec`.
Cadre : `phase=exploration_v5_hors_registre`, `backend=cpu_reference`,
`mode=mesure`, `public_status=not_claimed`.

## Ce que je retire

J'ai écrit, dans `QUESTION_CLAUDE_EXPOSANTS_PAR_REGIME_20260828.md`, que
`terrain` demanderait **31 jours** de calcul agrégé à 10 M points et qu'il lui
manquait « exactement 0,86 d'exposant ». **C'est faux, et l'audit avait
identifié la cause avant que je ne la mesure** : ma colonne « évaluations
Jung » était `jung_cert_skip`, un proxy étroit — ni le coût de la lane q4, ni
son temps.

Avec `completions_q4`, qui est un **vrai compteur de travail de production**
(les complétions effectivement énumérées par la lane q4), les mêmes reçus
donnent :

| famille | 8 000 | 16 000 | 32 000 | 50 000 | exposants locaux |
|---|---|---|---|---|---|
| `uniform` | 52 198 607 | 110 279 498 | 229 726 923 | 366 947 557 | 1,08 → 1,06 → **1,05** |
| `terrain` | 17 542 086 | 73 431 917 | 256 463 974 | 514 838 729 | 2,07 → 1,80 → **1,56** |
| `scanline_single_pass` | 17 139 132 | 46 483 440 | 152 913 065 | 335 781 778 | 1,44 → 1,72 → **1,76** |

Et pour les seeds q4 : `uniform` 1,05 ; `terrain` 1,80 → 1,63 → **1,48** ;
`scanline` 1,25 → 1,74 → 1,53.

**L'exposant de `terrain` DÉCROÎT** (2,07 → 1,80 → 1,56) là où mon proxy le
montrait croissant (2,82 → 2,83 → 3,14). Le coût par ancre y sature d'ailleurs
visiblement : seeds par ancre 9,4 → 13,7 → 17,4 → **17,9**, complétions par
seed 3,78 → 4,55 → 5,13 → **5,33**.

Extrapolation refaite avec l'exposant du dernier intervalle et le même débit
hypothétique de $4{,}8 \times 10^{10}$/s (**toujours non mesuré**) :

| famille | exposant | complétions à 10 M | temps agrégé |
|---|---|---|---|
| `uniform` | 1,05 | $9{,}5 \times 10^{10}$ | **2,0 s** |
| `terrain` | 1,56 | $2{,}0 \times 10^{12}$ | **42 s** |
| `scanline_single_pass` | 1,76 | $3{,}8 \times 10^{12}$ | **1,3 min** |

## Ce que cela change

1. **La génération n'est pas le mur du passage à 10–30 M.** Les trois régimes
   mesurés y sont plausibles à la minute près, pas à la journée. Ma
   dramatisation était un artefact de proxy.
2. **Le seul exposant qui se dégrade est celui de `scanline`** (1,44 → 1,72 →
   1,76), et c'est le seul point de vigilance de la génération. `terrain`, que
   je désignais comme le régime perdu, s'améliore.
3. **Le mur réel du passage à l'échelle est la mémoire**, pas le temps de
   génération : ≈ 0,35 Mo par point au pic du fold (mesuré : `max_fold` de
   17,7 Go à 50 000 points), soit ≈ 3,5 To à 10 M. C'est le chantier L2–L4
   (réducteur vivant, durées de vie externes, amont streamé), pas la
   sous-quadraticité de la génération.
4. **Le raffinement post-séparation reste ce que la mesure en dit** : un gain
   sur `scanline` q3 seulement, nul ou négatif ailleurs. Il n'était déjà pas
   la réponse ; il l'est encore moins maintenant que `terrain` n'est plus le
   problème.

## Réserves maintenues, y compris contre cette correction

- Les quatre tailles ne viennent toujours pas du **même binaire** ni du même
  `coord` (8/16/32 k de `mhgp5_conformity_v4`, 50 k de `mhgp5_probe`) : ce
  sont des pentes à source épinglée, pas une loi. La série appariée reste à
  faire, et elle peut encore déplacer ces exposants.
- `completions_q4` est **un** compteur de travail, pas le mur : le préfiltre,
  le census, l'expansion et le fold n'y sont pas, et à 50 000 points le fold
  pèse 44,6 % du mur.
- Le débit de $4{,}8 \times 10^{10}$/s n'est pas mesuré ; ces temps sont des
  ordres de grandeur conditionnels.
- Une pente locale sur moins d'une décade ne dit rien de l'asymptote. Que
  `terrain` décroisse de 2,07 à 1,56 est encourageant et non concluant.

## Ce que je fais de cette correction

- Je ne propose plus aucune refonte de la génération au nom de l'échelle.
- Je reporte l'effort sur ce que la mesure désigne : la **mémoire** (L2–L4) et
  la **vigilance sur `scanline`**.
- Je note la leçon de méthode, qui est la vraie : **un proxy nommé comme s'il
  était le coût a produit trois documents et une extrapolation faux de deux
  ordres de grandeur.** L'audit l'a vu à la lecture du nom du compteur. Je
  vérifierai désormais qu'un compteur mesure ce que son nom prétend avant de
  bâtir dessus.
