# Fiche Technique — HGP-old (`HGP-clusterer`)

> [!IMPORTANT]
> **Référence historique figée.** Ce document présente la fiche technique de la version historique **HGP-old** (`HGP-clusterer`), développée dans le cadre des travaux de thèse de **Louis Hauseux** (*Utilisation de graphes pour la classification et l'extraction de structures. Généralisation à des interactions d'ordre supérieur*).
> Cette implémentation constitue le précurseur algorithmique du projet `MorseHGP3D`.

---

## 1. Contexte Théorique & Manuscrit de Thèse

L'algorithme **HGP** (*Hypergraph Percolation Clustering*) repose sur les fondements mathématiques développés dans les deux premières parties du manuscrit de thèse de Louis Hauseux :

### Partie I — Modèle de Hartigan & Clustering par Densité
* **Modèle de Hartigan (1975, 1981)** : Définition des clusters à haute densité comme les composantes connexes des ensembles de sur-niveau de la fonction de densité de probabilité sous-jacente $f$ :
$$E(\lambda) = \left\lbrace x \in \mathbb{R}^d \;\middle|\; f(x) \ge \lambda \right\rbrace$$
* **Limites du Single-Linkage Euclidien** : Le Single-Linkage classique souffre de l'effet de chaînage (*chaining effect*), où un faible nombre de points isolés dans des régions de faible densité peut connecter artificiellement deux clusters denses distincts.
* **Généralisation aux interactions d'ordre supérieur** : Pour surmonter cet écueil sans imposer de forme géométrique a priori (comme les k-means ou GMM), HGP étend le concept de graphe de voisinnage aux **hypergraphes géométriques** et complexes simpliciaux (basés sur Delaunay et les $\alpha$-shapes d'ordre $k$).

### Partie II — Percolation d'Hypergraphes & Algorithmique HGP
* **Percolation d'ordre $k$** : L'algorithme étudie la connectivité topologique des simples d'ordre $k$ (paires pour $k=1$, triangles pour $k=2$, tétraèdres pour $k=3$) filtrés par le rayon de leur sphère englobante minimale (miniball).
* **Adjacences et Réduction par $K$-MST** : Démontre l'équivalence entre la multicouverture de boules, les polyèdres de Gabriel d'ordre $k$ et la réduction des composantes non triviales par arbre couvrant de poids minimum d'ordre $k$ ($K$-MST).
* **Splitting dynamique** : Permet de découper (*splitter*) interactivement des clusters sous différents seuils de densité ou d'échelle sans recalculer la triangulation géométrique sous-jacente.

---

## 2. Architecture Logicielle d'HGP-old (`HGP-clusterer`)

Le paquet `HGP-clusterer` est une bibliothèque hybride C++/Cython/Python intégrée avec l'écosystème `scikit-learn`.

### Double Backend Géométrique
1. **Backend CGAL (Référence exactitude)** :
   * S'appuie sur CGAL (`CGALDelaunay`) pour calculer la triangulation de Delaunay d'ordre $k$ en 3D (liftée en 4D pour les puissances/miniballs).
   * Garantit l'exactitude arithmétique grâce aux noyaux exacts de CGAL.
2. **Backend Geogram (Haute performance)** :
   * Conçu pour le calcul multicœur intensif via Intel TBB.
   * **Welzl Zero-Malloc** : Calcul parallélisé des sphères englobantes minimales (miniballs) sans aucune allocation dynamique.
   * **Fast 4D Lower-Hull** : Algorithme simplifié d'extraction du diagramme de puissance pour réduire l'empreinte mémoire et accélérer l'extraction des facettes.

### API & Intégration Python
* Classe principale `HGPClusterer` compatible avec l'API `scikit-learn` (`fit`, `fit_predict`).
* Paramètres principaux :
  * `K` : Ordre des interactions ($K=1$ pour graphe simple, $K=2$ pour triangles de Gabriel).
  * `min_cluster_size` : Taille minimale d'un cluster retenu.
  * `min_samples` : Seuil de densité locale (voisins minimums).
  * `backend` : `'geogram'` ou `'cgal'`.

---

## 3. Transition vers MorseHGP3D

Bien qu'efficace sur des jeux de données modérés, l'architecture d'**HGP-old** a mis en évidence des verrous structurels ayant motivé la conception de **MorseHGP3D** :

1. **Goulot d'étranglement mémoire** : `HGP-old` matérialise la mosaïque de Delaunay d'ordre supérieur complète (dont la complexité mémoire et temporelle peut atteindre $O(n^{d+1})$).
2. **Passage à l'échelle limité** : L'approche ne permettait pas de traiter directement des nuages de 10M à 50M de points sur GPU sans explosion mémoire.
3. **Apport de MorseHGP3D** : MorseHGP3D élimine la matérialisation de la mosaïque de Delaunay globale en calculant directement la hiérarchie de Morse $H_0$ via des structures spatiales creuses LBVH résidantes GPU et des oracles exacts.

---

## 4. Licence & Usage

> [!CAUTION]
> **USAGE NON-COMMERCIAL UNIQUEMENT.**
> Le sous-arbre `HGP-old` est distribué sous sa licence historique propre (usage académique, recherche et éducation uniquement). Tout usage commercial de ce répertoire est strictement interdit. Voir `HGP-old/LICENSE`.
