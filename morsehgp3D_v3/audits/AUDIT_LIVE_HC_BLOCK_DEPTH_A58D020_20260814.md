# Contre-audit live de `HCBlockDepth`

Date : 14 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## 0. Snapshot et verdict

Le parent stable est
`HEAD=a58d0207d8e0482ced4b0207144fa311193c0388`. Le delta mobile audité ajoute
`HCBlockDepth` et `--hc` avec les empreintes SHA-256 suivantes :

- `prototype/midball_block.hpp` :
  `921e649f0ebbbfb7a8034bedaeeb0a14a2eaaadf9eadb092f4f8c3cdbfd9403b` ;
- `prototype/midball_probe.cpp` :
  `587ecc58ebdd592245449fef901be9d2fac3f4b9287752648554c48f2e7dcc49` ;
- `prototype/wspd_wavefront_probe.cpp` :
  `6330b79586066c575c59e4480a17fdf864fdef77b261a3a7f33c66bd68ed9c5b`.

Verdict : le certificat `ALL` est mathématiquement sûr sous ses préconditions
u16, et il réduit réellement le résiduel q2/q3/q4. L'intégration est toutefois
NO-GO en l'état : aucune CTest HC, une CTest Midball existante cassée, jusqu'à
trois recalculs identiques par tâche, composition `--hc --midball` refusée par
un faux plancher, ABI sans `UNKNOWN/preflight` et coût CPU diagnostique en
hausse. Ce cœur universel est un prune de support partiel, jamais la boule
unique d'un événement q3/q4 complet.

L'auditeur n'a modifié aucun logiciel. GCP non utilisé.

## 1. Théorème reçu

Pour `e=z-a`, `t=b-z`, poser :

```text
H = t dot e
C = t cross e
```

L'identité de Lagrange donne `||t||^2 ||e||^2 = H^2 + ||C||^2`. Les trois
lanes de témoin universel d'une ancre paire sont donc :

```text
q2 : H > 0
q3 : H > 0 et 3 H^2 > ||C||^2
q4 : H > 0 et 2 H^2 > ||C||^2
```

Le minimum de `H` sur `A×B×Z` est la somme de trois minima bilinéaires/concaves
aux extrémités : `hmin` est exact sur l'AABB continue. Pour chaque composante
`C_i=t_p e_q-t_q e_p`, les deux intervalles de produits puis leur différence
contiennent toutes les valeurs réalisables. Si `M_i` est le plus grand module
de cet intervalle, alors `sum M_i^2` majore `||C||^2`. Par conséquent :

```text
hmin > 0 et 2 hmin^2 > sum M_i^2  => ALL q4
hmin > 0 et 3 hmin^2 > sum M_i^2  => ALL q3
hmin > 0                            => ALL q2
```

La perte de corrélation ne produit que de faux `MIXED`, jamais de faux `ALL`.
Les inégalités sont strictes ; l'égalité reste shell. Sous u16,
`|C_i|<=2*65535^2<2^33` et la somme des carrés tient dans `i128`, à condition
de promouvoir avant le carré.

Cette propriété ne valide pas la cascade de rang. Pour un support minimal
positif complet q3/q4, sa miniboule canonique est unique et son census porte
sur cette boule. Ici, à partir de la seule paire `a,b`, HC prouve qu'un même
témoin est intérieur à **toutes** les boules admissibles de la lane. C'est une
condition suffisante plus forte, utile avant la génération des complétions.

## 2. Réception logicielle ouverte

Le header ne vérifie ni `0<=lo<=hi<=65535`, ni paire propre, ni identités et
n'expose aucun statut `INVALID/UNKNOWN`. Il réutilise `cone::kLaneNone`, qui
signifie ailleurs une réfutation ponctuelle, pour dire ici « le classifieur ne
conclut pas ». Un résultat typé séparé est requis afin qu'un futur consommateur
ne transforme pas un échec ou une entrée invalide en `NONE` exact.

