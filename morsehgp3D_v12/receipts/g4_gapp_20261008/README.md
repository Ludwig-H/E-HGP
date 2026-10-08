# Session G4 G-APP : l'étage G sur l'appareil, trois postes archétypes (`MES-G-APP`) — rejeté sur les propositions

8 octobre 2026. Session gardée `v12.20261008.gapp` (`gcp-migration/v12_session.py`, commit `389b5e453`, preuve
`pushed_commit`), cible `us-central1-c` / `ehgp-v7-3b1d496aed430749ea7e049f`, `--max-run-seconds 4200`. VM de 10:39:11
à 10:44:37 UTC, **arrêt certifié `TERMINATED`** par le lanceur, relu indépendamment à 10:45:09 UTC. Reçu sans
identité de compte : [`receipt.json`](receipt.json) ; sorties sous `resultats/` ; empreintes : `SHA256SUMS`.

```text
phase=exploration_v12_hors_registre
backend=cpu_reference (produit, témoin) ; cuda_g4 (noyaux du microbanc, hors produit)
objet=full_pi0 (étage G de la tour K1..5 : requêtes et résultats)
quantification=quantized_u21_input_only
public_status=not_claimed
```

Microbanc [`mes_g_appareil`](../../microbancs/mes_g_appareil/README.md), issu de
l'[étude de faisabilité](../developpement_20261008/etude_g_appareil.md). Les requêtes de census sont interceptées dans
le vrai `resolve_tower` du produit ; les sondes et les propositions sont rejouées sur les mêmes entrées. Chaque poste
est joué par le produit sur l'hôte (48 fils) et par un noyau en source unique `__host__ __device__` sur l'appareil, avec
identité exigée à l'octet. Trames : ng00 (39 885 sites), la trame médiane de `v12set` (`kitti_ng_02_001606`, 64 740) et
la trame maximale (`kitti_ng_08_002119`, 99 099). 5 processus par trame, 5 passes, A/A des deux côtés. Mutant « côté
nul » tué (code 1).

| Commande | État | Durée |
| --- | --- | ---: |
| `g_app_autotest` | ok | 0,5 s |
| `g_app_pilote` | ok, verdict rendu, aucun refus | 43 s |

## Verdict de `REGLE_G_APPAREIL` (écrite à 10:04 UTC, avant toute mesure) : **rejeté**

Rapport appareil / hôte (moyenne géométrique sur 5 processus, IC 95 %). Seuils : census ≤ 0,20, sondes ≤ 0,20,
propositions ≤ 0,50, borne haute, sur chacune des trois trames :

| Trame | census | sondes | propositions |
| --- | --- | --- | --- |
| ng00 | **0,107** (0,104–0,110) | **0,053** (0,052–0,053) | 0,758 (0,754–0,762) |
| médiane | **0,106** (0,105–0,106) | **0,051** (0,051–0,051) | 0,738 (0,737–0,740) |
| maximum | **0,091** (0,090–0,093) | **0,046** (0,046–0,047) | 0,738 (0,737–0,739) |

Temps (ms, hôte → appareil, sans transferts), trame maximale : census 21,0 → 1,9 ; sondes 16,5 → 0,8 ; propositions
7,5 → 5,5. Identité complète sur les trois trames : 646 621 census, 9 998 189 représentants et 2 481 101 propositions
sur la trame maximale, aucune requête non résolue. A/A de 0,980 à 1,003.

**Lecture.** Le census (×9 à ×11) et les sondes (×19 à ×22) passent largement leurs seuils. Les propositions DWelzl
restent en binaire64, et ce GPU les calcule à 1/64 du débit FP32 : elles ne gagnent que 26 %, et la règle rejette la
suite telle qu'elle était conçue, avec les trois postes sur l'appareil. En temps absolu, les trois postes de la trame
maximale passent pourtant de 45,0 à 8,2 ms. Une suite exigerait une autre conception des propositions, avec une règle
nouvelle écrite d'avance : voie entière exacte pour les paires et les triangles aigus, et binaire64 pour le reste ; ou
propositions sur l'hôte, recouvertes par le reste de G sur l'appareil. La tranche « G sur l'appareil » n'est pas
ouverte sur ce verdict.

## Information non jugée : le census « à plat » sur l'hôte

La même source unique, jouée sur l'hôte, coûte **0,570 / 0,581 / 0,585** fois le census du produit (ng00 / médiane /
maximum). Elle évite `Result`, le verrou par requête et `Point::make`. Le census pèse environ 23 % de G à un fil après
T2-d-B : ce serait un levier CPU d'environ −10 % de G, transmis au chantier T2-d-B2. Les propositions jouées par cette
source sur l'hôte coûtent 0,97 à 0,98 fois celles du produit : aucun gain.

Appareil relevé : NVIDIA RTX PRO 6000 Blackwell Server Edition, 188 SM, cc 12.0, L2 de 128 Mio, 95 Gio, bus de 512 bits,
pilote 13.0. Transferts publiés à part (trame maximale, hôte vers appareil : census 3,7 ms, sondes 12,0 ms,
propositions 2,2 ms).

## Ce que cette session n'établit pas

Ni le coût d'intégration d'un G complet sur l'appareil (fronts, compactages, lancements), ni la voie entière des
propositions sur l'appareil, ni le gain du census à plat dans le produit. GCP utilisé pour cette seule session, arrêt
certifié.
