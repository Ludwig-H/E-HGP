# q4 : composer les réductions et mesurer les ports sur LiDAR

Audit indépendant A, 20 septembre 2026, produit 28/29 relu à `31b0243a`,
sur main. `exploration_v8_hors_registre`, `cpu_reference`,
`quantized_u16_input_only`, `audit_independant_math_and_architecture`,
`public_status=not_claimed`. Écritures sous audits/ uniquement. GCP non utilisé.

Les noyaux 28/29 relus sont cohérents. Le pas utile suivant est une composition
**certifiée et mesurée**, avec une protection explicite des événements
isolés. Les nouvelles mesures LiDAR montrent pourquoi les couches duales
ne doivent pas remplacer automatiquement les blocs locaux.

## Réponse au choix d'architecture

La [composition démontrée](COMPOSITION.md) est : noyau global R, partition
de R en cellules, puis éventuellement noyau local des actives au seuil T−c,
où c est le compte uniforme de la cellule. Les certificats sont emboîtés.
Une intersection de deux noyaux construits indépendamment est incorrecte :
la contre-fixture positive donnée dans la note perd les témoins qui
justifiaient leurs retraits. L'atlas28 existant peut aussi servir de pont,
avec son compte et ses actives intersectées au seul noyau global, mais
il conserve alors toute sa préparation sur le cover original.

Le recentrage des couches est sûr mathématiquement. Pour le porter, calculer
l'orientation avec le déterminant original : le déterminant traduit contient
un facteur positif Q inutile et peut atteindre 151 bits dans le modèle à Q=2⁴⁴.
Les comparaisons réduites restent en i128. La préparation locale et ses copies
restent à mesurer ; cette proposition n'est pas un gain produit acquis.

Pour une énumération directe, la [fixture à huit sites](DEGENERACIES.md)
est discriminante : une vraie boule q4 positive et propriétaire, coquille 8,
n'a qu'un **point isolé** de profondeur admissible à K3. Une exploration des
seules régions ouvertes la perd. Les intersections fermées de dimensions 0/1,
poids, orientations opposées et coquilles complètes doivent être conservés.
L'énumérateur fermé proposé est complet mais potentiellement combinatoire ;
il sert d'oracle. Au budget local d'intérieurs nul, une seule intersection
fermée suffit, ce qui donne un cas précis à tester sans arrangement complet.

## Contrelecture du produit et périmètre des nouvelles mesures

[Sources et reçus relus](PRODUCT_REVIEW.md), [hashes](PRODUCT_PINS.json).
Pas de défaut identifié dans les contrats examinés : population disjointe,
contacts conservés, finition terminale payée une fois, clipping, propriété
des frontières, couches duales complètes, census sans crédit ajouté et
capacités simultanées. Les deux captures synthétiques de référence sont
cohérentes ; leur lecture ne rejoue pas les 89/90 CTests constructeur.

La [sonde](lidar_product_probe.cpp) appelle réellement les entrées 28 et 29
sur le **même** nuage/index/cover, arête réindexée 0/1. Elle lie en lecture
seule les bibliothèques des builds 29 épinglés ; ses propres exécutables
Release et Clang ASan/UBSan sont construits dans ce dossier. Helpers de
collecte/juge réutilisés explicitement et hashés ; aucun port ni préparation
de la composition proposée ci-dessus n'est caché dans cette comparaison.

Les records complets (support, clé, profondeur, coquille) sont comparés avant
leurs digests. Un juge rationnel vérifie chaque support publié et fait le
census global une fois par boule distincte. Sur les petits cas, un second
oracle cartésien Python énumère indépendamment toutes les complétions et
compare toutes les présentations attendues. Le juge des grandes émissions
seul n'est pas une preuve autonome de complétude.

Qualification de la sonde : 200 commandes, 192 appels sur 32 petits nuages
aux K3/5/10, 60 sorties cumulées, coquille 30 ; Release/sanitizers concordants.
L'oracle Python énumère 5 769 tétraèdres. Les nouvelles fixtures positives
isolées et d'intersection incorrecte sont exercées dans le produit, avec
respectivement une sortie et zéro à K3. Aucun échec dans cette capture.

