#!/bin/bash
set -e

echo "[CHECK] Analyse des plafonds globaux et régionaux pour europe-west4..."

# 1. Extraction du plafond mondial de GPU
GLOBAL_GPU=$(gcloud compute project-info describe --format="json" | jq -r '.quotas[] | select(.metric == "GPUS_ALL_REGIONS") | .limit')

# 2. Extraction des métriques régionales
REGIONAL_DATA=$(gcloud compute regions describe europe-west4 --format="json")
L4_NOMINAL=$(echo "$REGIONAL_DATA" | jq -r '.quotas[] | select(.metric == "NVIDIA_L4_GPUS") | .limit')
L4_SPOT=$(echo "$REGIONAL_DATA" | jq -r '.quotas[] | select(.metric == "PREEMPTIBLE_NVIDIA_L4_GPUS") | .limit')
CPUS_SPOT=$(echo "$REGIONAL_DATA" | jq -r '.quotas[] | select(.metric == "PREEMPTIBLE_CPUS") | .limit')

echo "--------------------------------------------------"
echo "MÉTRIQUE                   | ACTUEL | REQUIS"
echo "--------------------------------------------------"
echo "GPUS_ALL_REGIONS           | $GLOBAL_GPU    | >= 4.0"
echo "NVIDIA_L4_GPUS             | $L4_NOMINAL    | >= 4.0"
echo "PREEMPTIBLE_NVIDIA_L4_GPUS | $L4_SPOT    | >= 4.0"
echo "PREEMPTIBLE_CPUS           | $CPUS_SPOT    | >= 48.0"
echo "--------------------------------------------------"

# Validation logique à l'aide de jq
IS_GLOBAL_GPU_LOW=$(jq -n "${GLOBAL_GPU:-0} < 4.0")
IS_L4_NOMINAL_LOW=$(jq -n "${L4_NOMINAL:-0} < 4.0")
IS_L4_SPOT_LOW=$(jq -n "${L4_SPOT:-0} < 4.0")
IS_CPUS_SPOT_LOW=$(jq -n "${CPUS_SPOT:-0} < 48.0")

if [ "$IS_GLOBAL_GPU_LOW" = "true" ] || \
   [ "$IS_L4_NOMINAL_LOW" = "true" ] || \
   [ "$IS_L4_SPOT_LOW" = "true" ] || \
   [ "$IS_CPUS_SPOT_LOW" = "true" ]; then
    echo "[ÉCHEC] Les quotas actuels interdisent le boot d'une g4-standard-48."
    exit 1
else
    echo "[SUCCÈS] Feu vert des quotas. Passage à la phase d'instanciation."
fi
