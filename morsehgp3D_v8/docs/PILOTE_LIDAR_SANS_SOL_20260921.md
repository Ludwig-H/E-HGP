# Pilote sans sol : masque original, puis préparation exacte

21 septembre 2026. `exploration_v8_hors_registre`, `cpu_reference`,
`lossless_float32_input_only`, `ground_mask_preparation`, `not_claimed`.
Suite du [choix de méthodes et du protocole](LIDAR_SANS_SOL_PROTOCOLE_20260921.md).
Cette tranche n'est pas le générateur HGP ni une mesure de tour.

## Ce qui est retiré, ce qui reste

L'adaptateur natif reçoit la trame entière, dans son repère capteur, sans
labels. Il utilise une instance neuve de Patchwork++ et rend un octet par
retour original :0indécis,1sol,2non-sol. Seul1autorise le retrait.
Les points hors portée, omis par le composant ou hors de son domaine
numérique restent indécis et conservés. Le traitement RNR de bruit par
réflectance est désactivé : aucun filtre de bruit ne se cache dans « sans sol ».

Version tierce épinglée : [Patchwork++3e6903a1](https://github.com/url-kaist/patchwork-plusplus/tree/3e6903a1d5537a4cc2ace897b0bbb98a92d6014c),
sources C++ sous BSD-2-Clause ; adaptateur séparé dans `bench/`, sans ROS,
Open3D ni module Python natif, Eigen système haché. Les sources tierces ne
sont pas modifiées. `EIGEN_DONT_PARALLELIZE`, un thread Eigen et absence
d'OpenMP imposent le profil mono. Les valeurs initiales réellement chargées
sont publiées par `--describe`, pas déduites d'un exemple Python.

Le pilote fixe RVPF et TGR actifs, RNR inactif, hauteur capteur1,723m,
portée radiale XY dans ]2,7m;80m], seuils graines/distance0,125m,
uprightness0,707. Six paramètres géométriques sont exposés par la CLI ;
ces valeurs restent fixes sur les trois trames, sans réglage sur08.
Un état frais signifie une nouvelle instance par trame, pas un cache disque
froid. Les adaptations internes au passage d'une trame restent celles du
composant. Les paramètres initiaux sont enregistrés, pas une prétendue
trace de chaque seuil adapté.

Deux exclusions numériques explicites deviennent **indécis**, pas suppressions :
|XYZ|>2^40 et le mot z=`0x00800000` (`FLT_MIN`, sentinelle silencieuse de
la bibliothèque même lorsque RNR est désactivé). Le premier limite les
moments de covariance, pas la taille de la recherche HGP ; il ne certifie
pas la qualité des plans ajustés. Toute omission supplémentaire est comptée
`upstream_unassigned`. XYZ NaN/Inf fait échouer l'entrée, jamais l'efface.
Le brut vide ou sans point dans le domaine évite entièrement l'appel tiers.
La limite d'IDs du composant est `INT_MAX` retours, explicitement refusée
au-delà ; ce n'est pas un quota de recherche. Les dizaines de millions
n'y sont pas testées par ce pilote.

Le masque ne remplace jamais les coordonnées. Le nouveau préparateur
`prepare_lidar_ground.py` réutilise explicitement `prepare_lidar_precision`
sur **tout le brut**, puis sélectionne ses sites par les IDs du masque.
XYZ finis exigés, y compris pour un retour déclaré sol ; les bits de
réflectance restent dans le brut haché, même s'ils représentent NaN/Inf.
Float32 original par défaut, seuls les zéros signés sont normalisés comme
dans le préparateur existant. Une grille explicite garde son pas exact,
1mm par défaut, et sa translation calculée sur la trame brute complète.
Retirer le minimum de la scène ne change donc pas l'origine de grille.

## Correspondances et découpes

Une géométrie dupliquée est conservée si au moins un de ses retours est
conservé. Les désaccords sol/non-sol/indécis sont comptés ; cette règle
s'applique aussi aux fusions de grille. Elle ne prétend pas que le site
conservé est du non-sol certain. Aucun seuil sur le nombre de sites.

Les27fichiers binaires préparés conservent :

- le masque et les listes disjointes de retours conservés/retirés ;
- le mapping de **tous** les retours vers les sites originaux ;
- les IDs originaux des sites retenus et un bit de conservation par site ;
- pour chacun des sept morceaux : coordonnées, IDs dans le nuage retenu,
  IDs dans le nuage original.

Il n'y a pas de sentinelle u32 ambiguë. Un retour sol ayant un doublon
conservé garde son mapping original et rejoint le même site retenu.
Le brut vide est refusé comme dans le préparateur précédent ; un résultat
sans site après retrait est autorisé, avec sept morceaux vides explicites.

