# Plan d'exécution

26 septembre 2026. Ordre révisé après le
[réaudit global](REAUDIT_GLOBAL_20260926.md). Le
[protocole de mesure](MESURE.md) fixe les comparaisons.
Ce document définit les travaux suivants ; cette revue n'a lancé aucun
apprentissage ni nouvelle mesure native.

## Principe — deux pilotes indépendants

**FULL enseignant** demande des requêtes structurelles correctes et un élève
sans fuite de cible. **FULL dans l'architecture** demande en plus des coupes,
masses, réserves et opérateurs composables. Le premier peut commencer dès
que son interface est prête ; les portes du second ne le bloquent pas.

La conformité de Morse HGP 3D v9 est posée. Les décisions portent sur sa
traduction en objets d'apprentissage et sur leur valeur pour le LiDAR.

## Phase 0A — couverture et compilation des cibles K1

1. Exporter **coverage_v1** : états datés, cartes à la coupe, unions de sites,
   permutation géométrie→site, table site→retours. Tester avant/au/après
   événement, dont la continuation growth_ABCZ.
2. Compiler un **GuidanceBundle** K1 : IDs de paires, rayons, connexité,
   horizon et censure, provenance de la vue enseignante.
3. Recalculer l'entrée depuis la seule vue élève. Modifier la partie cachée
   de l'enseignant à vue élève fixée doit laisser le forward élève inchangé.
4. Définir la loi de requêtes et le diagnostic conditionnel **Y_V=0**.
   Calculer sur les mêmes requêtes les références rayon seul, distance,
   statistiques locales et connexité/MST de la vue élève.
5. Dimensionner cache et banque de vues sur plusieurs scènes, avec coût
   de FULL, compilation, lecture, réutilisation et stockage séparés.

**Sortie attendue.** Contrat de couverture et de censure vérifié sur
l'export natif ; aucun poids de Gabriel requis pour ce premier pilote.

Les fixtures K1 existantes et les
[nouveaux contrôles](receipts/reaudit_global_20260926/README.md)
éprouvent des cas abstraits ; elles ne qualifient pas cet export.

## Phase 0B — coupes et matrices pour HGP-UNet

1. Définir **weighted_gabriel_v1** : cofaces, frontières, ψ, horizon,
   univers gelé et naissance propre des facettes. Comparer au petit oracle
   explicite, avec coût des incidences. Le réducteur dur historique ne
   vérifie pas le vote doux.
2. Commencer à K1 en coupes globales. Ajouter les branches K autonomes,
   réserves persistantes, conservation des masses et moyennes pondérées.
   La compatibilité géométrique entre K et celle des poids sont deux portes.
3. Comparer coupes brutes, condensation **α relative au parent**, seuil
   rapporté à une masse de référence gelée et ordre de contraction par
   persistance. α local filtre le déséquilibre ; il ne garantit pas la
   compression. N'en retenir aucun par principe avant mesure.
4. Exporter un **CutBundle** par vue et identifier la réalisation géométrique
   optionnelle : retours, union de tous les supports minimaux, ou surfels.
   Fixer quelles boules datées alimentent un snapshot et quelle mesure
   définit ses moments. Commencer avec les retours et statistiques simples.
5. Mesurer jetons, réserves, incidences et arêtes **sur toutes les branches**.
   Un ratio de réduction souhaité n'est pas une coupe garantie ; tracer
   explicitement les refus de domaine ou de budget.

