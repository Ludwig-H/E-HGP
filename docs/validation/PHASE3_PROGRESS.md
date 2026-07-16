# Avancement de la Phase 3 — environnement CUDA G4 reproductible

## Cadre opérationnel

- Phase : 3, environnement reproductible G4.
- Backend cible : `cuda_g4`.
- Profils : couche commune, avec priorité au profil `hgp_reduced`.
- Mode : `certified` sous audit de reproductibilité; aucune promotion de statut public.
- Porte d'entrée : satisfaite par la Phase 0, conformément au registre d'implémentation.

La Phase 3 reste `in_progress`. Ce point d'avancement gèle son premier lot de construction et de sécurité avant toute exécution GPU réelle.

## Enveloppe de construction gelée

L'image de développement part de CUDA 12.9.2 sur Ubuntu 24.04, identifiée par le digest de manifeste `sha256:420850a3fd665171b3f1fd08946c51d50468d732a46d6c42345ea04444755048`. Les paquets Ubuntu sont résolus par le service de snapshots à l'instant `20260716T000000Z`. L'image exporte la seule architecture autorisée, `120-real`.

Les quatre presets publics sont fermés :

| preset | rôle | CUDA exécuté localement |
|---|---|---:|
| `cpu-release` | construction et tests CPU Release | non |
| `sanitizer` | construction et tests CPU sous ASan et UBSan | non |
| `cuda-release` | compilation AOT CUDA 12.9 pour `sm_120` réel | non |
| `cuda-audit` | même compilation avec informations de ligne et diagnostics `ptxas` | non |

CUDA reste désactivé par défaut. Son activation exige le compilateur NVIDIA 12.9.x et exactement `CMAKE_CUDA_ARCHITECTURES=120-real`. Les drapeaux d'architecture bruts, la génération PTX, `use_fast_math`, les fichiers d'options et les variables d'injection NVCC sont refusés avant compilation. Le premier kernel est une sonde de compilation AOT exclue de la construction CPU et jamais enregistrée comme test d'exécution.

## Fermeture GCP simulée

L'orchestrateur `run_phase3_qualification.sh` est verrouillé sur `devpod-gpu-exploration/europe-west4-a/ehgp-blackwell-spot`. Il exige un arbre propre dont le SHA est déjà présent sur `origin/main`, un arrêt invité fixé à 45 minutes et un répertoire de résultat hors du dépôt. Il ne démarre la cible que par `start_and_verify.sh` et ne la ferme que par `stop_and_verify.sh`.

Ses traps couvrent sortie normale, échec distant, `HUP`, `INT` et `TERM`. Après toute session démarrée, la réussite exige en plus une relecture GCE indépendante donnant exactement `TERMINATED`. Un arrêt par le script, une relecture illisible ou un état final différent possèdent des codes d'échec distincts. Le clone distant est détaché sur le SHA local relu, et seul son répertoire temporaire canonique peut être nettoyé.

Le worker distant et le banc runtime ne font pas partie de ce premier lot. L'orchestrateur échoue donc fermé avant la qualification distante tant que ce worker n'est pas livré; il ne doit pas encore être utilisé pour ouvrir une session réelle.

## Validations enregistrées

| validation | résultat |
|---|---:|
| workflow `cpu-release` | 26/26 tests, 321,59 s |
| workflow `sanitizer` | 26/26 tests, 286,37 s |
| cibles CPU réellement instrumentées à la compilation et à l'édition de liens | 9/9 |
| mutations CUDA interdites exercées sans compilateur | 7/7 refusées |
| scénarios de sécurité GCP avec doubles de commande | 21/21 |
| validation statique du contrat de build Phase 3.1 | réussie |
| politique GitHub GCP strictement en lecture seule | réussie |
| syntaxe de tous les scripts GCP et propreté du diff | réussies |

Les deux workflows CPU comprennent la campagne bornée de prédicats et toutes les suites différentielles existantes. Le profil sanitizer applique ASan et UBSan aux huit exécutables de test ou de dump et au binaire de replay, sans polluer la cible d'interface installée.

Les scénarios GCP simulés couvrent notamment l'arbre sale, le commit non poussé, une autre cible, un SHA distant divergent, l'échec du worker, une interruption réelle par signal, l'échec du script d'arrêt et une preuve finale illisible. Aucune commande GCP réelle n'a été lancée pour ce point d'avancement.

## Travaux restant avant fermeture

- intégrer effectivement CCCL/CUB, DLPack, NVTX et la liaison Python;
- livrer le harness JSONL et le manifeste complet attaché à chaque mesure;
- qualifier l'allocation asynchrone jusqu'au plafond puis sa libération sans fuite;
- vérifier le DLPack sans copie, le kernel déterministe et les erreurs CUDA structurées;
- prouver l'absence de compilation pendant les mesures `warm` et `resident`;
- livrer le worker distant, compiler et auditer les objets `sm_120` sur la G4;
- exécuter une unique session réelle autorisée, publier son artefact puis certifier la cible `TERMINATED`.

La Phase 2B demeure bloquée jusqu'à cette fermeture. Aucun résultat de benchmark, aucune sortie plausible et aucun succès de construction ne peut promouvoir `public_status=exact`.

## GCP

GCP non utilisé pour ce point d'avancement. Aucune VM n'a été créée, démarrée, arrêtée ou modifiée.
