# Procédure de Migration vers GCP (g4-standard-48)

Ce répertoire contient les scripts et instructions pour migrer votre GitHub Codespace vers une instance Compute Engine `g4-standard-48` dans la région `europe-west4`.

---

## 🛠️ État actuel et Historique
- **Authentification GCP :** Réalisée avec succès lors de la session précédente pour le compte `louis.hauseux@gmail.com` sur le projet GCP `devpod-gpu-exploration`.
- **Audit des quotas :** Exécuté et en échec. Les quotas par défaut bloquent actuellement le démarrage de cette VM.

---

## 📋 Action Requise : Demande de Quotas

Avant de lancer le déploiement, vous devez effectuer une demande d'augmentation manuelle dans la console GCP dans la section **IAM et administration > Quotas** (ou via [ce lien direct](https://console.cloud.google.com/iam-admin/quotas?project=devpod-gpu-exploration)).

| Nom de la métrique dans GCP | Dimension / Région | Valeur cible requise |
| :--- | :---: | :---: |
| **`GPUs (all regions)`** | Global | **`4`** |
| **`NVIDIA L4 GPUs`** | `europe-west4` | **`4`** |
| **`Preemptible NVIDIA L4 GPUs`** | `europe-west4` | **`4`** |
| **`Preemptible CPUs`** | `europe-west4` | **`48`** |

---

## 🚀 Utilisation des Scripts lors de la Prochaine Session

Une fois que les quotas ont été approuvés par Google Cloud, ouvrez un terminal dans ce Codespace et suivez ces étapes :

### 1. Valider que les quotas sont prêts
Exécutez le script d'audit des quotas pour obtenir le feu vert officiel :
```bash
chmod +x gcp-migration/check_quotas.sh
./gcp-migration/check_quotas.sh
```
*Si le script affiche `[SUCCÈS] Feu vert des quotas`, vous pouvez passer à l'étape suivante.*

### 2. Déployer l'instance VM
Exécutez le script de déploiement pour instancier la machine :
```bash
chmod +x gcp-migration/deploy.sh
./gcp-migration/deploy.sh
```

---

## ⚙️ Détails techniques de l'architecture
* **Type de machine :** `g4-standard-48` (embarquant nativement 4 GPU NVIDIA L4). Pas besoin de flag `--accelerator`.
* **Modèle de provisionnement :** `SPOT` (tarification économique).
* **Action en cas de préemption :** `TERMINATE` (la machine s'éteint mais le disque dur Hyperdisk Balanced persistant de 100 Go est conservé intact).
* **Image système :** `common-cu122-ubuntu-2204` de `deeplearning-platform-release` (Ubuntu avec pilotes CUDA 12.2 pré-configurés).