**Portes.** Les onze portes de
[SPECIFICATION §9](SPECIFICATION.md#9-portes-du-tokenizer)
s'appliquent aux opérateurs activés. La naturalité inter-K devient nécessaire
lorsqu'une carte inter-K est consommée, pas pour le bras enseignant K1.
La réalisation par supports et ses 512 sondes restent une ablation optionnelle.

Les sondes sans étiquette mesurent couverture, compression et dérive sous
rotation/décimation. Pureté, oracle d'instance et XGBoost demandent des labels
ou un ajustement ; leur protocole est défini, leur exécution reste distincte.
Le relèvement en dimension 6 sort de la spécification Morse HGP 3D et n'est
pas un prérequis du pilote.

## Phase 1 — sémantique supervisée par substitutions

**Construire.** Référence PTv3 épinglée, témoin T2 fort (densité, rayons K-NN,
anisotropie, mêmes canaux d'acquisition), puis FP/PUR avec connexion fine.
Le décodeur PTv3 utilise une remontée par indices inverses et un skip.

**Mesurer.** S1/S2/S6, puis règles de coupe ; témoin T1 avec budgets,
couverture et quotients valides ; T3 avec son domaine déclaré. Trois graines,
validation 08, résultats par portée, classe, taille et contact avec le sol.
Comparer brut et non-sol avec raccord à tous les retours et branche contexte.

**Décision.** Un gain expliqué par T2 ou T1 limite l'apport propre dans ce
bras. Il n'annule ni le bras enseignant ni toutes les utilisations de FULL.
Publier aussi les comparaisons de systèmes complets et la frontière qualité/coût.

## Phase 2 — graphe, biais et ordres

**Construire.** Graphe réduit d'événements avec messages en deux passages,
puis biais relationnel et OM séparément. Une étoile à m branches coûte O(m)
incidences ; elle n'est pas équivalente à une attention complète entre frères.
Garder un canal local pour les détails et mesurer les sauts nécessaires.

**Mesurer.** S3/S4/S5, seuls et en interaction. Pour S4, même noyau
d'attention pour sans biais, XYZ et HGP ; publier séparément le prix de la
sortie du chemin FlashAttention. Comparer K distincts à **des branches K1
répétées** à budget total égal ; renommer K par une permutation fixe ne
constitue pas ce témoin. Ablater le canal métrique sans confondre changement
d'échelle et changement de capteur.

**Décision.** Conserver les branches et opérateurs qui améliorent la frontière
qualité/coût dans les régimes visés. Un bras K1 utile reste un résultat du
projet, même si OM n'apporte rien.

## Phase 3 — guidage du pré-entraînement

Cette phase peut partir de **0A**, en parallèle de 0B/1/2.

**Construire.** A0G0, recette SSL ordinaire ; A0G1, même recette avec une
perte FULL K1. Conserver les mécanismes de diversité de la référence.
Comparer une tête conditionnelle au rayon à une **CDF par paire/vue** :
une distribution de rayons de fusion, plus une queue au-delà de l'horizon,
dont le rayon demandé choisit seulement le cumul.

**Mesurer.** Gain au-delà des témoins rayon seul/encodeur constant et géométrie
locale ; Brier/calibration sur une loi de requêtes commune ; performance
conditionnelle sur Y_V=0 ; variance/rang des représentations. Puis sonde
linéaire et réglage fin. Une bonne perte structurelle ne suffit pas.

Déclarer l'unité d'annotation : pourcentages de scans ou de points sont des
protocoles différents. Geler listes de scans et blocs temporels ; exclure
validation 08/test du corpus SSL primaire, des enseignants, caches et
statistiques apprises sur le corpus. Les statistiques par vue d'une règle
gelée restent disponibles à l'inférence.

Ajouter ensuite A1G0/A1G1 pour l'interaction avec le tokenizer, puis K
supérieur, persistance et accord régional. À K supérieur, Γ est une masse
de coaffectation ; le recouvrement de distributions est une autre cible,
à nommer et comparer. FM-6 reste une extension temporelle avec accès aux
trames/poses apparié, distincte du primaire mono-scan.

## Phase 4 — transfert et changement d'échelle

**Mesurer.** D'abord transfert extérieur inter-capteurs et inter-scènes,
hyperparamètres gelés. Le retrait simulé d'anneaux est un stress test ;
il ne remplace pas un vrai capteur différent. Ensuite autres tâches et
domaines, puis plusieurs tailles de modèle et de corpus lorsque le pilote
a justifié ce coût.

**Sortie attendue.** Une mesure de réutilisation des représentations.
Une seule progression sur SemanticKITTI ne suffit pas à établir un modèle
de fondation. Une efficacité accrue avec peu de labels reste un résultat
utile en l'absence de gain à plus grande échelle.

## Phase 5 — sélection d'instances

SEL reste séparé du chemin de segmentation sémantique. Définir d'abord
l'objectif : V(C)=max(g(C), somme des V(enfants)), avec fond/rejet et
couverture explicites. Comparer le DP à l'énumération sur petits arbres.

**Abandonner la somme naïve des meilleurs IoU locaux.** Même des scores
parfaits favorisent les fragments au départage. Comparer coût par objet
avec cardinalité et apprentissage structuré ; publier fusion/fragmentation,
puis le vote final aux retours. À K supérieur, ce vote couple les nœuds :
un coût après vote n'est pas automatiquement additif.

Comparer à l'excès de masse sur le même arbre, puis à ALPINE sur **les mêmes
prédictions sémantiques**, avec priors et budget publiés. L'oracle isolé par
nœud reste un diagnostic ; l'oracle de SEL doit considérer sélection et vote.

## Vue d'ensemble

| étape | prérequis | résultat qui permet de poursuivre |
| --- | --- | --- |
| 0A couverture/enseignant | FULL conforme | requêtes natives vérifiées et coût de cache borné sur le pilote |
| 0B coupes/tokenizer | FULL et incidences | composition, réserves, réalisation et budgets vérifiés |
| 1 sémantique | 0B, référence reproduite | effet attribuable de FP/PUR ou simplification documentée |
| 2 graphe et K | opérateurs du bras concerné vérifiés | apport mesuré à budget total égal |
| 3 guidage SSL | 0A ; 0B seulement pour A1 | représentations utiles au-delà des raccourcis |
| 4 transfert | représentation justifiée | réutilisation mesurée, réglages gelés |
| 5 instances | objectif et lecture après vote définis | gain à même prédiction sémantique et coût déclaré |
