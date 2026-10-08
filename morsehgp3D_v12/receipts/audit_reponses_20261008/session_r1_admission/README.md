# R1 : admission indépendante et derniers FULL GPU

**R1 satisfait sa règle préannoncée.** Relecture du 8 octobre 2026, Python normal et `-O`, sans nouveau moteur, compilation, contrôleur/cloud ni payload de scène. Les 112 journaux et 1 441 passes FULL sont admis par le lecteur strict épinglé LF15437. Le juge est rejoué avec la cohorte externe ; un second calcul des rapports/bootstrap retrouve exactement le jugement, sans écart flottant. Le critère R1 est un gain relatif, pas le contrat absolu de 100 ms.

## Source et fermeture

Session `v12.20261008.r1`, APRÈS `47feedc96ee4b4cb85b5a7c590f38c874bb04855`, AVANT `5f8e777cffbeddfe90e92fc616a920c28c1b985c`. Archive AVANT SHA `d07df84f…`, paquet APRÈS `73e657bc…`, plan `7484822f…`. Les 368/369 fichiers du périmètre source natif/sondes/tests/CMake correspondent aux objets Git ; les quatre sources supplémentaires du pilote sont comparées séparément. **Le seul fichier `src/` différent est `tower/registry_branches.cpp`.** La sonde FULL `1068940a…` est identique entre bras ; le cache de blocs de 8 Gio est le défaut commun. Le [raccord mathématique](../r1_raccord_math/README.md) reste une preuve distincte des mesures.

Archive résultats : 642 673 octets, SHA `f288289ad6842007aa14a8493346f28be600edb1a44778a687d07586284b2bd5`, 289 entrées de manifeste vérifiées. Worker/DONE 0, aucune erreur, résultats vérifiés, arrêt ciblé certifié code 0 en une tentative, RUNNING→TERMINATED. Les quatre commandes sont closes avec code 0 : socle 141,337 s, pilote 473,867 s, LiDAR 751,168 s, mutants tour 190,140 s. Ce sont des durées de commandes, pas des latences HGP.

Primaires CTest : 747 Passed au socle, 7 Passed LiDAR, 1 Passed pour le lanceur de mutants, aucun échec/saut dans ces trois sorties. Le socle comporte les six portes `registry_{fixtures,exhaustif,grand,inventaire,etapes,aleatoire}`. Le CTest du lanceur de mutants ne remplace pas une contrelecture individuelle de chaque mutant. Aucune campagne native supplémentaire exécutée par l'audit.

Les deux ELF sont épinglés à la construction puis après ng et après K10 ; leurs hashes restent égaux. A/A utilise exactement l'ELF AVANT. Les configurations archivées indiquent Release/u21/CUDA ON. GPU vide aux observations avant/après, sans preuve d'isolation continue. Codes, résumés et empreintes de journaux sont enregistrés par le pilote ; chaque sortie est relue entièrement avec ce code, sa configuration et la cohorte. Les stderr natifs sont vides.

## Cohorte réellement jouée

Le plan prescrit explicitement **ng : 5 tours × 10 passes**, **grandes : 6 tours × deux parcours de 21 trames**, **K10 informatif : 3 tours × 5 passes**, W48. Trois bras : AVANT, le même AVANT_BIS et APRÈS ; rotation sans inversion. Le manifeste externe des 37 trames est celui déjà admis dans les sessions A/M ; même hash d'archive déclaré, noms/effectifs/ordre réutilisés sans relire de coordonnées.

- Identité : 22 processus, 100 passes. Onze clés par bras, dont la Session des 37 trames, ng K5/K10, ng00 K5 W1 et uniformes 8k/16k/32k.
- Grandes décisives : 18 processus, 756 passes ; seul le second parcours compte, soit 126 chaudes par bras.
- ng K5 de garde : 45 processus, 450 passes ; première passe écartée, soit 45 chaudes par trame/bras.
- K10 informatif : 27 processus, 135 passes ; première passe écartée, soit 12 chaudes par trame/bras.

Total : 783 passes chaudes décisives et 108 informatives. Toutes emploient le catalogue GPU ; **W1 est ici une prise d'identité avec catalogue GPU, pas un nouveau banc CPU seul**. Les identités FUL1 sont stables et égales AVANT/APRÈS sur toute la cohorte. FUL1 ne sérialise pas les tableaux du registre R : ne pas transformer cette égalité en preuve CSR indépendante, fournie séparément par les portes/propositions de R1.

## FULL chaud : valeurs absolues

ng : médiane des médianes des processus ; colonne « réunies » = médiane de toutes les passes chaudes de la trame. Aucun quotient des médianes substitué aux rapports appariés du juge.

