# LiDAR réel : aucune hypothèse d'alignement des points

14 septembre 2026. Auditeur A, `exploration_v8_hors_registre`,
`cpu_reference`, `quantized_u16_input_only`,
`audit_independant_math_and_architecture`, `public_status=not_claimed`.
GCP non utilisé.

L'utilisateur précise que les optimisations doivent réussir sur les régimes
réels, en priorité LiDAR. Des nuages peuvent être correctement recalés dans
un même repère ; leurs points n'ont pour autant aucune raison de former
des colonnes coordonnées ou des lignes exactes. Les spécialisations axiales
restent des chemins facultatifs. Le chemin général et son coût résiduel
doivent être jugés sur cet échantillonnage irrégulier.

## Résultat utile au constructeur

**Le front général filtre fortement ces scans sans supposer des points
alignés ; la recherche de témoins coûte encore trop cher en mono.** Les
36 appels des quatre campagnes sont clos, sur les sources de da366f7f.
À 50 000 sites, Kmax=10 et s=8, voici les deux temps observés de front
avec callback, et le résidu q2 de `MidpointSamples` :

| Scan primaire | Pure, secondes | MidpointSamples, secondes | Paires q2 restantes | Part éliminée |
| --- | ---: | ---: | ---: | ---: |
| 000000 | 0,804–1,106 | 7,810–10,476 | 19 820 147 | 98,41 % |
| 000100 | 0,988–1,136 | 7,978–8,328 | 16 584 556 | 98,67 % |
| 000200 | 0,777–1,037 | 9,009–10,397 | 38 412 850 | 96,93 % |

La référence Pure conserve les 1 249 975 000 paires dans chaque lane.
Le front filtré émet 3,71 à 4,04 millions de rectangles et paie 177 à
197 millions de pas de recherche dans l'index. Ces compteurs localisent
un coût concret à réduire ; ils ne prouvent pas que tous ces pas sont
redondants. Aucun vainqueur de la chaîne complète ne peut être déduit
sans payer le census des résidus. Les tableaux complets, dont q3/q4,
sont dans [SUMMARY.json](SUMMARY.json) ; les masses par lane ne sont
pas des nombres de supports émis ni des résultats de census.

La prochaine comparaison utile raccorde directement les nœuds spatiaux
émis au census, avec nuage/index partagés, sans reconstruire un plan ou
recopier B pour chaque rectangle. Comparer alors **front + census + collecte**
sur les mêmes fichiers réels. Une réutilisation des recherches ou des
témoins devra conserver la preuve par lane et mesurer le travail total ;
le seul gain du front ne suffit pas. Cette orientation rejoint le raccord
annoncé par le constructeur et fournit ses entrées de comparaison.
La présente capture active simultanément q2/q3/q4. Le raccord q2 seul
annoncé ensuite devra comparer les deux modes avec le même masque q2,
dans une nouvelle capture : retirer Xi et les branches sans consommateur
change le travail mesuré, même si les fichiers d'entrée restent identiques.

À 8k sur le scan 000000, passer de s=8 à s=12 ne retire que 5,66 %
de paires q2 supplémentaires, tandis que le temps observé du front filtré
passe de 1,122 à 1,642 s. Garder s=8 comme référence appariée et conserver
s=10/12 dans la comparaison aval ; cette capture ne choisit pas un s
universel. Les tailles 8k/16k/32k/50k restent des préfixes d'une seule
captation, sans preuve de croissance générale ni de tour FULL.

Le contrôle secondaire à cinq scans donne 16 814 306 paires q2 résiduelles
à 50k, avec 18,598–18,752 s de front filtré contre 1,780–1,818 s en Pure.
Il illustre une autre difficulté géométrique à taille identique ; il ne
qualifie pas des captures indépendantes correctement recalées.

## Entrées acquises et objet réellement testé

