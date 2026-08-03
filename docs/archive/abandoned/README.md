# Registre des pistes abandonnées

« Abandonnée » signifie ici : interdite comme architecture ou mécanisme de complétude du produit. Une identité locale, une fixture ou un oracle borné issu de la piste peut rester utile.

| piste | raison de l'abandon | trace conservée |
|---|---|---|
| fenêtre Morton fixe ou préfixe fini de voisins comme autorité exhaustive | aucun rayon de fenêtre universel ne garantit toutes les paires ou tous les simplexes utiles | source surrogate et cible retirées du build actif; contre-exemples du registre des preuves; Morton reste un ordre de stockage et de parcours |
| forêt de MST de mutual reachability sur les points comme hiérarchie Morse exacte | à partir de l'ordre deux, elle oublie l'identité des simplexes, des facettes, des cofaces et des incidences qui portent la hiérarchie de Hartigan | [instantané point-MST v6](../../../morsehgp3d/archive/surrogates/point_mst_v6/README.md) retiré de l'API et du build actifs; validateurs, rapports et fixtures falsificatrices conservés |
| fermeture par les seules paires de rang utile | un triangle aigu de rang trois peut avoir ses trois côtés de rang quatre et n'apparaître dans aucun sous-graphe construit depuis les paires de rang au plus trois | fixture `hartigan_triangle_all_side_ranks_above_k.json` et checker exact |
| wedges Delaunay, étoile fermée, carré du graphe et fan de faces pour Gamma$_2$ | fixtures exactes à six et huit points; la correction `one_edge` explose avant déduplication | [rapport de falsification actif](../../validation/PHASE15_DELAUNAY_GAMMA2_FALSIFICATION.md) et [preuve bornée](../../math/DELAUNAY_ORDINAIRE_GAMMA2.md) |
| PDEL/Geogram et flot Gabriel comme chemin produit ou correction | dépendance à une triangulation ordinaire, décisions binary64 partielles, incidences Gamma silencieuses et arènes massives | cibles et source exécutable retirées du build actif; [diagnostic Phase 14](phase14/PHASE14_GEOGRAM_LOW_ORDER_GPU.md), [couverture Gabriel](phase15/PHASE15_GABRIEL_COVERAGE_G4.md), [rangs de voisins](phase15/PHASE15_GABRIEL_NEIGHBOR_RANK_G4.md) et helper épinglé [`tools/build_geogram_phase14.sh`](../../../tools/build_geogram_phase14.sh) conservés hors ligne |
| subdivision `prune-only` pilotée par l'hôte | 99,634 % de paires prunées à 3 125 points, mais 3,229 s à chaud à cause des vagues, revisites et synchronisations | [rapport G4 scellé](phase15/PHASE15_PRUNE_ONLY_FRONTIER_G4.md) |
| parcours stackless relancé par paire, ancre ou callback | coût répété et trafic hôte incompatibles avec 50 k; une frontière résidente commune est requise | artefacts `phase15_pair_rank_*` et note de progression Phase 15 |
| mosaïque de Delaunay d'ordre supérieur, Gamma global ou catalogue cellulaire comme produit | matérialise précisément les cellules, cofaces et incidences que MorseHGP3D doit éviter | oracles exhaustifs bornés de `reference/` seulement |
| tour globale de boules saturées énumérée exhaustivement | exacte sur le papier, mais jusqu'à des univers de supports et memberships incompatibles avec le passage à l'échelle | conservée comme repli de preuve borné dans le [registre de recherche](../../research/README.md) |
| grille, DTM, lissage entropique ou ANN comme définition exacte | modifie la filtration ou ne certifie pas la complétude | historique Git et [historique condensé](../../HISTORIQUE.md) |

## Règle de réouverture

Une piste archivée ne peut revenir dans la voie active qu'avec un nouveau théorème de complétude, une fixture qui falsifie le motif d'abandon sans casser les contre-exemples existants, une architecture sans structure globale interdite et un gate de performance distinct. Un benchmark moyen ou un bon rappel empirique ne suffit pas.
