# Reçu : session G4 R4b, ablation du cache des témoins et tour parallèle

23 septembre 2026, 02:47–02:55 UTC. Cadre : `exploration_v9_hors_registre`,
`backend=reference_cpu`, `profile=quantized_u18_input_only`,
`public_status=not_claimed`. **GCP utilisé** : session SPOT gardée sur la
cible fixe `devpod-gpu-exploration / us-central1-b /
ehgp-v7-4fa0e0789a7d5bb06b787d35` (g4-standard-48), génération
`2026-09-22T19:47:39.618-07:00`, arrêt ciblé certifié et `TERMINATED` relu
(dernier arrêt `2026-09-22T19:54:14.677-07:00`). GPU non utilisé. Reprise de
la session R4 préemptée ([reçu](../g4_tower_r4_preempted_20260923/README.md)),
même paquet.

Paquet `a1d7a9bc` (protocole v6 à réception durcie, snapshot `45c2b02d…`,
manifeste `8aebf8ec…`). Reçu hôte et worker **`completed`** : treize cas
`complete_relative`, préflight natif non vacant accepté (condensé
`73490cf88c02af30`), sept comparaisons d'objet toutes égales. Code : préparation
parallèle de la voie statique de la tour (`e0ae05a7`), preuve q3/q4 conjointe et
cache des nœuds témoins (`7f64a279`). Seul levier variant entre cas appariés :
`q34_witness_cache` ; 48 fils, tour statique 48, s = 8.

## Résultats

| trame | K | q3/q4 sans → avec cache (s) | total sans → avec (s) | CPU·s sans → avec | tour (s) | condensé |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| 000000 | 5 | 4,22 → 3,96 | 7,63 → 7,39 | 170 → 138 | 2,47 | `67450c64611075b1` |
| 000100 | 5 | 2,89 → 2,76 | 5,56 → 5,44 | 116 → 103 | 2,01 | `dbf799c8ed83f53f` |
| 000200 | 5 | 5,61 → 5,48 | 9,36 → 9,18 | 183 → 163 | 2,65 | `8240af3d4dce3d45` |
| 000000 | 10 | 9,16 → 8,71 | 27,24 → 26,69 | 488 → 447 | 14,89 | `ac108f7f71096c3f` |
| 000100 | 10 | 5,91 → 5,72 | 19,64 → 19,40 | 340 → 322 | 11,42 | `9ddbf7430c9086cc` |
| 000200 | 10 | 12,72 → 11,76 | 30,39 → 29,50 | 530 → 486 | 14,58 | `ba973af0c8da95bd` |

Répétition 000000 K10 avec cache : 26,71 s. Le cache rejette sans recherche
61 à 70 % des paires résiduelles développées après le filtre de rectangles
(`witness_cache_rejected_pairs / expanded_pairs`).

## Lecture

- Cache : CPU total −5 à −19 %, mur q3/q4 −2 à −8 % ; objet inchangé.
- Tour, contre R3 (même type de VM, autre paquet) : K10 22,8 → 14,9 s,
  17,5 → 11,4 s, 22,1 → 14,6 s ; K5 3,8 → 2,5 s, 3,1 → 2,0 s, 4,1 → 2,7 s,
  condensés égaux. Comparaison entre sessions, pas une ablation appariée.
- Chaîne complète : **5,4 / 7,4 / 9,2 s à K5**, **19,4 / 26,7 / 29,5 s à K10**
  (000100 / 000000 / 000200). À K10 la tour (lots séquentiels) domine encore.
- Une répétition par cas apparié ; aucune qualification de contrat.

## Contenu

`vm/` (sorties du worker, préflight, sondes, GNU time, résumés, preuves de
garde, reçu `completed`), `host/` (contrôleur, reçu hôte `completed`, journaux
expurgés ; `oslogin_add`, clé et archive non versionnées), `SUMMARY.json`,
`SHA256SUMS`.
