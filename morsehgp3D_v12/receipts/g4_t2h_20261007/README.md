# Session G4 v12.20261007.t2h : premiers temps de l'étage G de la tour, portes LiDAR, `MES-P` à 1, 4 et 48 fils

7 octobre 2026. Session gardée (`gcp-migration/v12_session.py`, instantané du worktree à `9c5809919`, soit le produit
de `99fa83246` : étage G de la tour et budget sous cache), cible `us-central1-c` / `ehgp-v7-3b1d496aed430749ea7e049f`,
`--max-run-seconds 4200`, **construction par défaut** au profil 21 sur la chaîne de la VM. VM démarrée à 21:35:32 UTC,
worker de 21:37:44 à 22:18:34 (code 1 : dernière commande coupée à l'échéance), **arrêt certifié `TERMINATED`** par le
lanceur (clôture `stopped`) et relu indépendamment à 22:20:56 UTC (`lastStopTimestamp` 22:20:19 UTC). Reçu sans
identité de compte : [`receipt.json`](receipt.json) ; sorties sous `resultats/` ; empreintes : `SHA256SUMS`.

```text
phase=exploration_v12_hors_registre
backend=cpu_reference (produit v12 ; v11 gelée ac081a06f pour MES-P)
quantification=quantized_u21_input_only
public_status=not_claimed
```

| Épingle | Valeur |
| --- | --- |
| paquet | `13877b3d37ce4cde…` |
| plan | `55bb91bb87ba03a5…` |
| données (trames ng00–02, archive des sources de la v11, archive `g4_small`) | manifeste `b11646af4d97e784…` |

## Portes sur la VM

`ctest -LE long` : **637 sur 637** (166 s). `ctest -L lidar` : **6 sur 6** (catalogue ng00–02 à K5 contre la v11,
Euler sur ng00, déterminisme de l'étage G sur ng00 à K5, 1 contre 8 fils).

## Étage G du produit (`mhgp12_tower_probe`, médiane des passes chaudes, ms)

Index et catalogue sont construits une fois par processus ; chaque passe rejoue l'étage G (classification et tables
« tables », comptage et remplissage des cellules, résolution), mesuré par la sonde.

| Cas | fils | passes | G chaud (max) | tables | résolution | par ordre (ordres 1 à K) | pic (Mo) |
| --- | ---: | ---: | ---: | ---: | ---: | --- | ---: |
| ng00 K5 | 48 | 10 | 80,1 (83,3) | 18,6 | 48,7 | 0,2 / 3,2 / 6,6 / 11,9 / 26,1 | 325 |
| ng01 K5 | 48 | 10 | 63,2 (64,7) | 15,4 | 37,0 | 0,2 / 2,7 / 5,3 / 9,5 / 19,4 | 274 |
| ng02 K5 | 48 | 10 | 75,7 (77,8) | 19,0 | 43,5 | 0,3 / 3,4 / 6,7 / 11,4 / 22,0 | 345 |
| ng00 K5 | **1** | 2 | 1 567,3 | 40,3 | 1 476,3 | 2 / 90 / 190 / 361 / 833 | 325 |
| ng00 K10 | 48 | 3 | 633,6 (645,6) | 120,6 | 437,9 | ordre 10 : 159,2 | 1 551 |
| ng01 K10 | 48 | 3 | 449,3 (457,6) | 82,4 | 307,7 | ordre 10 : 106,6 | 1 234 |
| ng02 K10 | 48 | 3 | 517,5 (518,2) | 101,0 | 342,0 | ordre 10 : 114,1 | 1 528 |
| uniforme 8 000 K5 | 48 | 5 | 31,2 | 6,6 | 19,6 | | 156 |
| uniforme 16 000 K5 | 48 | 5 | 75,4 | 16,5 | 47,6 | | 321 |
| uniforme 32 000 K5 | 48 | 5 | 181,9 | 42,1 | 114,1 | | 660 |

Empreintes identiques aux portes locales (ng00 K5 `231d826bb0d4fe57`, uniformes `5304d1c8…`, `fdd42b4e…`,
`9fd0fd9e…`) : la VM reproduit l'objet du codespace.

**Lecture.** Le budget de l'étage G est 25 à 30 ms à K5 (contrat de la tour, § 10) : le produit en est à **2,5 à 3
fois**. La résolution passe bien à l'échelle (×30 de 1 à 48 fils, efficacité 63 %), mais la classification et les
tables ne gagnent que ×2,2 (40 → 19 ms) : un quart de G à K5. Les deux chantiers de T2-c sont donc confirmés et
ordonnés : tables parallèles ou résidentes (visé ≤ 3 ms), puis coût unitaire de la résolution divisé par deux
(`G-L5`, recherche des supports avec moins de défauts de cache). À K10, l'ordre 10 seul coûte 107 à 159 ms.

## `MES-P` : la v11 gelée à 1, 4 et 48 fils (hors réseau et quasi-sphère)

140 des 149 nuages joués avant l'échéance (840 prises, deux expirations à K10 sur un fil : uniforme et huit amas de
10 000 sites). Cohorte commune des familles réelles : 123 nuages, identique aux trois nombres de fils.

| Familles réelles, K = 5 | 1 fil | 4 fils | 48 fils |
| --- | ---: | ---: | ---: |
| 100 à 299 sites (médiane 122) | 11,9 ms | 5,6 ms | 6,3 ms |
| 300 à 999 sites (423) | 52,4 ms | 18,6 ms | 10,4 ms |
| 1 000 à 2 999 sites (1 396) | 211,9 ms | 64,3 ms | 19,8 ms |
| 3 000 à 10 000 sites (5 820) | 749,2 ms | 215,0 ms | 44,4 ms |

À K10 et 48 fils : 11,0 / 25,1 / 66,5 / 224,0 ms dans les mêmes classes. **Lecture** : la v11 à un fil coûte 90 à
140 µs par site à K5, vingt fois son coût à 48 fils ; son coût fixe à 48 fils (5 à 7 ms) n'est donc pas celui du pool
(un fil est plus lent dès 100 sites), et 4 fils égalent 48 fils sous 300 sites. Le seuil de la voie CPU de la v12 se
jugera sur la v12 elle-même, à ces trois nombres de fils.

## Ce que cette session n'établit pas

Ni le catalogue appareil, ni les étages T, M, V et R, ni la chaîne complète : le temps de la tour n'est pas encore
mesurable de bout en bout. GCP utilisé pour cette seule session, arrêt certifié.
