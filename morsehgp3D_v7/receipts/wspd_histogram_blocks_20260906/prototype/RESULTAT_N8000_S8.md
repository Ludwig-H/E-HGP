# Verdict privé : ne pas intégrer le parcours de blocs sur cette population

Un seul passage O2, CPU6 isolé, n=8 000 uniforme, coordonnées 65 536, graine 3, s=8, seuils `[10,9,8]`. Statut `public_status=not_claimed`. Les trois bras utilisent les mêmes 754 686 rectangles survivants et donnent des histogrammes littéralement identiques : 8 436 096 valeurs comparées, soit deux comparaisons complètes de 4 218 048 valeurs. Aucun changement produit.

| Maximum des tailles des facteurs du rectangle | Rectangles | Scalaire produit (ms) | Blocs forcés (ms) | Hybride8 (ms) |
| --- | ---: | ---: | ---: | ---: |
| 1 | 450 424 | 34,939 | 39,918 | 37,568 |
| 2 | 246 927 | 35,822 | 87,224 | 40,030 |
| 3–4 | 56 547 | 22,285 | 57,394 | 22,983 |
| 5–8 | 788 | 0,774 | 2,025 | 0,736 |
| Total | 754 686 | 93,819 | 186,560 | 101,318 |

Ces durées incluent appels, aplatissement et agrégation des compteurs ; les certificats de blocs sont instrumentés. Ordre fixe scalaire/bloc0/hybride8, sans répétition : ce n'est ni un ratio stable, ni une mesure du chemin intégré. Les vérifications littérales et digests sont hors chrono. Le front commun prend séparément 37 734,985 ms ; aucun calcul de complétion ou FULL dans cette sonde.

Les facteurs sont minuscules : 3 361 400 occurrences facteur/lane, dont 2 625 994 singletons et 628 234 facteurs de taille 2. Le maximum réellement observé est 7. Le seuil hybride8 ne déclenche donc **aucun parcours de blocs** : ce run ne valide pas l'intérêt d'un dispatch sur de grands facteurs.

Le scalaire paie exactement 1 986 888 couples hors diagonale. Le bras bloc0 en évite 308 204 mais ajoute un parcours de 5 062 454 nœuds : 1 972 573 tests hmax, 88 225 tests Hmin, 41 584 tests Ximax. Partition physique exacte : 18 767 positions créditées en blocs + 289 437 couples rejetés par hmax + 1 678 684 tests scalaires = 1 986 888. Les crédits sont non vacants, mais aucun groupe de tailles observé ne favorise le bras blocs forcés.

Conclusion bornée : conserver le scalaire sur cette population. Les histogrammes ne sont pas ici le coût dominant ; le gain potentiel d'un nouveau certificat sur les grands facteurs reste non mesuré, et rien dans ce passage ne justifie son intégration. s10/s12 non mesurés pour ce prototype, conformément à l'arrêt du lot mono avant transition multi-CPU.

## Captures closes

Processus 479755, début `2026-09-06T12:15:18.661073+00:00`, fin `2026-09-06T12:15:56.942629+00:00`, code 0, groupe fermé, stderr vide. `taskset --cpu-list 6`, attente directe sans délai automatique ni quota CPU/fichier ; garde RAM 26 GiB. Sources et ELF inchangés. GCP non utilisé.

- Commande : `../v7_wspd_histogram_blocks_20260906_measure_r1/s8.command.json`, SHA `5b76dd040c68bdb7406fd17b413d8a9dca68c172864f76cba40641efbd9ae35f`.
- Brut : `../v7_wspd_histogram_blocks_20260906_measure_r1/s8.stdout`, SHA `837d92101c78cee3fa77ebcbc21e88320ad00ffd18bcfed1888f9a2c8aafa337`.
- Source sonde : `histogram_bench.cpp`, SHA `9722a9757af675c5be796ce831e0f2b9f221f01096126c8c65f5d81d1ecd7108` ; helper `histogram_blocks.hpp`, SHA `1da88985c33d66d016039f5d5494228d7cbc3b19ff9d50ccf144560194c5947a`.
- ELF : `4a584a3f6b8a8d4de186350fc8b97cabf60a30daf29b844f2e099ed4e2a79f2d` ; compilation O2 nominale `-Wall -Wextra -Wpedantic -Werror`, dépendances liées, code 0 puis argument inconnu code 2.
- Front digest : `bbecdac441d1518b176fde78dc15c2a424e5e4202c1b30db2aceb660b8aa3c57` ; histogrammes `d81db3c80b970299659149601033eafc75607dcfa7d535f6a8b03733f78cc3e0`.

Le recordeur initial de compilation reste conservé byte-exact sous `record_compile_original.py` (SHA `a9e6c8fce2d1a7e44770611ce7082c1ff1e295d265373f1f99476c21f1dca82b`). Le recordeur de mesure directe est distinct dans le temps, SHA `c7e61f66ede3e6443f86b0fcf5d50aed4e0e11266653dc5c28e357e9e87dc04b`. Les captures closes de compilation n'ont pas été réécrites.
