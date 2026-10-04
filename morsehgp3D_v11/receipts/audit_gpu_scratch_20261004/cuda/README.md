# CUDA scratch et pool : source 22a6af6

Pin exact : `22a6af6aa6c57302b3eac5adcb7f2b52c0f1e0b4`. Capture avant lecture, puis recontrôle des blobs après revue. Aucun build, exécution native, CUDA, G4 ou fit. Les captures antérieures restent intactes. La recoupe est favorable sur le protocole scalaire ; elle ne qualifie pas le matériel.

| Contrat revu | Résultat de lecture |
| --- | --- |
| Stockage entier d'une feuille | `leaf_batch.hpp:64–86` : 128 enregistrements et 1024 rangs locaux ; les comptes continuent après débordement, et fits reste faux. `leaf_batch_cuda.cu:61–68` n'autorise stored que pour kOk et fits. Une feuille non résolue a comptes nuls, stored faux et aucun ledger ajouté. |
| Copie ou rejeu | Copie des seuls stored non vides (:80–86) ; sélection des seuls kOk non vides non stored (:113–118,230–251). Domaines disjoints et complets pour les feuilles résolues émettrices. Le rejeu utilise les préfixes de l'indice original j, contrôle les fins et n'ajoute pas de ledger (:91–108). Le repli géométrique des unresolved est hors de ce sous-scope. |
| Transport compact | Les populations et supports sont des rangs locaux u8, domaine 0..31, sentinelle 255 (`leaf_batch.hpp:23–55`). I puis U suivent le même ordre d'émission. La conversion vers les SiteIdx et le compactage aval sont revus séparément par l'autre auditeur. |
| Réduction par warp | Un bloc de 32 fils, quinze champs initialisés à zéro ; même les fils hors count atteignent les cinq shuffle full-mask (:19–22,53–76). La somme de lane0 est exacte, et les sommes intermédiaires restent dans u64. Le modèle couvre 0..32 feuilles actives et quinze champs. |
| Count, scan, copy, fill | Count synchronisé avant scan (:443–446), CUB puis lecture bloquante des fins (:206–227). Copy et sélection/rejeu sont sur le même flux par défaut ; WritePhase synchronise avant téléchargement (:271–288). Une admission refusée après lancement peut sortir avant cette synchronisation : les frees sont alors enqueued après les usages sur le même flux. Aucun accès ultérieur du code revu à un pointeur rendu constaté. |
| Budget et coexistences | Réservation avant cudaMallocAsync (:133–143), allocations scratch/ordre explicitement budgetées (:396–427), téléchargements hôte budgetés alors que les grands tableaux device restent vivants (:331–341,449–472). Produits scratch : CPU ≤ 2^40 × 2048 = 2^51 octets, CUDA ≤ 2^32 × 2048 = 2^43 ; admission système/budget réelle requise. |
| Libération et pool | Le destructeur enqueue cudaFreeAsync sur le flux0, puis la réservation se termine (:120–145). Le pool par défaut reçoit un seuil u64max (:171–180). Le compte est celui des réservations logiques actives, pas de la mémoire physique gardée par le pool ni de tous les usages encore en vol après un refus. `device_bytes` additionne les allocations, y compris des temporaires de WritePhase détruits avant download ; ce n'est pas un pic global simultané. |
| Préchauffage | État statique unique, lancement sous mutex, lecture ns après join, thread rejoint au destructeur (:155–189,360–374). Favorable pour la durée de vie. L'analyse du chrono FULL est dans la capsule indépendante gpu_update008 ; aucun nouveau chrono n'est établi ici. |

Les propriétés de libération sur un même flux et de rétention du pool suivent la [documentation primaire CUDA 12.8 de l'allocateur ordonné](https://docs.nvidia.com/cuda/archive/12.8.0/cuda-runtime-api/group__CUDART__MEMORY__POOLS.html). Elles rendent nécessaire de distinguer mémoire utilisée, mémoire réservée par le pool et budget logique. Les conditions de participation au shuffle sont données dans le [guide CUDA 12.8, §7.22](https://docs.nvidia.com/cuda/archive/12.8.0/cuda-c-programming-guide/index.html#warp-shuffle-functions). Compilation, comportement CUB/pilote, erreurs d'exécution et quiescence matérielle restent non qualifiés par cette revue.

Un bord scalaire du refus est conservé sans priorité bloquante : `errors` est unsigned32, incrémenté une fois par échec de fill (:98,108), mais count peut valoir 2^32 (:388). Au plafond, 2^32 erreurs pourraient revenir à zéro avant le test (:348). Les slots seuls demanderaient 8 Tio, hors capacité G4 ; aucun résultat FULL erroné ni défaut GPU reproduit. Un atomicExch(errors,1u), puisque seule la non-nullité est utilisée, serait une garde saturante simple.

Le modèle ne fabrique pas des boules géométriques : ses séquences d'émissions factices isolent la machine de stockage, la partition copy/replay et le ledger logique. 187 séquences (dont unresolved après émissions partielles) et 495 cas de réduction donnent les mêmes sorties normal/−O. Les 18 426 contrôles incluent surtout les vérifications par émission ; ils ne sont pas 18 426 portes natives.

Rejeu portable :

```sh
python3 -B -S check_cuda_state.py
python3 -B -O -S check_cuda_state.py
```

`stdout_normal.json`, `stdout_opt.json` et `EXPECTED.json` portent les nombres exacts. Les deux erreurs initiales de recherche de chemin sont conservées dans CAPTURE_ERROR.json ; elles précèdent la revue et n'ont pas produit de constat de code. Les inventaires de fermeture sont exhaustifs, seul SHA256SUMS racine est exclu de son propre inventaire.
