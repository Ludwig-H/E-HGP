# Plan d'exécution

26 septembre 2026. Dans quel ordre construire, ce que chaque phase produit, et
à quel moment décider. Les mesures sont définies dans [`MESURE.md`](MESURE.md).

Principe d'ordonnancement : **ce qui peut annuler le projet se fait en premier
et coûte le moins cher.** Les deux témoins les plus tranchants — le canal de
densité seul, et la tour brouillée — ne demandent aucun développement
d'architecture.

## Phase 0 — le raccord et les sondes avant HGP-UNet

**Construire.**

1. Un **exportateur** de la tour vers un format consommable : forêt tous
   ordres, cartes verticales, populations, et pour chaque facette les rayons
   au carré de ses cofaces. Un adaptateur expérimental pourra préparer
   `CertifiedTowerInput` de `morsehgp3d/`, dont le réducteur exact (vote
   pondéré du § 9.1, excès de masse) attend déjà un producteur. Ne pas
   fabriquer les reçus de certification que la v9 ne publie pas.
2. La **condensation** : élagage à seuil **relatif** $\alpha$ sur la masse
   $m_\tau$ du § 9.1, généralisé aux multifusions (jamais binariser), couplé
   entre les ordres, avec revérification de la naturalité des cartes
   verticales. Sortie : arbre condensé, stabilité par nœud, niveau de sortie
   $\hat\lambda_x$ par point.
3. Le **constructeur d'échelle**, sur l'arbre condensé : les quatre règles
   `E-global`, `E-rang`, `E-persistance`, `E-relative`, et les chemins de pooling
   emboîtants : horizontal, vertical et anti-diagonal ($r$ croissant, $K$
   décroissant). L'iso-densité est réservée aux comparaisons latérales entre
   ordres. Sortie : $L$ matrices d'affectation creuses par trame.
4. Le **cache** : tours pré-calculées sur le corpus, plus une banque de vues
   décimées par trame pour FM-5. Chaque vue augmentée est re-quantifiée et sa tour
   recalculée ; le cache conserve la provenance de la vue.

**Mesurer.** D'abord les sondes sans étiquette de l'axe 0 : couverture
des retours, coût des incidences, stabilité sous décimation et rotation,
adaptativité, compression et naturalité de la condensation. Les oracles
d'instances, la pureté et XGBoost utilisent des étiquettes ou un ajustement
appris : leur protocole est défini dans [`MESURE.md`](MESURE.md), mais leur
exécution attend l'ouverture de cette évaluation. La perte des classes
filiformes après condensation restera alors une porte de décision.

**Préparer le témoin T2 dès cette phase.** Il demande toutefois l'entraînement
d'un PTv3 et sera exécuté avec la phase 1, lorsque les comparaisons apprises
seront ouvertes. La phase 0 ferme d'abord l'interface FULL → coupes et ses
portes sans apprentissage.

Le protocole comprend aussi quatre portes issues de l'audit de conception : **0.7** dérive de
la tour sous rotation pure, qui dit si la grille de 1 mm est assez fine pour
que le moindre écart de modèle soit interprétable ; **0.8** quels ordres $K$
servent et à quel $K$ casse un pont de bruit ; **0.9** plafond d'oracle
**stratifié par contact avec le sol**, avec et sans retrait du sol ; **0.10**
nombre de naissances d'une trame relevée en dimension 6, avant de croire au
relèvement métrique. Les portes 0.8 et 0.9 qui exploitent les étiquettes
restent définies ici et seront exécutées avec l'évaluation correspondante.

Les onze **portes du tokenizer** de [`SPECIFICATION.md`](SPECIFICATION.md) § 9
se ferment ici, notamment couverture des retours, composition des coupes,
naturalité entre K, budget et traçabilité.

**Décision.** Si l'export des incidences, la couverture des retours ou les
cartes entre coupes échouent, corriger ce raccord avant la phase 1. Les sondes de pureté,
d'adaptativité et de stabilité restent des diagnostics de la valeur du
tokenizer, et non une preuve de valeur apprise. Si la dérive de la porte 0.7
est du même ordre que les écarts que l'on espère mesurer, il faut affiner la
grille avant d'interpréter ces écarts.

## Phase 1 — HGP-UNet supervisé, et la substitution de l'échelle

**Construire.** HGP-UNet **dans** la base PTv3 : `GridPool` remplacé par
`FiltrationPool`, décodeur remplacé par PUR. Rien d'autre ne bouge.

**Mesurer.** D'abord T2 (canal de densité seul) à réseau inchangé, puis les
bras S1, S2 et S6 de l'étude de substitution ; les quatre
règles d'échelle ; les trois chemins emboîtants, avec l'iso-densité
comme lecture latérale ; le témoin T1 (tour brouillée) et T3
(niveaux permutés). Stratification par portée, par taille d'objet et par
classe. SemanticKITTI val 08, trois graines.

