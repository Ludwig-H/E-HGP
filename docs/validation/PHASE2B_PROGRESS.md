# Phase 2B — progression des prédicats exacts GPU

## Statut

- phase : `2B`, en cours;
- backend : `cuda_g4`;
- profil prioritaire : `hgp_reduced`;
- mode : `certified`;
- porte d'entrée : satisfaite par la fermeture de la phase 2A et la qualification de la phase 3;
- porte de sortie : non satisfaite.

Ce document suit les incréments techniques de la phase 2B. Il ne promeut aucun statut scientifique public et ne ferme pas la phase.

## Premier incrément : comparaison de distances carrées

Le premier portage GPU cible `compare_squared_distances`. Il conserve les neuf mots binary64 d'entrée et un identifiant de replay, puis produit un tri-état distinct : `negative`, `positive` ou `unknown`. La valeur `unknown` ne représente jamais une égalité exacte.

Le noyau CUDA calcule des intervalles avec les primitives d'arrondi dirigé `__dsub_rd/ru`, `__dmul_rd/ru` et `__dadd_rd/ru`. Tout mot non fini, tout intermédiaire non fini et tout intervalle contenant zéro donnent `unknown`. La cible reste AOT `sm_120`, sans fast math, FMA ni FTZ selon la politique CUDA fermée de la phase 3.

Le runner hôte conserve l'ordre et les identifiants. Il transmet chaque `unknown` à `decide_squared_distance_order` avec la politique CPU adaptative dans une tâche asynchrone. En mode d'audit, chaque signe connu du GPU est recalculé séparément avec la politique CPU `multiprecision_only`; une contradiction fait échouer le lot. Chaque décision publiée contient la commande canonique acceptée par l'outil de replay CPU.

## Validations locales avant qualification matérielle

- workflow `cpu-release` : 28 tests sur 28 réussis, contrôle statique 2B inclus;
- contrôle statique 2B : 17 mutations négatives rejetées;
- compilation syntaxique stricte des unités hôtes : réussie;
- différentiel borné avec lanceur GPU simulé : 2 052 cas, dont égalité exacte, sous-normal, overflow et plus de huit blocs logiques; 2 049 signes connus et trois replis CPU, zéro `unknown` terminal;
- replay CPU multiprécision de toutes les commandes émises : réussi.

Le lanceur simulé vérifie l'orchestration, les schémas, les compteurs et le replay; il ne qualifie ni le code device, ni le matériel G4.

## Qualification matérielle du 17 juillet 2026

Le commit propre et poussé `6f27a68177d21efb19f7ba4cb35ed4855ec73a90` a ensuite été compilé et exécuté sur `devpod-gpu-exploration/europe-west4-ai1a/ehgp-blackwell-spot-ai1a`, une `g4-standard-48` Spot. Le coupe-circuit GCE était `instanceTerminationAction=STOP` avec `maxRunDuration=3600`; l'arrêt invité était armé à 45 minutes.

Les workflows CUDA release et audit ont compilé le runner avec NVCC 12.9.86. Le différentiel réel a traité 2 052 cas : 2 049 signes FP64 GPU ont été certifiés puis revérifiés par l'oracle CPU multiprécision; les trois cas attendus — égalité exacte, sous-normal et overflow — ont produit `unknown`, ont été transmis au CPU et n'ont laissé aucun `unknown` terminal. Toutes les commandes émises ont été rejouées par l'outil CPU.

`cuobjdump` a trouvé uniquement l'ELF AOT `sm_120` et aucune entrée PTX. `compute-sanitizer --tool memcheck --leak-check full --error-exitcode=86` a réussi sur un lot contenant une décision GPU connue et un repli CPU. L'image réutilisée est `sha256:e3d96c187ca405790227e02aef1a66ca47df0820bb6b2a86b097359105956d58`, déjà qualifiée par la phase 3.

L'artefact hors dépôt est `/tmp/morsehgp3d-phase2b-6f27a68177d21efb19f7ba4cb35ed4855ec73a90-20260717T172423Z/phase2b-6f27a68177d21efb19f7ba4cb35ed4855ec73a90.json`, de SHA-256 `65bc7175ed922a2515ecbf15a1a3b3c80445581c473232bc233d0d370a36cff1`. Il ne revendique aucun résultat scientifique public.

Après le succès, l'arrêt ciblé de la génération `2026-07-17T10:24:56.396-07:00` a été exécuté et relu indépendamment : état final `TERMINATED` à `2026-07-17T17:27:51.338224Z`. Aucune autre VM `project=e-hgp` active n'a été observée et la clé OS Login de session a été révoquée puis supprimée. Deux tentatives préparatoires avaient échoué avant compilation sur l'accès Docker et son point de montage; chacune avait déjà été arrêtée et certifiée `TERMINATED` avant la tentative suivante.

## Travaux restant avant la porte de sortie

- étendre le différentiel au corpus distance de la phase 2A et à la campagne supplémentaire requise;
- porter les filtres et expansions encore nécessaires, notamment l'orientation et le bisecteur de puissance;
- mesurer et publier les taux de chaque étage sans en faire une condition de correction;
- vérifier que tout `unknown` GPU est transmis au CPU sur l'ensemble des prédicats portés.

La porte de sortie 2B exigera zéro différence CPU/GPU sur le corpus de phase 2A et sur au moins dix millions de signes supplémentaires. Elle reste donc ouverte après ce premier incrément.
