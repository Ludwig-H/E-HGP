# Stratégie de publication

## Histoire de papier recommandée

Le papier ne doit pas être vendu comme l'addition mécanique de HGP et HSA. Les précédents hiérarchie + Transformer sont nombreux, la fonction support est classique et les réseaux simpliciaux savent déjà propager ou pondérer des messages entre cellules.

L'histoire la plus défendable est :

> Les partitions spatiales usuelles des Transformers 3D sont souvent optimisées d'abord pour le calcul et ne modélisent pas explicitement les niveaux de l'estimateur de densité $K$-NN. Nous conservons un payload `marked_incidence` — facettes, cofaces de connexion, incidences et niveaux — avec carrier et autorité déclarés, utilisons le support source comme raccourci global, puis séparons causalement la valeur de cette représentation, de la hiérarchie et de l'opérateur d'attention sous échantillonnage LiDAR.

Cette histoire reste valide si le meilleur opérateur n'est finalement pas HSA. Elle ne reste pas valide si HGP n'est pas meilleur qu'un arbre simple.

Les tableaux conservent les noms contractuels exacts : `payload_kind=marked_incidence`, `carrier_kind` parmi `source_points`, `facet_pl`, `coface_pl`, `witness_union`, et `authority` parmi `incidence_complete`, `pl_complete`, `witness_exact`, `witness_approx`, `h0_only`. Ils enregistrent aussi `cut_policy`, `cut_level`, `cut_side` et `deltas` ; changer l'un de ces champs crée une configuration distincte.

## Classement des actifs

Trois actifs peuvent recevoir le budget de nouveauté d'une soumission, et ils n'ont pas la même valeur. Ce classement décide de ce qui va dans le titre et de ce qui reste en appendice ; son développement complet et l'ordre des mesures qui l'instruit figurent dans [ORDRE_DES_PREUVES.md](ORDRE_DES_PREUVES.md).

**HSA est un actif faible.** C'est le papier d'une autre équipe, dont les validations portent sur du texte uniquement : il ne contient aucune expérience 3D ni dense. Sous LayerNorm, l'énergie d'interaction ne dépend que des moyennes pondérées par la taille des requêtes et des clés de chaque sous-arbre, de sorte qu'une HSA fidèle est mécaniquement une attention sur moyennes de sous-arbres, très proche du contrôle bottom-up/top-down `mean` + MLP déjà prévu dans la matrice d'ablation. Les auteurs indiquent eux-mêmes que leur cadre n'ajoute aucun paramètre apprenable le long de la hiérarchie, et le descripteur géométrique n'y entre que par le produit scalaire $\varepsilon(A')^{\top}\varepsilon(B')$ entre frères, c'est-à-dire par un seul scalaire de biais par couple de frères. Enfin, le remplacement zero-shot dégrade fortement certaines tâches, QNLI tombant à $0{,}5072$, soit le niveau du hasard. L'employer fidèlement reste correct et nécessaire comme baseline ; le reproduire ne produit pas de nouveauté. Il faut ajouter que l'algorithme demande $D$ produits matrice creuse–vecteur séquentiels, où $D$ est la profondeur de la hiérarchie : la condensation de l'arbre de fusion est donc une condition d'existence sur GPU, pas une optimisation, et elle modifie l'objet, ce qui impose de la versionner et de l'ablater.

**Le descripteur est un actif moyen.** Le théorème de caractérisation du canal support, détaillé dans [DESCRIPTEURS_DE_NOEUD.md](DESCRIPTEURS_DE_NOEUD.md), donne une raison forte de choisir la fonction support plutôt qu'un descripteur arbitraire, et il explique pourquoi un second canal est nécessaire. Mais c'est du folklore recombiné — mesures idempotentes et maxitives, trichotomie de Fung-Fu, équation de translation d'Aczél, fonctions support de Schneider — et la grandeur qu'il caractérise, $h(u_{k})=\max_{i}\langle x_{i},u_{k}\rangle$, est littéralement un PointNet à première couche linéaire, tandis que le maximum sur les enfants d'un arbre de partition est déjà l'équation 1 de Superpoint Transformer. C'est un lemme justificatif ; il ne doit jamais être présenté comme une contribution.

