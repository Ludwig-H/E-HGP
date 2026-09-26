# Plan d'exécution

26 septembre 2026. Dans quel ordre construire, ce que chaque phase produit, et
à quel moment décider. Les mesures sont définies dans [`MESURE.md`](MESURE.md).

Principe d'ordonnancement : **ce qui peut annuler le projet se fait en premier
et coûte le moins cher.** Les deux témoins les plus tranchants — le canal de
densité seul, et la tour brouillée — ne demandent aucun développement
d'architecture.

## Phase 0 — le raccord et les sondes (aucun apprentissage)

**Construire.**

1. Un **exportateur** de la tour vers un format consommable : forêt tous
   ordres, cartes verticales, populations, et pour chaque facette les rayons
   au carré de ses cofaces. Cible naturelle : le `CertifiedTowerInput` de
   `morsehgp3d/`, dont le réducteur exact (vote pondéré du § 9.1, excès de
   masse) attend déjà un producteur.
2. Le **constructeur d'échelle** : les quatre règles `E-global`, `E-rang`,
   `E-persistance`, `E-relative`, et les trois chemins horizontal, vertical,
   diagonal iso-densité. Sortie : $L$ matrices d'affectation creuses par trame.
3. Le **cache** : tours pré-calculées sur le corpus, plus une banque de vues
   décimées par trame pour FM-5. Les augmentations rigides et d'échelle ne
   demandent aucun recalcul.

**Mesurer.** Tout l'axe 0 de [`MESURE.md`](MESURE.md) : plafonds d'oracle
stratifiés contre voxels, superpoints et HDBSCAN ; pureté des nœuds à nombre
d'unités égal ; stabilité sous décimation avec ses quatre témoins ; sonde
XGBoost et ses ablations ; statistiques de recouvrement et d'adaptativité.

**Et surtout : le témoin T2.** Donner $\hat f_K(x)$ comme variable par point à
un PTv3 inchangé. Il ne demande qu'un canal d'entrée supplémentaire et une
recette existante. S'il capte l'essentiel du gain que l'on espère, il vaut
mieux le savoir maintenant.

**Décision.** Si le plafond d'oracle n'est pas meilleur que celui des
superpoints à nombre d'unités égal, et si l'adaptativité du rayon avec la
portée n'est pas visible, la thèse de l'échelle canonique est fausse et la
phase 1 n'a pas lieu d'être.

## Phase 1 — HGP-UNet supervisé, et la substitution de l'échelle

**Construire.** HGP-UNet **dans** la base PTv3 : `GridPool` remplacé par
`FiltrationPool`, décodeur remplacé par PUR. Rien d'autre ne bouge.

**Mesurer.** Les bras S1, S2 et S6 de l'étude de substitution ; les quatre
règles d'échelle ; les trois chemins ; le témoin T1 (tour brouillée) et T3
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

**Construire.** Les cinq tâches de modélisation de filtration, le masquage par
nœuds entiers, l'auto-distillation avec décimation en portée, retrait d'anneaux
et occultation.

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
32 et retour, **sans réaccorder aucune constante** — puisqu'il n'y en a pas.
Puis le transfert vers l'intérieur, terrain d'Utonia. Puis les lois d'échelle,
trois tailles de modèle par trois tailles de corpus, avec la tendance de
l'écart publiée quelle qu'elle soit.

**Ce que la phase produit.** La revendication de fondation, ou sa
requalification honnête en efficacité d'échantillons et transfert.

## Phase 5 — les capacités propres

Panoptique contre ALPINE, propositions d'instance sans apprentissage,
anomalies guidées par un modèle 3D, cohérence multi-échelle garantie. Ces
tâches ont une valeur propre et restent publiables même si les phases 3 et 4
déçoivent.

## Ce qui peut être fait en parallèle, sans attendre

- la rédaction mathématique du lien entre la construction de Gabriel du
  manuscrit et la définition degré-Rips, qui conditionne l'argument de
  stabilité de [`ETAT_DE_LART.md`](ETAT_DE_LART.md) § 3 ;
- la reproduction de la base de référence SemanticKITTI, qui conditionne toute
  comparaison chiffrée ;
- la recherche d'antériorité exhaustive, à faire une fois, sérieusement.

## Vue d'ensemble

| phase | apprentissage | ce qui la termine | ce qui l'annule |
| --- | --- | --- | --- |
| 0 raccord et sondes | non | échelle construite, axe 0 publié | plafond d'oracle sous les superpoints ; T2 capte tout |
| 1 échelle | supervisé | S1, S2, S6 mesurés, T1 et T3 passés | T1 égale la tour |
| 2 voisinage et ordres | supervisé | S3, S4, S5 mesurés, T4 passé | S5 nul, donc OM retiré |
| 3 pré-entraînement | SSL | sonde linéaire et efficacité en étiquettes | filtration sans effet contre auto-distillation seule |
| 4 transfert et échelle | SSL | transfert inter-capteurs, lois d'échelle | écart qui s'inverse à l'échelle |
| 5 capacités propres | mixte | panoptique, anomalie, instance | ALPINE non battu |
