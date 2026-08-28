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

## 0. Les compteurs sémantiques de pente sont reproductibles sur le cas apparié

Vérifié le 28 août 2026 : un run **local à 8 fils** et le reçu **G4 à 48 fils**
donnent, sur `scanline_single_pass` à $n = 100\,000$, exactement les mêmes
nombres — 48 557 755 ancres q4, 384 464 candidats q4. Ces deux compteurs
sémantiques CPU peuvent donc servir aux campagnes locales de pente sur ce cas.
Cela ne s'étend ni aux compteurs d'ouvriers/device, ni aux temps, ni au RSS.

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
enfants ne deviennent toutefois pas le nouveau front WSPD canonique : la
séparation n'est pas héréditaire. L'intégration annule donc une scission avant
tout effet si l'un des deux enfants échoue au prédicat entier `separated`, puis
conserve `max(core_parent, core_enfant)`. L'aval itère exactement l'antichaîne
survivante et réapplique ses prétests sûrs.

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

## 4 bis. Raccordé au chemin CPU de référence, opt-in, sous portes bornées

`GenerateOptions::postsep_refine_levels` ($L \in [0, 3]$, défaut 0),
`RunOptions::postsep_refine_levels`, CLI `--postsep=L` (parse intégral et refus
hors domaine dans la bibliothèque). Les lanes CPU par lots propagent maintenant
la politique et sont appariées à $L=1$. Un override `RunOptions` arbitraire
reste refusé pour `L>0` faute de capacité déclarée; le CLI CUDA ne l'expose pas
et aucune mesure device n'est annoncée. Porte
`mhgp5_postsep_refine` (+ cinq mutants, code 4) :

- **objet complet identique à $L = 0, 1, 2, 3$** sur six familles :
  `digest_raw_candidates` du multiensemble canonique pré-RLE, activé seulement
  par l'option diagnostique de la porte afin que le `--digest` historique ne
  paie pas ce second hachage,
  `digest_balls`, cardinalités brutes par lane, événements, `batch_levels` et
  forêts; le bras $L=3$ est aussi identique entre un et quatre fils ;
- **grand-livre exact et fail-closed par lane** : `émis + tués == base`,
  vérifié avant RLE et avant toute publication; `base` est invariante en $L$.
  Un oracle test-only développe en outre le multiensemble littéral des couples
  sur de petits arbres et exerce des scissions de A et B ;
- **route q2 interdite structurellement** : en plus de `tués[q2] == 0` et
  `émis[q2] == base[q2]`, le pipeline exige zéro état, comptage ou rollback de
  raffinement et `parents == produits == rect_alive` à tout $L$ ;
- planchers de non-vacuité en q3 et q4, rollback non héréditaire gravé et
  compte frais monotone sur les familles de la porte ;
- mutants tués : perte d'un enfant, duplication d'un enfant, seuil de mort à
  $h-1$, ouverture q2 et recomptage enfant sans l'autorité de coin. Les deux
  dernières fautes gardent un ledger exact : la première change le multiensemble
  pré-RLE et `digest_balls`, la seconde est refusée par
  `fresh_child < parent.core` avant digest. La somme scalaire et les signatures
  aval ne jouent donc pas le même rôle.

Première mesure **intégrée** locale, huit fils, sur le worktree local de la
campagne non épinglé par un reçu brut, un seul run par bras donc diagnostique :

| $n$ | $L$ | masse q4 tuée | appels de cœur q4 | nœuds visités q4 | génération |
|---:|---:|---:|---:|---:|---:|
| 4 000 | 0 | 0 % | 0 | 0 | 1,266 s |
| 4 000 | 3 | 39,1 % | 203 062 | 19,90 M | 1,655 s |
| 8 000 | 0 | 0 % | 0 | 0 | 3,356 s |
| 8 000 | 1 | 21,8 % | 185 520 | 17,05 M | 3,781 s |
| 8 000 | 2 | 36,0 % | 338 994 | 34,62 M | 3,961 s |
| 8 000 | 3 | 43,7 % | 416 206 | 49,11 M | 4,294 s |
| 16 000 | 0 | 0 % | 0 | 0 | 9,088 s |
| 16 000 | 1 | 20,7 % | 363 952 | 34,52 M | 9,788 s |
| 16 000 | 3 | 44,0 % | 775 762 | 99,13 M | 10,862 s |

