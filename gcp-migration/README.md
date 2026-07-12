# Guide de Gestion de l'Instance GCP (g4-standard-48)

Ce répertoire contient les scripts et instructions pour gérer votre VM Compute Engine `g4-standard-48` dans la zone `europe-west4-a` pour vos calculs E-HGP.

---

## 🚀 Commandes de Gestion Quotidienne

### 1. Démarrer la machine
Allume l'instance de calcul (le démarrage prend généralement moins d'une minute) :
```bash
gcloud compute instances start ehgp-blackwell-spot --zone="europe-west4-a"
```

### 2. Se connecter en SSH
Ouvre une session interactive sécurisée sur la machine pour lancer vos scripts :
```bash
gcloud compute ssh ehgp-blackwell-spot --zone="europe-west4-a"
```

### 3. Éteindre la machine (Important pour la facturation)
Arrête l'instance. La facturation des ressources de calcul est stoppée immédiatement. Le disque dur Hyperdisk Balanced est conservé intact pour votre prochaine session :
```bash
gcloud compute instances stop ehgp-blackwell-spot --zone="europe-west4-a"
```

---

## 🛠️ Recréer la VM (Si elle a été supprimée)

Si l'instance a été supprimée et doit être réinstanciée avec les pilotes NVIDIA et CUDA 12.9 préconfigurés, exécutez la commande suivante :

```bash
gcloud compute instances create ehgp-blackwell-spot \
    --project="devpod-gpu-exploration" \
    --zone="europe-west4-a" \
    --machine-type="g4-standard-48" \
    --provisioning-model="SPOT" \
    --instance-termination-action="STOP" \
    --image-family="common-cu129-ubuntu-2204-nvidia-580" \
    --image-project="deeplearning-platform-release" \
    --boot-disk-size="100GB" \
    --boot-disk-type="hyperdisk-balanced" \
    --network-interface="network=default" \
    --metadata="install-nvidia-driver=true"
```

---

## 📊 Script d'Audit des Quotas

Pour vérifier que vos quotas GCP globaux et régionaux sont suffisants pour démarrer cette instance :
```bash
./gcp-migration/check_quotas.sh
```
*(Requis : `GPUS_ALL_REGIONS` >= 1.0, `PREEMPTIBLE_CPUS` >= 48.0).*
