# Campagne synthétique variable — plan avant nouveaux scores

Demande utilisateur du27 septembre2026 : données synthétiques, tailles,
difficultés et nombres de groupes variables. Anciennes preuves et sources
figées inchangées. Pas de SIPU ni de LiDAR dans cette campagne statistique.

## Qualité : 17 scénarios, deux nouvelles graines

Le scénario de référence est sphérique, n800, G8, séparation4 écarts-types
isotropes. Les axes sont changés séparément, sans grand produit cartésien :

| Axe | Valeurs | Autres paramètres |
|---|---|---|
| Taille | 400,800,1600 points | Sphérique, G8, δ4 |
| Groupes | 2,4,16,32, plus G8 ci-dessus | Sphérique, n800, δ4 |
| Difficulté | δ2,8, plus δ4 ci-dessus | Sphérique, n800, G8 |
| Forme et densité | Anisotrope, effectifs4:1, variances inégales | n800, G8, δ4 |
| Non-convexité | Anneaux épais, distances entre centres1,5/3/6 | n800, G8, rayon1 |
| Bruit | 10% uniforme, inclus dans n | Sphériqueδ4 et anneauxδ3, n800, G8 |

Les deux graines2026092801/2026092802 sont nouvelles. Les méthodes, cette
grille et le protocole sont fixés avant leurs labels de clustering. Deux
répétitions ne donnent pas une conclusion statistique générale. Si une
méthode est retouchée après lecture, ces graines deviennent du développement ;
une nouvelle confirmation distincte sera nécessaire.

K5 uniquement dans cette première capture ; m20/50 et expZ1/2, profil
principalK5/m20/z1. Sur chaque scène entière : routage exclusif ponctuel,
vote pondéré FULL, première couverture et HDBSCAN à EOM commun, plus
HDBSCAN standard en z1. Soit18 lignes par scène et612 lignes prévues.
Le vote pondéré a un seuil massique, les trois autres un seuil ponctuel :
publier cette distinction et les classes trop petites pour m. Pas de réglage
par vérité terrain, pas de meilleur z choisi par scène, pas de remplissage
du bruit ni de racine sélectionnée dans le protocole commun.

Exemples prévus : n800/G32 donne25 points par classe, donc aucune classe
pure ne peut être un groupe ponctuel sélectionné avec m50 ; les quatre
classes minoritaires du régime4:1 ont40 points. Ce sont des stress de seuil,
pas des motifs d'exclusion. Cette contrainte de cardinalité ne s'applique
pas automatiquement aux labels après vote massique de facettes.

L'anisotropie a des écarts-types2/1/0,5 avec rotations propres ; les écarts-types
isotropes hétéroscédastiques alternent0,5/1/2, sans renormalisation cachée.
δ est une distance de centres en unités du modèle, pas une distance de
Mahalanobis. n inclut le bruit : à n800,10% donne720 points de composantes
et80 points uniformes, sans rejet/rééchantillonnage guidé par labels.

Les classes sont celles du mécanisme de génération, pas nécessairement les
modes de densité observables lorsque les distributions se recouvrent. Le
bruit uniforme peut tomber près d'une composante : sa provenance ne certifie
pas une anomalie géométriquement reconnaissable. Les anneaux sont épaissis
en3D ; ce ne sont ni des données SIPU ni une promesse de séparation parfaite.

Rapporter toutes les scènes, ARI tous points et bruit en singletons, F1
apparié, couverture, nombres de groupes et bruit. Conserver erreurs et
captures partielles explicitement ; ne retirer aucun scénario défavorable.

## Passage à l'échelle : série distincte

Préparer neuf nuages : n8000/16000/32000 pour sphériqueδ4, anisotropeδ4
et anneauxδ3, G8, sans bruit injecté, graine2026092891. Les entrées préparées
ne valent **pas** exécution du pipeline. Mesurer séparément le travail
géométrique, facettes/incidences, arbre source, projection, EOM, mémoire,
durée native et coût complet. Trois répétitions sont requises pour un bilan
de temps ; les séries de qualité n'en tiennent pas lieu.

Conserver le même modèle et une même grille entre tailles. Les flux par
composante sont indépendants de n ; les effectifs et l'identité des tirages
restent publiés. Il ne s'agit pas des coupes capteur du protocole LiDAR.
Les rapports aux doublements doivent porter sur le travail total, pas une
primitive choisie. Même des rapports inférieurs à4 ne prouvent pas une borne
sous-quadratique générale. Aucun contrat GPU/FULL100ms n'est acquis ici.

## Précision et exécution

Les points synthétiques binary64 sont conservés. Avant les mesures, une
grille isotrope u18 commune est déterminée sur toutes les tailles de chaque
série famille/G/δ/bruit/graine, sans accès aux labels. Arrondi rationnel exact,
aucun écrêtage, toute collision refusée et conservée comme échec. Tous les
algorithmes reçoivent les **mêmes sites entiers**, exactement représentables
en float64 pour HDBSCAN. Ce ne sont pas des mètres ni la grille LiDAR1mm.

Sources, entrées, paramètres, binaires, commandes, sorties et fermetures
sont hachés. Réutilisation explicite des primitives qualifiées, mais **aucun
transfert de scores** depuis les anciennes scènes. Toute nouvelle scène
demande un vrai export FULL et de nouveaux fits HDBSCAN. L'ablation du
catalogue non Gabriel reste un chantier séparé : le premier pilote conserve
le catalogue Gabriel actuel, pour ne pas changer deux facteurs à la fois.
