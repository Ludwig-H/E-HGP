# Revue de porte Phase 7 — primitive H-polytope interne

> [!IMPORTANT]
> Cette revue ferme la Phase 7 pour le backend de proposition `cuda_g4`, le profil `generic_core` et le mode `benchmark_only`. La qualification établit que la TU CUDA exécute le transcript conservatif prévu et que le rejeu `reference_cpu` décide indépendamment la géométrie locale. Elle ne ferme aucun diagramme global, ne certifie aucun parent de la Phase 8 et ne promeut aucun résultat vers `public_status=exact`.

## Décision

- Phase : `7` — audit de la primitive de puissance.
- Primitive produit : H-polytope interne.
- Backend de proposition qualifié : `cuda_g4`, cible AOT `sm_120` sur G4.
- Backend de décision : `reference_cpu` exact, borné au domaine d'audit.
- Profil : `generic_core`.
- Mode : `benchmark_only`.
- Porte d'entrée : satisfaite par les Phases 2B et 4 fermées.
- Verdict de sortie : **satisfait**.
- Effet opérationnel du commit de fermeture : Phase 7 `completed`; Phase 8 `ready`; la Phase 5 reste l'unique phase `in_progress`.

La décision ne repose pas sur le nombre de records acceptés ni sur un accord moyen. Elle repose sur la séparation contractuelle entre transcript flottant et décision rationnelle, la fermeture des erreurs et des capacités, la qualification du chemin CUDA réel et le choix d'architecture déjà arrêté au jalon 7.4.

## Recul d'architecture

L'adaptateur Paragram direct est écarté parce que sa surface publique ne livre ni sommets, ni plans liants, ni incidences et que son parcours historique pouvait perdre une branche sans certificat de complétude. La série épinglée à deux patchs transforme l'overflow connu en erreur et impose une boîte explicite, mais une intégration produit complète devrait encore traverser le scratch de chunks, la gestion du stream, les fautes asynchrones et `cuBQL::gpuBuilder`. Elle resterait en outre site-centrique, alors que les Phases 8 et 9 demandent des parents H-polytope génériques.

La voie saine est donc celle effectivement livrée :

- Paragram reste un comparateur reproductible et une source optionnelle de graines sémantiques authentifiées;
- `build_exact_bounded_h_polytope_reference` décide exactement toute H-représentation fournie dans le domaine borné;
- le transcript batché réserve un slot authentifiable par triplet et traite toute ambiguïté comme `unknown_requires_cpu_exact`;
- la TU CUDA ne publie que des propositions par intervalles dirigés;
- le wrapper hôte reconstruit toujours la décision locale depuis les contraintes rationnelles.

Cette bifurcation évite de prolonger un fork dont les coutures ne correspondent plus aux besoins du raffinement canonique.

## Qualification G4

Le commit propre et déjà poussé `39670649e1af1b999c5be7d580650a2792a09008` a été qualifié avec le compagnon gardé Phase 7.8. L'artefact hors dépôt `phase7-h-polytope-39670649e1af1b999c5be7d580650a2792a09008.json` utilise le schéma `morsehgp3d.phase7.h_polytope_cuda_g4_qualification.v1` et porte le SHA-256 `7894bc6bd7dbce3bddb1f5405d345d8f8ffe321d0be2949908355dfe130cabca`.

| contrôle | résultat |
|---|---:|
| workflow `cuda-release` | passé |
| workflow `cuda-audit` | passé |
| architectures ELF | `sm_120` seulement |
| entrées PTX | 0 |
| cellules analytiques | 4 |
| records exhaustifs | 55 |
| records inconnus | 35 |
| rejets stricts proposés | 4 |
| survivants proposés | 16 |
| epochs | 1 puis 2 |
| déterminisme du transcript | vrai |
| recertification CPU exacte | vraie |
| `memcheck` | passé, 0 erreur et 0 octet de fuite |
| `racecheck` | passé, 0 hazard, 0 erreur et 0 avertissement |

