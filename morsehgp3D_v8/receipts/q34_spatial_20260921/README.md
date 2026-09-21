# q3/q4 sur les scènes et leurs découpes spatiales

21 septembre2026. Même moteur C++ que34R2, nouveau protocole de mesure.
[Méthode](../../docs/Q34_MESURES_SPATIALES_20260921.md),
[nouveau contrat principal](../../docs/CONTRAT_TRAMES_SEMANTICKITTI_20260921.md).
Le flux de candidats mesuré n'est **pas la tour FULL**. Aucune ancienne
mesure sur préfixes n'est requalifiée en scène entière.

## Portes locales closes

- `preflight/q34_spatial_gate_stsvs21b` : Python normal,14appels natifs,
  238records comparés à l'oracle rationnel indépendant.
- `qualification/q34_spatial_gate_8dl8cvcv` : même porte sous `python -O`,
  mêmes comptes et sorties.

Chaque porte vérifie29cas d'analyse, les sept nuages et deux configurations
Individual/W1 et LiveOnly/W2, K5/s8. L'oracle énumère688triangles et1964
tétraèdres ;263boules distinctes et3648puissances rationnelles sont vérifiées.
Les54tétraèdres positifs donnent50groupes q4 canoniques : le regroupement
ne repose pas seulement sur des tétraèdres isolés. Les fermetures des
sources, des binaires et de tous les artefacts figurent dans les
`COMPLETION.json`. Ce sont des vérifications du nouveau raccord, pas une
nouvelle exécution des96CTests34 ni une qualification GPU.

Autotests du nouveau lecteur :59corruptions rejetées, normal et−O,
sur `preflight/q34_spatial_gate_stsvs21b/native/spatial_ojvzkl_t` :

```bash
python morsehgp3D_v8/bench/run_q34_spatial.py selftest --path morsehgp3D_v8/receipts/q34_spatial_20260921/preflight/q34_spatial_gate_stsvs21b/native/spatial_ojvzkl_t --compact
python -O morsehgp3D_v8/bench/run_q34_spatial.py selftest --path morsehgp3D_v8/receipts/q34_spatial_20260921/preflight/q34_spatial_gate_stsvs21b/native/spatial_ojvzkl_t --compact
```

## Référence locale close

`performance/spatial_9kscvyt0` : sept morceaux du scan08/000000, K5/s8,
Local28/LiveOnly, témoins rectangle-pair/affine, census q3 boxes, W4.
Les sept commandes sont achevées, sources216 et protocole inchangés.
Lectures `--check-live`, analyse et59corruptions de reçus passent en
normal/−O. Les deux analyses sont identiques octet pour octet :
[croissance complète](GROWTH_SCAN0_K5_S8.json), SHA256
`93b027d078bccfd946d6922b3b0516b7d1be72436a5a77706e946b0ed393f8a6`.

Temps de pipeline incluant nuage/index/front/census/collecte, sur machine
partagée ; une observation, pas une médiane ni un gain stable. Les trois
colonnes de tests sont en milliards, séparées car leur coût unitaire diffère.

| Morceau | Sites | Pipeline CPU (s) | Bornes préparées q3 | Bornes blocs atlas q4 | Tests points atlas q4 |
| --- | ---: | ---: | ---: | ---: | ---: |
| Quart x−y− | 29 128 | 132,533 | 1,912 | 0,507 | 1,501 |
| Quart x−y+ | 30 061 | 271,504 | 1,696 | 0,839 | 2,081 |
| Quart x+y− | 30 027 | 68,523 | 0,529 | 0,210 | 0,673 |
| Quart x+y+ | 29 926 | 110,892 | 0,891 | 0,362 | 1,179 |
| Moitié x− | 59 189 | 283,295 | 3,944 | 1,272 | 3,526 |
| Moitié x+ | 59 953 | 135,786 | 1,594 | 0,607 | 1,928 |
| Trame entière | 119 142 | **383,311** | **8,889** | **3,010** | **7,717** |

La trame entière émet1252577candidats q3 et190405q4, avec4566620IDs
de coquille. Le temps CPU cumulé est1417,521s pour383,319s mur collecteur,
soit3,70CPU occupés en moyenne sur les quatre workers demandés. Les
quarts donnent seulement1,12 à1,74CPU moyens ; cela n'est pas une durée
par worker ni une séparation de l'effet d'ordonnancement et de la charge
extérieure. Un contrôle ponctuel du deuxième quart n'observait plus
qu'un worker de calcul actif après le départ des trois autres.

### Croissance : les six comparaisons, pas seulement les favorables

Pour les trois postes du tableau, les quatre relations moitié/quart
sont sous le seuil quadratique. Ce n'est **pas** vrai de toutes les
relations de la campagne : trame entière→moitié x+ a un rapport d'effectifs
1,9873, donc un seuil quadratique d'environ3,949. Les bornes q3 font×5,576
(exposant2,502), les bornes de blocs q4×4,962 (2,332), leurs tests de points
×4,004 (2,020). La population cumulée des covers fait×5,384 et les graines
q3×5,420. Ces coûts internes restent à réduire.

Sur les six comparaisons, les exposants des visites du front restent entre
0,699 et1,465, ceux des arêtes développées entre0,867 et1,859, ceux des
sorties q3 entre0,792 et1,256 et q4 entre0,463 et1,877. Les coûts élevés ne
s'expliquent donc pas simplement par le nombre de sorties. Tous les autres
postes et dépassements, y compris les petites branches, sont conservés
dans le JSON ; ne pas additionner ses compteurs imbriqués.

