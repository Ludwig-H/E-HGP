# Avancement de la Phase 3 — environnement CUDA G4 reproductible

## Cadre opérationnel

- Phase : 3, environnement reproductible G4.
- Backend cible : `cuda_g4`.
- Profils : couche commune, avec priorité au profil `hgp_reduced`.
- Mode : `certified` sous audit de reproductibilité; aucune promotion de statut public.
- Porte d'entrée : satisfaite par la Phase 0, conformément au registre d'implémentation.

La Phase 3 reste `in_progress`. Ce second point d'avancement livre le runtime, les dépendances et le worker de qualification, tout en conservant la fermeture conditionnée à une exécution GPU réelle courte et à la certification finale de l'arrêt.

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

L'orchestrateur `run_phase3_qualification.sh` est verrouillé sur `devpod-gpu-exploration/europe-west4-a/ehgp-blackwell-spot`. Il exige un arbre propre dont le SHA est déjà présent sur `origin/main`, arme l'arrêt invité 45 minutes après la certification des gardes et maintient séparément le coupe-circuit GCE borné entre 30 secondes et huit heures. Le répertoire de résultat reste hors du dépôt. L'orchestrateur ne démarre la cible que par `start_and_verify.sh` et ne la ferme que par `stop_and_verify.sh`.

Ses traps couvrent sortie normale, échec distant, `HUP`, `INT` et `TERM`. `start_and_verify.sh` publie atomiquement le handoff ciblé `e-hgp.start-handoff.v3` dès que la garde GCE post-démarrage a certifié le `lastStartTimestamp`, avant l'armement invité. Toute fermeture automatique transmet cette génération à `stop_and_verify.sh`, qui refuse avant mutation une cible redémarrée concurremment. Si la garde invitée puis l'arrêt d'urgence échouent, le handoff est conservé pour permettre la reprise exacte et bloquer une nouvelle session; un échec antérieur à la lecture de la génération interdit au contraire tout arrêt automatique non versionné et produit les coordonnées exactes de contrôle. Les appels GCP critiques sont bornés par GNU `timeout` avec `--foreground` et `--kill-after`. Après toute session démarrée, la réussite exige en plus une relecture GCE indépendante donnant exactement `TERMINATED` avec la même génération. Un arrêt par le script, une relecture illisible ou un état final différent possèdent des codes d'échec distincts. Tant que l'arrêt reste non certifié, le handoff local est conservé et empêche une nouvelle session sur le même SHA. L'arrêt ciblé a priorité sur le nettoyage distant : le clone détaché sur le SHA relu peut rester dans `/tmp` sur le disque de la VM arrêtée.

Le worker distant `phase3_remote_qualification.sh` relit directement `/run/systemd/shutdown/scheduled` avant tout travail lourd et exige `MODE=poweroff` avec une échéance future située entre 1 800 et 2 820 secondes. Il construit l'image exacte, exécute les workflows CUDA release et audit, puis lance le runtime, la liaison Python/DLPack, `cuobjdump` sur le runtime AOT et `compute-sanitizer --leak-check full`. Les conteneurs reprennent l'UID, le GID et les groupes supplémentaires de l'invité; le dépôt est monté en lecture seule, tandis que les répertoires de construction, de résultat et le `HOME` invité restent inscriptibles. Le worker ne contient aucune commande GCP et laisse la responsabilité de l'arrêt à l'orchestrateur externe.

L'assembleur refuse un manifeste ou un record incomplet, une architecture autre que `sm_120`, toute entrée PTX, des horodatages ou durées incohérents, une allocation invalide, une fuite déclarée ou une copie DLPack. Le worker ferme séparément sur tout échec de `compute-sanitizer`. Son artefact distant reste provisoire avec `status=worker_passed_pending_shutdown` : l'orchestrateur le récupère dans un fichier local partiel, valide strictement son schéma et son identité Git/image, certifie l'arrêt ciblé puis relit `TERMINATED`, enrichit `vm_lifecycle`, convertit le statut en `passed` et publie enfin le JSON atomiquement sans remplacer une cible concurrente. Aucun artefact final n'est laissé si l'arrêt ou sa relecture échoue; le handoff opérationnel reste alors disponible pour reprendre l'arrêt ciblé.

