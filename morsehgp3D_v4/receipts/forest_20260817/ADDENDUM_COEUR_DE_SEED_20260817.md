# Addendum — cœur universel du seed par Jung (audit b8c4a4d, étape 2)

Date : 17 août 2026. Base de mesure : `48e4467` (sweep à deux côtés
livré) ; code livré dans le commit portant ce reçu. Audit exécuté :
`audits/AUDIT_CIBLE_A524020_AXIAL_ARBRE_ET_COEUR_DE_SEED_20260817.md`,
§ 1 (le § 2, top-k sur l'arbre, est l'étape suivante — voir « suite »).

## La règle exacte

Pour un seed aigu $(a,b,x)$ d'ancre owner $(a,b)$ : tout tétraèdre
ACCEPTÉ par la production (six arêtes $\leq D$, centre strictement
intérieur, donc circumboule = miniboule de diamètre $\sqrt{D}$) vérifie
par Jung $R^2 \leq 3D/8$ ; avec $R_\mu^2 = DEX/(4G) + \mu^2/(4G)$ et
$\vert n \vert^2 = G$, sa racine axiale vérifie

$2\mu^2 \leq J = D\,(3G - 2EX)$

(normalisation VÉRIFIÉE dans les unités du code : `f.g` $= DE-F^2 = G$
et $c_3 = a + W/(2G)$ est le circumcentre — recontrôlé par
$2(c-a)\cdot d = D$ et $2(c-a)\cdot u = E$). Un site $z$ tel que

$P(z) < 0 \quad\text{et}\quad 2\,P(z)^2 > J\,B(z)^2$

est STRICTEMENT intérieur à TOUTE sphère admissible du seed
($\Phi_\mu(z) \leq P + \sqrt{J/2}\,\vert B \vert < 0$) — de façon
équivalente : sa racine $\mu_z = P/B$ est HORS de la bande admissible
$[-\sqrt{J/2}, +\sqrt{J/2}]$. L'égalité $2P^2 = J B^2$ n'est PAS
comptée (le site peut être sur la sphère extrémale) ; $J < 0$ signifie
$R_3^2 > 3D/8$ : aucun tétraèdre admissible, le seed meurt sans témoin.
$h_4$ témoins tuent le seed ENTIER avant tout tableau `AxialSite`.

Le compte descend l'antichaîne de l'ancre (`handles`) avec tests par
boîte — NONE : $P_{min} \geq 0$ ; ALL : $P_{max} < 0$ et
$2P_{max}^2 > J\,B_{abs}^2$, crédit du poids du nœud en $O(1)$ — et
arrêt à $h_4$. Produits jusqu'à ~210 bits sous u16 : comparaison en
U320 (`cmp_2p2_jb2`, réutilise `mul_192_128_to_320`/`cmp_u320`), jamais
i128, conformément au § 1 de l'audit.

## Pourquoi le cœur est COMMUN aux deux chemins

Il est appliqué avant la bifurcation baseline/axial : la porte appariée
reste appariée par construction, et la CORRECTION de la règle est jugée
par les portes à juge indépendant des petits $n$ (97 CTest verts, juge
inclus). L'exactitude post-RLE suit l'argument déjà gravé pour
`depth_dead` et $W_4$ : une émission q4 supprimée appartient à un
groupe qui meurt de toute façon ($\geq h_4$ intérieurs si le label
$q_{min}$ est 4 ; sinon le groupe subsiste par son émission d'arité
inférieure, inchangé). Fail-open : les témoins hors de l'antichaîne
sont simplement omis.

## Le piège découvert par la fixture : le diamètre antipodal

