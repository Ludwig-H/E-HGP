# E-HGP — hiérarchies K-NN d'ordre supérieur

E-HGP est un projet de recherche consacré au calcul de la **hiérarchie complète des amas de forte densité K-NN**. L'objectif n'est pas de produire une partition plate, ni d'échantillonner un champ sur une grille, mais de représenter les composantes connexes des multicovertures

$$L(k,t)=\left\lbrace y\in\mathbb{R}^{d}:D_k(y)\leq t\right\rbrace,$$

où $D_k(y)$ est la $k$-ième distance euclidienne au carré ordonnée, simultanément pour les ordres $1\leq k\leq K_{\max}$. Cette bifiltration ordre–échelle prolonge le Single-Linkage : pour $k=1$, sa structure mince est l'arbre minimum couvrant euclidien ; pour $k>1$, les objets porteurs deviennent des régions témoins et leurs événements critiques.

> [!IMPORTANT]
> Le dépôt contient aujourd'hui des implémentations historiques et le backend expérimental `PowerCover3D`. Les deux backends décrits ci-dessous constituent la **nouvelle cible scientifique** ; ils ne sont pas encore implémentés. Les documents mathématiques précisent ce qui est démontré, conditionnel ou encore conjectural.

## Deux backends, une même sortie hiérarchique

| régime | backend cible | structure mathématique | statut de sortie visé |
|---|---|---|---|
| dimension petite et fixée, notamment 3D ; données massives | `MorseHGP3D` | sphères critiques classées, descente K-NN–miniball, hyper-Kruskal gradué | HGP dur exact lorsque prédicats, catalogue, attaches et incidences de lots sont fermés ; mode rapide explicitement conditionnel sinon |
| grande ou très grande dimension | `HomogeneousLensTower` | atlas de blocs top-$K_{\max}$, continuation DTM $q=1$ vers HGP $q=\infty$, Borůvka certifié par coupes | exact sur l'atlas ; exact globalement sur un périmètre condensé seulement sous $\pi$-complétude certifiée |

Les deux backends doivent produire le même contrat abstrait, `GradedMergeForest` :

- une forêt de fusion $T_k$ pour chaque ordre ;
- les niveaux de naissance et de fusion en unités d'entrée ;
- les morphismes verticaux $T_{k+1}\to T_k$ induits par $L(k+1,t)\subseteq L(k,t)$ ;
- des lots déterministes pour les événements de même niveau ;
- un statut séparant `exact`, `atlas_exact` et `conditional`, avec un périmètre distinct `full` ou `condensed(pi)`.

La hiérarchie est le produit principal. Une partition, une condensation par persistance ou une affectation des points ne sont que des vues dérivées.

## Voie 1 — 3D massive et faible latence

`MorseHGP3D` vise notamment $K_{\max}=10$ sur des nuages 3D d'environ $50\,000$ points. Un unique catalogue de sphères critiques doit servir tous les ordres : une sphère de rang fermé $s$ porte la naissance d'ordre $s$ et, pour $s\geq2$, l'événement d'indice 1 d'ordre $s-1$.

La voie retenue est `PowerCascadeH0` :

1. représenter chaque domaine top-$k$ par un **diagramme de puissance ordinaire en 3D** sur les sites barycentriques pondérés, sans rhomboid tiling ni mosaïque de Delaunay d'ordre supérieur matérialisée ;
2. engendrer les sites de l'ordre suivant par la récurrence incrémentale, puis fermer chaque cellule par séparation top-$k$ exacte à ses sommets ;
3. extraire les supports de tailles deux à quatre depuis les faces, arêtes et sommets fermés, puis certifier centre, rang et niveau ;
4. certifier les attaches par descente K-NN–miniball sous les hypothèses explicites du résultat B ;
5. traiter les événements par niveau avec une variante graduée de Kruskal.

Le test aux sommets est le point nouveau décisif : la différence de deux puissances est affine, donc l'absence de colonne violatrice à tous les sommets d'une cellule bornée certifie la cellule contre l'ensemble implicite des $\binom{n}{k}$ labels. La complétude du catalogue exige en plus les classes entières d'ex æquo et l'appariement des incidences internes. La procédure termine en temps fini, mais sa borne de pire cas reste combinatoire ; un arrêt budgété doit donc publier `conditional`.

La cible « moins d'une seconde » est une cible de benchmark chaud, données déjà résidentes sur un GPU puissant. Elle ne sera évaluée que dans les régimes où le certificat entier — cellules, incidences et sommets séparés — reste mesuré proche du linéaire ; aucune complexité sous-seconde universelle n'est annoncée.

## Voie 2 — grande dimension

Une structure globalement exacte et toujours sparse est impossible sans hypothèses sur les données. `HomogeneousLensTower` adopte donc une sortie honnête : exacte sur un atlas adaptatif et globalement exacte seulement pour un périmètre condensé dont les événements, attaches, coupes et porteurs verticaux sont certifiés complets.

La régularisation principale est la lentille homogène

$$F_{k,q}(y)=\left(\frac{1}{k}\sum_{j=1}^{k}a_j(y)^q\right)^{1/q},\qquad 1\leq q\leq\infty,$$

