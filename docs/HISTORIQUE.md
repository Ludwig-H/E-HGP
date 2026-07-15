# Historique condensé du projet

E-HGP a exploré plusieurs façons de prolonger HGP-Clusterer : code géométrique proche de la thèse, régularisation entropique, atlas progressifs de cofaces, récupération sparse en grande dimension et approximation 3D sur grille.

## Éléments conservés

`HGP-old/` reste intégralement dans l'arbre. Il constitue la référence historique au code et aux expériences des deux premières parties du manuscrit. Ses dépendances, perturbations numériques et sorties ne lui donnent pas le statut d'oracle pour MorseHGP3D; l'oracle futur sera réécrit de façon indépendante.

`gcp-migration/` reste également actif. Il ne s'agit pas d'un ancien prototype scientifique, mais de l'infrastructure nécessaire pour créer, démarrer, contrôler et arrêter les VM GPU G4. Ses coupe-circuits et sa vérification finale sont désormais normatifs.

Le manuscrit et les articles redistribuables restent sous `docs/references/`, avec leurs licences et empreintes propres.

## Éléments retirés le 14 juillet 2026

Le package `perg_hgp` et l'expérience `PowerCover3D` ont été supprimés de la branche courante. Leur grille cubique approximait un champ régularisé, introduisait une échelle de discrétisation et ne répondait ni au contrat hiérarchique exact, ni au besoin de plusieurs millions de points. Conserver ce code actif aurait entretenu une fausse continuité avec MorseHGP3D.

Les anciens rapports A–E et l'architecture à deux backends ont aussi été remplacés par un corpus MorseHGP3D cohérent. Les idées utiles ont été reprises : distinction entre proposition et certification, lots de niveaux égaux, descente miniball, fermeture par séparation et statuts explicites.

La piste grande dimension, anciennement nommée `HomogeneousLensTower`, n'est pas déclarée fausse; elle est différée. Le dépôt courant doit d'abord résoudre et implémenter proprement le cas 3D. DTM et entropie restent des heuristiques de proposition, non des définitions de la hiérarchie.

## Décision actuelle

La voie exacte de référence active est :

$$\text{prédicats exacts}\longrightarrow\Gamma\text{ exhaustif}\longrightarrow\text{hyper-Kruskal par lots}\longrightarrow\text{tour ordre–échelle}.$$

La voie accélérée construit un catalogue critique peu profond et un flot de Gabriel, mais conserve une sémantique `partial_refinement` tant que toutes les incidences silencieuses ne sont pas localisées par certificat. Son énumération adapte l'algorithme incrémental des ordres à une primitive de diagramme de puissance GPU, sous forme de raffinements de Voronoï restreints. Elle ne matérialise ni mosaïque de Delaunay d'ordre supérieur, ni rhomboid tiling.

L'audit du 15 juillet 2026 a ajouté la [tour globale de boules saturées](math/TOUR_BOULES_SATUREES.md) comme piste de recherche. Sa représentation de Čech/Gamma par saturés et forêt d'intersections est mathématiquement exacte, mais son énumération brute de tous les supports de tailles un à quatre n'est pas une voie scalable au régime produit. Elle devient donc un futur oracle CPU borné et une source de raffinements partiels, sans remplacer l'architecture principale. Le même audit a corrigé la fenêtre critique de Morse en $\lvert I\rvert<k\leq\lvert I\rvert+\lvert U\rvert$ et l'a figée dans `morse-rank-window-regression-v1`.

## Traçabilité Git

L'arbre antérieur au premier nettoyage reste consultable dans l'[instantané `0b8a1a1`](https://github.com/Ludwig-H/E-HGP/tree/0b8a1a11750b931f486ce666265eed4b6e95e2b1). La consolidation intermédiaire à deux backends est disponible dans les commits suivants de l'historique. Les suppressions courantes n'effacent donc ni le code, ni les rapports, ni les notebooks; elles retirent seulement ces pistes du corpus maintenu.

## Leçons retenues

- Une régularisation locale n'engendre pas automatiquement une hiérarchie globale sparse.
- La DTM change la filtration; une continuation ne certifie pas le résultat dur.
- Une liste locale fixe peut manquer un simplexe de Gabriel.
- Un moteur flottant GPU propose des cellules, mais la fermeture exacte doit rester externe.
- Le pire cas 3D est superlinéaire; les budgets doivent dégrader le statut, jamais la vérité annoncée.
- Une hiérarchie HGP comporte des recouvrements et des applications entre ordres, pas seulement une partition.
- Les facettes isolées ne sont pas couvertes automatiquement par le théorème du K-MST; le profil complet exige des attaches.
- Une forêt couvrante de poids maximum peut compresser les coupes d'un graphe de générateurs, mais ses remplacements d'arêtes ne sont pas les événements d'un `MergeForest`.
- Une borne de rang suffisante pour les changements de Morse de $H_0$ ne borne pas les saturés nécessaires pour encoder toutes les incidences latentes.
