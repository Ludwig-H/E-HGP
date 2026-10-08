# Session G4 v12.20261008.fullk : première mesure du contrat FULL, et coût des profils (`MES-D6`)

8 octobre 2026. Session gardée (`gcp-migration/v12_session.py`, instantané du worktree à `c9ac60f20`), cible
`us-central1-c` / `ehgp-v7-3b1d496aed430749ea7e049f`, `--max-run-seconds 4200`. VM démarrée à 02:31:46 UTC, worker de
02:33:32 à 03:01:30 (code 1 : `MES-D6` en écart, voir plus bas), **arrêt certifié `TERMINATED`** par le lanceur
(clôture `stopped`) et relu indépendamment à 03:03:51 UTC (`lastStopTimestamp` 03:03:03 UTC). Reçu sans identité de
compte : [`receipt.json`](receipt.json) ; sorties sous `resultats/` ; empreintes : `SHA256SUMS`.

```text
phase=exploration_v12_hors_registre
backend=cuda_g4 (catalogue, voie hybride) ; cpu_reference (G, T, M, V, R ; bras CPU identifié)
objet=full_pi0 (tour FULL K1..5, verticales et registre compris)
quantification=quantized_u21_input_only (MES-D6 : profils 21, 24, 32)
public_status=not_claimed
```

| Épingle | Valeur |
| --- | --- |
| paquet | `aa75b3162d783d68…` |
| plan | `24cdb3e52c9b70ce…` |
| données (trames ng00–02 ; archive des 37 trames `v12set`, six séquences) | manifeste `c04343f5242db2e4…` |

## `MES-FULL` : verdict du contrat **non tenu** (règle écrite d'avance, aucun refus)

Sonde [`bench/full_probe.cpp`](../../bench/full_probe.cpp), pilote [`pilote_full.py`](../../microbancs/mes_full/pilote_full.py).
Le mur va de l'entrée quantifiée en mémoire à la tour complète en mémoire (P, C sur l'appareil avec transferts et fin
d'étage, G, raccord, T, M, V, R) ; Session ouverte une fois ; validation, empreinte FUL1 et libération hors du mur.
Empreintes FUL1 : une par trame et par K, identique sur toutes les passes, tous les processus et les deux voies
(appareil = CPU) ; aucune prise refusée.

| K5, voie appareil, ms | ng00 | ng01 | ng02 |
| --- | ---: | ---: | ---: |
| médiane chaude (5 processus × 9 passes) | **159,3** | **127,6** | **163,3** |
| maximum des médianes par processus | 160,8 | 127,8 | 164,6 |
| première passe (médiane) | 188,6 | 154,4 | 194,6 |
| P / C (dont transferts) / G | 1,7 / 34,9 (9,0) / 57,2 | 1,5 / 30,8 (7,9) / 43,8 | 1,9 / 37,4 (10,4) / 51,7 |
| T / M / V / R | 33,1 / 11,7 / 2,2 / 14,3 | 25,8 / 8,4 / 1,9 / 12,0 | 38,0 / 12,0 / 2,4 / 15,3 |

**Session sur les 37 trames `v12set`** (séquences 00, 02, 05, 06, 08, 10 ; 33 179 à 99 099 sites, médiane 64 740 ;
5 processus, second tour) : **médiane 241,3 ms, maximum 467,9 ms**. Le temps croît un peu plus vite que linéairement
avec les sites : 104 ms à 33 179 sites, 140 à 165 ms vers 50 000, 230 à 290 ms vers 65 000 à 73 000, 334 à 365 ms vers
80 000, 461 à 466 ms vers 96 000 à 99 000. Sur une trame typique (64 740 sites, 248,5 ms) : P 2,7, C 54,1 (transferts
14,6), G 79,0, T 63,2, M 16,3, V 3,4, R 22,9 ms. Tableaux complets :
[`tableaux_full.md`](resultats/cmd/000_mes_full/files/full/tableaux_full.md).

À K10 (3 × 5, informatif) : 793 / 603 / 715 ms sur ng00–02 (G 318 à 433 ms). Bras CPU à K5 : 441 / 368 / 447 ms
(catalogue CPU 274 à 329 ms).

**Lecture.** Sur les trois trames de référence, la tour coûte 1,3 à 1,6 fois le budget ; sur les trames réelles de six
séquences, dont la taille médiane (65 000 sites) dépasse celle de ng00–02, elle coûte 2,4 fois en médiane et 4,7 fois
au maximum. Aucun étage ne tient seul la part qu'il faudrait : G (79 ms vers 65 000 sites) et T (63 ms) d'abord, puis C
(54 ms, dont 15 de transferts), R et M. Les budgets d'étage d'`ARCHITECTURE.md` § 3 avaient été posés sur ng00
(40 000 sites) ; ils sont à reprendre à la taille médiane réelle.

## `MES-D6` : coût des profils — écart du lecteur, données publiées

Le pilote joué était celui de l'instantané `c9ac60f20`, dont le lecteur strict de la ligne de l'étage G ignorait les
neuf champs ajoutés par T2-c : toutes les lignes G ont été refusées (66 écarts, code 1) ; le lecteur adapté est livré
en `e37fd8935`, après le lancement. Les lignes du catalogue ont été admises. Catalogue, voie CPU, 48 fils, médiane des
passes 2 à 5, rapport au couple (21, ×1) du même tour (3 tours) :

| Trame | 24 / ×1 | 32 / ×1 | 21 / ×8 | 24 / ×8 | 32 / ×8 | 32 / ×2048 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| ng00 | 1,004 | 1,010 | 1,035 | 1,032 | 1,042 | **2,946** |
| ng01 | 1,000 | 1,006 | 1,060 | 1,061 | 1,067 | **2,904** |
| ng02 | 1,002 | 1,009 | 1,024 | 1,021 | 1,030 | **2,906** |

Étage G, **extrait hors ligne des journaux bruts publiés** (`brut/*_tour.jsonl`, médiane des passes 2 à 5 ; informatif,
pas une prise admise) : u24 à ×1 à 1,00 à 1,07 de u21, u32 à ×1 à 1,04 à 1,11, u32 à ×2048 à 1,09 à 1,13 ; empreintes de
la résolution à ×1 identiques aux trois profils sur chaque trame.

**Lecture.** Un profil élargi ne coûte presque rien sur les mêmes coordonnées (u24 : +0 à 1 % au catalogue ; u32 : +1 %
au catalogue, +5 % environ à G) ; une grille 8 fois plus fine coûte 2 à 7 %. En revanche, des coordonnées qui remplissent
32 bits (grille de 1/2048 mm) triplent le coût du catalogue CPU : les voies étroites de l'arithmétique ne servent plus.
La voie appareil du catalogue n'a été jouée qu'au profil 21.

## Ce que cette session n'établit pas

Ni le respect du contrat (il est mesuré non tenu), ni la voie appareil aux profils 24 et 32, ni le régime des petits
nuages et des scènes de plusieurs millions de sites pour la v12. GCP utilisé pour cette seule session, arrêt certifié.