## Première série : arêtes LiDAR déjà suivies

Scans séparés 0/100/200 de SemanticKITTI08, quantification historique 2 cm,
sans fusion ni hypothèse de correspondance. Les 45 cas comprennent 36 couples
scan/arête/taille : rangs 8/32/128 choisis à 8k puis mêmes arêtes à 16k/32k/50k,
et neuf arêtes plus larges à 8k. K5 et K10 donnent 90 mesures appariées.
Choix d'arêtes, hashes et provenance sont conservés ; ce n'est pas un front
WSPD ni un échantillon représentatif de toutes les arêtes.

| Sommes sur 45 cas par seuil | K5, blocs 28 | K5, couches 29 | K10, blocs 28 | K10, couches 29 |
|---|---:|---:|---:|---:|
| Seeds balayées | 1 562 | 402 | 1 562 | 892 |
| Lectures de témoins W | 1 270 | 14 843 | 2 224 | 108 603 |
| Comparaisons de tri | 521 | 82 743 | 1 091 | 871 827 |
| Groupes d'événements | 225 | 13 624 | 400 | 105 880 |
| Sorties q4 positives | 0 | 0 | 0 | 0 |

À K10, la sélection 29 retire des sites sur 29 cas et des seeds sur 20 ;
elle augmente pourtant W sur 39 cas, le réduit sur 3. Elle prépare 19 124
formes, 229 579 comparaisons lexicographiques et 547 140 orientations.
La préparation 28 paie 13 000 tests de blocs et 28 477 tests ponctuels : ces
comptes ne sont pas des unités de temps interchangeables avec le tri 29.

Le cas `scan 0/8k/ancre 3000/rang 512/K10` est parlant : 28 rejette collectivement
et ne lit aucune active au balayage ; 29 réduit 1 035 sites à 180 et 122 seeds
à 62, puis lit 11 160 témoins et trie 96 069 fois, pour la même sortie vide.
Une réduction du nombre de sites ne suffit donc pas à choisir le chemin.

Sommes des temps edge+collecte : 2,681/7,364 ms à K5 et 3,649/34,984 ms à K10
pour 28/29. Les préparations communes cloud/index/cover coûtent 424,871 et
426,971 ms cumulés respectivement et ne sont payées qu'une fois par paire.
Un essai, ordre 28 puis 29, hôte partagé : pas de gain temporel stable déduit.
Ces arêtes testent ici des rejets ; aucune collecte q4 positive n'est exercée
dans cette première série. Le complément ci-dessous traite ce manque.

## Complément productif : neuf arêtes vérifiées indépendamment

La [recherche bornée](positive_discovery.py) énumère des tétraèdres dans de
petits voisinages aux distances exactes. Elle retient les trois premières
arêtes distinctes par scan ayant une boule positive propriétaire de profondeur
0–2 à 8k. Elle ne sert ni de générateur exhaustif ni d'échantillon représentatif.
4 994 propositions sont documentées, 13 boules positives recensées par clé
entière, puis neuf fixtures confirmées aussi par census `Fraction` global.
[Normal](POSITIVE_FIXTURES.json) et [−O](POSITIVE_FIXTURES_optimized.json)
concordent. Aucun flottant, recalage ou mélange de scans n'intervient.

Les mêmes arêtes sont ensuite mesurées à 8k et 50k, aux K5/10 :
[36 paires supplémentaires](positive_capture/MANIFEST.json). Leurs cibles
géométriques sont recensées de nouveau à chaque taille ; une cible devenue
trop profonde doit disparaître. Toutes les 20 occurrences de cible sous le
seuil sont retrouvées, les 16 autres rejetées. Les ports produisent au total
54 présentations identiques, 216 occurrences d’IDs de coquille ; le juge contrôle toutes
les boules émises, pas seulement les neuf cibles.

