# Index global : campagne et protocole recoupés — 2 octobre 2026

**Source native exécutée : e8520481d1745627e156723ad995ac5175a8163f ; publication : 356cbdf883c4196697f09aaef3444daa114482e9.** La campagne index1 close est cohérente : matrice principale 1 149/1 149, complément ASan/UBSan B18 36/36 séparé, 18 processus de banc réussis. Elle qualifie les configurations et requêtes jouées de l'index/census CPU. Aucun catalogue raccordé à cet index, descente MEB, FULL, GPU, contrat de 100 ms ou massif n'est qualifié par ce banc.

## Gel et provenance

[Sources avant](sources_before.json) : 25 blobs Git e852, lecteurs et fixtures LIVE, campagne initialement non versionnée, reçu original et artefacts de session ; 48 pièces copiées. La présence d'index1 n'a pas été considérée seule comme une qualification : ses hashes, archive et juges sont recoupés ci-dessous. [Sources après](sources_after.json) : tous les fichiers figés et les artefacts de campagne sont intacts ; le checkout développeur est désormais propre à la publication 356cbdf88. DEVELOPMENT, INDEX, README des tests et le selftest du lecteur ont évolué pendant la revue. Ces modifications restent identifiées par leurs hashes : les replays de cette capsule portent sur la version figée, pas sur le selftest ultérieur. Le lecteur de campagne est resté identique.

L'[archive du paquet original](raw/package/package.tar.gz), 8 141 822 octets, SHA `ba6024599b8e63c6a11d0e4a0d99b40177d7cd4aa0ea53d3438efe2431a84c45`, correspond exactement à `git archive e8520481d morsehgp3D_v11 gcp-migration/v11_worker.sh` : 2 076 fichiers, 21 400 435 octets de contenu. Le [manifeste comparatif Git](package_git_manifest.json) conserve noms, tailles et hashes ; la sonde les vérifie tous sur le paquet copié. Les plans JSON/worker et le manifeste des données sont raccordés au [reçu original](raw/index1_receipt.json).

L'[archive unique de résultats](copies/morsehgp3D_v11/receipts/index_20261002/index1/results.tar.gz) conserve 122 fichiers : 121 entrées MANIFEST.sha256 vérifiées, plus le manifeste lui-même. Le lecteur vérifie aussi hash gzip, taille comprimée/décomprimée, absence de chemins dangereux/doublons, copies des résumés, et égalité reçu compact/original. Les entrées entières, IDs, masque et coordonnées sont ceux du manifeste q4levels1, identique octet pour octet ; les données KITTI brutes ne sont pas dupliquées dans cette capsule.

Même génération start/closing/observed_after : `2026-10-02T10:10:56.873-07:00`. Arrêt observé `10:17:04.054-07:00`, VM TERMINATED, arrêt ciblé certifié ; clé privée supprimée, clé OS Login retirée, réserve libérée. Trois groupes clos, non chevauchants, sans descendant résiduel tué ni flux tronqué : matrice 152,227 s, complément 11,695 s, banc 1,407 s, tous code 0. Worker 0, session completed. L'isolation des configurations internes reste une limite distincte des fermetures entre commandes ; aucune mesure de performance isolée n'est certifiée ici.

## Portes et oracles

La [matrice principale](copies/morsehgp3D_v11/receipts/index_20261002/index1/matrix.json) passe 1 149/1 149 sélections : Release B18 251 ; ASan/UBSan B24, TSan B21, bits21 et bits24 176 chacun ; poison B21 177 ; mutants 15 ; style 2. Clang absent. Le [complément](copies/morsehgp3D_v11/receipts/index_20261002/index1/asan18.json) passe séparément **36/36 = 17 num +17 index +2 style**. Il ne s'ajoute pas au compteur principal et ne couvre pas tout le moteur.

Caches, options de compilation et provenance sont raccordés aux profils et aux hashes enregistrés des binaires. Les trois exécutables du banc sont ceux de Release B18/bits21/bits24, sans sanitizer, TSan ou poison. Le complément confirme modules num;index, B18, ASan/UBSan activé, cibles et flags correspondants. Les binaires ne sont pas archivés : leurs hashes enregistrés sont vérifiés comme liens de provenance, pas rehachés après arrêt.

Les portes Fraction normal/−O jouent notamment : index 1 010 requêtes et 36 020 contrôles par oracle (302 complets, 702 saturés, six refus, 47 permutations) ; bornes 391 cas/3 400 contrôles (354 valides, 34 dégénérés, trois refus) ; géométrie num 11 838 contrôles. Les lignes natives et planchers sont contrôlés par le [lecteur figé](copies/morsehgp3D_v11/receipts/index_20261002/check.py). L'[oracle index](sources/morsehgp3D_v11/tests/index/fraction_model.py) résout Gram/Gauss en Fraction puis scanne tous les sites, sans reprendre l'arbre ni les formules de bornage.

Les manifestes du paquet et les sections LastTest complètes recoupent 131 mutants : core78, num20, cloud16, catalogue9, index8. Causes : **126 code, trois ligne, deux constructions attendues core**, zéro signal/délai. Les huit nouveaux mutants index et quatre nouveaux num sont détectés par le juge. Les neuf overrides numériques B21/B24 sont explicites ; la configuration mutants de base est B18. Le lecteur vérifie le préfixe JUnit lorsqu'il est tronqué et rattache la section LastTest au bon test/statut, sans confondre une absence de lancement et une mort causale.

