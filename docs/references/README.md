# Corpus bibliographique local

Ce dossier contient le manuscrit fondateur de HGP et quatre articles dont une copie locale est utile au développement de MorseHGP3D. Le manifeste [references.toml](references.toml) fixe, pour chaque PDF, la version, la source, la licence, la taille et l'empreinte SHA-256.

> [!IMPORTANT]
> La licence MIT placée à la racine du dépôt ne s'applique à aucun PDF de ce dossier. Chaque document demeure soumis à sa propre licence ou, pour le manuscrit de thèse, aux droits de son auteur.

## Documents versionnés

| Référence | Copie locale | Version exacte | DOI | Licence |
|---|---|---|---|---|
| Louis Hauseux, *Utilisation de graphes pour la classification et l'extraction de structures. Généralisation à des interactions d'ordre supérieur* | [manuscrit de thèse](MANUSCRIT_THESE_HAUSEUX.pdf) | copie auteur générée le 5 juillet 2026 | non attribué | droits de l'auteur |
| Herbert Edelsbrunner et Georg Osang, *A Simple Algorithm for Higher-order Delaunay Mosaics and Alpha Shapes* | [PDF](pdfs/edelsbrunner-osang-higher-order-delaunay-arxiv-2011.03617v1.pdf) | arXiv:2011.03617v1 | [10.48550/arXiv.2011.03617](https://doi.org/10.48550/arXiv.2011.03617) | [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/) |
| René Corbet, Michael Kerber, Michael Lesnick et Georg Osang, *Computing the Multicover Bifiltration* | [PDF](pdfs/corbet-et-al-multicover-bifiltration-arxiv-2103.07823v3.pdf) | arXiv:2103.07823v3, version intégrale | [10.48550/arXiv.2103.07823](https://doi.org/10.48550/arXiv.2103.07823) | [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/) |
| Bernardo Taveira et al., *Scalable GPU Construction of 3D Voronoi and Power Diagrams* | [PDF](pdfs/taveira-et-al-paragram-arxiv-2605.06408v1.pdf) | arXiv:2605.06408v1 | [10.48550/arXiv.2605.06408](https://doi.org/10.48550/arXiv.2605.06408) | [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/) |
| Louis Hauseux, Konstantin Avrachenkov et Josiane Zerubia, *Generalization of single-linkage with higher-order interactions* | [PDF éditeur](pdfs/hauseux-et-al-hgp-applied-network-science-2026.pdf) | *Applied Network Science* 11, article 9 (2026) | [10.1007/s41109-025-00756-1](https://doi.org/10.1007/s41109-025-00756-1) | [CC BY-NC-ND 4.0](https://creativecommons.org/licenses/by-nc-nd/4.0/) |

Le DOI de publication annoncé dans le PDF de Taveira et al. est `10.1145/3799902.3811229`. Tant que sa notice éditeur n'est pas stabilisée, l'identifiant arXiv reste la référence primaire de la copie locale.

La publication courte de Corbet et al. à SoCG 2021 porte le DOI [10.4230/LIPIcs.SoCG.2021.27](https://doi.org/10.4230/LIPIcs.SoCG.2021.27). La copie conservée ici est la version intégrale arXiv v3 de 28 pages, pas le PDF LIPIcs de 17 pages.

## Références accessibles par lien seulement

### Catalogue critique et topologie des multicovertures

- Y. Reani et O. Bobrowski, [*Morse Theory for the k-NN Distance Function*](https://arxiv.org/abs/2403.12792), caractérisation des centres critiques, indice $\mu=s-k$ et effet homologique. Le PDF mentionne CC BY 4.0, mais sa notice arXiv expose actuellement la licence de distribution non exclusive d'arXiv; il n'est pas redistribué tant que ces informations ne concordent pas.
- H. Edelsbrunner et G. Osang, [*The Multi-Cover Persistence of Euclidean Balls*](https://doi.org/10.1007/s00454-021-00281-9), structure ordre–échelle et mosaïques d'ordre supérieur.
- H. Edelsbrunner et A. Nikitenko, [*Poisson–Delaunay Mosaics of Order k*](https://arxiv.org/abs/1709.09380), comptes moyens à ordre fixé dans le régime de Poisson.
- R. Corbet, M. Kerber, M. Lesnick et G. Osang, [*Computing the Multicover Bifiltration*](https://doi.org/10.1007/s00454-022-00476-8), construction de la bifiltration multicoverture. Il ne faut pas confondre cet article avec celui d'Edelsbrunner–Osang ci-dessus.

### Géométrie exacte et structures GPU

- E. Welzl, [*Smallest Enclosing Disks (Balls and Ellipsoids)*](https://doi.org/10.1007/BFb0038202), miniballs en dimension fixée.
- J. R. Shewchuk, [*Adaptive Precision Floating-Point Arithmetic and Fast Robust Geometric Predicates*](https://www.cs.cmu.edu/~quake/robust.html), filtres et expansions exactes.
- M. Prokopenko et al., [*A Fast and Scalable Algorithm for Euclidean Minimum Spanning Tree Construction*](https://arxiv.org/abs/2207.00514), référence GPU pour l'ancre $k=1$.
- J. Erickson, [*Nice Point Sets Can Have Nasty Delaunay Triangulations*](https://arxiv.org/abs/cs/0103017), obstruction de pire cas en dimension trois.
- [zenseact/paragram](https://github.com/zenseact/paragram), code Apache-2.0 associé au calcul GPU de diagrammes de Voronoï et de puissance. Le premier audit devra épingler le commit `cadf96c854d27c8234d5b64749b8998e3d1af7f8`; ses calculs flottants et son option fast math en font une primitive de proposition, pas un certificat.

### Comparaisons sparse et grande dimension différée

- D. Buchet et al., [*Sparse Higher-Order Čech Filtrations*](https://arxiv.org/abs/2303.06666), comparaison approximative pour la sparsification.
- A. Alonso, [*Sparse Multicover Bifiltrations*](https://arxiv.org/abs/2411.06986), autre approche approximative de multicoverture.
- L. J. Guibas, Q. Mérigot et D. Morozov, [*Witnessed k-Distance*](https://arxiv.org/abs/1102.4972), approximation de la distance aux mesures.

Ces références servent à définir des baselines ou des bornes. Aucune approximation sparse n'est utilisée comme oracle de complétude de MorseHGP3D.

## Matrice de lecture critique

Cette matrice sépare l'apport effectivement réutilisé de ce que la source ne permet pas de conclure. Elle évite qu'une proximité de vocabulaire soit transformée en garantie algorithmique.

| source | résultat exploité dans MorseHGP3D | limite explicite |
|---|---|---|
| manuscrit et article HGP | équivalence multicoverture–K-polyèdres, adjacences élémentaires, nécessité de Gabriel pour une séparation, réduction par K-MST des composantes non triviales | ne donne ni énumérateur GPU du K-Gabriel, ni généalogie certifiée de toutes les facettes isolées |
| Reani–Bobrowski | caractérisation des centres critiques, indice $\mu=s-k$ et multiplicité locale $\binom{\lvert U\rvert-1}{\mu}$ | une selle locale ne dit pas quelles classes globales de $H_0$ fusionnent; les attaches restent nécessaires |
| Edelsbrunner–Osang et Corbet et al. | cellules top-$k$, identité incrémentale des ordres et complexes exacts de multicoverture | matérialiser la mosaïque ou le rhomboid tiling conserve une combinatoire dont MorseHGP3D n'a pas besoin |
| Edelsbrunner–Nikitenko | croissance moyenne favorable à ordre fixé dans le modèle de Poisson | aucune borne déterministe pour une entrée surfacique, LiDAR ou adversariale |
| Taveira et al. / Paragram | primitive cellulaire massivement parallèle pour Voronoï et puissance en 3D | les calculs flottants et `fast_math` proposent une géométrie; ils ne certifient ni les incidences, ni la complétude |
| Welzl et Shewchuk | miniballs en dimension fixée, filtres adaptatifs et prédicats exacts | ces briques ne résolvent pas l'énumération globale des labels |
| Prokopenko et al. | ancre GPU indépendante pour l'EMST et la tranche $k=1$ | ne généralise pas directement les interactions d'ordre supérieur |
| approches sparse, DTM et witnessed k-distance | baselines, priorités de candidats et modes approximatifs comparatifs | elles changent ou approchent la filtration et ne prouvent pas la tour HGP dure |

La décision architecturale résulte de cette intersection : reprendre l'induction sur les ordres, mais reconstruire seulement les cellules peu profondes utiles à $H_0$; utiliser une primitive de puissance comme moteur de proposition et fermer chaque décision par un oracle global exact; reconstruire enfin la hiérarchie avec les théorèmes de Gabriel du manuscrit.

## Intégrité et mise à jour

Après tout ajout ou remplacement d'un PDF, mettre à jour sa taille et son SHA-256 dans `references.toml`, puis exécuter :

```bash
python tools/check_references.py
```

Le contrôle échoue si un PDF n'est pas déclaré, si un chemin déclaré manque ou si son contenu diffère de l'empreinte enregistrée. Une nouvelle version d'un article doit recevoir un nom de fichier et une entrée de manifeste qui rendent sa version explicite.
