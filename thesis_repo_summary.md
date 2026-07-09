# Synthèse de la Thèse de Louis Hauseux et du Dépôt E-HGP

Cette synthèse présente les grandes lignes de la thèse doctorale de **Louis Hauseux** (intitulée *« Structuration automatique de données : clustering hiérarchique, percolation et interactions d’ordre supérieur »*) et analyse les développements et corrections de performance de la bibliothèque **PERG-HGP** (Phase 5).

---

## 1. Résumé et Contributions de la Thèse

Le manuscrit de thèse de Louis Hauseux traite du problème de la **structuration automatique de données** à partir d'observations locales, imparfaites et bruitées. Il se divise en trois parties majeures :

### Partie I : Du Single-Linkage à ses fondements
* **Introduction au Single-Linkage** : Analyse sous trois angles : heuristique (construction de l'arbre minimum couvrant / MST), géométrique (persistance de graphes géométriques) et statistique (estimation des amas de forte densité selon le modèle de Hartigan avec l'estimateur des $K$-NN).
* **Limites du Single-Linkage** : Le Single-Linkage souffre de l'**effet de chaînage** en dimension $p \ge 2$, où le bruit ou des « ponts » de faible densité fusionnent prématurément des amas distincts. Bien que des extensions (Robust Single-Linkage, DBSCAN, HDBSCAN) introduisent une condition de densité locale d'ordre $K$ via les voisins, elles continuent de propager la connexité à l'ordre $1$ (connexité binaire par des arêtes), ce qui conduit à une inconsistance sur le plan statistique.
* **Cadre unifié & applications** : Formalisation convexe du partitionnement et applications pratiques (détection d'anomalies LiDAR 3D avec Naval Group, segmentation d'instance 4D sur SemanticKITTI).

### Partie II : La généralisation du Single-Linkage avec des interactions d'ordre supérieur (HGP)
* **Hypergraph Percolation (HGP)** : La contribution centrale est l'algorithme **HGP-Clusterer**. L'idée clé est de remplacer le graphe géométrique classique par le **complexe de Čech**. Pour $K \ge 2$, les objets élémentaires ne sont plus des arêtes (interactions par paires) mais des $(K-1)$-simplexes codant l'intersection simultanée de $K$ boules.
* **Équivalence mathématique** : Il est prouvé qu'il existe une correspondance exacte entre les composantes connexes des $K$-polyèdres de Čech et les amas de forte densité de l'estimateur $K$-NN (modèle de Hartigan) à tous les niveaux de la hiérarchie.
* **Outils d'analyse continue** : La percolation continue est utilisée pour étudier la vitesse de percolation et comparer la robustesse de HGP à celle de DBSCAN et du Robust Single-Linkage.
* **Mosaïque de Delaunay d'ordre $K$** : En géométrie algorithmique, l'auteur démontre que la mosaïque de Delaunay d'ordre $K$ contient toute l'information nécessaire pour construire le $K$-arbre minimum couvrant de manière efficace en espace euclidien.

