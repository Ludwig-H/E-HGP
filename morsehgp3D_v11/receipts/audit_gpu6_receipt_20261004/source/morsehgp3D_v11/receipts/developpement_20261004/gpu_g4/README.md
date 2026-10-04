# Voie GPU des feuilles sur G4 — 4 octobre 2026

Cadre : `exploration_v11_hors_registre / cpu_reference (référence) et cuda_g4 (voie mesurée) /
quantized_u21_input_only / not_claimed`. Demande de l'utilisateur : « Fais les meilleurs temps possible (à froid ou à
chaud, K5 ou K10) pour GPU G4 », puis « pousse l'examen plus loin sur CUDA que simplement le temps (Nsight) ».
Voie décrite dans [`docs/CATALOGUE.md`](../../../docs/CATALOGUE.md), section « Voie GPU des feuilles ». Six sessions
`gcp-migration/v11_session.py` sur la VM G4 gardée `ehgp-v7-3b1d496aed430749ea7e049f` (us-central1-c ; RTX PRO 6000
Blackwell Server Edition, pilote 580.178.04, 97 887 Mio, `sm_120` ; CUDA 12.9.41, CMake 3.22.1, GCC 11.4), trames
entières sans sol `lidar_ng00/01/02` (39 885, 35 551, 45 845 sites), 48 fils, sortie FULL complète.

Bancs : [`bench/gpu_ab.py`](../../../bench/gpu_ab.py) construit la sonde CUDA sur la VM, joue chaque mode à froid
(un processus par prise, ordre de Williams) et à chaud (un processus, passes FULL successives), et exige à chaque
prise le même dump et le même registre du catalogue que la première prise CPU de la trame (à chaud : passes 1..P
toutes réussies, identité de la dernière passe). [`bench/gpu_profile.py`](../../../bench/gpu_profile.py) passe Nsight
Systems 2025.3.1 et Nsight Compute 2025.2.1.3 (épinglés par sha256) sur ng00.

## Sessions

| Session | Commit | Statut | Ce qu'elle établit |
| --- | --- | --- | --- |
| `claudegpu1` | `77db5738e` | `failed_remote` | CMake 3.22.1 de la VM ne connaît pas l'option du dialecte CUDA20 : arrêt à la génération ([journal](sessions/claudegpu1/build_configure.log)) |
| `claudegpu2` | `d5b1d0179` | `failed_remote` | construction CUDA réussie ; toute la voie GPU refusée par une garde fausse « feuilles ≤ sites » (les feuilles se recouvrent) ; outils Nsight validés sur la VM |
| `claudegpu3` | `00800dd88` | `completed` | première voie GPU exacte : dumps et registres identiques à K = 5 et 10 ; plus lente que le CPU ; premiers rapports Nsight |
| `claudegpu4` | `b74f9ea3a` | `completed` | cases par feuille, rassemblement parallèle, pages pré-touchées, pool mémoire ; trois modes (CPU, lot hôte, GPU) |
| `claudegpu5` | `16b482169` | `completed` | blocs d'un warp, copie parallèle du bloc de lot : le GPU passe devant à K = 10 |
| `claudegpu6` | `22a6af6aa` | `completed` | enregistrements compacts en rangs locaux, cases de 2 Kio |

Les six arrêts sont certifiés `TERMINATED` sur la cible exacte (`receipt.json` de chaque session). Aux sessions 3 à
6, **aucune prise refusée** : 372 prises à froid et 84 processus à chaud (onze rapports) ont rendu le dump et le
registre du catalogue de la voie CPU (verdict `conforme` de chaque rapport).

## Meilleurs temps (mur FULL, ms)

