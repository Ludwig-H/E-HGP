# Risques et réfutations

Ce qui peut tuer le projet, comment on le détecte tôt, et ce qui a déjà été
fermé. Ce document est destiné à vieillir : chaque risque devient un fait ou
disparaît.

## 1. Les trois risques majeurs

### R1 — l'hypothèse d'invariance est fausse

*Le risque.* Le poster affirme que la géométrie varie moins que
l'échantillonnage quand la portée augmente. C'est présenté comme une
**hypothèse**, et elle n'a jamais été mesurée. À grande portée un véhicule
donne quelques dizaines de retours sur deux ou trois nappes : la « surface »
reconstruite pourrait n'être qu'une autre lecture du motif de balayage.

*Détection.* Porte [G1](PROTOCOLE.md), sans apprentissage, quelques jours CPU.

*Si le risque se réalise.* L'argument « modèle de fondation » tombe, mais pas
la tour : l'exactitude, la hiérarchie et le plafond d'instances gardent leur
valeur pour l'anomalie et l'instance 4D (portes G-A et G-I). Il faut alors
réécrire la promesse, pas continuer en espérant.

### R2 — les objets filiformes naissent trop tard

*Le risque.* À $K \geq 2$, il faut $K$ boules qui s'intersectent
simultanément. Une structure mince — poteau, tronc, panneau, barrière,
cycliste — n'y parvient qu'à grand rayon, où elle a peut-être déjà fusionné
avec le sol ou la végétation. Or c'est exactement sur ces classes que se joue
la marge de mIoU.

*Détection.* Porte [G2](PROTOCOLE.md), plafond d'oracle **par classe**, avec
une attention particulière aux classes filiformes.

*Parade disponible.* $K = 1$ est le Single-Linkage, précoce sur les structures
minces. Garder $K = 1 \ldots K_{\max}$ dans le jeu de jetons et laisser
l'attention d'ordre choisir par région. La parade doit être **mesurée** : le
plafond par classe doit remonter quand on ajoute $K = 1$, sinon elle est
fausse.

### R3 — le descripteur est le levier le plus faible

*Le risque.* Hérité du corpus précédent, et probablement juste : le gain vient
de la tokenisation et de la hiérarchie, pas du codage de forme. On peut donc
dépenser beaucoup d'effort sur un descripteur raffiné pour un gain nul.

*Détection.* Porte [G3](PROTOCOLE.md), ablation XGBoost par famille de canaux,
heures de CPU.

*Parade.* Commencer par le descripteur le plus bête qui marche, et ne
l'enrichir que quand G3 montre un plafond. Ne jamais remplacer un descripteur
mesuré par un descripteur élégant non mesuré.

## 2. Risques de second rang

| risque | détection | parade |
| --- | --- | --- |
| Le budget de jetons explose : la sélection par excès de masse ne réduit pas assez | G0.3 | autre critère de sélection, ou bande plus étroite ; publier l'échec comme fausse piste |
| Les incidences facette–coface dépassent le plafond du réducteur (64 M) | G0.4 | publier des **plateaux** et non des simplexes énumérés ; sinon changer de poids |
| Le taux de refus de la tour biaise le corpus (coquille > 12 sites, dégénérescence) | G0.2 | corriger le moteur avant d'apprendre ; ne jamais remplacer une trame refusée en silence |
| Le cache de jetons ne tient pas sur disque | G0.5 | float16 sur la famille de forme, entiers sur le reste ; recalcul à la volée en dernier recours |
| Le sol domine la hiérarchie : une immense composante qui fusionne tôt | G2 et G3 (classe « route ») | garder le régime brut comme contrat, mais mesurer aussi les trames sans sol ; laisser l'axe des ordres séparer |
| Le modèle apprend la portée plutôt que la forme | G1 témoins, G5bis | garder la portée comme canal explicite ; tester l'égalisation de portée en augmentation |
| Le gain vient du budget de calcul, pas de la géométrie | G5, témoin apparié | ablation à paramètres, époques et matériel égaux ; aucune comparaison hors budget |
| L'axe des ordres ne sert à rien | G5bis | le retirer et simplifier ; ce serait un résultat négatif net, à publier |
| Latence du tokenizer, 1 à 2 s par trame | reçus R22 | acceptable pour le pré-entraînement (cache) ; **rédhibitoire pour l'embarqué**, qui n'est pas la cible de ce dossier |
| Contamination du jeu de test | règle de découpage | la séquence 08 est la seule validation ; le serveur en ligne n'est touché qu'une fois |