**HGP et son analyse par percolation sont l'actif fort**, et le seul que personne d'autre ne possède. La correspondance exacte entre $K$-polyèdres et amas discrets de forte densité $K$-NN, la fonction de percolation, la vitesse de percolation et la limite gaussienne $\mu=K+a\sqrt{K}$ forment ensemble une théorie quantitative de la fraction d'un cluster récupérable avant fusion parasite. Aucune équipe de vision 3D ne dispose de cet outil, et il répond exactement à la question que la littérature des partitions ne sait pas poser proprement : quelle hiérarchie est récupérable, et à quel niveau.

La conséquence pour la rédaction est directe. Le centre du papier n'est pas « nous avons ajouté HSA sur un arbre HGP et gagné du mIoU », mais « voici une théorie de ce qui est récupérable dans une hiérarchie de densité, voici sa version valable pour un échantillonnage capteur inhomogène, voici la mesure qui la vérifie sur données réelles, et voici l'opérateur qui l'exploite ». Un gain de segmentation devient alors une validation, et non la contribution.

### Le point théorique le plus prometteur

L'extension de l'analyse de percolation à une **intensité inhomogène** $\lambda(x)$ modélisant la portée du capteur est le seul développement théorique qui porterait à lui seul la partie mathématique du papier. En toute généralité, c'est difficile, et il faut le dire sans détour : les résultats de percolation disponibles sont énoncés pour un processus homogène, la fonction de percolation et sa vitesse perdent leur sens usuel dès que l'intensité varie, et rien ne garantit qu'un seuil critique global subsiste sous une intensité décroissant avec la portée.

La version atteignable est locale. Sur une fenêtre où $\lambda$ varie peu, un changement d'échelle ramène le problème au cas homogène et prédit un déplacement du niveau critique comme fonction explicite de la portée ; il resterait à vérifier empiriquement cette prédiction sur SemanticKITTI, en confrontant le niveau de fusion mesuré au niveau prédit par bins de portée. Un tel résultat, même local et même conditionnel à ses hypothèses, répondrait à R1 — la hiérarchie encode le capteur et non la sémantique — dans le même geste qu'il fournirait la contribution théorique. C'est pourquoi il mérite d'être tenté tôt, avec un critère d'abandon explicite si l'approximation locale ne se vérifie pas.

## Contribution minimale pour une venue 3D forte

1. intégration reproductible d'une hiérarchie HGP dans un backbone LiDAR fort ;
2. contrat reproductible du complexe HGP marqué, sans construction exhaustive du complexe ambiant ;
3. ablation causale arbre / représentation / opérateur ;
4. gains SemanticKITTI appariés et robustesse par portée ;
5. coût complet, y compris extraction des incidences, construction de la hiérarchie et, pour `witness_union`, $N_W$, $\varepsilon_W$, requêtes, patches et échantillons ;
6. comparaison SPT/EZ-SP, PTv3, SP2T, LSK3DNet, SphereFormer et RAPiD-Seg, avec LitePT comme contrôle architectural même sans score SemanticKITTI publié et le statut TTA de SphereFormer conservé à `NR`.

## Contribution attendue pour ICML/NeurIPS

En plus du niveau précédent, obtenir au moins une contribution générale :

- **optimalité conditionnelle** : `QC-HSA`, projection reverse-KL conditionnée par la feuille, avec certificat d'oscillation ; ce résultat technique ne devient central qu'avec un certificat HGP non vacu ou un résultat fidélité–coût réellement nouveau ;
- **stabilité** : analyse et correction de la hiérarchie sous échantillonnage range-dependent ;
- **représentation** : encodeur du complexe HGP marqué, invariant aux identifiants et aux certificats sparse équivalents, avec expressivité ou stabilité établie face à MPSN/CWN/EMPSN/SAT/TopNets ;
- **attention** : opérateur hiérarchique avec voie de correction dont l'effet et le coût sont analysés ;
- **statistique** : lien vérifiable entre qualité d'un cluster tree et erreur de propagation sémantique ;
- **généralisation** : résultat cohérent sur au moins deux capteurs/datasets.

Un seul nouveau score SemanticKITTI, même premier, est trop fragile pour porter seul une soumission généraliste.

## Choix de venue

