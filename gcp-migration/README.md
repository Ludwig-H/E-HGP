# Infrastructure de benchmark GPU sur GCP

> [!IMPORTANT]
> Ce répertoire est maintenu comme infrastructure active. Les scripts créent et
> pilotent des ressources facturables, mais exigent des confirmations
> interactives et ne suppriment jamais une instance ou un disque.

Ce répertoire gère par défaut la VM Compute Engine `ehgp-blackwell-spot`, une
`g4-standard-48` de `europe-west4-a` (48 vCPU, 180 Go de mémoire hôte) dotée
d'une RTX PRO 6000 Blackwell Server Edition (96 Go de VRAM). Le disque de
démarrage est un Hyperdisk Balanced, car la série G4 n'accepte pas Persistent
Disk. Cette configuration a été revérifiée le
14 juillet 2026 dans la documentation officielle : [série G4](https://cloud.google.com/compute/docs/accelerator-optimized-machines), [zones GPU](https://cloud.google.com/compute/docs/gpus/gpu-regions-zones) et [images Deep Learning VM](https://cloud.google.com/deep-learning-vm/docs/images).

L'orchestrateur Phase 3 accepte aussi le seul repli exact
`europe-west4-ai1a/ehgp-blackwell-spot-ai1a`. Cette zone de capacité appartient
à la même région et partage donc les quotas régionaux applicables; toute autre paire
zone/nom, y compris un croisement entre les deux noms autorisés, est refusée
avant le démarrage. La cible de repli doit être créée séparément avec les mêmes
labels, le même type G4 Spot, l'action `STOP` et une durée GCE bornée.
Les [zones IA](https://cloud.google.com/compute/docs/regions-zones/manage-ai-zones)
exigent en plus l'activation explicite de `ai-zones-visibility`. `deploy.sh` relit
cet état et échoue fermé s'il n'est pas `ENABLED`; il ne l'active jamais. Au
16 juillet 2026, cette fonctionnalité a été activée après autorisation explicite
de l'utilisateur. Cette activation n'autorise toujours ni une création ailleurs,
ni un provisioning Standard/on-demand.

Pour G4 Spot exclusivement, les quotas déterminants sont le quota régional
`PREEMPTIBLE_NVIDIA_RTX_PRO_6000_GPUS`, exposé dans Cloud Quotas sous
`PREEMPTIBLE-NVIDIA-RTX-PRO-6000-GPUS-per-project-region`, et le quota global
`GPUS_ALL_REGIONS`, exposé sous `GPUS-ALL-REGIONS-per-project`. La
[documentation des quotas Compute Engine](https://cloud.google.com/compute/resource-usage)
précise que G4 n'exige aucun quota CPU. Au 16 juillet 2026, leurs valeurs
effectives sont toutes deux `1` avec un usage global nul : le projet reste donc
dimensionné pour au plus une G4 Spot concurrente. Une hausse n'est demandée que
si la limite effective devient inférieure à un; aucun quota ni repli
Standard/on-demand n'est admis.

La sécurité repose sur deux coupe-circuits indépendants : l'arrêt GCE
`maxRunDuration` avec action `STOP`, puis un `shutdown` programmé dans l'OS
invité. Une session ne commence que si les deux sont vérifiés et se termine par
la certification `TERMINATED` de la cible exacte qui a été créée ou démarrée.
Les autres VM portant le label `project=e-hgp` sont inventoriées à titre
d'information, sans arrêt automatique : elles peuvent appartenir à une session
concurrente.

Une seule invocation de contrôle est autorisée à la fois pour un même triplet
projet/zone/instance. Les scripts ne possèdent pas de lease distribué : le
verrou de génération empêche d'arrêter un redémarrage concurrent déjà observé,
mais ne peut pas attribuer avec certitude une génération démarrée par un autre
contrôleur entre la mutation et la première relecture. Les opérateurs doivent
donc sérialiser toute mutation externe sur la cible.

La disponibilité effective d'une VM Spot n'est jamais garantie par le seul
quota. Les commandes de création et d'arrêt doivent être lancées depuis un
terminal local; le workflow GitHub ne fait qu'un contrôle de connexion en
lecture seule, déclenché manuellement.

## Dépendances et configuration

Les scripts exigent Bash, `gcloud`, Python 3 pour vérifier les échéances RFC 3339
et, pour le contrôle des quotas, `jq`.
Copiez les variables non secrètes si vous souhaitez changer la cible :

```bash
cp gcp-migration/.env.example .env.gcp
set -a
source .env.gcp
set +a
```

`.env.gcp` est ignoré par Git. Ne placez jamais de clé de compte de service
dans le dépôt; l'authentification GitHub utilise Workload Identity Federation.

## Préparer le contexte GCP

Toutes les commandes ciblent explicitement le même projet. Configurez-le avant
la première utilisation ; les scripts refusent un projet actif différent.

```bash
export GCP_PROJECT_ID="devpod-gpu-exploration"
gcloud config set project "${GCP_PROJECT_ID}"
gcloud config get-value project
```

Vérifiez ensuite les quotas nécessaires :

```bash
./gcp-migration/check_quotas.sh
```

Le contrôle interroge les identifiants Cloud Quotas exacts, puis vérifie la
limite globale et son usage, le quota RTX PRO 6000 préemptible régional, les
100 Go d'Hyperdisk Balanced, une instance et, lorsque le réseau le demande, une
adresse externe éphémère. Pour le disque provisionné à 3 600 IOPS et 290 MiB/s,
les quotas zonaux de performance ne comptent que l'excédent sur la base gratuite
de 3 000 IOPS et 140 MiB/s : le besoin contrôlé est donc 600 IOPS et 150 MiB/s.
Les disques Hyperdisk Balanced HA régionaux sont comptés deux fois en capacité,
et leur performance est attribuée à chaque zone de réplica. La vue Compute
régionale historique peut omettre le quota RTX récent; cette omission n'est pas
interprétée comme une absence. `PREEMPTIBLE_CPUS` est affiché à titre
informatif, sans être exigé par G4. `deploy.sh` exécute ce contrôle
automatiquement avant toute confirmation de création.

Cloud Quotas ne publie actuellement aucune dimension IOPS ou débit Hyperdisk
pour `europe-west4-ai1a`. Pour ce seul couple documenté, le contrôle qualifie
donc explicitement la limite de *dérivée* : il relit celle de la zone parente
`europe-west4-a` et additionne conservativement les usages du parent et de ses
zones IA associées. Toute autre zone IA sans correspondance documentée est
refusée. Cette dérivation n'est pas présentée comme une preuve de quota caché;
l'API de création GCE reste l'arbitre et peut refuser la requête.

## Créer la VM si elle n'existe plus

```bash
./gcp-migration/deploy.sh
```

Le script demande une confirmation textuelle et refuse d'écraser une instance
existante. Après création et certification du coupe-circuit, il arrête la VM et
vérifie l'état `TERMINATED`; il ne laisse donc pas une machine neuve tourner en
attendant le premier benchmark. Ses invariants sont :

- machine `g4-standard-48` ;
- image `common-cu129-ubuntu-2204-nvidia-580` (CUDA 12.9, pilote 580) ;
- provisioning Spot avec `instanceTerminationAction=STOP` et redémarrage
  automatique désactivé ;
- disque de démarrage Hyperdisk Balanced de 100 Go, provisionné à 3 600 IOPS et
  290 MiB/s ;
- coupe-circuit GCE `maxRunDuration=8h`, réglable à une valeur plus courte avec
  `GCP_MAX_RUN_DURATION`; le script exige une durée comprise entre 30 secondes
  (minimum GCE) et huit heures ;
- résolution de la dernière image non dépréciée de la famille avant la
  confirmation, puis création avec ce nom d'image exact ;
- politique de maintenance `TERMINATE`, protection contre la suppression et
  labels de suivi des coûts, tous relus après création avec le type de machine
  et le provisioning `SPOT` avant l'arrêt immédiat ;
- aucune identité de service attachée à la VM par défaut ; définir explicitement
  `GCP_RUNTIME_SERVICE_ACCOUNT` seulement si le calcul doit appeler des API GCP.

L'expiration du coupe-circuit et une préemption Spot arrêtent donc la VM et
conservent son disque ; elles ne la suppriment pas.

Par défaut, la VM utilise le réseau `default` avec une adresse externe afin de
préserver le chemin SSH existant. Pour une infrastructure durcie, fournissez
`GCP_NETWORK_INTERFACE` avec un VPC contrôlé et la clé `no-address`, puis
utilisez IAP/OS Login. En l'absence de `no-address`, `gcloud` attribue une
adresse externe éphémère et le préflight exige le quota correspondant. La
métadonnée OS Login est activée par le script.

## Ouvrir une session de calcul

N'utilisez jamais directement `gcloud compute instances start`. Le point
d'entrée unique vérifie d'abord que la VM arrêtée porte le bon label, qu'il
s'agit exactement d'une `g4-standard-48` Spot avec maintenance `TERMINATE` et
redémarrage automatique désactivé, que son action est `STOP` et que
`maxRunDuration` est compris entre 30 secondes et huit heures. Après le
démarrage, il relit tous ces invariants, certifie l'échéance GCE, arme un
`shutdown` dans l'OS invité puis relit explicitement son état. Une préemption
pendant le démarrage publie immédiatement la génération ciblée, puis certifie
son arrêt sans attendre le délai normal de cinq minutes. Le délai invité vaut
quatre heures par défaut et ne peut jamais dépasser 480 minutes; il doit rester
compatible avec `maxRunDuration`.

Lorsque Compute Engine expose `terminationTimestamp`, le script exige que cette
valeur concorde à 300 secondes près avec `lastStartTimestamp + maxRunDuration`.
Certaines zones IA n'exposent pas ce champ : uniquement pour une telle zone et
un champ entièrement vide, l'échéance est calculée depuis la nouvelle génération
`lastStartTimestamp` et la durée GCE numérique relue et bornée. Le plafond sûr
du coupe-circuit invité est fixé 300 secondes avant cette échéance calculée, ce
qui couvre aussi l'apparition tardive d'un timestamp valide. Une valeur non vide
mal formée ou incohérente, une génération non fraîche ou une modification de la
durée après démarrage restent refusées.

Le point d'entrée refuse désormais toute clé par défaut persistante ou
indisponible en mode batch. `GCP_SSH_KEY_FILE` doit désigner une paire ED25519
non chiffrée, privée en mode `0600`, déjà présente une seule fois dans OS Login
avec une expiration future. La durée restante doit couvrir `maxRunDuration`
sans le dépasser de plus de 660 secondes. Le script relit et transmet ensuite
l'échéance UTC absolue exacte à gcloud, afin qu'une réimportation implicite ne
puisse ni rendre la clé persistante ni renouveler un TTL relatif. Pour la cible
actuelle bornée à une heure, la préparation manuelle suivante crée une clé de
session hors dépôt et l'expire après 70 minutes :

```bash
export GCP_SSH_KEY_DIR="$(mktemp -d /tmp/ehgp-session-ssh.XXXXXXXX)"
chmod 700 "${GCP_SSH_KEY_DIR}"
export GCP_SSH_KEY_FILE="${GCP_SSH_KEY_DIR}/id_ed25519"
ssh-keygen -q -t ed25519 -N '' -C 'e-hgp-session' -f "${GCP_SSH_KEY_FILE}"
chmod 600 "${GCP_SSH_KEY_FILE}"
export GCP_SSH_KEY_EXPIRATION_UTC="$(python3 - <<'PY'
from datetime import datetime, timedelta, timezone
print((datetime.now(timezone.utc) + timedelta(minutes=70)).isoformat(timespec="seconds").replace("+00:00", "Z"))
PY
)"
gcloud compute os-login ssh-keys add \
  --key-file="${GCP_SSH_KEY_FILE}.pub" \
  --ttl=70m \
  --project="${GCP_PROJECT_ID}"

./gcp-migration/start_and_verify.sh --guest-shutdown-minutes 45
```

Pour une autre durée GCE, adaptez le TTL à la durée persistée sans dépasser la
marge de 660 secondes, puis choisissez un arrêt invité compatible. Une clé
chiffrée, même correcte, est refusée ici afin qu'aucun démarrage facturable ne
dépende d'une invite de passphrase invisible.

Le script est interactif. `--yes` existe pour une exécution non interactive
déjà explicitement autorisée; il ne supprime aucun contrôle. Si GCP, SSH ou
systemd ne permet pas de vérifier un coupe-circuit, le script tente un arrêt
immédiat et échoue fermé.

Connectez-vous ensuite avec la même clé, en indiquant toujours le projet et la
zone :

```bash
gcloud compute ssh "${GCP_INSTANCE_NAME:-ehgp-blackwell-spot}" \
  --project="${GCP_PROJECT_ID}" \
  --zone="${GCP_ZONE:-europe-west4-a}" \
  --ssh-key-file="${GCP_SSH_KEY_FILE}" \
  --ssh-key-expiration="${GCP_SSH_KEY_EXPIRATION_UTC}"
```

Dans la VM, lancez le preflight sans remplacer le coupe-circuit déjà armé :

```bash
cd /chemin/vers/E-HGP
./gcp-migration/blackwell_preflight.sh
```

Sans `--arm-shutdown`, le preflight reste purement diagnostique. Son smoke test
Docker lance un conteneur `--rm` sans montage de volume ; il peut télécharger
l'image `nvidia/cuda:12.9.1-base-ubuntu22.04`. Utilisez `--skip-docker` pour un
diagnostic hors ligne.

Le feu vert exige :

- exactement une RTX PRO 6000, compute capability 12.0 ;
- au moins 95 000 MiB visibles, soit la classe nominale 96 Go ;
- un pilote de branche 580 avec module noyau ouvert ;
- `nvcc` en CUDA 12.9 ;
- un accès GPU fonctionnel depuis Docker/NVIDIA Container Toolkit.

Pour couvrir le `PATH` réduit des connexions non interactives, le preflight
résout `nvcc` d'abord depuis `PATH`, puis depuis `CUDA_HOME/bin`,
`/usr/local/cuda/bin` ou `/usr/local/cuda-12.9/bin`. Le chemin retenu doit être
absolu et exécutable, et la sortie de `nvcc --version` doit toujours annoncer
CUDA 12.9.

Ne lancez les benchmarks lourds qu'après le bilan vert.

## Qualification bornée de la Phase 3

Après autorisation explicite, le point d'entrée Phase 3 réalise une seule session, exige un commit propre déjà poussé sur `origin/main`, puis certifie l'arrêt de la cible exacte même si le worker distant échoue. L'arrêt invité est armé pour 45 minutes à partir de la certification des gardes; contrairement au démarrage générique qui accepte une durée bornée entre 30 secondes et huit heures, cette qualification courte exige `maxRunDuration=3600` secondes exactement :

```bash
./gcp-migration/run_phase3_qualification.sh \
  --yes \
  --result-dir /tmp/morsehgp3d-phase3-qualification
```

Le même worker gardé peut qualifier en plus la référence CUDA exhaustive et la
couche de correction LBVH résidente de la Phase 4, sans leur attribuer de
statut scientifique public :

```bash
./gcp-migration/run_phase3_qualification.sh \
  --yes \
  --phase4-spatial-reference \
  --result-dir /tmp/morsehgp3d-phase3-qualification
```

Cette option ajoute le différentiel exact complet de la référence, les 1 013
cas `Fraction` du LBVH résident parallèle couvrant chaque taille de 1 à 1 000,
un sous-ensemble borné distinct sous `memcheck`, un passage `racecheck` et un
audit AOT séparé pour chacun des deux replays. Le compagnon Phase 4 utilise le
schéma combiné
`morsehgp3d.phase4.spatial_gpu_reference_and_lbvh_qualification.v3`; les deux
artefacts restent provisoires jusqu'à la certification `TERMINATED` de la même
cible et sont alors publiés sous `phase3-<SHA>.json` et
`phase4-spatial-<SHA>.json`.

La boucle GPU Borůvka complète de Phase 5 possède une option plus courte,
indépendante de la campagne spatiale Phase 4 :

```bash
./gcp-migration/run_phase3_qualification.sh \
  --yes \
  --phase5-k1-boruvka \
  --result-dir /tmp/morsehgp3d-phase5-k1-boruvka
```

Le worker exécute le replay réel multi-ronde
`morsehgp3d_gpu_k1_boruvka_full_replay`, exige l'émission GPU par sources
contiguës complètes avec des budgets fermés de 1, 7 et 3 enregistrements pour
les trois fixtures, la chaîne de propositions GPU, les décisions et
contractions CPU exactes, puis un rejeu GPU chunké indépendant et le témoin
EMST local. Les audits lient le volume logique au pic physique d'un tampon
device et d'un tampon hôte de 16 octets par candidat, sans scinder une source
entre deux chunks. Le worker vérifie ensuite un ELF exclusivement `sm_120`,
l'absence de PTX et les passages `compute-sanitizer` en `memcheck` et
`racecheck`. Le compagnon fermé
`morsehgp3d.phase5.k1_boruvka_gpu_qualification.v3`, de périmètre
`gpu_proposed_bounded_candidate_emission_cpu_exact_full_boruvka_local_emst_witness_only`,
reste au statut `worker_passed_pending_shutdown` jusqu'au même arrêt ciblé et
devient ensuite `phase5-k1-boruvka-<SHA>.json`. Il ne contient aucun
`public_status` : la réduction hiérarchique reste `not_performed`. Cette
qualification établit la borne physique des payloads candidats sur les
fixtures fermées; elle ne borne ni les autres tableaux résidents, ni le volume
logique total, ni le travail CPU exact, et ne qualifie donc pas encore la
scalabilité de la Phase 5.

Pour la cible de capacité explicitement autorisée :

```bash
GCP_ZONE=europe-west4-ai1a \
GCP_INSTANCE_NAME=ehgp-blackwell-spot-ai1a \
./gcp-migration/run_phase3_qualification.sh \
  --yes \
  --result-dir /tmp/morsehgp3d-phase3-qualification
```

Si le diagnostic borné a prouvé que l'image invitée ne contient aucun moteur
Docker, la même session peut autoriser explicitement sa préparation :

```bash
GCP_ZONE=europe-west4-ai1a \
GCP_INSTANCE_NAME=ehgp-blackwell-spot-ai1a \
./gcp-migration/run_phase3_qualification.sh \
  --yes \
  --provision-docker \
  --result-dir /tmp/morsehgp3d-phase3-qualification
```

Cette option n'élargit ni la cible ni le cycle de vie GCP. Elle n'est exécutée
qu'après les deux coupe-circuits et avant le worker. Le provisionneur séparé
`phase3_remote_docker_provision.sh` relit lui-même l'arrêt invité, exige Ubuntu
22.04 `amd64`, le toolkit NVIDIA 1.17.8-1 déjà installé et une fenêtre de
travail suffisante. Après son succès, l'orchestrateur recertifie la même
génération GCE et sa durée avant de permettre au worker de relire à son tour la
garde invitée.

Le répertoire de résultat est créé si nécessaire, doit rester hors du dépôt et ne doit déjà contenir ni `phase3-<SHA>.json`, ni `phase3-<SHA>.start-handoff.json`. L'orchestrateur utilise exclusivement `start_and_verify.sh` pour le démarrage et `stop_and_verify.sh --yes` pour l'arrêt. Dès que la garde GCE post-démarrage certifie la génération, le démarrage publie atomiquement un handoff v3 `targeted_running` avec le `lastStartTimestamp`, avant de tenter la garde invitée; une préemption immédiate publie de la même façon un handoff `targeted_stopping`. Toute fermeture automatique exige cette même génération et refuse une cible redémarrée. Si la garde invitée ou l'arrêt d'urgence échoue, ce handoff reste disponible pour reprendre l'arrêt exact et bloque une nouvelle session. Si une commande de démarrage échoue avant que la génération soit lisible, aucun arrêt non versionné n'est tenté : le script signale la cible et la commande de contrôle. Les appels GCP critiques sont bornés par GNU `timeout` dans son mode de groupe de processus par défaut, avec `--kill-after`; le client et ses descendants sont donc arrêtés ensemble à l'expiration. Le worker invité ne pilote aucune ressource GCP : il exige un arrêt `poweroff` futur dans `/run/systemd/shutdown/scheduled`, construit l'image CUDA épinglée, compile les profils release et audit, exécute les sondes runtime et Python/DLPack, vérifie le runtime AOT avec `cuobjdump`, puis passe ce runtime court sous `compute-sanitizer --leak-check full`. Si une unité échoue, il remonte avant nettoyage les 240 dernières lignes et 65 536 octets au plus de son journal, entourés de marqueurs explicites, tout en refusant l'artefact de succès.

Pour cette voie automatisée, aucune préparation SSH manuelle n'est nécessaire : l'orchestrateur crée une clé ED25519 de session dans un chemin physique canonisé hors dépôt, la valide localement, l'importe dans OS Login avec un TTL de 70 minutes juste avant le démarrage, relit son échéance UTC absolue exacte et transmet explicitement les deux à start, SSH et SCP. Tous les transports réutilisent cette échéance fixe, même si gcloud doit réimporter la clé; aucun TTL relatif n'est renouvelé. L'orchestrateur capture aussi l'état et la génération avant start. Après `TERMINATED` ciblé, il tente la révocation puis détruit la copie privée. Si start échoue sans handoff, ce nettoyage exige une seconde preuve `TERMINATED` avec génération inchangée; si l'arrêt ciblé ou cette preuve manque, la clé et le handoff éventuel sont conservés sous l'échéance initiale pour la reprise exacte de l'incident.

Pour cette qualification courte, l'orchestrateur exige après les deux gardes
`maxRunDuration=3600` secondes exactement et la même génération. Il transmet au
worker une échéance GCE sûre, placée 300 secondes avant l'échéance nominale. Le
worker en retranche encore 1 800 secondes. Le preflight, la construction et
chacune des sept unités CUDA ou d'audit de base, ainsi que les unités Phase 4
ou Phase 5 optionnelles, sont exécutés sous le binaire fixe
`/usr/bin/timeout`, dans un groupe de processus distinct. Les chemins fixes de
`timeout`, `date` et `sleep`, ainsi que tous leurs parents, sont certifiés root
et non inscriptibles par le groupe ou les autres avant le premier calcul de
deadline. La borne douce de chaque unité réserve encore cinq secondes pour
tuer les descendants et soixante secondes pour le diagnostic et le nettoyage;
l'unité n'est pas lancée si cette réserve n'est plus disponible. Chaque sonde
Docker ou Buildx relit elle aussi l'horloge fixe et borne son propre timeout au
minimum de son plafond local et du temps encore disponible. L'arrêt invité
doit en outre rester antérieur à l'échéance GCE sûre.

Après le preflight, le worker ignore tout client Docker injecté par `PATH` et
certifie exclusivement le client système fixe `/usr/bin/docker`, d'abord en
accès direct puis, si nécessaire, derrière `sudo -n`. Le binaire et tous ses
parents doivent appartenir à root et ne pas être inscriptibles par le groupe
ou les autres. Lorsque le daemon démarre encore, le worker effectue au
plus six tours, chacun sondant les voies disponibles puis attendant cinq
secondes avant le suivant. Aucune nouvelle sonde ne démarre une fois la
deadline atteinte et chaque sonde est elle-même limitée à cinq secondes. Il
n'installe aucun paquet et ne démarre aucun service. Si l'accès reste
impossible, il remonte un diagnostic
borné comprenant les erreurs directes et sudo, l'état systemd, les paquets
Docker/containerd/NVIDIA et au plus 80 lignes du journal Docker avant de
nettoyer ses temporaires. Toute commande Docker ultérieure conserve la voie
directe certifiée ou exactement `sudo -n -- /usr/bin/docker`; aucun chemin issu
du `PATH` utilisateur n'est exécuté. Chaque `docker run` écrit en outre son
identifiant dans un `cidfile` privé à la session et porte un nom ainsi qu'un
label propres à cette session. Le worker atteste le CID, le nom, l'image et le
label avant tout `docker rm -f` borné, sur succès comme sur échec ou signal,
puis exige des observations répétées de l'absence. Cette attestation fournit
aussi un repli sûr si le daemon a créé le conteneur avant d'écrire le `cidfile`;
un CID incomplet ou un échec de nettoyage ferme la qualification et interdit
l'artefact final.

Le provisionneur optionnel conserve cette séparation : il ne contient aucune
commande GCP et ne lance aucun conteneur. Il n'ajoute aucun dépôt, ne télécharge
aucun script et n'ajoute pas l'utilisateur au groupe `docker`. Si `docker.io`
ou `docker-buildx` manque, il met seulement à jour l'index des dépôts déjà
configurés, capture les candidats exacts, simule leur installation et refuse
toute suppression, mise à niveau ou rétrogradation. L'installation réelle
utilise ensuite les versions capturées avec `--no-remove`, `--no-upgrade` et
`--no-install-recommends`. Docker CE, `containerd.io`, `podman-docker`, Moby,
un état `dpkg` partiel, un binaire élevé non sûr ou un `daemon.json` étranger
font échouer la préparation sans réparation improvisée.

Les exécutables fixes de `/usr/bin` sont certifiés en un lot : le préfixe
partagé n'est sondé qu'une fois, puis chaque fichier conserve ses contrôles de
type, de lien, d'exécution, de propriétaire et de permissions avant une relecture
groupée des métadonnées. Cette réduction des appels privilégiés accélère la
préparation sans réduire l'enveloppe de sécurité.

Chaque unité mutante s'exécute sous un groupe de processus borné : à
l'expiration, `timeout` signale aussi les descendants de l'unité au lieu de
laisser un `apt-get`, un `dpkg` ou un configurateur continuer en arrière-plan.
La postcondition Docker compare en outre le chemin et les arguments du runtime
NVIDIA effectivement exposé par le daemon à la configuration absolue approuvée.

Lorsque la configuration Docker est absente, le seul générateur admis est le
`nvidia-ctk` 1.17.8-1 déjà présent. Le JSON créé doit contenir uniquement le
runtime NVIDIA attendu, rester root et non inscriptible par groupe ou autres,
puis réussir `dockerd --validate`. Le service est activé et redémarré une seule
fois sous timeout. Le même boot ID, l'arrêt invité futur, les services Docker
et containerd, `docker buildx` et le runtime NVIDIA exposé par `docker info`
sont relus avant succès. Un hôte déjà conforme suit une voie strictement
idempotente sans APT, configuration ni systemd. Les commandes privilégiées
utilisent toutes des chemins système fixes; les diagnostics restent bornés à
240 lignes, 65 536 octets et 80 lignes du journal Docker. Les procédures
suivent le paquet `docker.io` pris en charge par
[Canonical](https://ubuntu.com/server/docs/how-to/containers/docker-for-system-admins/)
et la configuration Docker prescrite par
[NVIDIA](https://docs.nvidia.com/datacenter/cloud-native/container-toolkit/1.17.8/install-guide.html).

L'artefact distant demeure provisoire avec `status=worker_passed_pending_shutdown`. L'orchestrateur le valide localement, arrête la cible, relit indépendamment l'état exact `TERMINATED`, ajoute cette preuve à `vm_lifecycle`, convertit le statut en `passed`, puis publie l'artefact final sans remplacement par lien dur. Un run Phase 3 seul effectue une publication atomique de fichier unique. Avec `--phase4-spatial-reference` ou `--phase5-k1-boruvka`, les noms ne sont pas présentés comme une transaction atomique impossible : Phase 3, artefact autonome, est liée en premier, puis chaque compagnon demandé. Chaque lien est atomique et sans remplacement. Si un lien compagnon échoue, les artefacts valides déjà publiés sont conservés et le diagnostic énumère précisément leurs noms; aucun rollback ne supprime un nom final susceptible d'avoir été remplacé concurremment. Un échec ou une relecture illisible de l'arrêt ne publie aucun artefact final, mais conserve le handoff ciblé local et bloque une nouvelle session sur le même SHA jusqu'à résolution. La priorité donnée à l'arrêt signifie que le clone temporaire distant peut rester dans `/tmp` sur le disque de la VM arrêtée.

## Cas Blackwell : « requires use of the NVIDIA open kernel modules »

Le preflight cherche ce message exact dans les journaux noyau. Il affiche aussi
le module chargé et l'état des paquets. Pour une inspection manuelle :

```bash
sudo journalctl -k -b --no-pager | \
  grep -F "requires use of the NVIDIA open kernel modules"

modinfo -F license nvidia
dpkg-query -W \
  linux-modules-nvidia-580-server-open-gcp \
  nvidia-driver-580-server-open
```

Si et seulement si le message est présent, demandez la réparation :

```bash
./gcp-migration/blackwell_preflight.sh \
  --install-open-driver \
  --skip-docker
```

Le script exige la phrase `INSTALLER 580-SERVER-OPEN`, puis exécute uniquement
l'équivalent de :

```bash
sudo apt-get update
sudo apt-get install --no-remove \
  linux-modules-nvidia-580-server-open-gcp \
  nvidia-driver-580-server-open
```

Il ne lance ni `purge`, ni `autoremove`, ni reboot. Une fois l'installation
terminée, redémarrez explicitement, réarmez immédiatement le coupe-circuit et exigez un
preflight entièrement vert avant les tests :

```bash
sudo reboot
# Après reconnexion :
./gcp-migration/blackwell_preflight.sh --arm-shutdown 240
```

L'option APT `--no-remove` fait échouer la réparation plutôt que de désinstaller
automatiquement un pilote en conflit. Dans ce cas, examinez le plan APT et les
paquets déjà installés avant toute intervention manuelle.

## Arrêter et vérifier après les calculs

Depuis la machine locale, utilisez le script de fermeture après chaque session,
y compris après une erreur, une interruption ou un benchmark abandonné. Il
demande de taper `STOPPER ehgp-blackwell-spot`, attend la fin de l'arrêt et
n'accepte comme succès que l'état GCE `TERMINATED` (qui signifie « arrêtée »,
pas « supprimée »). Avant toute mutation, il refuse une cible qui ne porte pas
le label `project=e-hgp`. Il inventorie ensuite les autres VM labellisées et
signale celles qui sont actives, sans les arrêter ni faire échouer la
certification de la cible.

```bash
./gcp-migration/stop_and_verify.sh
```

Le mode non interactif, à réserver à une fermeture explicitement autorisée, est :

```bash
./gcp-migration/stop_and_verify.sh --yes
```

Après confirmation `TERMINATED` d'une session ouverte manuellement, révoquez
la clé OS Login exacte avant d'effacer sa copie locale. Ne faites pas ce
nettoyage tant qu'un incident d'arrêt reste non certifié, car cette clé bornée
peut encore servir à la reprise ciblée :

```bash
gcloud compute os-login ssh-keys remove \
  --key-file="${GCP_SSH_KEY_FILE}.pub" \
  --project="${GCP_PROJECT_ID}"
rm -f -- "${GCP_SSH_KEY_FILE}" "${GCP_SSH_KEY_FILE}.pub"
rmdir -- "${GCP_SSH_KEY_DIR}"
unset GCP_SSH_KEY_FILE GCP_SSH_KEY_DIR GCP_SSH_KEY_EXPIRATION_UTC
```

La vérification manuelle équivalente est :

```bash
INSTANCE_NAME="${GCP_INSTANCE_NAME:-ehgp-blackwell-spot}"
ZONE="${GCP_ZONE:-europe-west4-a}"

gcloud compute instances stop "${INSTANCE_NAME}" \
  --project="${GCP_PROJECT_ID}" \
  --zone="${ZONE}"

gcloud compute instances describe "${INSTANCE_NAME}" \
  --project="${GCP_PROJECT_ID}" \
  --zone="${ZONE}" \
  --format='value(status)'
```

La dernière commande doit afficher `TERMINATED`. Si l'état de la cible est
illisible, sa fermeture n'est pas certifiée : contrôlez immédiatement le
projet, la zone et l'instance dans la console GCP. Un inventaire global
illisible est signalé, mais n'autorise aucune mutation des autres ressources.

## Contrôles du dépôt

La syntaxe des scripts est vérifiée à chaque passage de la CI :

```bash
python -m unittest discover -s tests/gcp -p 'test_*.py'
python tools/check_gcp_workflows.py
bash -n gcp-migration/*.sh
```

Le workflow manuel `.github/workflows/gcp.yml` utilise les versions courantes
de `google-github-actions/auth` et `setup-gcloud`. Les variables GitHub
`GCP_PROJECT_ID`, `GCP_INSTANCE_NAME` et `GCP_ZONE` peuvent remplacer les
valeurs par défaut sans modifier le workflow. La variable obligatoire
`GCP_VIEWER_SERVICE_ACCOUNT` doit désigner un compte de service dédié à la CI,
doté uniquement de `roles/compute.viewer` sur le projet; le compte de déploiement
ou un rôle tel que `roles/compute.instanceAdmin` est interdit. La relation WIF
côté GCP doit rester restreinte à ce dépôt et à ce workflow. Le contrôle
`tools/check_gcp_workflows.py` n'autorise que les lectures `gcloud auth list` et
`gcloud compute instances list|describe`, et refuse les blocs YAML repliés qui
pourraient masquer une commande.
