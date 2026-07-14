# Convention de sérialisation MorseHGP3D v2

Le [schéma JSON principal](../../schemas/morsehgp3d-contract-v2.schema.json) fige la forme corrective des contrats de données de la phase 0. Ce document fixe les invariants sémantiques que JSON Schema ne peut pas exprimer lisiblement. Il complète la [spécification mathématique](../SPECIFICATION_MORSEHGP3D.md), le [contrat de reconstruction M.1](M1_RECONSTRUCTION.md) et le [plan de tests](../TEST_PLAN_MORSEHGP3D.md); il ne remplace aucune preuve. Le [schéma v1](../../schemas/morsehgp3d-contract-v1.schema.json) reste une archive lisible, mais sa base `reduced_manuscript_theorem_5` ne peut plus autoriser une publication scientifique.

## 1. Portée et identité du contrat

- phase : `0`;
- backend : aucun calcul imposé par le schéma; les valeurs sérialisables sont `reference_cpu` et `cuda_g4`;
- profils : `hgp_reduced` et `full_pi0`;
- modes : `certified` et `budgeted`;
- identifiant court : `morsehgp3d-contract-v2`;
- version de chaque enregistrement : `schema_version = "2.0.0"`;
- dialecte : JSON Schema Draft 2020-12;
- type racine : `MorseHGP3DResult`;
- types indépendants : toutes les entrées de `$defs` demandées à la section 5 de la feuille de route.

Chaque objet sérialisable possède sa propre version, y compris lorsqu'il est imbriqué. Un extrait, un run externe ou un checkpoint conserve donc une version interprétable sans dépendre de son conteneur d'origine.

## 2. Politique de compatibilité et champs inconnus

Tous les objets scientifiques, certificats, budgets, checkpoints et benchmarks ont `additionalProperties: false`. Un lecteur v2 doit refuser tout champ inconnu avant d'utiliser le résultat. Il est interdit d'ignorer silencieusement un champ susceptible de modifier une unité, un ordre, un statut, une fermeture ou une identité canonique.

La compatibilité suit ces règles :

- une version corrective peut préciser la documentation ou renforcer un test qui était déjà requis par la sémantique v2; elle ne change ni champ ni octet canonique valide;
- une version mineure peut ajouter un champ optionnel ou une valeur non critique, mais un producteur ne l'émet qu'après négociation; un lecteur strict plus ancien a le droit de la refuser;
- toute suppression, tout renommage, toute modification d'unité, toute nouvelle interprétation d'un champ requis ou tout relâchement d'une porte `exact` exige une version majeure;
- un producteur ne réétiquette jamais un objet d'une autre version en `2.0.0`;
- une migration valide l'ancien objet contre son ancien schéma, transforme explicitement ses champs, puis valide le nouvel objet; une simple copie avec champs supprimés n'est pas une migration.

Les mots-clés `x-canonical-order` appartiennent au schéma, pas aux instances. Ils n'ouvrent aucun espace d'extension pour les données.

## 3. JSON canonique et nombres

La sérialisation canonique utilisée par les tests de phase 0 est UTF-8, sans clé dupliquée, sans valeur non finie, avec les clés d'objet triées, sans espace non significatif et avec les littéraux JSON minuscules. La fonction de référence est `canonical_json` dans [`tools/check_contracts.py`](../../tools/check_contracts.py). Les implémentations C++ et CUDA devront passer le même corpus octet par octet avant de produire des identifiants de contenu.

Les conventions numériques sont les suivantes :

| valeur | représentation | règle canonique |
|---|---|---|
| entier d'index, compte ou nombre d'octets | entier JSON | compris entre 0 et 9 007 199 254 740 991 inclus |
| grand entier signé | chaîne décimale | motif `^(0\|-?[1-9][0-9]*)$`; ni `+`, ni zéro initial, ni `-0` |
| grand entier non négatif | chaîne décimale | motif `^(0\|[1-9][0-9]*)$` |
| grand entier positif | chaîne décimale | motif `^[1-9][0-9]*$` |
| coordonnée d'entrée | 16 chiffres hexadécimaux minuscules | bits IEEE-754 binary64 exacts, sans préfixe `0x`, valeur finie obligatoire |
| durée | nombre JSON | secondes SI, finies et non négatives |
| mémoire ou stockage | entier JSON | octets exacts, jamais Mo ou Mio implicites |

