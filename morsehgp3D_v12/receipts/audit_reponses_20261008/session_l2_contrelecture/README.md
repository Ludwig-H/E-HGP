# Contre-lecteur L2, source a2 / FULL 902

Port explicite du [lecteur L1r](../session_l1r_contrelecture/README.md), inchangé.
Nouveau lecteur préparé/testé sur JSON synthétiques avant admission du lot réel.
Pins : **a2c2fccfd**, pilote **e10a9cd4**, dépendance partagée `lecteur_full.py`
**3594c5d3**, plan **1f9e4599**, commande **0**. L'émetteur FULL est inchangé par
rapport à ea62 (`201119a7…`, schéma mémoire 902). Les nombres de sites déclarés
proviennent du manifeste L2 `fa7d1544…` ; aucune coordonnée lue.

Le nouveau plan comporte **cinq cas CPU K5/W48, une passe chacun**, tous au-delà de
1,6 M sites : ni passe chaude, ni empreinte demandée, ni mesure FULL GPU/K10. Ce
plan diffère de la préparation L2 initiale ; son empreinte entière est imposée.
Les budgets commandés restent 160 Gio hôte/88 Gio appareil, mais les capacités et
pics appareil doivent être nuls sur la voie CPU. Le pilote construit avec CUDA et
capte encore l'environnement GPU ; cela ne transforme pas les prises CPU en GPU.

Seuls changent l'indice de commande, les pins source/pilote et les métadonnées de
session. Toutes les gardes L1r sont conservées : JSON exact hors bool, cohorte
complète, séquence full/libération/exit CPU, métadonnées, partition des durées,
paires mémoire et leurs frontières, budgets, CPU/RSS nullable sans remplacement
par zéro. Refus mémoire et `wide_leaf` restent des refus publiés ; les non joués
restent dans la cohorte. Aucune réussite partielle n'est promue en prise complète.

B1/B2/B3 portent la voie appareil, B4 K10 : tous sont non évalués pour ce plan CPU
K5, indépendamment de ses durées. Le verdict global du pilote « non tenu » faute
de critères évalués ne constitue donc pas un jugement de ces objectifs GPU.

[test_reader.py](test_reader.py) : cohorte entière synthétique, **25 corruptions
refusées**, deux types de refus, deux null conservés, cas manquant refusé/non joué
préservé, empreinte exigée sur un cas artificiel sous le seuil et préfixe froid
avant refus conservé. Normal/−O identiques. Ces tests n'appellent ni le pilote,
ni le lecteur produit, ni une sonde. [capture.json](capture.json) épingle aussi
le lecteur produit comme dépendance du pilote, pas comme oracle indépendant.

```sh
python3 -B test_reader.py --repo /workspaces/E-HGP --plan PLAN_L2
python3 -B -O test_reader.py --repo /workspaces/E-HGP --plan PLAN_L2
python3 -B reader.py --repo /workspaces/E-HGP --session L2 --plan PLAN_L2 --results DOSSIER_B
```

Aucune source produit modifiée, compilation, moteur ou GCP exécuté. Admission
statistique et provenance/arrêt restent des preuves distinctes. Le lecteur ne
crée aucun digest, code de processus ni résultat manquant ; codes déclarés dans
le rapport épinglé sauf comparaison externe explicitement fournie.
