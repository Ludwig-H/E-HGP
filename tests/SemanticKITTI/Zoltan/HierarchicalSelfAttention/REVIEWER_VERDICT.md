# Verdict de reviewer exigeant

## Réponse courte

Non : coupler aujourd'hui HGP-Clusterer, un descripteur support/radial et HSA ne donne aucune raison sérieuse de prévoir un état de l'art SemanticKITTI. L'idée initiale a une probabilité faible de suffire. Elle assemble deux objets intéressants, mais le descripteur perd encore de la géométrie, HGP peut surtout suivre le processus d'échantillonnage du capteur, et HSA n'a pas démontré son intérêt pour la segmentation dense 3D.

Oui : le projet peut devenir un bon papier si l'hypothèse est réduite à une question causale, si les échecs sont établis avant l'entraînement lourd et si une contribution générale survit aux contrôles. Le résultat crédible serait un modèle local fort auquel HGP apporte un contexte mesurable, avec une attention adaptative ou une correction de portée justifiée. Le score final serait une validation, pas la contribution à lui seul.

## Décision avec le dossier actuel

**Décision simulée : rejet, confiance élevée.** Aucun modèle appris ni résultat n'est encore rapporté. Même en supposant une implémentation correcte, un reviewer demanderait :

- quelle information HGP ajoute à un octree, RSL/HDBSCAN ou des superpoints ;
- quelle information le descripteur ajoute à Deep Sets, CDF projetées ou ECT/WECT ;
- pourquoi la densité observée correspond aux classes plutôt qu'à la portée et à l'occultation ;
- pourquoi HSA bat un simple passage bottom-up/top-down au même budget ;
- comment les $K$-polyèdres chevauchants deviennent une forêt laminaire sans perdre leur avantage ;
- si la latence inclut HGP, descripteurs, réseau et reprojection ;
- si le gain se reproduit sur un second capteur ou dataset.

Le point le plus dangereux est conceptuel : le maximum d'une projection est une fonction support et convexifie ; le maximum du rayon est une fonction radiale et remplit les trous dès que la forme n'est pas étoilée. Les combiner ne rend donc pas la représentation injective. Voir [l'audit géométrique](GEOMETRIC_DESCRIPTOR_AUDIT.md).

## Cible SOTA auditée au 13 août 2026

Il n'existe pas de seuil strict unique et certifié. RAPiD-Seg est le candidat publié mono-trame le plus élevé à 76,1 test, mais utilise deux inférences apprises successives et ne documente pas explicitement l'absence de TTA/ensemble. SphereFormer à 74,8 est le repère publié le plus défendable sans TTA apparent. SP2T à 75,4 et LSK3DNet à 75,6 emploient des augmentations au test ; TASeg à 76,5 est temporel et multimodal. Le CodaBench courant affiche 75,2 pour un compte pseudonyme sans fiche méthode, donc non attribuable scientifiquement.

Un objectif prudent est de dépasser 76,1 avec un protocole strict, reproductible et publié, ou de démontrer un meilleur Pareto qualité/coût avec incertitude excluant zéro. Le protocole RAPiD restant partiellement `NR`, même ce dépassement demanderait un audit comparatif. Ces nombres sont des instantanés à réauditer avant soumission, pas des objectifs à optimiser sur la séquence 08.

Les sources primaires, exclusions et colonnes de comparabilité sont détaillées dans [STATE_OF_THE_ART.md](STATE_OF_THE_ART.md) et [REFERENCES.md](REFERENCES.md).

## Ce que le papier HSA apporte réellement

HSA fournit une contrainte de blocs induite par un arbre, une projection reverse-KL vers une attention plate définie sous ses propres choix d'énergie, et un calcul structuré. Il ne prouve ni que l'arbre HGP est pertinent, ni que l'attention résultante approche une attention 3D arbitraire, ni que les frontières point-wise sont préservées. Ses validations publiées portent sur d'autres domaines, pas sur SemanticKITTI.

La première obligation est donc une reproduction fidèle sur petits arbres : même Q/K, normalisation, température, rescaling, masque et positions. Les tests doivent comparer HSA à sa cible plate, rejeter les mutants d'indices/normalisation et isoler strictement les scans d'un batch. Une racine factice commune ne doit jamais créer d'attention inter-scan.

`QC-HSA` est une relaxation raisonnable pour conserver des requêtes point-wise, mais sa projection fermée seule est trop proche de précédents multi-échelles pour porter un papier généraliste. Elle devient intéressante si un raffinement adaptatif fournit un certificat calculable fidélité–coût, ou si l'arbre HGP donne une borne non vacue sur les oscillations intra-bloc.

## Hypothèse révisée défendable

> À backbone local, budget et protocole fixes, une hiérarchie HGP indépendante des labels fournit-elle un contexte multi-échelle plus utile et plus stable que des hiérarchies de contrôle, après correction explicite du processus d'échantillonnage LiDAR ?

Le modèle minimal correspondant garde les points ou micro-voxels comme feuilles, une voie locale forte pour les frontières, des side channels métriques et capteur, puis peu de blocs hiérarchiques tardifs. HSA fidèle est une baseline ; pooling et message passing sont des contrôles ; l'opérateur gagnant n'est pas choisi à l'avance.

## Questions qui doivent recevoir une réponse falsifiable

1. Quel objet exact est sérialisé : sommets, simplexes actifs, niveaux de naissance, union plongée ou complexe abstrait ?
2. Pour $K\geq2$, quelle projection rend les recouvrements laminaires, et quelle information détruit-elle ?
3. Quelle fraction des réalisations est étoilée autour du centre choisi ? Combien de composantes radiales et de rayons vides observe-t-on ?
4. Les niveaux HGP restent-ils stables quand un même patch est transporté puis rééchantillonné selon la portée ?
5. L'arbre HGP améliore-t-il pureté, oracle de coupe et frontières face à RSL, octree, superpoints et arbre aléatoire à compression égale ?
6. Le descripteur distingue-t-il les collisions synthétiques et prédit-il la composition mieux qu'un encodeur de même budget ?
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
- un descripteur dont les collisions et la stabilité sont auditées bat les statistiques simples à budget comparable ;
- HSA ou son successeur bat pooling/message passing sur le même arbre ;
- les contraintes de mémoire permettent des batches et trois seeds réalistes.

Sinon, publier le résultat négatif le plus informatif ou pivoter : étude de stabilité des arbres de densité LiDAR, descripteur topologique borné, ou benchmark causal de hiérarchies. Ne pas protéger l'histoire initiale en changeant simultanément backbone, arbre, descripteur et recette.

## Verdict final honnête

Le couplage peut devenir un composant compétitif, mais il n'existe aujourd'hui aucune base honnête pour promettre le SOTA. Le chemin le plus solide n'est pas « HGP + HSA = meilleur score » ; c'est « nous avons identifié quand une hiérarchie de densité aide, prouvé ou certifié ce que l'opérateur conserve, et montré le même mécanisme sous changement de portée et de capteur ». Si ces trois éléments échouent, le papier de niveau visé n'est pas prêt, même avec un score favorable isolé.
