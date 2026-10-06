# Contrelecture indépendante des 24 identités supports

`native_review.json` complète le reçu global : lignes exactes, références
par profil, journaux, workers et comparaison du pic public avec les voies.

Depuis ce dossier :

```sh
python3 -B replay_native.py --check native_review.json
python3 -O -B replay_native.py --check native_review.json
```

Le script reconstruit le JSON en mémoire et compare ses octets au fichier
figé, sans aucune écriture. Il lit les objets Git au pin `98a009550` et
la session locale `v11.20261005.clauderepriser2` close : archive, reçu et
DONE. Aucun build, test natif, bibliothèque tierce ou accès cloud.
La capsule globale d’origine et son `SHA256SUMS` restent inchangés ;
`NATIVE_SHA256SUMS` couvre ce complément.
