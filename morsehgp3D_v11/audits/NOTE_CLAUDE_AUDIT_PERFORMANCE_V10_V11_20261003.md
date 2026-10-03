# Audit développeur : pourquoi la v11 est cinq à six fois plus lente que la v10

2026-10-03 12:07 UTC. Demande de l'utilisateur : expliquer l'écart sur G4 (« à peu près 1 seconde à K = 5 contre
200 ms pour la v10 »), publier un audit et corriger le code. Code diagnostiqué : **`895680ff8`** (HEAD de
`main`) ; correctifs dans le commit qui ajoute cette note. Cadre :
`exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`.

**GCP non utilisé** : le conteneur de cette session n'a aucun identifiant GCP, aucune commande GCP n'a été
lancée. Toutes les mesures nouvelles sont **locales** (conteneur partagé à 4 cœurs, bruit de ±10 %), sur les
entrées du manifeste `reuse1` reconstruites depuis le dépôt. Les temps G4 cités viennent de la
[reprise](AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md) et de ses reçus. Reçu :
[ecart_v10_v11](../receipts/developpement_20261003/ecart_v10_v11/README.md).

## 1. Constat

Sur G4 (48 travailleurs, K = 1..5, mode 2047), la v11 `ae817d09e` prend 1,15 à 1,51 s contre 0,20 à 0,25 s
pour la v10 (×5,7 à ×6,0). Sur 08/000000 : domaine (catalogue + lookup) 805 ms contre 164 ms ; forêts et
verticales 658 ms contre 89 ms ; W1/W8/W48 = 15,3/2,51/1,46 s. Deux faits orientent le diagnostic :
l'accélération W1 → W48 n'est que ×10,5, et localement à W4 le CPU total vaut 2,5 fois celui de la v10
(26,9 s contre 10,6 s sur 08/000000). Il y a
donc deux problèmes : un **chemin critique** sériel ou mal réparti à W48, et du **travail en trop**.

## 2. Causes, par ordre de poids

