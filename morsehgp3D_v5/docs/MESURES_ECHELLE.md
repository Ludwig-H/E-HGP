# Mesures d'échelle de la génération — où est le coût, et ce qui l'attaque

Document de **mesure** (les questions aux auditeurs vivent dans `audits/`,
qu'ils curent ; les mesures vivent ici pour être durables). Toutes les
grandeurs ci-dessous sont des **compteurs déterministes** ou des temps de mur
explicitement datés. Aucun statut public n'en découle.

Cadre : `phase=exploration_v5_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Sources : `receipts/campagne_g4_v5_20260828_grille/out/` (48 fils, G4),
runs locaux 8 fils, `bench/mhgp5_rect_probe`, `bench/mhgp5_q4_stage_probe`.

## 0. Les compteurs sont indépendants de la machine

Vérifié le 28 août 2026 : un run **local à 8 fils** et le reçu **G4 à 48 fils**
donnent, sur `scanline_single_pass` à $n = 100\,000$, exactement les mêmes
nombres — 48 557 755 ancres q4, 384 464 candidats q4. Les campagnes
d'exposant sur compteurs ne demandent donc **pas** de session payante ; seuls
les temps de mur et le RSS en demandent une.

## 1. Le mur, mesuré jusqu'à 200 000 points

| famille | mur total | lanes | part des lanes | candidats provisoires q4 | fold |
|---|---|---|---|---|---|
| `uniform` | $n^{1{,}09}$ | $n^{1{,}01}$ | 9,6 % → **8,4 %** | $n^{1{,}03}$ | — |
| `eight_clusters` | $n^{1{,}32}$ | $n^{1{,}73}$ | 16,9 % → 37,5 % | $n^{1{,}06}$ | — |
| `scanline_single_pass` | **$n^{2{,}72}$** | **$n^{3{,}14}$** | 19 % → **89,5 %** | **$n^{0{,}89}$** | $n^{1{,}07}$ |

À 200 000 points sur `scanline`, la génération occupe **91,17 % du mur**, la
somme des trois corps `rects` **89,50 %**, et le fold n'en pèse plus que 5,33 %.
Seule la sortie provisoire de la lane q4 est
sous-linéaire sur le dernier doublement; candidats totaux, boules, événements
et facettes restent approximativement linéaires. Entre 8 000 et 200 000, les
ancres q4 croissent de ×238,21 et les candidats q4 de ×14,95 : les ancres par
candidat passent de 16,94 à 269,94, soit une dégradation ×15,94, pas ×238. Le
mur q4 par candidat se dégrade séparément de ×100,81. Il n'existe pas un unique
« rendement ×238 » et cette sortie intermédiaire n'est pas la taille du
problème.

`terrain` n'a aucun run au-delà de 50 000 : sa pente s'y arrête.

## 2. La formulation exacte : la fraction de paires survivantes

La WSPD partitionne implicitement toutes les paires, l'élagage de rectangle en
tue une part, le reste devient des ancres. **Le catalogue explicite d'ancres est linéaire si et
seulement si la fraction survivante décroît en $n^{-1}$.**

| famille | 8 000 | 32 000 | 200 000 | exposant de la fraction |
|---|---|---|---|---|
| `uniform` | 4,010 % | 1,120 % | 0,195 % | −0,92 … **−0,96** |
| `eight_clusters` | 11,909 % | 6,618 % | 2,151 % | −0,51 … −0,70 |
| `scanline_single_pass` | 2,515 % | 1,358 % | 0,959 % | −0,61 … **−0,02** |

Sur `scanline`, entre 100 000 et 200 000, la fraction devient localement
presque plate ($n^{-0{,}02}$), autour de 0,96 % de toutes les paires : la masse
explicite suit donc une pente locale proche de $n^{1{,}98}$. Ce doublement ne
prouve ni une loi asymptotique ni $\Theta(n^{2})$. Sur `uniform`, la fraction
suit localement $n^{-0{,}96}$ sur l'intervalle indiqué.

Faire décroître la fraction survivante de `scanline` en $n^{-1}$ est donc une
cible falsifiable pour un **catalogue explicite**. Ce n'est pas l'unique contrat
algorithmique : un center-cover peut fermer un bloc sans émettre ses ancres et
un arrangement peut partager le traitement d'ancres nombreuses. L'objectif
général est « catalogue explicite linéaire **ou** traitement implicite
certifié à travail sous-quadratique ». Réduire la fraction reste une voie utile
d'élagage au niveau du rectangle, jamais une définition exhaustive du remède.

## 3. Où va le travail dans la lane q4 de `scanline`

| grandeur | 32 000 | 50 000 | 100 000 | exposants |
|---|---|---|---|---|
| covers construits | 1 673 861 | 2 643 464 | 5 436 957 | $n^{1{,}02}$ / $n^{1{,}04}$ |
| visites de points durant les covers | 708 593 623 | 1 401 729 056 | 5 363 089 001 | $n^{1{,}53}$ / $n^{1{,}94}$ |
| **tests de cœur** | 1 819 456 293 | 4 537 771 838 | **23 997 265 194** | $n^{2{,}05}$ / **$n^{2{,}40}$** |

Le plus grand compteur publié est le **test de cœur par seed** — 24,0 milliards
d'évaluations à 100 000 points. Cela ne suffit pas à le déclarer dominant en
temps : les unités n'ont pas le même prix et cette série locale n'a ni sortie
brute ni hash de binaire versionnés. Le nombre de covers construits croît près
du linéaire, mais leurs visites de points suivent déjà une pente 1,94 sur le
dernier doublement. Il faut donc chronométrer et apparier tests de cœur, visites
de points, sites retenus, profondeur et puissance avant de classer les postes.

## 4. Le raffinement post-séparation, mesuré

Mécanisme : prolonger la descente ternaire **à l'intérieur** d'un rectangle
vivant, en réévaluant le certificat universel de chaque sous-produit. Les
enfants radix partitionnent les paires du parent et un témoin universel du
parent le reste pour un enfant. Cela rend la mort d'un sous-produit sûre. Les
enfants ne deviennent toutefois pas des rectangles WSPD : la séparation n'est
pas héréditaire. L'objet reste inchangé seulement si l'aval itère exactement
l'antichaîne survivante, réapplique ses prétests sûrs et ferme les portes
post-RLE.

`scanline_single_pass`, lane q4 (`mhgp5_rect_probe --descente-seule`) :

| $n$ | ancres avant | ancres après | tuées | visites de nœuds ajoutées | visites par paire tuée |
|---|---:|---:|---:|---:|---:|
| 8 000 | 804 786 | 460 386 | 42,8 % | 43,98 M | 128 |
| 32 000 | 6 951 708 | 2 405 498 | 65,4 % | 580,42 M | 128 |
| 100 000 | 48 557 755 | **11 044 864** | **77,3 %** | 6 965,46 M | 186 |

Exposant des ancres : **avant** $n^{1{,}56}$ puis $n^{1{,}71}$ ; **après**
$n^{1{,}19}$ puis $n^{1{,}34}$. Le taux de mise à mort croît sur ces trois
tailles, mais les visites ajoutées suivent environ $n^{1{,}86}$ puis
$n^{2{,}18}$. Aucun rapport coût/gain n'est identifié. À 32 000, 4,546 M
paires sont tuées alors que seulement 1,674 M covers existent; à 100 000,
37,513 M paires sont tuées pour 5,437 M covers. L'intersection avec les paires
qui auraient réellement atteint un cover est inconnue. Multiplier toutes les
paires tuées par la moyenne globale de 423 puis 986 visites attribue donc des
covers inexistants et mélange visites de nœuds avec visites de points.

## 4 bis. Livré en production, derrière une option, digests bit-identiques

`GenerateOptions::postsep_refine_levels` ($L \in [0, 3]$, défaut 0),
`RunOptions::postsep_refine_levels`, CLI `--postsep=L` (refus hors domaine).
Porte `mhgp5_postsep_refine` (+ deux mutants, code 4) :

- **digest bit-identique à $L = 0, 1, 2, 3$** sur six familles, dont les deux
  contre-familles `two_lines` et `collinear_seven` — c'est la preuve que
  l'objet ne change pas (l'*ordre* d'énumération, lui, change quand B est
  scindé) ;
- **grand-livre exact par lane** : `émis + tués == base`, vérifié à chaque $L$
  et pour les trois lanes ; `base` est en outre invariante en $L$ ;
- **route q2 interdite** : `tués[q2] == 0` et `émis[q2] == base[q2]` à tout $L$ ;
- planchers de non-vacuité : au moins 1 000 paires tuées en q3 **et** en q4
  (mesuré : 180 959 et 224 667 à $L = 3$) ;
- mutants tués : `postsep-drop-child` (un enfant vivant jeté : le grand-livre
  le voit avant le digest) et `postsep-kill-h-minus-one` (seuil de mort à
  $h - 1$ : sur-tue, donc perd des boules).

Mesure à `scanline` $n = 4\,000$, masse de paires q4 : $L = 0$ → 305 981 ;
$L = 1$ → 242 224 (−20,8 %) ; $L = 2$ → 203 371 (−33,5 %) ; $L = 3$ → 185 120
(−39,5 %), pour 205 362 comptages universels.

**Le « réveil » d'histogramme q2 décrit par l'audit n'a pas pu être exhibé** :
ni sur ses quatre positions, ni sur 18 000 nuages entiers aléatoires (5 à 12
points, `smax` 3 à 7). Explication plausible : le cœur de l'enfant est
$\ge$ celui du parent, donc `need = h - core` diminue et compense la perte
d'histogramme. La route q2 reste fermée — elle ne coûte rien — mais elle est
gardée par un **invariant**, pas par un mutant : un mutant non réalisable ne
garde rien.

## 5. Ce qui n'est pas établi

- Le worktree contient une intégration CPU opt-in `L=0..3`, mais les chiffres
  ci-dessus viennent de la sonde, jamais du flux intégré. Le critère de
  réception — baisse du **temps** et des **visites payées**, à sorties exactes
  identiques — reste à établir.
- Rien au-delà de 100 000 pour le raffinement, or c'est entre 100 000 et
  200 000 que la fraction « avant » s'effondre à $n^{-0{,}02}$.
- La lane q3 n'est pas mesurée à grande échelle, ni `terrain`, ni
  `eight_clusters`.
- Le report « ancres $n^{1{,}34}$ ⟹ tests de cœur $n^{2{,}03}$ » est une
  **inférence** à coût par ancre constant, non une mesure.
- La série 100 000 et les masses d'étages ne possèdent pas encore de reçu brut
  versionné; elles restent diagnostiques jusqu'au pin, à la commande, au hash
  de binaire et à la sortie complète.
- La **route q2 doit rester interdite** : la contre-fixture
  `refine-hist-wakeup` (quatre positions, `s=1`, `smax=3`, `h2=2`) montre
  qu'un témoin du frère peut « revivre » dans l'histogramme enfant et faire
  émettre une boule supplémentaire, faute de prétest ponctuel en q2.
- Le critère `separated` n'est **pas héréditaire** (fixture 1D
  `x = {0, 99, 100, 512, 612}`, `s = 8`) : les sous-rectangles ne sont pas des
  rectangles WSPD, et ce post-traitement ne doit pas être appelé une nouvelle
  WSPD — le front canonique reste terminal à la première séparation.

## 6. Ce qui est fermé par théorème

Le résultat négatif est plus étroit que « généraliser la WSPD aux triplets ».
Pour $s>1$, une famille cercle--axe entièrement aiguë force
$\Omega(n^{2})$ blocs si l'on exige une décomposition ternaire **symétrique**,
fortement séparée et exact-once de tous ses supports croisés. Il ne ferme ni
les WSSD approximatives, ni une source asymétrique ancre--tiers, ni une source
restreinte à profondeur bornée, ni l'arrangement des centres. La preuve
autonome est le Théorème 4 de
[`RNG_JUNG_CLIQUES_ET_NIVEAUX_PEUPROFONDS.md`](../../docs/math/RNG_JUNG_CLIQUES_ET_NIVEAUX_PEUPROFONDS.md).

La phrase sur le circumrayon quasi collinéaire était fausse pour q3. Si $D$ est
la plus longue arête d'un triangle aigu, alors $R_c^{2}\leq D^{2}/3$ et le
circumcentre reste à distance carrée au plus $D^{2}/12$ de son milieu. C'est
précisément la localisation utilisée par l'arrangement q3.
