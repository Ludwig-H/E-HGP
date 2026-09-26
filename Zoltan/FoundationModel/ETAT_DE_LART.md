# Comparer les primitives, les informations visibles et les coûts

26 septembre 2026. Recherche ciblée, vérifiée sur publications et code officiels.
La question est **quelle contribution propre de FULL subsiste à données,
information visible et budget comparables ?** Le témoin principal conserve
un encodeur existant ; FULL peut guider son apprentissage ou remplacer une
primitive dans une expérience distincte.

## 1. Le problème d'échelle et ses différents mécanismes

Voxels et rayons fixent des distances physiques. Le nombre de voisins, la
taille des patches de sérialisation et le nombre de jetons fixent des
**budgets cardinaux** : leur étendue métrique varie avec l'échantillonnage.
Une partition de superpoints peut elle-même dépendre de la géométrie locale.
Ces mécanismes ne sont donc pas tous des variantes d'un rayon fixe.

HGP fournit une histoire à plusieurs ordres et rayons. Son interface réseau
garde des choix : grille d'entrée, K maximal, horizon, coupes, masse,
condensation, budget de jetons et d'arêtes. La contribution à mesurer est une
**meilleure sélection et utilisation de l'échelle**, avec réglages gelés au
transfert. Une suppression générale des hyperparamètres n'est pas revendiquée.

## 2. Antécédents et témoins utiles