Le digest FNV-1a de proposition vaut `15777663065238964103`. Le binaire qualifié porte le SHA-256 `562853695aa4c457afaced5b8e0ea4e781637df65e38ee34898cc8b37e1a2b68`. L'artefact Phase 3 associé porte le SHA-256 `51486a64c198f16bb2d1459028f733168acc05e194a46ec80181a5094a4c1b85`; l'image qualifiée est `sha256:00b495b40af952e717f46df813dfdda41cd45ed5616d214ccd3e8c666617fb7c`.

L'artefact conserve explicitement `proposal_semantics=proposal_only_exhaustive_plane_triple_transcript`, `decision_semantics=reference_cpu_exact_all_constraints`, `scientific_result_claimed=false` et `scientific_public_status=null`.

## Évaluation explicite de la porte

| critère de sortie | preuve | décision |
|---|---|---|
| choix de primitive | H-polytope interne sélectionné; série Paragram gelée comme comparateur | go |
| géométrie rejouable | contraintes rationnelles, IDs composites, triplets et incidences exactes disponibles côté référence | go |
| maîtrise des capacités | préflight complet, ligne entière ou fallback, aucune troncature de cellule | go |
| ambiguïté flottante | déterminant contenant zéro, non-fini ou comparaison indécidable deviennent inconnus | go |
| absence de verdict GPU exact | le checker interdit tout verdict exact et le wrapper recertifie indépendamment | go |
| exécution CUDA réelle | release, audit et fixture analytique passent sous CUDA 12.9 sur G4 | go |
| cible AOT | ELF limité à `sm_120`, aucune PTX | go |
| sûreté mémoire et concurrence | `memcheck` et `racecheck` passent | go |
| reproductibilité | SHA Git propre, image, binaire, journaux et artefacts empreintés | go |
| fermeture GCP | génération ciblée relue `TERMINATED` et clé de session révoquée | go |

**Verdict final : porte de sortie Phase 7 satisfaite.** La phase a choisi et qualifié une primitive de proposition que l'on peut rejouer exactement. Elle n'avait pas pour obligation de fermer les parents ou le diagramme global, qui relèvent de la Phase 8.

## Limites maintenues

- La fixture analytique qualifie le chemin logiciel; elle ne mesure pas le débit et n'établit aucune scalabilité.
- Le noyau exact certifie uniquement la H-représentation fournie; la complétude des colonnes et du parent reste ouverte.
- Aucun diagramme ordinaire global, `CatalogCertificate`, `RelevantGP`, événement critique ou réduction hiérarchique n'est produit.
- Le transcript GPU reste une proposition, même lorsque tous ses records sont déterministes.
- Aucun statut scientifique public n'est accordé.

## Porte d'entrée Phase 8

La Phase 8 dépend des Phases 1, 4 et 7, désormais toutes fermées. Sa porte d'entrée est donc satisfaite. Elle devient `ready` avec `backend=reference_cpu`, `profile=generic_core` et `mode=certified`; `cuda_g4` ne sera utilisé que comme accélérateur de propositions recertifiées.

Le premier jalon doit construire une AABB dyadique strictement paddée autour de l'AABB exacte des sites, enregistrer son témoin de padding et établir la base canonique. Le jalon suivant fermera les cellules ordinaires par séparation exacte à tous les sommets, ajout simultané des violateurs et co-minimiseurs, réconciliation des incidences actives et file globale vide. Ces obligations sont distinctes de la qualification 7.8 et interdisent toute promotion prématurée.

## GCP

La qualification a ciblé `devpod-gpu-exploration/europe-west4-ai1a/ehgp-blackwell-spot-ai1a`, de type `g4-standard-48`, en `SPOT`, avec `instanceTerminationAction=STOP`, `maxRunDuration=3600` et arrêt invité armé à 45 minutes. La génération démarrée à `2026-07-20T19:29:08.284-07:00` a été arrêtée par le workflow gardé; la relecture finale du `2026-07-21T02:32:56Z` certifie `TERMINATED`.

La clé OS Login de session a été révoquée et sa copie privée supprimée. Aucune autre VM active portant le label `project=e-hgp` n'a été observée au passage de relais.