Les backbones de segmentation 3D vont en CVPR, ICCV ou ECCV. NeurIPS et ICML acceptent la perception 3D lorsque le cadrage est représentation, auto-supervision, passage à l'échelle ou théorie, et les précédents sont nommables : PTv2 (NeurIPS 2022), Seal (NeurIPS 2023, spotlight), SFCNet (NeurIPS 2024), Concerto (NeurIPS 2025), Utonia (ICML 2026).

La conclusion suit sans ambiguïté. Un gain de mIoU sur SemanticKITTI, même net, est un papier CVPR/ICCV/ECCV, et il y sera jugé sur l'ingénierie du backbone autant que sur l'idée : recette d'entraînement, budget de calcul, augmentations, ce qui n'est pas l'avantage comparatif de ce projet. À l'inverse, une théorie de la récupérabilité des hiérarchies de densité sous échantillonnage capteur, accompagnée d'un benchmark causal des hiérarchies et d'un opérateur qui en tire parti, est une soumission NeurIPS/ICML plausible **sans exiger le premier rang d'un classement**. C'est le chemin où la valeur scientifique et les chances de publication coïncident, et il faut choisir la venue avant de figer le plan expérimental, non après.

## Claims autorisés et preuves requises

| Claim potentiel | Preuve minimale |
|---|---|
| HGP est un meilleur prior | arbres échangés à budget constant, seeds appariées, IC excluant zéro |
| le complexe HGP apporte de la géométrie utile | points, accès à $\Gamma_K^{\mathrm{elem}}$, sac de tokens sans messages, incidences du contrat, mutant invalide, MPSN/CWN/EMPSN/SAT et Deep Sets à capacité égale |
| le support source aide le complexe source/PL | complexe seul contre support source + complexe, mêmes `payload_kind`, `carrier_kind`, `authority`, coupe, cellules, capacité et recette ; aucun transfert au support de `witness_union` |
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
- préservation de la non-convexité par seul rayon extérieur hors cas étoilé ;
- nouveauté par le seul usage d'un réseau ou d'une attention simpliciale ;
- « complexe complet » sans préciser le contrat reconstruit et les cellules omises ;
- opérateur DAG conservatif tant que les poids $w_{iv}$, leur domaine et la contrainte $\sum_v w_{iv}=1$ ne sont pas définis et testés ;
- nouveauté par la seule utilisation d'ECT/WECT, de Fourier ou d'un kernel mean ;
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

> Les Transformers LiDAR efficaces structurent généralement les interactions par voxels, fenêtres ou superpoints. Nous évaluons une autre hypothèse : un complexe HGP marqué, indépendant des labels, peut conserver les interactions d'ordre supérieur et organiser le contexte multi-échelle. Notre modèle combine un encodeur local, une branche point–facette consciente des incidences, un raccourci de support normalisé et une propagation hiérarchique tardive vers les points. Une étude factorisée isole la valeur du complexe, du raccourci convexe, de la hiérarchie et de l'attention, puis analyse stabilité à la portée, frontières, mémoire et latence. Les résultats sur SemanticKITTI et un second capteur déterminent si ce prior améliore réellement les partitions spatiales conventionnelles.

Ce résumé ne doit recevoir aucun chiffre avant que les expériences soient terminées.

## Figures décisives

1. **Schéma d'architecture** : backbone local, graphe d'incidence point–facette, hiérarchie HGP, support global, HSA tardif et décodeur point-fin.
2. **Résultat QC-HSA** : rectangles HSA contre partitions feuille–sous-arbre, projection fermée, coût supplémentaire et pont conditionnel vers les hauteurs de fusion.
3. **Diagnostic de compression dure** : vote majoritaire réalisable et union par classe optimiste, distincts du modèle à proportions et de sa sortie point-wise.
4. **Ablation causale** : arbre × opérateur, montrant où naît le gain.
5. **Stabilité capteur** : variation de hiérarchie et mIoU selon portée/thinning.
6. **Représentations et collisions** : mêmes sommets/support mais incidences distinctes, certificats sparse équivalents et perturbations de filtration ; le hash canonique les sépare, tandis que les collisions du learned encoder sont mesurées jusqu'à preuve d'expressivité.
7. **Pareto système** : mIoU contre latence/VRAM, coût HGP inclus, avec $N_W$ et $\varepsilon_W$ pour l'union témoin.
8. **Analyse d'erreurs** : frontière sémantique traversée par une branche et rôle du gate résiduel.

## Tables décisives

