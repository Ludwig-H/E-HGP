# Audit mathématique courant — hiérarchie et clustering plat

4 octobre 2026. Audit depuis les fondations, **contrelecture complète e02a6c235** ;
démos **8f68622b2** relues ensuite, moteur inchangé ;
recherche privée `build/v11-points-select/` figée par les reçus ci-dessous.
Cadre : `exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`.
Cette note est mise à jour en place. Les détails des anciens échanges sont
[dans les reçus](../receipts/audit_dialogues_20261004/README.md), sans journal supplémentaire.

## Résultats utiles pour la tête plate

**Le critère B se calcule par une rencontre et sa date est stable en 3ε.**
Pour H_i=(o_i,e_i) et core_i=(c_i,d_k(x_i)), self compris :
`sB_i=max(e_i,d_k(x_i),naissance(LCA(o_i,c_i)))`. Un LCA par point
remplace le balayage de toutes les coupes du prototype. La preuve combine
l'alignement fort H3 et le segment entre points appariés, couvert au rayon
d_k+2ε ; k/m/IDs fixes, nuages finis, contacts fermés. **156 dates B**
concordent avec le code privé extrait par AST. Cela ne restaure pas les
points frontière perdus par B, ne stabilise pas une antichaîne EOM à égalité
et ne prouve pas une meilleure pertinence statistique.
[Preuve et 12 968 gardes exactes, normal/−O](../receipts/audit_giant_20261004/mathematics/README.md).

**B garde également le plafond L6 d'H.** Poser ρ=d_k(x), D=e−t′.
Pour le premier propriétaire qualifié p=(C,t′), choisir y∈C,
ℓ=|x−y|≤t′. Les k points de sa boule donnent ρ≤t′+ℓ. Le segment des centres
est couvert dans L_k au rayon S=max(t′,(ρ+t′+ℓ)/2), avec
ρ≤S≤t′+ρ/2. Il relie p au core, même si ce dernier n'est pas qualifié.
Ainsi **sB≤t′+ρ/2** et **0≤sB−e≤ρ/2−D**. Nuages finis, premier
propriétaire atteint, k/m fixes, sites unitaires et contacts fermés : mêmes
hypothèses que la preuve B3. Cela évite de cumuler deux plafonds pessimistes.

Ce supplément peut pourtant approcher ρ/2 : triangle
x=(0,0), y=(2N,1), z=(2N,−1), k2/m3, e=t′=(4N²+1)/(4N), sB=ρ=√(4N²+1).
La limite concerne une famille géométrique non bornée, pas une constante
optimale sur le domaine fini u21. B peut encore perdre une branche frontière ;
aucune optimalité statistique de B n'en découle.
[Preuve adverse, huit couples nuage/ordre et 931 gardes](../receipts/audit_deep_20261004/math/README.md).

La revue FULL détaille MEB/supports, Γ↔Lk, morceaux stricts, descentes,
multifusions et verticales. Sur huit nouveaux nuages : **32 ordres et
1 082 coupes ouvertes/fermées**, contrôle de la couverture complète de
chaque composante, parents/verticales/core compris. Aucun nouveau défaut
mathématique FULL établi ; les théorèmes restent nécessaires au-delà du
domaine borné. [Chaînes de preuve et limites](../receipts/audit_giant_20261004/mathematics/PROOF.md).

**La condensation et le raffinement en z sont confirmés mathématiquement.**
À arbre et cohortes fixés, augmenter z dans λ=r^(-z) raffine la sélection EOM,
racine d'admissibilité fixe, parent aux égalités certifiées. La preuve traite
le masquage des décisions par les ancêtres ; 728 comparaisons exactes sur
26 condensations concordent. Le raffinement porte sur les clusters sélectionnés ; agréger tous les
points bruit en un bloc ne transfère pas ce théorème à cette partition.
Cela ne compare ni deux modèles d'existence,
ni leurs IoU, et ne prouve pas l'optimalité statistique de z=1 ou z=3.
Le rapport modèle propose z=3 primaire ; le rapport LiDAR propose z=1.
Ce sont encore des propositions à arbitrer, pas une décision du développeur.
[Preuve indépendante et 12 917 gardes](../receipts/flat_model_followup_20261004/README.md).

Pour des dates de comptage s_i≥e_i, R_i est la mcs-ième valeur de
max(u_ij,s_j). Les blocs admissibles sont exactement ceux du facteur
**max(u_ij,R_i,R_j)**. Mais un point ne contribue au score qu'à
max(R_i,s_i) : transporter son calendrier et ses cohortes. Une condensation
standard du facteur ne conserve les scores automatiquement que pour A,
s_i=e_i. Témoin géométrique à six sites, calendrier différé admissible :
score 1/5, contre 3/10 si chacun est compté dès R_i. Ce calendrier n'est
pas présenté comme la valeur B/C/E de ce nuage. Pour les poids fixes, le
rang devient un seuil de masse cumulée.

