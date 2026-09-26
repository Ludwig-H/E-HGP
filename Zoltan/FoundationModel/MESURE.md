# Mesurer l'apport

26 septembre 2026. Comment établir, et non affirmer, ce que la hiérarchie
$K$-NN apporte par rapport aux méthodes existantes.

Le [protocole de guidage FULL](GUIDAGE_FULL_ET_PREENTRAINEMENT_20260926.md)
sépare FULL enseignant hors ligne et FULL dans le tokenizer. Le premier
pilote compare des vues d'une seule trame ; l'agrégat temporel FM-6 reste
un régime secondaire.

## 1. La doctrine : attribuer les effets et comparer les systèmes

Comparer « HGP-FM » à « Sonata » ou à « Utonia » mesure une performance de
système. Ces comparaisons sont utiles, mais un écart ne s'attribue pas à HGP
seul lorsque changent le squelette, les données, la recette ou le budget.

Pour attribuer un effet, le pilote utilise la **substitution** : même famille
de squelette, recette, données, augmentations et graines appariées ; une
intervention à la fois, puis les interactions. Apparier séparément exposition
et coût : changer la structure peut changer les FLOPs malgré des largeurs
identiques. Ajouter ensuite une comparaison des systèmes avec un budget de
réglage identique, pour ne pas confondre ablation figée et meilleur compromis
atteignable par chaque méthode.

C'est pour cela que l'architecture est une modification de PTv3 et non un
réseau neuf ([`ARCHITECTURE.md`](ARCHITECTURE.md) § 6). La table de
substitution de [`ETAT_DE_LART.md`](ETAT_DE_LART.md) § 4 n'est pas une
description : **c'est le plan d'expériences.**

Sept règles s'appliquent partout.

1. **Budget apparié.** Mêmes données et augmentations ; comparer à exposition
   égale, puis à coût total égal. Paramètres, époques et matériel identiques
   n'égalisent pas les FLOPs, la préparation, le cache ni le débit mesuré.
2. **Trois graines minimum.** Publier moyenne et écart, jamais le meilleur.
   Distinguer cet aléa de la variabilité entre scènes ; respecter les blocs
   temporels et séquences dans les intervalles d'évaluation.
3. **Témoins négatifs obligatoires** (§ 4). Une expérience sans témoin négatif
   ne peut pas distinguer « la structure aide » de « une structure aide ».
4. **Prédictions écrites d'avance** (§ 5), y compris là où l'on prédit de **ne
   pas** gagner.
5. **Tendance, pas point.** Un modèle de fondation se juge sur une pente, pas
   sur une case (§ 6).
6. **Comptabilité du coût** publiée avec chaque chiffre (§ 9).
7. **Partitions explicites.** La séquence 08 sert à la validation SemanticKITTI ;
   le serveur de test n'est touché qu'une fois, à la fin. Les scans 08 ne servent
   ni au pré-entraînement, ni à l'ajustement des statistiques apprises sur le
   corpus (normalisations, quantiles de référence, priors). Les statistiques
   calculées sur chaque vue par une règle gelée restent autorisées.
   Un régime transductif serait une expérience distincte.
   Réserver des blocs des séquences d'entraînement pour sélectionner les
   variantes ; figer ensuite la configuration avant le bilan sur 08. Une
   fenêtre temporelle de pré-entraînement reste dans la partition de son ancre.

## 2. Axe 0 — sondes sans apprentissage

Sondes à réaliser avant HGP-UNet. Certaines sont sans étiquette ; l'oracle
d'instance, la pureté et XGBoost utilisent des étiquettes ou un ajustement
appris. Leur protocole est défini ici, sans exécution dans l'audit présent.

### 0.1 Plafond d'oracle, stratifié

Pour chaque instance annotée $G$ : $\max_v \mathrm{IoU}(S_v, G)$ sur les nœuds
de la tour. C'est le **diagnostic de proposition par nœud unique**, à publier
par classe, portée et taille. Il ne borne ni une sortie point avec connexion
de saut, ni la sélection de plusieurs nœuds suivie du vote pondéré. Ajouter
un oracle soumis à la vraie antichaîne, aux mêmes poids § 9.1 et au même
budget de jetons, puis un oracle du décodeur point avec les mêmes matrices.

