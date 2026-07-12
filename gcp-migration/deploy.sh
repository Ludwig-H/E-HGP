#!/bin/bash
set -e

echo "[DEPLOY] Déploiement de l'instance ehgp-blackwell-spot (g4-standard-48)..."

gcloud compute instances create ehgp-blackwell-spot \
    --project="$(gcloud config get-value project)" \
    --zone="europe-west4-a" \
    --machine-type="g4-standard-48" \
    --provisioning-model="SPOT" \
    --instance-termination-action="TERMINATE" \
    --image-family="common-cu122-ubuntu-2204" \
    --image-project="deeplearning-platform-release" \
    --boot-disk-size="100GB" \
    --boot-disk-type="hyperdisk-balanced" \
    --network-interface="network=default,access-config-type=ONE_TO_ONE_NAT" \
    --metadata="install-nvidia-driver=true"
