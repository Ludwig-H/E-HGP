# Note — l'ordre des preuves et le centre de gravité du papier

Cette note ne conteste ni l'objectif — segmentation sémantique SemanticKITTI d'abord, instance ensuite — ni la spécification déjà écrite. Elle porte sur deux décisions distinctes : **dans quel ordre acquérir les preuves**, et **où placer le budget de nouveauté** d'une soumission.

## Constat sur l'état du dossier

Le dossier contient une spécification d'une qualité supérieure à la plupart des sections méthodes publiées : contrat d'entrée versionné, quatre carriers nommés et séparés, politique de coupe, matrice d'ablation appariée à dix lignes, douze questions falsifiables, onze risques avec no-go chiffrés, programme T0–T6. Il ne contient **aucune mesure**.

Le rendement marginal d'une ligne de spécification supplémentaire est donc aujourd'hui proche de zéro, et le rendement marginal de la première mesure est très élevé. Toute extension de la spécification avant cette mesure retarde la seule information qui puisse changer une décision.

## Le chemin critique n'est pas le descripteur

La chaîne d'implications réelle est :

hiérarchie calculable au bon coût → hiérarchie alignée sur la sémantique → descripteur de nœud informatif → opérateur hiérarchique utile → gain mIoU → gain mIoU supérieur à la concurrence.

Chaque maillon peut casser le suivant, et les deux premiers ne dépendent ni du descripteur ni de l'opérateur. Or ce sont eux qui sont les moins vérifiés :

- le maillon 1 dépend de la mosaïque d'ordre supérieur, dont la taille croît avec $K$ et $n$ ; le pipeline historique de la thèse sur SemanticKITTI tournait de l'ordre de la seconde par trame, pour une cible de dix trames par seconde ;
- le maillon 2 est exactement le risque [R1](RISKS_AND_GO_NO_GO.md) : dans un scan LiDAR, la densité locale est d'abord une fonction de la portée, de l'angle d'incidence et de l'occultation, pas de la classe. Le modèle de Hartigan suppose un échantillon d'une densité $f$ sur $\mathbb{R}^{3}$ ; un scan LiDAR est un échantillonnage de **surfaces** à densité angulaire fixée. L'hypothèse statistique qui fonde HGP n'est pas satisfaite telle quelle.

Tant que les maillons 1 et 2 ne sont pas mesurés, discuter du choix entre `support + rayon`, `support + objet marqué complet` ou toute autre variante revient à optimiser le maillon 3 d'une chaîne dont on ignore si le maillon 2 tient.

### Une correction sur le coût, et une objection plus grave que le coût

Sur le **coût**, le manuscrit fournit lui-même la sortie, et il faut la prendre au sérieux avant de conclure à un blocage. La voie exacte — Čech, mosaïque de Delaunay d'ordre $K$, $K$-graphe de Gabriel — coûte cher : la triangulation de Delaunay ordinaire est déjà en $\mathcal{O}\left(n^{\lceil p/2\rceil}\right)$ au pire dans $\mathbb{R}^{p}$. Mais le § 9.3 propose le complexe de Vietoris–Rips avec l'encadrement $\check{C}(X,r)\subseteq\mathrm{VR}(X,r)\subseteq\check{C}\left(X,\alpha_p r\right)$ et $\alpha_p=\sqrt{2p/(p+1)}$, soit $\alpha_3\approx1{,}22$ seulement en dimension $3$ — un encadrement serré. Cette voie se réduit à quatre opérations de graphe massivement parallélisables, et pour $K=2$ l'énumération des cliques est une simple énumération de triangles par intersection de listes d'adjacence triées, dont le coût suit le nombre réel de triangles et non $\binom{n}{K+1}$. Sur un graphe $k$-NN de scan LiDAR, c'est une charge GPU ordinaire.

