# `full1` : échec conservé du test d'entrée

Source `80e77544e018b42093e8369a587ce7a2e85a6b88`, session close
`failed_remote`, arrêt ciblé certifié, clés retirées et réserve libérée.
La matrice conserve **1 959/1 971 portes passées, 12 échecs** ; le supplément
ASan18 **137/139, deux échecs**. Dans chaque configuration concernée, les
deux mêmes portes échouent : `mhgp11_tower_full_bench_io` et `_opt`.
Les autres portes, notamment forêt et Fraction, passent dans ce périmètre.
La qualification globale reste refusée.

Le traceback original rend `ValueError: FULL natif petit temoin`, ligne 43
de l'ancien test. Sa fonction `write(order)` consomme `reversed(range(3))`
pour XYZ, puis réutilise l'itérateur épuisé pour les identifiants : **36 octets
XYZ et zéro octet IDs**. Le modèle Python indépendant de tout natif
[`../io_input_model.py`](../io_input_model.py) exécute seulement les AST des
deux fonctions d'écriture épinglées : trois témoins valident le correctif
qui matérialise l'ordre une fois, produisant 36 et 12 octets associés.

L'ancien test ne conservait pas le flux du processus enfant dans son erreur.
Ce flux perdu n'est pas reconstitué : `input_unreadable` est le refus attendu
par lecture du contrat, pas un message récupéré dans cette capture. Aucun
défaut du moteur n'est établi par cette porte mal alimentée. Le correctif
du seul harnais et de son diagnostic est publié en `c6ca345e0` ; il exige
une nouvelle exécution native, sans rendre cette capture verte.

La commande différentielle a configuré et compilé les deux exécutables v10
épinglés `c764e121aa52f2e5dd9b85fbe308c9c3511ff55e`, puis a refusé le
prérequis de qualification : **zéro comparaison v10/v11**. Le collecteur
FULL refuse le même prérequis : **zéro mesure FULL**, aucun rapport de
benchmark. Ces refus et les flux de compilation sont dans l'archive unique.

Depuis le dossier parent, `python check_failure.py` puis `python -O
check_failure.py` recoupent reçu brut local, source, archive épinglée et son
manifeste, copies compactes, inventaires JUnit, provenance, fermeture et
refus des quatre commandes. Le code 0 signifie échec cohérent conservé.
[`check.json`](check.json) conserve les sorties normales/−O identiques et
les deux erreurs de mise au point du lecteur, sans rejeu natif.

Capture LIVE dépendant du reçu brut local original. Aucun paquet source
volumineux, binaire ou octet KITTI copié. Le manifeste d'entrée est relié
aux empreintes d'upload ; les sources exactes de l'écrivain d'entrée sont
reliées à leurs commits dans `input_writer_pins.json`. Les expériences
ultérieures, dont le refus disque avant démarrage de `full2`, restent
distinctes de `full1`.
