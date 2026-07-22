# Revue de porte Phase 9 — flux direct de supports H0

> [!IMPORTANT]
> Cette revue ferme la Phase `9` pour le backend de proposition `cuda_g4`, le backend de décision `reference_cpu`, le profil `hgp_reduced` et le mode `certified`. La fermeture certifie une base interne terminale de supports deux à quatre et l'exécution réelle du filtre pair P1. Elle ne certifie ni SLO, ni voie 10 M+, ni forêt Morse, ni preuve M.1 et ne publie aucun `public_status`.

## Décision

- Phase : `9` — flux direct de supports H0 sans mosaïque.
- Backend de proposition qualifié : `cuda_g4`, cible AOT `sm_120` sur G4.
- Backend de décision : `reference_cpu` exact.
- Profil : `hgp_reduced`.
- Mode : `certified`.
- Porte d'entrée : satisfaite par les Phases 1, 2A, 4 et 7.
- Verdict de sortie : **satisfait pour la base interne**.
- Effet opérationnel du commit de fermeture : Phase 9 `completed`; Phase 10 `in_progress` avec sémantique `partial_refinement`.

La décision ne repose ni sur un accord moyen, ni sur les temps du smoke. Elle repose sur le rejeu terminal des deux autorités directes, leur comparaison à l'oracle borné, l'absence des structures globales interdites, la séparation proposition--décision et la qualification du chemin CUDA réel.

## Façade terminale et différentiel exact

La façade reconstruit fraîchement le flux pair et le flux supérieur depuis le nuage, exige leur terminaison et conserve deux digests d'autorité domain-separated. Elle normalise uniquement les événements et diagnostics de supports deux, trois et quatre; tout préfixe `budget_exhausted` ou payload source incohérent échoue fermé avec zéro sortie. Un diagnostic extra-shell rang-pertinent demeure explicitement visible dans la façade, puis force le premier journal de Phase 10 à retourner zéro payload.

Le différentiel couvre chaque taille $n=1,\ldots,14$ contre l'oracle exhaustif, puis une permutation d'entrée inversée à $n=14$. Il ne transforme pas l'oracle en architecture produit : la voie directe ne construit ni mosaïque d'ordre supérieur, ni cellule top-$m$, ni coface, ni incidence Gamma, ni arène de $\binom{n}{k}$ supports.

Le smoke `phase9-pair-smoke-2ed620b.json`, SHA-256 `28d4aed28474766e4ddfc4d127b950aac5de6b51f985129673c99e9761cc1247`, publie les compteurs pour 12 500, 25 000 et 50 000 points sur deux familles. Les six runs s'arrêtent volontairement après 20 000 unités avec `status=budget_exhausted`. Ils démontrent un arrêt transactionnel borné; ils ne démontrent ni complétude du flux, ni exposant, ni SLO.

## Qualification G4 de 9.1-CUDA-P1

Le commit propre et poussé qualifié est `976c1c6723760e9d1632f139e3cad238a40b1cb8`. L'artefact `phase9-pair-support-phi-976c1c6723760e9d1632f139e3cad238a40b1cb8.json` utilise le schéma `morsehgp3d.phase9.pair_support_phi_cuda_g4_qualification.v1` et porte le SHA-256 `f8d6b573c8178c1951d1817daa609ed7a5bd08bd9760d19e2199f151da6b06cc`.

| contrôle | résultat |
|---|---:|
| workflow `cuda-release` | passé |
| workflow `cuda-audit` | passé |
| architectures ELF | `sm_120` seulement |
| entrées PTX | 0 |
| propositions `strict_interior` | 1 |
| propositions `descend` | 1 |
| reçus recertifiés | 1 |
| epochs | 1 puis 2 |
| déterminisme | vrai |
| recertification CPU exacte | passée |
| `memcheck` | passé |
| `racecheck` | passé |
| prune global publié | faux |
| résultat scientifique revendiqué | faux |
| statut public | `null` |

