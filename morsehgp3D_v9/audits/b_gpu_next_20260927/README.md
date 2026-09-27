# Prochaine expérience G4 : coût du noyau diamétral, processus résident

27 septembre 2026. Audit du produit `ddf4776d754a8db59a1333e11d56b39d8cb6f51a`.
Cadre `exploration_v9_hors_registre`, `quantized_u18_input_only`,
`public_status=not_claimed`. Ce dossier prépare une expérience, pas un gain.

## Pourquoi cette expérience

Les 179 probes archivés portant `q34_gpu_certificates=true` gardent tous
`q34_dead_core=true` à ce commit. L'ablation R6 du noyau diamétral était CPU ;
son bénéfice ne démontre pas celui de S3 CUDA. La bascule existe déjà dans
`src/chain/tower_chain.cpp` et `src/gpu/certificate.hpp::certify_edge` : aucun
changement moteur ou protocole nécessaire. Le témoin OFF ne supprime pas
la preuve sur le cover : il évite seulement le préfiltre diamétral.

Hypothèse préalable : le noyau reste probablement utile, car il ferme
1 143 235 des 2 043 612 arêtes de S2 sur 00/K5. Mais son coût comprend
355 620 051 sites chargés et 60 575 576 cellules, en plus de la preuve sur
le cover. La bonne décision compare S3 **plus les voies survivantes**, puis
la chaîne, pas le nombre de rejets. Aucun signe du delta n'est promis.
Une régression OFF tranche utilement cette question sans développer une
nouvelle voie morte. Ne pas imposer la monotonie des masques de certificats
entre deux stratégies de preuve ; imposer l'égalité des objets finaux.

La dernière campagne q3 avait exclusivement `frames=1`. Le mode v30
`--frames=4` rejoue la chaîne entière sur la même trame, en gardant la
session CUDA ; ce n'est ni quatre scènes, ni quatre processus indépendants.
La création du contexte était déjà hors `chain_total` dans les deux bras :
ne pas en déduire artificiellement 120 ms de gain chaud dans la chaîne.

## Plan fermé : six processus, aucun autre levier ablaté

`plan.json` a passé `tower_worker_v9.validate_plan` et la fabrication du
snapshot officiel. Entrée 08/000000 sans sol, 39 885 sites, grille 1 mm,
toute la tour explicite K1..5, s8, 48 workers et 48 threads de tour.
Payload q3 et passe fusionnée L15 restent OFF dans tous les cas.

| Cas | Producteur | Noyau diamétral | Répétition | Passages |
| --- | --- | --- | --- | --- |
| 0 | GPU | ON | 0 | 4 |
| 1 | GPU | OFF | 0 | 4 |
| 2 | GPU | OFF | 1 | 4 |
| 3 | GPU | ON | 1 | 4 |
| 4 | moteur CPU témoin | ON | 0 | 1 |
| 5 | moteur CPU témoin | OFF | 0 | 1 |

Prédictions falsifiables : les trois digests, comptes d'objets et ordres
restent identiques dans les 18 passages ; OFF n'a aucun travail `core_*` ;
à noyau identique, le registre certificat GPU égale celui du témoin moteur.
La validation native vérifie chaque `frames.results`, pas seulement le
premier. Le lecteur supplémentaire compare **directement** le certificat
de chaque GPU OFF au moteur OFF : le comparateur générique groupe sinon
les cas contre la référence ON et n'exige pas cette égalité inter-leviers.

Publier séparément le premier passage, la médiane des trois suivants par
processus, les deux deltas ON/OFF par répétition, les temps des noyaux et
ceux des appels. Le protocole ne publie les registres détaillés que pour
le premier passage ; aucune attribution fine du gain chaud n'en découle.

## Ce que les temps actuels signifient

Autorité : `receipts/g4_q3_payload_20260926/vm/probe_0.stdout` (payload ON,
utilisé ici pour ventiler, **pas** témoin apparié de cette future campagne).

