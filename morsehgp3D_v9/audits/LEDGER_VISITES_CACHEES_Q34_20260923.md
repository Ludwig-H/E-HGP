# Audit v9 — visites q3/q4 absentes du grand-livre de tour

23 septembre 2026. Lecture du moteur et de la sonde au snapshot `be699a69`
de `main` (mêmes fichiers moteur que le worktree développeur `ba9980fd`).
Audit statique indépendant : **aucun moteur modifié, aucun nouveau test G4**.
Ce constat porte sur l'observabilité du coût, non sur une erreur de résultat.

**Suivi 4530644b :** la sonde v12 projette désormais ces six parcours et
`local.sweep.active_sites`, avec des identités bornées dans le lecteur.
La lacune ci-dessous concerne le reçu R7b/v11 ; une nouvelle campagne
appariée v12 doit encore mesurer la croissance de ces champs. La
contrelecture statique du port ne trouve pas de rejet d'une sortie valide
avec les options de chaîne actuelles (`RectanglePair`, `Local28`,
`LiveOnly`). La borne des graines q4 devra être revue si `Individual` ou
`Joined` remplace `LiveOnly`.

## Parcours déjà comptés, puis perdus par la chaîne

| Compteur du générateur | Travail réellement compté |
| --- | --- |
| `w.witness.rectangles.node_visits` | Chaque nœud témoin `Z` dépilé par le DFS de `filter_q34_witnesses` appelé une fois par rectangle WSPD en mode `RectanglePair`, avant l'expansion de son produit `A×B`. Un rectangle rejeté entièrement peut avoir payé ce DFS. |
| `w.witness.pairs.node_visits` | Les nœuds dépilés par les recherches de témoins des paires encore développées. Avec le cache actif, une paire entièrement rejetée par les nœuds réutilisés ne lance **pas** cette recherche ; ses tests figurent séparément dans `w.witness_cache.node_tests`. Une recherche sur les seules voies restées ouvertes est tout de même comptée ici. |
| `w.q3.seed_node_visits` | Les nœuds de l'index global visités pour chercher les troisièmes sommets aigus et possédés, une fois par appel `q3_edge`. Il s'agit d'un parcours **par arête q3 ouverte**, pas d'un parcours par graine retenue ; un certificat de racine de l'atlas peut éviter entièrement cet appel. |

Les deux premiers sont incrémentés à chaque nœud effectivement dépilé dans
[`q34_witness_search.cpp`](../src/gen/lanes/q34_witness_search.cpp) (`filter_impl`,
lignes 105–115), appelé au niveau rectangle et au niveau paire dans
[`wspd_q34.cpp`](../src/gen/pipeline/wspd_q34.cpp) (lignes 419–447 et
519–556). Le troisième est incrémenté dans `q3_edge` du même fichier
(lignes 700–765). Les structures et les réductions SUM/MAX des workers
conservent déjà ces valeurs :
[`wspd_q34.hpp`](../src/gen/pipeline/wspd_q34.hpp), lignes 78–101, et
[`wspd_q34.cpp`](../src/gen/pipeline/wspd_q34.cpp), lignes 197–269.

La configuration **réelle de la chaîne** choisit `Q4LocalOptions{}`
(`domain=Positive`), `Local28` et les graines `LiveOnly`
([`tower_chain.cpp`](../src/chain/tower_chain.cpp), lignes 347–363). Trois
autres DFS de l'index global sont alors comptés dans chaque résultat q4,
et déjà additionnés entre workers, mais ne sortent pas non plus dans le
grand-livre :