## Banc : portée et coûts

Le [rapport index](copies/morsehgp3D_v11/receipts/index_20261002/index1/index.json) contient 18/18 succès, zéro omission ou délai, six comparaisons inter-profils égales. **1 152 requêtes =18×64**, zéro dégénérescence dans ce banc réel. Chaque processus choisit 16 supports par arité q1..4, locaux ou dispersés dans Morton, avec seuils 5/10/13. Les refus et dégénérescences des portes Fraction restent des preuves distinctes. Ces 64 requêtes artificielles ne sont pas les descentes d'une tour et ne bornent pas le nombre de requêtes FULL.

Chaque scan témoin parcourt le Cloud entier et compare populations complètes ou K témoins stricts distincts ; il réutilise **num::side qualifié**. Il est indépendant du parcours d'index, pas de l'arithmétique. Le décodeur vérifie supports/paramètres, statut, IDs SiteIdx croissants et disjoints, populations, réservations et comptes des deux passes. Les sorties canoniques ont été supprimées sur VM après décodage : leurs hashes enregistrés sont comparés, sans relecture indépendante des payloads par cet audit.

Les six entrées ont les mêmes XYZ/IDs quantifiés u18 à 1 mm dans les trois profils compilés. B18/21/24 change l'arithmétique, pas la précision physique. Les trois LiDAR sans sol sont de la seule séquence 08. Le banc exige des poids unitaires ; l'index conserve les poids mais son census compte les sites distincts, sans développement de multiplicité. Aucune qualification FULL pondérée n'en découle.

Une répétition par entrée/profil. La somme census exclut construction Sphere, lecture/Cloud, construction d'index, scan témoin et sérialisation ; les factories sont chronométrées séparément, y compris pour les dégénérescences. Le processus inclut ces phases et les E/S natives ; le décodage Python vient ensuite. Le ledger additionne les deux parcours du census. Les pics concernent Buffer avec Cloud/index/résultat encore vivants, ni RSS ni tas Python.

| Entrée entière, B21 | Sites | Index (ms) | 64 census (ms) | Scan référence (ms) | Factories (ms) | Processus (ms) |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| uniform_u18_n8000 | 8000 | 0.045830 | 1.138290 | 13.277678 | 0.005250 | 15.956647 |
| uniform_u18_n16000 | 16000 | 0.089580 | 1.304940 | 26.494006 | 0.005390 | 29.443166 |
| uniform_u18_n32000 | 32000 | 0.173530 | 1.324300 | 52.820972 | 0.008840 | 56.378842 |
| lidar_ng01 | 35551 | 0.340740 | 0.620720 | 58.559432 | 0.004560 | 61.654331 |
| lidar_ng00 | 39885 | 0.400280 | 0.810420 | 65.449930 | 0.004180 | 68.963150 |
| lidar_ng02 | 45845 | 0.418410 | 0.702530 | 74.821059 | 0.005440 | 78.374499 |

Ces temps ne sont pas des chronos catalogue/FULL. La [sonde indépendante](review.py) conserve les phases exactes, hashes, populations, mémoire et durées de chacun des 18 essais : [verdict et mesures](review_normal.stdout). Aucune extrapolation aux dizaines de millions ou gain stable entre profils n'est réalisée.

## Protocole et replays

Le [plan actif e852](sources/morsehgp3D_v11/bench/plans/index_g4.json) lie trois commandes distinctes : matrice600, complément150, banc800 secondes ; budget interne550/130 et calendrier natif18×30=540 s. Il lie le banc aux deux qualifications et aux binaires hachés, conserve les préfixes interrompus et tente les 18 unités sans omission causale arbitraire. La qualification G4 en préparation dans les documents Git e852 décrit l'état avant cette campagne ; la publication 356 close les pièces sans changer la source exécutée.

[Huit replays finaux](review_runs.json), tous code 0 et stdout identiques normal/−O : lecteur développeur, selftest figé **18 témoins/58 corruptions**, collecteur **11 essais/17 corruptions/20 contrôles de provenance/6 schedules, native0**, sonde indépendante. Le [wrapper autonome](run_reader.py) redirige seulement le chemin du reçu original vers sa copie locale ; il conserve le juge. Le contrôle IO natif conservé dans les portes G4 n'est pas relancé localement. Aucun produit natif, build ou cloud n'a été exécuté par cet audit.

Une erreur initiale de notre juge est conservée : [code](review_pre_supplement_assumption.py), [commandes et sorties](review_runs_pre_supplement_assumption.json). Son attribution supposait à tort 15 num/19 index dans le complément ; les deux portes num_bounds_model normal/−O donnent réellement 17/17. Le juge final utilise l'inventaire sélectionné, sans transformer cet échec de contrôle en défaut produit.

## Fermeture de la capsule

LEDGER.json couvre tous les payloads, dont le manifeste **imbriqué** raw/package/data/SHA256SUMS. SHA256SUMS couvre les payloads et le ledger ; seul le manifeste SHA256SUMS **racine** est exclu de sa propre liste. Les inventaires sont comparés à l'ensemble effectif des fichiers. Aucune capsule antérieure ni note active n'a été modifiée.
