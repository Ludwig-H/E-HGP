# Verdict de reviewer exigeant

## Réponse courte

Non : même le couplage correctement formulé `support normalisé + objet HGP complet + HSA` ne donne aujourd'hui aucune raison sérieuse de prévoir un état de l'art SemanticKITTI. En revanche, cette idée est substantiellement plus forte et plus cohérente que `support + rayon` : un objet marqué complet peut déterminer un carrier non convexe, conserver les incidences, les régions de multicoverture et les niveaux que le support perd. Les risques réels sont désormais son coût, sa stabilité sous l'échantillonnage LiDAR, sa redondance possible avec un backbone fort, l'absence actuelle du payload amont et l'adéquation de HSA à cette structure.

Oui : le projet peut devenir un bon papier si l'hypothèse est testée causalement, si l'encodeur polyédral conserve effectivement l'information d'incidence et si une contribution générale survit aux contrôles. Le résultat crédible serait un modèle local fort auquel le complexe HGP apporte un contexte non convexe mesurable, avec une attention adaptée ou une correction de portée justifiée. Le score final serait une validation, pas la contribution à lui seul.

## Décision avec le dossier actuel

**Décision simulée : rejet, confiance élevée.** Aucun modèle appris ni résultat n'est encore rapporté. Même en supposant une implémentation correcte, un reviewer demanderait :

- quelle information HGP ajoute à un octree, RSL/HDBSCAN ou des superpoints ;
- quelle information l'encodeur du complexe complet ajoute aux mêmes points, au seul graphe $\Gamma^{K}$, aux réseaux simpliciaux et à Deep Sets ;
- pourquoi la densité observée correspond aux classes plutôt qu'à la portée et à l'occultation ;
- pourquoi HSA bat un simple passage bottom-up/top-down au même budget ;
- comment les $K$-polyèdres chevauchants deviennent une forêt laminaire sans perdre leur avantage ;
- si la latence inclut HGP, descripteurs, réseau et reprojection ;
- si le gain se reproduit sur un second capteur ou dataset.

La critique précédente `support + rayon` ne visait pas fidèlement la proposition. Le second canal demandé est l'objet HGP complet, pas son maximum radial ; il peut donc conserver la non-convexité. Trois objets doivent être nommés : le $K$-polyèdre source, qui est une union d'observations ; le porteur PL des facettes, dont le support égale celui de ces observations ; et le porteur continu de multicoverture, composante canonique du niveau de densité, dont le support peut être différent. Le papier doit définir exactement lequel est encodé et comment facettes, cofaces, incidences et filtration permettent de le reconstruire. La v3 courante ne livre pas encore ce payload complet sur sa route réduite. Voir [la branche polyédrale](POLYHEDRAL_COMPLEX_BRANCH.md).

## Cible SOTA auditée au 13 août 2026

Il n'existe pas de seuil strict unique et certifié. RAPiD-Seg est le candidat publié mono-trame le plus élevé à 76,1 test, mais utilise deux inférences apprises successives et ne documente pas explicitement l'absence de TTA/ensemble. SphereFormer à 74,8 reste un repère publié, mais son statut TTA est non rapporté dans la veille actuelle et ne permet pas d'en faire un repère strict certifié. SP2T à 75,4 et LSK3DNet à 75,6 emploient des augmentations au test ; TASeg à 76,5 est temporel et multimodal. Le CodaBench courant affiche 75,2 pour un compte pseudonyme sans fiche méthode, donc non attribuable scientifiquement.

Un objectif prudent est de dépasser 76,1 avec un protocole strict, reproductible et publié, ou de démontrer un meilleur Pareto qualité/coût avec incertitude excluant zéro. Le protocole RAPiD restant partiellement `NR`, même ce dépassement demanderait un audit comparatif. Ces nombres sont des instantanés à réauditer avant soumission, pas des objectifs à optimiser sur la séquence 08.

Les sources primaires, exclusions et colonnes de comparabilité sont détaillées dans [STATE_OF_THE_ART.md](STATE_OF_THE_ART.md) et [REFERENCES.md](REFERENCES.md).

## Ce que le papier HSA apporte réellement

HSA fournit une contrainte de blocs induite par un arbre, une projection reverse-KL vers une attention plate définie sous ses propres choix d'énergie, et un calcul structuré. Il ne prouve ni que l'arbre HGP est pertinent, ni que l'attention résultante approche une attention 3D arbitraire, ni que les frontières point-wise sont préservées. Ses validations publiées portent sur d'autres domaines, pas sur SemanticKITTI.

La première obligation est donc une reproduction fidèle sur petits arbres : même Q/K, normalisation, température, rescaling, masque et positions. Les tests doivent comparer HSA à sa cible plate, rejeter les mutants d'indices/normalisation et isoler strictement les scans d'un batch. Une racine factice commune ne doit jamais créer d'attention inter-scan.