`ExactRational3` stocke quatre grands entiers homogènes. Son dénominateur est strictement positif et le plus grand commun diviseur des valeurs absolues des trois numérateurs et du dénominateur vaut un. `ExactLevel` suit la même règle avec un numérateur non négatif. Le niveau est toujours un rayon au carré dans l'unité `input_coordinate_unit_squared`. Deux niveaux se comparent par produit croisé exact, jamais par conversion flottante ou epsilon.

Une approximation présente dans `VertexWitness.approximation_bits` ne participe ni à l'identité, ni à une décision certifiante. Les contraintes liantes et la position rationnelle homogène sont normatives.

## 4. Unités

`InputSemantics.coordinate_unit` nomme l'unité physique ou logique de l'entrée avec un identifiant ASCII stable, par exemple `metre` ou `unitless`. Les autres champs utilisent des marqueurs fermés :

- coordonnée ou centre : `input_coordinate_unit`;
- niveau public, rayon carré et poids de forêt : `input_coordinate_unit_squared`;
- taille mémoire, scratch, sortie et checkpoint : `byte`;
- temps et budget temporel : `second`;
- puissance GPU : watt, indiqué par le suffixe `_w`;
- température GPU : degré Celsius, indiqué par le suffixe `_c`;
- rang, ordre, profondeur, multiplicité, nombre de bras, compteur de prédicat et nombre de cellules : sans dimension.

Une similarité éventuelle est enregistrée dans `InputSemantics.similarity_transform`. Lorsque `applied=false`, l'échelle vaut exactement un et la translation est exactement nulle. Une transformation non isométrique qui ne correspond pas à ce contrat est refusée. `perturbation_policy=none` est le chemin scientifique normal; `explicit_symbolic_diagnostic` décrit un calcul de diagnostic et ne permet aucune promotion de l'entrée originale.

## 5. Indexation et tableaux positionnels

- `point_id`, `source_indices`, `step_index`, `source_chunk_index`, `sequence_number`, `repetition_index` et `star_pivot_facet_index` sont indexés à partir de zéro.
- Un ordre `k`, un rang fermé et `birth_order` ou `saddle_order` sont indexés à partir de un.
- Une profondeur de parent est indexée à partir de zéro. Pour `n=1`, `m_star=0` est la sentinelle contractuelle « aucune cascade lancée ».
- `catalog_complete_by_rank[r-1]` décrit le rang fermé `r` et possède exactement `s_max` entrées.
- `attachments_complete_by_order[k-1]`, `gamma_complete_by_order[k-1]` et `batches_complete_by_order[k-1]` décrivent l'ordre `k` et possèdent exactement `k_eff` entrées.
- `forests` contient exactement les ordres 1 à `k_eff`, en ordre croissant, pour toute sortie hiérarchique.
- `vertical_maps` contient exactement les sources 2 à `k_eff`; chaque cible vaut `source_order-1`.

Les formules vérifiées sont `k_eff = min(k_max, input_point_count)` et `s_max = min(k_eff + 1, input_point_count)`. Pour plus d'un point, `m_star = min(k_eff - 1, input_point_count - 2)`; le cas singleton suit la sentinelle précédente.

## 6. Ordre canonique des points et des collections

Avant attribution des `point_id`, chaque zéro signé est remplacé par le zéro positif. Pour un mot binary64 non signé `b`, la clé d'ordre total est le complément bit à bit de `b` si son bit de signe vaut un, et `b` avec son bit de signe basculé sinon. Les sites finis se trient lexicographiquement par les trois clés `x`, `y`, `z`, puis par le plus petit `source_index`. Cette règle est indépendante de l'ordre d'arrivée des threads.

