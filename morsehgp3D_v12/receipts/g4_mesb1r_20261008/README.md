# Sessions G4 L1 et L1 rejouée : la v12 sur 15 scènes LiDAR réelles entières (`MES-B`, régime (b))

8 octobre 2026. Deux sessions gardées (`gcp-migration/v12_session.py`, cible `us-central1-c` /
`ehgp-v7-3b1d496aed430749ea7e049f`, `--max-run-seconds 4200`, mode `--commit`, preuve `pushed_commit`). Reçu sans
identité de compte : [`receipt.json`](receipt.json) (session rejouée) ; sorties sous `resultats/` ; empreintes :
`SHA256SUMS`.

```text
phase=exploration_v12_hors_registre
backend=cuda_g4 (catalogue, voie hybride) ; cpu_reference (G, T, M, V, R ; bras CPU identifié)
objet=full_pi0 (tour FULL K1..5, verticales et registre compris)
quantification=quantized_u21_input_only
public_status=not_claimed
```

| Session | Commit | VM | Mesure | Rapatriement | Arrêt |
| --- | --- | --- | --- | --- | --- |
| `v12.20261008.mesb1` | `403736300` | 04:40:05 – 04:58:29 UTC | `mes_b`, worker au code 0 | **refusé** : 878 Mo libres sur le codespace, le lanceur exige 2 × l'archive + 1 Gio | `TERMINATED` certifié |
| `v12.20261008.mesb1r` | `ea62cd691` | 05:13:00 – 05:31:32 UTC | archive de L1 récupérée sur la VM, puis `mes_b` rejoué (760,8 s, pic RSS 62,4 Go) | vérifié | `TERMINATED` certifié |

L'archive de L1 (23 176 octets, SHA-256 `3497c745…573b`, égale à son `SHA256SUMS`) est restée sur le disque persistant
de la VM : l'élagage du lanceur garde `results.tar.gz` des sessions passées. Elle a été copiée par
[`archives_vm.py`](../../microbancs/outils/archives_vm.py) en première commande de la session rejouée ; son contenu
texte est extrait sous `resultats/cmd/000_archive_l1/files/l1_archive/extrait/`. Pour libérer le codespace, 131
paquets sources de sessions v10/v11 terminées ont été déplacés hors de `/workspaces`.

Construction sur la VM : GCC 11.4.0, nvcc 12.9.41, profil 21, CUDA ; carte NVIDIA RTX PRO 6000 Blackwell Server
Edition (97 887 Mio), GPU vide avant et après les deux campagnes. Empreintes de la sonde : `5bbd7b02…` (L1) et
`3c2c340e…` (L1 rejouée, qui publie en plus la mémoire par étage) ; journal de construction et extrait du `CMakeCache`
dans le rapport.

## Résultats (session rejouée ; L1 concorde à 1–3 % près, voir plus bas)

Captations entières : dalles IGN de 1 km², scans ETH3D, placettes FOR-instance, fenêtres de 1, 10 et 50 trames
consécutives de Boreas. Positions distinctes au millimètre ; sans sol = classe du producteur ou Patchwork++ épinglé.
K5, 48 fils, budget de l'hôte 160 Gio, budget propre de l'appareil 88 Gio, deux passes (une pour les dernières
scènes), mur de l'entrée quantifiée en mémoire à la tour complète en mémoire.

| Scène entière | sites | voie | mur chaud | s par million | hôte (budget) | appareil gardé |
| --- | ---: | --- | ---: | ---: | ---: | ---: |
| Boreas, 1 trame, sans sol | 146 316 | appareil | 0,52 s | 3,5 | 1,1 Gio | 1,8 Gio |
| Boreas, 1 trame | 215 665 | appareil | 0,70 s | 3,2 | 1,5 Gio | 2,2 Gio |
| Boreas, 10 trames, sans sol | 1 513 483 | appareil | 10,0 s | 6,6 | 15,9 Gio | 21,5 Gio |
| Boreas, 10 trames, sans sol | 1 513 483 | CPU | 22,3 s | 14,8 | 22,8 Gio | — |
| Boreas, 10 trames | 2 153 342 | appareil | 13,0 s | 6,0 | 20,2 Gio | 28,5 Gio |
| IGN Marseille, sans sol | 2 465 285 | appareil | 11,0 s | 4,4 | 15,9 Gio | 23,0 Gio |
| FOR-instance SCION 61, sans sol | 3 439 371 | appareil | 42,0 s | 12,2 | 58,2 Gio | 80,7 Gio |
| FOR-instance SCION 61 | 3 589 247 | appareil | 42,8 s (froide) | 11,9 | 59,4 Gio | 81,2 Gio |
| ETH3D meadow, station 1 | 6 181 091 | appareil | 35,3 s | 5,7 | 32,6 Gio | 47,5 Gio |
| IGN Marseille | 6 709 045 | appareil | 31,6 s (froide) | 4,7 | 38,2 Gio | 55,0 Gio |

