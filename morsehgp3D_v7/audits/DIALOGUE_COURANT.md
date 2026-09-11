# Dialogue actif avec le constructeur

11 septembre 2026. Priorité utilisateur : les objets permettant de paralléliser
**toutes** les étapes coûteuses de la tour, au-delà des seules MEB.
La [coordination](COORDINATION_AUDITEURS.md) répartit les écritures.

## Compatibilité physique : une solution constructive prête

Le [témoin d’encodage historique](receipts_historical_export_20260911/README.md)
reproduit la banque, les indices, les nœuds, les contributions, les niveaux
bruts et les verticales attendus par `same_payload`, après les horizontales
indépendantes. O2/SAN : dix entrées, soixante ordres, 2 184 nœuds et
1 390 contributions ; trois mutations réfutées sur les entrées géométriques.
Le piège du minimum excluant les blocs silencieux conserve un témoin abstrait
séparé ; ce cas n’est pas exercé par le corpus géométrique.

**La clé de première utilisation n’est pas seulement `(K,niveau,BallKey)`.**
À chaque date, grouper les rôles par ancre fermée, réduire le minimum BallKey
sur TOUS les blocs du groupe, puis parcourir groupe/bloc dans cet ordre.
Le minimum de la clé `(K,rang,min_groupe,rangBallKey)` par BallId fournit
l’ordre historique des lignes de banque, après les singletons PointId.
Construire un seul propriétaire partagé après cette réduction suffit.
Un second minimum sur le lot K entier conserve son premier `ExactLevel` brut.
Les nœuds se numérotent par groupe de création, puis les références se remappent.

Cela répond au diagnostic physique du [second auditeur](NOTE_CLAUDE_DECOUPE_TOUR_20260911.md),
§4ter, sans remplacer ses portes par un digest. Le coût des consultations,
tris et copies reste à payer. La gate est séquentielle et réutilise les
verticales du raccord pour transporter leurs indices ; aucun gain ni nouvelle
qualification géométrique n’est déduit.

Le [premier raccord atlas→graphes→FULL](../receipts/atlas_graph_full_20260911/README.md)
est publié dans 679f4a6f. Lecteurs normal/−O et contrelecture des corps clos :
114 census, banque unique, bijection d’identités natives, contributions et
verticales, routes MSF directes et projetées. Cette première demande est close.
Sa nouvelle convention est correctement déclarée ; les indices historiques
ci-dessus sont une option de compatibilité, pas un défaut déjà revendiqué.

Le constructeur prépare maintenant la vraie consommation par fenêtres sans
requests[R_K], targets[R] ni graphe complet, en conservant catalogue, masques,
φ et marques. La suite utile est son contrôle des terminales puis Builder/T2,
et la mesure du travail répété et de la résidence. Les preuves de [compression
par lots](receipts_composable_msf_20260911/README.md) et de [décomposition de toute
la tour](receipts_parallel_objects_20260911/README.md) restent acquises ; leurs
premiers prototypes ne sont plus à redemander.

## Acquis repris par le développeur

| Point clos | Preuve et portée conservées |
| --- | --- |
| Prototype MEB initial | [K7 et repli exact](receipts_meb_boundary_20260911/README.md) conservés ; cas de base réparé par le second auditeur, suivi du patch pris en charge dans abc960ac. |
| Lots et portes permanentes | Publication 324f6192, [40 CTests actifs](../receipts/full_ball_batch_active_cmake_20260911/README.md) ; lecteur contre-vérifié normal/−O. Les 65 cas callback comprennent 64 rejets et un cas positif sans requête. |
| Consommation cumulative hôte | [Deux correctifs contre-vérifiés](receipts_batch_work_20260911/README.md) ; [T2 par lots](../receipts/gpu_terminal_batch_t2_20260911/README.md) et [annotations HD](../receipts/gpu_terminal_batch_hd_20260911/README.md) relus sur leurs preuves publiées, sans exécution device par notre lecture. |
| Semis et vrai census→tour K10 | [Semis intégré](../docs/SEMIS_APRES_ECHANGE_20260911.md), [qualification K10](../docs/QUALIFICATION_TOUR_CENSUS_K10_20260911.md) ; ne plus demander le premier essai ni la porte permanente. |
| Journal incrémental et blocs 50k | [Premier raccord essayé puis non retenu](../receipts/incremental_full_trial_20260911/README.md), [qualification du second auditeur](receipts_cache_commit_20260911/README.md) conservée sur ses propres octets. |

Les [empreintes et lectures](ENTRETIEN.json) distinguent les autorités. Archive industrielle et contrats de temps de toute la tour restent ouverts ; aucun nouveau temps n’est déduit des compteurs locaux. GCP non utilisé par cet audit.
