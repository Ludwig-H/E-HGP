# HGP-HSA pour SemanticKITTI

## Idée

Construire sur les **points 3D bruts** un dendrogramme HGP, single-linkage ou robust single-linkage, puis le condenser avec `min_cluster_size` sans supprimer de points.

Chaque cluster \(C_v\) est représenté par une fonction support échantillonnée dans des directions fixes :

\[
s_v(j)=\max_{x\in C_v}\left\langle u_j,\frac{x-c_v}{R_v}\right\rangle .
\]

Ce vecteur est invariant par translation et changement d’échelle isotrope.

## Modèle

- Les clusters terminaux sont les **tokens**.
- Les clusters internes fournissent les **embeddings hiérarchiques** via leur fonction support.
- La **Hierarchical Self-Attention** parcourt tout le dendrogramme, sans choisir de coupe.
- Les interactions entre deux branches sont partagées par blocs, ce qui impose un biais multi-échelle.

Pour un arbre binaire, le calcul devient linéaire en nombre de nœuds, contre une attention plate quadratique.

## Entraînement

Sur SemanticKITTI :

1. prédire une distribution sémantique pour chaque cluster terminal ;
2. reprojeter cette prédiction sur ses points ;
3. entraîner par KL ou cross-entropy pondérée par la taille du cluster.

## Expérience décisive

Comparer, à architecture identique :

- MLP sur fonctions support ;
- attention plate ;
- HSA avec arbre aléatoire ;
- HSA avec single-linkage, HDBSCAN/RSL et HGP.

Le résultat recherché est de montrer que **la hiérarchie HGP constitue un meilleur biais inductif multi-échelle**, notamment sous amincissement du LiDAR et à longue distance.

## Référence principale

S. Amizadeh et al., *Hierarchical Self-Attention: Generalizing Neural Attention Mechanics to Multi-Scale Problems*, NeurIPS 2025.