| Poste | Champ JSON | ms |
| --- | --- | ---: |
| Front CPU | `q34_batch.front_ms` | 101,245 |
| Appel filtre S2 | `q34_batch.filter_ms` | 98,560 |
| CUDA S2, noyaux seulement | `q34_batch.filter_kernel_ms` | 64,056 |
| CUDA S2, transferts | `q34_batch.filter_transfer_ms` | 7,334 |
| CUDA S2, total appareil | `q34_batch.device_ms` | 71,390 |
| Appel certificats S3 | `q34_batch.certificate_ms` | 119,592 |
| Noyau S3 | `q34_batch.certificate_kernel_ms` | 91,443 |
| Noyaux voies S4 | `q34_batch.lanes_kernel_ms` | 56,217 |
| Tour FULL | `times_ms.tower` | 289,648 |
| Chaîne entière | `times_ms.chain_total` | 918,275 |

S2 n'est donc **ni 25 ms ni 98 ms de noyau** sur ce reçu. Les noyaux
S2/S3/S4 valent ensemble 211,716 ms en série. La chaleur ou les transferts
seuls ne permettent pas 100 ms ; il faut réduire le travail de ces noyaux
et celui de FULL. Le front compact par vagues reste un prototype CPU,
aucune option actuelle ne le déplace sur GPU. Les plans Pool/bandes ne
remplacent pas gratuitement le travail des voies acceptées ni FULL.

## Paquet et budget

Paquet préparé hors ligne, **aucun GCP utilisé par sa préparation** :
`/workspaces/E-HGP/build/v9-g4-core-snapshot-20260927/PACKAGE.json`.
Snapshot `2ccddb90e0e88bb072f7eef4f4b84ae25f3a148850d57bb03914f2c619221e8c` ;
provenance `commit`, six entrées historiques v8 hachées transportées par
le protocole, seule 00 mesurée, aucun octet KITTI ajouté à la v9.

Commande reproductible, avec une destination **neuve** :

```sh
python3 -B gcp-migration/tower_snapshot_v9.py \
  --commit ddf4776d754a8db59a1333e11d56b39d8cb6f51a \
  --plan morsehgp3D_v9/audits/b_gpu_next_20260927/plan.json \
  --output /workspaces/E-HGP/build/NOUVELLE_DESTINATION
```

Le responsable lance exclusivement `tower_session_v9.py --execute` avec
les chemins et quatre hashes de ce PACKAGE. Un seul SPOT, fermeture
`finally` par le garde officiel puis lecture de la même génération
`TERMINATED`. Objectif opérationnel inférieur à 15 minutes, arrêter par
SIGINT du contrôleur à dix minutes si le résultat n'est pas acquis, puis
vérifier sa fermeture : le protocole inchangé conserve en secours ses
budgets fixes 1500 s utiles / GCE 3600 s / invité 40 min. Il n'existe pas
d'option CLI de budget plus court ; ne pas prétendre avoir abaissé ces
garde-fous. Une expiration ou un échec reste publié sans résultat promu.

## Lecture et publication après fermeture

`readback.py <host ou reçu publié> --snapshot <snapshot.tar.gz>` exige une
capture complète, l'arrêt ciblé certifié, le describe final de la même
génération (option `--after-stop` pour un hôte non publié), puis rejoue
`tower_session_v9.validate_received`. `publish_closed.py` reçoit les quatre
options `--host`, `--package`, `--after-stop`, `--output` ; sa sortie doit
être neuve. Aucune récupération ou capture incomplète n'est promue ici.

Les fonctions de preuve/arrêt et de copie autorisée du précédent audit q3
sont importées avec deux SHA256 fixes. Ni son analyse ON/OFF payload ni
son README chiffré ne sont réutilisés. Aucun de ses fichiers n'est modifié.
Le nouveau résumé est entièrement recalculé ; deux lectures, normal et
`-O`, restent requises sur la capture réelle avant publication.

Portes hors ligne : `python3 -B .../test_tools.py` puis `python3 -O -B
.../test_tools.py`, **11 tests PASS dans chaque mode**. Les fixtures de
comparaison sont synthétiques et ne sont pas des preuves GPU : elles
tuent notamment une dérive du registre GPU OFF, une dérive du témoin OFF,
un objet différent et une erreur limitée au troisième passage chaud.
La validation d'arrêt refuse une capture ouverte ou en échec. Le plan et
le snapshot sont préparés ; aucun résultat de nouvelle session n'est
affirmé par ces tests.