Sept scans complets de la séquence 08 ont été extraits des archives publiques
KITTI : 000000 à 000004, puis 000100 et 000200, avec calibration et poses.
SemanticKITTI réutilise les scans KITTI odometry ; ses poses SuMa sont une
source distincte. Ici, les poses viennent de **KITTI odometry**, sans labels.
Sources primaires : [KITTI odometry](https://www.cvlibs.net/datasets/kitti/eval_odometry.php)
et [format SemanticKITTI](https://semantic-kitti.org/dataset.html).

Le [fetch borné](fetch_kitti08.py) a transféré 19 378 167 octets par
37 requêtes Range, avec réponses 206, ETag stable, longueurs, CRC ZIP et
SHA256 contrôlés. Le [manifest](FETCH_MANIFEST.json) conserve les neuf
fichiers, leurs archives et toute la trace. Les limites réseau portent
sur les octets transférés, jamais sur une troncature des scans. Le premier
HEAD en sandbox avait échoué en résolution DNS ; la reprise réseau autorisée
puis le script ont réussi. Les données restent dans `data/`, exclues de Git ;
leur licence est celle du fournisseur, distincte du code du dépôt.

Les **trois scans isolés** sont les témoins LiDAR primaires. Un assemblage
de cinq scans voisins a aussi été préparé avant la précision de l'utilisateur :
il reste un contrôle secondaire d'accumulation, **pas le benchmark de captures
correctement recalées qu'il visait**. Il ne représente ni des revisites
indépendantes ni une qualification de recalage. Les objets mobiles et les
différences d'échantillonnage restent présents.

| Entrée | Retours bruts | Sites u16 uniques | Retours fusionnés par quantification |
| --- | ---: | ---: | ---: |
| Scan 000000 | 123 389 | 119 142 | 4 247 |
| Scan 000100 | 124 479 | 119 995 | 4 484 |
| Scan 000200 | 125 526 | 120 759 | 4 767 |
| Contrôle cinq scans voisins | 615 906 | 593 542 | 22 364 |

Les retours bruts de chaque scan ont des coordonnées distinctes. Le contrôle
accumulé compte 21 396 fusions internes aux scans et 968 égalités de
triplets quantifiés entre scans, sans correspondance physique certifiée.
Le tableau est une mesure de préparation,
pas une propriété générale des LiDAR.

## Préparation explicite, sans adaptation des axes

Le [préparateur](prepare_inputs.py) utilise le repère LiDAR du scan 000000.
Si C est `Tr` et P_i la pose caméra fournie, sa matrice est
$T_i=C^{-1}P_0^{-1}P_iC$ ; les inverses sont généraux, les rotations complètes.
Cette convention concorde avec
[l'outil séquentiel officiel](https://github.com/PRBonn/semantic-kitti-api/blob/master/generate_sequential.py).
Les coordonnées sont ensuite quantifiées sur **une grille isotrope fixe de
2 cm**, `floor(50*x + 32768 + 0.5)` sur chaque axe. Toute sortie du domaine,
valeur non finie ou pose non rigide selon la tolérance déclarée fait refuser
la préparation. Pas de redimensionnement indépendant des axes.

Le moteur actuel exige des triplets u16 distincts. L'entrée mesurée est donc
l'ensemble des sites quantifiés, avec une correspondance exhaustive
`(frame, return_id) → site_id`. Cette transformation change l'objet : la
hiérarchie de ces sites uniques ne serait pas celle du multiensemble des
retours. Deux témoins confondus donnent une profondeur deux avec multiplicité,
un après fusion ; le selftest conserve ce contre-exemple à K=2.

Chaque ensemble est ordonné par une priorité BLAKE2b déterministe des
coordonnées quantifiées, graine 3 ; les tailles 8k/16k/32k/50k sont des
préfixes emboîtés. Il s'agit d'un prélèvement de sites réels, pas du traitement
de la captation entière. Les méthodes consomment les mêmes octets, à mêmes
K et s. Les sources, transformations, correspondances et seize sorties
sont épinglées par les [métadonnées de préparation](INPUTS.json), conservées
sans recopier les données brutes dans Git. Les 18 contrôles passent
en Python normal et −O, puis la préparation réelle ferme ses pins :
[reçu](PREPARATION_CHECKS.json).

Trois axes de croissance restent distincts pour les prochaines campagnes :
plus de sites dans une même scène, plus d'étendue à densité comparable,
et plus de captures correctement recalées dans une même zone. Augmenter n
par rejet/remplacement des doublons dans un générateur ne simule pas ces
acquisitions. La famille v8 `terrain` est un slab volumique aléatoire ; elle
reste utile comme contrôle synthétique, sans autorité de représentativité
LiDAR. La présente première capture ne couvre pas toute cette diversité.

## Mesure du front général

L'[adaptateur](front_input_probe.cpp) appelle le vrai propriétaire, l'index
global et le front `Pure`/`MidpointSamples` sur ces fichiers. Il n'emploie
aucun filtre de colonnes exactes ni de plan axial par rectangle. Les seize
sources produit consommées correspondent exactement à
[da366f7f](SOURCE_PUBLICATION.json), publié pendant l'audit. Le
[snapshot](source_snapshot.zip) et le [reçu de construction](BUILD.json)
conservent leur version. Le gate géométrique de ce snapshot passe :
727 parcours, 235 134 tests de points d'oracle et sept contre-modèles.
Le premier compilateur avait besoin du chemin Boost déjà utilisé par le
constructeur ; le premier appel du gate omettait `--selftest`. Ces deux
erreurs d'invocation restent dans le reçu, avec leurs corrections.

Les chronomètres incluent lecture du fichier u16, préparation, index, front
et callback, validation puis destruction. La transformation des retours
bruts précède cette mesure ; aucun temps de chaîne LiDAR complète n'en est
déduit. Le callback compte les descripteurs, sans développer les paires ni
exécuter le census, q3/q4 ou FULL. Les capacités mémoire rapportées ne sont
pas un pic RSS. Les temps sont locaux, sur un cœur logique, avec charge
ambiante consignée ; aucun résultat G4 n'est transféré.

Le [runner](measure_front.py) ferme quatre campagnes : tailles croissantes,
contrôles à 50k et séparations s8/10/12 selon `box_gap_diameter_v1`. À 50k,
les deux répétitions inversent l'ordre des modes ; les comptes discrets
répétés doivent être identiques. Cette précaution ne transforme pas deux
chronométrages bruités en estimation statistique générale.

## Rejouer

Depuis la racine, les données et sorties restent sous ce dossier :

```bash
python3 -B morsehgp3D_v8/audits/lidar08_20260914/fetch_kitti08.py --help
python3 -B morsehgp3D_v8/audits/lidar08_20260914/prepare_inputs.py --selftest
python3 -B -O morsehgp3D_v8/audits/lidar08_20260914/prepare_inputs.py --selftest
```

Pour une première reconstruction dans une copie neuve du dossier :

```bash
python3 -B morsehgp3D_v8/audits/lidar08_20260914/fetch_kitti08.py --manifest FETCH_REPLAY.json
python3 -B morsehgp3D_v8/audits/lidar08_20260914/prepare_inputs.py --data morsehgp3D_v8/audits/lidar08_20260914/data --out morsehgp3D_v8/audits/lidar08_20260914/prepared
```

Le fetch refuse de remplacer un reçu existant et réutilise les fichiers
bruts présents après contrôle CRC ; le préparateur exige une sortie neuve
ou vide. Les sorties de cette capture sont épinglées. Décompresser le snapshot
sous `.snapshot/` et reprendre les commandes complètes de BUILD.json, avec
le chemin Boost adapté à la machine. Le produit peut aussi être reconstruit
à da366f7f après vérification des seize pins. Les données préparées complètes
et leurs correspondances restent disponibles localement pour le constructeur.
Exemple d'appel de la sonde reconstruite :

```bash
morsehgp3D_v8/audits/lidar08_20260914/.build/front_input_probe morsehgp3D_v8/audits/lidar08_20260914/prepared/single_000000/n50000.u16le 10 8 samples
python3 -B morsehgp3D_v8/audits/lidar08_20260914/measure_front.py --validate morsehgp3D_v8/audits/lidar08_20260914/campaign_check50k
```

Les campagnes se valident aussi avec `python3 -B -O`. Le runner refuse
de réécrire ses quatre dossiers clos ; toute nouvelle mesure appelle une
nouvelle capture. Une reconstruction de binaire ne reprend pas son ancien
hash ni sa qualification. Les validations finales sont consignées dans
[VALIDATION.json](VALIDATION.json).
