# Census : minimum entier précalculé par boule

`phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Cette optimisation locale conserve les extrêmes exacts utilisés par
`ball_depth_at_least` et `ball_census`. Elle ne prouve ni l'exactitude
horizontale globale, ni le contrat de performance de la tour K1..K10.
Elle suit la priorité mono-CPU ; les résultats GPU restent distincts.

## Proposition et preuve

Pour un axe de la forme $P(z)=a\sum_i z_i^2+\sum_i b_i z_i+c$,
poser $f(t)=at^2+bt$, $q=\lfloor\frac{-b}{2a}\rfloor$ et $r=-b-2aq$.
Comme $a>0$, on a $0\leq r<2a$ et $f(q+1)-f(q)=a-r$.
Donc $m=q+\mathbf{1}_{r>a}$ est un minimiseur entier global ; l'égalité
$r=a$ permet les deux voisins, et le code choisit $q$.

La différence discrète $f(t+1)-f(t)=a(2t+1)+b$ croît strictement : la
suite décroît avant ses minimiseurs puis croît après. Son minimum sur
tout intervalle entier non vide $[\ell,h]$ est donc atteint en
$\mathrm{clip}(m,\ell,h)$. Toutes les boîtes de l'index admis sont incluses
dans $[0,65535]^3$. Précalculer $m_0=\mathrm{clip}(m,0,65535)$ conserve
le même minimum, puisque
$\mathrm{clip}(m_0,\ell,h)=\mathrm{clip}(m,\ell,h)$.

`AxisBounds` conserve les trois $m_0$ par boule et par passe. Chaque boîte
demande alors une seule évaluation par axe pour son minimum, au lieu des
quatre candidats clippés précédents. Le maximum reste celui des deux
extrémités. La somme des minima/maxima par axe est exacte sur le produit
cartésien entier de la boîte ; aucune distribution des points n'est supposée.

Les frontières restent identiques : depth élague à `mn >= 0`, le census
complet seulement à `mn > 0`, et le range-add strict exige `mx < 0`.
La coquille n'est ni perdue ni assimilée à l'intérieur. Trois i64 locaux
s'ajoutent à la requête ; aucune structure géométrique globale n'est créée.

## Réconciliation des bornes de domaine

Le [tableau conservateur S1](QUALIFICATION_S1_PRIMITIVES.md#4-largeurs-entières--bornes-séparées-des-portes)
donne $0<a<2^{68}$, $|b_i|<2^{87}$ et $|c|<2^{105}$, plus largement que
les commentaires historiques de `keys.hpp` (86/104 bits).
L'optimisation et sa porte adoptent les bornes conservatrices, sans
dépendre d'un éventuel raffinement de ces commentaires.

En q3, $G<2^{68}$ et $|W_i|<2^{86}$ impliquent
$|B_i|=|2Ga_i+W_i|<2^{87}$ et
$|C|\leq G\sum_i a_i^2+\sum_i|W_i a_i|<2^{105}$.
Les formes q2/q4 sont plus petites ; la division par un PGCD positif
n'augmente aucun module. Sous ces bornes, $2a<2^{69}$ et
$|2aq|\leq |b_i|+2a<2^{88}$ : quotient, reste et négation sont sûrs en
i128. Le clip u16 PRÉCÈDE la conversion en i64 ; aucun polynôme n'est
évalué à un centre lointain. Pour une coordonnée u16,
$|at^2|<2^{100}$ et $|b_i t|<2^{103}$ ; chaque somme intermédiaire reste
sous la borne conservatrice $2^{107}$, donc dans i128.

Ces fonctions bas niveau gardent leurs préconditions. La porte refuse
ses fixtures hors domaine AVANT appel ; elle ne prétend pas qu'AxisBounds
valide un BallKey arbitraire occupant tout i128.

## Qualification locale et limites de mesure

Le [reçu conservé](../receipts/axis_bounds_overlay_20260904/README.md)
épingle code, oracle OBig, patches, sorties et hashes, sans binaires.
L'oracle énumère les positions entières par axe et des petits volumes 3D,
sans partager division ni règle d'arrondi avec le produit. Toute saturation
OBig interdit le verdict. Planchers observés : 1 212 boîtes, 454 697
positions d'axe, 31 720 points 3D, 45 requêtes depth, résultats à 106 bits,
dix rejets de fixtures. Quatre boîtes dépassent simultanément les anciens
plafonds B/C ; ce compteur est obligatoire. Census, coquilles et plafonds
sont aussi comparés sur une grille de 729 points.

Release, ASAN/UBSAN et détection des fuites passent sur l'overlay. Les cinq
mutants floor-only, ceil-always, no-clip, narrow-coefficient et max-min
divergent explicitement (code 4). Après intégration : six CTests ciblés
passent, dans un build neuf de la seule porte `mhgp7_axis_bounds_gate`.
Ce n'est pas une nouvelle exécution de toute la suite.

Le microbench synthétique historique, scalaire et à affinité CPU fixe,
observe à 64 boîtes par boule un ratio médian initial/argmin de 1,65 pour
min+max et 3,89 pour min seul. Le cache du seul plancher n'y apporte pas
de gain stable. Les [90 observations brutes](../receipts/axis_bounds_overlay_20260904/microbench.jsonl)
sont conservées avec leur provenance et des digests appariés. Ces chiffres
ne valent ni mesure end-to-end, ni p95 produit, ni prédiction à 50k.
La paire mono B/C et la qualification globale ont leurs propres reçus.

GCP non utilisé pour cette qualification.
