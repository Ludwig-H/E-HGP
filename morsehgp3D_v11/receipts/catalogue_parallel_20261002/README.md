# Catalogue parallèle : capture `parallel5`

Source exécutée : `c1046dfc7689ce07faaee255671f13df96378b2d`.
Session `v11.20261002.parallel5`, close avec arrêt ciblé certifié, clés
retirées et verrou libéré. La matrice passe **1 779/1 779** portes ; son
supplément ASan/UBSan u18 `num;index;tower` passe **107/107**. Les **189**
mutants sont reliés à leur porte JUnit et à la sortie complète `LastTest.log`.
Il s'agit d'une qualification bornée de cette source, sans résultat FULL.

Le banc a exécuté les **36 tentatives prévues** : **30 succès K5**, puis
**6 délais K10** au plafond de 15 secondes. Aucune omission ni tentative
sans résultat persistant. La campagne reste `conforming=false`, commande
et worker en code 1 ; cet échec est conservé. Sa durée de 526,633 s inclut
aussi l'écriture et le décodage des sorties, hors temps natif du catalogue.

Les six groupes K5 sont égaux : hash sémantique entre u21/u24, hash brut
au sein du même profil entre répétitions/workers, treize compteurs logiques,
deux compteurs q4 et nombre de passes. Les trois groupes K10 sont incomplets,
sans sortie réussie à comparer. Aucun accord K10 n'en découle.

## Mesures K5

Valeurs en secondes, **cloud + création du pool + catalogue**. Les trois
répétitions W48 LiDAR donnent la médiane ; W8 et les uniformes n'ont chacun
qu'une mesure. Les entrées sans sol sont les trois trames 08/000000,
08/000100, 08/000200, donc une seule séquence. La préparation du masque,
la lecture disque et le décodage du résultat sont hors de ces durées.

| Entrée | Sites | u21 W48 | u24 W48 | u21 W8 | u24 W8 |
|---|---:|---:|---:|---:|---:|
| `lidar_ng00` | 39 885 | 4,460 | 4,671 | 4,889 | 5,113 |
| `lidar_ng01` | 35 551 | 3,002 | 3,182 | 3,936 | 4,126 |
| `lidar_ng02` | 45 845 | 4,028 | 4,279 | 4,054 | 4,337 |
| uniforme 8k | 8 000 | 0,785 | 0,908 | — | — |
| uniforme 16k | 16 000 | 1,862 | 2,034 | — | — |
| uniforme 32k | 32 000 | 4,252 | 4,747 | — | — |

Les ventilations sont conservées dans
[`parallel5/timing_summary.json`](parallel5/timing_summary.json), dérivé du
rapport brut. Les médianes calculées champ par champ ne s'additionnent pas.
Sur LiDAR W48, les médianes count/fill vont chacune de 0,896 à 1,528 s,
le tri de 0,888 à 1,423 s. Les sommes de durées de tâches sont des temps
cumulés ; elles ne se soustraient pas au temps mur. Ces résultats ne
respectent pas 200 ms, même pour le seul catalogue. Ils ne prouvent aucune
borne globale de complexité ni un gain stable de W48 sur W8.

## Lecture et périmètre de preuve

Depuis ce dossier :

```sh
python check.py
python -O check.py
python check_selftest.py
python -O check_selftest.py
```

Le code 0 du lecteur signifie **capture cohérente**, y compris son échec
conservé. Résultats normal/−O identiques dans
[`check_selftest.json`](check_selftest.json) : quatre témoins valides et
36 corruptions refusées, sans calcul natif local. Le lecteur relie archive,
manifeste intégral, copies compactes, source, caches, exécutables, statuts,
fermeture et calendrier causal. Il lit aussi le reçu brut local original :
cette capture LIVE n'est pas une archive autonome.

Les quatre scripts du collecteur exécuté sont figés dans `source_c104/`,
avec empreintes et inventaire des mutants dans `contract.json`. Le lecteur
réutilise aussi les helpers historiques conservés. Il vérifie séparément
60 inégalités somme des durées de tâches ≤ min(W,J) × temps mur : cette
garde mathématique supplémentaire est postérieure à l'exécution et ne doit
pas être attribuée au collecteur c104.

Les payloads canoniques ont été supprimés après décodage par le banc : le
lecteur compare leurs **hashes publiés**, sans recalcul possible depuis ces
payloads. Le manifeste des entrées est relié à l'upload ; aucun octet KITTI,
paquet source volumineux ou binaire natif n'est recopié ici. L'archive
`results.tar.gz` est conservée une seule fois, avec ses flux natifs originaux.

[`capture_log.json`](capture_log.json) conserve l'échec initial de recherche
du manifeste, puis deux refus dus au lecteur (cible ASan18 indue ; tuples
Python comparés aux tableaux JSON). Les corrections du lecteur n'ont pas
modifié la capture ni déclenché de rejeu natif. La capture `parallel4_failure/`
antérieure conserve séparément ses erreurs de compilation et son mutant
invalide ; elle n'est pas requalifiée par `parallel5`.
