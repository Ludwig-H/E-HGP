# L1 originale : bruts admis, objectifs MES-B non tenus

Rejeu indépendant du 8 octobre 2026, **source 403736300 / pilote 84777f19**, u21/W48,
budgets 160 Gio hôte et 88 Gio appareil. L'archive intérieure originale `3497c745…`
et son rapport `803aa13e…` sont seuls admis ici. La seconde exécution contenue dans
l'enveloppe de récupération, source ea62/schéma 902, reste distincte. La provenance
du rapatriement et les arrêts sont vérifiés séparément par l'audit de récupération.
Ce rejeu ne lance ni moteur ni GCP et ne lit aucune coordonnée.

Le [lecteur figé L1/L2](../session_l_contrelecture/README.md) admet, en normal et −O,
les **18 processus prévus, 10 réussites et 8 refus `memory_budget`**, sans condition
résiduelle ni divergence du rapport. Aucun cas non joué ou illisible : 15 scènes,
30 passes demandées, **18 complètes dont 8 chaudes**. Les huit processus à deux passes
n'ont chacun qu'une observation chaude ; médiane et maximum chaud sont donc identiques.
Les deux autres succès n'ont qu'une passe froide. Les refus n'ont aucune passe FULL.

| K5, voie catalogue appareil sauf mention CPU | Sites | Dernier FULL (s) | Régime |
| --- | ---: | ---: | --- |
| Boreas n1 sans sol | 146 316 | 0,530931 | chaude |
| Boreas n1 brut | 215 665 | 0,723835 | chaude |
| Boreas n10 sans sol | 1 513 483 | 10,066024 | chaude |
| Même scène, catalogue CPU | 1 513 483 | 22,406314 | chaude |
| Boreas n10 brut | 2 153 342 | 13,081724 | chaude |
| Marseille sans sol | 2 465 285 | 11,126866 | chaude |
| Scion sans sol | 3 439 371 | 42,021361 | chaude |
| Meadow scan1 | 6 181 091 | 35,350340 | chaude |
| Scion brut | 3 589 247 | 43,092741 | froide |
| Marseille brut | 6 709 045 | 31,889699 | froide |

Ce mur couvre P+C+G+raccord+TMVR, avec T/M/V/R inclus dans TMVR. Lecture d'entrée,
ouverture appareil, validation, empreinte et libération ne sont pas ajoutées au mur.
Le JSON [mesures.json](mesures.json) garde leurs valeurs séparées, les étapes exactes
de la dernière passe et toutes les statistiques par cas. Ces scènes ne constituent
pas la cohorte SemanticKITTI à 100 ms ; aucune nouvelle conclusion sur ce contrat.

Les six refus K5 sont Tuwien sans sol (5 199 758), Nibio sans sol (7 793 680),
Boreas n50 sans sol (7 857 268), Boreas n50 brut (10 766 998), Tuwien brut (6 236 167)
et Nibio brut (7 825 857 sites). K10 est refusé sur Boreas n10 sans sol et Marseille
sans sol. `memory_budget` ne désigne pas à lui seul le poste mémoire en cause.
Le succès Marseille brut et le refus Tuwien plus petit empêchent d'inférer un plafond
universel en nombre de sites.

**B1/B2/B4 non tenus ; B3 non évalué** : aucun groupe de croissance complet réussi.
L'exception B1 pour refus à ≥10 M est appliquée sans masquer les autres refus.
Les neuf succès GPU K5 dépassent tous le seuil B1 de 2 µs/site ; le meilleur dernier
rapport vaut 3,356 µs/site. Aucun ajustement de croissance sur une sélection de succès.

CPU/RSS sont renseignés sur les 18 passes complètes. Exemple Boreas n10 sans sol :
dernière passe GPU 206,250292 s CPU, contre 808,426314 s CPU pour la voie catalogue
CPU ; ce temps cumulé des fils n'est pas la latence. Maximum des pics déclarés parmi
les succès : **63 787 597 001 octets hôte**, **87 152 692 652 appareil** et RSS processus
**63 936 311 296**. Ces valeurs mesurent des notions distinctes ; ne pas les additionner
ou appeler le budget hôte « RSS ». L'épinglé reste 67 108 864 octets sur appareil et zéro
sur CPU. Les bornes du lecteur sont satisfaites, sans mesure externe continue de VRAM.

Trois scènes sous le seuil de 1,6 M portent une empreinte FUL1 stable entre passes ;
Boreas n10 sans sol concorde aussi entre CPU et appareil. Les autres scènes n'ont pas
d'empreinte demandée : aucun certificat d'identité n'est inventé. Le rapport déclare les
codes des processus ; aucune nouvelle preuve source→binaire n'est créée par ce lecteur.
Les instantanés GPU avant/après sont vides, sans prouver une isolation continue.

[capture.json](capture.json) épingle archive, rapport, inventaire des 36 fichiers bruts,
plan et lecteur. [replay.py](replay.py) refuse tout autre pin, rejuge le rapport depuis
les JSON, recalcule le résumé compact et vérifie leur stabilité. Le lecteur original
n'a pas été modifié ; le verdict d'admission est distinct de la qualification globale.

```sh
python3 -B replay.py --repo /workspaces/E-HGP --plan CHEMIN_PLAN_L1 \
  --results DOSSIER_ORIGINAL_B --original-archive ARCHIVE_INTERIEURE_L1
python3 -B -O replay.py --repo /workspaces/E-HGP --plan CHEMIN_PLAN_L1 \
  --results DOSSIER_ORIGINAL_B --original-archive ARCHIVE_INTERIEURE_L1
```