**Une égalité EOM peut être certifiée sans la supposer au budget.**
Pour e=√t+√M−√q>0, poser d=t+M−q, Δ=d²−4tM. Si Δ≠0 :

```
1/e = [(t−M−q)√t + (M−t−q)√M + d√q − 2√(tMq)] / Δ.
```

Si Δ=0 et e>0, e=2√min(t,M) ; les dates nulles exigent leur politique propre.
La réciproque et son cube utilisent au plus quatre classes carrées **pour
cette forme H**, et pour B/C qui choisissent H ou une date core. Ce domaine
ne couvre pas une maturité générale Eθ : une combinaison de quatre radicaux
indépendants exige huit classes dans le témoin algébrique relu, sans nuage
géométrique correspondant revendiqué. [Garde de portée](../receipts/audit_deep_20261004/math/README.md).
Le regroupement des classes certifie un score nul ; le signe non nul demande
encore intervalles, budget et refus. Témoin HGP entier plongé par x↦(x,x,0) :
entrées 6√2, fusion A/B à 8√2, racine à 12√2, scores parent/enfants **√2/8**.
La tête privée par intervalles choisit le bon parent mais signale un choix
forcé à 128 bits. Ce n'est pas une erreur de labels ; la garde aide à qualifier
la vraie égalité. **888 gardes normal/−O**, mutation du facteur 2 tuée.
[Formule, cas singulier et helper autonome](../receipts/eom_exact_audit_20261004/README.md).
Aucune borne rapide/native ne découle de ce regroupement naïf.

**Trois gardes préviennent des raccourcis futurs, sans défaut produit établi.**
La croix de quatre sites à k3 naît avec population4>k : population=k est
suffisante pour une naissance, pas nécessaire. Pour X={0,2,4,6}, k2/m4,
la qualification arrive à2 alors que la première couverture d'ordre4 est3 :
l'identité t′=α_(k+1) ne se généralise pas à α_m. Enfin la naissance de la
racine ne borne pas toutes les attaches/core : un témoin a root birth²16,
qualification²25 et core²80. Conserver ces niveaux dans l'arbre compact.
[Rejeux exacts et hypothèses](../receipts/audit_deep_20261004/math/README.md).

## Contrats à fixer avant intégration

Conserver comme bras explicite : masses entières des sites engagés, mcs≥2,
plateaux N-aires exacts, antichaîne globale, racine exclue, parent sur égalité
certifiée, non-affectés en bruit. Déclarer z=1/z=3, feuilles, calendrier
résolu et complétion ; tester sur les mêmes scènes figées. Les masses de
faces fractionnaires de la thèse changent le modèle et perdent T0 à mcs=3.

- La **complétion privée** suit les premières branches qualifiées, pas
  nécessairement le propriétaire P1 retenu après retard. Elle peut attribuer
  un point à un enfant avant son entrée H. C'est une politique plate distincte
  à nommer ; aucun défaut de partition n'est établi.
- L'option privée de racine `asc=True` peut rendre deux points pour mcs=3 ;
  déclarer cette exception ou protéger n<mcs. Le défaut excluant la racine
  rend bien le bruit. [Huit gardes AST](../receipts/flat_contract_followup_20261004/README.md).
- Pour mcs≥2, e_i≤u_ij permet d'ignorer les naissances singleton ; construire
  néanmoins le véritable arbre de points, avec les niveaux créés par les
  attaches. Après condensation, une petite branche peut sortir avant e_i^(-z).
  [Contrats](../receipts/flat_selection_contract_20261004/README.md),
  [précision sur les sorties condensées](../receipts/flat_selection_math_r2_20261004/README.md).
- L'ordre exact des dates ne qualifie pas les sommes EOM. Le prototype
  flottant a un mauvais gagnant scalaire ; celui à intervalles force parfois
  le parent. Distinguer égalité prouvée, signe séparé et arbitrage/refus au
  budget. Le pilote déjà capturé avait zéro choix forcé.

La stabilité 3ε de H ne garantit pas la stabilité de la partition à une
égalité EOM. À arbre/cohortes correspondants et rayons≥r0, une marge de score
supérieure à sa borne d'erreur garantit le choix. Les neuf sites
A={0,2,4}, B={7,9,11}, D={17,19,21} donnent A∪B|D à z=1 et A|B|D à z=3 ;
une petite perturbation près d'un ex æquo peut aussi inverser le gagnant.
[Preuve exacte et garde de marge](../receipts/flat_selection_math_20261004/README.md),
[correction de portée](../receipts/flat_selection_math_r2_20261004/README.md).

