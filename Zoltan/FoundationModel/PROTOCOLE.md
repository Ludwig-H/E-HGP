# Protocole de test

Comment savoir, **en cours de route** et le plus tôt possible, si HGP-FM vaut
mieux qu'un modèle point à point — et à quel moment abandonner.

Le protocole est une **échelle de portes**. Chaque porte a une question, un
coût, un critère de promotion et surtout un **critère de réfutation** écrit
avant l'expérience. On ne monte pas d'un barreau sans reçu.

Principe de construction : **les deux questions qui peuvent tuer le projet
(G1 et G2) se répondent sans aucun apprentissage, sur CPU, en quelques jours.**
Tout ce qui demande un GPU d'entraînement vient après.

## 0. Règles générales

**Découpage.** SemanticKITTI : séquences 00–07, 09, 10 en entraînement ;
séquence **08 en validation** ; le jeu de test (serveur en ligne) n'est touché
qu'une fois, à la toute fin, et jamais pour régler quoi que ce soit.

**Comparaison.** Se comparer à l'état de l'art en régime strict, jamais au
« scratch ». Repère hérité du corpus précédent, **à revérifier avant
publication** : cible val $73{,}1$ (DOS), depuis une base reproductible à
$68{,}0$–$70{,}3$. Aucune de ces valeurs n'a été reproduite dans ce dépôt.

**Ablation à budget égal.** Toute comparaison HGP-FM contre une base point à
point se fait à **même nombre de paramètres, mêmes époques, mêmes
augmentations, même matériel**. Un gain obtenu en dépensant plus n'est pas un
gain.

**Reçus.** Toute mesure produit un reçu immuable, ancré au commit, contenant :
le commit du dépôt, le `tower_digest` et le `catalogue_digest` de chaque trame
consommée, le sha256 du jeu de données, la configuration complète, les graines,
le matériel, et les sorties brutes. Une mesure sans reçu est requalifiée par
l'audit, c'est-à-dire qu'elle n'existe pas.

**Graines.** Trois graines minimum pour tout résultat appris ; publier la
moyenne **et** l'écart, jamais la meilleure.

**Licence.** Les poids Sonata / Concerto / Utonia sont en CC-BY-NC 4.0 : on les
compare, on ne les importe jamais dans la ligne produit. Aucun octet
SemanticKITTI n'est versionné.

**Données.** Les trois trames de référence quantifiées 1 mm sans sol
(08/000000, 000100, 000200 ; 39 885 / 35 551 / 45 845 sites) existent déjà dans
[`morsehgp3D_v8/receipts/lidar_ground_20260921/`](../../morsehgp3D_v8/receipts/lidar_ground_20260921/)
avec leurs manifestes sha256. Le reste du corpus se régénère depuis les scans
bruts dans un `data/` ignoré par Git ; seuls les manifestes sont versionnés.

## 1. G0 — portes de faisabilité du tokenizer (aucun apprentissage)

Coût : CPU et une session G4 courte. Durée : jours.

### G0.1 — l'exportateur existe et est fidèle

Exporter `ChainResult` vers `CertifiedTowerInput`. Juger l'export contre la
tour en mémoire : mêmes nœuds, mêmes parents, mêmes niveaux, mêmes verticales,
mêmes populations. Porte à code de sortie exact, mutants compilés.

*Réfutation* : une seule divergence sur une trame réelle bloque tout l'aval.

### G0.2 — couverture du corpus

Passer la tour sur **tout** SemanticKITTI, trames brutes avec sol. Compter les
statuts : `complete_relative`, `unsupported_degeneracy`, `resource_exhausted`,
refus de domaine par coquille supérieure à 12 sites.

*Réfutation* : au-delà de **1 %** de trames refusées, le corpus d'entraînement
est biaisé de manière non contrôlée ; il faut corriger le moteur avant
d'apprendre quoi que ce soit. Ce taux n'est aujourd'hui pas mesuré.

### G0.3 — budget de jetons

Après condensation § 9.1 et `select_excess_of_mass`, compter les jetons par
trame, par ordre, en antichaîne pure et en bande $L = 1, 2, 3$.

*Promotion* : la bande $L = 2$ tient sous **8 192 jetons** sur le 99e centile
des trames brutes.
*Réfutation* : s'il faut plus de 32 768 jetons, la sélection par excès de masse
ne suffit pas et il faut un autre critère — ce serait un vrai résultat négatif,
à publier dans les fausses pistes.

