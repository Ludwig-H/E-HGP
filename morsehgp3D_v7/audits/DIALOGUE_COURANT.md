# Dialogue actif avec le constructeur

11 septembre 2026, après **e3903b2a**. Priorité : les objets permettant de
paralléliser toutes les étapes coûteuses. Le développeur a accepté le delta
de rangs ci-dessous comme étape séparée des sources déjà figées. La [coordination](COORDINATION_AUDITEURS.md)
répartit les écritures.

## Prochain delta : conserver les gardes avec des rangs entiers

La [preuve publiée](receipts_prepared_catalogue_20260911/README.md), §2,
porte sur toutes les paires de boules du catalogue. Une fois les niveaux
représentants strictement ordonnés et **chaque liaison BallId→rang→niveau
certifiée**, la comparaison exacte des niveaux a le même signe que celle
des rangs. Cela couvre les plateaux et les fractions brutes équivalentes.
Le propriétaire et sa génération doivent rester identiques et immuables.

Appliquer cette preuve aux gardes du scatter et des semis initiaux :

| Garde actuelle | Prédicat entier équivalent |
| --- | --- |
| `first_consumer` | `rank(leader) <= rank(consumer)` |
| `terminal_admission_strict` | `target < balls.size() && admitted(target, K) && rank(target) < rank(consumer)` |
| `seed_not_strict` | `rank(seed_target) < rank(leader)` |

`admitted` représente ici le contrôle `atlas.block_id(target, K).has_value()`
existant. Garder cet ordre et le court-circuit **avant** l’accès au rang de
la terminale. K1 garde le contrôle de premier consommateur ; sa terminale
est un point, sans comparaison de rang BallId. Les semis conservent leur
liaison exhaustive au propriétaire et à K.

Les captures [ordonnées scellées](../receipts/ordered_streaming_20260911/README.md)
et le source **fb9f0c0c** donnent le travail logique suivant. Chaque occurrence
paye `first_consumer` ; chaque occurrence K≥2 paye aussi la garde terminale.
Ainsi le scatter évalue `2R − R_K1` comparaisons rationnelles ; chaque hit
de semis initial ajoute une comparaison distincte :

| n, s8, K1..10 | Scatter | Semis initiaux | Total substituable |
| ---: | ---: | ---: | ---: |
| 8 000 | 20 852 874 | 5 054 875 | 25 907 749 |
| 16 000 | 43 775 268 | 11 997 933 | 55 773 201 |
| 32 000 | 90 662 398 | 26 901 500 | 117 563 898 |

Ce calcul est reproduit depuis les compteurs scellés dans l’[entretien](ENTRETIEN.json).
Il compte des prédicats du source sur les exécutions réussies, sans les
assimiler à des instructions machine ou à un gain chronométré. Il ne retire
aucune garde et ne réduit aucun appel MEB. Les boules dynamiques initiales
et intermédiaires de Geometry n’ont pas automatiquement de rang certifié :
leurs contrôles exacts restent nécessaires. La contre-fixture du §4 du
paquet montre pourquoi le niveau de la seule terminale ne suffit pas.

## Workers : avancer sur le raccord déjà préparé

La lecture du pool persistant et du raccord dense est favorable : résultats
séparés par leader, scratch privé, barrière avant scatter, puis consommation
de φ/DSU dans l’ordre source. Un pool sert tous les K, avec une seule fenêtre
active. Le tri et la réduction restent séquentiels. Les hashes et la lecture
de la capture dense O2, 114 census/456 essais, sont consignés séparément ;
aucune exécution C++ supplémentaire par cet audit.

Une amélioration du runner est utile : borner les commandes de
`record_workers.py` par un timeout et préserver les captures échouées.
Les fixtures à latch rendent détectable une perte de participation seulement
si le runner sait aussi terminer un test bloqué. Aucun tel blocage observé.

## Décisions déjà prises

Le [plan préparé, les rangs liés et les semis par propriétaire/K](receipts_prepared_catalogue_20260911/README.md)
sont acceptés comme delta séparé du premier reçu dense : validation commune
conservée, métadonnées partagées, état mutable privé. Le réemploi d’une
terminale garde son seuil certifié et un repli sous ce seuil. Ces arguments
et leurs contre-fixtures ne sont plus répétés ici.

Le [raccord ordonné et son triplet](../receipts/ordered_streaming_20260911/README.md),
la [contraction directe](receipts_birth_stream_20260911/README.md),
l’[export](receipts_historical_export_20260911/README.md) et la
[composition](receipts_composable_msf_20260911/README.md) gardent leurs preuves
propres. Leurs premières gates ne sont pas redemandées.

## Acquis repris par le développeur

| Point clos | Preuve et portée conservées |
| --- | --- |
| Prototype MEB initial | [K7 et repli exact](receipts_meb_boundary_20260911/README.md) conservés ; cas de base réparé par le second auditeur, suivi du patch pris en charge dans abc960ac. |
| Lots et portes permanentes | Publication 324f6192, [40 CTests actifs](../receipts/full_ball_batch_active_cmake_20260911/README.md) ; lecteur contre-vérifié normal/−O. Les 65 cas callback comprennent 64 rejets et un cas positif sans requête. |
| Consommation cumulative hôte | [Deux correctifs contre-vérifiés](receipts_batch_work_20260911/README.md) ; [T2 par lots](../receipts/gpu_terminal_batch_t2_20260911/README.md) et [annotations HD](../receipts/gpu_terminal_batch_hd_20260911/README.md) relus sur leurs preuves publiées, sans exécution device par notre lecture. |
| Semis et vrai census→tour K10 | [Semis intégré](../docs/SEMIS_APRES_ECHANGE_20260911.md), [qualification K10](../docs/QUALIFICATION_TOUR_CENSUS_K10_20260911.md) ; ne plus demander le premier essai ni la porte permanente. |
| Journal incrémental et blocs 50k | [Premier raccord essayé puis non retenu](../receipts/incremental_full_trial_20260911/README.md), [qualification du second auditeur](receipts_cache_commit_20260911/README.md) conservée sur ses propres octets. |

Les [empreintes et lectures](ENTRETIEN.json) distinguent les autorités. Archive industrielle et contrats de temps de toute la tour restent ouverts ; aucun nouveau temps n’est déduit des compteurs locaux. GCP non utilisé par cet audit.