Le runtime effectue une seule allocation `cudaMallocAsync` exactement égale au plafond configuré. Il réserve ses registres et diagnostics hôte avant cette mutation afin qu'une exception d'allocation ne puisse pas abandonner l'arène. Deux sorties déterministes et l'espace temporaire de réduction CUB sont sous-alloués dans cette arène sans autre allocation CUDA. Après un échauffement AOT, les protocoles `warm` et `resident` sont chronométrés par événements CUDA et horloge hôte monotone, comparés bit à bit et confrontés à un oracle CPU indépendant par vecteur, somme et hash, puis l'arène est libérée et le pool par défaut est purgé. La liaison DLPack refuse les capsules étrangères avant toute interprétation du tenseur et valide un contexte de propriété distinct de l'adresse CPython réutilisable. Les checkouts DLPack, nanobind et robin-map doivent être propres en plus de porter leur commit exact. Chaque record JSONL contient le manifeste complet ou échoue fermé; l'artefact porte `scientific_result_claimed=false`, `scientific_public_status=null` et aucun champ `public_status`. Ces mesures qualifient uniquement l'infrastructure et ne constituent aucun résultat scientifique.

## Validations enregistrées

| validation | résultat |
|---|---:|
| workflow `cpu-release` | 27/27 tests, 204,95 s |
| workflow `sanitizer` | 27/27 tests, 297,34 s |
| cibles CPU réellement instrumentées à la compilation et à l'édition de liens | 9/9 |
| mutations CUDA interdites exercées sans compilateur | 7/7 refusées |
| scénarios de sécurité et de worker GCP avec doubles de commande | 48/48 |
| validation statique du contrat de build Phase 3.1 | réussie |
| validation statique du runtime et des dépendances Phase 3.2 | réussie |
| politique GitHub GCP strictement en lecture seule | réussie |
| syntaxe de tous les scripts GCP et propreté du diff | réussies |

Les deux workflows CPU comprennent la campagne bornée de prédicats et toutes les suites différentielles existantes. Le profil sanitizer applique ASan et UBSan aux huit exécutables de test ou de dump et au binaire de replay, sans polluer la cible d'interface installée.

Les scénarios GCP simulés couvrent notamment l'arbre sale, le commit non poussé, une autre cible, un SHA distant divergent, l'échec du worker, la race de handoff sous signal réel, l'interruption de la garde invitée suivie d'un arrêt illisible, le refus d'une génération concurrente, l'exigence d'un GNU `timeout` borné, l'interdiction de l'arrêt non versionné après timeout, la conservation du handoff d'incident, l'échec du script d'arrêt, une preuve finale illisible et l'absence d'artefact final avant `TERMINATED`. Les tests propres au worker couvrent aussi la garde invitée stricte, l'identité utilisateur des conteneurs, le digest de base, l'arène unique, l'oracle CPU, le refus des capsules DLPack étrangères, la séparation des sorties PTX, le memcheck et la publication atomique. Aucune commande GCP réelle n'a été lancée pour ce point d'avancement.

## Travaux restant avant fermeture

- compiler les trois cibles par les workflows CMake CUDA release et audit dans l'environnement exact;
- auditer les objets AOT `sm_120` et l'absence de PTX;
- exécuter sur la G4 le plafond d'allocation, le DLPack sans copie, le kernel déterministe, les erreurs structurées et `compute-sanitizer`;
- exécuter une unique session réelle autorisée, certifier la cible `TERMINATED` puis publier son artefact final.

La Phase 2B demeure bloquée jusqu'à cette fermeture. Aucun résultat de benchmark, aucune sortie plausible et aucun succès de construction ne peut promouvoir `public_status=exact`.

## GCP

GCP non utilisé pour ce point d'avancement. Aucune VM n'a été créée, démarrée, arrêtée ou modifiée.
