# Reçu : session G4 S1, filtre témoin exact q3/q4 sur GPU (seuil franchi)

23 septembre 2026, 13:50–13:53 UTC (VM). Cadre : `exploration_v9_hors_registre`,
`backend=cuda_g4`, `profile=quantized_u18_input_only`,
`public_status=not_claimed`.

**GCP utilisé.**
- **Session** : SPOT gardée sur la cible fixe `devpod-gpu-exploration /
  us-central1-b / ehgp-v7-4fa0e0789a7d5bb06b787d35` (g4-standard-48),
  génération `2026-09-23T06:49:47.113-07:00`.
- **Arrêt** : arrêt ciblé certifié par le contrôleur, puis `TERMINATED` relu
  en lecture seule (dernier arrêt `2026-09-23T06:53:11.984-07:00`).
- **Machine** : RTX PRO 6000 Blackwell Server Edition (pilote 580.173.02,
  97 887 Mio, sm 12.0), nvcc 12.9.41, g++ 11.4.0, CMake 3.22.1.

**Paquet** : construit depuis `6e0e43a0`, protocole au commit. Snapshot
`1702f86c…`, manifeste `817670dd…`, contrôleur `133cca17…`, worker
`0df81b65…`, binaire `c11182b3…`. Tentative précédente :
[échec de configuration](../g4_gpu_s1_attempt1_20260923/README.md).

**Reçus** : hôte et worker **`completed`**, les six cas sont `complete`.
- **Préflight natif** : 1 500 sites, 431 037 paires, masques égaux.
- **Mutant causal** `--inject=pair_mask` : détecté, exactement une paire
  différente.

## Ce qui est mesuré

La sonde `mhgp9_gpu_filter_probe` (schéma v2) reproduit la population q3/q4
de la chaîne : front WSPD (s = 8, voies 6), rectangles filtrés, rectangles
survivants développés en paires dans l'ordre du moteur.
- **Références CPU**, à 48 fils : rectangles, paires sans cache, paires avec
  le cache de ligne du moteur. Le cache donne partout les mêmes masques.
- **Passe GPU** : un fil par rectangle, un balayage CUB, puis un fil par
  paire sans cache.
- **Exactitude** : chaque masque GPU égale le masque CPU, et les totaux de
  nœuds visités sont égaux (même DFS, même ordre). C'est le cas dans les six
  cas.

`GPU total` est la **meilleure passe chaude** sur trois : transferts
hôte→device, noyaux, balayage, puis retour de **tous** les masques pour la
comparaison. `première` est la passe froide. Ni l'un ni l'autre n'est un
temps de chaîne ou de tour.

| trame | K | rectangles | paires | CPU rect. (ms) | CPU paires, cache (ms) | CPU paires, sans cache (ms) | GPU total (ms) | première (ms) | rapport |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 000000 | 5 | 3 133 819 | 23 686 751 | 343 | 836 | 2 281 | **63,8** | 64,1 | ×18,5 |
| 000000 | 10 | 4 782 714 | 30 777 213 | 609 | 1 613 | 3 995 | 103,0 | 105,1 | ×21,6 |
| 000100 | 5 | 2 348 056 | 11 960 420 | 225 | 585 | 1 026 | 43,4 | 44,2 | ×18,7 |
| 000100 | 10 | 3 549 478 | 17 488 839 | 428 | 1 110 | 1 632 | 70,3 | 70,5 | ×21,9 |
| 000200 | 5 | 2 964 033 | 22 722 345 | 367 | 790 | 1 888 | 64,2 | 65,5 | ×18,0 |
| 000200 | 10 | 4 695 935 | 32 789 701 | 587 | 1 953 | 3 979 | 106,8 | 109,0 | ×23,8 |

« rapport » : (CPU rectangles + CPU paires avec cache, 48 fils) / GPU total.

Détail GPU sur 000000/K5 :

| poste | durée | volume | débit |
| --- | ---: | --- | ---: |
| transfert vers le GPU | 1,41 ms | — | — |
| rectangles | 32,5 ms | 230 M visites | ≈ 7,1 G visites/s |
| balayage | 0,04 ms | — | — |
| paires | 29,0 ms | 1,11 G visites | ≈ 38 G visites/s |
| retour des masques | 0,81 ms | — | — |

## Lecture

- **Seuil S1 franchi.** Toute la population de filtrage de 08/000000/K5,
  sans cache, tient en 63,8 ms, sous les 100 ms fixés d'avance.
- **Débit.** Le GPU fait le filtrage exact 18 à 24 fois plus vite que le CPU
  à 48 fils, et cela sans cache : à 000000/K5, il lance une recherche pour
  chacune des 23,7 M paires, contre 7,16 M recherches pour la variante CPU
  avec cache. Ce rapport dépasse le ×11 que C exigeait avant d'engager un
  port plus large.
- **Premier levier du GPU.** Les rectangles (bornes générales de boîtes,
  arithmétique d'intervalles) visitent 5 fois moins de nœuds que les paires
  mais coûtent autant : c'est le premier levier.
- **Ce que le reçu ne dit pas.** Le front est ici calculé sur CPU en
  séquentiel (2,1 s), alors qu'il est parallèle dans la chaîne. Le filtrage
  ne pèse qu'environ 40 % du CPU q3/q4. Le gain sur la chaîne n'est pas
  mesuré : il faut raccorder le passage GPU à la chaîne. Le front reste sur
  CPU, le GPU filtre, et les paires survivantes (2,04 M à 000000/K5) passent
  au cœur, au certificat et à la génération sur CPU. Il faut aussi porter
  l'étape suivante, le cœur et le certificat de voie morte (18 %).
- **Contrat** : 1 s puis 100 ms non atteint, aucune qualification.

## Contenu

- `vm/` : sortie du worker (inventaire, versions, configuration, build,
  préflight et mutant, six cas, reçu).
- `host/` : journaux expurgés du contrôleur, sans `oslogin_add`, ni clé, ni
  archive.
- `PACKAGE.json`, `SUMMARY.json` (schéma `mhgp9_g4_gpu_s1_summary_v1`),
  `SHA256SUMS`.
