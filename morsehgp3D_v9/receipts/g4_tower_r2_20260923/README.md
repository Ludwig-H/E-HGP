# Reçu : deuxième session G4 de la tour FULL v9 (refusée par le validateur)

23 septembre 2026, 00:31–00:42 UTC. Cadre : `exploration_v9_hors_registre`,
`backend=reference_cpu`, `profile=quantized_u18_input_only`,
`public_status=not_claimed`. **GCP utilisé** : une session SPOT gardée sur la
cible fixe `devpod-gpu-exploration / us-central1-b /
ehgp-v7-4fa0e0789a7d5bb06b787d35` (g4-standard-48, AMD EPYC 9B45, 24 cœurs,
48 fils), génération `2026-09-22T17:31:19.248-07:00`, arrêt ciblé certifié
(`host/guarded_stop.redacted.stdout`, `targeted_shutdown_certified: true`) et
état `TERMINATED` relu après la session (dernier arrêt
`2026-09-22T17:41:46.928-07:00`). GPU non utilisé.

Paquet construit depuis le commit **`0b29b6c3`** (`PACKAGE.json`, snapshot
`ced2182d…`, manifeste `f9d00e36…`), compilé sur la VM par GCC 11.4 en Release
strict. Plan : treize cas sur les trois trames sans sol à 1 mm de la séquence
08, s = 8 ; K5 et K10 à 48 fils et tour statique à 48, deux répétitions, plus
000000 à K10 sur 24 fils (statique 24). Atlas saturant actif (défaut de la
chaîne à ce commit) ; census q3 sur feuille absent de ce commit.

## Statut : `probe_failed`, donc aucune qualification

Les treize sondes ont rendu le code 0 et `complete_relative`, mais le
validateur du worker empaqueté a refusé chaque sortie
(`ValueError: probe counters tower_work`) : il exigeait un compteur entier pour
tout champ de `tower_work`, alors que la sonde publie aussi
`meb_accounting` (chaîne) et `meb_supports_by_size` (tableau). Le selftest du
protocole jugeait une fausse sonde qui omettait ces deux champs. Le
contre-audit B
[`CONTRE_AUDIT_B_G4_R2_PREFLIGHT_20260923.md`](../../audits/CONTRE_AUDIT_B_G4_R2_PREFLIGHT_20260923.md)
avait prédit ce refus pendant la session. Le reçu hôte est donc
`worker_failed` et ce reçu ne qualifie rien.

Correction, dans le commit qui suit ce reçu : schéma de sonde v4 ; champs MEB
typés (libellé de comptabilité épinglé, histogramme d'au plus huit compteurs) ;
voies géométriques épinglées par cas et passées explicitement à la sonde ;
somme des sous-chronos bornée par le total de chaîne, lui-même borné par le
mur externe ; arrêt de la campagne au premier défaut de protocole ; porte
CTest `mhgp9_probe_worker_contract_{normal,optimized}` qui fait juger la
**vraie** sonde par le validateur du worker, avec onze mutants de schéma.

## Chronos bruts (exploratoires)

Lus dans les sorties brutes hachées (`vm/probe_*.stdout`, `SUMMARY.json`) :

| cas | trame | K | fils | total (s) | q3/q4 (s) | tour (s) | CPU·s | RSS (Go) | condensé |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 0, 6 | 000000 | 5 | 48 | 13,23 / 13,27 | 8,49 / 8,50 | 3,78 / 3,83 | 414 | 1,19 | `67450c64611075b1` |
| 1, 7 | 000100 | 5 | 48 | 10,45 / 10,46 | 6,71 / 6,72 | 3,08 / 3,10 | 327 | 0,99 | `dbf799c8ed83f53f` |
| 2, 8 | 000200 | 5 | 48 | 19,73 / 20,16 | 14,59 / 15,03 | 4,08 / 4,06 | 659 | 1,23 | `8240af3d4dce3d45` |
| 3, 9 | 000000 | 10 | 48 | 50,52 / 49,63 | 24,26 / 23,69 | 23,11 / 22,86 | 1 258 | 4,91 | `ac108f7f71096c3f` |
| 4, 10 | 000100 | 10 | 48 | 37,50 / 37,14 | 17,65 / 17,51 | 17,58 / 17,38 | 913 | 3,99 | `9ddbf7430c9086cc` |
| 5, 11 | 000200 | 10 | 48 | 64,16 / 64,42 | 38,78 / 38,93 | 22,26 / 22,37 | 1 858 | 4,91 | `ba973af0c8da95bd` |
| 12 | 000000 | 10 | 24 | 56,47 | 29,62 | 23,45 | 793 | 4,64 | `ac108f7f71096c3f` |

## Lecture

- Les six condensés (trame, K) sont **identiques** à ceux de la
  [session R1](../g4_tower_r1_20260922/README.md) (`e28296bb`), entre
  répétitions et entre 24 et 48 fils : l'atlas saturant, le noyau MEB « première
  paire maximale », le tri filtré des niveaux et la tour statique ne changent
  pas l'objet sur ces trames. Contrôle différentiel, pas un oracle.
- Contre R1 : K5 ×1,4 à ×1,5 (13,2 contre 18,8 s sur 000000), K10 ×2,0 à
  ×2,2 (50,5 contre 111,7 s), surtout par la tour statique.
- La tour K10 ne gagne rien de 24 à 48 fils (23,45 contre 23,11 s) : la voie
  statique plafonne ; q3/q4 gagne ×1,22 pour ×2 fils logiques (24 cœurs
  physiques, SMT).
- Écart au contrat d'une seconde : ×10 à ×20 à K5, ×37 à ×64 à K10.
  Répétitions à 3 % près.

## Contenu

- `vm/` : sorties du worker (sondes JSON, `/usr/bin/time -v`, résumés par cas
  avec le motif du refus, compilateur, lscpu, preuves de garde, manifestes de
  sources, reçu du worker `probe_failed`).
- `host/` : enregistrements des commandes du contrôleur, reçu hôte
  `worker_failed`, marques de garde, journaux de démarrage et d'arrêt
  **expurgés** (adresse du compte masquée). Les sorties de `oslogin_add`, la
  clé SSH éphémère et l'archive du paquet ne sont pas versionnées (paquet
  reconstructible depuis `0b29b6c3`).
- `SUMMARY.json` (dérivé des sorties brutes), `SHA256SUMS`.