Repère : la tour brute à $K \leq 10$ compte jusqu'à 16 274 683 nœuds
(reçu R22). Le facteur demandé est donc d'environ 2 000 à 4 000.

### G0.4 — incidences facette–coface

Mesurer le nombre d'incidences $(\tau, \sigma)$ publiées par l'exportateur, en
plateaux et en développé.

*Réfutation* : le budget du réducteur produit est
`maximum_point_simplex_incidences = 64 000 000`. Au-delà, il faut soit les
plateaux, soit un autre poids que $S_\tau$. À mesurer, jamais à supposer.

### G0.5 — coût du cache

Taille réelle d'une trame tokenisée en float16, et extrapolation au corpus.

*Promotion* : sous **300 Go** pour SemanticKITTI complet.

## 2. G1 — invariance en portée (aucun apprentissage) — **la porte décisive**

C'est l'hypothèse centrale du poster, mot pour mot : *sampling depends on
sensor and range, geometry may vary less*. Elle n'a jamais été mesurée. Tant
qu'elle ne l'est pas, tout le reste est une construction sur un axiome.

**Montage.** Deux sources d'appariement, à faire toutes les deux :

1. **réel** — KITTI tracking ou instances SemanticKITTI suivies : le même objet
   physique observé à des portées différentes sur des trames successives ;
2. **contrôlé** — décimation en portée simulée : on prend un objet proche et on
   lui applique le modèle de balayage d'un objet lointain (moins d'anneaux,
   moins de retours par anneau, même occultation).

**Mesure.** Pour chaque objet, on calcule le code de forme (famille 1) du nœud
qui le porte le mieux. Puis :

- **récupération** : l'objet à portée $d_2$ retrouve-t-il sa propre version à
  portée $d_1$ parmi tous les objets de la scène ? On publie le rang moyen et
  la mAP par tranche de portée ;
- **rapport de stabilité** : $\varrho = \delta_{\text{intra}} / \delta_{\text{inter}}$, où
  $\delta_{\text{intra}}$ est la distance moyenne du code d'un objet à lui-même vu à une
  autre portée, et $\delta_{\text{inter}}$ la distance moyenne à un autre objet de la même
  classe. Plus $\varrho$ est petit, meilleure est l'invariance.

**Témoins obligatoires.** Le même protocole avec : (a) une occupation
voxelisée brute, (b) une projection aléatoire fixe des points normalisés, (c)
un descripteur de covariance des points. Sans ces témoins, le résultat ne dit
rien.

*Promotion* : le code HGP a un $\varrho$ strictement plus petit que les trois
témoins, sur au moins **quatre des cinq tranches de portée**, et l'écart croît
avec la portée.
*Réfutation* : si le code HGP n'est **pas** plus stable que les témoins, alors
l'hypothèse centrale du poster est fausse dans ce régime et il faut le dire.
Le projet ne meurt pas — la hiérarchie et l'exactitude gardent de la valeur
pour l'instance et l'anomalie — mais l'argument « modèle de fondation » doit
être réécrit.

## 3. G2 — plafond d'oracle d'instances (aucun apprentissage)

**C'est une porte de réfutation, jamais de promotion.** Un oracle élevé ne
prouve rien sur le modèle appris ; un oracle bas prouve que le modèle appris ne
pourra pas.

**Mesure.** Pour chaque instance annotée $G$ de la séquence 08 :
$\max_v \mathrm{IoU}(S_v, G)$, sur **tous** les nœuds $v$ de la tour, tous ordres
et tous niveaux, où $S_v$ est la population du nœud. On publie par classe.

**Variantes à séparer.** Oracle sur la tour complète (plafond absolu) et oracle
sur les seuls jetons retenus par G0.3 (plafond réellement atteignable par le
modèle). L'écart entre les deux est le coût de la sélection : il doit être
publié, c'est un chiffre honnête et rarement montré.

**Témoins.** Le même oracle sur HDBSCAN, DBSCAN et une partition en
superpoints. C'est la seule façon de dire ce que HGP apporte.

