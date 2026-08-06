# Garde-fou structurel Delaunay/EMST à échelle (design minimal)

Directive normative du 6/8/2026 (scellée dans la roadmap) : l'exactitude
n'est vérifiée exhaustivement que sur de tous petits nuages (n=32 suffit).
Aux échelles moyennes et grandes, la vérification devient structurelle,
**externe à l'algorithme** : Delaunay ne doit JAMAIS apparaître dans
Morse HGP 3D lui-même — uniquement dans un vérificateur de comparaison,
et sans y investir un effort disproportionné (Delaunay n'est pas exact
pour k=2 ; c'est un garde-fou, pas un oracle).

## Ce que le garde vérifie

Sur un nuage d'échelle (50k, 1M, …) et la sortie du pipeline industriel :

1. **k=1 (EMST)** : l'arbre couvrant minimal euclidien est un sous-graphe
   de la triangulation de Delaunay. Le vérificateur calcule l'EMST par un
   outil externe (CGAL ou scipy/Delaunay + Kruskal sur les arêtes de la
   triangulation) et exige que la hiérarchie k=1 produite par la méthode
   retrouve exactement cet EMST (mêmes fusions, mêmes niveaux au carré
   rationnels — comparaison sur les paires d'index, pas sur des flottants).
2. **k=2 (garde-fou seulement)** : pour chaque triangle formé de deux
   arêtes Delaunay adjacentes, les clusters d'arêtes (k=2) que ce triangle
   relie doivent fusionner à un niveau **au moins supérieur** au niveau de
   l'arbre k=2 correspondant — un contrôle d'ordre, pas d'égalité :
   toute coupe/fusion manquée par la méthode se manifeste comme une
   violation d'ordre, tandis que les faux positifs Delaunay (non-exactitude
   k=2) ne déclenchent rien.

## Implémentation (bornée)

- Un script externe `tools/delaunay_structural_guard.py` (scipy.spatial
  Delaunay + union-find), consommant le rapport JSON du runner + un dump
  borné des fusions k=1/k=2 (à exposer par un drapeau `--emit-merge-log`
  du runner, sortie proportionnelle aux fusions, jamais de structure
  globale matérialisée dans le moteur).
- Exécution sur G4 uniquement, aux jalons d'échelle (50k, 1M), jamais en
  CI locale.
- Divergence = échec du gate d'échelle, jamais une correction silencieuse.

## Non-buts

Pas de Delaunay dans le moteur, pas d'oracle k≥2, pas de multiplication
de tests (un seul script, deux contrôles), pas de dépendance CGAL/scipy
dans le build C++ (outil externe séparé).
