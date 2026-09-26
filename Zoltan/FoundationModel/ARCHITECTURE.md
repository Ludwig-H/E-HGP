# HGP-FM — architecture proposée

26 septembre 2026. Proposition d'architecture pour un modèle de fondation 3D
du LiDAR extérieur automobile, construit sur la tour FULL Morse HGP 3D
(`morsehgp3D_v9`). Conception seulement : aucune expérience apprise n'est
rapportée, aucun chiffre d'apprentissage n'est revendiqué.

Prérequis de lecture : [`OBJET.md`](OBJET.md), dont les chiffres mesurés
contraignent tout ce qui suit.

## 1. Le principe directeur

> **Le tokenizer est exact et n'est pas appris. Le backbone est appris et reste
> ordinaire.**

C'est le seul découpage qui rende la contribution mesurable. Si l'on apprend la
tokenisation en même temps que le reste, on ne saura jamais dire si le gain
vient de la géométrie HGP ou du budget de calcul. En figeant le tokenizer sur
un objet mathématique défini par le manuscrit et calculé exactement, chaque
ablation devient une question à une seule variable : *même backbone, même
budget, autre tokenizer*.

Trois propriétés rendent ce choix tenable, et elles sont des faits mesurés, pas
des espoirs :

1. **Déterminisme.** La tour est une fonction pure de la trame quantifiée ; son
   condensé `tower_digest` est reproductible bit à bit (R22 : 24 comparaisons
   appariées égales). Donc elle se calcule une fois et se met en cache.
2. **Exactitude.** Tout prédicat est entier ; les dégénérescences donnent un
   refus explicite. Le jeton n'hérite d'aucun bruit numérique.
3. **Richesse propre.** La tour porte des grandeurs qu'aucun modèle point à
   point ne voit : niveau de naissance, niveau de mort, degré de multifusion,
   arité du support, image verticale d'un ordre à l'autre. Ce sont des
   étiquettes gratuites et exactes — on y revient en § 7.

## 2. La pile

```text
  L0  trame LiDAR brute (x, y, z, rémission, anneau, horodatage)
        | quantification 1 mm u18, PointId, table d'attributs hors moteur
        v
  L1  TOUR FULL exacte  (morsehgp3D_v9)          1,3 M a 16,3 M noeuds
        | condensation §9.1 (masses m_tau) + selection par exces de masse
        v
      JETONS POLYEDRIQUES                        cible 2 k a 8 k par trame
        | realisation geometrique P_v = union conv(S_b) + cinq canaux
        v
  L2  encodeur de jeton (petit, partage)         descripteur -> R^d
        v
  L3  backbone contextuel a trois canaux d'attention
        laterale (voisinage spatial) | verticale (parents/enfants)
                                     | d'ordre (cartes K <-> K-1)
        v
  L4  lecture au point : vote pondere §9.1, w_{x tau} = S_tau / T_x
        v
  L5  tetes : pre-entrainement (filtration + auto-distillation),
      segmentation semantique, instance, anomalie
```

## 3. L1 — HGP-Tok, le tokenizer exact

### 3.1 Le problème à résoudre en premier

`OBJET.md` § 4 le chiffre : une trame brute à $K \leq 10$ produit jusqu'à
16 274 683 nœuds. Pour 4 096 jetons il faut un facteur **3 973**. C'est *le*
problème d'architecture, et il a une réponse déjà écrite dans la thèse et déjà
implémentée, exactement, dans `morsehgp3d/`.

### 3.2 La réponse : condenser, puis sélectionner

**Étape 1 — condensation.** On retire les continuations et on pèse chaque
facette par la masse de § 9.1 :
$m_\tau = S_\tau \sum_{x \in \tau} 1/T_x$, avec
$S_\tau = \sum_{\sigma \supset \tau} \rho(\sigma)^{-3}$ et
$T_x = \sum_{\tau \ni x} S_\tau$. C'est exactement la masse que
`min_cluster_size` consomme dans l'arbre condensé, et c'est le seul poids qui
évite qu'un point incident à beaucoup de facettes soit surpondéré : chaque
point distribue une masse totale de 1.

**Étape 2 — sélection.** `select_excess_of_mass` sur l'arbre condensé
multi-ordres donne une antichaîne de nœuds saillants, exacte. Deux variantes à
comparer (porte G0.3) :

- **antichaîne pure** : un nœud par branche, comme HDBSCAN. Peu de jetons, mais
  le modèle perd le contexte multi-échelle que le poster revendique.