### Partie III : Extension aux données structurées
* **Graphes signés et communautés** : Proposition de nouvelles dynamiques bayésiennes de clusters (généralisant l'algorithme de Swendsen-Wang) basées sur des interactions d'ordre supérieur (triangulaires) pour la détection de communautés, avec des garanties théoriques issues de la percolation.
* **Traitement d'images** : Extension du filtre de Frangi classique à un hypergraphe spatial afin d'extraire de manière robuste des réseaux de fissures géologiques ou thermiques, la connexité d'ordre supérieur bloquant efficacement la percolation du bruit.

---

## 2. Structure et Organisation du Dépôt

Le dépôt contient deux implémentations complémentaires de la percolation d'hypergraphes régularisée par l'entropie, ciblant des échelles différentes :

```
/workspaces/E-HGP
├── E-HGP/                        <- Package standard e_hgp (Scikit-Learn compatible, CPU/GPU)
│   ├── e_hgp/
│   │   ├── clustering.py         <- Calcul de poids, condensation d'arbre et étiquetage
│   │   ├── estimator.py          <- EHGPClusterer (API scikit-learn standard)
│   │   ├── gabriel.py            <- Certification de Gabriel
│   │   ├── lazy_boruvka.py       <- Recherche du MST via Borůvka paresseux
│   │   ├── miniball.py           <- Calcul de rayon de Čech (weighted power miniball)
│   │   └── regularization.py     <- Régularisation de Gibbs et entropie locale
│   └── tests/
│       └── test_e_hgp.py         <- Test unitaire d'intégration d'E-HGP
│
├── perg_hgp/                     <- Package haute-performance PERG-HGP (Point clouds massifs)
│   ├── perg_hgp/
│   │   ├── cofaces.py            <- Extraction de cofaces et solveur miniball par batchs PyTorch
│   │   ├── config.py             <- Configuration et hyperparamètres
│   │   ├── dual_graph.py         <- Construction des facettes et arêtes duales (memmap/out-of-core)
│   │   ├── estimator.py          <- PERGHGPClusterer (API scikit-learn pour nuages 3D géants)
│   │   ├── gabriel.py            <- Filtres Gabriel locaux et globaux sur grille
│   │   ├── grid.py               <- Grille spatiale 3D avec Morton sort CPU/GPU & PyTorch KNN
│   │   ├── hierarchy.py          <- MST Borůvka parallélisé sur GPU, Union-Find et condensation
│   │   ├── local_gibbs.py        <- Régularisation Gibbs / entropie en PyTorch vectorisé
│   │   ├── rank_field.py         <- Calcul soft du champ de rang (programmation dynamique)
│   │   └── witnesses.py          <- Gestion des témoins critiques (WitnessPool)
│   └── tests/
│       └── test_perg_hgp.py       <- Suite de 15 tests unitaires pour PERG-HGP
```

---

## 3. Rapport d'Audit & Optimisations de la Phase 5 (Scalabilité Réelle)

Pour atteindre les objectifs cibles de $N = 30\text{M}$ points et d'ordres $K = 10\text{ à } 20$ dans les limites de ressources d'un environnement Colab standard (12 Go de RAM système, 15 Go de VRAM), plusieurs goulots d'étranglement majeurs ont été résolus.

### 3.1 Fusion de KNN + Gibbs en Streaming (Élimination de RAM Globale)
* **Problème** : Stocker globalement les voisins de $30\text{M}$ points avec $m_{local} = 128$ requiert $\sim 43\text{ Go}$ de RAM rien que pour stocker les indices et distances, provoquant des crashs OOM immédiats.
* **Solution** : Fusion complète de la recherche KNN et du calcul Gibbs local en un unique pipeline de streaming par blocs. La grille KNN est interrogée par paquets de $500\,000$ points, et les sites régularisés $(Z, a)$ sont calculés à la volée avant de libérer les voisinages de la mémoire. **Aucun tableau de taille $N \times m_{local}$ n'est jamais matérialisé en RAM**.

### 3.2 Chunking Interne de `query_knn_grid` (Protection VRAM)
* **Problème** : Avec un chunk externe de $500\,000$ requêtes, le déploiement des voisinages de cellules (rayon $\ge 7$) vectorise sur un grand nombre d'offsets ($500\,000 \times 675 \approx 337\text{ millions}$ d'éléments), saturant la VRAM GPU (pics $> 27\text{ Go}$).
* **Solution** : Implémentation d'un sous-chunking interne de taille $50\,000$ directement au sein de `query_knn_grid`. Les calculs de distances locales et d'offsets sont partitionnés pour maintenir l'empreinte VRAM active strictement inférieure à **$1.5\text{ Go}$**, tout en conservant une vitesse d'exécution optimale sur GPU.

### 3.3 Résolution Locale Adaptative par Extension du Rayon (Réduction du Fallback)
* **Problème** : À une densité typique de $30\text{M}$ points, un rayon maximum fixe de $8$ ne permet de visiter qu'environ $94$ voisins en moyenne. Pour $m_{local} = 128$, cela force $100\%$ des requêtes internes à échouer au certificat de queue et à basculer en fallback global (extrêmement coûteux).
* **Solution** : Augmentation du rayon de recherche maximal local à `max_radius = 24`. La boucle s'arrête de manière anticipée dès que le certificat est validé. Pour les points internes, la zone couverte permet de certifier localement la quasi-totalité des requêtes, faisant tomber le taux de fallback global à **$\sim 0\%$**. Les requêtes externes ou isolées restantes bénéficient toujours du fallback double-chunké exact.

### 3.4 Convergence Dynamique du MST de Borůvka
* **Problème** : L'algorithme Borůvka utilisait une borne fixe de 25 phases. À l'échelle de $42\text{M}$ de facettes, la borne théorique $\lceil\log_2(V)\rceil$ peut dépasser 25, ce qui risquait de générer une forêt incomplète ou déconnectée.
* **Solution** : Calcul dynamique du nombre maximal de phases à chaque exécution : `max_phases = max(25, int(np.ceil(np.log2(max(2, num_facets)))) + 2)`.

### 3.5 Correction des Diagnostic Queries lors des Retours Anticipés
* **Problème** : Lorsque la certification de Gabriel ne produit aucune coface (par exemple sur des données jouets bruitées ou en raison de seuils trop restrictifs), le dictionnaire d'exactitude retourné par l'estimateur ne contenait pas les indicateurs KNN, ce qui rompait le monitoring de la grille.
* **Solution** : Intégration des métriques `total_knn_queries`, `total_fallback_queries` et `global_fallback_rate` au sein de l'exactness report du retour anticipé de Gabriel.

---

## 4. Statut d'Exécution et Tests de Non-Régression

Tous les tests de la suite officielle passent avec succès sur l'environnement local :
* **Package `E-HGP`** : $1$ test validant le comportement de base sur CPU.
* **Package `perg_hgp`** : $15$ tests validant l'active-set miniball, l'équivalence exacte du KNN avec le scan brute-force, le streaming out-of-core de facettes, et le MST Borůvka.

```text
pytest /workspaces/E-HGP/E-HGP/tests /workspaces/E-HGP/perg_hgp/tests
============================= 16 passed in 30.68s ==============================
```
