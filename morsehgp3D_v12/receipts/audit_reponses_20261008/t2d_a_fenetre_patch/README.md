# A : transmettre la fenêtre réelle de la prépasse des feuilles

**Proposition non appliquée**, contre A/w902 `pipeline_run.cpp` **08b385e6…**, base
déclarée 902041f66. Le corps reste inchangé depuis le [témoin de sous-comptage](../t2d_a_fins/README.md).
Aucun produit, prototype, CUDA ou GCP modifié/exécuté ; aucune compilation native.

Le [diff ciblé](proposition.patch) ajoute un petit résultat optionnel `LeafWindow`
contenant les deux instants **déjà lus** par `slice_leaves`. `run_g` charge alors
exactement `[l0,l1]`, plutôt que `[fin_G,fin_G+(l1−l0)]`. Deux u64 locaux, aucun
tampon, tas, état partagé ni lecture d'horloge supplémentaire.

L'inventaire du corps épinglé contient exactement deux appels :

| Appel | Traitement proposé |
| --- | --- |
| Pré-passe dans `run_g`, numérotation prête et absence d'échec G | Remplit la fenêtre locale ; charge sa durée et sa partie après G, même si les feuilles refusent. |
| Feuilles manquantes dans `run_kernel_job` | Argument optionnel nul ; incrémente toujours le ledger des feuilles et `leaves_ns`. La fenêtre englobante du noyau est chargée une seule fois par `run_job`. |

Si `kSliceLeaves` est déjà posé, le noyau conserve son saut de `slice_leaves` ; le
travail du noyau reste chargé. Si la prépasse n'est pas appelée, aucune fenêtre n'est
lue. La fonction écrit les deux bornes après `resolve_leaves` quelle que soit son
issue ; une durée nulle donne deux bornes égales et une charge nulle. Aucune branche
de résolution, issue, émission, drapeau ou compteur logique n'est changée.

À instants fixés, `physical.leaves_ns` et l'accumulateur `ns` gagnent toujours
`l1−l0`. Le total forêt de la prépasse gagne aussi cette même durée. Pour un appel
noyau, la partition conserve `kernel_ns += fenêtre_noyau−leaves_ns` et charge la
forêt une seule fois sur la fenêtre englobante. Seule la localisation temporelle de
la prépasse change dans `foret_apres_g`. Ces égalités de comptabilité ne promettent
pas des valeurs chronométriques identiques après recompilation.

La limite de publication reste explicite : `charge_forest` conserve la fin globale G
connue à l'instant de la charge. Une tâche finie avant sa publication peut toujours
être classée avant G. Ce patch ne transforme donc pas ce diagnostic en mesure exacte
de tout le recouvrement. Il élimine seulement le décalage de fenêtre démontré.

[check.py](check.py) applique le diff **sur copie temporaire**, vérifie la postimage et
l'inverse, l'absence de nouvelle horloge et le maintien de l'appel noyau. Son modèle
indépendant compare les intersections d'intervalles entiers par énumération, avec
durées nulles, refus et feuilles précalculées ; quatre altérations ciblées sont
détectées (ancienne translation, double charge, omission sur refus, fenêtre élargie
au début G). Ce n'est ni un test du C++ compilé ni une qualification de concurrence.
Le développeur devra reprendre ses portes existantes avant intégration.

```sh
python3 -B check.py --source COPIE_A_W902/src/tower/pipeline_run.cpp
python3 -B -O check.py --source COPIE_A_W902/src/tower/pipeline_run.cpp
```

Pins et SHA de postimage : [capture.json](capture.json). Aucun source complet du
prototype n'est recopié dans ce reçu.