## 3. Risques de revendication

Aucune des briques suivantes n'est nouvelle : descripteur radial ou sphérique,
grille de distances à sondes fixes, apprentissage sur polyèdres et maillages,
Transformer hiérarchique, JEPA 3D, distillation LiDAR–image, pré-entraînement
multi-capteurs. Revendiquer l'une d'elles isolément est une erreur qu'un
relecteur sanctionnera immédiatement.

Ce qui reste revendicable est étroit, et c'est bien ainsi : un tokenizer exact
défini par un théorème, une bifiltration $(K, r)$ comme contexte, des objectifs
de pré-entraînement aux cibles géométriques exactes, et un retour aux points
démontré. Voir [`ARCHITECTURE.md`](ARCHITECTURE.md) § 10.

La revue d'antériorité de ce dossier est **ciblée sur les décisions de
conception**. Elle ne remplace pas une recherche d'antériorité exhaustive au
moment de la soumission.

## 4. Pistes déjà fermées, à ne pas rouvrir

Côté représentation :

| piste | fermée par |
| --- | --- |
| Représenter $P_v$ par sa fonction support $h_P$ seule | $h_P = h_{\mathrm{conv}(P)}$ : aveugle à la non-convexité et aux trous, qui sont l'essentiel d'une surface LiDAR partielle |
| Représenter $P_v$ par une fonction radiale $\rho(u)$ seule | une direction peut ne rencontrer aucune couche ou plusieurs ; bonne base de comparaison, pas une représentation |
| Remplacer $P_v$ par son enveloppe convexe | le polyèdre peut être ouvert, non convexe et partiellement occulté ; rien ne l'autorise |
| Forcer une partition des points à l'entrée du modèle | pour $K \geq 2$ le recouvrement **est** la contribution (manuscrit § 9.1) |
| Atlas de cartes appris par nœud | coutures et ancres changent sous décimation ; rend l'effet du tokenizer impossible à isoler |
| Champ implicite (UDF) ajusté par polyèdre | coûteux et redondant, la surface est déjà explicite ; pertinent en décodeur de complétion seulement |

Côté moteur, la liste faisant foi est
[`morsehgp3D_v9/docs/FAUSSES_PISTES.md`](../../morsehgp3D_v9/docs/FAUSSES_PISTES.md)
et les listes v7 et v8 qu'elle cite. Deux entrées concernent directement ce
dossier :

- **versionner des scans bruts ou des nuages dérivés KITTI** : fermé pour
  licence non commerciale et dépôt public. Seuls les manifestes sont versionnés ;
- **retrait du sol par seuil $z$ constant** : rejeté par le protocole sans sol.

Une piste de ces listes ne se rouvre qu'avec un **fait nouveau** : une preuve,
une fixture ou une mesure épinglée qui contredit la raison de sa fermeture.
Jamais sur un banc d'essai.

## 5. Ce qui n'est pas un risque

- **La laminarité.** Une attention sur arbre est possible : § 9.1 démontre que
  l'arbre est une partition des $(K-1)$-simplexes, donc laminaire sur les
  facettes, et la partition de l'unité $w_{x\tau} = S_\tau/T_x$ qui relie points
  et facettes y est fournie. Ce point a été soulevé et tranché ; il n'a pas à
  être rediscuté.
- **L'exactitude du réducteur aval.** `morsehgp3d/` calcule déjà le vote pondéré
  et les sélections en arithmétique exacte. Le risque n'est pas là ; il est
  dans le raccord amont (G0.1 à G0.4).
- **Le déterminisme du tokenizer.** Mesuré : 24 comparaisons appariées égales
  au reçu R22, condensés reproduits sur trois trames.