Le prix n'est donc pas le temps, c'est **l'exactitude** : sous Vietoris–Rips, le Théorème 2 — correspondance exacte entre $K$-polyèdres et amas discrets de forte densité $K$-NN — ne tient plus qu'à un facteur $\alpha_3$ près sur le rayon. Le papier ne pourra plus dire « exact », seulement « interpolé entre deux niveaux distants de $22\,\%$ ». C'est un arbitrage à décider explicitement, pas à subir.

L'objection sérieuse est ailleurs, et elle est écrite dans le manuscrit lui-même. Sur le jeu `birch2`, HDBSCAN est presque parfait tandis que HGP-Clusterer fusionne indûment : « les clusters sont essentiellement filiformes et sont donc mieux identifiés avec de simples graphes ». Le mécanisme est structurel, pas anecdotique. La connexité d'ordre $K$ exige que $K$ points soient **simultanément** proches. Le long d'une structure filiforme ou d'une surface mince échantillonnée de façon éparse, cette condition n'est satisfaite qu'à un rayon nettement plus grand que celui qui suffirait à une connexité par arêtes. La structure mince naît donc tard dans la filtration — et à ce niveau tardif, ses voisines l'ont déjà rejointe. Le résultat observable est bien celui du manuscrit : sous-segmentation des objets fins, pas fragmentation. HGP achète sa résistance au chaînage en retardant la naissance des objets minces.

Or les classes qui portent la marge de progression du mIoU SemanticKITTI sont exactement celles-là : `pole`, `traffic-sign`, `bicycle`, `person`, `bicyclist`, `motorcyclist`, `fence`. Les classes épaisses et bien remplies — `road`, `building`, `vegetation`, `terrain` — plafonnent déjà au-delà de $90$ d'IoU.

**C'est l'objection la plus spécifique et la plus dangereuse du dossier**, et elle n'apparaît dans aucun des risques R1–R11 : il existe une tension structurelle entre l'avantage revendiqué de HGP et le profil des classes qui décident de la métrique visée. Elle doit devenir un risque numéroté à part entière, avec son propre test — mIoU-oracle stratifié par dimension intrinsèque estimée du nœud, ou au minimum par classe fine contre classe volumique — et sa propre atténuation. Le manuscrit en suggère une : sur `birch2`, changer d'estimateur, $\hat\rho=1/r^{2}$, résout le problème. Une atténuation testable ne dispense pas de mesurer d'abord l'ampleur du problème.

## Les deux premières mesures, sans aucun entraînement

### M1 — Oracle d'antichaîne, alignement sémantique de l'arbre

Construire la forêt HGP sur la séquence 08, choisir une **antichaîne** — un ensemble de nœuds deux à deux non emboîtés couvrant tous les points — étiqueter chaque nœud par sa classe majoritaire, et calculer le mIoU obtenu. C'est le plafond atteignable si le modèle prédisait parfaitement une étiquette par région.

Attention à la formulation : le mIoU n'est pas additif sur les régions, donc « la meilleure antichaîne au sens du mIoU » n'est pas un problème d'optimisation raisonnable et ne doit pas être annoncé comme tel. La version correcte comporte deux volets. **(i)** À budget de régions fixé $R$, sélectionner l'antichaîne qui minimise l'**impureté totale** $\sum_{v}n_v H\left(\pi_v\right)$, critère additif sur les nœuds : ce problème admet une programmation dynamique exacte sur l'arbre, en une passe ascendante, avec un état « nombre de régions consommées dans le sous-arbre ». Rapporter ensuite le mIoU de l'antichaîne ainsi obtenue, comme descripteur et non comme optimum. **(ii)** En complément, rapporter le mIoU-oracle de coupes à niveau fixé, qui est la convention de la littérature superpoint et permet la comparaison directe avec ses chiffres publiés.

Tracer ces courbes en fonction du nombre de régions, à taux de compression apparié, contre : HDBSCAN/RSL au même $K$, un octree ou une grille de voxels, une partition superpoint, et un arbre aléatoire de mêmes tailles de régions.