Le commentaire de coût « une trentaine de multiplications » est faux au niveau
source. Un appel sain paie 24 produits pour `Hmin`, 24 produits pour les six
intervalles bilinéaires, trois carrés de composantes et un carré de `hmin`, soit
au moins 48 multiplications i64 et 4 multiplications i128, hors copies et
comparaisons. Le callsite se trouve dans la boucle des trois lanes et peut
refaire ce calcul trois fois pour la même tâche.

La bonne ABI calcule une seule fois par tâche un résultat du type :

```text
HCBounds { status, hmin, c2_upper, strongest_all_lane }
```

Le raccord calcule d'abord les trois verdicts centraux, appelle HC au plus une
fois si une lane reste non-`ALL`, puis diffuse `strongest_all_lane`. Si seule q2
est demandée, il s'arrête après `hmin`. HC subsume alors le `Midball ALL` q2 :
les deux flags partagent le résultat ou sont incompatibles, sans double calcul
ni plancher marginal obligatoire. Un second tier peut resserrer chaque
composante de `C` : elle est multi-affine dans les six coordonnées scalaires
concernées, donc ses extrema exacts sont à leurs 64 coins. Ce fallback reste
suffisant après sommation des maxima de composantes ; il ne remplace pas
SOC64/Corner512 lorsque les corrélations entre composantes sont décisives.

## 3. Portes et fautes observées

Le rejeu manuel suivant passe :

```text
selftest-hc=10000 seed=1 : accord=OUI, désaccords=0, q2/q3/q4=1068/96/235
hc-swap-coeff             : 55 désaccords, mutant tué, code 4
hc-correlation-ignoree    : 224 désaccords, mutant tué, code 4
```

Il n'existe pourtant aucune CTest HC. Pire, le changement du message de refus
casse une porte existante : `ctest -R '^mhgp3v_midball_'` affiche 12/13, avec
échec de `mhgp3v_midball_refus_sans_mode`, dont la regex attend l'ancien texte.
Le vert Midball stable ne peut donc pas être reporté sur ce worktree.

La composition est également fautive :

```bash
build/v3/mhgp3v_wspd_wavefront_probe --family=uniform --points=40 --sep-euclid=8/1 --tight --vwave --window=512 --window-ledger --hc --midball
```

HC consomme tous les gains q2 ; Midball publie `all=gains=0`, son plancher
inconditionnel rend le code `3`, alors que le calcul géométrique est sain. Les
planchers de non-vacuité doivent être des options explicites de campagne et les
compteurs remis à zéro par taille.

Portes minimales : préflight u16/boîtes/IDs, statut `UNKNOWN`, fixtures shell et
extrêmes u16, implication `HC_ALL => pire_point_ALL` sur petits produits,
mutants à code exact, CTest WSPD causale, parité des fates/pending, composition
Midball/SOC/BJD, puis test de coût. Le juge doit vérifier chaque promotion, pas
seulement une fermeture finale qui peut masquer une fausse unité.

## 4. Signal de performance

Sur `eight_clusters,n=1500,s=8,window=512`, mêmes rectangles :

```text
lectures       : 9 570 325 -> 8 383 723  (-12,40 %)
résiduel q2    :   106 809 ->    74 817  (-29,95 %)
résiduel q3    : 1 013 842 ->   868 803  (-14,31 %)
résiduel q4    : 1 071 162 ->   972 360  (-9,22 %)
fermetures     : 90 735/18 609/11 853 -> 93 193/30 720/22 425
pending/finale : 0/OUI dans les deux runs
```

Le signal logique est nettement meilleur que Midball seul. Il ne suffit pas à
recevoir le coût : trois runs alternés donnent une médiane CPU user
`0,825 s -> 1,210 s`, soit `+46,7 %`, sur la machine partagée. La priorité est
donc le calcul unique par tâche, les compteurs `appels/promotions par lane`, et
une ablation appariée à `200/600/1500/3000` sur uniforme et huit amas. HC agit
après la construction du front ; il ne ferme ni la source CK--WST, ni 0A/0B,
ni le payload. Aucun résultat 50 000/G4 ou SLO n'en découle.
