# Complément autonome du reçu de batterie

Ce sous-reçu complète la première capture LIVE sans modifier ses six payloads ni son ledger. Il conserve la copie intégrale de `ab_direct.py`, SHA256 `95ab6a774b0d14c8ff0143908c32c01d91b347e1cd7f193c7e8d1aa350d1e047`, identique à l'entrée de [sources.json](../sources.json) ; [provenance.json](provenance.json) lie aussi les empreintes des fichiers de la première capture. Python et NumPy suffisent au rejeu.

La sonde charge exclusivement [cette copie locale](ab_direct.py), à côté de son propre fichier. Depuis ce dossier :

```sh
python3 -B probe_void_scope.py
python3 -B -O probe_void_scope.py
```

Les deux commandes rendent le code 0 et les mêmes résultats que la capture initiale : pour la cible {0,1}, le bloc {0,1,2} vaut IoU 2/3 sur tous les sites, puis 1 quand le seul site 2 est masqué void. Arbres et masses géométriques fixes. [Résultats](probe_results.json) et [ledger distinct](SHA256SUMS). Aucun moteur, donnée LiDAR, GPU ou GCP ; aucune autre campagne.
