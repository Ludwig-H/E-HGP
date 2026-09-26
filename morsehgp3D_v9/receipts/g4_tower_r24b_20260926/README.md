# Reçu G4 R24-B — séparateur de recouvrement et choix de `s`

26 septembre 2026, 13 h 03–13 h 10 UTC. Cadre :
`phase=exploration_v9_hors_registre`, `backend=cuda_g4`,
`profile=quantized_u18_input_only`, `public_status=not_claimed`.

**GCP utilisé, puis arrêté.** Une seule session sur la G4 SPOT fixe
`devpod-gpu-exploration/us-central1-b/ehgp-v7-4fa0e0789a7d5bb06b787d35`,
génération `2026-09-26T06:03:17.170-07:00`. Le contrôleur a certifié les
deux coupe-circuits (GCE `STOP` à 3 600 s, arrêt invité à 40 min), puis son
arrêt ciblé. Une [relecture GCE indépendante](host/after_stop.json) après la session donne
`TERMINATED`, dernier arrêt `2026-09-26T06:10:03.052-07:00`.

Le [plan](../../audits/b_g4_r24_20260926/plan.json) et le
[lecteur reproductible](../../audits/b_g4_r24_20260926/analyze_receipt.py)
accompagnent les [sorties brutes](vm/) et la [synthèse](SUMMARY.json).
Paquet construit depuis `ebf7c342d`, protocole v28 commité inchangé ;
snapshot SHA-256 `cfb0b3be…`, worker `b1d9c65d…`. L'autotest local du
protocole passe 28/28 en mode normal et 28/28 sous `-O` avant la séance.

Le reçu hôte et celui du worker sont `completed` ; **36/36 cas** sont
`complete_relative`, dont **28 GPU** et 8 jumeaux moteur ; **28/28
comparaisons croisées** sont égales. Les condensés de tour, catalogue et
présentations, ainsi que les volumes de boules, coïncident pour tous les
bras d'une même trame et d'un même K, y compris `s=8/10/12`. Les sources,
dépendances compilées et binaire sont déclarés stables. Les fichiers
publiés sont inventoriés dans `SHA256SUMS` ; le lecteur reconstruit les
tables depuis les `vm/probe_N.stdout`, pas depuis ce texte.

## Recouvrement et queue E4

Sur les trames entières `00` sans sol (39 885 sites) et `b00` brute
(123 389 sites), grille entière 1 mm, W48, s8, deux répétitions dans
l'ordre A–B–C puis C–B–A : A = recouvrement statique + queue E4, B =
recouvrement statique sans queue E4, C = chemin sans recouvrement.
Médianes de la **chaîne interne FULL** en millisecondes :

| Trame / K | A | B | C | B−A | C−B |
| --- | ---: | ---: | ---: | ---: | ---: |
| 00 / K5 | 924,1 | 942,6 | 962,3 | +18,4 | +19,7 |
| 00 / K10 | 2 962,3 | 3 088,4 | 3 319,8 | +126,2 | +231,4 |
| b00 / K5 | 2 005,4 | 2 029,8 | 2 071,7 | +24,4 | +41,9 |
| b00 / K10 | 6 049,2 | 6 265,7 | 6 727,0 | +216,5 | +461,3 |

À K10, les deux différences appariées B−A ont le même signe et restent
importantes : +122/+130 ms sans sol, +209/+224 ms brute. À K5, elles sont
+36,5/+0,4 ms et +12,7/+36,1 ms ; le meilleur bras A brut varie lui-même
de 62,3 ms entre deux passages. La petite différence K5 ne constitue donc
pas un gain robuste établi. A↔B isole la queue E4 avec recouvrement actif.
B↔C change aussi l'ordonnancement des phases, l'ordre de la phase 0 et les
fils actifs : il ne mesure pas à lui seul la contention de A ni un gain
garanti d'un hypothétique budget global de fils.

## Séparation WSPD `s=8/10/12` sur GPU

K5, bras GPU complet avec jumeau moteur, un passage par `s` :

| Trame | `s` | GPU chaîne (ms) | Moteur chaîne (ms) | Front q3/q4 (ms) | Paires étendues q3/q4 (M) |
| --- | ---: | ---: | ---: | ---: | ---: |
| 00 | 8 | 916,3 | 2 744,3 | 98,6 | 23,69 |
| 00 | 10 | 998,5 | 2 761,8 | 119,7 | 18,26 |
| 00 | 12 | 1 004,1 | 2 842,5 | 137,6 | 14,76 |
| b00 | 8 | 2 036,6 | 5 615,3 | 297,8 | 22,03 |
| b00 | 10 | 2 063,4 | 5 718,2 | 334,7 | 14,52 |
| b00 | 12 | 2 107,2 | 5 920,3 | 374,4 | 11,26 |

Augmenter `s` réduit ici les paires étendues mais accroît les rectangles
du front et son coût ; `s=8` est le plus rapide **sur ces deux entrées dans
cette session**. Les temps `s=10/12` n'ont qu'un passage non entrelacé :
leur classement fin n'est pas une conclusion générale. L'identité des
objets, elle, est vérifiée directement sur GPU aux trois valeurs.

## Portée contractuelle

La meilleure chaîne interne K5 sans sol vaut 916,3 ms (tour seule :
282,7 ms), mais son mur externe vaut 1,726 s. Le seuil **100 ms FULL**
n'est pas atteint, et `contract_certified=false`. K10 sans sol vaut au
mieux 2,956 s dans ces répétitions ; K5 brut reste au-dessus de 1,97 s.
Sur le passage 00/K5 à 916,3 ms, q3/q4 prend 481,9 ms, le recensement
100,3 ms et la construction de la tour 282,7 ms : couper seulement la
queue E4 ne peut pas rapprocher la chaîne de 100 ms. Le mur externe
comprend en outre ouverture de session, vérifications et condensés ; il
ne doit pas être présenté comme une simple durée de noyau GPU.
Seules deux versions de la même trame 08/000000 (avec/sans masque de sol)
sont ici testées. Pas d'autres séquences, de profil float32 brut, de
quantification alternative, ni de preuve de croissance sous-quadratique.
L'exactitude testée est relative au catalogue croisé et aux épingles du
protocole ; ce reçu ne promeut aucun statut public d'exactitude.
