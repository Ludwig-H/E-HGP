# Dialogue actif avec le constructeur

11 septembre 2026. Priorité utilisateur : les objets permettant de paralléliser
**toutes** les étapes coûteuses de la tour. La [coordination](COORDINATION_AUDITEURS.md)
répartit les écritures.

## Partager les objets déjà validés

Le [nouvel audit](receipts_prepared_catalogue_20260911/README.md) répond aux
questions de préparation et de liaison. Le premier delta sûr retourne les
métadonnées du validateur, aujourd’hui détruites avant Atlas : ordre par clé,
ordre exact stable, programmes et ShellTable extras. Une fabrique commune
peut garder tous les contrôles existants et servir ses vues à Builder,
Atlas et Geometry. Elle ne doit pas retenir un Builder complet pour cela.

Les rangs entiers remplacent les comparaisons rationnelles répétées des
programmes après vérification des représentants stricts et de chaque liaison
BallId→rang→niveau. Préserver égalités, ordre des clés, couverture des cellules,
fractions brutes et permutation des masques PointId/BallData. Le partage
ne transforme pas la validation locale en preuve de complétude productrice.
Le constructeur a lu et accepté ce delta ; il le séparera de son premier
raccord dense pour conserver un différentiel clair.

## Semis partagés, état privé par worker

Une liaison exhaustive une fois par owner/génération/K peut produire un
objet opaque consommé par toutes les fenêtres, avec stockage vivant et
fermé aux mutations. Puisque chaque boule fournit un semis complet au seul
ordre |I|+|U|, Σ S_K≤B pour la tour. L’adapter actuel paie au contraire
Σ J_K·S_K comparaisons de liaison. Construction, tri et transferts restent
à compter ; il ne s’agit pas d’un gain de temps déjà mesuré.

Le Context actuel porte des buffers, un compteur batch et un état d’échec
mutables. Le pool partage seulement les objets immuables, avec espaces de
travail distincts et couverture explicite des résultats. Borner ensemble
fenêtres en vol et résultats terminés en attente évite une accumulation
si une ancienne fenêtre tarde. Le constructeur confirme que ses workers
ne recevront ni φ ni le DSU et ne partageront pas ce Context concurremment.

Pour un mémo de terminales, garder aussi le seuil sous lequel la requête
était certifiée. Réemploi direct sous un seuil supérieur ou égal ; en dessous,
**miss et repli**, pas rejet automatique. Le seul niveau de la terminale ne
suffit pas : la MEB initiale peut être plus haute. Le paquet conserve un
témoin exact à quatre points, et un seuil intermédiaire valide qui nécessite
le repli. Ce témoin d’API ne prétend pas être une occurrence FULL authentique.

## Qualifications précédentes closes

Le [réducteur ordonné](../receipts/ordered_streaming_20260911/README.md) et son
triplet 8k/16k/32k sont publiés et contre-lus, lecteurs normal/−O PASS.
Une visite par R et zéro retri de hubs sont établis ; le compact natif reste
exécuté. Les temps sous charge et le RSS sont correctement bornés. Le refus
LSan initial est conservé séparément ; aucune mesure n’est réattribuée.

La [preuve de contraction directe](receipts_birth_stream_20260911/README.md)
a été contre-exécutée par le développeur. Son draft C++ dense est favorable
en lecture ; son premier raccord et son pool CPU sont en préparation.
Les preuves d’[export historique](receipts_historical_export_20260911/README.md),
de [composition](receipts_composable_msf_20260911/README.md) et de
[décomposition de la tour](receipts_parallel_objects_20260911/README.md)
restent acquises à leur portée, sans redemander leurs premières gates.

## Acquis repris par le développeur

| Point clos | Preuve et portée conservées |
| --- | --- |
| Prototype MEB initial | [K7 et repli exact](receipts_meb_boundary_20260911/README.md) conservés ; cas de base réparé par le second auditeur, suivi du patch pris en charge dans abc960ac. |
| Lots et portes permanentes | Publication 324f6192, [40 CTests actifs](../receipts/full_ball_batch_active_cmake_20260911/README.md) ; lecteur contre-vérifié normal/−O. Les 65 cas callback comprennent 64 rejets et un cas positif sans requête. |
| Consommation cumulative hôte | [Deux correctifs contre-vérifiés](receipts_batch_work_20260911/README.md) ; [T2 par lots](../receipts/gpu_terminal_batch_t2_20260911/README.md) et [annotations HD](../receipts/gpu_terminal_batch_hd_20260911/README.md) relus sur leurs preuves publiées, sans exécution device par notre lecture. |
| Semis et vrai census→tour K10 | [Semis intégré](../docs/SEMIS_APRES_ECHANGE_20260911.md), [qualification K10](../docs/QUALIFICATION_TOUR_CENSUS_K10_20260911.md) ; ne plus demander le premier essai ni la porte permanente. |
| Journal incrémental et blocs 50k | [Premier raccord essayé puis non retenu](../receipts/incremental_full_trial_20260911/README.md), [qualification du second auditeur](receipts_cache_commit_20260911/README.md) conservée sur ses propres octets. |

Les [empreintes et lectures](ENTRETIEN.json) distinguent les autorités. Archive industrielle et contrats de temps de toute la tour restent ouverts ; aucun nouveau temps n’est déduit des compteurs locaux. GCP non utilisé par cet audit.
