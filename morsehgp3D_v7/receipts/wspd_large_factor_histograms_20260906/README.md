# Grands facteurs : gain q2, rejet angulaire encore coûteux

`public_status=not_claimed`. Trois mesures privées mono CPU6, un seul rectangle par nuage, aucun front complet ni calcul FULL. Les deux facteurs originaux sont les enfants de la racine, chacun de cardinal n/2, et couvrent tout X : le cœur hors facteurs est vide. Deux réseaux de pas 32, 32 positions par axe x/y, axe z allongé avec n, dans des cubes de côté ≤1024, séparés en x par l'offset 64512. Les contrôles passent pour **s8, s10 et s12** ; s n'intervient plus dans l'histogramme de ce rectangle fixé, donc les temps ne sont pas trois mesures de s différentes.

| n | Lane | Scalaire (ms) | Blocs forcés (ms) | Hybride8 (ms) | Visites bloc |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 8 000 | q2 | 503,590 | 113,321 | 113,689 | 2 740 948 |
| 8 000 | q3 | 829,138 | 641,535 | 617,685 | 12 741 112 |
| 8 000 | q4 | 708,960 | 707,221 | 734,270 | 14 680 432 |
| 16 000 | q2 | 2 239,338 | 465,763 | 464,124 | 10 769 936 |
| 16 000 | q3 | 3 228,604 | 2 751,615 | 2 736,669 | 52 997 140 |
| 16 000 | q4 | 2 925,427 | 3 115,275 | 3 120,519 | 61 030 668 |
| 32 000 | q2 | 8 470,138 | 1 842,450 | 1 834,742 | 42 917 936 |
| 32 000 | q3 | 11 909,521 | 11 803,996 | 11 736,376 | 234 075 844 |
| 32 000 | q4 | 10 696,857 | 13 445,798 | 13 946,741 | 269 139 988 |

Totaux scalaire/blocs/hybride : **2 041,687 / 1 462,077 / 1 465,644 ms** à8k ; **8 393,368 / 6 332,653 / 6 321,312 ms** à16k ; **31 076,516 / 27 092,244 / 27 517,858 ms** à32k. Un passage par bras dans un ordre fixe ; certificats instrumentés, sorties pré-réservées, comparaison littérale et digest hors chrono. Hybride8 et bloc0 suivent ici le même chemin, car les deux facteurs dépassent8 ; leurs différences de temps ne sont pas un effet algorithmique.

Toutes les sorties sont littéralement identiques : 48 000, 96 000 et 192 000 comparaisons de valeurs. Le travail scalaire total exact est 95 976 000, 383 952 000 et 1 535 904 000 couples. À32k/q4, les blocs paient encore 127 419 312 tests scalaires, 138 566 148 Hmin et 117 506 740 Ximax. Les partitions physiques sont vérifiées dans chaque bras : couples logiques = crédits de positions + rejets hmax + tests scalaires.

**Verdict borné :** le régime de grands facteurs existe et les crédits améliorent nettement q2. Mais les visites croissent encore presque quadratiquement dans cette famille, q3 perd son avantage et q4 ralentit. Aucun résultat sous-quadratique ni gain FULL revendiqué. Un certificat négatif angulaire q3/q4 reste une piste distincte, non testée ici. Ne pas transférer ce résultat aux petits facteurs : le [nuage uniforme](../wspd_histogram_blocks_20260906/README.md) conserve sa conclusion négative pour l'intégration des blocs.

## Provenance et fermeture

Les trois commandes [8k](r2/n8000.command.json), [16k](r2/n16000.command.json), [32k](r2/n32000.command.json) et leurs stdout/stderr sont conservées. Codes0, groupes fermés, stderr vides, sources/ELF inchangés ; dernière fermeture `2026-09-06T12:36:09.948753+00:00`, PID536680. Attente directe, aucun quota temps/CPU/fichier ; garde RAM26GiB. GCP non utilisé.

Le premier build échoué reste dans `r1/`, avec sa source exacte : conversion rétrécissante u64 vers PointId refusée par `-Werror`. R2 ajoute uniquement le cast explicite pour n≤32000 ; compile0 et argument inconnu2. Le [prototype exact](../wspd_histogram_blocks_20260906/prototype/histogram_blocks.hpp) reste inchangé et n'est pas recopié. Les headers produit sont ceux épinglés par19ff070a et le [snapshot déjà publié](../wspd_histogram_blocks_20260906/source_snapshot/) ; les hashes des sources et dépendances sont conservés dans chaque capture. La source et le recordeur gardent leurs chemins historiques privés, sans se présenter comme un nouveau lanceur portable.

[publication.json](publication.json) lie les copies et références externes. Un seul ELF réussi est omis explicitement ; aucune copie massive de headers, aucun nouveau moteur pour publier. `SHA256SUMS` contient des chemins canoniques sans `./`.
