# Profil de séparation : pourquoi $s \geq 8$, et ce que cela invalide

Document durable, écrit le 30 août 2026 au pin `f83fd184`. Il grave une
contrainte de domaine et **retire nommément** les mesures que j'ai publiées en
dehors d'elle.

## La contrainte

Le certificat de cœur d'un rectangle WSPD repose sur le **citron commun**,
l'intersection des fuseaux $W_q(a,b)$ sur $\mathrm{Box}(A)\times\mathrm{Box}(B)$.
Sous la condition de séparation $d_{\min}\geq s\max(r_A,r_B)$, sa demi-largeur
vaut

$$\left(\kappa_q-\frac{2}{s}\right)d_{\min},\qquad \kappa_2=\frac{1}{2},\quad \kappa_3=\frac{1}{2\sqrt{3}}=0{,}2887,\quad \kappa_4=\sin 15^\circ=0{,}2588.$$

Elle s'annule à $s=4$ pour q2, $s=6{,}93$ pour q3 et $s=7{,}73$ pour q4.

**Huit est le plus petit entier qui rend le certificat de cœur non dégénéré pour
les trois lanes.** En dessous, `core` est vide **par construction** : toute
statistique mesurée là décrit un pipeline amputé de son crédit principal, et
n'est pas un point de fonctionnement.

## Ce que cela invalide, nommément

Les mesures suivantes, que j'ai publiées, sont **hors profil et retirées** :

| note | ce qui est retiré |
|---|---|
| V144 | tableau « $s$ gouverne les deux crédits en sens opposés », lignes $s=2$, $s=4$, $s=6$ — dont $78{,}5\,\%$ de masse tuée sur `terrain` et $71{,}2\,\%$ sur `eight_clusters` |
| V144 | la phrase « le gain des extrémités écrase la perte du cœur », qui reposait entièrement sur ces lignes |
| V146 | la ligne `eight_clusters` $s=2$ du tableau de la forme résiduelle |
| V156 | les chiffres $s=2$ et $s=4$ relayés du contrôle adversarial : $33{,}574\,\%$, $42{,}005\,\%$, et les ratios $0{,}376$ et $0{,}225$ |

**Survit** tout ce qui est mesuré à $s\geq 8$ : la décomposition des trois crédits
à $s=8$, le fait que $100\,\%$ des morts d'ancre exigent $h_a+h_b$, le marginal
du cœur pendant la descente ($80{,}9$ à $99{,}8\,\%$ selon la cohorte), et le
rendement marginal des extrémités ($\times 18{,}69$ contre $\times 15{,}98$ pour
le cœur, `eight_clusters` $n=800$, $s=8$).

## Conséquence de conception, à $s=8$

Les crédits d'extrémité sont **plafonnés** par $\lvert A\rvert$ et
$\lvert B\rvert$ — mesurés entre $1{,}6$ et $4{,}2$ selon la cohorte, puisque
$h_a(a)\leq\lvert A\rvert-1$. Ils restent nécessaires à chaque mort d'ancre et
rendent davantage par unité de coût que le cœur sur `eight_clusters`, mais
**on ne peut pas les renforcer en abaissant $s$** : c'est une contrainte de
domaine, pas un réglage.

## Où la contrainte est appliquée

| point | comportement |
|---|---|
| `run_pipeline` | refus `invalid_input : separation s < 8` |
| CLI `mhgp5` | code 2, `REFUS : arguments (profil s >= 8, …)` |
| `alive_rectangles` | **fail-closed** : zéro rectangle si $s<8$, sauf opt-in explicite |
| `GenerateOptions` | l'opt-in n'existe **que** sous `MHGP5_TESTING` |

Le trou par lequel mes chiffres hors profil sont sortis était le troisième :
mes sondes appellent `alive_rectangles` directement et contournaient ainsi le
refus du CLI. Il est fermé, et deux portes CTest gravent le contrat —
`--s=7` doit rendre **code 2**, `--s=8` doit rendre **code 0**.

## Note de méthode

Trois contre-fixtures test-only travaillent légitimement sous le profil
(`postsep_refine_gate` à $s=4$ et $s=1$, entre autres) : elles ne mesurent que
des grands-livres, jamais un taux de coupe. Leur sortie du profil est désormais
**déclarée** dans leur source au lieu d'être silencieuse.