Le raccord tue donc une masse réelle, mais **ralentit tous les bras mesurés**.
À 16 000, $L=3$ épargne seulement 0,137 s dans les corps q3+q4, tandis que
la phase WSPD+raffinement q3+q4 ajoute 1,913 s. Le verrou est maintenant mieux
localisé : raffiner tous les parents n'est pas recevable; la prochaine sonde
doit stratifier rendement et coût par `h-core`, masse $\lvert A\rvert\lvert B\rvert$ et géométrie avant de tester une politique sélective. Une baisse de
masse seule ne vaut pas un gain de complexité ni de mur.

La première construction q2 à quatre positions n'était pas un rectangle radix
valide, mais une fixture valide à six points réalise ensuite le réveil : à
`s=1`, `smax=3`, un parent tue l'ancre `(2,5)` avec
`core + h_a + h_b = 0 + 3 + 0`, puis un sous-rectangle séparé la réveille avec
`0 + 1 + 0 < h2=2`. Le chemin test-only ouvert passe de 13 à 14 candidats et
ajoute une boule RLE, alors même que `tués[q2] == 0` et
`émis[q2] == base[q2]`. La monotonie du cœur ne compense donc pas
universellement la perte d'histogramme ; la route q2 reste fermée et cette
fixture doit tuer son mutant dédié. Les coordonnées et facteurs exacts sont
épinglés dans
[`QUESTION_CLAUDE_LANE_RESIDENTE_20260828.md`](../audits/QUESTION_CLAUDE_LANE_RESIDENTE_20260828.md).

## 4 ter. Sur le chemin CPU intégré local, le raffinement coupe 47 % des ancres et le mur augmente de 34 %

C'est la mesure que l'audit exigeait — « le succès n'est pas une baisse de
`seeds_cf` : il faut une baisse du **temps** et des **visites payées**
supérieure au surcoût des nouveaux comptages » — et elle est **défavorable**.

`scanline_single_pass`, $n = 100\,000$, 8 fils, avec digest, un seul passage
local sans sortie brute ni hash de binaire au tip :

| grandeur | `--postsep=0` | `--postsep=3` | écart |
|---|---|---|---|
| digest | `c5c40203…` | `c5c40203…` | **identique** ✓ |
| ancres q3 | 31 478 160 | **16 750 196** | **− 46,8 %** |
| ancres q4 | 48 557 755 | **25 590 309** | **− 47,3 %** |
| rectangles produits (q3 / q4) | 2 142 760 / 2 181 866 | 3 562 300 / 3 635 193 | + 66 % |
| **mur total** | **535 189 ms** | **717 202 ms** | **+ 34,0 %** |

**Le raffinement échoue son critère local de réception.** Il supprime près de
la moitié des ancres et coûte quand même un tiers de mur en plus. Ce verdict
justifie le défaut $L=0$; la durée isolée reste diagnostique, pas un reçu.

### Signal de redondance sur ce run, pas preuve structurelle

Le tableau disponible compare seulement une partie des compteurs de génération
entre les deux runs :

| compteur | `--postsep=0` | `--postsep=3` | |
|---|---|---|---|
| `rect_alive` au pin de mesure | 1 505 707 / 2 142 760 / 2 181 866 | idem | ancien sens : parents |
| `ancres` | 4 012 092 / 31 478 160 / 48 557 755 | 4 012 092 / 16 750 196 / 25 590 309 | **− 47 %** |
| `ancres_w3` | 8 190 272 | 5 427 276 | − 34 % |
| `ancres_w4` | 12 487 587 | 8 592 501 | − 31 % |
| **`candidats`** | 2 698 176 / 4 544 950 / 384 464 | **identique** | — |
| **`tues_profondeur`** | 0 / 318 183 753 / 129 399 348 | **identique** | — |

