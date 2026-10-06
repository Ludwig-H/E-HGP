# Conserver le témoin de placement dans les prochains rapports

Source `9d10de213`. `full_probe` émet déjà le nombre de cœurs du plan dans
`full.pipeline_tasks.placement_cores`. `gpu_ab.take_summary` l'omet : deux
sorties ne différant que par 0 ou 24 cœurs donnent des résumés identiques.
C'est précisément la distinction utile pour éviter de mesurer à nouveau
une comparaison A/A involontaire comme `claudeo1place`.

[Proposition minimale](proposal.patch) : conserver ce champ sous
`summary.pipeline_placement_cores`, avec `None` lorsqu'il est absent.
Zéro et inconnu restent distincts. Ce champ décrit le **plan** ; il ne
certifie pas l'acceptation de `sched_setaffinity` par le noyau. Le témoin
`pipeline_placement_requested` ajouté aux tests n'est pas sérialisé par la
sonde actuelle ; aucune observation de ce témoin n'est inventée ici.

Aucun refus global ajouté : K>5 ou une topologie inadéquate peuvent
légitimement désactiver le placement. Sur une future comparaison destinée
à décider de O1, constater des plans nuls demande de classer ce bras
inactif, et non de conclure sur le gain du placement.

Le patch concerne les **futurs rapports**. Il ne complète pas les anciens
reçus. Il est indépendant du correctif du cache des variantes proposé dans
l'audit précédent. Le [rejeu AST](replay.py) compare les fonctions Python
exactes avant/après, pour champ absent, zéro et 24 ; tous les autres champs
restent égaux. Aucun processus natif, test CUDA ou session cloud.

```sh
python3 -B replay.py
python3 -O -B replay.py
sha256sum -c SHA256SUMS
```