`QC-HSA` est une relaxation raisonnable pour conserver des requêtes point-wise, mais sa projection fermée seule est trop proche de précédents multi-échelles pour porter un papier généraliste. Elle devient intéressante si un raffinement adaptatif fournit un certificat calculable fidélité–coût, ou si l'arbre HGP donne une borne non vacue sur les oscillations intra-bloc.

## Hypothèse révisée défendable

> À backbone local, budget et protocole fixes, la combinaison d'un support convexe global et d'un encodeur de l'objet HGP marqué, muni d'un carrier potentiellement non convexe, fournit-elle un contexte multi-échelle plus utile et plus stable que des hiérarchies et complexes de contrôle, après correction explicite du processus d'échantillonnage LiDAR ?

Le modèle minimal correspondant garde les points ou micro-voxels comme feuilles et une voie locale forte pour les frontières. Une branche d'incidence encode points, facettes, cofaces et niveaux sans les moyenner immédiatement ; le support fournit un chemin global court ; des side channels métriques et capteur conservent l'échelle. HSA fidèle est une baseline inter-branches, distincte de l'encodeur intra-complexe ; pooling et message passing restent des contrôles.

## Questions qui doivent recevoir une réponse falsifiable

1. Le canal vise-t-il l'union d'observations, le porteur PL des facettes, le porteur continu de multicoverture ou leur certificat commun ?
2. Facettes, cofaces, incidences, coordonnées, niveaux et plateaux suffisent-ils à reconstruire exactement cet objet à réindexage près ?
3. Pour $K\geq2$, comment les recouvrements ponctuels sont-ils conservés dans la branche d'incidence puis projetés vers la forêt HSA sans double comptage ?
4. Les niveaux HGP restent-ils stables quand un même patch est transporté puis rééchantillonné selon la portée ?
5. L'arbre HGP améliore-t-il pureté, oracle de coupe et frontières face à RSL, octree, superpoints et arbre aléatoire à compression égale ?
6. L'encodeur distingue-t-il mêmes points et même support avec incidences différentes, puis bat-il points seuls, graphe $\Gamma^{K}$ seul et réseaux simpliciaux de même budget ?
7. Le gain vient-il de l'arbre, de la représentation ou de l'opérateur dans une ablation factorisée ?
8. HSA/QC-HSA améliore-t-il les points proches des frontières sans dégrader les classes rares ?
9. Le coût reste-t-il utile pour les arbres réels, y compris degrés, profondeur, condensation et $C_T$ ?
10. Le gain persiste-t-il avec au moins trois seeds appariées et un intervalle à 95 % ?
11. La méthode reste-t-elle compétitive sans TTA, ensemble, historique, RGB, pseudo-labels externes ou données externes ?
12. Le mécanisme se reproduit-il sur un second capteur/dataset ?

## Barre de preuve par type de soumission

Une venue 3D forte peut accepter une intégration bien isolée, des gains reproductibles, une analyse de portée et un coût complet, même sans être numéro un absolu. CVPR exige en pratique une méthode 3D et une évaluation nettement convaincantes. ICML/NeurIPS demandent en plus une contribution générale — théorème, opérateur, stabilité ou représentation — et une validation qui dépasse un benchmark unique.

Être premier sur le test caché n'est donc ni garanti, ni strictement nécessaire, ni suffisant. La barre raisonnable est : gain validation causal face à un backbone fort, Pareto qualité/coût crédible, absence de fuite, puis une seule soumission au test après gel. Les comparaisons doivent séparer le track LiDAR mono-scan strict des méthodes temporelles, multimodales, préentraînées, TTA ou en ensemble.

## Conditions de poursuite

Continuer vers un modèle complet seulement si :

- HGP bat au moins une hiérarchie de contrôle forte avec agrégateur simple ;
- l'effet ne disparaît pas sous stratification par portée et densité ;
- le complexe complet est sérialisé sans ambiguïté et son encodeur bat les mêmes points, le graphe $\Gamma^{K}$ seul et des contrôles simpliciaux à budget comparable ;
- `support + complexe` améliore le Pareto face à `complexe seul`, sinon le support est retiré sans remettre en cause la branche non convexe ;
- HSA ou son successeur bat pooling/message passing sur le même arbre ;
- les contraintes de mémoire permettent des batches et trois seeds réalistes.

Sinon, publier le résultat négatif le plus informatif ou pivoter : étude de stabilité des arbres de densité LiDAR, descripteur topologique borné, ou benchmark causal de hiérarchies. Ne pas protéger l'histoire initiale en changeant simultanément backbone, arbre, descripteur et recette.

## Verdict final honnête

Le couplage corrigé est une hypothèse compétitive crédible, mais il n'existe aujourd'hui aucune base honnête pour promettre le SOTA. Le chemin le plus solide est : « nous avons défini et encodé sans ambiguïté l'objet HGP marqué et son carrier non convexe déclaré, isolé leur apport au-delà du support et des mêmes points, puis montré quand leur couplage hiérarchique résiste au changement de portée et de capteur ». Si la sérialisation, l'effet causal de l'objet ou le gain de l'opérateur échoue, le papier de niveau visé n'est pas prêt, même avec un score favorable isolé.
