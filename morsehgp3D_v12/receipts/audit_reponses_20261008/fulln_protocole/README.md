# FULLN : protocole de l'actualisation CPU/GPU

8 octobre 2026. Source **`8a0716e74`**, produit adopté B2 + T1-d + R1, après retrait d'A6/A6b ;
**B3 est absent**. Les 488 fichiers de sources, tests, pilotes, CMake et worker sélectionnés dans
le paquet correspondent exactement à ce Git. Plan SHA-256 `9ed65139…`, paquet `23746526…` ;
empreintes complètes dans `capture.json`. Aucune exécution native ou GCP par l'auditeur.

Le lancement local est daté de **17:54:39 UTC**. À la capture **18:30:21 UTC**, `DONE` et le reçu final
sont présents, et `results/` contient 160 fichiers : **les résultats sont arrivés**, mais leur admission
et leurs chronos relèvent d'un reçu distinct. Ce texte ferme le protocole, pas son verdict.

## MES-FULL : trames entières sans sol

Même sonde Release/u21 construite avec CUDA, Session recouverte, **W48**, cache de blocs **8 Gio**
par défaut, FUL1 sur toutes les passes hors du mur. L'absence de `--device` sélectionne le catalogue
CPU : le bras CPU est donc toute la tour calculée sur CPU. Dans le bras appareil, seul le catalogue
utilise le GPU ; la suite de la tour reste sur CPU.

| Cohorte | Voie | K | Processus | Passes par processus | Prises chaudes attendues |
| --- | --- | ---: | --- | ---: | ---: |
| ng00/01/02, 39 885 / 35 551 / 45 845 sites | appareil | 5 | 5 par trame | 10 | 135 au total |
| mêmes trois trames | appareil | 10 | 3 par trame | 5 | 36 |
| mêmes trois trames | **CPU seul** | 5 | **3 par trame** | **5** | **36** |
| v12set, 37 trames de six séquences | appareil | 5 | 5 Sessions | 74, soit deux cycles | 185, second cycle |

Soit **38 processus, 610 passes prévues**, dont 392 prises chaudes. Les trames isolées excluent
la passe 0 de leurs statistiques chaudes ; v12set retient son second cycle. Ordre tournant entre
processus. Aucun bras CPU sur les 37 trames ni CPU K10 sur ng00–02 n'est prévu par ce pilote.

Budget logique de la sonde illimité par défaut, partagé avec l'appareil : ce n'est pas le protocole
massif aux limites séparées 160/88 Gio. Lecture, ouverture, validation, empreinte et destruction sont
hors du mur ; P+C+tour FULL, allocations et transferts du catalogue compris, sont dans le mur.
Le `cpu_ns` du processus additionne ses fils et ne remplace pas le mur du bras CPU.

Le juge compare les FUL1 **au sein de cette campagne**, notamment CPU/appareil à K5. Il ne charge
pas une empreinte R1/v11 comme référence externe et ne fait pas de campagne A/A. Son contrat K5
utilise médianes chaudes et maximum des médianes par processus/trame, seuil 100 ms ; le maximum
brut des passes est publié séparément. K10 est informatif. La cohorte de 37 trames est déclarée par
le plan et les données du pin ; son manifeste effectivement envoyé reste à fermer à l'admission.

## MES-C : petits nuages

Voies **CPU et appareil**, **K5 et K10**, **W4 et W48** : un processus par configuration, trois cycles
du même ensemble ; la valeur chaude d'un nuage vient des cycles 2 et 3, pas de trois processus.
Le paquet déclaré comporte 132 nuages réels, 15 synthétiques sains et 12 difficiles, soit 159.
L'ensemble en Session comprend donc 147 nuages ; les difficiles sont joués séparément à W48,
deux passes par nuage/voie/K. La matrice complète demanderait 56 processus et 3 624 passes.
Ce sont des effectifs prévus, à vérifier sur le manifeste et les résultats.

Cache 8 Gio conservé ; budget hôte 64 Gio et, sur la voie appareil, budget appareil séparé 64 Gio.
Délai global 2 050 s, 120 s par cas difficile ; des lignes `non_joue` peuvent être produites quand
le temps restant est insuffisant. K5 est traité en entier avant K10. Aucun W1 n'est demandé.

C1/C2 restent fondés sur la régression du groupe réel CPU/K5/W48 : coût fixe ≤2 ms et pente
≤241,3 ms / 64 740 sites. **Cette référence de pente est celle de la session K historique**, pas
un nouveau temps FULLN ou R1. C3 exige la cohorte difficile K5 complète sans refus/échec/expiration.
Les FUL1 sont comparées entre passes et voies à K égal.

L'actualisation CPU attendue sur ng00–02 comprend donc 36 prises chaudes, avec cache 8 Gio et le
code R1. La référence CPU M historique était sans cache : une différence observée ne serait pas
à elle seule un gain causal d'un levier. Les nouveaux chiffres exigent l'admission des résultats.

```sh
python -B check.py DEPOT_GIT DOSSIER_SESSION SNAPSHOT_PROTOCOLE
python -B -O check.py DEPOT_GIT DOSSIER_SESSION SNAPSHOT_PROTOCOLE
```

Lecteur limité au paquet source, au plan et à l'état local figé. Aucun accès aux coordonnées, IDs,
forêts de jeux, contrôleurs, commandes de session ou services distants.
