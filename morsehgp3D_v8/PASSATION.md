# Passation v8 — précision float32, puis chaîne parallèle

## Reprise du 22 septembre — entrée courante

Base `a74e90f2`, reprise du développement sur `main`. Lire d'abord
[REPRISE_U18_ET_ATLAS_SATURANT_20260922.md](docs/REPRISE_U18_ET_ATLAS_SATURANT_20260922.md).
Les 42 gates extrêmes laissées non commises sont finalisées avec des
contrôles distincts u18 ; les API numériques autonomes sont protégées.
Une option d'atlas interrompt les classifications déjà rejetées à K−1,
sans publier de fragment partiel. Les compteurs incluent le préfixe payé.
Les nouvelles captures, et non les anciens builds, font autorité.
Première référence entière sans sol à1mm :08/000000,39885sites,K5/s8/W8,
104,63s mur (812,82CPU·s), flux q3/q4 seul et saturation désactivée.
Une répétition ; ni FULL/GPU ni croissance qualifiés par cette ligne.

Priorité technique de cette reprise : moteur entier à 1 mm. Le contrat
normatif reste celui d'`AGENTS.md` : trame brute entière, float32 original
par défaut, grille optionnelle, LiDAR sans sol également prioritaire.
Les paragraphes suivants relatent les décisions/passations précédentes ;
ils ne transforment pas les résultats sans sol à 2 cm en qualification du
contrat principal. Les brouillons float32 sont conservés sans modification.

21 septembre 2026. Cadre actif : `exploration_v8_hors_registre`,
`backend=cpu_reference`, `quantized_u16_input_only`,
`implementation_v8_p0`, `not_claimed`. Aucun contrat de tour n'est encore acquis.
Le constructeur a changé le17 septembre, puis de nouveau à cette reprise
du20 septembre. L'ancien auditeur B a livré les tranches19–21 ; son canal
`audits/DIALOGUE_AUDITEUR_B.md` a été rouvert le21 septembre comme canal
d'audit indépendant. Ses anciens résultats de constructeur et ses nouveaux
audits ne qualifient jamais automatiquement les nouveaux ports.

## Historique : reprise développeur du 21 septembre (20:10 UTC)

L'utilisateur a fait passer l'auditeur B au rôle de **développeur** de la v8.
Directive : auditer entièrement, puis continuer le développement ; **régime
prioritaire = LiDAR SemanticKITTI sans sol, 30 000 à 60 000 sites**, où les
contrats **temps (1 s puis 100 ms)** et **K (5 ou 10)** doivent passer ;
parallélisation multi-CPU puis GPU ; feu vert G4 quand une mesure l'exige.
L'audit complet à neuf lentilles, l'état par composant, les incidents, les
décisions proposées (moteur entier vers les contrats, frontière du chronomètre,
régime de tests) et le plan par phases sont dans
[docs/AUDIT_REPRISE_DEVELOPPEUR_20260921.md](docs/AUDIT_REPRISE_DEVELOPPEUR_20260921.md) ;
ce document décrit l'état à cette date, avant la reprise du 22 septembre
placée en tête de cette passation. Première entrée du régime prioritaire :
nuages sans sol au profil u16 (2 cm), 39 815 / 35 491 / 45 114 sites
([reçu](receipts/lidar_ground_u16_20260921/README.md), commit ec2bc503).
Première mesure (diagnostic, à épingler en phase 0) : scène 0 sans sol, K5,
1 050 à 1 240 CPU·s en local pour un flux de candidats, contre 48 CPU·s pour
1 s sur 48 CPU. Brouillons non suivis de l'ancien constructeur (raccord global
float32) : non repris, sort à décider par l'utilisateur. Tranches du 21 au
22 septembre et prochain chantier (session G4 à 48 workers) dans
[docs/JOURNAL_DEVELOPPEMENT_20260921.md](docs/JOURNAL_DEVELOPPEMENT_20260921.md) ;
reçus [ground_baseline_20260921](receipts/ground_baseline_20260921/README.md)
et [ground_phase1_20260921](receipts/ground_phase1_20260921/README.md) :
scène 0 sans sol K5 un worker 890 s → 453 s, K10 huit workers 911 s → 323 s,
sorties identiques. **22 septembre** : décision utilisateur de poursuivre en
**entier 18 bits** (grille 1 mm, ±131 m ; profil `quantized_u18_input_only`,
les entrées u16 restant valides et bit-identiques) ; aucun développement
float32 pour l'instant. Note, inventaire et choix dans
[docs/ELARGISSEMENT_18_BITS_20260922.md](docs/ELARGISSEMENT_18_BITS_20260922.md) ;
port et résultats dans le journal (section « Élargissement du moteur entier
à 18 bits ») ; entrées 1 mm sans sol = `scene_0X_grid/full.u32le` du reçu
[lidar_ground_20260921](receipts/lidar_ground_20260921/README.md).

## Contrat principal actif — trames entières

La décision utilisateur du 21 septembre remplace la cible nominale 50k
par **une trame SemanticKITTI entière**, à qualifier sur plusieurs scènes.
Objectif **G4 : toute la tour K=1..10 en moins d'une seconde**, repli sur
toute la tour **K=1..5**, puis **100 ms** après le premier jalon.
Le [contrat détaillé](docs/CONTRAT_TRAMES_SEMANTICKITTI_20260921.md)
fait foi ; pas de sous-échantillonnage, de préfixe ou de plafond de points.

La précision cible est désormais **float32 original sans perte**, avec
une grille isotrope optionnelle, de pas paramétrable et **1 mm par défaut**.
La [nouvelle entrée et première primitive q2 exacte](docs/PRECISION_FLOAT32_ET_GRILLE_20260921.md)
ne sont pas encore raccordées au moteur complet ; les chronos u16 à 2 cm
restent historiques. Ne pas les transformer en performances float32 ou
millimétriques. Conserver les correspondances de tous les retours et
publier doublons/fusions. Trois trames de la séquence08 ne constituent pas
une validation de plusieurs séquences. Les coupes spatiales diagnostiquent la
croissance ; elles ne remplacent pas le contrat entier. Le flux q3/q4 CPU
n'est pas FULL ni GPU. Les reçus historiques 50k restent inchangés.

## Historique : plan de raccord float32, non repris dans la tranche u18

Lire le [census q3 float32 partagé](docs/CENSUS_Q3_FLOAT32_PARTAGE_20260921.md)
et ses [preuves/mesures](receipts/float32_q3_census_20260921/README.md).
Cette entrée traite UNE arête et un sous-arbre fournis. Les groupes X
partagent le préfixe de témoins ; chaque enfant/graine reçoit son propre
compte ET curseur. La coquille repart globalement, les graines invalides
restent témoins. Bornes de centres conditionnelles, intervalles physiques
continus, relais exact filtré. Pas de clé pour un rejet ni de propriété
par plus longue arête. Les états et buffers sont privés, mais le transport
asynchrone et le partage fin entre workers ne sont pas encore raccordés.
Release/Clang ASan/UBSan/LSan :612appels Fraction,1798supports,8610IDs,
37refus,16corruptions et458contrôles natifs ; deux mutations compilées
tuées par sorties géométriques fausses, pas par crash. Builds épinglés
dans le reçu. Mesures8k/16k/32k mono sur une arête synthétique seulement.
Suite : raccorder aux graines/arêtes natives et mesurer le vrai travail
global, puis partager les tickets possédés et porter les blocs q4.
Ni un rejet q3 ni ses seules sorties ne doivent commander l'accès à q4.

Le [pilote LiDAR sans sol](docs/PILOTE_LIDAR_SANS_SOL_20260921.md) est
maintenant implémenté dans `bench/` : Patchwork++ épinglé, RNRoff,
état neuf, un thread, inconnus conservés. Préparer tout le brut avant
masque : même translation de grille, doublons conservés si au moins un
retour reste,27payloads par profil pour tous les IDs et sept partitions.
Release44commandes etClang ASan/UBSan/LSan17 passent,20tests de préparation,
lectures normal/−O et42nuages. Trames0/100/200 :39 885/35 551/45 845sites
retenus, médianes lecture→masque30,358/29,843/29,927ms CPU partagé.
Pas de mesure HGP ni de score sémantique ; aucune annotation n'est nécessaire
pour lancer les futures comparaisons de calcul appariées. GCP non utilisé.
Conserver les [builds/reçus](receipts/lidar_ground_20260921/README.md) épinglés.

Priorité suivante : [raccord natif global](docs/RACCORD_NATIF_GLOBAL_PLAN_20260921.md),
avec front et filtres de produits/arêtes, propriété dans X AVANT census et
tickets partagés. Ne pas payer le census de toutes les graines puis filtrer
les sorties. B `f7b220c4` exige des sorties croissantes et des taux de repli
publiés à l'échelle ; ses limites sur la matrice précédente sont reconnues.
Pas de nouvelle micro-campagne q2, ni d'attente des labels avant ces mesures.

Lire d'abord [l'identité des boules et les événements q4 natifs](docs/IDENTITE_FLOAT32_ET_EVENEMENTS_Q4_20260921.md).
Clé primitive globale commune q2/q3/q4 à l'émission ; `from_support`
ne refait pas la positivité. Le format conserve les exposants des bits
nuls comprimés, sans quantification. Copies/déplacements de clé copient
leur vecteur : prévoir une arène de sorties avant un transport massif.
Comparateur q4 préparé184octets, signes des DEUX dénominateurs, contacts
exacts ; aucune formation des produits de degré9. Les
[qualifications communes](receipts/float32_identity_20260921/README.md)
rejouent les anciennes gates après extension du header entier.
Release et Clang ASan/UBSan clos :229clés/960événements,141refus,
18corruptions et deux mutants compilés ;948+9051contrôles entiers et
1636cas géométriques précédents repassent. Builds exacts dans le reçu,
à ne pas écraser. Relectures normal/−O concordantes ; GCP non utilisé.
Ce n'est ni le catalogue ni l'ordre des rayons ni le census natif.
Les bornes et le partage q3 sont portés dans l'entrée locale ci-dessus ;
restent le raccord au front et le transport possédé des tâches.

Supports q3/q4 natifs et puissances désormais qualifiés ; lire
[la nouvelle note numérique](docs/BOULES_FLOAT32_Q3_Q4_20260921.md).
Préparation positive stricte, objet128octets, coefficients d'intervalles
réutilisés et repli entier1728bits local au worker, sans clé/gcd par
candidat ni cache partagé mutable. Orientation q4 normalisée APRÈS les
quatre poids ; les requêtes ne recalculent pas la positivité déjà certifiée.
Release/Clang ASan/UBSan :1636cas Fraction,53refus,1287contrôles natifs,
948contrôles entiers. [Captures et builds épinglés](receipts/float32_ball_20260921/README.md),
aucune qualification héritée du moteur u16. GCP non utilisé.
La tranche suivante ci-dessus ajoute clés globales et comparateur réduit.
Restent les bornes certifiées de blocs puis le raccord aux graines/tâches.
Ne pas confondre objet support et identité de boule, ni étendre les bornes
1728bits aux expressions naïves de degré9 ou aux comparaisons de rayons.

