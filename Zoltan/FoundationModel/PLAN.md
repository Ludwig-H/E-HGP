# Plan d'exécution

26 septembre 2026. Dans quel ordre construire, ce que chaque phase produit, et
à quel moment décider. Les mesures sont définies dans [`MESURE.md`](MESURE.md).

Principe d'ordonnancement : **ce qui peut annuler le projet se fait en premier
et coûte le moins cher.** Les deux témoins les plus tranchants — le canal de
densité seul, et la tour brouillée — ne demandent aucun développement
d'architecture.

## Phase 0 — le raccord et les sondes avant HGP-UNet

**Construire.**

1. Un export **coverage_v1** : états FULL datés, cartes à la coupe, unions
   de sites, permutation géométrie→site et table site→retours. Exercer les
   coupes avant/au/après événement, notamment la continuation growth_ABCZ.
   Cet export permet aussi de compiler les requêtes K1 d'un **GuidanceBundle** :
   paires de retours, rayons, réponses enseignantes et censure. Il ouvre
   le pilote enseignant seul sans attendre les poids de K supérieur.
2. Le supplément **weighted_gabriel_v1** : fixer cofaces C, frontières F,
   ψ et horizon ; comparer l'accumulation par flux d'incidences à un petit
   oracle explicite. Résoudre chaque facette à sa naissance propre. Mesurer
   coûts et volumes avant toute matérialisation globale. Le routage dur de
   l'ancien réducteur ne sert pas d'oracle de la matrice douce.
3. Le **constructeur d'échelle** : commencer à K1 en coupes globales, puis
   branches K autonomes à univers/poids gelés, avec réserves persistantes.
   Vérifier composition et moyennes pondérées. Ajouter ensuite condensation
   relative et les quatre règles d'échelle ; conserver les sorties par
   incidence. Les changements de K exigent deux contrôles distincts,
   géométrique et pondéré ; OM reste latéral par défaut.
4. Le **cache CutBundle** : coupes, masses et provenance de chaque vue,
   dimensionnées avant le corpus complet ; conserver les tours/reçus
   scientifiques séparément. Chaque vue augmentée est re-quantifiée et sa tour
   recalculée.

Le [contrat détaillé](CONTRAT_COUPES_ET_MASSES_20260926.md) rend cet ordre
implémentable. Les [11 fixtures rationnelles](receipts/cut_algebra_20260926/README.md)
vérifient l'algèbre ; elles ne ferment pas les portes de l'export natif.

Le [contrat de guidage](GUIDAGE_FULL_ET_PREENTRAINEMENT_20260926.md) ouvre une
voie complémentaire : FULL enseignant dans la perte, avec encodeur ordinaire
à l'inférence. Ses fixtures testent suppression de ponts, censure, masse
commune et lissage par transport doux. L'export K1 suffit à préparer ce bras ;
les portes des matrices pondérées restent nécessaires au bras HGP-UNet.

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
la tour sous rotation pure et son effet sur les affectations consommées ;
**0.8** quels ordres $K$
servent et à quel $K$ casse un pont de bruit ; **0.9** plafond d'oracle
**stratifié par contact avec le sol**, avec et sans retrait du sol ; **0.10**
nombre de naissances d'une trame relevée en dimension 6, avant de croire au
relèvement métrique. Les portes 0.8 et 0.9 qui exploitent les étiquettes
restent définies ici et seront exécutées avec l'évaluation correspondante.

Les onze **portes du tokenizer** de [`SPECIFICATION.md`](SPECIFICATION.md) § 9
doivent être fermées dans cette phase, notamment couverture des retours, composition des coupes,
naturalité entre K, budget et traçabilité.

**Décision.** Si l'export des incidences, la couverture des retours ou les
cartes entre coupes échouent, corriger ce raccord avant la phase 1. Les sondes de pureté,
d'adaptativité et de stabilité restent des diagnostics de la valeur du
tokenizer, et non une preuve de valeur apprise. Si la dérive de la porte 0.7
dégrade les opérateurs utiles, comparer les précisions avec leur coût.
Une modification combinatoire de FULL ne suffit pas à conclure à une perte
de performance du réseau.