*Réfutation* : pour une classe, un plafond inférieur à la performance actuelle
de l'état de l'art sur cette classe signifie que la tour **perd** cette classe.
Surveiller en priorité les classes **filiformes** (poteau, panneau, tronc,
barrière, cycliste) : le risque documenté est que HGP retarde leur naissance.
Si le plafond y est mauvais, la parade est l'ordre $K = 1$ (Single-Linkage,
précoce sur les structures minces) — et il faut alors montrer que le plafond
remonte quand on ajoute $K = 1$ au jeu de jetons.

## 4. G3 — sonde XGBoost sur les descripteurs

### 4.1 Pourquoi XGBoost, ici précisément

Oui, il faut un outil de ce genre, et sa place est exactement ici. La question
de G3 n'est pas « quel est le meilleur modèle ? » mais **« l'information est-elle
dans le descripteur ? »**. Pour cette question, un ensemble d'arbres boostés est
supérieur à un réseau :

- il tourne en **minutes sur CPU** ; on peut donc se permettre des dizaines
  d'ablations, ce qui est impensable avec un Transformer ;
- il est **indifférent à l'échelle** des variables : on peut mélanger des
  rayons en mètres, des comptes entiers et des proportions sans se battre avec
  la normalisation ;
- il donne l'**importance par gain** et les valeurs de Shapley, c'est-à-dire
  une réponse directe à « quelle famille de canaux se paie ? » ;
- il ne demande **aucun réglage** pour être honnête, donc il ne flatte pas le
  descripteur.

Autrement dit : XGBoost est le bon instrument pour tester le *descripteur*, pas
pour construire le *modèle*.

### 4.2 Montage

Étiquette d'un nœud : classe SemanticKITTI majoritaire de sa population, avec
un seuil de pureté (par exemple $70\%$) ; en dessous, le nœud est marqué
« mixte » et forme une classe à part entière — pas un rejet silencieux, car la
proportion de nœuds mixtes est elle-même un résultat sur la qualité de la
hiérarchie.

Variables : les cinq familles de [`JETON.md`](JETON.md).
Métriques : F1 macro, matrice de confusion, F1 par classe.

### 4.3 Ablations imposées

1. chaque famille seule ;
2. toutes sauf une ;
3. base triviale : hauteur du centre, taille de boîte, portée ;
4. base « points » : mêmes points, descripteur de covariance, sans la
   hiérarchie ;
5. avec et sans les canaux de **filtration** — c'est l'ablation qui mesure
   directement la valeur de la tour par rapport à une simple segmentation en
   régions.

*Promotion* : le descripteur complet bat la base triviale d'au moins **15
points de F1 macro**, et l'ablation 5 montre que la filtration vaut au moins
**3 points**.
*Réfutation* : si le retrait de toute la famille filtration ne coûte rien,
alors la tour ne sert qu'à découper l'espace et n'importe quelle sur-
segmentation ferait aussi bien. Il faudrait le dire, et le projet se réduirait
à un tokenizer géométrique parmi d'autres.

## 5. G4 — mIoU sans apprentissage profond

Pousser les prédictions de nœuds de G3 vers les points par le vote pondéré de
§ 9.1 ($w_{x\tau} = S_\tau/T_x$, Proposition 7), et mesurer le mIoU sur la
séquence 08.

C'est un chiffre honnête et rarement produit : **ce que la géométrie seule
donne**, sans réseau. Il devient la base à battre pour tout le reste, et il se
compare directement aux bases sans apprentissage de la littérature (dont
l'article *Is clustering enough for LiDAR instance segmentation ?* déjà présent
à la racine du dépôt).

*Promotion* : établir le chiffre et le publier, quel qu'il soit.
*Réfutation* : aucune. Cette porte ne peut pas échouer ; elle peut seulement
être décevante, et c'est une information.

## 6. G5 — supervision à budget apparié

