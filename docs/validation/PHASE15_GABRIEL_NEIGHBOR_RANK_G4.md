# Phase 15 — rangs de voisinage des témoins Gabriel PDEL

## Verdict récent

Contexte administratif inchangé : Phase `15`, `backend=reference_cpu/cuda_proposal`, `profile=hgp_reduced`, `mode=budgeted`, `deployment_status=diagnostic_sidecar`, `public_status=not_claimed`. La porte d'entrée de Phase 15 reste satisfaite; ce diagnostic n'ouvre ni ne ferme une phase.

Au SHA `509a9c6f0e41fb4ae37975b2e8f10f87bc0101f6`, le sidecar Geogram/PDEL mesure le plus petit budget de voisins qui suffit à proposer chaque triangle `gabriel_binary64` accepté depuis une racine témoin PDEL. Il n'effectue pas une recherche par arc : chaque source lance un seul parcours complet du LBVH Morton. La fenêtre Morton initialise seulement le tas; la borne AABB binary64 dirigée vers le bas ne prune que sous inégalité stricte et toute égalité descend.

Le résultat principal est le suivant : sur le nuage canonique jusqu'à 30 000 001 points, la variante `symmetric_union_star` retrouve les 263 693 761 triangles acceptés avec $M=\lceil4k\ln n\rceil$ pour $k=2$. Les variantes plus strictes `directed_root_star` et `mutual_star` manquent respectivement 2 et 4 triangles à cette valeur sur 30 M; $M=\lceil5k\ln n\rceil$ couvre les trois variantes avec marge sur toutes les tailles testées. C'est une politique de proposition empirique, pas une borne universelle.

## Sémantique mesurée

- `directed_root_star` minimise, sur les racines PDEL disponibles, le maximum des deux rangs sortants;
- `symmetric_union_star` accepte chaque arête incidente dès que l'une de ses deux directions est dans le préfixe;
- `mutual_star` exige les deux directions de chaque arête incidente;
- un triangle de support 2 est évalué depuis sa racine formée par les deux arêtes PDEL disponibles;
- lorsque la troisième arête n'est pas PDEL, le seuil publié est le minimum sur les racines témoins disponibles et seulement une borne supérieure sûre du minimum absolu sur les trois racines.

La clé est exactement `(distance carrée binary64, PointId canonique)` jusqu'à $M$. Le rang `M+1` signifie seulement que le rang exact est strictement supérieur à $M$. Le transcript conserve deux octets par arc dirigé du CSR PDEL et ne matérialise ni table $n\times M$, ni matrice de paires, ni mosaïque de Delaunay d'ordre supérieur.

## Résultats massifs

Tous les runs utilisent la graine canonique `5570761781678720848`, $M=256$, $W=256$, sauf les deux lignes 50 k explicitement comparées. Les colonnes `3 / 2` comptent les triangles dont le seuil est établi à partir de trois paires PDEL ou de seulement deux paires formant une racine témoin.

| Points | Gabriel acceptés | 3 / 2 paires PDEL | Arcs CSR dirigés | Maximum `directed / union / mutual` | p99 `directed / union / mutual` | Noyau rang | Rang total | Sidecar froid |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 50 000, $M=128$ | 417 839 | 351 472 / 66 367 | 770 304 | 77 / 77 / 84 | 31 / 28 / 34 | 28,391 ms | 31,809 ms | 1,780 s |
| 50 000, $M=256$ | 417 839 | 351 472 / 66 367 | 770 304 | 77 / 77 / 84 | 31 / 28 / 34 | 66,686 ms | 70,071 ms | 1,822 s |
| 1 000 001 | 8 665 509 | 7 277 029 / 1 388 480 | 15 496 198 | 111 / 107 / 111 | 32 / 29 / 35 | 1,361 s | 1,409 s | 14,796 s |
| 10 000 001 | 87 631 258 | 73 541 644 / 14 089 614 | 155 179 034 | 120 / 117 / 132 | 32 / 30 / 35 | 15,948 s | 16,363 s | 156,870 s |
| 30 000 001 | 263 693 761 | 221 248 887 / 42 444 874 | 465 689 528 | 142 / 137 / 146 | 32 / 30 / 35 | 52,637 s | 53,861 s | 481,542 s |

Il n'existe aucun triangle sans racine témoin PDEL dans ces runs. Les rangs maximums restent très au-dessus du p99 : la queue rare, et non la masse, impose la marge de sécurité.