| # | Cause | Preuve | Poids estimé à W48 sur G4 |
|---|---|---|---|
| C1 | **Déséquilibre de la passe unique du catalogue.** Le plan adaptatif raffinait toutes les listes divisibles à chaque ronde : 1024 feuilles atteintes vers la profondeur 11 partout, tandis que les zones denses restaient des tâches de milliers de sites. | Durées par tâche à W1, 08/000000 (reçu) : 951 tâches, la plus longue **0,885 s sur 12,25 s** ; mur simulé à 48 workers **0,923 s** pour un idéal de 0,255 s, du même ordre que les 659 ms de géométrie observés sur G4. | ~0,4 s |
| C2 | **Forêts construites ordre après ordre par un pilote sériel.** Classification, naissances, lots de 4096 cellules, publication DSU et balayages s'enchaînaient par ordre, avec ~600 barrières `parallel_for` ; le pilote seul portait ~330 ms à W48 (classification 69, naissances 42, publication 161, balayages 62 ms). | Profils `perf`, chronos par phase de la sonde, reprise. | ~0,35 s |
| C3 | **Pas d'équivalent du semis H_K.** Chaque pas de descente refaisait MEB et localisation ; la v10 s'arrête sur une table de populations dans 71 à 86 % des pas. Le mémo direct de lane ne faisait que 2,6 % de succès. | Compteurs de descente ; `seed_hits` de la v10 sur la même trame ; 15,7 M présentations MEB de parties sur 08/000000. | premier poste CPU de la forêt |
| C4 | **Feuilles du catalogue : préfixes condamnés prolongés.** Droites J2 exactes avant G3, aucune paire vivante (la v10 a `live2`), aucune coupe par l'union des dominateurs du préfixe : 148 M tests de droites et 54 M évaluations exactes sur 08/000000. Filtre de nœud recalculant les témoins avec un `checked_add` par test, `__popcountdi2` logiciel. | Profil : `center_region_possible` 17,7 %, `prepare_node` 11,1 % du CPU ; passe unique à W4 local 3,18 s contre 1,70 s pour l'étage des boîtes v10. | passe unique ×1,9 en CPU |
| C5 | **Arithmétique exacte sans filtre là où la v10 filtre.** Tri du catalogue en produits croisés Wide à chaque comparaison (58 ms contre 11 ms sur G4) ; census de l'index convertissant chaque borne native en Wide ; MEB et côtés de sphère exacts sans filtre F6. | Profils ; reprise, points 3 et 4. | tri ~50 ms ; le reste en CPU |
| C6 | Coûts annexes : table des supports remplie par le pilote (~30 ms), `find` répétés de la publication (P0 de la reprise), copies de `DescentResult` et de ledgers à chaque trace, profil u21 (~6 % contre u18, mesuré localement sur l'ancien code). | Profils, compteurs. | quelques dizaines de ms |

Ce qui n'explique **pas** l'écart : la cible ISA (les chronos G4 de la v10 sont en build par défaut, comme la
v11 ; `x86-64-v3` reste non mesuré), l'index (0,4 ms), le Cloud (1 ms), l'assemblage (4,7 ms) et
l'échauffement (première passe v10 à 259,8 ms contre 252,0 ms).

## 3. Correctifs livrés

Aucune décision ne change. Sorties canoniques identiques octet pour octet à la base sur les six entrées et
dans toutes les configurations mesurées (35 exécutions, une seule empreinte de sortie par entrée).
Mécanismes, compteurs touchés et portes : [PERFORMANCE_FULL.md](../docs/PERFORMANCE_FULL.md).

| Cause | Correctif | Effet local mesuré |
|---|---|---|
| C1 | Plan « lourd d'abord » et réclamation LPT des tâches | plus longue tâche 0,885 → 0,035 s ; mur simulé W48 0,923 → 0,177 s (0,202 s sans graphe de paires) |
| C2 | Option `concurrent_orders` : classification par blocs, une distribution pour toutes les cellules régulières de tous les ordres, publications et balayages concurrents, images verticales en une distribution | pilote seul ~330 ms à W48 avant ; après, chaîne sérielle de l'ordre 5 à W1 local : naissances 36, publication 67, balayage 37 ms (~85 ms estimés sur G4) |
| C3 | Option `population_lookup` : table I ∪ U → boule (lemme du terminal), consultée avant chaque pas, voie courte dans `resolve_job` | présentations MEB de parties 15,7 M → 3,8 M ; chaque descente régulière finit sur la table |
| C4 | Filtre de nœud prétraité, G3 avant les droites, lignes vivantes et coupe de l'union du préfixe (voie graphe), popcount SWAR | CPU de la passe unique à W1 11,9 → 7,9 s (−34 %) ; droites évaluées 54,1 M → 31,3 M |
| C5 | Clés F3/F4 du tri indirect ; signes natifs des bornes du census | tri 1,71 → 0,38 s à W1, 454 → 97 ms à W4 |
| C6 | Table des supports par CAS ; unions de racines courantes ; ledger direct des succès de table ; garde de chaîne cyclique | gain non chiffré : mesures isolées dans le bruit |

## 4. Mesures locales appariées

Médianes de trois exécutions à W4 (LiDAR), une exécution sinon ; base `895680ff8` en mode 2047 (configuration
G4 de la reprise), nouvelle voie en mode 2047 (seuls les changements du catalogue, du census et des unions
jouent) et en mode 16379 (2047 + graphe de paires, table de populations, ordres concurrents, sans mémo).

| Entrée | Base 2047 | Nouveau 2047 | Nouveau 16379 | CPU base → 16379 |
|---|---:|---:|---:|---:|
| 08/000000, 39 885 sites | 7 550 ms | 6 286 ms | **4 471 ms** | 26,9 → 17,1 s |
| 08/000100, 35 551 sites | 5 731 ms | 4 777 ms | **3 624 ms** | 20,4 → 13,8 s |
| 08/000200, 45 845 sites | 6 977 ms | 5 836 ms | **4 336 ms** | 24,6 → 16,3 s |
| uniforme 8 000 | 2 682 ms | — | **1 696 ms** | 9,5 → 6,4 s |
| uniforme 16 000 | 5 900 ms | — | **3 759 ms** | 21,0 → 14,3 s |
| uniforme 32 000 | 13 580 ms | — | **8 223 ms** | 47,8 → 31,2 s |

À W1 sur 08/000000 : 25,2 s → 17,1 s (domaine 14,4 → 8,9 s, forêts 10,8 → 8,2 s). Sur 08/000000 à W4, le domaine
passe de 3 914 à 2 295 ms (tri 454 → 97 ms) et la forêt de 3 635 à 2 229 ms. Pic de réservation
`MemoryBudget` : 328 → 339 Mo (+3 %, table de populations). Quatre cœurs saturés ne montrent pas les gains de
chemin critique (C1, C2) : ils apparaissent dans la simulation par tâche et dans les phases à W1.

**Estimation G4, à confirmer en session gardée.** Le W1 de l'ancien code vaut 15,3 s sur G4 et 25,2 s ici
(facteur 1,65). Le chemin critique local à W48 se compose désormais de la passe unique (mur simulé 0,177 s),
du planning et des étages parallèles du catalogue (~0,05 s), de la phase régulière (7,6 s de CPU à W1, soit
~0,16 s sur 48 travailleurs) et de la chaîne sérielle de l'ordre 5 (~0,14 s). Soit ~0,53 s locale, **~0,32 s sur
G4** au lieu de 1,46 s mesurées : ×4,5 environ, mais encore ×1,3 par rapport aux 0,25 s de la v10. Ni la
contention mémoire à 48 fils ni le NUMA ne sont modélisés.

## 5. Ce qui reste, et dans quel ordre

1. **Mesurer sur G4** (session gardée, verrou commun) : A/B froid et répété `895680ff8` contre ce commit,
   modes 2047 et 16379, trois trames, u21, puis W1/W8/W48. C'est la seule qualification des gains annoncés.
2. **Chaîne sérielle par ordre** (~0,14 s locale à K = 5, désormais le premier verrou de la forêt à W48) :
   naissances en blocs parallèles (les comptes par bloc existent déjà à la classification ; tri des plateaux de
   rang égal par tâche) ; `largest_birth_run` calculé trois fois par ordre ; préchargement des états DSU des
   graines pendant la publication ; balayage vertical sans `cell_add` à chaque pas de `find`.
3. **CPU des descentes régulières** (7,6 s à W1) : préchargement par lots de la table de populations, comme le
   fait la v10 (empreintes, cases puis lignes avant de descendre ; ~10 % du CPU est de la latence mémoire) ;
   filtres F6 pour `side` et les bornes de puissance (census et juges du catalogue), puis pour la MEB.
4. **Catalogue** : F6 de `side` dans `census_and_emit` (7 % du CPU) ; le planning initial reste sériel.
5. K = 10 n'est pas mesuré ici.

## 6. Réponse à la reprise du 3 octobre

| Point de la reprise | Suite donnée |
|---|---|
| P0 catalogue : G3 avant les droites, seuil descendant | Fait. Le test de paires garde sa place. La coupe de l'union du préfixe (d > K − q) et les lignes vivantes ne changent ni juges ni émissions. Choix différent pour les compteurs : plutôt que de nouveaux compteurs de coupes, `prefixes` reste le compte logique de la voie graphe (préfixes écartés ajoutés), ce qui garde l'identité `same_work` du graphe et tous les compteurs historiques hors `region_line_*` : 120 449 590 préfixes avant et après sur 08/000000. Nouvelle porte `live_rows` sur douze nuages aléatoires (145 147 boules dont 57 174 admises au seuil). Votre `threshold_model.py` reste le juge indépendant de la coupe. |
| P0 tour : union de racines déjà trouvées | Fait (`unite_roots`), dans `regular_cell` et `cell`, avec une garde de chaîne cyclique dans `close` et le mutant `racine_courante_non_suivie`. |
| Étude du partage des faces | Non faite : la table de populations termine déjà chaque descente régulière ; à remesurer après F6. |
| P1 numérique : clés F3/F4 du tri | Fait : une clé par niveau, repli exact dans la bande, permutation identique, mutants `sort_cle_*`. |
| P1 protocole A/B froid et répété | Fait localement (reçu) ; à faire sur G4. |
| Granularité et barrières (point 5) | C1 et C2 : durées par tâche et simulation W8/W24/W48 au reçu. |
| Surcoût u21 (point 6) | ~6 % contre u18 sur l'ancien code (mesure locale isolée) ; non prioritaire. |

Portes : suite `fast` complète verte (voir le reçu) ; mutants nouveaux ou réancrés tous tués par code. Aucun
statut public changé, aucune qualification G4 revendiquée.

## 7. Tranche 2 du même jour (2026-10-03 12:32 UTC)

Ajoutée au commit suivant, sur la chaîne sérielle de la forêt et la latence mémoire des descentes ; reçu
[ecart_v10_v11_tranche2](../receipts/developpement_20261003/ecart_v10_v11_tranche2/README.md).

| Changement | Effet local mesuré |
|---|---|
| Séries de naissances composées à la classification (`BirthRuns`) au lieu de trois parcours de toutes les boules par ordre | phase des naissances à W4 (médianes) : 61 → 30, 52 → 27, 70 → 39 ms sur les trois trames |
| Préchargement en trois étages de la table de populations dans `resolve_lane` | phase régulière à W4 −2 à −9 %, −2 à −6 % à W1 |
| Pas de `find` du balayage comptés localement | dans le bruit |
| Préchargement des états DSU pendant la publication | essayé puis **retiré** : aucun gain mesurable |

Sorties identiques octet pour octet aux empreintes de la tranche 1 sur les six entrées. Les murs à W4 restent
dans le bruit de la machine (±10 %) ; le gain attendu est sur le chemin critique à W48. Nouvelle porte
`mhgp11_tower_birth_runs_composition` ; mutants `forest_cohort_capacity_short` et `forest_cohort_nonbirth_reset`
réancrés, `birth_runs_jonction_perdue` ajouté. La chaîne sérielle de l'ordre 5 vaut désormais environ
naissances 24–33, publication 65–68, balayage 36–46 ms à W1 local ; la publication (DSU par plateaux, 438 000
plateaux à K = 5) et le balayage sont les prochains verrous de cette chaîne, avec les naissances par blocs.
