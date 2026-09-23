# Reçu : session G4 R12, filtre témoin q3/q4 par lots sur GPU dans la chaîne (S2)

23 septembre 2026, 16:13–16:18 UTC (VM). Cadre : `exploration_v9_hors_registre`,
`backend=cuda_g4`, `profile=quantized_u18_input_only`,
`public_status=not_claimed`.

**GCP utilisé.**
- **Session** : SPOT gardée sur la cible fixe `devpod-gpu-exploration /
  us-central1-b / ehgp-v7-4fa0e0789a7d5bb06b787d35` (g4-standard-48),
  génération `2026-09-23T09:13:40.446-07:00`.
- **Arrêt** : arrêt ciblé certifié par le contrôleur, puis `TERMINATED` relu
  en lecture seule (dernier arrêt `2026-09-23T09:18:09.962-07:00`).
- **Machine** : RTX PRO 6000 Blackwell Server Edition (pilote 580.173.02,
  97 887 Mio, sm 12.0), nvcc 12.9.41.

**Paquet** : construit depuis `2059189d`, protocole au commit. Sonde v17,
plan v6 à 14 cas ; snapshot `bd818a67…`, manifeste `bb33b1fd…`,
contrôleur `971bf4bb…`, worker `6dc53893…`, binaire `d7d99aa0…`.

**Reçus** : hôte et worker **`completed`**, backend `cuda_g4`,
`GPU_attempted` et `GPU_executed` vrais.
- **Préflight** (1 500 sites, tous leviers, GPU) : condensé `73490cf8`,
  identique à son **jumeau moteur**.
- **Cas** : les 14 sont `complete_relative`, Euler « holds ».

## Plan et juge

Chaque (trame, K) tourne deux fois à W48 (tour statique 48, s = 8).
- **Cas GPU** : tous les leviers, dont `q34_batch_filter` et `q34_gpu_filter`.
- **Jumeau moteur** : mêmes leviers, sans lots ni GPU.
- **Paire répétée** : 08/000000/K5, pour la dispersion.

Les **huit comparaisons d'objet** GPU / moteur (condensé FULL, ordres,
catalogue, Euler) sont **toutes égales**. Les condensés sont aussi ceux
de R11 pour les mêmes trames et K.

## Résultats (chaîne de bout en bout, s)

| trame | K | moteur | GPU | gain | q3/q4 moteur → GPU | tour |
| --- | ---: | ---: | ---: | ---: | --- | ---: |
| 000100 | 5 | 2,51 | **2,01** | −20 % | 1,67 → 1,17 | 0,59 |
| 000000 | 5 | 3,23 / 3,29 | **2,47 / 2,57** | −23 % | 2,18 → 1,44 | 0,74–0,82 |
| 000200 | 5 | 3,59 | **2,69** | −25 % | 2,43 → 1,57 | 0,78 |
| 000100 | 10 | 7,78 | **6,63** | −15 % | 4,47 → 3,48 | 2,28–2,38 |
| 000000 | 10 | 10,44 | **8,32** | −20 % | 6,28 → 4,49 | 2,97–3,03 |
| 000200 | 10 | 10,69 | **8,70** | −19 % | 6,63 → 4,87 | 2,85 |

**Phases q3/q4 du chemin GPU** (ms) :

| trame | K | front | appel du filtre | dont passe GPU | survivants (cœur à q4) | survivants (nombre) |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 000100 | 5 | 75 | 225 | 47 | 834 | 1 732 176 |
| 000000 | 5 | 98 / 99 | 205 / 252 | 70 / 70 | 1 085 / 1 081 | 2 043 612 |
| 000200 | 5 | 95 | 248 | 70 | 1 181 | 2 237 912 |
| 000100 | 10 | 121 | 258 | 72 | 3 050 | 3 673 260 |
| 000000 | 10 | 163 | 301 | 113 | 3 960 | 4 507 278 |
| 000200 | 10 | 161 | 315 | 119 | 4 321 | 4 927 304 |

Le CPU cumulé des ouvriers q3/q4 (front et survivants, hors appel du
filtre fait par le fil principal) est divisé par 1,4 à 2 : à 000000/K5,
102 CPU·s sur le chemin moteur contre 52 sur le chemin GPU.

## Lecture

- **Meilleures chaînes** : K5 **2,01 / 2,47 / 2,69 s**, contre 2,54 / 3,21 /
  3,52 à R11 ; K10 **6,63 / 8,32 / 8,70 s**, contre 7,68 / 10,36 / 10,53.
  Toujours CPU pour le reste de la chaîne, trames sans sol de la séquence
  08, aucun contrat.
- **Appel du filtre** : 205 à 315 ms, dont seulement 47 à 119 ms de passe
  GPU mesurée par événements. Le reste est payé côté hôte : copie plate de
  l'index, tableaux, allocations, conversion des survivants, et création du
  contexte CUDA au premier appel du processus, hors événements. Premier
  levier, sans calcul : **ouvrir le contexte et préparer l'index pendant
  q2**, en parallèle.
- **Nouveau poste dominant de q3/q4** : les survivants (cœur, certificat,
  couverture, génération q3/q4) prennent 0,8 à 1,2 s à K5 et 3 à 4,3 s à K10.
  Le front est à 75–163 ms.
- **Prochain verrou à K5** : la tour (0,59–0,82 s), le même ordre de grandeur
  que les survivants.
- **Frontière de confiance** (A/B/C) : le contrôle structurel des survivants
  est actif, le préflight a son jumeau moteur, et chaque cas GPU son jumeau
  moteur dans le plan. Les juges indépendants de C sur le catalogue du chemin
  par lots restent la troisième couche.

## Contenu

- `vm/` : sortie du worker (inventaire, versions, configuration CUDA, build,
  préflight et jumeau moteur, 14 cas, reçu).
- `host/` : journaux expurgés du contrôleur, sans `oslogin_add`, ni clé, ni
  archive.
- `PACKAGE.json`, `SUMMARY.json` (schéma `mhgp9_g4_tower_summary_v3`),
  `SHA256SUMS`.
