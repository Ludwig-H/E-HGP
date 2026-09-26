# Mesurer l'apport

26 septembre 2026. Comment établir, et non affirmer, ce que la hiérarchie
$K$-NN apporte par rapport aux méthodes existantes.

## 1. La doctrine : substituer, ne pas comparer des systèmes

Comparer « HGP-FM » à « Sonata » ou à « Utonia » ne prouve rien. Deux systèmes
complets diffèrent par le squelette, les données, la recette, le budget de
calcul, les augmentations et mille détails. Un écart entre eux n'est pas
attribuable.

La seule mesure qui attribue est la **substitution** : on fixe tout — famille
de squelette, nombre de paramètres, recette d'entraînement, données,
augmentations, budget de calcul, graines — et on remplace **un seul composant**.

C'est pour cela que l'architecture est une modification de PTv3 et non un
réseau neuf ([`ARCHITECTURE.md`](ARCHITECTURE.md) § 6). La table de
substitution de [`ETAT_DE_LART.md`](ETAT_DE_LART.md) § 4 n'est pas une
description : **c'est le plan d'expériences.**

Sept règles s'appliquent partout.

1. **Budget apparié.** Mêmes paramètres, mêmes époques, mêmes augmentations,
   même matériel. Un gain obtenu en dépensant plus n'est pas un gain.
2. **Trois graines minimum.** Publier moyenne et écart, jamais le meilleur.
3. **Témoins négatifs obligatoires** (§ 4). Une expérience sans témoin négatif
   ne peut pas distinguer « la structure aide » de « une structure aide ».
4. **Prédictions écrites d'avance** (§ 5), y compris là où l'on prédit de **ne
   pas** gagner.
5. **Tendance, pas point.** Un modèle de fondation se juge sur une pente, pas
   sur une case (§ 6).
6. **Comptabilité du coût** publiée avec chaque chiffre (§ 9).
7. **Validation seule.** La séquence 08 pour SemanticKITTI ; le serveur de test
   n'est touché qu'une fois, à la fin, et jamais pour régler quoi que ce soit.

## 2. Axe 0 — sondes sans apprentissage

Rapides, sur CPU, à faire avant toute chose. Elles disent si l'information est
**présente**, indépendamment de tout modèle.

### 0.1 Plafond d'oracle, stratifié

Pour chaque instance annotée $G$ : $\max_v \mathrm{IoU}(S_v, G)$ sur les nœuds
de la tour. À publier **par classe, par tranche de portée et par taille
d'objet**.

Témoins sur exactement le même oracle : partition en voxels à plusieurs tailles,
superpoints de SPT, HDBSCAN, DBSCAN. C'est la seule façon de dire ce que la
tour ajoute *en tant que partition*, avant tout apprentissage.

C'est une **porte de réfutation, jamais de promotion** : un plafond haut ne
prouve rien sur le modèle appris ; un plafond bas prouve que le modèle appris
ne pourra pas.

### 0.2 Pureté des nœuds

Distribution de la pureté (part de la classe majoritaire) des nœuds, par
niveau, contre les superpoints de SPT à nombre d'unités égal. Comparaison
directe de la qualité de la partition, à budget d'unités identique.

### 0.3 Stabilité sous décimation