Stratifier obligatoirement par portée et par classe, et rapporter séparément les classes rares. C'est le seul diagnostic qui teste **l'effet arbre** sans confondre avec le descripteur ni avec l'opérateur, et il ne demande que la hiérarchie et les labels.

Règle de décision : si HGP ne domine aucun contrôle à compression égale, ou si la domination disparaît après stratification par portée, le programme sémantique est réfuté et il faut publier ce résultat négatif plutôt que l'absorber.

**Avertissement décisif sur l'interprétation de M1, à lire avant de l'exécuter.** La littérature superpoint publie déjà cet oracle, et son verdict est défavorable à l'hypothèse « meilleure partition $\Rightarrow$ meilleure segmentation ». SPG (CVPR 2018) rapporte sur S3DIS 6-fold un oracle de partition à $88{,}2$ mIoU pour un modèle à $62{,}1$. SPT (ICCV 2023) écrit que « the performance of SPT is more than 20 points below the oracle, suggesting that the partition does not strongly limit its performance » — soit un oracle $\gtrsim89$ sur Area 5 pour un modèle à $68{,}9$. SuperCluster (3DV 2024) constate de même que « the high performance of this oracle ($93{,}4$ PQ) indicates that very little precision is lost by working with superpoints ».

Autrement dit : **ces méthodes laissent déjà vingt points d'oracle non convertis.** Améliorer le plafond d'une partition qui n'est pas saturée ne peut pas produire de gain. M1 est donc une **porte de réfutation, pas une porte de promotion** : le perdre tue le programme, le gagner ne prouve presque rien. Il faut l'exécuter pour cette raison-là, et présenter son résultat avec cette asymétrie explicite.

Le corollaire est direct et doit être accepté : si le goulot n'est pas la partition, alors la valeur de HGP ne peut pas venir de la **qualité** de ses régions. Elle ne peut venir que d'autre chose — le fait que ses niveaux soient des niveaux de densité interprétables, la théorie qui prédit ce qui est récupérable, ou le recouvrement pour $K\geq2$. C'est cela qu'il faut mettre au centre, et non « notre arbre est meilleur ».

Deux données chiffrées confirment la même hiérarchie des priorités, et elles proviennent d'ablations publiées sur cette famille exacte de modèles. SPT : retirer toutes les features handcrafted de nœud coûte $-0{,}7$ mIoU sur S3DIS 6-fold, $-4{,}1$ sur KITTI-360, $-1{,}4$ sur DALES ; retirer l'encodage d'adjacence coûte $-6{,}3$ / $-5{,}4$ / $-3{,}0$ ; passer à un seul niveau de partition coûte $-8{,}4$ / $-5{,}1$ / $-0{,}9$. EZ-SP (ICRA 2026) va plus loin : remplacer les features handcrafted par un petit réseau appris change le résultat de $\pm0{,}1$ mIoU. **Le descripteur de nœud est le levier le plus faible des trois ; l'adjacence et le nombre de niveaux dominent.** C'est la réponse quantitative à la question « faut-il deux vecteurs ou l'objet marqué complet ? » : à ce stade, ni l'un ni l'autre ne décide.

### M2 — Stabilité sous transport et rééchantillonnage

Prendre des objets étiquetés à portée courte, les transporter à plusieurs portées et rééchantillonner selon un modèle capteur déclaré — amincissement angulaire, disparition de retours, changement d'occultation. Mesurer la dérive des niveaux de naissance/mort, la stabilité de l'ancêtre commun et la persistance relative.

Tester en même temps la correction : remplacer le niveau brut $a$ par un niveau **normalisé par la densité d'échantillonnage attendue** du capteur à cette portée et à cet angle d'incidence. Si cette correction ramène la dérive intra-objet nettement sous la séparation interclasse, elle devient une contribution en soi — une filtration HGP consciente du capteur — et elle répond à R1 au lieu de le contourner.

