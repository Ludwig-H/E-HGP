# Sonde audit-only : certificat spatial par blocs q3/q4 sur S2

Portée : le seul quartier sans sol `08/000200`, 1 288 sites u18/1 mm,
K10/s8, et les 55 657 survivants S2 du
[`b_s2_trace_20260924`](../b_s2_trace_20260924/README.md). Cette note est
un plan, pas une mesure ni une qualification du moteur.

## Raccord minimal

Créer un exécutable autonome dans ce répertoire, lié à `mhgp9_gen`, qui lit
les deux fichiers d'entrée épinglés par la trace et
`quarter_1288.trace.tsv`. Vérifier les SHA du `RESULT.json`, l'effectif
1 288, les 55 657 ordinals contigus, les IDs locaux/bruts et les masques
avant tout calcul. Reprendre seulement le lecteur d'entrée et
`make_q2_cloud_index(prepare_cloud(points))` de
[`trace.cpp`](../b_s2_trace_20260924/trace.cpp#L32-L95), sans reconstruire
front/S2/S3. `Q2CensusIndex::cloud().points()`, `spatial_order()`,
`spatial_points()` et `spatial_nodes()` sont publics ; un nœud fournit
`range`, `box`, `left`, `right`, `escape` ([`q2_census.hpp`](../../src/gen/pipeline/q2_census.hpp#L88-L135)).
Le constructeur certifie les boîtes exactes, les plages filles disjointes
et l'ordre préfixe/échappement ([`q2_census.cpp`](../../src/gen/pipeline/q2_census.cpp#L112-L170)).
Les colonnes `local_a/local_b` de la trace sont des IDs de points, tandis
que les plages de nœuds sont des rangs spatiaux : construire l'inverse de
`spatial_order()` une fois et vérifier cette distinction. La nouvelle preuve
parcourt le BVH du **nuage entier**, avant tout cover diamétral ; `F_e` ne
sert qu'à la comparaison de travail. Compter davantage de témoins stricts
que le core reste valide, car chaque témoin appartient au nuage d'entrée.

Pour chaque arête sélectionnée, reconstruire seulement `v=b-a`,
`m_2=a+b`, `D=|v|²` et la base entière `A,B` avec la même règle d'axe
dominant que [`Q34DeadLaneProver::load`](../../src/gen/lanes/q34_dead_lanes.cpp#L26-L50).
Ne pas appeler `load()` dans la nouvelle voie : il construit une forme par
site du cover ([`q34_dead_lanes.cpp`](../../src/gen/lanes/q34_dead_lanes.cpp#L51-L79)).
Le prover public reste le témoin de comparaison historique, déjà rejoué
dans [`trace.cpp`](../b_s2_trace_20260924/trace.cpp#L147-L192) avec
`Q34EdgeCover::make_diametral`, `load`, `prove(10, s2_mask)` et `F_e`.
Un rejeu ponctuel de ce témoin peut contrôler le chargement des entrées ;
il doit être chronométré séparément.

## Test exact et comptage

Pour une cellule fermée `C=[alpha0,alpha1]×[beta0,beta1]`, les bornes de
`alpha,beta` sont des entiers à l'échelle `s=2^20`. Pour un site `z`, poser
`w=2z-m_2`, `p=alpha A+beta B` et

`H(z,alpha,beta)=s(|w|²-D)-2 w·p`.

`H<0` signifie strictement intérieur ; `H=0` est la coquille et n'est
jamais crédité. Sur la boîte fermée `Z` d'un nœud, le maximum de `H` est
atteint parmi **8 coins de Z × 4 coins de C** : pour un centre fixé,
`H` est convexe en `z`, puis pour un `z` fixé il est affine en
`(alpha,beta)`. Calculer les 32 valeurs en `i128` après promotion ;
`max H<0` crédite d'un coup `node.range.size()` sites, sans charger ni
construire leurs formes. Le domaine u18 et `|alpha|,|beta|<=2^21` restent
dans `i128` (la forme ponctuelle actuelle tient même en `i64` :
[`q34_dead_lanes.hpp`](../../src/gen/lanes/q34_dead_lanes.hpp#L22-L32)).
Un nœud contenant l'un des deux endpoints ne peut satisfaire `max H<0`
car l'endpoint a `H=0` pour tout centre ; l'inverse des rangs permet de
l'asserter dans le sidecar.

Par cellule, garder une antichaîne de nœuds représentant des plages de
rangs disjointes. Si un nœud est crédité, retirer sa plage de la frontière
et transmettre son effectif hérité à **chaque** cellule fille ; ne jamais
additionner les comptes des quatre filles. Un nœud ambigu se divise en ses
deux fils ; une feuille ambiguë reste en frontière. Si le seuil est atteint,
cesser l'examen de cette cellule. Seuils à K10 : q3 `T3=9`, q4 `T4=8`,
traités selon les bits demandés par S2 ; les règles générales `K−1`, `K−2`
et les seuils nuls sont celles de [`prove`](../../src/gen/lanes/q34_dead_lanes.cpp#L89-L105).
Copier dans le sidecar les disques q3/q4 et le test de disjonction de cellule
([`q34_dead_lanes.cpp`](../../src/gen/lanes/q34_dead_lanes.cpp#L108-L130)),
avec racine `[-2s,2s]²`, profondeur min 2/max 6 comme le témoin ; avant
la profondeur 2, transmettre la racine BVH sans la classer. Une voie
n'est prouvée que si toute cellule rencontrant son disque est saturée ou
si tous ses descendants le sont. À profondeur max, l'incertitude laisse
la voie ouverte.

Pour limiter la frontière sans faux rejet, on peut aussi écarter un nœud
si le minorant `s*(Σ min w_i²-D)-2Σ max(w_i p_i)` est `>=0`, où chaque
`w_i` et `p_i` parcourt son intervalle sur `Z×C`. Les extrema de produit
sont ceux des quatre paires d'extrémités ; toute arithmétique est `i128`.
Ce minorant peut être faible mais son signe exclut bien tout intérieur
strict sur la cellule et ses filles. L'échec du test 32 coins ne permet
à lui seul aucun retrait.

Arrêt anticipé facultatif : au coin d'une cellule situé dans le disque de
la voie, compter la profondeur exacte par BVH, avec bornes puis feuilles,
ou obtenir un **majorant** du nombre possible d'intérieurs inférieur à T.
Pour un centre fixé, `sH=Σ(s w_i-p_i)²-|p|²-s²D` donne le minorant exact
continu d'une boîte en clampant chaque intervalle `s w_i-p_i` à zéro.
Une boîte dont ce minorant est `>=0` ne contient aucun intérieur ; les
boîtes restantes contribuent au majorant par leur population ou sont
raffinées. Un majorant `<T` réfute la preuve de voie. Un compte inférieur
partiel, une visite interrompue ou 32 coins seuls ne la réfutent jamais.

## Comparaison et bornes de travail

Publier pour chaque ordinal visité un masque prouvé **sous-ensemble de
`s2_mask`**, avec coûts séparés : cellules, nœuds/boîtes testés, tests
32 coins, évaluations de coins, feuilles, raffinement, copies de frontière,
temps CPU et mur, arrêts par budget. Les arêtes inachevées émettent masque
zéro. Le baseline par arête est `core_proved=s2_mask & ~post_core_mask`, et
sa masse `F_e` est la colonne `F` du TSV ([`trace.cpp`](../b_s2_trace_20260924/trace.cpp#L194-L202)).
Tabuler q3/q4 séparément et les quatre classes `bloc∩core`, `bloc∖core`,
`core∖bloc`, aucun, plus les arêtes dont **tous** les bits S2 sont
fermés. Écrire le TSV clairsemé `s2_ordinal<TAB>proved_mask` pour
[`join_shadow.py`](../b_s2_trace_20260924/join_shadow.py#L41-L96) : seul
`proved_mask==s2_mask` rend `F_e` éligible à éviter le core entier.
Les masses q3/q4 se chevauchent ; publier union et intersection et contrôler
`ΣF=1 151 766`, `core_closed_F=610 738` sur le trace complet
([`RESULT.json`](../b_s2_trace_20260924/RESULT.json)). Un `bloc∖core`
est une preuve candidate supplémentaire, puisque le core est incomplet ;
il exige les assertions géométriques et des vérifications directes sur
les petits nœuds, pas une égalité forcée au baseline.

Premier passage borné : 256 ordinals déterministes mêlant des strates de
`F_e`, masques S2 et succès/échecs core ; cap global de `10^6` tests
nœud-cellule, 50 000 cellules et 30 s mur. Vérifier par énumération directe
les sites réels de chaque nœud crédité dans un sous-échantillon fixé, aux
quatre coins de la cellule ; refuser la sonde à la première discordance.
Publier le nombre d'arêtes complètement examinées et celles interrompues.
Une extension aux 55 657 arêtes dépend de ces coûts observés ; extrapoler
les masses d'un échantillon ou `F_e` en gain de temps serait injustifié.
Le risque principal est le parcours répété de l'index entier : à 1 288
sites, il compte 2 575 nœuds, contre seulement 1 151 766 sites cumulés
dans tous les cores de la trace. La profondeur 6 permet au maximum 5 461
cellules par arête ; une énumération naïve nœuds × cellules dépasse 14
millions de tests pour **une** arête. Mesurer les nœuds crédités entiers,
les nœuds écartés, les feuilles et les arrêts anticipés avant tout jugement
de coût total. Un score de preuves seul ne justifie pas ce chemin.
