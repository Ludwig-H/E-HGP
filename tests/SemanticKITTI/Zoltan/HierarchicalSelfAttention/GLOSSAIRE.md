# Glossaire

## Objets et données

**Arbre complet**  
Arbre contenant tous les événements de naissance et de fusion utiles, et non seulement quelques coupes choisies.

**Branche**  
Suite d'états d'une même composante entre sa naissance et sa fusion dans un parent.

**Carrier**  
Réalisation géométrique associée à une cellule ou un nœud. Toujours préciser lequel ; le mot « polyèdre » seul ne suffit pas dans les tests numériques.

**Cellule / facette élémentaire**  
Unité minimale sérialisée et token de feuille du réseau principal.

**Coface**  
Cellule d'ordre supérieur reliant plusieurs facettes. Elle peut devenir un hyperedge dans l'encodeur d'incidences.

**Événement de fusion**  
Lot simultané dans lequel plusieurs branches deviennent un même parent. Il est traité comme un ensemble, sans ordre arbitraire entre enfants.

**Feuille**  
Token minimal de l'arbre. Dans le modèle principal, une feuille est une facette, jamais un point.

**Filtration**  
Famille emboîtée d'objets indexée par un niveau `λ`.

**Hiérarchie polyédrique**  
Feuilles, incidences, composantes et arbre de fusion produits hors gradient.

**Nœud**  
État persistant regroupant un ensemble de feuilles à un intervalle de niveaux.

**Niveau physique**  
Valeur de filtration ayant une interprétation géométrique ou statistique commune entre scans, par opposition à un simple numéro de couche ou nombre de clusters.

**Ordre `K`**  
Ordre d'interaction utilisé pour construire la structure. Les comparaisons multi-ordre restent une extension.

**Persistance**  
Durée de vie d'une branche dans le paramètre de filtration, de préférence mesurée en différence logarithmique lorsque cela est pertinent.

**Reprojection**  
Application linéaire des logits de feuilles vers les points originaux avec poids positifs sommant à un.

**Thinning**  
Suppression contrôlée de retours LiDAR pour simuler une densité plus faible ou une autre configuration de capteur.

## Canaux

**Canal `shape`**  
Descripteurs de forme normalisés : Gram, spectre, support, CDF projetée, moments et masques de dégénérescence.

**Canal `metric`**  
Grandeurs physiques : dimensions, aire/volume, centre, hauteur, pose et portée.

**Canal `filtration`**  
Naissance, mort, persistance, écarts de niveau, rang et croissance de masse.

**Canal `sensor`**  
Rémission, anneaux, couverture angulaire, incidence estimée et masques de retours manquants.

**Fonction support**  
Pour une direction `u`, maximum du produit scalaire avec les points d'un ensemble. Elle résume l'enveloppe convexe, pas la masse intérieure.

**CDF projetée**  
Distribution empirique des projections d'une cellule selon plusieurs directions. Contrairement au support, elle conserve une information sur la masse intérieure.

**Gram normalisé**  
Matrice des produits scalaires entre vecteurs d'une cellule après normalisation. Elle donne un descripteur invariant aux permutations et aux rotations si utilisée par invariants spectraux.

**Masque de dégénérescence**  
Bit indiquant qu'un axe, une normale ou un rapport spectral est mal défini. Le masque évite de présenter une valeur numérique arbitraire comme une mesure fiable.

**RPE — Relative Positional Encoding**  
Encodage d'une relation entre deux tokens, injecté dans les requêtes, clés, valeurs ou logits d'attention.

## Graphes et relations

**Arête horizontale**  
Relation entre cellules ou nœuds d'un même niveau, généralement issue d'une incidence ou d'une proximité d'interface.

**Arête verticale**  
Relation enfant→parent ou parent→enfant dans la hiérarchie.

**Graphe dual**  
Graphe dont les sommets sont les facettes et dont les arêtes représentent leurs incidences ou connexions élémentaires.

**Hyperedge**  
Relation reliant plus de deux tokens. Une coface ou un événement simultané peut être traité comme hyperedge.