Médiane à froid (cinq à sept prises, processus neufs) et meilleure passe à chaud (passes 2..P d'un même processus).

| K, feuille | Voie | Régime | ng00 | ng01 | ng02 | Session |
| --- | --- | --- | ---: | ---: | ---: | --- |
| 5, 16 | CPU | froid | 368,6 | 294,1 | 374,7 | 5 |
| 5, 16 | CPU | chaud | 363,3 | **274,6** | **330,0** | 5 |
| 5, 16 | CPU | chaud | **342,7** | 288,0 | 373,7 | 6 |
| 5, 16 | GPU | froid | 434,8 | 380,4 | 423,4 | 6 ; 5 pour ng01 |
| 5, 16 | GPU | chaud | 394,0 | 322,9 | 380,1 | 5 |
| 10, 24 | CPU | froid | 2 489,9 | 1 896,8 | 2 112,6 | 5 |
| 10, 24 | CPU | chaud | 2 437,2 | 1 796,5 | 2 029,5 | 5 |
| 10, 24 | GPU | froid | **2 398,6** | **1 796,7** | **2 009,4** | 6 ; 5 pour ng01 |
| 10, 24 | GPU | chaud | **2 348,5** | **1 761,9** | **1 987,2** | 5 |

À K = 5, la voie CPU reste la meilleure (275 à 343 ms à chaud) ; la voie GPU coûte 30 à 50 ms de plus. À K = 10, la
voie GPU gagne 2 à 6 % (1,76 à 2,35 s à chaud). Ce sont des diagnostics à quelques prises, pas une qualification ;
aucune revendication du contrat de 100 ms, qui reste à un facteur 3 (K = 5) et 20 (K = 10).

Pourquoi si peu à K = 5 : la passe unique CPU (parcours et feuilles) prend 127 à 160 ms, dont 40 à 63 ms de feuilles
à 48 fils ; la voie GPU retire ces feuilles du parcours (84 à 106 ms) mais ajoute l'exécuteur (71 à 116 ms à froid,
52 à 59 ms à chaud, session 6) et la matérialisation des Level (5 ms). À K = 10, les feuilles pèsent 415 à 535 ms sur
CPU contre 270 à 360 ms d'exécuteur GPU ; les forêts (1,2 à 1,7 s) dominent alors le mur.

## Ce que montre Nsight

**Nsight Compute** (`ncu --set full`, ng00, K = 5, feuilles 16 ; rapports bruts dans les archives des sessions) :

| Noyau | Session | Durée | Registres | Occupation théorique / atteinte | Fils actifs par warp | Créneaux d'émission occupés |
| --- | --- | ---: | ---: | --- | ---: | ---: |
| comptage | 3 | 44,4 ms | 210 | 16,7 % / 16,6 % | 3,36 / 32 | 31,6 % |
| écriture (toutes les feuilles) | 3 | 40,8 ms | 164 | 25 % / 19,9 % | 3,43 / 32 | 33,7 % |
| comptage | 5 | 38,5 ms | 168 | 25 % / 21,6 % | 3,43 / 32 | 34,8 % |
| écriture (feuilles qui débordent) | 6 | 21,6 ms | 168 | 25 % / 2,1 % | 4,10 / 32 | 5,3 % |

