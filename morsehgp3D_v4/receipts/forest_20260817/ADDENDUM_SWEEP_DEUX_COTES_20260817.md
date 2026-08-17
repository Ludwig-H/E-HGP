# Addendum — sweep axial à DEUX CÔTÉS (contre-audit 63d364a)

Date : 17 août 2026. Base de mesure : `a524020` ; code livré dans le
commit portant ce reçu. Chemin concerné : sélection axiale q4 opt-in
(`--axial-on`) de `src/pipeline/ball_stream.hpp` — la production CPU
reste la baseline énumérée, conformément au reçu « axial borné ».

## Ce qui est implémenté

L'identité du contre-audit, prise comme unique lecture de profondeur du
chemin axial : pour une racine $\mu$ de l'axe d'un seed $(a,b,x)$, avec
$p$ permanents ($B=0$, $A<0$),

$d_{cover}(\mu) = p + \#\left\lbrace z : B_z > 0,\ \mu_z < \mu \right\rbrace + \#\left\lbrace z : B_z < 0,\ \mu_z > \mu \right\rbrace$

et $d_{cover}(\mu)$ est EXACTEMENT le compte du scan `q4_power < 0` sur
le cover. Concrètement :

- **Fenêtre viable** $[L, U]$ : $U$ = $k$-ième plus petite $\mu$
  positive, $L$ = $k$-ième plus grande négative (multiplicités
  comprises ; côté à moins de $k$ racines → seuil infini),
  $k = h_4 - p$. Ties de frontière INCLUS dans la fenêtre.
- **Table UNIQUE de groupes de $\mu$ exacts fusionnant les deux
  signes** (comparaison `cmp_mu_same_side` après normalisation
  $(-A,-B)$ du côté négatif, qui laisse $\mu$ inchangée) : une sphère à
  coquille bilatérale n'est plus émise deux fois par le même seed —
  les émissions brutes q4 passent de 87 048 à 87 043 sur
  eight_clusters n=1000, sorties finales identiques.
- **Préfixes positifs / suffixes négatifs** sur la table triée, plus
  les masses hors fenêtre `pos_lt_L` / `neg_gt_U` : $d_j$ par groupe en
  $O(1)$.
- **Mort AVANT `valid_completion`/`q4_form`** : un groupe à
  $d_j \geq h_4$ ne paie plus aucune complétion. Le scan `depth_dead`
  par groupe du chemin axial est SUPPRIMÉ ; il est remplacé par
  l'assertion de réception `kAxialVerify` (voir portes).
- **Morts par le côté opposé aux bornes de la fenêtre** : une racine
  positive sous $L$ a $\geq k$ négatifs strictement au-dessus d'elle,
  une négative sur $U$ a $\geq k$ positifs strictement en dessous —
  $d \geq h_4$ exact dans les deux cas, compté dans `morts_bilat`. Les
  exclusions par leur PROPRE côté (positive sur $U$, négative sous $L$)
  ne comptent pas : elles existaient déjà dans le sweep unilatéral.

## Ce que la fixture § 5 a corrigé en chemin

Première version : le compteur bilatéral ne comptait que l'étage $d_j$
des groupes en fenêtre. La fixture l'a immédiatement montré vide à
smax=6 : la racine du complèteur y est exclue par le seuil $L$ — issu
de l'ordre NÉGATIF alors qu'elle est positive — donc la mort
bilatérale se produit à la classification, pas à l'étage $d_j$. Le
compteur couvre maintenant les deux étages ; c'est exactement le genre
de faute de comptabilité (pas de sémantique : les émissions étaient
déjà justes) que la fixture entière sert à graver.

## Fixture gravée (contre-audit § 5, coordonnées exactes)

$a=(10,10,10)$, $b=(20,10,10)$, $x=(15,17,10)$, $y=(15,10,17)$,
$z_1=(15,13,9)$, $z_2=(14,13,9)$, $z_3=(16,13,9)$. Sphère circonscrite
de $\lbrace a,b,x,y \rbrace$ : centre $(15, 82/7, 82/7)$, niveau
$R^2 = 1513/49$. Les trois $z_i$ en sont strictement intérieurs
($442/49$, $491/49$, $491/49 < 1513/49$), hors du cœur $W_4$ de
l'ancre ($2H^2 = 450 < \Xi = 1000$ : le filtre d'ancre ne les voit
pas), et TOUS du côté $B<0$ de l'axe du seed $(a,b,x)$ (plan $z=10$,
$B_{z_i} = -1$) quand le complèteur $y$ est du côté $B=7>0$ : la mort
ne se lit QUE sur le côté opposé. Vérifié dans la porte appariée :

- smax=6 ($h_4=3$) : la clé `q3_ball_key_reduce(q4_ball_form(q4_form(a,b,x,y)))`
  est ABSENTE des émissions brutes axiales, `morts_bilat >= 1`,
  baseline/axial identiques post-RLE.
- smax=7 ($h_4=4$) : la clé est PRÉSENTE au niveau sémantiquement égal
  à `q4_level_raw` ($1513/49$), baseline/axial identiques post-RLE.

## Portes (96 CTest verts, 6 mutants axiaux tués)

- La porte appariée `--axial-pair-gate` court désormais TOUS ses runs
  axiaux sous `kAxialVerify` : sur chaque groupe émis, $d_j$ est
  recoupé par le scan complet `q4_power < 0` du cover — tout désaccord
  est une violation (assertion de réception du contre-audit : égalité
  du COMPTE, pas du seul verdict).
- Mutants (code de sortie exact 4) : `axial-short-group`,
  `axial-drop-ties`, `axial-first-rep` (existants, re-tués),
  `axial-ignore-opposite-side` ($d_j$ sans le côté opposé — laisse
  survivre la sphère de la fixture à smax=6), `axial-reverse-negative`
  (suffixe négatif remplacé par le préfixe — même survie indue),
  `axial-depth-nonstrict` (le groupe compte sa propre coquille — tue à
  tort la sphère à smax=7).
- Le mutant `first-rep` reste discriminé PRÉ-RLE (sphère cosphérique
  R²=50), les planchers de réduction et l'équivariance de la porte
  sont inchangés.

## Mesure (eight_clusters n=1000, s=8, smax=11, seed=3 — le cas dur)

| chemin | t_gen | évaluations q4 tuées au scan |
|---|---|---|
| baseline énumérée | 135,9 s | 119 653 085 |
| axial par côté (reçu campagne) | 96,4 s | 285 028 |
| axial DEUX CÔTÉS (ce reçu) | 87,5 s | 0 (plus aucun scan de mort) |

Soit −9 % de plus sur `t_gen` (−36 % cumulés contre la baseline), avec
`morts_bilat = 877 737 092` racines tuées par le côté opposé sans
qu'aucune ne paie de scan ni de complétion. Sorties IDENTIQUES sur
toute la ligne : 219 653 événements, 225 357 boules uniques, mêmes
census/fusions/nœuds. Le poste dominant restant est le balayage
$A,B$ des 4 416 744 seeds (calcul de `q3_power` par site de cover) —
c'est la borne naturelle du gain de cette étape sur CPU ; c'est aussi
le noyau régulier que le contre-audit destinait au port GPU.

## Statut

Exploration hors registre, `public_status=not_claimed` ; le chemin
axial reste opt-in, la production CPU reste la baseline. La campagne
G4 (couverture) décidera de la promotion éventuelle, eight_clusters
smax=11 restant un run À RISQUE.
