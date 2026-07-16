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

La sécurité repose sur deux coupe-circuits indépendants : l'arrêt GCE
`maxRunDuration` avec action `STOP`, puis un `shutdown` programmé dans l'OS
invité. Une session ne commence que si les deux sont vérifiés et se termine par
la certification `TERMINATED` de la cible exacte qui a été créée ou démarrée.
Les autres VM portant le label `project=e-hgp` sont inventoriées à titre
d'information, sans arrêt automatique : elles peuvent appartenir à une session
concurrente.

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

Le contrôle utilise le quota réellement disponible (`limite - usage`) et
échoue si la métrique RTX PRO 6000 Spot est absente, au lieu de supposer qu'elle
vaut un.

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
- provisioning Spot avec `instanceTerminationAction=STOP` ;
- disque de démarrage Hyperdisk Balanced de 100 Go ;
- coupe-circuit GCE `maxRunDuration=8h`, réglable à une valeur plus courte avec
  `GCP_MAX_RUN_DURATION`; le script exige une durée comprise entre 30 secondes
  (minimum GCE) et huit heures ;
- résolution de la dernière image non dépréciée de la famille avant la
  confirmation, puis création avec ce nom d'image exact ;
- politique de maintenance `TERMINATE`, protection contre la suppression et
  labels de suivi des coûts ;
- aucune identité de service attachée à la VM par défaut ; définir explicitement
  `GCP_RUNTIME_SERVICE_ACCOUNT` seulement si le calcul doit appeler des API GCP.

L'expiration du coupe-circuit et une préemption Spot arrêtent donc la VM et
conservent son disque ; elles ne la suppriment pas.

Par défaut, la VM utilise le réseau `default` avec une adresse externe afin de
préserver le chemin SSH existant. Pour une infrastructure durcie, fournissez
`GCP_NETWORK_INTERFACE` avec un VPC contrôlé et utilisez IAP/OS Login. La
métadonnée OS Login est activée par le script.

## Ouvrir une session de calcul

N'utilisez jamais directement `gcloud compute instances start`. Le point
d'entrée unique vérifie d'abord que la VM arrêtée porte le bon label, qu'il
s'agit exactement d'une `g4-standard-48` Spot, que son action est `STOP` et que
`maxRunDuration` est compris entre 30 secondes et huit heures. Après le
démarrage, il certifie l'échéance GCE, arme un `shutdown` dans l'OS invité puis
relit explicitement son état. Le délai invité vaut quatre heures par défaut et
ne peut jamais dépasser 480 minutes :

```bash
./gcp-migration/start_and_verify.sh
```

Pour une session plus courte :

```bash
./gcp-migration/start_and_verify.sh --guest-shutdown-minutes 90
```

Le script est interactif. `--yes` existe pour une exécution non interactive
déjà explicitement autorisée; il ne supprime aucun contrôle. Si GCP, SSH ou
systemd ne permet pas de vérifier un coupe-circuit, le script tente un arrêt
immédiat et échoue fermé.

Connectez-vous ensuite en indiquant toujours le projet et la zone :

```bash
gcloud compute ssh "${GCP_INSTANCE_NAME:-ehgp-blackwell-spot}" \
  --project="${GCP_PROJECT_ID}" \
  --zone="${GCP_ZONE:-europe-west4-a}"
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

Ne lancez les benchmarks lourds qu'après le bilan vert.

## Qualification bornée de la Phase 3

Après autorisation explicite, le point d'entrée Phase 3 réalise une seule session, exige un commit propre déjà poussé sur `origin/main`, puis certifie l'arrêt de la cible exacte même si le worker distant échoue. L'arrêt invité est armé pour 45 minutes à partir de la certification des gardes; le coupe-circuit GCE reste indépendamment borné entre 30 secondes et huit heures :

```bash
./gcp-migration/run_phase3_qualification.sh \
  --yes \
  --result-dir /tmp/morsehgp3d-phase3-qualification
```

Le répertoire de résultat est créé si nécessaire, doit rester hors du dépôt et ne doit déjà contenir ni `phase3-<SHA>.json`, ni `phase3-<SHA>.start-handoff.json`. L'orchestrateur utilise exclusivement `start_and_verify.sh` pour le démarrage et `stop_and_verify.sh --yes` pour l'arrêt. Dès que la garde GCE post-démarrage certifie la génération, le démarrage publie atomiquement un handoff v3 ciblé avec le `lastStartTimestamp`, avant de tenter la garde invitée; toute fermeture automatique exige cette même génération et refuse une cible redémarrée. Si la garde invitée ou l'arrêt d'urgence échoue, ce handoff reste disponible pour reprendre l'arrêt exact et bloque une nouvelle session. Si une commande de démarrage échoue avant que la génération soit lisible, aucun arrêt non versionné n'est tenté : le script signale la cible et la commande de contrôle. Les appels GCP critiques sont bornés par GNU `timeout` avec `--foreground` et `--kill-after`. Le worker invité ne pilote aucune ressource GCP : il exige un arrêt `poweroff` futur dans `/run/systemd/shutdown/scheduled`, construit l'image CUDA épinglée, compile les profils release et audit, exécute les sondes runtime et Python/DLPack, vérifie le runtime AOT avec `cuobjdump`, puis passe ce runtime court sous `compute-sanitizer --leak-check full`.

L'artefact distant demeure provisoire avec `status=worker_passed_pending_shutdown`. L'orchestrateur le valide localement, arrête la cible, relit indépendamment l'état exact `TERMINATED`, ajoute cette preuve à `vm_lifecycle`, convertit le statut en `passed`, puis publie l'artefact final atomiquement et sans remplacement par lien dur. Un échec ou une relecture illisible de l'arrêt ne laisse aucun artefact final, mais conserve le handoff ciblé local et bloque une nouvelle session sur le même SHA jusqu'à résolution. La priorité donnée à l'arrêt signifie que le clone temporaire distant peut rester dans `/tmp` sur le disque de la VM arrêtée.

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