| Compteur q4 | Travail réellement compté |
| --- | --- |
| `w.local.geometry.domain.node_visits` | Parcours de `Q4PositiveDomain::build` pour trouver la boîte exacte des complétions de l'arête, endpoints exclus ; appelé par la géométrie positive avant l'atlas ([`q4_positive_domain.cpp`](../src/gen/lanes/q4_positive_domain.cpp), lignes 86–143 ; [`q4_local_partition.cpp`](../src/gen/lanes/q4_local_partition.cpp), lignes 47–73). |
| `w.local.geometry.cover_node_visits` | Second parcours de l'index, `decompose_cover`, pour transformer les plages du cover complet en antichaîne de nœuds sans recopier les sites ([`q4_local_partition.cpp`](../src/gen/lanes/q4_local_partition.cpp), lignes 76–106). Ce travail est distinct du premier `Q34EdgeCover::build` déjà publié comme `cover_node_visits`. |
| `w.local.node_visits` | Parcours des nœuds spatiaux candidats aux graines q4 par `Q4SeedCellEngine::live_only` ; chaque nœud effectivement considéré passe par `spatial_pass` ou `seed_pass`, qui incrémentent ce compteur ([`q4_local.cpp`](../src/gen/lanes/q4_local.cpp), lignes 641–669 et 689–710). Si l'atlas n'a aucune feuille vivante, le certificat de racine saute ce parcours. |

La borne `2n−1` ci-dessous vaut pour ces trois **avec les options de chaîne
ci-dessus** : `LiveOnly` fait une seule traversée par arête ; le backend
`Joined` est un autre algorithme de produits nœud×cellule et ne reçoit
pas cette borne par simple transfert. `w.local.sweep.active_sites`,
incrémenté à chaque site de fragment balayé pour chaque requête graine/feuille
([`q4_local.cpp`](../src/gen/lanes/q4_local.cpp), lignes 374–415), est
également absent du grand-livre. Le champ publié `q4_sweep_events` est
seulement `kept_events`, **pas** cette masse de balayages répétés. Ces six
DFS ne sont donc pas un inventaire exhaustif du travail q4 aval.

En revanche, [`tower_chain.cpp`](../src/chain/tower_chain.cpp),
lignes 379–416, ne copie aucun de ces six parcours ni
`local.sweep.active_sites` dans `GeneratorLedger` ; la
[`sonde de tour`](../bench/tower_probe.cpp), ligne 203, ne peut donc pas les
publier. Elle publie notamment `witness_input_pair_mass`, le nombre de
rectangles rejetés, les paires développées et `witness_cache_node_tests`,
mais pas ces visites ni le balayage total des feuilles q4. Le grand-livre
actuel ne permet donc pas d'attribuer tout le
travail de q3/q4 ni de mesurer sa pente par voie.

## Borne et interprétation correcte

L'index de `n` **sites préparés distincts** est un arbre binaire plein à
`n` feuilles, donc à `2n−1` nœuds ; sa construction et ses liens `escape`
sont dans [`q2_census.cpp`](../src/gen/pipeline/q2_census.cpp),
lignes 112–170. Chacun des six parcours ci-dessus visite un nœud au
plus une fois par recherche ou arête :

```text
rectangles.node_visits <= (2n-1) * rectangles.queries
pairs.node_visits      <= (2n-1) * pairs.queries
q3.seed_node_visits    <= (2n-1) * q3.edge_queries
q3.seed_node_visits     = q3.seed_point_tests + q3.seed_bound_tests
local.geometry.domain.node_visits        <= (2n-1) * q4_edges
local.geometry.cover_node_visits         <= (2n-1) * q4_edges
local.node_visits                        <= (2n-1) * q4_edges  [LiveOnly]
```

Ces bornes sont **par appel**, pas sous-quadratiques globalement.
`rectangles.queries` peut croître avec le front, `pairs.queries` avec la
masse résiduelle, `q3.edge_queries` et `q4_edges` avec les arêtes
survivantes ; une somme allant jusqu'à `O(n·R + n·P + n·E3 + n·E4)`
reste possible, **avant** les balayages répétés. Rien ici ne
prétend qu'elle est atteinte sur SemanticKITTI : les bornes géométriques
peuvent élaguer fortement. Il faut les valeurs réellement mesurées.

