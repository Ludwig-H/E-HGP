# MES-B1o : temps et comparaison B1t, 10 octobre 2026

**Les 23 FULL comparables sont plus rapides ; les cinq refus mémoire persistent.**
19 processus, 14 prises réussies, cinq refus, 23 FULL complètes dont huit chaudes,
exactement comme B1t. Les huit chaudes sont chacune l'unique seconde passe d'un processus,
pas des répétitions statistiques. Protocole dans la [prélecture](../mesb1o_prelecture/README.md),
sources et clôture dans l'[admission](../session_mesb1o_admission/README.md).
Source `ae8f8107` (A6c et B3-K) contre `caf9585e4` (R1) : comparaison descriptive entre
campagnes, sans attribution causale du gain à une seule modification.

Le plus grand K5 GPU réussi **dans ce lot** est Marseille entier : **6 709 045 sites,
13,535001 s, première passe seule**. Le plus grand avec une chaude est Meadow :
**6 181 091 sites, 13,499561 s**. Le CPU n'est mesuré que sur Boreas 10 sans sol,
1 513 483 sites : **17,627440 s, première passe seule**. Ce lot ne reteste pas Paris et
ne modifie pas à lui seul un record provenant d'une autre cohorte.

Temps mur FULL en secondes, de `prepare_cloud` à la fin de `build_tower`, catalogue et
verticales compris ; lecture, ouverture, segmentation/préparation hors ligne,
validation externe, digest et libération hors mur. W48, u21, hôte 160 Gio :

| Scène | K | Régime | B1t GPU | B1o GPU |
| --- | ---: | --- | ---: | ---: |
| Boreas 1 sans sol | 5 | chaude, 88 Gio | 0,305449 | 0,264179 |
| Boreas 1 entier | 5 | chaude, 88 Gio | 0,438878 | 0,353951 |
| Boreas 10 sans sol | 5 | chaude, 88 Gio | 7,184661 | 4,892115 |
| Boreas 10 entier | 5 | chaude, 88 Gio | 8,827338 | 6,230022 |
| Marseille sans sol | 5 | chaude, 88 Gio | 8,094787 | 5,169723 |
| Scion sans sol | 5 | chaude, 88 Gio | 36,221713 | 28,554895 |
| Meadow | 5 | chaude, 88 Gio | 29,620722 | 13,499561 |
| Boreas 10 sans sol | 10 | première seule, 88 Gio | 38,620355 | 36,252789 |
| Marseille sans sol | 10 | première seule, 88 Gio | 38,521377 | 36,341402 |
| Scion entier | 5 | première seule, 88 Gio | 30,369913 | 21,659181 |
| TU Wien entier | 5 | première seule, 88 Gio | 45,543974 | 36,114827 |
| Marseille entier | 5 | première seule, 88 Gio | 25,076427 | 13,535001 |

L'identité sous budget GPU 8 Gio donne Boreas 10 sans sol à **6,461631 s en première
passe**, puis **6,279673 s chaude** (ancienne chaude 8,200726 s). Le CPU passe de
19,483532 à 17,627440 s en première passe ; ne pas rapprocher ce nombre d'une GPU chaude.
Les sept chaudes principales diminuent de 13,51 à 54,43 %. Aucun contrat principal
100 ms n'est acquis par ces mesures massives ; verdict MES-B principal inchangé :
B1/B2/B4 non tenus, B3 non évalué. Aucun cas prévu n'a été sauté.

**Gains et prochain goulot.** La comparaison conserve les rangs, scènes, K, voies et
budgets, ainsi que les entiers ns. Pour Meadow chaude, FULL baisse de 16,121161 s,
la queue après G de 16,944029 s, tandis que la fenêtre G augmente de 0,771584 s.
G inclut la concurrence avec la forêt : ce n'est pas du travail CPU isolé.
Les décompositions sont calculées dans chaque passe, avec le reste non partitionné,
jamais par somme de médianes. Dernière fin R : K5 dans 21/23 anciennes passes ; désormais
K4 dans 17, K3 dans cinq, K2 dans une, aucune K5. G finit encore sur K1 dans 23/23,
ce que l'ordre de distribution des tâches peut expliquer ; ne pas en déduire un coût
intrinsèque maximal de K1.