## Phase 1 — HGP-UNet supervisé, et la substitution de l'échelle

**Construire.** La référence PTv3 et T2, puis les substitutions FP/PUR dont
les interfaces sont vérifiées. Séparer l'effet de la coupe, de l'agrégation
et du retour aux points par les bras S1/S2/S6 ; garder la connexion fine.

**Mesurer.** D'abord T2 (canal de densité seul) à réseau inchangé, puis les
bras S1, S2 et S6 de l'étude de substitution ; les quatre
règles d'échelle ; les trois chemins emboîtants, avec l'iso-densité
comme lecture latérale ; le témoin T1 (tour brouillée) et T3
(niveaux permutés). Stratification par portée, par taille d'objet et par
classe. SemanticKITTI val 08, trois graines.

**Ce que la phase produit.** Une mesure de l'effet du pooling HGP, stratifiée
par portée et à budget apparié. L'intérêt scientifique dépendra du résultat,
y compris s'il est nul ; cette comparaison ne dépend pas d'un
pré-entraînement HGP.

**Décision.** Si S1 ne paie pas mais que la stratification par portée montre
quand même l'adaptativité attendue, chercher du côté du voisinage (phase 2)
avant de conclure.

## Phase 2 — le voisinage, la position et les ordres

**Construire.** Les *patches* du graphe de fusion (S3), le biais ultramétrique
(S4), le mixage d'ordres dans ses trois réalisations par coût croissant (S5).

**Mesurer.** S3, S4, S5 en effet propre et en effet marginal ; le témoin T4
(ordre aléatoire) ; l'ablation du canal métrique et les transformations
d'échelle, qui mesurent le comportement appris du réseau.

**Ce que la phase produit.** La réponse à la question la plus spécifique du
projet : **l'axe des ordres vaut-il quelque chose ?** C'est une hypothèse
propre à l'usage de la tour, à simplifier si elle ne se paie pas.

## Phase 3 — pré-entraînement

**Construire.** D'abord le bras **A0G1**, encodeur ordinaire et une seule
perte de relations FULL K1, contre A0G0 (SSL de référence). Ce pilote peut
commencer dès que coverage_v1 est prêt, en parallèle de FP/PUR.
Conserver le mécanisme de diversité de la recette de référence ; recomposer
entièrement l'entrée élève depuis sa vue. Comparer ensuite A1G0/A1G1 pour
mesurer l'interaction avec le tokenizer HGP.

Ajouter séparément K supérieur, FM-2/3, accord régional sur IDs communs et
masques structurels, selon le [contrat](GUIDAGE_FULL_ET_PREENTRAINEMENT_20260926.md).
**FM-6 reste une extension temporelle secondaire** : elle demande un accès
identique aux trames/poses dans ses comparaisons et ne fait pas partie du
régime primaire mono-scan sans historique.

**Mesurer.** Sonde linéaire, réglage fin, efficacité en étiquettes à
$0{,}1 / 1 / 10 / 50 / 100\,\%$. Le témoin T5, diagnostic du raccourci
géométrique. L'ablation décisive : **le même pré-entraînement sans les tâches
de filtration**, auto-distillation seule. Si l'écart est nul, aucun apport
de ce prétexte n'est établi dans le régime mesuré ; l'effet architectural
reste une question distincte.

**Prédiction à vérifier.** P3 : le gain croîtrait quand les étiquettes se
raréfient. Une autre tendance impose de revoir ce mécanisme proposé, sans
invalider un gain mesuré à budget apparié.

**Ce que la phase produit.** Une mesure de la qualité des représentations et
du rôle de FULL. Comparer aux recettes pertinentes, notamment DOS pour le
LiDAR, à données/modalités appariées ; aucune revendication de fondation ne
découle de la seule baisse de la perte structurelle.

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
