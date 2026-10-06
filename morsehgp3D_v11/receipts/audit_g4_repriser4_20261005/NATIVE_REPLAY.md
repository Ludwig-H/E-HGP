# Contrelecture indépendante ASan/UBSan u24

`native_review.json` complète le reçu global : six lignes supports
exactes, journaux et workers, pic compute identique à FULL, drapeaux
sanitizer de la bibliothèque et de la sonde, lien et diagnostics.

```sh
python3 -B replay_native.py --check native_review.json
python3 -O -B replay_native.py --check native_review.json
```

Le script reconstruit le JSON et compare ses octets au fichier figé,
sans écriture. Il lit le pin Git `98a009550` et la session locale close
`v11.20261005.clauderepriser4` : archive, reçu et DONE.
Aucun build, test natif, bibliothèque tierce ni accès cloud.
Les quatre fichiers du reçu global et son `SHA256SUMS` sont conservés ;
`NATIVE_SHA256SUMS` couvre uniquement ce complément.
