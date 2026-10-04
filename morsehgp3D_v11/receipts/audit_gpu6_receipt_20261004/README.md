Contrelecture bornée du reçu **public c645b1aabd4c42dc2b68522a6b3616bec7981a80**, indépendante des capsules historiques closes. Les51copies Git sont exclusivement texte : lecteur, README, JSON/CSV des sessions et documentation catalogue. Les archives sources/résultats et rapports profiler binaires ne sont ni présents ni ouverts. Le README et les tableaux primaires sont relus ; aucune analyse exhaustive Nsight supplémentaire.

Après inspection, le lecteur public n'utilise que json/Path/sys et les fichiers locaux. Deux exécutions sur ces copies : `python3 -B -S source/morsehgp3D_v11/receipts/developpement_20261004/gpu_g4/check.py` et la même commande avec `-O`. Résultat identique : **recu_gpu_verdict conforme controles553**, stderr vide. Cela confirme les valeurs explicitement vérifiées par BEST/PARTS/NCU, les statuts déclarés, la cible d'arrêt et les hashes de prise conservés dans les JSON. Cela ne recertifie pas les dumps binaires, toutes les métriques Nsight ou les codes natifs.

GPU6 déclare source/worker **22a6af6aa6c57302b3eac5adcb7f2b52c0f1e0b4**, `completed`, `stopped`, arrêt ciblé certifié. SHA binaire des deux rapports : **8397f3f1ce5e5a2bbcd9a40874dd95349cb3af7a3dbd16c94ccd0a6f1d4b4c62**. Le plan local a le SHA déclaré. La source22 descend de4ec33e3d7, le commit d'enregistrements compacts en rangs locaux et cases2Kio : **cette compression a bien été mesurée**, et ne doit plus être présentée comme expérience future à faire. Les72prises froides et12processus chauds de GPU6 sont conformes ; toutes les séquences1..P ont ici été revérifiées, avec statuts `ok` et derniers hashes conformes. L'égalité de registres reste le contrôle du banc producteur : chaque rapport stocke le registre de référence, pas tous les registres bruts.

Table calculée GPU6 seulement, mur FULL en ms, colonnes CPU / GPU. La table publique «Meilleurs temps» combine explicitement plusieurs sessions, choisissant souvent S5 pour le chaud ; **ses valeurs chaudes sont les meilleures passes**, pas les médianes. Cette distinction est correcte et nécessaire.

| K/feuille | Trame | Médiane froide | Meilleure passe chaude | Médiane chaude2..P |
| --- | --- | ---: | ---: | ---: |
| 5/16 | ng00 | 368.7 / 434.8 | 342.7 / 398.0 | 346.8 / 419.6 |
| 5/16 | ng01 | 289.9 / 392.4 | 288.0 / 325.3 | 302.5 / 347.3 |
| 5/16 | ng02 | 374.3 / 423.4 | 373.7 / 387.1 | 375.6 / 390.6 |
| 10/24 | ng00 | 2513.6 / 2398.6 | 2471.8 / 2363.9 | 2479.0 / 2383.1 |
| 10/24 | ng01 | 1854.9 / 1843.4 | 1838.7 / 1793.4 | 1844.3 / 1808.0 |
| 10/24 | ng02 | 2108.4 / 2009.4 | 2077.3 / 2010.0 | 2092.4 / 2035.4 |

Les tableaux publics sont compatibles avec les rapports. Sur ng00, S5→S6, le retour K5 baisse5,6→2,3ms et le Level15,2→5,6ms ; le count monte32,8→37,6ms et le fill13,7→17,6ms. À K10/S6 : count218,1ms, fill112,9ms, retour7,8ms, Level21,4ms. Les médianes de postes peuvent appartenir à des prises différentes, et les intervalles peuvent se chevaucher : aucun total ni attribution causale isolée n'est déduit. Les recherches linéaires en rangs locaux sont une explication du code, pas une ablation isolée de leur coût ici. Le retrait de ces recherches par des rangs intrinsèques et une feuille coopérative est une piste cohérente avec le diagnostic publié ; ce reçu n'en démontre pas le gain.

K5 reste favorable au CPU dans les médianes GPU6 ; à K10, les forêts restent autour1,17–1,64s à chaud et dominent le mur. Aucun100ms, transfertu24, tous-limites ou qualification GPU générale n'est acquis. Le README public réserve lui-même les portes numériques extrêmes. Le lecteur553 est favorable à ce reçu de mesures, sans remplacer ces portes.

`SOURCE.json` et `AFTER.json` ancrent chaque copie Git ; `DERIVED.json` conserve les recomputations limitées GPU6. L'inventaire SHA exclut seulement sa propre racine. Aucun reçu clos antérieur ni fichier du développeur/audit actif n'est modifié.
