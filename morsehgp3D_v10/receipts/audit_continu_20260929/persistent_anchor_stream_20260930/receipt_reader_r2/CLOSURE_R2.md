# Lecteur R2 — métadonnées liées et captures séparées

Le paquet parent est clos et demeure octet pour octet intact. R2 ajoute
uniquement ce sous-dossier ; SHA256SUMS_R2 épingle aussi ../SHA256SUMS.
La preuve reste abstraite : aucun nouveau nuage, export natif, moteur ou GCP.

Le lecteur original rejouait exactement la preuve, mais ne liait pas tous
les champs historiques du receipt à la capture. R2 exige les inventaires
exacts {PROTOCOL.txt,check.py}, les deux argv historiques déclarés et codes0,
scope/counts/record_sha256/mutants/outputs_byte_identical reliés au JSON.
Les premières sorties historiques étaient combinées stdout/stderr ; R2
ne prétend pas avoir séparé rétroactivement ces flux.

Le nouveau receipt.json contient deux nouvelles invocations de check.py,
normale et −O, capturées séparément stdout/stderr avec argv, code, timestamps
UTC et monotones, pins avant/après. Le mode --capture du lecteur ne modifie
aucun fichier : il produit ce JSON, ensuite enregistré par apply_patch.
Les sources du lecteur et de cette note sont aussi épinglées avant/après
ces invocations. Les timestamps appartiennent à ce rejeu R2, pas à R1.

Le lecteur vérifie le manifeste original et R2 avant toute exécution du
script de preuve ; rejoue les deux commandes, compare leurs sorties
exactement puis revérifie les manifests. Les chemins historiques sont
validés comme argv déclarés ; aucune dépendance vivante n'est exigée sur
le chemin /tmp originel. Le lecteur se relocalise avec le paquet parent.

    python3 -B receipt_reader_r2/verify_r2.py
    python3 -B -O receipt_reader_r2/verify_r2.py

Aucune modification de check.py, PROTOCOL.txt, README.md ni des captures R1.
Les règles de complétude géométrique restent des hypothèses, non testées ici.
