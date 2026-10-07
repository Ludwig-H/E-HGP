# Session G4 v12.20261007.t0g : `MES-P`, la v11 gelée sur les petits nuages

7 octobre 2026. Session gardée (`gcp-migration/v12_session.py`, instantané du worktree à `0fd6c2286`), cible
`us-central1-c` / `ehgp-v7-3b1d496aed430749ea7e049f`, `--max-run-seconds 4200`, sans construction par défaut. VM démarrée à
19:32:14 UTC, worker de 19:33:55 à 19:39:50 (code 0), **arrêt certifié `TERMINATED`** par le lanceur (clôture `stopped`,
génération 19:32:14) et relu indépendamment à 19:42:41 UTC (`lastStopTimestamp` 19:41:25 UTC). Reçu sans identité de
compte : [`receipt.json`](receipt.json) ; résultats choisis sous `resultats/` ; empreintes : `SHA256SUMS`.

```text
phase=exploration_v12_hors_registre
backend=cpu_reference (v11 gelée ac081a06f, voie CPU 802811 ; microbanc hors produit)
quantification=quantized_u21_input_only
public_status=not_claimed
```

| Épingle | Valeur |
| --- | --- |
| paquet | `d214f6f2dcb8ae75…` |
| plan | `9920c66a4bbf35e6…` |
| données (une archive `g4_small.tar` et l'archive des sources de la v11) | manifeste `bcd183fec5d4d5c8…` |

| Commande | État | Durée | Pic RSS |
| --- | --- | ---: | ---: |
| `source_v11` | ok | 5 s | 0,3 Go |
| `mes_p` (`pilote_p.py --archive g4_small.tar --fils 48 --k 5,10 --passes 4 --delai 120`) | ok | 340 s | 2,4 Go |

Protocole : 159 nuages du paquet `g4_small` (132 morceaux de trames SemanticKITTI sans sol, familles `bout` 51, `kctx`
23, `knn` 30, `kobj` 28 ; 27 nuages synthétiques), chacun dans un processus neuf de la sonde FULL de la v11
(`mhgp11_full_bench`, vidage vers `/dev/null`), quatre passes par processus, temps chaud = médiane des passes 2 à 4,
48 fils, K5 et K10 ; 318 prises, 313 rendues. La droite unique du pilote (`mes_p.md` : coût fixe de −20,5 ms à K5) mêle
les familles réelles et les familles dégénérées et ne fixe rien ; la lecture ci-dessous est celle de
[`analyse_p.py`](../../microbancs/mes_p_petits/analyse_p.py), qui les sépare.

## Petits nuages LiDAR réels (familles `bout`, `kctx`, `knn`, `kobj`)

| K = 5 | prises | sites (médiane) | chaud, médiane (ms) | min | max | µs par site |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 100 à 299 sites | 35 | 128 | 6,3 | 3,9 | 9,3 | 45,8 |
| 300 à 999 sites | 36 | 413 | 10,4 | 8,1 | 17,3 | 23,8 |
| 1 000 à 2 999 sites | 28 | 1 449 | 19,7 | 12,3 | 28,1 | 12,5 |
| 3 000 à 10 000 sites | 33 | 5 865 | 47,1 | 23,4 | 108,1 | 8,7 |

| K = 10 | prises | sites (médiane) | chaud, médiane (ms) | min | max | µs par site |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 100 à 299 sites | 35 | 128 | 11,8 | 5,4 | 24,8 | 81,0 |
| 300 à 999 sites | 36 | 413 | 24,6 | 14,7 | 56,3 | 56,7 |
| 1 000 à 2 999 sites | 28 | 1 449 | 67,3 | 31,3 | 129,3 | 43,1 |
| 3 000 à 10 000 sites | 33 | 5 865 | 222,6 | 85,1 | 749,0 | 38,7 |

Droites des moindres carrés sur ces seules familles : **K5 : 6,9 ms + 7,4 µs par site ; K10 : 5,5 ms + 42,1 µs par
site** (48 fils). Sous un millier de sites, le coût fixe de la v11 à 48 fils (environ 5 à 7 ms : réserve, pool, étages
vides) domine.

## Familles synthétiques : la v11 s'effondre sur les dégénérescences

Temps chaud (ms) et, entre crochets, part de l'étage forêt dans la dernière passe (lignes brutes publiées) :

| Famille | K | sites : chaud [forêt] |
| --- | ---: | --- |
| réseau (`synth_lattice`) | 5 | 100 : 47,6 [92 %] ; 300 : 169 [96 %] ; 1 000 : 684 [99 %] ; 3 000 : 2 273 [99 %] ; 10 000 : 8 436 [99 %] |
| réseau | 10 | 100 : 138 [94 %] ; 300 : 637 [97 %] ; 1 000 : 2 932 [98 %] ; 3 000 : 10 516 [99 %] ; 10 000 : **expirée** (40,7 s par passe, 40,0 s d'étage forêt) |
| sphère (`synth_sphere`) | 5 | 100 : 18,1 [10 %] ; 300 : 83,3 [3 %] ; 1 000 : 171 [2 %] ; 3 000 et 10 000 : **refus** `unsupported_degeneracy` / `wide_leaf` |
| sphère | 10 | 100 : 87 [6 %] ; 300 : 576 [1 %] ; 1 000 : 1 365 [1 %] ; 3 000 et 10 000 : **refus** `wide_leaf` |
| uniforme | 5 | 100 : 6,2 ; 1 000 : 19,1 ; 10 000 : 144 [47 %] |
| uniforme | 10 | 100 : 12,5 ; 1 000 : 107 ; 10 000 : 1 382 [76 %] |
| huit amas, dalle, droite | 5 et 10 | entre 3,3 ms (droite, 100 sites) et 1 273 ms (amas, 10 000 sites, K10) ; table complète par `analyse_p.py` |

**Lecture.** (1) Sur le réseau entier (coquilles cosphériques massives), l'étage forêt de la v11 fait 92 à 99 % du temps,
pour 0,5 à 0,8 ms par site à K5 et 1,4 à 4,1 ms par site à K10, environ cent fois le coût par site des morceaux de
trames (exposant apparent de 1,1 à 1,4 entre deux tailles) : c'est le défaut de la dérive de l'étage forêt mesurée sur
ETH3D par `MES-E` (session `t2e2`), en bien pire. (2) Sur la sphère (tous les sites cosphériques), la
v11 refuse dès 3 000 sites une feuille trop large (`wide_leaf`) et, sous ce seuil, paie l'étage domaine. (3) Un nuage
volumique (uniforme) coûte à K10 quatre fois un morceau de trame de même taille (1,38 s contre 0,32 s à 10 000 sites) :
la surface du LiDAR est le régime visé, le volume en est le pire cas raisonnable.

**Conséquences pour la v12.** Le régime des petits nuages a deux exigences distinctes : un coût fixe bien plus bas
que 5 à 7 ms (voie CPU à petit nombre de fils sous un seuil à fixer quand la voie appareil sera mesurée, T1-b), et la
**tenue sur les dégénérescences** : forêt sans lots linéaire sur les plateaux (`D-F1`, `LEM-T4`, union par taille) et
aucune feuille refusée pour largeur (feuilles larges sur l'hôte, contrat du catalogue § 9). Les nuages `synth_lattice`
et `synth_sphere` deviennent des cas de régression du chemin produit de la v12, à toutes les tailles.

## Ce que cette session n'établit pas

Ni la v12 (absente de cette mesure), ni le régime à un fil (le plan n'a joué que 48 fils : le seuil CPU d'un petit
nuage reste à mesurer, à 1, 4 et 48 fils), ni le coût d'une Session résidente (chaque prise paie un processus neuf ;
« chaud » désigne ici les passes 2 à 4 d'un même processus). GCP utilisé pour cette seule session, arrêt certifié.