Le libellé `rect_alive` avait alors été déplacé silencieusement vers le nombre
de parents; le contrat courant l'a restauré au nombre de produits effectivement
remis à la lane et expose `postsep_parent_rects` séparément. Les nombres de ce
tableau restent donc historiques et ne doivent pas être relus avec le nouveau
sens du compteur.

Surtout, `tues_profondeur` ne compte pas chaque seed : il compte seulement les
seeds q3 ou complétions q4 qui atteignent le filtre final puis y meurent. En q3,
les seeds tués par cellules constituent une autre sortie. En q4, les seeds
peuvent mourir par cellules, cœur ou corde, puis chaque survivant engendre des
complétions qui se répartissent entre sept rejets amont, morts de profondeur et
candidats. L'égalité de `tues_profondeur` et `candidats` n'implique donc ni
l'égalité du nombre de seeds, ni celle des covers, des tests de cœur ou des
complétions. Le digest final identique prouve l'objet publié sur ce run, pas le
travail intermédiaire.

La conclusion recevable est plus étroite : sur cet unique run `scanline`, le
raffinement ne réduit ni les candidats ni les morts au filtre final rapportés,
et le mur augmente de 34 %. Cela constitue un **signal** que beaucoup de ses
mises à mort recouvrent les prétests d'ancre déjà payés, pas une preuve de
redondance structurelle. Pour trancher, une cible de profil intégrée doit
comparer à `L=0/3` les covers construits, visites et sites de cover, seeds,
tests de cœur, complétions, entrées de profondeur, tests de puissance et chaque
classe de rejet, avec sortie brute, pin/hash de binaire et répétitions appariées.

**La cible candidate suivante**, indiquée mais non prouvée par la même mesure :
seuls ≈ 11 % des ancres q4 construisent un cover (5 436 957 covers pour
48 557 755 ancres à $n = 100\,000$) et ce sont elles qui engendrent les seeds
et les **24,0 milliards de tests de cœur**. Un mécanisme utile doit réduire le
travail de ces survivantes-là; ce poste doit être mesuré directement avant de
conclure que toutes les ancres supprimées mouraient déjà à coût négligeable.

Pourquoi mon estimation précédente (« 3,3 puis 5,3 pour 1 en faveur du
raffinement ») était fausse : je comparais des visites de nœud à un **cover
entier** par paire tuée. Sur le chemin intégré, le cover n'est pas payé par
paire : ses handles sont partagés par rectangle et beaucoup d'ancres meurent
avant leur cover propre. Seule la consultation finale de l'histogramme est en
$O(1)$ après pré-calcul; W, secteurs et requête lisent encore une population.
Une paire tuée n'épargne donc qu'une fraction du travail aval, tandis que le
raffinement multiplie les produits émis par 1,66 : `rect_cover_handles` et
`corner_histograms` sont recalculés pour chaque sous-produit.

**Conséquence : `postsep_refine_levels` reste à 0 par défaut**, et le
mécanisme n'est pas activé en production. Il reste dans le code, gardé, comme
sujet mesurable — pas comme optimisation.

## 4 quater. Hypothèse candidate : réduire le travail des covers construits

Les mesures négatives du raffinement invitent à isoler un autre poste candidat.
`scanline_single_pass`, lane q4 :

