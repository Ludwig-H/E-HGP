# Phase 14 — diagnostic Geogram/CUDA aux ordres 1 et 2

> **Addendum Phase 15.** La restriction aux wedges ayant au moins deux arêtes de Delaunay a depuis été réfutée exactement sur une fixture générique à six points. Les fermetures par étoile, carré du graphe et fan de faces sont aussi réfutées à huit points. Ce document conserve la mesure historique; son prochain jalon « radix sort des wedges » est annulé. Voir [PHASE15_DELAUNAY_GAMMA2_FALSIFICATION.md](../../../validation/PHASE15_DELAUNAY_GAMMA2_FALSIFICATION.md) et [DELAUNAY_ORDINAIRE_GAMMA2.md](../../../math/DELAUNAY_ORDINAIRE_GAMMA2.md).

## Verdict

Le diagnostic `geogram_ordinary_delaunay_plus_cuda_g4_aabb_grid / hgp_reduced / proposal_only_then_bounded_exact_replay` traite bien sur GPU tous les triangles canoniques possédant au moins deux arêtes de la triangulation de Delaunay ordinaire fournie par Geogram. Il ne construit ni mosaïque de Delaunay d'ordre supérieur, ni univers global des $\binom{n}{3}$ triplets, ni six ordres Morton. L'ordre 1 est réduit par Kruskal sur les arêtes de Delaunay. Pour l'ordre 2, la sortie de Gabriel historique reste séparée du nouveau flux `restricted_Delaunay_wedge_Gamma2`, qui conserve tout wedge valide à son niveau de miniboule calculé sur GPU.

Sur E5 et quatre nuages aléatoires bornés de 12 à 14 points, le rejeu `Fraction` recalcule exactement chaque niveau et compare toutes les coupes strictes et fermées de `hgp_reduced`. Les wedges ne couvrent que 63,64 % à 76,36 % des triplets sur les quatre nuages aléatoires, mais les 2 640 états comparés coïncident tous avec Gamma$_2$ exhaustif; E5 ajoute 44 états exacts sur 44. Ce résultat montre que les cofaces absentes sont silencieuses sur ces entrées. Il ne prouve ni la complétude universelle de la restriction, ni son indépendance au SoS de Geogram sur les dégénérescences.

Le niveau binary64 n'est pas certifié : tous les records des quatre nuages aléatoires et six des dix records E5 diffèrent de leur miniboule rationnelle exacte, avec des erreurs précoces et tardives. La concordance annoncée porte donc uniquement sur la topologie wedge après recertification exacte des petits cas. À 50 000 points, le résultat reste `diagnostic_only`, `public_status=not_claimed`.

## Mesure 50 000 points sans cap

Le run fixe 48 workers CPU, le GPU Blackwell et `triangle_chunk_vertices=0`; le rapport confirme un seul chunk sans limite de candidats. Geogram produit 334 979 tétraèdres, 385 152 arêtes et un degré maximal 47. Le GPU énumère 5 870 145 wedges bruts, les déduplique en 4 396 699 triangles canoniques, calcule toutes leurs miniboules et compacte les 4 396 699 records valides pour Gamma$_2$ restreint. Il compte 417 839 triangles de Gabriel, zéro ambigu et zéro invalide.

L'énumération GPU prend 1,389 ms, la classification 9,391 ms et la compaction/tri device du flux restreint 6,869 ms. Geogram prend 32,643 ms et l'extraction de ses arêtes 45,747 ms. En revanche, le transfert du flux restreint prend 50,856 ms, son tri global hôte 102,628 ms et sa réduction hôte 3 609,314 ms. Le total froid vaut 4 372,066 ms : les objectifs 100 ms et une seconde sont tous deux manqués. Le profil localise donc nettement le prochain travail dans un radix sort et une réduction par lots de niveau égal sur GPU, pas dans une nouvelle variante Morton ni dans davantage de tests AABB.

Le flux restreint active 2 125 514 facettes et termine avec une composante. Cette hiérarchie n'est pas publiée comme exacte : la complétude de l'univers wedge n'est pas démontrée globalement et les niveaux restent binary64. La mémoire device maximale comptabilisée vaut 607 297 632 octets, hors scratch interne de Thrust.

## Capacité au-dessus de dix millions

Le passage précédent au SHA `9547ab8` traite 10 000 001 points avec Geogram, 48 workers et le GPU : 77 589 517 arêtes de Delaunay, 1 184 064 447 wedges bruts et 887 005 885 candidats canoniques en 201 chunks. Les classifications GPU prennent 2,194 s et l'EMST d'ordre 1 sélectionne 10 000 000 arêtes. Le total de 207,306 s est dominé par les 175,648 s de la seule réduction Gabriel sur CPU. Ce passage démontre la capacité d'énumération et de classification des wedges, pas une hiérarchie Gamma$_2$ restreinte massive : la nouvelle arène de tous les records valides et sa réduction n'ont pas été exécutées à cette taille.

Le prochain backend massif doit donc trier et réduire les records par chunk sur GPU, écrire des runs segmentés et fusionner sans matérialiser les 887 millions de records sur l'hôte. La voie 50 k peut rester résidente; la voie 10 M+ doit être streamée et reprenable. Dans les deux cas, aucun catalogue de Delaunay d'ordre supérieur n'est justifié.

## Comparaison au surrogate

Sur le même nuage de 50 000 points, le surrogate rapide produit dès l'ordre 1 un digest `4fd1fd3844f08e78` et une racine carrée `0.0060263515656`, contre `374028028d5a4594` et `0.00040516435914542874` pour l'EMST issu de Delaunay. Il ne coïncide donc pas avec la vérité de bas ordre, malgré son débit. Le diagnostic Geogram/CUDA est plus proche du modèle mathématique : ordre 1 recertifiable par l'EMST, et ordre 2 observationnellement identique à Gamma$_2$ réduit sur les petits cas après recertification des niveaux.

## Provenance et fermeture GCP

Le code est au commit `16d83082a09979796d62ca4d0e9b9e30731b022d`. Le build incrémental n'ayant pas relancé CMake, le champ SHA embarqué du binaire est resté `9547ab8c21a1a8753931b1acf5c5a926a25a49cf`; cette différence est enregistrée au lieu d'être masquée. Une seconde génération gardée a été ouverte pour corriger la provenance, mais le redémarrage avait nettoyé le cache Geogram sous `/tmp`; aucun benchmark n'a alors été lancé et la cible a été arrêtée immédiatement.

Les deux générations `SPOT` utilisaient `instanceTerminationAction=STOP`, `maxRunDuration=3600` et un arrêt invité vérifié. La cible exacte `devpod-gpu-exploration / europe-west4-ai1a / ehgp-blackwell-spot-ai1a` est finalement certifiée `TERMINATED` au `lastStartTimestamp=2026-07-27T10:38:18.453-07:00`; aucune autre VM `project=e-hgp` n'est active et les clés OS Login temporaires ont été révoquées. Les métriques structurées sont dans [phase14_geogram_low_order_g4_16d8308.json](../../../validation/phase14_geogram_low_order_g4_16d8308.json).
