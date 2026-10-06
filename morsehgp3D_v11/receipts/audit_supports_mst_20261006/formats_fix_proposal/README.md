# Proposition minimale : égalité des versions supports

Le patch ajoute un seul refus dans `bench/mhgp11_formats.py`, après décodage du fichier : la version MHGP11SP doit égaler la version déclarée dans le manifeste. Il se fonde sur le WIP capturé au HEAD `9eee2ed4bcef1e960cdf2456012b84416854dc20`, lecteur SHA256 `4d05bf53fb272bc2101bdd8069bab86ce28be6fa2451d852d33a6403a3248896`. Il ne modifie aucune règle de rétrocompatibilité, aucun écrivain ni aucun fichier du développeur.

Le rejeu stdlib applique exactement cette ligne au snapshot immuable adjacent `../formats/sources/`, vérifie le diff et les empreintes, puis appelle les lecteurs réels avant/après sur trois dossiers temporaires synthétiques de 200 octets :

- v2 binaire et manifeste v2 : accepté avant et après ;
- v1 binaire déclaré v2 : accepté avant, refusé après par la nouvelle égalité ;
- mêmes octets v2 que le premier cas, déclarés v1 : refusés avant et après par la politique actuelle du manifeste, avant la nouvelle garde.

Le deuxième cas établit causalement l'effet du correctif ; le troisième répond à la fausse déclaration du même binaire sans prétendre modifier la politique v1. Les fixtures minimales n'exercent pas les nouveaux arbres MST ni l'écrivain C++.

Depuis ce dossier :

```sh
python3 replay.py
python3 -O replay.py
sha256sum -c SHA256SUMS
```

Les deux lectures sont identiques. `summary.json` conserve les seuls verdicts, tailles et empreintes synthétiques. Le snapshot adjacent est nécessaire : cette proposition est un complément à la capture publiée, pas une archive autonome. Aucun calcul natif, accès cloud ou octet LiDAR ; aucune qualification native du correctif. Le développeur reste libre de choisir séparément la politique de rétrolecture v1.
