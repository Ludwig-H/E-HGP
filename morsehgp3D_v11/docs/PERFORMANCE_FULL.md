# Voie rapide FULL du 3 octobre 2026

Cadre : `exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`.
Tranche développeur (Claude) répondant à l'écart v10/v11 mesuré sur G4 ; diagnostic, chiffres et
limites dans la [note d'audit](../audits/NOTE_CLAUDE_AUDIT_PERFORMANCE_V10_V11_20261003.md) et le
[reçu local](../receipts/developpement_20261003/ecart_v10_v11/README.md). Rien n'est qualifié sur G4.

Règle commune : **aucune décision ne change**. Les mêmes boules sont jugées et émises, les mêmes
descentes rendent les mêmes graines et dates, les forêts et verticales publiées sont identiques
octet pour octet à la voie historique. Seuls changent l'ordonnancement, le travail évité et
quelques compteurs de travail physique, énumérés plus bas. Les nouvelles voies de la tour sont des
options `FullParams` inactives par défaut. Les changements du catalogue, du census et des unions
remplacent la voie existante : ils ne changent aucune sortie, et seuls les compteurs `region_line_*`
du catalogue y changent de périmètre.

## Catalogue

**Plan lourd d'abord et ordre LPT.** Décrits dans
[CATALOGUE_FRONTIERE_ADAPTATIVE.md](CATALOGUE_FRONTIERE_ADAPTATIVE.md) : seule la moitié lourde des
listes divisibles est raffinée à chaque ronde, puis les tâches sont réclamées par taille décroissante.

**Filtre G1 d'un nœud** (`boxes.cpp`). Les termes du test de dominance sont séparés par site,
2·largeur·(x−lo) par axe et |x−lo|², et ceux des témoins du réservoir sont calculés une fois par nœud.
Leur différence redonne exactement 2·largeur·(x−y) : mêmes entiers (moins de 2^(2B+2)), mêmes
décisions. L'arrêt à K dominateurs est inchangé ; `filter_tests` est ajouté une fois par nœud et
reste égal au compte historique.

**G3 avant les droites J2** (`leaf.cpp`). Le test de paires garde sa position (contrat des compteurs
du graphe), puis vient G3 (union des masques de dominateurs, un mot), et seulement ensuite les droites
de centres exactes. Les filtres sont conjonctifs : juges, émissions et récursion sont inchangés ;
`region_line_tests`, `region_line_evaluations`, `region_line_cache_hits` et `region_line_rejects`
ne comptent plus que les préfixes admis par G3.

**Paires vivantes et coupe du préfixe** (voie graphe, feuilles d'au plus 32 sites). Après les masques,
`live[q−2][x]` contient les voisins y de x tels que |Dom(x) ∪ Dom(y)| ≤ K+1−q, pour q = 2, 3, 4.
Les candidats de cardinal q+1 sont les vivants restants, intersectés avec la ligne de chaque site du
préfixe ; un préfixe dont l'union dépasse déjà K−q n'est plus prolongé. L'union des dominateurs est
monotone et les seuils décroissent : tout préfixe écarté aurait été visité puis rejeté par G3 seul,
sans autre effet sur cette voie (aucun test de paire). `prefixes` reste donc le compte logique : la
récursion porte aussi l'ensemble que visiterait la voie graphe sans ces coupes, et la différence est
ajoutée. Sur 08/000000, 120 449 590 préfixes avant et après. Sans graphe, rien ne change.

**Population d'un mot.** Instruction matérielle si la cible a POPCNT, sinon forme SWAR (le profil
x86-64 de base appelait `__popcountdi2`) : même valeur.

**Clés F3/F4 du tri indirect** (`sort_indices.cpp`). Une clé binary64 par niveau : 64 bits de tête
tronqués puis une conversion (E = 2 par entier), quotient (E = 6). Deux niveaux ne sont ordonnés par
les clés que si x̃ < c·ỹ avec c = 1−2^(−40), conformément à F4 ; sinon la comparaison exacte
historique décide, supports et ordinal compris. Un niveau nul a un numérateur nul, donc une clé
exactement nulle : toute comparaison qu'elle tranche est exacte. La permutation est identique.

**Table des supports en parallèle** (`full_domain.cpp`). Même table à sondage linéaire, remplie par
le Pool avec prise des cases vides par CAS. La disposition des sondages peut dépendre de
l'ordonnancement, jamais la réponse d'une recherche ; un doublon de S* reste refusé.

## Forêts

**Table de populations** (`population_lookup.hpp`, option `population_lookup`). Lemme : si une partie
de k sites est exactement la population I ∪ U d'une boule b du catalogue, b la contient et son support
canonique S* est dans U, donc MEB = b, puis p < k et t = m : le pas de référence est terminal, graine
(b, k), dates égales au niveau de b. La table (populations des boules telles que p+m ≤ K) répond par
égalité exacte des sites ; l'étiquette de hachage ne fait que filtrer les sondages. Sans mémo de lane,
elle est consultée **avant chaque pas** (`descend_each_step`) : un succès remplace exactement le pas
terminal, la décroissance stricte des dates reste contrôlée. Sur 08/000000, chaque descente régulière
se termine ainsi (1 350 322 succès pour 1 350 322 descentes à K = 5). `ForestParallel::resolve_job`
utilise une voie courte (`hit`) qui ajoute directement les trois compteurs d'un pas.

**Ordres concurrents** (`forest_concurrent.cpp`, option `concurrent_orders`, exige les lots, Q > 0).
Étages sur tous les ordres : A classification par blocs, B naissances et états DSU (une tâche par
ordre), C toutes les cellules régulières de tous les ordres en une distribution par lanes (ordinal
global, mémos et espaces census des lanes), D publication par plateaux (une tâche par ordre, ordre
canonique de la voie par lots), E images verticales en **une** distribution par blocs de 2048
naissances, puis K−1 balayages fermés concurrents. Chaque forêt ne lit que ses graines et son DSU.
Les espaces census physiques valent min(W, lanes) dans ce mode.

**Unions de racines courantes** (`forest_plateau.cpp`). Dans `cell` et `regular_cell`, la première
racine reste la racine courante de la composante réunie, déjà touchée : `unite_roots` fusionne sans
nouveau `find` ni `touch`. Mêmes états, mêmes listes, mêmes compteurs (proposition P0 de la reprise).

**Census en signes** (`power_bound_signs`). Le parcours de l'index ne lit que le signe des deux
bornes. Sur la voie native certifiée, les sommes i128 de `power_bounds` sont lues sans conversion
Wide, comme le fait déjà `center_side` ; l'ordre des bornes reste contrôlé. Hors voie native,
`power_bounds` est appelé tel quel.

## Compteurs qui changent

| Compteur | Effet |
|---|---|
| `region_line_*` | Seuls les préfixes admis par G3 évaluent leurs droites. |
| `population_hits`, `part_meb_*`, `catalogue_hits` | Un pas de table remplace un pas MEB ; `steps` et `census_calls` inchangés. |
| `memo_*` des verticales concurrentes | Images sans mémo de lane. |
| Durées | Nouvelles phases `classify/births/regular/publish/verticals` en mode concurrent. |

Les invariants publiés restent vrais : `census_calls + catalogue_hits + singleton_hits = steps`,
`population_hits ≤ catalogue_hits + singleton_hits`, `tests = évaluations + hits` pour les droites.

## Portes

`mhgp11_tower_population_concurrent_*` (lemme contre la descente de référence pour chaque boule
éligible, équivalence des forêts à W1/W4/W48, Q = 1 et 4096, sept combinaisons d'options, refus) ;
`mhgp11_num_unit_bounds` et la sonde Fraction des bornes (signes égaux à ceux de `power_bounds`) ;
portes du graphe de paires (`same_work`, ordre des émissions) et du cache J2 inchangées. Mutants
nouveaux ou réancrés : `bound_signs_native_swapped`, `contact_exterieur`, `contact_interieur`,
`dominance_egalite_retiree`, `temoin_commun_compte_deux_fois`, `prefixe_q3_obtus_rejete`,
`pair_graph_intersection_missing`, `lignes_vivantes_seuil_strict`,
`lignes_vivantes_compte_logique_omis`, `prefixe_seuil_suivant_strict`, `sort_cle_marge_inversee`,
`sort_cle_produit`, les mutants `population_*` et `concurrent_*`, `racine_courante_non_suivie`.
