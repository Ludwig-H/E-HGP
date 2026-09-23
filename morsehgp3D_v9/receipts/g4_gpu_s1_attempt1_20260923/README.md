# Reçu : session G4 S1, tentative 1 (échec de configuration CMake)

23 septembre 2026, 13:43–13:46 UTC (VM). Cadre : `exploration_v9_hors_registre`,
`backend=cuda_g4`, `profile=quantized_u18_input_only`,
`public_status=not_claimed`.

**GCP utilisé.**
- **Session** : SPOT gardée sur la cible fixe `devpod-gpu-exploration /
  us-central1-b / ehgp-v7-4fa0e0789a7d5bb06b787d35` (g4-standard-48),
  génération `2026-09-23T06:43:29.921-07:00`.
- **Arrêt** : arrêt ciblé certifié par le contrôleur, puis `TERMINATED` relu
  en lecture seule (dernier arrêt `2026-09-23T06:46:20.733-07:00`).
- **GPU** : aucun passage exécuté (`GPU_attempted` et `GPU_executed` faux).

Paquet construit depuis `7565451f` : protocole au commit, snapshot
`7ad078bb…`, contrôleur `133cca17…`, worker `0df81b65…`.

## Ce qui s'est passé

Le worker a validé la garde, les 48 vCPU et le paquet, puis relevé l'outillage :
- g++ 11.4.0 ;
- nvcc 12.9 (V12.9.41) ;
- RTX PRO 6000 Blackwell Server Edition, pilote 580.173.02, 97 887 Mio,
  sm 12.0 ;
- **CMake 3.22.1**.

La configuration a échoué, avant toute compilation :

```text
Target "mhgp9_gpu" requires the language dialect "CUDA20" , but CMake does
not know the compile flags to use to enable it.
```

CMake 3.22 ne connaît pas `CUDA_STANDARD 20` ; mon contrôle local avait été
fait sous CMake 3.28. Statut du worker : `build_failed`. Statut hôte :
`worker_failed`, capture reçue.

## Correction

L'unité CUDA n'emploie que du C++17 (`if constexpr`, variables `inline
constexpr`) : `CUDA_STANDARD 17`. La configuration et la compilation de
`mhgp9_gpu_filter_probe` ont été rejouées localement sous le même CMake
3.22.1 (installé par pip) avec nvcc 12.9. Le relevé de dépendances du worker
accepte les depfiles réels, celui de nvcc compris.

## Contenu

- `vm/` : sortie du worker (inventaire, versions, configuration, reçu).
- `host/` : journaux expurgés du contrôleur, sans `oslogin_add`, ni clé, ni
  archive.
- `PACKAGE.json`, `SHA256SUMS`.