Les moitiés/quarts héritent des partitions de la trame originale préparée,
après masque global et déduplication. La grille garde ses changements de
côté publiés, sans déplacer les plans par morceau. On ne segmente pas à
nouveau les demi-scènes. Chaque nuage HGP doit ensuite reconstruire ses
propres témoins : les hiérarchies ne sont pas simplement restreintes.

## Preuve du transport, pas vérité sémantique

Le descripteur du masque lie SHA256 du brut, SHA256 du masque, nombre de
retours et provenance du producteur. Le lecteur reconstruit les27payloads
depuis le brut et le masque ; il ne se satisfait pas de hashes de sortie
cohérents mais forgés. Les deux sources Python réutilisées sont épinglées
avant/après, ainsi que les trois fichiers d'entrée. Les sorties sont
neuves et exclusives, un échec d'écriture laisse un reçu FAILED.

Ces contrôles démontrent la cohérence des coordonnées, IDs et partitions,
pas que Patchwork++ identifie correctement tout le sol physique. Les
labels ne sont jamais une entrée du segmentateur. Une future évaluation
sur annotations séparera sol restant et objets retirés à tort. Les trois
trames disponibles de la séquence08 ne constituent pas plusieurs séquences.

## Mesures et suite

Les [reçus propres](../receipts/lidar_ground_20260921/README.md) ferment
44commandes Release et17Clang ASan/UBSan/LSan. Vingt tests du préparateur
passent normalement et sous−O : oracle Fraction indépendant, masques et
hashes altérés, doublons contradictoires, entièrement retiré, coordonnées
extrêmes et erreurs d'écriture. L'adaptateur exerce quatre fixtures et
dix refus ; trois trames×trois appels neufs donnent des masques identiques.
Six préparations complètes couvrent float32/grille1mm et42nuages, sans
fusion ni désaccord de doublons sur ces trames. Ces branches non présentes
dans les données sont exercées par les petites fixtures.

| Trame08 | Retours bruts | Sites conservés | Indécis conservés | Segment médian | Lecture→masque médian |
|---|---:|---:|---:|---:|---:|
| 000000 | 123389 | 39885 | 41 | 21,186ms | 30,358ms |
| 000100 | 124479 | 35551 | 32 | 21,141ms | 29,843ms |
| 000200 | 125526 | 45845 | 34 | 21,271ms | 29,927ms |

Trois répétitions mono sur CPU local partagé, sans GCP ni GPU. Les indécis
observés sont tous hors portée ; aucun autre motif d'omission ici. Ce sont
des décisions du segmentateur, pas des effectifs de sol correct certifiés.
Le temps lecture→masque varie de29,106à31,772ms ; il n'est pas un engagement
de latence sous charge ni une mesure de tour. Les essais natifs sont
séquentiels mais deux calculs longs de l'auditeur occupaient aussi l'hôte.

Builds désormais épinglés : `build/v8_ground_patchwork_20260921` et
`build/v8_ground_patchwork_sanitize_20260921`, chacun neuf et clos en
neuf commandes. Les builds historiques de géométrie sont inchangés.

Les coûts du segmentateur, de l'adaptation du masque et de la préparation
hors ligne restent séparés. Aucun temps de préparation seule ne devient
un chrono FULL, et aucun temps CPU local ne devient un résultat GPU/G4.
Les sources tierces et Eigen doivent rester liés à leur build/commit ;
les lecteurs vivants ne sont pas des archives autonomes sans dépendances.

`segment` englobe construction/destruction du composant, sa copie de travail,
son estimation et les copies des listes d'IDs. `prepare` englobe validation,
remappage, matrice de travail et hash brut ; `mask` inclut son hash. `wall`
couvre lecture brute → fermeture du masque, sans lancement du processus ni
écriture du JSON sur stdout. Les fichiers de préparation Python et leur
reconstruction Fraction sont **hors de ce chrono** : ce sont des objets de
qualification hors ligne, pas une voie d'entrée industrielle chronométrée.

Le code tiers épinglé répartit les points en cellules, les trie localement
puis effectue un nombre fixe de passages et d'ajustements3×3. À paramètres
fixés ici, la somme des tris est au plus O(n log n) ; les parcours de points
et copies ne forment pas de tableau de paires. Cette inspection du
segmentateur ne démontre aucune croissance du générateur HGP. Les trois
trames ont des tailles proches : leurs temps ne sont pas une expérience
d'échelle8k/16k/32k ou parent/enfant.

La suite HGP prioritaire est décrite dans le
[plan de raccord natif global](RACCORD_NATIF_GLOBAL_PLAN_20260921.md) :
front float32, rejet de produits/arêtes, propriété intégrée à la descente
des graines et partage du census, puis q4 indépendant. Mesurer alors
les sept objets bruts et les sept sans sol, sur les effectifs réels.
Moins de points peut faire apparaître davantage de boules peu profondes ;
le gain HGP n'est donc pas garanti par le seul taux de retrait du sol.
