# Mesures G4 appariées du 4 octobre 2026 — q3 différé, lemme R, compteurs R1, enveloppes M3/E4, K = 10, W24

Cadre : `exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`. Deux sessions de
`gcp-migration/v11_session.py` sur la VM G4 gardée `ehgp-v7-3b1d496aed430749ea7e049f` (us-central1-c), trames entières
sans sol `lidar_ng00/01/02` (39 885, 35 551 et 45 845 sites), K = 5 sauf mention, feuilles de 16, sortie FULL complète.
Banc : `bench/ab_g4.py` (un processus neuf par prise, dumps exigés identiques à la référence de chaque trame, cinq
prises à 48 fils et une à un fil) puis `bench/full_timing.py`. Lecture appariée : `bench/ab_summary.py`.

| Session | Commit | Variantes (binaires) | Statut | Arrêt |
| --- | --- | --- | --- | --- |
| `claudeab8` | `54c167bb6` | `base` = `17514012b` (`7d3a09f0ae90`), `q3` = q3 différé `56216392e` (`12644f20ccaa`), `new` = q3 + lemme R `9b9244a00` (`5a6f78ad767d`) | `completed` ; 673/673 portes ; mutants `num` 3/3 et `catalogue` tués | `TERMINATED` certifié |
| `claudediag1` | `e49ea4690` | `qr` = `qr2` = q3 + R (`5a6f78ad767d`, même binaire que `new` ci-dessus : bras A/A), `r1` = compteurs locaux `0c358261c` (`c1351d791ed6`), `new` = enveloppes M3/E4 (`967b2a54d52c`) | `failed_remote` : seul refus, la porte de style (fonction de 110 lignes, corrigée depuis par `publish_timings`) ; 676/678 portes, TSan 8/8, mutants R1 et M3/E4 7/7 tués | `TERMINATED` certifié |

Toutes les prises sont conformes et leurs dumps égalent la référence de leur trame (176 contrôles de `check.py`).

## À un fil : le seul régime qui tranche ici

Rapport B/A du mur FULL (et de la passe unique du catalogue), une paire par trame, ng00 / ng01 / ng02 :

| Changement | Mur | Passe unique |
| --- | --- | --- |
| bras A/A (`qr` → `qr2`, même binaire) | 0,9972 / 1,0044 / 0,9995 | 0,9982 / 1,0041 / 0,9981 |
| q3 différé (`base` → `q3`) | **0,9870 / 0,9904 / 0,9893** | **0,9822 / 0,9838 / 0,9850** |
| lemme R (`q3` → `new`) | 0,9997 / 1,0058 / 1,0012 | 1,0028 / 1,0035 / 0,9984 |
| compteurs locaux R1 (`qr` → `r1`) | 0,9994 / 1,0029 / 0,9988 | 0,9986 / 1,0020 / 0,9988 |
| enveloppes M3/E4 (`r1` → `new`) | **1,0030 / 1,0135 / 1,0103** | **1,0136 / 1,0143 / 1,0120** |

Le bruit du bras A/A est d'environ ±0,5 %. Lecture : le q3 différé gagne 1,0 à 1,3 % du mur et 1,5 à 1,8 % de la
passe unique ; le lemme R et R1 sont neutres, ce qu'on attendait de R1, qui est un correctif de contrat ; les
enveloppes M3/E4 **coûtent** 1,2 à 1,4 % de passe unique, sur les trois trames, au-delà du bras A/A. Leur exactitude
est relue favorablement par l'auditeur (7 552 gardes), mais le gain attendu n'est pas là : leur test coûte plus que les
candidats qu'il évite. Une paire par trame reste descriptive.

## À 48 fils

Les rapports par paire vont de 0,82 à 1,10 et le bras A/A seul s'écarte jusqu'à 9 % sur la passe unique : à cinq
paires (p bilatérale minimale 0,0625), aucun de ces changements n'est mesurable ici. Les rapports complets sont ceux
de `ab_summary.py` rejoué sur `sessions/*/ab_report.json`.

## K = 10 et nombre de fils (`claudediag1`, binaire `new`)

Médianes de mur FULL en ms (trois prises à K = 10, cinq à K = 5), ng00 / ng01 / ng02 :

| Configuration | ng00 | ng01 | ng02 |
| --- | ---: | ---: | ---: |
| K = 10, feuilles 16, W48 | 3 284,7 | 2 506,4 | 2 738,8 |
| K = 10, feuilles 24, W48 | **2 506,1** | **1 822,5** | **2 064,5** |
| K = 5, W24 épinglé sur un fil par cœur (`taskset 0-23`) | 574,9 | 442,6 | 527,7 |
| K = 5, W48 libre | 408,4 | 296,1 | 359,7 |

À K = 10, des feuilles de 24 font gagner 24 à 27 % ; c'est la première référence K = 10 de la v11, à 18–33 fois le
contrat de 100 ms. Les 48 fils logiques battent les 24 cœurs épinglés d'un facteur 1,4 à 1,5 : l'hyperthreading paie
sur ce moteur, qui attend la mémoire.

## Rejeu

`python3 check.py` (et `python3 -O check.py`) relit les deux reçus de session (statut, arrêt certifié, cible exacte),
le verdict et les refus des deux rapports, la conformité et l'identité de chaque prise, puis recalcule les rapports
W1 du tableau et les médianes K = 10 et W24/W48 ; verdict attendu `recu_mesures_verdict conforme controles176`. Les
archives `results.tar.gz` ne sont pas versionnées : leurs empreintes sont dans les `receipt.json`. Aucune coordonnée
n'est copiée. Ce reçu mesure ; il ne qualifie aucun gain au sens du protocole apparié et ne change aucun statut public.
