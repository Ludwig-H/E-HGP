# Lecture indépendante du chantier sélection à plat — 4 octobre 2026

Le développeur travaille dans `build/v11-points-select/`, workflow privé `wf_fb625b66-561`, à partir de la v11 publiée `ab1a739d17f801823a66d74609209696152c8705`. Cette capsule constate un modèle et des essais locaux en cours ; elle ne qualifie aucune tête plate native/G4. Sources sélectionnées figées avant la dérivation, puis rehachées ; aucun fit ni test produit relancé.

La question actuelle est séparée en existence des clusters (masses et seuil mcs), condensation, sélection EOM/feuilles/epsilon puis complétion. `contexte/CONTEXTE.md:61–66` pose précisément le problème des points de bord qui feraient exister un cluster. `modele_lib.py:379–447` construit les masses par dates de comptage et traite toutes les réunions d'un rang ensemble : une seule lignée survivante continue, plusieurs lignées deviennent les enfants d'un parent. `nary_head.py:258–291` donne la lecture descendante correspondante. L'omission de la diagonale pour mcs≥2 est justifiable pour une ultramétrique u(i,j)≥max(e_i,e_j) : un bloc non singleton à r a tous ses sites déjà entrés à r. Cela ne justifie pas de modifier les masses sans définir leurs dates.

## Résultat utile : garder la baseline officielle intacte

`equite/equivalence_out.json` déclare sklearn **1.9.1**, NumPy **2.5.3**, 150 arbres de 30/80/200 points et 6 752 configurations ; 113 arbres comportent des plateaux. À epsilon=0, la tête N-aire et sklearn effectivement exécuté diffèrent sur **520/2 400** configurations, sans TypeError. La normalisation des plateaux change donc parfois la partition.

Le total **1 015/6 752** n'est pas un décompte de différences uniquement contre des sorties officielles obtenues : `equivalence.py:142–145` remplace la sortie i par une transcription après TypeError dans le bloc fit/tree_to_labels. Aux epsilons positifs, 495 différences agrégées sur 4 352 configurations côtoient 1 672 bascules ; leur répartition officielle/transcrite n'est pas enregistrée. La route III (code sklearn sur arbre normalisé) est disponible sur 5 192 configurations, avec 0 différence contre II ; les 1 560 exceptions restantes sont distinctes. La route V transcrite coïncide avec II sur 6 752 configurations. Les exceptions détaillées ne sont pas conservées : pas de diagnostic causal de panne de bibliothèque ici.

Recommandation : publier **sklearn tel quel** comme adversaire principal, puis un **bras de sélection commune atomique** appliqué aux deux hiérarchies. Qualifier ce bras et les versions sur la G4 avant la campagne ; les comparaisons antérieures épinglaient sklearn 1.7.2. Une équivalence à 1.9.1 n'est pas transférée à 1.7.2. Aux epsilons positifs, afficher refus et dénominateurs séparés ; ne pas compter une transcription comme résultat du logiciel officiel.

## Deux gardes avant le verdict exact

- `mesure/metriques.py:13,95–97` annonce un certificat dual d'optimalité, mais vérifie la somme de l'affectation et une majoration par les maxima de lignes. Cette borne ne certifie pas l'optimalité : retirer cette revendication ou ajouter un vrai dual serré/solveur exact borné. Aucun score existant n'est déclaré faux par cette lecture.
- `modele_lib.py:532–534` choisit le parent lorsqu'une comparaison de stabilités reste indécise au budget, avec compteur `forced`. Pour une porte exacte, exiger `forced=0` ou refuser la comparaison ; sinon annoncer un arbitrage heuristique. Le pilote capturé a **forced=0** : 135/135 accords hors ex aequo, 140/165 avec ex aequo. Aucun arbitrage forcé n'est imputé à ces sorties.

## Ce que la v10 apporte, et ses limites

La source v10 au même commit est copiée pour attribution, pas portée. `head/head.hpp:20–24` utilise EOM, z=1 et `allow_single_cluster=false` par défaut ; z=3 cité dans le contexte est un choix expérimental historique. `head.cpp:85–105` continue un enfant unique, scinde plusieurs enfants lourds et écarte les légers ; le mécanisme est une référence de structure. Ses masses sont des multiplicités u64, ses stabilités sont des doubles, et ses événements géométriques acceptent l'égalité de rang entre enfants et parents (`points/dendrogram.cpp:18`). La v11 doit définir la masse/sites versus retours, les dates d'entrée, les plateaux atomiques et le traitement de la racine dans sa propre porte ; une conformité v10 ne répond pas à ces nouveaux choix.

Aucune mesure locale citée ici ne qualifie les tailles 8k/16k/32k, les scènes réelles, une borne de croissance, le GPU ou le contrat 100 ms. Les sorties de petits nuages sont des contrôles de modèle. Les résultats G4 de hiérarchie de points déjà publiés ne sont pas des résultats de sélection plate.

## Fermeture

`review.py` lit seulement les copies de sources et leurs métadonnées avec la bibliothèque standard. Normal et −O : **48** contrôles, sorties identiques. `SOURCE_BEFORE.json` et `SOURCE_AFTER.json` attribuent le snapshot même si le chantier privé évolue. `LEDGER.json` inventorie tous les payloads sauf lui-même et le seul `SHA256SUMS` racine ; `SHA256SUMS` inventorie ensuite tous les fichiers sauf lui-même, ledger compris. Aucune donnée KITTI ni journal de session sensible copié.
