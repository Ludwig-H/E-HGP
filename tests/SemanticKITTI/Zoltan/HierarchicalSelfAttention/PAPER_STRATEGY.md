# Stratégie de publication

## Histoire de papier recommandée

Le papier ne doit pas être vendu comme l'addition mécanique de HGP et HSA. Les précédents hiérarchie + Transformer sont nombreux, et la fonction support est classique.

L'histoire la plus défendable est :

> Les partitions spatiales usuelles des Transformers 3D sont souvent optimisées d'abord pour le calcul et ne modélisent pas explicitement les niveaux de l'estimateur de densité $K$-NN. Nous étudions une hiérarchie HGP exogène comme prior structurel, séparons sa valeur de celle du descripteur et de l'opérateur d'attention, puis construisons une propagation hiérarchique robuste à l'échantillonnage LiDAR.

Cette histoire reste valide si le meilleur opérateur n'est finalement pas HSA. Elle ne reste pas valide si HGP n'est pas meilleur qu'un arbre simple.

## Contribution minimale pour une venue 3D forte

1. intégration reproductible d'une hiérarchie HGP dans un backbone LiDAR fort ;
2. ablation causale arbre / représentation / opérateur ;
3. gains SemanticKITTI appariés et robustesse par portée ;
4. coût complet, y compris construction de la hiérarchie ;
5. comparaison SPT/EZ-SP, PTv3, SP2T, LSK3DNet, SphereFormer et RAPiD-Seg.

## Contribution attendue pour ICML/NeurIPS

En plus du niveau précédent, obtenir au moins une contribution générale :

- **optimalité conditionnelle** : `QC-HSA`, projection reverse-KL conditionnée par la feuille, avec certificat d'oscillation ; ce résultat technique ne devient central qu'avec un certificat HGP non vacu ou un résultat fidélité–coût réellement nouveau ;
- **stabilité** : analyse et correction de la hiérarchie sous échantillonnage range-dependent ;
- **représentation** : sketch de masse projetée, fusionnable, avec borne d'approximation ou de sensibilité ;
- **attention** : opérateur hiérarchique avec voie de correction dont l'effet et le coût sont analysés ;
- **statistique** : lien vérifiable entre qualité d'un cluster tree et erreur de propagation sémantique ;
- **généralisation** : résultat cohérent sur au moins deux capteurs/datasets.

Un seul nouveau score SemanticKITTI, même premier, est trop fragile pour porter seul une soumission généraliste.

## Claims autorisés et preuves requises

| Claim potentiel | Preuve minimale |
|---|---|
| HGP est un meilleur prior | arbres échangés à budget constant, seeds appariées, IC excluant zéro |
| le sketch support + masse projetée apporte de la géométrie utile | baselines de même dimension, collisions et stress tests inclus |
| HSA exploite mieux HGP | même arbre/features contre pooling et message passing |
| QC-HSA est la projection optimale annoncée | preuve complète, solveur dense sur petits arbres, inclusion HSA et facteur de cardinalité vérifiés |
| QC-HSA préserve mieux les points | reverse-KL et erreurs de frontière inférieurs à HSA, coût $C_T$ et latence inclus |
| robuste à longue portée | gains par bins et perturbations, pas seulement moyenne globale |
| plus efficace | latence et mémoire end-to-end sur même matériel |
| SOTA LiDAR mono-trame | audit frais, protocole strict, résultat test caché |
| général | second dataset/capteur et mécanisme cohérent |

Ne pas revendiquer :

- invariance rotationnelle d'un vecteur sur directions fixes ;
- préservation complète de la géométrie par fonction support ;
- complexité linéaire sans bornes de degré et mesure réelle ;
- optimalité sémantique ou approximation d'une attention arbitraire à partir du théorème KL très borné de HSA ;
- certificat mIoU à partir de Pinsker ou d'une marge point-wise ;
- exactitude ou statut GPU de MorseHGP3D hors registre.

## Titre et résumé de travail

Titres possibles, à choisir après les résultats :

- *K-NN Density Trees as Structural Priors for LiDAR Semantic Segmentation*
- *Density Hierarchies for Multi-Scale Attention on 3D Point Clouds*
- *Beyond Spatial Partitions: Auditing Density Trees for LiDAR Transformers*

Résumé de travail :

