# Parité device M5a — frontière device-tiled supports 3/4 (G4, 5-6 août 2026)

- Commit source : `f5a45e5` (moteur+kernel `733d237`, harnais `f5a45e5`).
- Machine : `ehgp-blackwell-spot-ai1a` (g4-standard-48 SPOT, europe-west4-ai1a),
  GPU **NVIDIA RTX PRO 6000 Blackwell Server Edition** (sm_120), conteneur
  `morsehgp3d-phase3:f5a45e5` (CUDA 12.9.2), preset `cuda-release`.
- Compilation du kernel natif `phase15_higher_support_device_tiled_frontier.cu` :
  **première passe nvcc sans aucune erreur ni avertissement bloquant**.
- Outil : `morsehgp3d_gpu_higher_support_device_tiled_frontier_parity`
  (seam natif CUDA vs driver moteur hôte certifié bit-identique au fake par la
  suite locale ; comparaison INTÉGRALE, sans aucun masque de champ).

## Sortie intégrale de l'outil (premier run)

```
device/line12/K5: OK (2 chunks)
device/line12/K3: OK (1 chunks)
device/clusters10/K5: OK (8 chunks)
device/sphere8/K3: OK (2 chunks)
device/sphere8/carrier: OK (2 chunks)
device/clusters10/quantum2: OK (8 chunks)
device/wide-dyadic/K4: OK (1 chunks)
higher-support device tiled frontier device parity passed
EXIT=0
```

Couverture : contrôles de slots (y compris les compteurs de routage
deferred_int512/1024 et rational_drain, non masqués), records de prune,
records terminaux et reçus de sonde bit-identiques chunk par chunk ; tuiles
multi-chunks (8 chunks), deux politiques terminales, quantum minimal 2,
drain rationnel hôte exercé (nuage 1e200/1e-200). Par transitivité avec la
suite locale `gpu_higher_support_device_tiled_slot_engine` :
**natif CUDA ≡ moteur hôte ≡ fake scientifique**.

Non-claims : cette parité ne constitue ni l'artefact de qualification formel
du composant (schéma type v6 — M5b), ni la re-mesure no-go n=32, ni un SLO,
ni un statut public. `exact_higher_support_terminal_classification_native_cuda`
reste `false` (classification boule fermée hôte).