| Points | $M=\lceil2k\ln n\rceil$ | Manqués `directed / union / mutual` | $M=\lceil4k\ln n\rceil$ | Manqués `directed / union / mutual` | $M=\lceil5k\ln n\rceil$ | Verdict observé |
|---:|---:|---:|---:|---:|---:|---|
| 50 000 | 44 | 476 / 290 / 904 | 87 | 0 / 0 / 0 | 109 | trois variantes complètes |
| 1 000 001 | 56 | 1 918 / 1 134 / 3 830 | 111 | 0 / 0 / 0 | 139 | trois variantes complètes |
| 10 000 001 | 65 | 5 404 / 3 282 / 10 833 | 129 | 0 / 0 / 2 | 162 | trois variantes complètes |
| 30 000 001 | 69 | 8 960 / 5 267 / 18 431 | 138 | 2 / 0 / 4 | 173 | trois variantes complètes |

À 50 k, $M=128$ donne exactement les mêmes seuils de triangles que $M=256$ et le noyau reste à 28,391 ms. Ce temps ne qualifie pas le SLO produit : il exclut la découverte sans Delaunay, les flux exacts `pair`, `higher` et `extra_shell`, le merge de plateaux et le reducer jusqu'aux dix ordres. Le temps froid du tableau inclut au contraire PDEL et le sidecar diagnostique, qui ne sont jamais autorisés dans le produit.

## Contrôles de correction GPU

- le rejeu CPU exhaustif compare les 3 598 arcs PDEL du run 257 points : zéro désaccord à $M=128$ et $M=256$;
- la fixture E5 observe une permutation source--canonique non identitaire et des égalités de distance binary64 : zéro désaccord sur 20 arcs;
- les histogrammes et les seuils sont invariants entre $(M,W,B)=(128,128,17)$ et $(128,256,257)$;
- le passage de $M=128$ à $M=256$ conserve tous les seuils capturés et ferme la censure restante des arcs;
- Compute Sanitizer rapporte zéro erreur et zéro fuite sur 257 points, 16 lots de requêtes;
- le kernel déclare 4 096 octets de stockage local par thread, zéro spill explicite selon `ptxas`, et le binaire contient seulement trois cubins `sm_120`, sans PTX;
- le checker refuse les permutations, CSR, transcripts, drapeaux de complétude, tailles de lot, règles $W\geq M$, claims exacts/publics et partitions support 2/3 invalides.

Le binaire qualifié a le SHA-256 `66e58085c73a34f07f4531a391299e1a9391e6044e8886f7e4a88fe6651c49d7`. La G4 était une RTX PRO 6000 Blackwell Server Edition, compute capability 12.0, pilote 580.173.02 et CUDA 12.9.

## Décision produit sans Delaunay

Geogram PDEL demeure uniquement l'oracle massif hors ligne. Le produit ne doit ni le lier, ni l'appeler, ni relire ses résultats. Les seuils observés sélectionnent une première passe rapide : `symmetric_union_star` avec $M=\lceil4k\ln n\rceil$, ou $M=\lceil5k\ln n\rceil$ si le même budget doit couvrir aussi les variantes dirigée et mutuelle. Toute frontière résiduelle est ensuite traitée exactement; elle ne peut jamais être assimilée à un succès statistique.

La structure retenue est un LBVH partagé et résident, puis une frontière bloc--bloc plate entièrement GPU par vagues `count -> scan -> emit`. Les propositions binary64 ne deviennent décisions qu'après recertification; un cap dépassé ou un reçu refusé échoue fermé. À $k=1$, la spécialisation doit terminer l'EMST exact et chaque ronde Borůvka avec `frontier_empty=true`. À $k=2$, elle doit émettre les événements `pair`, `higher` et `extra_shell`, fermer égalités de coque, triangles droits et cosphères, puis fusionner les plateaux avant le reducer sparse. Aucun callback par produit, transfert de terminaux par vague, tableau $n\times M$, catalogue global de triangles ou Delaunay n'appartient à cette route.

## Artefacts scellés

Les artefacts `phase15_gabriel_neighbor_rank_*_g4_509a9c6.{json,txt}` de ce répertoire contiennent les smokes, l'invariance, le memcheck, les runs 50 k, 1 M, 10 M et 30 M et leurs profils `/usr/bin/time`. Les SHA-256 des quatre artefacts massifs $M=256$ sont respectivement `2bc53d393bc55c6fe519a78e3b7f81191afd16ce9585be0996c96d1485274a2e`, `e26205c026f489334e72b9fcdd17d0b92d8837464c4205`, `ab92644d4e580da4ae8d2983ac86b50819079ec27b8201098991903659d991e2` et `36347423747c767f8b30c20181eff8d6ddc20fb386312206731ae5deb7c56930`.

La session GCP gardée ciblait `devpod-gpu-exploration / europe-west4-ai1a / ehgp-blackwell-spot-ai1a`, génération exacte `2026-07-28T08:32:30.735-07:00`, `g4-standard-48`, `SPOT`, action `STOP`, durée GCE 3 600 secondes et arrêt invité 45 minutes. Après rapatriement, cette cible est certifiée `TERMINATED`, aucune autre VM `project=e-hgp` n'est active et la clé OS Login éphémère a été révoquée puis supprimée localement.
