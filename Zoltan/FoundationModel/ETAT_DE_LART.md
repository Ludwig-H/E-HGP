# Où est le verrou, et ce que la tour remplace

26 septembre 2026. Ce document répond à une seule question : **par rapport à
quoi mesure-t-on l'apport ?** Il identifie le composant que les architectures
actuelles fixent à la main, montre comment la littérature 2025–2026 en
rattrape les conséquences, et dresse la table de substitution qui sert de plan
de mesure.

## 1. Le verrou : toute architecture 3D code en dur une échelle métrique

Deux choix d'échelle reviennent souvent dans un encodeur 3D :

- **l'échelle de sous-échantillonnage** — la taille de voxel et les pas
  (MinkowskiNet, SparseConv), la liste de rayons et le $k$ (PointNet++,
  KPConv), la **taille de grille du *grid pooling*** (Point Transformer V2 et
  V3), le nombre de niveaux de la partition (Superpoint Transformer) ;
- **le système de voisinage** — rayon fixe, $k$ plus proches voisins, ou une
  **fenêtre de taille fixe sur une courbe remplissante** (PTv3 sérialise sur
  Z-order et Hilbert, puis découpe en *patches* non recouvrants).

Ces choix peuvent devoir être réaccordés quand le capteur, la portée ou le
domaine changent ; la tour HGP propose d'en dériver une partie des retours.
Dans un LiDAR automobile, la densité de retours dépend de la portée, de
l'incidence et des occultations ; la loi $1/d^{2}$ n'en est qu'une approximation
sur des surfaces favorables. **Un rayon fixe n'assure pas une population
comparable à toutes les portées.**

## 2. Comment la littérature récente rattrape le problème

Chacune des trois références les plus proches ajoute un correctif *à côté* de
l'architecture, sans toucher l'échelle elle-même.

| travail | ce qu'il garde | le correctif ajouté |
| --- | --- | --- |
| **Sonata** (CVPR 2025, *Highlight*) | PTv3, grid pooling, patch attention | identifie le **raccourci géométrique** — la SSL 3D s'effondre sur des indices spatiaux de bas niveau — et le *masque* : bruit gaussien sur les coordonnées masquées, ordonnanceur progressif de taille et de taux de masque, auto-distillation directement sur la sortie de l'encodeur, 140 k nuages |
| **Vernata** (IROS 2026) | Sonata, donc PTv3 | **augmentation par vues éparses** pour la robustesse aux densités variables, bancs de mémoire, distillation intermodale depuis un modèle de fondation 2D |
| **Utonia** (ICML 2026) | PTv3 | **Perceptual Granularity Rescale** — un rééchelonnage explicite de la granularité pour joindre télédétection, LiDAR extérieur, RGB-D intérieur, CAO et vidéo ; plus *Causal Modality Blinding* et un RoPE inter-domaines |

Le diagnostic se lit dans les noms mêmes : *rescale*, *sparse view
augmentation*, *obscuring spatial information*. Les trois traitent le symptôme
d'une échelle posée à la main. Aucun ne supprime la constante.

**Superpoint Transformer** (ICCV 2023) est le seul à attaquer la structure :
il remplace les points par une **partition hiérarchique de superpoints** qui
« s'adapte aux propriétés locales de l'acquisition à plusieurs échelles
simultanément », puis fait de l'attention éparse entre superpoints. Avec
212 k paramètres il atteint $76{,}0$ sur S3DIS, $63{,}5$ sur KITTI-360 et
$79{,}6$ sur DALES, soit jusqu'à 200 fois plus compact que l'état de l'art.
**C'est l'antécédent architectural le plus important du projet, et il faut le
citer comme tel.** Sa partition reste cependant obtenue par une énergie de
partition minimale réglée par un paramètre de régularisation par niveau : elle
est adaptative mais non canonique, et c'est une partition à un paramètre, pas
une filtration.

Enfin **ALPINE** (2025) montre qu'un simple regroupement géométrique, sans
aucun apprentissage d'instance, atteint $\mathrm{PQ} = 64{,}2$ sur
SemanticKITTI avec les seules étiquettes sémantiques. Autrement dit : une part
substantielle de la structure d'instance est déjà dans la géométrie. C'est un
argument fort pour le projet — et un témoin exigeant.

