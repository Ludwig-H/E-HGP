# HGP-UNet — l'architecture

26 septembre 2026. Conception. Aucune expérience apprise n'est rapportée.

Prérequis : [`ETAT_DE_LART.md`](ETAT_DE_LART.md), qui établit ce que la tour
remplace, et [`OBJET.md`](OBJET.md), qui dit ce qu'elle fournit.

## 1. La thèse, en un paragraphe

Tout encodeur 3D contient une **échelle métrique posée à la main** — taille de
voxel, liste de rayons, $k$, taille de *patch* sur une courbe remplissante — et
c'est exactement ce qui casse quand le capteur, la portée ou le domaine
changent. Les modèles de fondation 3D de 2025–2026 ne suppriment pas cette
constante : ils en rattrapent les effets par du rééchelonnage, de l'augmentation
et du brouillage. La tour HGP fournit à la place une **échelle canonique,
dérivée des données, et prouvée stable en tant qu'objet multiparamètre**. On ne
l'ajoute donc pas à une architecture : **on la substitue aux composants qui
portent la constante.**

Le modèle qui en découle n'est pas un nouveau réseau exotique. C'est un U-Net /
Transformer ordinaire dont **le pooling, le voisinage, l'encodage de position
relative et le décodeur** sont tous lus dans la bifiltration $(K, r)$. La
nouveauté est dans ce qui structure le calcul, pas dans les couches.

## 2. Correction d'une erreur de cadrage

Une première lecture des chiffres de la tour (1,3 M à 16,3 M nœuds par trame)
conduit naturellement à dire : « il faut sélectionner quelques milliers de
jetons ». **C'est le mauvais cadre, et il faut l'abandonner.**

Un U-Net ne consomme pas l'ensemble des nœuds : il consomme **$L$ coupes**,
c'est-à-dire $L$ recouvrements emboîtés de l'ensemble des points. Avec un
rapport de $4$ par niveau sur une trame brute :

```text
  points   120 000
  niveau 1  30 000
  niveau 2   7 500
  niveau 3   1 900
  niveau 4     470
  niveau 5     120
  total    ~160 000 unités, tous niveaux confondus
```

C'est **le même ordre de grandeur que PTv3 sur la même trame**. Les 16 M nœuds
de la tour ne sont jamais matérialisés : ils sont l'espace dans lequel on lit
$L$ partitions. Le coût de l'architecture est donc celui d'un U-Net 3D
ordinaire, et la question n'est pas « combien de jetons » mais **« quel chemin
prendre dans le treillis $(K, r)$ »**.

## 3. Les six primitives

La tour fournit exactement six objets dont une architecture 3D a besoin et
qu'elle se procure aujourd'hui par des constantes.

1. **Une échelle de recouvrements** $A_1 \succ A_2 \succ \cdots \succ A_L$ :
   remplace le *grid pooling*, le FPS et les niveaux de superpoints.
2. **Des matrices d'affectation douces** $P_\ell$, aux poids du § 9.1 :
   remplacent le *pooling* de cellule, sont différentiables et conservent la
   masse.
3. **Un graphe de fusion** par niveau : deux nœuds sont voisins s'ils
   fusionnent, et l'arête porte le **rayon de fusion**. Remplace le graphe
   $k$-NN et la fenêtre sur la sérialisation.
4. **Un axe d'ordre $K$**, avec une carte verticale dont la v9 vérifie la
   naturalité. **Sans équivalent dans aucune architecture existante.**
5. **Des scalaires structurels exacts** par nœud : naissance, mort,
   persistance, mélange d'arités, degré de multifusion, population. Gratuits, à
   la fois comme variables d'entrée et comme cibles de pré-entraînement.
6. **Une lecture exacte vers les points** (§ 9.1 et Proposition 7) : remplace
   l'interpolation trilinéaire ou $k$-NN du décodeur.

## 4. Le vrai problème de conception : le chemin dans le treillis

### 4.1 Pourquoi une coupe horizontale est le mauvais objet

La densité LiDAR décroît en $1/d^{2}$. À un rayon global $r$, le champ proche
est déjà un bloc unique alors que le champ lointain est encore de la poussière.
Une coupe horizontale est donc empiriquement mauvaise — et, ce qui est plus
grave, **théoriquement mauvaise** : Rolle et Scoccola montrent que les tranches
à un paramètre de la bifiltration par degré redonnent les méthodes connues de
regroupement par densité mais **sont instables**, tandis que l'objet
multiparamètre est stable.

