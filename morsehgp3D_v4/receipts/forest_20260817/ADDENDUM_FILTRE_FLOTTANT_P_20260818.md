# Addendum — étage flottant certifié du signe de P, à borne DYNAMIQUE par forme

Date : 18 août 2026. Base : `e60a5a1` (fold compact) ; code livré dans
le commit portant ce reçu. Exécute le § 1.1 (et § 1.4) de la réponse
d'audit `REPONSE_CLAUDE_E573_FILTRE_FLOTTANT_ET_Q3_DEMI_PLANS`, avec
un raffinement dont l'audit voudra vérifier la dérivation.

## La règle implémentée

`q3_power_float_sign` : $S = \vert v\vert^2$ calculé en ENTIER (exact
en binaire64), $G$ et $W$ convertis UNE FOIS par forme, séquence FIGÉE
en `fma`, round-to-nearest, jamais de fast-math. Décision :
$\hat{P} < -E_f \Rightarrow P < 0$ certifié ;
$\hat{P} > +E_f \Rightarrow P > 0$ certifié ; sinon repli `q3_power`
exact. Utilisée par le scan de profondeur q3 (le signe suffit, les
deux côtés gagnent) et par la garde $P > 0$ du cœur de seed (le côté
négatif recalcule l'exact, requis par Jung — son étage d'intervalles
§ 1.2 est le chantier suivant). Jung et `cmp_mu` n'utilisent PAS cette
borne, conformément à l'audit.

## Le raffinement : borne dynamique par forme

La borne absolue $2^{58}$ proposée est l'instance du profil u16 PLEIN.
Or les familles du banc vivent à densité constante (coordonnées
$\sim n^{1/3}$, soit ~200-400 à n=8000) : $\vert P\vert \sim 2^{48}$,
et la borne absolue ne certifiait RIEN (mesuré : 92 certifications
pour 25 M de replis). Toutes les erreurs de la séquence étant
RELATIVES aux grandeurs (conversions = ulp, arrondis = ulp), la borne
correcte est par forme :

$E_f = 2^{-48} \cdot (G_d \cdot S_{max} + \Vert W \Vert_1 \cdot v_{max})$

avec $S_{max} = 2 D^2$ (cover coef 3 : $\vert z-m\vert^2 \leq 0{,}75
D^2$ et $\vert a-m\vert = D/2$ donnent $\vert z-a\vert^2 < 2D^2$) et
$v_{max} = \sqrt{S_{max}}+1$. Dérivation : ~10 termes d'erreur, chacun
$\leq 2^{-53}$ × la grandeur du terme, soit $\leq 2^{-49{,}7} \cdot
(G S + \Sigma\vert W_i v_i\vert)$ ; le facteur $2^{-48}$ laisse ×3 de
marge et absorbe les arrondis du calcul de $E_f$ lui-même. La borne
$2^{58}$ de l'audit s'en déduit à $G \sim 2^{68}$, $S \sim 2^{36}$.
CETTE DÉRIVATION EST SOUMISE À CONTRE-AUDIT — le mécanisme, lui, est
protégé par la réception (chaque signe certifié recoupé).

## Piège rencontré et gravé : l'échelle puissance de 2

La première fixture du mutant multipliait les points cocirculaires par
2048 = 2^11 : les mantisses restent EXACTES en binaire64, $\hat{P} =
0{,}0$ exactement, et le mutant survivait. L'échelle est passée à
×1999 : les conversions de $G \sim 2^{56}$ et $W \sim 2^{67}$
deviennent inexactes, le bruit des sites $P = 0$ est réel et reste
sous $E_f$ (repli obligatoire) — le mutant (borne $/2^{20}$) les
certifie avec le signe du bruit contre un exact NUL : tué à code 4.

## Portes (113 CTest verts)

`--float-gate` : sous `kFloatVerify`, CHAQUE signe certifié est
recoupé par l'exact — 24,9 M de signes certifiés, 209 503 replis
(0,8 %), **0 désaccord** ; planchers des deux signes et du repli
(fixture cocirculaire ×1999). Mutant `float-threshold-too-small` : 4.
Compteurs publiés par le probe (`flottant=neg/pos/replis`).

## Mesures (n=8000, smax=11, 4 fils — sorties IDENTIQUES au compte près)

| famille | t_gen avant | t_gen avec filtre | signes certifiés | replis |
|---|---|---|---|---|
| eight_clusters | 142,6 s | **123,2 s** (−14 %) | 11,43 G | 7,1 M (0,06 %) |
| uniform | 56,9-62 s | **56,0 s** | 0,89 G | 6,3 M |

Le gain est encore BORNÉ par le cœur de seed : pour chaque site à
$P < 0$ certifié (8,5 G sur eight_clusters), l'exact est recalculé car
Jung ($2P^2$ vs $JB^2$) l'exige. Le chantier suivant est l'étage
d'INTERVALLES de Jung (audit § 1.2 : certifier
$\mathrm{lower}(2P^2) > \mathrm{upper}(JB^2)$ en flottant, repli
`cmp_2p2_jb2`) — c'est lui qui convertira les 8,5 G d'évaluations
i128 restantes ; puis `cmp_mu` (§ 1.3, borne $2^{114}$ propre), et
l'index q3 par couches convexes (§ 2) qui change la COMPLEXITÉ là où
le flottant ne change que la constante.