## Comparaison à HDBSCAN et métriques

Le rapport d'équité a adopté **R0 officiel intact** et un bras distinct à
sélection N-aire commune. C'est le bon périmètre ; les versions locales
1.9.1/NumPy2.5.3 et G4 1.7.2 restent à qualifier sur le même banc.
520/2 400 partitions diffèrent à epsilon=0 après atomisation ; le total
1 015/6 752 mélange sorties officielles et secours après exceptions.
La comparaison de sources 1.7.2/1.9.1 n'est pas une nouvelle porte G4.

**Corriger M3 du rapport de mesure : B(H) plafonne uniquement des clusters
qui sont des blocs de cet H.** Sur leur fixture F2, R0 produit {6,9}, union
binaire qui n'est pas un bloc du plateau atomique. En prenant ses trois
clusters comme cible : score plat 1, plafond atomique 5/6 (7/9 sans
singletons). Ce n'est pas une anomalie de score observé : les univers diffèrent.
Réserver le plafond atomique aux bras N-aires, ou annoncer un plafond R0
sur son propre arbre binaire. [Témoin et 39 contrôles](../receipts/flat_evidence_followup_20261004/README.md).

Le meilleur bloc par objet peut sélectionner parent et enfant simultanément :
ce n'est pas une partition. Mesurer une antichaîne et un appariement global
un à un ; meilleur-IoU reste un diagnostic. Le vérificateur hongrois contrôle
sa somme et une borne lâche, sans certificat dual d'optimalité ; fournir ce
certificat ou réduire la revendication. Aucun score réellement faux n'est
imputé à cette absence. [Archives et gardes](../receipts/flat_selection_evidence_20261004/README.md).
Les plans 432 scènes/6–9 sessions sont des hypothèses, non des mesures.
Le nouveau lot de membres fournit un cas réellement compatible avec une
antichaîne : deux vélos, k5, blocs HGP133/83 disjoints, IoU0,964/0,711.
À k10, leurs meilleurs blocs sont imbriqués. Ce témoin aide la future tête ;
il ne démontre ni sélection EOM, ni coupe commune, ni score plat déjà obtenu.
[JSON clos et vérification des intersections](../receipts/audit_deep_20261004/math/README.md).

## Socle FULL → points : acquis et limites

Règle étudiée à k≥2 fixé : P1 après qualification m=k+1, en rayon, coupes
fermées ; e=t′+sup_q(m(p,q)−h(q)), propriétaire vivant au plateau exact.
[Q1–Q8, preuves détaillées](../receipts/points_answers_20261003/README.md),
[réponse courante du développeur](REPONSE_CLAUDE_POINTS_20261003.md).

| Question | Résultat actuel et portée |
|---|---|
| Stabilité | Identifiants appariés, poids positifs fixes, k/m fixes, nuages finis : interleaving FULL ε, dates/réunions 3ε ; Pκ : (1+2κ)ε. Aucune garantie d'IoU/labels. |
| Optimalité | P1 optimal dans le domaine abstrait intrinsèque. Constante minimale 3 sur l'image géométrique non démontrée. |
| Retard | 0≤e−t′≤d_k/2, t′≤d_(k+1). Pour entrer strictement avant F, exiger t′+d_k/2<F ; ≤ ne vaut qu'au plateau fermé. |
| Plusieurs k | Six sites 0,3,6,18,19,20 : branches qualifiées k2/k3 croisées à r=7. Pas de laminarité commune automatique. |
| Insertion | L'impossibilité générale Q6 sous trois axiomes est fausse. En revanche, Hausdorff ou W_p normalisé fini ne contrôlent pas uniformément les dates SUP avec seuils unitaires ; W∞/erreur pondérée non tranchés. |
| Modèle infini | Palm donne un déficit conditionnel aux mêmes λ/F, pas une convergence des fenêtres. Localement fini ne garantit pas une rencontre atteinte. |
| Port natif | PointRadiusDate et scores exacts restent à porter/qualifier. L'export FULL C++ + projection Python n'est pas ce module. |

[Insertion/Wasserstein](../receipts/measure_metric_20261004/README.md),
[Palm conditionnel](../receipts/palm_obstruction_20261003/README.md),
[rencontres infinies et κ](../receipts/points_math_followup_20261004/README.md).
κ augmente le rappel dans une même composante FULL ; ni IoU ni partition ne
sont monotones. ER0h n'a pas de constante uniforme pour ses dates en rayon.
Le défaut Decimal de propriétaire au plateau est corrigé et exercé par F.
Le domaine m>n refuse actuellement : déclarer m≤n ou des points inactifs.
[État natif, budgets et preuves G4](AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md).