Ces deux mesures coûtent moins d'une journée de calcul et tranchent davantage que toute la matrice d'ablation prévue en aval.

### M3 — Le raccourci qui donne un nombre publiable le plus vite

[STATE_OF_THE_ART.md](STATE_OF_THE_ART.md) conclut, à juste titre, qu'« utiliser un clustering pour les instances » n'est plus une contribution suffisante, et [RISKS_AND_GO_NO_GO.md](RISKS_AND_GO_NO_GO.md) déclare la phase instance fermée. Ce qui suit ne conteste ni l'un ni l'autre : il ne s'agit pas de faire de l'instance la **contribution**, mais d'en faire la **mesure** qui teste l'effet arbre au coût le plus bas, et dont le résultat conditionne l'engagement de semaines de GPU sur la voie sémantique. Une phase fermée pour la publication peut rester ouverte pour le diagnostic.

C'est la mesure la plus rentable du dossier, et les chiffres d'ALPINE la rendent immédiatement exploitable.

Le clusterer d'ALPINE est, littéralement, **du single-linkage** : projection BEV par classe *thing*, graphe $k$-NN avec $k=32$, suppression des arêtes plus longues qu'un seuil constant par classe $t_c$ tiré de dimensions d'objets trouvées sur le web, puis composantes connexes, plus un découpage récursif des composantes dont la boîte dépasse la boîte de référence de $30\,\%$. Autrement dit : **HGP à $K=1$, avec un rayon dépendant de la classe et une projection BEV.** HGP à $K\geq2$ en est exactement la généralisation d'ordre supérieur — la généralisation que la thèse construit et justifie.

Le mode d'échec déclaré d'ALPINE est aussi exactement celui que HGP prétend corriger : « failure cases can be crafted by making two objects closer than the chosen threshold », c'est-à-dire l'**effet de chaînage**, motivation centrale de toute la thèse.

Les chiffres publiés fixent le cadre de l'expérience, et ils sont à double tranchant.

- **Le clusterer compte beaucoup.** À sémantique MinkUNet identique et découpage de boîtes désactivé pour tous, SemanticKITTI val : ALPINE $65{,}5$ PQ à $14{,}4$ Hz, D&M $61{,}8$, DBSCAN $56{,}7$, HDBSCAN $55{,}1$. L'écart imputable au seul clusterer est de $10{,}4$ PQ, et HDBSCAN — dont HGP est le correctif de principe — est bon dernier.
- **Le plafond est bas.** L'oracle d'instance à sémantique figée ne rapporte que $+4{,}3$ PQ sur nuScenes avec PTv3 ($78{,}9\to83{,}2$) et $+3{,}8$ avec WaffleIron-768. Sous sémantique parfaite, le clusterer d'ALPINE ne perd déjà que $1{,}0$ PQ sur SemanticKITTI. Les auteurs concluent que « instance extraction is largely saturated ».
- **Le barème est donc serré mais lisible.** ALPINE ne bat les têtes d'instance entraînées que de $+0{,}1$ à $+0{,}8$ PQ, et D&M de $+3{,}7$. Dans ce contexte, $+1$ à $+2$ PQ sur ALPINE à sémantique figée serait un résultat fort, et $+4$ serait le maximum atteignable.
- **La contrainte réelle est le temps.** ALPINE tourne à $14{,}4$ Hz sur un seul cœur CPU. Le pipeline HGP historique de la thèse était de l'ordre de la seconde par trame. Il y a plus d'un ordre de grandeur à combler, et aucun jury ne l'ignorera.

L'expérience à faire est donc à une seule variable : **reprendre le pipeline ALPINE tel quel — mêmes logits sémantiques gelés publics, mêmes $t_c$, même découpage de boîtes — et remplacer uniquement les composantes connexes par les $K$-polyèdres à $K=2,3$.** Rapporter PQ, PQ_Th, la ventilation par portée et le temps. Aucun entraînement, une baseline publiée, un plafond publié, et une hypothèse — la connexité d'ordre supérieur résiste au chaînage — qui est précisément celle de la thèse.

