# Deux triangles : géométrie source, dates et frontières

Reçu borné du 30 septembre 2026. [check.py](check.py) vérifie les formules dans
`Q(√3)` et lit deux exports natifs déjà existants. Aucun appel moteur,
compilation, campagne, GCP ou modification du produit. Les contrôles restent
actifs sous `python -O`.

Source primaire : [thèse de Hauseux](../../../../../docs/references/MANUSCRIT_THESE_HAUSEUX.pdf),
SHA256 `579f83671ebca34cd810f350820074eb42672411713160f9c9c2a458ff4f4fef`.
Pages imprimées 54–56 / PDF 80–82 : § 6.1 et figures 6.1–6.4 ; imprimée 59 /
PDF 85 : figure 6.5. La correspondance des pages est vérifiée sur les
[images PDF 81](sources/these_pdf_81.png) et [PDF 82](sources/these_pdf_82.png),
avec [extraction imprimées 54–59](sources/these_imprimees_54_59_pdf_80_85.txt).
La [réponse du développeur, § 3](sources/REPONSE_CLAUDE_AUDIT_GEANT_20260930.md)
est figée séparément. [SOURCE_CAPTURE.json](SOURCE_CAPTURE.json) constate
les hashes avant/après des dix sources lues ; le PDF reste une dépendance
primaire extérieure. Le contrôle géométrique est rejouable avec les petites
copies locales, sans PDF ni moteur.

## Configuration et séquence exactes

La thèse impose deux triangles équilatéraux de côté `2r₀`, parfaitement
opposés, avec `CD = 2r₀`, pour `r₀ > 0`. Leur orientation est explicitée par
les figures : AB et EF verticaux, C et D face à face sur l'axe horizontal.
À isométrie près, leurs positions exactes sont :

| Point | x / r₀ | y / r₀ |
| --- | --- | --- |
| A | −√3 | 1 |
| B | −√3 | −1 |
| C | 0 | 0 |
| D | 2 | 0 |
| E | 2 + √3 | 1 |
| F | 2 + √3 | −1 |

Ces formules décrivent la figure ; elles ne supposent pas un nouvel espacement
ou une rotation absente de la source. La section ne donne aucune plage de
perturbation ε. Le `r′ ≳ r` de la figure 6.2 sert à montrer les intersections
juste après leur naissance, sans perturber les coordonnées.

En notant `ρ` le rayon courant et `β = ρ²`, les dates sont :

| Événement K = 2 | ρ / r₀ | β / r₀² | λ r₀² pour z = 2 |
| --- | --- | --- | --- |
| Sept composantes AB, AC, BC, CD, DE, DF, EF | 1 | 1 | 1 |
| Fusion des trois arêtes de chaque triangle | 2 / √3 | 4 / 3 | 3 / 4 |
| Fusion globale des **trois** composantes | √(2 + √3) | 2 + √3 | 2 − √3 |

Les deux rayons de fusion valent environ `1,154700538r₀` et `1,931851653r₀`.
Pour `z = 1`, les λ correspondants sont `1/r₀`, `√3/(2r₀)` et
`√(2 − √3)/r₀`. La convention v10 est bien `λ = ρ^(−z) = β^(−z/2)`
([head.hpp](sources/head.hpp), [head.cpp](sources/head.cpp)). Les dates en
rayon ne sont pas directement des dates en λ ; λ diminue quand ρ augmente.

Entre les deux fusions, la hiérarchie FULL/Γ₂ de la thèse a **ABC, CD, DEF**,
avec recouvrement de C et D, comme le montre explicitement la figure 6.5.
La partition exclusive **ABC | DEF** demandée par l'utilisateur est donc une
cible supplémentaire pour la projection des points. Le pont CD existe dans
la hiérarchie géométrique : un point partagé ne suffit pas à fusionner les
composantes. La thèse explique dès la page imprimée 54 que les amas K ≥ 2
peuvent se recouvrir ; elle reporte la partition stricte au chapitre 9.

Les circoncercles de ABC et DEF ont `p = 0, q_min = 3`. Les quatre boules
diamétrales AD, BD, CE, CF de fusion globale ont `p = 1, q_min = 2` : C est
strictement intérieur aux deux premières, D aux deux dernières. Ces six
boules ont `p + q_min = K + 1` ; elles portent les fusions FULL, mais ne font
pas partie de l'univers fort de couverture à `p + q_min ≤ K`.

