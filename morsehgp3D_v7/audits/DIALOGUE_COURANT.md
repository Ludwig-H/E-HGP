# Dialogue actif avec le constructeur

11 septembre 2026, reprise sur **abc960ac**. La [coordination](COORDINATION_AUDITEURS.md) répartit les écritures ; les reçus historiques restent inchangés.

## Proposition prête à reprendre

La [canonisation par support certifié](receipts_certified_support_20260911/README.md) retire des calculs sans changer le résultat canonique. Après certification par un support positif S et confinement complet, chaque candidat de la coquille U se juge sur S seul, au plus quatre points. Si U=S, restituer directement S trié : aucune énumération supplémentaire. Ce cas ne demande aucune régularité globale.

Le helper C++ et sa preuve gardent les champs de `anchor_meb`, y compris le support et la coquille. Sur 21 cas O2/SAN : neuf retours directs et six passages d’un certificat q4 au support canonique q2 ; deux rejets exercent les préconditions. Le coût du certificat reste visible. Le raccord proposé est local : réutiliser le passage de confinement, puis appliquer le retour direct ou les tests sur S. Une simple `boundary_ball` doit encore recevoir son certificat positif ; à défaut, garder la voie complète ou le repli.

Cette réduction complète le proposeur réparé du [second auditeur](NOTE_CLAUDE_COEUR_MEB_20260911.md) et l’essai diamétral du constructeur. Son utilité sur le vrai flux reste à mesurer avec les coûts de proposition, certification et repli. Aucun changement de BallId ou de trajectoire n’est nécessaire.

La [réduction du graphe aux naissances](receipts_filtered_graph_20260911/README.md) garde sa preuve sur coupes, parents, contributions et ancres. Sa prochaine étape utile reste la comparaison C++ au calendrier et au juge T2 sur petits census, sans recommencer le modèle abstrait acquis.

## Acquis repris par le développeur

| Point clos | Preuve et portée conservées |
| --- | --- |
| Prototype MEB initial | [K7 et repli exact](receipts_meb_boundary_20260911/README.md) conservés ; cas de base réparé par le second auditeur, suivi du patch pris en charge dans abc960ac. |
| Lots et portes permanentes | Publication 324f6192, [40 CTests actifs](../receipts/full_ball_batch_active_cmake_20260911/README.md) ; lecteur contre-vérifié normal/−O. Les 65 cas callback comprennent 64 rejets et un cas positif sans requête. |
| Consommation cumulative hôte | [Deux correctifs contre-vérifiés](receipts_batch_work_20260911/README.md) ; [T2 par lots](../receipts/gpu_terminal_batch_t2_20260911/README.md) et [annotations HD](../receipts/gpu_terminal_batch_hd_20260911/README.md) relus sur leurs preuves publiées, sans exécution device par notre lecture. |
| Semis et vrai census→tour K10 | [Semis intégré](../docs/SEMIS_APRES_ECHANGE_20260911.md), [qualification K10](../docs/QUALIFICATION_TOUR_CENSUS_K10_20260911.md) ; ne plus demander le premier essai ni la porte permanente. |
| Journal incrémental et blocs 50k | [Premier raccord essayé puis non retenu](../receipts/incremental_full_trial_20260911/README.md), [qualification du second auditeur](receipts_cache_commit_20260911/README.md) conservée sur ses propres octets. |

Les [empreintes et lectures](ENTRETIEN.json) distinguent les autorités. Archive industrielle et contrats de temps de toute la tour restent ouverts ; aucun nouveau temps n’est déduit des compteurs locaux. GCP non utilisé par cet audit.
