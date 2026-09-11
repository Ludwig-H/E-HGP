# Dialogue actif avec le constructeur

11 septembre 2026. Priorité utilisateur : les objets permettant de paralléliser
**toutes** les étapes coûteuses de la tour, au-delà des seules MEB.
La [coordination](COORDINATION_AUDITEURS.md) répartit les écritures.

## Proposition prête à raccorder

Le [nouveau dossier](receipts_parallel_objects_20260911/README.md) donne un
contrat de tableaux : catalogue partagé des boules, rôles `(K,B)`, requêtes
`(K,F)`, graphe filtré sur naissances, arbre de multifusions, marques datées.
Sous régularité, chaque boule n’a que deux rôles à deux ordres voisins.
Les horizontales K sont indépendantes une fois leurs terminales connues.

**Les verticales sortent elles aussi de la boucle K.** Pour chaque nœud,
choisir une naissance descendante, puis chercher son image à coupe fermée
dans l’arbre horizontal inférieur ; nul besoin des verticales de cet ordre
inférieur. Les gardes de naturalité se calculent ensuite indépendamment.
Un index de chaînes lourdes donne ces requêtes en O(log N) avec stockage O(N).

Le témoin Python normal/−O compare sauts et chaînes lourdes au BFS : 11 cas,
234 coupes, 5 178 requêtes, 12 nœuds verticaux et cinq mutants. Il emploie une
construction séquentielle d’arbre comme juge ; il ne livre pas un backend
parallèle. RCTT et PANDORA sont des algorithmes publiés à considérer pour
MSF→dendrogramme, avec contraction des raffinements binaires de même niveau.
La profondeur de sortie n’impose donc pas autant de rondes de calcul.

Le même dossier expose le sweep q4 en deux scans exacts de groupes, les tuiles
WSPD implicites, le tri indirect des candidats, la compaction du census et
l’export par scans. Le constructeur prépare maintenant le prototype MSF et
l’atlas de rangs/blocs ; notre contrelecture confirme sa suppression de la
boucle K sans nouvelle MEB verticale. La confrontation aux petits census/T2
et au calendrier précède les mesures de débit et de résidence.

La [canonisation par support certifié](receipts_certified_support_20260911/README.md)
reste acquise. Le second auditeur a publié dans b5271aad un premier histogramme
sur le [flux réel](NOTE_CLAUDE_COEUR_MEB_20260911.md) ; ne plus demander cette
première mesure. Ses observations ne deviennent pas une généralité ni une
mesure du raccourci intégré. Notre priorité courante est l’architecture de la tour.

## Acquis repris par le développeur

| Point clos | Preuve et portée conservées |
| --- | --- |
| Prototype MEB initial | [K7 et repli exact](receipts_meb_boundary_20260911/README.md) conservés ; cas de base réparé par le second auditeur, suivi du patch pris en charge dans abc960ac. |
| Lots et portes permanentes | Publication 324f6192, [40 CTests actifs](../receipts/full_ball_batch_active_cmake_20260911/README.md) ; lecteur contre-vérifié normal/−O. Les 65 cas callback comprennent 64 rejets et un cas positif sans requête. |
| Consommation cumulative hôte | [Deux correctifs contre-vérifiés](receipts_batch_work_20260911/README.md) ; [T2 par lots](../receipts/gpu_terminal_batch_t2_20260911/README.md) et [annotations HD](../receipts/gpu_terminal_batch_hd_20260911/README.md) relus sur leurs preuves publiées, sans exécution device par notre lecture. |
| Semis et vrai census→tour K10 | [Semis intégré](../docs/SEMIS_APRES_ECHANGE_20260911.md), [qualification K10](../docs/QUALIFICATION_TOUR_CENSUS_K10_20260911.md) ; ne plus demander le premier essai ni la porte permanente. |
| Journal incrémental et blocs 50k | [Premier raccord essayé puis non retenu](../receipts/incremental_full_trial_20260911/README.md), [qualification du second auditeur](receipts_cache_commit_20260911/README.md) conservée sur ses propres octets. |

Les [empreintes et lectures](ENTRETIEN.json) distinguent les autorités. Archive industrielle et contrats de temps de toute la tour restent ouverts ; aucun nouveau temps n’est déduit des compteurs locaux. GCP non utilisé par cet audit.
