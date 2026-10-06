# Contrelecture indépendante API/CLI

`native_review.json` complète le reçu global : 23 API et 28 CLI,
IDs des manifestes, verdicts individuels, témoins et cause de la correction.
La catégorie `code` est observée dans le journal ; le chemin menant au
refus `parameter_out_of_range` du mutant CLI se déduit des sources.
Les sorties détaillées des variants tués ne sont pas archivées.

Depuis ce dossier :

```sh
python3 -B replay_native.py --check native_review.json
python3 -O -B replay_native.py --check native_review.json
```

Le script reconstruit le JSON et compare ses octets au fichier figé,
sans écriture. Il lit le pin Git `98a009550` et la session locale close
`v11.20261005.clauderepriser3` : archive, reçu et DONE.
Aucun build, test natif, bibliothèque tierce ni accès cloud.
Les quatre fichiers du reçu global et son `SHA256SUMS` sont conservés ;
`NATIVE_SHA256SUMS` couvre uniquement ce complément.
