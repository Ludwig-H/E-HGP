# Lecture CUDA coopérative en rédaction

Coupe WIP au HEAD développeur 3b76a3fcf, empreintes avant/après égales ; aucune édition produit, aucun build natif ou GPU. Cette source ne transfère aucune qualification du moteur publié.

Point à corriger : coop_count effectue une lecture ordinaire de s.unresolved à la ligne55, après la barrière53 mais avant les atomicExch de la boucle63. La première barrière ordonne la phase précédente ; elle ne termine pas les lectures de ce garde avant les écritures de la phase courante. Sous [Independent Thread Scheduling](https://docs.nvidia.com/cuda/archive/12.6.0/cuda-c-programming-guide/index.html#independent-thread-scheduling), une lane peut lire0 et atteindre son refus pendant qu'une autre n'a pas encore lu le garde. La lecture ordinaire et l'écriture atomique ne sont pas séparées par une frontière de phase.

Correction minimale proposée : `const bool root_resolved = s.unresolved == 0; __syncwarp(); if (root_resolved) { ... }`. Le snapshot privé est lu par toutes les lanes avant toute atomicExch. Autre possibilité : lecture atomique du garde. La lecture finale après __syncwarp69 est ordonnée ; le modèle ne montre donc ni faux FULL ni oubli de repli. Aucun diagnostic natif de race n'est prétendu.

Capacités et publication favorables en lecture : la paire compressée u16 donne un CoopShared de8040 octets sous ABI déclarée (sizeof non exécuté) ; 496 paires, comptes/préfixes u32 sous la borne2^22. Scan/places restent dans l'ordre des paires. Les feuilles non résolues publient zéro volume/compteur et stored=false. Les erreurs du remplissage rendent le lot refusé avant matérialisation. La préparation séquentielle est revenue à i<j dans leaf_device.hpp.

Contrôle G4 ciblé : [racecheck et synccheck](https://docs.nvidia.com/compute-sanitizer/ComputeSanitizer/index.html), avec refus injecté précoce d'une paire et garde retardé chez une autre lane ; dump/ledger et rejet complet de la feuille. Une émulation host séquentielle ne joue pas cette concurrence. Pas de gain revendiqué.

Rejeu stdlib en lecture seule : `python3 -B replay.py --check proof.json`, puis `python3 -O -B replay.py --check proof.json`.