Des coordonnées identiques sont soit refusées, soit agrégées selon `duplicate_policy`. Une agrégation conserve `source_indices` triés, sans doublon, et exige `multiplicity` égal à leur longueur. Aucun jitter n'est permis.

L'extension `x-canonical-order` se lit ainsi :

- `ascending` : valeurs scalaires, identifiants ou tuples numériques en ordre croissant;
- `lexicographic` : ordre lexicographique de la sérialisation JSON canonique complète;
- `by:field` : ordre croissant de la valeur du champ nommé.

Tout tableau représentant un ensemble porte aussi `uniqueItems: true`. Les tuples à ordre intrinsèque ne sont pas triés : `coordinate_bits` reste `(x,y,z)`, `barycentric_signs` suit `minimal_support_ids`, les tableaux de complétude sont positionnels, et `descent_path` conserve l'ordre causal.

## 7. Identifiants canoniques

Sauf `point_id`, tout champ terminé par `_id` et déclaré `CanonicalId` contient 64 chiffres hexadécimaux minuscules, soit un SHA-256. Le préfixe de domaine ASCII `MorseHGP3D/v2/<Type>/` précède la sérialisation canonique de la projection d'identité. Le changement majeur invalide volontairement les identifiants v1 : une migration recalcule tous les identifiants et toutes leurs références, même lorsque l'objet scientifique paraît inchangé. Dans le domaine v2, l'identifiant lui-même, toute clé `schema_version`, les temps, les compteurs, les approximations flottantes, l'environnement et l'ordre d'arrivée sont exclus sauf pour les objets opérationnels qui les mesurent explicitement.

| type | projection d'identité minimale |
|---|---|
| `VertexWitness` | position rationnelle exacte, contraintes liantes, dimension affine, marque de frontière artificielle et classe de dégénérescence; approximation et étage de calcul exclus |
| `CriticalEvent` | intérieur, shell complet, support minimal, centre rationnel et niveau exact |
| `GabrielHyperedge` | `event_id`, ordre, simplexe, facettes, bras stricts et niveau exact |
| `GammaCoface` | ordre, simplexe, toutes ses facettes et niveau exact; aucune dépendance à un événement critique |
| `Attachment` | exactement `(event_id, order, removed_shell_id)` |
| `EqualLevelBatch` | ordre, niveau exact, `event_ids`, `gamma_coface_ids`, `hyperedge_ids` et `attachment_ids` complets |
| `MergeNode` | ordre de sa forêt, profil, nature, niveau, enfants, événements et `batch_id` |
| `MergeForest` | ordre, profil, sémantique, nœuds, racines et journal de couverture |
| `VerticalMap` | ordres source/cible et affectations canoniques |
| `CanonicalCellCertificate` | profondeur, label, contraintes, incidences actives et identifiants/projections des témoins exacts |
| `MorseHGP3DResult` | hash d'entrée, profil, sémantique et identifiants des objets scientifiques; `run_id`, backend, mode et diagnostics sont exclus afin de comparer CPU et GPU |
| objets de run, checkpoint ou benchmark | manifeste opérationnel complet explicitement mesuré |

Une implémentation peut calculer ces hashes après un tri externe; elle ne peut jamais utiliser un compteur atomique, un index de bloc ou une adresse mémoire comme identité publique. Une collision de hash est vérifiée par comparaison de la projection complète.

## 8. Invariants par famille de types

