# L1r : seconde exécution, bruts 902 admis

Source **ea62cd691**, pilote **457d0e6f**, commande **1** du plan **49892867**.
Le rapport `d7357b6c…` décrit une nouvelle exécution, distincte de
[L1 originale](../session_l1_admission/README.md) récupérée par la commande 0.
Le [nouveau lecteur 902](../session_l1r_contrelecture/README.md) a été préparé et
testé avant ce rejeu réel ; les deux lecteurs et leurs preuves restent séparés.
Provenance extérieure et arrêt relèvent de l'audit de récupération ; aucun moteur,
GCP ou payload n'est lancé/lu ici.

**Normal/−O : bruts admis, zéro condition, rapport concordant.** Même cohorte fermée :
18 processus, 15 scènes, 10 succès, 8 refus `memory_budget`, 18 passes complètes
sur 30 demandées, dont 8 chaudes. Tous CPU/RSS renseignés. Le verdict reste **B1/B2/B4
non tenus, B3 non évalué**, sans série de croissance complète ni refus omis.

| K5, voie catalogue appareil sauf mention CPU | Sites | Dernier FULL (s) | Régime |
| --- | ---: | ---: | --- |
| Boreas n1 sans sol | 146 316 | 0,519050 | chaude |
| Boreas n1 brut | 215 665 | 0,699520 | chaude |
| Boreas n10 sans sol | 1 513 483 | 10,024965 | chaude |
| Même scène, catalogue CPU | 1 513 483 | 22,344450 | chaude |
| Boreas n10 brut | 2 153 342 | 13,001688 | chaude |
| Marseille sans sol | 2 465 285 | 10,955437 | chaude |
| Scion sans sol | 3 439 371 | 41,951244 | chaude |
| Meadow scan1 | 6 181 091 | 35,324820 | chaude |
| Scion brut | 3 589 247 | 42,829632 | froide |
| Marseille brut | 6 709 045 | 31,548426 | froide |

Chaque succès à deux passes n'apporte qu'une passe chaude ; sa médiane et son maximum
chauds sont la même observation. Aucune dispersion interprocessus n'est estimée.
Les six refus K5 et les deux K10 sont les mêmes que L1 originale et n'ont pas de passe
FULL. Les identités FUL1 des trois scènes sous 1,6 M concordent entre passes ; Boreas
n10 sans sol concorde aussi CPU/appareil. Au-delà du seuil, aucune empreinte n'est
inventée. Cette cohorte ne qualifie pas le contrat SemanticKITTI à 100 ms.

La nouvelle attribution mémoire est cohérente sur les cinq étapes : usages≤pics,
usage précédent≤pic suivant, maximum des pics égal au pic global, entrée résidente,
raccord sans nouvelle allocation budgétée. Sur chaque succès GPU, le pic hôte est
celui de TMVR. Sur Boreas n10 sans sol CPU, le pic est celui de C : **24 453 936 844
octets**, contre **17 049 044 532 hôte** et **23 036 138 508 appareil** pour la voie
catalogue GPU. Ce sont des budgets séparés, pas une mesure unique de RSS/VRAM.
Les capacités conservées, l'épinglé et RSS restent distincts dans
[mesures.json](mesures.json), avec les cinq couples `[usage,pic]` de chaque dernier
succès et les statistiques de toutes ses passes.

Les derniers murs séquentiels montrent le coût restant après C, sans supposer de
recouvrement : Boreas n10 sans sol C/G/TMVR = **1,742/2,947/5,261 s** ; Scion sans sol
**5,804/14,631/21,271 s** ; Meadow **3,397/7,730/23,802 s**. Ce sont des partitions
d'une seule passe par scène, pas des sommes de médianes. Les durées CPU cumulées,
validation, empreinte et libération hors mur sont conservées à part. Ces observations
ne prédisent pas le gain du pipeline A ou d'un autre levier.

Le moteur `src/` n'a pas changé entre les deux sources selon la provenance séparée ;
la sonde ajoute l'instrumentation mémoire. Les écarts L1/L1r ne constituent pas un
gain causal ni une comparaison statistique d'implantations. Les codes des processus
restent ceux du rapport épinglé ; le lecteur ne fabrique aucune preuve de build.
GPU vide avant/après n'atteste pas à lui seul une isolation continue.

[capture.json](capture.json) épingle l'enveloppe `34eebfbf…`, le rapport de commande1,
l'inventaire des 36 bruts, le plan et le lecteur. [replay.py](replay.py) rejuge les JSON,
recalcule le résumé et vérifie les octets avant/après ; aucun reçu antérieur modifié.

```sh
python3 -B replay.py --repo /workspaces/E-HGP --plan PLAN_L1R \
  --results DOSSIER_NOUVEAU_B --archive ENVELOPPE_L1R
python3 -B -O replay.py --repo /workspaces/E-HGP --plan PLAN_L1R \
  --results DOSSIER_NOUVEAU_B --archive ENVELOPPE_L1R
```