Petit Transformer sur jetons (L2 + L3 de l'[architecture](ARCHITECTURE.md)),
sans pré-entraînement, entraîné sur 00–07, 09, 10, évalué sur 08.

**Témoin obligatoire** : un modèle point/voxel de la même famille de taille
(MinkowskiNet ou un Transformer de points sérialisés en version petite),
**même nombre de paramètres, mêmes époques, même matériel**.

*Promotion* : HGP-FM bat G4 d'une marge nette, et n'est pas en dessous du
témoin apparié.
*Réfutation* : si HGP-FM est en dessous du témoin apparié de plus de 2 points
de mIoU, le tokenizer coûte plus qu'il ne rapporte en supervision pleine. Il
faut alors se replier sur le régime à peu d'étiquettes (G6), où l'a priori
géométrique devrait avoir le plus de valeur, ou sur l'instance et l'anomalie.

### G5bis — ablations du backbone

Retirer un canal d'attention à la fois : sans latérale, sans verticale, sans
axe des ordres. L'**axe des ordres** est la contribution la plus spécifique du
projet ; s'il ne vaut rien, il faut le dire et simplifier le modèle.

## 7. G6 — pré-entraînement

Objectifs de [§ 7 de l'architecture](ARCHITECTURE.md) : modélisation de
filtration et auto-distillation avec augmentations de portée.

Évaluation : sonde linéaire, puis affinage complet, et surtout **régime à peu
d'étiquettes** : $1\%$, $10\%$, $100\%$ des étiquettes.

*Promotion* : le pré-entraînement améliore G5, et **améliore davantage à $1\%$
d'étiquettes qu'à $100\%$**. C'est la signature d'un vrai modèle de fondation ;
un gain uniforme est plutôt le symptôme d'une régularisation.
*Réfutation* : un gain nul en sonde linéaire signifie que les objectifs de
filtration n'apprennent rien de transférable. Essayer l'auto-distillation
seule avant de conclure.

### G6bis — l'ablation qui décide de l'originalité

Le même pré-entraînement **sans** les objectifs de filtration (auto-
distillation seule). Si l'écart est nul, la contribution propre du projet au
pré-entraînement est nulle, et il ne reste que le tokenizer.

## 8. G7 — transfert inter-capteurs

Pré-entraîner sur un capteur, évaluer sur un autre : KITTI 64 nappes vers
nuScenes 32 nappes, et retour. C'est le test où l'hypothèse d'invariance doit
payer le plus, et c'est le terrain de Sonata, Utonia et Vernata.

*Promotion* : la chute de performance en transfert est **plus faible** pour
HGP-FM que pour le témoin point/voxel apparié.
*Réfutation* : une chute égale ou pire est le verdict le plus sévère possible,
puisque c'est exactement ce que le projet promet.

## 9. Applications de la thèse, à ne pas oublier

La hiérarchie est aussi un **espace de résolution** où l'on injecte des a priori
sans apprentissage (manuscrit, § 5.3 et § 5.4). Deux portes indépendantes de la
chaîne d'apprentissage, et déjà démontrées à l'échelle dans le poster :

- **G-A, anomalies** : objets présents dans un balayage et absents d'un modèle
  3D de référence, par sélection d'arbre guidée par la géométrie. Le poster
  montre 2 millions de points traités sans aucun entraînement ;
- **G-I, instance 4D** : segmentation d'instance sur SemanticKITTI par
  exploration guidée de l'arbre.

Ces deux portes ont un intérêt propre : elles produisent des résultats
publiables même si G5 à G7 échouent, et elles valident la tour sur des tâches
où la sur-segmentation n'est pas pénalisante.

## 10. Vue d'ensemble

| porte | question | apprentissage | coût | ce qui la tue |
| --- | --- | --- | --- | --- |
| G0 | le tokenizer tient-il ? | non | jours CPU + une session G4 | refus > 1 %, budget > 32 k jetons |
| **G1** | la forme est-elle plus stable que l'échantillonnage ? | non | jours CPU | pas mieux que les témoins |
| **G2** | la tour contient-elle les instances ? | non | jours CPU | plafond bas sur les classes filiformes |
| G3 | le descripteur porte-t-il l'information ? | XGBoost | heures CPU | la filtration ne vaut rien |
| G4 | que donne la géométrie seule ? | XGBoost | heures CPU | rien (porte informative) |
| G5 | mieux qu'un modèle point apparié ? | oui | jours GPU | en dessous du témoin apparié |
| G6 | le pré-entraînement transfère-t-il ? | oui | semaines GPU | gain nul en sonde linéaire |
| G7 | transfert inter-capteurs ? | oui | semaines GPU | chute égale ou pire |

Les deux portes en gras se franchissent sans une heure de GPU d'entraînement.
Commencer par elles n'est pas de la prudence : c'est le seul ordre qui permette
d'échouer vite et à bas prix.