où $a_j(y)$ est la $j$-ième distance au carré ordonnée. Elle relie exactement la DTM empirique au carré, $q=1$, au champ HGP dur, $q=\infty$. L'entropie régularise les comparaisons locales et fournit des noyaux batchables ; elle ne crée pas, à elle seule, la sparsité globale.

Le backend doit partager les blocs top-$K_{\max}$ entre tous les ordres, matérialiser leurs faces à la demande, poursuivre $q$ vers l'infini et certifier les niveaux minimax par coupes fondamentales. UMAP ou un index ANN peuvent proposer des blocs, jamais certifier seuls la hiérarchie.

## Résultats mathématiques structurants

Les cinq annexes suivantes remplacent la liste de problèmes ouverte du rapport du 14 juillet :

| résultat | objet | rôle dans le projet |
|---|---|---|
| [A — Lentilles homogènes](docs/math/RESULTAT_A_LENTILLES_HOMOGENES.md) | interpolation KL, convexité, bifiltration et swaps | relaxation commune DTM–HGP |
| [B — Descente atomique](docs/math/RESULTAT_B_DESCENTE_ATOMIQUE.md) | terminaison, chemins sous-niveau et attaches | oracle local commun aux deux régimes |
| [C — Complexe de Morse bifiltré](docs/math/RESULTAT_C_COMPLEXE_MORSE_BIFILTRE.md) | sphères classées, événements et morphismes verticaux | définition de la sortie exacte |
| [D — Énumération 3D par puissance](docs/math/RESULTAT_D_ENUMERATION_3D_POWER_GPU.md) | construction incrémentale et certificat de fermeture | cœur géométrique de `MorseHGP3D` |
| [E — Récupération condensée par coupes](docs/math/RESULTAT_E_RECUPERATION_CONDENSEE_COUPES.md) | certificats minimax et branches persistantes | contrat global de `HomogeneousLensTower` |

Voir aussi l'[index général de la documentation](docs/README.md), l'[index mathématique](docs/math/README.md) et le [rapport de synthèse ordre–échelle](RAPPORT_HGP_ORDRE_ECHELLE_DESCENTE_GPU_2026-07-14.md).

## Principes de conception désormais retenus

- La sortie reste hiérarchique pour tous les ordres, y compris lorsqu'elle est condensée.
- Le cas $k=1$ reste exactement ancré par l'EMST ; aucune régularisation ne doit le déformer.
- La sparsité provient d'un catalogue d'événements, d'un atlas ou de certificats de coupes, pas du lissage entropique seul.
- Le nouveau backend 3D n'utilise ni grille uniforme ni mosaïque d'ordre supérieur comme structure de production.
- Le backend haute dimension distingue toujours exactitude sur atlas et exactitude globale.
- Toute accélération ANN reste une couche de proposition suivie d'un reranking ou d'un certificat.
- Les prédicats géométriques, les niveaux égaux et les dégénérescences font partie du contrat mathématique.

## État du code

| chemin | rôle actuel |
|---|---|
| [`perg_hgp/`](perg_hgp/) | package Python actif, atlas historique et backend expérimental `PowerCover3D` |
| [`perg_hgp/POWER_COVER_3D.md`](perg_hgp/POWER_COVER_3D.md) | contrat du backend cubique actuel |
| [`E-HGP/`](E-HGP/) | implémentation E-HGP antérieure |
| [`HGP-old/`](HGP-old/) | prototypes historiques |
| [`PERG-HGP/`](PERG-HGP/) | spécifications et expérimentations antérieures |

`PowerCover3D` reste une référence utile pour l'infrastructure CUDA, les budgets mémoire et les audits numériques. Son champ régularisé puis échantillonné sur grille n'est toutefois pas l'algorithme mathématique cible décrit ici.

## Installation et validation du code existant

Le chemin CPU de `perg_hgp` s'installe avec :

```bash
pip install -e perg_hgp
```

Pour l'atlas historique et les tests :

```bash
pip install -e 'perg_hgp[test]'
PYTEST_DISABLE_PLUGIN_AUTOLOAD=1 python -m pytest -q perg_hgp/tests
```

La pile CUDA actuelle repose sur CuPy et RAPIDS. Les nouveaux backends auront leurs propres contrats et suites de tests ; ils ne doivent pas être simulés par une extension silencieuse de `PowerCover3D`.

## Feuille de route immédiate

1. stabiliser les hypothèses et contre-exemples des résultats A–E ;
2. confronter le certificat de fermeture 3D et les lots dégénérés à un oracle exhaustif de petite taille ;
3. chercher un oracle d'exclusion sensible à la sortie $H_0$, afin d'éviter de fermer des cellules inutiles ;
4. figer le schéma `GradedMergeForest`, la condensation verticale et les statuts de certification ;
5. seulement ensuite, spécifier les noyaux GPU des deux backends.

Les benchmarks historiques et audits restent consultables depuis [`perg_hgp/benchmarks/`](perg_hgp/benchmarks/) et les rapports datés. Ils décrivent l'état des prototypes, non les performances futures de `MorseHGP3D` ou `HomogeneousLensTower`.