La queue ne pèse plus que 11,1 à 19,1 % des sept chaudes principales. La supprimer à
reste constant laisserait 0,235 à 25,229 s : une nouvelle réduction de R(K5) ne suffit
plus à changer l'échelle. Priorité mesurable : réduire le travail de G sur tous les
ordres et le coût C, en conservant les compteurs discrets ; vérifier le mur FULL car
la contention peut changer. Le catalogue représente 78,7 % de la seule FULL CPU.
Ce diagnostic oriente les sondes ; il ne promet aucun gain d'une proposition.

**Mémoire et refus.** Les 23 pics hôte augmentent ; les 23 pics appareil sont identiques
à B1t, et C augmente dans 22/23 passes malgré la baisse du mur total. TU Wien sans sol
(5 199 758 sites) donne **24,818231500 s**, libère en **1,877791703 s**, puis refuse
la deuxième passe avec `resource_exhausted/memory_budget`, code 2 : aucune chaude.
Pic/capacité appareil identiques à B1t : **85 886 193 388 octets** ; pic hôte
**84 448 763 345**, contre 80 067 741 265 ; SMI 89 663 Mio dans les deux processus.
Ces mesures ne localisent pas le budget ni l'étage fautif : **CST0243 reste ouvert**.
Les quatre autres refus ne donnent que `open` puis `exit`, sans FULL ni stderr :

| Cas du manifeste | Sites | FULL |
| --- | ---: | ---: |
| NIBIO 12 sans sol | 7 793 680 | 0 |
| NIBIO 12 entier | 7 825 857 | 0 |
| Boreas 50 sans sol | 7 857 268 | 0 |
| Boreas 50 entier | 10 766 998 | 0 |

Ne pas leur attribuer un temps ni une cause « hôte pendant la tour » absente des
journaux. Cette attribution du README développeur `fbd5923a8` reste une hypothèse.
Ses deux parenthèses d'effectifs après « entière et sans sol » inversent cet ordre ;
les noms du manifeste et des rapports donnent les correspondances exactes ci-dessus.

Les quatre clés FUL1 sous le seuil 1,6 M sont identiques entre passes, voies/budgets
disponibles et campagnes. Au-delà, aucune empreinte différentielle n'est produite.
Les codes natifs restent déclarés par le pilote ; l'ELF est haché initialement, pas à
la fin. Le lecteur ne remplace pas l'admission de provenance ni l'[arrêt G4](../g4_controle_mesb1o/README.md).

[check.py](check.py) relit les rapports et JSONL avec le lecteur FULL épinglé, identique
dans les deux sources ; [capture.json](capture.json) fige leurs empreintes,
[results.json](results.json) conserve les comparaisons exactes. Lectures normale et `-O`
conformes ; aucun moteur, compilateur, cloud ou payload ouvert. B1t est relu depuis sa
copie publique admise : JSONL identiques à l'archive, blocs numériques des rapports
identiques, mais rapports entiers expurgés à publication. Pour reproduire les empreintes,
utiliser ce chemin public, pas les rapports bruts de l'ancienne extraction.
Exemple depuis ce dossier :

```sh
python3 -B -S check.py --repo /workspaces/.ehgp-auditors/root-v12 --old-cmd /workspaces/.ehgp-auditors/root-v12/morsehgp3D_v12/receipts/g4_mesb1t_20261008/resultats/cmd --new-cmd /workspaces/.ehgp-auditors/evidence-snapshots/mesb1o_admission_20261010/raw/cmd --old-plan /workspaces/.ehgp-sessions/v12.20261008.mesb1t/package/plan.json --new-plan /workspaces/.ehgp-sessions/v12.20261010.mesb1o/package/plan.json
```