| K5/W48/GPU | Sites | AVANT, ms | APRÈS, ms | APRÈS réunies, ms | Maximum brut APRÈS, ms |
|---|---:|---:|---:|---:|---:|
| ng00 | 39 885 | 82,825836 | **79,596547** | 79,658116 | 87,808475 |
| ng01 | 35 551 | 67,989060 | **66,104721** | 65,949531 | 80,816667 |
| ng02 | 45 845 | 85,573285 | **82,814686** | 82,813316 | 83,947216 |

Ces 135 passes APRÈS K5 sur ng sont toutes sous 100 ms. Les **21 grandes trames, de 61 198 à 99 099 sites**, donnent :

| Agrégat K5 grandes | AVANT, ms | APRÈS, ms |
|---|---:|---:|
| Médiane des 21 médianes par trame (six prises) | 157,781446 | **152,784483** |
| Médiane des 126 passes chaudes réunies | 156,848047 | 153,035254 |
| Plus grande médiane par trame | 295,508961 | **287,934213** |
| Maximum brut des 126 chaudes | 298,858218 | 290,860032 |

Les 21 médianes APRÈS et les 126 passes chaudes restent au-dessus de 100 ms. Les 37 trames sont couvertes en identité ; **leur latence contractuelle complète n'est pas remesurée ici**, les chronos décisifs visant le sous-ensemble de 21 grandes. Aucun FULL multi-million dans ce lot.

| K10 informatif, médiane des trois médianes de processus | AVANT, ms | APRÈS, ms | APRÈS réunies, ms | Maximum brut APRÈS, ms |
|---|---:|---:|---:|---:|
| ng00 | 517,487341 | **495,820565** | 495,952160 | 499,246421 |
| ng01 | 384,877473 | **367,228245** | 367,262291 | 367,790817 |
| ng02 | 442,374377 | **423,830273** | 423,830273 | 424,450553 |

FULL est le mur de la Session recouverte en mémoire, P+C+tour et verticales comprises. Entrées déjà quantifiées u21 et masque figé ; lecture, ouverture initiale, validation externe, empreinte et libération sont hors de ce mur selon la sonde. Les fenêtres physiques G/T/M/V/R peuvent se recouvrir ; elles ne sont pas additionnées aux temps FULL. CPU·s et RSS restent des métriques distinctes. Le pic du budget actif n'inclut pas les blocs hôte inactifs du cache de 8 Gio et ne constitue pas un total RSS+VRAM.

## Règle relative, inchangée

Bootstrap de 10 000 tirages sur les **tours**, graine 20261008. Grandes : moyenne des logarithmes des rapports par trame, puis un échantillon par tour ; ng : rapports des médianes chaudes par processus. Bornes strictes : grandes <0,99, chaque ng <1,02 ; veto A/A sur la GM hors [0,985 ; 1,015]. La largeur de l'IC A/A n'est pas elle-même le veto préannoncé.

| Cohorte | GM APRÈS/AVANT | Borne haute IC95 % | GM A/A |
|---|---:|---:|---:|
| Grandes | 0,9733622769227891 | 0,9769597871648749 | 0,9994462150749543 |
| ng00 | 0,9631940407634909 | 0,968381235749243 | 1,0002617794977529 |
| ng01 | 0,9703096196238041 | 0,993340243252971 | 0,9935781082355671 |
| ng02 | 0,965653864047588 | 0,9694727186037502 | 1,0002713628622448 |

Les quatre critères tiennent, aucune prise manquante, aucune identité divergente, A/A valide : adoption reproduite. Les résultats K10 restent informatifs. Comparaison causale bornée à ces deux bras et à ce régime ; aucune attribution de ces gains à un changement CPU seul, à une autre campagne ou au massif.

## Rejouer sans calcul géométrique

[check.py](check.py), [capture.json](capture.json) et [results.json](results.json) épinglent sources, métadonnées et agrégats. Les helpers de manifeste et de comparaison Git déjà publiés sont réutilisés. Le snapshot ne reçoit que rapport et JSONL/err issus du retour fermé, hors dépôt ; aucun binaire ni payload.

```sh
python -B check.py --repo /workspaces/E-HGP --session /workspaces/.ehgp-sessions/v12.20261008.r1 --snapshot /workspaces/.ehgp-auditors/evidence-snapshots/r1_admission_20261008
python -B -O check.py --repo /workspaces/E-HGP --session /workspaces/.ehgp-sessions/v12.20261008.r1 --snapshot /workspaces/.ehgp-auditors/evidence-snapshots/r1_admission_20261008
```

Les archives de session, le paquet source et l'archive source AVANT restent requis. Aucune restauration d'un moteur ou extraction d'une archive de données n'est faite par ces commandes.