| type | invariant principal |
|---|---|
| `InputSemantics` | bits finis, unité explicite, politiques de doublons et perturbation déclarées, hash de l'entrée canonique |
| `CertifiedPoint3` | point canonique exact; `multiplicity` égale le nombre de sources agrégées |
| `ExactRational3`, `ExactLevel` | dénominateur positif, fraction réduite, unité fermée |
| `VertexWitness` | contraintes liantes rejouables; l'approximation ne certifie rien |
| `CriticalEvent` | intérieur, shell et support disjoints/cohérents; `closed_rank` égale la taille de l'intérieur et du shell |
| `GabrielHyperedge` | ordre `k`, simplexe de taille `k+1`, toutes ses facettes de taille `k`, bras stricts issus du shell |
| `GammaCoface` | toute coface exhaustive, Gabriel ou non; affectation unique à son lot exact |
| `Attachment` | identité par événement, ordre et point du shell retiré; chaque étape de descente diminue exactement le niveau |
| `EqualLevelBatch` | un seul ordre et niveau exact; état pré-lot figé et état post-lot après toutes les unions |
| `MergeForest` | niveaux non décroissants, multifusions non binarisées, croissance `q=1` dans `coverage_log` sans faux nœud |
| `VerticalMap` | cible adjacente unique, aucun échec de naturalité pour une sortie exacte |
| `PartialScope` | objets clos énumérés positivement, loci ouverts explicites, aucune assertion d'absence |
| `FragmentHint` | proposition seulement; ne ferme jamais une cellule |
| `CanonicalCellCertificate` | fermeture de la reconstruction canonique, file vide, sommets et incidences croisées tous certifiés |
| `BudgetPolicy` | limites en octets/secondes ou `null` explicitement illimité; aucun downgrade de `require_exact` |
| `BudgetSnapshot` | limite, utilisé, réservé et restant cohérents à une frontière transactionnelle |
| `CheckpointManifest` | artefacts checksummés, aucun lot égal partiellement appliqué, publication par renommage atomique durable |
| `BenchmarkRecord` | une répétition brute et son payload; temps, ressources, configuration et sécurité GCP séparés du statut scientifique |
| `RunCertificate` | toutes les portes permettant de calculer le statut public sans consulter un score de benchmark |
| `MorseHGP3DResult` | axes orthogonaux, `gamma_cofaces` et objets canoniquement triés, certificat cohérent avec la racine |

Pour `CriticalEvent.morse_roles`, un rôle d'indice zéro a `local_multiplicity=1` et `arm_count=0`. Un rôle d'indice un a `arm_count` égal à la taille du shell et `local_multiplicity=arm_count-1`. Cette multiplicité est locale; elle ne garantit pas le nombre de classes globales tuées.

## 9. Variantes de résultat et base de preuve

`MorseHGP3DResult.result_kind` contrôle la présence scientifique, sans autoriser de champ inconnu :

- `hierarchy` déclare `forest_semantics=exact`, contient une forêt par ordre et toutes les flèches adjacentes;
- `partial_hierarchy` déclare `forest_semantics=partial_refinement` et fournit `partial_scope`;
- `verified_events_only` omet `forest_semantics`, conserve les tableaux hiérarchiques vides et fournit `partial_scope`.

La racine et le certificat doivent avoir les mêmes `run_id`, `result_kind`, backend effectif, profil effectif, mode demandé, `k_eff` et sémantique de forêt.

La base mathématique est explicite :

| profil et voie | `reconstruction_contract_id` | `proof_basis` | statut `exact` autorisé |
|---|---|---|---|
| `hgp_reduced`, Gamma exhaustif | `hgp-reduced-v2` | `gamma_exhaustive_reference` | oui, uniquement avec `effective_backend=reference_cpu` et toutes les autres portes fermées |
| `hgp_reduced`, flot Gabriel brut | `hgp-reduced-v2` | `gabriel_positive_connectivity` | non; seulement `partial_refinement` avec statut `conditional` ou `budget_exhausted` |
| `full_pi0` avant preuve M.1 | `M1-reconstruction-v1` | `m1_conditional_contract` | non |
| `full_pi0` après fermeture formelle | version contractuelle future | valeur future hors de l'énumération v2 | non dans le schéma actif; exige une nouvelle révision après fermeture de M.1 |

Un certificat `exact` exige notamment toutes les entrées des tableaux de complétude vraies, dont `gamma_complete_by_order`, `relevant_gp_complete`, `canonical_children_complete`, `active_cross_incidences_complete` et `vertical_maps_complete` vrais, zéro locus non résolu, zéro dégénérescence non prise en charge, zéro échec numérique et `stop_reason=completed`. Pour `hgp_reduced`, il exige en plus l'énumération exhaustive de chaque `GammaCoface`, y compris non-Gabriel, et son affectation unique à un `EqualLevelBatch` du même ordre et niveau sur le backend CPU de référence. Un lot sans événement critique reste obligatoire lorsqu'il porte une incidence Gamma silencieuse. Un mode `budgeted` ne publie jamais `exact`.