| travail | apport établi dans sa propre expérience | conséquence pour notre comparaison |
| --- | --- | --- |
| [PTv3, CVPR 2024](https://openaccess.thecvf.com/content/CVPR2024/papers/Wu_Point_Transformer_V3_Simpler_Faster_Stronger_CVPR_2024_paper.pdf) | sérialisation, grid pooling, champs réceptifs étendus à coût contenu | témoin efficace à reproduire ; payer toute conversion de représentation |
| [Sonata, CVPR 2025](https://openaccess.thecvf.com/content/CVPR2025/papers/Wu_Sonata_Self-Supervised_Learning_of_Reliable_Point_Representations_CVPR_2025_paper.pdf) | auto-distillation et réduction du raccourci vers hauteur/normales, en obscurcissant l'information spatiale et en sollicitant les attributs | même recette SSL avec et sans enseignant FULL ; le raccourci n'est pas réductible à une erreur d'échelle |
| [Vernata, arXiv v1, août 2026](https://arxiv.org/html/2608.06919v1) | vues raréfiées, banque mémoire et distillation depuis l'image, sur LiDAR extérieur | séparer raréfaction et apport multimodal ; les deux bras reçoivent la même information |
| [Utonia, v2](https://arxiv.org/html/2603.03283v2) | rééchelonnage de granularité, masquage de modalités, RoPE et apprentissage inter-domaines | témoin de transfert ; publier réglages et coût du rééchelonnage |
| [Superpoint Transformer, ICCV 2023](https://arxiv.org/html/2306.08045v1) | partition hiérarchique adaptative et attention entre régions | témoin prioritaire de structure, à budget de régions apparié |
| [DOS, AAAI 2026](https://ojs.aaai.org/index.php/AAAI/article/view/39030) | distillation de softmaps sur points observables et prior de fréquence des prototypes | témoin SSL pour le contrôle des fuites et du déséquilibre |

**SPT** est un antécédent architectural direct, sans exclusivité revendiquée.
Son ablation à un niveau perd 5,1 points de mIoU sur KITTI-360 validation ;
cela justifie de tester une hiérarchie et ne prédit aucun gain HGP. Sa
partition optimise une énergie régularisée à plusieurs niveaux ; ses gains
dépendent aussi des attributs, des relations et du protocole. [SPT, tableau 4](https://arxiv.org/html/2306.08045v1).

**ALPINE** est un témoin pour la tête d'instances. Il reçoit des **prédictions
sémantiques** et utilise des dimensions typiques par classe pour ses seuils
et découpes. Son PQ 64,2 sur SemanticKITTI validation correspond notamment au
bras MinkUNet sans TTA ; ce n'est pas un score de géométrie seule. Comparer
ALPINE et la tête HGP avec les **mêmes prédictions sémantiques**, mêmes priors
autorisés et même TTA. La tête d'instances n'est pas entraînée. [Article v2,
§ 3 et tableau 1](https://arxiv.org/html/2503.13203v2).

**BPS** motive un encodage de distances à des sondes fixes. **PolyhedronNet**
est une référence sur polyèdres avec faces et attributs déjà fournis :
son gain ne certifie ni une reconstruction LiDAR ni l'adéquation des supports
Gabriel à une surface physique. Les deux sont des témoins d'**encodage**,
distincts d'un témoin de hiérarchie. [BPS](https://arxiv.org/abs/1908.09186),
[PolyhedronNet](https://proceedings.iclr.cc/paper_files/paper/2025/hash/d551343f85fcf5e1a230fd393406306e-Abstract-Conference.html).

## 3. Le point de substitution exact dans PTv3

Le papier nomme le pooling « Grid Pool ». L'implémentation officielle consultée
utilise `SerializedPooling` et `SerializedUnpooling` :
regroupement par codes sérialisés plus grossiers, puis restitution par indice
inverse et skip projeté. Il ne s'agit pas d'un décodeur k-NN à remplacer.
Quantification, CPE par convolution sparse et patches sérialisés sont des
postes distincts. [Code officiel PTv3](https://github.com/Pointcept/PointTransformerV3/blob/main/model.py).

| expérience | substitution précise | contrôle indispensable |
| --- | --- | --- |
| **Guidage** | ajouter une perte enseignante FULL, encodeur et sortie inchangés | mêmes vues et données ; objets enseignants réservés à la perte ; coût d'apprentissage publié |
| **S1 — regroupement** | remplacer l'affectation aux cellules par une coupe HGP | même réduction de features ; scatter et restitution explicitement adaptés |
| **S2 — pondération** | remplacer la réduction par la moyenne pondérée du contrat | comparer moyenne et max sur les deux regroupements |
| **S3 — voisinage** | utiliser des arêtes dérivées des fusions | même budget d'arêtes et noyau d'attention ; la forêt n'impose pas seule un voisinage sparse |
| **S4 — biais** | ajouter un biais de niveau de fusion | comparer zéro, biais métrique et biais HGP sur le même noyau et la même précision |
| **S5 — ordres** | ajouter branches K et échanges verticaux | budget total fixé ; témoin avec plusieurs branches K1 |
| **S6 — restitution** | remplacer le gather dur par la projection pondérée vers les retours | conserver le skip fin ; séparer probabilités mélangées et logits |

**S4 n'est pas un branchement gratuit dans PTv3 par défaut.** Le code active
FlashAttention, désactive le RPE et refuse le RPE sur la voie Flash. Un biais
HGP demande un noyau compatible ou une autre voie. Publier une comparaison
scientifique à noyau commun, puis une comparaison de systèmes avec leurs
meilleurs noyaux, durées et mémoires. Remplacer seulement le pooling conserve
les dépendances métriques de CPE et sérialisation.
[Code officiel, `SerializedAttention` et `Block`](https://github.com/Pointcept/PointTransformerV3/blob/main/model.py).

Avant une exécution, épingler commit, configuration, checkpoint, bibliothèques
et définition de l'entrée. Les liens `main` identifient ici le composant ;
ils ne constituent pas un reçu reproductible. Un passage à des régions change
aussi les positions sparsifiées : collisions de centres et reconstruction
des codes font partie de l'adaptateur évalué.

## 4. Ce que FULL permet de proposer

Sous conformité à Morse HGP 3D, FULL fournit historiques, événements,
populations et cartes verticales à niveaux exacts. Les théorèmes justifient cet
objet sur les retours acquis. Ils ne sélectionnent pas les coupes du réseau,
ses poids ni sa fonction de perte.

La proposition est d'utiliser les historiques K/r comme **supervision
structurelle contrôlable**, puis de comparer leur lecture directe dans le
réseau. À K1, un niveau de fusion peut être une simple demi-distance ; une
cible exacte n'est pas automatiquement non locale. Le
[contrat de guidage](GUIDAGE_FULL_ET_PREENTRAINEMENT_20260926.md) impose un témoin
local et sépare graphes élève et enseignant. Projection douce et transport
entre coupes relèvent du [contrat de masses](CONTRAT_COUPES_ET_MASSES_20260926.md).

## 5. Les chiffres ne remplacent pas un protocole commun

DOS rapporte **73,1 mIoU après fine-tuning**, contre **67,5 en sonde linéaire**,
sur SemanticKITTI validation sans données additionnelles dans son tableau 1.
Données de pré-entraînement, taux d'annotation, tête et augmentations doivent
être alignés pour servir de cible. [DOS, tableau 1](https://ojs.aaai.org/index.php/AAAI/article/download/39030/42992).

Les anciens repères « base 68,0–70,3 » ne désignent pas une baseline unique
reproduite dans ce dépôt. Partir d'un commit et d'une configuration exécutables,
puis consigner leur résultat. Aucun chiffre de littérature ni cette recherche
ciblée ne constitue une expérience HGP ou une recherche d'antériorité exhaustive.
