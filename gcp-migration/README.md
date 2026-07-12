# Procédure de Migration vers GCP (g4-standard-48)

Ce répertoire contient les scripts et instructions pour migrer votre GitHub Codespace vers une instance Compute Engine `g4-standard-48` dans la région `europe-west4`.

---

## 🛠️ État actuel et Historique
- **Statut de l'instance :** **🟢 RUNNING**
- **Nom de la VM :** `ehgp-blackwell-spot`
- **Zone :** `europe-west4-a`
- **IP Externe :** `34.178.198.3`
- **IP Interne :** `10.164.0.4`
- **Machine Type :** `g4-standard-48` (1x NVIDIA RTX PRO 6000 Blackwell GPU, 48 vCPUs, 180 GB RAM).

> [!NOTE]
> Il y avait une confusion initiale sur l'architecture : la machine `g4-standard-48` embarque **1 seul GPU NVIDIA RTX PRO 6000 (Blackwell)** et non 4x NVIDIA L4. Vos quotas actuels (`GPUS_ALL_REGIONS` à 1.0 et `PREEMPTIBLE_CPUS` à 48.0) sont donc déjà suffisants pour faire tourner cette instance.

---

## 🚀 Utilisation des Scripts

### 1. Valider les quotas requis
```bash
./gcp-migration/check_quotas.sh
```

### 2. Gestion de l'instance VM
Comme l'instance est déjà créée, vous n'avez plus besoin de la commande `create`. Utilisez plutôt les commandes de démarrage et d'arrêt pour gérer ses coûts :

- **Démarrer l'instance :**
  ```bash
  gcloud compute instances start ehgp-blackwell-spot --zone="europe-west4-a"
  ```
- **Arrêter l'instance (pour ne pas consommer de crédits) :**
  ```bash
  gcloud compute instances stop ehgp-blackwell-spot --zone="europe-west4-a"
  ```
- **Se connecter en SSH à l'instance :**
  ```bash
  gcloud compute ssh ehgp-blackwell-spot --zone="europe-west4-a"
  ```
