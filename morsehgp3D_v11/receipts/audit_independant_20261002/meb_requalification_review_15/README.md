# MEB3 : requalification bornée G4 — 2 octobre 2026

**Qualification recoupée : MEB3**, source exécutée
`25792084eb4e672c5222d62f5b2ae87bd2ee4948`, publiée par `9a5fd6a61`.
MEB2 est une tentative de démarrage refusée, distincte. Aucun produit natif,
build ni appel GCP n'a été exécuté par cet audit. Les copies ont été prises
avant lecture en /tmp puis déplacées dans cette capsule après publication
par la racine : [avant](sources_before.json), [sources et paquet](package_sources_before.json),
[stockout](stockout_before.json), [après](sources_after.json). Toutes sont stables.
Le [raccord publication/source](publication_mapping.json) conserve la distinction
avec le port ultérieur `7f1922c77`, non qualifié par cette campagne.

La matrice passe **1 266/1 266** portes : Release B18 270, ASan24/TSan21/
profils21/24 chacun 195, poison B21 196, mutants 18, style deux. Clang absent.
Le complément ASan18 `num;index;tower` passe **55/55**. Le lecteur exige les
unités tower, leurs compteurs, les oracles Fraction et les portes IO normal/−O.
Les 372 contrôles natifs comprennent les huit groupes d'unités (339) et
starvation (33). L'oracle Gram/Fraction produit par profil 350 requêtes,
14 631 contrôles, 31 permutations et 18 refus. Son modèle pur affiche
43 800 contrôles, 27 corruptions et trois JSON malformés, normal/−O.

Les 141 mutations ont 136 morts par code, trois par ligne attendue et deux
refus de construction core attendus ; aucun signal/délai pris pour une mort.
Les dix mutations tower sont mortes par leur juge. Ce compte de mutations
reste distinct des 18 portes CTest du harnais. Les sorties LastTest complètes
sont conservées et rejugées ; les [provenances binaires](binary_pins.json)
conservent profils effectifs, caches/flags, tailles et hashes. Les exécutables
natifs ne sont pas archivés ; leurs hashes enregistrés sont recoupés, pas
recalculés à partir d'exécutables absents.

Le [paquet](raw/package/package.tar.gz), 26 447 522 octets, SHA
`a7efa34d4935d5a574acc9c041b7a00d245b56210204ddb20089581abc3fd63e`,
contient exactement **2 578 fichiers Git 257**, 46 069 010 octets expansés :
[inventaire](package_git_inventory.json). L'archive a 122 fichiers, 4 595 381
octets expansés ; [son manifeste](extraits/results/MANIFEST.sha256) couvre
exactement les 121 autres fichiers, hashes vérifiés. Le paquet exécuté
contient bien le sérialiseur réparé. Entre ab04 et 257, aucun src/CMake ne
change ; la réparation du banc est distinguée du calcul mathématique MEB.

Le [banc](measurements.json) passe **18/18 essais**, aucune omission/refus/
temporisation, une répétition : six entrées entières × profils18/21/24,
48 parties choisies de tailles 1..12 par essai. Les 864 premières requêtes
donnent 216 census complets et 648 saturés ; six comparaisons sont identiques
en sémantique et travail logique. Les mêmes coordonnées u18 à 1 mm servent
aux trois profils. Le banc utilise des poids de site unitaires ; les trois
LiDAR sans sol, 39 885 / 35 551 / 45 845 sites, sont de la seule séquence 08.
Les sorties canoniques ont été supprimées : leurs hashes publiés sont
comparés ; le décodeur ne relit pas des payloads canoniques absents.

Les compteurs décrivent le premier MEB/census ; le wrapper recommence les
deux. On obtient 170 352 présentations MEB dans les premières séries,
340 704 en incluant leur répétition par le wrapper. Chaque premier census
paie deux passes, répétées dans le wrapper. La première population reste
vivante : le pic du wrapper contient les deux sorties. Les temps séparent
MEB, census, wrapper, scan de référence, préparation et processus complet ;
le wrapper ne se déduit pas en additionnant les autres durées.
Pour les LiDAR u21, les sommes des 48 intervalles MEB valent 1,116–1,178 ms,
les wrappers 1,377–1,777 ms, mais les processus complets 49,833–67,324 ms,
dont les références 44,064–61,467 ms. Le décodage Python est payé séparément.
Ce sont des parties choisies, pas des descentes ni une mesure de tour FULL.
Le scan global partage num::side ; l'oracle Fraction indépendant est celui
des petites fixtures. Aucun nouveau contrat FULL/GPU/100 ms n'en découle.

Les groupes ferment avec codes **0/0/0**, murs **172,627/14,303/1,469 s**,
sans interruption, troncature ni groupe résiduel tué. Le [reçu de session](session_facts.json)
lie la génération `2026-10-02T12:11:26.693−07:00` à la cible TERMINATED,
arrêt `12:17:57.793−07:00`, clés et réserve retirées. Cela ne constitue pas
une certification d'isolation des temps mesurés.

Les échecs restent séparés : **MEB1** source ab04, 1 254/1 266 + 53/55,
zéro grand banc ; **MEB2** source 257, worker non lancé, contrôleur conservé
`shutdown_uncertified/generation_unknown`. Sa vérification externe constate
ancienne génération inchangée, start DONE en stockout et aucun start en
attente ; elle n'attribue pas un arrêt de génération nouvelle à MEB2.

[Huit replays Python](logs/commands.json) passent normal/−O : lecteur des
trois états, modèle du lecteur (20 positifs / 79 corruptions), modèle stockout
(deux positifs / 26 corruptions), [contrôle portable](review.py), tous sans
natif/cloud. L'adaptateur portable ne change que les chemins des métadonnées
brutes vers leurs copies octet-identiques ; les vérifications restent actives.
Le code 0 du lecteur signifie preuve cohérente, avec deux tentatives en échec
et une campagne conforme, jamais trois campagnes conformes.

[LEDGER.json](LEDGER.json) exclut seulement lui-même et SHA256SUMS racine.
[SHA256SUMS](SHA256SUMS) couvre tous les fichiers sauf lui-même, y compris
l'inventaire imbriqué. Aucune capsule précédemment fermée n'est modifiée.
