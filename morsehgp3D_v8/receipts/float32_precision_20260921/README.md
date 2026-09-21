# Entrée float32 / grille1mm et primitive q2 exacte

21 septembre2026. Qualification CPU locale séparée du moteur u16,
`exploration_v8_hors_registre`, `public_status=not_claimed`.
**GCP non utilisé. Aucun nouveau chrono moteur, FULL ou contrat G4.**
Voir la [note d'implémentation et les limites](../../docs/PRECISION_FLOAT32_ET_GRILLE_20260921.md).

## Captures et échec conservé

| Capture | Verdict | Portée |
|---|---|---|
| `release/precision_8wdp64v4` | PASS initial,24commandes | GCC, six préparations entières ; ancien lanceur |
| `sanitize/precision_f9d1henc` | FAILED,2commandes | échec de lien avant toute gate native |
| `release_r2/precision_a1drpf9i` | PASS,24commandes | qualification corrigée faisant autorité |
| `sanitize_r2/precision_9a18o3i7` | PASS,6commandes | Clang ASan/UBSan avec détection des fuites explicite |

Le premier lanceur résolvait le lien symbolique du compilateur avant
invocation : `clang++` devenait `clang`, perdait le mode de lien C++ et
échouait avec des références manquantes. R2 conserve le chemin du driver
et continue à hacher son exécutable. Aucun changement du prédicat, du
préparateur ou des fixtures entre ces essais ; aucune désactivation
d'avertissement ou de sanitizer. Les diagnostics et instantanés R1
restent archivés ; ne pas présenter l'échec comme un test réussi.
Le lecteur courant est lié aux sources R2 ; l'ancien reçu Release reste
historique, pas un reçu relu avec les sources corrigées.

Builds désormais épinglés, jamais à écraser :

- `build/v8_float32_precision_20260921` ;
- `build/v8_float32_precision_sanitize_20260921` (échec) ;
- `build/v8_float32_precision_r2_20260921` ;
- `build/v8_float32_precision_sanitize_r2_20260921`.

Les deux qualifications R2 lancent les gates en Python normal et `-O`.
Chaque gate passe15tests de préparation,3923requêtes q2 contre Fraction
et49contrôles natifs. Les cinq mutations sont des résultats altérés,
pas des mutants compilés. Les deux compilateurs donnent le même flux
natif et les mêmes registres :916intérieurs,1208contacts,1799extérieurs ;
2715décisions filtrées et1208replis exacts. Les quatre modes d'arrondi sont
exercés par la sonde ; aucune qualification GPU ou FTZ/DAZ n'est déduite.
Ce ne sont pas96CTest du moteur historique, ni un nouveau total de CTest.

## Données préparées

R2 prépare les trames brutes08/000000,08/000100 et08/000200 dans leur
repère capteur, chacune en float32 et sur grille1mm, puis relit chaque
préparation en normal/−O. Cela représente6préparations et42nuages,
sans échantillonnage ni restriction à50k.

| Trame | Retours | Sites float32 | Sites1mm | Fusions1mm | Changements de quart1mm |
|---|---:|---:|---:|---:|---:|
| 000000 | 123389 | 123389 | 123389 | 0 | 3 |
| 000100 | 124479 | 124479 | 124479 | 0 | 1 |
| 000200 | 125526 | 125526 | 125526 | 0 | 4 |

Le round-trip float32 vérifie370167/373437/376578coordonnées sans erreur.
La grille vérifie autant de coordonnées avec erreur≤0,5mm par axe.
Les maxima encodés sont respectivement(158607,158284,30595),
(159495,155301,9016) et(159832,95896,16573) ; **u16 ne convient plus**.
L'absence de fusion n'implique pas l'identité des distances, des contacts
ou de la topologie. Les changements de côté et tous les effectifs des
moitiés/quarts sont publiés dans chaque manifeste.

Les bits de réflectance restent dans les sources brutes hachées avec IDs
de retour ; XYZ seulement est géométriquement validé comme fini. Les
doublonsXYZ exacts restent un seul site avec toutes leurs correspondances.
Les formats `f32le`/`u32le` ne doivent pas être chargés dans les sondes u16.

## Reproduction et fermeture

Le lanceur [run_lidar_precision_checks.py](../../bench/run_lidar_precision_checks.py)
exige un build neuf. Exemple de qualification corrigée :

```bash
python -B morsehgp3D_v8/bench/run_lidar_precision_checks.py run --build build/nouveau_float32_release --compiler /usr/bin/g++ --output chemin/neuf --inputs chemin/000000.bin chemin/000100.bin chemin/000200.bin
python -B morsehgp3D_v8/bench/run_lidar_precision_checks.py run --build build/nouveau_float32_sanitize --compiler /usr/bin/clang++ --sanitize --output chemin/autre
python -B morsehgp3D_v8/bench/run_lidar_precision_checks.py read --path morsehgp3D_v8/receipts/float32_precision_20260921/release_r2/precision_a1drpf9i
python -B -O morsehgp3D_v8/bench/run_lidar_precision_checks.py read --path morsehgp3D_v8/receipts/float32_precision_20260921/release_r2/precision_a1drpf9i
```

Les manifestes lient lancement/configuration, sept sources instantanées,
compilateur, dépendances natives, exécutable, commandes et données brutes.
Les flux natifs complets et leur entrée sont conservés hors de la simple
sortie résumée des gates ; le lecteur rejoue l'oracle Fraction sur ces flux.
Il reconstruit également les partitions depuis les sources brutes et
vérifie les deux profils demandés, dont le défaut1mm, sans se contenter
de hashes de fichiers. Les sources restent hachées avant/après.

Les quatre relectures de fermeture (Release/Sanitize × normal/−O) sont
identiques par paire : [POST_READBACK.json](POST_READBACK.json). Le lecteur
gelé vérifie les binaires/dépendances avant reconstruction des scans ;
il ne les recontrôle pas à sa toute fin. Pour ces captures, une vérification
explicite après les quatre lectures ferme aussi les binaires et leurs
252/255dépendances, sans modifier le lanceur gelé. Réserve à corriger
dans la prochaine révision du lecteur, pas un changement de géométrie.

Clôtures R2 :

- Release : `09eacb2cfaa27285ba2db79e5e87710f507383229154648aef1ce720c38ac87a` ;
- ASan/UBSan : `8be076e996727cc9484a3d9d92994e522f4be25b320603783db91551f9308470`.

Ce chantier est un prérequis numérique. Il n'apporte aucune mesure de
croissance du générateur q2/q3/q4 ni validation d'une tour sur les nouvelles
entrées. Les chronos20mm précédents restent des références historiques.