Première version de la fixture-cœur : six points cocirculaires dont
`c1` à 36,87° — l'ANTIPODE de `a` (216,87°). Le plan du triple
$(a, c_1, y)$ contient alors le diamètre, donc l'axe et le centre de la
sphère : le circumcercle de $(a,c_1,y)$ est un GRAND cercle de la même
sphère, et la lane q3 émet la même clé en secours — le mutant n'était
plus discriminé (la clé revenait par l'arité 3). Règle gravée dans la
fixture : AUCUNE paire antipodale parmi les points cocirculaires. C'est
un fait de complétude inter-lanes, pas un défaut : la même boule peut
naître q3 par un grand cercle et q4 par un tétraèdre.

## Fixtures et portes (97 CTest verts, 8 mutants axiaux)

- **Fixture-cœur** (§ 4.2 de l'audit) : cercle $R^2 = 25$ du plan
  $z=10$, seed $(11,7,10),(20,10,10),(15,15,10)$, cocirculaires
  $(11,13,10),(18,14,10),(12,14,10)$ (points du faisceau $P=0$, $B=0$ :
  cas d'égalité $2P^2 = J B^2$), complétion $y=(15,10,16)$, sphère
  $R^2 = 3721/144$ dont les cocirculaires sont des points de coquille.
  Règle stricte : aucun témoin, clé émise à smax=6 par les deux
  chemins. Mutant `seed-core-nonstrict` (toutes les égalités comptées) :
  tout seed porté par le cercle meurt, l'exact-once ($pid(y)$ maximal)
  et le centre-hors-tétraèdre bloquent tout secours hors plan — la clé
  disparaît, code 4.
- **Fixture § 5 à deux variantes** : (0) le triplet gravé par le
  contre-audit 63d364a — $\vert\mu\vert = 1770 > \sqrt{J/2} \approx 433$,
  témoins UNIVERSELS : c'est désormais le cœur qui tue à smax=6
  (compteur `seeds_core`) ; (1) un triplet recalculé NON universel dans
  la bande admissible — $(19,14,9)$ et $(11,14,9)$ à $\mu = 400$,
  $(17,15,8)$ à $\mu = 320$, tous dans $(\mu_y = 240,\ 433]$ et
  strictement intérieurs ($1401/49 < 1513/49$) : le cœur ne compte
  rien et c'est la lecture bilatérale du sweep qui tue (compteur
  `morts_bilat`). Les deux variantes : clé absente à smax=6, présente
  au niveau exact $1513/49$ à smax=7, appariement post-RLE.
- Les six mutants du sweep re-tués à code 4 ; `kAxialVerify` actif sur
  tous les runs axiaux de la porte.

## Mesures (sorties IDENTIQUES aux reçus antérieurs, au compte près)

| run (s=8, smax=11, seed=3) | avant (2 côtés seuls) | avec cœur | événements |
|---|---|---|---|
| eight_clusters n=1000, axial | 87,5 s | **34,9 s** | 219 653 = |
| eight_clusters n=1000, baseline | 135,9 s (origine) | **35,4 s** | 219 653 = |
| uniform n=1600, baseline | 32,5 s | **27,0 s** | 532 181 = |
| uniform n=1600, axial | 34,9 s (+7 %) | **27,2 s** (parité) | 532 181 = |

Sur le cas dur : le cœur tue **3 994 641 seeds sur 4 416 744 (90,4 %)**
avant tout tableau axial — `t_gen` cumule **−74 %** contre la baseline
d'origine, et le chemin de production (baseline) en profite autant que
l'axial. Le régime clairsemé n'est PAS en régression : uniform gagne
−17 % et l'axial y revient à parité.

Décomposition du run axial eight_clusters (audit § 3) :
`t_core = 26,9 s` (562 269 006 nœuds visités, ~127 par seed),
`t_ab = 1,4 s`, `t_reduce = 1,3 s`, `t_emit = 0,15 s`. Le poste
dominant est désormais LA DESCENTE DU CŒUR elle-même — exactement le
poste que l'étape 3 de l'audit (top-k par branchement sur l'arbre,
bornes rationnelles de $\mu$ par nœud) doit attaquer, avec en ligne de
mire le partage du travail entre seeds d'une même ancre.

## Statut et suite

Exploration hors registre, `public_status=not_claimed`. Étape 3 (top-k
axial sur l'arbre : mutants `ratio-bound-wrong-sign` et
`tree-prune-boundary-ties`, comparaison flat/tree) : chantier suivant.
Cible : réduire les ~127 nœuds/seed et l'équivalent plat des sites.
