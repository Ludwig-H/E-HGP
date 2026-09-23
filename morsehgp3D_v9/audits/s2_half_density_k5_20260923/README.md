# S2 CPU K5 : demi-scènes physiques × densité, LiDAR brut 08/000000

23 septembre 2026. Reçu d'audit exploratoire, **neuf entrées, dix-huit
exécutions appariées** moteur/lot CPU, sans CUDA ni GCP. Source S2
`a6d81f9ce`, binaire Release `mhgp9_tower_probe_v17` SHA-256
`076312fb9502dee5a7f0a5fbe549b68c942c511892b4c0ba5583770b511758d7`.
Le [reçu de build versionné](../q34_batch_density_quarter_20260923/receipt.json)
(SHA-256 `75a504bf824c921b7ed73c09da9d65f7431c869d33d24c7de784c648af48f536`)
épingle `Release`, `MHGP9_ENABLE_CUDA=OFF` et les trois sources produit,
qui correspondent exactement à `git show a6d81f9ce` : `wspd_q34.cpp`
`1e34498e40f5a7a40c0e22ad14bb14bbaf23347494e0313553f51910d50c7892`,
`tower_chain.cpp` `4dcd5cdeab4887abf5d2c4a6942ddbc97e79d88e968d4c4e894e34f4dfb02748`,
`tower_probe.cpp` `d03445006f439643dd08aac050544f0881fd596c098993e149c086ede4b1d50b`.
Entre le commit local testé `803216ba2c8d42f0d8f454cc0909a6c220ff3312`
de ce reçu et sa publication `a6d81f9ce`, le diff est vide sur
`morsehgp3D_v9/src/`, `cmake/`, `CMakeLists.txt` et `bench/tower_probe.cpp`.
Le SHA du binaire est vérifié avant/après chaque
processus. Les entrées proviennent du [manifeste physique](../lidar_raw_physical_scaling_20260923/MANIFEST.json)
SHA-256 `6e64125abddbfcd589ba61cf747e493e455adec4beeaea81c2739ce98fe16095` :
123 389 retours de la trame **brute avec sol**, plans `x=0` par le capteur
sur les float32 originaux, coordonnées HGP de la grille **commune** 1 mm/u18.
Les sélections globales par hash 1/4 ⊂ 1/2 ⊂ 1 sont intersectées avec
chaque demi ; les mêmes retours gardent les mêmes coordonnées. K5/s8/W8,
`--static=8`, `nice 19`, seul `q34_batch_filter` varie entre les deux voies.

Toutes les sorties ont `status=complete_relative`. À chaque entrée,
`status`, `reason`, `input`, `generator`, `catalogue`, `tower_work`, les cinq
ordres et le digest de tour sont égaux moteur/lot ; les compteurs du cœur
le sont aussi. Cela recoupe les émissions relatives au catalogue, **sans
certifier les clés jamais émises ni comparer les clés une à une**. Le
compteur `F=core_sites` est le nombre de formes réellement préparées et
écrites par les charges du cœur, extrémités incluses : les dix-huit stdout
vérifient `F = dead_core_form_sites + 2 × dead_core_loads`.

| Densité | Sites plein / demi x<0 / demi x≥0 | F plein / demi x<0 / demi x≥0 | `R_H=ΣF(demis)/F(plein)` | `Σcharges/charges(plein)` |
| --- | ---: | ---: | ---: | ---: |
| 1/4 | 30 847 / 15 437 / 15 410 | 37 009 904 / 8 858 072 / 12 113 778 | **0,566655** | 0,968151 |
| 1/2 | 61 694 / 30 644 / 31 050 | 128 852 821 / 26 998 706 / 29 104 417 | **0,435405** | 0,970440 |
| Entière | 123 389 / 61 045 / 62 344 | 559 661 741 / 95 703 102 / 80 777 133 | **0,315334** | 0,969119 |

Le repère homogène `B_H=Σ(n_demi/n_plein)²` vaut respectivement
0,500000 / 0,500022 / 0,500055. **La part des formes dans les demis
diminue avec la densité malgré une part stable des charges** : sur cette
trame et ce port, réunir les deux demis accroît la masse *par cœur*. Les
tours des demis sont recalculées sur des nuages différents ; leur somme
ne constitue pas la tour du plein, et ce rapport seul n'identifie pas
quelles arêtes causent le surcoût. Le [reçu par arête sur les
quarts](../edge_matched_core_20260923/README.md) analyse cet autre niveau.