L'hypothèse du poster : la forme varie moins que l'échantillonnage. Mesure sans
apprentissage : pour un même objet vu à des portées croissantes (suivi
d'instances réel, **et** décimation contrôlée selon le modèle de balayage),
calculer $\varrho = \delta_{\text{intra}} / \delta_{\text{inter}}$, rapport de
la distance du code à lui-même vu autrement sur sa distance aux autres objets
de la même classe.

Témoins : occupation voxélisée, projection aléatoire fixe, covariance des
points, descripteur de superpoint SPT. Sans ces témoins, le chiffre ne dit
rien.

### 0.4 Sonde XGBoost sur les variables de nœud

Un ensemble d'arbres boostés est ici supérieur à un réseau, et pour des raisons
précises : il tourne en minutes sur CPU, donc on peut se permettre des
dizaines d'ablations ; il est indifférent à l'échelle des variables, donc on
mélange sans peine des rayons en mètres, des comptes entiers et des
proportions ; il donne l'importance par gain et les valeurs de Shapley, donc
une réponse directe à « quelle famille de canaux se paie ? » ; et il ne demande
aucun réglage pour être honnête, donc il ne flatte pas le descripteur.

**XGBoost est l'instrument du descripteur, jamais le modèle.** Ablations
imposées : chaque famille seule, toutes sauf une, base triviale (hauteur,
taille, portée), base « points » (covariance sans hiérarchie), et surtout
**avec et sans les canaux de filtration** — c'est cette dernière qui mesure la
valeur propre de la tour par rapport à une simple sur-segmentation.

### 0.5 Statistiques de l'échelle

Trois observables qui décident du coût réel et de la crédibilité du discours :

- **recouvrement** : nombre moyen de nœuds auxquels un point appartient, par
  niveau ; c'est le nombre de non-zéros de $P_\ell$ ;
- **adaptativité** : histogramme du rayon effectif $r$ par niveau et par
  tranche de portée. Si $r$ ne varie pas avec la portée, l'échelle n'est pas
  adaptative et l'argument central tombe ;
- **forme** : nombre d'unités par niveau, et sa variance entre trames.

### 0.6 Condensation : compression, perte et stabilité

Trois mesures, aucune ne demandant d'apprentissage, à faire dès que l'arbre
condensé existe.

- **Compression** : nombre de nœuds condensés contre nœuds bruts, par ordre et
  par valeur du seuil relatif $\alpha$. Sur une trame sans sol à $K = 1$, la
  forêt brute compte 39 796 fusions pour 39 885 sites : on attend deux ordres
  de grandeur.
- **Perte** : le **plafond d'oracle par classe, avant et après condensation**.
  C'est le vrai coût, et il se paie d'abord sur les classes filiformes — un
  poteau à quarante mètres est une branche de faible masse, donc exactement ce
  qu'un seuil élague. Courbe du plafond en fonction de $\alpha$, par classe.
- **Naturalité** : après condensation couplée entre ordres, revérifier que
  l'image d'une fusion est la fusion des images. Ce n'est pas automatique.

### 0.7 Stabilité de la tour sous rotation pure

La tour est équivariante en théorie ; le moteur consomme un nuage **quantifié à
1 mm**, donc le chemin réel `tourner → quantifier → tour` ne commute pas, et les
prédicats exacts peuvent basculer sur une égalité.

Mesure : dérive du condensé de tour, du nombre de nœuds par niveau et du
plafond d'oracle sous rotations pures autour de $z$. C'est une **mesure directe
de la stabilité de l'objet au pas de quantification choisi**, utile bien
au-delà de ce dossier : si la dérive est forte, la grille de 1 mm est trop
grossière pour la géométrie observée. Coût : quelques heures.

Conséquence opérationnelle quelle que soit l'issue : **on recalcule la tour par
vue augmentée**, on ne suppose pas la commutation.

### 0.8 Quels ordres $K$ servent, et où le pont de bruit casse

Deux questions, une seule expérience, aucune apprentissage.

- **Quels $K$ ?** Plafond d'oracle et pureté par ordre. La tour à
  $K \leq 10$ coûte environ cinq fois celle à $K \leq 5$ ; si les ordres $7$ à
  $10$ n'ajoutent rien, la question est réglée pour tout le reste du projet.
- **Où casse le pont ?** Pour un contact objet–sol donné, le plus petit $K$ qui
  sépare les deux composantes. Le chapitre 7 du manuscrit dit que $K$ résiste
  aux ponts de bruit et le Théorème 3 chiffre la fraction récupérable ; il faut
  la courbe empirique, par classe.

### 0.9 Plafond d'oracle stratifié par **contact**

C'est la porte qui décide du potentiel en instance, et elle manquait.

Plafond d'oracle par classe, **en séparant les objets en contact avec le sol
des objets isolés**, en fonction de $K$, **avec et sans retrait du sol**. Trois
lectures :

1. l'écart entre les deux strates mesure exactement ce que la limite de fond
   coûte (§ 4.6 de l'[architecture](ARCHITECTURE.md)) ;
2. la pente en $K$ dit si l'axe des ordres remplace le retrait du sol. Si oui,
   c'est un résultat en soi : on supprime un prétraitement à seuil posé à la
   main ;
3. le régime sans sol sert de **repère**, jamais de contrat : il masque la
   difficulté au lieu de la résoudre.

### 0.10 Relèvement métrique : compter les naissances avant d'y croire

Construire la tour sur $(x, y, z, \lambda n)$ sépare des surfaces en contact
d'orientations différentes. Avant tout port, une seule mesure : **le nombre de
naissances d'une trame relevée en dimension 6**, contre la même trame en
dimension 3. Le chantier [`E-HGP/`](../../E-HGP/) a mesuré que les naissances
passent de $O(n)$ à $\binom{n}{k}$ quand la dimension monte, l'obstruction
étant gouvernée par la dimension intrinsèque et ramenée par le bruit ambiant.
Si elles explosent, la piste se ferme en une journée.

## 3. Axe 1 — l'étude de substitution

Six bras, un par ligne de la table de substitution. Tout est fixé sauf la ligne
testée.

| bras | référence | variante HGP |
| --- | --- | --- |
| S1 échelle | `GridPool` de PTv3 (taille de voxel) | échelle de la tour, règle `E-persistance` |
| S2 regroupement | max ou moyenne sur cellule | affectation douce aux poids du § 9.1 |
| S3 voisinage | *patch* sur sérialisation Z-order/Hilbert | *patch* du graphe de fusion |
| S4 position | encodage relatif $xyz$ | biais ultramétrique $\varphi(\log r_{uv})$ |
| S5 ordre | néant (un seul graphe) | mixage d'ordres, calendrier de $K$ puis attention croisée |
| S6 décodeur | interpolation trilinéaire ou $k$-NN | vote pondéré § 9.1, Proposition 7 |
| S7 tête d'instance | propositions, suppression non maximale, appariement | sélection par programme dynamique à coût appris sur l'arbre condensé |

S7 a deux témoins obligatoires, et ils sont plus exigeants qu'un détecteur :
**l'excès de masse sur le même arbre condensé** — si le coût appris ne bat pas
$-\widehat{E}(C)$, il n'apporte rien — et **ALPINE**, qui atteint
$\mathrm{PQ} = 64{,}2$ par regroupement géométrique sans aucune étiquette
d'instance.

Deux mesures complémentaires : **une ligne à la fois** depuis la référence
(effet propre), et **une ligne retirée à la fois** depuis le modèle complet
(effet marginal). L'écart entre les deux lectures donne les interactions, qui
sont souvent la vraie information.

À cela s'ajoutent deux comparaisons de chemin, propres à l'objet :
**E-global / E-rang / E-persistance / E-relative** pour la règle de
contraction, et **horizontal / vertical / diagonal iso-densité** pour la
direction dans le treillis.

## 4. Axe 2 — témoins négatifs

**C'est la partie que personne ne fait, et c'est celle qui rend le reste
crédible.** Chacun de ces témoins a le pouvoir d'annuler la conclusion.

### T1 — tour brouillée

Garder exactement le nombre de niveaux, le nombre d'unités par niveau et la
distribution de taille, mais **réaffecter les points au hasard** dans une
partition spatialement locale de mêmes tailles. Si les performances tiennent,
ce n'est pas la structure de la tour qui aide, c'est le fait d'avoir *une*
hiérarchie multi-échelle. Ce serait un résultat négatif majeur, et il faut le
publier.

### T2 — canal de densité seul

Donner $\hat f_K(x) = K / (n \omega_3 r_K(x)^{3})$ comme simple variable par
point à un PTv3 **inchangé**, pour quelques $K$. Si cela capte l'essentiel du
gain, alors la tour n'apporte rien au-delà d'un canal de densité, et il faut le
dire. **C'est le témoin le plus tranchant et le moins cher du protocole ; il
doit être fait en premier.**

### T3 — niveaux permutés

Permuter l'ordre des niveaux de l'échelle. Si l'emboîtement n'a pas
d'importance, la hiérarchie n'agit pas comme une hiérarchie.

### T4 — ordre aléatoire

Remplacer l'axe $K$ par une affectation d'ordre aléatoire dans le mixage. Teste
si OM exploite la sémantique de $K$ ou seulement une capacité supplémentaire.

### T5 — diagnostic du raccourci géométrique

Reprendre le diagnostic de Sonata sur les deux modèles pré-entraînés : à quel
point la représentation encode-t-elle les coordonnées et normales de bas
niveau ? L'argument du § 7.1 de l'architecture prédit que les prétextes de
filtration y sont moins sujets. C'est une prédiction, donc une mesure.

## 5. Axe 3 — prédictions pré-enregistrées

Si le mécanisme est réel, on sait **où** il doit payer. Écrire ces prédictions
avant de mesurer est ce qui transforme un écart en explication.

| # | prédiction | pourquoi |
| --- | --- | --- |
| P1 | le gain **croît avec la portée** | c'est là que l'échelle métrique fixe est la plus fausse |
| P2 | le gain **croît en transfert inter-capteurs** (64 nappes $\to$ 32, et retour) | il n'y a aucune constante à réaccorder |
| P3 | le gain **croît quand les étiquettes se raréfient** ($100 \to 10 \to 1\,\%$) | un a priori structurel vaut le plus quand les données manquent |
| P4 | le gain **croît sous corruption** (pluie, brouillard, neige) | le Théorème 3 chiffre la résistance aux ponts de bruit en fonction de $K$ |
| P5 | le gain sur les classes **filiformes** dépend du calendrier de $K$, et **disparaît si l'on retire $K = 1$** | HGP retarde la naissance des structures minces à $K$ élevé |
| P6 | le gain est **faible ou nul** en champ proche, dense, uniforme, à étiquetage complet | il n'y a là aucune variation d'échelle à absorber |
| P7 | l'arbre **condensé** est nettement plus stable sous décimation que l'arbre brut | il ne garde que les événements qui ont de la masse, donc ceux qui survivent à une perte de points |
| P8 | **FM-6 bat FM-5** : une cible dense d'agrégat multi-trames apprend mieux qu'un simple accord entre deux vues pauvres | un accord entre deux vues peut être satisfait par une représentation triviale, une cible dense non |
| P9 | le plus petit $K$ qui sépare un objet de son support **croît avec la portée** | le contact se fait par moins de retours quand la densité tombe |

**P6 est la plus importante.** Si l'on gagne uniformément, y compris là où la
théorie ne prédit rien, le gain vient probablement du budget de calcul ou d'un
détail d'implémentation, et non du mécanisme. Prédire où l'on ne doit **pas**
gagner est ce qui rend les cinq autres prédictions crédibles.

## 6. Axe 4 — lois d'échelle

Un a priori aide le plus quand les données manquent. Un modèle de fondation est
entraîné sur beaucoup de données. **Il faut donc s'attendre à ce que l'écart se
resserre, et le mesurer honnêtement plutôt que de choisir un point favorable.**

Trois tailles de modèle $\times$ trois tailles de corpus, et l'on ajuste la
tendance de l'écart. Trois issues, toutes publiables :

- l'écart **se maintient** : la tour apporte une information que le modèle
  n'apprend pas seul. C'est la revendication forte ;
- l'écart **se resserre** : la valeur est dans l'efficacité en échantillons et
  le transfert, pas dans le plafond asymptotique. C'est la revendication
  honnête, et elle reste une bonne revendication ;
- l'écart **s'inverse** : la contrainte structurelle devient un carcan à
  grande échelle. Il faut le dire, et probablement n'utiliser la tour qu'au
  pré-entraînement.

## 7. Axe 5 — protocoles d'évaluation, ceux de la communauté

Pour être comparable, on utilise les protocoles établis, sans en inventer.

- **Sonde linéaire** : squelette gelé, tête `BatchNorm` + linéaire. C'est
  l'évaluation qui a servi à établir le problème du raccourci géométrique ;
  elle mesure la qualité de la représentation, pas celle du réglage fin.
- **Réglage fin complet.**
- **Efficacité en étiquettes** : $0{,}1$, $1$, $10$, $50$, $100\,\%$ — les
  points usuels sont $1\,\%$ sur SemanticKITTI, $1$ et $10\,\%$ sur nuScenes.
- **Transfert inter-domaines** : sans réglage, puis avec.
- Métrique : mIoU sur validation, par classe et global ; PQ pour le panoptique.

Suite de tâches, par ordre de priorité :

1. **segmentation sémantique** — SemanticKITTI (val 08), nuScenes, Waymo ;
2. **panoptique** — témoin exigeant : ALPINE atteint $\mathrm{PQ} = 64{,}2$
   **sans étiquette d'instance**, par simple regroupement géométrique. Si la
   tour ne fait pas mieux qu'ALPINE sur l'instance, la géométrie HGP n'apporte
   rien à ce que la géométrie ordinaire donne déjà ;
3. **transfert inter-capteurs** — c'est l'expérience décisive du projet ;
4. **intérieur** (ScanNet, S3DIS) — le terrain d'Utonia, où le rééchelonnage
   explicite est leur réponse et l'échelle dérivée serait la nôtre ;
5. **anomalie et hors-distribution** — application propre du manuscrit, où la
   sur-segmentation ne pénalise pas ;
6. **détection** — les nœuds comme propositions.

## 8. Axe 6 — ce que la tour permet et qu'on ne peut pas comparer

Certaines capacités n'ont pas d'équivalent chez les concurrents et se mesurent
en valeur absolue, pas en écart :

- **propositions d'instance sans apprentissage** à tous les niveaux ;
- **détection d'anomalies guidée par un modèle 3D**, par sélection de branche ;
- **cohérence multi-échelle garantie** : la sortie est emboîtée par
  construction, ce qu'aucune segmentation par points ne garantit ;
- **déterminisme et reproductibilité bit à bit** de la tokenisation.

## 9. Comptabilité du coût

Publiée avec chaque chiffre, sans exception : pré-calcul de la tour (heures
GPU, une fois), taille du cache, débit d'entraînement (échantillons par
seconde), mémoire, paramètres, latence d'inférence de bout en bout. Y compris
quand c'est défavorable.

## 10. Règles de décision

Écrites d'avance, pour que l'échec soit reconnaissable.

| observation | décision |
| --- | --- |
| T2 (canal de densité seul) capte l'essentiel du gain | arrêter la voie architecturale ; publier le canal de densité comme résultat utile et honnête |
| T1 (tour brouillée) égale la tour | ce n'est pas la structure qui aide ; revenir aux sondes de l'axe 0 et comprendre pourquoi |
| S1 seul ne paie pas, mais S3 et S4 paient | la valeur est dans le voisinage et l'encodage de position, pas dans le pooling ; simplifier |
| S5 (axe des ordres) ne paie pas | retirer OM ; le projet perd sa contribution la plus spécifique et il faut le reconnaître |
| S7 à coût appris ne bat pas l'excès de masse sur le même arbre | garder la sélection statistique ; la tête apprise ne se justifie pas |
| Le plafond d'oracle chute fortement à la condensation, quel que soit $\alpha$ | condenser plus tard dans la chaîne, ou garder deux échelles, une condensée pour le contexte et une brute pour les niveaux fins |
| Le plafond d'oracle sur les objets **en contact** reste bas à tout $K$, sans retrait du sol | la limite de fond mord ; soit on assume le retrait du sol et on le dit, soit on mesure le relèvement métrique (porte 0.10) |
| Les ordres $7$ à $10$ n'ajoutent rien (porte 0.8) | fixer $K_{\max} = 6$ partout et diviser le coût de préparation par cinq |
| La dérive sous rotation pure (porte 0.7) est du même ordre que le gain mesuré | la grille de 1 mm est trop grossière ; aucun écart de modèle n'est interprétable avant de l'avoir affinée |
| Le plafond d'oracle est bas sur les classes filiformes même avec $K = 1$ | la tour perd ces classes ; se replier sur l'instance et l'anomalie |
| P1 à P4 se vérifient, P6 aussi | le mécanisme est établi ; passer à l'échelle |
| Gain uniforme, y compris là où P6 prédit rien | chercher le confondant avant de publier |

## 11. Reçus

Toute mesure produit un reçu immuable ancré au commit : condensés de tour et de
catalogue des trames consommées, sha256 du corpus, configuration complète,
graines, matériel, sorties brutes, et le coût du § 9. Une mesure sans reçu est
requalifiée par l'audit, c'est-à-dire qu'elle n'existe pas. C'est la règle du
dépôt et elle s'applique à l'apprentissage comme au moteur.

Les poids pré-entraînés de la lignée Pointcept sont en CC-BY-NC 4.0 : on les
compare, on ne les importe jamais dans la ligne produit. Aucun octet
SemanticKITTI n'est versionné ; seuls les manifestes le sont.
