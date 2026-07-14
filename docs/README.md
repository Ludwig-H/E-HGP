# Documentation E-HGP

Ce dossier contient uniquement la documentation scientifique active. Les rapports datés et les prototypes remplacés ont été retirés de l'arbre courant; l'[historique du projet](HISTORIQUE.md) indique où les retrouver.

## Ordre de lecture

1. Le [`README.md` racine](../README.md) fixe l'ambition, les deux régimes de données et le statut actuel du code.
2. L'[architecture mathématique](ARCHITECTURE_MATHEMATIQUE.md) définit la bifiltration, les contrats des deux backends et le programme expérimental susceptible de les invalider.
3. Le [corpus mathématique A–E](math/README.md) sépare les résultats démontrés, conditionnels, conjecturaux et heuristiques.
4. Le [manuscrit de thèse](references/MANUSCRIT_THESE_HAUSEUX.pdf) fournit les fondations HGP, multicovertures et K-arbres couvrants.

## Lecture par régime

### Données massives en 3D

Lire les résultats C, B puis D. La sortie cible est le HGP dur jusqu'à $K_{\max}=10$. La structure de production envisagée est un catalogue classé d'événements critiques, fermé par diagrammes de puissance ordinaires puis réduit par attaches et lots.

### Grande dimension

Lire les résultats A, B, C puis E. La sortie cible est une tour de forêts exacte sur l'atlas courant et globalement certifiée seulement sur le périmètre condensé effectivement fermé.

## Statuts à ne pas confondre

| statut d'exécution | sens |
|---|---|
| `exact` | la hiérarchie annoncée est complète sur le périmètre `full` |
| `atlas_exact` | toutes les décisions sont exactes relativement à l'atlas matérialisé |
| `conditional` | au moins une condition globale de fermeture ou un budget reste non certifié |

Une mesure empirique, un reranking exact des candidats trouvés ou un bon accord moyen ne transforme jamais `conditional` en `exact`.

## Prototype conservé

[`HGP-old`](../HGP-old/) est conservé comme référence historique liée au manuscrit. Le package [`perg_hgp`](../perg_hgp/) et l'[expérience PowerCover3D](../experiments/powercover3d/) sont des références d'ingénierie postérieures. Aucun de ces éléments n'est un backend cible, et leurs performances ne préjugent pas de celles de `MorseHGP3D` ou `HomogeneousLensTower`.