Conséquence directe sur l'architecture : **l'échelle ne doit jamais être un
rayon global, et le modèle doit voir plusieurs ordres.** Ce n'est pas une
préférence de conception, c'est une conséquence d'un théorème de stabilité.

### 4.2 Deux axes indépendants, donc trois familles de chemins

La bifiltration offre ce qu'aucune architecture n'a : **deux axes de
grossissement indépendants**.

- horizontal, $r$ croissant à $K$ fixé : **grossir en espace à densité
  décroissante** ;
- vertical, $K$ croissant à $r$ fixé : **grossir en exigence de densité à
  échelle fixée** ;
- diagonal, $K$ et $r$ croissant ensemble à $\hat f_K \propto K/r^{3}$ constant :
  **grossir en espace à densité constante** — un véritable espace d'échelle
  iso-densité.

Le chemin diagonal est à mon avis le bon défaut, et c'est un choix de fond : il
sépare proprement « je regarde plus grand » de « je regarde plus dense », ce
qu'aucun voxel ne sait faire. Il doit être mesuré contre les deux autres.

### 4.3 La condensation : l'étape que le manuscrit prescrit déjà

Avant de contracter, il faut **condenser**, au sens exact de HDBSCAN. Ce n'est
pas un ajout emprunté à l'extérieur : le § 9.1 du manuscrit le prescrit, avec
le poids qu'il faut.

> « La masse d'une face dans l'arbre condensé est alors
> $m_\tau = S_\tau \sum_{x \in \tau} 1/T_x$ […] C'est ce poids $m_\tau$, et non
> le simple comptage des faces, qui est utilisé par le seuil
> `min_cluster_size` dans l'arbre condensé (mêmes idées algorithmiques que
> HDBSCAN). »

**Pourquoi c'est nécessaire et non optionnel.** La forêt brute est dominée par
des événements triviaux. À $K = 1$ sur une trame sans sol de 39 885 sites, il y
a 39 796 fusions, c'est-à-dire le dendrogramme complet du Single-Linkage : la
quasi-totalité sont des « un point rejoint une grosse composante ». Le § 4.4.3
du manuscrit les nomme pour ce qu'ils sont — du bruit de micro-composantes —
et note que l'élagage de HDBSCAN est en réalité **un seuil de percolation** non
formulé comme tel. Une échelle construite en comptant les fusions brutes
dépenserait ses premiers niveaux à absorber des singletons.

**Ce que la condensation rend.** Quatre choses, dont trois sont des entrées
directes de l'architecture :

1. le **squelette** : les vraies scissions, sans les continuations ;
2. la **stabilité** de chaque nœud, $\widehat{E}(C) \propto \sum_{x \in C} (\hat\lambda_x - \hat\lambda_{\min})$,
   c'est-à-dire l'ordre dans lequel contracter ;
3. le niveau de sortie $\hat\lambda_x$ de chaque point, **rendu comme variable
   par point**. La condensation est donc un *changement de représentation* et
   non une perte : ce qu'elle retire de la structure, elle le rend en scalaire ;
4. la **tête de sélection** du modèle, par le mécanisme du § 5.2 (voir § 5 bis).

**Ce qu'elle doit rendre stable, et c'est une prédiction.** L'arbre brut bouge
beaucoup sous décimation : chaque micro-fusion se déplace. L'arbre condensé ne
garde que les événements qui ont de la masse, donc précisément ceux qui
survivent à une perte de points. **La condensation devrait donc améliorer
l'invariance en portée**, et c'est mesurable sans apprentissage (prédiction P7
de [`MESURE.md`](MESURE.md)).

### 4.4 Le piège : `min_cluster_size` est exactement la constante que l'on voulait supprimer

Il faut nommer la tension au lieu de la contourner. Toute la thèse de ce
dossier est de retirer les constantes posées à la main ; or la condensation de
HDBSCAN en introduit une, `min_cluster_size`, et un seuil en *nombre de points*
ne transfère ni d'un capteur à l'autre, ni du champ proche au champ lointain.