Cela ne change pas l'objectif sémantique. Cela fournit une preuve d'existence de l'effet arbre à un coût de calcul marginal, contre une baseline que personne ne contestera, avant d'engager des semaines de GPU sur un backbone. Et si HGP ne bat pas du single-linkage sur la tâche que la thèse a conçue pour lui, il est peu probable qu'il apporte quoi que ce soit à la segmentation sémantique.

**Un signal contraire à consigner.** L'ablation d'ALPINE montre qu'un seuil proportionnel à la portée, à la manière de LESS, donne $75{,}9$ PQ contre $76{,}3$ pour le seuil constant par classe, malgré une optimisation de son coefficient à l'échelle du jeu de données. Une correction de portée naïve **dégrade** donc leur clustering. Cela n'invalide pas la correction proposée en M2 — leur seuil est un rayon de liaison, pas un niveau de densité $K$-NN, et les deux ne se corrigent pas de la même façon — mais cela interdit de présenter la correction range-aware comme acquise. Elle doit être mesurée, et le résultat négatif d'ALPINE cité.

## La tension centrale : la laminarisation détruit ce qui distingue HGP

Ce point mérite d'être énoncé seul, parce qu'il conditionne le choix de l'opérateur.

Ce qui sépare HGP de HDBSCAN n'est pas la hiérarchie mais la **connexité d'ordre supérieur** : pour $K\geq2$, les $K$-polyèdres **se recouvrent**, un même point pouvant appartenir simultanément à plusieurs composantes d'un même niveau. Le manuscrit le souligne dès la figure des six points : « on voit déjà apparaître le phénomène essentiel : pour $K\geq2$, les polyèdres peuvent se recouvrir ».

Or HSA exige un arbre strictement laminaire : son lemme de sous-structure optimale est énoncé sur une **partition** de l'ensemble d'indices, avec des ensembles de feuilles disjoints, et sa définition de la distance positionnelle repose sur l'unicité des chemins vers la racine. Alimenter HSA impose donc de laminariser, c'est-à-dire d'attribuer chaque point à une seule composante.

La conséquence est inconfortable et doit être assumée : **la projection laminaire supprime exactement la propriété qui distingue HGP de la concurrence.** Ce qui reste après laminarisation est un arbre de fusion de densité, c'est-à-dire, du point de vue de l'opérateur, une variante de ce que HDBSCAN ou une partition superpoint fournissent déjà — avec un coût de construction plus élevé. Et à $K=1$, HGP **est** le single-linkage : la configuration la plus simple du programme est aussi celle qui n'a aucune nouveauté structurelle.

Deux issues seulement, et il faut choisir explicitement.

1. Assumer la laminarisation, et faire porter la contribution sur autre chose que la structure : la théorie de récupérabilité, la correction capteur, ou le descripteur. La comparaison honnête devient alors « notre arbre laminarisé contre les autres arbres », et elle doit être gagnée sur l'oracle d'antichaîne, pas sur un argument théorique.
2. Traiter le recouvrement comme l'objet, c'est-à-dire construire l'opérateur d'attention sur le **DAG de recouvrement**, avec des poids d'appartenance $w_{iv}$, conservation de masse et réduction exacte au cas laminaire. C'est exactement T6 du [programme théorique](THEOREM_PROGRAM.md), qualifié à juste titre de « plus distinctif mais plus risqué ».

C'est la seconde issue qui mérite le budget de nouveauté d'un opérateur. Si l'on doit inventer une attention, il faut l'inventer là où la mathématique de HGP survit à l'opérateur — sur les recouvrements — et non reproduire HSA sur un arbre dont la laminarisation a déjà effacé la différence.

