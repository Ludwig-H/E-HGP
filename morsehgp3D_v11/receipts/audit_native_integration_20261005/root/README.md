# Contrôle du comparateur et du lecteur PH — 5 octobre 2026

Sources S3/S5 et rapport WIP capturés, puis identiques en fin de lecture :
[SOURCES.json](SOURCES.json), [CLOSURE.json](CLOSURE.json).
Le contrôle lit les bits du banc réel : FULL16379 correspond à l'ordre seul7035,
retirant uniquement128,1024,8192. Le juge S3 conserve déjà cette référence.

[check.py](check.py) exécute aussi les seules déclarations Python du lecteur PH,
extraites par AST, sur15 buffers synthétiques u18/u21/u24 et K1/2/5/10/12.
Troncature, ajout, magie et version invalides sont refusés. Cela vérifie le lecteur
sur ces buffers, sans produire un export natif ni qualifier les arbres correspondants.

**127 gardes nouvelles**, sorties [normal.stdout](normal.stdout) et
[optimized.stdout](optimized.stdout) identiques ; stderr vide, code0.
Rejouer avec `python3 -S -B check.py` et `python3 -O -S -B check.py`.
Aucun moteur HGP, build, fit, KITTI, CUDA ou GCP exécuté.