Propriétaire/index natif float32 désormais écrit et qualifié ; voir
[l'index et le plan numérique q3/q4](docs/INDEX_FLOAT32_ET_SUITE_Q34_20260921.md).
Trois tris partagés puis partitions médianes : O(n log n), profondeur
logarithmique, boîtes exactes, IDs/rangs distincts et liens escape contigus.
Ne pas le remplacer par les coupes au milieu u16 : fixture278points,
profondeur277 par milieu contre9 au médian. Requêtes fermées sans pile,
index immuable partagé par quatre lecteurs testés ; construction mono.
Les [captures](receipts/float32_index_20260921/README.md) donnent27 fixtures,
404requêtes,35refus,151contrôles natifs, Release/Clang ASan/UBSan et
relectures normal/−O.54 constructions mesurées, dont trois trames et les
sept morceaux de la première. Cela ne qualifie pas une WSPD/census/FULL.

Le port des filtres, replis ponctuels et clés est traité ci-dessus ;
restent les bornes de blocs et leur raccord au census. Ne pas allouer
cinq tableaux multiprécision à chaque candidat : les bornes de dimension
ponctuelles et le comparateur de racines ne qualifient pas le tri des rayons.
Partager ensuite blocs de graines et tâches intérieures avec ces contrats.

Socle d'entrée qualifié :15tests de préparation,3923requêtes rationnelles
q2 et49contrôles natifs, Release/Clang ASan/UBSan, lectures normal/−O
concordantes ;42nuages issus de trois trames, sans fusion à1mm. Voir les
[captures R2 et l'échec initial conservé](receipts/float32_precision_20260921/README.md).
Les quatre builds de précision R1/R2 sont épinglés selon leurs noms exacts
dans ce README ; ne pas les écraser. GCP non utilisé pour ce lot ;
CMake et moteur u16 inchangés.

Poursuivre le port des formats numériques et des bornes sans simple
élargissement des types : carrés, produits intermédiaires, clés de
boules et comparaisons exactes q3/q4. L'index neuf ne corrige pas par
héritage les anciennes clés d'unicité et piles u16. Ne pas charger f32/u32 dans les
sondes u16. La primitive q2 filtrée est un socle arithmétique, pas une
nouvelle recherche de quelques pourcents sur q2. Le partage des graines
q3 et des tâches intérieures q3/q4 reste la priorité de parallélisation.

Prendre la trame entière comme référence principale ; utiliser le
[protocole spatial LiDAR](docs/PROTOCOLE_LIDAR_SPATIAL_20260921.md)
comme diagnostic : scan entier dans son repère capteur, deux
moitiés x puis quatre quarts y, densité inchangée, tous les morceaux et
effectifs réels. Les anciens sous-échantillons ne prouvent pas cette
croissance. Les préparations historiques100/200 sont recalées sur0 :
repartir du brut, ne pas couper ces fichiers avec une fausse origine.
La validation des nouvelles partitions et le raccord des mesures sont
distincts de la qualification moteur34 ci-dessous.
Préparations closes dans [les reçus spatiaux](receipts/lidar_spatial_20260921/README.md) :
trois scènes,21nuages,11tests normal/−O,78artefacts. Le
[raccord de mesures spatiales](docs/Q34_MESURES_SPATIALES_20260921.md)
accepte maintenant les effectifs réels sans contourner les anciens lecteurs.
Deux portes rationnelles normal/−O passent (14appels,238records chacune),
ainsi que59corruptions de reçus. Sept mesures scan0/K5/s8/W4 sont closes,
lectures et analyses normal/−O identiques ; trame entière119142sites en
383,311s sur CPU local partagé. Les six relations parent/enfant sont
publiées, y compris les défavorables : trame→moitié positive garde
exposants2,502 pour les bornes q3 et2,332 pour les bornes de blocs q4.
Ce n'est ni une borne asymptotique ni FULL ni un résultat GPU.
La suite moteur prioritaire est le port complet SharedPrefix avant
construction des boules q3, compte/curseur privés et coquille globale ;
le plan de raccord et ses obligations figurent dans la note spatiale.

Session G4 SPOT close : quart0 et trames entières0/100/200,K5/s8/W48
CPU, quatre portes natives et lectures normal/−O PASS. Les trames prennent
165,214/34,319/505,479s, pour4,19/11,13/1,93CPU logiques occupés en moyenne.
Le plan Coarse768/769/768jobs ne partage pas leur intérieur ni les
grosses arêtes ; future redistribution à prévoir avec des parents possédés.
Ne pas confondre48CPU logiques et24cœurs physiques ni une moyenne globale
avec un temps par worker. Deux comparaisons local/G4 conservent travail
géométrique et sorties. Sources/commandes et arrêt ciblé TERMINATED clos
dans les [reçus G4](receipts/q34_spatial_20260921/README.md). Aucun nouveau
contrat FULL/GPU ; pas de relance de cette référence identique sans
hypothèse utile. Les entièresK10/s10/s12 et d'autres séquences restent ouverts.

La [tranche34](docs/Q4_GRAINES_ET_CELLULES_20260921.md) porte les modes
Local28 LiveOnly et Joined, sans changement de défaut. Résumé immuable
des feuilles vivantes, court-circuit des atlas sans feuille avant résumé,
cache paresseux par bloc disjoint et balayage direct des incidences.
Les37 nouveaux compteurs séparent préparation, cache et navigation.
Ne pas attribuer au parcours conjoint le gain du simple test d'atlas mort.
Qualification corrigée close :96CTest Release, quatre portes/24sondes
instrumentées avec détection explicite des fuites, trois mutants causaux,
lectures normal/−O et145corruptions détectées. Conserver les échecs et
captures R1 des [reçus34](receipts/q4_seed_cells_20260921/README.md).
Les builds `build/v8_q4_seed_cells_20260921` et
`build/v8_q4_seed_cells_sanitize_20260921` sont épinglés. R2 reconstruit
séparément après renforcement de la seule fixture de contact et du lecteur.
Les builds `build/v8_q4_seed_cells_r2_20260921` et
`build/v8_q4_seed_cells_sanitize_r2_20260921` sont aussi épinglés.
La sonde Release R1/R2 est bit-identique ; les binaires SAN ont des hashes
différents, sans prétendre une identité binaire. Huit paires d'anciennes
CLI33/34R1 préservent les records complets, avec +256octets/WorkerState.

Trente grandes observations34 closes :6R1+24R2. Surscan0/K5/s8,
visites d'atlas LiveOnly×2,687/×2,441 au lieu de×4,018/×5,603. Les autres
scans etK10 confirment la baisse de ce poste, pas un gain de temps stable.
Joined demeure expérimental : moins de familles mais bornes plus coûteuses
et jusqu'à57,242M initialisations de cache à32k/K5. L'aval est inchangé.
Bornes d'atlas scan0K5×4,112 et scan200×4,355, censusq3 scan200×4,317 :
ce sont les priorités restantes, pas une clôture P0 globale.

Pas d'autre micro-variante q2. Après mesure des trois tailles q4,
prioriser q3 : la boîte de centres conditionnelle proposée par A évite
de refaire un intervalle cubique par X×Z ; bornes λ et comparateur de
fractions sans dépassement i128 à qualifier. Aucun crédit sans prefixe
entièrement classifié ; compte/curseur transmis ensemble, ou census zéro.
La préparation d'atlas et les tâches fines dans une arête restent ouvertes.

### Historique33

La [tranche33](docs/Q34_BORNES_AFFINES_ET_EXCLUSION_20260921.md) porte
le rejet Xi négatif proposé par B et la préparation affine des paires.
Les trois modes Legacy/Exclusion/Affine partagent les mêmes seuils stricts
K−1/K−2 ; seule une quantité suffisante de témoins rejette une arête.
Une boîte exclue retire seulement un bit de son cadre Z, jamais du masque
global. Les12 nouveaux compteurs par étage paient aussi Xi lorsque Hmin≤0.
Legacy reste le défaut et l'ancien overload est conservé. Qualification33
close :95 CTests Release, quatre portes et24sondes ASan/UBSan/LSan,
trois mutants compilés,48petites sondes candidates et huit paires de
compatibilité32/33. L'état privé augmente réellement de192octets/worker.
Trente mesures LiDAR grandes tailles sont closes ; builds32 intacts.
Les nouveaux builds `build/v8_q34_affine_20260921` et
`build/v8_q34_affine_sanitize_20260921` sont désormais épinglés.
Voir les [reçus33](receipts/q34_affine_20260921/README.md).

Le poste recherche de témoins progresse, mais pas toute la chaîne :
scan0/K5/s8, visites par paire×2,603/×2,773 ; scan200/K5/s8, bornes census
q3×4,317 et parcours q4×6,540 au premier doublement. Ne pas masquer ce
second régime par une moyenne favorable. Une question de bornes de
puissance communes à X×Z, arête fixe et bloc de graines q3, est adressée
aux auditeurs dans le journal ; ce n'est pas une implémentation.
Prioriser ces postes q3/q4 restants devant de nouvelles micro-variantes
du filtre33. Les temps sous charge ne déterminent pas un défaut performant.

Après33, ne pas ajouter de nouvelle micro-variante q2. Le partage proposé
par A nécessite deux préfixes entièrement classifiés, avec compte/curseur
par voie ; une feuille témoin ambiguë force la subdivision A/B AVANT
consommation. Garder le même ordre de témoins dans tous les descendants.
Les ordres circulaire ou orienté sur un pivot sont sûrs sous ce contrat,
mais leur rentabilité reste à mesurer. Ne pas réutiliser les sorties
terminales32 comme si elles possédaient cet invariant.

La [note q4 par produits graines/cellules](docs/Q4_BLOCS_SEEDS_PISTE_20260921.md)
décrit une autre piste de partage pour les visites de l'atlas Local28 :
bornes déjà présentes, parcours conjoint, contacts conservés, balayage
direct de la feuille et coût des préparations répétées payé. Aucun port
ni gain acquis ; un filtre qui rescannerait toutes les cellules par graine
ne résoudrait pas le problème. Les parents immuables et IDs de nœuds sont
compatibles avec de futures tâches fines ; l'arête actuelle reste atomique.

### Historique32

La [tranche32](docs/Q34_TEMOINS_INDEXES_ET_CENSUS_BOITES_20260921.md) porte
les recommandations B : rejets indexés rectangles/paires AVANT couvertures,
puis census q3 global par boîtes et collecte séparée de coquille.
Options explicites, défauts historiques conservés. L'ancien coût q3 est
fortement réduit sur le scan0/K5/s8 ; les trois tailles8k/16k/32k sont
maintenant réellement exécutées et reçues. Les visites q4 Local28 restent
au-dessus du quadruplement ; les recherches de témoins par paire aussi
sur certains postes. Window30 a maintenant été comparé GLOBAL à28,
K5/10 et trois tailles ; s10/12 également àK5. Ne pas déclarer le problème
réglé ni promouvoir un backend sur un seul compteur favorable.
Voir les [reçus32](receipts/q34_indexed_20260921/README.md) pour le bilan
complet ; aucun transfert des temps CPU locaux à G4/GPU ni à FULL.

Qualification32 close :94 CTests Release, trois gates et12sondes
ASan/UBSan/LSan, six mutants compilés,29grandes mesures et36petites
sondes. Huit comparaisons au défaut31 retrouvent sorties complètes et
travail géométrique ; état privé accru de704octets par worker, autres
capacités dépendantes du scheduling publiées. Les builds
`build/v8_q34_indexed_20260921` et
`build/v8_q34_indexed_sanitize_20260921` sont désormais épinglés.
GCP non utilisé dans32. Les échecs de lecteur et de contexte LSan restent
archivés, distincts des reprises réussies.

Lire en premier les [pistes après32](docs/Q34_PISTES_APRES_INDEXATION_20260921.md) :
le filtre actuel n'exclut que Hmax≤0, donc peut fouiller inutilement la
partie de la boule diamètre extérieure au citron. Pour une paire fixe,
Xi est une somme de carrés de formes affines ; des bornes plus serrées
et un rejet négatif par voie sont proposés. Puis partager les comptes
et les nœuds Z non consommés entre sous-produits, comme en q2. Aucun
crédit n'est ajouté au census d'une boule. L'arête reste atomique : prévoir
plages de seeds et parents immuables après réduction du travail, sans
nouvelle préparation par plage. Pas de nouvelle micro-variante q2.

Question soumise A/B : filtrer les seeds q3 avec le noyau29 au seuil
K−1 (et non K−2), géométrie Disk, puis conserver le census32 inchangé.
Ne pas transmettre un noyau paramétré K+1 à un sweep q4 qui en déduirait
son seuil : il faut séparer seuil de sélection et seuil d'émission.
Proposition non implémentée, préparation et résidu à mesurer ensemble.
Réponse B reçue à74196a31 : sûreté confirmée (le groupe positif suffit
pour filtrer les seeds), mais rentabilité déconseillée sur ses LiDAR par
un modèle de coûts, pas par un benchmark de ce filtre inexistant. Différer
le port q3 par couches ; priorité à Xi, héritage des recherches et travail
q4. B confirme aussi la validité du rejet négatif Xi et de l'héritage par
monotonie ; les frontières ambiguës doivent rester disjointes, stockées
et payées, sans réintroduire une liste O(n) par paire.

### Historique31

Tranche31,21 septembre : l'utilisateur privilégie maintenant la croissance
sur les régimes considérés, surtout SemanticKITTI, pas l'attente d'une borne
universelle. Le [raccord global](docs/Q34_GLOBAL_ET_LIDAR_20260921.md)
consomme le front multivoie avec un cover partagé par arête, un census q3
séparé et q4 Local28 ou Window30. `run_wspd_q34_parallel` réutilise les
jobs Coarse avec moteurs/buffers/callbacks privés, jointure avant retour.
Chaque arête reste atomique ; les sommes/pics de buffers ne sont pas le RSS.
92 CTests Release et trois mutants causaux passent. Les144 mesures des
neuf arêtes LiDAR donnent des sorties identiques28/29/30 à8k/16k/32k/50k,
sans promouvoir30 comme remplacement universel. Les dix gates instrumentées
passent aussi. Trois comparaisons rationnel/entier de l'oracle ont ensuite
été corrigées pour Boost ancien/C++20, puis92 CTests, dix gates instrumentées
et trois mutants requalifiés dans de nouveaux builds. Moteur inchangé.
Les sessions CPU48/G4 et leurs arrêts ciblés sont décrits séparément dans
les [reçus31](receipts/lidar_global_20260921/README.md).
Les scans sont quantifiés à2cm, échantillons imbriqués par priorité hash ;
100/200 sont exprimés dans le repère du scan0, sans mélange des scans.

Pilote G4 SPOT terminé et arrêté : cinq commandes, dont1k/W1-W48 à
géométrie/digest égaux, puis2k/4k/8k/W48. Temps48workers=0,863/8,470/
59,274/614,744s. À8k, le travail et les digests concordent aussi avec
la commande localeW4. Census q3 multiplié par10,79/10,08/10,76 aux
doublements : PAS de croissance sous-quadratique sur cette série.
Occupation moyenne à8k=3,23 CPU pour48workers ; arêtes/rectangles
restent atomiques. Réduire le travail ET rendre les grosses tâches
redistribuables. Les mesures concernent les candidats seuls, pas FULL/GPU.

Le coût GLOBAL mesuré impose maintenant la priorité : à8k/K5/s8/W4,
2,286M arêtes,780,662M seeds q3 et361,201Md tests ponctuels q3,
dont99,1% extérieurs, pour93 914 émissions q3. Le tri de coquilles ne
fait que327 815 comparaisons. Voir le [diagnostic](receipts/lidar_global_20260921/PARTIAL_GLOBAL_8K.md).
La campagne locale FAILED garde sa ligne8k achevée ;16k interrompu,
32k non commencé, aucun exposant8→16→32 calculable.
Implémenter le census q3 par boîtes exactes et, surtout, supprimer des
blocs de seeds avant construction des clés ; renforcer le rejet WSPD
h/h_a/h_b sans histogrammes carrés. Les bornes séparables proposées en
fin de note31 restent à porter/tester. Ne pas optimiser seulement neuf
arêtes favorables ni ajouter une
nouvelle variante de file q2. Catalogue/intérieurs, reconstruction FULL
et backend GPU restent distincts. L'index de couches proposé en30 reste
une piste, plus un préalable obligatoire à ce raccord.

### Tranche30 précédente close àf07fbd8c

La tranche30 ajoute `run_q4_window_edge_candidates` et son entrée par
seed sur le noyau29 : [preuve et architecture](docs/Q4_FENETRE_DE_FAIBLE_PROFONDEUR_20260920.md).
Poser T=K−2, c constantes intérieures, H=T−c. c≥T rejette ; sinon
fenêtre fermée [H-ième sortie décroissante,H-ième entrée croissante],
bornes infinies si nécessaire. L=U reste un événement admissible.
Deux tas bornés puis deux scans complets conservent toute profondeur
et coquille dans la fenêtre ; seuls2H−2 IDs strictement intérieurs
au plus sont triés. Les constantes intérieures hors fenêtre sont payées,
jamais oubliées ni confondues avec les témoins constamment extérieurs.

Premier essai dense permuté32k/K10 : run29/30=577,742/84,302ms,
comparaisons de racines20,458M/2,446M en incluant tas et fenêtre.
Le pic global propre reste1 861 533octets, dominé par la préparation ;
seuls les buffers de balayage passent de16 416 à320octets. Pas du RSS.
Les deux passages font208488/596136/1829992 visites à8k/16k/32k
(×2,859/×3,070). Mais l'adversaireK10 garde n(n−2) au premier passage :
pas de sous-quadratique général. Ce sont des candidats q4 sur UNE arête,
pas la tour ; les voies28/29 et les défauts restent disponibles et inchangés.

Qualification30 close après correction d'un seul test Python historique :
première régression90/91 FAILED, mutant `worker_digest` parfois égal à
l'original lorsque sa valeur est déjà nulle. Les52 mesures initiales et
toutes les preuves restent archivées ; reprise distincte du gel189,
moteur/binaires inchangés :91 CTests Release, neuf gates/dix sondes Clang
ASan/UBSan (pas91),6206 contrôles, trois mutants compilés,20 différentiels
contre29 et52 nouvelles mesures passent. Les32 grandes configurations
ont tous leurs champs hors temps identiques au premier gel.
Dix lectures/auto-tests normal/−O concordent ;46 corruptions du lecteur
sont refusées. Les lectures auxiliaires concordent aussi.
Les builds `v8_q4_window_20260920` et `v8_q4_window_sanitize_20260920`
sont désormais épinglés ; construire la suite dans de nouveaux dossiers.
Voir les [reçus propres30](receipts/q4_window_20260920/README.md).

**Suite structurelle :** conserver les frontières convexes comme index
de chaînes de racines, avec multiplicités, bords plats et constantes
nulles distinctes, pour enlever S×r. L'ordre projectif suggère O(1)
chaînes par couche, mais tangences/recherches binaires exactes et grandes
coquilles ne sont pas implémentées ni qualifiées. Ne pas annoncer O(log r)
avant ce port ; les seuls voisins de la couche seed ne suffisent pas.
La réponse A `WINDOW_INDEX.md`, arrivée à la clôture, confirme le
certificat30 et précise chaînes, pôle et plateaux ; modèle indépendant
508 cas, pas qualification du port. Ses126 mesures LiDAR28/29, dont
36 sur neuf arêtes productives, avertissent aussi de garder les blocs28.
Inclure ces cas réels dans la prochaine comparaison, sans héritage de preuve.
Les avis A sur composition emboîtée et régions fermées de dimension zéro
ont été lus ; sa fixture huit sites est explicitement requalifiée par
l'oracle constructeur. Ne jamais intersecter deux noyaux indépendants.
Raccorder ensuite q3, WSPD multivoie s8/10/12, catalogue/intérieurs et
FULL. Aucun contrat50k/G4, backend GPU ou borne massive acquis.
GCP non utilisé ; aucune nouvelle micro-optimisation du moteur q2.
Le [journal du correctif de test](receipts/q4_window_20260920/preflight/WORKER_DIGEST_FIX.md)
signale un risque analogue dans la gate de reçus parallèle, non observé
dans cette suite et non modifié ici ; ne pas confondre sa réussite
présente avec la preuve qu'un mutant forçant zéro change toujours la valeur.

### Tranche29 précédente close à31b0243a

La tranche29 ajoute les [couches convexes duales q4](docs/Q4_COUCHES_DUALES_20260920.md)
dans `run_q4_shallow_edge_candidates`, entrée explicite sans changer28.
Pour chaque signe de la constante, conserver K−2 frontières COMPLÈTES
et tous les sites de constante nulle. Si le census retenu est<K−2,
les sites écartés sont tous strictement extérieurs : profondeur et coquille
exactes, suppression conjointe des témoins et des seeds impossibles.
Les plans possèdent le même index, les buffers de balayage sont privés.

Le dense permuté8k/16k/32k/K10 garde408/708/1256 sites : lectures
165648/499848/1575024 (×3,018/×3,151), tris×3,251/×3,475.
Les mêmes sorties que28 sont réellement produites. Ce progrès ne ferme
pas le carré : l'adversaireK10 garde tous ses sites, ses lectures font
×4,133/×4,065/×4,032 de32 à256, et son run256 régresse×10,27.
Le coût O(m log(1+m)+Km) concerne la sélection, pas le moteur complet ;
le producteur aval reste O(Sr log(1+r)), hors travail du consommateur.
Lire les [preuves et mesures](receipts/q4_shallow_20260920/README.md).

Qualification close :90 CTests Release, huit gates/dix sondes Clang
ASan/UBSan (pas90),3686 contrôles nouveaux, trois mutants compilés,
20 différentiels contre28 et52 mesures. Lecteurs normal/−O concordants,
41 corruptions de reçus détectées ;184 sources et builds
`v8_q4_shallow_20260920` / `v8_q4_shallow_sanitize_20260920` épinglés.
Quatre appels concurrents testent le partage immuable, pas un ordonnanceur
q4 ou un nouveau backend parallèle/GPU. Pas de nouvelle porte TSan.

**Suite structurelle :** éviter d'énumérer et trier les événements profonds
entre les survivants ; ne pas augmenter les budgets de couches/cartes.
Comparer une extraction certifiée des seuls événements peu profonds et
une composition mesurée avec les partitions28, sans perdre contacts,
multiplicités, coquilles ni remettre S×m dans leur préparation.
Le fond cap montre qu'une réduction coûteuse n'aide pas lorsque deux
seeds suffisent et que28 sait déjà classer les autres témoins par blocs.
Ne pas promouvoir29 comme remplacement universel de28.
Raccorder q3 séparément, puis WSPD multivoie, catalogue/intérieurs et FULL ;
aucun progrès local ne vaut contrat50k/G4. GCP non utilisé.

### Tranche28 précédente close àc051bdb0

La tranche28 livre les [fragments exacts et balayages q4 locaux](docs/Q4_FRAGMENTS_ET_BALAYAGES_LOCAUX_20260920.md)
de `run_q4_local_edge_candidates`. Atlas immuable partagé, index global
inchangé, partition disjointe intérieur uniforme/extérieur strict/actif.
Les contacts min=0 ne disparaissent pas. La finition de chaque partition
terminale est faite une fois avant toutes ses requêtes, pas sous le budget
Z intermédiaire. Le clipping retire les événements extérieurs du tri
en gardant leur constante ; les frontières possédées règlent l'émission.
q4 seul : aucune dépendance à l'acceptation q2/q3, pas de catalogue.

Qualification close :89 CTests Release, sept gates/vingt sondes Clang
ASan/UBSan (pas89),11981 contrôles locaux, trois mutants compilés,
vingt différentiels contre27 et72 mesures, lecteurs normal/−O concordants.
Sources177 et nouveaux builds `v8_q4_local_r2_20260920` /
`v8_q4_local_sanitize_r2_20260920` épinglés. Les builds r1 sans `_r2`
restent historiques : un mutant d'intérieur clippé avait survécu ; fixture
et plancher du runner renforcés sans changer le moteur, captures préservées.
Lire les [reçus](receipts/q4_local_20260920/README.md) avant toute reprise.

**Verrou restant : W dans les feuilles.** Réels balayages denses8k/16k/32k :
préfixe W=5,311M/13,799M/15,791M ; permuté0,913M/3,579M/14,974M,
soit×3,922/×4,184, tris×4,564/×4,857. À32k/K10 permuté,981463 groupes
sont rejetés pour profondeur contre30 présentations de15 boules.
Ce n'est pas ici une croissance imposée par la sortie. Suite prioritaire :
décomposition adaptative des régions de faible profondeur et traitement
conjoint de groupes de faces/témoins, en dialogue avec A. Regrouper les
droites identiques exige de préserver orientations, multiplicités et IDs
de coquille. Payer construction, conflits, copies et résidu ; « adaptatif »
ne prouve aucune borne. Garder28 comme référence exacte, ne pas augmenter
uniformément les budgets ni paralléliser simplement ce carré.

L'atlas est immuable et les buffers privés ; quatre appels concurrents
passent, mais aucun ordonnanceur q4 multithread/GPU ni TSan nouveau n'est
qualifié. Les chronos mono sous charge ne sont pas des contrats G4.
q3 global, WSPD multivoie, catalogue, intérieurs après regroupement,
FULL, G4 et massif restent ouverts ; s8/10/12 au futur raccord WSPD.
GCP non utilisé.

### Tranche27 précédente close à66b1551f

La tranche27 livre une [carte q4 partagée à la demande](docs/Q4_CARTE_CENTRES_PARTAGEE_20260920.md)
et un domaine positif construit par blocs du même index. L'entrée
`run_q34_mapped_edge_candidates` garde le même pool que26 et ne crée la
carte qu'après son premier échec q4. Requêtes de droites locales, états
globaux profonds/hors domaine distincts ; q3 et repli exact indépendants.
Le budget borne seulement le certificat, jamais les sorties.

Qualification close :88 CTests Release, six gates/24 sondes Clang
ASan/UBSan,1956 contrôles nouveaux, trois mutants compilés tués,
20 différentiels contre26,128 mesures principales et48 filtres denses
auxiliaires. Lectures normal/−O concordantes. Les170 sources et builds
q4_center_map/q4_center_map_sanitize du20260920 sont désormais épinglés.
Lire les [reçus propres](receipts/q4_center_map_20260920/README.md).

**Le résidu reste le problème principal.** Positive7 ajoute peu de rejets
avec C64 : dense32k préfixe185,344M/357,472M lectures minimales futures
K5/10 ; grille permutée123,776M/300,608M. Repli dense non exécuté,
pas de gain stable ni de borne sous-quadratique. Le domaine positif ne
paie que sept visites de blocs et zéro test scalaire sur ces cas.
L'audit A2920b8b5 utilisait tous les témoins, pas ce pool ; ses gains
ne sont ni contredits ni hérités. Défaut historique inchangé.

Suite : blocs témoins et balayages exacts locaux du résidu, en dialogue
avec A. **Ne pas recycler les listes27** : min≥0 peut être retiré du
filtre mais min=0 doit rester actif pour les coquilles ; un compte
parent comprimé n'est pas exact. Il faut une partition disjointe exacte
I(max<0)/E(min>0)/actives, des frontières de cellules possédées, et payer
la duplication des actives, les tris et la collecte. Éviter un scan
scalaire complet par cellule ou un arrangement exhaustif. Ne pas simplement
augmenter les budgets ou re-paralléliser le carré. La carte actuelle est
mutable exclusive ; sa distribution GPU reste à concevoir et qualifier.
WSPD q3/q4 global, catalogue, intérieurs après regroupement, FULL et
contrats G4 restent ouverts ; GCP non utilisé.

### Tranche26 précédente close àddaafdc0

La tranche26 porte le [minimum collectif du pool et une corde resserrée](docs/Q3_Q4_CERTIFICAT_COLLECTIF_20260920.md).
Les quatre options sont comparées sur le même pool spatial ; le défaut
historique25 reste intact. Le minimum strict traite sorties, groupe,
puis entrées aux deux bornes fermées, sans saturation. Workspace privé
réutilisé par arête, comptes q3/q4 séparés, repli à zéro ; aucun crédit
de filtre ajouté au census. Le pic mémoire couple réellement workspace
et buffers simultanés du repli. Ne pas former DV² en i128.

Qualification :87 CTests Release, cinq portes/24 sondes Clang ASan/UBSan,
8549 contrôles de la nouvelle gate, trois mutants compilés tués,
20 différentiels contre le build25,208 mesures principales et48 filtres
denses auxiliaires. Lecteurs normal/−O concordants ; les163 sources
et les deux builds q34_collective du20260920 sont désormais épinglés. Lire les
[reçus](receipts/q34_collective_20260920/README.md).

Le port apporte un gain de rejet mais ne résout pas le régime dense :
à32k/C64, minimum futur du repli185,344M lectures K5 /358,240M K10,
dernier doublement×9,694/×6,223. Pas de dense complet lancé ni de gain
de tour inféré. Ne pas rouvrir des variantes de pool pour quelques % :
suite prioritaire, carte commune de certificats dans le plan des centres
par arête, actuellement étudiée par A dans son dialogue courant.
Pas d'arrangement complet de droites ; domaine comprenant les
complétions obtuses, témoins hors lentille conservés, UNKNOWN vers repli
exact et construction/requêtes/résidu payés. Ses preuves resteront
indépendantes jusqu'au port requalifié. WSPD q3/q4 global, catalogue,
intérieurs après regroupement, FULL et GPU/G4 ouverts ; GCP non utilisé.

### Tranche25 précédente close à8d0a0f0f

La tranche25 ajoute le [pool universel partagé](docs/Q3_Q4_REJET_FAMILIAL_20260920.md)
et deux entrées explicites `run_q34_pruned_*`. Le défaut couvert historique
reste inchangé. Les témoins sont proposés par positions spatiales
floor(i*m/C), pas par premiers IDs ; C est un coût de proposition, pas
une troncature. Deux seuils indépendants, aucun crédit ajouté au repli,
q3 seul réellement saturant si q4 est rejeté. Le pool possède cover/index.

Qualification :86 CTests Release, quatre portes/18 sondes sous Clang
ASan/UBSan,96 mesures closes, lecteurs normal/−O. La baisse de lectures
sur le petit adversaire ne borne pas les familles denses à8k/16k/32k.
Lire les [reçus et limites](receipts/q34_pruning_20260920/README.md).
Builds q34_pruning et q34_pruning_sanitize du20260920 à conserver épinglés.

Suite prioritaire : comparer le minimum collectif du même pool et la
corde resserrée proposés par l'[auditeur A à4215dd16](audits/q34_collectif_20260920/README.md),
sur échec du certificat simple et en payant leurs tris. Ses résultats
utilisent un autre pool ; aucun gain ni preuve produit n'est hérité.
Si le grand résidu demeure, passer au traitement de blocs dans le plan
des centres commun à l'arête, pas à de nouvelles constantes de pool.
Ne pas construire l'arrangement complet des droites, potentiellement
quadratique. Le générateur WSPD global, catalogue, intérieurs après
regroupement, FULL et GPU/G4 restent à construire ; GCP non utilisé.

### Tranche24 précédente close à77f659e4

La tranche24 ajoute les [covers partagés par arête](docs/Q3_Q4_COVERS_PARTAGES_20260920.md)
et la génération de toutes ses faces aiguës propriétaires. L'ancien raccord
par seed reste une référence inchangée. Les émissions du raccord couvert
ont une profondeur et une coquille globales, mais seulement **après**
certification de positivité/propriété : ses racines arbitraires portent des
comptes partiels. Aucun raccord q3/q4 à la WSPD globale n'est encore livré.
Le terme nombre de faces × taille du cover reste à éliminer collectivement ;
ne pas brancher cette entrée sur tous les produits comme si P0 était clos.

Qualification close :85 CTests Release, trois portes et six sondes Clang
ASan/UBSan, trois mutants compilés tués,32 mesures et lecteurs normal/−O.
Builds q34_cover et q34_cover_sanitize du20260920 désormais épinglés.
Les [reçus](receipts/q34_cover_20260920/README.md) publient le gain du fond
lointain, la régression dense et le contre-régime à S=n−2,m=n :65 024
lectures pour14 sorties K5 à n256. Ce dernier interdit toute prétention
sous-quadratique générale. Le prochain gain doit retirer des familles
entières ou partager leur census, pas seulement distribuer leurs scans.

### Tranche23 précédente close à785d0589

La tranche suivante après9ae4e28b est le
[raccord des candidats q3/q4 par seed](docs/Q3_Q4_CANDIDATS_PAR_SEED_20260920.md),
qualifié : `ExactBall` commun aux arités2/3/4 et
`run_q34_seed_candidates`. Ce dernier ne promet ni toutes les incidences
ni un catalogue dédupliqué ; il publie la profondeur et la coquille, pas
les IDs intérieurs. Ne pas répéter ensuite un census pour chaque
présentation avant d'avoir étudié le regroupement des clés. Le générateur
WSPD de seeds et les covers demeurent le prochain verrou de travail global.
84 CTests Release et les deux nouvelles gates sous Clang ASan/UBSan passent,
avec635 appels de raccord et1807 cas de géométrie contre oracles rationnels.
Quatre mutations compilées sont tuées ; trente mesures dont dix-huit
8k/16k/32k sont closes. Coût O(n log n) par seed seulement ; ne pas
multiplier ce scan global par toutes les seeds sans traiter les covers.
Builds désormais épinglés `build/v8_q34_candidates_20260920` et
`build/v8_q34_candidates_sanitize_20260920` ; les builds q4_family restent
épinglés. Les changements de cette tranche ne modifient pas q2.

### Reprise précédente close à9ae4e28b

Le constructeur reprend après `3e94c868`. Lire d'abord
[l'audit de reprise](docs/REPRISE_DEVELOPPEMENT_20260920.md).
Les130 sources/98 artefacts et lectures normal/−O de la tranche21 ont été
vérifiés ; il ne s'agit pas d'une nouvelle exécution historique des81 tests.
Conformément à la priorité utilisateur, ne plus poursuivre les quelques
pourcents q2. Le [balayage d'une famille q4](docs/Q4_FAMILLE_ET_BALAYAGE_20260920.md)
est livré :82 CTests Release, nouvelle gate sous Clang ASan/UBSan,
206 appels contre oracle indépendant, quinze mesures et lectures normal/−O
closes. Coût O(n+e log(1+e)) et mémoire O(n) pour UNE seed aiguë seulement.
La suite prioritaire est le générateur q3/q4 (accès complet, positivité,
ownership), puis catalogue et FULL ; aucun oracle d'audit n'est importé
comme qualification du nouveau moteur.
Profil q2 explicite de comparaison `{2,16,true}`, défaut inchangé.
Nouveaux builds désormais épinglés `build/v8_q4_family_20260920` et
`build/v8_q4_family_sanitize_20260920`. GCP non utilisé.

Lire aussi l'[audit des facettes silencieuses](audits/FACETTES_SILENCIEUSES_REPRISE_V7_20260919.md)
à c92aad13 : fixture ABCDE obligatoire, MEB intermédiaires hors catalogue,
clôture atomique des plateaux et ancres inertes à conserver. Le support
canonique du MEB se choisit sur la coquille sélectionnée dans F, pas sur
la coquille globale du census. Le petit-pivot proposé reste un prototype
indépendant à porter/juger ; ne pas importer le plafond de coquille12 v7.
Le [retour A](audits/DIALOGUE_COURANT.md) à94960c5e confirme les gains q2
sur trois scans LiDAR séparés et précise trois pièges du futur héritage
multivoie. Ses mesures restent indépendantes de cette tranche22.

## Passation précédente du17 septembre — contexte conservé

Les [témoins hérités](docs/P0_TEMOINS_HERITES_Q2.md) et la
[fenêtre élargie](docs/P0_SURPROPOSITION_TEMOINS_Q2.md) sont livrés comme
options ; **le défaut de toutes les entrées reste le front historique**.
Décisions et mesures à prendre, dans cet ordre :

1. Décider si les sondes et campagnes des tranches suivantes passent
   explicitement `WspdFrontProposals{2, 16, true}` (jamais un changement du
   défaut de l'API : les reçus antérieurs cesseraient d'être comparables).
2. Le front q2 n'a plus de levier de constante mesuré au-dessus de 10 % : à
   uniforme 32k K10, 29,0 s sont devenus 12,1 s puis 10,5 s ; les cycles se
   partagent entre la fenêtre et ses tests H (un quart), la descente (un
   sixième) et le census (la moitié). Pistes instruites et fermées par la
   mesure : reprise exacte de la descente (×0,94 à ×0,98), réutilisation du
   pivot du parent (jusqu'à ×6), descente exacte plafonnée à K, certificat de
   bloc. Piste non instruite : sauter la recherche des produits de masse 1
   (arbitrage front contre census, ×0,88 à ×0,93 au prototype). Le levier
   suivant du census serait de lui transmettre les témoins certifiés du
   rectangle émis, comptés une fois : non conçu, non mesuré.
3. Rangées : ni la fenêtre élargie ni l'héritage n'y retirent un produit ;
   les candidates qui restent sont les paires entre rangées, sans aucun
   témoin universel. Seuls des certificats collectifs traitent ce régime, en
   q2 comme en q3/q4.
4. Porter l'héritage aux voies q3/q4 demande un juge indépendant des bornes Ξ
   et un niveau par rang (crédits emboîtés q4 ⊂ q3 ⊂ q2) ; l'audit du
   14 septembre y mesurait un résidu ×0,53 et ×0,60 sur un front plus ancien.
5. Alléger `mhgp8_wspd_q2_proposals_gate` sous sanitizers (12 min) ; la
   porte d'héritage en prend cinq.
6. Moteurs q3/q4 produit, catalogue canonique de boules et tranche FULL
   minimale restent ouverts ; un oracle entier q3/q4 indépendant existe dans
   `audits/oracle_q3q4_20260915/`.

## Vingt-et-unième tranche close le 17 septembre — témoins hérités

`WspdFrontProposals{window_factor, small_factor_limit, inherit_witnesses}` :
un produit non rejeté transmet à ses deux enfants la liste des rangs de ses
témoins certifiés, au plus Kmax − 1, portée par valeur dans la tâche (72
octets au lieu de 32, pour toute option) et jamais dans l'objet front ;
l'enfant part de ce compte, saute sans test un rang déjà reçu, ajoute ses
nouveaux crédits. `h_minimum` étant un minimum exact et les boîtes des
descendants incluses dans celles des ancêtres, un rang reçu reste un témoin
universel strict hors des facteurs : le rejet reste certifié par Kmax rangs
distincts. Voie q2 seule (refus sinon et en mode `Pure`), refus au-delà de
2^32 sites. Cinq compteurs fusionnés par `merge_work`, et une identité exacte
qui ferme le registre : nouveaux crédits + crédits reçus / 2 = Kmax × produits
rejetés + crédits des rectangles émis. Le compteur de rejets du moteur est un
**majorant** du contrefactuel, que seul le rejeu indépendant calcule ; le
nombre de produits rejetés n'est **pas** monotone, seule la masse rejetée
l'est. Qualification propre close : 81 CTests Release et Clang ASan/UBSan,
portes héritage, dispatch et jobs sous Clang TSan, 1 125 rejeux indépendants,
douze lignes d'un modèle Python indépendant, oracle de sûreté par force
brute, dix mutants causaux tués dont quatre non sûrs, trois fixtures nommées
de cinq points, nuage de 320 sites et campagne à 70 000 sites pour la largeur
des rangs stockés, 990 appels des cinq entrées contre l'oracle, 854 mesures
et 24 lectures/analyses normal/−O identiques. Exécution avec héritage
rapportée à sa jumelle, n8k/16k/32k : fenêtre 2K petits facteurs, uniforme
×0,86 à ×0,93, amas ×0,88 à ×0,96, terrain ×0,89 à ×0,96, rangées ×0,99 à
×1,03 ; fenêtre historique, uniforme ×0,50 à ×0,63, amas ×0,57 à ×0,72,
terrain ×0,68 à ×0,75. Le moteur sans héritage égale le build épinglé de la
tranche 20 sur douze septuplets différentiels, en temps ×0,96 à ×1,03.
Builds épinglés : `build/v8_front_inheritance_20260917`,
`build/v8_front_inheritance_sanitize_20260917`,
`build/v8_front_inheritance_tsan_clang_20260917`. Lire les
[reçus](receipts/q2_front_inheritance_20260917/README.md) ; le dossier
`cadrage/` archive les mesures qui ont choisi ce levier et le moteur à deux
leviers, reprise exacte de la descente comprise, en patch. Deux relectures
indépendantes ont précédé le gel, de la conception puis de l'implémentation ;
leurs constats sont consignés dans le PREFLIGHT des reçus.

## Vingtième tranche publiée à 8190e7ab — fenêtre de propositions élargie, historique

`WspdFrontProposals{window_factor ∈ {1, 2, 4}, small_factor_limit}` :
fenêtre historique de min(Kmax, n) rangs d'abord, puis intervalles gauche et
droit complétant la fenêtre élargie autour du même pivot, crédits conservés
dans ce seul appel, rangs de A/B comptés puis sautés, H = 0 jamais crédité,
voie q2 seule (refus si une voie q3/q4 est active ou en mode `Pure`). Cinq
compteurs d'extension inclus dans les totaux historiques et fusionnés par
`merge_work` (garde statique sur la taille de la structure). Un seul corps de
boucle autour de la boucle historique : le défaut vaut ×0,98 à ×1,03 du build
épinglé de la tranche 19 en temps, et lui est identique en compteurs sur les
douze triplets différentiels. Qualification propre close : 78 CTests Release
et Clang ASan/UBSan, portes proposals et dispatch sous Clang TSan, 1 575
rejeux indépendants du front q2, dix mutants causaux tués, deux fixtures
minimales nommées, constantes du moteur d'avant la tranche gravées, 990
appels des cinq entrées contre l'oracle force brute, 684 mesures et 22
lectures/analyses normal/−O identiques. Fenêtre 2K petits facteurs, n ≥ 8 000 :
uniforme ×0,42 à ×0,54, amas ×0,51 à ×0,62, terrain ×0,64 à ×0,71, rangées
×1,01 à ×1,18 ; limite 16 et tous produits indiscernables ; 4K utile
seulement sur uniforme et amas aux grandes tailles. Builds épinglés :
`build/v8_front_proposals_20260917`, `build/v8_front_proposals_sanitize_20260917`,
`build/v8_front_proposals_tsan_clang_20260917`. Lire les
[reçus](receipts/q2_front_proposals_20260917/README.md). Quatre relecteurs
indépendants ont précédé le gel ; leurs constats (périmètre q2 seul,
régression du défaut corrigée, mutants et fixtures manquants, théorèmes du
registre, différentiel épinglé) sont consignés dans le PREFLIGHT des reçus.

## Dix-neuvième tranche publiée à 8d615cfd — résultat négatif, historique

Les [lots de singletons](docs/P0_LOTS_SINGLETON_Q2.md) sont implémentés dans
une entrée Coarse distincte (`run_wspd_q2_census_batched`) : feuilles
Shared/Individual seulement, contexte original et préfixe acquis copiés
dans un état privé de 72 octets, étapes Entry/Witness/Emit, lot vidé en fin
de seed et avant Pool, Pool et replis synchrones. Qualification propre
close : 75 CTests Release et Clang ASan/UBSan, porte Clang TSan, 595 appels
à lots et 178 Coarse contre oracle (10 958 paires, coquille 30), 172 mesures
et 26 lectures/analyses normal/−O identiques. **Le format régresse dans les
54 comparaisons à n8k/16k/32k** (lots / Coarse 1,005 à 1,225, médiane
1,112, quantum 64 ; ×1,29 à ×1,76 au quantum 1 en r0) et l'entrelacement ne
gagne rien sur une voie. Tous les comptes géométriques égalent Coarse.
Builds épinglés : `build/v8_singleton_batch_r3_20260917`,
`build/v8_singleton_batch_sanitize_r3_20260917`,
`build/v8_singleton_batch_tsan_clang_r3_20260917`, plus les builds r0 et r2
de leurs captures historiques. Lire les [reçus](receipts/q2_singleton_batch_20260915/README.md).
Ne pas rouvrir les lots compacts sans hypothèse nouvelle : le Shared
singleton était déjà une boucle sans allocation ni pile Z.

## Dix-huitième tranche publiée à2741d614 — historique

Le [partage des plages d'ancres](docs/P0_PLAGES_ANCRES_Q2.md) est implémenté
dans une entrée distincte. Moteurs et buffers privés réutilisables ;
aucune continuation par petite racine ; parents Pool immuables préparés
une seule fois puis possédés jusqu'au dernier consommateur. Le repli
Pool sans rejet garde le Shared global et partage ses ancres. La file
porte des valeurs de plage, pas une pile de49 cadres.

Qualification propre close :415 appels ranges/135 Coarse,
10 920 paires et778 110 sites de l'oracle passent, coquille30 et transfert
de bandes filtrées positivement exercés.72 CTests Release et Clang
ASan/UBSan, gate Clang TSan et174 mesures8k/16k/32k K5/10 s8/10/12
passent ; lecteurs/analyseurs normal/−O identiques. Builds
`build/v8_anchor_ranges_20260915`, `build/v8_anchor_ranges_sanitize_20260915`
et `build/v8_anchor_ranges_tsan_clang_20260915` désormais épinglés.
Ne pas les écraser. Lire les [reçus propres](receipts/q2_anchor_ranges_20260915/README.md).

Les temps Pool sont maintenant préparation plus intervalles actifs des
plages ; les sommes de maxima par créateur ne bornent plus la mémoire
simultanée des plans partagés. Lire les nouveaux champs de durée de vie.
Les six postes géométriques restent exactement ceux de Coarse ; aucun
gain stable sous la charge concurrente des qualifications/audits.
Les278 dons mesurés sont208 Shared et70 replis Pool ;52 après attribution
des seeds. Aucun don Pool filtré à ce grain64 dans les grands nuages,
mais115/101/103 dans les gates Release/ASan/TSan. La file porte72 octets
par plage, pas les6 272 octets d'une pile de continuation ; elle ne peut
pas répartir une ancre déjà commencée.

**Priorité énoncée à la clôture de cette tranche (depuis mesurée et fermée
par la tranche 19) : les millions de petites requêtes, pas une nouvelle
file.** Uniforme32k conserve10,181M plages,11,084M ancres et1,001Md visites
census, sans offre au grain64. Préparer des lots de singletons compacts
avec contexte original, rang d'ancre, stade et certificat frère dû,
puis payer la collecte complète. Borner la mémoire des lots sans limiter
la recherche. Réduire et mesurer le travail réel front/census avant GPU.
Sur les quatre séries Pool64, six postes sous×3 ; F rangées×4 et Pairwise
Pool amas jusqu'à×3,447. Quinze ratios de scheduling W4 dépassent×4.
O(A+R) pour la gestion des plages ne borne ni A(n), ni F, ni les sorties.
Aucun contrat FULL/G4 ni moteur q3/q4 nouveau. GCP non utilisé.

## Dix-septième tranche publiée à beee3341 — historique

Le [raccord à une équipe persistante](docs/P0_EQUIPE_PERSISTANTE_Q2.md)
est implémenté : une seule équipe pour seeds Coarse et branches q2 hors
Pool, même index possédé, compte/curseur/phase/B original conservés.
Descripteur et masse restent chargés une fois par rectangle, pas par
fragment. Pool et ses replis restent synchrones ; la nouvelle API est
explicite et ne remplace pas le défaut.

Les [174 mesures closes](receipts/q2_cooperative_20260915/README.md) gardent
exactement géométrie et supports.69 CTests Release et Clang ASan/UBSan,
ainsi que la gate Clang TSan, PASS. La reprise ASan/UBSan est close après
redémarrage, sans effacer l'essai incomplet. La gate compare235 appels
coopératifs,91 Coarse et5 744 paires
de l'oracle exhaustif. Aucun gain de vitesse stable ; les rangées sans
Pool régressent dans17 des18 observations malgré81 042 dons au total.

**Prochaine priorité : la granularité, pas une autre micro-variante de file.**
Partager des plages d'ancres non commencées permet de réutiliser le moteur
privé du receveur sans allouer une pile complète par ancre. Le parent paie
une seule fois sa préparation, notamment Pool ; les bandes gardent leur
permutation, pas les nœuds B. Garder aussi le repli Shared sans rejet.
Pour les petits rectangles, un singleton ne nécessite plus de pile B :
préparer un état compact avec contexte original, et payer toute la coquille.
Ces deux nouveaux formats ne sont pas implémentés par la tranche17.

Uniforme32k :11,084M racines,1,001Md visites census, facteur maximal10 ;
aucune continuation au seuil16. Rangées8k/Pool64 :78,8% des visites dans
un worker à cause du repli synchrone. Six postes sous×3 dans la campagne
Pool64 ne veut pas dire tous les coûts : F rangées×4, résidu Pool amas
jusqu'à×3,447, populations de témoins distinctes des opérations. Lire
les tableaux et la portée avant toute affirmation sous-quadratique.

Builds de cette tranche : `build/v8_cooperative_20260915`,
`build/v8_cooperative_sanitize_20260915`,
`build/v8_cooperative_tsan_clang_20260915`. Tous trois désormais épinglés,
ne pas les écraser. FULL, q3/q4 produit, GPU/G4 et massif restent ouverts.

## Seizième tranche publiée à897085f8 — historique

Le [détachement intérieur q2](docs/P0_DETACHEMENT_CENSUS_Q2.md) est implémenté
et qualifié :66 CTests Release/Clang ASan/UBSan, gate Clang TSan du répartiteur,
302 scénarios de détachement et2 304 appels parallèles. Les
[preuves propres](receipts/q2_census_split_20260915/README.md) comprennent144
mesures d'ancres et12 régressions q2 complet. Builds désormais épinglés :
`build/v8_census_split_20260915`, `build/v8_census_split_sanitize_20260915`,
`build/v8_census_split_tsan_clang_20260915`. Le préflight et les échecs de
compilation/lien/collecteur sont explicitement conservés ; ne pas les effacer.

Suite prioritaire : **une seule équipe persistante front+census**, pas une
équipe par ancre. Les144 mesures montrent pourquoi : seuls23 cas W4 ont
plusieurs workers actifs, et8 des72 comparaisons W1/W4 montrent un gain
ponctuel sous charge, sans qualification de vitesse. Les36 compteurs et
les quatre étapes restent identiques ; le q2 complet garde sa croissance
mesurée sous×3 sur les quatre familles. Les sauts de l'ancre sélectionnée
sont publiés et ne constituent pas une étude de croissance globale.

Quatre obligations à raccorder : produit du front, curseur de rectangle,
continuation Shared et curseur de bande Pool. Conserver le même index,
B original, compte/curseur/phase ; ne copier aucun historique ni créer
de root_start sur un fragment. Préparer Pool une fois par parent possédé.
Une bande Pool n'est pas un nœud B spatial : garder sa voie Pairwise,
et le repli Shared si aucune paire n'est retirée. Les petits jobs restent
locaux ; la fermeture/annulation doit couvrir toutes les obligations.

La file actuelle possède des continuations complètes à6 272 octets de
pile chacune ; Q+W borne les objets internes, pas le RSS ni les objets
créés par l'utilisateur. Le prochain format pourra ne garder que la
branche Entry et son contexte parental ; ce format compact n'existe pas
encore. Collecte atomique par support, pas de délai garanti par quantum.
Admission multiple impossible pour même arbre B/Z et règle actuelle :
preuve dans la note, ne plus chercher une fixture de ce cas ici.

Réponse Bbc9b2dc5 lue, six pins sources concordants ; ses504 960 appels
parallèles restent indépendants de nos preuves. Oracle entier B5124095b
disponible pour le prochain port q3/q4. FULL, GPU/G4 et massif restent ouverts.

## Quinze premières tranches — historique

La [continuation possédée d'une ancre q2](docs/P0_CENSUS_REPRENABLE_Q2.md)
est qualifiée :62 CTests Release/Clang ASan/UBSan, gate Clang TSan,
768 reprises contre référence et oracle,144 mesures d'ancres et12 de
q2 complet closes ; lecteurs/analyseurs normal/−O égaux. Les trois builds
`v8_census_resume_20260914`, `v8_census_resume_sanitize_20260914` et
`v8_census_resume_tsan_clang_20260914` sont épinglés, ne pas les écraser.
Le [bilan propre](receipts/q2_census_resume_20260914/README.md) distingue
strictement l'ancre sélectionnée du chemin q2 complet inchangé.

Prochaine brique : détacher un frère B encore non visité avec son propre
compte/curseur/phase et le même B original ; répartir les masses et les
compteurs déjà payés sans les répéter. La classe actuelle migre la pile
entière, pas ses frères simultanément. Elle réserve6 272 octets de cadres :
ne pas instancier ce stockage pour toutes les ancres à la fois. Aucun
Pool/joint/Pairwise asynchrone n'est raccordé. Conserver une préparation
Pool par rectangle parent, possédée et partagée, jamais par tranche A_i.
La collecte d'un support reste atomique, donc le quantum ne borne pas sa durée.

Lire la [proposition q3/q4](docs/Q3_Q4_OBJETS_ET_STRATEGIE_20260914.md) avant
le futur port : six contre-fixtures d'indépendance des voies sont exécutées.
Arête×bloc de complétions, lot de formes q3×Z et segment d'événements q4
sont les objets proposés, pas des moteurs déjà qualifiés. L'identité
`GΔ=P2B1−P1B2` permet un comparateur q4 direct i128, avec signes des
dénominateurs ; elle ne réduit pas automatiquement corde/clés/niveaux.
La réponse B7b86e36b est lue, ses preuves restent indépendantes.
P0 global, FULL, GPU et contrats G4 ne sont pas clos.

## Quatorzième tranche publiée à4e878754 — historique

La [redistribution des produits pendants](docs/P0_REDISTRIBUTION_FRONT_Q2.md)
est implémentée et qualifiée :57 CTests Release/Clang ASan/UBSan,
deux gates Clang TSan,32 différentiels et192 mesures closes, lecteurs
normal/−O identiques. Le défaut `Coarse` reste
inchangé ; `Donate` ajoute une file bornée avec poursuite locale lorsqu'une
offre échoue, et un réveil sûr des workers à l'annulation. Comparer sur les
données LiDAR déjà préparées, pas seulement les familles déjà équilibrées.
Ne pas écraser les trois builds workers historiques ci-dessous. Les nouveaux
builds portent le préfixe `v8_dynamic_front` et sont également épinglés,
y compris le GCC-TSan en échec conservé et sa reprise Clang distincte.
Le [bilan propre](receipts/q2_dynamic_front_20260914/README.md) ne montre
pas de gain général : ne pas promouvoir les médianes sans leurs étendues.
Le dispatch garde la géométrie, mais n'abrège pas un callback déjà engagé.
Suite : continuations de census possédées et plans parentaux partagés ;
aucune reprise depuis une nouvelle racine, préparation par A_i ou pointeur
vers la pile d'un worker fini. Mesurer aussi l'attente, aujourd'hui incluse
dans le temps de présence. q3/q4, FULL et GPU restent à construire.

## Treizième tranche publiée à b268cf6f — historique

Le [front distribué et ses workers q2](docs/P0_FRONT_WORKERS_Q2.md)
sont implémentés : 53 CTests Release/Clang ASan/UBSan et la porte
ThreadSanitizer passent, 32 configurations ancien/nouveau mono concordent.
La [campagne propre](receipts/q2_front_workers_20260914/README.md) est close :
134 mesures, lecteurs normal/−O identiques. Les trois builds workers
Release/ASan/ThreadSanitizer sont désormais épinglés ; repartir dans un
répertoire neuf. À 8k/K10 sur quatre cœurs physiques, médianes de trois
essais : uniforme5,387→1,379 s, terrain0,971→0,286 s,
amas2,994→0,770 s, rangées0,230→0,119 s. Ne pas mélanger avec
le corpus86 sur deux cœurs physiques/quatre SMT. À s8, les principaux
comptes restent sous ×4 à chaque doublement8k/16k/32k, sans preuve générale.
Chaque job conserve les masques hérités et l'identité du même index.
Les plans Pool et contextes d'ordre restent synchrones dans un worker,
pas dans une file de tâches empruntées. Les callbacks d'un même slot
sont séquentiels ; entre slots, leur état mutable doit être séparé.
La fermeture des threads précède tout retour ou propagation d'exception.
L'annulation est coopérative entre jobs, pas au milieu d'un gros job.
La géométrie et le travail total restent ceux du mono : la parallélisation
ne résout pas à elle seule la complexité. Prochaine implémentation :
redistribuer les produits encore pendants sans refaire leurs parents,
continuer localement si la file est pleine ; un réveil d'annulation est
indispensable avant les joins si des workers peuvent dormir. Conserver
la voie grossière comme référence, mesurer dons/refus/somme du travail.
Les gros callbacks restent un sujet distinct. FULL/G4 restent ouverts.

## Douzième tranche publiée à ba11e3ab — historique

Le [port Pool terminal](docs/P0_POOL_TERMINAL_Q2.md) est implémenté.
Les crédits appartiennent aux deux nœuds du même index, regroupés en au
plus K bandes disjointes. Ni nouveau nuage ni préparation par job, ni
préchargement de crédit dans le census global. Le plan n'est ni copiable
ni déplaçable ; seule sa durée synchrone est garantie pour l'instant.
La permutation B locale sert aux requêtes individuelles, jamais à Z.
Un filtre sans réduction retourne au parcours existant : les rangées
ont motivé ce repli, qui conserve le coût de préparation dans les reçus.

[Qualification et campagnes](receipts/q2_terminal_pool_20260914/README.md)
closes dans les builds désormais épinglés v8_pool_terminal_20260914 et
v8_pool_terminal_sanitize_20260914 : 49 CTests Release/Clang ASan/UBSan
PASS, 32 commandes par qualification, lecteurs normal/−O identiques,
80 mesures propres. Aucun temps hérité de fbbecc01.
La décision porte sur toute la chaîne q2, F et census/sorties inclus,
à K5/K10, s8/10/12 à8k et croissance s8 à16k/32k. Sur amas/K10,
13,412/47,179/184,306 s deviennent 3,589/7,614/19,180 s ; les visites
font ×2,958/×2,701 avec Pool, contre ×4,106/×4,229 sans. Le gain ne
s'étend pas à uniforme/terrain, où aucun plan n'est sélectionné. Le
travail mesuré est sous le quadruplement, pas une borne générale.
Après ce port,
prioriser front et petits rectangles ; un filtrage partiellement efficace
peut encore perdre face au partagé. Les emplois massivement parallèles
devront posséder et partager le plan parent, sans recopier B par job.
q3/q4, FULL, multi-CPU, GPU et contrats G4 restent ouverts ; GCP non utilisé.

L'audit A d608cc28 a été lu : Global sur les seules racines singleton
ne gagne pas de façon stable, ne pas intégrer cette variante. La
prochaine parallélisation doit partager le front et ses petits rectangles,
avec moteur/collecteur par worker et réduction des compteurs, pas seulement
les rares paires survivantes des gros plans Pool. Les tâches en vol
doivent posséder contexte et plan, et les temps mur/temps cumulés ne
doivent pas être soustraits entre eux. Proposition détaillée dans le
contrat de cette tranche, non encore une implémentation multi-CPU.

## Onzième tranche publiée à b2106c3c — historique

Le [census conjoint A×B](docs/P0_CENSUS_CONJOINT_Q2.md) est implémenté :
bornes 96 octets, compte/curseur/phase conservés à la reprise singleton.
SharedProduct divise A/B ; SharedAnchors ne divise qu'A avant le relais.
Aucun retrait de tout A, redémarrage de Z ou nouveau tableau de témoins.
Le défaut reste Individual. Les trois modes et les supports complets
sont confrontés à l'oracle sur 5 336 appels.

L'[essai initial](receipts/q2_joint_20260914/README.md) est épinglé :
Release47 PASS, Clang46/47, perte de stdout reproduite dans le collecteur
Python lors d'une interruption. Le correctif protège aussi les lectures,
pas seulement Popen ; six nouvelles fixtures complètent les six tests
de lancement. Ni les octets perdus ni l'échec ne sont masqués.
Les [captures corrigées r2](receipts/q2_joint_r2_20260914/README.md)
utilisent v8_joint_r2_20260914 et v8_joint_sanitize_r2_20260914,
sans changement C++ par rapport au premier gel. Les builds sont désormais
épinglés : 47 CTests Release/Clang ASan/UBSan passent, 27 commandes par
qualification, lecteurs normal/−O identiques et 44 mesures closes.
Sur amas8k/16k/32k, A seul prend 12,121/45,934/182,155 s ; les visites
après relais font ×4,107/×4,231. Ce n'est pas la réduction de croissance
cherchée. Le bras équilibré est pire à 8k, notamment sur les rangées,
car il fragmente B avant les certificats frère. Les trois s8/10/12
sont comparés à 8k, la croissance à s8 seulement. Ne promouvoir ni
un gain global ni le partage par défaut ; P0 reste ouvert.

**Priorité suivante : [Pool terminal → census global](docs/P0_POOL_TERMINAL_RACCORD.md).**
L'audit A fbbecc01 confirme le gain q2 complet sur amas/LiDAR50k,
dans un prototype séparé. Porter d'abord Pool/paires, propriétaire et
index uniques, rangs distincts des IDs, au plus K bandes résiduelles,
compte census nul. Pas de crédit préchargé, de scan A×B rejeté, ni de
plan recalculé par job. Le partagé local ne gagne pas de façon stable
dans l'audit ; le front et les petits rectangles restent dominants
après Pool. Ni ces preuves de prototype ni les tranches antérieures
ne qualifient automatiquement le futur port produit. FULL/G4 restent ouverts.

## Dixième tranche publiée à e3af11a7 — historique

L'[ordre complément/B original](docs/P0_ORDRE_TEMOINS_Q2.md) est implémenté
et qualifié localement. `ComplementFirst` garde le B original
et le rang de a dans un contexte immuable, puis transmet phase/cursor/count
aux enfants. Les ascendants de B original et de a sont divisés avant
toute borne ; a est exclu du seul comptage, jamais de la coquille.
Builds `v8_witness_order_20260914` et
`v8_witness_order_sanitize_20260914` désormais épinglés : 45 CTests
Release/Clang ASan/UBSan passent, détection des fuites conservée.
[56 mesures propres](receipts/q2_witness_order_20260914/README.md) :
s8/10/12 à 8k et croissance s8 en Complement/sibling. Temps q2
8k/16k/32k : rangées 0,201/0,437/0,942 s ; amas
11,378/43,750/173,471 s. Les visites des amas font encore ×4,106 puis
×4,229. Uniforme8k ne gagne pas en temps ; les visites retirées sont
en grande partie remplacées par des opérations structurelles.

**Priorité suivante : un census conjoint A×B**, puis reprise du chemin
actuel dès A singleton, avec le même compte/curseur/phase, sans racine
recommencée. Ne pas exclure tout A du compte : d'autres ancres peuvent
être intérieures. Lire la dernière section du contrat pour invariant,
fixture K1/K2 et compteurs de masses/tâches/bornes conjointes. Les
preuves de bornes A §9 existent, pas encore ce raccord produit. Garder
le coût plus élevé d'une borne conjointe dans la comparaison. L'audit A
publié à a1ee8cb0 contre-vérifie l'ordre actuel et propose aussi une
comparaison isolée de l'ancien ordre sur les seules racines B singleton.
Ne pas transformer les enfants déjà crédités par ce raccourci.
La représentation de contexte doit devenir possédée avant une file
asynchrone CPU/GPU. Aucun contrat FULL/G4 acquis ; GCP non utilisé.

## Neuvième tranche publiée à 39b58f37 — historique

L'option [certificat autonome du frère](docs/P0_CERTIFICAT_FRERE_Q2.md)
est implémentée dans le seul raccord WSPD q2. Elle n'additionne aucun
crédit et n'avance jamais Z ; le défaut reste Disabled. Les builds
`build/v8_sibling_r2_20260914` et `build/v8_sibling_sanitize_r2_20260914`
sont maintenant épinglés : **44 CTests Release/Clang ASan/UBSan passent**,
lecteurs normal/−O et [32 mesures](receipts/q2_sibling_20260914/README.md) clos.
Les premiers essais de qualification sont conservés en échec : course
de collecte du runner à l'interruption et LeakSanitizer bloqué dans le
sandbox. Le runner corrigé passe six contre-tests déterministes de
lancement/interruption/session en plus des 41 contrôles historiques.

Le gain est réel sur les rangées : 0,241/0,481/0,969 s à n8k/16k/32k,
s8/K10, et croissance des visites+tests frère ×2,20 puis ×2,13. Le
temps de référence apparié 8k est 1,494 s. Les amas restent presque
quadratiques : évaluations ×4,29 puis ×4,22, 214,90 s à 32k. Pas de
gain universel ni de borne globale ; ne pas activer ce mode par défaut.
Les comparaisons s10/12 nouvelles sont limitées à 8k, la croissance à s8.

**Prochaine expérience prioritaire : ordre complément/B original et
exclusion de l'ancre du comptage seulement**, selon la dernière section
du contrat. État compact phase/curseur/compte, contexte original commun
aux enfants ; raffinement structurel des ancêtres du groupe différé et
de l'ancre avant toute consommation. Tester la fixture 3D B cube4³,
a=(1000,1000,1000), douze W proches de a, puis les quatre régimes et
les données LiDAR. Ne pas remettre une liste de frontières avant cette
variante minimale. Le partage A×B×Z, les reprises distribuables et les
workers CPU/GPU restent distincts ; les contrats FULL/G4 ne sont pas acquis.
L'auditeur A a publié la preuve K−c et les limites LiDAR à 5c32ab95 ;
pas de port de cette micro-variante sans gain net justifié. GCP non utilisé.

## Huitième tranche publiée à f7edd646 — historique

Le [raccord q2 global](docs/P0_FRONT_ET_CENSUS_Q2.md) traite directement
les nœuds WSPD du même index, sans factory, copie B ou arbre local.
Pairwise et Shared comparent les mêmes supports sur le même nuage.
Le plus petit facteur fournit les ancres ; le groupe opposé hérite du
compte et du curseur DFS à chaque subdivision. Le front demande q2 seul,
pas les trois voies de la capture historique. Les temps englobent front,
census, collecte/callback et destructions : ne pas les appeler census isolé.

Le [reçu de cette tranche](receipts/wspd_q2_census_20260914/README.md) conserve les
tests et mesures de cette tranche. Le gate indépendant confronte 1 255
appels et 46 762 supports complets, avec 60 subdivisions après crédit.
43 CTests Release et Clang ASan/UBSan passent sur ces sources historiques.
Les builds `v8_front_census_20260914` et
`v8_front_census_sanitize_20260914` sont désormais épinglés. Les 53
mesures et lecteurs normal/−O sont clos ; repartir dans un build neuf.

**Priorité : diminuer le travail par ancre et celui du proposeur.**
Le raccord supprime une reconstruction mais conserve Σmin(|A|,|B|)
démarrages et beaucoup de visites Z. Sur amas8k/16k/32k à s8, ces visites
font 0,973/4,199/17,665 milliards, soit ×4,31 puis ×4,21, pour des
supports proches du linéaire : le critère de croissance demandé échoue.
Étudier un état portant encore deux groupes (A,B,compte,curseur Z), mais
agir aussi sur le choix de Z : la contre-fixture collinéaire a=1000,
B=0..63 montre que le DFS croissant fragmente B avant tout crédit.
Avec A singleton, agrandir seulement l'état en A×B×Z ne résout rien.
Expérience minimale contre-vérifiée : au split de B, tester son frère
comme certificat autonome de K témoins stricts pour chaque enfant.
S'il échoue, ne rien accumuler et reprendre compte/cursor inchangés.
Ce test de coût constant par enfant reste à implémenter et qualifier.
Un compte ne se réutilise qu'avec son préfixe consommé et son index précis. La
propagation de témoins de B est une autre proposition, pas un gain de
temps acquis pour le produit. Le test de lentille a un rendement faible
dans son nouveau prototype (0–1 % de recherches évitables).

L'auditeur A a publié ses données LiDAR et prépare la mesure q2 sur ces
entrées ; son travail reste indépendant, avec ses propres sources/reçus.
Le terrain synthétique ne remplace pas ce corpus. Ne pas confondre retrait
de Xi, amélioration du front et partage du census. Reprises distribuables,
workers CPU/GPU, q3/q4 et FULL sont encore absents ; contrats G4 ouverts.

## Septième tranche publiée à da366f7f — historique

Le [premier front WSPD réel](docs/P0_FRONT_REEL.md) manipule les nœuds du
même index Z et leur permutation. Aucun plan Pool/Axis ni parcours de
facteur n'est préparé par produit. La convention de séparation est
`box_gap_diameter_v1`, la comparaison `Pure`/`MidpointSamples` conserve
les entrées, pas nécessairement les descripteurs. Les rejets sont propres
à chaque voie et hérités ; les comptes partiels ne sont pas hérités.
Les preuves et coûts sont dans le [nouveau reçu](receipts/wspd_front_20260914/README.md).

40 CTests Release/Clang ASan/UBSan passent. La mesure uniforme32k/s8
réduit le front de 56,8 à 20,9 millions de rectangles, mais passe de
4,87 s à 37,4 s : le proposeur est trop cher (954 millions de pas
d'index). Ne pas promouvoir `MidpointSamples` comme gain de temps
acquis, ni retirer Pure de la comparaison du futur chemin consommé.
Les 72 mesures sont closes : quatre familles, n8k/16k/32k, Kmax10,
s8/10/12, deux modes. Les tâches croissent sous ×4 dans ces mesures,
mais les résidus Samples des amas font presque ×4, comme q3/q4 sur les
rangées. P0 n'est pas clos. Les deux builds `v8_front_20260914` et
`v8_front_sanitize_20260914` sont épinglés ; reprendre dans des builds neufs.

**Priorité immédiate : consommer directement le front par le census q2.**
Ne pas reconstruire une factory de rectangle, Axis ou un arbre B par
descripteur. Garder les nœuds et leur ordre spatial, factoriser l'état du
census global et réutiliser ses tampons. Comparer à l'exhaustif les vrais
supports, intérieurs et coquilles, puis mesurer le coût complet aux trois
tailles. Le proposeur recommence encore depuis la racine : test préalable
de lentille et certificats partiels distincts sont à comparer sur ce chemin.

q3/q4 nécessitent aussi une réduction collective : deux rangées parallèles
laissent m² candidates transversales même avec tous les témoins ponctuels.
Ne pas développer ce résidu par défaut, ni conclure qu'il s'agit de m²
supports utiles. La génération canonique par plus longue arête et la
rétention par boule restent requises ; un rejet q4 ne tue pas q2.
Les tâches par handles permettent une distribution future, mais le moteur
courant est mono-thread. FULL, multi-CPU, GPU et contrats G4 restent ouverts.

L'auditeur A prépare le régime LiDAR simple et superposé. Le `terrain`
actuel est un slab aléatoire mince, pas un simulateur de capteur : ne pas
choisir une variante pour le LiDAR sur cette seule mesure. Ajouter un
corpus déclaré, poses/quantification et collisions explicites, en séparant
densification de la même zone et extension du trajet. Ses captures en
préparation restent indépendantes de la présente qualification.

## Sixième tranche publiée à 85015a8c — historique

La sixième tranche partage [le nuage et l'index Z](docs/P0_NUAGE_ET_INDEX_PARTAGES.md)
entre rectangles et seuils. 37 CTests passent en Release et Clang ASan/UBSan.
Les copies globales répétées et le scan des facteurs pour leurs boîtes
sont retirés du nouvel appel partagé. L'ancien adaptateur reste coûteux
par construction et ne doit pas alimenter un front WSPD. Les builds
`v8_cloud_20260914` et `v8_cloud_sanitize_20260914` sont réservés à ces preuves.

Les [66 essais clos](receipts/cloud_reuse_20260914/README.md) couvrent
8k/16k/32k, Kmax10, s8/10/12 à R32, puis R32/64/128 sur grille/déséquilibré.
À 32k/R32/s8, partage : grille 457–465 ms, déséquilibré 87–95 ms,
nappe environ 16 s. Les mêmes sorties et compteurs locaux sont vérifiés,
les préparations globales sont divisées exactement par R. En revanche,
les copies de restrictions grilles font ×3,94 puis ×3,97 quand R et n
doublent : ce verrou n'est pas résolu. La subdivision garde 35,6 millions
de candidates sur nappe32k. Tous les chronomètres, y compris un doublement
grille à ×5,62, sont conservés ; les temps restent bruités. Les builds sont
désormais épinglés. Aucun résultat ancien n'est réattribué à ce moteur.

**Priorité immédiate : pilote de front réel avec coût cumulé des facteurs.**
Ne pas prendre le partage du nuage pour une solution complète : Pool/Axis
gardent Ω(R|B|) sur une partition A_i×B et le découpage peut perdre des
témoins utiles aux minorants. R croissant avec n doit rester dans les
tests. Faire porter permutation et boîtes par les nœuds spatiaux certifiés ;
l'arbre de plages originales n'est qu'un raccord de compatibilité.
Tester les rejets sur produits ancêtres avant séparation complète et un
chemin sans tris/allocations lourds pour petits facteurs. Le nouvel audit
du front pur v4 montre leur nombre élevé, sans qualifier une WSPD v8.
Fixer explicitement la convention de s avant la comparaison s8/10/12.

Le seuil du census vient du rectangle du plan, l'index n'en possède plus.
Les restrictions restent liées au même rectangle exact. Les ordres B/Z
ne sont pas interchangeables ; l'auditeur précise en §9.4 pourquoi une
couverture compacte dans un ordre peut devenir linéaire dans un autre.
Raccorder Pool sans axe imposé, puis suspension mono, workers CPU et GPU.
Les continuations doivent suspendre sur saturation, jamais tronquer.
q3/q4 et FULL restent ouverts ; génération par plus longue arête et
rétention par boule (union des voies qui la conservent) sont inscrites au
plan. GCP non utilisé ; aucun contrat de tour n'est acquis.

## Cinquième tranche publiée à 3c29ea1e — historique

La cinquième tranche est qualifiée : [bornes préparées](docs/P0_BORNES_PREPAREES_ET_PARALLELISATION.md)
de 48 octets, 34 CTests Release/Clang ASan/UBSan, oracle de 1 424 cas et
comparaison des révisions sur [124 mesures](receipts/q2_prepared_bounds_20260914/README.md).
Compteurs et sorties inchangés ; baisse exploratoire du total Shared
de 4–10 % sur les médianes répétées grille/nappe32k/K10/s8. Les temps
doublent par 1,58–2,41 sur ces familles, sans borne générale ni gain
garanti sur tout nuage. Les deux builds `v8_prepared_bounds_20260913`
et `v8_prepared_bounds_sanitize_20260913` sont épinglés.

**Prochain chantier : séparer nuage/index global et contexte de rectangle.**
Ne pas recopier et revalider n sites pour chaque rectangle WSPD. Préserver
les trois compatibilités : même nuage pour index/requête, même contexte
rectangle/seuil pour crédits, même permutation B et index Z précis pour
plages/continuations. Porter les contre-fixtures de l'auditeur avant de
relâcher les contrôles de propriétaires actuels. Raccorder Pool seul par
les préfixes de classes (§9.2 de son audit), sans forcer le filtre axial.
Puis expliciter les transitions du curseur pour suspension mono testée,
workers CPU et enfin GPU ; des files pleines suspendent, jamais ne tronquent.
Partager l'index ne borne pas la somme des tailles de facteurs ni les sorties.

La déduplication des boules doit précéder les collectes répétées tout en
conservant les incidences des supports. q3/q4 et la tranche FULL minimale
restent prioritaires : ne pas repartir dans une série de micro-variantes
de la seule borne q2. Les contrats 50k portent sur la tour entière sur
G4 ; mono local, G4 et massif sont des qualifications distinctes.
GCP non utilisé.

## Quatrième tranche publiée à f4815cd4 — historique

La quatrième tranche implémente le [census q2](docs/P0_CENSUS_Q2_PARTAGE.md)
sur les résidus compacts. Le parcours partagé emploie les échappements
DFS proposés par l'auditeur : compte uniforme et curseur, sans arena de
continuations. Il est comparé au parcours individuel sur le même index de
tous les sites. La seconde collecte émet réellement intérieurs, coquille
et support/clé entière ; son temps et son callback sont inclus. Le gate
géométrique passe 277 cas ; les 31 CTests Release/ASan/UBSan et les lecteurs
normal/−O passent. Les [204 mesures](receipts/q2_census_20260913/README.md)
ferment quatre campagnes, avec 164 configurations distinctes. Les builds
`v8_census_20260913` et `v8_census_sanitize_20260913` sont épinglés.

Résultat : l'intersection paie son coût sur les grilles (environ 131 ms
à n32k/K10 avec census individuel, contre 3,74–3,79 s après Additive seul).
Sur nappes32k, le census partagé réduit les visites mais reste plus lent.
À 50k/K10, individuel après intersection : 176–186 ms sur grilles,
4,05–4,09 s sur nappes ; K5 sur nappes reste à environ 1,55 s. Ce ne sont
pas des tours FULL. Les doublements 8k/16k/32k sont inférieurs à ×4
dans ces familles ; aucune borne globale n'en découle.

Suite mono bornée : préparer les constantes de bornes `(a,B)` une fois
par tâche, vérifier les mêmes décisions/visites/sorties puis mesurer le
temps complet. Éviter de choisir automatiquement Shared sur les petits
résidus. Le coût Pool seul doit encore être comparé via une interface de
résidu non liée à l'axe. Ne pas prolonger ces variantes au détriment du
propriétaire global WSPD et de la tranche q3/q4/FULL minimale.

La déduplication globale des boules, q3/q4, le partage du propriétaire
sur une vraie WSPD et les parents FULL restent distincts. Ne pas reprendre
un compte saturé à un seuil supérieur comme s'il était exact. L'ancien
parcours à listes n'est pas conservé comme moteur parallèle. GCP non utilisé.

## Troisième tranche publiée à f5430f57

La troisième tranche ajoute le mode `Additive`, son intersection intégrée
avec un plan local q2 et la suppression des allocations actives aussitôt
remplacées. Lire le [contrat](docs/P0_ADDITION_ET_INTERSECTION.md) et les
[648 mesures](receipts/additive_q2_20260913/README.md). 26 CTests Release
et Clang ASan/UBSan passent, incluant 650 plans confrontés au nouveau juge,
frontières, cœur, copie de restriction, 7 contre-modèles et 15 rejets d'API.

Résultat précis : la nappe complète n32k/K10 garde 3 928 390 candidates
contre 6 483 670 auparavant, mais la sélection additive seule coûte
environ 238 ms contre 57 ms. Sur la grille, l'intersection Pool garde
114 716 candidates en 34–35 ms ; Pool seul garde 378 840 candidates en
2,4–2,6 ms. Ne pas confondre moins de candidates et moins de temps total.
La croissance mesurée à 8k/16k/32k est inférieure au quadratique dans
ces familles, pas une qualification générale. Aucun choix automatique
de filtre ni changement du mode `Independent` par défaut n'est installé.

**Prochain jalon : payer le census q2 et partager ses requêtes par blocs.**
Il doit indexer tous les sites, distinguer intérieurs stricts et coquille,
conserver les IDs et canoniser les boules. Comparer une référence de
requêtes individuelles avec le traitement partagé, coût total inclus.
L'auditeur a livré les [bornes sur trois boîtes](audits/P0_SOUS_RECTANGLES_ET_GROUPES.md#9-census-q2--des-extrema-exacts-pour-partager-les-recherches)
et un [prototype de consommation indexée](../audits/morsehgp3D_v8_complementaire/P0_CONSOMMATION_INDEXEE_Q2.md).
Ce dernier épingle `8e406f9b`, pas les nouvelles sources : ses grandes
mesures sont exploratoires. Ne pas précharger le cœur puis revoir ses IDs.

Les rangs filtrés B proposés pour composer deux plans déjà construits
restent une alternative au parcours intégré. Les fenêtres A/B peuvent
encore réduire le résidu des nappes ; ne pas retarder le census et la
tranche FULL minimale derrière une optimisation sans fin du seul cas axial.
Le vrai partage du propriétaire/index sur la WSPD, q3/q4, parents FULL,
parallélisation et contrats 50k/massif restent ouverts. GCP non utilisé.
Les builds `v8_additive_20260913` et `v8_additive_sanitize_20260913` sont
épinglés ; repartir dans un nouveau répertoire de construction.

## Deuxième tranche publiée à 8e406f9b

La deuxième tranche implémente `CreditBatch` (trois voies géométriques,
pas une tour K) et `AxisQ2Plan` (colonnes exactes + plages de l'index B).
Vingt et un CTests passent en Release GCC et Clang ASan/UBSan ;
[594 mesures appariées](receipts/shared_axis_20260913/README.md) couvrent
8k/16k/32k, Kmax5/10, s8/10/12 et deux ordres sur le même propriétaire.
Lire le [contrat expliqué](docs/P0_PARTAGE_ET_FILTRE_AXIAL.md).

Acquis bornés : préparation Tubes divisée par trois à plans identiques ;
résidu q2 des nappes complètes passant de 256 millions à 6,48 millions
de paires à n32k. Le filtre axial coûte environ 57 ms, hors propriétaire,
sans expansion/census. Il peut faire moins bien que Pool sur les cubes,
et une rotation peut lui faire conserver toutes les paires. Aucun
algorithme général ni contrat de tour n'est acquis.

Suite prioritaire : additionner les colonnes exactes disjointes dans les
requêtes d'index, supprimer les tableaux initialisés puis remplacés,
comparer queues/fenêtres A/B et intersections de résidus. Puis implémenter
le consommateur q2 exact avec un index sur tous les sites, pas un scan
de n sites pour chaque paire ; ne pas recompter le cœur implicitement.
Le partage de validation sur toute la WSPD reste à faire. Les groupes
collectifs q3/q4 restent une piste distincte à qualifier avec l'auditeur.

Les pannes d'affectation de plans/batches sont désormais injectées et
couvertes par la garantie forte. Les reçus appariés vérifient sources,
matrices, identités, convention de checksum et provenances déclarées.
Les captures préliminaires exclues sont conservées avec leur motif.
Les nouveaux builds `v8_shared_axis_20260913` et sa variante sanitizer
sont épinglés ; ne pas les écraser. Aucun fichier v6/v7 ni statut formel
n'est modifié par cette tranche. GCP non utilisé.

## Première tranche publiée : historique à 3589a2c9

La bibliothèque `mhgp8_p0`, ses deux gates C++ et sa sonde sont implémentées.
Pool/DualBlocks/Tubes produisent des crédits sûrs et un résidu compact
sur un seul rectangle séparé. Huit CTests passent en Release GCC et en
Debug Clang ASan/UBSan ; 729 mesures mono sont conservées avec sources,
commandes, entrée et compteurs. Voir le
[contrat](docs/P0_CREDITS_LOCAUX.md) et les
[résultats](receipts/p0_local_credits_20260913/README.md).

Trois défauts d'audit sont corrigés avant livraison : le propriétaire
n'est plus copiable/réaffectable derrière un plan, ses coordonnées sont
copiées dans un stockage privé sans alias mutable hérité, et les reçus sont
confrontés strictement aux commandes avec sorties brutes et hashes de
fermeture. La première capture, antérieure aux correctifs, reste dans
`first_pass_pre_owner_fix/`, la deuxième dans `second_pass_pre_alias_fix/` ;
seules les nouvelles captures r3, copie des coordonnées incluse, font autorité.

La préparation quadratique systématique a une alternative effective :
les tubes coûtent O(m log m) par facteur. Mais **ni cette borne locale,
ni les petites latences ne ferment P0**. Les nappes gardent toutes les
paires ; sur les grilles q3/q4, les tubes sont rapides mais créditent
moins que DualBlocks. Le pool global peut rater les régions utiles :
contre-fixture des rails à 2 718 sites, q4, 1 846 881 paires contre 2 916
pour DualBlocks et Tubes. Ne pas généraliser un gagnant unique.

Suite prioritaire : partager le tri des tubes entre voies, partager la
validation du nuage, puis raffiner les produits résiduels difficiles en
préservant couverture et IDs. Il faut tester notamment les nappes, où
l'universalité sur toute la boîte opposée est trop restrictive. Une
combinaison de crédits utilise leur maximum, jamais leur somme sans
disjonction. Le cœur n'a pas encore ses IDs exportés : ne pas le recompter
implicitement dans le census. Mesurer ensuite le consommateur exact q2
minimal avant d'étendre census/FULL et la parallélisation.

Le vrai constructeur WSPD et sa comparaison s8/10/12, les voies complètes
q3/q4, census, parents et tour FULL restent à implémenter. Les benchmarks
actuels n'effectuent pas ce travail. Les builds v8 datés du 13 septembre
et les reçus sont épinglés. Aucun statut formel ni fichier v6/v7 modifié
par cette tranche ; aucun GPU/GCP utilisé.

## Historique de l'audit d'ouverture

La demande remplace l'optimisation incrémentale v7 par un audit complet
avant reconstruction. La base publiée examinée est main `dc57ffd5`.
Les modifications v6/v7 déjà présentes sont conservées, sans les inclure
dans cette ouverture v8. Le delta privé fused_history du 11 septembre
reste distinct des résultats publiés ; sa publication n'est pas poursuivie
dans ce changement de cap. Aucun processus de benchmark restant constaté
à l'ouverture du 13 septembre ; GCP non utilisé.

L'audit d'ouverture est rédigé : [synthèse](docs/AUDIT_V7_SYNTHESE.md),
[exposé pédagogique](docs/ALGORITHME_EXPLIQUE.md), quatre rapports détaillés
et [plan de refonte](docs/PLAN_DE_REFONTE.md). Il couvre WSPD et témoins,
supports q2/q3/q4, census, rattachements, histoires, verticales,
parallélisation, mémoire, tests et livraison. Sa portée n'est pas une
certification ligne par ligne de tout le corpus ni une réexécution C++.

Décisions principales : conserver le contrat FULL avec vrais parents,
partager les objets géométriques, préparer les rattachements indépendamment
de l'histoire, découper l'intérieur des gros rectangles, reconstruire
les histoires par contractions parallèles, puis réemployer les marques et
index. Le détail distingue explicitement régularité, coquilles, sorties
quadratiques et propositions encore sans qualification.

Les contrats restent ouverts : 50k 1..10/1..5 en environ 419/34 s sur les
dernières complétions publiées, pas de nouvelle capture 50k des prototypes
récents, pas de chaîne intégralement GPU, pas de dizaines de millions.

L'inventaire épingle la base publiée et les deltas locaux. Six lecteurs
de reçus clos et deux recalculs rationnels ont passé ; les contrôles de
livraison sont dans [PUBLICATION_CHECKS](receipts/audit_v7_20260913/PUBLICATION_CHECKS.json).
Le contrôleur documentaire couvre désormais la v8 et possède un test
positif/négatif de découverte, sans inclure les futurs audits indépendants.
Aucun statut formel modifié, aucun moteur compilé, GCP non utilisé.

Suite réordonnée sur demande explicite du 13 septembre : **P0 d'abord,
supprimer la préparation systématique O(|A|²+|B|²)** après l'échec des
témoins universels. Comparer minorants issus de petits ensembles,
parcours conjoints de blocs, requêtes géométriques groupées et sélection
directe de sous-produits ; aucune solution n'est imposée. Le
[plan](docs/PLAN_DE_REFONTE.md) fixe les preuves et mesures de travail
total, coût aval compris, avant les campagnes CPU/GPU. P0 reste ouverte.

Avant tout port, lire aussi [VERROUS_ARCHITECTURE](docs/VERROUS_ARCHITECTURE.md).
La note conserve les cinq obstacles suivants : recherches spatiales
répétées, triangles de départ × voisinages q3/q4, MEB/descentes répétées,
histoire/export centraux, puis résidence et échanges CPU/GPU. Elle fixe
les sources, nuances et critères de validation pour le futur développeur.
P0 garde son premier rang ; B1–B5 restent ouverts et ne sont pas des
impossibilités intrinsèques de HGP. Aucun nouveau résultat n'est revendiqué.

La tranche FULL minimale et les petits juges servent cette comparaison ;
ne pas la repousser derrière un port général de la v7. Garder les tailles
8k/16k/32k et s8/10/12. Aucun moteur n'est modifié par cette décision.
Ne pas reprendre le chantier fused v7 comme si le changement de cap
n'avait pas eu lieu. Le
[journal v8](../audits/COORDINATION_MORSEHGP3D_V8.md) porte les questions
à l'auditeur indépendant et la coordination d'index.
