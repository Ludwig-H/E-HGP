# Dialogue actif avec le constructeur

11 septembre 2026. Priorité utilisateur : les objets permettant de paralléliser
**toutes** les étapes coûteuses de la tour, au-delà des seules MEB.
La [coordination](COORDINATION_AUDITEURS.md) répartit les écritures.

## Prochain raccord : fenêtres géométriques et certificats composables

**Les arêtes peuvent être comprimées avant de toutes les avoir résolues.**
La [nouvelle preuve](receipts_composable_msf_20260911/README.md) conserve toutes
les coupes par MSF de fenêtres, puis MSF de leurs unions. L’ordre d’arrivée
n’impose pas l’ordre des poids. Naissances isolées, admissions, contributions
et références verticales restent dans leurs tables ; les multifusions sont
reconstruites depuis le certificat final.

Pour le premier raccord, émettre directement les arêtes B–T du flux de
résolution et les comprimer sur les hubs ; garder un pivot stable par hub.
Après calcul de φ, projeter le certificat vers les naissances et reprendre
sa MSF. Cette route évite de stocker simultanément R clés développées et R
résultats terminaux. Si A/L le justifie, la variante en deux passes résout
les A−L pivots d’abord et les R−A+L occurrences restantes ensuite, pour
comprimer directement sur L sommets.

Le témoin normal/−O couvre dix graphes de composition et trois de projection :
198 compositions, 5 152 comparaisons de partitions, cinq mutants. Une fixture
montre deux certificats différents après projection, avec la même multifusion
ternaire. Le numérotage public ne doit donc pas dépendre du certificat retenu.
Les compteurs sont des buffers logiques du modèle ; aucun gain RSS ou temps
n’est mesuré. Une réduction parallèle retenant tous les résumés peut rester
O(R) en mémoire ; borner tâches actives et certificats en attente.

Le constructeur a publié dans f2bea998 ses [objets](../docs/OBJETS_PARALLELES_TOUR_20260911.md),
son [calendrier/HLD](../receipts/filtered_calendar_20260911/README.md) et son
[atlas](../receipts/rank_atlas_20260911/README.md). Les quatre lecteurs passent
normal/−O. Contrelecture HLD favorable : 307 500 requêtes CPU1/2/4 dans les
captures O2/SAN, stockage linéaire. L’atlas est comparé à `visit_block` sur
13 vrais census ; les deux ordres de masques sont explicitement traduits.
Cette première qualification de briques est close. Le raccord atlas→résolutions→
graphe→FULL est en préparation privée pour comparaison à Builder/T2, avec
contributions et verticales. Cette première référence garde les tableaux
complets ; la consommation fenêtrée vient ensuite.

La [décomposition précédente](receipts_parallel_objects_20260911/README.md)
reste acquise : horizontales indépendantes puis toutes les verticales par
requêtes historiques, sans nouvelle MEB verticale. La première fréquence
U=S du [second auditeur](NOTE_CLAUDE_COEUR_MEB_20260911.md) est publiée ; aucune
redemande de ces résultats déjà documentés.

## Acquis repris par le développeur

| Point clos | Preuve et portée conservées |
| --- | --- |
| Prototype MEB initial | [K7 et repli exact](receipts_meb_boundary_20260911/README.md) conservés ; cas de base réparé par le second auditeur, suivi du patch pris en charge dans abc960ac. |
| Lots et portes permanentes | Publication 324f6192, [40 CTests actifs](../receipts/full_ball_batch_active_cmake_20260911/README.md) ; lecteur contre-vérifié normal/−O. Les 65 cas callback comprennent 64 rejets et un cas positif sans requête. |
| Consommation cumulative hôte | [Deux correctifs contre-vérifiés](receipts_batch_work_20260911/README.md) ; [T2 par lots](../receipts/gpu_terminal_batch_t2_20260911/README.md) et [annotations HD](../receipts/gpu_terminal_batch_hd_20260911/README.md) relus sur leurs preuves publiées, sans exécution device par notre lecture. |
| Semis et vrai census→tour K10 | [Semis intégré](../docs/SEMIS_APRES_ECHANGE_20260911.md), [qualification K10](../docs/QUALIFICATION_TOUR_CENSUS_K10_20260911.md) ; ne plus demander le premier essai ni la porte permanente. |
| Journal incrémental et blocs 50k | [Premier raccord essayé puis non retenu](../receipts/incremental_full_trial_20260911/README.md), [qualification du second auditeur](receipts_cache_commit_20260911/README.md) conservée sur ses propres octets. |

Les [empreintes et lectures](ENTRETIEN.json) distinguent les autorités. Archive industrielle et contrats de temps de toute la tour restent ouverts ; aucun nouveau temps n’est déduit des compteurs locaux. GCP non utilisé par cet audit.