Les pentes finies de `F` aux deux liens 1/4→1/2 puis 1/2→entière sont
1,799741 / **2,118806** pour le plein, 1,625360 / 1,836200 pour le demi
`x<0`, 1,251169 / 1,464424 pour le demi `x≥0`. Un calcul indépendant sur
`PANEL.json` emploie des fractions exactes pour `R_H` :
`10485925/18504952`, `56103123/128852821`, `176480235/559661741` ;
les logarithmes `Decimal` à 50 chiffres retrouvent les six pentes du
lecteur. Les **six champs déterministes** `sites`, `core_sites`,
`dead_core_loads`, `catalogue_balls`, `expanded_pairs` et
`dead_core_uniform_tests` sont identiques sur **9/9** entrées au reçu v12
historique, ce que le lecteur recontrôle. S2 CPU n'a donc pas déplacé ces
masses sur cette matrice. Les CPU·s du lot donnent `R_H` 0,886 / 0,886 /
0,889, mais les chronos sont descriptifs : hôte partagé, une exécution
par cas, tests du constructeur concurrents. Les murs présentent une forte
contension et ne servent pas à inférer un effet causal. Aucune borne
sous-quadratique, qualification K10, G4 ou contrat 1 s n'en découle.

`ATTEMPTS.jsonl` garde les commandes, heures, SHA, compteurs et tout essai
avant validation ; `read_panel.py` reconstruit les neuf cas depuis les stdout,
recalcule le résumé et le compare **octet pour octet** au `SUMMARY.json`
archivé, sans l'écraser. Les 18 stdout et 18 stderr sont archivés ;
aucune grosse trace binaire. Pour reconstruire les entrées depuis les
sources v8 versionnées puis relire ce reçu sur une autre machine :

```sh
repo=/workspaces/E-HGP
inputs=/tmp/mhgp9-s2-half-density-inputs
receipt=/chemin/vers/ce-reçu
python3 -B morsehgp3D_v9/audits/lidar_raw_physical_scaling_20260923/generate.py --repo "$repo" --out "$inputs"
python3 -B "$receipt/read_panel.py" --repo "$repo" --inputs "$inputs" --receipt "$receipt"
```

Le lecteur n'a pas besoin du binaire hors ligne ; avec `--binary <chemin>`,
il vérifie aussi son SHA LIVE. Les chemins absolus de la capture gardés
dans `ATTEMPTS.jsonl` sont contrôlés par rôle, nom et hash de payload,
pas supposés exister lors d'une relecture. Pour un rejeu **neuf** avec le
même binaire épinglé et un dossier de sortie vide :

```sh
nouveau_receipt=/tmp/mhgp9-s2-half-density-replay
binary=/chemin/vers/mhgp9_tower_probe
python3 -B "$receipt/replay.py" --repo "$repo" --inputs "$inputs" --receipt "$nouveau_receipt" --binary "$binary"
python3 -B "$receipt/read_panel.py" --repo "$repo" --inputs "$inputs" --receipt "$nouveau_receipt" --binary "$binary" --write-summary
python3 -B "$receipt/read_panel.py" --repo "$repo" --inputs "$inputs" --receipt "$nouveau_receipt" --binary "$binary"
```

La capture initiale utilisait un lanceur à chemins `/tmp` fixes ; le
`replay.py` fourni ne change que ses paramètres de chemins, pas les
commandes HGP enregistrées. Un binaire reconstruit avec un autre SHA exige
un reçu distinct. `--write-summary` crée le résumé une fois et refuse
de remplacer un résumé existant. Par défaut, `read_panel.py` refuse
un résumé absent ou altéré, une matrice 3×3 incomplète et tout essai en
échec ; son mode `--partial` ne sert qu'au suivi pendant une capture et
ne lit ni n'écrit le résumé. La relecture LIVE avec le binaire, puis hors ligne sans lui,
produit le même `SUMMARY.json` SHA-256
`702d33dbe304e09be2664c3e6a82723291def397ccb20da5531ec6cc900097b2`.
