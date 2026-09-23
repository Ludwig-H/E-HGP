# Contrelecture B — tour K10 sur la trame LiDAR brute 08/000000

23 septembre 2026. Reçu source :
[`lidar_raw_k10_density_20260923`](lidar_raw_k10_density_20260923/README.md),
publié par `d7fd32a1`. Cette lecture est **indépendante de la mesure** ;
elle ne lance ni nouvelle sonde ni GCP.

## Intégrité et périmètre

Les **10/10** entrées de `SHA256SUMS` passent dans un worktree propre.
Le manifeste d'entrée est celui de la matrice brute K5, SHA-256
`6e64125abddbfcd589ba61cf747e493e455adec4beeaea81c2739ce98fe16095`.
Les neuf payloads coordonnées/IDs des trois densités, six flux
stdout/stderr et deux manifestes source ont été recoupés avec leurs
empreintes ; les coordonnées sélectionnées reprennent exactement celles
de la trame pleine aux IDs inscrits. Les ensembles d'IDs de retours
sont strictement emboîtés et distincts : **30 847 ⊂ 61 694 ⊂
123 389**. C'est une densification du **même support spatial complet**,
pas la comparaison de quarts/moitiés capteur.

Les trois sondes `mhgp9_tower_probe_v12` ont code 0, sans timeout,
`complete_relative`, K10/s8/W8, huit fils statiques, grille 1 mm,
six leviers actifs, `run_tower=true`, dix ordres K1..10 et un digest
non nul. Les cinq premiers résumés d'ordres sont identiques à ceux
du reçu K5 sur les mêmes entrées. Le binaire est épinglé par SHA-256
`e1ba126fbcea8ad483ebff2265f446c18e90eaefe04bf20021e76cd0df09af80`.
Ces contrôles établissent la cohérence de l'archive et des sorties
**émises**, pas l'absence de boules manquantes : le statut reste
`complete_relative`.

## Ce que disent les trois tailles

| Sites | CPU chaîne (CPU·s) | Formes cœur | Charges cœur | Paires q3/q4 développées | Catalogue | RSS pic |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 30 847 | 173,008 | 103 153 147 | 1 721 708 | 5 402 246 | 2 760 701 | 2,131 GiB |
| 61 694 | 383,704 | 323 686 030 | 3 614 332 | 12 861 574 | 5 669 283 | 4,189 GiB |
| 123 389 | 905,514 | 1 238 630 455 | 7 811 827 | 37 868 819 | 11 387 391 | 8,219 GiB |

Avec les effectifs réels, les pentes finies 1/4→1/2 puis 1/2→plein
sont **1,650 puis 1,936** pour les formes cœur, **1,251 puis 1,558**
pour les paires développées et **1,149 puis 1,239** pour le CPU total.
Le dernier lien des formes s'approche du carré, tandis que K5 sur les
**mêmes entrées** l'avait dépassé (2,136) : aucune propriété globale
sous-quadratique n'en découle. K10 plein examine **2,245×** les formes
et émet **4,035×** les boules de K5 plein. La mémoire devient également
un poste industriel, distinct de la seule ordonnance des workers.

La chaîne pleine affiche **254,575 s de mur local**, dont **205,099 s**
q3/q4 et **34,004 s** pour la tour. Pendant le cas plein, un juge q3
indépendant consommait lui aussi du CPU sur l'hôte partagé. Les ratios
CPU/mur changent fortement entre 1/4 et 1/2 ; les pentes murales ne
forment pas un benchmark propre. Les compteurs de travail et les CPU·s
sont ici plus lisibles, tout en restant **une seule répétition**.

## Limites de preuve et suite

Les lignes `CASES.jsonl` ont été capturées avant l'ajout du champ
`validated` ; elles n'archivent donc pas littéralement
`validated=true`. Le lecteur publié recalcule néanmoins leurs hashes,
options, ordres et invariants, et la contrelecture des payloads est
positive. La reprise après échec et le contrôle des six leviers ont
été corrigés dans le lecteur **avant** sa publication ; ne pas attribuer
rétrospectivement cette validation au moment de la capture.

Ce reçu concerne **une seule trame brute avec sol de la séquence 08**,
en u18/grille 1 mm, CPU local W8. Il ne mesure ni plusieurs scènes,
ni float32 natif, ni GPU/G4, ni le jalon <1 s, et ne certifie pas la
complétude des clés non émises. Prochaine porte : même protocole sur
plusieurs trames et séquences, s8/10/12, puis appariement sur G4 avec
GPU et coût intégral de la tour. Pour l'architecture, réduire en amont
les **formes de cœur réellement matérialisées** et les sorties
intermédiaires est plus pertinent que gagner quelques pourcents sur
le tri ou les compteurs.