Témoins sur exactement le même oracle : partition en voxels à plusieurs tailles,
superpoints de SPT, HDBSCAN, DBSCAN. C'est la seule façon de dire ce que la
tour ajoute *en tant que partition*, avant tout apprentissage.

**C'est une porte de réfutation pour une tête qui rend un seul nœud par
instance** : un plafond haut ne prouve rien sur le modèle appris ; un plafond
bas n'écarte pas les décodeurs plus expressifs ci-dessus.

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

Séparer raréfaction des retours et perte de visibilité. L'occultation peut
retirer un pont et changer correctement la tour, y compris sur les IDs
communs : FM-5 ne demande donc pas l'égalité des arbres recalculés. Le
[contre-exemple K1](GUIDAGE_FULL_ET_PREENTRAINEMENT_20260926.md) et la masse
commune définissent ce que l'on peut comparer entre vues.

### 0.4 Sonde XGBoost sur les variables de nœud

Un ensemble d'arbres boostés fournit une sonde peu coûteuse pour comparer les
familles de variables. Fixer son budget de réglage et ses partitions avant les
ablations ; mesurer le temps. Les importances par gain ou Shapley sont des
diagnostics, à confronter au retrait effectif des canaux corrélés.

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
  tranche de portée, accompagné de couverture et de masse conservée. Un rayon
  variable peut seulement suivre la raréfaction ; son utilité reste à mesurer ;
- **forme** : nombre d'unités par niveau, et sa variance entre trames.

### 0.6 Condensation : compression, perte et stabilité

Trois mesures, aucune ne demandant d'apprentissage, à faire dès que l'arbre
condensé existe.

- **Compression** : nombre de nœuds condensés contre nœuds bruts, par ordre et
  par valeur du seuil relatif $\alpha$. Sur une trame sans sol à $K = 1$, la
  forêt brute compte 39 796 fusions pour 39 885 sites ; le facteur de
  compression reste à mesurer, sans le déduire de ces seuls comptes.
- **Perte** : le **plafond d'oracle par classe, avant et après condensation**.
  C'est un coût pour les propositions par nœud ; il peut toucher les
  structures filiformes de faible masse. Publier la courbe selon $\alpha$,
  puis distinguer ce diagnostic du décodeur point avec connexion fine.
- **Naturalité** : après condensation couplée entre ordres, revérifier que
  l'image d'une fusion est la fusion des images. Ce n'est pas automatique.

### 0.7 Stabilité de la tour sous rotation pure

La tour est équivariante en théorie ; le moteur consomme un nuage **quantifié à
1 mm**, donc le chemin réel `tourner → quantifier → tour` ne commute pas, et les
prédicats exacts peuvent basculer sur une égalité.

Mesure : dérive du condensé de tour, du nombre de nœuds par niveau et du
plafond d'oracle sous rotations pures autour de $z$. C'est une **mesure directe
de la stabilité de l'objet au pas de quantification choisi**, utile bien
au-delà de ce dossier. Si la dérive est forte, comparer plusieurs précisions
et surtout ses effets sur affectations, couverture et sorties utiles ; un
changement combinatoire seul ne prouve pas que le profil est inutilisable.

Conséquence opérationnelle quelle que soit l'issue : **on recalcule la tour par
vue augmentée**, on ne suppose pas la commutation.

### 0.8 Quels ordres $K$ servent, et où le pont de bruit casse

Deux diagnostics sans apprentissage, prolongés par les ablations aval.

- **Quels $K$ ?** Plafond d'oracle, couverture et pureté par ordre, puis
  apport aval à budget total de requêtes ou de jetons égal. Mesurer les coûts
  de FULL, de l'export et du réseau pour chaque Kmax ; une réduction de Kmax
  ne garantit aucun facteur fixe de réduction du coût.
- **Où casse le pont ?** Pour un contact objet–sol donné, le plus petit $K$ qui
  sépare les deux composantes. Le chapitre 7 du manuscrit dit que $K$ résiste
  aux ponts de bruit et le Théorème 3 chiffre la fraction récupérable ; il faut
  la courbe empirique, par classe.

### 0.9 Plafond d'oracle stratifié par **contact**

