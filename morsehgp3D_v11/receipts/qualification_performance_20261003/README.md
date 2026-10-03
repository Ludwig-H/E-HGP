# Comparaison FULL préparée le 3 octobre 2026

Ce dossier conserve une **source comparative**, pas une qualification ni un
binaire. `baseline_source_895680ff8.tar.gz` contient 428 fichiers Git du commit
`895680ff866fbe41c450c87b2498ebff2ac7408b`, 743 847 octets compressés,
SHA256 `d9f8765c7284887d54bc248223fac3d00b9602aedbba1b8566e4727bdfbc8eef`.
Le manifeste donne chaque chemin, blob Git, taille et SHA256. Sélection explicite :
CMakeLists, README, .gitattributes, src, cmake, bench, tests, reference et tools.
Aucun receipts, audit, dump de documentation, donnée KITTI ou binaire inclus.
La baseline est construite avec les mêmes compilateur/options que le nouveau
profil u21 ; seule la cible `mhgp11_full_bench` est compilée. Ses anciennes portes
ne sont pas héritées.

Deux sessions gardées, au **même commit publié**, chacune maxRun4200s :

1. `bench/plans/full_qualification_g4.json` joue la matrice complète actuelle puis
   ASan18. Le worker et l'arrêt ciblé doivent être clos avant la suite.
2. Instancier `bench/plans/full_paired_g4.json` hors Git en remplaçant les deux
   marqueurs par le chemin distant et le SHA256 de `results.tar.gz` de la première
   session. Ce fichier reste après la purge du contrôleur ; ses anciens builds
   sont supprimés. Le lecteur vérifie son manifeste exhaustif et la source du
   worker, puis la seconde session **reconstruit** bits21 et joue ses portes
   ciblées, dont les gardes population/contextes/admission et tri/FENV/pannes.
   Le nouveau binaire reçoit ses propres hashes/provenance ; aucune identité
   binaire avec la première compilation n'est revendiquée.

Le calendrier contient 81 invocations : trois trames sans sol entières de reuse1,
K5/u21, W1/W8/W48, trois prises ; baseline2047 (27), courant2047 et courant16379
(54). Chaque triplet tourne son ordre entre les trois prises. « Froid » signifie
un processus et des propriétaires natifs neufs ; les caches OS ne sont pas vidés.
Une seule cache de résumés paie la première inspection et rehache chaque payload
actuel ; les événements actuels sont toujours validés. Chaque dump complet est
comparé octet pour octet à une référence de sa trame avant suppression. Les traces
et dumps en échec restent disponibles. Aucun invariant de travail n'est imposé
entre algorithmes : les compteurs, phases, CPU, réservations et coûts Python sont
rapportés avec leurs périmètres.

Les délais sont des budgets, pas des mesures qualifiées. Session1 : 1870s de
commandes ; session2 : 1920s. Avec maxRun4200, arrêt invité55min et les réserves du
contrôleur, environ2340s sont disponibles avant setup120 et le coût d'upload.
Le manifeste reuse1 hors Git est à `build/v11-full-data-20261002` (2930382 octets).
Préflight et échéance D restent autoritaires. Le cap résultats128MiB n'inclut
pas tous les dumps réussis ; les références/dumps en échec restent sur la VM et
les résultats structurés les identifient. Préserver les pièces utiles avant toute
session suivante qui peut purger ces scratchs.

Contrôles locaux autorisés : Python autonome/factices seulement, normal et −O,
plus syntaxe du worker. Aucun produit natif compilé/exécuté et GCP non utilisé.
Les premières attentes de masques des collecteurs dense/pair_graph (1423/1167)
ont échoué après l'ajout des bits4096/8192 ; elles sont mises à jour à13711/13455,
avec résultats suivants conservés dans `protocol_validation.json`.
