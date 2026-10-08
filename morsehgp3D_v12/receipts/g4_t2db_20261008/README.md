# Session G4 T2-d-B : coût interne de l'étage G — lot adopté, garde seule et proposition rejetées

8 octobre 2026. Session gardée `v12.20261008.t2db` (`gcp-migration/v12_session.py`, commit `41d4d828b`, preuve
`pushed_commit`), cible `us-central1-c` / `ehgp-v7-3b1d496aed430749ea7e049f`, `--max-run-seconds 4200`. VM de 08:09:34
à 08:42:51 UTC, **arrêt certifié `TERMINATED`** par le lanceur, puis relu indépendamment à 08:43:41 UTC. Reçu sans
identité de compte : [`receipt.json`](receipt.json) ; sorties sous `resultats/` ; empreintes : `SHA256SUMS`.

```text
phase=exploration_v12_hors_registre
backend=cpu_reference (étage G, décisif) ; cuda_g4 (catalogue, informations FULL)
objet=full_pi0 (étage G de la tour, puis tour FULL K1..5 pour les informations)
quantification=quantized_u21_input_only
public_status=not_claimed
```

**Bras.** Le pilote [`pilote_t2d_b.py`](../../microbancs/mes_t2d_b/pilote_t2d_b.py) reconstruit tous ses bras depuis
l'archive épinglée de `902041f66` (SHA-256 `0f91cda2…`), par les substitutions exactes de
[`bras_t2d_b.json`](../../microbancs/mes_t2d_b/bras_t2d_b.json) : avant, avant_bis (A/A), garde (L1), report (L2),
témoins (L3), census (L1 + L2 + L3), proposition (L4), après (le lot). Les fichiers du lot sont identiques entre
`902041f66` et `main`, mais pas le Pool ni la Session recouverte, qui sont postérieurs. La règle `REGLE_T2D_B` a été
écrite à 04:48 UTC et amendée à 06:04 UTC, avant toute mesure G4. Campagne : ng00–02 à K5, 48 fils, 10 tours ×
10 passes, ordre tournant sans inversion. Mesure décisive : le mur de `resolve_tower`.

| Commande | État | Durée |
| --- | --- | ---: |
| `socle_ctest` (construction par défaut, `ctest -LE long`) | ok | 141 s |
| `t2d_b_pilote` | ok, aucun refus, A/A dans sa fenêtre | 582 s |
| `lidar_ctest` | ok, 7 portes LiDAR, dont `MES-M0` (`mhgp12_tower_chain_m0`, 715 s), qui avait expiré à 180 s dans la session T2-d-A | 764 s |
| `mutants_index_num_tour` | ok, campagnes index, num et tour | 270 s |

## Verdicts de `REGLE_T2D_B`

Rapport bras / avant du mur de G (moyenne géométrique sur 10 tours, IC 95 % par bootstrap). Un levier est adopté si
la borne haute est sous 1 sur chacune des trois trames.

| Levier | ng00 | ng01 | ng02 | Verdict |
| --- | --- | --- | --- | --- |
| lot (L1 à L4) | 0,916 (0,914–0,917) | 0,926 (0,924–0,929) | 0,932 (0,931–0,934) | **adopté** |
| census combiné (L1 + L2 + L3) | 0,924 (0,922–0,926) | 0,934 (0,931–0,936) | 0,939 (0,936–0,941) | **adopté** |
| témoins seuls (L3) | 0,935 (0,931–0,939) | 0,945 (0,941–0,948) | 0,951 (0,949–0,954) | **adopté** |
| report seul (L2) | 0,995 (0,993–0,997) | 0,996 (0,993–0,998) | 0,994 (0,992–0,997) | **adopté** |
| garde seule (L1) | 0,994 (0,992–0,998) | 0,998 (0,995–1,000) | 0,996 (0,993–0,998) | **rejeté** |
| proposition (L4) | 0,999 (0,995–1,003) | 0,998 (0,996–1,000) | 0,996 (0,994–0,999) | **rejeté** |
| A/A (avant → avant_bis) | 1,001 (0,998–1,005) | 1,000 (0,998–1,003) | 0,999 (0,996–1,001) | dans la fenêtre |

Rapports publiés sans verdict : témoins après la garde 0,929 / 0,936 / 0,943 ; proposition après le census 0,991 /
0,992 / 0,993. Empreintes de l'objet identiques dans tous les processus de tous les bras ; sur ng00, égales à la ligne
gravée de la porte de déterminisme. Compteurs de travail identiques entre les bras.

Mur de G, médiane des processus (ms) : ng00 53,1 → 48,6 ; ng01 42,0 → 38,9 ; ng02 48,2 → 45,0 (avant → lot).

## Décision : le produit devient le bras « census », sans L4

La règle rejette L4, la proposition entière exacte : son effet seul n'est pas établi. Elle est **retirée du produit**
dans le commit qui publie ce reçu : `proposal.hpp` et `resolve.cpp` reviennent à DWelzl seul, et la porte
`mhgp12_tower_proposal` et les sept mutants de L4 disparaissent (plancher des mutants de la tour : 45 → 38). Le produit
est alors le bras « census » (L1 + L2 + L3), mesuré et adopté. C'est le chemin mesuré.

L1, la garde resserrée de l'auditeur, est rejetée seule mais fait partie de cette combinaison adoptée. Elle reste.
L'information « proposition après le census » (environ −0,8 %) n'est pas un verdict : L4 pourra être reproposé avec
une règle qui le juge contre le bras « census ».

## Informations (non jugées)

- **Mur FULL K5 sur l'appareil**, voie séquentielle de `902041f66`, deux processus par bras : 161,9 → 157,2 ms
  (ng00), 129,8 → 126,9 (ng01), 165,8 → 164,3 (ng02), FUL1 identique. Ce n'est pas le produit : ni la Session
  recouverte, ni le catalogue T2-d-C, ni le Pool à équipe.
- **K10, 48 fils**, étage G : 441 → 414 ms (ng00), 322 → 305 (ng01), 354 → 340 (ng02).
- **Uniformes à K5, 48 fils**, étage G : 24,6 → 23,5 ms (8 000 sites), 49,7 → 48,4 (16 000), 110,5 → 105,7 (32 000).
  Les empreintes sont celles des portes d'échelle.
- **Profil à un fil sur ng00 K5** (temps cumulés, ms, avant → après) : census saturé 917 → 640, census complet
  432 → 332, proposition 494 → 439. Les autres postes bougent de moins de 3 % : t1 886 → 883, trace 796 → 819,
  sonde 612 → 595, arrêt 300 → 296, certificat 181 → 184. G à un fil : 1 652 → 1 509 ms.

## Lecture

Le census à témoins (L3) porte presque tout le gain. Sur 48 fils, G baisse de 6 à 8 %, bien moins que les −22 %
d'instructions comptées à un fil. Le comptage d'instructions ne prédit pas le temps ; c'était déjà dit pour le SASS.
Les postes suivants de G sont maintenant t1, la trace, la sonde et le census saturé.

## Ce que cette session n'établit pas

Ni le gain sur le produit (Session recouverte, catalogue T2-d-C, Pool à équipe), ni le contrat FULL : tous deux
seront mesurés par la session `MES-FULL` suivante, sur le produit sans L4. GCP utilisé pour cette seule session,
arrêt certifié.