Le binaire qualifié porte le SHA-256 `947c680440d3d9cc1a69d0048fa352c649112bd2bbfef6a398df1d15f19a52d6`. L'artefact d'environnement `phase3-976c1c6723760e9d1632f139e3cad238a40b1cb8.json` porte le SHA-256 `eeef5b3c8f6b57f646c882661b1b34999d3b76082301ad4450409cb1084401df`.

Le noyau propose seulement une borne supérieure binary64 dirigée pour un batch borné de triplets $(A,B,C)$. L'hôte recalcule le maximum rationnel exact $M(A,B,C)$ avant tout reçu :

$$\overline{M}_{\mathrm{GPU}}\geq M(A,B,C),\qquad \overline{M}_{\mathrm{GPU}}<0,\qquad M_{\mathrm{CPU}}=M(A,B,C)\leq\overline{M}_{\mathrm{GPU}}<0.$$

## Évaluation explicite de la porte

| critère de sortie | preuve | décision |
|---|---|---|
| flux direct terminal deux à quatre | façade fraîche, autorités paire et supérieure séparées, rejet de tout préfixe | go |
| accord oracle borné | différentiel exact $n=1,\ldots,14$ et permutation inversée à $n=14$ | go |
| absence d'objets interdits | checker statique : zéro mosaïque, Gamma et arène combinatoire dans le chemin direct | go |
| diagnostic 50 000 points honnête | compteurs publiés; chaque run reste `budget_exhausted`, sans revendication de SLO | go pour la porte interne |
| GPU dès la Phase 9 | CUDA P1 exécuté réellement sur G4 | go |
| absence de verdict GPU exact | borne dirigée de proposition puis recertification rationnelle CPU | go |
| cible et sûreté CUDA | ELF `sm_120`, zéro PTX, `memcheck` et `racecheck` passés | go |
| fermeture GCP | cible exacte relue `TERMINATED` après l'arrêt ciblé | go |
| statut scientifique | `scientific_result_claimed=false`, `scientific_public_status=null` | go |

**Verdict final : porte de sortie interne de la Phase 9 satisfaite.** La façade terminale et le filtre GPU P1 suffisent pour ouvrir le journal direct de Phase 10. Ils ne suffisent pas pour promouvoir la hiérarchie publique.

## Limites maintenues

- P1 traite seulement un batch borné; aucun parcours témoin par escape, aucune antichaîne et aucun prune global de produit ne sont sur GPU.
- Morton/radix/LBVH device, count/`DeviceScan`/emit, doubles frontières device et CUDA Graph restent absents.
- Les supports trois et quatre n'ont aucune voie GPU qualifiée.
- Le SLO 50 000 points inférieur à la seconde et la voie 10 M+ ne sont ni mesurés ni établis.
- Aucune forêt Morse, aucun certificat M.1, aucune génération complète de gateways silencieux et aucun `public_status=exact` ne sont acquis.

## Porte d'entrée Phase 10

La Phase 10 dépend de cette fermeture et du seul jalon local de Phase 5 `compact_k1_forest_certified/local_k1_compact_forest_only`. Ces deux prérequis sont satisfaits. La Phase 10 est donc ouverte avec `backend=reference_cpu`, `profile=hgp_reduced`, `mode=certified` et sémantique `partial_refinement`; la Phase 5 globale reste `ready`.

Le premier incrément livré est le journal d'événements directs : il ajoute les naissances singletons, projette naissances et selles des supports terminaux, puis groupe les rôles par couple exact ordre--niveau. Il ne construit encore ni bras de selle, ni hypergraphe quotient, ni forêt ou `GatewayAttach`.

## GCP

La qualification a ciblé `devpod-gpu-exploration/europe-west4-ai1a/ehgp-blackwell-spot-ai1a`, de type `g4-standard-48`, en `SPOT`, avec `instanceTerminationAction=STOP`, `maxRunDuration=3600` et arrêt invité armé à 45 minutes. La génération a démarré à `2026-07-22T07:51:44.652-07:00`; l'arrêt ciblé est horodaté `2026-07-22T07:55:55.202-07:00` et la relecture finale du `2026-07-22T14:56:04Z` certifie `TERMINATED` pour cette cible précise.
