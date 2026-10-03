# vertical1 — échec conservé, aucun banc lancé

Source `5e39d2726041684e9499bf1ba05f37f31f2f39a5`, session
`/workspaces/.ehgp-sessions/v11.20261003.vertical1`. Le lecteur vérifie la
cohérence d'une campagne **non conforme** ; son code 0 ne qualifie ni FULL,
ni les verticales, ni la réutilisation du census, ni une performance.

La matrice conserve **3068/3075** portes, le complément ASan18 **254/255**.
Les huit constructions natives ont réussi. Dans chacune des sept
configurations fonctionnelles, la seule porte en échec est
`mhgp11_tower_census_reuse_descents` : 1338 contrôles, un échec de
`census_reuse_test.cpp:35`, `saturated > 0`, plancher 500 conservé.
Les petites fixtures de cette porte n'exercent pas le cas saturant compté.
Cet échec de couverture ne démontre pas une divergence géométrique du produit.

Le même échec fait tomber le témoin non muté de tower : **aucun de ses
88 mutants déclarés n'a été jugé**. Les six autres campagnes de mutations
conservent 188 morts par code ou ligne et deux refus de construction attendus,
sans mort par signal ou délai. Les inventaires individuels sont confrontés
aux manifestes du commit et aux sorties complètes archivées.

La troisième commande a refusé la qualification avant lancement du banc :
code 2, `full_parallel_refused: ValueError`, aucun rapport de mesures.
Les **20 unités prévues sont non démarrées** ; zéro résultat perdu, zéro
mesure FULL et zéro tentative de réutilisation sémantique. Le premier extrait
brut est conservé dans `first_failure.txt` ; ses autres lignes « ECHEC »
incluent des contre-tests attendus et ne remplacent pas l'inventaire JUnit.

Le préflight original reste inchangé : 1680 s de commandes + 120 s de
construction/préparation = **1800 s > 1737 s** de fenêtre utile. Son
`oversubscribed=true` et son avertissement sont vérifiés. La copie du plan
ultérieur `95d178314` ne sert qu'à documenter la correction : 1600 + 120 =
1720 s, budget propre du banc 500 s. Elle n'est pas une exécution ni une
correction rétroactive de vertical1.

La fermeture du reçu original est vérifiée : `failed_remote`, worker 1,
`DONE=3`, arrêt ciblé certifié, même génération, VM `TERMINATED`, garde
invitée intacte, réserve libérée, clé OS Login retirée et clé privée supprimée.
La cible est `ehgp-v7-3b1d496aed430749ea7e049f`, `us-central1-c`, projet
`devpod-gpu-exploration`. Erreurs et avertissements de fermeture sont vides.

`results.tar.gz` est l'archive originale de 644554 octets, SHA-256
`58a8507dbba8c4b46b27ef230aec663058f465358b88f31180609064dda268ef`.
Elle contient les commandes, sorties, JUnit et provenances, aucun paquet
source, binaire natif ou nuage. Le lecteur borne la décompression, refuse
les chemins dangereux et vérifie tous les membres du manifeste. Les hashes
des binaires et données déclarés dans les reçus ne sont pas présentés comme
une relecture de ces objets absents.

Depuis ce dossier :

```sh
PYTHONDONTWRITEBYTECODE=1 python3 check.py
PYTHONDONTWRITEBYTECODE=1 python3 -O check.py
PYTHONDONTWRITEBYTECODE=1 python3 check_selftest.py
PYTHONDONTWRITEBYTECODE=1 python3 -O check_selftest.py
```

Le lecteur est **LIVE** : il exige le reçu local original, son `DONE`, les
deux auxiliaires bruts épinglés et les objets Git des sources déclarées.
Ses sept helpers historiques sont copiés sous `reader_sources/`, hachés
avant import et confrontés au commit source ; aucune dépendance au code WIP
du banc. `checks.json` conserve les lectures et contre-tests Python,
`files.json` les empreintes de cette capsule. Aucun natif ni appel cloud
n'est exécuté par ces commandes.