C'est la porte qui décide du potentiel en instance, et elle manquait.

Plafond d'oracle par classe, **en séparant les objets en contact avec le sol
des objets isolés**, en fonction de $K$, **avec et sans retrait du sol**. Trois
lectures :

1. l'écart entre les strates diagnostique la difficulté du contact ; portée,
   taille et visibilité restent des facteurs à contrôler ;
2. la pente en K teste si certains ordres séparent le contact avant la perte
   de l'objet ; cette séparation n'est pas garantie ;
3. le sans-sol est une variante légitime : comparer tour brute et tour
   non-sol avec branche sol/contexte, raccordées à **tous les IDs**. Figer le
   masque géométrique sur la trame entière avant les vues ; garder les unknown,
   payer son coût et ne pas confondre masque sol et classe sémantique. Ce
   régime ne remplace pas la qualification du moteur sur brut.

### 0.10 Relèvement métrique : compter les naissances avant d'y croire

Construire la tour sur $(x, y, z, \lambda n)$ sépare des surfaces en contact
d'orientations différentes. Avant tout port, une seule mesure : **le nombre de
naissances d'une trame relevée en dimension 6**, contre la même trame en
dimension 3. Le chantier [`E-HGP/`](../../E-HGP/) a mesuré que les naissances
passent de $O(n)$ à $\binom{n}{k}$ quand la dimension monte, l'obstruction
étant gouvernée par la dimension intrinsèque et ramenée par le bruit ambiant.
Si elles explosent, la piste se ferme en une journée.

## 3. Axe 1 — l'étude de substitution

Sept substitutions architecturales, un composant à la fois. Les croiser avec
l'usage de FULL comme enseignant selon le plan A0G0/A0G1/A1G0/A1G1 du
[protocole de guidage](GUIDAGE_FULL_ET_PREENTRAINEMENT_20260926.md). Commencer
par A0G0/A0G1, qui gardent le tokenizer de référence.

| bras | référence | variante HGP |
| --- | --- | --- |
| S1 échelle | `GridPool` de PTv3 (taille de voxel) | échelle de la tour, règle `E-persistance` |
| S2 regroupement | max ou moyenne sur cellule | affectation douce aux poids du § 9.1 |
| S3 voisinage | *patch* sur sérialisation Z-order/Hilbert | *patch* du graphe de fusion |
| S4 position | sans biais ou biais XYZ sur noyau compatible | biais dérivé des rayons de fusion, sur le même noyau |
| S5 ordre | K1 répété et capacité appariée | branches K autonomes et attention croisée ; calendrier seulement après compatibilité pondérée |
| S6 décodeur | gather par indices inverses + skip de PTv3 | projection pondérée vers les retours, avec skip fin conservé |
| S7 tête d'instance | propositions, suppression non maximale, appariement | sélection par programme dynamique à coût appris sur l'arbre condensé |

