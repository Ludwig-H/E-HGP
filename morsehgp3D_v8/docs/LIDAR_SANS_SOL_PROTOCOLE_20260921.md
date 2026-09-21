# LiDAR sans sol : méthode candidate et protocole de mesure

Décision utilisateur du 21 septembre 2026 : le LiDAR après retrait du sol
devient un régime prioritaire de mesure HGP. Il complète la référence
brute, sans la remplacer. Cette note est une recherche bibliographique et
un protocole proposé, **pas une implémentation ni une qualification**.
Aucun paquet, dépôt tiers ou jeu de données n'a été installé ou téléchargé
pour cette recherche ; aucune mesure native ou GCP n'en découle.

Le [contrat de trame entière](CONTRAT_TRAMES_SEMANTICKITTI_20260921.md)
et la [préférence float32 originale](PRECISION_FLOAT32_ET_GRILLE_20260921.md)
restent applicables. La cible porte sur toute la tour K=1..10, puis K=1..5
si nécessaire, sur G4 ; un masque de sol rapide ne qualifie ni cette tour,
ni son exécution GPU, ni le contrat 1 s/100 ms.

## Recommandation

Commencer par un **adaptateur C++ Patchwork++ produisant uniquement un
masque d'IDs**, avec un état neuf pour chaque trame indépendante. Comparer
ensuite GroundGrid comme concurrent sérieux, et LineFit comme référence
légère. C'est un ordre de prototypage, **pas un classement démontré de
vitesse ou de qualité sur nos scènes**. Le choix définitif de méthode,
paramètres et politique temporelle doit être figé après l'évaluation de
segmentation et avant les chronométrages HGP de qualification.

Un seuil z constant confond altitude et appartenance au sol : il coupe
mal une pente et peut supprimer le bas des objets. Les méthodes ci-dessous
estiment une surface locale. Elles restent approximatives : pente,
bordures, végétation basse, objets fins, réflexions, raréfaction lointaine
et orientation du capteur doivent être évaluées, pas supposées résolues.

## Ce que disent effectivement les sources primaires

Sources consultées le 21 septembre 2026. Les temps sont ceux de leurs
auteurs, avec leurs versions et protocoles ; **ce ne sont pas des mesures
Morse HGP ni des prédictions G4**. Les branches de dépôts sont mobiles :
un futur port devra épingler un commit et ses dépendances.

