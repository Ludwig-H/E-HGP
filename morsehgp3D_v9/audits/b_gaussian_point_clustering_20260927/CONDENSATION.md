# Arbre condensé de clusters — API commune HGP/HDBSCAN

Ce nouvel adaptateur n'altère ni les anciennes captures, ni les points, ni le
moteur géométrique. Il appelle `condense_eom` dans le fichier historique
`b_point_hierarchy_k_20260927/eom.py`, vérifié par SHA-256
`c7121c857010fd89b6798ec634a0f55c4abd14c1c6bc6305c551c06e4625dead`.
Il expose son arbre condensé, distinct de la sélection finale de labels.

## Utilisation et règle de nettoyage

`point_clusterer_from_tree(tree, min_cluster_size, exp_z=1)` accepte
`tree={n, children, heights}` : mêmes tableaux pour un arbre de points HGP ou
l'arbre extrait de HDBSCAN. Les feuilles `0..n-1` sont les identifiants originaux.
Les hauteurs sont des **rayons**, jamais des rayons au carré. L'adaptateur ne
recalcule aucune distance et ne choisit aucun K.

En allant de la racine vers les feuilles, le rayon décroît et
`lambda=1/r**exp_z` augmente. À chaque multifusion inversée, pour le seuil m :

- Aucun enfant de taille au moins m : tous les points restants sortent du
  cluster à cette date ; les sous-branches petites ne deviennent pas des clusters.
- Un seul grand enfant : il prolonge le même cluster ; les autres points sortent.
- Au moins deux grands enfants : le cluster termine et ces enfants deviennent
  simultanément de nouveaux clusters, avec leurs masses de naissance.

Les subdivisions internes de même hauteur sont atomisées avant ce traitement.
La racine structurelle est toujours conservée, même pour `n<m` ou `n=1`, mais
elle n'est **jamais sélectionnable par EOM**. `min_cluster_size>=2`, expZ vaut
1 ou 2, epsilon vaut zéro, `allow_single_cluster=False`.

## Sortie compacte

Le résultat contient `selection`, `condensed_tree`, `stats`, `validation` et
`provenance`. `selection.labels` conserve exactement n entrées ; -1 signifie
bruit de la sélection EOM, pas disparition d'un point et pas un cluster durable.
`selection.selected` emploie les nouveaux IDs compacts de clusters.

L'objet `condensed_tree`, schéma `mhgp9_condensed_point_tree_v1`, utilise deux
espaces d'identifiants distincts : points `0..n-1`, clusters `0..C-1`, racine 0.

| Champ | Signification |
|---|---|
| `n_points`, `min_cluster_size`, `exp_z`, `root` | Domaine et paramètres déclarés |
| `parent[C]` | Parent de cluster, -1 pour la racine |
| `birth_lambda[C]`, `death_lambda[C]` | Naissance et dernière sortie/transfert de masse |
| `mass_at_birth[C]`, `own_stability[C]` | Masse initiale et intégrale de masse propre, avant choix EOM |
| `children_offsets[C+1]`, `children[C-1]` | Enfants-clusters en CSR ; date de l'arête = naissance de l'enfant, masse = sa masse initiale |
| `point_exit_parent[n]`, `point_exit_lambda[n]` | Cluster dont chaque point sort individuellement, et date de sortie |
| `point_exit_offsets[C+1]`, `point_exit_ids[n]` | Les mêmes sorties regroupées par cluster, chaque point exactement une fois |
| `legacy_cluster_ids[C]` | Correspondance vers les identifiants historiques n..n+C-1 |

Un cluster peut perdre des points avant sa bifurcation. Ses seuls points sortis
directement ne constituent donc **pas** son ensemble à la naissance : celui-ci
est l'union disjointe de ces points et des ensembles de naissance de ses enfants.
Il n'y a aucune liste de membres complète dupliquée par ancêtre dans le format.

`stats` sépare les nœuds internes bruts, ceux contractés à hauteur égale, les
petites branches supprimées comme clusters et les continuations sans nouvelle
identité de cluster. L'identité de comptage est : internes bruts = internes source
retenus + contractions de plateau + petites branches + continuations.
La racine ajoutée pour un singleton n'est pas un nœud interne source.

## Coupes, masses et validation

La convention de coupe est `[birth_lambda, death_lambda)` : à la naissance d'un
enfant, son parent a terminé ; à sa date de sortie individuelle, le point est
inactif. `condensed_cut(tree, lambda_value)` fournit les clusters vivants et les
`inactive_points` séparément. Ceux-ci restent des identités individuelles, jamais
un unique « cluster bruit ». En complétant la coupe par leurs singletons, on
obtient des partitions qui se raffinent quand lambda augmente.

`validate_condensed_tree` vérifie les tableaux en O(n+C), puis trie les dates
locales des événements : coût total O(n+C+Σ e_c log e_c), au pire
O((n+C) log(n+C)), et stockage O(n+C). Les contrôles portent sur :
arbre de parents unique, dates ordonnées, tailles à la naissance, points sortis
une fois exactement, masse de chaque cluster égale aux masses de ses enfants
plus ses sorties propres, et rejouabilité des stabilités. Les pertes simultanées
sont traitées en un seul lot ; une branche non racine vivante conserve au moins m
points. Une descendance unique pour chaque point et ces contraintes de dates
garantissent des membres laminaires et des coupes vivantes disjointes.
Après cette validation, le lecteur explicite de coupe coûte O(n × profondeur
de l'arbre condensé), réservé au diagnostic ;
la validation ne développe pas toutes les coupes ni toutes les appartenances.

À rayon zéro, lambda est `+inf`. `[inf,inf]` a durée zéro, pas NaN. Les infinis
sont des valeurs sémantiques Python explicites ; un export JSON strict doit les
encoder par une convention déclarée, jamais comme des nombres JSON non finis.
Les fixtures de la campagne gaussienne à coordonnées distinctes ne deviennent
pas pour autant une preuve générale d'absence de rayons nuls.

La condensation/EOM reste le post-traitement binary64 historique, non certifié
numériquement. Les dates rationnelles HGP, si fournies, restent dans l'arbre brut
et ne sont pas remplacées par ce format lambda flottant. Les labels et la
qualité statistique sont distincts de la validité structurelle de cet arbre.

## Tests ciblés

`python3 -B test_condensed.py` et `python3 -B -O test_condensed.py` couvrent les
cas zéro/un/deux grands enfants, une multifurcation, m=10/20/50/100, la racine
sous le seuil, le singleton, les plateaux, les rayons nuls, les deux exposants,
la conservation de tous les points, les coupes raffinantes et des corruptions.
Aucun fit sklearn, GPU, GCP ou changement du moteur n'est nécessaire.