**Refus de ressources (`resource_exhausted/memory_budget`), tous au budget de l'appareil, en 4 à 8 s :** FOR-instance
TUWIEN (5,20 M sans sol, 6,24 M avec sol), NIBIO 12 (7,79 et 7,83 M), Boreas 50 trames (7,86 M sans sol, 10,77 M avec
sol), et K10 dès Boreas 10 trames sans sol (1,51 M) et IGN Marseille sans sol (2,47 M). Aucun échec, aucune sortie
illisible, aucun crash. Empreinte FUL1 identique entre passes et entre les deux voies sur Boreas 10 trames sans sol
(`49f90d23…`), ainsi que sur les deux trames seules.

Étages à chaud, en secondes (mur = P + C + G + raccord + T + M + V + R) :

| Scène | C (dont transferts) | G | T | M | V | R | validation (hors mur) |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Boreas 10 trames sans sol | 1,74 (0,47) | 2,95 | 3,51 | 0,46 | 0,14 | 0,95 | 3,25 |
| IGN Marseille sans sol | 1,78 (0,46) | 2,84 | 4,19 | 0,50 | 0,20 | 1,07 | 3,73 |
| SCION 61 sans sol | 5,80 (1,79) | 14,63 | 14,25 | 1,67 | 0,56 | 4,07 | 15,29 |
| ETH3D meadow | 3,40 (0,81) | 7,73 | **20,00** | 1,14 | 0,24 | 1,97 | 5,25 |
| IGN Marseille | 4,03 (1,04) | 6,15 | **15,39** | 1,34 | 0,61 | 2,98 | 14,02 |

Mémoire du budget de l'hôte par étage (Ko par site, en usage à la fin de l'étage / pic pendant l'étage), voie
appareil : C 1,6 à 7,5 / 1,7 à 9,9 ; G 3,5 à 12,4 / 4,3 à 14,3 ; tour complète 5,3 à 17,2 / 5,7 à 18,2. Le pic de la
passe est celui de T, M, V, R ; sur la voie CPU, c'est celui du catalogue (16,2 Ko par site sur Boreas 10 trames sans
sol, contre 5,8 sur la voie appareil). Tableaux complets :
[`tableaux_b.md`](resultats/cmd/001_mes_b/files/b/tableaux_b.md).

## Verdicts écrits d'avance : non tenu

- **B1** (K5, au plus 2 s par million) : non tenu, de 3,2 à 12,2 s par million, et cinq refus sous 10 M.
- **B2** (aucun refus sous 10 M à K5) : non tenu (cinq scènes).
- **B3** (exposant ≤ 1,1 sur les séries Boreas) : non évalué, Boreas 50 trames étant refusé.
- **B4** (K10, au plus 10 s par million) : non tenu (deux refus).

**Reproductibilité.** La session L1, rapatriée par la session suivante, donne les mêmes issues (mêmes neuf scènes
calculées, mêmes neuf refus) et des murs à 1–3 % près (par exemple 3,629 contre 3,547 s par million sur la trame
Boreas sans sol, 12,218 contre 12,197 sur SCION 61 sans sol).

## Lecture

1. **La mémoire de l'appareil est le mur.** Le catalogue garde sur la carte tous ses tableaux, à leur plus grande
   capacité, et une croissance par 1,5 réserve l'ancien et le nouveau tableau ensemble : 8,3 à 25,2 Ko d'appareil par
   site à K5 (ETH3D le moins, la forêt de FOR-instance le plus), donc un refus dès 5 M de sites sur une carte de
   96 Go. Le catalogue en flux par lots de feuilles, avec un budget d'appareil fixe ([`ARCHITECTURE.md`](../../docs/ARCHITECTURE.md)
   § 4.6), est la condition du régime (b). Il n'existe pas encore.
2. **T croît plus vite que les sites.** Le noyau union-find, séquentiel par ordre, coûte 20 s à 6,2 M de sites (ETH3D),
   soit 3,2 µs par site, contre 0,8 µs vers 40 000 sites. C'est l'étage qui s'éloigne le plus de la linéarité.
3. **L'hôte suit** : de 5,3 à 17,2 Ko par site pour la tour complète. 160 Gio porteraient de 10 à 30 M de sites selon
   la scène, une fois l'appareil borné.
4. Hors du mur, l'empreinte FUL1 coûte 55,6 s pour 1,51 M de sites (37 µs par site, un seul flux SHA-256 portable) et
   la validation jusqu'à 15 s.

## Ce que ces sessions n'établissent pas

Aucun objectif du régime (b) n'est tenu. Il n'y a ni mesure à K10 (refusée), ni identité FUL1 au-delà de
1,6 M de sites, ni scène de plus de 7 M de sites calculée. GCP utilisé pour ces deux seules sessions, arrêts
certifiés.
