# Gaussiennes 3D — plan préfixé du 27 septembre 2026

48 scènes entières de 1 200 points, sans bruit injecté, sans rejet de
réalisation ni rééchantillonnage selon sa qualité. Les labels de vérité
sont les **composantes génératrices**, numérotées 1..G. Le chevauchement de
densités gaussiennes rend parfois ces labels difficilement séparables par
la seule géométrie ; il ne transforme pas les points en bruit de vérité.

Le lot est distinct de `b_point_hierarchy_k_20260927`, dont aucune source
ni capture n'est modifiée. Son quantificateur est importé après vérification
du SHA256 `7ff3d93677f51eabe9c1bea0f56a62bbcd53ce531c3a49bdfafeab29ab14f53c`.
Les données privées ne sont pas versionnées ; aucune donnée externe,
installation, API distante ou ressource GCP n'est utilisée pour ce nouveau lot.

## Plan et effectifs

| Régime | G | Séparation δ | Graines | Nombre de scènes | Effectifs vrais |
|---|---|---|---:|---:|---|
| Isotrope équilibré | 2/4/8/16 | 8/4/2 | 3 | 36 | 600/300/150/75 par classe |
| Anisotrope équilibré | 8 | 8/4/2 | 2 | 6 | 150 par classe |
| Isotrope déséquilibré | 8 | 8/4/2 | 2 | 6 | 240/60 alternés, poids 4:1 |

Graines : 2026092711, 2026092712, 2026092713 ; les stress utilisent les deux
premières. La première est marquée development, les suivantes evaluation.
Ce marquage est préfixé et ne prétend ni rendre les données aveugles, ni
transformer les scènes appariées en observations indépendantes.

La grille de comparaison prévue est K={5,10}, min_cluster_size={10,20,50,100},
exp_z={1,2} ; primaire K=5, min_cluster_size=20, exp_z=1. Le générateur
ne lance aucun score HGP/HDBSCAN et ne choisit aucun paramètre à partir de
la vérité. Il conserve les 48 réalisations et tous les points. À G=16,
les classes de 75 points sont plus petites que min_cluster_size=100 ; les
petites classes du stress déséquilibré en comptent 60. Cette incompatibilité
fait partie du diagnostic voulu, pas d'un motif de suppression des classes
ou de modification des labels. Une taille minimale n'est pas le nombre de
communautés à retrouver, et ne fixe pas celui-ci.

## Moyennes et appariement

Disposition déterministe parmi les 27 sommets de `{-1,0,1}³`, par
farthest-first : premier point (-1,-1,-1), puis maximisation de la distance
carrée minimale aux points retenus ; égalités départagées lexicographiquement.
Ces comparaisons utilisent des entiers. Les G centres sont recentrés puis
divisés par leur distance minimale, qui devient 1. Une rotation propre
commune, déterminée par (G,graine), est obtenue par QR d'une matrice normale
3×3, signes de diagonale corrigés et déterminant positif. Les moyennes sont
δ fois ces centres. Leur distance euclidienne minimale est donc δ, aux
erreurs flottantes déclarées de construction près ; sa valeur mesurée est
conservée. Ce n'est pas un simple alignement 1D de moyennes pour tous les G.

Pour chaque (régime,G,graine), changer δ conserve **les mêmes normales
standardisées, labels, covariances, centres unitaires et rotations**. Seules
les moyennes sont dilatées. Chaque scène est donc appariée aux deux autres
séparations de sa famille. Il n'y a aucun test d'acceptation sur séparation
empirique, score ou réalisation favorable. La grille u18 propre à chaque
scène dépend de son étendue : l'appariement est celui des réalisations en
unités physiques, pas une égalité artificielle des résidus après quantification.

NumPy `Generator(PCG64(SeedSequence([seed,G,stream])))` : stream 0 pour les
1 200 normales 3D, 1 pour la rotation commune, 100+j pour la rotation de
covariance de la composante j. Les points restent dans l'ordre des composantes
génératrices ; leurs IDs sont ces indices et sont identiques dans les deux
méthodes. Aucune colonne de labels n'entre dans les fichiers de coordonnées.

## Covariances, mélange et vérité

Isotrope : covariance I₃. Anisotrope : écarts-types propres (2,1,0.5),
covariance `R_j diag(4,1,0.25) R_jᵀ` avec rotations propres individuelles
distinctes déterminées par la graine. δ reste exprimé en unités de l'écart-type
isotrope de référence 1, **pas** en distance de Mahalanobis commune à ces
composantes anisotropes. Les deux stress sont séparés : pas de combinaison
anisotropie × déséquilibre supplémentaire.