| grandeur | 100 000 | 200 000 | exposant |
|---|---|---|---|
| ancres q4 | 48 557 755 | 191 710 560 | $n^{1{,}98}$ |
| **covers construits** | 5 436 957 | non mesuré | $n^{1{,}04}$ sur 32 k→100 k |
| grilles de cellules construites | 493 461 | 990 888 | **$n^{1{,}01}$** |
| ancres tuées par la grille | 330 745 | 687 851 | $n^{1{,}06}$ |
| **seeds tués par la grille, sans balayage** | 77 525 565 | 277 911 630 | $n^{1{,}84}$ |
| seeds tués par le cœur (balayage) | 109 531 539 | 491 912 062 | **$n^{2{,}17}$** |
| seeds tués par la corde | 41 529 741 | 171 517 481 | $n^{2{,}05}$ |

Trois lectures :

1. **Seuls 9,1 % des covers reçoivent une grille** (493 461 sur 5 436 957), et
   les covers eux-mêmes ne sont que **11 %** des ancres (5,4 M sur 48,6 M).
   Les 89 % restants évitent la construction du cover, mais peuvent encore
   payer W, secteurs ou requête : ils ne sont pas « gratuits ». La seule
   conclusion intégrée disponible est que la politique post-séparation mesurée
   au § 4 ter ralentit le mur total.
2. **Le nombre de covers est quasi linéaire sur la seule plage 32 k→100 k**
   ($n^{1{,}04}$), mais le travail par cover croît : 423 sites par cover à
   32 000, **986 à 100 000**, et les tests de cœur suivent $n^{2{,}40}$ sur
   cette plage. La taille des covers est donc un poste candidat à isoler. La
   projection conditionnelle à 200 k serait d'environ 11 M covers, mais elle
   n'est ni mesurée ni présente dans le reçu G4.
3. **La part attribuée à la grille parmi les trois classes de morts rapportées**
   `cellules + cœur + corde` passe de 33,9 % à 100 000 à 29,5 % à 200 000.
   Ce n'est pas une fraction de tous les seeds : les survivants amont ne sont
   pas dans ce dénominateur. Les kills de grille suivent $n^{1{,}84}$ contre
   $n^{2{,}17}$ pour le cœur ; sa politique (`cover >= 256 sites`,
   `seeds aigus × 8 >= cover`, `near_m < h`) est une hypothèse candidate à
   mesurer, pas encore le levier causal dominant.

**Balayage du seuil de grille** (`--cell-min-sites`), `scanline`
$n = 32\,000$, 8 fils, **digest identique partout** (`2bc14286…`) :

| seuil | 0 (forcé) | **32** | 64 | 128 | 256 (défaut) | 1024 |
|---|---|---|---|---|---|---|
| mur (ms) | 89 512 | **64 935** | 68 460 | 75 940 | 71 365 | 76 009 |

Le seuil 32 semblait donner 9,0 % de mur en moins que le défaut sur ce passage,
et le mode forcé 25 % de plus. Une comparaison locale de quatre runs par seuil,
annoncée comme entrelacée mais sans sorties brutes, ordre AB/BA, commande, pin
fonctionnel ni hash de binaire versionnés, ne reproduit pas ce signal :

| répétition | seuil 256 | seuil 32 |
|---|---|---|
| 1 | 68 266 | 76 353 |
| 2 | 78 842 | 76 724 |
| 3 | 59 762 | 63 674 |
| 4 | 69 688 | 43 096 |
| **moyenne** | 69 140 | 64 962 (− 6,0 %) |
| **médiane** | 68 977 | 70 014 (**+ 1,5 %**) |
| `(max−min)/moyenne` | 28 % | **52 %** |

Les quatre différences appariées sont de signes et d'amplitudes instables : le
protocole est **inconclusif** et ne résout pas un effet de 5 à 10 %. Il ne permet
ni d'attribuer le premier −9 % au bruit, ni d'estimer un seuil de détectabilité
à 20 %. Le +34,0 % post-séparation du § 4 ter reste lui aussi un signal à un run
par bras, cohérent avec les compteurs de travail mais non recertifié par ces
runs de seuil. Les deux compteurs appariés du § 0 restent exacts sur le cas
exercé ; aucune reproductibilité plus générale n'est déduite de ces murs.

### Piste géométrique corrigée : enveloppe entière q3/q4