- **bande** : l'antichaîne d'excès de masse **plus** ses $L$ ancêtres et $L$
  descendants condensés. On garde la trajectoire d'échelle autour de chaque
  objet, à budget contrôlé.

Je recommande la **bande avec $L = 2$**, parce que la contribution annoncée
n'est pas « des régions » (SPT le fait déjà) mais « des régions **et leur
hiérarchie de fusion** ». Une antichaîne pure jetterait l'argument.

**Étape 3 — budget.** Un plafond dur par trame, réparti par ordre
proportionnellement à la masse, avec repli déterministe (les plus fortes
persistances d'abord, départage par clé canonique). Le tokenizer doit être
**total** : aucune trame ne doit échouer faute de budget, sinon les statistiques
d'entraînement se biaisent silencieusement.

### 3.3 Les trois granularités, et l'analogie du poster

Le poster dit : GPT découpe le texte en jetons, souvent des morceaux de mots.
L'analogie est exacte et utile si on nomme les trois niveaux :

| texte | HGP | objet v9 |
| --- | --- | --- |
| caractère | boule minimale de Gabriel, support de 2, 3 ou 4 points | `BallData` |
| sous-mot | nœud condensé de la tour | nœud de `FullCoverageCertificate` |
| mot | nœud sélectionné par excès de masse | sortie de `select_excess_of_mass` |

Le modèle travaille au niveau **sous-mot et mot** ; le niveau caractère sert à
construire le descripteur. C'est l'exacte structure d'un tokenizer BPE, avec
une différence de taille : ici le vocabulaire n'est pas appris sur un corpus,
il est **dérivé de la géométrie de chaque scène**. Il n'y a donc pas de
vocabulaire fixe, et c'est voulu : un objet jamais vu produit quand même ses
jetons.

### 3.4 Réalisation géométrique

Pour un nœud $v$, la géométrie est
$P_v = \bigcup_b \mathrm{conv}(S_b)$, où $b$ parcourt les boules du catalogue qui
contribuent à $v$ et $S_b$ est leur support ($2$, $3$ ou $4$ points : arête
diamétrale, triangle aigu, tétraèdre). C'est la définition de la diapositive 11
de la présentation, et elle est ouverte, non convexe et potentiellement
multicouche — on ne la remplace jamais par son enveloppe convexe.

Le descripteur est traité à part, dans [`JETON.md`](JETON.md).

## 4. L2 — encodeur de jeton

Un MLP partagé sur le descripteur figé suffit pour démarrer, et c'est
délibéré : au premier palier on veut savoir ce que porte le descripteur, pas ce
qu'un encodeur sait en tirer.

Variante à évaluer ensuite, si et seulement si G3 montre que le descripteur
figé plafonne : un petit encodeur d'ensemble (type Set Transformer) sur les
supports $S_b$ du nœud, avec leur arité et leur rayon comme attributs d'entrée.
Il lit la structure exacte au lieu d'une grille, donc il ne perd pas les
détails fins — c'est le remède documenté au défaut connu des grilles de
distances.

## 5. L3 — backbone contextuel

Un Transformer sur les jetons, avec **trois canaux d'attention** qui
correspondent aux trois relations que la tour fournit et qu'un modèle point à
point ne possède pas :

- **latérale** — voisinage spatial à échelle comparable. On sérialise les
  jetons par clé de Morton sur le centre (la v9 calcule déjà des clés de Morton
  sur les positions uniques) et on attend par fenêtres, à la manière des
  Transformers de points sérialisés. Coût linéaire, pas de graphe à construire.
- **verticale** — parent et enfants dans l'arbre condensé du même ordre. C'est
  le contexte multi-échelle du poster. La laminarité requise existe : § 9.1
  démontre que l'arbre est une partition des $(K-1)$-simplexes.
- **d'ordre** — les cartes verticales `lower_nodes` relient un nœud d'ordre $K$
  à son image d'ordre $K-1$. C'est **l'axe le plus spécifique du projet** :
  aucun concurrent n'a de second paramètre de filtration.

Le biais d'attention encode la relation et sa mesure : écart de niveau
logarithmique pour la verticale, écart d'ordre pour la troisième, distance
métrique normalisée pour la latérale.

### 5.1 Pourquoi l'axe des ordres compte vraiment

Un risque connu et sérieux : **HGP retarde la naissance des objets
filiformes**. Un poteau, un tronc, une barrière sont minces ; à $K \geq 2$ il
faut $K$ boules qui s'intersectent, donc la pièce naît à un rayon plus grand,
où elle peut avoir déjà fusionné avec son environnement. Or c'est justement sur
les petites classes filiformes que se joue le mIoU.

La tour donne la parade sans invention : **$K = 1$ est le Single-Linkage**, et
il est précoce sur les structures minces. En gardant $K = 1 \ldots K_{\max}$
dans le jeu de jetons et en laissant l'attention d'ordre choisir, on obtient un
choix d'ordre **par région**, appris. C'est un avantage architectural réel, pas
une figure de style : aucun modèle à un seul graphe de voisinage ne peut
l'offrir.

La porte G2 mesure ce risque par classe avant tout apprentissage.

## 6. L4 — retour aux points

On n'invente aucune interpolation. Le vote pondéré de § 9.1 donne, pour chaque
point $x$ et chaque classe $c$,
$V_x(c) = \sum_{\tau \ni x,\ \ell(\tau) = c} S_\tau / T_x$, et l'étiquette est
l'argmax, avec une règle de départage déterministe. La Proposition 7 garantit
que cela définit une partition stricte des points, $C_{-1}$ compris (non
classés).

Trois raisons d'en faire le décodeur officiel :

1. c'est **exact** et déjà implémenté dans `morsehgp3d/`
   (`SimplexPointWeighting::inverse_radius`, $p = 3$) ;
2. les poids $w_{x\tau} = S_\tau/T_x$ forment une **partition de l'unité** :
   c'est un adoucissement propre pour la rétropropagation vers les jetons ;
3. cela garde la propriété qui justifie tout l'édifice : les points peuvent
   appartenir à plusieurs polyèdres **jusqu'à la toute dernière étape**.

Pour l'entraînement, on utilise la version douce (les $w_{x\tau}$ comme poids
d'un mélange de logits de jetons) ; pour l'évaluation en partition, la version
dure de la Proposition 7.

## 7. L5 — objectifs de pré-entraînement

### 7.1 Modélisation de filtration (spécifique au projet)

La tour fabrique gratuitement des cibles exactes que nul autre tokenizer ne
possède. On masque une partie de la structure et on la fait prédire :

- **niveau de mort** — à quel rayon ce nœud fusionne-t-il ? (régression sur
  $\log r$) ;
- **partenaire de fusion** — parmi $m$ candidats, lequel fusionne le premier
  avec ce nœud ? (classement) ;
- **ordre d'apparition** — à quel $K$ ce nœud apparaît-il pour la première
  fois ? ;
- **arité du support** — la pièce est-elle portée par des arêtes diamétrales,
  des triangles aigus ou des tétraèdres ? ;
- **degré de multifusion** — combien de parents à l'événement ?

C'est l'analogue direct du *masked language modelling*, avec un avantage :
les cibles ne sont pas des jetons arbitraires mais des **grandeurs
géométriques exactes**. Un modèle qui les prédit bien a nécessairement appris
la trajectoire d'échelle locale — c'est-à-dire ce que le poster appelle
« multiscale context from their merging hierarchy ».

### 7.2 Auto-distillation et invariance de capteur (au niveau de l'état de l'art)

En parallèle, une auto-distillation enseignant/élève de la famille Sonata, avec
les augmentations qui comptent ici :

- **décimation en portée** : simuler un objet plus lointain en réduisant la
  densité selon le modèle de balayage, pas uniformément ;
- **retrait d'anneaux** : simuler un capteur à moins de nappes ;
- **occultation** : masquer un secteur angulaire.

L'invariant demandé à l'élève est que le **code de forme** ne bouge pas, tandis
que les canaux physiques et d'acquisition, eux, bougent. C'est l'hypothèse
centrale du poster traduite en fonction de perte — et la porte G1 la teste
**avant** tout apprentissage, sans quoi on entraînerait un modèle sur une
hypothèse fausse.

## 8. Ce que l'on écarte, et pourquoi

| conception écartée | raison |
| --- | --- |
| Donner tous les nœuds de la tour au Transformer | 1,3 M à 16,3 M nœuds par trame (mesuré, R22). Sans objet. |
| Apprendre la tokenisation (FPS, k-moyennes, superpoints appris) | On perd l'exactitude, le déterminisme et la bifiltration — c'est-à-dire tout ce qui distingue le projet. Et l'ablation devient inintelligible. |
| Forcer une partition des points dès l'entrée | Le recouvrement pour $K \geq 2$ **est** la contribution (manuscrit § 9.1). Le partitionner d'emblée jette l'information d'ordre supérieur. |
| Faire du backbone une attention hiérarchique sur arbre, seule | La laminarité vaut sur les facettes, pas sur les points ; et un arbre seul supprime le voisinage latéral, dont les scènes de rue ont besoin. On garde l'arbre comme **un** canal sur trois. |
| Représenter chaque nœud par une fonction radiale $\rho(u)$ | Une direction peut ne rencontrer aucune couche, ou plusieurs. C'est une bonne base de comparaison, pas une représentation universelle (présentation, § IV). |
| Représenter le nœud par $\mathrm{conv}(P_v)$ ou par sa fonction support $h_P$ | $h_P = h_{\mathrm{conv}(P)}$ : la fonction support est **aveugle** à la non-convexité et aux trous, qui sont l'essentiel d'une surface LiDAR partielle. |
| Un atlas de cartes appris par nœud | Coutures et ancres changent sous décimation ; apprendre l'atlas en même temps que le backbone rend l'effet du tokenizer impossible à isoler. |
| Ajuster un champ implicite (UDF) par polyèdre | Coûteux et redondant : la surface est déjà explicite. Pertinent comme décodeur de complétion, pas comme entrée. |

## 9. Dimensionnement

Avec une bande $L = 2$ et un budget de 4 096 jetons par trame :

| poste | ordre de grandeur | source |
| --- | --- | --- |
| tour par trame, $K \leq 5$, sans sol | 0,76–0,98 s | reçu R22 |
| tour par trame, $K \leq 5$, brut | 1,81–2,03 s | reçu R22 |
| tokenisation de SemanticKITTI (23 201 trames d'entraînement) | ≈ 12–13 h sur une G4, une fois | produit des deux précédents |
| RSS de la tour | 1,2–6,4 Go | reçu R22 |
| cache de jetons, 4 096 jetons × ~700 flottants | ≈ 11 Mo par trame en float32, ≈ 5,6 Mo en float16 | à confirmer, porte G0.5 |
| cache total SemanticKITTI | ≈ 130–260 Go | idem |

Le cache est le vrai poste dimensionnant, pas le calcul. Il impose une décision
explicite : **quantifier le descripteur en float16 et stocker les canaux exacts
en entiers**, ce qui est cohérent avec la doctrine du dépôt (le flottant est
une sortie, jamais un maillon de la chaîne d'exactitude).

## 10. Ce qui est nouveau, et ce qui ne l'est pas

À dire soi-même avant qu'un relecteur ne le dise. Aucune des briques suivantes
n'est revendicable isolément : descripteur radial ou sphérique, grille de
distances à sondes fixes, apprentissage sur polyèdres et maillages, Transformer
hiérarchique, auto-distillation LiDAR, pré-entraînement multi-capteurs.

Ce qui peut l'être, et seulement cela :

1. un **tokenizer exact, déterministe et non appris** défini par un théorème
   (Théorème 2 : les jetons sont les amas de forte densité $K$-NN, niveau par
   niveau) ;
2. une **bifiltration $(K, r)$** comme structure de contexte, avec un axe des
   ordres qu'aucun concurrent ne possède ;
3. des **objectifs de pré-entraînement dérivés de la filtration**, aux cibles
   géométriques exactes et gratuites ;
4. un **retour aux points démontré** (Proposition 7) plutôt qu'une
   interpolation choisie à la main.

Si les portes G1 et G2 du [protocole](PROTOCOLE.md) échouent, les quatre points
ci-dessus restent vrais mathématiquement et sans intérêt pratique. C'est
exactement ce qu'il faut savoir tôt.

## 11. Ordre de construction

Chaque étape est terminée par une porte du [protocole](PROTOCOLE.md), et
produit un reçu épinglé.

1. **Exportateur** `mhgp9_tower_export` : `ChainResult` → `CertifiedTowerInput`
   (forêt, verticales, simplexes projetables en plateaux). Portes G0.1 à G0.4.
2. **Raccord au réducteur** `morsehgp3d::build_exact_point_hierarchy`, puis
   `select_excess_of_mass`. Première mesure du budget de jetons. Porte G0.3.
3. **Descripteur** et cache sur disque, sans aucun apprentissage. Porte G0.5.
4. **G1** : invariance en portée. C'est la porte qui décide si le projet a un
   fondement.
5. **G2** : plafond d'oracle d'instances, par classe.
6. **G3** : sonde XGBoost sur les descripteurs. Ablation par famille de canaux.
7. **G4** : mIoU sans apprentissage profond (XGBoost → points par § 9.1).
8. **G5** à **G7** : supervision, pré-entraînement, transfert inter-capteurs.

Rien avant l'étape 4 ne demande un GPU d'entraînement. C'est délibéré : les
deux questions qui peuvent tuer le projet se répondent sur CPU, en quelques
jours, pour un coût négligeable.