**Ce que la phase produit.** Un premier résultat publiable et net :
*remplacer le pooling par grille d'un Transformer de points par une échelle de
densité canonique*, avec la stratification par portée qui en montre le
mécanisme. C'est un article qui ne dépend d'aucun pré-entraînement et se
compare à budget apparié.

**Décision.** Si S1 ne paie pas mais que la stratification par portée montre
quand même l'adaptativité attendue, chercher du côté du voisinage (phase 2)
avant de conclure.

## Phase 2 — le voisinage, la position et les ordres

**Construire.** Les *patches* du graphe de fusion (S3), le biais ultramétrique
(S4), le mixage d'ordres dans ses trois réalisations par coût croissant (S5).

**Mesurer.** S3, S4, S5 en effet propre et en effet marginal ; le témoin T4
(ordre aléatoire) ; l'ablation du canal $\log r$, qui dit si le modèle
fonctionne en régime d'invariance ou d'équivariance d'échelle.

**Ce que la phase produit.** La réponse à la question la plus spécifique du
projet : **l'axe des ordres vaut-il quelque chose ?** C'est la contribution
qu'aucun concurrent ne peut avoir, et c'est donc celle qu'il faut savoir
abandonner si elle ne se paie pas.

## Phase 3 — pré-entraînement

**Construire.** Les six tâches de modélisation de filtration — dont **FM-6, la
distillation d'agrégat**, qui prédit depuis une trame isolée la structure de
fusion de la tour d'un agrégat multi-trames recalé par l'odométrie —, le
masquage par nœuds entiers, l'auto-distillation avec décimation en portée,
retrait d'anneaux et occultation.

**Mesurer.** Sonde linéaire, réglage fin, efficacité en étiquettes à
$0{,}1 / 1 / 10 / 50 / 100\,\%$. Le témoin T5, diagnostic du raccourci
géométrique. L'ablation décisive : **le même pré-entraînement sans les tâches
de filtration**, auto-distillation seule. Si l'écart est nul, la contribution
propre au pré-entraînement est nulle, et il ne reste que l'architecture.

**Prédiction à vérifier.** P3 : le gain doit croître quand les étiquettes se
raréfient. Un gain uniforme est plutôt le symptôme d'une régularisation.

**Ce que la phase produit.** Le premier résultat de modèle de fondation, et la
comparaison frontale avec Sonata, Vernata et Utonia sur leurs propres
protocoles.

## Phase 4 — transfert et échelle

**Mesurer.** L'expérience décisive : transfert inter-capteurs, 64 nappes vers
32 et retour, **avec les hyperparamètres du tokenizer gelés**. Leur transfert sans
réaccord est une hypothèse à tester.
Puis le transfert vers l'intérieur, terrain d'Utonia. Puis les lois d'échelle,
trois tailles de modèle par trois tailles de corpus, avec la tendance de
l'écart publiée quelle qu'elle soit.

**Ce que la phase produit.** La revendication de fondation, ou sa
requalification honnête en efficacité d'échantillons et transfert.

## Phase 5 — les capacités propres et la tête de sélection

La tête SEL — programme dynamique du § 5.2 à coût appris — se mesure ici, avec
son vrai témoin : **l'excès de masse sur le même arbre condensé**. Si le coût
appris ne bat pas $-\widehat{E}(C)$, il ne se justifie pas. Son plafond est
celui de l'oracle de la porte 0.1.

Panoptique contre ALPINE, propositions d'instance sans apprentissage,
anomalies guidées par un modèle 3D, cohérence multi-échelle garantie. Ces
tâches ont une valeur propre et restent publiables même si les phases 3 et 4
déçoivent.

## Ce qui peut être fait en parallèle, sans attendre

- le contrat mathématique et logiciel des coupes et cartes entre K réellement
  consommées, depuis FULL jusqu'aux matrices du tokenizer ;
- la reproduction de la base de référence SemanticKITTI, qui conditionne toute
  comparaison chiffrée ;
- la recherche d'antériorité exhaustive, à faire une fois, sérieusement.

## Vue d'ensemble

| phase | apprentissage | ce qui la termine | ce qui l'annule |
| --- | --- | --- | --- |
| 0 raccord et sondes | aucun HGP-UNet | export/coupes vérifiés, sondes sans étiquette publiées | incidences ou cartes incompatibles, coût hors budget |
| 1 échelle | supervisé | T2 puis S1, S2, S6 mesurés, T1 et T3 passés | T2 ou T1 explique le gain |
| 2 voisinage et ordres | supervisé | S3, S4, S5 mesurés, T4 passé | S5 nul, donc OM retiré |
| 3 pré-entraînement | SSL | sonde linéaire et efficacité en étiquettes | filtration sans effet contre auto-distillation seule |
| 4 transfert et échelle | SSL | transfert inter-capteurs, lois d'échelle | écart qui s'inverse à l'échelle |
| 5 capacités propres | mixte | panoptique, anomalie, instance | ALPINE non battu |
