# Reçu : sonde CUDA sur la VM G4 (29 septembre 2026)

`backend=cuda_g4` pour cette sonde seulement, `public_status=not_claimed`. Aucune partie de la v10 ne tourne sur GPU.

## Session

- Commit `3b3ea7241`. Plan [`plan_s7_cuda.json`](plan_s7_cuda.json) : `bench/g4/cuda_probe.py` compile
  `bench/g4/cuda_probe.cu` avec `/usr/local/cuda/bin/nvcc -O3 -std=c++17 -arch=sm_120` ([`compile.txt`](compile.txt))
  puis l'exécute.
- VM `g4-standard-48` SPOT. **Arrêt certifié TERMINATED** sur la cible exacte : génération `12:42:36.374-07:00`,
  arrêt `12:49:24.820-07:00` ; clé retirée. Le statut `failed_remote` ne vient que de pip.
- `session/receipt.json` et `session/preflight.json` : adresse du compte masquée.

## Résultats ([`cuda_probe.json`](cuda_probe.json))

- Device : NVIDIA RTX PRO 6000 Blackwell Server Edition, capacité 12.0, 188 SM, 95 Gio.
- **Exactitude de `__int128` sur le device** : 16 777 216 paires de produits i64 × i64 comparés, contre l'hôte.
  - 2 011 255 égalités forcées, avec les bornes INT64_MIN et INT64_MAX.
  - **0 écart**.
- **Débit** d'une chaîne dépendante de produits et d'accumulations : 2 579 G op/s en i64, 1 111 G op/s en i128.
  Le 128 bits coûte ×2,3 seulement.
- Bande passante en mémoire épinglée : 56,8 Go/s de l'hôte vers le GPU, 56,4 Go/s en retour.
- Latence de lancement d'un noyau vide : 1,86 µs.

## Lecture

- L'arithmétique exacte de la v10 se porte sur ce GPU.
  - Les prédicats du filtre et des feuilles sont en i64 et i128.
  - Les niveaux sont en I192/I128 : il faudra émuler le 192 bits sur trois mots, sur le même modèle.
- Avec 57 Go/s, transférer les listes des feuilles d'une trame LiDAR (quelques dizaines de Mo) prend moins d'une
  milliseconde.
- Ces chiffres alimentent la révision du plan GPU (J4 et suivants), en cours dans le workflow de conception.