**Fratrie**  
Ensemble des enfants d'un même événement de fusion.

**Weighted Jaccard**  
Score de recouvrement pondéré utilisé pour apparier des nœuds de deux vues : somme des minima divisée par somme des maxima des poids d'appartenance.

## Modèles

**PolyTreeFormer**  
Nom de travail de la famille de modèles polyèdre-only. Ce nom n'est pas un claim de nouveauté.

**`PolyTreeFormer-Nano`**  
Premier porteur : adaptation du mode `nano` de Superpoint Transformer à quatre niveaux de la structure.

**`PolyTreeFormer-Full`**  
Modèle cible opérant sur l'arbre complet avec attention parent–enfants, fratrie et arêtes latérales.

**SPT — Superpoint Transformer**  
Architecture hiérarchique de régions 3D. Son mode `nano` retire l'étage point-wise.

**Sequoia-fixed**  
Adaptation de Sequoia où la hiérarchie est fournie par le prétraitement plutôt qu'apprise.

**HSA — Hierarchical Self-Attention**  
Attention dérivée mathématiquement sous contrainte hiérarchique. Utilisée comme opérateur comparatif après les baselines simples.

**AllSet-incidence**  
Extension où les messages passent par les multiensembles d'incidences ou cofaces avec un Set Transformer.

**MeanTree**  
Baseline sans attention : agrégation parent–enfants par moyenne/somme et MLP.

**Inducing tokens**  
Petit ensemble de latents servant d'intermédiaires dans une grande fratrie afin d'éviter une attention quadratique complète.

## Apprentissage

**JEPA — Joint-Embedding Predictive Architecture**  
Cadre auto-supervisé dans lequel le contexte prédit la représentation latente d'une cible plutôt que son entrée brute.

**Range-Hierarchy JEPA**  
Pré-entraînement du projet : un student traite une vue LiDAR dégradée et prédit les embeddings teacher de branches appariées dans la vue complète.

**Teacher EMA**  
Encodeur cible dont les paramètres sont une moyenne exponentielle de ceux du student ; il ne reçoit pas de gradient direct.

**Observable-only**  
Une cible ou une loss n'est calculée que lorsque l'unité est effectivement observable dans la vue student, pour éviter les fuites de position ou d'identité.

**Softmap**  
Distribution douce sur des prototypes, plus riche qu'un indice de prototype dur.

**Anti-effondrement**  
Régularisation empêchant tous les embeddings de devenir identiques : variance minimale, décorrélation ou normalisation contrôlée.

**Channel dropout**  
Masquage aléatoire d'une famille de canaux, notamment `sensor`, pour réduire les raccourcis.

**Linear probing**  
Évaluation où l'encodeur est gelé et seule une tête linéaire est entraînée.

**Fine-tuning**  
Entraînement supervisé de tout ou partie du modèle pré-entraîné.

## Protocole

**Oracle de feuilles**  
Meilleure prédiction possible si chaque feuille connaît sa distribution GT. Il mesure le plafond de la tokenisation et de la reprojection.

**Arbre aléatoire apparié**  
Contrôle dont profondeur, degrés et masses ressemblent à l'arbre réel, mais dont les associations sont aléatoires.

**Niveaux permutés**  
Null test conservant la topologie mais détruisant le sens du paramètre de filtration.

**Graine**  
Initialisation et ordre aléatoires d'un entraînement. Les résultats principaux sont moyennés sur au moins trois graines.

**mIoU**  
Moyenne des intersections-sur-union des 19 classes SemanticKITTI.

**F-score de frontière**  
Mesure diagnostique de précision et rappel des frontières après reprojection.

**mCE / mRR**  
Métriques de robustesse aux corruptions utilisées dans Robo3D : erreur moyenne relative et taux de résilience.

**TTA — Test-Time Augmentation**  
Moyenne ou vote de plusieurs inférences augmentées. Exclue du résultat principal strict.

**Track strict**  
SemanticKITTI uniquement, un scan, LiDAR seul, sans TTA ni ensemble.