- comparaison track A strict, avec colonnes modality, temporal, external data, TTA, ensemble ;
- ablation HGP $K=1$/SL comme fixture, puis HGP $K=2,3$ vs RSL/octree/superpoints/random ;
- points seuls, $\Gamma_K^{\mathrm{elem}}$ avec tokens précalculés, sac des mêmes tokens sans messages, complexe d'incidence, complexe + support source et mutant d'incidences invalide, à budget égal ;
- encodeur proposé vs MPSN/CWN/EMPSN/SAT/TopNets et Deep Sets ;
- HSA vs pooling/message passing/local attention ;
- QC-HSA vs HSA : KL, sortie, frontière, mIoU, $C_T$, VRAM et latence ;
- IoU par classe et distance ;
- temps construction/arbre/extraction des incidences/encodeur/réseau/reprojection ;
- second dataset et changement de capteur.

## Expériences qui doivent pouvoir être négatives

- $K=2$ n'est pas forcément meilleur pour la sémantique malgré son résultat instance avec masques GT ;
- le complexe contractuel peut être redondant avec un backbone local fort ;
- le support peut ne rien ajouter au complexe seul ;
- un MPSN standard peut dominer l'encodeur proposé ;
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

### Résultat D — le support échoue, le complexe réussit

Le support ne donne aucun gain conditionnel, mais les incidences HGP en donnent un face aux graphes mélangés et aux encodeurs appariés. Le support sort alors du modèle et la contribution devient structurelle.

### Résultat E — aucune valeur face aux contrôles

Arrêt du projet modèle. Une étude négative peut être utile si elle contient des bornes, des contre-exemples et un audit reproductible, mais ne doit pas être présentée comme voie SOTA.

### Résultat F — HGP perd sur les classes fines et gagne sur les classes volumiques

Ce pivot manquait, alors qu'il correspond au mode d'échec que le manuscrit documente lui-même. Sur le jeu `birch2`, HDBSCAN à $k=100$ obtient un ARI de $0{,}996$ en classant $99{,}7$ % des points, contre $0{,}441$ et $83{,}9$ % pour HGP-Clusterer à $k=84$, la cause citée étant que « les clusters sont essentiellement filiformes et sont donc mieux identifiés avec de simples graphes ». Le mécanisme est structurel et non anecdotique : la connexité d'ordre $K$ exige $K$ points simultanément proches, condition qu'une structure mince échantillonnée de façon éparse ne satisfait qu'à un rayon nettement plus grand, si bien que l'objet fin naît tard dans la filtration et qu'à ce niveau ses voisines l'ont déjà rejoint. Le résultat observable est une sous-segmentation des objets fins, et non une fragmentation. Or la marge de progression du mIoU SemanticKITTI porte précisément sur `pole`, `traffic-sign`, `bicycle`, `person`, `bicyclist`, `motorcyclist` et `fence`, les classes volumiques plafonnant déjà très haut.

Si les mesures confirment ce profil — gain sur les classes volumiques, perte sur les classes fines —, la publication honnête n'est pas un papier d'architecture mais une étude de la limite de la connexité d'ordre supérieur : à quelle dimension intrinsèque et à quelle densité d'échantillonnage l'exigence de $K$ points simultanément proches cesse d'être payante, mesurée par IoU et par oracle d'antichaîne stratifiés par classe fine contre classe volumique. Le manuscrit suggère une atténuation observée sur `birch2`, le changement d'estimateur $\hat{\rho}=1/r^{2}$, qui doit être testée mais ne dispense pas de mesurer d'abord l'ampleur du problème. Ce résultat reste publiable et utile pour la communauté ; il ne doit pas être présenté comme une voie vers le premier rang d'un classement.

## Segmentation d'instance : appendice futur, pas promesse actuelle

Après fermeture positive de la sémantique, l'arbre pourra fournir des propositions d'instances. La comparaison centrale sera ALPINE contre coupe HGP, à logits gelés. Cette extension ne doit apparaître dans une première soumission que si elle renforce une contribution déjà établie et n'affaiblit pas la profondeur de l'étude sémantique.

## Questions qui doivent recevoir une réponse falsifiable