**La règle qui résout la tension : un rapport transfère, une longueur non.**
Le seuil doit être **relatif** — une scission n'est validée que si chaque
branche conserve au moins une fraction $\alpha$ de la masse $m_\tau$ du parent.
$\alpha$ est sans dimension, donc canonique et transférable ; un compte absolu
ne l'est pas. C'est la seule forme de condensation admissible ici.

Deux garde-fous s'y ajoutent :

- **coupler $\alpha$ entre les ordres.** Condenser chaque $K$ avec le même
  $\alpha$, et **revérifier la naturalité des cartes verticales après
  condensation** : si une fusion est retirée à l'ordre $K$ mais conservée à
  $K-1$, le carré peut cesser de commuter. Ce n'est pas automatique, c'est
  vérifiable, et il faut le vérifier ;
- **ne jamais binariser les multifusions.** La condensation de HDBSCAN parcourt
  des scissions binaires ; la tour publie des événements à trois parents ou
  plus au même niveau exact. Binariser inventerait un ordre qui n'existe pas et
  détruirait la canonicité. La règle doit être généralisée aux événements
  $k$-aires : à une multifusion, plusieurs branches peuvent tomber à la fois.

### 4.5 Quatre règles de construction de l'échelle

Le chemin fixe la direction ; il reste à fixer **comment on contracte**.

| règle | définition | attendu |
| --- | --- | --- |
| **E-global** | couper à des rayons globaux $r_1 < \cdots < r_L$ | témoin, doit échouer sur la variation de portée |
| **E-rang** | contracter les fusions dans l'ordre de $r$ jusqu'à une cible de compte | équilibré, mais encore globalement ordonné |
| **E-persistance** | contracter d'abord les fusions de plus faible persistance | **défaut recommandé** : localement adaptatif, dense et clairsemé contractés au même niveau |
| **E-relative** | contracter dans l'échelle normalisée $r / r_K(x)$ | invariance de portée par construction ; rival principal |

Les quatre règles s'appliquent **sur l'arbre condensé**, jamais sur la forêt
brute. `E-persistance` est recommandé parce qu'il est exactement la réponse que
HDBSCAN apporte au problème de densité variable — un $\varepsilon$ global ne
marche pas, l'arbre condensé si — et parce qu'il donne des niveaux dont le
nombre d'unités est contrôlé, donc une architecture de forme fixe et des lots
faciles à former. Dit autrement : `E-persistance` **est** une condensation à
seuil mobile ; la rendre explicite ne change pas le calcul, mais nomme le
critère et rend gratuitement la stabilité et les $\hat\lambda_x$.

Le rayon effectif de chaque nœud varie alors d'un bout à l'autre d'un même
niveau. **C'est le but** : c'est précisément ce qu'un voxel ne peut pas faire,
et c'est la mesure de l'adaptativité. Il faut le publier (histogramme de $r$
par niveau et par tranche de portée), car c'est l'observable qui montre que
l'architecture fait ce qu'elle prétend.

## 5. Les six composants

### FP — Pooling de filtration

$h_\ell = \phi\!\left(P_\ell^{\top} h_{\ell-1} W_\ell\right)$, où $P_\ell$ est
l'affectation du niveau $\ell-1$ vers le niveau $\ell$, normalisée en lignes
par les poids $w_{x\tau} = S_\tau / T_x$ du § 9.1. Le dépliage est $P_\ell$
appliqué en sens inverse, avec connexion de saut, comme dans tout U-Net.

Trois propriétés qu'un *grid pooling* n'a pas : l'affectation est **douce**
(un point appartenant à plusieurs nœuds répartit une masse totale de $1$),
**canonique** (aucune grille, aucune graine), et **conservative** (la masse est
préservée sur toute antichaîne).

Le recouvrement se paie en nombre de non-zéros de $P_\ell$. C'est une
statistique à mesurer avant tout entraînement — elle décide du coût réel.

### MGA — Attention sur le graphe de fusion, à biais ultramétrique

À chaque niveau, l'attention est restreinte au **graphe de fusion** : les nœuds
voisins sont ceux qui fusionneront, et la distance naturelle entre deux nœuds
$u$ et $v$ est le niveau $r_{uv}$ auquel ils se rejoignent.

Ce $r_{uv}$ n'est pas une distance quelconque : c'est une **ultramétrique**.
Le chapitre 3 du manuscrit établit l'équivalence entre dendrogrammes et
ultramétriques ; la hiérarchie *est* une ultramétrique sur ses feuilles. On
pose donc le biais d'attention

