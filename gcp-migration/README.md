# Infrastructure de benchmark GPU sur GCP

> [!IMPORTANT]
> Ce répertoire est maintenu comme infrastructure active. Les scripts créent et
> pilotent des ressources facturables, mais exigent des confirmations
> interactives et ne suppriment jamais une instance ou un disque.

Ce répertoire gère par défaut la VM Compute Engine `ehgp-blackwell-spot`, une
`g4-standard-48` de `europe-west4-a` dotée d'une RTX PRO 6000 Blackwell
Server Edition (96 Go de VRAM). Cette configuration a été revérifiée le
14 juillet 2026 dans la documentation officielle : [série G4](https://cloud.google.com/compute/docs/accelerator-optimized-machines), [zones GPU](https://cloud.google.com/compute/docs/gpus/gpu-regions-zones) et [images Deep Learning VM](https://cloud.google.com/deep-learning-vm/docs/images).

La disponibilité effective d'une VM Spot n'est jamais garantie par le seul
quota. Les commandes de création et d'arrêt doivent être lancées depuis un
terminal local; le workflow GitHub ne fait qu'un contrôle de connexion en
lecture seule, déclenché manuellement.

## Dépendances et configuration

Les scripts exigent Bash, `gcloud` et, pour le contrôle des quotas, `jq`.
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
existante. Ses invariants sont :

- machine `g4-standard-48` ;
- image `common-cu129-ubuntu-2204-nvidia-580` (CUDA 12.9, pilote 580) ;
- provisioning Spot avec `instanceTerminationAction=STOP` ;
- disque de démarrage Hyperdisk Balanced de 100 Go ;
- coupe-circuit GCE `maxRunDuration=8h`, réglable avant création avec
  `GCP_MAX_RUN_DURATION` (par exemple `GCP_MAX_RUN_DURATION=12h`) ;
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

Démarrez puis connectez-vous en indiquant toujours le projet :

```bash
INSTANCE_NAME="${GCP_INSTANCE_NAME:-ehgp-blackwell-spot}"
ZONE="${GCP_ZONE:-europe-west4-a}"

gcloud compute instances start "${INSTANCE_NAME}" \
  --project="${GCP_PROJECT_ID}" \
  --zone="${ZONE}"

gcloud compute ssh "${INSTANCE_NAME}" \
  --project="${GCP_PROJECT_ID}" \
  --zone="${ZONE}"
```

Dans la VM, armez immédiatement un second coupe-circuit et lancez le preflight.
Le délai ci-dessous est de quatre heures et reste armé après la fin du script :

```bash
cd /chemin/vers/E-HGP
./gcp-migration/blackwell_preflight.sh --arm-shutdown 240
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
terminée, redémarrez explicitement, réarmez le coupe-circuit et exigez un
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

Depuis la machine locale, utilisez le script de fermeture. Il demande de taper
`STOPPER ehgp-blackwell-spot`, attend la fin de l'arrêt et n'accepte comme succès
que l'état GCE `TERMINATED` (qui signifie « arrêtée », pas « supprimée »).

```bash
./gcp-migration/stop_and_verify.sh
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

La dernière commande doit afficher `TERMINATED`. Si ce n'est pas le cas,
contrôlez immédiatement la VM dans la console GCP.

## Contrôles du dépôt

La syntaxe des scripts est vérifiée à chaque passage de la CI :

```bash
bash -n gcp-migration/*.sh
```

Le workflow manuel `.github/workflows/gcp.yml` utilise les versions courantes
de `google-github-actions/auth` et `setup-gcloud`. Les variables GitHub
`GCP_PROJECT_ID`, `GCP_INSTANCE_NAME` et `GCP_ZONE` peuvent remplacer les
valeurs par défaut sans modifier le workflow. La relation WIF côté GCP doit
rester restreinte à ce dépôt et à ce workflow.