## 3. Ce que la tour Morse HGP apporte, mathématiquement

Pour tout $(K,r)$, la tour donne les composantes de
$L_K(r)=\lbrace y:|B(y,r)\cap\mathcal X|\geq K\rbrace$ selon la
spécification Morse HGP 3D. Le Théorème 2 relie les K-polyèdres aux amas
discrets de forte densité de l'estimateur K-NN ; les minima Gabriel et la
mosaïque d'ordre K rendent leur histoire calculable. FULL publie événements,
populations et cartes verticales, à niveaux exacts.

L'architecture proposée lit plusieurs historiques K et plusieurs niveaux r.
Ce choix préserve des alternatives de sensibilité et de robustesse que perdrait
une coupe unique ; il ne prouve pas, à lui seul, que les coupes ou jetons
choisis par le réseau soient stables ou utiles. C'est l'objet des substitutions
et des témoins négatifs ci-dessous.

## 4. La table de substitution

C'est le cœur du plan de mesure. La tour ne s'ajoute pas à une architecture :
elle **remplace, un par un**, les composants qui portent la constante métrique.
Chaque ligne est une expérience contrôlée à budget identique.

| primitive | ce que fait l'état de l'art | ce que fournit la tour | remplace |
| --- | --- | --- | --- |
| échelle de sous-échantillonnage | grid pooling (taille de voxel), FPS + rayon, niveaux de superpoints | une **échelle de recouvrements** lue dans la forêt de fusion | `GridPool` de PTv3 |
| opérateur de regroupement | max ou moyenne sur une cellule | une **matrice d'affectation douce** aux poids du § 9.1, conservant la masse | *pooling* de cellule |
| système de voisinage | fenêtre sur Z-order/Hilbert, $k$-NN, rayon fixe | le **graphe de fusion** : deux nœuds voisins s'ils fusionnent, pondérés par le rayon de fusion | *patch grouping* |
| encodage de position relative | décalage $xyz$, RoPE | **biais ultramétrique** $\varphi(\log r_{uv})$, où $r_{uv}$ est le niveau de fusion | encodage relatif métrique |
| axe de robustesse | aucun | l'**ordre $K$**, avec sa carte verticale dont la naturalité est vérifiée | *sans équivalent* |
| retour aux points | interpolation trilinéaire ou $k$-NN | le **vote pondéré** du § 9.1, avec la Proposition 7 | décodeur d'interpolation |
| tâche prétexte | masquage de coordonnées, auto-distillation | **modélisation de filtration** : cibles non locales exactes | prétexte sujet au raccourci géométrique |

La dernière ligne mérite un mot. Sonata a montré que la SSL 3D s'effondre parce
que **la géométrie est l'entrée** : prédire une coordonnée masquée se résout
par interpolation locale. Un rayon de fusion entre deux composantes est au
contraire une grandeur de **percolation** — il dépend du goulot de densité
entre elles, donc d'une intégration sur tout l'espace intermédiaire. Il peut
porter une information non locale, mais certains cas simples se résolvent
localement ; les prétextes HGP doivent battre un témoin géométrique local et
masquer les variables qui révèlent déjà la cible.

## 5. Ce qui reste à vérifier dans cette section

- La recherche d'antériorité ci-dessus est **ciblée sur les décisions de
  conception** ; elle ne remplace pas une recherche exhaustive au moment de la
  soumission. Les chiffres cités proviennent des publications et n'ont pas été
  reproduits ici.
- Le raccord exact entre les objets FULL v9, les facettes projectables du
  § 9.1 et les coupes consommées par HGP-UNet doit être spécifié et testé.
  FULL seul ne livre pas encore les poids et matrices du tokenizer.
- Les repères hérités sur SemanticKITTI ($73{,}1$ pour DOS, base reproductible
  à $68{,}0$–$70{,}3$) n'ont jamais été reproduits dans ce dépôt et doivent
  l'être avant de servir de cible.