S7 a deux témoins obligatoires : **l'excès de masse sur le même arbre condensé**
et **ALPINE**, appliqués aux mêmes prédictions sémantiques pour isoler la tête
d'instance. Le $\mathrm{PQ}=64{,}2$ publié pour ALPINE + MinkUNet sans TTA sur
la validation SemanticKITTI dépend de cette tête sémantique ; ce n'est pas un
seuil universel à battre avec un autre backbone. Rapporter PQ, PQ des choses,
RQ et SQ ainsi que mIoU, budget et recours aux labels d'instance. Voir la
[table 1 d'ALPINE](https://arxiv.org/html/2503.13203v2#S4.T1).

Deux mesures complémentaires : **une ligne à la fois** depuis la référence
(effet propre), et **une ligne retirée à la fois** depuis le modèle complet
(effet marginal). L'écart entre les deux lectures donne les interactions, qui
sont souvent la vraie information.

À cela s'ajoutent deux comparaisons de chemin, propres à l'objet :
**E-global / E-rang / E-persistance / E-relative** pour la règle de
contraction, et **horizontal / vertical / anti-diagonal emboîtant** pour le
pooling. L'iso-densité est une comparaison latérale entre ordres, pas un
chemin de pooling.

## 4. Axe 2 — témoins négatifs

Chaque témoin teste une explication précise, dans le régime et le budget
évalués ; il ne réfute pas à lui seul tous les usages de FULL.

### T1 — tour brouillée

Construire une hiérarchie témoin spatialement locale, avec budgets de niveaux,
unités, masses et arêtes appariés. Toute réaffectation doit conserver les
contrats de couverture, de quotient et de composition : des parents invalides
ne sont pas un témoin causal. Publier les distributions effectivement
appariées. Une égalité de performance limite la valeur de la structure exacte
dans ce bras ; elle ne tranche pas le rôle d'enseignant. Pour celui-ci,
brouiller les cibles dans des strates distance/portée/rayon préservées.

### T2 — concentration et géométrie locales seules

Donner $\hat f_K(x) = K / (n \omega_3 r_K(x)^{3})$ comme simple variable par
point à un PTv3 **inchangé**, pour quelques K avec $r_K(x)>0$ ; publier la
convention du voisin propre, des sites distincts et des doublons, et traiter
les zéros séparément. Ce score décrit une **concentration ambiante 3D**, pas
automatiquement une densité physique de surface ; il dépend aussi du n global.
Le [calcul sur une surface plane](GUIDAGE_FULL_ET_PREENTRAINEMENT_20260926.md)
explicite la distinction.

Renforcer le témoin par des profils de $\log(r_K/r_0)$, comptes multi-rayons et
anisotropie. Apparier portée, rémission et autres canaux d'acquisition entre
bras ; mesurer l'apport conditionnel de chaque famille. Si ce témoin capte le
gain, simplifier la variante concernée puis évaluer séparément le guidage
FULL. Ce témoin appris est prioritaire, après les contrôles sans apprentissage.

### T3 — niveaux permutés

Permuter les canaux de niveau dans une variante conservant les mêmes coupes
et cartes valides. Pour tester le choix des coupes lui-même, construire une
autre chaîne emboîtée à budget apparié. Ne pas inverser arbitrairement les
opérateurs de pooling : une incompatibilité de dimensions ou de parents
testerait une interface cassée.

### T4 — information des ordres et capacité

Une permutation **globale fixe** des noms de K peut être absorbée par le
réseau : elle ne teste pas la valeur géométrique des ordres. Le témoin
principal répète K1 dans autant de branches que la variante multi-K, avec
mêmes largeurs, têtes et budget total de requêtes/jetons. Comparer aussi K1
unique à capacité totale appariée, puis multi-K sans son canal numérique K.
Une permutation des canaux K entre trames, sans casser cartes ni affectations,
diagnostique l'usage de l'étiquette K ; elle ne remplace pas ces contrôles de
capacité. Publier le budget effectivement apparié et les différences restantes.

### T5 — diagnostic du raccourci géométrique

Reprendre le diagnostic de Sonata sur les deux modèles pré-entraînés, sans
traiter toute information géométrique comme un échec : elle reste utile en
3D. Comparer les prétextes à des témoins locaux, puis retirer le contexte
distant pour voir ce qu'il apporte. Mesurer surtout la qualité aval : une
prédiction FULL exacte à partir d'une distance locale ne démontre pas une
représentation sémantique plus utile.

Ajouter le témoin **encodeur constant + tête recevant r** du
[contrat de guidage](GUIDAGE_FULL_ET_PREENTRAINEMENT_20260926.md), puis les
priors ajustés sur statistiques visibles. Une majorité différente selon le
rayon peut donner une bonne précision avec deux classes équilibrées au total.
Pour K1, publier BCE/Brier et gain face au prior sur $Y_V=0$, séparément du
cas $Y_V=1$ déjà résolu par inclusion. Garder un échantillon d'évaluation
représentatif de la loi des requêtes ; une calibration sur positifs/négatifs
rééquilibrés sans correction est celle d'une autre distribution. Le guidage
multi-K compare collision Γ et, si retenu, recouvrement des distributions O,
avec couverture et concentration des incidences publiées.

## 5. Axe 3 — prédictions pré-enregistrées

Écrire les hypothèses avant de mesurer, avec les régimes où elles pourraient
échouer. Elles ne sont pas des conséquences de l'exactitude de FULL.

| # | prédiction | pourquoi |
| --- | --- | --- |
| P1 | le gain pourrait croître avec la portée | une échelle adaptative peut aider, mais la perte d'information peut aussi dominer |
| P2 | le gain pourrait croître en transfert inter-capteurs (64 nappes $\to$ 32, et retour) | geler les réglages ; le changement de balayage et de visibilité n'est pas une homothétie |
| P3 | le gain **croît quand les étiquettes se raréfient** ($100 \to 10 \to 1\,\%$) | un a priori structurel vaut le plus quand les données manquent |
| P4 | le gain pourrait croître sous certaines corruptions | tester les corruptions physiques ; une borne sur des ponts de bruit ne couvre pas tous les effets météo |
| P5 | le gain sur les classes **filiformes** dépend des branches K et peut diminuer sans K1 | comparer avec et sans K1 à couverture et budget déclarés ; garder la connexion fine |
| P6 | le gain est **faible ou nul** en champ proche, dense, uniforme, à étiquetage complet | il n'y a là aucune variation d'échelle à absorber |
| P7 | l'arbre **condensé** pourrait être plus stable sous décimation que le brut | mesurer stabilité et couverture ; un seuil peut aussi retirer des structures utiles |
| P8 | dans le régime temporel secondaire, FM-6 pourrait apporter plus que les vues mono-scan | apparier accès aux trames/odométrie et coût ; une cible dense n'exclut pas par elle-même une représentation constante |
| P9 | le plus petit $K$ qui sépare un objet de son support **varie avec la portée**, sans signe imposé | raréfaction du contact et perte de l'objet agissent en sens opposés ; mesurer les deux |

P6 aide à tester l'explication par l'adaptation d'échelle. Un gain uniforme
invite à comparer les autres mécanismes et budgets ; il ne prouve pas un
confondant. Une perte prétexte faible ne prouve pas non plus un gain sémantique.

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

## 7. Axe 5 — protocoles d'évaluation et unités de comparaison

Épingler le protocole, son code et les manifestes des sous-ensembles avant la
comparaison. Les protocoles existants ne partagent pas tous la même unité de
faible annotation ni le même accès aux données de pré-entraînement.

- **Sonde linéaire** : squelette gelé, tête `BatchNorm` + linéaire. C'est
  l'évaluation qui a servi à établir le problème du raccourci géométrique ;
  elle mesure la qualité de la représentation, pas celle du réglage fin.
- **Réglage fin complet.**
- **Efficacité en étiquettes** : $0{,}1$, $1$, $10$, $50$, $100\,\%$, en
  déclarant l'unité. Le protocole SemanticKITTI de
  [TARL §4.1](https://www.ipb.uni-bonn.de/pdfs/nunes2023cvpr.pdf) reprend les
  sous-ensembles SegContrast de **scans annotés** ; ce n'est pas 1 % de points
  étiquetés dans toutes les trames. Reprendre les manifestes et publier scans,
  points annotés et classes présentes. Les scribbles constituent un autre
  budget. Pour nuScenes, déclarer précisément le sous-ensemble adopté, sans
  assimiler `mini`, pourcentage de scènes et pourcentage de points.
- **Transfert inter-domaines** : sans réglage, puis avec ; publier la table de
  classes communes et celles ignorées. Un changement de corpus mêle capteur,
  environnement et taxonomie ; le diagnostic de raréfaction à scène fixe et
  le transfert réel répondent à deux questions distinctes.
- Métrique : mIoU sur validation, par classe et global ; PQ pour le panoptique.

Suite de tâches, par ordre de priorité :

1. **segmentation sémantique** — SemanticKITTI (val 08), nuScenes, Waymo ;
2. **panoptique** — comparer ALPINE et SEL avec la même tête sémantique,
   le même protocole et budget ; une absence de gain limite cette tête
   d'instance, sans réfuter le guidage de représentations ;
3. **transfert inter-capteurs** — c'est l'expérience décisive du projet ;
4. **intérieur** (ScanNet, S3DIS) — le terrain d'Utonia, où le rééchelonnage
   explicite est leur réponse et l'échelle dérivée serait la nôtre ;
5. **anomalie et hors-distribution** — application propre du manuscrit, où la
   sur-segmentation ne pénalise pas ;
6. **détection** — les nœuds comme propositions.

## 8. Axe 6 — capacités structurelles à mesurer

Les capacités suivantes ont aussi des antécédents ; mesurer leurs propriétés
propres et leurs coûts, puis comparer les sorties réellement permises :

- **propositions d'instance sans apprentissage** à tous les niveaux ;
- **détection d'anomalies guidée par un modèle 3D**, par sélection de branche ;
- **cohérence multi-échelle** : les quotients retenus sont emboîtés sous les
  conditions du contrat ; des argmax pondérés par point à plusieurs niveaux
  ne deviennent pas automatiquement des partitions emboîtées ;
- **déterminisme de la tokenisation**, avec environnement numérique épinglé
  pour les poids et descripteurs, décisions exactes distinguées des arrondis.

## 9. Comptabilité du coût

Publier préparation des vues/masques, FULL, export, cache et transferts,
puis paramètres, FLOPs, non-zéros des incidences, mémoire et débit réel
d'entraînement. Compter la banque de vues entière, pas une seule tour par
scan si plusieurs sont calculées. Comparer exposition égale et coût total
égal. À l'inférence : latence froide/chaude, débit et mémoire de bout en bout ;
le bras enseignant seul n'y calcule pas FULL. Les coûts mesurés décident des
compromis, y compris lorsqu'ils sont défavorables.

## 10. Règles de décision

Écrites d'avance, pour que l'échec soit reconnaissable.

| observation | décision |
| --- | --- |
| T2 (témoin local renforcé) capte l'essentiel du gain | simplifier la variante testée ; évaluer séparément FULL enseignant avant toute conclusion plus large |
| T1 (hiérarchie témoin valide) égale la tour | limiter la revendication de structure exacte pour ce bras et ce budget ; revenir aux sondes de l'axe 0 |
| S1 seul ne paie pas, mais S3 et S4 paient | la valeur est dans le voisinage et l'encodage de position, pas dans le pooling ; simplifier |
| S5 (axe des ordres) ne paie pas | retirer OM ; le projet perd sa contribution la plus spécifique et il faut le reconnaître |
| S7 à coût appris ne bat pas l'excès de masse sur le même arbre | garder la sélection statistique ; la tête apprise ne se justifie pas |
| Le plafond d'oracle chute fortement à la condensation, quel que soit $\alpha$ | condenser plus tard dans la chaîne, ou garder deux échelles, une condensée pour le contexte et une brute pour les niveaux fins |
| L'oracle par nœud sur les objets **en contact** reste bas à tout K, sans retrait du sol | tester tour non-sol + contexte sol et décodeur point avant d'engager le relèvement métrique distinct de la porte 0.10 |
| Les ordres $7$ à $10$ n'ajoutent rien dans les bras évalués (porte 0.8) | essayer Kmax=6 et publier la réduction de coût réellement mesurée, sans facteur présumé ni extension à tous les usages |
| La dérive sous rotation pure (porte 0.7) dégrade les affectations ou sorties utiles | comparer des profils de précision et publier leurs coûts ; ne pas comparer directement un compte de nœuds à une métrique aval |
| L'oracle par nœud unique est bas sur les classes filiformes même avec K1 | écarter cette tête de proposition ; tester séparément lecture multi-nœuds et décodeur point avec connexion fine |
| P1 à P4 se vérifient, P6 aussi | poursuivre à plus grande échelle avec les témoins ; ces tendances soutiennent le mécanisme sans le démontrer seules |
| Gain uniforme, y compris là où P6 prédit rien | revoir l'explication et vérifier budgets/interactions sans conclure automatiquement à un confondant |

## 11. Reçus

Toute mesure produit un reçu immuable ancré au commit : condensés de tour et de
catalogue des trames consommées, sha256 du corpus, configuration complète,
graines, matériel, sorties brutes, et le coût du § 9. Une mesure sans reçu est
requalifiée par l'audit, c'est-à-dire qu'elle n'existe pas. C'est la règle du
dépôt et elle s'applique à l'apprentissage comme au moteur.

Les poids pré-entraînés de la lignée Pointcept sont en CC-BY-NC 4.0 : on les
compare, on ne les importe jamais dans la ligne produit. Aucun octet
SemanticKITTI n'est versionné ; seuls les manifestes le sont.
