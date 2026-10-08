# L1/L2 : plans et observation du lancement, aucune mesure admise

Observation locale du **8 octobre 2026 à 04:44:40 UTC**, sans action distante ni lecture de payload.
Sources de L1 : commit publié `403736300900a709b82316721eeb589fbc4a5350`.
Le pilote MES-B reste `84777f19…`, identique à la [contrelecture de livraison](../mes_b_livraison/README.md).
`capture.json` conserve les empreintes des plans, manifestes, sources et artefacts locaux, sans identité
de compte ni cible d'infrastructure. L'observation est historique ; elle n'annonce pas l'état futur de la session.

`launch.json` atteste un lancement à **04:39:17 UTC**. `host/lifecycle.txt` porte `targeted_running`,
génération **04:40:05.285 UTC**. À l'observation, ni reçu final ni résultats rapatriés n'étaient présents.
Aucun arrêt certifié ni temps de calcul ne découle de ce lancement.

## Fenêtre du contrôleur

Préflight L1 : 1 440 427 786 octets de données, paquet source de 5 007 961 octets ;
plan `cdc81d10…`, paquet `e547240a…`. La fenêtre de 2 202 s est **après** le coût estimé d'envoi :

`3300 − 318 (envoi et préparation) − 660 (fermeture) − 120 (emballage) = 2202`.

Le contrôleur réserve ensuite 120 s fixes : commande de 2 000 s ≤ 2 082 s disponibles,
donc `oversubscribed=false` est cohérent. Ajouter une seconde fois les 318 s serait erroné.
Le pilote emploie 1 930 s pour prévoir et borner ses cas. Son build interne garde toutefois son
timeout propre de 3 600 s : ce délai du pilote n'est pas un coupe-circuit global strict.
La commande externe reste bornée par le contrôleur ; ses éventuels échecs ou troncatures devront être
lus dans les futurs bruts, pas transformés en prises réussies.

Le refus initial de **L** pour volume de données est annoncé par le développeur, mais aucune trace
primaire de cette tentative n'a été retrouvée ici. Le contrôleur épinglé vérifie bien le débit prudent
de 2 Mio/s avant les opérations de lancement. Cela établit le mécanisme **conditionnel**, pas le fait
historique « aucune VM » sans trace du refus. Aucun tel certificat n'est ajouté par cet audit.

## Cohortes prévues et limites

| Plan | Scènes distinctes | Processus prévus | Passes prévues | Passes après la première | Processus à une passe |
| --- | ---: | ---: | ---: | ---: | ---: |
| L initial | 19 | 22 | 35 | 13 | 9 |
| L1 | 15 | 18 | 30 | 12 | 6 |
| L2 | 5 | 5 | 6 | 1 | 4 |

L1 et L2 reprennent tous les cas de L et ajoutent Lyon avec sol (32 412 887 sites déclarés),
sans suppression de scène dans les plans. W48, profil compilé u21, budgets hôte/appareil séparés
160/88 Gio et seuil FUL1 de 1 600 000 sites sont communs. Les deux manifestes déclarent u21 et
au plus 20 bits nécessaires ; ces propriétés n'ont pas été recomputées depuis les coordonnées.

L1 conserve les deux séries Boreas n1/n10/n50 et les deux cas K10, ces derniers **à une seule passe** :
pas de mesure chaude K10 prévue. L2 ne prévoit que K5, quatre cas sur cinq à une seule passe.
Les nombres du tableau sont des maxima prévus : le pilote peut publier `non_joue` lorsque son délai
ne permet plus de lancer un cas. Ils ne sont ni des exécutions ni une cohorte effectivement admise.

FUL1 est demandé sur cinq processus L1 (neuf passes prévues : trois noms Boreas, dont le bras CPU
et un K10) ; aucun cas L2 n'est sous le seuil. L'identité des grandes scènes ne sera donc pas contrôlée
par FUL1 dans ces plans. Les noms ne désignent pas de découpes ; la déclaration « captations entières »
et les deux écarts annoncés — dédoublonnage au millimètre, retrait du sol pour `sans_sol` — sont cohérents
avec les métadonnées. L'intégrité géométrique des payloads n'est pas attestée par cette lecture seule.

`check.py` compare les sources au commit, puis les artefacts locaux stables et les plans aux SHA capturés,
recalcule les cohortes, l'union L1/L2 et l'arithmétique de fenêtre. Il ne lit que les manifestes des données.
Un état de cycle de vie ayant évolué est signalé sans remplacer l'observation historique.

```sh
python3 -B check.py --repo /chemin/depot --session-dir /chemin/session_l1 --plans-dir /chemin/plans
python3 -B -O check.py --repo /chemin/depot --session-dir /chemin/session_l1 --plans-dir /chemin/plans
```