La géométrie des deux moitiés est très différente : ces exposants sont
des diagnostics de cette scène, pas une preuve asymptotique. Inversement,
cette asymétrie n'autorise pas à supprimer la moitié défavorable du rapport.
Le sous-quadratique de toute la chaîne sur les régimes visés reste ouvert.

## Pilote G4 clos — quatre commandes terminées

Le [plan G4](GCP_PLAN.json) a été exécuté intégralement : premier quart
puis trames entières08/000000,08/000100 et08/000200, K5/s8,
Local28/LiveOnly, W48. **CPU sur G4, pas GPU ni FULL**. La VM expose
48CPU logiques,24cœurs/2SMT, AMD EPYC9B45. Chaque cas a lancé48workers.
Les durées ci-dessous sont une observation par cas, pas des médianes.
Le pipeline inclut nuage/index/front/census/collecte, exclut le chargement
disque et la sérialisation finale, publiés séparément dans les reçus.

| Entrée | Sites | Pipeline (s) | CPU cumulé (s) | CPU logiques occupés en moyenne | q3 émis | q4 émis |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Quart scan0 x−y− | 29 128 | 45,594 | 159,02 | 3,49 | 316 274 | 52 870 |
| Trame08/000000 | 119 142 | **165,214** | 691,65 | **4,19** | 1 252 577 | 190 405 |
| Trame08/000100 | 119 942 | **34,319** | 382,00 | **11,13** | 1 212 999 | 164 753 |
| Trame08/000200 | 120 725 | **505,479** | 973,37 | **1,93** | 1 369 563 | 209 223 |

Le temps CPU et le temps mur collecteur viennent de GNUtime, sur le
processus entier avec tous ses threads ; ce ne sont pas des durées par
worker. Les trois trames paient respectivement8,889/4,497/10,431Md bornes
préparées q3 et10,727/5,028/18,246Md tests de partition q4 (blocs+points).
Ces deux familles ont des coûts unitaires différents. Le moteur construit
244,805/122,737/273,138M boules q3, pour seulement1,253/1,213/1,370M
émissions q3 : le travail avant rejet reste central.

Le plan Coarse produit768/769/768jobs pour les trames et les achève tous.
Les sous-arbres attribués et leurs arêtes coûteuses restent indivisibles ;
il n'y a pas de redistribution intérieure q3/q4. La faible occupation
confirme un défaut de parallélisation utile, sans isoler précisément le
coût de chaque arête ni attribuer toute attente à une cause unique. Le
rapport local/G4 du pipeline vaut2,907 pour le quart et2,320 pour la trame0,
mais compare des machines différentes et un local partagé : **pas** un
speedup W4→W48 isolé. Le contrat1s/100ms demeure non atteint/non qualifié,
même pour ce flux incomplet ; K10 et s10/s12 restent à mesurer sur les
trames entières, et d'autres séquences sont nécessaires avant qualification.

### Fermeture, égalité et sécurité

[Relecture normale](gcp_r1/READBACK.json) et
[relecture−O](gcp_r1_optimized/READBACK.json) : `validated_complete`,
quatre cas terminés, quatre portes natives PASS, sources et dépendances
compilées fermées. Les rapports sont identiques hors hash du conteneur
gzip, dont les inventaires et contenus hachés sont identiques. Sur les
deux entrées communes local/G4, sorties et tous les comptes géométriques
coïncident ; seuls deux pics de capacité privée sont neutralisés dans
cette comparaison, explicitement nommés dans le reçu. Cela n'est pas un
oracle exhaustif des grandes scènes.

L'adaptateur CPU34 explicite passe9tests locaux en normal/−O dans
`gcp-migration/receipts/q34_spatial_20260921/local_zzork064` ; l'ancien
adaptateur31 et les scripts de garde ne sont pas modifiés. Le lecteur
posthoc passe5tests dans chaque mode. Une contrelecture a détecté une
liste d'archivage trop permissive avant sa première utilisation : elle
a été remplacée par l'inventaire exact des commandes/pièces/marqueurs,
avec tests de fichiers parasites et de chemins de clés. Aucun secret
n'a été observé ni archivé. Les236fichiers du dossier hôte sont archivés,
jamais son parent contenant la clé SSH. Hash final du lecteur :
`0502d42ab908b4956398b2cdd694f6ffaa6da1517b39a45341bf4bdbfdc70db3`.
Le préflight de permission de clé refusé avant démarrage est conservé
dans [son reçu](GCP_PREFLIGHT_KEY_MODE.json), sans donnée privée.

Une seule session SPOT, budget utile900s compilation comprise, quatre
cas achevés sans interruption ni dépassement. Arrêts invité30min et
GCE3600s vérifiés. La cible
`devpod-gpu-exploration/us-central1-b/ehgp-v7-4fa0e0789a7d5bb06b787d35`,
génération `2026-09-21T04:53:28.400-07:00`, a été arrêtée et certifiée
**TERMINATED à12:09:34UTC le21 septembre2026**. Le contrôle de fermeture
ne détectait aucune autre VM étiquetée active. Les archives comprennent
les sorties brutes de démarrage/arrêt ; aucun coût monétaire n'est déduit
d'une durée seule.
