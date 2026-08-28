# Mesures d'échelle de la génération — où est le coût, et ce qui l'attaque

Document de **mesure** (les questions aux auditeurs vivent dans `audits/`,
qu'ils curent ; les mesures vivent ici pour être durables). Toutes les
grandeurs ci-dessous sont des **compteurs déterministes** ou des temps de mur
explicitement datés. Aucun statut public n'en découle.

Sources : `receipts/campagne_g4_v5_20260828_grille/out/` (48 fils, G4),
runs locaux 8 fils, `bench/mhgp5_rect_probe`, `bench/mhgp5_q4_stage_probe`.

## 0. Les compteurs sont indépendants de la machine

Vérifié le 28 août 2026 : un run **local à 8 fils** et le reçu **G4 à 48 fils**
donnent, sur `scanline_single_pass` à $n = 100\,000$, exactement les mêmes
nombres — 48 557 755 ancres q4, 384 464 candidats q4. Les campagnes
d'exposant sur compteurs ne demandent donc **pas** de session payante ; seuls
les temps de mur et le RSS en demandent une.

## 1. Le mur, mesuré jusqu'à 200 000 points

| famille | mur total | lanes | part des lanes | sortie (candidats q4) | fold |
|---|---|---|---|---|---|
| `uniform` | $n^{1{,}09}$ | $n^{1{,}01}$ | 9,6 % → **8,4 %** | $n^{1{,}03}$ | — |
| `eight_clusters` | $n^{1{,}32}$ | $n^{1{,}73}$ | 16,9 % → 37,5 % | $n^{1{,}06}$ | — |
| `scanline_single_pass` | **$n^{2{,}72}$** | **$n^{3{,}14}$** | 19 % → **89,5 %** | **$n^{0{,}89}$** | $n^{1{,}07}$ |

À 200 000 points sur `scanline`, la génération occupe **89,5 % du mur**, le
fold reste linéaire et n'en pèse plus que 5,3 %, et la **sortie est
sous-linéaire** : 710 211 candidats pour 191 710 560 ancres. Le travail par
unité de sortie croît d'un facteur 238 entre 8 000 et 200 000 : c'est le
**rendement** qui s'effondre, pas la taille du problème.

`terrain` n'a aucun run au-delà de 50 000 : sa pente s'y arrête.

## 2. La formulation exacte : la fraction de paires survivantes

La WSPD énumère toutes les paires, l'élagage de rectangle en tue une part, le
reste devient des ancres. **Le nombre d'ancres est linéaire si et seulement si
la fraction survivante décroît en $n^{-1}$.**

| famille | 8 000 | 32 000 | 200 000 | exposant de la fraction |
|---|---|---|---|---|
| `uniform` | 4,010 % | 1,120 % | 0,195 % | −0,92 … **−0,96** |
| `eight_clusters` | 11,909 % | 6,618 % | 2,151 % | −0,51 … −0,70 |
| `scanline_single_pass` | 2,515 % | 1,358 % | 0,959 % | −0,61 … **−0,02** |

Sur `scanline`, entre 100 000 et 200 000, la fraction **cesse de décroître**
($n^{-0{,}02}$), bloquée à ≈ 0,96 % de toutes les paires : c'est, à la lettre,
la définition de la quadraticité. Sur `uniform` elle décroît en $n^{-0{,}96}$,
d'où la santé de cette famille.

**L'objectif est donc un énoncé unique et falsifiable : faire décroître la
fraction survivante de `scanline` en $n^{-1}$.** C'est un objectif d'élagage
au niveau du **rectangle** ; ni la WSPD, ni le fold, ni la sortie n'y entrent.

## 3. Où va le travail dans la lane q4 de `scanline`

| grandeur | 32 000 | 50 000 | 100 000 | exposants |
|---|---|---|---|---|
| covers construits | 1 673 861 | 2 643 464 | 5 436 957 | $n^{1{,}02}$ / $n^{1{,}04}$ |
| visites de handles | 708 593 623 | 1 401 729 056 | 5 363 089 001 | $n^{1{,}53}$ / $n^{1{,}94}$ |
| **tests de cœur** | 1 819 456 293 | 4 537 771 838 | **23 997 265 194** | $n^{2{,}05}$ / **$n^{2{,}40}$** |

Le poste dominant est le **test de cœur par seed** — 24,0 milliards
d'évaluations à 100 000 points. Les covers *construits* croissent, eux,
linéairement : ce n'est pas leur construction qui gouverne, c'est le nombre
d'ancres qui les rebalaient.

## 4. Le raffinement post-séparation, mesuré

Mécanisme : prolonger la descente ternaire **à l'intérieur** d'un rectangle
vivant, en réévaluant le certificat universel de chaque sous-rectangle. Scinder
un rectangle vivant **partitionne** ses paires — objet, complétude et critère
terminal de la WSPD intacts — et resserre les boîtes, donc augmente les
témoins universels, avec le prédicat de production inchangé. Aucun théorème
nouveau n'est requis.

`scanline_single_pass`, lane q4 (`mhgp5_rect_probe --descente-seule`) :

| $n$ | ancres avant | ancres après | tuées | visites de nœud par paire tuée | visites de site par cover | rapport |
|---|---|---|---|---|---|---|
| 8 000 | 804 786 | 460 386 | 42,8 % | 128 | — | — |
| 32 000 | 6 951 708 | 2 405 498 | 65,4 % | 128 | 423 | **3,3 : 1** |
| 100 000 | 48 557 755 | **11 044 864** | **77,3 %** | 186 | 986 | **5,3 : 1** |

Exposant des ancres : **avant** $n^{1{,}56}$ puis $n^{1{,}71}$ ; **après**
$n^{1{,}19}$ puis $n^{1{,}34}$. Le taux de mise à mort et le rapport coût/gain
**croissent** tous deux avec la taille, parce que le cover par ancre grossit
(423 → 986 sites) plus vite que le coût du test (128 → 186 visites).

## 5. Ce qui n'est pas établi

- Le raffinement n'est **pas implémenté** dans le pipeline : ces chiffres sont
  ceux d'une sonde, jamais du flux de production. Le critère de réception —
  baisse du **temps** et des **visites payées** — reste à établir.
- Rien au-delà de 100 000 pour le raffinement, or c'est entre 100 000 et
  200 000 que la fraction « avant » s'effondre à $n^{-0{,}02}$.
- La lane q3 n'est pas mesurée à grande échelle, ni `terrain`, ni
  `eight_clusters`.
- Le report « ancres $n^{1{,}34}$ ⟹ tests de cœur $n^{2{,}03}$ » est une
  **inférence** à coût par ancre constant, non une mesure.
- La **route q2 doit rester interdite** : la contre-fixture
  `refine-hist-wakeup` (quatre positions, `s=1`, `smax=3`, `h2=2`) montre
  qu'un témoin du frère peut « revivre » dans l'histogramme enfant et faire
  émettre une boule supplémentaire, faute de prétest ponctuel en q2.
- Le critère `separated` n'est **pas héréditaire** (fixture 1D
  `x = {0, 99, 100, 512, 612}`, `s = 8`) : les sous-rectangles ne sont pas des
  rectangles WSPD, et ce post-traitement ne doit pas être appelé une nouvelle
  WSPD — le front canonique reste terminal à la première séparation.

## 6. Ce qui est fermé par théorème

La **généralisation de la WSPD aux triplets** : pour tout $s > 4$, il existe
des nuages dont toute décomposition ternaire $s$-séparée a $\Omega(n^{2})$
triplets, et la séparation ne borne pas le circumrayon (triplet quasi
colinéaire). Énoncé, preuve et contre-famille :
`audits/QUESTION_CLAUDE_TRIPLETS_IMPOSSIBILITE_20260828.md`. Ce théorème
**justifie** la conception actuelle : l'ancrage sur l'arête maximale joint à
l'acuité localise le centre là où la séparation échoue.
