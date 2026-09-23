# Préflight du protocole G4 S2 : distinguer préflight GPU et tour LiDAR GPU

23 septembre 2026, 15 h 25 UTC. Contrelecture **WIP** en lecture seule
des diffs non commis `tower_worker_v9.py`, `tower_session_v9.py` et
`tower_selftest_v9.py` du développeur sur `5577f0f2a`. Aucun GCP lancé.
Le protocole peut encore changer ; cette note ne décrit pas un reçu
publié.

**Relecture après publication (`c265a5dae`, reçu R12 sur `2059189d8`).**
R12 est un vrai reçu G4, 14/14 cas achevés dont sept GPU ; le scénario
partiel ci-dessous **ne s'y produit pas**. Le code courant recalcule et
publie `unpaired_batch_cases` pour les tours par lots achevées sans jumeau
moteur achevé ; son selftest refuse une dissimulation. Le lien CUDA, les
selftests v17 et le mutant doublon ont été corrigés. Il reste toutefois
possible, avec un plan mixte autorisé, que seul le préflight GPU réussisse,
qu'aucun cas LiDAR GPU ne s'achève et qu'un cas CPU s'achève : le worker
marque `GPU_executed` au préflight (`tower_worker_v9.py:955`), et l'hôte
le déduit du plan à réception partielle (`tower_session_v9.py:489`).
`unpaired_batch_cases` est alors vide, puisqu'aucun cas batch n'est
achevé. Ces marqueurs signifient au plus « GPU vu dans le préflight »,
pas « tour LiDAR GPU achevée ». Ce défaut de **libellé** ne retire rien
aux sept cas GPU complets de R12. Le différentiel catalogue GPU–moteur
clé par clé reste à faire ; les six paires distinctes de R12 comparent
une projection logique, pas les catalogues entiers.
Dans R12, `catalogue.euler.status=holds` ne porte que sur les degrés
jusqu'à `checkable_max_k=3` de la dimension simpliciale ; ce n'est pas
une vérification indépendante des dix niveaux de la tour.

## Faux statut positif possible dans une réception partielle

Le worker met `GPU_executed=True` dès que le **préflight de 1 500 sites**
utilise le levier GPU et réussit. L'hôte accepte ensuite une campagne
`partial` avec **une tour complète quelconque**, sans exiger une tour
LiDAR GPU complète. Enfin il met `GPU_executed=True` dès que le plan
**contient** un cas GPU, sans le lier à son issue.

Scénario accepté en l'état : préflight GPU correct ; premier cas LiDAR
GPU tué ou refusé ; cas CPU ultérieur `complete_relative`. Le reçu
partiel passe avec `FULL_executed=True`, `GPU_executed=True`, alors
qu'**aucune tour LiDAR GPU n'a abouti**. Ce n'est pas une erreur de
géométrie ; c'est une erreur de portée du marqueur et de la règle de
réception, susceptible de transformer un échec du contrat S2 en
succès de présentation. **Nuance** : le plan par défaut mutable met
actuellement GPU ON sur ses huit cas ; ce contre-exemple exige un plan
mixte personnalisé, que `collect(..., plan_raw=...)` permet. Sur le
plan tout-GPU, une réception `partial` avec au moins une complétion
implique bien une complétion GPU, mais le libellé reste mal défini.

Conserver trois états séparés : `GPU_preflight_executed`,
`GPU_LiDAR_attempted`, `GPU_LiDAR_completed`. Le dernier doit être
recalculé **depuis les issues brutes des cas** et valoir vrai seulement
si un cas `complete_relative` possède `q34_gpu_filter=true`, le backend
CUDA attendu et des objets/digests jugés. Pour un reçu annoncé comme
qualification de tour GPU, exiger au moins un tel cas (et les cas
fixés au plan si la qualification porte sur plusieurs scènes), pas
seulement le préflight. Une campagne uniquement CPU peut rester un
reçu CPU partiel ; elle ne doit pas porter `GPU_executed=true` au sens
du contrat de trame.

Ajouter au selftest causal un plan mixte où le cas GPU est tué/refusé
et un cas CPU passe, puis exiger que la réception **refuse la preuve
GPU** tout en conservant le reçu d'échec. Vérifier aussi le cas tout
GPU sans aucune complétion et le cas GPU réellement complet. Les
qualifications `cuda_g4`, FULL et <1 s doivent être liées aux cas
achevés, pas seulement aux options du plan.

## Le plan tout-GPU ne compare pas la tour au CPU

Le `default_plan()` actuel met les douze leviers à vrai dans les huit
cas. `compare_cases()` compare donc des sorties **GPU entre nombres
de workers**, pas GPU contre la référence CPU sur les mêmes octets,
K, s et options géométriques. Le préflight positif est lui aussi GPU ;
la porte S1 antérieure juge seulement les masques du filtre, pas le
catalogue ni FULL du nouveau raccord. Pour une qualification
d'exactitude de S2, inclure au moins un cas CPU batch et un cas GPU
appariés sur **la même trame**, ou une porte hors chrono qui compare
exhaustivement leurs candidats, clés, catalogue, parents et digest.
Sans cela, un reçu tout-GPU ne prouve qu'une cohérence interne de
plusieurs exécutions GPU. Le levier `q34_witness_cache=true` reste dans
le plan mais le batch le désactive par construction ; ne pas dire que
**tous** les leviers sont exercés par le préflight.

Le selftest nominal WIP exige encore `GPU_executed is False` alors que
le nouveau plan par défaut est tout-GPU ; il doit être adapté et
rejoué après commit du schéma v17. Les selftests exécutés avant commit
utilisent encore la sonde v16 du commit pour construire le snapshot,
alors que le lecteur mutable attend v17 : ils ne qualifient pas le
protocole S2 à cet instant. Un essai local du WIP montre aussi que
`Protocol.test_probe_and_time_validation` échoue sur deux mutants
hérités, `cache_queries_zero` et `cache_node_tests_zero` : ces zéros
sont maintenant **normaux** dans le batch, donc ces mutants ne sont
plus causaux. Les remplacer par des altérations réellement interdites
(cache non nul, backend ou survivants incorrects, ou
`witness_pair_queries≠expanded_pairs`) et prouver leur refus.