- **Divergence** : un fil traite une feuille entière, et les feuilles ont des coûts très inégaux (la moitié n'émet
  rien, les plus lourdes émettent jusqu'à 189 boules à K = 5 et 609 à K = 10) ; en moyenne 3,2 à 3,4 fils sur 32
  travaillent. Le tri des feuilles par taille ne l'a pas changé : la taille ne prédit pas le travail.
- **Mémoire locale** : 3,2 Kio de pile par fil (préfixes, masques, cache J2 simulé), lus à 2,2 octets utiles par
  secteur de 32 ; accès globaux non coalescés (35 à 48 % de secteurs en trop). Le pipeline ALU entier n'est qu'à
  24 % : ce n'est pas l'arithmétique `i128` qui borne.
- **Attentes** (cycles par instruction émise, comptage) : à la session 3, attente fixe 1,54, barrière 1,32, mémoire
  longue 1,19. Retirer la barrière (blocs d'un warp, session 5) l'annule mais porte l'attente mémoire à 3,03 : le
  temps se déplace, le noyau ne gagne que 3 %.
- **Lignes chaudes** (échantillons d'attente, session 3) : barrière de la réduction par bloc 19,6 % ; test de droite
  du lemme Z (`center_line_meets`, arithmétique `i128`) environ 20 % ; cache J2 simulé, qui ne sert qu'aux compteurs
  du registre, 5 à 7 %.
- **Queue lourde** : les quelques feuilles qui débordent de leur case sont les plus lourdes ; rejouées chacune par un
  seul fil, leur passe coûte la durée de la plus lente (13 à 20 ms pour 7 à 15 feuilles à K = 5, 106 à 117 ms pour
  3 700 à 4 900 feuilles à K = 10).

**Nsight Systems** (session 3, trois passes sur ng00) : ouverture du contexte 78 ms (`cudaFree(0)`), en partie
recouverte à froid par l'ouverture anticipée ; envoi à 28 Go/s, mais retour à 4 Go/s (77 Mo en 19 ms) parce que les
pages de destination, neuves, faisaient faute pendant la copie. Préfixes CUB : 5 µs.

## Ce que chaque correctif a changé (ng00, médianes à froid, ms)

| Composant | S3 | S4 | S5 | S6 | K = 10 S3 → S6 |
| --- | ---: | ---: | ---: | ---: | --- |
| ouverture du contexte (attente) | 43,0 | 32,6 | 27,7 | 32,2 | 0 à 27 (recouverte) |
| comptage | 37,6 | 34,4 | 32,8 | 37,6 | 223,0 → 218,1 |
| écriture | 35,4 | 18,7 | 13,7 | 17,6 | 199,6 → 112,9 |
| retour | 21,9 | 5,5 | 5,6 | 2,3 | 107,1 → 7,8 |
| Level sur l'hôte | 15,3 | 15,2 | 15,2 | 5,6 | 66,7 → 21,4 |
| rassemblement des files | 10,5 | 2,4 | 2,4 | 2,5 | 18,7 → 3,1 |

Les cases par feuille ont supprimé la double exécution des feuilles. Les pages pré-touchées ont porté le retour à
plus de 10 Go/s. Le format compact a divisé le retour et la matérialisation par deux à trois. Mais la conversion
en rangs locaux (une recherche linéaire par incidence) ajoute environ 30 ms au comptage à K = 10, si bien que le
format compact est à peu près neutre sur le mur.

## Limites et suite

La voie « un fil par feuille » est bornée par la divergence et la mémoire locale, non par l'arithmétique. La suite
naturelle est une feuille coopérative par warp : sites et masques de la feuille en mémoire partagée, un fil par site
ou par candidat, phases de paires, triplets et quadruplets. C'est la forme J3 dont l'auditeur a accepté le contrat de
compteurs. Elle traiterait aussi la queue lourde. Les rangs locaux devraient venir de la feuille elle-même, sans
recherche. À K = 10, le mur est dominé par les forêts. Pour la qualification : les portes G4 natives demandées par
l'auditeur (q3 extrême, q4 au seuil 2^20 et 2^20 + 1, préfixe obtus, coquille à qmin = 2) restent à écrire.

## Rejeu

`python3 check.py` (et `python3 -O check.py`) relit les six reçus de session (statut, arrêt certifié, cible exacte),
les verdicts des rapports (sessions 3 à 6 conformes, sans refus ; session 2 refusée), puis recalcule à partir des
rapports chaque valeur des tableaux « Meilleurs temps » et « Ce que chaque correctif a changé », ainsi que les
métriques Nsight Compute citées. Verdict attendu : `recu_gpu_verdict conforme controles553`. Les archives `results.tar.gz` et les
rapports bruts Nsight (`.nsys-rep`, `.ncu-rep`) ne sont pas versionnés ; leurs empreintes sont dans les `receipt.json`
et les `gpu_profile.json`. Aucune coordonnée n'est copiée. Ce reçu mesure ; il ne change aucun statut public.
