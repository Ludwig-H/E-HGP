# Spécification de la v10 : l'objet, tel qu'il est implémenté

28 septembre 2026. Document autonome : il suffit pour lire et juger le code de `morsehgp3D_v10/`. Les documents
de conception détaillés, l'audit de la v9 et la base de connaissances sont cités, pas recopiés.

```text
phase=exploration_v10_hors_registre
backend=cpu_reference
profile=quantized_u18_input_only
public_status=not_claimed
```

## 1. Entrée

Des points à coordonnées entières $0 \leq x < 2^{18}$ par axe (grille 1 mm pour le LiDAR, pas isotrope commun pour
les bancs synthétiques) et des `PointId` u32 arbitraires et uniques. Les points de même position forment un
**site** de poids $w \geq 1$ ; l'indice de site est le rang de Morton de la position. Le catalogue accepte les poids ;
la tour refuse encore explicitement une entrée pondérée (sémantique pondérée à écrire).

## 2. Objet

Pour $k = 1, \ldots, K_{\max}$ et $a \geq 0$, soit $D_k(y)$ la $k$-ième plus petite distance carrée de $y$ aux points
(multiplicités comprises) et

$$L_k(a) = \lbrace y \in \mathbb{R}^{3} : D_k(y) \leq a \rbrace .$$

La **tour FULL** est, pour chaque $k$, l'arbre de fusion des composantes de $L_k(a)$ quand $a$ croît (naissances,
multifusions N-aires jamais binarisées, plateaux atomiques par niveau exact), avec les applications verticales
$L_{k+1}(a) \subseteq L_k(a)$. Les niveaux sont des rayons carrés rationnels exacts. Le théorème 2 du manuscrit
identifie ces composantes à celles du graphe $\Gamma_k(a)$ des $k$-parties de rayon minimal $\leq a$, reliées quand
leur union de $k+1$ points a un rayon minimal $\leq a$ ; c'est l'oracle exhaustif des petits nuages.

## 3. Catalogue critique (`src/catalogue/`)

Une boule critique a un support $S$ de 2 à 4 sites affinement indépendants dont le centre $c$ est dans l'intérieur
relatif de l'enveloppe convexe ; $I$ est l'intérieur strict (poids $p$), $U$ la coquille complète (poids $u$),
$q_{\min}$ le plus petit support, $S^{*}$ le support canonique (plus petit lexicographique de cardinal $q_{\min}$),
qui identifie la boule. Elle est admise pour les ordres $\leq K$ si $p + q_{\min} \leq K + 1$ (coquille sans site
pondéré ; sinon le sur-ensemble sûr $p \leq K - 1$).

**Génération par boîtes de centres.** Chaque boule est énumérée dans l'unique feuille demi-ouverte d'un octree de
centres qui contient son centre. La liste d'une feuille contient la boule $K$-NN fermée de tout point de la feuille :
un site n'y reste que si l'une de $K$ gardes n'est pas strictement plus proche partout dans la boîte (lemme G) et si
moins de $K$ dominateurs sont strictement plus proches partout (lemme D). Le recensement local d'une boule
d'intérieur de poids $< K$ est alors exact (théorème C). Aucune décision flottante ; racine = cube dyadique
minimal contenant les sites ; stagnation comptée seulement sous le pas de grille ; feuille trop large = refus
`resource_exhausted/wide_leaf`. Conception : `GEN_v1`/`GEN_v2` (lentille L13 de l'audit).

## 4. Tour (`src/tower/`)

Pour une boule et un ordre $k$ avec $p < k \leq p + m$ ($m$ = sites de la coquille), $t = k - p$ :

- aucun sous-ensemble de $U$ de taille $t$ n'est **séparable** (le centre n'est pas dans son enveloppe convexe,
  théorème de Gordan) : **naissance** d'une composante au niveau de la boule ;
- sinon les **morceaux locaux** (composantes du graphe $A \sim A'$ si et seulement si $A \cup A'$ est séparable)
  sont **joints** au niveau de la boule.

