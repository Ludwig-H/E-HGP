# census1 — échec conservé du mutant de cohorte

La session `v11.20261003.census1`, source poussée
`c6954f231bf726d37f399fc9298f6a4309033e8e`, est fermée `failed_remote`
(worker 1, DONE 3). Le lecteur rend `coherent=true`, `conforming=false`.
L’arrêt ciblé, la génération, le retrait des clés et la libération de la réserve
sont certifiés dans le reçu brut ; `errors` et `warnings` sont vides.

| Configuration | Portes réussies / sélectionnées |
|---|---:|
| GCC Release | 580 / 580 |
| Mutants | 20 / 21 |
| GCC ASan/UBSan | 505 / 505 |
| GCC TSan | 505 / 505 |
| Profil u21 | 505 / 505 |
| Profil u24 | 505 / 505 |
| Poison | 506 / 506 |
| Style | 2 / 2 |
| Total matrice | **3 128 / 3 129** |
| Supplément ASan18 | **264 / 264** |

Clang Release est déclaré facultatif et absent. Toutes les constructions de
base aboutissent. Aucun échec de porte fonctionnelle produit n’est observé.
Le seul échec de la matrice est `mhgp11_mutants_tower`.

Le manifeste tower exécuté contient 88 mutants. Les sorties complètes JUnit et
LastTest concordent : **87 morts par code**, et **un mutant invalide à la
compilation**, `forest_cohort_nonbirth_reset`. Son ajout de `rank.reset()`
provoque GCC `-Werror=maybe-uninitialized`, à `forest_build.cpp:63:15`, sur
l’optional `rank`. Ce refus n’est ni une mort causale ni une construction
attendue : son manifeste exige une exécution. Le lecteur vérifie les 88 IDs,
leurs verdicts et le diagnostic dans la section exacte du test en échec.
Les six autres modules apportent 191 morts par code/ligne et deux constructions
attendues, vérifiées séparément contre leurs manifestes épinglés. Aucun
survivant n’est annoncé par ces sorties ; la campagne mutants reste refusée.

La commande `002_census_full` finit code 2 avec
`full_parallel_refused: ValueError`, sans rapport ni essai natif. Son calendrier
épinglé prévoyait **29 unités**, toutes non démarrées : triples 127/255/511 sur
les six entrées LiDAR u21/u24, deux essais W1/W8 sur ng00 u21 en mode 511, et
neuf essais uniformes. Le flux ne conserve pas de traceback du collecteur ;
le lecteur ne fabrique aucun résultat ni flux enfant. Aucun temps FULL,
réemploi sémantique ou gain census n’est qualifié dans cette capture.

Le préflight original déclare 850 + 180 + 570 secondes de commandes et
120 secondes de préparation, soit **1 720 ≤ 1 737 secondes** utiles. Il n’est
pas sursouscrit. Les commandes ont leurs groupes fermés, sans flux tronqué ni
groupe résiduel tué. Le premier diagnostic extrait est gardé dans
[first_failure.txt](first_failure.txt) ; le correctif ultérieur du mutant
n’est pas appliqué à cette preuve historique.

## Lecture et portée

Depuis la racine du dépôt :

```sh
python -B morsehgp3D_v11/receipts/full_census_20261003/census1_failure/check.py
python -B -O morsehgp3D_v11/receipts/full_census_20261003/census1_failure/check.py
python -B morsehgp3D_v11/receipts/full_census_20261003/census1_failure/check_selftest.py
python -B -O morsehgp3D_v11/receipts/full_census_20261003/census1_failure/check_selftest.py
```

Le lecteur exige les objets Git de la source exécutée, le reçu brut local,
l’archive originale et le paquet source local (le lien vers son déplacement
bit-identique est accepté). Il rehash ces pièces, contrôle le manifeste tar,
les copies compactes, les configurations/cache/provenances de construction,
les verdicts JUnit et les sections LastTest. Les sept helpers historiques sont
copiés depuis la même source et vérifiés **avant import** ; aucun banc mutable
n’est importé. Seule la fonction pure de calendrier est chargée depuis l’AST
épinglé. Les empreintes de binaires sont conservées, pas leurs binaires.

[reader_proof.json](reader_proof.json) conserve les lectures normal/−O
identiques : **11 témoins et 80 corruptions rejetées**, zéro appel natif ou
cloud. Les altérations comprennent la reclassification du mutant invalide,
les deux sorties modifiées de concert, une mort par signal, les IDs perdus ou
doublés, les garde-fous de fermeture et le budget initial. Un premier refus du
lecteur, dû à la normalisation du plan de préflight, reste consigné séparément.

La capsule contient une seule archive de résultats compressée et les résumés,
manifestes et scripts légers nécessaires. Aucun paquet source, payload LiDAR,
binaire natif ou nouveau résultat produit n’est copié ici. Cette lecture de
preuves n’est pas un rejeu natif ni un audit exhaustif des helpers historiques.
