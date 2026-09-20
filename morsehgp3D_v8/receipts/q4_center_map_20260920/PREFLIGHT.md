# Préflights de la carte de centres

20 septembre2026. Essais avant gel, distincts des captures finales.

Le premier appel de build ciblant la nouvelle gate a retourné
`No rule to make target` : le générateur CMake n'avait pas encore
enregistré la cible ajoutée. Reconfiguration du seul nouveau build,
puis compilation stricte Release réussie. Aucun ancien build touché.

La première gate passe1834 contrôles en Release et Clang ASan/UBSan.
La relecture demande ensuite un compteur strictement « obtus » plutôt
que « non aigu », un domaine à une seule complétion, et des tests plus
ciblés de compression/échec d'allocation. Les captures finales font
autorité après ces ajouts, pas cette première gate.

La gate enrichie passe1956 contrôles dans les deux builds avec le même
JSON. Deux compressions DEEP, quatre allocations fautives et une descente
jusqu'à21 sont exercées ; l'option de profondeur44 ne signifie pas que
toutes ses profondeurs sont positivement atteintes. La compression d'un
parent entièrement hors domaine reste à zéro dans cette gate.

Les préflights des sondes comparent physiquement les sorties au moteur26
et passent le lecteur normal/−O. La carte Positive de profondeur7 ne
gagne que deux familles sur l'adversaire256/K5, aucune à K10.
Ces chiffres de développement ne sont pas une qualification de temps.

Avant gel, trois compteurs structurels ont été ajoutés : indices copiés,
listes non vides créées et agrandissements du tableau. Les copies des
frères jamais interrogés doivent rester visibles dans le coût total.
Ce changement ne modifie pas les décisions géométriques.

Les filtres de toutes les faces sont aussi testés à8k/16k/32k, avec
préfixes spatiaux puis permutation déterministe du jeu complet avant
préfixe. Le traitement résiduel dense n'est pas exécuté ; son minimum
de lectures est annoncé comme projection, pas comme temps de tour.
GCP non utilisé ; les fichiers de l'auditeur restent indépendants.
