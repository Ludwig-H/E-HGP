# Contrelecture B — omissions silencieuses et juge q2 de C

23 septembre 2026. Relecture du commit `b05fbf36`, des deux sources
`c_omission_20260923/{omission_tower_probe,q2_sample_judge}.cpp` et des
sorties publiées. Aucun GCP ni nouveau test lourd par B. Il s'agit de
sondes d'audit, non d'une qualification `FULL` ou du contrat.

## Résultat solide, portée exacte

La campagne d'omissions confirme un risque de contrôle réel : une
suppression **artificielle** de clé de rang haut est parfois acceptée
par FULL avec `complete_relative` et change le condensé de la tour.
Pour les q2 de la zone potentiellement aveugle, **10/68** retraits
acceptés changent ce condensé ; pour q3, **26/67**. Un condensé
différent prouve que la sortie mutée diffère du témoin sain. Il ne
prouve pas qu'une telle omission existe dans le générateur courant,
ni que le témoin `complete_relative` est globalement complet :
« tour mutée différente du témoin après omission » est la portée
expérimentale, pas une erreur de production observée. Un condensé
égal ne prouve pas l'identité complète du payload.

Le mode `b13` exige maintenant dans son code de sortie les refus
attendus ; la campagne inclut K10. La preuve conditionnelle de première
cofacette pour `p+u≤Kmax` reste distincte de la complétude du
générateur et des omissions mêlées à la couche haute.

Le nouveau juge q2 recense directement la boule diamétrale de chaque
paire `(a,b)` pour un ensemble de sites `a` tirés, en produits scalaires
entiers u18, puis cherche sa coquille dans le catalogue. Le test
géométrique est correct sur le domaine déclaré : le produit reste
strictement sous `2^38`, et une boule canonique dont la coquille
complète est sur la sphère diamétrale avec les antipodes `a,b` a
nécessairement cette même MEB. Il ne réutilise pas la WSPD, les
témoins, ni Pool ; il partage encore l'index et les invariants
`BallData` du produit.

Les six cas publiés ont **204 683 vérifications de paires admissibles
sur 204 683 présentes**, dont 16 506 au rang critique `p=9` à K10,
et une suppression plantée détectée par cas. Ce sont des
**présentations orientées**, pas 204 683 `BallKey` distinctes : une
boule peut être revue depuis plusieurs ancres ou plusieurs paires de
sa coquille. Le mutant enlève la première boule trouvée, pas une clé
`p=Kmax−1` spécifiquement. La « trame entière » est seulement
08/000000 **sans sol**, 39 885 sites, avec 200 ancres (~0,5 %) ; aucun
brut ni autre séquence. Les sources u32le locales ne sont pas hachées
dans ces sorties. Le juge a été compilé contre la source v15
`243373f6` et ses résultats ne sont pas automatiquement ceux du
nouvel ordonnancement v16.

Les scripts de campagne emploient `set -u`, puis écrivent un fichier
`DONE` même si une sous-commande échoue ; leurs sorties publiées
portent cependant `exit=0`. Pour des captures futures, propager le
premier échec (`set -e` ou agrégation explicite), hacher les entrées
et publier le commit/binaire réellement exécuté.

## Prochaine porte prioritaire

Conserver ce juge q2 pour les supports longs, avec nombres de
`BallKey` uniques et mutations ciblées de fin de fenêtre. Le trou de
preuve le plus exposé est **q3 à `p=Kmax−2`** : les omissions plantées
peuvent changer FULL sans refus, tandis que le juge du développeur
choisit seulement des voisins proches. Chercher une porte q3
indépendante et stratifiée sur les supports longs, hors temps de
contrat, avant de qualifier la complétude générale ; ne pas transformer
un balayage exhaustif potentiellement cubique en coût produit.
