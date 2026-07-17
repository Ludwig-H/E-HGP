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

Le lanceur simulé vérifie l'orchestration, les schémas, les compteurs et le replay; il ne qualifie ni le code device, ni le matériel G4. La qualification CUDA réelle reste obligatoire sur le commit poussé exact.

## Travaux restant avant la porte de sortie

- compiler et exécuter ce lot sur G4 Spot gardée, puis auditer AOT, absence de PTX et `compute-sanitizer`;
- étendre le différentiel au corpus distance de la phase 2A et à la campagne supplémentaire requise;
- porter les filtres et expansions encore nécessaires, notamment l'orientation et le bisecteur de puissance;
- mesurer et publier les taux de chaque étage sans en faire une condition de correction;
- vérifier que tout `unknown` GPU est transmis au CPU sur l'ensemble des prédicats portés.

La porte de sortie 2B exigera zéro différence CPU/GPU sur le corpus de phase 2A et sur au moins dix millions de signes supplémentaires. Elle reste donc ouverte après ce premier incrément.