Le reçu [G4 R7b](../receipts/g4_tower_r7b_20260923/README.md) montre
sur 08/000100 sans sol, K10/s8/W48, `17 488 839` paires développées,
`1 235 154` arêtes q3 après preuves et `5,226 s` de q3/q4, mais pas
ces six visites ni `sweep.active_sites`. Les `1 235 154` arêtes sont seulement une borne
supérieure du nombre de `q3_edge` : certaines sont sautées par l'atlas.
Les trois trames de R7b sont de la **même séquence**, de tailles et de
géométries différentes ; elles ne fournissent ni une pente appariée
8k/16k/32k ni une qualification sous-quadratique.

## Porte minimale, sans modifier le générateur

1. Un sidecar d'audit appelle `run_wspd_q34_parallel` avec les **mêmes**
   options que [`tower_chain.cpp`](../src/chain/tower_chain.cpp),
   lignes 347–374, et lit directement `r34.pipeline.work`. Publier les
   six comptes ci-dessus avec `rectangles.queries`, `pairs.queries`,
   `q3.edge_queries`, `q3.seed_point_tests`, `q3.seed_bound_tests`, les
   visites du cache, `q4_edges`, `local.geometry.preparations`,
   `q4_seed_cells.queries`, `local.sweep.active_sites` et les masses du
   front. Compter séparément le coût
   d'entrée/index et le temps de ce flux ; le sidecar n'est pas FULL.
2. Sur une même trame et un même masque sans sol, utiliser les coupes
   spatiales passant par le capteur et les tailles appariées
   8k/16k/32k. Comparer W1/W8 puis s8/10/12, K5/K10. Vérifier le flux
   exact complet — clés de boule, arité, support, profondeur et IDs de
   coquille, triés hors ordre des callbacks — entre exécutions, pas
   seulement un condensé. Conserver aussi `cover_sites`, `core_sites`,
   visites de covers, tests/copies d'atlas et sorties pour juger **tout**
   le travail, sans inférer une pente des seuls compteurs ajoutés.
3. Dans la prochaine sonde **FULL**, projeter simplement ces compteurs
   déjà agrégés dans `GeneratorLedger` et le JSON du probe, avec leurs
   identités/bornes ci-dessus et la provenance du paquet. Cela touche la
   télémétrie `chain/bench`, pas l'algorithme `gen` ; le coût de la
   projection est négligeable. Un histogramme des tailles `|A|×|B|` des
   produits restant après le filtre rectangle compléterait cette porte
   pour décider si un certificat pré-expansion par blocs est rentable.

Tant que ces mesures manquent, la réduction des paires et des covers ne
prouve pas que les DFS de témoins, de graines et de préparation q4 restent sous-quadratiques
dans le régime LiDAR visé. Aucun défaut géométrique ni contrat G4/GPU
n'est déduit de cette lacune de publication.

Une réduction **à tester après comptage** est disponible pour les graines
q3 **et q4** : si `ab` est leur arête propriétaire, avec `D=|a−b|²`, leur troisième
site `x` vérifie `|x−a|²,|x−b|²≤D`. L'identité du parallélogramme donne
`|x−(a+b)/2|²=(|x−a|²+|x−b|²)/2−D/4≤3D/4<D`. Chaque graine admissible
est donc dans le [cover complet](../src/gen/lanes/edge_cover.hpp) déjà
construit pour cette arête. Restreindre le parcours aux plages certifiées
de ce cover ne perdrait aucune graine. Les deux voies testent actuellement
la même acuité et le même propriétaire, mais peuvent être désactivées
séparément ; une sélection partagée pourrait éviter deux parcours quand
elles sont toutes deux ouvertes. Scanner toutes les plages peut néanmoins
coûter plus que les deux DFS qui éliminent tôt des boîtes entières.
Comparer visites, sites testés, flux exact et temps avant de porter
cette piste.