$b_{uv} = \varphi\!\left(\log r_{uv} - \log r_u\right)$,

normalisé par l'échelle propre du nœud. C'est un **encodage de position
relative par la hiérarchie et non par la métrique**, et il est invariant de
portée par construction : $r_{uv}$ grandit tout seul là où le nuage
s'appauvrit, exactement dans la proportion où le voisinage s'élargit.

L'encodage relatif $xyz$ ordinaire reste disponible comme **canal séparé, à
ablater** : on veut savoir lequel des deux porte l'information.

### OM — Mixage d'ordres

À un niveau donné, la même région possède une représentation pour chaque $K$,
et les cartes verticales fournissent la correspondance. On fusionne par
attention croisée ou par porte apprise.

Ce n'est pas un enrichissement décoratif. Le chapitre 7 du manuscrit fait de
$K$ le paramètre de **résistance à la percolation du bruit** : $K = 1$ est le
Single-Linkage, sensible et sujet au chaînage ; $K$ grand résiste aux ponts de
bruit mais retarde la naissance des structures minces. Le Théorème 3 chiffre la
fraction récupérable avant fusion parasite. **$K$ est donc littéralement un
bouton sensibilité/robustesse, et OM le rend apprenable par région.**

Un poteau veut $K$ petit ; séparer une voiture du sol qui la touche veut $K$
grand. Aucune architecture à un seul graphe de voisinage ne peut offrir ce
choix.

Trois réalisations, par coût croissant : **calendrier de $K$ selon la
profondeur** (défaut, gratuit : $K$ petit aux niveaux fins, grand aux niveaux
grossiers), **attention croisée entre ordres au goulot**, **branches parallèles
par $K$** (borne supérieure coûteuse, à mesurer une fois).

### PUR — Lecture par partition de l'unité

Le décodeur ne réinvente aucune interpolation : $p(x) = \sum_{\tau \ni x} w_{x\tau} \, p_\tau$
en entraînement, argmax de la Proposition 7 en inférence, qui garantit une
partition stricte des points. Les $w_{x\tau}$ somment à $1$, donc c'est une
relaxation différentiable propre et la masse est conservée.

### FM — Modélisation de filtration

Voir § 7.

### SEL — Tête de sélection apprise, ou « hacker HDBSCAN »

C'est la conséquence la plus productive de la condensation, et elle vient
encore du manuscrit. Le § 5.2 observe que l'extraction d'un partitionnement à
plat depuis l'arbre condensé est un **programme dynamique ascendant** à
fonction de coût **remplaçable** :

