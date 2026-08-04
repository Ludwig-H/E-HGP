# Qualification G4 du pipeline résident partagé au SHA `3405f3c`

## Portée et porte d'entrée

Cette qualification conserve `phase=15`, `profile=hgp_reduced` et `public_status=not_claimed`. Le backend de campagne est `cuda_g4`; les deux raccords vers la forêt conditionnelle emploient `cuda_g4_plus_reference_cpu`. Le mode qualifié reste un assemblage de composants bornés : scheduler transactionnel de paires, vue LBVH partagée, adaptateur positif de supports supérieurs, forêt H0 conditionnelle, propositions verticales relatives à la forêt et frontière device. La porte d'entrée de la Phase 15 était satisfaite avant le lancement; aucune phase n'est ouverte ou fermée par ce rapport.

L'arbre exécuté était propre et déjà présent sur `origin/main` au SHA complet `3405f3c8b1b01260472c28ae71163c151bc21fd9`. Les deux sorties canoniques sont archivées octet pour octet :

- [campagne Phase 15](phase15_resident_transactional_semantic_g4_3405f3c.json), SHA-256 `4e03d3d0870503fb87b7fe1013ddedf6b903544ebad848dd8eccebf61bd77130`;
- [environnement Phase 3](phase3_g4_resident_transactional_semantic_3405f3c.json), SHA-256 `6fe313d1ea9cc92a35745df8c9bf5f7fba4a6abc0ee3f3b2777565e6fac69d5c`.

Le reçu Phase 15 a `status=passed`, `artifact_role=component_qualification_only` et `qualified_scope=bounded_scheduler_conditional_forest_forest_relative_vertical_target_pipeline_and_device_frontier_components_only`. Le reçu Phase 3 a `status=passed` dans la seule portée `environment_reproducibility_only`.

## Cycle de vie GCP gardé

La première cible `devpod-gpu-exploration/europe-west4-a/ehgp-blackwell-spot` était `SPOT`, `g4-standard-48`, `instanceTerminationAction=STOP` et `maxRunDuration=14400`. Après démarrage, GCE n'a pas matérialisé `terminationTimestamp` dans les douze relectures exigées. Le garde-fou a donc échoué fermé avant tout benchmark, puis l'arrêt d'urgence a ciblé exactement la génération `2026-08-04T01:57:35.082-07:00`. Une seconde invocation de `stop_and_verify.sh` a recertifié cette cible `TERMINATED`; la clé OS Login a ensuite été révoquée et sa copie privée locale détruite.

La campagne canonique a utilisé la cible de secours autorisée `devpod-gpu-exploration/europe-west4-ai1a/ehgp-blackwell-spot-ai1a`. Le coupe-circuit GCE de 14 400 secondes et l'arrêt invité de 210 minutes ont été vérifiés avant le travail lourd. La génération exacte était `2026-08-04T02:07:16.784-07:00`. L'orchestrateur a ensuite arrêté cette cible, l'a relue `TERMINATED` le `2026-08-04T11:19:25Z`, n'a trouvé aucune autre VM `project=e-hgp` active, a révoqué la clé OS Login de session et a supprimé sa copie privée locale.

## Résultats qualifiés dans leur portée

Les schedulers résidents `K=5` et `K=10` ferment chacun la partition de 120 paires sur la fixture `semantic_line_16`. Ils qualifient uniquement `pair_block_partition_scheduler_only`; `scale_eligible=false` et tous les claims de catalogue, supports trois--quatre, hiérarchie, SLO et statut public restent faux.

Les raccords reducer `K=5` et `K=10` ont tous deux `qualified=true`. Chaque run conserve exactement un snapshot LBVH, un accounting de capacité, zéro copie de snapshot, une vue paire, une vue supports supérieurs et une seule consommation du lease source. Les durées totales sont respectivement `8026276908` ns et `62803440136` ns sur la fixture bornée `eight_clusters_12`.

L'adaptateur positif de supports supérieurs rapporte `native_exact_authority=true`, zéro cache miss et aucune tâche à la demande. Il rapporte aussi explicitement `terminal_classification_native_cuda=false`. Les succès bornés certifient donc les décisions exactes positives et leur raccord conditionnel, pas des classifieurs terminaux CUDA natifs pour les supports trois et quatre.

La frontière 50 000 à rang fermé maximal 11 termine avec `success=true`, `coverage_partition_complete=true` et `total_ns=14454104329`. Elle n'exerce ni tous les seuils de rang ni le triplet de rangs requis. Elle est une qualification de composant, pas un `warm_e2e`, et `slo=false`.

Le diagnostic direct 10 000 000 termine sous sa borne de 2 400 secondes. Il rapporte `execution_success=true`, `coverage_success=true`, `coverage_complete=true`, `censored=false` et `total_ns=1886560592946`, soit environ `1886.561` secondes. Il reste `profile_only`, sans claim de qualification ou de scalabilité, et `process_restart_resumable=false`.

Le diagnostic direct 30 000 000 atteint le timeout canonique de 5 400 secondes : statut de processus 124, mur externe `5405186189811` ns, stdout et stderr vides, `qualified=false` et `scale_eligible=false`. Ce résultat est une censure reproductible, pas une validation 30M.

## Limites et porte suivante

La campagne ne matérialise ni matrice globale de paires, ni Delaunay ordinaire ou d'ordre supérieur, ni cellule, coface ou incidence globale. Elle ne publie ni catalogue complet de paires, ni hiérarchie produit, ni morphismes verticaux complets, ni M.1, O.7, `full_pi0`, `min_cluster_size`, reprise durable, scalabilité ou exactitude publique.

La porte suivante reste composée de quatre obligations distinctes : classifieurs terminaux CUDA natifs pour supports trois et quatre; mesure réellement `warm_e2e` à 50 000 et `K_max=10` avec p95 sous 100 ms; checkpoint durable et reprise après destruction du processus; exécution 30M qualifiante suivie du vérificateur scientifique indépendant. Aucun de ces points n'est fermé par les deux reçus archivés ici.