Les effectifs sont fixes, équilibrés ou exactement dans le rapport 4:1
alterné. Les points sont indépendants conditionnellement à ces labels ; les
effectifs de classes ne sont pas tirés par une multinomiale. Les priors connus
du diagnostic sont les proportions prescrites n_j/1200. Les moyennes,
covariances, priors, rotations, effectifs et hash des normales standardisées
restent dans `parameters.json`. Les labels positifs évitent la convention
historique du quantificateur où zéro désigne le bruit et devient -1.

## Diagnostic de difficulté à modèle connu

Le MAP ponctuel maximise `log prior_j - log det(cov_j)/2 -
(x-mu_j)^T cov_j^-1 (x-mu_j)/2` ; la constante normale commune est incluse
dans le code. Aucun paramètre n'est estimé à partir des points ni optimisé
selon leurs labels. Une égalité numérique exacte choisit le plus petit label.
Le calcul est flottant, avec covariances explicitement positives définies.

Deux prédictions sont sauvegardées dans `bayes_map.json` :

- `predictions_original` sur les coordonnées générées ;
- `predictions_reconstructed_grid` sur **origin + step × q**, c'est-à-dire
  les coordonnées quantifiées utilisées par les méthodes, remises dans
  l'unité physique des paramètres connus. C'est celle à utiliser comme
  diagnostic de la géométrie effectivement comparée.

Le fichier conserve aussi les deux fractions correctement classées,
`grid_prediction_changes`, la matrice de confusion original→MAP et
`fitted=false`. C'est une difficulté **empirique sur ce tirage** avec
connaissance du modèle générateur, pas l'erreur Bayes théorique intégrée,
pas une baseline de clustering et pas une borne supérieure d'ARI/NMI.
Le MAP ponctuel n'exploite pas une contrainte collective d'effectifs fixes.
Il n'est utilisé ni pour filtrer des points, ni pour choisir les paramètres.

## Quantification et interface

Le quantificateur antérieur épinglé effectue une seule translation par axe
et **un seul pas isotrope** `max_span/(2^18-1)`, avec arrondi rationnel exact
au plus proche, égalités vers le pair, des coordonnées binary64 générées.
Pas de normalisation par axe, de jitter, de fusion cachée ou de rééchantillonnage.
Tout doublon quantifié est refusé, y compris à même label. Erreurs mesurées,
borne h/2 par coordonnée, borne euclidienne sqrt(3)h/2, pas/origine rationnels
et comptes de doublons/collisions sont conservés.

`prepare(root)` écrit d'abord `plan.json`, puis `manifest.json` et
`prepared/CASE/`. Les sorties déjà présentes sont réutilisées seulement à
octets identiques ; aucune différence n'est écrasée. Numpy et opérations
transcendantes/QR sont épinglés par version et hashes de réalisations, pas
promis byte-identiques sur toutes les architectures. Environnement préparatoire
observé : NumPy 2.5.3, aucune nouvelle dépendance.

Chaque cas expose les clés communes `id,name,n,dimension,split,points_u32le,
points_npy,labels_json,source_sha256,prepared_sha256,quantization`, ainsi que
`regime,communities,separation,seed,true_counts,parameters_json,bayes_map_json`.
`prepared_sha256` porte sur les trois entrées communes ; `diagnostic_sha256`
porte séparément sur les paramètres et MAP.

`points.u32le` est un flux XYZ uint32 little-endian sans en-tête, 12n octets.
`points.npy` contient exactement les mêmes q, représentables sans perte en
float64. `labels.json` contient les 1 200 labels positifs génératifs. Le
`source_sha256` identifie les XYZ physiques float64 little-endian puis les
labels int64 little-endian, en ordre C ; ce n'est pas le hash d'un fichier
de coordonnées d'origine prétendument versionné.

Racine privée : `/workspaces/E-HGP/build/v9-gaussian-clustering-data-20260927`.

```sh
python3 -B gaussian_data.py /CHEMIN/PRIVE
python3 -B test_gaussian_data.py
python3 -B -O test_gaussian_data.py
```

La préparation locale des 48 cas a conservé 57 600 points, zéro doublon brut,
zéro collision de grille, zéro fusion et zéro bruit injecté. Les décisions
MAP brut/reconstruction de grille ne diffèrent sur aucun point de ce lot.
Cette vérification n'est ni une évaluation des algorithmes de clustering,
ni une qualification géométrique, statistique, FULL ou GPU.