| Sommes sur neuf arêtes | W28 | W29 | Tri28 | Tri29 | Sorties identiques |
|---|---:|---:|---:|---:|---:|
| 8k, K5 | 485 | 480 | 795 | 1 551 | 14 |
| 8k, K10 | 485 | 500 | 795 | 1 691 | 21 |
| 50k, K5 | 1 222 | 3 505 | 833 | 18 352 | 0 |
| 50k, K10 | 5 628 | 14 219 | 4 725 | 100 670 | 19 |

Ces 54 présentations ne sont pas 54 boules distinctes.
À 8k/K10, 29 ne retire aucun site : 137 sites couverts cumulés, 32 seeds.
À 50k/K10, il réduit 856 sites à 643, mais ne retire qu'une seed sur 190.
Cela ne compense pas ici l'abandon de la localisation 28. Inversement, en cumul, les
appels 29 à 8k sont plus rapides dans cet essai, malgré davantage de
comparaisons : ne pas transformer W seul en prédiction de temps.

Lectures [normale](POSITIVE_READ_normal.json.gz) et
[optimisée](POSITIVE_READ_optimized.json.gz) identiques. Un échec d'import
du runner, avant tout appel moteur, est conservé avec sa source dans
[SUPPLEMENT_PREFLIGHT.json](SUPPLEMENT_PREFLIGHT.json) ; le chemin de module
a été corrigé avant cette capture close. Aucun échec moteur n'est masqué.

## Retour au certificat de fenêtre 30 en chantier

La [preuve indépendante](WINDOW_INDEX.md) confirme la fenêtre fermée proposée :
après c constantes intérieures, H=T−c, conserver de la H-ième sortie
décroissante à la H-ième entrée croissante. L=U reste possible ; les égalités
aux bornes et les coquilles constantes restent entières. La borne 2H−2 porte
sur les **IDs strictement entre les bornes**, pas sur les coquilles.

L'index futur doit conserver toutes les frontières cycliques des couches,
leurs poids et les plateaux, ainsi que le traitement séparé des c=0.
Tangences, pôle et point dual de la seed découpent les projections en chaînes
monotones ; les seuls voisins de la couche seed ne suffisent pas. La note
donne les formules exactes, sans promettre de recherche logarithmique acquise.
[508 cas de modèle](WINDOW_CHECKS.json) passent en normal/−O. Le port 30
en cours n'est ni lié ni qualifié par nos exécutables 28/29.

## Preuves reproductibles et limites

[Qualification](qualification/MANIFEST.json), [90 mesures](lidar_capture/MANIFEST.json),
lectures [normale](READ_normal.json.gz) et [optimisée](READ_optimized.json.gz).
Le lecteur rejoue aussi l'oracle cartésien petit et vérifie les fichiers
d'entrée, commandes, bibliothèques, binaires et comptes. Les deux lectures
sont identiques. Les 65 sources/entrées de compilation propres sont épinglées.
Le complément productif épingle aussi son chercheur, ses fixtures et son runner.

Le [modèle de composition](composition_gate.py) couvre 72 noyaux, 288 fragments,
125 réductions locales, 9 342 centres globaux, 4 474 points locaux et 400
identités de translation. Le [modèle dégénéré](degeneracy_gate.py) couvre 77
cas fermés, 933 états et 260 sommets. Ces modèles rationnels ne constituent
pas un raccord produit ni une qualification de complexité.

Depuis la racine : `python3 -B morsehgp3D_v8/audits/q4_kernel_composition_20260920/read.py`,
puis avec `-O`. Les bibliothèques épinglées et données LiDAR restent requises.
Depuis ce dossier d’audit, pour une nouvelle capture utiliser `campaign.py qualify qualification_nouvelle`,
puis `campaign.py measure capture_nouvelle CHEMIN_DU_BINAIRE` ; les noms
existants sont refusés. Ne pas modifier les sources de preuves closes.
Complément : `positive_campaign.py read`, puis avec `-O`. Une nouvelle
recherche prend `positive_discovery.py --output NOM_NEUF.json` ; ses reçus
conservent toutes les propositions. [Fermeture finale](CHECKS.json).

La préparation par arête, son cumul au front global, les coquilles/intérieurs,
la tour FULL, le GPU et les contrats 50k/massif restent ouverts. Les nombres
ci-dessus ne constituent pas une mesure de la tour.
