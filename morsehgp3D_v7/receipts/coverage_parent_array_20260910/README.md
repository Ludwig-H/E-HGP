# Régression de l'arène des parents du journal FULL

10 septembre 2026. Capture de la gate renforcée par ROOT après l'audit
514038ed. Aucun header produit ni fichier actif modifié par le sous-agent.
Les parents, successeurs, niveaux et contributions sont comparés aux actions
d'entrée, indépendamment des lecteurs de couverture. Le carré K2 vérifie une
fusion à quatre parents, sans naissance diagonale.

Qualification directe stricte C++20 : O2 puis ASan/UBSan avec fuites actives,
codes 0 avec `--selftest`, 2 avec `--unknown`. Snapshot projet construit avant
compilation depuis la dépendance `-MM`, confirmé par les deux fichiers `-MMD`,
puis relecture des SHA des sources actives après les tests. Compilation et
exécution isolées du snapshot. Les headers Boost sont ceux du chemin enregistré
dans les commandes ; les headers système ne sont ni copiés ni déclarés vendus.

Mutant privé exact, substitution unique dans `full_coverage_certificate.hpp` :
`out.parents_.push_back(parent)` devient `out.parents_.push_back(0)`.
Il doit compiler avec les mêmes avertissements stricts, puis rendre code 1
et uniquement le diagnostic `FAIL arena.parent_value`. Aucun macro-switch
produit, aucun changement des sources actives.

`receipt.json` et les onze commandes portent les statuts réels. La capture
portable omet les ELF, dont les SHA restent enregistrés, et conserve seulement
le header modifié du mutant en plus de l'unique snapshot commun.
`python3 -B verify.py` et `python3 -B -O verify.py` vérifient le paquet sans
exécuter de binaire. Le manifeste porte tous les fichiers sauf lui-même ; son
SHA externe doit être conservé lors de la publication.

Pour reproduire dans une copie du dépôt disposant des mêmes dépendances :
`python3 -B record.py --root /chemin/du/depot --output /chemin/neuf/de/capture`.
Le répertoire de sortie doit être inexistant. Les chemins absolus conservés
dans les commandes décrivent l'exécution effectivement observée, pas un nouveau
claim de reproductibilité binaire entre environnements différents.

Portée structurelle seulement : ce delta ne qualifie ni la géométrie, ni les
parents d'un producteur arbitraire, ni la complétude WSPD, ni un contrat de
performance. GCP et CMake non utilisés.
