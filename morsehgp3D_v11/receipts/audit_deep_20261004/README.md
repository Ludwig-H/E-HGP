# Contrelecture approfondie de l'audit — 4 octobre 2026

Cadre : `exploration_v11_hors_registre / cpu_reference /
quantized_u21_input_only / not_claimed`. Deuxième lecture complète au pin
**e02a6c235bc4a706519cdaa15f4b1465a6275eba**, après le
[premier audit](../audit_giant_20261004/README.md). Dernier delta lu :
**8f68622b2181e2537280ca09275363a46f2f34dd**, sans modification de
src/CMake/cmake/bench/tools/tests. Les trois notes actives sont corrigées
en place ; aucun ancien reçu clos ni code de production n'est réécrit.

## Corrections et aide au développeur

**Vitesse : les XYZ v10 et v11 sont identiques**, à grille 1 mm et masque
sans sol identique. u18/u21 n'explique donc pas une différence de résolution
d'entrée. L'écart actuel descriptif est **×1,50–1,72**, contre environ ×6
avant les optimisations. Captures distinctes : passe chaude v10, processus
neufs v11, pas d'A/B causal. Le juge v10 incomplet limite sa qualification ;
il ne démontre pas que sa vitesse provient d'une tour omise. Catalogue et
cinq forêts ont les mêmes cardinalités ; le différentiel canonique intégral
sur ces trames reste à fermer.

Le semis avant MEB, les filtres, la répartition et l'ordonnancement expliquent
du travail évité et des gains de paquets **appariés entre v11**. Deux pistes
résiduelles précises : Level q3 construit avant certains rejets, et partition
des centres T0/T6 différente. Le témoin q3 établit un calcul jeté ; aucun
pourcentage de temps ni gain de subdivision n'est revendiqué. « Réduit »
dans la capsule performance désigne le **degré6**, sans PGCD : le
[supplément](performance_precision/README.md) précise cet encodage et les
bornes à reprendre avant un paramètre T6/u24.

**Mathématiques : B garde le plafond L6 d'H**, sB≤t′+d_k/2 et
0≤sB−e≤d_k/2−D, sous les hypothèses finies/atteintes de H3/B3. Une famille
géométrique montre que le supplément peut approcher d_k/2 ; aucune constante
optimale u21 ni optimalité statistique n'est déduite. Les tests adverses ne
réfutent pas FULL/H3/B3. Ils protègent les cas non réguliers, les dates au-delà
de la naissance de racine, le seuil m distinct de k et le domaine algébrique
des réciproques EOM.

**Concurrence : le chemin normal est confirmé par la revue**, avec scratch
privé, publication par ordre et dépendances vers les ordres inférieurs.
La garde après réveil sur abandon reste nécessaire. Le refus POINTS192 du
tétraèdre u24 valide 196/148 bits est également maintenu. Aucun faux succès
FULL, race native ou nouveau TSan n'est revendiqué.

**Derniers bouts G4 :** lecture des deux sessions closes, 360 bouts puis
31 bouts + cinq démos ; arrêts ciblés certifiés. À k5, les deux meilleurs blocs
de vélos 133/83 sont disjoints, IoU0,964/0,711. Ils peuvent former une
antichaîne ; EOM, coupe commune et score plat restent à vérifier. Le lecteur
de métadonnées vérifie la présence des membres, pas la correction géométrique
de tous les blocs. [Provenance et sorties](LATEST_READ.json),
[vérification du cas concret](math/native_best_blocks.json).

## Couverture, contrôles et limites

[CODE_REVIEW.json](CODE_REVIEW.json) inventorie **101/101 fichiers src**, leurs
lecteurs et copies exactes. Les sept groupes clos sont repris sans mutation :

| Groupe | Objet et portée | Gardes par mode |
|---|---|---:|
| [Fondations](foundations/README.md) | Statuts, propriété, cloud, FULL, 29 sources | Lecture, pas de nouveau test |
| [Concurrence](parallel/README.md) | 1 029 cas de scratch, 668 configurations pipeline | 5 415 |
| [Géométrie](geometry/README.md) | 249 dépendances, q3 différé et bornes | 1 733 |
| [Mathématiques](math/README.md) | Γ/MEB, plafond B, réciproques, membres réels | 931 |
| [Performance](performance/README.md) | Captures, mêmes entrées, comptes et ratios | 279 dérivations + 1 045 contrôles d'intégrité |
| [Précisions](performance_precision/README.md) | Formule q3, subdivisions et garde u24 | Lecture de cinq sources |
| Pins restants | Métadonnées et helper vertical | Lecture de cinq sources |

[RUNS.json](RUNS.json) conserve le rejeu **depuis ces copies** normal/−O,
sorties identiques. Les nombres sont des gardes de modèles/source ou des
contrôles de reçus ; ce ne sont pas des tests natifs ni des preuves générales
tirées de petits exemples. Le premier audit est aussi relu par son lecteur
d'intégrité dans les deux modes, sans relancer ses anciennes campagnes.

Aucun build, test natif, fit, workflow ou GCP lancé pour cette publication.
Les qualifications natives/G4 sont héritées uniquement des lots épinglés et
dans leurs domaines. FULL K10 au contrat, 100 ms, GPU, multi-millions et tête
plate native restent ouverts. Aucun octet brut KITTI n'est ajouté.

`LEDGER.json` inventorie les payloads ; `SHA256SUMS` inclut le ledger et
exclut uniquement sa propre racine. `python3 -B check.py` et
`python3 -B -O check.py` vérifient inventaire exhaustif, hashes de tous les
sous-reçus, 101 copies et égalité des replays. Le lecteur ne lance aucun
produit ni accès réseau. Les archives et chemins LIVE mentionnés dans les
sous-reçus gardent leur portée historique ; les snapshots sont autonomes.
