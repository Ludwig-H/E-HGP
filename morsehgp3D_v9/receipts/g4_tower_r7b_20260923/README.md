# Reçu : session G4 R7b, ablation appariée du MEB proposé

23 septembre 2026, 06:08–06:16 UTC. Cadre : `exploration_v9_hors_registre`,
`backend=reference_cpu`, `profile=quantized_u18_input_only`,
`public_status=not_claimed`. **GCP utilisé** : session SPOT gardée sur la
cible fixe `devpod-gpu-exploration / us-central1-b /
ehgp-v7-4fa0e0789a7d5bb06b787d35` (g4-standard-48, 24 cœurs × 2 fils),
génération `2026-09-22T23:08:58.035-07:00`, arrêt ciblé certifié et
`TERMINATED` relu (dernier arrêt `2026-09-22T23:16:11.824-07:00`). GPU non
utilisé. Reprise de la tentative R7 refusée par rupture de stock
([reçu](../g4_tower_r7_stockout_20260923/README.md)), avec un paquet neuf.

Paquet `8e8b83a3` (sonde v11, plan v6, snapshot `69aa6e57…`, manifeste
`9b599060…`, protocole au commit). Reçu hôte et worker **`completed`** :
24 cas `complete_relative`, préflight natif non vacant (tous leviers ON,
condensé `73490cf88c02af30`), dix-huit comparaisons d'objet égales. Seul
levier variant : `tower_meb_proposal` ; 48 fils, tour statique 48, s = 8,
deux répétitions entrelacées ON/OFF. Ce paquet contient aussi, par rapport à
R6 : tri d'échantillonnage des présentations, condensé chronométré hors
`chain_total` (`times_ms.digest`), catalogue trié certifié par balayage,
index exact des clés et parties parallèles de la phase statique. Il ne
contient **pas** le Welzl à déplacement en tête (`8fa03046`, postérieur).

## Résultats (deux répétitions ; MEB OFF → ON)

`chain_total` exclut le condensé ; `digest` est publié à part.

| trame | K | chaîne (s) | tour (s) | condensé (s) | CPU·s | propositions (toutes vérifiées) | condensé tour |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 000100 | 5 | 3,69 / 3,68 → 3,67 / 3,81 | 0,71 → 0,73 | 0,16 | 88 → 87 | 502 662 | `dbf799c8ed83f53f` |
| 000000 | 5 | 5,51 / 5,31 → 5,70 / 5,56 | 0,89 → 0,91 | 0,19 | 113 → 116 | 654 852 | `67450c64611075b1` |
| 000200 | 5 | 6,44 / 6,40 → 6,57 / 6,44 | 0,95 → 0,97 | 0,21 | 125 → 125 | 561 045 | `8240af3d4dce3d45` |
| 000100 | 10 | 10,05 / 9,97 → 9,58 / 9,58 | 3,46 → 3,18 | 0,80 | 278 → 268 | 6 098 213 | `9ddbf7430c9086cc` |
| 000000 | 10 | 14,29 / 14,27 → 13,93 / 13,91 | 4,39 → 4,02 | 1,00 | 364 → 351 | 8 206 753 | `ac108f7f71096c3f` |
| 000200 | 10 | 15,63 / 15,65 → 15,30 / 15,28 | 4,05 → 3,85 | 1,00 | 381 → 372 | 6 544 650 | `ba973af0c8da95bd` |

Aucun repli : toutes les propositions ont été vérifiées exactement.

## Lecture

- Objets identiques ON/OFF dans chaque paire (catalogue, ordres, travail
  hors MEB, condensé).
- MEB proposé : tour **−5 à −8,4 %** à K10, chaîne −2 à −4 % ; à K5 la tour
  est neutre (dans le bruit) et la moyenne de chaîne varie de +1 à +4 %
  (écarts de même ordre que ceux entre répétitions).
- Contre R6 (même cible, autres paquets ; ajouter `digest` à `chain_total`
  pour le périmètre ancien) : 000000/K10 16,67 → 13,91 + 1,00 s, tour
  5,26 → 4,02 s ; fusion 0,17 → 0,04 s à K5, 0,78 → 0,11 s à K10.
  Comparaison entre sessions, pas une ablation causale de chaque changement.
- Meilleurs totaux hors condensé : **3,67 s à K5** et **9,58 s à K10**.
  Contrat (1 s puis 100 ms) non atteint : aucune qualification.

## Contenu

`vm/` (sorties du worker, préflight, sondes, GNU time, résumés, preuves de
garde, reçu `completed`), `host/` (contrôleur, reçu hôte `completed` avec
`verified_guard`, journaux de démarrage et d'arrêt expurgés de l'adresse du
compte ; `oslogin_add`, clé et archives non versionnées), `PACKAGE.json`,
`SUMMARY.json`, `SHA256SUMS`.