Chaque morceau est représenté par une $k$-partie $I \cup A$ rattachée à une naissance par **descente** : miniboule
exacte de la partie, puis saut aux $k$ plus proches du centre s'il y a au moins $k$ sites strictement intérieurs,
arrêt à la cellule (boule, $k$) si la miniboule est une boule du catalogue dont la fenêtre contient $k$ (cellule
résolue une fois), sinon un représentant de morceau ; le niveau décroît strictement à chaque pas. Un Kruskal par
plateaux exacts construit la forêt de chaque ordre, avec exactement une racine. Les ordres sont indépendants à
catalogue donné.

## 5. Hiérarchie de points et tête (`src/points/`, `src/head/`)

Un site $x$ entre dans l'ordre $k$ au niveau $D_k(x)$ et appartient à la composante de $L_k(D_k(x))$ qui le
contient : la descente de ses $k$ plus proches voisins donne une naissance dont l'ancêtre à ce niveau fermé est le
nœud de $x$ (hiérarchie **C∩X**, distance-cœur d'HDBSCAN avec `min_samples` $= k$, mais connexité de la
multicouverture exacte). La tête condense exactement comme HDBSCAN (Campello, Moulavi, Sander 2013) sur des fusions
N-aires, sélectionne par excès de masse ou par feuilles, avec l'échelle $\lambda = r^{-z}$. HDBSCAN lui-même n'est
jamais réimplémenté : les bancs appellent `sklearn.cluster.HDBSCAN`.

## 6. Portes

| Porte | Ce qu'elle établit |
| --- | --- |
| `mhgp10_unit` | entiers larges contre un juge décimal, ordonnanceur, statuts, tampons, nuage, requêtes exactes |
| `mhgp10_catalogue_oracle` | catalogue égal à l'oracle brut exact (grilles cosphériques, coplanaires, génériques, multiplicités, extrêmes u18) : enregistrements entiers, $S^{*}$ canonique dans l'ordre de Morton, niveaux exacts (`--dump-levels`), rang dense depuis 0, poids et drapeaux ; invariance par translation ; mutants systématiques tués |
| `mhgp10_tower_oracle` | forêts et partitions C∩X égales à l'oracle $\Gamma_k$ à tous les niveaux critiques, coupes ouvertes et fermées, E5 compris ; témoins de naissance (`--dump-births`) : bijection des nœuds vivants sur les composantes de $\Gamma_k$ et verticales jugées à chaque naissance et fusion, même sans point entré ; mutants verticaux systématiques tués |
| `mhgp10_*_oracle_mutant_*` | quinze mutants gravés des deux juges (verticale de l'audit, descendant, image née après, fusion, image absente, témoins ; $S^{*}$ non canonique, niveau, rangs, drapeaux, poids, admission pondérée) : fixture acceptée, mutant rejeté, code 4 |
| `mhgp10_head_condensation_vs_sklearn` | condensation égale à sklearn là où les objets coïncident ($K = 1, 2$ sur l'atteignabilité mutuelle) |
| référence Python | `reference/test_ref.py` : tour par minima, morceaux et descente contre $\Gamma_k$, dégénérescences comprises |

À l'échelle : le catalogue de la trame LiDAR 08/000200 sans sol égale celui de la v9 (1 407 885 boules à K5,
5 483 320 à K10, comptes par $(q_{\min}, p)$ identiques) ; chaque ordre de la tour a une seule racine.

## 7. Ce qui n'est pas établi

- Borne de pire cas du générateur (la sortie peut être $\Omega(n^{2})$ ; linéarité mesurée seulement).
- Multiplicités dans la tour ; invariant d'Euler pondéré.
- Contrats de temps LiDAR (1 s, 100 ms) : non mesurés sur G4 pour la v10.
- Supériorité du clustering sur HDBSCAN : non établie ; la campagne de développement montre pour l'instant une
  parité avec un HDBSCAN réglé sur les mêmes graines.