En q3, le cover coefficient 3 est la plus petite **boule centrée en $m$** qui
contient l'enveloppe continue de toutes les circumboules admissibles : le centre
est à distance au plus $D/(2\sqrt{3})$ de $m$ et le rayon au plus
$D/\sqrt{3}$. L'équilatéral et le point extérieur aligné atteignent
$\sqrt{3}D/2$, ce qui prouve le serrage parmi ces boules englobantes. Cela ne
signifie ni que le cover est égal à l'union, ni que l'union finie des candidats
du nuage est un solide de révolution. Cette démonstration q3 ne s'étend pas au
cover historique q4 coefficient 3, qui est un sous-ensemble fail-open pour les
intérieurs et la coquille.

Le ratio précédemment annoncé
$\lvert\mathrm{cover}\rvert/\lvert P\cap W_q\rvert=0{,}48$ est retiré : pour
une même ancre et une même multiplicité, $W_q\subset W_2$ est déjà inclus dans
le cover coefficient 1, donc ce quotient doit être au moins un. Les valeurs
0,48 et 15,3 mélangent nécessairement des cohortes, unités ou sens de quotient ;
elles n'ont ni sortie brute ni reçu et ne chiffrent aucun gain.

L'existence d'une enveloppe entière plus serrée n'est en revanche pas ouverte.
Avec $d=b-a$, $D^{2}=\lVert d\rVert^{2}$, $w=2z-a-b$,
$S=\lVert w\rVert^{2}-D^{2}$ et $\Xi=\lVert d\times w\rVert^{2}$, l'enveloppe
continue q3 vérifie exactement $S\leq0$ ou $3S^{2}\leq4\Xi$. La borne de Jung
q4 fournit le sur-ensemble sûr $S\leq0$ ou $S^{2}\leq2\Xi$ ; l'intersecter avec
le cover q4 coefficient 3 ne l'élargit pas et peut retirer des sites incapables
d'appartenir à une boule admissible. Le plan de test, la borne de boîte
fail-open et la séparation q3/q4 sont détaillés dans
[`QUESTION_CLAUDE_EXPOSANTS_PAR_REGIME_20260828.md`](../audits/QUESTION_CLAUDE_EXPOSANTS_PAR_REGIME_20260828.md#enveloppe-entière-immédiate--ne-pas-attendre-larrangement).
Ce filtre n'a encore aucune mesure de mur ni preuve différentielle v5.

## 5. Ce qui n'est pas établi

- La série 8 000/32 000/100 000 du § 4 vient de la sonde. Les bras intégrés
  4 000/8 000/16 000 du § 4 bis sont tous négatifs en mur et ne constituent
  qu'un diagnostic local à un run. Aucune politique sélective n'est encore
  mesurée ni reçue.
- Rien au-delà de 100 000 pour le raffinement, or c'est entre 100 000 et
  200 000 que la fraction « avant » s'effondre à $n^{-0{,}02}$.
- La lane q3 n'est pas mesurée à grande échelle, ni `terrain`, ni
  `eight_clusters`.
- Le report « ancres $n^{1{,}34}$ ⟹ tests de cœur $n^{2{,}03}$ » est une
  **inférence** à coût par ancre constant, non une mesure.
- La série 100 000 et les masses d'étages ne possèdent pas encore de reçu brut
  versionné; elles restent diagnostiques jusqu'au pin, à la commande, au hash
  de binaire et à la sortie complète.
- L'identité de `candidats` et `tues_profondeur` au § 4 ter ne prouve ni une
  redondance structurelle avec les prétests d'ancre, ni l'identité des covers,
  seeds, tests de cœur et complétions; ces étages doivent être profilés
  directement sur les deux bras.
- La **route q2 doit rester interdite** : la fixture à six points exhibe une
  divergence de candidats et de `digest_balls` que le ledger de masse ne voit
  pas. Le mutant test-only et son CTest permanent sont désormais présents.
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