> Les Transformers LiDAR efficaces structurent généralement les interactions par voxels, fenêtres ou superpoints. Nous évaluons une autre hypothèse : une hiérarchie de niveaux de densité $K$-NN, indépendante des labels, peut organiser le contexte multi-échelle. Notre modèle combine un encodeur local, des relations géométriques décrivant extrêmes et masse intérieure, des proportions sémantiques déduites des descendants et une propagation hiérarchique vers les points. Une étude factorisée isole la valeur de l'arbre, du sketch distributionnel et de l'attention, et analyse stabilité à la portée, frontières, mémoire et latence. Les résultats sur SemanticKITTI et un second capteur déterminent si ce prior améliore réellement les partitions spatiales conventionnelles.

Ce résumé ne doit recevoir aucun chiffre avant que les expériences soient terminées.

## Figures décisives

1. **Schéma d'architecture** : backbone local, arbre HGP, descripteurs relationnels, HSA tardif, décodeur point-fin.
2. **Résultat QC-HSA** : rectangles HSA contre partitions feuille–sous-arbre, projection fermée, coût supplémentaire et pont conditionnel vers les hauteurs de fusion.
3. **Diagnostic de compression dure** : vote majoritaire réalisable et union par classe optimiste, distincts du modèle à proportions et de sa sortie point-wise.
4. **Ablation causale** : arbre × opérateur, montrant où naît le gain.
5. **Stabilité capteur** : variation de hiérarchie et mIoU selon portée/thinning.
6. **Support et collisions** : mêmes enveloppes, distributions différentes, puis correction par quantiles/side channels.
7. **Pareto système** : mIoU contre latence/VRAM, coût HGP inclus.
8. **Analyse d'erreurs** : frontière sémantique traversée par une branche et rôle du gate résiduel.

## Tables décisives

- comparaison track A strict, avec colonnes modality, temporal, external data, TTA, ensemble ;
- ablation HGP $K=1$/SL comme fixture, puis HGP $K=2,3$ vs RSL/octree/superpoints/random ;
- support vs CDF projetées/quantiles/moments/PointNet à budget égal ;
- HSA vs pooling/message passing/local attention ;
- QC-HSA vs HSA : KL, sortie, frontière, mIoU, $C_T$, VRAM et latence ;
- IoU par classe et distance ;
- temps construction/arbre/descripteur/réseau/reprojection ;
- second dataset et changement de capteur.

## Expériences qui doivent pouvoir être négatives

- $K=2$ n'est pas forcément meilleur pour la sémantique malgré son résultat instance avec masques GT ;
- support maximal peut être dominé par un mini-PointNet ;
- HSA peut être dominé par un simple passage top-down ;
- HGP brut peut être moins stable qu'un octree à longue portée ;
- une projection laminaire peut perdre l'intérêt des chevauchements d'ordre supérieur.

Ces résultats ne doivent pas être cachés. Bien analysés, ils déterminent le vrai papier et préviennent un claim faux.

## Pivots de publication

### Résultat A — HGP + HSA gagne nettement

Papier complet candidat : nouvelle architecture, analyse de stabilité, deux datasets, SOTA ou Pareto fort, sous réserve de l'audit de nouveauté.

### Résultat B — HGP gagne, HSA non

Papier sur les hiérarchies de densité comme prior ; agrégateur simple ; contribution de stabilité/efficacité. Le mot HSA sort du titre.

### Résultat C — HGP aide seulement sous perturbation

Papier robustness/domain shift : arbre ou métrique corrigée de la portée, évaluation capteurs multiples.

### Résultat D — support échoue, arbre réussit

Résultat négatif formel sur support + remplacement par pooling appris. La contribution devient structurelle.

### Résultat E — aucune valeur face aux contrôles

Arrêt du projet modèle. Une étude négative peut être utile si elle contient des bornes, des contre-exemples et un audit reproductible, mais ne doit pas être présentée comme voie SOTA.

## Segmentation d'instance : appendice futur, pas promesse actuelle

Après fermeture positive de la sémantique, l'arbre pourra fournir des propositions d'instances. La comparaison centrale sera ALPINE contre coupe HGP, à logits gelés. Cette extension ne doit apparaître dans une première soumission que si elle renforce une contribution déjà établie et n'affaiblit pas la profondeur de l'étude sémantique.

## Checklist avant rédaction

- concurrence réauditée à la date de soumission ;
- splits, ressources et test submissions documentés ;
- hypothèses et kill gates datés avant les runs finaux ;
- baselines reproduites avec leur code officiel lorsque possible ;
- seeds et intervalles présents ;
- coût HGP inclus ;
- théorie invoquée exactement dans son domaine ;
- limitations et résultats négatifs explicites ;
- second dataset terminé ;
- artefacts reproductibles et licence vérifiés.