| Méthode | Principe et intérêt | Performance publiée, avec sa portée |
|---|---|---|
| Patchwork++ | Plans locaux dans des secteurs concentriques, seuils adaptatifs et corrections de segmentation. Bon premier candidat pratique. | 54,85 Hz, soit environ 18,23 ms par conversion de fréquence, sur SemanticKITTI séquence 05, CPU i7-7700K. Sans TGR : 67,84 Hz, mais variante différente. Ne pas appeler cela une qualification mono-thread sans vérifier le build. [Article, §V-F/table II](https://arxiv.org/html/2207.11919v2). |
| GroundGrid | Carte d'élévation, statistiques locales, interpolation et historique. Concurrent particulièrement intéressant pour une acquisition séquentielle. | 5,85 ms / 171 Hz sur i5-13600K, **jusqu'à huit threads** ; IoU moyenne sol annoncée 94,78 %. Grille de 0,33 m dans l'expérience, sans implication de quantification des coordonnées HGP. [Article, §III/IV](https://arxiv.org/html/2405.15664v1). |
| LineFit | Ajustements de lignes dans des secteurs radiaux ; référence simple à comparer aux plans locaux. | 58,96 Hz dans la comparaison du papier Patchwork++, sur la même séquence et le même CPU que sa ligne ci-dessus ; ce n'est pas notre reproduction. [Table II](https://arxiv.org/html/2207.11919v2). |
| Patchwork classique optimisé | Autre candidat peu coûteux à examiner si son masque convient mieux. Ne pas le confondre avec Patchwork++. | Son dépôt annonce 100 Hz après ajout de TBB en 2024 ; il renvoie explicitement à un autre code pour reproduire le papier. CPU, parallélisme et révision doivent être retrouvés avant toute comparaison chiffrée. [Dépôt des auteurs](https://github.com/LimHyungTae/patchwork). |

Les 5,85 ms et 18,23 ms ne permettent donc pas de conclure « GroundGrid
est trois fois plus rapide » : CPU, parallélisme, état et implémentation
diffèrent. Une moyenne ne donne ni un maximum par trame ni un percentile
de latence.

Patchwork++ dispose d'un noyau C++ sans obligation ROS et expose
`getGroundIndices`/`getNongroundIndices`. C'est la raison principale de sa
priorité d'intégration, et non une supériorité géométrique présumée. Il
faut néanmoins vérifier l'exhaustivité et la signification de ces listes,
notamment les retours hors portée et les bruits. Les valeurs par défaut
du guide et du header consultés ne coïncident pas toutes : publier les
paramètres **effectivement chargés**, pas un nom de configuration.
[Dépôt officiel](https://github.com/url-kaist/patchwork-plusplus),
[API et paramètres du noyau](https://raw.githubusercontent.com/url-kaist/patchwork-plusplus/master/cpp/patchworkpp/include/patchwork/patchworkpp.h).

GroundGrid réutilise sa carte précédente ; une première trame sans
historique n'est pas la même expérience qu'une séquence déjà initialisée.
Son code actuel emploie odométrie et transformations pour déplacer la
carte : leur provenance et leur coût doivent être déclarés, jamais
remplacés silencieusement par une information parfaite gratuite.
[Article, §III](https://arxiv.org/html/2405.15664v1),
[initialisation et mise à jour officielles](https://raw.githubusercontent.com/dcmlr/groundgrid/main/src/GroundGrid.cpp).

Le dépôt LineFit cité est celui de **cette implémentation**, pas une preuve
qu'il s'agit du code original des auteurs de l'article de 2010. Il expose
hauteur du capteur, limites de portée, pente, repère aligné sur la gravité
et nombre de threads ; son noyau peut être compilé séparément de ROS.
[Dépôt de l'implémentation](https://github.com/lorenwel/linefit_ground_segmentation).

### Licence et intégration industrielle

Les fichiers LICENSE consultés indiquent
[BSD-2-Clause pour Patchwork++](https://raw.githubusercontent.com/url-kaist/patchwork-plusplus/master/LICENSE),
[BSD-3-Clause pour GroundGrid](https://raw.githubusercontent.com/dcmlr/groundgrid/main/LICENSE)
et [BSD-3-Clause pour LineFit](https://raw.githubusercontent.com/lorenwel/linefit_ground_segmentation/master/LICENSE).
Ne pas attribuer automatiquement la licence MIT du dépôt Patchwork
classique à Patchwork++. Ces constats ne constituent pas une revue
juridique ou un audit complet des dépendances.

Avant incorporation : inventorier les fichiers réellement utilisés,
licences, notices et modifications par commit. Le noyau Patchwork++ lie
Eigen et son composant commun ; GroundGrid déclare notamment ROS,
grid_map, cv_bridge et pcl_ros. Séparer les dépendances du calcul de celles
des démonstrations/visualisations ; ne pas présumer une incompatibilité,
ni une permission globale à partir de la seule licence du dépôt.
[CMake Patchwork++](https://raw.githubusercontent.com/url-kaist/patchwork-plusplus/master/cpp/patchworkpp/CMakeLists.txt),
[manifest GroundGrid](https://raw.githubusercontent.com/dcmlr/groundgrid/main/package.xml).

## Masque de sol, pas nouveau nuage reconstruit

Le contrat de l'adaptateur proposé est le suivant :

1. Lire **tous les retours de la trame originale**, garder leur ID, les
   bits XYZ float32 et la réflectance brute. La segmentation peut employer
   une copie de travail ; elle ne modifie jamais ces données possédées.
2. Produire pour chaque retour un statut explicite : sol, non-sol,
   indécis/hors domaine, éventuellement bruit distinct. Seul le statut
   sol autorise son retrait dans ce profil. Par défaut, conserver les
   autres statuts, y compris les points que le composant tiers omettrait
   de ses listes. Publier ces omissions au lieu de les transformer en sol.
3. Sélectionner les **coordonnées originales par IDs**. Ni centres de
   cases, ni points projetés sur un plan, ni voxels moyens, ni recherche
   approximative du retour le plus proche pour reconstruire les IDs.
   Une grille interne de segmentation n'est pas la grille numérique HGP.
4. Conserver le masque complet, y compris les IDs retirés, et les
   correspondances retour/site global/site retenu/site du morceau. Des
   doublons XYZ avec décisions différentes doivent rester visibles ;
   proposition conservatrice : garder le site si au moins un retour est
   conservé, compter ces conflits et toutes les correspondances.
5. Pour l'option grille HGP, réutiliser le même masque obtenu sur les
   originaux, puis appliquer le protocole de grille globale et translation
   commune. Publier fusions et changements de côté ; ne pas réadapter le
   masque à chaque pas pour diminuer artificiellement le travail.

Le RNR de Patchwork++ est un traitement de bruit réfléchi, distinct du
retrait du sol. La portée configurée est également distincte de la
cardinalité : les points hors zone traitée restent conservés/indécis.
La réflectance non finie ne doit pas invalider une géométrie XYZ finie :
désactiver ce critère ou définir une voie indécise explicite et compter
ces cas. Aucun filtre supplémentaire n'est introduit sous le nom « sans
sol ». [Description des options du composant](https://github.com/url-kaist/patchwork-plusplus).

Une correction d'attitude utilisée par le segmentateur ne modifie que
sa copie de travail et doit être déclarée. HGP reçoit toujours les XYZ
originaux dans le repère capteur ; les coupes restent celles de ce repère.

## Évaluer le retrait sans utiliser les labels pour le décider

Les fichiers `.label` ne sont lus que par l'évaluateur **après production
du masque**. Ni la segmentation, ni son adaptation, ni HGP ne reçoivent
les labels sémantiques ou d'instance. Le masque idéal issu des labels
peut servir de diagnostic distinct, jamais de méthode produit ni de temps
de segmentation prétendument nul.

La définition du sol doit être figée avant les comparaisons : proposition
de score principal, classes brutes 40/44/48/49/60/72, respectivement route,
parking, trottoir, autre sol, marquage et terrain. Exclure 0/1/70 du score
binaire principal et publier leur devenir séparément. Les IDs sont ceux
du [fichier officiel SemanticKITTI](https://raw.githubusercontent.com/PRBonn/semantic-kitti-api/master/config/semantic-kitti.yaml).
Inclure le trottoir dans ce score ne prouve pas la bonne conservation des
petites bordures : compléter par une inspection ciblée de ces frontières.

La classe végétation mélange couvert bas et végétation élevée. Le guide
officiel explique pourquoi les papiers Patchwork et Patchwork++ utilisent
des définitions différentes. **Exclure une classe du score ne signifie
jamais retirer ses points avant l'algorithme.** Tous les candidats doivent
être évalués sous notre même définition, sans comparer directement des
F1 issus de protocoles différents.
[Protocoles d'évaluation officiels](https://url-kaist.github.io/patchwork-plusplus/USAGE.html).

Rapporter précision du sol prédit, rappel, F1/IoU, nombres absolus d'objets
retirés à tort et de points de sol restants. Détailler les erreurs pour
piétons, cyclistes, poteaux, véhicules, végétation et par distance/pente ;
un bon score moyen peut masquer la suppression d'un petit objet. Pour
HGP, préférer une politique conservatrice de suppression aux gains de
temps obtenus en retirant davantage d'objets.

Choisir à l'avance plusieurs séquences et situations, séparer les données
de réglage de celles de qualification et conserver les échecs. Les trois
trames disponibles de séquence 08 ne suffisent pas. Ne pas choisir ensuite
les scènes où le masque retire le plus de points ou accélère le plus HGP.

### Deux expériences temporelles séparées

Patchwork++ adapte aussi ses paramètres à partir d'estimations passées ;
« sans apprentissage supervisé » ne signifie pas « sans état ».
[Article, §III-D/E](https://arxiv.org/html/2207.11919v2).

- **Trame indépendante** : instance/état remis à zéro pour chaque trame ;
  paramètres initiaux identiques. C'est le premier pilote proposé.
- **Séquence causale** : ordre chronologique, remise à zéro au début de
  chaque séquence, historique provenant seulement du passé. Publier
  amorçage, poses et état ; aucune initialisation par des trames futures.

Ne pas comparer un GroundGrid ou Patchwork++ déjà adapté à un concurrent
réinitialisé à chaque trame sans le signaler. Répéter le calcul HGP sur un
masque figé est utile, mais répéter la segmentation sur la même trame avec
un état qui évolue n'est pas une répétition de la même expérience.

## Trame entière, puis moitiés/quarts

Appliquer la segmentation **une seule fois à la trame entière**. Figer et
hacher le masque, construire le sous-nuage complet retenu, puis ses deux
moitiés `x<0`/`x>=0` et ses quatre quarts selon `y<0`/`y>=0`, dans le repère
du capteur. Ne pas réestimer le sol séparément dans les morceaux : cela
changerait les entrées et détruirait la comparaison de croissance.

Conserver deux familles appariées de sept objets : brute et sans sol.
Pour chacun, HGP reconstruit son propre index et ses témoins avec ses seuls
sites, conformément au [protocole spatial](PROTOCOLE_LIDAR_SPATIAL_20260921.md).
Les hiérarchies d'un morceau ne sont pas la restriction des hiérarchies de
la trame entière. De même, les hiérarchies sans sol ne sont pas celles du
nuage brut : enlever des témoins modifie les profondeurs et peut même
faire apparaître davantage de candidats de faible profondeur.

Publier les **six relations parent/enfant** dans chacune des deux familles,
avec les effectifs réels de sites. Comparer le rapport de travail au carré
du rapport réel des tailles, pas automatiquement à quatre. Pour un coût
positif W et des effectifs distincts positifs :

$$ r = \frac{N_{parent}}{N_{enfant}},\qquad \alpha = \frac{\log(W_{parent}/W_{enfant})}{\log(r)}. $$

Un cas vide, un coût nul ou des effectifs égaux est publié comme non
estimable ; il n'est pas supprimé. Ne pas trier les quarts pour inventer
une série. Les tailles 8k/16k/32k restent des diagnostics synthétiques,
pas des plafonds ou sous-échantillonnages des nouvelles mesures LiDAR.
Ces observations ne prouvent pas une borne globale sous-quadratique.

## Coûts à publier et portes avant raccord

Mesurer séparément segmentation complète, adaptation/copies/masque,
préparation des sites/index, chaque étape HGP et collecte des sorties.
Publier aussi leur **temps bout en bout réellement mesuré**, transferts
nécessaires compris. En présence de recouvrement des tâches, la somme des
temps de service n'est pas le temps mur. Afficher le coût segmentateur
une fois par trame, pas une fois par morceau, et ne pas le faire disparaître
du scénario d'exploitation à partir d'une trame brute.

Pour comparer les méthodes : CPU, nombre de cœurs/fils, affinité,
compilateur/options, état froid/chaud, version, paramètres, taille d'entrée,
nombre conservé et mémoire maximale. Commencer à un thread, puis comparer
quelques threads sur les mêmes entrées ; ne pas extrapoler à 48 CPU ou au
GPU. Faire des répétitions sans grosse charge concurrente. Publier les
comptes de travail et sorties HGP, pas seulement les durées : moins de
points ne garantit pas une baisse proportionnelle du travail.

Premières portes proposées pour l'adaptateur, avant campagne coûteuse :

- partition complète des IDs et intégrité bit à bit des coordonnées
  retenues ; cas vide, tout sol, aucun sol, hors portée et doublons ;
- paramètres, état temporel et politiques de bruit explicites ; entrée
  `.label` inaccessible au processus de segmentation ;
- plusieurs scènes, pente/rupture de pente, objet proche du sol et
  végétation ; critères de qualité fixés avant sélection finale ;
- relecture du masque depuis l'entrée source et inventaire de ses hashes,
  puis vérification des sept partitions et des correspondances ;
- mesures appariées K5/K10 et s8/s10/s12, sans changer le masque en fonction
  de K, s, du nombre de workers ou du succès du contrat temporel.

**Suite concrète :** intégrer et qualifier le masque Patchwork++ en premier,
sans toucher à l'exactitude numérique de HGP ; décider ensuite, sur les
erreurs et coûts observés, s'il reste la méthode principale ou si GroundGrid
le remplace. Le retrait du sol est une sélection géométrique approximative
explicite. L'exactitude HGP demeure à prouver sur **tous les sites retenus** ;
elle ne devient ni une garantie de segmentation ni une garantie concernant
les points supprimés.
