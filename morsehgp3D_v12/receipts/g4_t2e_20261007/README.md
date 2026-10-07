# Session G4 v12.20261007.t2e2 : `G-L3` (seconde moitié), profil `MES-M7`, `MES-E`

7 octobre 2026. Session gardée (`gcp-migration/v12_session.py`, instantané du worktree à `576e7aaf9`), cible
`us-central1-c` / `ehgp-v7-3b1d496aed430749ea7e049f`, `--max-run-seconds 4200`, sans construction par défaut. VM démarrée à
18:43:03 UTC, worker lancé à 18:45:32, sorti au code 0 à 19:08:46, **arrêt certifié `TERMINATED`** par le lanceur
(clôture `stopped`, génération 18:43:03) et relu indépendamment à 19:10:44 (`lastStopTimestamp` 19:10:22 UTC). Reçu sans
identité de compte : [`receipt.json`](receipt.json) ; résultats choisis sous `resultats/` ; empreintes : `SHA256SUMS`.

Une première tentative (`v12.20261007.t2e`, 18:23 UTC) n'a pas démarré de VM : rupture de stock du GPU dans la zone
(`ZONE_RESOURCE_POOL_EXHAUSTED`, `STOCKOUT`) ; le lanceur a conclu `shutdown_uncertified` (génération inconnue) ; deux
lectures seules (18:25 et 18:29 UTC) ont confirmé `TERMINATED`, dernier démarrage inchangé (celui de la session D) et la
seule opération `start` récente en échec : aucune VM n'a tourné.

```text
phase=exploration_v12_hors_registre
backend=cpu_reference (microbancs hors produit, v11 gelée ac081a06f liée)
quantification=quantized_u21_input_only
public_status=not_claimed
```

| Épingle | Valeur |
| --- | --- |
| paquet | `5ee835b31c8a208f…` |
| plan | `1d2efde91527f662…` |
| données (32 fichiers : trames ng00–02, uniformes, archive v11, découpes de `MES-E`) | manifeste `db97037d5d956c55…` |

| Commande | État | Durée | Pic RSS |
| --- | --- | ---: | ---: |
| `source_v11` | ok | 5 s | 0,3 Go |
| `g1_k5` (pilote de `G-L3`, construction, portes, campagne de 5 processus × 3 cas, juge `REGLE_G1`) | ok | 300 s | 0,7 Go |
| `g1_k5_publier` | ok | 0 s | — |
| `m7` (profil de la résolution, ng00–02 à K5, ng00 à K10) | ok | 304 s | 3,1 Go |
| `mes_e` (v11 gelée sur découpes LiDAR réelles, 11 prises) | ok | 776 s | 78,6 Go |

## `G-L3` : **rejeté** (règle écrite d'avance, `REGLE_G1`)

[Tables](resultats/cmd/002_g1_k5_publier/files/g1_k5/tableaux_g1.md). Le saut certifié par les voisins évite 81,1 /
81,5 / 82,5 % des censuses saturés sur ng00 / ng01 / ng02 à K5 et la forêt reste **identique** à celle de la v11 dans les
quinze prises ; mais la résolution à un fil est **plus lente** : rapport saut / v12 de 1,032 / 1,041 / 1,035, bornes hautes
de l'IC 95 % 1,036 / 1,043 / 1,036, toutes au-dessus de 1. À l'ordre 5 de ng00, 219 779 tentatives pour 125 275 sauts
certifiés : les tentatives qui échouent (sphères à $p<k$ ou candidats insuffisants) paient leurs 14,9 tests exacts puis le
census. Le levier n'est pas adopté ; la voie produit garde le census gardé.

## `MES-M7` : profil de la résolution sur G4 (publié)

[Lignes JSON](resultats/cmd/003_m7/files/m7/). Bras `replique_v12`, un fil, compteur de cycles encadré par `lfence`, graines
identiques au vidage et occurrences égales aux routes à tous les ordres (contrôles conformes 4/4, 4/4, 4/4, 9/9). Parts
des cycles sur les ordres 2..K :

| Cas | sondes | proposition + `LEM-T1` | certificat | census saturé | census complet | saut | trace stricte | reste | secondes |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| ng00 K5 | 38,6 % | 26,0 % | 2,0 % | 12,8 % | 5,8 % | 0,4 % | 1,4 % | 13,0 % | 1,75 |
| ng01 K5 | 39,9 % | 25,5 % | 2,0 % | 11,9 % | 5,1 % | 0,4 % | 1,5 % | 13,7 % | 1,32 |
| ng02 K5 | 42,9 % | 25,9 % | 1,6 % | 9,3 % | 4,6 % | 0,3 % | 1,5 % | 14,0 % | 1,57 |
| ng00 K10 | 34,4 % | 32,1 % | 2,8 % | 13,5 % | 5,6 % | 0,3 % | 1,6 % | 9,6 % | 15,62 |

À l'ordre 5 de ng00 : 148 ns par sonde, 394 ns par proposition, 1,03 µs par census saturé, 1,51 µs par census complet.
**Les sondes de la table de populations puis la proposition sont les premiers postes** (le profil local le disait) :
ordre des leviers de l'étage G confirmé, `G-L5` (sondes en masse par jointure triée) et la proposition d'abord.

## `MES-E` : la v11 gelée sur des scènes réelles de 1 à 8 millions de sites (publié)

[Tableau](resultats/cmd/004_mes_e/files/mes_e/mes_e.md). Voie CPU de la v11 (`802811`), 48 fils, budget de la sonde
160 Gio, vidage FULL écrit vers `/dev/null` ; toutes les prises au code 0 :

| Scène (sans sol sauf ETH3D) | K | sites | secondes | pic RSS | boules / site | incidences / site | µs / site |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Boreas (10 trames agrégées) | 5 | 1 M | 27,6 | 16,4 Gio | 44,3 | 208 | 27,6 |
| IGN Lyon | 5 | 1 / 2 / 4 / 8 M | 16,0 / 33,6 / 71,7 / 155,3 | 11,7 / 18,8 / 38,0 / 75,0 Gio | 25,1 à 26,4 | 114 à 121 | 16,0 à 19,4 |
| ETH3D courtyard | 5 | 1 / 2 / 4 / 8 M | 16,1 / 34,4 / 84,2 / 203,0 | 9,4 / 14,9 / 28,2 / 53,9 Gio | 20,9 à 21,0 | 95 à 98 | 16,1 à 25,4 |
| IGN Lyon | 10 | 1 M | 84,1 | 40,4 Gio | 95,1 | 767 | 84,1 |
| ETH3D courtyard | 10 | 1 M | 48,6 | 26,3 Gio | 66,4 | 517 | 48,6 |

Lecture : à K5, le temps par site de la v11 reste presque constant de 1 à 8 millions sur IGN (16 à 19 µs), mais l'étage
forêt dérive sur ETH3D (4,3 s à 1 M, 121,9 s à 8 M : ×28 pour ×8 sites) ; la mémoire résidente vaut 7 à 17 Ko par site à
K5 et 28 à 43 Ko par site à K10, ce qui place la rupture de la v11 à K10 vers 4 millions de sites sur cette VM de 177 Gio.
Ces chiffres fixent les objectifs du régime (b) de la v12 : **flux par lots** (résidence bornée, `ARCHITECTURE.md` § 4.6) et
un étage forêt linéaire.

## Ce que cette session n'établit pas

Ni le catalogue ni la tour de la v12 (absents de l'instantané joué, hors microbancs), ni le budget de 100 ms : ce sont des
microbancs et des mesures de la v11 gelée. GCP utilisé pour cette seule session, arrêt certifié.