## Où placer le budget de nouveauté

Trois actifs sont candidats, et ils n'ont pas la même valeur.

- **HSA** est un actif faible pour une soumission, et trois faits tirés du papier lui-même le confirment au-delà de l'absence d'expérience 3D. Premièrement, sous LayerNorm l'énergie d'interaction ne dépend que des **moyennes pondérées par la taille** des requêtes et des clés de chaque sous-arbre : c'est cette réduction à une moyenne qui produit tout le gain de complexité. Mécaniquement, HSA fidèle est donc une attention sur moyennes de sous-arbres, c'est-à-dire très proche du contrôle « bottom-up/top-down `mean` + MLP » déjà prévu dans la matrice d'ablation. Sa nouveauté est la **dérivation** — optimalité KL sous contrainte de blocs — pas le calcul. Deuxièmement, les auteurs indiquent explicitement que leur cadre **n'ajoute aucun paramètre apprenable** le long de la hiérarchie. Troisièmement, le remplacement zero-shot dégrade fortement certaines tâches, jusqu'à l'effondrement au niveau du hasard sur QNLI. L'employer fidèlement est correct et nécessaire comme baseline ; le reproduire ne produit pas de nouveauté. `QC-HSA` est une variation utile qui reste, de l'aveu du [programme théorique](THEOREM_PROGRAM.md), trop proche des précédents multi-échelles pour porter seule un papier.

  Deux conséquences directes pour le descripteur de nœud, qui doivent être écrites avant l'implémentation. **(a)** Dans HSA fidèle, le descripteur géométrique n'entre que par l'embedding positionnel, via le seul produit scalaire $\varepsilon_\Omega(A')^{\top}\varepsilon_\Omega(B')$ entre frères : tout le descripteur, aussi riche soit-il, est comprimé en **un scalaire de biais par couple de frères**. Débattre du contenu du descripteur sans changer ce goulot revient à raffiner une entrée dont la sortie est un nombre. Si le descripteur doit compter, il faut le faire entrer aussi dans la voie des valeurs, ce qui sort du théorème HSA et doit être annoncé comme variante. **(b)** L'algorithme demande $D$ produits matrice creuse–vecteur **séquentiels**, où $D$ est la profondeur de la hiérarchie. Or un arbre de fusion issu d'une filtration est typiquement très déséquilibré, avec de longues chaînes de fusions ponctuelles : sa profondeur peut être du même ordre que le nombre de points. Une condensation préalable — au sens `min_cluster_size` de HDBSCAN, ou une autre règle déclarée — n'est donc pas une optimisation optionnelle mais une **condition d'existence** du modèle sur GPU ; et cette condensation modifie l'objet, donc doit être versionnée et ablatée comme tel.
- **Le descripteur** est un actif moyen. Le [théorème de caractérisation](NOTE_CLAUDE_DEUX_CANAUX_DIRECTIONNELS.md) donne une raison forte de choisir la fonction support plutôt qu'un descripteur arbitraire, et la dichotomie continu/discontinu explique pourquoi il faut un second canal. C'est un bon lemme ; ce n'est pas une contribution centrale.
- **HGP et son analyse par percolation** sont l'actif fort, et c'est le seul que personne d'autre ne possède. La correspondance exacte entre $K$-polyèdres et amas discrets de forte densité $K$-NN, la fonction de percolation $\Theta^{cc}_{K,p}$, la vitesse de percolation et la limite gaussienne $\mu=K+a\sqrt{K}$ constituent une théorie quantitative de **la fraction d'un cluster récupérable avant fusion parasite**. Aucune équipe de vision 3D ne dispose de cet outil, et il répond exactement à la question que personne ne sait poser proprement : quelle hiérarchie de partition est récupérable, et à quel niveau.

La conséquence est directe : le centre du papier ne doit pas être « nous avons ajouté HSA sur un arbre HGP et gagné du mIoU », mais « voici une théorie de ce qui est récupérable dans une hiérarchie de densité, voici sa version valable pour un échantillonnage capteur inhomogène, voici la mesure qui la vérifie sur données réelles, et voici l'opérateur qui l'exploite ». Un gain de segmentation devient alors une validation, pas la contribution — ce que [REVIEWER_VERDICT.md](REVIEWER_VERDICT.md) demande déjà, mais appliqué au bon actif.

Le point théorique le plus prometteur est l'extension de l'analyse de percolation à une **intensité inhomogène** $\lambda(x)$ modélisant la portée. C'est difficile en toute généralité ; une version locale par changement d'échelle, prédisant que le niveau critique se déplace comme une fonction explicite de la portée, et vérifiée empiriquement sur SemanticKITTI, suffirait à porter la partie théorique et résoudrait R1 dans le même geste.

## Deux faits de veille qui recadrent la cible

**La barre val honnête est vers $68$, pas vers $76$.** Les chiffres de $75$–$76$ du dossier sont des scores *test* obtenus dans des régimes hétérogènes : TASeg $76{,}5$ agrège explicitement une fenêtre de trames passées, LSK3DNet $75{,}6$ annonce lui-même « instance CutMix and Test Time Augmentation … and extra training epochs », PTv3+PPT $75{,}5$ s'entraîne sur plusieurs jeux, UniSeg $75{,}2$ est multimodal ; RAPiD-Seg $76{,}1$ reste le meilleur candidat LiDAR-seul. En régime *val, mono-scan, LiDAR seul, sans TTA ni ensemble* — celui du dossier — une reproduction récente et contrôlée place MinkUNet à $63{,}8$, Cylinder3D $64{,}3$, SPVNAS $64{,}7$, PTv3 reproduit $66{,}2$, SphereFormer $67{,}8$, WaffleIron-256 $68{,}0$. Le $70{,}8$ val annoncé par PTv3 n'est pas reproductible et fait l'objet d'une issue ouverte chez Pointcept. La cible réaliste à atteindre puis à dépasser est donc **$\approx68$ mIoU val**, ce qui est nettement moins décourageant que $76$ — et il faut le dire ainsi dans le dossier plutôt que de viser un nombre issu d'un autre régime.

**Aucune méthode de la lignée superpoint ne publie SemanticKITTI mono-scan.** Ni SPG, ni SSP, ni SPNet, ni SPT, ni SuperCluster, ni EZ-SP : tous rapportent S3DIS, ScanNet, DALES ou KITTI-360 *accumulé*. Cela se lit dans les deux sens. C'est un créneau libre — personne n'occupe « partition hiérarchique sur scan LiDAR unique ». C'est surtout un **avertissement** : cette communauté a de facto concédé le mono-scan aux méthodes voxel/point denses, très probablement parce qu'un scan unique offre trop peu de points par région pour qu'une partition soit informative. Entrer sur ce benchmark avec une méthode par partition, c'est attaquer le seul terrain où cette famille a échoué à s'imposer. Il faut soit une raison explicite de croire que HGP change cela, soit un changement de terrain.

## Barre de publication, sans complaisance

Un gain de mIoU sur SemanticKITTI, même net, n'est pas un papier NeurIPS ou ICML : c'est un papier de vision, donc CVPR/ICCV/ECCV, et il y sera jugé sur l'ingénierie du backbone autant que sur l'idée. Atteindre l'état de l'art strict demande en outre une recette d'entraînement lourde qui n'est pas l'avantage comparatif de ce projet.

À l'inverse, une théorie de la récupérabilité des hiérarchies de densité sous échantillonnage capteur, accompagnée d'un benchmark causal des hiérarchies et d'un opérateur qui en tire parti, est une soumission NeurIPS/ICML plausible **sans exiger le premier rang d'un classement**. C'est le chemin où les chances de publication haute et la valeur scientifique coïncident.
