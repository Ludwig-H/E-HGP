# Recoupement natif borné : la bande à K fixé ne commute pas avec les verticales

Nuage colinéaire `[(0,0,0),(1,0,0),(2,0,0),(6,0,0),(9,0,0)]`, IDs `0..4`,
`η = 1/8`, coupe fermée de rayon `r = 3`, donc `β = 9`.
L'exporteur existant construit les ordres `1..3` avec un catalogue servant K3.
Le contexte réel `ArmContext`, avec recalcul exact de `q_min`, applique ensuite
la bande propre à chaque ordre. Aucun fichier produit ni build n'est modifié.

| Ordre | Dates d'entrée β par ID | Partition de points à β = 9 |
|---|---|---|
| K2 | 1/4, 1, 1/4, 9/4, 9/4 | {0,1,2}, {3,4} |
| K3 | 1, 1, 1, 25/4, 49/4 | {0,1,2,3}, {4} |

La composante K3 vivante à β = 9 a pour image verticale la composante K2 gauche.
Le point d'ID 3, de coordonnée x = 6, est pourtant déjà ancré dans la composante
K2 droite. Les deux blocs {0,1,2} et {0,1,2,3} s'emboîtent, mais le bloc K2
{3,4} croise {0,1,2,3}. Ce résultat ne contredit pas la laminarité à K fixé :
il montre que cette projection indépendante par ordre ne fournit pas une
hiérarchie laminaire commune à tous les K et ne commute pas avec la verticale
FULL sur les points. Cela se produit ici sans ambiguïté de première couverture
pour le point x = 6 ; le problème dépasse le départage des seuls ex aequo.

Le script construit indépendamment Γ à partir de **toutes** les K-parties et
de toutes leurs unions Johnson adjacentes de taille K+1. En dimension 1, leur
rayon carré MEB est exactement `(max x − min x)²/4`, calculé avec `Fraction`.
À 20 coupes couvrant tous les niveaux critiques K2/K3 et les entrées, chaque
composante FULL native correspond à exactement une composante Γ, et toutes
les composantes Γ sont représentées. La laminarité de chaque projection dans
son propre ordre est également vérifiée. Le contrôle vertical utilise les
images `lower` natives après les activations à la coupe fermée.

Les exécutions Python normale et `-O` produisent les mêmes dates, témoins,
partitions et exports natifs exacts. Les deux reçus enregistrent 432 fichiers
de dépendance inchangés avant/après : sources du build comparées aux pins du
reçu développeur, fichiers `.o.d` et tous leurs chemins, objets, archive,
exporteur, compilateur et paramètres CMake, Python et sources du consumer.
Le binaire est celui du build existant identifié dans le reçu développeur,
SHA256 `3b6820fb5d6fc07b0cccbe33b99f0e4e6a4aa2c8264a983571703bc833e43972`.
Ce contrôle hérite de sa capture de compilation ; il ne prétend pas refaire
cette compilation. Les quatre sources du consumer/exporteur sont conservées
ici depuis le commit `4b7d70422` et hachées.

`normal/receipt.json` et `optimized/receipt.json` conservent les arguments,
sorties natives, métadonnées, niveaux, parents, témoins sélectionnés et la
fermeture des dépendances. `comparison.json` vérifie leur accord matériel ;
`SHA256SUMS` ferme les artefacts locaux. Les exports JSON permettent une
relecture hors ligne de ce résultat. Un nouveau replay natif exige les
dépendances externes explicites enregistrées dans les reçus et un nouveau
dossier `--out` ; aucun binaire n'est recherché automatiquement.

Limites : une fixture de cinq sites u18, aucune campagne, GCP, mesure de
performance, qualification statistique ou validation condensation/EOM.
Le contre-exemple exhaustif autonome complémentaire est dans le dossier
voisin `intrinsic_cross_k/` ; il ne dépend pas de cet exporteur.
