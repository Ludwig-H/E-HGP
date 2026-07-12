#!/bin/bash
set -e

echo "[CHECK] Analyse des plafonds réels requis pour une g4-standard-48..."

# 1. Extraction du plafond mondial de GPU
GLOBAL_GPU=$(gcloud compute project-info describe --format="json" | jq -r '.quotas[] | select(.metric == "GPUS_ALL_REGIONS") | .limit')

# 2. Extraction des métriques régionales
REGIONAL_DATA=$(gcloud compute regions describe europe-west4 --format="json")
CPUS_SPOT=$(echo "$REGIONAL_DATA" | jq -r '.quotas[] | select(.metric == "PREEMPTIBLE_CPUS") | .limit')

# Extraction facultative de la métrique spécifique RTX 6000 si elle existe (par défaut peut être non listée si non restreinte)
RTX_SPOT=$(echo "$REGIONAL_DATA" | jq -r '.quotas[] | select(.metric == "PREEMPTIBLE_NVIDIA_RTX_PRO_6000_GPUS") | .limit')
RTX_SPOT=${RTX_SPOT:-1.0} # Si non listée, on assume qu'elle n'est pas restreinte à 0 par défaut

echo "--------------------------------------------------"
echo "MÉTRIQUE                   | ACTUEL | REQUIS"
echo "--------------------------------------------------"
echo "GPUS_ALL_REGIONS           | $GLOBAL_GPU    | >= 1.0"
echo "PREEMPTIBLE_CPUS           | $CPUS_SPOT    | >= 48.0"
echo "PREEMPTIBLE_RTX_PRO_6000   | $RTX_SPOT    | >= 1.0"
echo "--------------------------------------------------"

# Validation logique à l'aide de jq
IS_GLOBAL_GPU_LOW=$(jq -n "${GLOBAL_GPU:-0} < 1.0")
IS_CPUS_SPOT_LOW=$(jq -n "${CPUS_SPOT:-0} < 48.0")
IS_RTX_SPOT_LOW=$(jq -n "${RTX_SPOT:-0} < 1.0")

if [ "$IS_GLOBAL_GPU_LOW" = "true" ] || \
   [ "$IS_CPUS_SPOT_LOW" = "true" ] || \
   [ "$IS_RTX_SPOT_LOW" = "true" ]; then
    echo "[ÉCHEC] Les quotas actuels interdisent le boot d'une g4-standard-48."
    exit 1
else
    echo "[SUCCÈS] Feu vert des quotas. Passage à la phase d'instanciation."
fi