1. Le canal vise-t-il l'union d'observations, le porteur PL des facettes, le porteur continu de multicoverture ou leur certificat commun ?
2. Facettes, cofaces, incidences, coordonnées, niveaux et plateaux suffisent-ils à reconstruire exactement cet objet à réindexage près ?
3. Pour $K\geq2$, comment les recouvrements ponctuels sont-ils conservés dans la branche d'incidence puis projetés vers la forêt HSA sans double comptage ?
4. Les niveaux HGP restent-ils stables quand un même patch est transporté puis rééchantillonné selon la portée ?
5. L'arbre HGP améliore-t-il pureté, oracle de coupe et frontières face à RSL, octree, superpoints et arbre aléatoire à compression égale ?
6. L'encodeur distingue-t-il mêmes points et même support avec incidences différentes, puis bat-il points seuls, graphe $\Gamma^{K}$ seul et réseaux simpliciaux de même budget ?
7. Le gain vient-il de l'arbre, de la représentation ou de l'opérateur dans une ablation factorisée ?
8. HSA/QC-HSA améliore-t-il les points proches des frontières sans dégrader les classes rares ?
9. Le coût reste-t-il utile pour les arbres réels, y compris degrés, profondeur, condensation et $C_T$ ?
10. Le gain persiste-t-il avec au moins trois seeds appariées et un intervalle à 95 % ?
11. La méthode reste-t-elle compétitive sans TTA, ensemble, historique, RGB, pseudo-labels externes ou données externes ?
12. Le mécanisme se reproduit-il sur un second capteur/dataset ?

## Conditions de poursuite

Continuer vers un modèle complet seulement si :

- HGP bat au moins une hiérarchie de contrôle forte avec agrégateur simple ;
- l'effet ne disparaît pas sous stratification par portée et densité ;
- le complexe complet est sérialisé sans ambiguïté et son encodeur bat les mêmes points, le graphe $\Gamma^{K}$ seul et des contrôles simpliciaux à budget comparable ;
- `support + complexe` améliore le Pareto face à `complexe seul`, sinon le support est retiré sans remettre en cause la branche non convexe ;
- HSA ou son successeur bat pooling/message passing sur le même arbre ;
- les contraintes de mémoire permettent des batches et trois seeds réalistes.

L'ordre dans lequel ces conditions sont éprouvées n'est pas libre, et il ne commence pas par le descripteur. Les ablations publiées sur cette famille exacte placent le descripteur de nœud au dernier rang des trois leviers : sur Superpoint Transformer, retirer toutes les caractéristiques manuelles de nœud coûte $-0{,}7$ mIoU sur S3DIS 6-fold, $-4{,}1$ sur KITTI-360 et $-1{,}4$ sur DALES, tandis que retirer l'encodage d'adjacence coûte $-6{,}3$, $-5{,}4$ et $-3{,}0$, et que passer à un seul niveau de partition coûte $-8{,}4$, $-5{,}1$ et $-0{,}9$ ; EZ-SP rapporte de son côté que remplacer les caractéristiques manuelles par un petit réseau appris ne déplace le résultat que de $\pm 0{,}1$ mIoU. Investir d'abord dans le canal de descripteur revient donc à travailler le levier le plus faible, alors que l'adjacence et le nombre de niveaux dominent. Les mesures doivent en conséquence suivre l'ordre fixé par [ORDRE_DES_PREUVES.md](ORDRE_DES_PREUVES.md), et la définition des canaux celle de [DESCRIPTEURS_DE_NOEUD.md](DESCRIPTEURS_DE_NOEUD.md).

Sinon, publier le résultat négatif le plus informatif ou pivoter : étude de stabilité des arbres de densité LiDAR, descripteur topologique borné, ou benchmark causal de hiérarchies. Ne pas protéger l'histoire initiale en changeant simultanément backbone, arbre, descripteur et recette.

## Verdict honnête

Le couplage corrigé est une hypothèse compétitive crédible, mais il n'existe aujourd'hui aucune base honnête pour promettre le SOTA. Le chemin le plus solide est : « nous avons défini et encodé sans ambiguïté l'objet HGP marqué et son carrier non convexe déclaré, isolé leur apport au-delà du support et des mêmes points, puis montré quand leur couplage hiérarchique résiste au changement de portée et de capteur ». Si la sérialisation, l'effet causal de l'objet ou le gain de l'opérateur échoue, le papier de niveau visé n'est pas prêt, même avec un score favorable isolé.

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