## Ce que les points frontière distinguent

Dans la configuration idéale, les premières incidences fortes de C sont
AC, BC et CD, toutes à `β = r₀²` ; celles de D sont DE, DF et CD. Pour
`η = 1/8`, la bande en β se ferme à `81r₀²/64`, avant la naissance des
triangles `4r₀²/3`. Elle contient ces trois témoins de C/D.

LCA et antichaîne conservent une vraie lignée concurrente CD : C et D attendent
la fusion globale. A/B entrent dans leur triangle à `β = 4r₀²/3`, E/F dans
l'autre. Avant la fusion globale, la partition projetée a donc AB et EF,
avec C et D séparés. La réduction des ancêtres redondants ne retire pas le pont.

Une majorité stricte, avec dénominateur figé sur ces témoins de bande, reçoit
deux voix sur trois dans le triangle à sa naissance. Les poids uniformes et
`1/β` coïncident ici puisque tous les témoins ont le même β. Cela explique
`ABC | DEF` sur **cette** géométrie, sans démontrer une qualité statistique ni
une règle universelle. Un choix canonique entre témoins ex aequo ne justifie
pas à lui seul cette attribution. Avec le core v10 K2 comptant le site lui-même,
le point entre à distance du plus proche autre site, `2r₀`, après la fusion
globale ; il perd donc ici les deux blocs séparés de points.

## Les deux exports entiers effectivement disponibles

Les [entrées et exports](inputs/) proviennent de `/tmp/deux_triangles/` et
déclarent `engine_commit = e9eab2754f3d9f2a9e16d8c542b908c19b725c9a`, u18,
K = Kmax = 2, `only_order`. Ce sont des captures héritées ; leur déclaration
de commit ne remplace pas une fermeture des dépendances compilées.

Le premier brut fixe A `(268,3000,0)`, B `(268,1000,0)`, C `(2000,2000,0)`,
D `(4000,2000,0)`, E `(5732,3000,0)`, F `(5732,1000,0)`. Ici `r₀ = 1000`,
`h = 1732` remplace `1000√3` : les triangles sont très légèrement isocèles.
La deuxième variante translate **D, E, F de −2 en x**, sans toucher ABC.

| β exact | Arêtes inclinées plus courtes | Pont plus court |
| --- | --- | --- |
| AC, BC, DE, DF | 999956 | 999956 |
| AB, EF | 1000000 | 1000000 |
| CD | 1000000 | 998001 |
| Deux fusions de triangles | 249978000484 / 187489 | idem |
| Fusion globale | 3731956 | 3728225 |

Seules les **quatre arêtes inclinées** sont plus courtes que 2000 d'environ
`0,044000484`. Le pont court mesure **1998**, soit environ `1,956` de moins
que ces arêtes ; ce n'est pas une variation de `0,04` de même amplitude.
Dans ces variantes, C n'a donc pas trois incidences à sa toute première date.
Elles sont néanmoins toutes dans sa bande η = 1/8. L'intervalle de contrôle
`ρ ∈ [1,3r₀, 1,7r₀]` est strictement entre les deux fusions pour les deux bruts.

La lecture exacte vérifie : six coordonnées et leur permutation Morton,
chaque niveau, tous les intérieurs/coquilles des 13 boules, sept témoins
forts, deux fusions de triangles, quatre boules de fusion globale, dix
nœuds et la racine à **trois enfants**, ainsi que les distances d'entrée core.
`pont_plus_long` est un alias byte pour byte du premier export et du premier
brut ; il n'ajoute pas une troisième géométrie indépendante.

## Validation et portée

[normal.json](normal.json) et [optimized.json](optimized.json) contiennent la
même sortie, obtenue par `python3 -B check.py` et `python3 -B -O check.py`.
[RUN.json](RUN.json) conserve les commandes et codes retour.
[SHA256SUMS](SHA256SUMS) ferme les fichiers du reçu et est contrôlé par le
lecteur lorsqu'il existe. Les valeurs décimales ne servent qu'à l'affichage.

Ce reçu vérifie les unités, les formules, les populations et la structure des
deux captures observées. Il ne qualifie ni la complétude du générateur natif,
ni les autres K, ni la condensation, ni les scores de clustering. L'oracle
indépendant de Γ₂ et du vote idéal est un autre reçu de l'audit parent.
