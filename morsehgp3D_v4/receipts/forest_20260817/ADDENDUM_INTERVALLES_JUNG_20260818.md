# Addendum — étage d'intervalles de Jung (2P² vs J·B² sans exact quand les intervalles se séparent)

Date : 18 août 2026. Base : `bd80652` (kernel affine). Exécute le § 1.2
de l'audit « filtre certifié et niveaux q3 » et le n° 4 de l'« ordre
conseillé » du contre-audit `5d274a1` (« l'intervalle certifié de
2P²−JB² ; c'est le gain arithmétique immédiatement mesuré »), dans la
forme prescrite par le contre-audit `04c71a2` § 6 : « jamais le seuil
de P mis au carré — intervalle sortant sur [P], puis propagation ».

## La règle implémentée (`jung_interval_sign`, partagée lanes/portes)

Préconditions : $L < 0$ certifié par l'étage affine
($\hat{L} < -E$, donc $P \in \left[ (\hat{L}-E)/4, (\hat{L}+E)/4 \right]$
avec borne supérieure négative) et $J \geq 0$ ($J < 0$ a déjà tué le
seed sans témoin). Avec $P_u = (\hat{L}+E)/4$, $P_l = (\hat{L}-E)/4$ et
le facteur de garde $\delta = 2^{-40}$ :

- $2 P_u^2 (1-\delta) > J(1+\delta) \cdot B^2(1+\delta)$
  $\Rightarrow 2P^2 > J B^2$ STRICT : témoin certifié, compté sous la
  règle stricte ET sous le mutant nonstrict — sans i128 ni U320 ;
- $2 P_l^2 (1+\delta) < J(1-\delta) \cdot B^2(1-\delta)$
  $\Rightarrow 2P^2 < J B^2$ : non-témoin certifié, exclu sous les deux
  règles — sans i128 ni U320 ;
- sinon repli exact : $P = L/4$ (affine i128) puis `cmp_2p2_jb2`
  (U320). Les ÉGALITÉS tombent toujours dans le repli — la sémantique
  du mutant `seed-core-nonstrict` vit dans l'exact, jamais dans
  l'intervalle.

## Dérivation de la garde

Toutes les erreurs sont RELATIVES — aucune annulation catastrophique :
$\hat{L} \pm E$ garde le signe ($\hat{L} < -E$), l'addition IEEE a une
erreur relative $\leq u$ même près de zéro, et tous les produits
composent des termes de même signe. Par côté : conversions de $J$
(i128, $\leq u$) et de $B$ (i64, $\vert B\vert < 2^{54}$, $\leq u$),
$(\hat{L} \pm E)$ ($u$), mise au carré ($u$ ; $2\times$ et $/4$
exacts), deux produits ($2u$) — moins de $8u$ par côté. La garde
$\delta = 2^{-40} = 2^{13} u$ les absorbe avec un facteur $> 1000$.
Débordement impossible : $J B^2 < 2^{109} \cdot 2^{108} = 2^{217}$,
$2P^2 < 2^{207}$, loin de $2^{1024}$. Un site séparé n'exécute que :
$B$ (3 multiplications i64), 6 opérations double, 2 comparaisons —
contre 4 multiplications i128 + un `cmp_2p2_jb2` U320 auparavant.

## Portes (120 CTest verts)

- `--float-gate` étendue : sous `kFloatVerify` chaque décision
  d'intervalle certifiée est recoupée par l'exact complet
  (`P = L/4` + `cmp_2p2_jb2`) — jung=4 137 414 kills certifiés /
  3 280 075 skips certifiés / 3 replis, **0 désaccord** ; planchers des
  DEUX certifications ajoutés à la porte.
- `--q3-affine-gate` étendue : trois témoins GRAVÉS de la primitive
  ($\hat{L} = -2^{60}$, $E = 2^{55}$, fenêtre
  $2P^2 \in \left[ \sim 2^{116{,}91}, \sim 2^{117{,}09} \right]$) :
  kill franc ($J B^2 = 2^{80}$ → +1), skip franc ($2^{130}$ → −1), et
  À CHEVAL ($J = 2^{59}$, $B = 2^{29}$, $J B^2 = 2^{117}$ dans la
  fenêtre → 0, repli OBLIGATOIRE). Piège rencontré et gravé : la
  première fixture visait $2^{118}$ — HORS fenêtre (erreur de calcul
  mental $2 P_u^2 \sim 2^{117}$, pas $2^{118}$) ; la porte l'a refusée
  (`jung_temoin_faux=1`) avant tout banc, exactement son travail.
- MUTANT `jung-swap-bounds` (le kill teste $2 P_l^2$, le mauvais bout
  de l'intervalle) : le témoin à cheval rend +1 au lieu du repli → tué
  (code 4). Les mutants existants (`float-threshold-too-small`,
  `seed-core-nonstrict`) restent tués — le second par le chemin exact
  du repli, seul endroit où sa sémantique agit.

## Mesures (n=8000, smax=11, 4 fils — sorties IDENTIQUES : mêmes événements, mêmes cardinalités)

| famille | avant (bd80652) | avec intervalles | jung kill/skip/repli |
|---|---|---|---|
| eight_clusters t_gen | 127,0 s | **123,1 s** | 672,8 M / 675,2 M / **80** |
| uniform t_gen | 57,3 s | **55,5 s** | 162,4 M / 90,8 M / **145** |

Le cœur de seed passe de 64,4 à 48,2 s CPU (eight_clusters) et de 11,7
à 9,1 s (uniform) : les ~1,35 G de `cmp_2p2_jb2` U320 du cœur q4 sont
réduits à 80/145 replis exacts — l'arithmétique du cœur est désormais
presque entièrement flottante-certifiée, régulière et portable GPU. La
régression affine du reçu précédent est effacée (retour à la baseline
sur eight_clusters, légèrement mieux sur uniform) ; le poste dominant
de t_gen est maintenant le scan de profondeur q3 (structurel — c'est
la cible des couches convexes différées, pas d'une constante).

## Ce qui reste (audit § 1.3 / § 5)

`cmp_mu` (ordre axial, borne propre $\sim 2^{114}$ sur le déterminant
$A_1 B_2 - A_2 B_1$) — chemin axial opt-in seulement, différé avec lui.
Le schéma L/U à deux bornes (§ 5) reste le cadre du port GPU (tâche
#26).