Le nom futur `incidence_complete_reduction_proved` est réservé à une éventuelle réduction qui rétablirait et prouverait toutes les incidences silencieuses nécessaires. Il n'appartient pas à l'énumération v2 et aucun producteur ne peut l'émettre avant mise à jour du registre, du schéma et des tests de falsification.

## 10. Budgets, checkpoints et GCP

Dans une politique, `null` signifie « aucune limite imposée par cette politique » et non « mesure inconnue ». Dans une télémétrie facultative, `null` signifie au contraire « indisponible » et ne doit pas être remplacé par zéro. La différence est déterminée par le type et le nom du champ.

Un `BudgetSnapshot` est pris avant chaque lot, run, merge et sérialisation. Les réserves transactionnelles couvrent simultanément l'ancien état, le temporaire, le pire espace de merge, le checkpoint et la marge. Un refus se produit avant écriture et nomme la première ressource insuffisante.

Un `CheckpointManifest` n'est publiable qu'après écriture temporaire, synchronisation, vérification des sommes puis renommage atomique. `equal_level_batch_state` vaut `none_open`, `not_applied` ou `fully_applied`, jamais « partiellement appliqué ».

`BenchmarkRecord.gcp_safety` ne démarre ni n'arrête une VM. Il enregistre seulement la cible exacte et les preuves opérationnelles. Lorsque `used=true`, la validation sémantique exige `g4-standard-48`, Spot, action `STOP`, durée de 30 à 28 800 secondes, arrêt invité armé et `final_state=TERMINATED`. Les autres VM ne sont jamais attribuées au run par simple découverte.

## 11. Validation

Les commandes minimales sont :

```bash
python -m json.tool schemas/morsehgp3d-contract-v2.schema.json >/dev/null
python tools/check_contracts.py
python tools/check_docs.py
```

`check_contracts.py` valide chaque exemple contre son `$defs`, effectue un aller-retour JSON canonique, refuse les propriétés critiques inconnues et vérifie les invariants croisés du certificat et de la racine. Il vérifie aussi l'ordre total binary64 des points, les identifiants de contenu, toutes les références typées, l'arithmétique des budgets, l'exhaustivité déclarée des événements, hyperarêtes et attaches, ainsi que les garanties positives d'un `PartialScope`.

Une sortie `hgp_reduced` exacte et une sortie `full_pi0` conditionnelle qui déclare toutes ses portes de catalogue, Gamma, attaches et lots fermées sont rejouées lot par lot. L'état `pre_lot_components` doit être le dernier état post-lot strict, toutes les cofaces Gamma du niveau sont contractées simultanément, les nœuds de naissance ou multifusion doivent correspondre aux racines obtenues, et le delta pré/post doit être identique dans le lot, la forêt et la racine. Une croissance silencieuse qui ajoute une facette sans ajouter de point reste obligatoire dans `coverage_log`.

Les tests négatifs couvrent notamment un champ inconnu, une unité incorrecte, un rationnel non canonique, un tableau non trié, une référence pendante, un identifiant recalculé sur une projection falsifiée, une longueur de complétude incohérente, un lot non rejouable, une flèche verticale incomplète, une arithmétique de budget ouverte, une tentative de publier `full_pi0` exact sur la seule base conditionnelle M.1, une tentative de promouvoir Gabriel brut, et une tentative d'utiliser `gamma_exhaustive_reference` comme base exacte sur `cuda_g4`.

Le migrateur `tools/rekey_contract_fixtures_v2.py --write` prépare, migre et valide l'ensemble des chemins demandés avant la première publication. Chaque remplacement de fichier est atomique; si un remplacement système échoue après des publications réussies, l'outil énumère précisément les chemins déjà publiés et la cible dont l'état doit être inspecté, sans annoncer de rollback multi-fichiers impossible.