$\text{si } \mathrm{loss}(C_{\text{père}}) < \sum_i \mathrm{loss}(C_{\text{fils},i}) \implies \text{conserver le père, sinon continuer d'explorer.}$

Avec $\mathrm{loss}(C) = -\widehat{E}(C)$ on retrouve exactement l'excès de
masse de HDBSCAN. Et le manuscrit en tire la remarque décisive : **« L'excès de
masse est un critère purement statistique, aveugle à la géométrie »** ; en
substituant une évaluation propre au problème — alignement d'une structure,
volume attendu, conformité à un modèle 3D — « l'algorithme se transforme en un
extracteur guidé géométriquement ». C'est ce que font les deux applications du
§ 5.3 et du § 5.4, avec des coûts écrits à la main.

**Le mouvement du modèle de fondation est donc évident : apprendre ce coût.**
On pose $\mathrm{loss}(C) = -g_\theta(h_C)$, où $h_C$ est l'état du nœud produit
par HGP-UNet, et l'on garde le programme dynamique tel quel.

Quatre propriétés font de cette tête un bon objet, et non un gadget :

1. **le programme dynamique est exact et coûte une passe ascendante** sur
   l'arbre condensé, soit quelques milliers de nœuds ;
2. **il rend une antichaîne par construction.** Les nœuds choisis ne se
   recouvrent donc jamais dans l'arbre : ni suppression non maximale, ni
   appariement hongrois, ni seuil de recouvrement à régler. La Proposition 7
   convertit ensuite en partition stricte des points. Une tête d'instance sans
   aucune machinerie de propositions, c'est rare ;
3. **il se supervise simplement** : cible de $g_\theta$ = l'IoU du nœud avec la
   meilleure instance annotée, programme dynamique exact à l'inférence. Une
   relaxation continue n'est utile qu'ensuite, si la discrétisation coûte ;
4. **il généralise les deux applications du manuscrit** au lieu de les
   concurrencer : le coût guidé par un modèle 3D de la détection d'anomalies
   devient un cas particulier à coût figé.

Deux limites à énoncer tout de suite. Le plafond de cette tête est le **plafond
d'oracle** de la porte 0.1 : si l'instance annotée n'est pas un nœud de l'arbre
condensé, aucun coût appris ne la trouvera. Et le témoin qui compte n'est pas
un détecteur à boîtes, c'est **l'excès de masse sur le même arbre** : si le
coût appris ne bat pas $-\widehat{E}(C)$, il n'apporte rien. À quoi s'ajoute
ALPINE, qui atteint $\mathrm{PQ} = 64{,}2$ par regroupement géométrique sans
aucune étiquette d'instance.


## 6. Réalisation : une modification de PTv3, pas un nouveau réseau

**C'est la décision d'ingénierie la plus importante du projet.** HGP-UNet doit
être écrit *dans* la base de code PTv3, en remplaçant :

- `GridPool` par `FiltrationPool` ;
- le regroupement en *patches* sur la sérialisation par les *patches* du graphe
  de fusion (en conservant la sérialisation **à l'intérieur** d'un nœud, ce qui
  garde l'attention par fenêtre efficace) ;
- l'encodage relatif par le biais ultramétrique ;
- le décodeur d'interpolation par PUR ;

**et rien d'autre.** Même nombre de couches, mêmes largeurs, même optimiseur,
même recette.

Deux raisons, et elles pèsent plus que l'élégance :

1. **la substitution devient exacte.** Chaque composant se mesure contre son
   équivalent standard, toutes choses égales par ailleurs. Un réseau écrit de
   zéro rendrait tout écart inintelligible ;
2. **le résultat devient lisible par la communauté.** « Nous remplaçons le
   *grid pooling* de PTv3 par une échelle de densité canonique et nous gagnons
   $x$ points » est une phrase vérifiable. « Nous proposons une nouvelle
   architecture » ne l'est pas.

## 7. Pré-entraînement

### 7.1 Pourquoi la tour répond au raccourci géométrique

Sonata a établi le diagnostic central de la SSL 3D : les représentations
s'effondrent sur des indices spatiaux de bas niveau, **parce que la géométrie
est l'entrée**. Prédire une coordonnée masquée se résout par interpolation
locale. Sonata *atténue* : bruit gaussien sur les coordonnées masquées,
ordonnanceur de masque, suppression du décodeur hiérarchique.

Les cibles de la tour n'ont pas ce défaut. Le rayon auquel deux composantes
fusionnent est une grandeur de **percolation** : il dépend du goulot de densité
entre elles, donc d'une intégration sur tout l'espace intermédiaire. Il n'est
pas lisible sur les coordonnées locales. **Là où Sonata masque le raccourci, la
tour fournit une tâche où il n'existe pas.** C'est, à ma connaissance, la
première famille de prétextes 3D dont on peut argumenter cela par construction
et non par expérience.

### 7.2 Les cinq tâches

| tâche | cible | pourquoi elle n'est pas locale |
| --- | --- | --- |
| **FM-1 fusion** | $\log r_{uv}$ pour deux nœuds adjacents | goulot de densité entre eux |
| **FM-2 persistance** | mort $-$ naissance d'un nœud | dépend de tout le voisinage jusqu'à la fusion |
| **FM-3 profil en $K$** | l'ordre auquel un nœud cesse d'exister | mesure la densité locale relative au reste |
| **FM-4 rétablissement** | masquer une région, prédire la **structure de fusion** qu'elle aurait | reconstruction topologique, pas géométrique |
| **FM-5 accord inter-vues** | même structure de fusion sous décimation en portée et occultation | c'est l'hypothèse d'invariance, posée en fonction de perte |

Toutes ces cibles sont **exactes et gratuites** : elles sortent du moteur, sans
annotation.

### 7.3 Masquage par nœuds entiers

En traitement du langage, masquer un mot entier bat le masquage de sous-mots.
La tour donne l'unité correspondante en 3D : **on masque un nœud entier, pas
des points au hasard.** Un nœud est une pièce géométriquement cohérente, donc
le masque n'est plus rattrapable par interpolation. C'est le levier le plus
simple à essayer, et il se greffe sur n'importe quelle recette d'auto-
distillation existante sans changer le reste.

### 7.4 Auto-distillation

En parallèle, une auto-distillation enseignant/élève de la famille Sonata, avec
les augmentations que le domaine impose : décimation en portée selon le modèle
de balayage, retrait d'anneaux, occultation par secteur. L'invariant demandé à
l'élève est que **la structure**, et non les coordonnées, soit préservée.

### 7.5 Un cadeau pratique : les augmentations qui ne coûtent rien

Les rotations, translations et changements d'échelle uniformes **commutent
exactement** avec la tour : le nuage tourne, la tour tourne avec lui, sans
recalcul. Seules les augmentations de densité et d'occultation la changent —
et ce sont précisément celles que l'on veut pour FM-5. Il suffit donc de
**pré-calculer une banque de quelques vues décimées par trame**. Le
pré-entraînement n'a jamais besoin de recalculer une tour en ligne.

## 8. Variables de nœud

Le détail est dans [`JETON.md`](JETON.md). Le principe tient en une règle :
**une seule famille est normalisée**, la forme ; les grandeurs physiques, les
canaux de filtration et les canaux d'acquisition gardent leurs unités, dans des
canaux séparés et ablatables.

Point de conception important pour le transfert inter-capteurs : on veut une
**équivariance d'échelle, pas une invariance**. Une voiture mesure quatre
mètres, et c'est une information réelle. On donne donc $\log r$ comme canal
explicite, tandis que la *structure* (pooling, voisinage, biais) reste sans
échelle. Le modèle peut ainsi utiliser l'échelle absolue là où elle aide et
l'ignorer là où elle nuit — et l'ablation du canal $\log r$ dit lequel des deux
régimes domine.

## 9. Ce que l'architecture n'est pas

| conception écartée | raison |
| --- | --- |
| Sélectionner quelques milliers de jetons dans la tour et les donner à un Transformer plat | perd la hiérarchie, qui est la contribution ; et c'était un mauvais cadrage du coût (§ 2) |
| Vectoriser la persistance (code-barres, paysages, images de persistance) en variables d'entrée d'un réseau standard | c'est la voie TDA classique : elle jette la structure pour n'en garder qu'un résumé, et elle est largement explorée |
| Consommer une seule tranche $\lambda$ ou un seul $K$ | instable, par le résultat de Rolle et Scoccola ; l'architecture hériterait de cette instabilité |
| Apprendre la partition (superpoints appris, $k$-moyennes différentiables) | on reperd la canonicité et le déterminisme, et l'ablation redevient inintelligible |
| Écrire un réseau nouveau de zéro | rend la substitution impossible à interpréter et le résultat invérifiable |
| Forcer une partition stricte des points à l'entrée | pour $K \geq 2$, le recouvrement **est** l'information d'ordre supérieur (§ 9.1) |
| Remplacer un nœud par l'enveloppe convexe de sa géométrie, ou par sa seule fonction support | $h_P = h_{\mathrm{conv}(P)}$ : aveugle à la non-convexité et aux trous, qui sont l'essentiel d'une surface LiDAR |

## 10. Ce qui est revendicable

Aucune brique n'est nouvelle isolément : U-Net 3D, attention éparse
hiérarchique, partitions multi-échelles, auto-distillation, persistance
multiparamètre. Ce qui peut l'être :

1. **remplacer l'échelle métrique posée à la main par une échelle canonique
   dérivée des données**, et montrer par substitution ce que cela vaut ;
2. **utiliser un objet multiparamètre prouvé stable** là où l'état de l'art
   utilise une tranche instable, avec l'axe $K$ comme bouton
   sensibilité/robustesse apprenable ;
3. **un encodage de position relative ultramétrique**, invariant de portée par
   construction ;
4. **une famille de prétextes sans raccourci géométrique**, aux cibles exactes
   et gratuites ;
5. **un décodeur démontré** (Proposition 7) au lieu d'une interpolation choisie
   à la main.

Chacun de ces cinq points correspond à une ligne de la table de substitution de
[`ETAT_DE_LART.md`](ETAT_DE_LART.md) § 4, donc à une expérience contrôlée. Si
une ligne ne se paie pas, on la retire et on le dit. C'est l'objet de
[`MESURE.md`](MESURE.md).
